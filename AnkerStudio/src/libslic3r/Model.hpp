#ifndef slic3r_Model_hpp_
#define slic3r_Model_hpp_

#include "libslic3r.h"
#include "enum_bitmask.hpp"
#include "Geometry.hpp"
#include "ObjectID.hpp"
#include "Point.hpp"
#include "PrintConfig.hpp"
#include "Slicing.hpp"
#include "SLA/SupportPoint.hpp"
#include "SLA/Hollowing.hpp"
#include "TriangleMesh.hpp"
#include "Arrange.hpp"
#include "CustomGCode.hpp"
#include "enum_bitmask.hpp"
#include "TextConfiguration.hpp"
#include "EmbossShape.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>

namespace cereal {
	class BinaryInputArchive;
	class BinaryOutputArchive;
	template <class T> void load_optional(BinaryInputArchive &ar, std::shared_ptr<const T> &ptr);
	template <class T> void save_optional(BinaryOutputArchive &ar, const std::shared_ptr<const T> &ptr);
	template <class T> void load_by_value(BinaryInputArchive &ar, T &obj);
	template <class T> void save_by_value(BinaryOutputArchive &ar, const T &obj);
}

namespace Slic3r {
enum class ConversionType;

class BuildVolume;
class Model;
class ModelInstance;
class ModelMaterial;
class ModelObject;
class ModelVolume;
class ModelWipeTower;
class Print;
class SLAPrint;
class TriangleSelector;

namespace UndoRedo {
	class StackImpl;
}

class ModelConfigObject : public ObjectBase, public ModelConfig
{
private:
	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	friend class ModelObject;
	friend class ModelVolume;
	friend class ModelMaterial;

    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit ModelConfigObject() = default;
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit ModelConfigObject(int) : ObjectBase(-1) {}
    // Copy constructor copies the ID.
	explicit ModelConfigObject(const ModelConfigObject &cfg) = default;
    // Move constructor copies the ID.
	explicit ModelConfigObject(ModelConfigObject &&cfg) = default;

    Timestamp          timestamp() const throw() override { return this->ModelConfig::timestamp(); }
    bool               object_id_and_timestamp_match(const ModelConfigObject &rhs) const throw() { return this->id() == rhs.id() && this->timestamp() == rhs.timestamp(); }

    // called by ModelObject::assign_copy()
	ModelConfigObject& operator=(const ModelConfigObject &rhs) = default;
    ModelConfigObject& operator=(ModelConfigObject &&rhs) = default;

    template<class Archive> void serialize(Archive &ar) {
        ar(cereal::base_class<ModelConfig>(this));
    }
};

namespace Internal {
	template<typename T>
	class StaticSerializationWrapper
	{
	public:
		StaticSerializationWrapper(T &wrap) : wrapped(wrap) {}
	private:
		friend class cereal::access;
		friend class UndoRedo::StackImpl;
		template<class Archive> void load(Archive &ar) { cereal::load_by_value(ar, wrapped); }
		template<class Archive> void save(Archive &ar) const { cereal::save_by_value(ar, wrapped); }
		T&	wrapped;
	};
}

typedef std::string t_model_material_id;
typedef std::string t_model_material_attribute;
typedef std::map<t_model_material_attribute, std::string> t_model_material_attributes;

typedef std::map<t_model_material_id, ModelMaterial*> ModelMaterialMap;
typedef std::vector<ModelObject*> ModelObjectPtrs;
typedef std::vector<ModelVolume*> ModelVolumePtrs;
typedef std::vector<ModelInstance*> ModelInstancePtrs;

#define OBJECTBASE_DERIVED_COPY_MOVE_CLONE(TYPE) \
    /* Copy a model, copy the IDs. The Print::apply() will call the TYPE::copy() method */ \
    /* to make a private copy for background processing. */ \
    static TYPE* new_copy(const TYPE &rhs)  { auto *ret = new TYPE(rhs); assert(ret->id() == rhs.id()); return ret; } \
    static TYPE* new_copy(TYPE &&rhs)       { auto *ret = new TYPE(std::move(rhs)); assert(ret->id() == rhs.id()); return ret; } \
    static TYPE  make_copy(const TYPE &rhs) { TYPE ret(rhs); assert(ret.id() == rhs.id()); return ret; } \
    static TYPE  make_copy(TYPE &&rhs)      { TYPE ret(std::move(rhs)); assert(ret.id() == rhs.id()); return ret; } \
    TYPE&        assign_copy(const TYPE &rhs); \
    TYPE&        assign_copy(TYPE &&rhs); \
    /* Copy a TYPE, generate new IDs. The front end will use this call. */ \
    static TYPE* new_clone(const TYPE &rhs) { \
        /* Default constructor assigning an invalid ID. */ \
        auto obj = new TYPE(-1); \
        obj->assign_clone(rhs); \
        assert(obj->id().valid() && obj->id() != rhs.id()); \
        return obj; \
	} \
    TYPE         make_clone(const TYPE &rhs) { \
        /* Default constructor assigning an invalid ID. */ \
        TYPE obj(-1); \
        obj.assign_clone(rhs); \
        assert(obj.id().valid() && obj.id() != rhs.id()); \
        return obj; \
    } \
    TYPE&        assign_clone(const TYPE &rhs) { \
        this->assign_copy(rhs); \
        assert(this->id().valid() && this->id() == rhs.id()); \
        this->assign_new_unique_ids_recursive(); \
        assert(this->id().valid() && this->id() != rhs.id()); \
		return *this; \
    }

// Material, which may be shared across multiple ModelObjects of a single Model.
class ModelMaterial final : public ObjectBase
{
public:
    // Attributes are defined by the AMF file format, but they don't seem to be used by Slic3r for any purpose.
    t_model_material_attributes attributes;
    // Dynamic configuration storage for the object specific configuration values, overriding the global configuration.
    ModelConfigObject config;

    Model* get_model() const { return m_model; }
    void apply(const t_model_material_attributes &attributes)
        { this->attributes.insert(attributes.begin(), attributes.end()); }

private:
    // Parent, owning this material.
    Model *m_model;

    // To be accessed by the Model.
    friend class Model;
	// Constructor, which assigns a new unique ID to the material and to its config.
	ModelMaterial(Model *model) : m_model(model) { assert(this->id().valid()); }
	// Copy constructor copies the IDs of the ModelMaterial and its config, and m_model!
	ModelMaterial(const ModelMaterial &rhs) = default;
	void set_model(Model *model) { m_model = model; }
	void set_new_unique_id() { ObjectBase::set_new_unique_id(); this->config.set_new_unique_id(); }

	// To be accessed by the serialization and Undo/Redo code.
	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Create an object for deserialization, don't allocate IDs for ModelMaterial and its config.
	ModelMaterial() : ObjectBase(-1), config(-1) { assert(this->id().invalid()); assert(this->config.id().invalid()); }
	template<class Archive> void serialize(Archive &ar) { 
		assert(this->id().invalid()); assert(this->config.id().invalid());
		Internal::StaticSerializationWrapper<ModelConfigObject> config_wrapper(config);
		ar(attributes, config_wrapper);
		// assert(this->id().valid()); assert(this->config.id().valid());
	}

	// Disabled methods.
	ModelMaterial(ModelMaterial &&rhs) = delete;
	ModelMaterial& operator=(const ModelMaterial &rhs) = delete;
    ModelMaterial& operator=(ModelMaterial &&rhs) = delete;
};

class LayerHeightProfile final : public ObjectWithTimestamp {
public:
    // Assign the content if the timestamp differs, don't assign an ObjectID.
    void assign(const LayerHeightProfile &rhs) { if (! this->timestamp_matches(rhs)) { m_data = rhs.m_data; this->copy_timestamp(rhs); } }
    void assign(LayerHeightProfile &&rhs) { if (! this->timestamp_matches(rhs)) { m_data = std::move(rhs.m_data); this->copy_timestamp(rhs); } }

    std::vector<coordf_t> get() const throw() { return m_data; }
    bool                  empty() const throw() { return m_data.empty(); }
    void                  set(const std::vector<coordf_t> &data) { if (m_data != data) { m_data = data; this->touch(); } }
    void                  set(std::vector<coordf_t> &&data) { if (m_data != data) { m_data = std::move(data); this->touch(); } }
    void                  clear() { m_data.clear(); this->touch(); }

    template<class Archive> void serialize(Archive &ar)
    {
        ar(cereal::base_class<ObjectWithTimestamp>(this), m_data);
    }

private:
    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit LayerHeightProfile() = default;
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit LayerHeightProfile(int) : ObjectWithTimestamp(-1) {}
    // Copy constructor copies the ID.
    explicit LayerHeightProfile(const LayerHeightProfile &rhs) = default;
    // Move constructor copies the ID.
    explicit LayerHeightProfile(LayerHeightProfile &&rhs) = default;

    // called by ModelObject::assign_copy()
    LayerHeightProfile& operator=(const LayerHeightProfile &rhs) = default;
    LayerHeightProfile& operator=(LayerHeightProfile &&rhs) = default;

    std::vector<coordf_t> m_data;

    // to access set_new_unique_id() when copy / pasting an object
    friend class ModelObject;
};

enum class CutConnectorType : int {
    Plug
    , Dowel
    , Snap
    , Undef
};

enum class CutConnectorStyle : int {
    Prism
    , Frustum
    , Undef
    //,Claw
};

enum class CutConnectorShape : int {
    Triangle
    , Square
    , Hexagon
    , Circle
    , Undef
    //,D-shape
};

struct CutConnectorAttributes
{
    CutConnectorType    type{ CutConnectorType::Plug };
    CutConnectorStyle   style{ CutConnectorStyle::Prism };
    CutConnectorShape   shape{ CutConnectorShape::Circle };

    CutConnectorAttributes() {}

    CutConnectorAttributes(CutConnectorType t, CutConnectorStyle st, CutConnectorShape sh)
        : type(t), style(st), shape(sh)
    {}

    CutConnectorAttributes(const CutConnectorAttributes& rhs) :
        CutConnectorAttributes(rhs.type, rhs.style, rhs.shape) {}

    bool operator==(const CutConnectorAttributes& other) const;

    bool operator!=(const CutConnectorAttributes& other) const { return !(other == (*this)); }

    bool operator<(const CutConnectorAttributes& other) const {
        return   this->type <  other.type ||
                (this->type == other.type && this->style <  other.style) ||
                (this->type == other.type && this->style == other.style && this->shape < other.shape);
    }

    template<class Archive> inline void serialize(Archive& ar) {
        ar(type, style, shape);
    }
};

struct CutConnector
{
    Vec3d pos;
    Transform3d rotation_m;
    float radius;
    float height;
    float radius_tolerance;// [0.f : 1.f]
    float height_tolerance;// [0.f : 1.f]
    float z_angle{ 0.f };
    CutConnectorAttributes attribs;

    CutConnector()
        : pos(Vec3d::Zero()), rotation_m(Transform3d::Identity()), radius(5.f), height(10.f), radius_tolerance(0.f), height_tolerance(0.1f), z_angle(0.f)
    {}

    CutConnector(Vec3d p, Transform3d rot, float r, float h, float rt, float ht, float za, CutConnectorAttributes attributes)
        : pos(p), rotation_m(rot), radius(r), height(h), radius_tolerance(rt), height_tolerance(ht), z_angle(za), attribs(attributes)
    {}

    CutConnector(const CutConnector& rhs) :
        CutConnector(rhs.pos, rhs.rotation_m, rhs.radius, rhs.height, rhs.radius_tolerance, rhs.height_tolerance, rhs.z_angle, rhs.attribs) {}

    bool operator==(const CutConnector& other) const;

    bool operator!=(const CutConnector& other) const { return !(other == (*this)); }

    template<class Archive> inline void serialize(Archive& ar) {
        ar(pos, rotation_m, radius, height, radius_tolerance, height_tolerance, z_angle, attribs);
    }
};

using CutConnectors = std::vector<CutConnector>;


// Declared outside of ModelVolume, so it could be forward declared.
enum class ModelVolumeType : int {
    INVALID = -1,
    MODEL_PART = 0,
    NEGATIVE_VOLUME,
    PARAMETER_MODIFIER,
    SUPPORT_BLOCKER,
    SUPPORT_ENFORCER,
};

enum class ModelObjectCutAttribute : int { KeepUpper, KeepLower, KeepAsParts, FlipUpper, FlipLower, PlaceOnCutUpper, PlaceOnCutLower, CreateDowels, InvalidateCutInfo };
using ModelObjectCutAttributes = enum_bitmask<ModelObjectCutAttribute>;
ENABLE_ENUM_BITMASK_OPERATORS(ModelObjectCutAttribute);

// A printable object, possibly having multiple print volumes (each with its own set of parameters and materials),
// and possibly having multiple modifier volumes, each modifier volume with its set of parameters and materials.
// Each ModelObject may be instantiated mutliple times, each instance having different placement on the print bed,
// different rotation and different uniform scaling.
class ModelObject final : public ObjectBase
{
public:
    std::string             name;
    std::string             input_file;    // XXX: consider fs::path
    // Instances of this ModelObject. Each instance defines a shift on the print bed, rotation around the Z axis and a uniform scaling.
    // Instances are owned by this ModelObject.
    ModelInstancePtrs       instances;
    // Printable and modifier volumes, each with its material ID and a set of override parameters.
    // ModelVolumes are owned by this ModelObject.
    ModelVolumePtrs         volumes;
    // Configuration parameters specific to a single ModelObject, overriding the global Slic3r settings.
    ModelConfigObject 		config;
    // Variation of a layer thickness for spans of Z coordinates + optional parameter overrides.
    t_layer_config_ranges   layer_config_ranges;
    // Profile of increasing z to a layer height, to be linearly interpolated when calculating the layers.
    // The pairs of <z, layer_height> are packed into a 1D array.
    LayerHeightProfile      layer_height_profile;
    // Whether or not this object is printable
    bool                    printable { true };

    // This vector holds position of selected support points for SLA. The data are
    // saved in mesh coordinates to allow using them for several instances.
    // The format is (x, y, z, point_size, supports_island)
    sla::SupportPoints      sla_support_points;
    // To keep track of where the points came from (used for synchronization between
    // the SLA gizmo and the backend).
    sla::PointsStatus       sla_points_status = sla::PointsStatus::NoPoints;

    // Holes to be drilled into the object so resin can flow out
    sla::DrainHoles         sla_drain_holes;

    // Connectors to be added into the object before cut and are used to create a solid/negative volumes during a cut perform
    CutConnectors           cut_connectors;
    CutObjectBase           cut_id;

    /* This vector accumulates the total translation applied to the object by the
        center_around_origin() method. Callers might want to apply the same translation
        to new volumes before adding them to this object in order to preserve alignment
        when user expects that. */
    Vec3d                   origin_translation;

    Model*                  get_model() { return m_model; }
    const Model*            get_model() const { return m_model; }

    ModelVolume*            add_volume(const TriangleMesh &mesh);
    ModelVolume*            add_volume(TriangleMesh &&mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART);
    ModelVolume*            add_volume(const ModelVolume &volume, ModelVolumeType type = ModelVolumeType::INVALID);
    ModelVolume*            add_volume(const ModelVolume &volume, TriangleMesh &&mesh);
    ModelVolume*            add_volume_with_shared_mesh(const ModelVolume& other, ModelVolumeType type = ModelVolumeType::MODEL_PART);
    void                    delete_volume(size_t idx);
    void                    clear_volumes();
    void                    sort_volumes(bool full_sort);
    bool                    is_multiparts() const { return volumes.size() > 1; }
    // Checks if any of object volume is painted using the fdm support painting gizmo.
    bool                    is_fdm_support_painted() const;
    // Checks if any of object volume is painted using the seam painting gizmo.
    bool                    is_seam_painted() const;
    // Checks if any of object volume is painted using the multi-material painting gizmo.
    bool                    is_mm_painted() const;
    // Checks if object contains just one volume and it's a text
    bool                    is_text() const;
    // This object may have a varying layer height by painting or by a table.
    // Even if true is returned, the layer height profile may be "flat" with no difference to default layering.
    bool                    has_custom_layering() const 
        { return ! this->layer_config_ranges.empty() || ! this->layer_height_profile.empty(); }

    ModelInstance*          add_instance();
    ModelInstance*          add_instance(const ModelInstance &instance);
    ModelInstance*          add_instance(const Vec3d &offset, const Vec3d &scaling_factor, const Vec3d &rotation, const Vec3d &mirror);
    void                    delete_instance(size_t idx);
    void                    delete_last_instance();
    void                    clear_instances();

    // Returns the bounding box of the transformed instances. This bounding box is approximate and not snug, it is being cached.
    const BoundingBoxf3&    bounding_box_approx() const;
    // Returns an exact bounding box of the transformed instances. The result it is being cached.
    const BoundingBoxf3&    bounding_box_exact() const;
    // Return minimum / maximum of a printable object transformed into the world coordinate system.
    // All instances share the same min / max Z.
    double                  min_z() const;
    double                  max_z() const;

    void invalidate_bounding_box() { 
        m_bounding_box_approx_valid     = false;
        m_bounding_box_exact_valid      = false;
        m_min_max_z_valid               = false;
        m_raw_bounding_box_valid        = false;
        m_raw_mesh_bounding_box_valid   = false;
    }

    // A mesh containing all transformed instances of this object.
    TriangleMesh mesh() const;
    // Non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
    // Currently used by ModelObject::mesh() and to calculate the 2D envelope for 2D plater.
    TriangleMesh raw_mesh() const;
    // The same as above, but producing a lightweight indexed_triangle_set.
    indexed_triangle_set raw_indexed_triangle_set() const;
    // A transformed snug bounding box around the non-modifier object volumes, without the translation applied.
    // This bounding box is only used for the actual slicing.
    const BoundingBoxf3& raw_bounding_box() const;
    // A snug bounding box around the transformed non-modifier object volumes.
    BoundingBoxf3 instance_bounding_box(size_t instance_idx, bool dont_translate = false) const;
	// A snug bounding box of non-transformed (non-rotated, non-scaled, non-translated) sum of non-modifier object volumes.
	const BoundingBoxf3& raw_mesh_bounding_box() const;
	// A snug bounding box of non-transformed (non-rotated, non-scaled, non-translated) sum of all object volumes.
    BoundingBoxf3 full_raw_mesh_bounding_box() const;

    // Calculate 2D convex hull of of a projection of the transformed printable volumes into the XY plane.
    // This method is cheap in that it does not make any unnecessary copy of the volume meshes.
    // This method is used by the auto arrange function.
    Polygon       convex_hull_2d(const Transform3d &trafo_instance) const;

    void center_around_origin(bool include_modifiers = true);
    void ensure_on_bed(bool allow_negative_z = false);

    void translate_instances(const Vec3d& vector);
    void translate_instance(size_t instance_idx, const Vec3d& vector);
    void translate(const Vec3d &vector) { this->translate(vector(0), vector(1), vector(2)); }
    void translate(double x, double y, double z);
    void scale(const Vec3d &versor);
    void scale(const double s) { this->scale(Vec3d(s, s, s)); }
    void scale(double x, double y, double z) { this->scale(Vec3d(x, y, z)); }
    /// Scale the current ModelObject to fit by altering the scaling factor of ModelInstances.
    /// It operates on the total size by duplicating the object according to all the instances.
    /// \param size Sizef3 the size vector
    void scale_to_fit(const Vec3d &size);
    void rotate(double angle, Axis axis);
    void rotate(double angle, const Vec3d& axis);
    void mirror(Axis axis);

    // This method could only be called before the meshes of this ModelVolumes are not shared!
    void scale_mesh_after_creation(const float scale);
    void convert_units(ModelObjectPtrs&new_objects, ConversionType conv_type, std::vector<int> volume_idxs);

    size_t materials_count() const;
    size_t facets_count() const;
    size_t parts_count() const;
    static indexed_triangle_set get_connector_mesh(CutConnectorAttributes connector_attributes);
    void apply_cut_connectors(const std::string& name);
    // invalidate cut state for this object and its connectors/volumes
    void invalidate_cut();
    // delete volumes which are marked as connector for this object
    void delete_connectors();
    void synchronize_model_after_cut();
    void apply_cut_attributes(ModelObjectCutAttributes attributes);
    void clone_for_cut(ModelObject **obj);
    void process_connector_cut(ModelVolume* volume, const Transform3d& instance_matrix, const Transform3d& cut_matrix,
                               ModelObjectCutAttributes attributes, ModelObject* upper, ModelObject* lower,
                               std::vector<ModelObject*>& dowels, Vec3d& local_dowels_displace);
    void process_modifier_cut(ModelVolume* volume, const Transform3d& instance_matrix, const Transform3d& inverse_cut_matrix,
                              ModelObjectCutAttributes attributes, ModelObject* upper, ModelObject* lower);
    void process_volume_cut(ModelVolume* volume, const Transform3d& instance_matrix, const Transform3d& cut_matrix,
                            ModelObjectCutAttributes attributes, TriangleMesh& upper_mesh, TriangleMesh& lower_mesh);
    void process_solid_part_cut(ModelVolume* volume, const Transform3d& instance_matrix, const Transform3d& cut_matrix,
                                ModelObjectCutAttributes attributes, ModelObject* upper, ModelObject* lower, Vec3d& local_displace);
    ModelObjectPtrs cut(size_t instance, const Transform3d&cut_matrix, ModelObjectCutAttributes attributes);
    void split(ModelObjectPtrs*new_objects);
    void merge();
    // Support for non-uniform scaling of instances. If an instance is rotated by angles, which are not multiples of ninety degrees,
    // then the scaling in world coordinate system is not representable by the Geometry::Transformation structure.
    // This situation is solved by baking in the instance transformation into the mesh vertices.
    // Rotation and mirroring is being baked in. In case the instance scaling was non-uniform, it is baked in as well.
    void bake_xy_rotation_into_meshes(size_t instance_idx);

    double get_instance_min_z(size_t instance_idx) const;
    double get_instance_max_z(size_t instance_idx) const;

    // Print object statistics to console.
    void print_info() const;

    std::string get_export_filename() const;

    // Get full stl statistics for all object's meshes
    TriangleMeshStats get_object_stl_stats() const;
    // Get count of errors in the mesh( or all object's meshes, if volume index isn't defined)
    int         get_repaired_errors_count(const int vol_idx = -1) const;

    // Detect if object has at least one solid mash
    bool has_solid_mesh() const;
    // Detect if object has at least one negative volume mash
    bool has_negative_volume_mesh() const;
    // Detect if object has at least one sla drain hole
    bool has_sla_drain_holes() const { return !sla_drain_holes.empty(); }
    bool is_cut() const { return cut_id.id().valid(); }
    bool has_connectors() const;

private:
    friend class Model;
    // This constructor assigns new ID to this ModelObject and its config.
    explicit ModelObject(Model* model) : m_model(model), origin_translation(Vec3d::Zero())
    { 
        assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
    }
    explicit ModelObject(int) : ObjectBase(-1), config(-1), layer_height_profile(-1), origin_translation(Vec3d::Zero())
    { 
        assert(this->id().invalid()); 
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
    }
	~ModelObject();
	void assign_new_unique_ids_recursive() override;

    // To be able to return an object from own copy / clone methods. Hopefully the compiler will do the "Copy elision"
    // (Omits copy and move(since C++11) constructors, resulting in zero - copy pass - by - value semantics).
    ModelObject(const ModelObject &rhs) : ObjectBase(-1), config(-1), layer_height_profile(-1), m_model(rhs.m_model) { 
    	assert(this->id().invalid()); 
        assert(this->config.id().invalid()); 
        assert(this->layer_height_profile.id().invalid());
        assert(rhs.id() != rhs.config.id());
        assert(rhs.id() != rhs.layer_height_profile.id());
    	this->assign_copy(rhs);
    	assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->layer_height_profile.id().valid()); 
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id()); 
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    }
    explicit ModelObject(ModelObject &&rhs) : ObjectBase(-1), config(-1), layer_height_profile(-1) { 
    	assert(this->id().invalid()); 
        assert(this->config.id().invalid()); 
        assert(this->layer_height_profile.id().invalid());
        assert(rhs.id() != rhs.config.id());
        assert(rhs.id() != rhs.layer_height_profile.id());
    	this->assign_copy(std::move(rhs));
    	assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    }
    ModelObject& operator=(const ModelObject &rhs) {
    	this->assign_copy(rhs); 
    	m_model = rhs.m_model;
    	assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id()); 
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    	return *this;
    }
    ModelObject& operator=(ModelObject &&rhs) {
    	this->assign_copy(std::move(rhs)); 
    	m_model = rhs.m_model;
    	assert(this->id().valid()); 
        assert(this->config.id().valid());
        assert(this->layer_height_profile.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->layer_height_profile.id());
    	assert(this->id() == rhs.id());
        assert(this->config.id() == rhs.config.id());
        assert(this->layer_height_profile.id() == rhs.layer_height_profile.id());
    	return *this;
    }
	void set_new_unique_id() { 
        ObjectBase::set_new_unique_id(); 
        this->config.set_new_unique_id();
        this->layer_height_profile.set_new_unique_id();
    }

    OBJECTBASE_DERIVED_COPY_MOVE_CLONE(ModelObject)

    // Parent object, owning this ModelObject. Set to nullptr here, so the macros above will have it initialized.
    Model                *m_model { nullptr };

    // Bounding box, cached.
    mutable BoundingBoxf3 m_bounding_box_approx;
    mutable bool          m_bounding_box_approx_valid { false };
    mutable BoundingBoxf3 m_bounding_box_exact;
    mutable bool          m_bounding_box_exact_valid { false };
    mutable bool          m_min_max_z_valid { false };
    mutable BoundingBoxf3 m_raw_bounding_box;
    mutable bool          m_raw_bounding_box_valid { false };
    mutable BoundingBoxf3 m_raw_mesh_bounding_box;
    mutable bool          m_raw_mesh_bounding_box_valid { false };

    // Only use this method if now the source and dest ModelObjects are equal, for example they were synchronized by Print::apply().
    void copy_transformation_caches(const ModelObject &src) {
        m_bounding_box_approx             = src.m_bounding_box_approx;
        m_bounding_box_approx_valid       = src.m_bounding_box_approx_valid;
        m_bounding_box_exact              = src.m_bounding_box_exact;
        m_bounding_box_exact_valid        = src.m_bounding_box_exact_valid;
        m_min_max_z_valid                 = src.m_min_max_z_valid;
        m_raw_bounding_box                = src.m_raw_bounding_box;
        m_raw_bounding_box_valid          = src.m_raw_bounding_box_valid;
        m_raw_mesh_bounding_box           = src.m_raw_mesh_bounding_box;
        m_raw_mesh_bounding_box_valid     = src.m_raw_mesh_bounding_box_valid;
    }

    // Called by Print::apply() to set the model pointer after making a copy.
    friend class Print;
    friend class SLAPrint;
    void        set_model(Model *model) { m_model = model; }

    // Undo / Redo through the cereal serialization library
	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization -> Don't allocate any IDs for the ModelObject or its config.
	ModelObject() : 
        ObjectBase(-1), config(-1), layer_height_profile(-1) {
		assert(this->id().invalid()); 
        assert(this->config.id().invalid());
        assert(this->layer_height_profile.id().invalid());
	}
	template<class Archive> void serialize(Archive &ar) {
		ar(cereal::base_class<ObjectBase>(this));
		Internal::StaticSerializationWrapper<ModelConfigObject> config_wrapper(config);
        Internal::StaticSerializationWrapper<LayerHeightProfile> layer_heigth_profile_wrapper(layer_height_profile);
        ar(name, input_file, instances, volumes, config_wrapper, layer_config_ranges, layer_heigth_profile_wrapper, 
            sla_support_points, sla_points_status, sla_drain_holes, printable, origin_translation,
            m_bounding_box_approx, m_bounding_box_approx_valid, 
            m_bounding_box_exact, m_bounding_box_exact_valid, m_min_max_z_valid,
            m_raw_bounding_box, m_raw_bounding_box_valid, m_raw_mesh_bounding_box, m_raw_mesh_bounding_box_valid,
            cut_connectors, cut_id);
	}

    // Called by Print::validate() from the UI thread.
    unsigned int update_instances_print_volume_state(const BuildVolume &build_volume);

    // Called by min_z(), max_z()
    void update_min_max_z();
};

enum class EnforcerBlockerType : int8_t {
    // Maximum is 3. The value is serialized in TriangleSelector into 2 bits.
    NONE      = 0,
    ENFORCER  = 1,
    BLOCKER   = 2,
    // Maximum is 15. The value is serialized in TriangleSelector into 6 bits using a 2 bit prefix code.
    Extruder1 = ENFORCER,
    Extruder2 = BLOCKER,
    Extruder3,
    Extruder4,
    Extruder5,
    Extruder6,
    Extruder7,
    Extruder8,
    Extruder9,
    Extruder10,
    Extruder11,
    Extruder12,
    Extruder13,
    Extruder14,
    Extruder15,
};

enum class ConversionType : int {
    CONV_TO_INCH,
    CONV_FROM_INCH,
    CONV_TO_METER,
    CONV_FROM_METER,
};

class FacetsAnnotation final : public ObjectWithTimestamp {
public:
    // Assign the content if the timestamp differs, don't assign an ObjectID.
    void assign(const FacetsAnnotation& rhs) { if (! this->timestamp_matches(rhs)) { m_data = rhs.m_data; this->copy_timestamp(rhs); } }
    void assign(FacetsAnnotation&& rhs) { if (! this->timestamp_matches(rhs)) { m_data = std::move(rhs.m_data); this->copy_timestamp(rhs); } }
    const std::pair<std::vector<std::pair<int, int>>, std::vector<bool>>& get_data() const throw() { return m_data; }
    bool set(const TriangleSelector& selector);
    indexed_triangle_set get_facets(const ModelVolume& mv, EnforcerBlockerType type) const;
    indexed_triangle_set get_facets_strict(const ModelVolume& mv, EnforcerBlockerType type) const;
    bool has_facets(const ModelVolume& mv, EnforcerBlockerType type) const;
    bool empty() const { return m_data.first.empty(); }

    // Following method clears the config and increases its timestamp, so the deleted
    // state is considered changed from perspective of the undo/redo stack.
    void reset();

    // Serialize triangle into string, for serialization into 3MF/AMF.
    std::string get_triangle_as_string(int i) const;

    // Before deserialization, reserve space for n_triangles.
    void reserve(int n_triangles) { m_data.first.reserve(n_triangles); }
    // Deserialize triangles one by one, with strictly increasing triangle_id.
    void set_triangle_from_string(int triangle_id, const std::string& str);
    // After deserializing the last triangle, shrink data to fit.
    void shrink_to_fit() { m_data.first.shrink_to_fit(); m_data.second.shrink_to_fit(); }

private:
    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit FacetsAnnotation() = default;
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit FacetsAnnotation(int) : ObjectWithTimestamp(-1) {}
    // Copy constructor copies the ID.
    explicit FacetsAnnotation(const FacetsAnnotation &rhs) = default;
    // Move constructor copies the ID.
    explicit FacetsAnnotation(FacetsAnnotation &&rhs) = default;

    // called by ModelVolume::assign_copy()
    FacetsAnnotation& operator=(const FacetsAnnotation &rhs) = default;
    FacetsAnnotation& operator=(FacetsAnnotation &&rhs) = default;

    friend class cereal::access;
    friend class UndoRedo::StackImpl;

    template<class Archive> void serialize(Archive &ar)
    {
        ar(cereal::base_class<ObjectWithTimestamp>(this), m_data);
    }

    std::pair<std::vector<std::pair<int, int>>, std::vector<bool>> m_data;

    // To access set_new_unique_id() when copy / pasting a ModelVolume.
    friend class ModelVolume;
};

// An object STL, or a modifier volume, over which a different set of parameters shall be applied.
// ModelVolume instances are owned by a ModelObject.
class ModelVolume final : public ObjectBase
{
public:
    std::string         name;
    // struct used by reload from disk command to recover data from disk
    struct Source
    {
        std::string input_file;
        int object_idx{ -1 };
        int volume_idx{ -1 };
        Vec3d mesh_offset{ Vec3d::Zero() };
        Geometry::Transformation transform;
        bool is_converted_from_inches{ false };
        bool is_converted_from_meters{ false };
        bool is_from_builtin_objects{ false };

        template<class Archive> void serialize(Archive& ar) { 
            //FIXME Vojtech: Serialize / deserialize only if the Source is set.
            // likely testing input_file or object_idx would be sufficient.
            ar(input_file, object_idx, volume_idx, mesh_offset, transform, is_converted_from_inches, is_converted_from_meters, is_from_builtin_objects);
        }
    };
    Source              source;

    // struct used by cut command 
    // It contains information about connectors
    struct CutInfo
    {
        bool                is_from_upper{ true };
        bool                is_connector{ false };
        bool                is_processed{ true };
        CutConnectorType    connector_type{ CutConnectorType::Plug };
        float               radius_tolerance{ 0.f };// [0.f : 1.f]
        float               height_tolerance{ 0.f };// [0.f : 1.f]

        CutInfo() = default;
        CutInfo(CutConnectorType type, float rad_tolerance, float h_tolerance, bool processed = false) :
        is_connector(true),
        is_processed(processed),
        connector_type(type),
        radius_tolerance(rad_tolerance),
        height_tolerance(h_tolerance)
        {}

        void set_processed() { is_processed = true; }
        void invalidate()    { is_connector = false; }
        void reset_from_upper() { is_from_upper = true; }

        template<class Archive> inline void serialize(Archive& ar) {
            ar(is_connector, is_processed, connector_type, radius_tolerance, height_tolerance);
        }
    };
    CutInfo             cut_info;

    bool                is_from_upper() const { return cut_info.is_from_upper; }
    void                reset_from_upper() { cut_info.reset_from_upper(); }

    bool                is_cut_connector() const { return cut_info.is_processed && cut_info.is_connector; }
    void                invalidate_cut_info()    { cut_info.invalidate(); }

    // The triangular model.
    const TriangleMesh& mesh() const { return *m_mesh.get(); }
    std::shared_ptr<const TriangleMesh> mesh_ptr() const { return m_mesh; }
    void                set_mesh(const TriangleMesh &mesh) { m_mesh = std::make_shared<const TriangleMesh>(mesh); }
    void                set_mesh(TriangleMesh &&mesh) { m_mesh = std::make_shared<const TriangleMesh>(std::move(mesh)); }
    void                set_mesh(const indexed_triangle_set &mesh) { m_mesh = std::make_shared<const TriangleMesh>(mesh); }
    void                set_mesh(indexed_triangle_set &&mesh) { m_mesh = std::make_shared<const TriangleMesh>(std::move(mesh)); }
    void                set_mesh(std::shared_ptr<const TriangleMesh> &mesh) { m_mesh = mesh; }
    void                set_mesh(std::unique_ptr<const TriangleMesh> &&mesh) { m_mesh = std::move(mesh); }
	void				reset_mesh() { m_mesh = std::make_shared<const TriangleMesh>(); }
    const std::shared_ptr<const TriangleMesh>& get_mesh_shared_ptr() const { return m_mesh; }
    // Configuration parameters specific to an object model geometry or a modifier volume, 
    // overriding the global Slic3r settings and the ModelObject settings.
    ModelConfigObject	config;

    // List of mesh facets to be supported/unsupported.
    FacetsAnnotation    supported_facets;

    // List of seam enforcers/blockers.
    FacetsAnnotation    seam_facets;

    // List of mesh facets painted for MMU segmentation.
    FacetsAnnotation    mmu_segmentation_facets;

    // Is set only when volume is Embossed Text type
    // Contain information how to re-create volume
    std::optional<TextConfiguration> text_configuration;

    // Is set only when volume is Embossed Shape
    // Contain 2d information about embossed shape to be editabled
    std::optional<EmbossShape> emboss_shape; 

    // A parent object owning this modifier volume.
    ModelObject*        get_object() const { return this->object; }
    ModelVolumeType     type() const { return m_type; }
    void                set_type(const ModelVolumeType t) { m_type = t; }
	bool                is_model_part()         const { return m_type == ModelVolumeType::MODEL_PART; }
    bool                is_negative_volume()    const { return m_type == ModelVolumeType::NEGATIVE_VOLUME; }
	bool                is_modifier()           const { return m_type == ModelVolumeType::PARAMETER_MODIFIER; }
	bool                is_support_enforcer()   const { return m_type == ModelVolumeType::SUPPORT_ENFORCER; }
	bool                is_support_blocker()    const { return m_type == ModelVolumeType::SUPPORT_BLOCKER; }
	bool                is_support_modifier()   const { return m_type == ModelVolumeType::SUPPORT_BLOCKER || m_type == ModelVolumeType::SUPPORT_ENFORCER; }
    bool                is_text()               const { return text_configuration.has_value(); }
    bool                is_svg() const { return emboss_shape.has_value()  && !text_configuration.has_value(); }
    bool                is_the_only_one_part() const; // behave like an object
    t_model_material_id material_id() const { return m_material_id; }
    void                reset_extra_facets();
    void                apply_tolerance();
    void                set_material_id(t_model_material_id material_id);
    ModelMaterial*      material() const;
    void                set_material(t_model_material_id material_id, const ModelMaterial &material);
    // Extract the current extruder ID based on this ModelVolume's config and the parent ModelObject's config.
    // Extruder ID is only valid for FFF. Returns -1 for SLA or if the extruder ID is not applicable (support volumes).
    int                 extruder_id() const;

    bool                is_splittable() const;

    // Split this volume, append the result to the object owning this volume.
    // Return the number of volumes created from this one.
    // This is useful to assign different materials to different volumes of an object.
    size_t              split(unsigned int max_extruders);
    void                translate(double x, double y, double z) { translate(Vec3d(x, y, z)); }
    void                translate(const Vec3d& displacement);
    void                scale(const Vec3d& scaling_factors);
    void                scale(double x, double y, double z) { scale(Vec3d(x, y, z)); }
    void                scale(double s) { scale(Vec3d(s, s, s)); }
    void                rotate(double angle, Axis axis);
    void                rotate(double angle, const Vec3d& axis);
    void                mirror(Axis axis);

    // This method could only be called before the meshes of this ModelVolumes are not shared!
    void                scale_geometry_after_creation(const Vec3f &versor);
    void                scale_geometry_after_creation(const float scale) { this->scale_geometry_after_creation(Vec3f(scale, scale, scale)); }

    // Translates the mesh and the convex hull so that the origin of their vertices is in the center of this volume's bounding box.
    // Attention! This method may only be called just after ModelVolume creation! It must not be called once the TriangleMesh of this ModelVolume is shared!
    void                center_geometry_after_creation(bool update_source_offset = true);

    void                calculate_convex_hull();
    const TriangleMesh& get_convex_hull() const;
    const std::shared_ptr<const TriangleMesh>& get_convex_hull_shared_ptr() const { return m_convex_hull; }
    // Get count of errors in the mesh
    int                 get_repaired_errors_count() const;

    // Helpers for loading / storing into AMF / 3MF files.
    static ModelVolumeType type_from_string(const std::string &s);
    static std::string  type_to_string(const ModelVolumeType t);

    const Geometry::Transformation& get_transformation() const { return m_transformation; }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }
    void set_transformation(const Transform3d& trafo) { m_transformation.set_matrix(trafo); }

    Vec3d get_offset() const { return m_transformation.get_offset(); }

    double get_offset(Axis axis) const { return m_transformation.get_offset(axis); }

    void set_offset(const Vec3d& offset) { m_transformation.set_offset(offset); }
    void set_offset(Axis axis, double offset) { m_transformation.set_offset(axis, offset); }

    Vec3d get_rotation() const { return m_transformation.get_rotation(); }
    double get_rotation(Axis axis) const { return m_transformation.get_rotation(axis); }

    void set_rotation(const Vec3d& rotation) { m_transformation.set_rotation(rotation); }
    void set_rotation(Axis axis, double rotation) { m_transformation.set_rotation(axis, rotation); }

    Vec3d get_scaling_factor() const { return m_transformation.get_scaling_factor(); }
    double get_scaling_factor(Axis axis) const { return m_transformation.get_scaling_factor(axis); }

    void set_scaling_factor(const Vec3d& scaling_factor) { m_transformation.set_scaling_factor(scaling_factor); }
    void set_scaling_factor(Axis axis, double scaling_factor) { m_transformation.set_scaling_factor(axis, scaling_factor); }

    Vec3d get_mirror() const { return m_transformation.get_mirror(); }
    double get_mirror(Axis axis) const { return m_transformation.get_mirror(axis); }
    bool is_left_handed() const { return m_transformation.is_left_handed(); }

    void set_mirror(const Vec3d& mirror) { m_transformation.set_mirror(mirror); }
    void set_mirror(Axis axis, double mirror) { m_transformation.set_mirror(axis, mirror); }
    void convert_from_imperial_units();
    void convert_from_meters();

    const Transform3d& get_matrix() const { return m_transformation.get_matrix(); }
    Transform3d get_matrix_no_offset() const { return m_transformation.get_matrix_no_offset(); }

	void set_new_unique_id() { 
        ObjectBase::set_new_unique_id();
        this->config.set_new_unique_id();
        this->supported_facets.set_new_unique_id();
        this->seam_facets.set_new_unique_id();
        this->mmu_segmentation_facets.set_new_unique_id();
    }

    bool is_fdm_support_painted() const { return !this->supported_facets.empty(); }
    bool is_seam_painted() const { return !this->seam_facets.empty(); }
    bool is_mm_painted() const { return !this->mmu_segmentation_facets.empty(); }

protected:
	friend class Print;
    friend class SLAPrint;
    friend class Model;
	friend class ModelObject;
    friend void model_volume_list_update_supports(ModelObject& model_object_dst, const ModelObject& model_object_new);

	// Copies IDs of both the ModelVolume and its config.
	explicit ModelVolume(const ModelVolume &rhs) = default;
    void     set_model_object(ModelObject *model_object) { object = model_object; }
	void 	 assign_new_unique_ids_recursive() override;
    void     transform_this_mesh(const Transform3d& t, bool fix_left_handed);
    void     transform_this_mesh(const Matrix3d& m, bool fix_left_handed);

private:
    // Parent object owning this ModelVolume.
    ModelObject*                    	object;
    // The triangular model.
    std::shared_ptr<const TriangleMesh> m_mesh;
    // Is it an object to be printed, or a modifier volume?
    ModelVolumeType                 	m_type;
    t_model_material_id             	m_material_id;
    // The convex hull of this model's mesh.
    std::shared_ptr<const TriangleMesh> m_convex_hull;
    Geometry::Transformation        	m_transformation;

    // flag to optimize the checking if the volume is splittable
    //     -1   ->   is unknown value (before first cheking)
    //      0   ->   is not splittable
    //      1   ->   is splittable
    mutable int               		m_is_splittable{ -1 };

    inline bool check() {
        assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
        return true;
    }

	ModelVolume(ModelObject *object, const TriangleMesh &mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART) :
        m_mesh(new TriangleMesh(mesh)), m_type(type), object(object)
    {
        assert(check());
        if (m_mesh->facets_count() > 1) calculate_convex_hull();
    }
    ModelVolume(ModelObject* object, const std::shared_ptr<const TriangleMesh>& mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART) : m_mesh(mesh), m_type(type), object(object)
    {
        assert(this->id().valid());
        assert(this->config.id().valid());
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
    }
    ModelVolume(ModelObject *object, TriangleMesh &&mesh, ModelVolumeType type = ModelVolumeType::MODEL_PART)
        : m_mesh(new TriangleMesh(std::move(mesh))), m_type(type), object(object)
    {
        assert(check());
        if (m_mesh->facets_count() > 1) calculate_convex_hull();
    }
    ModelVolume(ModelObject *object, TriangleMesh &&mesh, TriangleMesh &&convex_hull, ModelVolumeType type = ModelVolumeType::MODEL_PART) :
		m_mesh(new TriangleMesh(std::move(mesh))), m_convex_hull(new TriangleMesh(std::move(convex_hull))), m_type(type), object(object) {
        assert(check());
	}

    // Copying an existing volume, therefore this volume will get a copy of the ID assigned.
    ModelVolume(ModelObject *object, const ModelVolume &other) :
        ObjectBase(other),
        name(other.name), source(other.source), m_mesh(other.m_mesh), m_convex_hull(other.m_convex_hull),
        config(other.config), m_type(other.m_type), object(object), m_transformation(other.m_transformation),
        supported_facets(other.supported_facets), seam_facets(other.seam_facets), mmu_segmentation_facets(other.mmu_segmentation_facets),
        cut_info(other.cut_info), text_configuration(other.text_configuration), emboss_shape(other.emboss_shape)
    {
		assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
		assert(this->id() == other.id());
        assert(this->config.id() == other.config.id());
        assert(this->supported_facets.id() == other.supported_facets.id());
        assert(this->seam_facets.id() == other.seam_facets.id());
        assert(this->mmu_segmentation_facets.id() == other.mmu_segmentation_facets.id());
        this->set_material_id(other.material_id());
    }
    // Providing a new mesh, therefore this volume will get a new unique ID assigned.
    ModelVolume(ModelObject *object, const ModelVolume &other, TriangleMesh &&mesh) :
        name(other.name), source(other.source), config(other.config), object(object), m_mesh(new TriangleMesh(std::move(mesh))), m_type(other.m_type), m_transformation(other.m_transformation),
        cut_info(other.cut_info), text_configuration(other.text_configuration), emboss_shape(other.emboss_shape)
    {
		assert(this->id().valid()); 
        assert(this->config.id().valid()); 
        assert(this->supported_facets.id().valid());
        assert(this->seam_facets.id().valid());
        assert(this->mmu_segmentation_facets.id().valid());
        assert(this->id() != this->config.id());
        assert(this->id() != this->supported_facets.id());
        assert(this->id() != this->seam_facets.id());
        assert(this->id() != this->mmu_segmentation_facets.id());
		assert(this->id() != other.id());
        assert(this->config.id() == other.config.id());
        this->set_material_id(other.material_id());
        this->config.set_new_unique_id();
        if (m_mesh->facets_count() > 1)
            calculate_convex_hull();
		assert(this->config.id().valid()); 
        assert(this->config.id() != other.config.id()); 
        assert(this->supported_facets.id() != other.supported_facets.id());
        assert(this->seam_facets.id() != other.seam_facets.id());
        assert(this->mmu_segmentation_facets.id() != other.mmu_segmentation_facets.id());
        assert(this->id() != this->config.id());
        assert(this->supported_facets.empty());
        assert(this->seam_facets.empty());
        assert(this->mmu_segmentation_facets.empty());
    }

    ModelVolume& operator=(ModelVolume &rhs) = delete;

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization, therefore no IDs are allocated.
	ModelVolume() : ObjectBase(-1), config(-1), supported_facets(-1), seam_facets(-1), mmu_segmentation_facets(-1), object(nullptr) {
		assert(this->id().invalid());
        assert(this->config.id().invalid());
        assert(this->supported_facets.id().invalid());
        assert(this->seam_facets.id().invalid());
        assert(this->mmu_segmentation_facets.id().invalid());
	}
	template<class Archive> void load(Archive &ar) {
		bool has_convex_hull;
        ar(name, source, m_mesh, m_type, m_material_id, m_transformation, m_is_splittable, has_convex_hull, cut_info);
        cereal::load_by_value(ar, supported_facets);
        cereal::load_by_value(ar, seam_facets);
        cereal::load_by_value(ar, mmu_segmentation_facets);
        cereal::load_by_value(ar, config);
        cereal::load(ar, text_configuration);
        cereal::load(ar, emboss_shape);
		assert(m_mesh);
		if (has_convex_hull) {
			cereal::load_optional(ar, m_convex_hull);
			if (! m_convex_hull && ! m_mesh->empty())
				// The convex hull was released from the Undo / Redo stack to conserve memory. Recalculate it.
				this->calculate_convex_hull();
		} else
			m_convex_hull.reset();
	}
	template<class Archive> void save(Archive &ar) const {
		bool has_convex_hull = m_convex_hull.get() != nullptr;
        ar(name, source, m_mesh, m_type, m_material_id, m_transformation, m_is_splittable, has_convex_hull, cut_info);
        cereal::save_by_value(ar, supported_facets);
        cereal::save_by_value(ar, seam_facets);
        cereal::save_by_value(ar, mmu_segmentation_facets);
        cereal::save_by_value(ar, config);
        cereal::save(ar, text_configuration);
        cereal::save(ar, emboss_shape);
		if (has_convex_hull)
			cereal::save_optional(ar, m_convex_hull);
	}
};

inline void model_volumes_sort_by_id(ModelVolumePtrs &model_volumes)
{
    std::sort(model_volumes.begin(), model_volumes.end(), [](const ModelVolume *l, const ModelVolume *r) { return l->id() < r->id(); });
}

inline const ModelVolume* model_volume_find_by_id(const ModelVolumePtrs &model_volumes, const ObjectID id)
{
    auto it = lower_bound_by_predicate(model_volumes.begin(), model_volumes.end(), [id](const ModelVolume *mv) { return mv->id() < id; });
    return it != model_volumes.end() && (*it)->id() == id ? *it : nullptr;
}

enum ModelInstanceEPrintVolumeState : unsigned char
{
    ModelInstancePVS_Inside,
    ModelInstancePVS_Partly_Outside,
    ModelInstancePVS_Fully_Outside,
    ModelInstanceNum_BedStates
};

// A single instance of a ModelObject.
// Knows the affine transformation of an object.
class ModelInstance final : public ObjectBase
{
private:
    Geometry::Transformation m_transformation;
    Geometry::Transformation m_assemble_transformation;
    Vec3d m_offset_to_assembly{ 0.0, 0.0, 0.0 };
    bool m_assemble_initialized { false };

public:
    // flag showing the position of this instance with respect to the print volume (set by Print::validate() using ModelObject::check_instances_print_volume_state())
    ModelInstanceEPrintVolumeState print_volume_state;
    // Whether or not this instance is printable
    bool printable { true };

    ModelObject* get_object() const { return this->object; }

    const Geometry::Transformation& get_transformation() const { return m_transformation; }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }

    const Geometry::Transformation& get_assemble_transformation() const { return m_assemble_transformation; }
    void set_assemble_transformation(const Geometry::Transformation& transformation) {
        m_assemble_initialized = true;
        m_assemble_transformation = transformation;
    }
    void set_assemble_from_transform(Transform3d& transform) {
        m_assemble_initialized = true;
        m_assemble_transformation.set_from_transform(transform);
    }
    void set_assemble_offset(const Vec3d& offset) { m_assemble_transformation.set_offset(offset); }
    void rotate_assemble(double angle, const Vec3d& axis) {
        m_assemble_transformation.set_rotation(m_assemble_transformation.get_rotation() + Geometry::extract_euler_angles(Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis)).toRotationMatrix()));
    }

    void set_offset_to_assembly(const Vec3d& offset) { m_offset_to_assembly = offset; }
    Vec3d get_offset_to_assembly() const { return m_offset_to_assembly; }

    Vec3d get_offset() const { return m_transformation.get_offset(); }
    double get_offset(Axis axis) const { return m_transformation.get_offset(axis); }
    
    void set_offset(const Vec3d& offset) { m_transformation.set_offset(offset); }
    void set_offset(Axis axis, double offset) { m_transformation.set_offset(axis, offset); }

    Vec3d get_rotation() const { return m_transformation.get_rotation(); }
    double get_rotation(Axis axis) const { return m_transformation.get_rotation(axis); }

    void set_rotation(const Vec3d& rotation) { m_transformation.set_rotation(rotation); }
    void set_rotation(Axis axis, double rotation) { m_transformation.set_rotation(axis, rotation); }

    void rotate(const Matrix3d& matrix) {
        auto R = get_matrix().matrix().block<3, 3>(0, 0);
        auto R_new = matrix * R;
        auto euler_angles = extract_euler_angles(R_new);
        set_rotation(euler_angles);
    }

    Vec3d extract_euler_angles(const Eigen::Matrix<double, 3, 3, Eigen::DontAlign>& rotation_matrix) {
        Vec3d angle1 = Vec3d::Zero();
        Vec3d angle2 = Vec3d::Zero();

        if (std::abs(std::abs(rotation_matrix(2, 0)) - 1.0) < 1e-5 || std::abs(rotation_matrix(2, 0)) > 1)
        {
            angle1(2) = 0.0;
            if (rotation_matrix(2, 0) < 0.0) // == -1.0
            {
                angle1(1) = 0.5 * (double)PI;
                angle1(0) = angle1(2) + ::atan2(rotation_matrix(0, 1), rotation_matrix(0, 2));
            }
            else // == 1.0
            {
                angle1(1) = -0.5 * (double)PI;
                angle1(0) = -angle1(2) + ::atan2(-rotation_matrix(0, 1), -rotation_matrix(0, 2));
            }
            angle2 = angle1;
        }
        else
        {
            angle1(1) = -::asin(rotation_matrix(2, 0));
            double inv_cos1 = 1.0 / ::cos(angle1(1));
            angle1(0) = ::atan2(rotation_matrix(2, 1) * inv_cos1, rotation_matrix(2, 2) * inv_cos1);
            angle1(2) = ::atan2(rotation_matrix(1, 0) * inv_cos1, rotation_matrix(0, 0) * inv_cos1);

            angle2(1) = (double)PI - angle1(1);
            double inv_cos2 = 1.0 / ::cos(angle2(1));
            angle2(0) = ::atan2(rotation_matrix(2, 1) * inv_cos2, rotation_matrix(2, 2) * inv_cos2);
            angle2(2) = ::atan2(rotation_matrix(1, 0) * inv_cos2, rotation_matrix(0, 0) * inv_cos2);
        }

        // The following euristic is the best found up to now (in the sense that it works fine with the greatest number of edge use-cases)
        // but there are other use-cases were it does not
        // We need to improve it
        double min_1 = angle1.cwiseAbs().minCoeff();
        double min_2 = angle2.cwiseAbs().minCoeff();
        bool use_1 = (min_1 < min_2) || (is_approx(min_1, min_2) && (angle1.norm() <= angle2.norm()));

        return use_1 ? angle1 : angle2;
    }


    Vec3d get_scaling_factor() const { return m_transformation.get_scaling_factor(); }
    double get_scaling_factor(Axis axis) const { return m_transformation.get_scaling_factor(axis); }

    void set_scaling_factor(const Vec3d& scaling_factor) { m_transformation.set_scaling_factor(scaling_factor); }
    void set_scaling_factor(Axis axis, double scaling_factor) { m_transformation.set_scaling_factor(axis, scaling_factor); }

    Vec3d get_mirror() const { return m_transformation.get_mirror(); }
    double get_mirror(Axis axis) const { return m_transformation.get_mirror(axis); }
    bool is_left_handed() const { return m_transformation.is_left_handed(); }
    
    void set_mirror(const Vec3d& mirror) { m_transformation.set_mirror(mirror); }
    void set_mirror(Axis axis, double mirror) { m_transformation.set_mirror(axis, mirror); }

    // To be called on an external mesh
    void transform_mesh(TriangleMesh* mesh, bool dont_translate = false) const;
    // Transform an external bounding box, thus the resulting bounding box is no more snug.
    BoundingBoxf3 transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate = false) const;
    // Transform an external vector.
    Vec3d transform_vector(const Vec3d& v, bool dont_translate = false) const;
    // To be called on an external polygon. It does not translate the polygon, only rotates and scales.
    void transform_polygon(Polygon* polygon) const;

    const Transform3d& get_matrix() const { return m_transformation.get_matrix(); }
    Transform3d get_matrix_no_offset() const { return m_transformation.get_matrix_no_offset(); }

    bool is_printable() const { return object->printable && printable && (print_volume_state == ModelInstancePVS_Inside); }
    bool is_assemble_initialized() { return m_assemble_initialized; }

    void invalidate_object_bounding_box() { object->invalidate_bounding_box(); }
    // Getting the input polygon for arrange
    arrangement::ArrangePolygon get_arrange_polygon() const;
    
    // Apply the arrange result on the ModelInstance
    void apply_arrange_result(const Vec2d& offs, double rotation)
    {
        // write the transformation data into the model instance
        set_rotation(Z, rotation);
        set_offset(X, unscale<double>(offs(X)));
        set_offset(Y, unscale<double>(offs(Y)));
        this->object->invalidate_bounding_box();
    }

protected:
    friend class Print;
    friend class SLAPrint;
    friend class Model;
    friend class ModelObject;

    explicit ModelInstance(const ModelInstance &rhs) = default;
    void     set_model_object(ModelObject *model_object) { object = model_object; }

private:
    // Parent object, owning this instance.
    ModelObject* object;

    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject* object) : print_volume_state(ModelInstancePVS_Inside), object(object), m_assemble_initialized(false) { assert(this->id().valid()); }
    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject *object, const ModelInstance &other) :
        m_transformation(other.m_transformation), m_assemble_transformation(other.m_assemble_transformation), print_volume_state(ModelInstancePVS_Inside), printable(other.printable), object(object), m_assemble_initialized(false) { assert(this->id().valid() && this->id() != other.id()); }

    explicit ModelInstance(ModelInstance &&rhs) = delete;
    ModelInstance& operator=(const ModelInstance &rhs) = delete;
    ModelInstance& operator=(ModelInstance &&rhs) = delete;

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	// Used for deserialization, therefore no IDs are allocated.
	ModelInstance() : ObjectBase(-1), object(nullptr) { assert(this->id().invalid()); }
	template<class Archive> void serialize(Archive &ar) {
        ar(m_transformation, print_volume_state, printable, m_assemble_transformation, m_offset_to_assembly, m_assemble_initialized);
    }
};


class ModelWipeTower final : public ObjectBase
{
public:
	Vec2d		position;
	double 		rotation;

private:
	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	friend class Model;

    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit ModelWipeTower() {}
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit ModelWipeTower(int) : ObjectBase(-1) {}
    // Copy constructor copies the ID.
	explicit ModelWipeTower(const ModelWipeTower &cfg) = default;

	// Disabled methods.
	ModelWipeTower(ModelWipeTower &&rhs) = delete;
	ModelWipeTower& operator=(const ModelWipeTower &rhs) = delete;
    ModelWipeTower& operator=(ModelWipeTower &&rhs) = delete;

    // For serialization / deserialization of ModelWipeTower composed into another class into the Undo / Redo stack as a separate object.
    template<typename Archive> void serialize(Archive &ar) { ar(position, rotation); }
};

// structure stores extruder parameters and speed map of all models
struct ExtruderParams
{
    std::string materialName;
    int bedTemp;
    double heatEndTemp;
};

struct GlobalSpeedMap
{
    double perimeterSpeed;
    double externalPerimeterSpeed;
    double infillSpeed;
    double solidInfillSpeed;
    double topSolidInfillSpeed;
    double supportSpeed;
    double smallPerimeterSpeed;
    double maxSpeed;
    Polygon bed_poly;
};

// The print bed content.
// Description of a triangular model with multiple materials, multiple instances with various affine transformations
// and with multiple modifier meshes.
// A model groups multiple objects, each object having possibly multiple instances,
// all objects may share multiple materials.
class Model final : public ObjectBase
{
public:
    // Materials are owned by a model and referenced by objects through t_model_material_id.
    // Single material may be shared by multiple models.
    ModelMaterialMap    materials;
    // Objects are owned by a model. Each model may have multiple instances, each instance having its own transformation (shift, scale, rotation).
    ModelObjectPtrs     objects;
    // Wipe tower object.
    ModelWipeTower	    wipe_tower;

    // static members store extruder parameters and speed map of all models
    static std::map<size_t, ExtruderParams> extruderParamsMap;
    static GlobalSpeedMap printSpeedMap;
<<<<<<< HEAD

    // Extensions for color print
// CustomGCode::Info custom_gcode_per_print_z;
//BBS: replace model custom gcode with current plate custom gcode
    int curr_plate_index{ 0 };
    std::map<int, CustomGCode::Info> plates_custom_gcodes; //map<plate_index, CustomGCode::Info>
=======
>>>>>>> 84b4984 (feat: 1.5.21 open source)
    
    // Extensions for color print
    CustomGCode::Info custom_gcode_per_print_z;
    
    // Default constructor assigns a new ID to the model.
    Model() { assert(this->id().valid()); }
    ~Model() { this->clear_objects(); this->clear_materials(); }

    /* To be able to return an object from own copy / clone methods. Hopefully the compiler will do the "Copy elision" */
    /* (Omits copy and move(since C++11) constructors, resulting in zero - copy pass - by - value semantics). */
    Model(const Model &rhs) : ObjectBase(-1) { assert(this->id().invalid()); this->assign_copy(rhs); assert(this->id().valid()); assert(this->id() == rhs.id()); }
    explicit Model(Model &&rhs) : ObjectBase(-1) { assert(this->id().invalid()); this->assign_copy(std::move(rhs)); assert(this->id().valid()); assert(this->id() == rhs.id()); }
    Model& operator=(const Model &rhs) { this->assign_copy(rhs); assert(this->id().valid()); assert(this->id() == rhs.id()); return *this; }
    Model& operator=(Model &&rhs) { this->assign_copy(std::move(rhs)); assert(this->id().valid()); assert(this->id() == rhs.id()); return *this; }

    OBJECTBASE_DERIVED_COPY_MOVE_CLONE(Model)

    enum class LoadAttribute : int {
        AddDefaultInstances,
        CheckVersion
    };
    using LoadAttributes = enum_bitmask<LoadAttribute>;

    static Model read_from_file(
        const std::string& input_file, 
        DynamicPrintConfig* config = nullptr, ConfigSubstitutionContext* config_substitutions = nullptr,
        LoadAttributes options = LoadAttribute::AddDefaultInstances);
    static Model read_from_archive(
        const std::string& input_file, 
        DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions,
        LoadAttributes options = LoadAttribute::AddDefaultInstances);

    // BBS 
    static Polygon getBedPolygon() { return Model::printSpeedMap.bed_poly; };
	static double findMaxSpeed(const ModelObject* object);
	static double getThermalLength(const ModelVolume* modelVolumePtr);
	static double getThermalLength(const std::vector<ModelVolume*> modelVolumePtrs);

    // Add a new ModelObject to this Model, generate a new ID for this ModelObject.
    ModelObject* add_object();
    ModelObject* add_object(const char *name, const char *path, const TriangleMesh &mesh);
    ModelObject* add_object(const char *name, const char *path, TriangleMesh &&mesh);
    ModelObject* add_object(const ModelObject &other);
    void         delete_object(size_t idx);
    bool         delete_object(ObjectID id);
    bool         delete_object(ModelObject* object);
    void         clear_objects();

    ModelMaterial* add_material(t_model_material_id material_id);
    ModelMaterial* add_material(t_model_material_id material_id, const ModelMaterial &other);
    ModelMaterial* get_material(t_model_material_id material_id) {
        ModelMaterialMap::iterator i = this->materials.find(material_id);
        return (i == this->materials.end()) ? nullptr : i->second;
    }

    void          delete_material(t_model_material_id material_id);
    void          clear_materials();
    bool          add_default_instances();
    // Returns approximate axis aligned bounding box of this model.
    BoundingBoxf3 bounding_box_approx() const;
    // Returns exact axis aligned bounding box of this model.
    BoundingBoxf3 bounding_box_exact() const;
    // Return maximum height of all printable objects.
    double        max_z() const;
    // Set the print_volume_state of PrintObject::instances, 
    // return total number of printable objects.
    unsigned int  update_print_volume_state(const BuildVolume &build_volume);
    // Returns true if any ModelObject was modified.
    bool 		  center_instances_around_point(const Vec2d &point);
    void 		  translate(coordf_t x, coordf_t y, coordf_t z) { for (ModelObject *o : this->objects) o->translate(x, y, z); }
    TriangleMesh  mesh() const;
    
    // Croaks if the duplicated objects do not fit the print bed.
    void duplicate_objects_grid(size_t x, size_t y, coordf_t dist);

    bool 		  looks_like_multipart_object() const;
    void 		  convert_multipart_object(unsigned int max_extruders);
    bool          looks_like_imperial_units() const;
    void          convert_from_imperial_units(bool only_small_volumes);
    bool          looks_like_saved_in_meters() const;
    void          convert_from_meters(bool only_small_volumes);
    int           removed_objects_with_zero_volume();

    // Ensures that the min z of the model is not negative
    void 		  adjust_min_z();

    void 		  print_info() const { for (const ModelObject *o : this->objects) o->print_info(); }

    // Propose an output file name & path based on the first printable object's name and source input file's path.
    std::string   propose_export_file_name_and_path() const;
    // Propose an output path, replace extension. The new_extension shall contain the initial dot.
    std::string   propose_export_file_name_and_path(const std::string &new_extension) const;

    // Checks if any of objects is painted using the fdm support painting gizmo.
    bool          is_fdm_support_painted() const;
    // Checks if any of objects is painted using the seam painting gizmo.
    bool          is_seam_painted() const;
    // Checks if any of objects is painted using the multi-material painting gizmo.
    bool          is_mm_painted() const;

private:
    explicit Model(int) : ObjectBase(-1) { assert(this->id().invalid()); }
	void assign_new_unique_ids_recursive();
	void update_links_bottom_up_recursive();

	friend class cereal::access;
	friend class UndoRedo::StackImpl;
	template<class Archive> void serialize(Archive &ar) {
		Internal::StaticSerializationWrapper<ModelWipeTower> wipe_tower_wrapper(wipe_tower);
		ar(materials, objects, wipe_tower_wrapper);
    }
};

ENABLE_ENUM_BITMASK_OPERATORS(Model::LoadAttribute)

#undef OBJECTBASE_DERIVED_COPY_MOVE_CLONE
#undef OBJECTBASE_DERIVED_PRIVATE_COPY_MOVE

// Test whether the two models contain the same number of ModelObjects with the same set of IDs
// ordered in the same order. In that case it is not necessary to kill the background processing.
bool model_object_list_equal(const Model &model_old, const Model &model_new);

// Test whether the new model is just an extension of the old model (new objects were added
// to the end of the original list. In that case it is not necessary to kill the background processing.
bool model_object_list_extended(const Model &model_old, const Model &model_new);

// Test whether the new ModelObject contains a different set of volumes (or sorted in a different order)
// than the old ModelObject.
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const ModelVolumeType type);
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const std::initializer_list<ModelVolumeType> &types);

// Test whether the now ModelObject has newer custom supports data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_supports_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer custom seam data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_seam_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer MMU segmentation data than the old one.
// The function assumes that volumes list is synchronized.
extern bool model_mmu_segmentation_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// If the model has object(s) which contains a modifier, then it is currently not supported by the SLA mode.
// Either the model cannot be loaded, or a SLA printer has to be activated.
bool model_has_parameter_modifiers_in_objects(const Model& model);
// If the model has multi-part objects, then it is currently not supported by the SLA mode.
// Either the model cannot be loaded, or a SLA printer has to be activated.
bool model_has_multi_part_objects(const Model &model);
// If the model has objects with cut connects, then it is currently not supported by the SLA mode.
bool model_has_connectors(const Model& model);
// If the model has advanced features, then it cannot be processed in simple mode.
bool model_has_advanced_features(const Model &model);

#ifndef NDEBUG
// Verify whether the IDs of Model / ModelObject / ModelVolume / ModelInstance / ModelMaterial are valid and unique.
void check_model_ids_validity(const Model &model);
void check_model_ids_equal(const Model &model1, const Model &model2);
#endif /* NDEBUG */

static const float SINKING_Z_THRESHOLD = -0.001f;
static const double SINKING_MIN_Z_THRESHOLD = 0.05;

} // namespace Slic3r

namespace cereal
{
	template <class Archive> struct specialize<Archive, Slic3r::ModelVolume, cereal::specialization::member_load_save> {};
	template <class Archive> struct specialize<Archive, Slic3r::ModelConfigObject, cereal::specialization::member_serialize> {};
}

#endif /* slic3r_Model_hpp_ */

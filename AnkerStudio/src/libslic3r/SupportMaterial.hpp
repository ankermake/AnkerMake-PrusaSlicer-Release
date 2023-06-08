#ifndef slic3r_SupportMaterial_hpp_
#define slic3r_SupportMaterial_hpp_

#include "Flow.hpp"
#include "PrintConfig.hpp"
#include "Slicing.hpp"

namespace Slic3r {

class PrintObject;
class PrintConfig;
class PrintObjectConfig;

// Support layer type to be used by SupportGeneratorLayer. This type carries a much more detailed information
// about the support layer type than the final support layers stored in a PrintObject.
enum class SupporLayerType {
	Unknown = 0,
	// Ratft base layer, to be printed with the support material.
	RaftBase,
	// Raft interface layer, to be printed with the support interface material. 
	RaftInterface,
	// Bottom contact layer placed over a top surface of an object. To be printed with a support interface material.
	BottomContact,
	// Dense interface layer, to be printed with the support interface material.
	// This layer is separated from an object by an BottomContact layer.
	BottomInterface,
	// Sparse base support layer, to be printed with a support material.
	Base,
	// Dense interface layer, to be printed with the support interface material.
	// This layer is separated from an object with TopContact layer.
	TopInterface,
	// Top contact layer directly supporting an overhang. To be printed with a support interface material.
	TopContact,
	// Some undecided type yet. It will turn into Base first, then it may turn into BottomInterface or TopInterface.
	Intermediate,
};

// A support layer type used internally by the SupportMaterial class. This class carries a much more detailed
// information about the support layer than the layers stored in the PrintObject, mainly
// the SupportGeneratorLayer is aware of the bridging flow and the interface gaps between the object and the support.
class SupportGeneratorLayer
{
public:
	void reset() {
		*this = SupportGeneratorLayer();
	}

	bool operator==(const SupportGeneratorLayer &layer2) const {
		return print_z == layer2.print_z && height == layer2.height && bridging == layer2.bridging;
	}

	// Order the layers by lexicographically by an increasing print_z and a decreasing layer height.
	bool operator<(const SupportGeneratorLayer &layer2) const {
		if (print_z < layer2.print_z) {
			return true;
		} else if (print_z == layer2.print_z) {
		 	if (height > layer2.height)
		 		return true;
		 	else if (height == layer2.height) {
		 		// Bridging layers first.
		 	 	return bridging && ! layer2.bridging;
		 	} else
		 		return false;
		} else
			return false;
	}

	void merge(SupportGeneratorLayer &&rhs) {
        // The union_() does not support move semantic yet, but maybe one day it will.
        this->polygons = union_(this->polygons, std::move(rhs.polygons));
        auto merge = [](std::unique_ptr<Polygons> &dst, std::unique_ptr<Polygons> &src) {
        	if (! dst || dst->empty())
        		dst = std::move(src);
        	else if (src && ! src->empty())
    			*dst = union_(*dst, std::move(*src));
        };
        merge(this->contact_polygons,  rhs.contact_polygons);
        merge(this->overhang_polygons, rhs.overhang_polygons);
        merge(this->enforcer_polygons, rhs.enforcer_polygons);
        rhs.reset();
    }

	// For the bridging flow, bottom_print_z will be above bottom_z to account for the vertical separation.
	// For the non-bridging flow, bottom_print_z will be equal to bottom_z.
	coordf_t bottom_print_z() const { return print_z - height; }

	// To sort the extremes of top / bottom interface layers.
	coordf_t extreme_z() const { return (this->layer_type == SupporLayerType::TopContact) ? this->bottom_z : this->print_z; }

	SupporLayerType layer_type { SupporLayerType::Unknown };
	// Z used for printing, in unscaled coordinates.
	coordf_t print_z { 0 };
	// Bottom Z of this layer. For soluble layers, bottom_z + height = print_z,
	// otherwise bottom_z + gap + height = print_z.
	coordf_t bottom_z { 0 };
	// Layer height in unscaled coordinates.
	coordf_t height { 0 };
	// Index of a PrintObject layer_id supported by this layer. This will be set for top contact layers.
	// If this is not a contact layer, it will be set to size_t(-1).
	size_t 	 idx_object_layer_above { size_t(-1) };
	// Index of a PrintObject layer_id, which supports this layer. This will be set for bottom contact layers.
	// If this is not a contact layer, it will be set to size_t(-1).
	size_t 	 idx_object_layer_below { size_t(-1) };
	// Use a bridging flow when printing this support layer.
	bool 	 bridging { false };

	// Polygons to be filled by the support pattern.
	Polygons polygons;
	// Currently for the contact layers only.
	std::unique_ptr<Polygons> contact_polygons;
	std::unique_ptr<Polygons> overhang_polygons;
	// Enforcers need to be propagated independently in case the "support on build plate only" option is enabled.
	std::unique_ptr<Polygons> enforcer_polygons;
};

// Layers are allocated and owned by a deque. Once a layer is allocated, it is maintained
// up to the end of a generate() method. The layer storage may be replaced by an allocator class in the future, 
// which would allocate layers by multiple chunks.
using SupportGeneratorLayerStorage	= std::deque<SupportGeneratorLayer>;
using SupportGeneratorLayersPtr		= std::vector<SupportGeneratorLayer*>;

struct SupportParameters {
	SupportParameters(const PrintObject &object);

	// Flow at the 1st print layer.
	Flow 					first_layer_flow;
	// Flow at the support base (neither top, nor bottom interface).
	// Also flow at the raft base with the exception of raft interface and contact layers.
	Flow 					support_material_flow;
	// Flow at the top interface and contact layers.
	Flow 					support_material_interface_flow;
	// Flow at the bottom interfaces and contacts.
	Flow 					support_material_bottom_interface_flow;
	// Flow at raft inteface & contact layers.
	Flow    				raft_interface_flow;
	// Is merging of regions allowed? Could the interface & base support regions be printed with the same extruder?
	bool 					can_merge_support_regions;

    coordf_t 				support_layer_height_min;
//	coordf_t				support_layer_height_max;

	coordf_t				gap_xy;

    float    				base_angle;
    float    				interface_angle;

    // Density of the top / bottom interface and contact layers.
    coordf_t 				interface_density;
    // Density of the raft interface and contact layers.
    coordf_t 				raft_interface_density;
    // Density of the base support layers.
    coordf_t 				support_density;

    // Pattern of the sparse infill including sparse raft layers.
    InfillPattern           base_fill_pattern;
    // Pattern of the top / bottom interface and contact layers.
    InfillPattern           interface_fill_pattern;
    // Pattern of the raft interface and contact layers.
    InfillPattern           raft_interface_fill_pattern;
    // Pattern of the contact layers.
    InfillPattern 			contact_fill_pattern;
    // Shall the sparse (base) layers be printed with a single perimeter line (sheath) for robustness?
    bool                    with_sheath;

    float 					raft_angle_1st_layer;
    float 					raft_angle_base;
    float 					raft_angle_interface;

    // Produce a raft interface angle for a given SupportLayer::interface_id()
    float 					raft_interface_angle(size_t interface_id) const 
    	{ return this->raft_angle_interface + ((interface_id & 1) ? float(- M_PI / 4.) : float(+ M_PI / 4.)); }
};

// Remove bridges from support contact areas.
// To be called if PrintObjectConfig::dont_support_bridges.
void remove_bridges_from_contacts(
    const PrintConfig   &print_config, 
    const Layer         &lower_layer,
    const LayerRegion   &layerm,
    float                fw, 
    Polygons            &contact_polygons);

// Generate raft layers, also expand the 1st support layer
// in case there is no raft layer to improve support adhesion.
SupportGeneratorLayersPtr generate_raft_base(
	const PrintObject				&object,
	const SupportParameters			&support_params,
	const SlicingParameters			&slicing_params,
	const SupportGeneratorLayersPtr &top_contacts,
	const SupportGeneratorLayersPtr &interface_layers,
	const SupportGeneratorLayersPtr &base_interface_layers,
	const SupportGeneratorLayersPtr &base_layers,
	SupportGeneratorLayerStorage    &layer_storage);

// returns sorted layers
SupportGeneratorLayersPtr generate_support_layers(
	PrintObject							&object,
    const SupportGeneratorLayersPtr     &raft_layers,
    const SupportGeneratorLayersPtr     &bottom_contacts,
    const SupportGeneratorLayersPtr     &top_contacts,
    const SupportGeneratorLayersPtr     &intermediate_layers,
    const SupportGeneratorLayersPtr     &interface_layers,
    const SupportGeneratorLayersPtr     &base_interface_layers);

// Produce the support G-code.
// Used by both classic and tree supports.
void generate_support_toolpaths(
	SupportLayerPtrs    				&support_layers,
	const PrintObjectConfig 			&config,
	const SupportParameters 			&support_params,
	const SlicingParameters 			&slicing_params,
    const SupportGeneratorLayersPtr 	&raft_layers,
    const SupportGeneratorLayersPtr   	&bottom_contacts,
    const SupportGeneratorLayersPtr   	&top_contacts,
    const SupportGeneratorLayersPtr   	&intermediate_layers,
	const SupportGeneratorLayersPtr   	&interface_layers,
    const SupportGeneratorLayersPtr   	&base_interface_layers);

void export_print_z_polygons_to_svg(const char *path, SupportGeneratorLayer ** const layers, size_t n_layers);
void export_print_z_polygons_and_extrusions_to_svg(const char *path, SupportGeneratorLayer ** const layers, size_t n_layers, SupportLayer& support_layer);

// This class manages raft and supports for a single PrintObject.
// Instantiated by Slic3r::Print::Object->_support_material()
// This class is instantiated before the slicing starts as Object.pm will query
// the parameters of the raft to determine the 1st layer height and thickness.
class PrintObjectSupportMaterial
{
public:
	PrintObjectSupportMaterial(const PrintObject *object, const SlicingParameters &slicing_params);

	// Is raft enabled?
	bool 		has_raft() 					const { return m_slicing_params.has_raft(); }
	// Has any support?
	bool 		has_support()				const { return m_object_config->support_material.value || m_object_config->support_material_enforce_layers; }
	bool 		build_plate_only() 			const { return this->has_support() && m_object_config->support_material_buildplate_only.value; }

	bool 		synchronize_layers()		const { return m_slicing_params.soluble_interface && m_object_config->support_material_synchronize_layers.value; }
	bool 		has_contact_loops() 		const { return m_object_config->support_material_interface_contact_loops.value; }

	// Generate support material for the object.
	// New support layers will be added to the object,
	// with extrusion paths and islands filled in for each support layer.
	void 		generate(PrintObject &object);

private:
	std::vector<Polygons> buildplate_covered(const PrintObject &object) const;

	// Generate top contact layers supporting overhangs.
	// For a soluble interface material synchronize the layer heights with the object, otherwise leave the layer height undefined.
	// If supports over bed surface only are requested, don't generate contact layers over an object.
	SupportGeneratorLayersPtr top_contact_layers(const PrintObject &object, const std::vector<Polygons> &buildplate_covered, SupportGeneratorLayerStorage &layer_storage) const;

	// Generate bottom contact layers supporting the top contact layers.
	// For a soluble interface material synchronize the layer heights with the object, 
	// otherwise set the layer height to a bridging flow of a support interface nozzle.
	SupportGeneratorLayersPtr bottom_contact_layers_and_layer_support_areas(
		const PrintObject &object, const SupportGeneratorLayersPtr &top_contacts, std::vector<Polygons> &buildplate_covered, 
		SupportGeneratorLayerStorage &layer_storage, std::vector<Polygons> &layer_support_areas) const;

	// Trim the top_contacts layers with the bottom_contacts layers if they overlap, so there would not be enough vertical space for both of them.
	void trim_top_contacts_by_bottom_contacts(const PrintObject &object, const SupportGeneratorLayersPtr &bottom_contacts, SupportGeneratorLayersPtr &top_contacts) const;

	// Generate raft layers and the intermediate support layers between the bottom contact and top contact surfaces.
	SupportGeneratorLayersPtr raft_and_intermediate_support_layers(
	    const PrintObject   &object,
	    const SupportGeneratorLayersPtr   &bottom_contacts,
	    const SupportGeneratorLayersPtr   &top_contacts,
	    SupportGeneratorLayerStorage	 	&layer_storage) const;

	// Fill in the base layers with polygons.
	void generate_base_layers(
	    const PrintObject   &object,
	    const SupportGeneratorLayersPtr   &bottom_contacts,
	    const SupportGeneratorLayersPtr   &top_contacts,
	    SupportGeneratorLayersPtr         &intermediate_layers,
	    const std::vector<Polygons> &layer_support_areas) const;

	// Turn some of the base layers into base interface layers.
	// For soluble interfaces with non-soluble bases, print maximum two first interface layers with the base
	// extruder to improve adhesion of the soluble filament to the base.
	std::pair<SupportGeneratorLayersPtr, SupportGeneratorLayersPtr> generate_interface_layers(
	    const SupportGeneratorLayersPtr   &bottom_contacts,
	    const SupportGeneratorLayersPtr   &top_contacts,
	    SupportGeneratorLayersPtr         &intermediate_layers,
	    SupportGeneratorLayerStorage      &layer_storage) const;
	

	// Trim support layers by an object to leave a defined gap between
	// the support volume and the object.
	void trim_support_layers_by_object(
	    const PrintObject   &object,
	    SupportGeneratorLayersPtr         &support_layers,
	    const coordf_t       gap_extra_above,
	    const coordf_t       gap_extra_below,
	    const coordf_t       gap_xy) const;

/*
	void generate_pillars_shape();
	void clip_with_shape();
*/

	// Following objects are not owned by SupportMaterial class.
	const PrintConfig 		*m_print_config;
	const PrintObjectConfig *m_object_config;
	// Pre-calculated parameters shared between the object slicer and the support generator,
	// carrying information on a raft, 1st layer height, 1st object layer height, gap between the raft and object etc.
	SlicingParameters	     m_slicing_params;
	// Various precomputed support parameters to be shared with external functions.
	SupportParameters 		 m_support_params;
};

} // namespace Slic3r

#endif /* slic3r_SupportMaterial_hpp_ */

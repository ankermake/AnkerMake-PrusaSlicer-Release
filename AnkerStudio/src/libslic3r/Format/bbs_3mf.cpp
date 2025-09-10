#include "../libslic3r.h"
#include "../Exception.hpp"
#include "../Model.hpp"
#include "../Preset.hpp"
#include "../Utils.hpp"
#include "../LocalesUtils.hpp"
#include "../GCode.hpp"
#include "../Geometry.hpp"
#include "../GCode/ThumbnailData.hpp"
#include "../Semver.hpp"
#include "../Time.hpp"

#include "../I18N.hpp"

#include "bbs_3mf.hpp"

#include <limits>
#include <stdexcept>
#include <iomanip>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/string_file.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/log/trivial.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <openssl/md5.h>

namespace pt = boost::property_tree;

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

#include <expat.h>
#include <Eigen/Dense>
#include "miniz_extension.hpp"
#include <fast_float/fast_float.h>

// Slightly faster than sprintf("%.9g"), but there is an issue with the karma floating point formatter,
// https://github.com/boostorg/spirit/pull/586
// where the exported string is one digit shorter than it should be to guarantee lossless round trip.
// The code is left here for the ocasion boost guys improve.
#define EXPORT_3MF_USE_SPIRIT_KARMA_FP 0

#define WRITE_ZIP_LANGUAGE_ENCODING 1

// Encode an UTF-8 string to the local code page.
static std::string encode_path(const char* src)
{
#ifdef WIN32
    // Convert the source utf8 encoded string to a wide string.
    std::wstring wstr_src = boost::nowide::widen(src);
    if (wstr_src.length() == 0)
        return std::string();
    // Convert a wide string to a local code page.
    int size_needed = ::WideCharToMultiByte(0, 0, wstr_src.data(), (int)wstr_src.size(), nullptr, 0, nullptr, nullptr);
    std::string str_dst(size_needed, 0);
    ::WideCharToMultiByte(0, 0, wstr_src.data(), (int)wstr_src.size(), str_dst.data(), size_needed, nullptr, nullptr);
    return str_dst;
#else /* WIN32 */
    return src;
#endif /* WIN32 */
}

// Encode an 8-bit string from a local code page to UTF-8.
// Multibyte to utf8
static std::string decode_path(const char* src)
{
#ifdef WIN32
    int len = int(strlen(src));
    if (len == 0)
        return std::string();
    // Convert the string encoded using the local code page to a wide string.
    int size_needed = ::MultiByteToWideChar(0, 0, src, len, nullptr, 0);
    std::wstring wstr_dst(size_needed, 0);
    ::MultiByteToWideChar(0, 0, src, len, wstr_dst.data(), size_needed);
    // Convert a wide string to utf8.
    return boost::nowide::narrow(wstr_dst.c_str());
#else /* WIN32 */
    return src;
#endif /* WIN32 */
}

// @see https://commons.apache.org/proper/commons-compress/apidocs/src-html/org/apache/commons/compress/archivers/zip/AbstractUnicodeExtraField.html
struct ZipUnicodePathExtraField
{
    static std::string encode(std::string const& u8path, std::string const& path) {
        std::string extra;
        if (u8path != path) {
            // 0x7075 - for Unicode filenames
            extra.push_back('\x75');
            extra.push_back('\x70');
            boost::uint16_t len = 5 + u8path.length();
            extra.push_back((char)(len & 0xff));
            extra.push_back((char)(len >> 8));
            auto crc = mz_crc32(0, (unsigned char *) path.c_str(), path.length());
            extra.push_back('\x01'); // version 1
            extra.append((char *)&crc, (char *)&crc + 4); // Little Endian
            extra.append(u8path);
        }
        return extra;
    }
    static std::string decode(std::string const& extra, std::string const& path = {}) {
        char const * p = extra.data();
        char const * e = p + extra.length();
        while (p + 4 < e) {
            boost::uint16_t len = ((boost::uint16_t)p[2]) | ((boost::uint16_t)p[3] << 8);
            if (p[0] == '\x75' && p[1] == '\x70' && len >= 5 && p + 4 + len < e && p[4] == '\x01') {
                return std::string(p + 9, p + 4 + len);
            }
            else {
                p += 4 + len;
            }
        }
        return decode_path(path.c_str());
    }
};

// VERSION NUMBERS
// 0 : .3mf, files saved by older slic3r or other applications. No version definition in them.
// 1 : Introduction of 3mf versioning. No other change in data saved into 3mf files.
// 2 : Volumes' matrices and source data added to Metadata/Slic3r_PE_model.config file, meshes transformed back to their coordinate system on loading.
// WARNING !! -> the version number has been rolled back to 1
//               the next change should use 3
const unsigned int VERSION_BBS_3MF = 1;
// Allow loading version 2 file as well.
const unsigned int VERSION_BBS_3MF_COMPATIBLE = 2;
const char* BBS_3MF_VERSION1 = "bamboo_slicer:Version3mf"; // definition of the metadata name saved into .model file
const char* BBS_3MF_VERSION = "BambuStudio:3mfVersion"; //compatible with prusa currently
// Painting gizmos data version numbers
// 0 : initial version of fdm, seam, mm
const unsigned int FDM_SUPPORTS_PAINTING_VERSION = 0;
const unsigned int SEAM_PAINTING_VERSION         = 0;
const unsigned int MM_PAINTING_VERSION           = 0;

const std::string BBS_FDM_SUPPORTS_PAINTING_VERSION = "BambuStudio:FdmSupportsPaintingVersion";
const std::string BBS_SEAM_PAINTING_VERSION         = "BambuStudio:SeamPaintingVersion";
const std::string BBS_MM_PAINTING_VERSION           = "BambuStudio:MmPaintingVersion";
const std::string BBL_MODEL_ID_TAG                  = "model_id";
const std::string BBL_MODEL_NAME_TAG                = "Title";
const std::string BBL_ORIGIN_TAG                    = "Origin";
const std::string BBL_DESIGNER_TAG                  = "Designer";
const std::string BBL_DESIGNER_USER_ID_TAG          = "DesignerUserId";
const std::string BBL_DESIGNER_COVER_FILE_TAG       = "DesignerCover";
const std::string BBL_DESCRIPTION_TAG               = "Description";
const std::string BBL_COPYRIGHT_TAG                 = "CopyRight";
const std::string BBL_COPYRIGHT_NORMATIVE_TAG       = "Copyright";
const std::string BBL_LICENSE_TAG                   = "License";
const std::string BBL_REGION_TAG                    = "Region";
const std::string BBL_MODIFICATION_TAG              = "ModificationDate";
const std::string BBL_CREATION_DATE_TAG             = "CreationDate";
const std::string BBL_APPLICATION_TAG               = "Application";

const std::string BBL_PROFILE_TITLE_TAG             = "ProfileTitle";
const std::string BBL_PROFILE_COVER_TAG             = "ProfileCover";
const std::string BBL_PROFILE_DESCRIPTION_TAG       = "ProfileDescription";
const std::string BBL_PROFILE_USER_ID_TAG           = "ProfileUserId";
const std::string BBL_PROFILE_USER_NAME_TAG         = "ProfileUserName";

const std::string MODEL_FOLDER = "3D/";
const std::string MODEL_EXTENSION = ".model";
const std::string MODEL_FILE = "3D/3dmodel.model"; // << this is the only format of the string which works with CURA
const std::string MODEL_RELS_FILE = "3D/_rels/3dmodel.model.rels";
//BBS: add metadata_folder
const std::string METADATA_DIR = "Metadata/";
const std::string ACCESOR_DIR = "accesories/";
const std::string GCODE_EXTENSION = ".gcode";
const std::string THUMBNAIL_EXTENSION = ".png";
const std::string CALIBRATION_INFO_EXTENSION = ".json";
const std::string CONTENT_TYPES_FILE = "[Content_Types].xml";
const std::string RELATIONSHIPS_FILE = "_rels/.rels";
const std::string THUMBNAIL_FILE = "Metadata/plate_1.png";
const std::string THUMBNAIL_FOR_PRINTER_FILE = "Metadata/bbl_thumbnail.png";
const std::string PRINTER_THUMBNAIL_SMALL_FILE = "/Auxiliaries/.thumbnails/thumbnail_small.png";
const std::string PRINTER_THUMBNAIL_MIDDLE_FILE = "/Auxiliaries/.thumbnails/thumbnail_middle.png";
const std::string _3MF_COVER_FILE = "/Auxiliaries/.thumbnails/thumbnail_3mf.png";
//const std::string PRINT_CONFIG_FILE = "Metadata/Slic3r_PE.config";
//const std::string MODEL_CONFIG_FILE = "Metadata/Slic3r_PE_model.config";
const std::string BBS_PRINT_CONFIG_FILE = "Metadata/print_profile.config";
const std::string BBS_PROJECT_CONFIG_FILE = "Metadata/project_settings.config";
const std::string BBS_MODEL_CONFIG_FILE = "Metadata/model_settings.config";
const std::string BBS_MODEL_CONFIG_RELS_FILE = "Metadata/_rels/model_settings.config.rels";
const std::string SLICE_INFO_CONFIG_FILE = "Metadata/slice_info.config";
const std::string BBS_LAYER_HEIGHTS_PROFILE_FILE = "Metadata/layer_heights_profile.txt";
const std::string LAYER_CONFIG_RANGES_FILE = "Metadata/layer_config_ranges.xml";
/*const std::string SLA_SUPPORT_POINTS_FILE = "Metadata/Slic3r_PE_sla_support_points.txt";
const std::string SLA_DRAIN_HOLES_FILE = "Metadata/Slic3r_PE_sla_drain_holes.txt";*/
const std::string CUSTOM_GCODE_PER_PRINT_Z_FILE = "Metadata/custom_gcode_per_layer.xml";
const std::string AUXILIARY_DIR = "Auxiliaries/";
const std::string PROJECT_EMBEDDED_PRINT_PRESETS_FILE = "Metadata/print_setting_";
const std::string PROJECT_EMBEDDED_SLICE_PRESETS_FILE = "Metadata/process_settings_";
const std::string PROJECT_EMBEDDED_FILAMENT_PRESETS_FILE = "Metadata/filament_settings_";
const std::string PROJECT_EMBEDDED_PRINTER_PRESETS_FILE = "Metadata/machine_settings_";
const std::string CUT_INFORMATION_FILE = "Metadata/cut_information.xml";

const unsigned int AUXILIARY_STR_LEN = 12;
const unsigned int METADATA_STR_LEN = 9;


static constexpr const char* MODEL_TAG = "model";
static constexpr const char* RESOURCES_TAG = "resources";
static constexpr const char* COLOR_GROUP_TAG = "m:colorgroup";
static constexpr const char* COLOR_TAG = "m:color";
static constexpr const char* OBJECT_TAG = "object";
static constexpr const char* MESH_TAG = "mesh";
static constexpr const char* MESH_STAT_TAG = "mesh_stat";
static constexpr const char* VERTICES_TAG = "vertices";
static constexpr const char* VERTEX_TAG = "vertex";
static constexpr const char* TRIANGLES_TAG = "triangles";
static constexpr const char* TRIANGLE_TAG = "triangle";
static constexpr const char* COMPONENTS_TAG = "components";
static constexpr const char* COMPONENT_TAG = "component";
static constexpr const char* BUILD_TAG = "build";
static constexpr const char* ITEM_TAG = "item";
static constexpr const char* METADATA_TAG = "metadata";
static constexpr const char* FILAMENT_TAG = "filament";
static constexpr const char* SLICE_WARNING_TAG = "warning";
static constexpr const char* WARNING_MSG_TAG = "msg";
static constexpr const char *FILAMENT_ID_TAG   = "id";
static constexpr const char* FILAMENT_TYPE_TAG = "type";
static constexpr const char *FILAMENT_COLOR_TAG = "color";
static constexpr const char *FILAMENT_USED_M_TAG = "used_m";
static constexpr const char *FILAMENT_USED_G_TAG = "used_g";


static constexpr const char* CONFIG_TAG = "config";
static constexpr const char* VOLUME_TAG = "volume";
static constexpr const char* PART_TAG = "part";
static constexpr const char* PLATE_TAG = "plate";
static constexpr const char* INSTANCE_TAG = "model_instance";
//BBS
static constexpr const char* ASSEMBLE_TAG = "assemble";
static constexpr const char* ASSEMBLE_ITEM_TAG = "assemble_item";
static constexpr const char* SLICE_HEADER_TAG = "header";
static constexpr const char* SLICE_HEADER_ITEM_TAG = "header_item";

// text_info
static constexpr const char* TEXT_INFO_TAG        = "text_info";
static constexpr const char* TEXT_ATTR            = "text";
static constexpr const char* FONT_NAME_ATTR       = "font_name";
static constexpr const char* FONT_INDEX_ATTR      = "font_index";
static constexpr const char* FONT_SIZE_ATTR       = "font_size";
static constexpr const char* THICKNESS_ATTR       = "thickness";
static constexpr const char* EMBEDED_DEPTH_ATTR   = "embeded_depth";
static constexpr const char* ROTATE_ANGLE_ATTR    = "rotate_angle";
static constexpr const char* TEXT_GAP_ATTR        = "text_gap";
static constexpr const char* BOLD_ATTR            = "bold";
static constexpr const char* ITALIC_ATTR          = "italic";
static constexpr const char* SURFACE_TEXT_ATTR    = "surface_text";
static constexpr const char* KEEP_HORIZONTAL_ATTR = "keep_horizontal";
static constexpr const char* HIT_MESH_ATTR        = "hit_mesh";
static constexpr const char* HIT_POSITION_ATTR    = "hit_position";
static constexpr const char* HIT_NORMAL_ATTR      = "hit_normal";

// BBS: encrypt
static constexpr const char* RELATIONSHIP_TAG = "Relationship";
static constexpr const char* PID_ATTR = "pid";
static constexpr const char* PUUID_ATTR = "p:UUID";
static constexpr const char* PUUID_LOWER_ATTR = "p:uuid";
static constexpr const char* PPATH_ATTR = "p:path";
static constexpr const char *OBJECT_UUID_SUFFIX = "-61cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char *OBJECT_UUID_SUFFIX2 = "-71cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char *SUB_OBJECT_UUID_SUFFIX = "-81cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char *COMPONENT_UUID_SUFFIX = "-b206-40ff-9872-83e8017abed1";
static constexpr const char* BUILD_UUID = "2c7c17d8-22b5-4d84-8835-1976022ea369";
static constexpr const char* BUILD_UUID_SUFFIX = "-b1ec-4553-aec9-835e5b724bb4";
static constexpr const char* TARGET_ATTR = "Target";
static constexpr const char* RELS_TYPE_ATTR = "Type";

static constexpr const char* UNIT_ATTR = "unit";
static constexpr const char* NAME_ATTR = "name";
static constexpr const char* COLOR_ATTR = "color";
static constexpr const char* TYPE_ATTR = "type";
static constexpr const char* ID_ATTR = "id";
static constexpr const char* X_ATTR = "x";
static constexpr const char* Y_ATTR = "y";
static constexpr const char* Z_ATTR = "z";
static constexpr const char* V1_ATTR = "v1";
static constexpr const char* V2_ATTR = "v2";
static constexpr const char* V3_ATTR = "v3";
static constexpr const char* OBJECTID_ATTR = "objectid";
static constexpr const char* TRANSFORM_ATTR = "transform";
// BBS
static constexpr const char* OFFSET_ATTR = "offset";
static constexpr const char* PRINTABLE_ATTR = "printable";
static constexpr const char* INSTANCESCOUNT_ATTR = "instances_count";
static constexpr const char* CUSTOM_SUPPORTS_ATTR = "paint_supports";
static constexpr const char* CUSTOM_SEAM_ATTR = "paint_seam";
static constexpr const char* MMU_SEGMENTATION_ATTR = "paint_color";
// BBS
static constexpr const char* FACE_PROPERTY_ATTR = "face_property";

static constexpr const char* KEY_ATTR = "key";
static constexpr const char* VALUE_ATTR = "value";
static constexpr const char* FIRST_TRIANGLE_ID_ATTR = "firstid";
static constexpr const char* LAST_TRIANGLE_ID_ATTR = "lastid";
static constexpr const char* SUBTYPE_ATTR = "subtype";
static constexpr const char* LOCK_ATTR = "locked";
static constexpr const char* BED_TYPE_ATTR = "bed_type";
static constexpr const char* PRINT_SEQUENCE_ATTR = "print_sequence";
static constexpr const char* FIRST_LAYER_PRINT_SEQUENCE_ATTR = "first_layer_print_sequence";
static constexpr const char* SPIRAL_VASE_MODE = "spiral_mode";
static constexpr const char* GCODE_FILE_ATTR = "gcode_file";
static constexpr const char* THUMBNAIL_FILE_ATTR = "thumbnail_file";
static constexpr const char* TOP_FILE_ATTR = "top_file";
static constexpr const char* PICK_FILE_ATTR = "pick_file";
static constexpr const char* PATTERN_FILE_ATTR = "pattern_file";
static constexpr const char* PATTERN_BBOX_FILE_ATTR = "pattern_bbox_file";
static constexpr const char* OBJECT_ID_ATTR = "object_id";
static constexpr const char* INSTANCEID_ATTR = "instance_id";
static constexpr const char* IDENTIFYID_ATTR = "identify_id";
static constexpr const char* PLATERID_ATTR = "plater_id";
static constexpr const char* PLATER_NAME_ATTR  = "plater_name";
static constexpr const char* PLATE_IDX_ATTR = "index";
static constexpr const char* PRINTER_MODEL_ID_ATTR = "printer_model_id";
static constexpr const char* NOZZLE_DIAMETERS_ATTR = "nozzle_diameters";
static constexpr const char* SLICE_PREDICTION_ATTR = "prediction";
static constexpr const char* SLICE_WEIGHT_ATTR = "weight";
static constexpr const char* TIMELAPSE_TYPE_ATTR = "timelapse_type";
static constexpr const char* TIMELAPSE_ERROR_CODE_ATTR = "timelapse_error_code";
static constexpr const char* OUTSIDE_ATTR = "outside";
static constexpr const char* SUPPORT_USED_ATTR = "support_used";
static constexpr const char* LABEL_OBJECT_ENABLED_ATTR = "label_object_enabled";
static constexpr const char* SKIPPED_ATTR = "skipped";

static constexpr const char* OBJECT_TYPE = "object";
static constexpr const char* VOLUME_TYPE = "volume";
static constexpr const char* PART_TYPE = "part";

static constexpr const char* NAME_KEY = "name";
static constexpr const char* VOLUME_TYPE_KEY = "volume_type";
static constexpr const char* PART_TYPE_KEY = "part_type";
static constexpr const char* MATRIX_KEY = "matrix";
static constexpr const char* SOURCE_FILE_KEY = "source_file";
static constexpr const char* SOURCE_OBJECT_ID_KEY = "source_object_id";
static constexpr const char* SOURCE_VOLUME_ID_KEY = "source_volume_id";
static constexpr const char* SOURCE_OFFSET_X_KEY = "source_offset_x";
static constexpr const char* SOURCE_OFFSET_Y_KEY = "source_offset_y";
static constexpr const char* SOURCE_OFFSET_Z_KEY = "source_offset_z";
static constexpr const char* SOURCE_IN_INCHES    = "source_in_inches";
static constexpr const char* SOURCE_IN_METERS    = "source_in_meters";

static constexpr const char* MESH_SHARED_KEY = "mesh_shared";

static constexpr const char* MESH_STAT_EDGES_FIXED          = "edges_fixed";
static constexpr const char* MESH_STAT_DEGENERATED_FACETS   = "degenerate_facets";
static constexpr const char* MESH_STAT_FACETS_REMOVED       = "facets_removed";
static constexpr const char* MESH_STAT_FACETS_RESERVED      = "facets_reversed";
static constexpr const char* MESH_STAT_BACKWARDS_EDGES      = "backwards_edges";


const unsigned int BBS_VALID_OBJECT_TYPES_COUNT = 2;
const char* BBS_VALID_OBJECT_TYPES[] =
{
    "model",
    "other"
};

const char* BBS_INVALID_OBJECT_TYPES[] =
{
    "solidsupport",
    "support",
    "surface"
};

template <typename T>
struct hex_wrap
{
    T t;
};

namespace std {
    template <class _Elem, class _Traits, class _Arg>
    basic_ostream<_Elem, _Traits>& operator<<(basic_ostream<_Elem, _Traits>& ostr,
        const hex_wrap<_Arg>& wrap) { // insert by calling function with output stream and argument
        auto of = ostr.fill('0');
        ostr << setw(sizeof(_Arg) * 2) << std::hex << wrap.t;
        ostr << std::dec << setw(0);
        ostr.fill(of);
        return ostr;
    }
}

class version_error : public Slic3r::FileIOError
{
public:
    version_error(const std::string& what_arg) : Slic3r::FileIOError(what_arg) {}
    version_error(const char* what_arg) : Slic3r::FileIOError(what_arg) {}
};

const char* bbs_get_attribute_value_charptr(const char** attributes, unsigned int attributes_size, const char* attribute_key)
{
    if ((attributes == nullptr) || (attributes_size == 0) || (attributes_size % 2 != 0) || (attribute_key == nullptr))
        return nullptr;

    for (unsigned int a = 0; a < attributes_size; a += 2) {
        if (::strcmp(attributes[a], attribute_key) == 0)
            return attributes[a + 1];
    }

    return nullptr;
}

std::string bbs_get_attribute_value_string(const char** attributes, unsigned int attributes_size, const char* attribute_key)
{
    const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
    return (text != nullptr) ? text : "";
}

float bbs_get_attribute_value_float(const char** attributes, unsigned int attributes_size, const char* attribute_key)
{
    float value = 0.0f;
    if (const char *text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key); text != nullptr)
        fast_float::from_chars(text, text + strlen(text), value);
    return value;
}

int bbs_get_attribute_value_int(const char** attributes, unsigned int attributes_size, const char* attribute_key)
{
    int value = 0;
    if (const char *text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key); text != nullptr)
        boost::spirit::qi::parse(text, text + strlen(text), boost::spirit::qi::int_, value);
    return value;
}

bool bbs_get_attribute_value_bool(const char** attributes, unsigned int attributes_size, const char* attribute_key)
{
    const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
    return (text != nullptr) ? (bool)::atoi(text) : true;
}

void add_vec3(std::stringstream &stream, const Slic3r::Vec3f &tr)
{
    for (unsigned r = 0; r < 3; ++r) {
        stream << tr(r);
        if (r != 2)
            stream << " ";
    }
}

Slic3r::Vec3f get_vec3_from_string(const std::string &pos_str)
{
    Slic3r::Vec3f pos(0, 0, 0);
    if (pos_str.empty())
        return pos;

    std::vector<std::string> values;
    boost::split(values, pos_str, boost::is_any_of(" "), boost::token_compress_on);

    if (values.size() != 3)
        return pos;

    for (int i = 0; i < 3; ++i)
        pos(i) = ::atof(values[i].c_str());

    return pos;
}

Slic3r::Transform3d bbs_get_transform_from_3mf_specs_string(const std::string& mat_str)
{
    // check: https://3mf.io/3d-manufacturing-format/ or https://github.com/3MFConsortium/spec_core/blob/master/3MF%20Core%20Specification.md
    // to see how matrices are stored inside 3mf according to specifications
    Slic3r::Transform3d ret = Slic3r::Transform3d::Identity();

    if (mat_str.empty())
        // empty string means default identity matrix
        return ret;

    std::vector<std::string> mat_elements_str;
    boost::split(mat_elements_str, mat_str, boost::is_any_of(" "), boost::token_compress_on);

    unsigned int size = (unsigned int)mat_elements_str.size();
    if (size != 12)
        // invalid data, return identity matrix
        return ret;

    unsigned int i = 0;
    // matrices are stored into 3mf files as 4x3
    // we need to transpose them
    for (unsigned int c = 0; c < 4; ++c) {
        for (unsigned int r = 0; r < 3; ++r) {
            ret(r, c) = ::atof(mat_elements_str[i++].c_str());
        }
    }
    return ret;
}

Slic3r::Vec3d bbs_get_offset_from_3mf_specs_string(const std::string& vec_str)
{
    Slic3r::Vec3d ofs2ass(0, 0, 0);

    if (vec_str.empty())
        // empty string means default zero offset
        return ofs2ass;

    std::vector<std::string> vec_elements_str;
    boost::split(vec_elements_str, vec_str, boost::is_any_of(" "), boost::token_compress_on);

    unsigned int size = (unsigned int)vec_elements_str.size();
    if (size != 3)
        // invalid data, return zero offset
        return ofs2ass;

    for (unsigned int i = 0; i < 3; i++) {
        ofs2ass(i) = ::atof(vec_elements_str[i].c_str());
    }

    return ofs2ass;
}

float bbs_get_unit_factor(const std::string& unit)
{
    const char* text = unit.c_str();

    if (::strcmp(text, "micron") == 0)
        return 0.001f;
    else if (::strcmp(text, "centimeter") == 0)
        return 10.0f;
    else if (::strcmp(text, "inch") == 0)
        return 25.4f;
    else if (::strcmp(text, "foot") == 0)
        return 304.8f;
    else if (::strcmp(text, "meter") == 0)
        return 1000.0f;
    else
        // default "millimeters" (see specification)
        return 1.0f;
}

bool bbs_is_valid_object_type(const std::string& type)
{
    // if the type is empty defaults to "model" (see specification)
    if (type.empty())
        return true;

    for (unsigned int i = 0; i < BBS_VALID_OBJECT_TYPES_COUNT; ++i) {
        if (::strcmp(type.c_str(), BBS_VALID_OBJECT_TYPES[i]) == 0)
            return true;
    }

    return false;
}

namespace Slic3r {

    static std::string xml_unescape(std::string s)
    {
        std::string ret;
        std::string::size_type i = 0;
        std::string::size_type pos = 0;
        while (i < s.size()) {
            std::string rep;
            if (s[i] == '&') {
                if (s.substr(i, 4) == "&lt;") {
                    ret += s.substr(pos, i - pos) + "<";
                    i += 4;
                    pos = i;
                }
                else if (s.substr(i, 4) == "&gt;") {
                    ret += s.substr(pos, i - pos) + ">";
                    i += 4;
                    pos = i;
                }
                else if (s.substr(i, 5) == "&amp;") {
                    ret += s.substr(pos, i - pos) + "&";
                    i += 5;
                    pos = i;
                }
                else {
                    ++i;
                }
            }
            else {
                ++i;
            }
        }

        ret += s.substr(pos);
        return ret;
    }

    static mz_uint mz_zip_reader_get_extra(mz_zip_archive* pZip, mz_uint file_index, char* pExtra, mz_uint extra_buf_size)
    {
        mz_zip_archive_file_stat file_stat;
        mz_uint extra_len;

        // Ensure the file index is within bounds
        if (file_index >= mz_zip_reader_get_num_files(pZip))
        {
            if (extra_buf_size)
                pExtra[0] = '\0';
            return 0;
        }

        // Get file stat for the file index
        if (!mz_zip_reader_file_stat(pZip, file_index, &file_stat))
        {
            if (extra_buf_size)
                pExtra[0] = '\0';
            return 0;
        }

        // Get the extra field length
        extra_len = 0;

        if (extra_buf_size)
        {
            // Ensure we don't read more than the buffer can hold
            extra_len = extra_buf_size - 1;

            // Read the extra field data
            if (!mz_zip_reader_extract_file_to_mem(pZip, file_stat.m_filename, pExtra, extra_len, 0))
            {
                pExtra[0] = '\0';
                return 0;
            }

            // Null-terminate the extra field data
            pExtra[extra_len] = '\0';
        }

        // Return the length of the extra field data plus null terminator
        return extra_len + 1;
    }

//! macro used to mark string used at localization,
//! return same string
#define L(s) (s)
#define _(s) Slic3r::I18N::translate(s)

    // Base class with error messages management
    class _BBS_3MF_Base
    {
        mutable boost::mutex mutex;
        mutable std::vector<std::string> m_errors;

    protected:
        void add_error(const std::string& error) const { boost::unique_lock l(mutex); m_errors.push_back(error); }
        void clear_errors() { m_errors.clear(); }

    public:
        void log_errors()
        {
            for (const std::string& error : m_errors)
                BOOST_LOG_TRIVIAL(error) << error;
        }
    };

    class _BBS_3MF_Importer : public _BBS_3MF_Base
    {
        typedef std::pair<std::string, int> Id; // BBS: encrypt

        struct Component
        {
            Id object_id;
            Transform3d transform;

            explicit Component(Id object_id)
                : object_id(object_id)
                , transform(Transform3d::Identity())
            {
            }

            Component(Id object_id, const Transform3d& transform)
                : object_id(object_id)
                , transform(transform)
            {
            }
        };

        typedef std::vector<Component> ComponentsList;

        struct Geometry
        {
            std::vector<Vec3f> vertices;
            std::vector<Vec3i> triangles;
            std::vector<std::string> custom_supports;
            std::vector<std::string> custom_seam;
            std::vector<std::string> mmu_segmentation;
            // BBS
            std::vector<std::string> face_properties;

            bool empty() { return vertices.empty() || triangles.empty(); }

            // backup & restore
            void swap(Geometry& o) {
                std::swap(vertices, o.vertices);
                std::swap(triangles, o.triangles);
                std::swap(custom_supports, o.custom_supports);
                std::swap(custom_seam, o.custom_seam);
            }

            void reset() {
                vertices.clear();
                triangles.clear();
                custom_supports.clear();
                custom_seam.clear();
                mmu_segmentation.clear();
            }
        };

        struct CurrentObject
        {
            // ID of the object inside the 3MF file, 1 based.
            int id;
            // Index of the ModelObject in its respective Model, zero based.
            int model_object_idx;
            Geometry geometry;
            ModelObject* object;
            ComponentsList components;

            //BBS: sub object id
            //int subobject_id;
            std::string name;
            std::string uuid;
            int         pid{-1};
            //bool is_model_object;

            CurrentObject() { reset(); }

            void reset() {
                id = -1;
                model_object_idx = -1;
                geometry.reset();
                object = nullptr;
                components.clear();
                //BBS: sub object id
                uuid.clear();
                name.clear();
            }
        };

        struct CurrentConfig
        {
            int object_id {-1};
            int volume_id {-1};
        };

        struct CurrentInstance
        {
            int object_id;
            int instance_id;
            int identify_id;
        };

        struct Instance
        {
            ModelInstance* instance;
            Transform3d transform;

            Instance(ModelInstance* instance, const Transform3d& transform)
                : instance(instance)
                , transform(transform)
            {
            }
        };

        struct Metadata
        {
            std::string key;
            std::string value;

            Metadata(const std::string& key, const std::string& value)
                : key(key)
                , value(value)
            {
            }
        };

        typedef std::vector<Metadata> MetadataList;

        struct ObjectMetadata
        {
            struct VolumeMetadata
            {
                //BBS: refine the part logic
                unsigned int first_triangle_id;
                unsigned int last_triangle_id;
                int subobject_id;
                MetadataList metadata;
                RepairedMeshErrors mesh_stats;
                ModelVolumeType part_type;

                VolumeMetadata(unsigned int first_triangle_id, unsigned int last_triangle_id, ModelVolumeType type = ModelVolumeType::MODEL_PART)
                    : first_triangle_id(first_triangle_id)
                    , last_triangle_id(last_triangle_id)
                    , part_type(type)
                    , subobject_id(-1)
                {
                }

                VolumeMetadata(int sub_id, ModelVolumeType type = ModelVolumeType::MODEL_PART)
                    : subobject_id(sub_id)
                    , part_type(type)
                    , first_triangle_id(0)
                    , last_triangle_id(0)
                {
                }
            };

            typedef std::vector<VolumeMetadata> VolumeMetadataList;

            MetadataList metadata;
            VolumeMetadataList volumes;
        };

        struct CutObjectInfo
        {
            struct Connector
            {
                int   volume_id;
                int   type;
                float radius;
                float height;
                float r_tolerance;
                float h_tolerance;
            };
            CutObjectBase          id;
            std::vector<Connector> connectors;
        };

        // Map from a 1 based 3MF object ID to a 0 based ModelObject index inside m_model->objects.
        //typedef std::pair<std::string, int> Id; // BBS: encrypt
        typedef std::map<Id, CurrentObject> IdToCurrentObjectMap;
        typedef std::map<int, std::string> IndexToPathMap;
        typedef std::map<Id, int> IdToModelObjectMap;
        //typedef std::map<Id, ComponentsList> IdToAliasesMap;
        typedef std::vector<Instance> InstancesList;
        typedef std::map<int, ObjectMetadata> IdToMetadataMap;
        typedef std::map<int, CutObjectInfo>  IdToCutObjectInfoMap;
        //typedef std::map<Id, Geometry> IdToGeometryMap;
        typedef std::map<int, std::vector<coordf_t>> IdToLayerHeightsProfileMap;
        typedef std::map<int, t_layer_config_ranges> IdToLayerConfigRangesMap;
        /*typedef std::map<int, std::vector<sla::SupportPoint>> IdToSlaSupportPointsMap;
        typedef std::map<int, std::vector<sla::DrainHole>> IdToSlaDrainHolesMap;*/

        struct ObjectImporter
        {
            IdToCurrentObjectMap object_list;
            CurrentObject *current_object{nullptr};
            std::string object_path;
            std::string zip_path;
            _BBS_3MF_Importer *top_importer{nullptr};
            XML_Parser object_xml_parser;
            bool obj_parse_error { false };
            std::string obj_parse_error_message;

            //local parsed datas
            std::string obj_curr_metadata_name;
            std::string obj_curr_characters;
            float object_unit_factor;
            int object_current_color_group{-1};
            std::map<int, std::string> object_group_id_to_color;
            bool is_bbl_3mf { false };

            ObjectImporter(_BBS_3MF_Importer *importer, std::string file_path, std::string obj_path)
            {
                top_importer = importer;
                object_path = obj_path;
                zip_path = file_path;
            }

            ~ObjectImporter()
            {
                _destroy_object_xml_parser();
            }

            void _destroy_object_xml_parser()
            {
                if (object_xml_parser != nullptr) {
                    XML_ParserFree(object_xml_parser);
                    object_xml_parser = nullptr;
                }
            }

            void _stop_object_xml_parser(const std::string& msg = std::string())
            {
                assert(! obj_parse_error);
                assert(obj_parse_error_message.empty());
                assert(object_xml_parser != nullptr);
                obj_parse_error = true;
                obj_parse_error_message = msg;
                XML_StopParser(object_xml_parser, false);
            }

            bool        object_parse_error()         const { return obj_parse_error; }
            const char* object_parse_error_message() const {
                return obj_parse_error ?
                    // The error was signalled by the user code, not the expat parser.
                    (obj_parse_error_message.empty() ? "Invalid 3MF format" : obj_parse_error_message.c_str()) :
                    // The error was signalled by the expat parser.
                    XML_ErrorString(XML_GetErrorCode(object_xml_parser));
            }

            bool _extract_object_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

            bool extract_object_model()
            {
                mz_zip_archive archive;
                mz_zip_archive_file_stat stat;
                mz_zip_zero_struct(&archive);

                if (!open_zip_reader(&archive, zip_path)) {
                    top_importer->add_error("Unable to open the zipfile "+ zip_path);
                    return false;
                }

                if (!top_importer->_extract_from_archive(archive, object_path, [this] (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_object_from_archive(archive, stat);
                }, top_importer->m_load_restore)) {
                    std::string error_msg = std::string("Archive does not contain a valid model for ") + object_path;
                    top_importer->add_error(error_msg);

                    close_zip_reader(&archive);
                    return false;
                }

                close_zip_reader(&archive);

                if (obj_parse_error) {
                    //already add_error inside
                    //top_importer->add_error(object_parse_error_message());
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Found error while extrace object %1%\n")%object_path;
                    return false;
                }
                return true;
            }

            bool _handle_object_start_model(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_model();

            bool _handle_object_start_resources(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_resources();

            bool _handle_object_start_object(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_object();

            bool _handle_object_start_color_group(const char **attributes, unsigned int num_attributes);
            bool _handle_object_end_color_group();

            bool _handle_object_start_color(const char **attributes, unsigned int num_attributes);
            bool _handle_object_end_color();

            bool _handle_object_start_mesh(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_mesh();

            bool _handle_object_start_vertices(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_vertices();

            bool _handle_object_start_vertex(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_vertex();

            bool _handle_object_start_triangles(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_triangles();

            bool _handle_object_start_triangle(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_triangle();

            bool _handle_object_start_components(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_components();

            bool _handle_object_start_component(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_component();

            bool _handle_object_start_metadata(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_metadata();

            void _handle_object_start_model_xml_element(const char* name, const char** attributes);
            void _handle_object_end_model_xml_element(const char* name);
            void _handle_object_xml_characters(const XML_Char* s, int len);

            // callbacks to parse the .model file of an object
            static void XMLCALL _handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes);
            static void XMLCALL _handle_object_end_model_xml_element(void* userData, const char* name);
            static void XMLCALL _handle_object_xml_characters(void* userData, const XML_Char* s, int len);
        };

        // Version of the 3mf file
        unsigned int m_version;
        bool m_check_version = false;
        bool m_load_model = false;
        bool m_load_aux = false;
        bool m_load_config = false;
        // backup & restore
        bool m_load_restore = false;
        std::string m_backup_path;
        std::string m_origin_file;
        // Semantic version of Bambu Studio, that generated this 3MF.
        boost::optional<Semver> m_bambuslicer_generator_version;
        unsigned int m_fdm_supports_painting_version = 0;
        unsigned int m_seam_painting_version         = 0;
        unsigned int m_mm_painting_version           = 0;
        std::string  m_model_id;
        std::string  m_contry_code;
        std::string  m_designer;
        std::string  m_designer_user_id;
        std::string  m_designer_cover;
        std::string  m_profile_title;
        std::string  m_profile_cover;
        std::string  m_Profile_description;
        std::string  m_profile_user_id;
        std::string  m_profile_user_name;

        XML_Parser m_xml_parser;
        // Error code returned by the application side of the parser. In that case the expat may not reliably deliver the error state
        // after returning from XML_Parse() function, thus we keep the error state here.
        bool m_parse_error { false };
        std::string m_parse_error_message;
        Model* m_model;
        float m_unit_factor;
        CurrentObject* m_curr_object{nullptr};
        IdToCurrentObjectMap m_current_objects;
        IndexToPathMap       m_index_paths;
        IdToModelObjectMap m_objects;
        //IdToAliasesMap m_objects_aliases;
        InstancesList m_instances;
        //IdToGeometryMap m_geometries;
        //IdToGeometryMap m_orig_geometries; // backup & restore
        CurrentConfig m_curr_config;
        IdToMetadataMap m_objects_metadata;
        IdToCutObjectInfoMap       m_cut_object_infos;
        IdToLayerHeightsProfileMap m_layer_heights_profiles;
        IdToLayerConfigRangesMap m_layer_config_ranges;
        /*IdToSlaSupportPointsMap m_sla_support_points;
        IdToSlaDrainHolesMap    m_sla_drain_holes;*/
        std::string m_curr_metadata_name;
        std::string m_curr_characters;
        std::string m_name;
        std::string m_sub_model_path;

        std::string m_start_part_path;
        std::string m_thumbnail_path;
        std::string m_thumbnail_middle;
        std::string m_thumbnail_small;
        std::vector<std::string> m_sub_model_paths;
        std::vector<ObjectImporter*> m_object_importers;

        std::map<int, ModelVolume*> m_shared_meshes;

        //BBS: plater related structures
        bool m_is_bbl_3mf { false };
        bool m_parsing_slice_info { false };
        CurrentInstance m_curr_instance;

        int m_current_color_group{-1};
        std::map<int, std::string> m_group_id_to_color;

    public:
        _BBS_3MF_Importer();
        ~_BBS_3MF_Importer();

        //BBS: add plate data related logic
        // add backup & restore logic
        bool load_model_from_file(const std::string& filename, Model& model, DynamicPrintConfig& config,
            ConfigSubstitutionContext& config_substitutions, LoadStrategy strategy, bool& is_bbl_3mf, Semver& file_version, Import3mfProgressFn proFn = nullptr, int plate_id = 0);
        bool get_thumbnail(const std::string &filename, std::string &data);
        bool load_gcode_3mf_from_stream(std::istream & data, Model& model, DynamicPrintConfig& config, Semver& file_version);
        unsigned int version() const { return m_version; }

    private:
        void _destroy_xml_parser();
        void _stop_xml_parser(const std::string& msg = std::string());

        bool        parse_error()         const { return m_parse_error; }
        const char* parse_error_message() const {
            return m_parse_error ?
                // The error was signalled by the user code, not the expat parser.
                (m_parse_error_message.empty() ? "Invalid 3MF format" : m_parse_error_message.c_str()) :
                // The error was signalled by the expat parser.
                XML_ErrorString(XML_GetErrorCode(m_xml_parser));
        }

        //BBS: add plate data related logic
        // add backup & restore logic
        bool _load_model_from_file(std::string filename, Model& model, DynamicPrintConfig& config, ConfigSubstitutionContext& config_substitutions, Import3mfProgressFn proFn = nullptr, int plate_id = 0);
        bool _extract_from_archive(mz_zip_archive& archive, std::string const & path, std::function<bool (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)>, bool restore = false);
        bool _extract_xml_from_archive(mz_zip_archive& archive, std::string const & path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);
        bool _extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);
        bool _extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_cut_information_from_archive(mz_zip_archive &archive, const mz_zip_archive_file_stat &stat, ConfigSubstitutionContext &config_substitutions);
        void _extract_layer_heights_profile_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_layer_config_ranges_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, ConfigSubstitutionContext& config_substitutions);
        void _extract_sla_support_points_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_sla_drain_holes_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        void _extract_custom_gcode_per_print_z_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        void _extract_print_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, DynamicPrintConfig& config, ConfigSubstitutionContext& subs_context, const std::string& archive_filename);

        void _extract_auxiliary_file_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, Model& model);
        void _extract_file_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        // handlers to parse the .model file
        void _handle_start_model_xml_element(const char* name, const char** attributes);
        void _handle_end_model_xml_element(const char* name);
        void _handle_xml_characters(const XML_Char* s, int len);

        // handlers to parse the MODEL_CONFIG_FILE file
        void _handle_start_config_xml_element(const char* name, const char** attributes);
        void _handle_end_config_xml_element(const char* name);

        bool _handle_start_model(const char** attributes, unsigned int num_attributes);
        bool _handle_end_model();

        bool _handle_start_resources(const char** attributes, unsigned int num_attributes);
        bool _handle_end_resources();

        bool _handle_start_object(const char** attributes, unsigned int num_attributes);
        bool _handle_end_object();

        bool _handle_start_color_group(const char **attributes, unsigned int num_attributes);
        bool _handle_end_color_group();

        bool _handle_start_color(const char **attributes, unsigned int num_attributes);
        bool _handle_end_color();

        bool _handle_start_mesh(const char** attributes, unsigned int num_attributes);
        bool _handle_end_mesh();

        bool _handle_start_vertices(const char** attributes, unsigned int num_attributes);
        bool _handle_end_vertices();

        bool _handle_start_vertex(const char** attributes, unsigned int num_attributes);
        bool _handle_end_vertex();

        bool _handle_start_triangles(const char** attributes, unsigned int num_attributes);
        bool _handle_end_triangles();

        bool _handle_start_triangle(const char** attributes, unsigned int num_attributes);
        bool _handle_end_triangle();

        bool _handle_start_components(const char** attributes, unsigned int num_attributes);
        bool _handle_end_components();

        bool _handle_start_component(const char** attributes, unsigned int num_attributes);
        bool _handle_end_component();

        bool _handle_start_build(const char** attributes, unsigned int num_attributes);
        bool _handle_end_build();

        bool _handle_start_item(const char** attributes, unsigned int num_attributes);
        bool _handle_end_item();

        bool _handle_start_metadata(const char** attributes, unsigned int num_attributes);
        bool _handle_end_metadata();

        bool _create_object_instance(std::string const & path, int object_id, const Transform3d& transform, const bool printable, unsigned int recur_counter);

        void _apply_transform(ModelInstance& instance, const Transform3d& transform);

        bool _handle_start_config(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config();

        bool _handle_start_config_object(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_object();

        bool _handle_start_config_volume(const char** attributes, unsigned int num_attributes);
        bool _handle_start_config_volume_mesh(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_volume();
        bool _handle_end_config_volume_mesh();

        bool _handle_start_config_metadata(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_metadata();

        bool _handle_start_config_filament(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_filament();

        bool _handle_start_config_warning(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_warning();

        //BBS: add plater config parse functions
        bool _handle_start_config_plater(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_plater();

        bool _handle_start_config_plater_instance(const char** attributes, unsigned int num_attributes);
        bool _handle_end_config_plater_instance();

        bool _handle_start_assemble(const char** attributes, unsigned int num_attributes);
        bool _handle_end_assemble();

        bool _handle_start_assemble_item(const char** attributes, unsigned int num_attributes);
        bool _handle_end_assemble_item();

        bool _handle_start_text_info_item(const char **attributes, unsigned int num_attributes);
        bool _handle_end_text_info_item();

        // BBS: callbacks to parse the .rels file
        static void XMLCALL _handle_start_relationships_element(void* userData, const char* name, const char** attributes);
        static void XMLCALL _handle_end_relationships_element(void* userData, const char* name);

        void _handle_start_relationships_element(const char* name, const char** attributes);
        void _handle_end_relationships_element(const char* name);

        bool _handle_start_relationship(const char** attributes, unsigned int num_attributes);

        void _generate_current_object_list(std::vector<Component> &sub_objects, Id object_id, IdToCurrentObjectMap& current_objects);
        bool _generate_volumes_new(ModelObject& object, const std::vector<Component> &sub_objects, const ObjectMetadata::VolumeMetadataList& volumes, ConfigSubstitutionContext& config_substitutions);
        bool _generate_volumes(ModelObject& object, const Geometry& geometry, const ObjectMetadata::VolumeMetadataList& volumes, ConfigSubstitutionContext& config_substitutions);

        // callbacks to parse the .model file
        static void XMLCALL _handle_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void XMLCALL _handle_end_model_xml_element(void* userData, const char* name);
        static void XMLCALL _handle_xml_characters(void* userData, const XML_Char* s, int len);

        // callbacks to parse the MODEL_CONFIG_FILE file
        static void XMLCALL _handle_start_config_xml_element(void* userData, const char* name, const char** attributes);
        static void XMLCALL _handle_end_config_xml_element(void* userData, const char* name);
    };

    _BBS_3MF_Importer::_BBS_3MF_Importer()
        : m_version(0)
        , m_check_version(false)
        , m_xml_parser(nullptr)
        , m_model(nullptr)
        , m_unit_factor(1.0f)
        , m_curr_metadata_name("")
        , m_curr_characters("")
        , m_name("")
    {
    }

    _BBS_3MF_Importer::~_BBS_3MF_Importer()
    {
        _destroy_xml_parser();
        clear_errors();

        if (m_curr_object) {
            delete m_curr_object;
            m_curr_object = nullptr;
        }
        m_current_objects.clear();
        m_index_paths.clear();
        m_objects.clear();
        m_instances.clear();
        m_objects_metadata.clear();
        m_curr_metadata_name.clear();
        m_curr_characters.clear();
    }

    //BBS: add plate data related logic
        // add backup & restore logic
    bool _BBS_3MF_Importer::load_model_from_file(const std::string& filename, Model& model, DynamicPrintConfig& config,
        ConfigSubstitutionContext& config_substitutions, LoadStrategy strategy, bool& is_bbl_3mf, Semver& file_version, Import3mfProgressFn proFn, int plate_id)
    {
        m_version = 0;
        m_fdm_supports_painting_version = 0;
        m_seam_painting_version = 0;
        m_mm_painting_version = 0;
        m_check_version = strategy & LoadStrategy::CheckVersion;
        //BBS: auxiliary data
        m_load_model  = strategy & LoadStrategy::LoadModel;
        m_load_aux = strategy & LoadStrategy::LoadAuxiliary;
        m_load_restore = strategy & LoadStrategy::Restore;
        m_load_config = strategy & LoadStrategy::LoadConfig;
        m_model = &model;
        m_unit_factor = 1.0f;
        m_curr_object = nullptr;
        m_current_objects.clear();
        m_index_paths.clear();
        m_objects.clear();
        //m_objects_aliases.clear();
        m_instances.clear();
        //m_geometries.clear();
        m_curr_config.object_id = -1;
        m_curr_config.volume_id = -1;
        m_objects_metadata.clear();
        m_layer_heights_profiles.clear();
        m_layer_config_ranges.clear();
        //m_sla_support_points.clear();
        m_curr_metadata_name.clear();
        m_curr_characters.clear();
        m_curr_instance.object_id = -1;
        m_curr_instance.instance_id = -1;
        m_curr_instance.identify_id = 0;
        clear_errors();

        bool result = _load_model_from_file(filename, model, config, config_substitutions, proFn, plate_id);
        is_bbl_3mf = m_is_bbl_3mf;
        if (m_bambuslicer_generator_version)
            file_version = *m_bambuslicer_generator_version;

        return result;
    }

    bool _BBS_3MF_Importer::get_thumbnail(const std::string &filename, std::string &data)
    {
        mz_zip_archive archive;
        mz_zip_zero_struct(&archive);

        struct close_lock
        {
            mz_zip_archive *archive;
            void            close()
            {
                if (archive) {
                    close_zip_reader(archive);
                    archive = nullptr;
                }
            }
            ~close_lock() { close(); }
        } lock{&archive};

        if (!open_zip_reader(&archive, filename)) {
            add_error("Unable to open the file"+filename);
            return false;
        }

        // BBS: load relationships
        if (!_extract_xml_from_archive(archive, RELATIONSHIPS_FILE, _handle_start_relationships_element, _handle_end_relationships_element))
            return false;
        if (m_thumbnail_middle.empty()) m_thumbnail_middle = m_thumbnail_path;
        if (!m_thumbnail_middle.empty()) {
            return _extract_from_archive(archive, m_thumbnail_middle, [&data](auto &archive, auto const &stat) -> bool {
                data.resize(stat.m_uncomp_size);
                return mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, data.data(), data.size(), 0);
            });
        }
        return _extract_from_archive(archive, THUMBNAIL_FILE, [&data](auto &archive, auto const &stat) -> bool {
            data.resize(stat.m_uncomp_size);
            return mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, data.data(), data.size(), 0);
        });
    }

    static size_t mz_zip_read_istream(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
    {
        auto & is = *reinterpret_cast<std::istream*>(pOpaque);
        is.seekg(file_ofs, std::istream::beg);
        if (!is)
            return 0;
        is.read((char *)pBuf, n);
        return is.gcount();
    }

    bool _BBS_3MF_Importer::load_gcode_3mf_from_stream(std::istream &data, Model &model, DynamicPrintConfig &config, Semver &file_version)
    {
        mz_zip_archive archive;
        mz_zip_zero_struct(&archive);
        archive.m_pRead = mz_zip_read_istream;
        archive.m_pIO_opaque = &data;

        data.seekg(0, std::istream::end);
        mz_uint64 size = data.tellg();
        data.seekg(0, std::istream::beg);
        if (!mz_zip_reader_init(&archive, size, 0))
            return false;

        struct close_lock
        {
            mz_zip_archive *archive;
            void            close()
            {
                if (archive) {
                    mz_zip_reader_end(archive);
                    archive = nullptr;
                }
            }
            ~close_lock() { close(); }
        } lock{&archive};

        // BBS: load relationships
        if (!_extract_xml_from_archive(archive, RELATIONSHIPS_FILE, _handle_start_relationships_element, _handle_end_relationships_element))
            return false;
        if (m_start_part_path.empty())
            return false;

        //extract model files
        m_model = &model;
        if (!_extract_from_archive(archive, m_start_part_path, [this] (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_model_from_archive(archive, stat);
            })) {
            add_error("Archive does not contain a valid model");
            return false;
        }

        if (m_thumbnail_middle.empty()) m_thumbnail_middle = m_thumbnail_path;
        if (m_thumbnail_small.empty()) m_thumbnail_small = m_thumbnail_path;
        if (!m_thumbnail_small.empty() && m_thumbnail_small.front() == '/')
            m_thumbnail_small.erase(m_thumbnail_small.begin());
        if (!m_thumbnail_middle.empty() && m_thumbnail_middle.front() == '/')
            m_thumbnail_middle.erase(m_thumbnail_middle.begin());

        // we then loop again the entries to read other files stored in the archive
        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        mz_zip_archive_file_stat stat;
        for (mz_uint i = 0; i < num_entries; ++i) {
            if (mz_zip_reader_file_stat(&archive, i, &stat)) {
                std::string name(stat.m_filename);
                std::replace(name.begin(), name.end(), '\\', '/');

                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("extract %1%th file %2%, total=%3%\n")%(i+1)%name%num_entries;

                if (boost::algorithm::iequals(name, BBS_MODEL_CONFIG_FILE)) {
                    // extract slic3r model config file
                    if (!_extract_xml_from_archive(archive, stat, _handle_start_config_xml_element, _handle_end_config_xml_element)) {
                        add_error("Archive does not contain a valid model config");
                        return false;
                    }
                }
                else if (boost::algorithm::iequals(name, SLICE_INFO_CONFIG_FILE)) {
                    m_parsing_slice_info = true;
                    //extract slice info from archive
                    _extract_xml_from_archive(archive, stat, _handle_start_config_xml_element, _handle_end_config_xml_element);
                    m_parsing_slice_info = false;
                }
            }
        }
 
        lock.close();

        return true;
    }

    void _BBS_3MF_Importer::_destroy_xml_parser()
    {
        if (m_xml_parser != nullptr) {
            XML_ParserFree(m_xml_parser);
            m_xml_parser = nullptr;
        }
    }

    void _BBS_3MF_Importer::_stop_xml_parser(const std::string &msg)
    {
        assert(! m_parse_error);
        assert(m_parse_error_message.empty());
        assert(m_xml_parser != nullptr);
        m_parse_error = true;
        m_parse_error_message = msg;
        XML_StopParser(m_xml_parser, false);
    }

    //BBS: add plate data related logic
    bool _BBS_3MF_Importer::_load_model_from_file(
        std::string filename,
        Model& model,
        DynamicPrintConfig& config,
        ConfigSubstitutionContext& config_substitutions,
        Import3mfProgressFn proFn,
        int plate_id)
    {
        bool cb_cancel = false;
        //BBS progress point
        // prepare restore
        if (m_load_restore) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_RESTORE\n");
            if (proFn) {
                proFn(IMPORT_STAGE_RESTORE, 0, 1, cb_cancel);
                if (cb_cancel)
                    return false;
            }
        }

        //BBS progress point
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_OPEN, m_load_restore=%1%\n")%m_load_restore;
        if (proFn) {
            proFn(IMPORT_STAGE_OPEN, 0, 1, cb_cancel);
            if (cb_cancel)
                return false;
        }

        mz_zip_archive archive;
        mz_zip_zero_struct(&archive);

        struct close_lock
        {
            mz_zip_archive * archive;
            void close() {
                if (archive) {
                    close_zip_reader(archive);
                    archive = nullptr;
                }
            }
            ~close_lock() {
                close();
            }
        } lock{ &archive };

        if (!open_zip_reader(&archive, filename)) {
            add_error("Unable to open the file"+filename);
            return false;
        }

        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);

        mz_zip_archive_file_stat stat;

        m_name = boost::filesystem::path(filename).stem().string();

        //BBS progress point
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_READ_FILES\n");
        if (proFn) {
            proFn(IMPORT_STAGE_READ_FILES, 0, 3, cb_cancel);
            if (cb_cancel)
                return false;
        }
        // BBS: load relationships
        if (!_extract_xml_from_archive(archive, RELATIONSHIPS_FILE, _handle_start_relationships_element, _handle_end_relationships_element))
            return false;
        if (m_start_part_path.empty())
            return false;
        // BBS: load sub models (Production Extension)
        std::string sub_rels = m_start_part_path;
        sub_rels.insert(boost::find_last(sub_rels, "/").end() - sub_rels.begin(), "_rels/");
        sub_rels.append(".rels");
        if (sub_rels.front() == '/') sub_rels = sub_rels.substr(1);

        //check whether sub relation file is exist or not
        int sub_index = mz_zip_reader_locate_file(&archive, sub_rels.c_str(), nullptr, 0);
        if (sub_index == -1) {
            //no submodule files found, use only one 3dmodel.model
        }
        else {
            _extract_xml_from_archive(archive, sub_rels, _handle_start_relationships_element, _handle_end_relationships_element);
            int index = 0;

#if 0
            for (auto path : m_sub_model_paths) {
                if (proFn) {
                    proFn(IMPORT_STAGE_READ_FILES, ++index, 3 + m_sub_model_paths.size(), cb_cancel);
                    if (cb_cancel)
                        return false;
                }
                else
                    ++index;
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format(", read %1%th sub model file %2%\n")%index %path;
                m_sub_model_path = path;
                if (!_extract_from_archive(archive, path, [this] (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_model_from_archive(archive, stat);
                }, m_load_restore)) {
                    add_error("Archive does not contain a valid model");
                    return false;
                }
                m_sub_model_path.clear();
            }
#else
            for (auto path : m_sub_model_paths) {
                ObjectImporter *object_importer = new ObjectImporter(this, filename, path);
                m_object_importers.push_back(object_importer);
            }

            bool object_load_result = true;
            boost::mutex mutex;
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, m_object_importers.size()),
                [this, &mutex, &object_load_result](const tbb::blocked_range<size_t>& importer_range) {
                    CNumericLocalesSetter locales_setter;
                    for (size_t object_index = importer_range.begin(); object_index < importer_range.end(); ++ object_index) {
                        bool result = m_object_importers[object_index]->extract_object_model();
                        {
                            boost::unique_lock l(mutex);
                            object_load_result &= result;
                        }
                    }
                }
            );

            if (!object_load_result) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", loading sub-objects error\n");
                return false;
            }

            //merge these objects into one
            for (auto obj_importer : m_object_importers) {
                for (const IdToCurrentObjectMap::value_type&  obj : obj_importer->object_list)
                    m_current_objects.insert({ std::move(obj.first), std::move(obj.second)});
                for (auto group_color : obj_importer->object_group_id_to_color)
                    m_group_id_to_color.insert(std::move(group_color));

                delete obj_importer;
            }
            m_object_importers.clear();
#endif
            // BBS: load root model
            if (proFn) {
                proFn(IMPORT_STAGE_READ_FILES, 2, 3, cb_cancel);
                if (cb_cancel)
                    return false;
            }
        }

        //extract model files
        if (!_extract_from_archive(archive, m_start_part_path, [this] (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_model_from_archive(archive, stat);
            })) {
            add_error("Archive does not contain a valid model");
            return false;
        }

        //BBS: version check
        bool dont_load_config = !m_load_config;
        if (m_bambuslicer_generator_version) {
            Semver app_version = *(Semver::parse(SLIC3R_VERSION));
            Semver file_version = *m_bambuslicer_generator_version;
            if (file_version.maj() != app_version.maj())
                dont_load_config = true;
        }
        else {
            m_bambuslicer_generator_version = Semver::parse("0.0.0.0");
            dont_load_config = true;
        }

        // we then loop again the entries to read other files stored in the archive
        for (mz_uint i = 0; i < num_entries; ++i) {
            if (mz_zip_reader_file_stat(&archive, i, &stat)) {

                //BBS progress point
                if (proFn) {
                    proFn(IMPORT_STAGE_EXTRACT, i, num_entries, cb_cancel);
                    if (cb_cancel)
                        return false;
                }

                std::string name(stat.m_filename);
                std::replace(name.begin(), name.end(), '\\', '/');

                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("extract %1%th file %2%, total=%3%")%(i+1)%name%num_entries;

                if (name.find("/../") != std::string::npos) {
                    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format(", find file path including /../, not valid, skip it\n");
                    continue;
                }

                if (boost::algorithm::iequals(name, BBS_LAYER_HEIGHTS_PROFILE_FILE)) {
                    // extract slic3r layer heights profile file
                    _extract_layer_heights_profile_config_from_archive(archive, stat);
                }
                else
                if (boost::algorithm::iequals(name, LAYER_CONFIG_RANGES_FILE)) {
                    // extract slic3r layer config ranges file
                    _extract_layer_config_ranges_from_archive(archive, stat, config_substitutions);
                }
                //BBS: disable SLA related files currently
                /*else if (boost::algorithm::iequals(name, SLA_SUPPORT_POINTS_FILE)) {
                    // extract sla support points file
                    _extract_sla_support_points_from_archive(archive, stat);
                }
                else if (boost::algorithm::iequals(name, SLA_DRAIN_HOLES_FILE)) {
                    // extract sla support points file
                    _extract_sla_drain_holes_from_archive(archive, stat);
                }*/
                //BBS: project setting file
                //if (!dont_load_config && boost::algorithm::iequals(name, BBS_PRINT_CONFIG_FILE)) {
                    // extract slic3r print config file
                //    _extract_print_config_from_archive(archive, stat, config, config_substitutions, filename);
                //} else
                if (boost::algorithm::iequals(name, CUT_INFORMATION_FILE)) {
                    // extract object cut info
                    _extract_cut_information_from_archive(archive, stat, config_substitutions);
                }
                else if (!dont_load_config && boost::algorithm::iequals(name, CUSTOM_GCODE_PER_PRINT_Z_FILE)) {
                    // extract slic3r layer config ranges file
                    _extract_custom_gcode_per_print_z_from_archive(archive, stat);
                }
                //else if (boost::algorithm::iequals(name, BBS_MODEL_CONFIG_FILE)) {
                //    // extract slic3r model config file
                //    if (!_extract_xml_from_archive(archive, stat, _handle_start_config_xml_element, _handle_end_config_xml_element)) {
                //        add_error("Archive does not contain a valid model config");
                //        return false;
                //    }
                //}
                else if (!dont_load_config && boost::algorithm::iequals(name, SLICE_INFO_CONFIG_FILE)) {
                    m_parsing_slice_info = true;
                    //extract slice info from archive
                    _extract_xml_from_archive(archive, stat, _handle_start_config_xml_element, _handle_end_config_xml_element);
                    m_parsing_slice_info = false;
                }
                //else if (boost::algorithm::istarts_with(name, AUXILIARY_DIR)) {
                //    // extract auxiliary directory to temp directory, do nothing for restore
                //    if (m_load_aux && !m_load_restore)
                //        _extract_auxiliary_file_from_archive(archive, stat, model);
                //}
                else if (!dont_load_config && boost::algorithm::istarts_with(name, METADATA_DIR) && boost::algorithm::iends_with(name, GCODE_EXTENSION)) {
                    //load gcode files
                    _extract_file_from_archive(archive, stat);
                }
                else if (!dont_load_config && boost::algorithm::istarts_with(name, METADATA_DIR) && boost::algorithm::iends_with(name, THUMBNAIL_EXTENSION)) {
                    //BBS parsing pattern thumbnail and plate thumbnails
                    _extract_file_from_archive(archive, stat);
                }
                else if (!dont_load_config && boost::algorithm::istarts_with(name, METADATA_DIR) && boost::algorithm::iends_with(name, CALIBRATION_INFO_EXTENSION)) {
                    //BBS parsing pattern config files
                    _extract_file_from_archive(archive, stat);
                }
                else {
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format(", %1% skipped, already parsed or a directory or not supported\n")%name;
                }
            }
        }

        lock.close();

        if (!m_is_bbl_3mf) {
            // if the 3mf was not produced by BambuStudio and there is more than one instance,
            // split the object in as many objects as instances
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format(", found 3mf from other vendor, split as instance");
            for (const IdToModelObjectMap::value_type& object : m_objects) {
                if (object.second >= int(m_model->objects.size())) {
                    add_error("3rd 3mf, invalid object, id: "+std::to_string(object.first.second));
                    return false;
                }
                ModelObject* model_object = m_model->objects[object.second];
                if (model_object->instances.size() > 1) {
                    IdToCurrentObjectMap::const_iterator current_object = m_current_objects.find(object.first);
                    if (current_object == m_current_objects.end()) {
                        add_error("3rd 3mf, can not find object, id " + std::to_string(object.first.second));
                        return false;
                    }
                    std::vector<Component> object_id_list;
                    _generate_current_object_list(object_id_list, object.first, m_current_objects);

                    ObjectMetadata::VolumeMetadataList volumes;
                    ObjectMetadata::VolumeMetadataList* volumes_ptr = nullptr;

                    for (int k = 0; k < object_id_list.size(); k++)
                    {
                        Id object_id = object_id_list[k].object_id;
                        volumes.emplace_back(object_id.second);
                    }

                    // select as volumes
                    volumes_ptr = &volumes;

                    // for each instance after the 1st, create a new model object containing only that instance
                    // and copy into it the geometry
                    while (model_object->instances.size() > 1) {
                        ModelObject* new_model_object = m_model->add_object(*model_object);
                        new_model_object->clear_instances();
                        new_model_object->add_instance(*model_object->instances.back());
                        model_object->delete_last_instance();
                        if (!_generate_volumes_new(*new_model_object, object_id_list, *volumes_ptr, config_substitutions))
                            return false;
                    }
                }
            }
        }

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format(", process group colors, size %1%\n")%m_group_id_to_color.size();
        std::map<int, int> color_group_id_to_extruder_id_map;
        std::map<std::string, int> color_to_extruder_id_map;
        int extruder_id = 0;
        for (auto group_iter = m_group_id_to_color.begin(); group_iter != m_group_id_to_color.end(); ++group_iter) {
            auto color_iter = color_to_extruder_id_map.find(group_iter->second);
            if (color_iter == color_to_extruder_id_map.end()) {
                ++extruder_id;
                color_to_extruder_id_map[group_iter->second] = extruder_id;
                color_group_id_to_extruder_id_map[group_iter->first] = extruder_id;
            } else {
                color_group_id_to_extruder_id_map[group_iter->first] = color_iter->second;
            }
        }

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format(", begin to assemble objects, size %1%\n")%m_objects.size();
        for (const IdToModelObjectMap::value_type& object : m_objects) {
            if (object.second >= int(m_model->objects.size())) {
                add_error("invalid object, id: "+std::to_string(object.first.second));
                return false;
            }

            ModelObject* model_object = m_model->objects[object.second];
            /*IdToGeometryMap::const_iterator obj_geometry = m_geometries.find(object.first);
            if (obj_geometry == m_geometries.end()) {
                add_error("Unable to find object geometry");
                return false;
            }*/

            // m_layer_heights_profiles are indexed by a 1 based model object index.
            IdToLayerHeightsProfileMap::iterator obj_layer_heights_profile = m_layer_heights_profiles.find(object.second + 1);
            if (obj_layer_heights_profile != m_layer_heights_profiles.end())
                model_object->layer_height_profile.set(std::move(obj_layer_heights_profile->second));

            // m_layer_config_ranges are indexed by a 1 based model object index.
            IdToLayerConfigRangesMap::iterator obj_layer_config_ranges = m_layer_config_ranges.find(object.second + 1);
            if (obj_layer_config_ranges != m_layer_config_ranges.end())
                model_object->layer_config_ranges = std::move(obj_layer_config_ranges->second);

            // m_sla_support_points are indexed by a 1 based model object index.
            /*IdToSlaSupportPointsMap::iterator obj_sla_support_points = m_sla_support_points.find(object.second + 1);
            if (obj_sla_support_points != m_sla_support_points.end() && !obj_sla_support_points->second.empty()) {
                model_object->sla_support_points = std::move(obj_sla_support_points->second);
                model_object->sla_points_status = sla::PointsStatus::UserModified;
            }

            IdToSlaDrainHolesMap::iterator obj_drain_holes = m_sla_drain_holes.find(object.second + 1);
            if (obj_drain_holes != m_sla_drain_holes.end() && !obj_drain_holes->second.empty()) {
                model_object->sla_drain_holes = std::move(obj_drain_holes->second);
            }*/

            std::vector<Component> object_id_list;
            _generate_current_object_list(object_id_list, object.first, m_current_objects);

            ObjectMetadata::VolumeMetadataList volumes;
            ObjectMetadata::VolumeMetadataList* volumes_ptr = nullptr;

            IdToMetadataMap::iterator obj_metadata = m_objects_metadata.find(object.first.second);
            if (obj_metadata != m_objects_metadata.end()) {
                // config data has been found, this model was saved using slic3r pe

                // apply object's name and config data
                for (const Metadata& metadata : obj_metadata->second.metadata) {
                    if (metadata.key == "name")
                        model_object->name = metadata.value;
                    //BBS: add module name
                    else if (metadata.key == "module") {
                        //model_object->module_name = metadata.value;
                    }
                    else
                        model_object->config.set_deserialize(metadata.key, metadata.value, config_substitutions);
                }

                // select object's detected volumes
                volumes_ptr = &obj_metadata->second.volumes;
            }
            else {
                // config data not found, this model was not saved using slic3r pe

                // add the entire geometry as the single volume to generate
                //volumes.emplace_back(0, (int)obj_geometry->second.triangles.size() - 1);
                for (int k = 0; k < object_id_list.size(); k++)
                {
                    Id object_id = object_id_list[k].object_id;
                    volumes.emplace_back(object_id.second);
                }

                IdToCurrentObjectMap::const_iterator current_object = m_current_objects.find(object.first);
                if (current_object != m_current_objects.end()) {
                    // get name
                    if (!current_object->second.name.empty())
                        model_object->name = current_object->second.name;
                    else
                        model_object->name = "Object_"+std::to_string(object.second+1);

                    // get color
                    auto extruder_itor = color_group_id_to_extruder_id_map.find(current_object->second.pid);
                    if (extruder_itor != color_group_id_to_extruder_id_map.end()) {
                        model_object->config.set_key_value("extruder", new ConfigOptionInt(extruder_itor->second));
                    }
                }

                // select as volumes
                volumes_ptr = &volumes;
            }

            if (!_generate_volumes_new(*model_object, object_id_list, *volumes_ptr, config_substitutions))
                return false;

            // Apply cut information for object if any was loaded
            // m_cut_object_ids are indexed by a 1 based model object index.
            IdToCutObjectInfoMap::iterator cut_object_info = m_cut_object_infos.find(object.second + 1);
            if (cut_object_info != m_cut_object_infos.end()) {
                model_object->cut_id = cut_object_info->second.id;

                for (auto connector : cut_object_info->second.connectors) {
                    assert(0 <= connector.volume_id && connector.volume_id <= int(model_object->volumes.size()));
                    model_object->volumes[connector.volume_id]->cut_info =
                        ModelVolume::CutInfo(CutConnectorType(connector.type), connector.r_tolerance, connector.h_tolerance, true);
                }
            }
        }

        // If instances contain a single volume, the volume offset should be 0,0,0
        // This equals to say that instance world position and volume world position should match
        // Correct all instances/volumes for which this does not hold
        for (int obj_id = 0; obj_id < int(model.objects.size()); ++obj_id) {
            ModelObject *o = model.objects[obj_id];
            if (o->volumes.size() == 1) {
                ModelVolume *                           v                 = o->volumes.front();
                const Slic3r::Geometry::Transformation &first_inst_trafo  = o->instances.front()->get_transformation();
                const Vec3d                             world_vol_offset  = (first_inst_trafo * v->get_transformation()).get_offset();
                const Vec3d                             world_inst_offset = first_inst_trafo.get_offset();

                if (!world_vol_offset.isApprox(world_inst_offset)) {
                    const Slic3r::Geometry::Transformation &vol_trafo = v->get_transformation();
                    for (int inst_id = 0; inst_id < int(o->instances.size()); ++inst_id) {
                        ModelInstance *                         i          = o->instances[inst_id];
                        const Slic3r::Geometry::Transformation &inst_trafo = i->get_transformation();
                        i->set_offset((inst_trafo * vol_trafo).get_offset());
                    }
                    v->set_offset(Vec3d::Zero());
                }
            }
        }

        int object_idx = 0;
        for (ModelObject* o : model.objects) {
            int volume_idx = 0;
            for (ModelVolume* v : o->volumes) {
                if (v->source.input_file.empty() && v->type() == ModelVolumeType::MODEL_PART) {
                    v->source.input_file = filename;
                    if (v->source.volume_idx == -1)
                        v->source.volume_idx = volume_idx;
                    if (v->source.object_idx == -1)
                        v->source.object_idx = object_idx;
                }
                ++volume_idx;
            }
            ++object_idx;
        }

        const ConfigOptionStrings* filament_ids_opt = config.option<ConfigOptionStrings>("filament_settings_id");
        int max_filament_id = filament_ids_opt ? filament_ids_opt->size() : std::numeric_limits<int>::max();
        for (ModelObject* mo : m_model->objects) {
            const ConfigOptionInt* extruder_opt = dynamic_cast<const ConfigOptionInt*>(mo->config.option("extruder"));
            int extruder_id = 0;
            if (extruder_opt != nullptr)
                extruder_id = extruder_opt->getInt();

            if (extruder_id == 0 || extruder_id > max_filament_id)
                mo->config.set_key_value("extruder", new ConfigOptionInt(1));

            if (mo->volumes.size() == 1) {
                mo->volumes[0]->config.erase("extruder");
            }
            else {
                for (ModelVolume* mv : mo->volumes) {
                    const ConfigOptionInt* vol_extruder_opt = dynamic_cast<const ConfigOptionInt*>(mv->config.option("extruder"));
                    if (vol_extruder_opt == nullptr)
                        continue;

                    if (vol_extruder_opt->getInt() == 0)
                        mv->config.erase("extruder");
                    else if (vol_extruder_opt->getInt() > max_filament_id)
                        mv->config.set_key_value("extruder", new ConfigOptionInt(1));
                }
            }
        }

//        // fixes the min z of the model if negative
//        model.adjust_min_z();

        //BBS progress point
        if (proFn) {
            proFn(IMPORT_STAGE_LOADING_PLATES, 0, 1, cb_cancel);
            if (cb_cancel)
                return false;
        }    
      
        //BBS progress point
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" << __LINE__ << boost::format("import 3mf IMPORT_STAGE_FINISH\n");
        if (proFn) {
            proFn(IMPORT_STAGE_FINISH, 0, 1, cb_cancel);
            if (cb_cancel)
                return false;
        }

        return true;
    }

    bool _BBS_3MF_Importer::_extract_from_archive(mz_zip_archive& archive, std::string const & path, std::function<bool (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)> extract, bool restore)
    {
        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        mz_zip_archive_file_stat stat;
        std::string path2 = path;
        if (path2.front() == '/') path2 = path2.substr(1);
        // try utf8 encoding
        int index = mz_zip_reader_locate_file(&archive, path2.c_str(), nullptr, 0);
        if (index < 0) {
            // try native encoding
            std::string native_path = encode_path(path2.c_str());
            index = mz_zip_reader_locate_file(&archive, native_path.c_str(), nullptr, 0);
        }
        if (index < 0) {
            // try unicode path extra
            std::string extra(1024, 0);
            for (mz_uint i = 0; i < archive.m_total_files; ++i) {
                size_t n = mz_zip_reader_get_extra(&archive, i, extra.data(), extra.size());
                if (n > 0 && path2 == ZipUnicodePathExtraField::decode(extra.substr(0, n))) {
                    index = i;
                    break;
                }
            }
        }
        if (index < 0 || !mz_zip_reader_file_stat(&archive, index, &stat)) {
            if (restore) {
                std::vector<std::string> paths = {m_backup_path + path};
                if (!m_origin_file.empty()) paths.push_back(m_origin_file);
                for (auto & path2 : paths) {
                    bool result = false;
                    if (boost::filesystem::exists(path2)) {
                        mz_zip_archive archive;
                        mz_zip_zero_struct(&archive);
                        if (open_zip_reader(&archive, path2)) {
                            result = _extract_from_archive(archive, path, extract);
                            close_zip_reader(&archive);
                        }
                    }
                    if (result) return result;
                }
            }
            char error_buf[1024];
            ::snprintf(error_buf, 1024, "File %s not found from archive", path.c_str());
            add_error(error_buf);
            return false;
        }
        try
        {
            if (!extract(archive, stat)) {
                return false;
            }
        }
        catch (const std::exception& e)
        {
            // ensure the zip archive is closed and rethrow the exception
            add_error(e.what());
            return false;
        }
        return true;
    }

    bool _BBS_3MF_Importer::_extract_xml_from_archive(mz_zip_archive& archive, const std::string & path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        return _extract_from_archive(archive, path, [this, start_handler, end_handler](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_xml_from_archive(archive, stat, start_handler, end_handler);
        });
    }

    bool _BBS_3MF_Importer::_extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return false;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return false;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, start_handler, end_handler);
        XML_SetCharacterDataHandler(m_xml_parser, _BBS_3MF_Importer::_handle_xml_characters);

        void* parser_buffer = XML_GetBuffer(m_xml_parser, (int)stat.m_uncomp_size);
        if (parser_buffer == nullptr) {
            add_error("Unable to create buffer");
            return false;
        }

        mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, parser_buffer, (size_t)stat.m_uncomp_size, 0);
        if (res == 0) {
            add_error("Error while reading config data to buffer");
            return false;
        }

        if (!XML_ParseBuffer(m_xml_parser, (int)stat.m_uncomp_size, 1)) {
            char error_buf[1024];
            ::snprintf(error_buf, 1024, "Error (%s) while parsing xml file at line %d", XML_ErrorString(XML_GetErrorCode(m_xml_parser)), (int)XML_GetCurrentLineNumber(m_xml_parser));
            add_error(error_buf);
            return false;
        }

        return true;
    }

    bool _BBS_3MF_Importer::_extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return false;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return false;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, _BBS_3MF_Importer::_handle_start_model_xml_element, _BBS_3MF_Importer::_handle_end_model_xml_element);
        XML_SetCharacterDataHandler(m_xml_parser, _BBS_3MF_Importer::_handle_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            _BBS_3MF_Importer& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, _BBS_3MF_Importer& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(m_xml_parser, *this, stat);

        mz_bool res = 0;

        try
        {
            mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
                CallbackData* data = (CallbackData*)pOpaque;
                if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0) || data->importer.parse_error()) {
                    char error_buf[1024];
                    ::snprintf(error_buf, 1024, "Error (%s) while parsing '%s' at line %d", data->importer.parse_error_message(), data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                    throw Slic3r::FileIOError(error_buf);
                }
                return n;
            };
            void* opaque = &data;
            res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);
        }
        catch (const version_error& e)
        {
            // rethrow the exception
            throw Slic3r::FileIOError(e.what());
        }
        catch (std::exception& e)
        {
            add_error(e.what());
            return false;
        }

        if (res == 0) {
            add_error("Error while extracting model data from zip archive");
            return false;
        }

        return true;
    }

    void _BBS_3MF_Importer::_extract_cut_information_from_archive(mz_zip_archive &archive, const mz_zip_archive_file_stat &stat, ConfigSubstitutionContext &config_substitutions)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t) stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void *) buffer.data(), (size_t) stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading cut information data to buffer");
                return;
            }

            std::istringstream iss(buffer); // wrap returned xml to istringstream
            pt::ptree          objects_tree;
            pt::read_xml(iss, objects_tree);

            for (const auto &object : objects_tree.get_child("objects")) {
                pt::ptree object_tree = object.second;
                int       obj_idx     = object_tree.get<int>("<xmlattr>.id", -1);
                if (obj_idx <= 0) {
                    add_error("Found invalid object id");
                    continue;
                }

                IdToCutObjectInfoMap::iterator object_item = m_cut_object_infos.find(obj_idx);
                if (object_item != m_cut_object_infos.end()) {
                    add_error("Found duplicated cut_object_id");
                    continue;
                }

                CutObjectBase                         cut_id;
                std::vector<CutObjectInfo::Connector> connectors;

                for (const auto &obj_cut_info : object_tree) {
                    if (obj_cut_info.first == "cut_id") {
                        pt::ptree cut_id_tree = obj_cut_info.second;
                        cut_id                = CutObjectBase(ObjectID(cut_id_tree.get<size_t>("<xmlattr>.id")), cut_id_tree.get<size_t>("<xmlattr>.check_sum"),
                                               cut_id_tree.get<size_t>("<xmlattr>.connectors_cnt"));
                    }
                    if (obj_cut_info.first == "connectors") {
                        pt::ptree cut_connectors_tree = obj_cut_info.second;
                        for (const auto &cut_connector : cut_connectors_tree) {
                            if (cut_connector.first != "connector") continue;
                            pt::ptree                connector_tree = cut_connector.second;
                            CutObjectInfo::Connector connector      = {connector_tree.get<int>("<xmlattr>.volume_id"), connector_tree.get<int>("<xmlattr>.type"),
                                                                  connector_tree.get<float>("<xmlattr>.radius", 0.f), connector_tree.get<float>("<xmlattr>.height", 0.f),
                                                                  connector_tree.get<float>("<xmlattr>.r_tolerance"), connector_tree.get<float>("<xmlattr>.h_tolerance")};
                            connectors.emplace_back(connector);
                        }
                    }
                }

                CutObjectInfo cut_info{cut_id, connectors};
                m_cut_object_infos.insert({obj_idx, cut_info});
            }
        }
    }

    void _BBS_3MF_Importer::_extract_print_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, DynamicPrintConfig& config, ConfigSubstitutionContext& config_substitutions, const std::string& archive_filename)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t)stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading config data to buffer");
                return;
            }
            ConfigBase::load_from_gcode_string_legacy(config, buffer.data(), config_substitutions);
        }
    }

    //void _BBS_3MF_Importer::_extract_auxiliary_file_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, Model& model)
    //{
    //    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", stat.m_uncomp_size is %1%")%stat.m_uncomp_size;
    //    if (stat.m_uncomp_size > 0) {
    //        std::string dest_file;
    //        if (stat.m_is_utf8) {
    //            dest_file = stat.m_filename;
    //        } else {
    //            std::string extra(1024, 0);
    //            size_t n = mz_zip_reader_get_extra(&archive, stat.m_file_index, extra.data(), extra.size());
    //            dest_file = ZipUnicodePathExtraField::decode(extra.substr(0, n), stat.m_filename);
    //        }
    //        std::string temp_path = model.get_auxiliary_file_temp_path();
    //        //aux directory from model
    //        boost::filesystem::path dir = boost::filesystem::path(temp_path);
    //        std::size_t found = dest_file.find(AUXILIARY_DIR);
    //        if (found != std::string::npos)
    //            dest_file = dest_file.substr(found + AUXILIARY_STR_LEN);
    //        else
    //            return;

    //        if (dest_file.find('/') != std::string::npos) {
    //            boost::filesystem::path src_path = boost::filesystem::path(dest_file);
    //            boost::filesystem::path parent_path = src_path.parent_path();
    //            std::string temp_path = dir.string() + std::string("/") + parent_path.string();
    //            boost::filesystem::path parent_full_path =  boost::filesystem::path(temp_path);
    //            if (!boost::filesystem::exists(parent_full_path))
    //                boost::filesystem::create_directories(parent_full_path);
    //        }
    //        dest_file = dir.string() + std::string("/") + dest_file;
    //        std::string dest_zip_file = encode_path(dest_file.c_str());
    //        mz_bool res = mz_zip_reader_extract_to_file(&archive, stat.m_file_index, dest_zip_file.c_str(), 0);
    //        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", extract  %1% from 3mf %2%, ret %3%\n") % dest_file % stat.m_filename % res;
    //        if (res == 0) {
    //            add_error("Error while extract auxiliary file to file");
    //            return;
    //        }
    //    }
    //}

    void _BBS_3MF_Importer::_extract_file_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size > 0) {
            std::string src_file = decode_path(stat.m_filename);
            // BBS: use backup path
            //aux directory from model
            boost::filesystem::path dest_path = boost::filesystem::path(m_backup_path + "/" + src_file);
            std::string dest_zip_file = encode_path(dest_path.string().c_str());
            mz_bool res = mz_zip_reader_extract_to_file(&archive, stat.m_file_index, dest_zip_file.c_str(), 0);
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(", extract  %1% from 3mf %2%, ret %3%\n") % dest_path % stat.m_filename % res;
            if (res == 0) {
                add_error("Error while extract file to temp directory");
                return;
            }
        }
        return;
    }

    void _BBS_3MF_Importer::_extract_layer_heights_profile_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t)stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading layer heights profile data to buffer");
                return;
            }

            if (buffer.back() == '\n')
                buffer.pop_back();

            std::vector<std::string> objects;
            boost::split(objects, buffer, boost::is_any_of("\n"), boost::token_compress_off);

            for (const std::string& object : objects)             {
                std::vector<std::string> object_data;
                boost::split(object_data, object, boost::is_any_of("|"), boost::token_compress_off);
                if (object_data.size() != 2) {
                    add_error("Error while reading object data");
                    continue;
                }

                std::vector<std::string> object_data_id;
                boost::split(object_data_id, object_data[0], boost::is_any_of("="), boost::token_compress_off);
                if (object_data_id.size() != 2) {
                    add_error("Error while reading object id");
                    continue;
                }

                int object_id = std::atoi(object_data_id[1].c_str());
                if (object_id == 0) {
                    add_error("Found invalid object id");
                    continue;
                }

                IdToLayerHeightsProfileMap::iterator object_item = m_layer_heights_profiles.find(object_id);
                if (object_item != m_layer_heights_profiles.end()) {
                    add_error("Found duplicated layer heights profile");
                    continue;
                }

                std::vector<std::string> object_data_profile;
                boost::split(object_data_profile, object_data[1], boost::is_any_of(";"), boost::token_compress_off);
                if (object_data_profile.size() <= 4 || object_data_profile.size() % 2 != 0) {
                    add_error("Found invalid layer heights profile");
                    continue;
                }

                std::vector<coordf_t> profile;
                profile.reserve(object_data_profile.size());

                for (const std::string& value : object_data_profile) {
                    profile.push_back((coordf_t)std::atof(value.c_str()));
                }

                m_layer_heights_profiles.insert({ object_id, profile });
            }
        }
    }

    void _BBS_3MF_Importer::_extract_layer_config_ranges_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, ConfigSubstitutionContext& config_substitutions)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t)stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading layer config ranges data to buffer");
                return;
            }

            std::istringstream iss(buffer); // wrap returned xml to istringstream
            pt::ptree objects_tree;
            pt::read_xml(iss, objects_tree);

            for (const auto& object : objects_tree.get_child("objects")) {
                pt::ptree object_tree = object.second;
                int obj_idx = object_tree.get<int>("<xmlattr>.id", -1);
                if (obj_idx <= 0) {
                    add_error("Found invalid object id");
                    continue;
                }

                IdToLayerConfigRangesMap::iterator object_item = m_layer_config_ranges.find(obj_idx);
                if (object_item != m_layer_config_ranges.end()) {
                    add_error("Found duplicated layer config range");
                    continue;
                }

                t_layer_config_ranges config_ranges;

                for (const auto& range : object_tree) {
                    if (range.first != "range")
                        continue;
                    pt::ptree range_tree = range.second;
                    double min_z = range_tree.get<double>("<xmlattr>.min_z");
                    double max_z = range_tree.get<double>("<xmlattr>.max_z");

                    // get Z range information
                    DynamicPrintConfig config;

                    for (const auto& option : range_tree) {
                        if (option.first != "option")
                            continue;
                        std::string opt_key = option.second.get<std::string>("<xmlattr>.opt_key");
                        std::string value = option.second.data();

                        config.set_deserialize(opt_key, value, config_substitutions);
                    }

                    config_ranges[{ min_z, max_z }].assign_config(std::move(config));
                }

                if (!config_ranges.empty())
                    m_layer_config_ranges.insert({ obj_idx, std::move(config_ranges) });
            }
        }
    }
    /*
    void _BBS_3MF_Importer::_extract_sla_support_points_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t)stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading sla support points data to buffer");
                return;
            }

            if (buffer.back() == '\n')
                buffer.pop_back();

            std::vector<std::string> objects;
            boost::split(objects, buffer, boost::is_any_of("\n"), boost::token_compress_off);

            // Info on format versioning - see 3mf.hpp
            int version = 0;
            std::string key("support_points_format_version=");
            if (!objects.empty() && objects[0].find(key) != std::string::npos) {
                objects[0].erase(objects[0].begin(), objects[0].begin() + long(key.size())); // removes the string
                version = std::stoi(objects[0]);
                objects.erase(objects.begin()); // pop the header
            }

            for (const std::string& object : objects) {
                std::vector<std::string> object_data;
                boost::split(object_data, object, boost::is_any_of("|"), boost::token_compress_off);

                if (object_data.size() != 2) {
                    add_error("Error while reading object data");
                    continue;
                }

                std::vector<std::string> object_data_id;
                boost::split(object_data_id, object_data[0], boost::is_any_of("="), boost::token_compress_off);
                if (object_data_id.size() != 2) {
                    add_error("Error while reading object id");
                    continue;
                }

                int object_id = std::atoi(object_data_id[1].c_str());
                if (object_id == 0) {
                    add_error("Found invalid object id");
                    continue;
                }

                IdToSlaSupportPointsMap::iterator object_item = m_sla_support_points.find(object_id);
                if (object_item != m_sla_support_points.end()) {
                    add_error("Found duplicated SLA support points");
                    continue;
                }

                std::vector<std::string> object_data_points;
                boost::split(object_data_points, object_data[1], boost::is_any_of(" "), boost::token_compress_off);

                std::vector<sla::SupportPoint> sla_support_points;

                if (version == 0) {
                    for (unsigned int i=0; i<object_data_points.size(); i+=3)
                    sla_support_points.emplace_back(float(std::atof(object_data_points[i+0].c_str())),
                                                    float(std::atof(object_data_points[i+1].c_str())),
													float(std::atof(object_data_points[i+2].c_str())),
                                                    0.4f,
                                                    false);
                }
                if (version == 1) {
                    for (unsigned int i=0; i<object_data_points.size(); i+=5)
                    sla_support_points.emplace_back(float(std::atof(object_data_points[i+0].c_str())),
                                                    float(std::atof(object_data_points[i+1].c_str())),
                                                    float(std::atof(object_data_points[i+2].c_str())),
                                                    float(std::atof(object_data_points[i+3].c_str())),
													//FIXME storing boolean as 0 / 1 and importing it as float.
                                                    std::abs(std::atof(object_data_points[i+4].c_str()) - 1.) < EPSILON);
                }

                if (!sla_support_points.empty())
                    m_sla_support_points.insert({ object_id, sla_support_points });
            }
        }
    }

    void _BBS_3MF_Importer::_extract_sla_drain_holes_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size > 0) {
            std::string buffer(size_t(stat.m_uncomp_size), 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading sla support points data to buffer");
                return;
            }

            if (buffer.back() == '\n')
                buffer.pop_back();

            std::vector<std::string> objects;
            boost::split(objects, buffer, boost::is_any_of("\n"), boost::token_compress_off);

            // Info on format versioning - see 3mf.hpp
            int version = 0;
            std::string key("drain_holes_format_version=");
            if (!objects.empty() && objects[0].find(key) != std::string::npos) {
                objects[0].erase(objects[0].begin(), objects[0].begin() + long(key.size())); // removes the string
                version = std::stoi(objects[0]);
                objects.erase(objects.begin()); // pop the header
            }

            for (const std::string& object : objects) {
                std::vector<std::string> object_data;
                boost::split(object_data, object, boost::is_any_of("|"), boost::token_compress_off);

                if (object_data.size() != 2) {
                    add_error("Error while reading object data");
                    continue;
                }

                std::vector<std::string> object_data_id;
                boost::split(object_data_id, object_data[0], boost::is_any_of("="), boost::token_compress_off);
                if (object_data_id.size() != 2) {
                    add_error("Error while reading object id");
                    continue;
                }

                int object_id = std::atoi(object_data_id[1].c_str());
                if (object_id == 0) {
                    add_error("Found invalid object id");
                    continue;
                }

                IdToSlaDrainHolesMap::iterator object_item = m_sla_drain_holes.find(object_id);
                if (object_item != m_sla_drain_holes.end()) {
                    add_error("Found duplicated SLA drain holes");
                    continue;
                }

                std::vector<std::string> object_data_points;
                boost::split(object_data_points, object_data[1], boost::is_any_of(" "), boost::token_compress_off);

                sla::DrainHoles sla_drain_holes;

                if (version == 1) {
                    for (unsigned int i=0; i<object_data_points.size(); i+=8)
                        sla_drain_holes.emplace_back(Vec3f{float(std::atof(object_data_points[i+0].c_str())),
                                                      float(std::atof(object_data_points[i+1].c_str())),
                                                      float(std::atof(object_data_points[i+2].c_str()))},
                                                     Vec3f{float(std::atof(object_data_points[i+3].c_str())),
                                                      float(std::atof(object_data_points[i+4].c_str())),
                                                      float(std::atof(object_data_points[i+5].c_str()))},
                                                      float(std::atof(object_data_points[i+6].c_str())),
                                                      float(std::atof(object_data_points[i+7].c_str())));
                }

                // The holes are saved elevated above the mesh and deeper (bad idea indeed).
                // This is retained for compatibility.
                // Place the hole to the mesh and make it shallower to compensate.
                // The offset is 1 mm above the mesh.
                for (sla::DrainHole& hole : sla_drain_holes) {
                    hole.pos += hole.normal.normalized();
                    hole.height -= 1.f;
                }

                if (!sla_drain_holes.empty())
                    m_sla_drain_holes.insert({ object_id, sla_drain_holes });
            }
        }
    }*/

    void _BBS_3MF_Importer::_extract_custom_gcode_per_print_z_from_archive(::mz_zip_archive &archive, const mz_zip_archive_file_stat &stat)
    {
        //BBS: add plate tree related logic
        if (stat.m_uncomp_size > 0) {
            std::string buffer((size_t)stat.m_uncomp_size, 0);
            mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
            if (res == 0) {
                add_error("Error while reading custom Gcodes per height data to buffer");
                return;
            }

            std::istringstream iss(buffer); // wrap returned xml to istringstream
            pt::ptree main_tree;
            pt::read_xml(iss, main_tree);

            if (main_tree.front().first != "custom_gcodes_per_layer")
                return;

            auto extract_code = [this](int plate_id, pt::ptree code_tree) {
                for (const auto& code : code_tree) {
                    if (code.first == "mode") {
                        pt::ptree tree = code.second;
                        std::string mode = tree.get<std::string>("<xmlattr>.value");
                    }
                    if (code.first == "layer") {
                        pt::ptree tree = code.second;
                        double print_z = tree.get<double>("<xmlattr>.top_z");
                        int extruder = tree.get<int>("<xmlattr>.extruder");
                        std::string color = tree.get<std::string>("<xmlattr>.color");

                        CustomGCode::Type   type;
                        std::string         extra;
                        pt::ptree attr_tree = tree.find("<xmlattr>")->second;
                        if (attr_tree.find("type") == attr_tree.not_found()) {
                            // It means that data was saved in old version (2.2.0 and older) of PrusaSlicer
                            // read old data ...
                            std::string gcode = tree.get<std::string>("<xmlattr>.gcode");
                            // ... and interpret them to the new data
                            type = gcode == "M600" ? CustomGCode::ColorChange :
                                gcode == "M601" ? CustomGCode::PausePrint :
                                gcode == "tool_change" ? CustomGCode::ToolChange : CustomGCode::Custom;
                            extra = type == CustomGCode::PausePrint ? color :
                                type == CustomGCode::Custom ? gcode : "";
                        }
                        else {
                            type = static_cast<CustomGCode::Type>(tree.get<int>("<xmlattr>.type"));
                            extra = tree.get<std::string>("<xmlattr>.extra");
                        }
                    }
                }
            };

            bool has_plate_info = false;
            for (const auto& element : main_tree.front().second) {
                if (element.first == "plate") {
                    has_plate_info = true;

                    int plate_id = -1;
                    pt::ptree code_tree = element.second;
                    for (const auto& code : code_tree) {
                        if (code.first == "plate_info") {
                            plate_id = code.second.get<int>("<xmlattr>.id");
                        }

                    }
                    if (plate_id == -1)
                        continue;

                    extract_code(plate_id, code_tree);
                }
            }

            if (!has_plate_info) {
                int plate_id = 1;
                pt::ptree code_tree = main_tree.front().second;
                extract_code(plate_id, code_tree);
            }
        }
    }

    void _BBS_3MF_Importer::_handle_start_model_xml_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_start_model(attributes, num_attributes);
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_start_resources(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_start_object(attributes, num_attributes);
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_start_color_group(attributes, num_attributes);
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_start_color(attributes, num_attributes);
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_start_mesh(attributes, num_attributes);
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_start_vertices(attributes, num_attributes);
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_start_vertex(attributes, num_attributes);
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_start_triangles(attributes, num_attributes);
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_start_triangle(attributes, num_attributes);
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_start_components(attributes, num_attributes);
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_start_component(attributes, num_attributes);
        else if (::strcmp(BUILD_TAG, name) == 0)
            res = _handle_start_build(attributes, num_attributes);
        else if (::strcmp(ITEM_TAG, name) == 0)
            res = _handle_start_item(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_start_metadata(attributes, num_attributes);

        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_end_model_xml_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_end_model();
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_end_resources();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_end_object();
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_end_color_group();
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_end_color();
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_end_mesh();
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_end_vertices();
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_end_vertex();
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_end_triangles();
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_end_triangle();
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_end_components();
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_end_component();
        else if (::strcmp(BUILD_TAG, name) == 0)
            res = _handle_end_build();
        else if (::strcmp(ITEM_TAG, name) == 0)
            res = _handle_end_item();
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_end_metadata();

        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_xml_characters(const XML_Char* s, int len)
    {
        m_curr_characters.append(s, len);
    }

    void _BBS_3MF_Importer::_handle_start_config_xml_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(CONFIG_TAG, name) == 0)
            res = _handle_start_config(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_start_config_object(attributes, num_attributes);
        else if (::strcmp(VOLUME_TAG, name) == 0)
            res = _handle_start_config_volume(attributes, num_attributes);
        else if (::strcmp(PART_TAG, name) == 0)
            res = _handle_start_config_volume(attributes, num_attributes);
        else if (::strcmp(MESH_STAT_TAG, name) == 0)
            res = _handle_start_config_volume_mesh(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_start_config_metadata(attributes, num_attributes);
        else if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_start_config_plater(attributes, num_attributes);
        else if (::strcmp(INSTANCE_TAG, name) == 0)
            res = _handle_start_config_plater_instance(attributes, num_attributes);
        else if (::strcmp(FILAMENT_TAG, name) == 0)
            res = _handle_start_config_filament(attributes, num_attributes);
        else if (::strcmp(SLICE_WARNING_TAG, name) == 0)
            res = _handle_start_config_warning(attributes, num_attributes);
        else if (::strcmp(ASSEMBLE_TAG, name) == 0)
            res = _handle_start_assemble(attributes, num_attributes);
        else if (::strcmp(ASSEMBLE_ITEM_TAG, name) == 0)
            res = _handle_start_assemble_item(attributes, num_attributes);
        else if (::strcmp(TEXT_INFO_TAG, name) == 0)
            res = _handle_start_text_info_item(attributes, num_attributes);

        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_end_config_xml_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(CONFIG_TAG, name) == 0)
            res = _handle_end_config();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_end_config_object();
        else if (::strcmp(VOLUME_TAG, name) == 0)
            res = _handle_end_config_volume();
        else if (::strcmp(PART_TAG, name) == 0)
            res = _handle_end_config_volume();
        else if (::strcmp(MESH_STAT_TAG, name) == 0)
            res = _handle_end_config_volume_mesh();
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_end_config_metadata();
        else if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_end_config_plater();
        else if (::strcmp(FILAMENT_TAG, name) == 0)
            res = _handle_end_config_filament();
        else if (::strcmp(INSTANCE_TAG, name) == 0)
            res = _handle_end_config_plater_instance();
        else if (::strcmp(ASSEMBLE_TAG, name) == 0)
            res = _handle_end_assemble();
        else if (::strcmp(ASSEMBLE_ITEM_TAG, name) == 0)
            res = _handle_end_assemble_item();

        if (!res)
            _stop_xml_parser();
    }

    bool _BBS_3MF_Importer::_handle_start_model(const char** attributes, unsigned int num_attributes)
    {
        m_unit_factor = bbs_get_unit_factor(bbs_get_attribute_value_string(attributes, num_attributes, UNIT_ATTR));

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_model()
    {
        // BBS: Production Extension
        if (!m_sub_model_path.empty())
            return true;

        // deletes all non-built or non-instanced objects
        for (const IdToModelObjectMap::value_type& object : m_objects) {
            if (object.second >= int(m_model->objects.size())) {
                add_error("Unable to find object");
                return false;
            }
            ModelObject *model_object = m_model->objects[object.second];
            if (model_object != nullptr && model_object->instances.size() == 0)
                m_model->delete_object(model_object);
        }

        //construct the index maps
        for (const IdToCurrentObjectMap::value_type& object : m_current_objects) {
            m_index_paths.insert({ object.first.second, object.first.first});
        }

        if (!m_is_bbl_3mf) {
            // if the 3mf was not produced by BambuStudio and there is only one object,
            // set the object name to match the filename
            if (m_model->objects.size() == 1)
                m_model->objects.front()->name = m_name;
        }

        // applies instances' matrices
        for (Instance& instance : m_instances) {
            if (instance.instance != nullptr && instance.instance->get_object() != nullptr)
                // apply the transform to the instance
                _apply_transform(*instance.instance, instance.transform);
        }

        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_resources(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_resources()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_object(const char** attributes, unsigned int num_attributes)
    {
        // reset current object data
        if (m_curr_object) {
            delete m_curr_object;
            m_curr_object = nullptr;
        }

        std::string object_type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);

        if (bbs_is_valid_object_type(object_type)) {
            if (!m_curr_object) {
                m_curr_object = new CurrentObject();
                // create new object (it may be removed later if no instances are generated from it)
                /*m_curr_object->model_object_idx = (int)m_model->objects.size();
                m_curr_object.object = m_model->add_object();
                if (m_curr_object.object == nullptr) {
                    add_error("Unable to create object");
                    return false;
                }*/
            }

            m_curr_object->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            m_curr_object->name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);

            m_curr_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            if (m_curr_object->uuid.empty()) {
                m_curr_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_LOWER_ATTR);
            }
            m_curr_object->pid = bbs_get_attribute_value_int(attributes, num_attributes, PID_ATTR);
        }

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_object()
    {
        if (!m_load_model) {
            delete m_curr_object;
            m_curr_object = nullptr;
            return true;
        }
        if (!m_curr_object || (m_curr_object->id == -1)) {
            add_error("Found invalid object");
            return false;
        }
        else {
            if (m_is_bbl_3mf && boost::ends_with(m_curr_object->uuid, OBJECT_UUID_SUFFIX) && m_load_restore) {
                // Adjust backup object/volume id
                std::istringstream iss(m_curr_object->uuid);
                int backup_id;
                bool need_replace = false;
                if (iss >> std::hex >> backup_id) {
                    need_replace = (m_curr_object->id != backup_id);
                    m_curr_object->id = backup_id;
                }
                if (!m_curr_object->components.empty())
                {
                    Id first_id = m_curr_object->components.front().object_id;
                    first_id.second = 0;
                    IdToCurrentObjectMap::iterator current_object = m_current_objects.lower_bound(first_id);
                    IdToCurrentObjectMap new_map;
                    for (int index = 0; index < m_curr_object->components.size(); index++)
                    {
                        Component& component = m_curr_object->components[index];
                        Id new_id = component.object_id;
                        new_id.second = (index + 1) << 16 | backup_id;
                        if (current_object != m_current_objects.end()
                                && (new_id.first.empty() || new_id.first == current_object->first.first)) {
                            current_object->second.id   = new_id.second;
                            new_map.insert({new_id, std::move(current_object->second)});
                            current_object = m_current_objects.erase(current_object);
                        }
                        else {
                            add_error("can not find object for component, id=" + std::to_string(component.object_id.second));
                            delete m_curr_object;
                            m_curr_object = nullptr;
                            return false;
                        }
                        component.object_id.second = new_id.second;
                    }
                    for (auto & obj : new_map)
                        m_current_objects.insert({obj.first, std::move(obj.second)});
                }
            }

            Id id = std::make_pair(m_sub_model_path, m_curr_object->id);
            if (m_current_objects.find(id) == m_current_objects.end()) {
                m_current_objects.insert({ id, std::move(*m_curr_object) });
                delete m_curr_object;
                m_curr_object = nullptr;
            }
            else {
                add_error("Found object with duplicate id");
                delete m_curr_object;
                m_curr_object = nullptr;
                return false;
            }
        }

        /*if (m_curr_object.object != nullptr) {
            if (m_curr_object.id != -1) {
                if (m_curr_object.geometry.empty()) {
                    // no geometry defined
                    // remove the object from the model
                    m_model->delete_object(m_curr_object.object);

                    if (m_curr_object.components.empty()) {
                        // no components defined -> invalid object, delete it
                        IdToModelObjectMap::iterator object_item = m_objects.find(id);
                        if (object_item != m_objects.end())
                            m_objects.erase(object_item);

                        IdToAliasesMap::iterator alias_item = m_objects_aliases.find(id);
                        if (alias_item != m_objects_aliases.end())
                            m_objects_aliases.erase(alias_item);
                    }
                    else
                        // adds components to aliases
                        m_objects_aliases.insert({ id, m_curr_object.components });
                }
                else {
                    // geometry defined, store it for later use
                    m_geometries.insert({ id, std::move(m_curr_object.geometry) });

                    // stores the object for later use
                    if (m_objects.find(id) == m_objects.end()) {
                        m_objects.insert({ id, m_curr_object.model_object_idx });
                        m_objects_aliases.insert({ id, { 1, Component(m_curr_object.id) } }); // aliases itself
                    }
                    else {
                        add_error("Found object with duplicate id");
                        return false;
                    }
                }
            }
            else {
                //sub objects
            }
        }*/

        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_color_group(const char **attributes, unsigned int num_attributes)
    {
        m_current_color_group = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_color_group()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_color(const char **attributes, unsigned int num_attributes)
    {
        std::string color = bbs_get_attribute_value_string(attributes, num_attributes, COLOR_ATTR);
        m_group_id_to_color[m_current_color_group] = color;
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_color()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_mesh(const char** attributes, unsigned int num_attributes)
    {
        // reset current geometry
        if (m_curr_object)
            m_curr_object->geometry.reset();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_mesh()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_vertices(const char** attributes, unsigned int num_attributes)
    {
        // reset current vertices
        if (m_curr_object)
            m_curr_object->geometry.vertices.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_vertices()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_vertex(const char** attributes, unsigned int num_attributes)
    {
        // appends the vertex coordinates
        // missing values are set equal to ZERO
        if (m_curr_object)
            m_curr_object->geometry.vertices.emplace_back(
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, X_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Y_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Z_ATTR));
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_vertex()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_triangles(const char** attributes, unsigned int num_attributes)
    {
        // reset current triangles
        if (m_curr_object)
            m_curr_object->geometry.triangles.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_triangles()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_triangle(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes:
        // p1
        // p2
        // p3
        // pid
        // see specifications

        // appends the triangle's vertices indices
        // missing values are set equal to ZERO
        if (m_curr_object) {
            m_curr_object->geometry.triangles.emplace_back(
                bbs_get_attribute_value_int(attributes, num_attributes, V1_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V2_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V3_ATTR));

            m_curr_object->geometry.custom_supports.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SUPPORTS_ATTR));
            m_curr_object->geometry.custom_seam.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SEAM_ATTR));
            m_curr_object->geometry.mmu_segmentation.push_back(bbs_get_attribute_value_string(attributes, num_attributes, MMU_SEGMENTATION_ATTR));
            // BBS
            m_curr_object->geometry.face_properties.push_back(bbs_get_attribute_value_string(attributes, num_attributes, FACE_PROPERTY_ATTR));
        }
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_triangle()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_components(const char** attributes, unsigned int num_attributes)
    {
        // reset current components
        if (m_curr_object)
            m_curr_object->components.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_components()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_component(const char** attributes, unsigned int num_attributes)
    {
        std::string path      = xml_unescape(bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR));
        int         object_id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
        Transform3d transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));

        /*Id id = std::make_pair(m_sub_model_path, object_id);
        IdToModelObjectMap::iterator object_item = m_objects.find(id);
        if (object_item == m_objects.end()) {
            IdToAliasesMap::iterator alias_item = m_objects_aliases.find(id);
            if (alias_item == m_objects_aliases.end()) {
                add_error("Found component with invalid object id");
                return false;
            }
        }*/

        if (m_curr_object) {
            Id id = std::make_pair(m_sub_model_path.empty() ? path : m_sub_model_path, object_id);
            m_curr_object->components.emplace_back(id, transform);
        }

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_component()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_build(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_build()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_item(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes
        // thumbnail
        // partnumber
        // pid
        // pindex
        // see specifications

        int object_id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
        std::string path = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
        Transform3d transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));
        int printable = bbs_get_attribute_value_bool(attributes, num_attributes, PRINTABLE_ATTR);

        return !m_load_model || _create_object_instance(path, object_id, transform, printable, 1);
    }

    bool _BBS_3MF_Importer::_handle_end_item()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_metadata(const char** attributes, unsigned int num_attributes)
    {
        m_curr_characters.clear();

        std::string name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);
        if (!name.empty()) {
            m_curr_metadata_name = name;
        }

        return true;
    }

    inline static void check_painting_version(unsigned int loaded_version, unsigned int highest_supported_version, const std::string &error_msg)
    {
        if (loaded_version > highest_supported_version)
            throw version_error(error_msg);
    }

    bool _BBS_3MF_Importer::_handle_end_metadata()
    {
        if ((m_curr_metadata_name == BBS_3MF_VERSION)||(m_curr_metadata_name == BBS_3MF_VERSION1)) {
            //m_is_bbl_3mf = true;
            m_version = (unsigned int)atoi(m_curr_characters.c_str());
            /*if (m_check_version && (m_version > VERSION_BBS_3MF_COMPATIBLE)) {
                // std::string msg = _(L("The selected 3mf file has been saved with a newer version of " + std::string(SLIC3R_APP_NAME) + " and is not compatible."));
                // throw version_error(msg.c_str());
                const std::string msg = (boost::format(_(L("The selected 3mf file has been saved with a newer version of %1% and is not compatible."))) % std::string(SLIC3R_APP_NAME)).str();
                throw version_error(msg);
            }*/
        } else if (m_curr_metadata_name == BBL_APPLICATION_TAG) {
            // Generator application of the 3MF.
            // SLIC3R_APP_KEY - SLIC3R_VERSION
            if (boost::starts_with(m_curr_characters, "BambuStudio-")) {
                m_is_bbl_3mf = true;
                m_bambuslicer_generator_version = Semver::parse(m_curr_characters.substr(12));
            }
        //TODO: currently use version 0, no need to load&&save this string
        /*} else if (m_curr_metadata_name == BBS_FDM_SUPPORTS_PAINTING_VERSION) {
            m_fdm_supports_painting_version = (unsigned int) atoi(m_curr_characters.c_str());
            check_painting_version(m_fdm_supports_painting_version, FDM_SUPPORTS_PAINTING_VERSION,
                _(L("The selected 3MF contains FDM supports painted object using a newer version of BambuStudio and is not compatible.")));
        } else if (m_curr_metadata_name == BBS_SEAM_PAINTING_VERSION) {
            m_seam_painting_version = (unsigned int) atoi(m_curr_characters.c_str());
            check_painting_version(m_seam_painting_version, SEAM_PAINTING_VERSION,
                _(L("The selected 3MF contains seam painted object using a newer version of BambuStudio and is not compatible.")));
        } else if (m_curr_metadata_name == BBS_MM_PAINTING_VERSION) {
            m_mm_painting_version = (unsigned int) atoi(m_curr_characters.c_str());
            check_painting_version(m_mm_painting_version, MM_PAINTING_VERSION,
                _(L("The selected 3MF contains multi-material painted object using a newer version of BambuStudio and is not compatible.")));*/
        } else if (m_curr_metadata_name == BBL_MODEL_ID_TAG) {
            m_model_id = xml_unescape(m_curr_characters);
        } else if (m_curr_metadata_name == BBL_CREATION_DATE_TAG) {
            ;
        } else if (m_curr_metadata_name == BBL_MODIFICATION_TAG) {
            ;
        } else {
            ;
        }

        return true;
    }

    bool _BBS_3MF_Importer::_create_object_instance(std::string const & path, int object_id, const Transform3d& transform, const bool printable, unsigned int recur_counter)
    {
        static const unsigned int MAX_RECURSIONS = 10;

        // escape from circular aliasing
        if (recur_counter > MAX_RECURSIONS) {
            add_error("Too many recursions");
            return false;
        }

        Id id{path, object_id};
        IdToCurrentObjectMap::iterator it = m_current_objects.find(id);
        if (it == m_current_objects.end()) {
            add_error("can not find object id " + std::to_string(object_id) + " to builditem");
            return false;
        }

        IdToModelObjectMap::iterator object_item = m_objects.find(id);
        if (object_item == m_objects.end()) {
            //add object
            CurrentObject& current_object = it->second;
            int object_index =  (int)m_model->objects.size();
            ModelObject* model_object = m_model->add_object();
            if (model_object == nullptr) {
                add_error("Unable to create object for builditem, id " + std::to_string(object_id));
                return false;
            }
            m_objects.insert({ id, object_index });
            current_object.model_object_idx = object_index;
            current_object.object = model_object;

            ModelInstance* instance = m_model->objects[object_index]->add_instance();
            if (instance == nullptr) {
                add_error("error when add object instance for id " + std::to_string(object_id));
                return false;
            }
            instance->printable = printable;

            m_instances.emplace_back(instance, transform);

            if (m_is_bbl_3mf && boost::ends_with(current_object.uuid, OBJECT_UUID_SUFFIX)) {
                std::istringstream iss(current_object.uuid);
                int backup_id;
                if (iss >> std::hex >> backup_id) {

                }
            }
            /*if (!current_object.geometry.empty()) {
            }
            else if (!current_object.components.empty()) {
                 // recursively process nested components
                for (const Component& component : it->second) {
                    if (!_create_object_instance(path, component.object_id, transform * component.transform, printable, recur_counter + 1))
                        return false;
                }
            }
            else {
                add_error("can not construct build items with invalid object, id " + std::to_string(object_id));
                return false;
            }*/
        }
        else {
            //add instance
            ModelInstance* instance = m_model->objects[object_item->second]->add_instance();
            if (instance == nullptr) {
                add_error("error when add object instance for id " + std::to_string(object_id));
                return false;
            }
            instance->printable = printable;

            m_instances.emplace_back(instance, transform);
        }

        /*if (it->second.size() == 1 && it->second[0].object_id == object_id) {
            // aliasing to itself

            IdToModelObjectMap::iterator object_item = m_objects.find(id);
            if (object_item == m_objects.end() || object_item->second == -1) {
                add_error("Found invalid object");
                return false;
            }
            else {
                ModelInstance* instance = m_model->objects[object_item->second]->add_instance();
                if (instance == nullptr) {
                    add_error("Unable to add object instance");
                    return false;
                }
                instance->printable = printable;

                m_instances.emplace_back(instance, transform);
            }
        }
        else {
            // recursively process nested components
            for (const Component& component : it->second) {
                if (!_create_object_instance(path, component.object_id, transform * component.transform, printable, recur_counter + 1))
                    return false;
            }
        }*/

        return true;
    }

    void _BBS_3MF_Importer::_apply_transform(ModelInstance& instance, const Transform3d& transform)
    {
        Slic3r::Geometry::Transformation t(transform);
        // invalid scale value, return
        if (!t.get_scaling_factor().all())
            return;

        instance.set_transformation(t);
    }

    bool _BBS_3MF_Importer::_handle_start_config(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_object(const char** attributes, unsigned int num_attributes)
    {
        if (m_parsing_slice_info)
            return true;
        int object_id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        IdToMetadataMap::iterator object_item = m_objects_metadata.find(object_id);
        if (object_item != m_objects_metadata.end()) {
            add_error("Duplicated object id: " + std::to_string(object_id) + " in model_settings.config");
            return false;
        }

        // Added because of github #3435, currently not used by PrusaSlicer
        // int instances_count_id = bbs_get_attribute_value_int(attributes, num_attributes, INSTANCESCOUNT_ATTR);

        m_objects_metadata.insert({ object_id, ObjectMetadata() });
        m_curr_config.object_id = object_id;
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_object()
    {
        m_curr_config.object_id = -1;
        return true;
    }

    //BBS: refine the model part names
    ModelVolumeType bbs_type_from_string(const std::string& s)
    {
        // New type (supporting the support enforcers & blockers)
        if (s == "normal_part")
            return ModelVolumeType::MODEL_PART;
        if (s == "negative_part")
            return ModelVolumeType::NEGATIVE_VOLUME;
        if (s == "modifier_part")
            return ModelVolumeType::PARAMETER_MODIFIER;
        if (s == "support_enforcer")
            return ModelVolumeType::SUPPORT_ENFORCER;
        if (s == "support_blocker")
            return ModelVolumeType::SUPPORT_BLOCKER;
        //assert(s == "0");
        // Default value if invalud type string received.
        return ModelVolumeType::MODEL_PART;
    }

    bool _BBS_3MF_Importer::_handle_start_config_volume(const char** attributes, unsigned int num_attributes)
    {
        IdToMetadataMap::iterator object = m_objects_metadata.find(m_curr_config.object_id);
        if (object == m_objects_metadata.end()) {
            add_error("can not find object for part, id " + std::to_string(m_curr_config.object_id) );
            return false;
        }

        m_curr_config.volume_id = (int)object->second.volumes.size();

        unsigned int first_triangle_id = (unsigned int)bbs_get_attribute_value_int(attributes, num_attributes, FIRST_TRIANGLE_ID_ATTR);
        unsigned int last_triangle_id = (unsigned int)bbs_get_attribute_value_int(attributes, num_attributes, LAST_TRIANGLE_ID_ATTR);

        //BBS: refine the part type logic
        std::string subtype_str = bbs_get_attribute_value_string(attributes, num_attributes, SUBTYPE_ATTR);
        ModelVolumeType type = bbs_type_from_string(subtype_str);

        int subbject_id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);

        if (last_triangle_id > 0)
            object->second.volumes.emplace_back(first_triangle_id, last_triangle_id, type);
        else
            object->second.volumes.emplace_back(subbject_id, type);
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_volume_mesh(const char** attributes, unsigned int num_attributes)
    {
        IdToMetadataMap::iterator object = m_objects_metadata.find(m_curr_config.object_id);
        if (object == m_objects_metadata.end()) {
            add_error("can not find object for mesh_stats, id " + std::to_string(m_curr_config.object_id) );
            return false;
        }
        if ((m_curr_config.volume_id == -1) || ((object->second.volumes.size() - 1) < m_curr_config.volume_id)) {
            add_error("can not find part for mesh_stats");
            return false;
        }

        ObjectMetadata::VolumeMetadata& volume = object->second.volumes[m_curr_config.volume_id];

        int edges_fixed         = bbs_get_attribute_value_int(attributes, num_attributes, MESH_STAT_EDGES_FIXED       );
        int degenerate_facets   = bbs_get_attribute_value_int(attributes, num_attributes, MESH_STAT_DEGENERATED_FACETS);
        int facets_removed      = bbs_get_attribute_value_int(attributes, num_attributes, MESH_STAT_FACETS_REMOVED    );
        int facets_reversed     = bbs_get_attribute_value_int(attributes, num_attributes, MESH_STAT_FACETS_RESERVED   );
        int backwards_edges     = bbs_get_attribute_value_int(attributes, num_attributes, MESH_STAT_BACKWARDS_EDGES   );

        volume.mesh_stats = { edges_fixed, degenerate_facets, facets_removed, facets_reversed, backwards_edges };

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_volume()
    {
        m_curr_config.volume_id = -1;
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_volume_mesh()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_metadata(const char** attributes, unsigned int num_attributes)
    {
        //std::string type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);
        std::string key = bbs_get_attribute_value_string(attributes, num_attributes, KEY_ATTR);
        std::string value = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);

        if (!m_parsing_slice_info)
        {
            IdToMetadataMap::iterator object = m_objects_metadata.find(m_curr_config.object_id);
            if (object == m_objects_metadata.end()) {
                add_error("Cannot find object for metadata, id " + std::to_string(m_curr_config.object_id));
                return false;
            }
            if (m_curr_config.volume_id == -1)
                object->second.metadata.emplace_back(key, value);
            else {
                if (size_t(m_curr_config.volume_id) < object->second.volumes.size())
                    object->second.volumes[m_curr_config.volume_id].metadata.emplace_back(key, value);
            }
        }
        else
        {
            if (key == INSTANCEID_ATTR)
            {
                m_curr_instance.instance_id = atoi(value.c_str());
            }
            else if (key == IDENTIFYID_ATTR)
            {
                m_curr_instance.identify_id = atoi(value.c_str());
            }
            else if (key == OBJECT_ID_ATTR)
            {
                m_curr_instance.object_id = atoi(value.c_str());
                /*int obj_id = atoi(value.c_str());
                m_curr_instance.object_id = -1;
                IndexToPathMap::iterator index_iter = m_index_paths.find(obj_id);
                if (index_iter == m_index_paths.end()) {
                    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ":" << __LINE__
                        << boost::format(", can not find object for plate's item, id=%1%, skip this object")%obj_id;
                    return true;
                }
                Id temp_id = std::make_pair(index_iter->second, index_iter->first);
                IdToModelObjectMap::iterator object_item = m_objects.find(temp_id);
                if (object_item == m_objects.end()) {
                    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ":" << __LINE__
                        << boost::format(", can not find object for plate's item, ID <%1%, %2%>, skip this object")%index_iter->second %index_iter->first;
                    return true;
                }
                m_curr_instance.object_id = object_item->second;*/
            }
        }

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_metadata()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_filament(const char** attributes, unsigned int num_attributes)
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_filament()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_warning(const char** attributes, unsigned int num_attributes)
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_warning()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_plater(const char** attributes, unsigned int num_attributes)
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_plater()
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_config_plater_instance(const char** attributes, unsigned int num_attributes)
    {
        //do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_config_plater_instance()
    {
        if ((m_curr_instance.object_id == -1) || (m_curr_instance.instance_id == -1))
        {
            //add_error("invalid object id/instance id");
            //skip this instance
            m_curr_instance.object_id = m_curr_instance.instance_id = -1;
            m_curr_instance.identify_id = 0;
            return true;
        }

        m_curr_instance.object_id = m_curr_instance.instance_id = -1;
        m_curr_instance.identify_id = 0;
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_assemble(const char** attributes, unsigned int num_attributes)
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_assemble()
    {
        //do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_assemble_item(const char** attributes, unsigned int num_attributes)
    {
        if (!m_load_model) return true;

        int object_id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECT_ID_ATTR);
        int instance_id = bbs_get_attribute_value_int(attributes, num_attributes, INSTANCEID_ATTR);

        IndexToPathMap::iterator index_iter = m_index_paths.find(object_id);
        if (index_iter == m_index_paths.end()) {
            add_error("can not find object for assemble item, id= " + std::to_string(object_id));
            return false;
        }
        Id temp_id = std::make_pair(index_iter->second, index_iter->first);
        IdToModelObjectMap::iterator object_item = m_objects.find(temp_id);
        if (object_item == m_objects.end()) {
            add_error("can not find object for assemble item, id= " + std::to_string(object_id));
            return false;
        }
        object_id = object_item->second;

        Transform3d transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));
        Vec3d ofs2ass = bbs_get_offset_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, OFFSET_ATTR));
        if (object_id < m_model->objects.size()) {
            if (instance_id < m_model->objects[object_id]->instances.size()) {
                //m_model->objects[object_id]->instances[instance_id]->set_assemble_from_transform(transform);
                //m_model->objects[object_id]->instances[instance_id]->set_offset_to_assembly(ofs2ass);
            }
        }
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_assemble_item()
    {
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_text_info_item(const char **attributes, unsigned int num_attributes)
    {
        IdToMetadataMap::iterator object = m_objects_metadata.find(m_curr_config.object_id);
        if (object == m_objects_metadata.end()) {
            add_error("can not find object for text_info, id " + std::to_string(m_curr_config.object_id));
            return false;
        }
        if ((m_curr_config.volume_id == -1) || ((object->second.volumes.size() - 1) < m_curr_config.volume_id)) {
            add_error("can not find part for text_info");
            return false;
        }
 
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_text_info_item()
    {
        return true;
    }

    void XMLCALL _BBS_3MF_Importer::_handle_start_relationships_element(void* userData, const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_start_relationships_element(name, attributes);
    }

    void XMLCALL _BBS_3MF_Importer::_handle_end_relationships_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_end_relationships_element(name);
    }

    void _BBS_3MF_Importer::_handle_start_relationships_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(RELATIONSHIP_TAG, name) == 0)
            res = _handle_start_relationship(attributes, num_attributes);

        m_curr_characters.clear();
        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_end_relationships_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (!res)
            _stop_xml_parser();
    }

    bool _BBS_3MF_Importer::_handle_start_relationship(const char** attributes, unsigned int num_attributes)
    {
        std::string path = bbs_get_attribute_value_string(attributes, num_attributes, TARGET_ATTR);
        std::string type = bbs_get_attribute_value_string(attributes, num_attributes, RELS_TYPE_ATTR);
        if (boost::starts_with(type, "http://schemas.microsoft.com/3dmanufacturing/") && boost::ends_with(type, "3dmodel")) {
            if (m_start_part_path.empty()) m_start_part_path = path;
            else m_sub_model_paths.push_back(path);
        } else if (boost::starts_with(type, "http://schemas.openxmlformats.org/") && boost::ends_with(type, "thumbnail")) {
            if (boost::algorithm::ends_with(path, ".png"))
                m_thumbnail_path = path;
        } else if (boost::starts_with(type, "http://schemas.bambulab.com/") && boost::ends_with(type, "cover-thumbnail-middle")) {
            m_thumbnail_middle = path;
        } else if (boost::starts_with(type, "http://schemas.bambulab.com/") && boost::ends_with(type, "cover-thumbnail-small")) {
            m_thumbnail_small = path;
        }
        return true;
    }

    void _BBS_3MF_Importer::_generate_current_object_list(std::vector<Component> &sub_objects, Id object_id, IdToCurrentObjectMap &current_objects)
    {
        std::list<std::pair<Component, Transform3d>> id_list;
        id_list.push_back(std::make_pair(Component(object_id, Transform3d::Identity()), Transform3d::Identity()));

        while (!id_list.empty())
        {
            auto current_item = id_list.front();
            Component current_id = current_item.first;
            id_list.pop_front();
            IdToCurrentObjectMap::iterator current_object = current_objects.find(current_id.object_id);
            if (current_object != current_objects.end()) {
                //found one
                if (!current_object->second.components.empty()) {
                    for (const Component &comp : current_object->second.components) {
                        id_list.push_back(std::pair(comp, current_item.second * comp.transform));
                    }
                }
                else if (!(current_object->second.geometry.empty())) {
                    //CurrentObject* ptr = &(current_objects[current_id]);
                    //CurrentObject* ptr2 = &(current_object->second);
                    sub_objects.push_back({ current_object->first, current_item.second});
                }
            }
        }
    }

    bool _BBS_3MF_Importer::_generate_volumes_new(ModelObject& object, const std::vector<Component> &sub_objects, const ObjectMetadata::VolumeMetadataList& volumes, ConfigSubstitutionContext& config_substitutions)
    {
        if (!object.volumes.empty()) {
            add_error("object already built with parts");
            return false;
        }

        //unsigned int geo_tri_count = (unsigned int)geometry.triangles.size();
        unsigned int renamed_volumes_count = 0;

        for (unsigned int index = 0; index < sub_objects.size(); index++)
        {
            //find the volume metadata firstly
            Component sub_comp = sub_objects[index];
            Id object_id = sub_comp.object_id;
            IdToCurrentObjectMap::iterator current_object = m_current_objects.find(object_id);
            if (current_object == m_current_objects.end()) {
                add_error("sub_objects can not be found, id=" + std::to_string(object_id.second));
                return false;
            }
            CurrentObject* sub_object = &(current_object->second);

            const ObjectMetadata::VolumeMetadata* volume_data = nullptr;
            ObjectMetadata::VolumeMetadata default_volume_data(sub_object->id);
            if (index < volumes.size() && volumes[index].subobject_id == sub_object->id)
                volume_data = &volumes[index];
            else for (const ObjectMetadata::VolumeMetadata& volume_iter : volumes) {
                if (volume_iter.subobject_id == sub_object->id) {
                    volume_data = &volume_iter;
                    break;
                }
            }

            Transform3d volume_matrix_to_object = Transform3d::Identity();
            bool        has_transform 		    = false;
            int         shared_mesh_id          = object_id.second;
            if (volume_data)
            {
                int found_count = 0;
                // extract the volume transformation from the volume's metadata, if present
                for (const Metadata& metadata : volume_data->metadata) {
                    if (metadata.key == MATRIX_KEY) {
                        volume_matrix_to_object = Slic3r::Geometry::transform3d_from_string(metadata.value);
                        has_transform 			= ! volume_matrix_to_object.isApprox(Transform3d::Identity(), 1e-10);
                        found_count++;
                    }
                    else if (metadata.key == MESH_SHARED_KEY){
                        //add the shared mesh logic
                        shared_mesh_id = ::atoi(metadata.value.c_str());
                        found_count++;
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": line %1%, shared_mesh_id %2%")%__LINE__%shared_mesh_id;
                    }

                    if (found_count >= 2)
                        break;
                }
            }
            else {
                //create a volume_data
                volume_data = &default_volume_data;
            }

            ModelVolume* volume = nullptr;
            ModelVolume *shared_volume = nullptr;
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": line %1%, subobject_id %2%, shared_mesh_id %3%")%__LINE__ %sub_object->id %shared_mesh_id;
            if (shared_mesh_id != -1) {
                std::map<int, ModelVolume*>::iterator iter = m_shared_meshes.find(shared_mesh_id);
                if (iter != m_shared_meshes.end()) {
                    shared_volume = iter->second;
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": line %1%, found shared mesh, id %2%, mesh %3%")%__LINE__%shared_mesh_id%shared_volume;
                }
            }
            else {
                //for some cases, object point to this shared mesh already loaded, treat that one as the root
                std::map<int, ModelVolume*>::iterator iter = m_shared_meshes.find(sub_object->id);
                if (iter != m_shared_meshes.end()) {
                    shared_volume = iter->second;
                    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": line %1%, already loaded copy-share mesh before, id %2%, mesh %3%")%__LINE__%sub_object->id%shared_volume;
                }
            }

            const size_t triangles_count = sub_object->geometry.triangles.size();
            if (triangles_count == 0) {
                add_error("found no trianges in the object " + std::to_string(sub_object->id));
                return false;
            }
            if (!shared_volume){
                // splits volume out of imported geometry
                indexed_triangle_set its;
                its.indices.assign(sub_object->geometry.triangles.begin(), sub_object->geometry.triangles.end());
                //const size_t triangles_count = its.indices.size();
                //if (triangles_count == 0) {
                //    add_error("found no trianges in the object " + std::to_string(sub_object->id));
                //    return false;
                //}
                for (const Vec3i& face : its.indices) {
                    for (const int tri_id : face) {
                        if (tri_id < 0 || tri_id >= int(sub_object->geometry.vertices.size())) {
                            add_error("invalid vertex id in object " + std::to_string(sub_object->id));
                            return false;
                        }
                    }
                }

                its.vertices.assign(sub_object->geometry.vertices.begin(), sub_object->geometry.vertices.end());

                TriangleMesh triangle_mesh(std::move(its), volume_data->mesh_stats);

                // BBS: no need to multiply the instance matrix into the volume
                //if (!m_is_bbl_3mf) {
                //    // if the 3mf was not produced by BambuStudio and there is only one instance,
                //    // bake the transformation into the geometry to allow the reload from disk command
                //    // to work properly
                //    if (object.instances.size() == 1) {
                //        triangle_mesh.transform(object.instances.front()->get_transformation().get_matrix(), false);
                //        object.instances.front()->set_transformation(Slic3r::Geometry::Transformation());
                //        //FIXME do the mesh fixing?
                //    }
                //}
                if (triangle_mesh.volume() < 0)
                    triangle_mesh.flip_triangles();

                volume = object.add_volume(std::move(triangle_mesh));

                if (shared_mesh_id != -1)
                    //for some cases the shared mesh is in other plate and not loaded in cli slicing
                    //we need to use the first one in the same plate instead
                    m_shared_meshes[shared_mesh_id] = volume;
                else
                    m_shared_meshes[sub_object->id] = volume;
            }
            else {
                //create volume to use shared mesh
                volume = object.add_volume_with_shared_mesh(*shared_volume);
                BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": line %1%, create volume using shared_mesh %2%")%__LINE__%shared_volume;
            }
            // stores the volume matrix taken from the metadata, if present
            if (has_transform)
                volume->source.transform = Slic3r::Geometry::Transformation(volume_matrix_to_object);

            volume->calculate_convex_hull();

            //set transform from 3mf
            Slic3r::Geometry::Transformation comp_transformatino(sub_comp.transform);
            volume->set_transformation(comp_transformatino * volume->get_transformation());
            if (shared_volume) {
                const TriangleMesh& trangle_mesh = volume->mesh();
                Vec3d shift = trangle_mesh.get_init_shift();
                if (!shift.isApprox(Vec3d::Zero()))
                    volume->translate(shift);
            }

            // recreate custom supports, seam and mmu segmentation from previously loaded attribute
            {
                volume->supported_facets.reserve(triangles_count);
                volume->seam_facets.reserve(triangles_count);
                volume->mmu_segmentation_facets.reserve(triangles_count);
                for (size_t i=0; i<triangles_count; ++i) {
                    assert(i < sub_object->geometry.custom_supports.size());
                    assert(i < sub_object->geometry.custom_seam.size());
                    assert(i < sub_object->geometry.mmu_segmentation.size());
                    if (! sub_object->geometry.custom_supports[i].empty())
                        volume->supported_facets.set_triangle_from_string(i, sub_object->geometry.custom_supports[i]);
                    if (! sub_object->geometry.custom_seam[i].empty())
                        volume->seam_facets.set_triangle_from_string(i, sub_object->geometry.custom_seam[i]);
                    if (! sub_object->geometry.mmu_segmentation[i].empty())
                        volume->mmu_segmentation_facets.set_triangle_from_string(i, sub_object->geometry.mmu_segmentation[i]);
                }
                volume->supported_facets.shrink_to_fit();
                volume->seam_facets.shrink_to_fit();
                volume->mmu_segmentation_facets.shrink_to_fit();
                volume->mmu_segmentation_facets.touch();
            }

            volume->set_type(volume_data->part_type);

            // apply the remaining volume's metadata
            for (const Metadata& metadata : volume_data->metadata) {
                if (metadata.key == NAME_KEY)
                    volume->name = metadata.value;
                //else if ((metadata.key == MODIFIER_KEY) && (metadata.value == "1"))
				//	volume->set_type(ModelVolumeType::PARAMETER_MODIFIER);
				//for old format
                else if ((metadata.key == VOLUME_TYPE_KEY) || (metadata.key == PART_TYPE_KEY))
                    volume->set_type(bbs_type_from_string(metadata.value));
                else if (metadata.key == SOURCE_FILE_KEY)
                    volume->source.input_file = metadata.value;
                else if (metadata.key == SOURCE_OBJECT_ID_KEY)
                    volume->source.object_idx = ::atoi(metadata.value.c_str());
                else if (metadata.key == SOURCE_VOLUME_ID_KEY)
                    volume->source.volume_idx = ::atoi(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_X_KEY)
                    volume->source.mesh_offset(0) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_Y_KEY)
                    volume->source.mesh_offset(1) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_Z_KEY)
                    volume->source.mesh_offset(2) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_IN_INCHES)
                    volume->source.is_converted_from_inches = metadata.value == "1";
                else if (metadata.key == SOURCE_IN_METERS)
                    volume->source.is_converted_from_meters = metadata.value == "1";
                else if ((metadata.key == MATRIX_KEY) || (metadata.key == MESH_SHARED_KEY))
                    continue;
                else
                    volume->config.set_deserialize(metadata.key, metadata.value, config_substitutions);
            }

            // this may happen for 3mf saved by 3rd part softwares
            if (volume->name.empty()) {
                volume->name = object.name;
                if (renamed_volumes_count > 0)
                    volume->name += "_" + std::to_string(renamed_volumes_count + 1);
                ++renamed_volumes_count;
            }
        }

        return true;
    }

    bool _BBS_3MF_Importer::_generate_volumes(ModelObject& object, const Geometry& geometry, const ObjectMetadata::VolumeMetadataList& volumes, ConfigSubstitutionContext& config_substitutions)
    {
        if (!object.volumes.empty()) {
            add_error("Found invalid volumes count");
            return false;
        }

        unsigned int geo_tri_count = (unsigned int)geometry.triangles.size();
        unsigned int renamed_volumes_count = 0;

        for (const ObjectMetadata::VolumeMetadata& volume_data : volumes) {
            if (geo_tri_count <= volume_data.first_triangle_id || geo_tri_count <= volume_data.last_triangle_id || volume_data.last_triangle_id < volume_data.first_triangle_id) {
                add_error("Found invalid triangle id");
                return false;
            }

            Transform3d volume_matrix_to_object = Transform3d::Identity();
            bool        has_transform 		    = false;
            // extract the volume transformation from the volume's metadata, if present
            for (const Metadata& metadata : volume_data.metadata) {
                if (metadata.key == MATRIX_KEY) {
                    volume_matrix_to_object = Slic3r::Geometry::transform3d_from_string(metadata.value);
                    has_transform 			= ! volume_matrix_to_object.isApprox(Transform3d::Identity(), 1e-10);
                    break;
                }
            }

            // splits volume out of imported geometry
            indexed_triangle_set its;
            its.indices.assign(geometry.triangles.begin() + volume_data.first_triangle_id, geometry.triangles.begin() + volume_data.last_triangle_id + 1);
            const size_t triangles_count = its.indices.size();
            if (triangles_count == 0) {
                add_error("An empty triangle mesh found");
                return false;
            }

            {
                int min_id = its.indices.front()[0];
                int max_id = min_id;
                for (const Vec3i& face : its.indices) {
                    for (const int tri_id : face) {
                        if (tri_id < 0 || tri_id >= int(geometry.vertices.size())) {
                            add_error("Found invalid vertex id");
                            return false;
                        }
                        min_id = std::min(min_id, tri_id);
                        max_id = std::max(max_id, tri_id);
                    }
                }
                its.vertices.assign(geometry.vertices.begin() + min_id, geometry.vertices.begin() + max_id + 1);

                // rebase indices to the current vertices list
                for (Vec3i& face : its.indices)
                    for (int& tri_id : face)
                        tri_id -= min_id;
            }

            TriangleMesh triangle_mesh(std::move(its), volume_data.mesh_stats);

            if (!m_is_bbl_3mf) {
                // if the 3mf was not produced by BambuStudio and there is only one instance,
                // bake the transformation into the geometry to allow the reload from disk command
                // to work properly
                if (object.instances.size() == 1) {
                    triangle_mesh.transform(object.instances.front()->get_transformation().get_matrix(), false);
                    object.instances.front()->set_transformation(Slic3r::Geometry::Transformation());
                    //FIXME do the mesh fixing?
                }
            }
            if (triangle_mesh.volume() < 0)
                triangle_mesh.flip_triangles();

			ModelVolume* volume = object.add_volume(std::move(triangle_mesh));
            // stores the volume matrix taken from the metadata, if present
            if (has_transform)
                volume->source.transform = Slic3r::Geometry::Transformation(volume_matrix_to_object);
            volume->calculate_convex_hull();

            // recreate custom supports, seam and mmu segmentation from previously loaded attribute
            volume->supported_facets.reserve(triangles_count);
            volume->seam_facets.reserve(triangles_count);
            volume->mmu_segmentation_facets.reserve(triangles_count);
            for (size_t i=0; i<triangles_count; ++i) {
                size_t index = volume_data.first_triangle_id + i;
                assert(index < geometry.custom_supports.size());
                assert(index < geometry.custom_seam.size());
                assert(index < geometry.mmu_segmentation.size());
                if (! geometry.custom_supports[index].empty())
                    volume->supported_facets.set_triangle_from_string(i, geometry.custom_supports[index]);
                if (! geometry.custom_seam[index].empty())
                    volume->seam_facets.set_triangle_from_string(i, geometry.custom_seam[index]);
                if (! geometry.mmu_segmentation[index].empty())
                    volume->mmu_segmentation_facets.set_triangle_from_string(i, geometry.mmu_segmentation[index]);
            }
            volume->supported_facets.shrink_to_fit();
            volume->seam_facets.shrink_to_fit();
            volume->mmu_segmentation_facets.shrink_to_fit();

            volume->set_type(volume_data.part_type);

            // apply the remaining volume's metadata
            for (const Metadata& metadata : volume_data.metadata) {
                if (metadata.key == NAME_KEY)
                    volume->name = metadata.value;
                //else if ((metadata.key == MODIFIER_KEY) && (metadata.value == "1"))
				//	volume->set_type(ModelVolumeType::PARAMETER_MODIFIER);
				//for old format
                else if ((metadata.key == VOLUME_TYPE_KEY) || (metadata.key == PART_TYPE_KEY))
                    volume->set_type(bbs_type_from_string(metadata.value));
                else if (metadata.key == SOURCE_FILE_KEY)
                    volume->source.input_file = metadata.value;
                else if (metadata.key == SOURCE_OBJECT_ID_KEY)
                    volume->source.object_idx = ::atoi(metadata.value.c_str());
                else if (metadata.key == SOURCE_VOLUME_ID_KEY)
                    volume->source.volume_idx = ::atoi(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_X_KEY)
                    volume->source.mesh_offset(0) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_Y_KEY)
                    volume->source.mesh_offset(1) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_OFFSET_Z_KEY)
                    volume->source.mesh_offset(2) = ::atof(metadata.value.c_str());
                else if (metadata.key == SOURCE_IN_INCHES)
                    volume->source.is_converted_from_inches = metadata.value == "1";
                else if (metadata.key == SOURCE_IN_METERS)
                    volume->source.is_converted_from_meters = metadata.value == "1";
                else
                    volume->config.set_deserialize(metadata.key, metadata.value, config_substitutions);
            }

            // this may happen for 3mf saved by 3rd part softwares
            if (volume->name.empty()) {
                volume->name = object.name;
                if (renamed_volumes_count > 0)
                    volume->name += "_" + std::to_string(renamed_volumes_count + 1);
                ++renamed_volumes_count;
            }
        }

        return true;
    }

    void XMLCALL _BBS_3MF_Importer::_handle_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_start_model_xml_element(name, attributes);
    }

    void XMLCALL _BBS_3MF_Importer::_handle_end_model_xml_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_end_model_xml_element(name);
    }

    void XMLCALL _BBS_3MF_Importer::_handle_xml_characters(void* userData, const XML_Char* s, int len)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_xml_characters(s, len);
    }

    void XMLCALL _BBS_3MF_Importer::_handle_start_config_xml_element(void* userData, const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_start_config_xml_element(name, attributes);
    }

    void XMLCALL _BBS_3MF_Importer::_handle_end_config_xml_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_end_config_xml_element(name);
    }


    /* functions of ObjectImporter */
    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_model(const char** attributes, unsigned int num_attributes)
    {
        object_unit_factor = bbs_get_unit_factor(bbs_get_attribute_value_string(attributes, num_attributes, UNIT_ATTR));

        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_model()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_resources(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_resources()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_object(const char** attributes, unsigned int num_attributes)
    {
        // reset current object data
        if (current_object) {
            delete current_object;
            current_object = nullptr;
        }

        std::string object_type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);

        if (bbs_is_valid_object_type(object_type)) {
            if (!current_object) {
                current_object = new CurrentObject();
            }

            current_object->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            current_object->name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);

            current_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            if (current_object->uuid.empty()) {
                current_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_LOWER_ATTR);
            }
            current_object->pid = bbs_get_attribute_value_int(attributes, num_attributes, PID_ATTR);
        }

        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_object()
    {
        if (!current_object || (current_object->id == -1)) {
            top_importer->add_error("Found invalid object for "+ object_path);
            return false;
        }
        else {
            if (is_bbl_3mf && boost::ends_with(current_object->uuid, OBJECT_UUID_SUFFIX) && top_importer->m_load_restore) {
                std::istringstream iss(current_object->uuid);
                int backup_id;
                bool need_replace = false;
                if (iss >> std::hex >> backup_id) {
                    need_replace = (current_object->id != backup_id);
                    current_object->id = backup_id;
                }
                //if (need_replace)
                {
                    for (int index = 0; index < current_object->components.size(); index++)
                    {
                        int temp_id = (index + 1) << 16 | backup_id;
                        Component& component = current_object->components[index];
                        std::string new_path = component.object_id.first;
                        Id new_id = std::make_pair(new_path, temp_id);
                        IdToCurrentObjectMap::iterator object_it = object_list.find(component.object_id);
                        if (object_it != object_list.end()) {
                            CurrentObject new_object;
                            new_object.geometry = std::move(object_it->second.geometry);
                            new_object.id = temp_id;
                            new_object.model_object_idx = object_it->second.model_object_idx;
                            new_object.name = object_it->second.name;
                            new_object.uuid = object_it->second.uuid;

                            object_list.erase(object_it);
                            object_list.insert({ new_id, std::move(new_object) });
                        }
                        else {
                            top_importer->add_error("can not find object for component, id=" + std::to_string(component.object_id.second));
                            delete current_object;
                            current_object = nullptr;
                            return false;
                        }

                        component.object_id.second = temp_id;
                    }
                }
            }
            Id id = std::make_pair(object_path, current_object->id);
            if (object_list.find(id) == object_list.end()) {
                object_list.insert({ id, std::move(*current_object) });
                delete current_object;
                current_object = nullptr;
            }
            else {
                top_importer->add_error("Found object with duplicate id for "+object_path);
                delete current_object;
                current_object = nullptr;
                return false;
            }
        }

        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_color_group(const char **attributes, unsigned int num_attributes)
    {
        object_current_color_group = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_color_group()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_color(const char **attributes, unsigned int num_attributes)
    {
        std::string color = bbs_get_attribute_value_string(attributes, num_attributes, COLOR_ATTR);
        object_group_id_to_color[object_current_color_group] = color;
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_color()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_mesh(const char** attributes, unsigned int num_attributes)
    {
        // reset current geometry
        if (current_object)
            current_object->geometry.reset();
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_mesh()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_vertices(const char** attributes, unsigned int num_attributes)
    {
        // reset current vertices
        if (current_object)
            current_object->geometry.vertices.clear();
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_vertices()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_vertex(const char** attributes, unsigned int num_attributes)
    {
        // appends the vertex coordinates
        // missing values are set equal to ZERO
        if (current_object)
            current_object->geometry.vertices.emplace_back(
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, X_ATTR),
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Y_ATTR),
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Z_ATTR));
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_vertex()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_triangles(const char** attributes, unsigned int num_attributes)
    {
        // reset current triangles
        if (current_object)
            current_object->geometry.triangles.clear();
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_triangles()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_triangle(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes:
        // p1
        // p2
        // p3
        // pid
        // see specifications

        // appends the triangle's vertices indices
        // missing values are set equal to ZERO
        if (current_object) {
            current_object->geometry.triangles.emplace_back(
                bbs_get_attribute_value_int(attributes, num_attributes, V1_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V2_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V3_ATTR));

            current_object->geometry.custom_supports.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SUPPORTS_ATTR));
            current_object->geometry.custom_seam.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SEAM_ATTR));
            current_object->geometry.mmu_segmentation.push_back(bbs_get_attribute_value_string(attributes, num_attributes, MMU_SEGMENTATION_ATTR));
            // BBS
            current_object->geometry.face_properties.push_back(bbs_get_attribute_value_string(attributes, num_attributes, FACE_PROPERTY_ATTR));
        }
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_triangle()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_components(const char** attributes, unsigned int num_attributes)
    {
        // reset current components
        if (current_object)
            current_object->components.clear();
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_components()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_component(const char** attributes, unsigned int num_attributes)
    {
        int object_id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
        Transform3d transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));

        /*Id id = std::make_pair(m_sub_model_path, object_id);
        IdToModelObjectMap::iterator object_item = m_objects.find(id);
        if (object_item == m_objects.end()) {
            IdToAliasesMap::iterator alias_item = m_objects_aliases.find(id);
            if (alias_item == m_objects_aliases.end()) {
                add_error("Found component with invalid object id");
                return false;
            }
        }*/

        if (current_object) {
            Id id = std::make_pair(object_path, object_id);
            current_object->components.emplace_back(id, transform);
        }

        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_component()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_start_metadata(const char** attributes, unsigned int num_attributes)
    {
        obj_curr_metadata_name.clear();

        std::string name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);
        if (!name.empty()) {
            obj_curr_metadata_name = name;
        }

        return true;
    }

    bool _BBS_3MF_Importer::ObjectImporter::_handle_object_end_metadata()
    {
        if ((obj_curr_metadata_name == BBS_3MF_VERSION)||(obj_curr_metadata_name == BBS_3MF_VERSION1)) {
            is_bbl_3mf = true;
        }
        return true;
    }
    void _BBS_3MF_Importer::ObjectImporter::_handle_object_start_model_xml_element(const char* name, const char** attributes)
    {
        if (object_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(object_xml_parser);

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_object_start_model(attributes, num_attributes);
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_object_start_resources(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_object_start_object(attributes, num_attributes);
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_object_start_color_group(attributes, num_attributes);
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_object_start_color(attributes, num_attributes);
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_object_start_mesh(attributes, num_attributes);
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_object_start_vertices(attributes, num_attributes);
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_object_start_vertex(attributes, num_attributes);
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_object_start_triangles(attributes, num_attributes);
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_object_start_triangle(attributes, num_attributes);
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_object_start_components(attributes, num_attributes);
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_object_start_component(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_object_start_metadata(attributes, num_attributes);

        if (!res)
            _stop_object_xml_parser();
    }

    void _BBS_3MF_Importer::ObjectImporter::_handle_object_end_model_xml_element(const char* name)
    {
        if (object_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_object_end_model();
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_object_end_resources();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_object_end_object();
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_object_end_color_group();
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_object_end_color();
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_object_end_mesh();
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_object_end_vertices();
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_object_end_vertex();
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_object_end_triangles();
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_object_end_triangle();
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_object_end_components();
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_object_end_component();
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_object_end_metadata();

        if (!res)
            _stop_object_xml_parser();
    }

    void _BBS_3MF_Importer::ObjectImporter::_handle_object_xml_characters(const XML_Char* s, int len)
    {
        obj_curr_characters.append(s, len);
    }

    void XMLCALL _BBS_3MF_Importer::ObjectImporter::_handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_start_model_xml_element(name, attributes);
    }

    void XMLCALL _BBS_3MF_Importer::ObjectImporter::_handle_object_end_model_xml_element(void* userData, const char* name)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_end_model_xml_element(name);
    }

    void XMLCALL _BBS_3MF_Importer::ObjectImporter::_handle_object_xml_characters(void* userData, const XML_Char* s, int len)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_xml_characters(s, len);
    }

    bool _BBS_3MF_Importer::ObjectImporter::_extract_object_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            top_importer->add_error("Found invalid size for "+object_path);
            return false;
        }

        object_xml_parser = XML_ParserCreate(nullptr);
        if (object_xml_parser == nullptr) {
            top_importer->add_error("Unable to create parser for "+object_path);
            return false;
        }

        XML_SetUserData(object_xml_parser, (void*)this);
        XML_SetElementHandler(object_xml_parser, _BBS_3MF_Importer::ObjectImporter::_handle_object_start_model_xml_element, _BBS_3MF_Importer::ObjectImporter::_handle_object_end_model_xml_element);
        XML_SetCharacterDataHandler(object_xml_parser, _BBS_3MF_Importer::ObjectImporter::_handle_object_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            _BBS_3MF_Importer::ObjectImporter& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, _BBS_3MF_Importer::ObjectImporter& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(object_xml_parser, *this, stat);

        mz_bool res = 0;

        try
        {
            mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
                CallbackData* data = (CallbackData*)pOpaque;
                if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0) || data->importer.object_parse_error()) {
                    char error_buf[1024];
                    ::snprintf(error_buf, 1024, "Error (%s) while parsing '%s' at line %d", data->importer.object_parse_error_message(), data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                    throw Slic3r::FileIOError(error_buf);
                }
                return n;
            };
            void* opaque = &data;
            res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);
        }
        catch (const version_error& e)
        {
            // rethrow the exception
            std::string error_message = std::string(e.what()) + " for " + object_path;
            throw Slic3r::FileIOError(error_message);
        }
        catch (std::exception& e)
        {
            std::string error_message = std::string(e.what()) + " for " + object_path;
            top_importer->add_error(error_message);
            return false;
        }

        if (res == 0) {
            top_importer->add_error("Error while extracting model data from zip archive for "+object_path);
            return false;
        }

        return true;
    }

    static void reset_stream(std::stringstream &stream)
    {
        stream.str("");
        stream.clear();
        // https://en.cppreference.com/w/cpp/types/numeric_limits/max_digits10
        // Conversion of a floating-point value to text and back is exact as long as at least max_digits10 were used (9 for float, 17 for double).
        // It is guaranteed to produce the same floating-point value, even though the intermediate text representation is not exact.
        // The default value of std::stream precision is 6 digits only!
        stream << std::setprecision(std::numeric_limits<float>::max_digits10);
    }

// Perform conversions based on the config values available.
//FIXME provide a version of PrusaSlicer that stored the project file (3MF).
static void handle_legacy_project_loaded(unsigned int version_project_file, DynamicPrintConfig& config)
{
    if (! config.has("brim_object_gap")) {
        if (auto *opt_elephant_foot   = config.option<ConfigOptionFloat>("elefant_foot_compensation", false); opt_elephant_foot) {
            // Conversion from older PrusaSlicer which applied brim separation equal to elephant foot compensation.
            auto *opt_brim_separation = config.option<ConfigOptionFloat>("brim_object_gap", true);
            opt_brim_separation->value = opt_elephant_foot->value;
        }
    }
}

//BBS: add plate data list related logic
bool load_bbs_3mf(const char* path, DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions, Model* model,
    bool* is_bbl_3mf, Semver* file_version, Import3mfProgressFn proFn, LoadStrategy strategy, int plate_id)
{
    if (path == nullptr || config == nullptr || model == nullptr)
        return false;

    // All import should use "C" locales for number formatting.
    CNumericLocalesSetter locales_setter;
    _BBS_3MF_Importer importer;
    bool res = importer.load_model_from_file(path, *model, *config, *config_substitutions, strategy, *is_bbl_3mf, *file_version, proFn, plate_id);
    importer.log_errors();
    //BBS: remove legacy project logic currently
    //handle_legacy_project_loaded(importer.version(), *config);
    return res;
}

struct check_bbs_3mf
{
    std::string temp;
    std::string application;
    bool is_start{ false };
};

static void XMLCALL characterDataHandler(void* userData, const XML_Char* s, int len) {
    check_bbs_3mf* data = (check_bbs_3mf*)userData;
    if (data->is_start) {
        data->temp.append(s, len);
    }
}

static void XMLCALL startElementHandler(void* userData, const XML_Char* name, const XML_Char** atts) {
    check_bbs_3mf* data = (check_bbs_3mf*)userData;
    if (strcmp(name, "metadata") == 0) {
        for (int i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "name") == 0 
                && strcmp(atts[i + 1], BBL_APPLICATION_TAG.c_str()) == 0)
            {
                //boost 
                // The next attribute should be the value of Application
                data->is_start = true;
            }
        }
    }
}

static void XMLCALL endElementHandler(void* userData, const XML_Char* name) {
    check_bbs_3mf* data = (check_bbs_3mf*)userData;
    if (strcmp(name, "metadata") == 0) {
        if (data->is_start) {
            data->application = data->temp;
            data->is_start = false;
        }
    }
}

bool check_3mf_from_bambu_custom(const std::string filename)
{
    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    if (!open_zip_reader(&archive, filename)) {
        // throw Slic3r::RuntimeError("Loading 3mf file failed.");
        return false;
    }

    bool is_bambu_3mf = false;
    char* uncompressed_data = nullptr;

    do
    {
        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        mz_zip_archive_file_stat stat;
        int folder_index = -1;

        // we first loop the entries to read from the archive the .model file only, in order to extract the version from it
        for (mz_uint i = 0; i < num_entries; ++i) {
            if (mz_zip_reader_file_stat(&archive, i, &stat)) {
                std::string name(stat.m_filename);
                std::replace(name.begin(), name.end(), '\\', '/');
                if (boost::starts_with(name, "3D/Objects/")) {
                    folder_index = stat.m_file_index;
                    break;
                }
            }
        }

        if (folder_index < 0) {
            break;
        }

        int model_file_index = mz_zip_reader_locate_file(&archive, "3D/3dmodel.model", nullptr, 0);
        if (model_file_index < 0) {
            break;
        }

        // Extract the file to heap
        size_t uncompressed_size;
        uncompressed_data = (char*)mz_zip_reader_extract_to_heap(&archive, model_file_index, &uncompressed_size, 0);
        if (!uncompressed_data) {
            break;
        }

        // Prepare Expat XML parser
        XML_Parser parser = XML_ParserCreate(NULL);
        if (!parser) {
            break;
        }

        check_bbs_3mf data;
        XML_SetUserData(parser, &data);
        XML_SetElementHandler(parser, startElementHandler, endElementHandler);
        XML_SetCharacterDataHandler(parser, characterDataHandler);

        // Parse the XML data
        XML_Parse(parser, uncompressed_data, (int)uncompressed_size, XML_TRUE);
        XML_ParserFree(parser);

        if (boost::starts_with(data.application, "BambuStudio-")) {
            is_bambu_3mf = true;
        }

    } while (0); 

    mz_free(uncompressed_data);
    close_zip_reader(&archive);
    return is_bambu_3mf;
}

} // namespace Slic3r

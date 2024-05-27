#ifndef BBS_3MF_hpp_
#define BBS_3MF_hpp_

#include "../GCode/ThumbnailData.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include <functional>
#include <expat.h>

namespace Slic3r {
class Model;
class ModelObject;
struct ConfigSubstitutionContext;
class DynamicPrintConfig;
class Preset;
struct FilamentInfo;
struct ThumbnailData;


#define PLATE_THUMBNAIL_SMALL_WIDTH     128
#define PLATE_THUMBNAIL_SMALL_HEIGHT    128

#define GCODE_FILE_FORMAT               "Metadata/plate_%1%.gcode"
#define THUMBNAIL_FILE_FORMAT           "Metadata/plate_%1%.png"
#define TOP_FILE_FORMAT                 "Metadata/top_%1%.png"
#define PICK_FILE_FORMAT                "Metadata/pick_%1%.png"
//#define PATTERN_FILE_FORMAT             "Metadata/plate_%1%_pattern_layer_0.png"
#define PATTERN_CONFIG_FILE_FORMAT      "Metadata/plate_%1%.json"
#define EMBEDDED_PRINT_FILE_FORMAT      "Metadata/process_settings_%1%.config"
#define EMBEDDED_FILAMENT_FILE_FORMAT      "Metadata/filament_settings_%1%.config"
#define EMBEDDED_PRINTER_FILE_FORMAT      "Metadata/machine_settings_%1%.config"

#define BBL_DESIGNER_MODEL_TITLE_TAG     "Title"
#define BBL_DESIGNER_PROFILE_ID_TAG      "DesignProfileId"
#define BBL_DESIGNER_PROFILE_TITLE_TAG   "ProfileTitle"
#define BBL_DESIGNER_MODEL_ID_TAG        "DesignModelId"


//BBS: define assistant struct to store temporary variable during exporting 3mf
class PackingTemporaryData
{
public:
    std::string _3mf_thumbnail;
    std::string _3mf_printer_thumbnail_middle;
    std::string _3mf_printer_thumbnail_small;

    PackingTemporaryData() {}
};

// BBS: encrypt
enum class SaveStrategy
{
    Default = 0,
    FullPathSources     = 1,
    Zip64               = 1 << 1,
    ProductionExt       = 1 << 2,
    SecureContentExt    = 1 << 3,
    WithGcode           = 1 << 4,
    Silence             = 1 << 5,
    SkipStatic          = 1 << 6,
    SkipModel           = 1 << 7,
    WithSliceInfo       = 1 << 8,
    SkipAuxiliary       = 1 << 9,
    UseLoadedId         = 1 << 10,
    ShareMesh           = 1 << 11,

    SplitModel = 0x1000 | ProductionExt,
    Encrypted  = SecureContentExt | SplitModel,
    Backup = 0x10000 | WithGcode | Silence | SkipStatic | SplitModel,
};

inline SaveStrategy operator | (SaveStrategy lhs, SaveStrategy rhs)
{
    using T = std::underlying_type_t <SaveStrategy>;
    return static_cast<SaveStrategy>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline bool operator & (SaveStrategy & lhs, SaveStrategy rhs)
{
    using T = std::underlying_type_t <SaveStrategy>;
    return ((static_cast<T>(lhs) & static_cast<T>(rhs))) == static_cast<T>(rhs);
}

enum class LoadStrategy
{
    Default = 0,
    AddDefaultInstances = 1,
    CheckVersion = 2,
    LoadModel = 4,
    LoadConfig = 8,
    LoadAuxiliary = 16,
    Silence = 32,
    ImperialUnits = 64,

    Restore = 0x10000 | LoadModel | LoadConfig | LoadAuxiliary | Silence,
};

inline LoadStrategy operator | (LoadStrategy lhs, LoadStrategy rhs)
{
    using T = std::underlying_type_t <LoadStrategy>;
    return static_cast<LoadStrategy>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline bool operator & (LoadStrategy & lhs, LoadStrategy rhs)
{
    using T = std::underlying_type_t <LoadStrategy>;
    return (static_cast<T>(lhs) & static_cast<T>(rhs)) == static_cast<T>(rhs);
}

const int EXPORT_STAGE_OPEN_3MF         = 0;
const int EXPORT_STAGE_CONTENT_TYPES    = 1;
const int EXPORT_STAGE_ADD_THUMBNAILS   = 2;
const int EXPORT_STAGE_ADD_RELATIONS    = 3;
const int EXPORT_STAGE_ADD_MODELS       = 4;
const int EXPORT_STAGE_ADD_LAYER_RANGE  = 5;
const int EXPORT_STAGE_ADD_SUPPORT      = 6;
const int EXPORT_STAGE_ADD_CUSTOM_GCODE = 7;
const int EXPORT_STAGE_ADD_PRINT_CONFIG = 8;
const int EXPORT_STAGE_ADD_PROJECT_CONFIG = 9;
const int EXPORT_STAGE_ADD_CONFIG_FILE  = 10;
const int EXPORT_STAGE_ADD_SLICE_INFO   = 11;
const int EXPORT_STAGE_ADD_GCODE        = 12;
const int EXPORT_STAGE_ADD_AUXILIARIES  = 13;
const int EXPORT_STAGE_FINISH           = 14;

const int IMPORT_STAGE_RESTORE          = 0;
const int IMPORT_STAGE_OPEN             = 1;
const int IMPORT_STAGE_READ_FILES       = 2;
const int IMPORT_STAGE_EXTRACT          = 3;
const int IMPORT_STAGE_LOADING_OBJECTS  = 4;
const int IMPORT_STAGE_LOADING_PLATES   = 5;
const int IMPORT_STAGE_FINISH           = 6;
const int IMPORT_STAGE_ADD_INSTANCE     = 7;
const int IMPORT_STAGE_UPDATE_GCODE     = 8;
const int IMPORT_STAGE_CHECK_MODE_GCODE = 9;
const int UPDATE_GCODE_RESULT           = 10;
const int IMPORT_LOAD_CONFIG            = 11;
const int IMPORT_LOAD_MODEL_OBJECTS     = 12;
const int IMPORT_STAGE_MAX              = 13;

//BBS export 3mf progress
typedef std::function<void(int import_stage, int current, int total, bool& cancel)> Import3mfProgressFn;

//BBS: add plate data list related logic
// add restore logic
// Load the content of a 3mf file into the given model and preset bundle.
extern bool load_bbs_3mf(const char* path, DynamicPrintConfig* config, ConfigSubstitutionContext* config_substitutions, Model* model,
        bool* is_bbl_3mf, Semver* file_version, Import3mfProgressFn proFn = nullptr, LoadStrategy strategy = LoadStrategy::Default, int plate_id = 0);

//extern std::string bbs_3mf_get_thumbnail(const char * path);
//
//extern bool load_gcode_3mf_from_stream(std::istream & data, DynamicPrintConfig* config, Model* model, Semver* file_version);

extern bool check_3mf_from_bambu_custom(const std::string filename);
//BBS: add plate data list related logic
// add backup logic
// Save the given model and the config data contained in the given Print into a 3mf file.
// The model could be modified during the export process if meshes are not repaired or have no shared vertices
/*
extern bool store_bbs_3mf(const char* path,
                          Model* model,
                          PlateDataPtrs& plate_data_list,
                          std::vector<Preset*>& project_presets,
                          const DynamicPrintConfig* config,
                          bool fullpath_sources,
                          const std::vector<ThumbnailData*>& thumbnail_data,
                          bool zip64 = true,
                          bool skip_static = false,
                          Export3mfProgressFn proFn = nullptr,
                          bool silence = true);
*/

} // namespace Slic3r

#endif /* BBS_3MF_hpp_ */

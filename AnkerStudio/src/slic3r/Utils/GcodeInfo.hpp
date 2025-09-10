#ifndef slic3r_GcodeInfo_hpp_
#define slic3r_GcodeInfo_hpp_

#include <string>
#include "slic3r/GUI/ViewModel/AnkerMaterialMappingViewModel.h"
#include "AnkerNetDefines.h"

namespace Slic3r {

class GcodeInfo {
public:
    using LineProcess_T = std::function<void(const std::string& line, int index, bool& stop)>;

    // utf8GcodeFilePath: you should transfer the utf8 string for gcode file path
    static void GetMachineInfoFromGCode(const std::string& utf8GcodeFilePath, 
        bool& isAnkerBrand, anker_device_type& machineType);

    static bool GetTemperatureFromGCode(const std::string& gcodeFilePath,int& tempature);

    static std::vector<CardInfo> GetColorMaterialIdInfo(const std::string& gcodeFilePath);

    static void ParseGcodeInfoToViewModel(const std::string& strGcodeFilePath, GUI::AnkerMaterialMappingViewModel* pViewModel);

    static std::vector<std::string> GetFilamentFromGCode(const wxString& utf8GcodeFilePath);

    static std::vector<std::string> ExtractFilamentStrings(const std::string& input);

    static std::string GetFilamentName(const std::string& filamentStr);

private:
    static int ReverseReadFile(const std::string& file_name, LineProcess_T lineFunc = nullptr, int offset = 0, int max_line = 1000);
    static void ReverseReadFileExt(std::ifstream& file, LineProcess_T lineFunc = nullptr, int readCount = 1000);

    static std::string GetStringAfterEqual(const std::string& input);

    static bool GetIsAnkerFromGCode(const std::string& line);

    static anker_device_type GetMachineNameFromGCode(const std::string& line);

    static void ParseIdMap(const std::string& input, std::vector<CardInfo>&);
};

}

#endif

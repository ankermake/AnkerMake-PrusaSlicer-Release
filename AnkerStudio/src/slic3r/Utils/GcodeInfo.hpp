#ifndef slic3r_GcodeInfo_hpp_
#define slic3r_GcodeInfo_hpp_

#include <string>
#include "slic3r/GUI/ViewModel/AnkerMaterialMappingViewModel.h"
#include "AnkerNetDefines.h"

namespace Slic3r {

class GcodeInfo {
public:
    // utf8GcodeFilePath: you should transfer the utf8 string for gcode file path
    static void GetMachineInfoFromGCode(const std::string& utf8GcodeFilePath, 
        bool& isAnkerBrand, anker_device_type& machineType);
    static bool GetTemperatureFromGCode(const std::string& gcodeFilePath,int& tempature);

    static std::vector<CardInfo> GetColorMaterialIdInfo(const std::string& gcodeFilePath);

    static void ParseGcodeInfoToViewModel(const std::string& strGcodeFilePath, GUI::AnkerMaterialMappingViewModel* pViewModel);

private:
    static bool GetIsAnkerFromGCode(const std::string& line);
    static anker_device_type GetMachineNameFromGCode(const std::string& line);
    static void ParseIdMap(const std::string& input, std::vector<CardInfo>&);
};

}

#endif

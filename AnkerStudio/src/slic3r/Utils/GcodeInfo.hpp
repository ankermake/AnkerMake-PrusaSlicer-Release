#ifndef slic3r_GcodeInfo_hpp_
#define slic3r_GcodeInfo_hpp_

#include <string>
#include "anker_net/business/mqttBusiness/mqttprotocolbase.h"

namespace Slic3r {

class GcodeInfo {
public:
    static void GetMachineInfoFromGCode(const std::string& gcodeFilePath, bool& isAnkerBrand, mqtt_device_type& machineType);

private:
    static bool GetIsAnkerFromGCode(const std::string& line);
    static mqtt_device_type GetMachineNameFromGCode(const std::string& line);
};

}

#endif

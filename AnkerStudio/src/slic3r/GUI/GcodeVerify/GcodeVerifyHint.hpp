#ifndef __GcodeVerifyHint_H__
#define __GcodeVerifyHint_H__

#include "AnkerNetDefines.h"
using namespace AnkerNet;
class GcodeVerifyHint
{
public:
    //Check whether pop-up dialog needs to show by app-options 
    static bool CheckOption();

    //Check whether pop-up dialog needs to show by device type
    static bool CheckDeviceType(const std::string& filePath, const anker_device_type deviceType);

    //Check whether pop-up dialog needs to show by temperature
    static bool CheckTemperature(const std::string& filePath, const int temperature);

    //Show tips dialog and return whether print is continue
    static bool ShowModalTipsDialog(wxWindow* parent);
};

#endif

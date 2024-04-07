#ifndef ANKER_MSG_TEXT_HPP
#define ANKER_MSG_TEXT_HPP

#include <utility>
#include <string>
#include "../slic3r/GUI/I18N.hpp"
#include "AnkerNetDefines.h"
using namespace AnkerNet;

enum NetworkMsgLevel
{
    LEVEL_URGENCY = 0,
    LEVEL_NORMAL1 = 1,
};

struct NetworkMsg
{
    bool haveCancel = false;
    NetworkMsgLevel level = LEVEL_NORMAL1;  // NetworkMsgLevel
    bool clear = false;
    GeneralException2Gui type = GeneralException2Gui::GeneralException2Gui_No_Error;
    std::string sn;
    std::string title = "";
    std::string context = "";
    std::string btn1Text = "";
    std::string btn2Text = "";

    bool operator==(const NetworkMsg& other) const {
        return haveCancel == other.haveCancel &&
            level == other.level &&
            clear == other.clear &&
            type == other.type &&
            sn == other.sn &&
            title == other.title &&
            context == other.context &&
            btn1Text == other.btn1Text &&
            btn2Text == other.btn2Text;
    }
};

namespace NetworkMsgText {
    inline NetworkMsg getGeneralException2Gui_One_Mos_Msg_Text(const std::string& deviceName) {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_warning").c_str();
        wxString tmpStr;
        tmpStr.Printf(_L("common_popup_content_1mos"), deviceName);
        msg.context = tmpStr.c_str();      
        msg.btn1Text = _L("OK").c_str();
        msg.level = LEVEL_URGENCY;
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Two_Mos_Msg_Text(const std::string& deviceName) {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_warning").c_str();
        wxString tmpStr;
        tmpStr.Printf(_L("common_popup_content_2mos"), deviceName);
        msg.context = tmpStr.c_str();
        msg.btn1Text = _L("OK").c_str();
        msg.level = LEVEL_URGENCY;
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Nozzle_Temp_Too_High_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_temptohigh").c_str();
        msg.context = _L("common_popup_content_temptohigh").c_str();
        msg.btn1Text = _L("OK").c_str();        
        msg.clear = true;
        return msg;
    }

    inline  NetworkMsg getGeneralException2Gui_HotBed_Temp_Too_High_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_temptohigh").c_str();
        msg.context = _L("common_popup_content_temptohigh").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Nozzle_Heating_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_heatingerror").c_str();
        msg.context = _L("common_popup_content_temptohigh").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_HotBed_Heating_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_heatingerror").c_str();
        msg.context = _L("common_popup_content_temptohigh").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Filament_Broken_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_titlenotice").c_str();
        msg.context = _L("common_print_popup_filamentbroken").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Auto_Level_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_levelerror").c_str();
        msg.context = _L("common_popup_content_levelerror").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getgetGeneralException2Gui_Auto_Level_Failed_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_levelfailed").c_str();
        msg.context = _L("common_popup_content_levelfailed").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_System_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_datatranserror").c_str();
        msg.context = _L("common_popup_content_sysdataerror").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Type_C_Transmission_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_datatranserror").c_str();
        msg.context = _L("common_popup_content_typecdataerror").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Advance_Pause_Error_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_m601pasued").c_str();
        msg.context = _L("common_popup_content_m601pasued").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        msg.clear = true;
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Bed_Adhesion_Failure_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popupai_title_adhesionfailure").c_str();
        msg.context = _L("common_popupai_content").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_Spaghetti_Mess_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popupai_title_spaghettimess").c_str();
        msg.context = _L("common_popupai_content").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        return msg;
    }

    inline  NetworkMsg getGeneralException2Gui_HomingFailed_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_homingfail").c_str();
        msg.context = _L("common_popup_content_homingfail").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        return msg;
    }

    inline  NetworkMsg getGeneralException2Gui_Level_100_Times_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_levelerror").c_str();
        msg.context = _L("common_print_popup_levelnotice").c_str();
        msg.haveCancel = true;
        msg.btn1Text = _L("OK").c_str();
        msg.btn2Text = _L("Cancel").c_str();
        
        return msg;
    }

    inline NetworkMsg getGeneralException2Gui_LowTemperature_Msg_Text() {
        NetworkMsg msg;
        msg.title = _L("common_popup_title_lowtemp").c_str();
        msg.context = _L("common_popup_content_lowtemp").c_str();
        msg.btn1Text = _L("OK").c_str();
        
        return msg;
    }
}



#endif // !Anker_Msg_TEXT_HPP
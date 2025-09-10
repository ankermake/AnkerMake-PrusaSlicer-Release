#include "PrintCheckHint.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/AppConfig.hpp"
#include <slic3r/Utils/GcodeInfo.hpp>
#include <slic3r/GUI/Common/AnkerMsgDialog.hpp>
#include <slic3r/Utils/DeviceVersionUtil.hpp>
#include "DeviceObjectBase.h"
#include <slic3r/Utils/DataMangerUi.hpp>

// v8111/v8110 p2p transfer preheat function
static const std::string V8111_PREHEAT_HINT_VERSION = "3.2.24_3.0.97";
static const std::string V8110_PREHEAT_HINT_VERSION1 = "V3.1.47";
static const std::string V8110_PREHEAT_HINT_VERSION2 = "V3.1.51";


bool PrintCheckHint::QueryStatus(DeviceObjectBasePtr currentDev)
{
    if (!currentDev) {
        ANKER_LOG_WARNING << "current dev is null";
        return true;
    }

    currentDev->AsyQueryAllInfo();

    return false;
}

bool PrintCheckHint::IsV6Uninited(DeviceObjectBasePtr currentDev)
{
    if (!currentDev) {
        ANKER_LOG_WARNING << "current dev is null";
        return false;
    }

    // V6 uninited reminder
    if (currentDev->GetMultiColorDeviceUnInited()) {
        ANKER_LOG_WARNING << "device is uninited";
        m_type = HintType::ONLY_OK;
        m_reminderStr = wxString::Format(_L("common_print_popup_v6initialize")).ToStdString(wxConvUTF8);
        m_title = _L("common_popup_titlenotice").ToStdString(wxConvUTF8);
        return true;
    }
    return false;
}

bool PrintCheckHint::StopForV6UnInited(const std::string& sn, wxWindow* parent)
{
    PrintCheckHint printCheckHint;
    auto currentDev = CurDevObject(sn);
    if (printCheckHint.IsV6Uninited(currentDev)) {
        printCheckHint.Hint(currentDev, parent);
        return true;
    }
    return false;
}

void PrintCheckHint::SetFuncs(OkFunc_T okFunc, CancelFunc_T cancelFunc)
{
    m_okFunc = okFunc;
    m_cancelFunc = cancelFunc;
}

void PrintCheckHint::SetInfoByType(DeviceObjectBasePtr currentDev, HintType type)
{
    if (type == HintType::NEED_LEVEL) {
        m_type = type;
        m_reminderStr = wxString::Format(_("common_print_popup_levelnotice"), "10").ToStdString(wxConvUTF8);
        if (currentDev->IsMultiColorDevice())
            m_reminderStr = wxString::Format(_("common_print_popup_levelnotice"), "15").ToStdString(wxConvUTF8);

        m_title = _("common_print_popupfinished_noticefailed").ToStdString(wxConvUTF8);
    }
}

bool PrintCheckHint::CheckNeedRemind(
    DeviceObjectBasePtr currentDev, 
    const std::string& utf8GcodeFile)
{
    m_currentDev = currentDev;
    if (!currentDev) {
        ANKER_LOG_WARNING << "current dev is null, no need print check";
        return false;
    }

    QueryStatus(currentDev);
    // TXTODO: this should block and wait for the status
    
    // if device not onlined, so don't remind
    if (!currentDev->GetOnline()) {
        ANKER_LOG_WARNING << "device is offline, no need print check";
        return false;
    }

    ANKER_LOG_INFO << "cur devicePartsType: " << currentDev->GetDevicePartsType();

    m_type = HintType::ONLY_OK;
    // V6 uninited reminder
    if (IsV6Uninited(currentDev)) {
        ANKER_LOG_INFO << "v6 is uninited, need print check";
        return true;
    }
    // V6 need calibrate reminder
    if (!currentDev->GetIsCalibrated()) {
        ANKER_LOG_INFO << "not calibrated, need print check";
        m_reminderStr = wxString::Format(_L("common_print_popup_v6calibrationnotice")).ToStdString(wxConvUTF8);
        m_title = _L("common_popup_titlenotice").ToStdString(wxConvUTF8);
        return true;
    }
    // V6 have calibrator reminder
    if (currentDev->GetHaveCalibrator()) {
        ANKER_LOG_INFO << "have calibrator, need print check";
        m_reminderStr = wxString::Format(_L("common_print_popup_v6calibrationremove")).ToStdString(wxConvUTF8);
        m_title = _L("common_popup_titlenotice").ToStdString(wxConvUTF8);
        return true;
    }

    // need leving
    if (!currentDev->GetIsLeveled()) {
        ANKER_LOG_INFO << "unleveled, need print check";
        SetInfoByType(currentDev, HintType::NEED_LEVEL);
        return true;
    }

    // gcode && machine type match check
    if (!IsGcodeMachineMatched(currentDev, utf8GcodeFile)) {
        ANKER_LOG_INFO << "gcode machine not match, need print check";
        m_title = _L("common_popup_titlenotice").ToStdString(wxConvUTF8);
        return true;
    }

    // nozzle and heatbed temperature hint check (only transfer gcode)
    if (!utf8GcodeFile.empty() && IsTempertureHint(currentDev)) {
        ANKER_LOG_INFO << "nozzle and heatbed temperature, need print check";
        m_title = _L("common_popup_titlenotice").ToStdString(wxConvUTF8);
        return true;
    }

    return false;
}

// return false: no continue to print
bool PrintCheckHint::Hint(DeviceObjectBasePtr currentDev,
    wxWindow* parent,
    const wxSize& dialogSize, 
    const wxPoint& dialogPosition, 
    bool remotePrint)
{
    wxSize childSize = dialogSize;
    if (dialogSize == wxDefaultSize) {
        childSize = AnkerSize(400, 200);
    }

    wxPoint childPos = dialogPosition;
    // popup in the center of parent
    if (parent && dialogPosition == wxDefaultPosition) {
        wxPoint parentCenterPoint(parent->GetPosition().x + parent->GetSize().GetWidth() / 2,
            parent->GetPosition().y + parent->GetSize().GetHeight() / 2);
        childPos = wxPoint(parentCenterPoint.x - childSize.x / 2,
            parentCenterPoint.y - childSize.y / 2);
    }

    wxWindow* parentTmp = parent;
#if !defined (WIN32)
    parentTmp = nullptr;
#endif

    ANKER_LOG_INFO << "cur type: " << (int)m_type;
    if (m_type == HintType::ONLY_OK) {
         AnkerMessageBox(nullptr, m_reminderStr, m_title, false);
         if (m_okFunc) {
             m_okFunc(m_currentDev ? m_currentDev->GetSn() : "");
         }
         return false;
    }

    if (m_type == HintType::NEED_LEVEL) {
        AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, m_reminderStr, m_title);
        if (result == AnkerMsgDialog::MsgResult::MSG_OK) {
            if (m_currentDev) {
                m_currentDev->setLevelBegin();
            }
            if (m_okFunc) {
                ANKER_LOG_INFO << "call okFunc";
                m_okFunc(m_currentDev ? m_currentDev->GetSn() : "");
            }
        }
        else {
            if (m_currentDev) {
                m_currentDev->clearDeviceCtrlResult();
            }
            if (m_cancelFunc) {
                ANKER_LOG_INFO << "call cancelFunc";
                m_cancelFunc(m_currentDev ? m_currentDev->GetSn() : "");
            }
        }
        return false;
    }
    bool needTphint = false;
    if (m_type == HintType::GCODE_DEVICE_NO_MATCH ||
        m_type == HintType::GCODE_NOT_ANKERBAND) {
        auto ret = GcodeNoMatchHint(parentTmp, childSize, childPos);
        if (!ret) {
            return false;
        }
        if (!remotePrint) {
            needTphint = IsTempertureHint(currentDev);
        }
    }
    ANKER_LOG_INFO << "needTphint: " << needTphint;
    if (needTphint || m_type == HintType::TEMPERATURE_HINT) {
        TemperatureHint(parentTmp, childSize, childPos);
        return true;
    }

    return true;
}

bool PrintCheckHint::Hint(
    DeviceObjectBasePtr currentDev,
    HintType type, 
    wxWindow* parent, 
    const wxSize& dialogSize, 
    const wxPoint& dialogPosition)
{
    SetInfoByType(currentDev, type);
    m_currentDev = currentDev;
    return Hint(currentDev, parent, dialogSize, dialogPosition);
}

bool PrintCheckHint::IsGcodeMachineMatched(
    DeviceObjectBasePtr currentDev, 
    const std::string& gcodeFile)
{
    if (gcodeFile.empty()) {
        auto gcodeInfo = currentDev->GetGcodeInfo();
        if (gcodeInfo.isNull) {
            ANKER_LOG_WARNING << "gcodeInfo get nullptr";
            m_type = HintType::GCODE_NOT_ANKERBAND;
            return false;
        }
        ANKER_LOG_INFO << "get gcode status " << gcodeInfo.file_status << " from device";

        if (gcodeInfo.file_status == MatchGCodeFileStatusType_Normal) {
            return true;
        }
        if (gcodeInfo.file_status == MatchGCodeFileStatusType_NotBelongToCurrentPrinter) {
            m_type = HintType::GCODE_DEVICE_NO_MATCH;
        }
        else {
            m_type = HintType::GCODE_NOT_ANKERBAND;
        }
        return false;
    }
    else {
        bool isAnkerBrand;
        anker_device_type machineType;
        Slic3r::GcodeInfo::GetMachineInfoFromGCode(gcodeFile, isAnkerBrand, machineType);
        bool ret = false;
        if (machineType == DEVICE_V7111_TYPE) {
            ret = currentDev->GetDevicePartsType() == DEVICE_PARTS_MULTI_COLOR;
        }
        else if (currentDev->GetDevicePartsType() == DEVICE_PARTS_MULTI_COLOR) {
            ret = machineType == DEVICE_V7111_TYPE;
        }
        else {
            ret = machineType == currentDev->GetDeviceType();
        }
        if (!isAnkerBrand) {
            m_type = HintType::GCODE_NOT_ANKERBAND;
        }
        else if (!ret) {
            m_type = HintType::GCODE_DEVICE_NO_MATCH;
        }
        ANKER_LOG_INFO << "isAnkerBrand: " << isAnkerBrand << " machineType: " << machineType << ", " <<
            currentDev->GetDeviceType() << ", parts: " << currentDev->GetDevicePartsType() << ", ret: " << ret <<
            ", type: " << (int)m_type;

        return isAnkerBrand && ret;
    }

    return true;
}

bool PrintCheckHint::IsTempertureHint(DeviceObjectBasePtr currentDev)
{    
    if (!DevVerMatchForTempHint(currentDev)) {
        return false;
    }

    auto appConfig = Slic3r::GUI::wxGetApp().app_config;

    auto needHint = true;
    if (appConfig) {
        auto hintConfig = appConfig->get("Print Check", "nozzlebed_temperature_hint");
        if (!hintConfig.empty()) {
            needHint = hintConfig != "0";
        }
        ANKER_LOG_INFO << "get nozzlebed_temperature_hint: " << needHint;
    }
    if (needHint) {
        m_type = HintType::TEMPERATURE_HINT;
    }
    return needHint;
}

bool PrintCheckHint::GcodeNoMatchHint(wxWindow* parent, const wxSize& dialogSize, const wxPoint& dialogPosition)
{
    auto appConfig = Slic3r::GUI::wxGetApp().app_config;

    bool haveCheckBox = false;
    if (m_type == HintType::GCODE_DEVICE_NO_MATCH) {
        m_reminderStr = _L("common_print_gcode_device_nomatch_hint").ToStdString(wxConvUTF8);
    }
    if (m_type == HintType::GCODE_NOT_ANKERBAND) {
        haveCheckBox = true;
        m_reminderStr = _L("common_print_gcode_not_anker_hint").ToStdString(wxConvUTF8);
    }

    auto printCheck = false;
    if (appConfig) {
        printCheck = appConfig->get_bool("Print Check", "machine_gcode_unmatched_nohint");
    }
    ANKER_LOG_INFO << "haveCheckBox: " << haveCheckBox << ", printCheck: " << printCheck;
    if (haveCheckBox && printCheck) {
        ANKER_LOG_INFO << "no need to match the machine and gcode's machine name for user set";
        return true;
    }

    AnkerDialog dialog(parent, wxID_ANY, wxString::FromUTF8(m_title), wxString::FromUTF8(m_reminderStr), dialogPosition, dialogSize);

    int result = wxID_CANCEL;
    bool isCheck = false;
    if (!haveCheckBox) {
        result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextNoYesDialog);
    }
    else {
        result = dialog.ShowAnkerModal2(AnkerDialogType_DisplayTextCheckBoxNoYesDialog, _L("common_button_donotshowagain"),
            [appConfig, &isCheck](wxCommandEvent& event, wxControl* control) {
                auto checkBox = dynamic_cast<wxCheckBox*>(control);
                if (!checkBox || !appConfig) {
                    return;
                }
                isCheck = checkBox->IsChecked();
            });
    }
    ANKER_LOG_INFO << "dialog exe result: " << result;
    if (result != wxID_OK) {
        ANKER_LOG_INFO << "you select not continue to print";
        if (m_cancelFunc) {
            ANKER_LOG_INFO << "call cancelFunc";
            m_cancelFunc(m_currentDev ? m_currentDev->GetSn() : "");
        }
        return false;
    }
    if (haveCheckBox && appConfig) {
        appConfig->set("Print Check", "machine_gcode_unmatched_nohint", isCheck ? "1" : "0");
    }
    if (m_okFunc) {
        ANKER_LOG_INFO << "call okFunc";
        m_okFunc(m_currentDev ? m_currentDev->GetSn() : "");
    }

    return true;
}

void PrintCheckHint::TemperatureHint(wxWindow* parent, const wxSize& dialogSize, const wxPoint& dialogPosition)
{
    AnkerDialog dialog(parent, wxID_ANY, wxString::FromUTF8(m_title), 
        _L("common_printnotice_temperature_hint")
        /*("The nozzle and heatbed will preheat during the file transferring, please be careful not to touch them.")*/,
        dialogPosition, dialogSize);

    dialog.ShowAnkerModal2(AnkerDialogType_DisplayTextCheckBoxOkDialog, _L("common_button_donotshowagain"),
        [](wxCommandEvent& event, wxControl* control) {
            auto checkBox = dynamic_cast<wxCheckBox*>(control);
            if (!checkBox) {
                return;
            }
            auto appConfig = Slic3r::GUI::wxGetApp().app_config;
            auto isCheck = checkBox->IsChecked();
            if (appConfig) {
                appConfig->set("Print Check", "nozzlebed_temperature_hint", isCheck ? "0" : "1");
                ANKER_LOG_INFO << "set nozzlebed_temperature_hint: " << isCheck;
            }
        });
}

// return true: current device should show the temperature hint
bool PrintCheckHint::DevVerMatchForTempHint(DeviceObjectBasePtr currentDev)
{
    if (!currentDev) {
        ANKER_LOG_INFO << "current dev is nullptr";
        return false;
    }
    if (currentDev->GetPreheatFunction()) {
        return true;
    }

    auto curDevVesion = currentDev->GetDeviceVersion();
    std::string targetVersion;
    
    ANKER_LOG_INFO << "cur device version: " << curDevVesion << ", device type: "
        << currentDev->GetDeviceType();
    if (currentDev->GetDeviceType() == DEVICE_V8110_TYPE) {
        if (DeviceVersionUtil::Equal(curDevVesion, V8110_PREHEAT_HINT_VERSION1) ||
            DeviceVersionUtil::Equal(curDevVesion, V8110_PREHEAT_HINT_VERSION2)) {
            return true;
        }
        return false;
    }
    else if (currentDev->GetDeviceType() == DEVICE_V8111_TYPE) {
        if (DeviceVersionUtil::Equal(curDevVesion, V8111_PREHEAT_HINT_VERSION)) {
            return true;
        }
        return false;
    }
    else {
        ANKER_LOG_INFO << "not suit device, no need temp hint";
        return false;
    }

    auto ret = DeviceVersionUtil::IsTargetLessCurrent(targetVersion, curDevVesion);
    return ret;
}
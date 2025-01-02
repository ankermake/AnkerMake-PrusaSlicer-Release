#include "AnkerTaskPanel.hpp"
#include "AnkerBtn.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "slic3r/GUI/Common/AnkerLoadingMask.hpp"
#include "slic3r/GUI/Common/AnkerProgressCtrl.hpp"
#include "slic3r/Utils/DataMangerUi.hpp"
#include "wx/dc.h"
#include "wx/artprov.h"
#include "wx/event.h"
#include <wx/clntdata.h>

#include <ctime>
#include <slic3r/Utils/GcodeInfo.hpp>
#include <slic3r/GUI/GcodeVerify/PrintCheckHint.hpp>
#include "AnkerNetModule/BuryDefines.h"
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"
#include "AnkerComFunction.hpp"


#define USE_OLD_PRINT_FINISH_UI 1

#define CONTROL_DISABLE_COLOR 125, 125, 125

wxDEFINE_EVENT(wxANKEREVT_CALIBRATING_STOPPED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_LEVELING_STOPPED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_STARTED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_PAUSED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_CONTINUE, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_STOPPED, wxCommandEvent);

AnkerTaskPanel::AnkerTaskPanel(std::string currentSn, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, AnkerSize(760, 241))
    , m_printCompleteCheckFlag(false)
    , m_modaling(false)
    , m_toPrinting(false)
    , m_lastLeftSeconds(INT_MAX)
    , m_currentDeviceSn(currentSn)
    , m_lastDeviceStatus(GUI_DEVICE_STATUS_TYPE_IDLE)
    , m_currentMode(TASK_MODE_NONE)
    , m_pInfoEmptyPanel(nullptr)
    , m_pInfoPanel(nullptr)
    , m_pCtrlHSizer(nullptr)
    , m_pSpeedInfoPanel(nullptr)
    , m_pFilamentInfoPanel(nullptr)
    , m_pLayerInfoPanel(nullptr)
    , m_pFinishTimeInfoPanel(nullptr)
    , m_pPreviewImage(nullptr)
    , m_pTimeLeftValueText(nullptr)
    , m_pTimeLeftLabelText(nullptr)
    , m_pSpeedInfoText(nullptr)
    , m_pFilamentInfoText(nullptr)
    , m_pLayerInfoText(nullptr)
    , m_pFinishTimeInfoText(nullptr)
    , m_pCtrlTitleText(nullptr)
    , m_pCtrlStatusText(nullptr)
    , m_pRefreshButton(nullptr)
    , m_pStartButton(nullptr)
    , m_pStopButton(nullptr)
    , m_pPauseButton(nullptr)
    , m_pTaskProgressCtrl(nullptr)
    , m_pGCodeImportDialog(nullptr)
    , m_pLoadingMask(nullptr)
    , m_countDownSeconds(-1)
{
    m_panelBackColor = wxColour(PANEL_BACK_RGB_INT);
    m_textLightColor = wxColour(TEXT_LIGHT_RGB_INT);
    m_textDarkColor = wxColour(TEXT_DARK_RGB_INT);
    m_systemColor = wxColour(ANKER_RGB_INT);
    m_imageBackColor = wxColour(PANEL_BACK_LIGHT_RGB_INT);

    initUI();

    m_pGCodeImportDialog = new AnkerGCodeImportDialog(m_currentDeviceSn, this);
    m_pGCodeImportDialog->Hide();

    m_pPrintFinishDialog = new AnkerPrintFinishDialog(m_currentDeviceSn, this);
    m_pPrintFinishDialog->Hide();

    Bind(wxEVT_SIZE, &AnkerTaskPanel::OnSize, this);

    m_pToPrintingTimer = new wxTimer();
    m_pToPrintingTimer->Bind(wxEVT_TIMER, [this](wxTimerEvent& event) {
        ANKER_LOG_INFO << "m_pToPrintingTimer Enter";
        m_toPrinting = false; 
        PrintStartType type = PrintStartType::TYPE_COUNT;
        switch (m_gcodeImportResult.m_srcType)
        {
        case Slic3r::GUI::FileSelectMode::FSM_COMPUTER:
            type = PrintStartType::TYPE_LOCAL_FILE;
            break;
        case Slic3r::GUI::FileSelectMode::FSM_STORAGE:
            type = PrintStartType::TYPE_REMOTE_STORAGE;
            break;
        case Slic3r::GUI::FileSelectMode::FSM_USB:
            type = PrintStartType::TYPE_REMOTE_USB;
            break;
        case Slic3r::GUI::FileSelectMode::FSM_SLICE:
            type = PrintStartType::TYPE_SLICE;
            break;
        default:
            type = PrintStartType::TYPE_COUNT;
            break;
        }
        if (type != PrintStartType::TYPE_COUNT)
        {
            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_print_type, std::to_string(type)));
            buryMap.insert(std::make_pair(c_result, std::to_string(PrintStartResult::START_FAIL)));
            BuryAddEvent(e_print_start, buryMap);
        }
        });

    initTimer();

    m_pLocalCounter = new wxTimer();
    m_pLocalCounter->Bind(wxEVT_TIMER, &AnkerTaskPanel::OnCountdownTimer, this);
}

AnkerTaskPanel::~AnkerTaskPanel()
{
    m_pLoadingMask = nullptr;

    if (m_pToPrintingTimer)
    {
        delete m_pToPrintingTimer;
        m_pToPrintingTimer = nullptr;
    }

    if (m_pLocalCounter)
    {
        delete m_pLocalCounter;
        m_pLocalCounter = nullptr;
    }
}

void AnkerTaskPanel::initTimer()
{
    m_pModalingTimer = new wxTimer();

#if USE_OLD_PRINT_FINISH_UI
    m_pModalingTimer->Bind(wxEVT_TIMER, [&](wxTimerEvent& event) {
        ANKER_LOG_INFO << "m_pModalingTimer Enter";

        auto finish_dialog = [&](bool bSuccess, AnkerPrintFinishDialog::PrintFinishInfo& result) {
            int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
            int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
            m_pPrintFinishDialog->SetPosition(wxPoint((screenW - m_pPrintFinishDialog->GetSize().x) / 2, (screenH - m_pPrintFinishDialog->GetSize().y) / 2));
            m_pPrintFinishDialog->setCurrentDeviceSn(m_currentDeviceSn);

            AnkerGCodeImportDialog::GCodeImportResult importResult = m_pGCodeImportDialog->getImportResult();
            result.m_previewImage = importResult.m_previewImage;
            m_pPrintFinishDialog->ShowPrintFinishDialog(bSuccess, result);
        };

        DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
        if (!currentDev) {
            return;
        }

        if (m_modalingDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FAILED)
        {
            ANKER_LOG_INFO << "showModal for print failed";
            m_printCompleteCheckFlag = true;

            AnkerPrintFinishDialog::PrintFinishInfo result;
            auto layerPtr = currentDev->GetLayerPtr();
            auto dataInfo = currentDev->GetPrintFailedInfo();
            if (!dataInfo.isNull)
            {
                std::string filamentUsedStr = "0.0";
                if (!layerPtr.isNull && layerPtr.total_layer > 0) {
                    float filamentUsed = (float)layerPtr.real_print_layer / (float)layerPtr.total_layer * dataInfo.filamentUsed;
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(1) << filamentUsed;
                    filamentUsedStr = ss.str();
                    result.m_currentLayer = layerPtr.real_print_layer;
                    result.m_totleLayer = layerPtr.total_layer;
                }

                if (!dataInfo.name.empty() && result.m_fileName != dataInfo.name)
                    result.m_fileName = dataInfo.name;
                result.m_filamentStr = filamentUsedStr + dataInfo.filamentUnit;
                if (dataInfo.totalTime >= 0)
                    result.m_timeSecond = dataInfo.totalTime;
            }

            finish_dialog(false, result);
            DatamangerUi::GetInstance().SetIsPrintFinishFailedDialogShow(true);
            auto dialogRet = m_pPrintFinishDialog->ShowModal();
            DatamangerUi::GetInstance().SetIsPrintFinishFailedDialogShow(false);

            if (dialogRet == wxOK) {
                currentDev->setDevicePrintAgain();
            }
            else {
                currentDev->resetDeviceIdel();
                m_gcodeImportResult.m_filamentStr = "--";
                m_gcodeImportResult.m_speedStr = "--";
                m_gcodeImportResult.m_fileName = "--";
                m_gcodeImportResult.m_timeSecond = 0;
            }

            currentDev->clearExceptionFinished();
        }
        else if (m_modalingDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED)
        {
            ANKER_LOG_INFO << "showModal for print finished";
            m_printCompleteCheckFlag = true;

            AnkerPrintFinishDialog::PrintFinishInfo result;
            auto dataPtr = currentDev->GetPrintNoticeInfo();
            if (!dataPtr.isNull) {
                if (!dataPtr.name.empty())
                    result.m_fileName = dataPtr.name;
                result.m_filamentStr = std::to_string(dataPtr.filamentUsed) + dataPtr.filamentUnit;
                result.m_timeSecond = dataPtr.totalTime;
            }

            finish_dialog(true, result);
            DatamangerUi::GetInstance().SetIsPrintFinishFailedDialogShow(true);
            auto dialogRet = m_pPrintFinishDialog->ShowModal();
            DatamangerUi::GetInstance().SetIsPrintFinishFailedDialogShow(false);

            if (dialogRet == wxOK) {
                currentDev->setDevicePrintAgain();
            }
            else {
                currentDev->resetDeviceIdel();
                m_gcodeImportResult.m_filamentStr = "--";
                m_gcodeImportResult.m_speedStr = "--";
                m_gcodeImportResult.m_fileName = "--";
                m_gcodeImportResult.m_timeSecond = 0;
            }
        }

        m_modaling = false;
        });
#else
    m_pModalingTimer->Bind(wxEVT_TIMER, [this](wxTimerEvent& event) {
        if (m_modalingDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FAILED || m_modalingDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED)
        {
            m_printCompleteCheckFlag = true;


            Datamanger& dm = Datamanger::GetInstance();
            DeviceObjectBasePtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

            AnkerPrintFinishDialog::PrintFinishInfo result;
            result.finishSuss = m_modalingDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED;
            if (currentDev) {
                result.m_fileName = currentDev->printFile;
                result.m_currentLayer = currentDev->currentLayer;
                result.m_totleLayer = currentDev->totalLayer;
                result.m_timeSecond = currentDev->totalTime;

                double usedFilament = currentDev->filamentUsed * currentDev->progress * 1.0 / 10000.0;
                char usedFilamentText[20];
                sprintf(usedFilamentText, "%.2f", usedFilament);
                result.m_filamentStr = usedFilamentText + currentDev->filamentUnit;
            }

            AnkerGCodeImportDialog::GCodeImportResult importResult = m_pGCodeImportDialog->getImportResult();
            ANKER_LOG_INFO << "Print Finish " << wxString(result.finishSuss ? "success" : "fail") << "."
                << "  ==>DeviceObjectData: "
                << "   fileName:" << result.m_fileName
                << "   filamentStr:" << result.m_filamentStr
                << "   timeSecond:" << result.m_timeSecond
                << "   currentLayer:" << result.m_currentLayer
                << "   totleLayer:" << result.m_totleLayer
                << "  ==>gcodeImportResult: "
                << "   fileName:" << importResult.m_fileName
                << "   filamentStr:" << importResult.m_filamentStr
                << "   timeSecond:" << importResult.m_timeSecond;

            result.m_previewImage = importResult.m_previewImage;

            //result.m_filePath = currentDev->filp;
            int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
            int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
            m_pPrintFinishDialog->SetPosition(wxPoint((screenW - m_pPrintFinishDialog->GetSize().x) / 2, (screenH - m_pPrintFinishDialog->GetSize().y) / 2));
            m_pPrintFinishDialog->setCurrentDeviceSn(m_currentDeviceSn);
            m_pPrintFinishDialog->ShowPrintFinishDialog(false, result);
            if (m_pPrintFinishDialog->ShowModal() == wxOK)
            {
                currentDev->setDevicePrintAgain();
            }
            else
            {
                currentDev->resetDeviceIdel();

                m_gcodeImportResult.m_filamentStr = "--";
                m_gcodeImportResult.m_speedStr = "--";
                m_gcodeImportResult.m_fileName = "--";
                m_gcodeImportResult.m_timeSecond = 0;
            }
        }

        m_modaling = false;
        });
#endif
}

void AnkerTaskPanel::switchMode(TaskMode mode, bool force)
{
    if (m_currentMode != mode || force)
    {
        ANKER_LOG_INFO << "switch mode to: " << mode;

        switch (mode)
        {
        case AnkerTaskPanel::TASK_MODE_NONE:
            m_pInfoEmptyPanel->Show(true);
            m_pInfoPanel->Show(false);
            m_pCtrlHSizer->Show(false);

            m_pCtrlTitleText->SetLabelText("N/A");
            m_pCtrlTitleText->Fit();
            break;
        case AnkerTaskPanel::TASK_MODE_PRINT:
        {  
			m_pInfoEmptyPanel->Show(false);

			m_pInfoPanel->Show(true);
			m_pSpeedInfoPanel->Show(true);
			m_pFilamentInfoPanel->Show(true);
			m_pLayerInfoPanel->Show(true);
			m_pFinishTimeInfoPanel->Show(true);

			m_pCtrlHSizer->Show(true);
			m_pStartButton->Show(false);
			m_pStopButton->Show(true);
			m_pPauseButton->Show(true);

            m_pTaskProgressCtrl->updateProgress(0);

            // update preview image
            // Todo: request image from server: gcodeInfo->completeUrl
            wxImage image = m_gcodeImportResult.m_previewImage;
            if (m_gcodeImportResult.m_previewImage.IsOk())
                image = m_gcodeImportResult.m_previewImage;
            else
            {
                image = wxImage(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
            }
            image.Rescale(97, 97);
            image.Replace(0, 0, 0, m_imageBackColor.Red(), m_imageBackColor.Green(), m_imageBackColor.Blue());
            wxBitmap scaledBitmap(image);
            m_pPreviewImage->SetBitmap(scaledBitmap);

            m_printCompleteCheckFlag = false;
		}
		break;
        case AnkerTaskPanel::TASK_MODE_LEVEL:
        case AnkerTaskPanel::TASK_MODE_CALIBRATE:
        {
            DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
            m_pInfoEmptyPanel->Show(false);

            m_pInfoPanel->Show(true);
            m_pSpeedInfoPanel->Show(false);
            m_pFilamentInfoPanel->Show(false);
            if (currentDev && currentDev->IsMultiColorDevice())
                m_pLayerInfoPanel->Show(false);
            else
                m_pLayerInfoPanel->Show(true);
            m_pFinishTimeInfoPanel->Show(false);

            m_pCtrlHSizer->Show(true);
            m_pStartButton->Show(false);
            m_pStopButton->Show(true);
            m_pPauseButton->Show(false);

            m_pTaskProgressCtrl->updateProgress(0);

            wxString levelImagePath = wxString::FromUTF8(Slic3r::var("V8110_Online_n.png"));
            if (currentDev && currentDev->GetDeviceType() == anker_device_type::DEVICE_V8111_TYPE)
                levelImagePath = wxString::FromUTF8(Slic3r::var("V8111_Online.png"));
            wxImage image = wxImage(levelImagePath, wxBITMAP_TYPE_PNG);
            image.Rescale(97, 97);
            image.Replace(0, 0, 0, m_imageBackColor.Red(), m_imageBackColor.Green(), m_imageBackColor.Blue());
            wxBitmap scaledBitmap(image);
            m_pPreviewImage->SetBitmap(scaledBitmap);
        }
		break;
        case AnkerTaskPanel::TASK_MODE_TRANFER:
        {
            m_pInfoEmptyPanel->Show(false);

            m_pInfoPanel->Show(true);
            m_pSpeedInfoPanel->Show(true);
            m_pFilamentInfoPanel->Show(true);
            m_pLayerInfoPanel->Show(true);
            m_pFinishTimeInfoPanel->Show(true);

            m_pCtrlHSizer->Show(true);
            m_pStartButton->Show(false);
            m_pStopButton->Show(false);
            m_pPauseButton->Show(false);
            
            m_pTaskProgressCtrl->updateProgress(0);

            // update preview image
            // Todo: request image from server: gcodeInfo->completeUrl
            wxImage image = m_gcodeImportResult.m_previewImage;
            if (m_gcodeImportResult.m_previewImage.IsOk())
                image = m_gcodeImportResult.m_previewImage;
            else
            {
                image = wxImage(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
            }
            image.Rescale(97, 97);
            image.Replace(0, 0, 0, m_imageBackColor.Red(), m_imageBackColor.Green(), m_imageBackColor.Blue());
            wxBitmap scaledBitmap(image);
            m_pPreviewImage->SetBitmap(scaledBitmap);

            // update printer status
            m_pCtrlStatusText->SetLabelText(/*L"Transfering"*/wxString::Format(_("common_print_taskbar_statustransfere"), "0%"));

            // update printing gcode name
            m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(m_gcodeImportResult.m_fileName));
            m_pCtrlTitleText->Fit();

            // update printing gcode info
            //std::string speedStr = m_gcodeImportResult.m_speedStr;
            std::string speedStr = "--";
            m_pSpeedInfoText->SetLabelText(speedStr);
            m_pSpeedInfoText->Fit();

            //std::string filamentStr = m_gcodeImportResult.m_filamentStr;
            std::string filamentStr = "--";
            m_pFilamentInfoText->SetLabelText(filamentStr);
            m_pFilamentInfoText->Fit();

            std::string layerStr = "--/--";
            m_pLayerInfoText->SetLabelText(layerStr);
            m_pLayerInfoText->Fit();

            //if (m_gcodeImportResult.m_timeSecond > 0)
            //{
            //    time_t t = time(nullptr);
            //    struct tm* now = localtime(&t);
            //    int remainingSeconds = m_gcodeImportResult.m_timeSecond % 60;
            //    int remainingMinutes = m_gcodeImportResult.m_timeSecond / 60 % 60;
            //    int remainingHours = m_gcodeImportResult.m_timeSecond / 60 / 60 % 24;
            //    int remainingDays = m_gcodeImportResult.m_timeSecond / 60 / 60 / 24;
            //    int incre = 0;
            //    int finishSeconds = now->tm_sec + remainingSeconds;
            //    finishSeconds = finishSeconds >= 60 ? (incre = 1, finishSeconds - 60) : (incre = 0, finishSeconds);
            //    int  finishMinutes = now->tm_min + remainingMinutes + incre;
            //    finishMinutes = finishMinutes >= 60 ? (incre = 1, finishMinutes - 60) : (incre = 0, finishMinutes);
            //    int finishHours = now->tm_hour + remainingHours + incre;
            //    finishHours = finishHours >= 24 ? (incre = 1, finishHours - 24) : (incre = 0, finishHours);
            //    int finishDays = now->tm_wday + remainingDays + incre;
            //    std::string suffix;
            //    if (finishHours < 12)
            //        suffix = "AM";
            //    else
            //    {
            //        finishHours = finishHours > 12 ? finishHours - 12 : finishHours;
            //        suffix = "PM";
            //    }
            //    std::string timeStr = (finishHours < 10 ? "0" : "") + std::to_string(finishHours) + ":" + (finishMinutes < 10 ? "0" : "") + std::to_string(finishMinutes) + " " + suffix;
            //    m_pFinishTimeInfoText->SetLabelText(timeStr);
            //}
            //else
            {
                m_pFinishTimeInfoText->SetLabelText("--");
            }

            //m_printCompleteCheckFlag = false;
        }
        break;
        default:
            break;
        }

        m_pTimeLeftLabelText -> SetLabelText(/*L"Time Left"*/_("common_print_taskbar_timeleft"));
        m_lastLeftSeconds = INT_MAX;
        m_currentMode = mode;

        Layout();

        setLoadingVisible(false);
    }
}

void AnkerTaskPanel::updateStatus(std::string currentSn, int type)
{
    wxWindowUpdateLocker updateLocker(this);
	if (currentSn.empty())
		return;

    m_currentDeviceSn = currentSn;
    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);    
    if (currentDev == nullptr)
        return;

    if (m_pGCodeImportDialog)
        m_pGCodeImportDialog->setCurrentDeviceSn(m_currentDeviceSn);

    auto tmpDeviceStatus = currentDev->getGuiDeviceStatus();
    if (tmpDeviceStatus != m_currentDeviceStatus) {        
        ANKER_LOG_INFO << "change device {" << currentSn << "} gui_status " << 
            m_currentDeviceStatus << " -> " << tmpDeviceStatus;
        m_currentDeviceStatus = tmpDeviceStatus;
    }

    auto levelData = currentDev->GetProgressValue();
    auto calibrateData = currentDev->getCalibrationInfo();

    // level reminder
    if (!currentDev->GetIsLeveled(type)) {
        // mind level
        PrintCheckHint printCheckHint;
        printCheckHint.SetFuncs([this, &currentDev](const std::string& sn) {
            setLoadingVisible(false);
        }, [this](const std::string& sn) {
            setLoadingVisible(false);
        });
        ANKER_LOG_INFO << "start to check need remind for remote select gcode print";
        printCheckHint.Hint(currentDev, PrintCheckHint::HintType::NEED_LEVEL, this);
        return;
        }       
    
    if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINTING || 
        m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_HEATING || 
        m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_FILE_TRANSFER_DECOMPRESSING)
    {
        if (m_toPrinting)
        {
            PrintStartType type = PrintStartType::TYPE_COUNT;
            switch (m_gcodeImportResult.m_srcType)
            {
            case Slic3r::GUI::FileSelectMode::FSM_COMPUTER:
                type = PrintStartType::TYPE_LOCAL_FILE;
                break;
            case Slic3r::GUI::FileSelectMode::FSM_STORAGE:
                type = PrintStartType::TYPE_REMOTE_STORAGE;
                break;
            case Slic3r::GUI::FileSelectMode::FSM_USB:
                type = PrintStartType::TYPE_REMOTE_USB;
                break;
            case Slic3r::GUI::FileSelectMode::FSM_SLICE:
                type = PrintStartType::TYPE_SLICE;
                break;
            default:
                type = PrintStartType::TYPE_COUNT;
                break;
            }
            if (type != PrintStartType::TYPE_COUNT)
            {
                std::map<std::string, std::string> map;
                map.insert(std::make_pair(c_print_type, std::to_string(type)));
                map.insert(std::make_pair(c_result, std::to_string(PrintStartResult::START_SUCCESS)));
                BuryAddEvent(e_print_start, map);
            }
        }

        m_toPrinting = false;
        m_pToPrintingTimer->Stop();
    }

    if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINTING || 
        m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_HEATING ||
        m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PAUSED)
        switchMode(TASK_MODE_PRINT);
    else if (currentDev->GetGeneralException() != GeneralException2Gui::GeneralException2Gui_Auto_Level_Error &&
        (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_LEVELING || 
            m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_HEATING))
        switchMode(TASK_MODE_LEVEL);
    else if ((m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPECALIBRATION ||
        m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_CALIBRATION_PREHEATING))
        switchMode(TASK_MODE_CALIBRATE);
    else if (currentDev->getCustomDeviceStatus() == CustomDeviceStatus::CustomDeviceStatus_File_Transfer)
        switchMode(TASK_MODE_TRANFER);
    else if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_FILE_TRANSFER_DECOMPRESSING)
    {
        // TODO: may show info on the panel
    }
    else if (!m_toPrinting)
        switchMode(TASK_MODE_NONE);

    if (!m_modaling && m_pGCodeImportDialog && m_pPrintFinishDialog && !m_printCompleteCheckFlag)
    {
        if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FAILED || 
            m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED)
        {
            m_modaling = true;

            if (m_modalingDeviceStatus != m_currentDeviceStatus) {
                ANKER_LOG_INFO << "change modaling device {" << currentSn << "} status " <<
                    m_modalingDeviceStatus << " -> " << m_currentDeviceStatus;
                m_modalingDeviceStatus = m_currentDeviceStatus;
            }
            m_pModalingTimer->Stop();
            m_pModalingTimer->StartOnce(10);
        }
    }

    if ((m_lastDeviceStatus == GUI_DEVICE_STATUS_TYPE_LEVELING && m_currentDeviceStatus != GUI_DEVICE_STATUS_TYPE_LEVELING)
        || (m_lastDeviceStatus == GUI_DEVICE_STATUS_TYPECALIBRATION && m_currentDeviceStatus != GUI_DEVICE_STATUS_TYPECALIBRATION)
        || (m_lastDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINTING && m_currentDeviceStatus != GUI_DEVICE_STATUS_TYPE_PRINTING))
        m_pLocalCounter->Stop();

    if (m_currentMode == TASK_MODE_PRINT)
    {
        bool updateLeftTime = true;
        // update printer status
        switch (m_currentDeviceStatus)
        {
        case(GUI_DEVICE_STATUS_TYPE_PRINT_HEATING):
            m_pCtrlStatusText->SetLabelText(/*L"Print Heating"*/_("common_print_taskbar_statusheating"));
            m_pStopButton->Show(true);
            m_pStartButton->Show(false);
            m_pPauseButton->Show(false);

            // update the progress inner count down: just update the progress, not the left time label
            //m_pLocalCounter->Start(1000);
            m_countDownSeconds = -1;
            break;
        case(GUI_DEVICE_STATUS_TYPE_DOWNLOADING):
            m_pCtrlStatusText->SetLabelText(/*L"Downloading"*/_("common_print_taskbar_statusdownload"));

            // stop the progress inner count down
            m_countDownSeconds = -1;
            break;
        case(GUI_DEVICE_STATUS_TYPE_PRINTING):
            m_pCtrlStatusText->SetLabelText(/*L"Printing"*/_("common_print_taskbar_statusprinting"));
            m_pStopButton->Show(true);
            m_pStartButton->Show(false);
            m_pPauseButton->Show(true);

            setInfoPanelEnable(true);

            // update the progress inner count down
            if (m_lastLeftSeconds > currentDev->GetTime())
                m_countDownSeconds = currentDev->GetTime();
            
            if (m_lastLeftSeconds > 0 && m_lastLeftSeconds != INT_MAX && !m_pLocalCounter->IsRunning())
                m_pLocalCounter->Start(1000);

            m_lastLeftSeconds = currentDev->GetTime();
            updateLeftTime = false;
            break;
        case(GUI_DEVICE_STATUS_TYPE_PAUSED):
            m_pCtrlStatusText->SetLabelText(/*L"Paused"*/_("common_print_taskbar_statuspaused"));
            m_pStopButton->Show(true);
            m_pStartButton->Show(true);
            m_pPauseButton->Show(false);
            setInfoPanelEnable(false);

            // stop the progress inner count down
            m_countDownSeconds = -1;
            break;
        default:
            m_pCtrlStatusText->SetLabelText("--");
            break;
        }

        // update printing gcode name
        m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(currentDev->GetPrintFile()));
        m_pCtrlTitleText->Fit();

        // update printing gcode info
        auto noticeDataPtr = currentDev->GetPrintNoticeInfo();
        std::string speedStr =  std::to_string(0) + "mm/s";
        if (!noticeDataPtr.isNull) {
            speedStr = std::to_string(noticeDataPtr.realSpeed) + "mm/s";
        }
        m_pSpeedInfoText->SetLabelText(speedStr);
        m_pSpeedInfoText->Fit();
        double usedFilament = currentDev->GetFilamentUsed() * currentDev->GetProcess() * 1.0 / 10000.0;
        char usedFilamentText[10];
        sprintf(usedFilamentText, "%.2f", usedFilament);
        std::string filamentStr = usedFilamentText + currentDev->GetFilamentUnit();
        m_pFilamentInfoText->SetLabelText(filamentStr);
        m_pFilamentInfoText->Fit();

        std::string layerStr = std::to_string(currentDev->GetCurrentLayer()) + "/" + std::to_string(currentDev->GetTotalLayer());
        m_pLayerInfoText->SetLabelText(layerStr);
        m_pLayerInfoText->Fit();

        time_t t = time(nullptr);
        struct tm* now = localtime(&t);
        auto devTime = currentDev->GetTime();
        int remainingSeconds = devTime % 60;
        int remainingMinutes = devTime / 60 % 60;
        int remainingHours = devTime / 60 / 60 % 24;
        int remainingDays = devTime / 60 / 60 / 24;
        int incre = 0;
        int finishSeconds = now->tm_sec + remainingSeconds;
        finishSeconds = finishSeconds >= 60 ? (incre = 1, finishSeconds - 60) : (incre = 0, finishSeconds);
        int  finishMinutes = now->tm_min + remainingMinutes + incre;
        finishMinutes = finishMinutes >= 60 ? (incre = 1, finishMinutes - 60) : (incre = 0, finishMinutes);
        int finishHours = now->tm_hour + remainingHours + incre;
        finishHours = finishHours >= 24 ? (incre = 1, finishHours - 24) : (incre = 0, finishHours);
        int finishDays = now->tm_wday + remainingDays + incre;
        std::string suffix;
        if (finishHours < 12)
            suffix = "AM";
        else
        {
            finishHours = finishHours > 12 ? finishHours - 12 : finishHours;
            suffix = "PM";
        }
        std::string timeStr = (finishHours < 10 ? "0" : "") + std::to_string(finishHours) + ":" + (finishMinutes < 10 ? "0" : "") + std::to_string(finishMinutes) + " " + suffix;
        m_pFinishTimeInfoText->SetLabelText(timeStr);

        // update printing progress
        if (updateLeftTime)
        {
            remainingHours = remainingHours + remainingDays * 24;
            std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + ":" + (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + ":" + (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
            m_pTimeLeftValueText->SetLabelText(timeLeftStr); 
            m_pTimeLeftValueText->Fit();
        }

        {
            m_pTaskProgressCtrl->setProgressRange(10000);
            m_pTaskProgressCtrl->updateProgress(currentDev->GetProcess());
        }
    }
    else if (m_currentMode == TASK_MODE_LEVEL)
    {
        if (!currentDev->IsMultiColorDevice()) {
            bool updateLeftTime = true;

            setInfoPanelEnable(true);

            // update printer status
            switch (m_currentDeviceStatus)
            {
            case(GUI_DEVICE_STATUS_TYPE_HEATING):
                m_pCtrlStatusText->SetLabelText(/*L"Heating"*/_("common_print_taskbar_statusheating"));

                // update the progress inner count down: just update the progress, not the left time label
                //m_pLocalCounter->Start(1000);
                m_countDownSeconds = -1;
                break;
            case(GUI_DEVICE_STATUS_TYPE_LEVELING):
                m_pCtrlStatusText->SetLabelText(/*L"Leveling"*/_("common_print_taskbar_statusleveling"));

                // update the progress inner count down
                if (m_lastLeftSeconds > currentDev->GetTime())
                    m_countDownSeconds = currentDev->GetTime();

                if (m_lastLeftSeconds > 0 && m_lastLeftSeconds != INT_MAX && !m_pLocalCounter->IsRunning())
                    m_pLocalCounter->Start(1000);

                m_lastLeftSeconds = currentDev->GetTime();
                updateLeftTime = false;
                break;
            default:
                m_pCtrlStatusText->SetLabelText("--");

                // stop the progress inner count down
                m_countDownSeconds = -1;
                break;
            }

            // update level title
            m_pCtrlTitleText->SetLabelText(/*L"Auto Level"*/_("common_print_taskbar_progressleveling"));
            m_pCtrlTitleText->Fit();

            // update preview image

            // update level info
            std::string layerStr = std::to_string(levelData.value) + "/" + std::to_string(levelData.total_point);
            ANKER_LOG_DEBUG << "current level layer / total level layer: " << layerStr;
            m_pLayerInfoText->SetLabelText(layerStr);

            // update leveling progress
            if (updateLeftTime)
            {
                auto devTime = currentDev->GetTime();
                int remainingSeconds = devTime % 60;
                int remainingMinutes = devTime / 60 % 60;
                int remainingHours = devTime / 60 / 60;
                int remainingDays = devTime / 60 / 60 / 24;
                std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + ":" + 
                    (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + ":" + 
                    (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
                m_pTimeLeftValueText->SetLabelText(timeLeftStr);
                m_pTimeLeftValueText->Fit();
            }

            {
                m_pTaskProgressCtrl->setProgressRange(levelData.total_point);
                m_pTaskProgressCtrl->updateProgress(levelData.value);
            }
        }
        else {
            wxStaticText* StatusText = m_pTimeLeftValueText;
            StatusText->SetLabelText(/*L"Leveling"*/_("common_print_taskbar_statusleveling"));

            std::string levelValueStr = std::to_string(levelData.value) + "/" + std::to_string(levelData.total_point);
            if (levelData.value > levelData.total_point && levelData.total_point > 0) {
                levelValueStr = std::to_string(levelData.value % levelData.total_point) + "/" + std::to_string(levelData.total_point);
            }
            wxStaticText* levelingValueText = m_pTimeLeftLabelText;
            levelingValueText->SetLabelText(levelValueStr);

            m_pTaskProgressCtrl->setProgressRange(100);
            m_pTaskProgressCtrl->updateProgress(levelData.progress);

            m_pCtrlTitleText->SetLabelText(/*L"Auto Level"*/_("common_print_taskbar_progressleveling"));
            m_pCtrlTitleText->Fit();

            if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_HEATING) {
                levelingValueText->SetLabelText(wxString(""));
                m_pCtrlStatusText->SetLabelText(/*L"Heating"*/_("common_print_taskbar_statusheating"));
            }
            else if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_LEVELING) {
                m_pCtrlStatusText->SetLabelText(/*L"Leveling"*/wxString("#") + std::to_string(levelData.nozzle_index + 1) + 
                    " " + _("common_print_taskbar_statusleveling"));
            }
            else
            { 
                m_pCtrlStatusText->SetLabelText("--");
            }
            // ANKER_LOG_INFO << "leveling, #" << currentDev->nozzle_index + 1 <<"   value:" << levelValueStr <<"   progress:"<<currentDev->Progress;
        }
    }
    else if (m_currentMode == TASK_MODE_CALIBRATE && !calibrateData.isNull)
    {
        wxStaticText* StatusText = m_pTimeLeftValueText;
        StatusText->SetLabelText(_L("common_print_taskbar_statuscalibrating_title")/*_("common_print_taskbar_statuscalibrating")*/);

        wxString calibrateNozzleStr = wxString("#") + std::to_string(calibrateData.nozzelNum + 1) + "/" + std::to_string(6);
        wxStaticText* calibratingNozzleText = m_pTimeLeftLabelText;
        calibratingNozzleText->SetLabelText(calibrateNozzleStr);

        m_pTaskProgressCtrl->setProgressRange(100);
        m_pTaskProgressCtrl->updateProgress(calibrateData.progress);

        m_pCtrlTitleText->SetLabelText(/*L"Calibration"*/_("Calibration"));
        m_pCtrlTitleText->Fit();

        if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_CALIBRATION_PREHEATING) {
            calibratingNozzleText->SetLabelText(wxString(""));
            m_pCtrlStatusText->SetLabelText(/*L"Heating"*/_("common_print_taskbar_statusheating"));
        }
        else if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPECALIBRATION) {
            m_pCtrlStatusText->SetLabelText(/*L"Leveling"*/wxString::Format("#%d ", calibrateData.nozzelNum + 1) + 
                _("common_print_taskbar_statuscalibrating_title"));
        }
    }
    else if (m_currentMode == TASK_MODE_TRANFER)
    {
         // update printing gcode name
	     m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(currentDev->GetFileName()));
	     m_pCtrlTitleText->Fit();
     }

    Layout();
    Refresh();

    if (m_lastDeviceStatus != m_currentDeviceStatus) {
        ANKER_LOG_INFO << "change last device {" << currentSn << "} status " <<
            m_lastDeviceStatus << " -> " << m_currentDeviceStatus;
        m_lastDeviceStatus = m_currentDeviceStatus;
    }
}

void AnkerTaskPanel::requestCallback(int type)
{
    if (m_pGCodeImportDialog)
        m_pGCodeImportDialog->requestCallback(type);
}

void AnkerTaskPanel::initUI()
{
    SetSize(AnkerSize(760, 241));
    SetMinSize(AnkerSize(760, 241));

    wxBoxSizer* taskBarVSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(taskBarVSizer);
    SetBackgroundColour(m_panelBackColor);

	// title
	initTitlePanel(this);
	taskBarVSizer->Add(m_pTitledPanel, 0, wxEXPAND, 0);

    // split line
    wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
    splitLineCtrl->SetMaxSize(wxSize(100000, 1));
    splitLineCtrl->SetMinSize(wxSize(1, 1));
    taskBarVSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER, 0);

    // empty panel
    initInfoEmptyPanel(this);
    taskBarVSizer->Add(m_pInfoEmptyPanel, 1, wxEXPAND, 0);


    // task panel
    initInfoPanel(this);
    taskBarVSizer->Add(m_pInfoPanel, wxEXPAND | wxALL, wxEXPAND|wxALL, 0);

    m_pInfoEmptyPanel->Show(true);
    m_pInfoPanel->Show(false);

    // split line
	splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
	splitLineCtrl->SetMaxSize(wxSize(100000, 1));
	splitLineCtrl->SetMinSize(wxSize(1, 1));
	taskBarVSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER, 0);

    // control panel
    initControlPanel(this);
    taskBarVSizer->Add(m_pControlPanel, 0, wxEXPAND, 0);
    m_pCtrlHSizer->Show(false);
}

void AnkerTaskPanel::initTitlePanel(wxWindow* parent)
{
    m_pTitledPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    m_pTitledPanel->SetMinSize(AnkerSize(760, 35));
    wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pTitledPanel->SetSizer(titleHSizer);
    m_pTitledPanel->SetBackgroundColour(m_panelBackColor);

    titleHSizer->AddSpacer(12);

    wxStaticText* titleBanner = new wxStaticText(m_pTitledPanel, wxID_ANY, /*"Status"*/_("common_print_taskbar_progress"));
    titleBanner->SetMinSize(AnkerSize(100, 16));
    titleBanner->SetSize(AnkerSize(100, 35));
    titleBanner->SetBackgroundColour(m_panelBackColor);
    titleBanner->SetForegroundColour(m_textLightColor);
    titleBanner->SetCanFocus(false);
    titleBanner->SetFont(ANKER_BOLD_FONT_NO_1);
    titleBanner->Fit();
    titleHSizer->Add(titleBanner, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 7);

    titleHSizer->AddStretchSpacer(1);

    m_pTitledPanel->Fit();
}

void AnkerTaskPanel::initInfoEmptyPanel(wxWindow* parent)
{
    m_pInfoEmptyPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* emptyVSizer = new wxBoxSizer(wxVERTICAL);
    m_pInfoEmptyPanel->SetSizer(emptyVSizer);
    m_pInfoEmptyPanel->SetBackgroundColour(m_panelBackColor);

    m_titleEmptyBanner = new wxStaticText(m_pInfoEmptyPanel, wxID_ANY, "N/A");
    //titleEmptyBanner->SetMaxSize(AnkerSize(760, 20));
    m_titleEmptyBanner->SetBackgroundColour(m_panelBackColor);
    m_titleEmptyBanner->SetForegroundColour(m_textDarkColor);
    m_titleEmptyBanner->SetFont(ANKER_FONT_NO_1);
    m_titleEmptyBanner->Fit();
    emptyVSizer->Add(m_titleEmptyBanner, 0, wxALIGN_LEFT | wxLEFT | wxTOP, 12);

    wxBoxSizer* containerSizer = new wxBoxSizer(wxVERTICAL);
    emptyVSizer->Add(containerSizer, 1, wxEXPAND, 0);

    containerSizer->AddStretchSpacer(38);

    m_newTaskButton = new AnkerBtn(m_pInfoEmptyPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_CENTER);	
    m_newTaskButton->SetMinSize(AnkerSize(252, 24));
    m_newTaskButton->SetMaxSize(AnkerSize(252, 24));
    m_newTaskButton->SetText(_("common_print_taskbar_buttonstart"));
    m_newTaskButton->SetBackgroundColour(m_systemColor);
    m_newTaskButton->SetRadius(5);
    m_newTaskButton->SetTextColor(m_textLightColor);
    m_newTaskButton->SetFont(ANKER_BOLD_FONT_NO_1);
    m_newTaskButton->Bind(wxEVT_BUTTON, &AnkerTaskPanel::OnNewBtn, this);
    containerSizer->Add(m_newTaskButton, 0, wxALIGN_CENTER_HORIZONTAL, 0);

    containerSizer->AddStretchSpacer(66);

    m_pInfoEmptyPanel->Show(true);
}

void AnkerTaskPanel::initInfoPanel(wxWindow* parent)
{
    m_pInfoPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* taskHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pInfoPanel->SetSizer(taskHSizer);
    m_pInfoPanel->SetMinSize(AnkerSize(760, 160));
    //m_pInfoPanel->SetMaxSize(AnkerSize(760, 160));
    m_pInfoPanel->SetSize(AnkerSize(760, 160));
    m_pInfoPanel->SetBackgroundColour(m_panelBackColor);

    // preview image
    wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
    wxImage image = bitmapEx.ConvertToImage();
    image.Rescale(97, 97);
    image.Replace(0, 0, 0, m_imageBackColor.Red(), m_imageBackColor.Green(), m_imageBackColor.Blue());
    wxBitmap scaledBitmap(image);

    m_pPreviewImage = new wxStaticBitmap(m_pInfoPanel, wxID_ANY, scaledBitmap);
    m_pPreviewImage->SetMinSize(wxSize(123, 123));
    m_pPreviewImage->SetMaxSize(wxSize(123, 123));
    m_pPreviewImage->SetSize(wxSize(123, 123));
    m_pPreviewImage->SetBackgroundColour(m_imageBackColor);
    taskHSizer->Add(m_pPreviewImage, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 16);

    // task detail
    wxBoxSizer* taskVDetailSizer = new wxBoxSizer(wxVERTICAL);
    taskHSizer->Add(taskVDetailSizer, 6, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxLEFT, 16);

    taskVDetailSizer->AddStretchSpacer(1);

    // remaining time
    m_pTimeLeftValueText = new wxStaticText(m_pInfoPanel, wxID_ANY, "00:00:00");
    m_pTimeLeftValueText->SetMinSize(AnkerSize(144, 55));
    m_pTimeLeftValueText->SetMaxSize(AnkerSize(500, 55));
    m_pTimeLeftValueText->SetBackgroundColour(m_panelBackColor);
    m_pTimeLeftValueText->SetForegroundColour(m_systemColor);
    m_pTimeLeftValueText->SetFont(ANKER_BOLD_FONT_SIZE(30));
    m_pTimeLeftValueText->Fit();
    taskVDetailSizer->Add(m_pTimeLeftValueText, 0, wxEXPAND | wxALIGN_BOTTOM | wxALIGN_LEFT, 0);

    taskVDetailSizer->AddSpacer(1);

    // remaining label
    m_pTimeLeftLabelText = new wxStaticText(m_pInfoPanel, wxID_ANY, /*"Time Left"*/_("common_print_taskbar_timeleft"));
    m_pTimeLeftLabelText->SetMinSize(AnkerSize(100, 20));
    m_pTimeLeftLabelText->SetSize(AnkerSize(100, 20));
    m_pTimeLeftLabelText->SetBackgroundColour(m_panelBackColor);
    m_pTimeLeftLabelText->SetForegroundColour(m_systemColor);
    m_pTimeLeftLabelText->SetFont(ANKER_FONT_NO_1);
    m_pTimeLeftLabelText->Fit();
    taskVDetailSizer->Add(m_pTimeLeftLabelText, 0, wxEXPAND | wxALIGN_TOP | wxALIGN_LEFT, 0);

    // progress
    m_pTaskProgressCtrl = new AnkerProgressCtrl(m_pInfoPanel);
    m_pTaskProgressCtrl->SetMinSize(AnkerSize(300, 30));
    m_pTaskProgressCtrl->SetMaxSize(AnkerSize(20000, 30));
    m_pTaskProgressCtrl->setLineWidth(5);
    m_pTaskProgressCtrl->setProgressColor(m_systemColor);
    m_pTaskProgressCtrl->updateProgress(0);
    m_pTaskProgressCtrl->setLabelFont(ANKER_FONT_NO_1);
    taskVDetailSizer->Add(m_pTaskProgressCtrl, 0, wxEXPAND | wxALIGN_LEFT, 0);

    // printing detail
    wxBoxSizer* taskInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
    taskVDetailSizer->Add(taskInfoHSizer, 1, wxEXPAND | wxALIGN_LEFT | wxTOP | wxBOTTOM, 2);

    // speed
    {
        m_pSpeedInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* speedHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pSpeedInfoPanel->SetSizer(speedHSizer);
        m_pSpeedInfoPanel->SetSizeHints(AnkerSize(90, 30), AnkerSize(180, 30));
        taskInfoHSizer->Add(m_pSpeedInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap speedBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("printer_speed.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* speedIcon = new wxStaticBitmap(m_pSpeedInfoPanel, wxID_ANY, speedBitmap);
        speedIcon->SetMinSize(speedBitmap.GetSize());
        speedHSizer->Add(speedIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        speedHSizer->AddSpacer(8);

        m_pSpeedInfoText = new wxStaticText(m_pSpeedInfoPanel, wxID_ANY, "250mm/s");
        m_pSpeedInfoText->SetBackgroundColour(m_panelBackColor);
        m_pSpeedInfoText->SetForegroundColour(m_textDarkColor);
        m_pSpeedInfoText->SetFont(ANKER_FONT_NO_1);
        speedHSizer->Add(m_pSpeedInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        speedHSizer->AddSpacer(AnkerLength(10));
    }

    // used_filament
    {
        m_pFilamentInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* filamentInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pFilamentInfoPanel->SetSizer(filamentInfoHSizer);
        m_pFilamentInfoPanel->SetMinSize(AnkerSize(90, 30));
        m_pFilamentInfoPanel->SetMaxSize(AnkerSize(180, 30));
        taskInfoHSizer->Add(m_pFilamentInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFilamentInfoPanel, wxID_ANY, usedFBitmap);
        usedFIcon->SetMinSize(usedFBitmap.GetSize());
        filamentInfoHSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        filamentInfoHSizer->AddSpacer(8);

        m_pFilamentInfoText = new wxStaticText(m_pFilamentInfoPanel, wxID_ANY, "100g");
        m_pFilamentInfoText->SetBackgroundColour(m_panelBackColor);
        m_pFilamentInfoText->SetForegroundColour(m_textDarkColor);
        m_pFilamentInfoText->SetFont(ANKER_FONT_NO_1);
        filamentInfoHSizer->Add(m_pFilamentInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        filamentInfoHSizer->AddSpacer(AnkerLength(10));
    }

    // layers
    {
        m_pLayerInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* layerInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pLayerInfoPanel->SetSizer(layerInfoHSizer);
        m_pLayerInfoPanel->SetMinSize(AnkerSize(90, 30));
        m_pLayerInfoPanel->SetMaxSize(AnkerSize(180, 30));
        taskInfoHSizer->Add(m_pLayerInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap layerBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("layer_icon.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* layerIcon = new wxStaticBitmap(m_pLayerInfoPanel, wxID_ANY, layerBitmap);
        layerIcon->SetMinSize(layerBitmap.GetSize());
        layerInfoHSizer->Add(layerIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        layerInfoHSizer->AddSpacer(8);

        m_pLayerInfoText = new wxStaticText(m_pLayerInfoPanel, wxID_ANY, "0 / 0");
        m_pLayerInfoText->SetBackgroundColour(m_panelBackColor);
        m_pLayerInfoText->SetForegroundColour(m_textDarkColor);
        m_pLayerInfoText->SetFont(ANKER_FONT_NO_1);
        layerInfoHSizer->Add(m_pLayerInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        layerInfoHSizer->AddSpacer(AnkerLength(10));
    }

    // finish time
    {
        m_pFinishTimeInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* finishTimeInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pFinishTimeInfoPanel->SetSizer(finishTimeInfoHSizer);
        m_pFinishTimeInfoPanel->SetMinSize(AnkerSize(90, 30));
        m_pFinishTimeInfoPanel->SetMaxSize(AnkerSize(180, 30));
        taskInfoHSizer->Add(m_pFinishTimeInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap ftBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* ftIcon = new wxStaticBitmap(m_pFinishTimeInfoPanel, wxID_ANY, ftBitmap);
        ftIcon->SetMinSize(ftBitmap.GetSize());
        finishTimeInfoHSizer->Add(ftIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        finishTimeInfoHSizer->AddSpacer(8);

        m_pFinishTimeInfoText = new wxStaticText(m_pFinishTimeInfoPanel, wxID_ANY, "00:00 am");
        m_pFinishTimeInfoText->SetBackgroundColour(m_panelBackColor);
        m_pFinishTimeInfoText->SetForegroundColour(m_textDarkColor);
        m_pFinishTimeInfoText->SetFont(ANKER_FONT_NO_1);
        finishTimeInfoHSizer->Add(m_pFinishTimeInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        finishTimeInfoHSizer->AddSpacer(AnkerLength(10));
    }

    taskInfoHSizer->AddStretchSpacer(1);
}

void AnkerTaskPanel::initControlPanel(wxWindow* parent)
{
    m_pControlPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* ctrlTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pControlPanel->SetSizer(ctrlTitleHSizer);
    m_pControlPanel->SetMinSize(AnkerSize(760, 55));
    m_pControlPanel->SetSize(AnkerSize(760, 55));
    m_pControlPanel->SetBackgroundColour(m_panelBackColor);

    ctrlTitleHSizer->AddSpacer(12);

    m_pCtrlTitleText = new wxStaticText(m_pControlPanel, wxID_ANY, "N/A");
    m_pCtrlTitleText->SetBackgroundColour(m_panelBackColor);
    m_pCtrlTitleText->SetForegroundColour(m_textDarkColor);
    m_pCtrlTitleText->SetFont(ANKER_FONT_NO_1);
    m_pCtrlTitleText->Fit();
    ctrlTitleHSizer->Add(m_pCtrlTitleText, 0, wxALIGN_CENTER_VERTICAL, 0);


    m_pCtrlHSizer = new wxBoxSizer(wxHORIZONTAL);
    ctrlTitleHSizer->Add(m_pCtrlHSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL, 0);

    wxStaticText* splitText = new wxStaticText(m_pControlPanel, wxID_ANY, "|");
    splitText->SetBackgroundColour(m_panelBackColor);
    splitText->SetForegroundColour(m_textDarkColor);
    splitText->SetFont(ANKER_FONT_NO_2);
    splitText->Fit();
    m_pCtrlHSizer->Add(splitText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 16);

    m_pCtrlStatusText = new wxStaticText(m_pControlPanel, wxID_ANY, "--");
    m_pCtrlStatusText->SetBackgroundColour(m_panelBackColor);
    m_pCtrlStatusText->SetForegroundColour(m_textDarkColor);
    m_pCtrlStatusText->SetFont(ANKER_FONT_NO_1);
    m_pCtrlStatusText->Fit();
    m_pCtrlHSizer->Add(m_pCtrlStatusText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

    //m_pCtrlProgressText = new wxStaticText(m_pControlPanel, wxID_ANY, "--");
    //m_pCtrlProgressText->SetBackgroundColour(m_panelBackColor);
    //m_pCtrlProgressText->SetForegroundColour(m_textDarkColor);
    //m_pCtrlProgressText->SetFont(ANKER_FONT_NO_1);
    //m_pCtrlProgressText->Fit();
    //m_pCtrlHSizer->Add(m_pCtrlProgressText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 9);

    m_pCtrlHSizer->AddStretchSpacer(1);

    m_pStopButton = new wxButton(m_pControlPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pStopButton->SetBackgroundColour(m_panelBackColor);
    m_pStopButton->SetForegroundColour(m_textDarkColor);
    wxBitmap stopBtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("ctrl_stop.png")), wxBITMAP_TYPE_PNG);
    wxImage stopBtnImage = stopBtnBitmap.ConvertToImage();
    stopBtnImage.Rescale(24, 24);
    wxBitmap scaledStopBitmap(stopBtnImage);
    m_pStopButton->SetBitmap(scaledStopBitmap);
    m_pStopButton->SetMinSize(scaledStopBitmap.GetSize());
    m_pStopButton->SetMaxSize(scaledStopBitmap.GetSize());
    m_pStopButton->Bind(wxEVT_BUTTON, &AnkerTaskPanel::OnStopBtn, this);
    m_pStopButton->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
    m_pStopButton->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });
    m_pCtrlHSizer->Add(m_pStopButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);

    m_pStartButton = new wxButton(m_pControlPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pStartButton->SetBackgroundColour(m_panelBackColor);
    m_pStartButton->SetForegroundColour(m_textDarkColor);
    wxBitmap startBtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("ctrl_start.png")), wxBITMAP_TYPE_PNG);
    wxImage startBtnImage = startBtnBitmap.ConvertToImage();
    startBtnImage.Rescale(24, 24);
    wxBitmap scaledStartBitmap(startBtnImage);
    m_pStartButton->SetBitmap(scaledStartBitmap);
    m_pStartButton->SetMinSize(scaledStartBitmap.GetSize());
    m_pStartButton->SetMaxSize(scaledStartBitmap.GetSize());
    m_pStartButton->Bind(wxEVT_BUTTON, &AnkerTaskPanel::OnStartBtn, this);
    m_pStartButton->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
    m_pStartButton->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });
    m_pCtrlHSizer->Add(m_pStartButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    m_pPauseButton = new wxButton(m_pControlPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pPauseButton->SetBackgroundColour(m_panelBackColor);
    m_pPauseButton->SetForegroundColour(m_textDarkColor);
    wxBitmap pauseBtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("ctrl_pause.png")), wxBITMAP_TYPE_PNG);
    wxImage pauseImage = pauseBtnBitmap.ConvertToImage();
    pauseImage.Rescale(24, 24);
    wxBitmap scaledPauseBitmap(pauseImage);
    m_pPauseButton->SetBitmap(scaledPauseBitmap);
    m_pPauseButton->SetMinSize(scaledPauseBitmap.GetSize());
    m_pPauseButton->SetMaxSize(scaledPauseBitmap.GetSize());
    m_pPauseButton->Bind(wxEVT_BUTTON, &AnkerTaskPanel::OnPauseBtn, this);
    m_pPauseButton->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
    m_pPauseButton->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });
    m_pCtrlHSizer->Add(m_pPauseButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    m_pCtrlHSizer->AddSpacer(12);

    m_pControlPanel->Fit();
}

void AnkerTaskPanel::startPrint()
{
    m_gcodeImportResult = m_pGCodeImportDialog->getImportResult();
    std::string errorCode = "0";
    std::string errorMsg = "success to start print";
    std::map<std::string, std::string> buryMap;
    if (m_currentDeviceSn.empty())
        return;

    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
    if (!currentDev)
        return;

    if (m_gcodeImportResult.m_srcType == Slic3r::GUI::FSM_COMPUTER)
    {
        if (currentDev->GetDevicePartsType() == DEVICE_PARTS_NO)
        {
            PrintCheckHint printCheckHint;
            ANKER_LOG_INFO << "start to check need remind for local select gcode print";
            if (printCheckHint.CheckNeedRemind(currentDev, m_gcodeImportResult.m_filePath)) {
                if (!printCheckHint.Hint(currentDev, m_pGCodeImportDialog)) {
                    errorCode = "1";
                    errorMsg = "user cancel start to print";
                    buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                    buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                    ANKER_LOG_INFO << "Report bury event is " << e_start_print;
                    reportBuryEvent(e_start_print, buryMap);
                    return;
                }
            }
        }

        auto ankerNet = AnkerNetInst();
        if (ankerNet) {            
            ankerNet->StartP2pOperator(P2P_TRANSFER_LOCAL_FILE, m_currentDeviceSn, m_gcodeImportResult.m_filePath);
        }
    }
    else
    {
        if (currentDev->GetDevicePartsType() == DEVICE_PARTS_NO)
        {
            PrintCheckHint printCheckHint;
            ANKER_LOG_INFO << "start to check need remind for remote print";
            if (printCheckHint.CheckNeedRemind(currentDev)) {
                if (!printCheckHint.Hint(currentDev, 
                    m_pGCodeImportDialog,
                    wxDefaultSize, 
                    wxDefaultPosition,
                    true)) {
                    errorCode = "1";
                    errorMsg = "user cancel start to print";
                    buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                    buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                    ANKER_LOG_INFO << "Report bury event is " << e_start_print;
                    reportBuryEvent(e_start_print, buryMap);
                    return;
                }
            }
            ANKER_LOG_INFO << "start print device file";
            //start print for non-V6 device
            VrCardInfoMap vrCardInfoMap;
            currentDev->SetRemotePrintData(vrCardInfoMap, m_gcodeImportResult.m_filePath);
        }
    }

    wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_STARTED);
    //wxPostEvent(this->GetParent(), evt);
    ProcessEvent(evt);

    setLoadingVisible(true);
    
    buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
    buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
    ANKER_LOG_INFO << "Report bury event is " << e_start_print;
    reportBuryEvent(e_start_print, buryMap);
    m_toPrinting = true;
    m_pToPrintingTimer->Stop();
    m_pToPrintingTimer->StartOnce(30000);
}

void AnkerTaskPanel::setLoadingVisible(bool visible)
{
    // loading frame
    if (m_pLoadingMask == nullptr)
    {
        m_pLoadingMask = new AnkerLoadingMask(this, 30000);
        m_pLoadingMask->setText("");
        m_pLoadingMask->Bind(wxANKEREVT_LOADING_TIMEOUT, &AnkerTaskPanel::OnLoadingTimeout, this);
        m_pLoadingMask->Bind(wxANKEREVT_LOADMASK_RECTUPDATE, &AnkerTaskPanel::OnLoadMaskRectUpdate, this);
    }

    int x, y;
    GetScreenPosition(&x, &y);
    wxSize clientSize = GetClientSize();

    m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
    m_pLoadingMask->Show(visible);
    if (visible)
        m_pLoadingMask->start();
    else
        m_pLoadingMask->stop();
}

void AnkerTaskPanel::setInfoPanelEnable(bool enable)
{
    m_pTimeLeftValueText->SetForegroundColour(enable ? wxColour(ANKER_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pTimeLeftLabelText->SetForegroundColour(enable ? wxColour(ANKER_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pTaskProgressCtrl->setProgressColor(enable ? wxColour(ANKER_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pTaskProgressCtrl->setUnProgressColor(enable ? wxColour(ANKER_RGB_INT).ChangeLightness(50) : wxColour(CONTROL_DISABLE_COLOR).ChangeLightness(50));
    m_pSpeedInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pFilamentInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pLayerInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
    m_pFinishTimeInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_RGB_INT) : wxColour(CONTROL_DISABLE_COLOR));
}

void AnkerTaskPanel::OnSize(wxSizeEvent& event)
{
    Layout();
    Refresh();
}

void AnkerTaskPanel::OnNewBtn(wxCommandEvent& event)
{
    DeviceObjectBasePtr deviceObj = CurDevObject(m_currentDeviceSn);
    if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
        return;
    }

    ANKER_LOG_INFO << "start printing button click";
    if (deviceObj) {
        deviceObj->SetLastFilament();
        deviceObj->AsyQueryAllInfo();
    }

    m_pGCodeImportDialog->switch2FileSelect(Slic3r::GUI::FSM_COMPUTER);

    m_modaling = true;

    int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
    int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
    m_pGCodeImportDialog->SetPosition(wxPoint((screenW - m_pGCodeImportDialog->GetSize().x) / 2, (screenH - m_pGCodeImportDialog->GetSize().y) / 2));

    if (m_pGCodeImportDialog->ShowModal() == wxOK)
        startPrint();

    m_modaling = false;

    return;
}

void AnkerTaskPanel::OnStartBtn(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "start button";

    if (m_currentDeviceSn.empty())
        return;

    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
    if (!currentDev)
        return;

    if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PAUSED)
        currentDev->setDevicePrintResume();   

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        if (m_currentDeviceStatus == GUI_DEVICE_STATUS_TYPE_PAUSED)
        {
            wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_CONTINUE);
            ProcessEvent(evt);
        }
    }

    //Layout();
}

void AnkerTaskPanel::OnStopBtn(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "stop button";

    std::string levelReminder = /*"Are you sure you want to quit?"*/_("common_print_popup_printstop").utf8_str().data();
    std::string title = /*"Stopping"*/_("common_print_controlbutton_stop").utf8_str().data();
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, true);
    if (result != AnkerMsgDialog::MSG_OK)
        return;

    setLoadingVisible(true);

    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
    if (!currentDev)
        return;

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        currentDev->setDevicePrintStop();

        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_STOPPED);
        ProcessEvent(evt);
    }
    else if (m_currentMode == TASK_MODE_LEVEL)
    {
        currentDev->setLevelStop();

        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_LEVELING_STOPPED);
        ProcessEvent(evt);
    }
    else if(m_currentMode == TASK_MODE_CALIBRATE)
    {
        currentDev->setCalibrationStop();
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_CALIBRATING_STOPPED);
        ProcessEvent(evt);
    }
}

void AnkerTaskPanel::OnPauseBtn(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "pause button";

    std::string levelReminder = /*"Are you sure you want to pause the task?"*/_("common_print_popup_printpause").ToStdString(wxConvUTF8);
    std::string title = /*"Pausing"*/_("common_print_taskbar_statuspaused").ToStdString(wxConvUTF8);
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, true, _("common_button_yes").ToStdString(wxConvUTF8), _("common_button_no").ToStdString(wxConvUTF8));
    if (result != AnkerMsgDialog::MSG_OK)
        return;

	if (m_currentDeviceSn.empty())
		return;
    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);

    if (!currentDev)
        return;

    currentDev->setDevicePrintPause();

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_PAUSED);
        //wxPostEvent(this->GetParent(), evt);
        ProcessEvent(evt);
    }

    //m_pStartButton->Show(true);
    //m_pStopButton->Show(true);
    //m_pPauseButton->Show(false);

    //Layout();
}

void AnkerTaskPanel::OnCountdownTimer(wxTimerEvent& event)
{
    ANKER_LOG_INFO << "m_pLocalCounter timer Enter1";
    // update time left
    if (m_countDownSeconds > 0 && (m_lastDeviceStatus == GUI_DEVICE_STATUS_TYPE_PRINTING || m_lastDeviceStatus == GUI_DEVICE_STATUS_TYPE_LEVELING))
    {
        m_countDownSeconds--;

        int remainingSeconds = m_countDownSeconds % 60;
        int remainingMinutes = m_countDownSeconds / 60 % 60;
        int remainingHours = m_countDownSeconds / 60 / 60;
        int remainingDays = m_countDownSeconds / 60 / 60 / 24;
        // update printing progress
        std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + ":" + (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + ":" + (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        m_pTimeLeftValueText->SetLabelText(timeLeftStr);
        m_pTimeLeftValueText->Fit();
    }
    ANKER_LOG_INFO << "m_pLocalCounter timer Enter2";

    if (m_currentMode != TASK_MODE_LEVEL) {
        // update progress
        double curProgress = m_pTaskProgressCtrl->getCurrentProgress();
        m_pTaskProgressCtrl->updateProgress(curProgress + 1);
    }
    ANKER_LOG_INFO << "m_pLocalCounter timer Leave";
}

void AnkerTaskPanel::OnLoadingTimeout(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "loading time out";

    setLoadingVisible(false);

    std::string levelReminder = /*"Failed to connect to the machine"*/_("common_popup_content_datadisconnect").ToStdString(wxConvUTF8);
    std::string title = /*"Error"*/_("common_popup_titlenotice").ToStdString(wxConvUTF8);
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, false);
}

void AnkerTaskPanel::OnLoadMaskRectUpdate(wxCommandEvent& event)
{
    if (m_pLoadingMask && m_pLoadingMask->IsShown())
    {
        int x, y;
        GetScreenPosition(&x, &y);
        wxSize clientSize = GetClientSize();

        m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
    }
}

void AnkerTaskPanel::updateFileTransferStatus(std::string currentSn, int progress, FileTransferResult result)
{
    ANKER_LOG_DEBUG << "currentSn: " + currentSn + ", progress: " + std::to_string(progress);

    wxWindowUpdateLocker updateLocker(this);
    if (result != FileTransferResult::Transfering &&
        result != FileTransferResult::Succeed) {
        ANKER_LOG_ERROR << "TTGCODE003 transfer failed, " << progress;        

		DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
        std::string levelReminder = _("common_print_transfer_errornotice").ToStdString(wxConvUTF8);
        std::string title = _("common_popup_titlenotice").ToStdString(wxConvUTF8);                
        levelReminder = levelReminder + " " + std::to_string((int)result);

        switchMode(TASK_MODE_NONE, true);
        if (result == FileTransferResult::NeedLevel) {
            // mind level
            PrintCheckHint printCheckHint;
            ANKER_LOG_INFO << "start to check need remind for p2p file transfer";
            printCheckHint.Hint(currentDev, PrintCheckHint::HintType::NEED_LEVEL, this);
        }
        else {
            AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, false);
            if (AnkerMsgDialog::MSG_OK == result) {                                
                if (currentDev) {
                    currentDev->clearDeviceExceptionInfo();
                }
            }
        }              
        return;
    }
    else if (result == FileTransferResult::Succeed) {
        return;
    }

    switchMode(TASK_MODE_TRANFER);

    // update printing progress
    std::string timeLeftStr = "00:00:00";
    m_pTimeLeftValueText->SetLabelText(timeLeftStr);
    m_pTimeLeftValueText->Fit();

    m_pCtrlStatusText->SetLabelText(/*L"Transfering"*/
        wxString::Format(_("common_print_taskbar_statustransfere"), 
        std::to_string(progress) + "%"));
    if (!m_toPrinting) {
            m_gcodeImportResult.m_srcType = Slic3r::GUI::FSM_SLICE;

            m_toPrinting = true;
            m_pToPrintingTimer->Stop();
            m_pToPrintingTimer->StartOnce(30000);
        }

    DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
    if (currentDev) {
        m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(currentDev->GetFileName()));
    }    
    m_pCtrlTitleText->Fit();
    Layout();
    m_pCtrlTitleText->Refresh();
}

void AnkerTaskPanel::activate(bool active)
{
    if (m_pLoadingMask && m_pLoadingMask->isLoading())
    {
        m_pLoadingMask->Show(active);
    }
}

void AnkerTaskPanel::setOfflineStatus()
{
	m_newTaskButton->Enable(false);
	m_newTaskButton->SetBackgroundColour(wxColor("#2E2F32"));
	m_newTaskButton->SetTextColor(wxColor("#69696C"));

    m_pCtrlTitleText->SetLabelText("--");    
    m_titleEmptyBanner->SetLabelText("--");

	m_pCtrlTitleText->SetForegroundColour(wxColor(169,170,171));
    m_titleEmptyBanner->SetForegroundColour(wxColor(169,170,171));	
}

void AnkerTaskPanel::LevelHint(DeviceObjectBasePtr currentDev)
{
    if (!currentDev) {
        return;
    }

    int levelTime = currentDev->GetTime() * 1.0 / 60;
    std::string levelReminder = wxString::Format(_("common_print_popup_levelnotice"), "10").ToStdString();
    if (currentDev->IsMultiColorDevice())
        levelReminder = wxString::Format(_("common_print_popup_levelnotice"), "15").ToStdString(wxConvUTF8);
    std::string title = _("common_print_popupfinished_noticefailed").ToStdString(wxConvUTF8);
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title);
    if (result == AnkerMsgDialog::MsgResult::MSG_OK) {
        currentDev->setLevelBegin();
    } else {
        currentDev->clearDeviceCtrlResult();
    }
}

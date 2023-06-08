#include "AnkerTaskPanel.hpp"
#include "AnkerBtn.hpp"
#include "AnkerMsgDialog.hpp"
#include "AnkerLoadingMask.hpp"
#include "GUI.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/DataManger.hpp"
#include "slic3r/GUI/Network/basetype.hpp"

#include "wx/dc.h"
#include "wx/artprov.h"
#include "wx/event.h"
#include <wx/clntdata.h>

#include <ctime>

#define PANEL_BACK_COLOR 41, 42, 45
#define TEXT_LIGHT_COLOR 255, 255, 255
#define TEXT_DARK_COLOR 183, 183, 183
#define CONTROL_DISABLE_COLOR 125, 125, 125
#define SYSTEM_COLOR "#62D361"


wxDEFINE_EVENT(wxANKEREVT_LEVELING_STOPPED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_STARTED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_PAUSED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_CONTINUE, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_PRINTING_STOPPED, wxCommandEvent);

AnkerTaskPanel::AnkerTaskPanel(std::string currentSn, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(760, 237))
    , m_printCompleteCheckFlag(false)
    , m_currentDeviceSn(currentSn)
    , m_lastDeviceStatus(MQTT_PRINT_EVENT_MAX)
    , m_currentMode(TASK_MODE_NONE)
    , m_pInfoEmptyPanel(nullptr)
    , m_pInfoPanel(nullptr)
    , m_pCtrlSizer(nullptr)
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
    , m_countDownSeconds(INT_MAX)
{
    m_panelBackColor = wxColour(PANEL_BACK_COLOR);
    m_textLightColor = wxColour(TEXT_LIGHT_COLOR);
    m_textDarkColor = wxColour(TEXT_DARK_COLOR);
    m_systemColor = wxColour(SYSTEM_COLOR);

    initUI();

    m_pGCodeImportDialog = new AnkerGCodeImportDialog(m_currentDeviceSn, nullptr);
    m_pGCodeImportDialog->Hide();

    Bind(wxEVT_SIZE, &AnkerTaskPanel::OnSize, this);

    m_pLocalCounter = new wxTimer();
    m_pLocalCounter->Bind(wxEVT_TIMER, &AnkerTaskPanel::OnTimer, this);
}

AnkerTaskPanel::~AnkerTaskPanel()
{
}

void AnkerTaskPanel::switchMode(TaskMode mode)
{
    if (m_currentMode != mode)
    {
        switch (mode)
        {
        case AnkerTaskPanel::TASK_MODE_NONE:
            m_pInfoEmptyPanel->Show(true);
            m_pInfoPanel->Show(false);
            m_pCtrlSizer->Show(false);

            m_pCtrlTitleText->SetLabelText("N/A");
            break;
        case AnkerTaskPanel::TASK_MODE_PRINT:
        {  
			m_pInfoEmptyPanel->Show(false);

			m_pInfoPanel->Show(true);
			m_pSpeedInfoPanel->Show(true);
			m_pFilamentInfoPanel->Show(true);
			m_pLayerInfoPanel->Show(true);
			m_pFinishTimeInfoPanel->Show(true);

			m_pCtrlSizer->Show(true);
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
            image.Rescale(123, 123);
            wxBitmap scaledBitmap(image);
            m_pPreviewImage->SetBitmap(scaledBitmap);
		}
		break;
        case AnkerTaskPanel::TASK_MODE_LEVEL:
        {
            m_pInfoEmptyPanel->Show(false);

            m_pInfoPanel->Show(true);
            m_pSpeedInfoPanel->Show(false);
            m_pFilamentInfoPanel->Show(false);
            m_pLayerInfoPanel->Show(true);
            m_pFinishTimeInfoPanel->Show(false);

            m_pCtrlSizer->Show(true);
            m_pStartButton->Show(false);
            m_pStopButton->Show(true);
            m_pPauseButton->Show(false);

            m_pTaskProgressCtrl->updateProgress(0);

            wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("leveling_preview_sample.png")), wxBITMAP_TYPE_PNG);
            wxImage image = bitmapEx.ConvertToImage();
            image.Rescale(123, 123);
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

            m_pCtrlSizer->Show(true);
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
            image.Rescale(123, 123);
            wxBitmap scaledBitmap(image);
            m_pPreviewImage->SetBitmap(scaledBitmap);

            // update printer status
            m_pCtrlStatusText->SetLabelText("Transfering");

            // update printing gcode name
            m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(m_gcodeImportResult.m_fileName));
            m_pCtrlTitleText->Fit();

            // update printing gcode info
            std::string speedStr = m_gcodeImportResult.m_speedStr;
            m_pSpeedInfoText->SetLabelText(speedStr);

            std::string filamentStr = m_gcodeImportResult.m_filamentStr;
            m_pFilamentInfoText->SetLabelText(filamentStr);

            std::string layerStr = std::to_string(0) + "/" + std::to_string(0);
            m_pLayerInfoText->SetLabelText(layerStr);

            time_t t = time(nullptr);
            struct tm* now = localtime(&t);
            int remainingSeconds = m_gcodeImportResult.m_timeSecond % 60;
            int remainingMinutes = m_gcodeImportResult.m_timeSecond / 60 % 60;
            int remainingHours = m_gcodeImportResult.m_timeSecond / 60 / 60 % 60;
            int remainingDays = m_gcodeImportResult.m_timeSecond / 60 / 60 / 60 % 24;
            int incre = 0;
            int finishSeconds = now->tm_sec + remainingSeconds;
            finishSeconds = finishSeconds >= 60 ? (incre = 1, finishSeconds - 60) : (incre = 0, finishSeconds);
            int  finishMinutes = now->tm_min + remainingMinutes + incre;
            finishMinutes = finishMinutes >= 60 ? (incre = 1, finishMinutes - 60) : (incre = 0, finishMinutes);
            int finishHours = now->tm_hour + remainingHours + incre;
            finishHours = finishHours >= 24 ? (incre = 1, finishHours - 24) : (incre = 0, finishHours);
            int finishDays = now->tm_wday + remainingDays + incre;
            std::string timeStr = (finishHours < 10 ? "0" : "") + std::to_string(finishHours) + ":" + (finishMinutes < 10 ? "0" : "") + std::to_string(finishMinutes);
            m_pFinishTimeInfoText->SetLabelText(timeStr);
        }
        break;
        default:
            break;
        }

        m_currentMode = mode;

        //setLoadingVisible(false);

        Layout();
    }

    setLoadingVisible(false);
}

void AnkerTaskPanel::updateStatus(std::string currentSn)
{
    std::cout << "currentSn = " << currentSn << " isShown = " << IsShown() << " isShownOnSreen = " << IsShownOnScreen() << std::endl;

    m_currentDeviceSn = currentSn;
    Datamanger& dm = Datamanger::GetInstance();
    DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

    if (currentDev == nullptr)
        return;

    if (m_pGCodeImportDialog)
        m_pGCodeImportDialog->setCurrentDeviceSn(m_currentDeviceSn);

    std::cout << "currentDev->deviceStatus = " << currentDev->deviceStatus << std::endl;
    std::cout << "currentDev->exceptionStatus = " << currentDev->exceptionStatus << std::endl;

    if (currentDev->deviceStatus == MQTT_PRINT_EVENT_PRINTING || currentDev->deviceStatus == MQTT_PRINT_EVENT_PRINT_HEATING ||
        /*currentDev->deviceStatus == MQTT_PRINT_EVENT_COMPLETED ||*/ currentDev->deviceStatus == MQTT_PRINT_EVENT_PAUSED)
        switchMode(TASK_MODE_PRINT);
    else if ((currentDev->deviceStatus == MQTT_PRINT_EVENT_LEVELING && currentDev->generalException != GeneralException2Gui::GeneralException2Gui_Auto_Level_Error)
        || currentDev->deviceStatus == MQTT_PRINT_EVENT_HEATING)
        switchMode(TASK_MODE_LEVEL);
    else if (currentDev->getCustomDeviceStatus() == CustomDeviceStatus::CustomDeviceStatus_File_Transfer)
        switchMode(TASK_MODE_TRANFER);
    else if (currentDev->deviceStatus == MQTT_PRINT_EVENT_IDLE || currentDev->deviceStatus == MQTT_PRINT_EVENT_COMPLETED 
        || (currentDev->deviceStatus == MQTT_PRINT_EVENT_LEVELING && currentDev->generalException == GeneralException2Gui::GeneralException2Gui_Auto_Level_Error))
        switchMode(TASK_MODE_NONE);
    
    if (m_pGCodeImportDialog && !m_printCompleteCheckFlag 
        && (currentDev->deviceStatus == MQTT_PRINT_EVENT_COMPLETED 
            || checkIsExceptionFinished(currentDev->deviceStatus)))
    {
        m_printCompleteCheckFlag = true;

        if (checkIsExceptionFinished(currentDev->deviceStatus))
        {
            int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
            int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
            m_pGCodeImportDialog->SetPosition(wxPoint((screenW - m_pGCodeImportDialog->GetSize().x) / 2, (screenH - m_pGCodeImportDialog->GetSize().y) / 2));

            AnkerGCodeImportDialog::GCodeImportResult result = m_pGCodeImportDialog->getImportResult();


            MqttType::PrintingNoticeDataPtr dataPtr = currentDev->getPrintingNotice();
            if (dataPtr) {
                if (!dataPtr->name.empty())
                    result.m_fileName = dataPtr->name;
                result.m_filamentStr = std::to_string(dataPtr->filamentUsed) + dataPtr->filamentUnit;
                //result.m_speedStr = std::to_string(dataPtr->spe) + "mm/s";
                result.m_timeSecond = dataPtr->saveTime;
            }
            
            //result.m_filePath = currentDev->filp;
            m_pGCodeImportDialog->switch2PrintFinished(false, result);
            if (m_pGCodeImportDialog->ShowModal() == wxOK)
            {
                switchMode(TASK_MODE_PRINT);
                currentDev->setDevicePrintAgain("");
            }
            else
            {
                switchMode(TASK_MODE_NONE);
            }
        }
        else if (currentDev->deviceStatus == MQTT_PRINT_EVENT_COMPLETED)
        {
            int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
            int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
            m_pGCodeImportDialog->SetPosition(wxPoint((screenW - m_pGCodeImportDialog->GetSize().x) / 2, (screenH - m_pGCodeImportDialog->GetSize().y) / 2));

            AnkerGCodeImportDialog::GCodeImportResult result = m_pGCodeImportDialog->getImportResult();


            MqttType::PrintingNoticeDataPtr dataPtr = currentDev->getPrintingNotice();
            if (dataPtr) {
                if (!dataPtr->name.empty())
                    result.m_fileName = dataPtr->name;
                result.m_filamentStr = std::to_string(dataPtr->filamentUsed) + dataPtr->filamentUnit;
                //result.m_speedStr = std::to_string(dataPtr->spe) + "mm/s";
                result.m_timeSecond = dataPtr->saveTime;
            }
            
            //result.m_filePath = currentDev->filp;
            m_pGCodeImportDialog->switch2PrintFinished(true, result);
            if (m_pGCodeImportDialog->ShowModal() == wxOK)
            {
                switchMode(TASK_MODE_PRINT);
                currentDev->setDevicePrintAgain("");
            }
            else
            {
                switchMode(TASK_MODE_NONE);
            }
        }
    }
    else if (currentDev->deviceStatus != MQTT_PRINT_EVENT_COMPLETED)
    {
        m_printCompleteCheckFlag = false;
    }

    m_pLocalCounter->Stop();

    if (m_currentMode == TASK_MODE_PRINT)
    {
        // update printer status
        switch (currentDev->deviceStatus)
        {
        case(MQTT_PRINT_EVENT_PRINT_HEATING):
            m_pCtrlStatusText->SetLabelText("Print Heating");
            m_pStopButton->Show(true);
            m_pStartButton->Show(false);
            m_pPauseButton->Show(false);

            m_pLocalCounter->Start(1000);
            break;
        case(MQTT_PRINT_EVENT_DOWNLOADING):
            m_pCtrlStatusText->SetLabelText("Downloading");
            m_countDownSeconds = -1;
            break;
        case(MQTT_PRINT_EVENT_PRINTING):
            m_pCtrlStatusText->SetLabelText("Printing");
            m_pStopButton->Show(true);
            m_pStartButton->Show(false);
            m_pPauseButton->Show(true);

            setInfoPanelEnable(true);

            if (m_countDownSeconds < 0)
                m_countDownSeconds = currentDev->time;
            else
                m_countDownSeconds = currentDev->time >= m_countDownSeconds ? m_countDownSeconds : currentDev->time;
            m_pLocalCounter->Start(1000);
            break;
        case(MQTT_PRINT_EVENT_PAUSED):
            m_pCtrlStatusText->SetLabelText("Paused");
            m_pStopButton->Show(true);
            m_pStartButton->Show(true);
            m_pPauseButton->Show(false);
            setInfoPanelEnable(false);
            m_countDownSeconds = -1;
            break;
        case(MQTT_PRINT_EVENT_STOPPED):
            m_pCtrlStatusText->SetLabelText("Stopped");
            m_pStopButton->Show(false);
            m_pStartButton->Show(true);
            m_pPauseButton->Show(false);
            setInfoPanelEnable(true);
            m_countDownSeconds = -1;
            break;
        case(MQTT_PRINT_EVENT_COMPLETED):
            m_pCtrlStatusText->SetLabelText("Complete");
            m_pStopButton->Show(false);
            m_pStartButton->Show(true);
            m_pPauseButton->Show(false);
            setInfoPanelEnable(true);
            m_countDownSeconds = -1;
            break;
        default:
            m_pCtrlStatusText->SetLabelText("--");
            break;
        }

        // update printing gcode name
        m_pCtrlTitleText->SetLabelText(wxString::FromUTF8(currentDev->printFile));
        m_pCtrlTitleText->Fit();

        // update printing gcode info
        std::string speedStr = std::to_string(currentDev->speed) + "mm/s";
        m_pSpeedInfoText->SetLabelText(speedStr);

        std::string filamentStr = std::to_string(currentDev->filamentUsed) + currentDev->filamentUnit;
        m_pFilamentInfoText->SetLabelText(filamentStr);

        std::string layerStr = std::to_string(currentDev->currentLayer) + "/" + std::to_string(currentDev->totalLayer);
        m_pLayerInfoText->SetLabelText(layerStr);
        
        time_t t = time(nullptr);
        struct tm* now = localtime(&t);
        int remainingSeconds = currentDev->time % 60;
        int remainingMinutes = currentDev->time / 60 % 60;
        int remainingHours = currentDev->time / 60 / 60 % 60;
        int remainingDays = currentDev->time / 60 / 60 / 60 % 24;
        int incre = 0;
        int finishSeconds = now->tm_sec + remainingSeconds;
        finishSeconds = finishSeconds >= 60 ? (incre = 1, finishSeconds - 60) : (incre = 0, finishSeconds);
        int  finishMinutes = now->tm_min + remainingMinutes + incre;
        finishMinutes = finishMinutes >= 60 ? (incre = 1, finishMinutes - 60) : (incre = 0, finishMinutes);
        int finishHours = now->tm_hour + remainingHours + incre;
        finishHours = finishHours >= 24 ? (incre = 1, finishHours - 24) : (incre = 0, finishHours);
        int finishDays = now->tm_wday + remainingDays + incre;
        std::string suffix;
        if (finishHours < 12 || (finishHours == 12 && finishMinutes > 0))
            suffix = "am";
        else
        {
            finishHours = finishHours > 12 ? finishHours - 12 : finishHours;
            suffix = "pm";
        }
        std::string timeStr = (finishHours < 10 ? "0" : "") + std::to_string(finishHours) + ":" + (finishMinutes < 10 ? "0" : "") + std::to_string(finishMinutes) + " " + suffix;
        m_pFinishTimeInfoText->SetLabelText(timeStr);

        // update printing progress
        std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + " : " + (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + " : " + (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        m_pTimeLeftValueText->SetLabelText(timeLeftStr);

        //if (currentDev->deviceStatus == MQTT_PRINT_EVENT_PRINT_HEATING)
        //{
        //    m_pTaskProgressCtrl->setProgressRange(currentDev->nozzle[1] + currentDev->hotdBed[1]);
        //    m_pTaskProgressCtrl->updateProgress(currentDev->nozzle[0] + currentDev->hotdBed[0]);
        //}
        //else
        {
            m_pTaskProgressCtrl->setProgressRange(10000);
            m_pTaskProgressCtrl->updateProgress(currentDev->progress);
        }
    }
    else if (m_currentMode == TASK_MODE_LEVEL)
    {
        // update printer status
        switch (currentDev->deviceStatus)
        {
        case(MQTT_PRINT_EVENT_HEATING):
            m_pCtrlStatusText->SetLabelText("Heating");

            m_pLocalCounter->Start(1000);
            m_countDownSeconds = -1;
            break;
        case(MQTT_PRINT_EVENT_LEVELING):
            m_pCtrlStatusText->SetLabelText("Leveling");

            if (m_countDownSeconds < 0)
                m_countDownSeconds = currentDev->time;
            else
                m_countDownSeconds = currentDev->time >= m_countDownSeconds ? m_countDownSeconds : currentDev->time;
            m_pLocalCounter->Start(1000);
            break;
        default:
            m_pCtrlStatusText->SetLabelText("--");
            m_countDownSeconds = -1;
            break;
        }

        // update level title
        m_pCtrlTitleText->SetLabelText("Auto Level");

        // update preview image

        // update level info
        std::string layerStr = std::to_string(currentDev->autoLevelingValue) + "/" + std::to_string(currentDev->autoLevelingTotalValue);
        m_pLayerInfoText->SetLabelText(layerStr);

        // update leveling progress
        int remainingSeconds = currentDev->time % 60;
        int remainingMinutes = currentDev->time / 60 % 60;
        int remainingHours = currentDev->time / 60 / 60 % 60;
        int remainingDays = currentDev->time / 60 / 60 / 60 % 24;
        std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + " : " + (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + " : " + (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        m_pTimeLeftValueText->SetLabelText(timeLeftStr);

        //if (currentDev->deviceStatus == MQTT_PRINT_EVENT_HEATING)
        //{
        //    m_pTaskProgressCtrl->setProgressRange(currentDev->nozzle[1] + currentDev->hotdBed[1]);
        //    m_pTaskProgressCtrl->updateProgress(currentDev->nozzle[0] + currentDev->hotdBed[0]);
        //}
        //else
        {
            m_pTaskProgressCtrl->setProgressRange(currentDev->autoLevelingTotalValue);
            m_pTaskProgressCtrl->updateProgress(currentDev->autoLevelingValue);
        }
    }

    Layout();
    Refresh();

    m_lastDeviceStatus = currentDev->deviceStatus;
}

void AnkerTaskPanel::requestCallback()
{
    if (m_pGCodeImportDialog)
        m_pGCodeImportDialog->requestCallback();
}

void AnkerTaskPanel::initUI()
{
    wxBoxSizer* taskBarSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(taskBarSizer);
    SetBackgroundColour(m_panelBackColor);

	// title
	initTitlePanel(this);
	taskBarSizer->Add(m_pTitlePanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 7);

    // split line
    wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
    splitLineCtrl->SetMaxSize(wxSize(100000, 1));
    splitLineCtrl->SetMinSize(wxSize(1, 1));
    taskBarSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER, 0);

    // empty panel
    initInfoEmptyPanel(this);
    wxSizerFlags flags;
    flags.Proportion(128);
    flags.Border(wxLEFT, 12);
    flags.Border(wxTOP, 16);
    flags.Border(wxBOTTOM, 16);
    flags.Expand();
    taskBarSizer->Add(m_pInfoEmptyPanel, flags);

    // task panel
    initInfoPanel(this);
    taskBarSizer->Add(m_pInfoPanel, 1, wxEXPAND | wxTOP | wxBOTTOM | wxLEFT, 15);

    m_pInfoEmptyPanel->Show(true);
    m_pInfoPanel->Show(false);

    // split line
	splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
	splitLineCtrl->SetMaxSize(wxSize(100000, 1));
	splitLineCtrl->SetMinSize(wxSize(1, 1));
	taskBarSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER, 0);

    // control panel
    initControlPanel(this);
    taskBarSizer->Add(m_pControlPanel, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 3);
    m_pCtrlSizer->Show(false);
}

void AnkerTaskPanel::initTitlePanel(wxWindow* parent)
{
    m_pTitlePanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pTitlePanel->SetSizer(titleHSizer);
    m_pTitlePanel->SetBackgroundColour(m_backgroundColour);

    titleHSizer->AddSpacer(12);

    wxStaticText* titleBanner = new wxStaticText(m_pTitlePanel, wxID_ANY, "Status");
    titleBanner->SetBackgroundColour(m_panelBackColor);
    titleBanner->SetForegroundColour(m_textLightColor);
    titleBanner->SetCanFocus(false);

    wxFont font = titleBanner->GetFont();
    font.SetPointSize(11);
    titleBanner->SetFont(font.Bold());

    titleHSizer->Add(titleBanner, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 0);

    titleHSizer->AddStretchSpacer(1);
}

void AnkerTaskPanel::initInfoEmptyPanel(wxWindow* parent)
{
    m_pInfoEmptyPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* emptyVSizer = new wxBoxSizer(wxVERTICAL);
    m_pInfoEmptyPanel->SetSizer(emptyVSizer);
    m_pInfoEmptyPanel->SetBackgroundColour(m_backgroundColour);

    wxStaticText* titleEmptyBanner = new wxStaticText(m_pInfoEmptyPanel, wxID_ANY, "N/A");
    //titleEmptyBanner->SetMaxSize(wxSize(760, 20));
    titleEmptyBanner->SetBackgroundColour(m_panelBackColor);
    titleEmptyBanner->SetForegroundColour(m_textDarkColor);
    wxFont font = titleEmptyBanner->GetFont();
    font.SetPointSize(9);
    titleEmptyBanner->SetFont(font);
    emptyVSizer->Add(titleEmptyBanner, 0, wxALIGN_LEFT | wxLEFT, 12);

    wxBoxSizer* containerSizer = new wxBoxSizer(wxVERTICAL);
    emptyVSizer->Add(containerSizer, 1, wxEXPAND | wxLEFT, 12);

    containerSizer->AddStretchSpacer(1);

    AnkerBtn* newTaskButton = new AnkerBtn(m_pInfoEmptyPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    newTaskButton->SetMinSize(wxSize(252, 24));
    newTaskButton->SetMaxSize(wxSize(252, 24));
    newTaskButton->SetText(L"Start Printing");
    newTaskButton->SetBackgroundColour(m_systemColor);
    newTaskButton->SetRadius(5);
    newTaskButton->SetTextColor(m_textLightColor);
    newTaskButton->Bind(wxEVT_BUTTON, &AnkerTaskPanel::OnNewBtn, this);
    containerSizer->Add(newTaskButton, 0, wxALIGN_CENTER_HORIZONTAL, 0);

    containerSizer->AddStretchSpacer(1);

    m_pInfoEmptyPanel->Show(true);
}

void AnkerTaskPanel::initInfoPanel(wxWindow* parent)
{
    m_pInfoPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* taskHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pInfoPanel->SetSizer(taskHSizer);
    m_pInfoPanel->SetMinSize(wxSize(760, 170));
    m_pInfoPanel->SetSize(wxSize(760, 170));
    m_pInfoPanel->SetBackgroundColour(m_backgroundColour);

    // preview image
    wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
    wxImage image = bitmapEx.ConvertToImage();
    image.Rescale(115, 115);
    wxBitmap scaledBitmap(image);
    m_pPreviewImage = new wxStaticBitmap(m_pInfoPanel, wxID_ANY, scaledBitmap);
    //m_pPreviewImage->SetMinSize(scaledBitmap.GetSize());
    //m_pPreviewImage->SetMaxSize(scaledBitmap.GetSize());
    m_pPreviewImage->SetBackgroundColour(m_backgroundColour);
    taskHSizer->Add(m_pPreviewImage, 123, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 0);

    // task detail
    wxBoxSizer* taskDetailSizer = new wxBoxSizer(wxVERTICAL);
    taskHSizer->Add(taskDetailSizer, 621, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxLEFT | wxRIGHT, 16);

    taskDetailSizer->AddStretchSpacer(1);

    // remaining time
    m_pTimeLeftValueText = new wxStaticText(m_pInfoPanel, wxID_ANY, "00:00:00");
    m_pTimeLeftValueText->SetMinSize(wxSize(144, 67));
    m_pTimeLeftValueText->SetBackgroundColour(m_panelBackColor);
    m_pTimeLeftValueText->SetForegroundColour(m_systemColor);
    wxFont font = m_pTimeLeftValueText->GetFont();
    font.SetPointSize(30);
    m_pTimeLeftValueText->SetFont(font.Bold());
    taskDetailSizer->Add(m_pTimeLeftValueText, 1, wxEXPAND | wxALIGN_BOTTOM | wxALIGN_LEFT, 0);

    // remaining label
    m_pTimeLeftLabelText = new wxStaticText(m_pInfoPanel, wxID_ANY, "Time Left");
    m_pTimeLeftLabelText->SetMinSize(wxSize(56, 36));
    m_pTimeLeftLabelText->SetBackgroundColour(m_panelBackColor);
    m_pTimeLeftLabelText->SetForegroundColour(m_systemColor);
    font = m_pTimeLeftLabelText->GetFont();
    font.SetPointSize(10);
    m_pTimeLeftLabelText->SetFont(font);
    taskDetailSizer->Add(m_pTimeLeftLabelText, 1, wxEXPAND | wxALIGN_TOP | wxALIGN_LEFT, 0);

    // progress
    m_pTaskProgressCtrl = new AnkerProgressCtrl(m_pInfoPanel);
    m_pTaskProgressCtrl->SetMinSize(wxSize(580, 26));
    m_pTaskProgressCtrl->setLineWidth(5);
    m_pTaskProgressCtrl->setProgressColor(m_systemColor);
    m_pTaskProgressCtrl->updateProgress(0);
    font = m_pTaskProgressCtrl->GetFont();
    font.SetPointSize(9);
    m_pTaskProgressCtrl->setLabelFont(font);
    taskDetailSizer->Add(m_pTaskProgressCtrl, 0, wxEXPAND | wxALIGN_LEFT, 0);

    // printing detail
    wxBoxSizer* taskInfoSizer = new wxBoxSizer(wxHORIZONTAL);
    taskDetailSizer->Add(taskInfoSizer, 0, wxEXPAND | wxALIGN_LEFT | wxTOP, 12);

    // speed
    {
        m_pSpeedInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* speedHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pSpeedInfoPanel->SetSizer(speedHSizer);
        m_pSpeedInfoPanel->SetMinSize(wxSize(90, 30));
        taskInfoSizer->Add(m_pSpeedInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap speedBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("printer_speed.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* speedIcon = new wxStaticBitmap(m_pSpeedInfoPanel, wxID_ANY, speedBitmap);
        speedIcon->SetMinSize(speedBitmap.GetSize());
        speedHSizer->Add(speedIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        speedHSizer->AddSpacer(8);

        m_pSpeedInfoText = new wxStaticText(m_pSpeedInfoPanel, wxID_ANY, "250mm/s");
        m_pSpeedInfoText->SetBackgroundColour(m_panelBackColor);
        m_pSpeedInfoText->SetForegroundColour(m_textDarkColor);
        font = m_pSpeedInfoText->GetFont();
        font.SetPointSize(10);
        m_pSpeedInfoText->SetFont(font);
        speedHSizer->Add(m_pSpeedInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        speedHSizer->AddSpacer(53);
    }

    //taskInfoSizer->AddSpacer(33);


    // used_filament
    {
        m_pFilamentInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* filamentInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pFilamentInfoPanel->SetSizer(filamentInfoHSizer);
        m_pFilamentInfoPanel->SetMinSize(wxSize(90, 30));
        taskInfoSizer->Add(m_pFilamentInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFilamentInfoPanel, wxID_ANY, usedFBitmap);
        usedFIcon->SetMinSize(usedFBitmap.GetSize());
        filamentInfoHSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        filamentInfoHSizer->AddSpacer(8);

        m_pFilamentInfoText = new wxStaticText(m_pFilamentInfoPanel, wxID_ANY, "100g");
        m_pFilamentInfoText->SetBackgroundColour(m_panelBackColor);
        m_pFilamentInfoText->SetForegroundColour(m_textDarkColor);
        font = m_pFilamentInfoText->GetFont();
        font.SetPointSize(10);
        m_pFilamentInfoText->SetFont(font);
        filamentInfoHSizer->Add(m_pFilamentInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        filamentInfoHSizer->AddSpacer(53);
    }

    //taskInfoSizer->AddSpacer(33);

    // layers
    {
        m_pLayerInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* layerInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pLayerInfoPanel->SetSizer(layerInfoHSizer);
        m_pLayerInfoPanel->SetMinSize(wxSize(90, 30));
        taskInfoSizer->Add(m_pLayerInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap layerBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("layer_icon.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* layerIcon = new wxStaticBitmap(m_pLayerInfoPanel, wxID_ANY, layerBitmap);
        layerIcon->SetMinSize(layerBitmap.GetSize());
        layerInfoHSizer->Add(layerIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        layerInfoHSizer->AddSpacer(8);

        m_pLayerInfoText = new wxStaticText(m_pLayerInfoPanel, wxID_ANY, "0 / 0");
        m_pLayerInfoText->SetBackgroundColour(m_panelBackColor);
        m_pLayerInfoText->SetForegroundColour(m_textDarkColor);
        font = m_pLayerInfoText->GetFont();
        font.SetPointSize(10);
        m_pLayerInfoText->SetFont(font);
        layerInfoHSizer->Add(m_pLayerInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        layerInfoHSizer->AddSpacer(53);
    }

    //taskInfoSizer->AddSpacer(33);

    // finish time
    {
        m_pFinishTimeInfoPanel = new wxPanel(m_pInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxBoxSizer* finishTimeInfoHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_pFinishTimeInfoPanel->SetSizer(finishTimeInfoHSizer);
        m_pFinishTimeInfoPanel->SetMinSize(wxSize(90, 30));
        taskInfoSizer->Add(m_pFinishTimeInfoPanel, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        wxBitmap ftBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
        wxStaticBitmap* ftIcon = new wxStaticBitmap(m_pFinishTimeInfoPanel, wxID_ANY, ftBitmap);
        ftIcon->SetMinSize(ftBitmap.GetSize());
        finishTimeInfoHSizer->Add(ftIcon, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        finishTimeInfoHSizer->AddSpacer(8);

        m_pFinishTimeInfoText = new wxStaticText(m_pFinishTimeInfoPanel, wxID_ANY, "00:00 am");
        m_pFinishTimeInfoText->SetBackgroundColour(m_panelBackColor);
        m_pFinishTimeInfoText->SetForegroundColour(m_textDarkColor);
        font = m_pFinishTimeInfoText->GetFont();
        font.SetPointSize(10);
        m_pFinishTimeInfoText->SetFont(font);
        finishTimeInfoHSizer->Add(m_pFinishTimeInfoText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
    }

    taskInfoSizer->AddStretchSpacer(3);

    //taskDetailSizer->AddStretchSpacer(1);
}

void AnkerTaskPanel::initControlPanel(wxWindow* parent)
{
    m_pControlPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    wxBoxSizer* ctrlTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pControlPanel->SetSizer(ctrlTitleHSizer);
    m_pControlPanel->SetMinSize(wxSize(760, 56));
    m_pControlPanel->SetBackgroundColour(m_backgroundColour);

    ctrlTitleHSizer->AddSpacer(12);

    m_pCtrlTitleText = new wxStaticText(m_pControlPanel, wxID_ANY, "N/A");
    m_pCtrlTitleText->SetBackgroundColour(m_panelBackColor);
    m_pCtrlTitleText->SetForegroundColour(m_textDarkColor);
    wxFont font = m_pCtrlTitleText->GetFont();
    font.SetPointSize(9);
    m_pCtrlTitleText->SetFont(font);
    ctrlTitleHSizer->Add(m_pCtrlTitleText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 0);


    m_pCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
    //m_pCtrlSizer->SetMinSize(wxSize(700, 24));
    ctrlTitleHSizer->Add(m_pCtrlSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM, 0);

    wxStaticText* splitText = new wxStaticText(m_pControlPanel, wxID_ANY, "|");
    splitText->SetBackgroundColour(m_panelBackColor);
    splitText->SetForegroundColour(m_textDarkColor);
    font = splitText->GetFont();
    font.SetPointSize(9);
    splitText->SetFont(font);
    m_pCtrlSizer->Add(splitText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 16);

    m_pCtrlStatusText = new wxStaticText(m_pControlPanel, wxID_ANY, "--");
    m_pCtrlStatusText->SetBackgroundColour(m_panelBackColor);
    m_pCtrlStatusText->SetForegroundColour(m_textDarkColor);
    font = m_pCtrlStatusText->GetFont();
    font.SetPointSize(9);
    m_pCtrlStatusText->SetFont(font);
    m_pCtrlSizer->Add(m_pCtrlStatusText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 0);

    m_pCtrlSizer->AddStretchSpacer(1);

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
    m_pCtrlSizer->Add(m_pStopButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 0);

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
    m_pCtrlSizer->Add(m_pStartButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

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
    m_pCtrlSizer->Add(m_pPauseButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    m_pCtrlSizer->AddSpacer(12);
}

void AnkerTaskPanel::startPrint()
{
    m_gcodeImportResult = m_pGCodeImportDialog->getImportResult();

    Datamanger& dm = Datamanger::GetInstance();
    DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);
    if (!currentDev)
        return;

    if (currentDev->getDeviceCtrlResult() == MqttType::PrintCtlResult::PrintCtlResult_Reminder_leveling)
    {
        int levelTime = currentDev->time * 1.0 / 60;
        std::string levelReminder = "Before printing, we suggest running the auto-level for a better printing experience, it will takes about " + std::to_string(levelTime) + " minutes to go.";
        std::string title = "Print Fail";
        AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title);
        if (result == AnkerMsgDialog::MsgResult::MSG_OK)
        {
            Datamanger& dm = Datamanger::GetInstance();
            DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

            if (currentDev)
            {
                currentDev->setLevelBegin();
            }
        }

        return;
    }

    if (m_gcodeImportResult.m_srcType == AnkerGCodeImportDialog::FileSelectMode::FSM_COMPUTER)
    {
        currentDev->printComputerLocalFile(m_gcodeImportResult.m_filePath);
    }
    else
    {
        currentDev->setDevicePrintBegin(m_gcodeImportResult.m_filePath);
    }

    setLoadingVisible(true);
}

void AnkerTaskPanel::setLoadingVisible(bool visible)
{
    if (m_pLoadingMask == nullptr)
    {
        m_pLoadingMask = new AnkerLoadingMask(this);
    }

    m_pLoadingMask->SetSize(GetSize());
    m_pLoadingMask->SetPosition(wxPoint(0, 0));
    bool success = m_pLoadingMask->Show(visible);
}

void AnkerTaskPanel::setInfoPanelEnable(bool enable)
{
    m_pTimeLeftValueText->SetForegroundColour(enable ? wxColour(SYSTEM_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pTimeLeftLabelText->SetForegroundColour(enable ? wxColour(SYSTEM_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pTaskProgressCtrl->setProgressColor(enable ? wxColour(SYSTEM_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pSpeedInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pFilamentInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pLayerInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
    m_pFinishTimeInfoText->SetForegroundColour(enable ? wxColour(TEXT_DARK_COLOR) : wxColour(CONTROL_DISABLE_COLOR));
}

void AnkerTaskPanel::OnSize(wxSizeEvent& event)
{
    Layout();
    Refresh();
}

void AnkerTaskPanel::OnNewBtn(wxCommandEvent& event)
{
    m_pGCodeImportDialog->switch2FileSelect(AnkerGCodeImportDialog::FSM_COMPUTER);

    int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
    int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
    m_pGCodeImportDialog->SetPosition(wxPoint((screenW - 400) / 2, (screenH - 180) / 2));

    if (m_pGCodeImportDialog->ShowModal() == wxID_CANCEL)
        return;

    startPrint();
}

void AnkerTaskPanel::OnStartBtn(wxCommandEvent& event)
{
    Datamanger& dm = Datamanger::GetInstance();
    DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

    if (!currentDev)
        return;

    if (currentDev->deviceStatus == MQTT_PRINT_EVENT_PAUSED)
        currentDev->setDevicePrintResume("");
    else if (currentDev->deviceStatus == MQTT_PRINT_EVENT_STOPPED)
        currentDev->setDevicePrintBegin(m_gcodeImportResult.m_filePath);

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        if (currentDev->deviceStatus == MQTT_PRINT_EVENT_PAUSED)
            wxPostEvent(this->GetParent(), wxCommandEvent(wxANKEREVT_PRINTING_CONTINUE));
        else if (currentDev->deviceStatus == MQTT_PRINT_EVENT_STOPPED)
            wxPostEvent(this->GetParent(), wxCommandEvent(wxANKEREVT_PRINTING_STARTED));
    }

    m_pStartButton->Show(false);
    m_pStopButton->Show(true);
    m_pPauseButton->Show(true);

    Layout();
}

void AnkerTaskPanel::OnStopBtn(wxCommandEvent& event)
{
    std::string levelReminder = "Are you sure you want to quit?";
    std::string title = "Stopping";
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, true);
    if (result != AnkerMsgDialog::MSG_OK)
        return;

    setLoadingVisible(true);

    Datamanger& dm = Datamanger::GetInstance();
    DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

    if (!currentDev)
        return;

    if (m_currentMode == TASK_MODE_PRINT)
        currentDev->setDevicePrintStop("");
    else if (m_currentMode == TASK_MODE_LEVEL)
        currentDev->setLevelStop();

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_STOPPED);
        ProcessEvent(evt);
    }
    else if (m_currentMode == TASK_MODE_LEVEL)
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_LEVELING_STOPPED);
        ProcessEvent(evt);
    }
}

void AnkerTaskPanel::OnPauseBtn(wxCommandEvent& event)
{
    std::string levelReminder = "Are you sure you want to pause the task?";
    std::string title = "Pausing";
    AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, true, "Yes", "No");
    if (result != AnkerMsgDialog::MSG_OK)
        return;

    Datamanger& dm = Datamanger::GetInstance();
    DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

    if (!currentDev)
        return;

    currentDev->setDevicePrintPause(m_gcodeImportResult.m_filePath);

    // if success to control
    if (m_currentMode == TASK_MODE_PRINT)
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_PRINTING_PAUSED);
        //wxPostEvent(this->GetParent(), evt);
        ProcessEvent(evt);
    }

    m_pStartButton->Show(true);
    m_pStopButton->Show(true);
    m_pPauseButton->Show(false);

    Layout();
}

void AnkerTaskPanel::OnTimer(wxTimerEvent& event)
{
    // update time left
    if (m_countDownSeconds > 0 && (m_lastDeviceStatus == MQTT_PRINT_EVENT_PRINTING || m_lastDeviceStatus == MQTT_PRINT_EVENT_LEVELING))
    {
        m_countDownSeconds--;

        int remainingSeconds = m_countDownSeconds % 60;
        int remainingMinutes = m_countDownSeconds / 60 % 60;
        int remainingHours = m_countDownSeconds / 60 / 60 % 60;
        int remainingDays = m_countDownSeconds / 60 / 60 / 60 % 24;
        // update printing progress
        std::string timeLeftStr = (remainingHours < 10 ? "0" : "") + std::to_string(remainingHours) + " : " + (remainingMinutes < 10 ? "0" : "") + std::to_string(remainingMinutes) + " : " + (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        m_pTimeLeftValueText->SetLabelText(timeLeftStr);
    }

    // update progress
    double curProgress = m_pTaskProgressCtrl->getCurrentProgress();
    m_pTaskProgressCtrl->updateProgress(curProgress + 1);
}

void AnkerTaskPanel::updateFileTransferStatus(std::string currentSn, int progress)
{
    if (progress < 0)
    {
        std::string levelReminder = "Failed to transfer the gcode file.";
        std::string title = "ERROR";

        AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, false);
        
        switchMode(TASK_MODE_NONE);

        return;
    }

    // update printing progress
    std::string timeLeftStr = "00:00:00";
    m_pTimeLeftValueText->SetLabelText(timeLeftStr);

    //m_pTaskProgressCtrl->setProgressRange(100);
    //m_pTaskProgressCtrl->updateProgress(progress);

    //m_pLocalCounter->Start(1000);
}

AnkerProgressCtrl::AnkerProgressCtrl(wxWindow* parent)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_lineLenRate(0.98)
    , m_lineWidth(3)
    , m_margin(1)
    , m_progressedLineLenRate(0)
    , m_rangeMin(0)
    , m_rangeMax(700)
    , m_currentProgress(0)
    , m_progressColor(SYSTEM_COLOR)
    , m_unProgressColor(125, 125, 125)
    , m_labelVisible(true)
    , m_labelStr("0%")
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &AnkerProgressCtrl::OnPaint, this);
    Bind(wxEVT_SIZE, &AnkerProgressCtrl::OnSize, this);
}

AnkerProgressCtrl::~AnkerProgressCtrl()
{
}

bool AnkerProgressCtrl::setProgressRange(double max, double min)
{
    if (max <= min)
        return false;

    m_rangeMax = max; 
    m_rangeMin = min; 
    
    m_currentProgress = std::max(std::min(m_currentProgress, m_rangeMax), m_rangeMin);
    updateProgress(m_currentProgress);

    return true;
}

void AnkerProgressCtrl::updateProgress(double progress)
{
    if (progress != m_currentProgress && progress >= m_rangeMin && progress <= m_rangeMax)
    {
        m_currentProgress = progress;

        m_progressedLineLenRate = m_currentProgress * 1.0 / (m_rangeMax - m_rangeMin);
        m_labelStr = std::to_string((int)(m_progressedLineLenRate * 100)) + "%";
    }
}

void AnkerProgressCtrl::setLineLenRate(double rate)
{
    if (rate <= 0 || rate == m_lineLenRate)
        return;

    m_lineLenRate = rate;
}

void AnkerProgressCtrl::setMargin(int margin)
{
    if (margin < 0 || margin == m_margin)
        return;

    m_margin = margin;
}

void AnkerProgressCtrl::initUI()
{
}

void AnkerProgressCtrl::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.Clear();

    wxRect rect = GetClientRect();
    wxBrush brush(wxColour(PANEL_BACK_COLOR));
    wxPen pen(wxColour(PANEL_BACK_COLOR));
    dc.SetBrush(brush);
    dc.SetPen(pen);
    dc.DrawRectangle(rect);

    int winLen = GetSize().x;
    int lineLen = winLen * m_lineLenRate;
    int progressedLineLen = winLen * m_lineLenRate * m_progressedLineLenRate ;

    if (m_labelVisible)
    {
        wxBrush brush(m_progressColor);
        wxPen pen(m_progressColor);
        dc.SetBrush(brush);
        dc.SetPen(pen);
        dc.SetFont(m_labelFont);
        dc.SetTextForeground(m_progressColor);
        wxPoint textPoint = wxPoint(lineLen - m_labelFont.GetPointSize() * 4, GetSize().y - m_lineWidth - m_labelFont.GetPointSize() * 2.5);
        dc.DrawText(m_labelStr, textPoint);
    }

    if (lineLen > 0 && m_margin >= 0)
    {
        int centerY = GetSize().y / 2;
        int radius = 2;

        // draw unProgress line
        dc.SetBrush(wxBrush(m_unProgressColor));
        dc.SetPen(wxPen(m_unProgressColor));
        //dc.SetTextForeground(m_unProgressColor);
        dc.DrawRoundedRectangle(m_margin, GetSize().y - m_lineWidth, lineLen, m_lineWidth, radius);

        //  draw progress line
        dc.SetBrush(wxBrush(m_progressColor));
        dc.SetPen(wxPen(m_progressColor));
        //dc.SetTextForeground(m_progressColor);
        dc.DrawRoundedRectangle(m_margin, GetSize().y - m_lineWidth, progressedLineLen, m_lineWidth, radius);
    }
}

void AnkerProgressCtrl::OnSize(wxSizeEvent& event)
{
    //Layout();
    Update();
}

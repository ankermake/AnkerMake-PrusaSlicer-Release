#ifndef _ANKER_TASK_PANEL_H_
#define _ANKER_TASK_PANEL_H_

#include "wx/wx.h"
#include "AnkerGCodeImportDialog.hpp"
#include "AnkerPrintFinishDialog.hpp"
#include "AnkerNetDefines.h"

namespace AnkerNet
{
	DEF_PTR(DeviceObjectBase)
}
using namespace AnkerNet;

wxDECLARE_EVENT(wxANKEREVT_LEVELING_STOPPED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_STARTED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_PAUSED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_CONTINUE, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_STOPPED, wxCommandEvent);

class AnkerLoadingMask;
class AnkerProgressCtrl;
class AnkerTaskPanel : public wxPanel
{
public:
	enum TaskMode
	{
		TASK_MODE_NONE,
		TASK_MODE_PRINT,
		TASK_MODE_LEVEL,
		TASK_MODE_CALIBRATE,
		TASK_MODE_TRANFER
	};

	enum PrintStartType
	{
		TYPE_SLICE = 0,
		TYPE_LOCAL_FILE,
		TYPE_REMOTE_STORAGE,
		TYPE_REMOTE_USB,
		TYPE_COUNT
	};

	enum PrintStartResult
	{
		START_SUCCESS = 0,
		START_FAIL
	};
public:
	AnkerTaskPanel(std::string currentSn, wxWindow* parent);
	~AnkerTaskPanel();

	// switch layout
	void switchMode(TaskMode mode, bool force = false);
	// update task info
	void updateStatus(std::string currentSn, int type);
	// network request callback
	void requestCallback(int type = -1);
	// update file transfer status
	void updateFileTransferStatus(std::string currentSn, int progress, FileTransferResult result);

	void activate(bool active);

	void setOfflineStatus();
private:
	void initUI();
	void initTitlePanel(wxWindow* parent);
	void initInfoEmptyPanel(wxWindow* parent);
	void initInfoPanel(wxWindow* parent);
	void initControlPanel(wxWindow* parent);
	void startPrint();
	void setLoadingVisible(bool visible);
	void setInfoPanelEnable(bool enable);

	void OnSize(wxSizeEvent& event);
	void OnShow(wxShowEvent& event);
	void OnNewBtn(wxCommandEvent& event);
	void OnStartBtn(wxCommandEvent& event);
	void OnStopBtn(wxCommandEvent& event);
	void OnPauseBtn(wxCommandEvent& event);
	void OnCountdownTimer(wxTimerEvent& event);
	void OnLoadingTimeout(wxCommandEvent& event);
	void OnLoadMaskRectUpdate(wxCommandEvent& event);
	void LevelHint(AnkerNet::DeviceObjectBasePtr currentDev);

private:
	bool m_printCompleteCheckFlag;
	bool m_modaling;
	bool m_toPrinting;
	int m_lastLeftSeconds;
	std::string m_currentDeviceSn;
	wxTimer* m_pToPrintingTimer{nullptr};
	wxTimer* m_pModalingTimer{ nullptr };
	GUI_DEVICE_STATUS_TYPE m_currentDeviceStatus;
	GUI_DEVICE_STATUS_TYPE m_lastDeviceStatus;
	GUI_DEVICE_STATUS_TYPE m_modalingDeviceStatus;

	TaskMode m_currentMode;

	wxColour m_panelBackColor;
	wxColour m_textLightColor;
	wxColour m_textDarkColor;
	wxColour m_systemColor;
	wxColour m_imageBackColor;

	wxPanel* m_pTitledPanel;
	wxPanel* m_pInfoEmptyPanel;
	wxPanel* m_pInfoPanel;
	wxPanel* m_pControlPanel;
	wxPanel* m_pSpeedInfoPanel;
	wxPanel* m_pFilamentInfoPanel;
	wxPanel* m_pLayerInfoPanel;
	wxPanel* m_pFinishTimeInfoPanel;
	wxBoxSizer* m_pCtrlHSizer;

	wxStaticBitmap* m_pPreviewImage;
	wxStaticText* m_pTimeLeftValueText;
	wxStaticText* m_pTimeLeftLabelText;
	wxStaticText* m_pSpeedInfoText;
	wxStaticText* m_pFilamentInfoText;
	wxStaticText* m_pLayerInfoText;
	wxStaticText* m_pFinishTimeInfoText;
	wxStaticText* m_pCtrlTitleText;
	wxStaticText* m_pCtrlStatusText;
	wxButton* m_pRefreshButton;
	wxButton* m_pStartButton;
	wxButton* m_pStopButton;
	wxButton* m_pPauseButton;
	AnkerBtn* m_newTaskButton;

	AnkerProgressCtrl* m_pTaskProgressCtrl;

	AnkerGCodeImportDialog::GCodeImportResult m_gcodeImportResult;
	AnkerGCodeImportDialog* m_pGCodeImportDialog;

	AnkerPrintFinishDialog* m_pPrintFinishDialog{ nullptr };

	AnkerLoadingMask* m_pLoadingMask;
	wxStaticText* m_titleEmptyBanner;

	int m_countDownSeconds;
	wxTimer* m_pLocalCounter{nullptr};
};

#endif // _ANKER_TASK_PANEL_H_

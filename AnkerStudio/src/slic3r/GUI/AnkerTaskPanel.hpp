#ifndef _ANKER_TASK_PANEL_H_
#define _ANKER_TASK_PANEL_H_

#include "wx/wx.h"
#include "AnkerGCodeImportDialog.hpp"


wxDECLARE_EVENT(wxANKEREVT_LEVELING_STOPPED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_STARTED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_PAUSED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_CONTINUE, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_PRINTING_STOPPED, wxCommandEvent);

class AnkerLoadingMask;
class AnkerProgressCtrl : public wxControl
{
public:
	AnkerProgressCtrl(wxWindow* parent);
	~AnkerProgressCtrl();

	void setProgressColor(wxColour color) { m_progressColor = color; }
	void setUnProgressColor(wxColour color) { m_unProgressColor = color; }

	bool setProgressRange(double max, double min = 0.0);
	void updateProgress(double progress);
	double getProgressRange() { return m_rangeMax - m_rangeMin; }
	double getCurrentProgress() { return m_currentProgress; }

	// line length rate of the window
	void setLineLenRate(double rate);
	void setLineWidth(int width) { m_lineWidth = width; }
	void setMargin(int margin);

	void setLabelFont(wxFont font) { m_labelFont = font; }
	void setLabelVisible(bool visible) { m_labelVisible = visible; }

private:
	void initUI();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	int m_lineWidth;
	int m_margin;
	double m_lineLenRate;
	double m_progressedLineLenRate;

	int m_rangeMin;
	int m_rangeMax;
	int m_currentProgress;

	wxColour m_progressColor;
	wxColour m_unProgressColor;

	bool m_labelVisible;
	std::string m_labelStr;
	wxFont m_labelFont;
};


class AnkerTaskPanel : public wxPanel
{
public:
	enum TaskMode
	{
		TASK_MODE_NONE,
		TASK_MODE_PRINT,
		TASK_MODE_LEVEL,
		TASK_MODE_TRANFER
	}; 
public:
	AnkerTaskPanel(std::string currentSn, wxWindow* parent);
	~AnkerTaskPanel();

	// switch layout
	void switchMode(TaskMode mode);
	// update task info
	void updateStatus(std::string currentSn);
	// network request callback
	void requestCallback();
	// update file transfer status
	void updateFileTransferStatus(std::string currentSn, int progress);

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
	void OnNewBtn(wxCommandEvent& event);
	void OnStartBtn(wxCommandEvent& event);
	void OnStopBtn(wxCommandEvent& event);
	void OnPauseBtn(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);

	// check the device is exception finished
	bool checkIsExceptionFinished(mqtt_print_event_e currentStatus)
	{
		return currentStatus == MQTT_PRINT_EVENT_IDLE 
			&& (m_lastDeviceStatus == MQTT_PRINT_EVENT_PRINT_HEATING 
				|| m_lastDeviceStatus == MQTT_PRINT_EVENT_PAUSED
				|| m_lastDeviceStatus == MQTT_PRINT_EVENT_PRINTING
				|| m_lastDeviceStatus == MQTT_PRINT_EVENT_PRINT_PREHEATING);
	}

private:
	bool m_printCompleteCheckFlag;
	std::string m_currentDeviceSn;
	mqtt_print_event_e m_lastDeviceStatus;

	TaskMode m_currentMode;

	wxColour m_panelBackColor;
	wxColour m_textLightColor;
	wxColour m_textDarkColor;
	wxColour m_systemColor;

	wxPanel* m_pTitlePanel;
	wxPanel* m_pInfoEmptyPanel;
	wxPanel* m_pInfoPanel;
	wxPanel* m_pControlPanel;
	wxPanel* m_pSpeedInfoPanel;
	wxPanel* m_pFilamentInfoPanel;
	wxPanel* m_pLayerInfoPanel;
	wxPanel* m_pFinishTimeInfoPanel;
	wxBoxSizer* m_pCtrlSizer;

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

	AnkerProgressCtrl* m_pTaskProgressCtrl;

	AnkerGCodeImportDialog::GCodeImportResult m_gcodeImportResult;
	AnkerGCodeImportDialog* m_pGCodeImportDialog;

	AnkerLoadingMask* m_pLoadingMask;

	int m_countDownSeconds;
	wxTimer* m_pLocalCounter;
};

#endif // _ANKER_TASK_PANEL_H_

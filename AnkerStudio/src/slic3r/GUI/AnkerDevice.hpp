#ifndef _ANKER_DEVICE_HPP_
#define _ANKER_DEVICE_HPP_

#include <iostream>
#include <vector>

#include "wx/wx.h"
#include "AnkerBtn.hpp"
#include <wx/event.h>
#include <wx/stattext.h>
#include <wx/regex.h>
#include <wx/valtext.h>
#include <boost/signals2.hpp>
#include "AnkerAdjustItemWidget.hpp"
#include "AnkerTaskPanel.hpp"
#include "AnkerTextLabel.hpp"
#include "AnkerCustomEvent.hpp"
#include "AnkerHyperlink.hpp"
#include "AnkerShowWidget.hpp"
#include "AnkerNavWidget.hpp"
#include "AnkerLineEdit.hpp"
#include "wx/timer.h"


class AnkerDeviceControl;
class AnkerOtherWidget;
class AnkerLoadingMask;

wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_DEVICE_LIST_REFRESH, wxCommandEvent);

class AnkerAdjustWidget :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerAdjustWidget)
	DECLARE_EVENT_TABLE()


public:
	enum ItemStyle
	{
		ITEM_LEVEL
	};

	AnkerAdjustWidget();
	AnkerAdjustWidget(wxString sn, wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerAdjustWidget();
	void setItemStatus(bool isAvailable, ItemStyle Style);
protected:
	void initUi();
private:
	wxStaticText* m_title;
	AnkerAdjustItemWidget* m_pAdjustItemWidget;
	wxString	  m_sn{ "" };
};

class AnkerTemperatureWidget :public wxControl
{

public:

	AnkerTemperatureWidget(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerTemperatureWidget();
	void showEditTempertrueBtn(bool show);
	void setMaxRage(const int &maxRange);
	void setMinRage(const int& minRange);
	void setTipsText(const wxString& text);
	void setHintSize(const wxString& text);
	void setTemperatureNum(const wxString &numText);
	void updateWorkStatus(bool isWork);
protected:
	void initUi();
	void OnEnter(wxCommandEvent& event);
	void OnTextCtrlEditFinished(wxCommandEvent& event);	
private:
	wxStaticText* m_pNumText;
	wxStaticText* m_pNumUnitText;

	wxPanel* m_pNormalPanel;
	wxStaticText* m_pTipsText;	
	wxButton* m_pEditBtn;

	wxPanel* m_pEditPanel;
	AnkerLineEdit* m_pTemperatureEdit;
	wxStaticText* m_TemperatureUnitText;
		
	wxString	m_tipsText = { "" };
	int			m_minSize = 0;
	int			m_maxSize = 0;

	bool		m_isheating = false;
	bool		m_isEditStatus = false;

};

class AnkerControlWidget :public wxControl
{
public:
	AnkerControlWidget(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerControlWidget();
	void updateTemperatrue(const std::string& sn);
	void showEditTempertrueBtn(bool show);
	void updateWorkStatus(bool iswork);
protected:
	void initUi();

private:
	wxStaticText* m_title;
	AnkerTemperatureWidget* m_pNozWidegt;
	AnkerTemperatureWidget* m_pHeatBedWidegt;
};

class AnkerDeviceControl :public wxControl
{
public:
	AnkerDeviceControl(std::string currentDeviceID, wxWindow * parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerDeviceControl();
	
	void updateTemperatrue(const std::string& sn);
	void updateLevel(const std::string& sn);
	void updateFileTransferStatus(const std::string& sn, int progress);

	// video
	void displayVideoFrame(const unsigned char* imgData, int width, int height);
	void onP2pvideoStreamSessionInit();
	void onRecvVideoStreamClosing();
	void onP2pvideoStreamSessionClosed();
	void onRcvP2pVideoStreamCtrlAbnomal();
	void onRecvCameraLightStateNotify(bool onOff);
	void onRecVideoModeNotify(int mode);

protected:
	void initUi();
private:
	wxPanel* m_pVideoWidget;
	AnkerTaskPanel* m_pStatusWidget;

	AnkerControlWidget* m_pControlWidget;
	AnkerAdjustWidget* m_pAdjustWidget;
	AnkerOtherWidget* m_pOtherWidget;

	std::string	m_currentDeviceId;
};

class AnkerDevice :public wxControl
{
public:
	AnkerDevice(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerDevice();	

	enum WARNING_STYLE {
		LEVEL_WARNING = 0,

	};

	void showUnlogin();
	bool loadDeviceList(bool isFresh = false);
	void showNoDevice();
	void showDeviceList(); 
	void showOfflineDevice();

	void updateAboutMqttStatus(std::string sn);
	void updateDeviceList(bool rc);
	void updateFileTransferStatus(std::string sn, int progress);

	void showMsgDialog(const std::string& title, const std::string &content, bool isCancel = false);
	void showWarningDialog(const std::string& title, const std::string& content,const std::string& sn, WARNING_STYLE warningStyle);	
	void setLoadingVisible(bool visible);
protected:
	void updateLevel(const std::string& sn);	

	//0 No 1 heatbed
	//void updateTemperatrue(const std::string& sn, const std::string& value, int style);
	void updateTemperatrue(const std::string& sn);

	// video
	void setVideoCallBacks();
	void unsetVideoCallBacks();
	void displayVideoFrame(std::string& sn, const unsigned char* imgData, int width, int height);
	void onP2pvideoStreamSessionInit(const std::string& sn);

	void onRecvVideoStreamClosing(const std::string& sn);
	void onP2pvideoStreamSessionClosed(const std::string& sn);
	void onRcvP2pVideoStreamCtrlAbnomal(const std::string& sn);
	
	void onRecvCameraLightStateNotify(const std::string& sn, bool onOff);
	void onRecVideoModeNotify(const std::string& sn, int mode);
	//void onVideoP2pInited();
	//void onRcvP2pVideoStreamCtrlBusyFeedback();
		
	void showDevice(const std::string &snId);
	void clearWidegt();
	void initUi();
	void removeExpiredDevice();	
	
private:
	AnkerUnLoginPanel* m_pUnloginPanel;
	AnkerNavBar* m_pNavBarWidget;
	AnkerEmptyDevice* m_pEmptyDeviceWidget;

	AnkerOfflineDevice* m_pOfflineDeviceWidget;
	std::list<AnkerDeviceControl*> m_deviceWidgetList;
	wxBoxSizer* m_pMainHSizer;
	wxBoxSizer* m_pDeviceHSizer;
	std::string	m_currentDeviceId = {""};
	wxTimer	  * m_reloadTimer;

	AnkerLoadingMask* m_pLoadingMask;
};


#endif


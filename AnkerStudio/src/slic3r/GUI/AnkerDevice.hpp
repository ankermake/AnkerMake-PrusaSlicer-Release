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
#include "Common/AnkerFrameModal.hpp"
#include "Common/AnkerMerterialBoxBtn.hpp"
#include "Common/AnkerLineEditUnit.hpp"
#include "AnkerVideo.hpp"
#include "slic3r/GUI/Network/MsgText.hpp"

class AnkerDeviceControl;
class AnkerOtherWidget;
class AnkerLoadingMask;

wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_DEVICE_LIST_REFRESH, wxCommandEvent);

wxDECLARE_EVENT(wxCUSTOMEVT_ZOFFSET_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_EXTRUDEX, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_RETRACT, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_STOP_EXTRUDEX, wxCommandEvent);


enum  ExtrudeStatus
{
	EXTRUDE_FREE = 1,
	EXTRUDING = 2,
	EXTRUDE_RETACTING = 3,
	EXTRUDE_DISABLE = 4,
};

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
	AnkerTemperatureWidget(const std::string& deviceSn, wxWindow* parent,
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
	wxString getValue();
	void setOfflineStatus();
protected:
	void initUi();
	void OnEnter(wxCommandEvent& event);
	void OnTextCtrlEditFinished(wxCommandEvent& event);	
private:
	std::string	m_currentDeviceSn{ "" };
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
	AnkerControlWidget(const std::string& deviceSn, wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerControlWidget();

	int getCurrentWorkNozzle();
	void updateTemperatrue(const std::string& sn);
	void updateNozzleStatus(const std::string& sn);
	void updateZoffesetValue(const std::string& sn);
	void showEditTempertrueBtn(bool show);
	void updateWorkStatus(bool iswork);
	void onMaterilBtnClicked(wxCommandEvent& event);

	void showMulitColorNozzle(bool isShow);
	void setIsSingleNozOfflineStatus(bool isSimpleNozOffline);	

	void setZoffsetDisabel(bool isAble);
	void setExtrudeAble(bool isAble, int statusValue = 0);
	void setNozsAble(bool isAble);//mulit nozs

	void accurateCalculation(wxString& data);
	void resetExtrudeBtns();
	void onExtrudeBtnClicked(wxCommandEvent& event);
	void onRetractBtnClicked(wxCommandEvent& event);
	void setNozzleMaxTemp(int temp); 
protected:
	void initUi();
	void showStopBtnStatus(AnkerBtn* pBtn);
	void showDisAbelBtnStatus(AnkerBtn* pBtn);
	void showNorBtnStatus(AnkerBtn* pBtn,const wxString& btnName);
private:
	std::string	m_currentDeviceSn{ "" };
	wxStaticText* m_title{ nullptr };
	wxPanel* m_pNumPanel{ nullptr };
	wxPanel* m_pNumLine{ nullptr };

	wxStaticText* m_pNozzelNum{ nullptr };
	AnkerTemperatureWidget* m_pNozWidegt{ nullptr };
	AnkerTemperatureWidget* m_pHeatBedWidegt{ nullptr };

	wxStaticBitmap* m_pDeviceImg{ nullptr };
	wxPanel* m_pPrintNozzlesWidget{ nullptr };
	AnkerLineEditUnit* m_pLengthLineEdit{ nullptr };
	AnkerLineEditUnit* m_pOffsetLineEdit{ nullptr };
	wxPanel* m_pOffsetPanel{ nullptr };
	wxStaticText* m_pLabel{ nullptr };

	AnkerMerterialBoxBtn* m_pBtn1{ nullptr };//nozzle num 1
	AnkerMerterialBoxBtn* m_pBtn2{ nullptr };//nozzle num 2
	AnkerMerterialBoxBtn* m_pBtn3{ nullptr };//nozzle num 3
	AnkerMerterialBoxBtn* m_pBtn4{ nullptr };//nozzle num 4
	AnkerMerterialBoxBtn* m_pBtn5{ nullptr };//nozzle num 5
	AnkerMerterialBoxBtn* m_pBtn6{ nullptr };//nozzle num 6

	wxStaticText* m_pMaterialLengthLabel = nullptr;
	AnkerBtn* m_pRetractBtn = nullptr;
	AnkerBtn* m_pExtrudeBtn = nullptr;
	wxPanel* m_pLengthPanel = nullptr;
	ExtrudeStatus m_extrudeStatus = EXTRUDE_FREE;

	AnkerBtn* m_pdeduceBtn = nullptr;
	AnkerBtn* m_pAddBtn = nullptr;
	std::vector<AnkerMerterialBoxBtn*> m_btnList;
	
	bool m_isMulitColor = false;
};

class AnkerDeviceControl :public wxControl
{
public:
	AnkerDeviceControl(std::string currentDeviceID, wxWindow * parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerDeviceControl();
		
	int  getCurrentWorkNozzle();
	void updateDeviceStatus(const std::string& sn, AnkerNet::aknmt_command_type_e type );
	void updateZoffsetValue(const std::string& sn);
	void updateFileTransferStatus(const std::string& sn, int progress, AnkerNet::FileTransferResult result);
	void showMulitColorNozzle(bool isShow);
	void setIsSingleNozOfflineStatus(bool isSimpleNozOffline);

	// video
	void displayVideoFrame(const unsigned char* imgData, int width, int height);
	void onP2pvideoStreamSessionInit();
	void onRecvVideoStreamClosing();
	void onP2pvideoStreamSessionClosed();
	void onRcvP2pVideoStreamCtrlAbnomal();
	void onRecvCameraLightStateNotify(bool onOff);
	void onRecVideoModeNotify(int mode);

	std::string getCurrentDeviceId() const;
	void showMsgFrame(const NetworkMsg& msg);

	void activate(bool active);

    void updateExtrude(bool isAble, int statusValue = 0);
    void updateZoffset(bool isAble);
	void OnStopExtrude(wxCommandEvent& event);
	void OnExtrude(wxCommandEvent& event);
	void OnRetract(wxCommandEvent& event);

	void updateWindowDisplay(wxUpdateUIEvent& event);
	void setNozzleMaxTemp(int temp);
protected:
	void initUi();
	void OnShow(wxShowEvent& event);

private:
	AnkerVideo* m_pVideoWidget{nullptr};
	AnkerTaskPanel* m_pStatusWidget{nullptr};

	AnkerControlWidget* m_pControlWidget{ nullptr };	
	AnkerOtherWidget* m_pOtherWidget{ nullptr };

	std::string	m_currentDeviceId{""};	// current device sn
	HalfModalDialog* m_halfDialog{ nullptr };
};

class AnkerDevice :public wxControl
{
public:
	AnkerDevice(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerDevice();	

	void Init();
	void showUnlogin();
	bool loadDeviceList( bool freshAll = false);
	void showNoDevice();
	void showDeviceList(bool freshAll = false);
	void showOfflineDevice();

	void updateAboutMqttStatus(std::string sn, AnkerNet::aknmt_command_type_e type);
	void updateAboutZoffsetStatus(std::string sn);
	//void updateDeviceList();
	void updateFileTransferStatus(std::string sn, int progress, AnkerNet::FileTransferResult result);

	// Notice: content and title parameter should be utf8 format
	void showMsgLevelDialog(const NetworkMsg& msg);

	void setLoadingVisible(bool visible);

	void switchDevicePage(const std::string& deviceSn);

	void activate(bool active);

protected:
	void updateDeviceStatus(const std::string& sn, AnkerNet::aknmt_command_type_e type);
	void updateZoffsetValue(const std::string& sn);
	//0 No 1 heatbed
	//void updateTemperatrue(const std::string& sn, const std::string& value, int style);	

	// video
	void setVideoCallBacks();
	//void unsetVideoCallBacks();
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
	void clearWidget();
	void initUi();
	void removeExpiredDevice();	

	void showMsgLV0Dialog(const NetworkMsg& msg);
	void showMsgLV1Dialog(const NetworkMsg& msg);
<<<<<<< HEAD
=======
	void showMsgLV3Dialog(const NetworkMsg& msg);
>>>>>>> 84b4984 (feat: 1.5.21 open source)
	void OnShow(wxShowEvent& event);

private:
	AnkerUnLoginPanel* m_pUnloginPanel = nullptr;
	AnkerNavBar* m_pNavBarWidget = nullptr;	
	AnkerEmptyDevice* m_pEmptyDeviceWidget = nullptr;

	AnkerDeviceControl* m_pOfflineDeviceWidget = nullptr;
	AnkerDeviceControl* m_pCurrentDeviceWidget = nullptr;
	std::list<AnkerDeviceControl*> m_deviceWidgetList;
	wxBoxSizer* m_pMainHSizer;
	wxBoxSizer* m_pDeviceHSizer;
	std::string	m_currentDeviceId = {""};	

	AnkerLoadingMask* m_pLoadingMask = nullptr;

	bool m_inited = false;
};


#endif


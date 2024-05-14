#include <algorithm>
#include "AnkerDevice.hpp"
#include "AnkerVideo.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "slic3r/GUI/Common/AnkerLoadingMask.hpp"

#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "wx/sizer.h"
#include <wx/event.h>
#include <wx/stattext.h>
#include <boost/bind/bind.hpp>
#include <boost/signals2/connection.hpp>
#include <wx/clntdata.h>
#include "libslic3r/Utils.hpp"
#include "I18N.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"

#include "Common/AnkerRoundBaseDialog.hpp"
#include "Common/AnkerRoundDialog.hpp"
#include "Common/AnkerMsgDialog.hpp"
#include "AnkerGCodeImportDialog.hpp"
#include "Common/AnkerRoundPanel.hpp"
#include "Common/AnkerNozzlesStausPanel.h"
#include <slic3r/GUI/GcodeVerify/PrintCheckHint.hpp>
#include "FilamentMaterialConvertor.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "DeviceObjectBase.h"
#include "AnkerNetBase.h"

using namespace AnkerNet;

wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_DEVICE_LIST_REFRESH, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ZOFFSET_CHANGED, wxCommandEvent);

wxDEFINE_EVENT(wxCUSTOMEVT_EXTRUDEX, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_RETRACT, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_STOP_EXTRUDEX, wxCommandEvent);

#define OFFILE_COLOR wxColor(105, 105, 108)

AnkerDevice::AnkerDevice(wxWindow* parent,
						wxWindowID winid /*= wxID_ANY*/,
						const wxPoint& pos /*= wxDefaultPosition*/,
						const wxSize& size /*= wxDefaultSize*/)
						: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)						
	                    , m_pLoadingMask(nullptr)
	                    , m_pCurrentDeviceWidget(nullptr)
{	
	Bind(wxEVT_SHOW, &AnkerDevice::OnShow, this);

	SetBackgroundColour(wxColour("#18191B"));
	m_pMainHSizer = new wxBoxSizer(wxHORIZONTAL);

	//don't login and show the panel
	{
		m_pUnloginPanel = new AnkerUnLoginPanel(this);
		m_pUnloginPanel->Bind(wxCUSTOMEVT_LOGIN_CLCIKED, [this](wxCommandEvent& event) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_LOGIN_CLCIKED);
			wxPostEvent(this->GetParent(), evt);
			});

		m_pMainHSizer->Add(m_pUnloginPanel, 1, wxEXPAND | wxALL, 0);
	}

	SetSizer(m_pMainHSizer);
}

AnkerDevice::~AnkerDevice()
{	
	Unbind(wxEVT_SHOW, &AnkerDevice::OnShow, this);
}

void AnkerDevice::Init()
{
	if (!m_inited) {
		setVideoCallBacks();
		initUi();
		m_inited = true;
	}
}

void AnkerDevice::showUnlogin()
{	
	m_pUnloginPanel->Show(true);
	if (m_pNavBarWidget)
		m_pNavBarWidget->Hide();
	if (m_pEmptyDeviceWidget)
		m_pEmptyDeviceWidget->Hide();
	if (m_pOfflineDeviceWidget)
		m_pOfflineDeviceWidget->Hide();
	
	clearWidget();
	Refresh();
	Layout();
}


bool AnkerDevice::loadDeviceList(bool freshAll)
{
	ANKER_LOG_INFO << "enter loadDeviceList";
	auto ankerNet = AnkerNetInst();
	if (!ankerNet || !ankerNet->IsLogined()) {
		return false;
	}

	ANKER_LOG_WARNING << "enter loadDeviceList before lock";
	wxWindowUpdateLocker updateLocker(this);
	ANKER_LOG_WARNING << "enter loadDeviceList after unlock";

	bool res = false;
	m_pCurrentDeviceWidget = nullptr;

	std::list<DeviceObjectBasePtr> deviceList = ankerNet->GetDeviceList();

	if (deviceList.size() > 0) {
		res = true;
	} else {
		showNoDevice();
		return false;
	}

	ANKER_LOG_INFO << "start delete not work list";
	removeExpiredDevice();	

	for (auto iter : deviceList) {
		auto currentSn = iter->GetSn();
		//if exist and do not add.
		if (m_pNavBarWidget->checkTabExist(currentSn))
			continue;

		if (m_pNavBarWidget->getCount() <= 0)
			m_currentDeviceId = currentSn;

		//tab
		m_pNavBarWidget->addItem(iter->GetStationName(), currentSn);

		//device control		
		wxVariant snID(currentSn);
		AnkerDeviceControl* pWidget = new AnkerDeviceControl(currentSn, this);
		pWidget->SetBackgroundColour(wxColor("#18191B"));

		pWidget->SetClientObject(new wxStringClientData(snID));

		DeviceObjectBasePtr deviceObj = ankerNet->getDeviceObjectFromSn(currentSn);
		if (deviceObj) {
			pWidget->showMulitColorNozzle(false);
		}		

		pWidget->Bind(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE, [this, pWidget, deviceObj](wxCommandEvent& event) {
			wxVariant* pData = (wxVariant*)event.GetClientData();
			if (pData)
			{
				wxVariantList list = pData->GetList();
				wxString tpValue = list[0]->GetString();
				auto tpStyle = list[1]->GetInteger();
				int tempertureValue = 0;
				if (!tpValue.ToInt(&tempertureValue))
				{
					ANKER_LOG_ERROR << "get temperture error :" << tpValue.c_str();
					return;
				}

				ANKER_LOG_INFO << "handle value " << tpValue << ", style " << tpStyle;

				if (!deviceObj) {
					return;
				}

				// 0 nozzle 1 bed
				if(tpStyle)
					deviceObj->SetTemperture(tempertureValue, -1);
				else
					deviceObj->SetTemperture(-1, tempertureValue);
			}
		});

		pWidget->Bind(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, [this, pWidget, deviceObj](wxCommandEvent& event) {
			wxVariant* pData = (wxVariant*)event.GetClientData();

			if (pData)
			{
				wxVariantList list = pData->GetList();
				wxString tpValue = list[0]->GetString();
				auto tpStyle = list[1]->GetInteger();

				int tempertureValue = 0;
				if (!tpValue.ToInt(&tempertureValue))
				{
					ANKER_LOG_ERROR << "get temperture error :" << tpValue.c_str();
					return;
				}

				ANKER_LOG_INFO << "handle value " << tpValue << ", style " << tpStyle;
				
				if (!deviceObj) {
					return;
				}

				// 0 nozzle 1 bed
				if (tpStyle)
					deviceObj->SetTemperture(tempertureValue, -1);
				else
					deviceObj->SetTemperture(-1, tempertureValue);
			}
		});

		pWidget->SetMinSize(AnkerSize(1200, 750));

		auto temp = static_cast<int>(iter->GetNozzleMaxTemp());
		pWidget->setNozzleMaxTemp(temp);

		//add layout		
		int couts = m_pDeviceHSizer->GetItemCount();
		m_pDeviceHSizer->Add(pWidget, 17, wxEXPAND, 0);
		pWidget->Hide();
		m_deviceWidgetList.push_back(pWidget);
	}

	if (res)
	{
		ANKER_LOG_INFO << "load devs and show devs list";
		showDeviceList(freshAll);
	}
	else
	{
		ANKER_LOG_INFO << "no devs and show no dev";
		showNoDevice();
	}

	return res;
}

void AnkerDevice::showNoDevice()
{
	m_pUnloginPanel->Hide();
	m_pNavBarWidget->Show(true);
	m_pNavBarWidget->showEmptyPanel(true);
	m_pEmptyDeviceWidget->Show(true);
	m_pOfflineDeviceWidget->Hide();

	for (auto iter : m_deviceWidgetList)
	{
		iter->Hide();
	}	

	m_pCurrentDeviceWidget = nullptr;

	Layout();
}


void AnkerDevice::showDeviceList(bool freshAll)
{	
	//wxWindowUpdateLocker updateLocker(this);

 	m_pUnloginPanel->Hide();

 	m_pEmptyDeviceWidget->Hide();
 	m_pOfflineDeviceWidget->Hide();
	
	m_pNavBarWidget->Show(true);
	m_pNavBarWidget->showEmptyPanel(false);

	auto ankerNet = AnkerNetInst();

	bool lastIndexIsValid = false;
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == m_currentDeviceId)
		{
			wxString deviceWidgetId = pIterSnId->GetData().ToStdString();
			m_pCurrentDeviceWidget = iter;
			
			DeviceObjectBasePtr deviceObj = ankerNet->getDeviceObjectFromSn(widgetId);
			if (!deviceObj) {
				break;
			}

			if (deviceObj->IsOnlined())
			{
				iter->showMulitColorNozzle(false);
				iter->Show(true);
				m_pNavBarWidget->setTabOnlineStatus(true, deviceWidgetId);
			}
			else
			{
				if (!m_pOfflineDeviceWidget->IsShown())
				{
					m_pNavBarWidget->setTabOnlineStatus(false, deviceWidgetId);					
					m_pOfflineDeviceWidget->setIsSingleNozOfflineStatus(true);					
					m_pOfflineDeviceWidget->Show();
				}
			}			
			lastIndexIsValid = true;
			break;
		}
	}
	if (!lastIndexIsValid)
	{
		if (m_deviceWidgetList.size() > 0)
		{
			auto iter = m_deviceWidgetList.begin();
			wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
			DeviceObjectBasePtr deviceObj = ankerNet->getDeviceObjectFromSn(m_currentDeviceId);
			if (pIterSnId && deviceObj)
			{
				wxString widgetId = pIterSnId->GetData().ToStdString();
				m_currentDeviceId = widgetId.ToStdString();
				
				if (deviceObj->IsOnlined())
				{
					(*iter)->Show();
					m_pNavBarWidget->setTabOnlineStatus(true, widgetId);
				}
				else
				{
					m_pNavBarWidget->setTabOnlineStatus(false, widgetId);
					m_pOfflineDeviceWidget->setIsSingleNozOfflineStatus(true);
					m_pOfflineDeviceWidget->Show();
				}				
			}
			else
			{
				if(pIterSnId)
					m_currentDeviceId = pIterSnId->GetData().ToStdString();
			}
				
		}
	}

	if (freshAll) {
		for (auto iter : m_deviceWidgetList) {
			if (!iter || !m_pNavBarWidget) {
				continue;
			}
			wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
			if (pIterSnId) {
				auto widgetId = pIterSnId->GetData().ToStdString();

				auto* ankerNet = AnkerNetInst();
				if (ankerNet) {
					DeviceObjectBasePtr deviceObj = ankerNet->getDeviceObjectFromSn(widgetId);
					if (deviceObj) {
						m_pNavBarWidget->setTabOnlineStatus(deviceObj->IsOnlined(), widgetId);
					}
				}
			}
		}
	}

	m_pNavBarWidget->switchTabFromSn(m_currentDeviceId);
	showDevice(m_currentDeviceId);
	Layout();
	Refresh();	
}


void AnkerDevice::showOfflineDevice()
{	
	m_pNavBarWidget->Show(true);
	m_pOfflineDeviceWidget->Show(true);

	for (auto iter : m_deviceWidgetList)
	{
		iter->Hide();
	}

	m_pCurrentDeviceWidget = nullptr;
	
}

// update the sn device by mqtt server signal
void AnkerDevice::updateAboutMqttStatus(std::string sn, ::aknmt_command_type_e type)
{
	updateDeviceStatus(sn, type);
}

void AnkerDevice::updateAboutZoffsetStatus(std::string sn)
{
	updateZoffsetValue(sn);
}

void AnkerDevice::updateDeviceStatus(const std::string& sn, ::aknmt_command_type_e type)
{	
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->updateDeviceStatus(sn, type);
		}
	}
}

void AnkerDevice::updateZoffsetValue(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->updateZoffsetValue(sn);
		}

	}
}

void AnkerDevice::updateFileTransferStatus(std::string sn, int progress, FileTransferResult result)
{
	if (sn != m_currentDeviceId)
		return;

	for (auto iter : m_deviceWidgetList) {
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn) {
			iter->updateFileTransferStatus(sn, progress, result);
		}
	}
}

void AnkerDevice::showMsgLevelDialog(const NetworkMsg& msg)
{
	ANKER_LOG_INFO << "show msg: " << msg.sn << ", " << msg.type << ", level: " << msg.level;
	switch (msg.level)
	{			  
	case LEVEL_URGENCY:	 showMsgLV0Dialog(msg); break;
	case LEVEL_NORMAL1:	 showMsgLV1Dialog(msg); break;
	default:
		break;
	}	
}

void AnkerDevice::showMsgLV0Dialog(const NetworkMsg& msg)
{
	bool isCancel = msg.haveCancel;
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, msg.context, msg.title, isCancel);
	if (result == AnkerMsgDialog::MSG_OK) {
		auto ankerNet = AnkerNetInst();
		if (!ankerNet) {
			return;
		}
		if (!msg.sn.empty()) {
			DeviceObjectBasePtr deviceObject = ankerNet->getDeviceObjectFromSn(msg.sn);
			if (deviceObject) {
				ANKER_LOG_INFO << "clear the exception for " << msg.sn << ", "
					<< msg.type << ", " << msg.context;
				deviceObject->clearDeviceExceptionInfo();
			}
		}
	}

	DatamangerUi::GetInstance().ShowNextAlertMsg();
}

void AnkerDevice::showMsgLV1Dialog(const NetworkMsg& msg)
{
	for (auto iter : m_deviceWidgetList)
	{
		if (iter->getCurrentDeviceId() == msg.sn) {
			iter->showMsgFrame(msg);
		}
	}
}

void AnkerDevice::OnShow(wxShowEvent& event)
{
	activate(event.IsShown());
}

void AnkerDevice::displayVideoFrame(std::string& sn, const unsigned char* imgData, int width, int height)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->displayVideoFrame(imgData, width, height);
		}
	}
}


void AnkerDevice::onP2pvideoStreamSessionInit(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onP2pvideoStreamSessionInit();
		}
	}
}

void AnkerDevice::onRecvVideoStreamClosing(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onRecvVideoStreamClosing();
		}
	}
}

void AnkerDevice::onP2pvideoStreamSessionClosed(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onP2pvideoStreamSessionClosed();
		}
	}
}

void AnkerDevice::onRcvP2pVideoStreamCtrlAbnomal(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onRcvP2pVideoStreamCtrlAbnomal();
		}
	}
}

void AnkerDevice::onRecvCameraLightStateNotify(const std::string& sn, bool onOff)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onRecvCameraLightStateNotify(onOff);
		}
	}
}

void AnkerDevice::onRecVideoModeNotify(const std::string& sn, int mod)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->onRecVideoModeNotify(mod);
		}
	}
}


void AnkerDevice::setVideoCallBacks()
{
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	SnImgCallBackFunc cb = std::bind(&AnkerDevice::displayVideoFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	ankerNet->setVideoFrameDataReadyCallBack(cb);

	SnCallBackFunc cb1 = std::bind(&AnkerDevice::onP2pvideoStreamSessionInit, this, std::placeholders::_1);
	ankerNet->setP2PVideoStreamSessionInitedCallBack(cb1);

	SnCallBackFunc cb2 = std::bind(&AnkerDevice::onRecvVideoStreamClosing, this, std::placeholders::_1);
	ankerNet->setP2PVideoStreamSessionClosingCallBack(cb2);

	SnCallBackFunc cb3 = std::bind(&AnkerDevice::onP2pvideoStreamSessionClosed, this, std::placeholders::_1);
	ankerNet->setP2PVideoStreamSessionClosedCallBack(cb3);

	SnCallBackFunc cb4 = std::bind(&AnkerDevice::onRcvP2pVideoStreamCtrlAbnomal, this, std::placeholders::_1);
	ankerNet->setP2PVideoStreamCtrlAbnormalCallBack(cb4);

	SnStateCallBackFunc cb5 = std::bind(&AnkerDevice::onRecvCameraLightStateNotify, this, std::placeholders::_1, std::placeholders::_2);
	ankerNet->setCameraLightStateCallBack(cb5);

	SnStateCallBackFunc cb6 = std::bind(&AnkerDevice::onRecVideoModeNotify, this, std::placeholders::_1, std::placeholders::_2);
	ankerNet->setVideoModeCallBack(cb6);
}

//void AnkerDevice::unsetVideoCallBacks()
//{
//	AnkerNetBase* ankerNet = AnkerNetInst();
//	if (ankerNet) {
//		ankerNet->unsetAllVideoCallBacks();
//	}
//}

void AnkerDevice::showDevice(const std::string& snId)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		DeviceObjectBasePtr deviceObj = ankerNet->getDeviceObjectFromSn(widgetId);

		if (!deviceObj)
		{
			ANKER_LOG_ERROR << "showDevice fail for sn:" << widgetId;
			continue;
		}

		if (widgetId == snId)
		{
			m_pCurrentDeviceWidget = iter;
			if (deviceObj->IsOnlined())
			{
				m_pOfflineDeviceWidget->Hide();
				iter->Show(true);

				m_pNavBarWidget->setTabOnlineStatus(true, widgetId);
			}
			else
			{				
				ANKER_LOG_INFO << "devs offline";
				showOfflineDevice();				
				m_pOfflineDeviceWidget->setIsSingleNozOfflineStatus(true);				
							
				m_pNavBarWidget->setTabOnlineStatus(false, widgetId);
			}
		}
		else
			iter->Hide();
	}	

	Layout();
	Refresh();
}


void AnkerDevice::clearWidget()
{
	if (m_pNavBarWidget)
		m_pNavBarWidget->clearItem();	

	int widgetCount = m_deviceWidgetList.size();

	for(int i=0; i < widgetCount; i++)
		if(m_pDeviceHSizer && m_pDeviceHSizer->GetItemCount() >= 4)
			m_pDeviceHSizer->Remove(4);

	for (auto iter : m_deviceWidgetList)
	{
		delete iter;
	}

	m_pCurrentDeviceWidget = nullptr;

	m_deviceWidgetList.clear();
}

void AnkerDevice::initUi()
{
	//logined and no device, has device by the navbarwdiget control
	{
		m_pDeviceHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pNavBarWidget = new AnkerNavBar(this);
		m_pNavBarWidget->Bind(wxCUSTOMEVT_SWITCH_DEVICE, [this](wxCommandEvent& event) {												
			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
			if(pData)
			{
				wxWindowUpdateLocker updateLocker(this);
				std::string snId = pData->GetData().ToStdString();
				m_currentDeviceId = snId;
				showDevice(snId);
			}
		});

		//update device list
		m_pNavBarWidget->Bind(wxCUSTOMEVT_BTN_CLICKED, [this](wxCommandEvent& event) {			
			auto ankerNet = AnkerNetInst();
			if (ankerNet && ankerNet->IsLogined()) {
				ankerNet->AsyRefreshDeviceList();
			}			
		});

		m_pNavBarWidget->SetMinSize(AnkerSize(215, 750));		
		m_pNavBarWidget->Hide();
		m_pNavBarWidget->showEmptyPanel(true);

		m_pEmptyDeviceWidget = new AnkerEmptyDevice(this);		
		m_pEmptyDeviceWidget->Hide();
		m_pOfflineDeviceWidget = new AnkerDeviceControl("", this);
		m_pOfflineDeviceWidget->SetBackgroundColour(wxColour("#18191B"));
		m_pOfflineDeviceWidget->setIsSingleNozOfflineStatus(true);
		m_pOfflineDeviceWidget->Hide();

		m_pDeviceHSizer->Add(m_pNavBarWidget, 0, wxEXPAND | wxALL, 12);
		m_pDeviceHSizer->Add(m_pEmptyDeviceWidget, 1, wxEXPAND, 0);
		m_pDeviceHSizer->Add(m_pOfflineDeviceWidget, 1, wxEXPAND, 0);

		m_pMainHSizer->Add(m_pDeviceHSizer, 1, wxEXPAND | wxALL, 2);
	}	
}

void AnkerDevice::removeExpiredDevice()
{
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;		
	}

	//clear ExpiredDevice tab Widget
	m_pNavBarWidget->clearExpiredTab(m_currentDeviceId);

	auto tempDeviceList = m_deviceWidgetList;

	//clear ExpiredDevice Widget
	for (auto iter = m_deviceWidgetList.begin(); iter != m_deviceWidgetList.end(); ) 
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		if (pIterSnId)
		{
			std::string widgetId = pIterSnId->GetData().ToStdString();
			if (!ankerNet->getDeviceObjectFromSn(widgetId))
			{								
				for (int i = 0; i < m_pDeviceHSizer->GetItemCount(); i++)
				{
					if (m_pDeviceHSizer->GetItem(i)->GetWindow() == (*iter))
					{
						m_pDeviceHSizer->Remove(i);
						break;
					}
				}
				
				delete* iter;
				iter = m_deviceWidgetList.erase(iter);
			}
			else
			{
				(*iter)->Hide();
				++iter;
			}
		}		
	}		
}

void AnkerDevice::setLoadingVisible(bool visible)
{
	if (m_pLoadingMask == nullptr)
	{
		m_pLoadingMask = new AnkerLoadingMask(this);		
	}

	m_pLoadingMask->SetSize(GetSize());
	m_pLoadingMask->SetPosition(wxPoint(0, 0));
	m_pLoadingMask->Show(visible);

}
void AnkerDevice::switchDevicePage(const std::string& deviceSn)
{
	m_currentDeviceId = deviceSn;
	showDevice(m_currentDeviceId);
	m_pNavBarWidget->switchTabFromSn(deviceSn);
}
void AnkerDevice::activate(bool active)
{
	if (m_pCurrentDeviceWidget)
	{
		m_pCurrentDeviceWidget->activate(active);
	}
}

AnkerControlWidget::AnkerControlWidget(const std::string& deviceSn, wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: m_currentDeviceSn(deviceSn),
	wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerControlWidget::~AnkerControlWidget()
{
	
}

int AnkerControlWidget::getCurrentWorkNozzle()
{
	if (!m_isMulitColor)
		return 0;

	for (auto item : m_btnList)
	{
		if (item->getBtnStatus() == BOX_SELECT || item->getBtnStatus() == BOX_WORKING)
		{
			//auto data = item->GetClientData();
			wxVariant* pData = (wxVariant*)item->GetClientData();			
			if (pData)
			{
				wxVariantList list = pData->GetList();				

				wxString strData = list[0]->GetString();
				
				int index = 0;
				if (strData.ToInt(&index))
				{
					return index;
				}
				else
				{
					ANKER_LOG_ERROR << "get current Num index error for: " << strData;
					return 0;
				}
			}
		}
	}
	return 0;
}

void AnkerControlWidget::updateNozzleStatus(const std::string& sn)
{	
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(sn);
	if (!devceiObj)
	{
		return;
	}
	
	wxWindowUpdateLocker updateLocker(this);
	int CurrentNozzleIndex = getCurrentWorkNozzle();
}

void AnkerControlWidget::updateTemperatrue(const std::string& sn)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(sn);
	if (!devceiObj)
		return;

	int CurrentNozzleIndex = getCurrentWorkNozzle();

	//single color index is 0
	auto nozzData = devceiObj->GetNozzleTemperature(CurrentNozzleIndex);

	wxString nozzleNum = "0";
	wxString desNozzleNum = "0";

	if (!nozzData.isNull)
	{
		nozzleNum = wxString::Format(wxT("%d"), nozzData.currentTemp);
		desNozzleNum = wxString::Format(wxT("%d"), nozzData.targetTemp);
	}

	wxString hotdBedNum = wxString::Format(wxT("%d"), devceiObj->GetHotBedCurrentTemperature());	
	wxString desHotdBedNum = wxString::Format(wxT("%d"), devceiObj->GetHotBedTargetTemperature());

	//current temperature  
	m_pNozWidegt->setTemperatureNum(nozzleNum);
	m_pHeatBedWidegt->setTemperatureNum(hotdBedNum);

	wxString strNo = wxString(_L("common_print_control_nozzle"));
	wxString strheatbed = wxString(_L("common_print_control_headbed"));

	if (desNozzleNum == "0")
	{
		m_pNozWidegt->setTipsText(strNo + ":--" + _L("#"));
	}
	else
	{
		m_pNozWidegt->setTipsText(strNo + ":" + desNozzleNum + _L("#"));
	}	

	if (desHotdBedNum == "0")
	{
		m_pHeatBedWidegt->setTipsText(strheatbed + ":--" + _L("#"));
	}
	else
	{
		m_pHeatBedWidegt->setTipsText(strheatbed + ":" + desHotdBedNum + _L("#"));
	}			
}

void AnkerControlWidget::updateZoffesetValue(const std::string& sn)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(sn);
	if (!devceiObj)
		return;

	float zoffsetValue = devceiObj->getZAxisCompensationValue();
	
	wxString strValue = wxString::Format(wxT("%0.2f"), zoffsetValue);
	m_pOffsetLineEdit->SetValue(strValue);
}

void AnkerControlWidget::showEditTempertrueBtn(bool show)
{
	m_pNozWidegt->showEditTempertrueBtn(show);
	m_pHeatBedWidegt->showEditTempertrueBtn(show);
}


void AnkerControlWidget::updateWorkStatus(bool iswork)
{
	m_pNozWidegt->updateWorkStatus(iswork);
	m_pHeatBedWidegt->updateWorkStatus(iswork);
}


void AnkerControlWidget::onMaterilBtnClicked(wxCommandEvent& event)
{
	if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
		return;
	}

	auto iter = m_btnList.begin();
	AnkerMerterialBoxBtn* pBtn = wxDynamicCast(event.GetEventObject(), AnkerMerterialBoxBtn);
	int index = 0;
	int nozzelNum = 0;
	while (iter != m_btnList.end())
	{

		(*iter)->reSetBtnStatus(BOX_NOR);
		if (pBtn == (*iter))
			nozzelNum = index;
		index++;
		++iter;

	}
	
	if(pBtn)
		pBtn->setBtnStatus(BOX_SELECT);

	wxString labelValue =  wxString::Format(wxT("%d"), nozzelNum + 1) + "#";
	m_pNozzelNum->SetLabel(labelValue);
	
	auto devceiObj = CurDevObject(m_currentDeviceSn);
	if (!devceiObj)
		return;

	if(nozzelNum >=0 && nozzelNum <= 5)
		devceiObj->NozzleSwitch(nozzelNum);
}


void AnkerControlWidget::showMulitColorNozzle(bool isShow)
{
	wxWindowUpdateLocker updateLocker(this);
	m_isMulitColor = isShow;

	if (isShow)
	{		
		m_pDeviceImg->Hide();

		if (m_pOffsetPanel)
			m_pOffsetPanel->Hide();

		if (m_pLengthPanel)
			m_pLengthPanel->Hide();

		if (m_pNumLine)
			m_pNumLine->Hide();

		m_pPrintNozzlesWidget->Show();
		m_pPrintNozzlesWidget->Update();
		m_pPrintNozzlesWidget->Layout();		
		m_pNumPanel->Show();
	}
	else
	{
		m_pDeviceImg->Show();

		if(m_pOffsetPanel)
			m_pOffsetPanel->Show();

		if (m_pLengthPanel)
			m_pLengthPanel->Show();

		if (m_pNumLine)
			m_pNumLine->Show();
		
		m_pPrintNozzlesWidget->Hide();
		m_pNumPanel->Hide();
		
	}

	Refresh();
	Layout();
}


void AnkerControlWidget::setIsSingleNozOfflineStatus(bool isSimpleNozOffline)
{	
	m_pNozWidegt->setOfflineStatus();
	m_pHeatBedWidegt->setOfflineStatus();

	m_pRetractBtn->Enable(false);
	m_pRetractBtn->SetBgDisableColor(wxColor(46, 47, 50));
	m_pRetractBtn->SetBackgroundColour(wxColor(46, 47, 50));
	m_pRetractBtn->SetTextColor(OFFILE_COLOR);

	m_pExtrudeBtn->Enable(false);
	m_pExtrudeBtn->SetBgDisableColor(wxColor(46, 47, 50));
	m_pExtrudeBtn->SetBackgroundColour(wxColor(46, 47, 50));
	m_pExtrudeBtn->SetTextColor(OFFILE_COLOR);

	m_pMaterialLengthLabel->SetForegroundColour(OFFILE_COLOR);

	m_pOffsetLineEdit->Enable(false);
	m_pOffsetLineEdit->SetValue("--");
	m_pOffsetLineEdit->setLineEditTextColor(OFFILE_COLOR);
	m_pOffsetLineEdit->setLineUnitTextColor(OFFILE_COLOR);

	m_pLengthLineEdit->Enable(false);
	m_pLengthLineEdit->SetValue("40");
	m_pLengthLineEdit->setLineEditTextColor(OFFILE_COLOR);
	m_pLengthLineEdit->setLineUnitTextColor(OFFILE_COLOR);

	m_pdeduceBtn->Enable(false);
	m_pAddBtn->Enable(false);

	m_pdeduceBtn->SetNorTextColor(wxColor(105, 105, 108));
	m_pdeduceBtn->SetDisableTextColor(wxColor(105, 105, 108));

	m_pdeduceBtn->SetBgNorColor(wxColor(41, 42, 45));
	m_pdeduceBtn->SetBgHoverColor(wxColor(41, 42, 45));
	m_pdeduceBtn->SetBgPressedColor(wxColor(41, 42, 45));
	m_pdeduceBtn->SetBgDisableColor(wxColor(41, 42, 45));

	m_pdeduceBtn->SetborderNorColor(wxColor(49, 50, 53));
	m_pdeduceBtn->SetborderHoverBGColor(wxColor(49, 50, 53));
	m_pdeduceBtn->SetborderPressedBGColor(wxColor(49, 50, 53));
	//m_pdeduceBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
	m_pdeduceBtn->SetborderDisableBGColor(wxColor(67, 68, 71));

	m_pAddBtn->SetBgNorColor(wxColor(41, 42, 45));
	m_pAddBtn->SetBgHoverColor(wxColor(41, 42, 45));
	m_pAddBtn->SetBgPressedColor(wxColor(41, 42, 45));
	m_pAddBtn->SetBgDisableColor(wxColor(41, 42, 45));

	m_pAddBtn->SetNorTextColor(wxColor(105, 105, 108));
	m_pAddBtn->SetDisableTextColor(wxColor(105, 105, 108));
	m_pAddBtn->SetborderNorColor(wxColor(49, 50, 53));
	m_pAddBtn->SetborderHoverBGColor(wxColor(49, 50, 53));
	m_pAddBtn->SetborderPressedBGColor(wxColor(49, 50, 53));
	//m_pAddBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
	m_pAddBtn->SetborderDisableBGColor(wxColor(67, 68, 71));
	

	m_pBtn1->setBtnStatus(BOX_OFFLINE);
	m_pBtn2->setBtnStatus(BOX_OFFLINE);
	m_pBtn3->setBtnStatus(BOX_OFFLINE);
	m_pBtn4->setBtnStatus(BOX_OFFLINE);
	m_pBtn5->setBtnStatus(BOX_OFFLINE);
	m_pBtn6->setBtnStatus(BOX_OFFLINE);

	m_pBtn1->setMaterialName("--");
	m_pBtn2->setMaterialName("--");
	m_pBtn3->setMaterialName("--");
	m_pBtn4->setMaterialName("--");
	m_pBtn5->setMaterialName("--");
	m_pBtn6->setMaterialName("--");

	if (isSimpleNozOffline)
	{		
		m_pDeviceImg->Show();
		m_pNumLine->Show();
		m_pLengthPanel->Show();
		m_pNumPanel->Hide();
		m_pOffsetPanel->Show();
		m_pPrintNozzlesWidget->Hide();
				
	}
	else//mulit color
	{
		m_pDeviceImg->Hide();						
		m_pNumPanel->Show();
		m_pOffsetPanel->Hide();		
		m_pLengthPanel->Hide();
		m_pNumLine->Hide();
		m_pPrintNozzlesWidget->Show();
	}

	Refresh();
	Layout();
}


void AnkerControlWidget::setZoffsetDisabel(bool isAble)
{
	if (isAble)
	{
		m_pOffsetLineEdit->Enable(true);
		m_pOffsetLineEdit->setLineEditTextColor("#FFFFFF");		

		m_pdeduceBtn->Enable(true);
		m_pAddBtn->Enable(true);
	
		m_pdeduceBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pdeduceBtn->SetNorTextColor(wxColor(242, 242, 242));
		m_pdeduceBtn->SetborderNorColor(wxColor(67, 68, 71));
		m_pdeduceBtn->SetborderHoverBGColor(wxColor(169, 170, 171));
		m_pdeduceBtn->SetborderPressedBGColor(wxColor(169, 170, 171));
		//m_pdeduceBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pdeduceBtn->SetborderDisableBGColor(wxColor(67, 68, 71));
		
		m_pAddBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pAddBtn->SetNorTextColor(wxColor(242, 242, 242));
		m_pAddBtn->SetborderNorColor(wxColor(67, 68, 71));
		m_pAddBtn->SetborderHoverBGColor(wxColor(169, 170, 171));
		m_pAddBtn->SetborderPressedBGColor(wxColor(169, 170, 171));
		//m_pAddBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pAddBtn->SetborderDisableBGColor(wxColor(67, 68, 71));
	}
	else
	{
		m_pOffsetLineEdit->Enable(false);		
		m_pOffsetLineEdit->setLineEditTextColor(OFFILE_COLOR);
		m_pOffsetLineEdit->setLineUnitTextColor(OFFILE_COLOR);

		m_pdeduceBtn->Enable(false);
		m_pAddBtn->Enable(false);
				
		m_pdeduceBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pdeduceBtn->SetNorTextColor(wxColor(105, 105, 108));
		m_pdeduceBtn->SetDisableTextColor(wxColor(105, 105, 108));
		m_pdeduceBtn->SetborderNorColor(wxColor(49, 50, 53));
		m_pdeduceBtn->SetborderHoverBGColor(wxColor(49, 50, 53));
		m_pdeduceBtn->SetborderPressedBGColor(wxColor(49, 50, 53));
		//m_pdeduceBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pdeduceBtn->SetborderDisableBGColor(wxColor(67, 68, 71));

		m_pAddBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pAddBtn->SetNorTextColor(wxColor(105, 105, 108));
		m_pAddBtn->SetDisableTextColor(wxColor(105, 105, 108));
		m_pAddBtn->SetborderNorColor(wxColor(49, 50, 53));
		m_pAddBtn->SetborderHoverBGColor(wxColor(49, 50, 53));
		m_pAddBtn->SetborderPressedBGColor(wxColor(49, 50, 53));
		//m_pAddBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pAddBtn->SetborderDisableBGColor(wxColor(67, 68, 71));
	}
}

void AnkerControlWidget::setExtrudeAble(bool isAble, int statusValue /*= 0*/)
{
	if (isAble)
	{
		if (statusValue == 0)
		{
			resetExtrudeBtns();
		}
		else if (statusValue == 2 || statusValue == 1)
		{
			showStopBtnStatus(m_pExtrudeBtn);
			showDisAbelBtnStatus(m_pRetractBtn);
			m_extrudeStatus = EXTRUDING;
		}
		else if (statusValue == 3)
		{
			showStopBtnStatus(m_pRetractBtn);
			showDisAbelBtnStatus(m_pExtrudeBtn);
			m_extrudeStatus = EXTRUDE_RETACTING;
		}

		m_pLengthLineEdit->Enable(true);		
		m_pLengthLineEdit->setLineEditTextColor("#FFFFFF");
	}
	else
	{	
		m_pLengthLineEdit->Enable(false);		
		m_pLengthLineEdit->setLineEditTextColor(OFFILE_COLOR);
		m_pLengthLineEdit->setLineUnitTextColor(OFFILE_COLOR);
		showDisAbelBtnStatus(m_pExtrudeBtn);
		showDisAbelBtnStatus(m_pRetractBtn);	    		
	}	
}

void AnkerControlWidget::setNozsAble(bool isAble)
{
	MaterialBoxStatus status = BOX_NOR;
	if (!isAble)
		status = BOX_DISWORK;
	for (auto item : m_btnList)
	{
		item->setBtnStatus(status);
	}
}


void AnkerControlWidget::accurateCalculation(wxString& data)
{
	double dValue = 0;

	if (data.ToCDouble(&dValue))
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << dValue; 
		data = wxString::FromAscii(stream.str().c_str());  
	}
	else
	{
		data = "0";
	}
}

void AnkerControlWidget::resetExtrudeBtns()
{
	m_extrudeStatus = EXTRUDE_FREE;
	showNorBtnStatus(m_pRetractBtn, _L("common_print_controlbutton_retract"));
	showNorBtnStatus(m_pExtrudeBtn, _L("common_print_controlbutton_extrude"));
}

void AnkerControlWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));

	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	//title	;
	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel* pTitleWidget = new wxPanel(this);
	pTitleWidget->SetBackgroundColour(wxColour("#292A2D"));
	pTitleWidget->SetMinSize(AnkerSize(450, 30));

	m_title = new wxStaticText(this, wxID_ANY, _L("common_print_control_title"));			
	m_title->SetFont(ANKER_BOLD_FONT_NO_1);
	m_title->SetSize(AnkerSize(100, 30));
	m_title->SetMinSize(AnkerSize(100, 30));
	m_title->SetForegroundColour(wxColour("#FFFFFF"));

	m_pNumPanel = new wxPanel(this,wxID_ANY);
	m_pNumPanel->SetMinSize(AnkerSize(35, 20));
	m_pNumPanel->SetMaxSize(AnkerSize(35, 20));
	wxBoxSizer *pMulitNumHSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pNumPanel->SetSizer(pMulitNumHSizer);
	m_pNumPanel->Hide();
	
	m_pNozzelNum = new wxStaticText(m_pNumPanel, wxID_ANY, "1#", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
	m_pNozzelNum->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pNozzelNum->SetSize(AnkerSize(16, 20));
	m_pNozzelNum->SetMinSize(AnkerSize(16, 20));
	m_pNozzelNum->SetMaxSize(AnkerSize(16, 20));
	m_pNozzelNum->SetForegroundColour(wxColour(179, 179, 181));	

	m_pNumPanel->SetBackgroundColour(wxColour(65, 66, 69));

	pMulitNumHSizer->AddStretchSpacer(1);
	pMulitNumHSizer->Add(m_pNozzelNum, wxALIGN_TOP, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMulitNumHSizer->AddStretchSpacer(1);

	pTitleHSizer->AddSpacer(5);
	pTitleHSizer->Add(m_title, wxEXPAND, wxEXPAND | wxALL, 7);
	pTitleHSizer->AddStretchSpacer(1);
	pTitleHSizer->Add(m_pNumPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddSpacer(AnkerLength(7));

	wxPanel* pLine = new wxPanel(this);
	pLine->SetBackgroundColour(wxColour("#3E3F42"));
	pLine->SetMinSize(wxSize(2000, 1));
	pLine->SetMaxSize(wxSize(2000, 1));

	pTitleWidget->SetSizer(pTitleHSizer);
	pMainVSizer->Add(pTitleWidget, 0, wxEXPAND, 0);
	pMainVSizer->Add(pLine, 0, wxEXPAND, 0);

	wxPanel* pBodyPanel = new wxPanel(this);
	pBodyPanel->SetMinSize(AnkerSize(450, 75));
	pBodyPanel->SetBackgroundColour(wxColour("#292A2D"));	
	wxBoxSizer* pBodyHSizer = new wxBoxSizer(wxHORIZONTAL);

	//content
	{
		m_pNozWidegt = new AnkerTemperatureWidget(m_currentDeviceSn, pBodyPanel);	
		m_pNozWidegt->SetMaxSize(AnkerSize(130,70));
		m_pNozWidegt->SetMinSize(AnkerSize(130, 70));
		m_pNozWidegt->SetSize(AnkerSize(130, 70));
		//m_pNozWidegt->SetBackgroundColour(wxColour("#FF8C00"));
		m_pNozWidegt->setMinRage(0);
		m_pNozWidegt->setMaxRage(280);
		m_pNozWidegt->setHintSize("0~280");

		m_pNozWidegt->Bind(wxCUSTOMEVT_TEMPERATURE_UPDATE, [this](wxCommandEvent& event) {			
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE);
			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());				
			if (pData)
			{
				std::string numTpData = pData->GetData().ToStdString();					

				wxVariant eventData;
				eventData.ClearList();
				eventData.Append(wxVariant(numTpData));
				eventData.Append(wxVariant(0));
				evt.SetClientData(new wxVariant(eventData));
				ProcessEvent(evt);					
			}
		});

		wxString tipsLabel = wxString(_L("common_print_control_nozzle"));
		m_pNozWidegt->setTipsText(tipsLabel + ":--");
		//m_pNozWidegt->SetSize(wxSize(170, 110));

		m_pHeatBedWidegt = new AnkerTemperatureWidget(m_currentDeviceSn, pBodyPanel);
		m_pHeatBedWidegt->SetMaxSize(AnkerSize(130, 70));
		m_pHeatBedWidegt->SetMinSize(AnkerSize(130, 70));
		m_pHeatBedWidegt->SetSize(AnkerSize(130, 70));
		//m_pHeatBedWidegt->SetBackgroundColour(wxColour("#FF8C00"));
		m_pHeatBedWidegt->setMinRage(0);
		m_pHeatBedWidegt->setMaxRage(100);
		m_pHeatBedWidegt->setHintSize("0~100");
		m_pHeatBedWidegt->Bind(wxCUSTOMEVT_TEMPERATURE_UPDATE, [this](wxCommandEvent& event) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE);
			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
			if (pData)
			{
				std::string numTpData = pData->GetData().ToStdString();

				wxVariant eventData;
				eventData.ClearList();
				eventData.Append(wxVariant(numTpData));
				eventData.Append(wxVariant(1));
				evt.SetClientData(new wxVariant(eventData));
				ProcessEvent(evt);
			}
		});

		wxString strHeatBed = wxString(_L("common_print_control_headbed"));
		m_pHeatBedWidegt->setTipsText(strHeatBed + ":--");
		//m_pHeatBedWidegt->SetMinSize(wxSize(170, 110));

		pBodyHSizer->AddStretchSpacer(1);
		//pBodyHSizer->AddSpacer(65);
		pBodyHSizer->Add(m_pNozWidegt, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pBodyHSizer->AddSpacer(40);
		pBodyHSizer->Add(m_pHeatBedWidegt, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		//pBodyHSizer->AddSpacer(65);
		pBodyHSizer->AddStretchSpacer(1);
	}

	pBodyPanel->SetSizer(pBodyHSizer);

	//spacing
	pMainVSizer->AddSpacer(12);
	pMainVSizer->Add(pBodyPanel, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
	pMainVSizer->AddSpacer(12);
	{
		wxSizer* spacingHSizer = new wxBoxSizer(wxHORIZONTAL);

		wxPanel* pLine = new wxPanel(this);
		pLine->SetBackgroundColour(wxColour("#3E3F42"));
		pLine->SetMinSize(AnkerSize(450, 1));
		pLine->SetMaxSize(AnkerSize(450, 1));
		pLine->SetSize(AnkerSize(450, 1));

		spacingHSizer->AddSpacer(12);
		spacingHSizer->Add(pLine, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		spacingHSizer->AddSpacer(12);

		pMainVSizer->Add(spacingHSizer);
	}

	{
		wxBoxSizer* pHSizer = new wxBoxSizer(wxHORIZONTAL);

		wxBoxSizer* pLeftSizer = new wxBoxSizer(wxVERTICAL);

		wxImage deviceImage = wxImage(wxString::FromUTF8(Slic3r::var("device_nozzle.png")), wxBITMAP_TYPE_PNG);
		deviceImage.Rescale(150, 150, wxIMAGE_QUALITY_HIGH);
		wxBitmap deviceBitmap(deviceImage);		
		m_pDeviceImg = new wxStaticBitmap(this, wxID_ANY, deviceBitmap);		

		m_pPrintNozzlesWidget = new wxPanel(this, wxID_ANY);		
		m_pPrintNozzlesWidget->SetMaxSize(AnkerSize(190, 145));
		m_pPrintNozzlesWidget->SetMinSize(AnkerSize(190, 145));
		m_pPrintNozzlesWidget->SetSize(AnkerSize(190, 145));
		wxBoxSizer* pWidgetVSizer = new wxBoxSizer(wxVERTICAL);
		m_pPrintNozzlesWidget->SetSizer(pWidgetVSizer);

		wxBoxSizer* pRowAHSizer = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* pRowBHSizer = new wxBoxSizer(wxHORIZONTAL);

		pWidgetVSizer->AddSpacer(5);
		pWidgetVSizer->Add(pRowAHSizer);
		pWidgetVSizer->AddSpacer(10);
		pWidgetVSizer->Add(pRowBHSizer);
		pWidgetVSizer->AddSpacer(5);

		wxCursor handCursor(wxCURSOR_HAND);				

		m_pBtn1 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));
		m_pBtn3 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));
		m_pBtn5 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));
		m_pBtn1->SetCursor(handCursor);
		m_pBtn3->SetCursor(handCursor);
		m_pBtn5->SetCursor(handCursor);

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("0"));
			m_pBtn1->SetClientData(new wxVariant(eventData));
		}

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("2"));
			m_pBtn3->SetClientData(new wxVariant(eventData));
		}

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("4"));
			m_pBtn5->SetClientData(new wxVariant(eventData));
		}
		m_pBtn1->setBtnStatus(BOX_SELECT);

		m_pBtn1->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);
		m_pBtn3->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);
		m_pBtn5->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);

		m_pBtn1->SetMinSize(AnkerSize(50, 65));
		m_pBtn1->SetMaxSize(AnkerSize(50, 65));
		m_pBtn1->SetSize(AnkerSize(50, 65));

		m_pBtn3->SetMinSize(AnkerSize(50, 65));
		m_pBtn3->SetMaxSize(AnkerSize(50, 65));
		m_pBtn3->SetSize(AnkerSize(50, 65));

		m_pBtn5->SetMinSize(AnkerSize(50, 65));
		m_pBtn5->SetMaxSize(AnkerSize(50, 65));
		m_pBtn5->SetSize(AnkerSize(50, 65));

		pRowAHSizer->AddSpacer(10);
		pRowAHSizer->Add(m_pBtn1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowAHSizer->AddSpacer(10);
		pRowAHSizer->Add(m_pBtn3, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowAHSizer->AddSpacer(10);
		pRowAHSizer->Add(m_pBtn5, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowAHSizer->AddSpacer(10);

		m_pBtn2 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));
		m_pBtn4 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));
		m_pBtn6 = new AnkerMerterialBoxBtn(m_pPrintNozzlesWidget, wxColour(0, 191, 255));

		m_pBtn2->SetCursor(handCursor);
		m_pBtn4->SetCursor(handCursor);
		m_pBtn6->SetCursor(handCursor);

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("1"));
			m_pBtn2->SetClientData(new wxVariant(eventData));
		}

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("3"));
			m_pBtn4->SetClientData(new wxVariant(eventData));
		}

		{
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant("5"));
			m_pBtn6->SetClientData(new wxVariant(eventData));
		}

		m_pBtn2->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);
		m_pBtn4->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);
		m_pBtn6->Bind(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, &AnkerControlWidget::onMaterilBtnClicked, this);

		m_pBtn2->SetMinSize(AnkerSize(50, 65));
		m_pBtn2->SetMaxSize(AnkerSize(50, 65));
		m_pBtn2->SetSize(AnkerSize(50, 65));

		m_pBtn4->SetMinSize(AnkerSize(50, 65));
		m_pBtn4->SetMaxSize(AnkerSize(50, 65));
		m_pBtn4->SetSize(AnkerSize(50, 65));

		m_pBtn6->SetMinSize(AnkerSize(50, 65));
		m_pBtn6->SetMaxSize(AnkerSize(50, 65));
		m_pBtn6->SetSize(AnkerSize(50, 65));

		pRowBHSizer->AddSpacer(10);
		pRowBHSizer->Add(m_pBtn2, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowBHSizer->AddSpacer(10);
		pRowBHSizer->Add(m_pBtn4, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowBHSizer->AddSpacer(10);
		pRowBHSizer->Add(m_pBtn6, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRowBHSizer->AddSpacer(10);
		
		m_btnList.push_back(m_pBtn1);
		m_btnList.push_back(m_pBtn2);
		m_btnList.push_back(m_pBtn3);
		m_btnList.push_back(m_pBtn4);
		m_btnList.push_back(m_pBtn5);
		m_btnList.push_back(m_pBtn6);		

		pLeftSizer->AddSpacer(5);
		pLeftSizer->Add(m_pDeviceImg, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pLeftSizer->Add(m_pPrintNozzlesWidget, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);		
		pLeftSizer->AddSpacer(5);
		showMulitColorNozzle(false);
		
		wxBoxSizer* pRightVSizer = new wxBoxSizer(wxVERTICAL);
		m_pLengthPanel = new wxPanel(this);
		m_pLengthPanel->SetBackgroundColour(wxColour(41, 42, 45));

		wxBoxSizer* pLineEditSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pLengthPanel->SetSizer(pLineEditSizer);
		m_pMaterialLengthLabel = new wxStaticText(m_pLengthPanel,wxID_ANY,_L("common_print_control_length"));
		m_pMaterialLengthLabel->SetFont(ANKER_FONT_NO_1);
		wxClientDC dc(this);
		dc.SetFont(ANKER_FONT_NO_1);
		wxSize size = dc.GetTextExtent(_L("common_print_control_length"));
		int textWidth = size.GetWidth();
		m_pMaterialLengthLabel->SetForegroundColour(wxColour("#a9aaab"));
		m_pMaterialLengthLabel->SetMaxSize(AnkerSize(textWidth, 24));
		m_pMaterialLengthLabel->SetMinSize(AnkerSize(textWidth, 24));
		m_pMaterialLengthLabel->SetSize(AnkerSize(textWidth, 24));

		wxBoxSizer* pMacLabelVsizer = new wxBoxSizer(wxVERTICAL);
		
		int editHeight = 25;
		m_pLengthLineEdit = new AnkerLineEditUnit(m_pLengthPanel, _L("mm"), ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
		m_pLengthLineEdit->SetValue("40");
		m_pLengthLineEdit->SetMaxSize(AnkerSize(84, editHeight));
		m_pLengthLineEdit->SetMinSize(AnkerSize(84, editHeight));
		m_pLengthLineEdit->SetSize(AnkerSize(84, editHeight));
		m_pLengthLineEdit->getTextEdit()->SetHint("20~200");
		m_pLengthLineEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this](wxCommandEvent&event) {
			wxString value = m_pLengthLineEdit->GetValue();		

			ANKER_LOG_INFO << "length value changed" << value.ToStdString().c_str();
			
			int uValue = 0;
			if (!value.ToInt(&uValue))
			{
				value = "20";
				m_pLengthLineEdit->SetValue(value);
			}
			else {
				if (uValue > 200)
				{
					value = "200";
					m_pLengthLineEdit->SetValue(value);
				}
				else if (uValue < 20)
				{
					value = "20";
					m_pLengthLineEdit->SetValue(value);
				}
			}

			});

		int lineEditHeight = 25;

#ifdef __WXOSX__
		int shrink = 10;
		pMacLabelVsizer->AddSpacer(10);
#else
		int shrink = 4;
#endif
		m_pLengthLineEdit->getTextEdit()->SetMinSize(wxSize(-1, lineEditHeight - shrink));
		m_pLengthLineEdit->getTextEdit()->SetMaxSize(wxSize(-1, lineEditHeight - shrink));
		m_pLengthLineEdit->getTextEdit()->SetSize(wxSize(-1, lineEditHeight - shrink));
		m_pLengthLineEdit->getUnitEdit()->SetMinSize(wxSize(-1, lineEditHeight - shrink));
		m_pLengthLineEdit->getUnitEdit()->SetMaxSize(wxSize(-1, lineEditHeight - shrink));
		m_pLengthLineEdit->getUnitEdit()->SetSize(wxSize(-1, lineEditHeight - shrink));

		pMacLabelVsizer->Add(m_pMaterialLengthLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pLineEditSizer->AddStretchSpacer(1);
		pLineEditSizer->Add(pMacLabelVsizer, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pLineEditSizer->AddSpacer(16);
		pLineEditSizer->Add(m_pLengthLineEdit, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pLineEditSizer->AddStretchSpacer(1);

		
		m_pRetractBtn = new AnkerBtn(this, wxID_ANY);
		m_pRetractBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerControlWidget::onRetractBtnClicked, this);

		m_pRetractBtn->SetRadius(4);			
		m_pRetractBtn->SetText(_L("common_print_controlbutton_retract"));
		m_pRetractBtn->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pRetractBtn->SetMaxSize(AnkerSize(143, 24));
		m_pRetractBtn->SetMinSize(AnkerSize(143, 24));
		m_pRetractBtn->SetSize(AnkerSize(143, 24));

		m_pRetractBtn->SetBackgroundColour(wxColor(38, 50, 47));		

		m_pRetractBtn->SetNorTextColor(wxColor(98, 211, 97));
		m_pRetractBtn->SetHoverTextColor(wxColor(98, 211, 97));
		m_pRetractBtn->SetPressedTextColor(wxColor(98, 211, 97));
		m_pRetractBtn->SetDisableTextColor(wxColor(98, 98, 100));

		m_pRetractBtn->SetBgNorColor(wxColor(38, 50, 41));
		m_pRetractBtn->SetBgHoverColor(wxColor(59, 70, 62));
		m_pRetractBtn->SetBgPressedColor(wxColor(30, 40, 43));
		m_pRetractBtn->SetBgDisableColor(wxColor(39, 40, 43));		
		
		m_pExtrudeBtn = new AnkerBtn(this, wxID_ANY);
		m_pExtrudeBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerControlWidget::onExtrudeBtnClicked, this);

		m_pExtrudeBtn->SetRadius(4);
		m_pExtrudeBtn->SetText(_L("common_print_controlbutton_extrude"));
		m_pExtrudeBtn->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pExtrudeBtn->SetMaxSize(AnkerSize(143, 24));
		m_pExtrudeBtn->SetMinSize(AnkerSize(143, 24));
		m_pExtrudeBtn->SetSize(AnkerSize(143, 24));

		m_pExtrudeBtn->SetBackgroundColour(wxColor(38, 50, 47));

		m_pExtrudeBtn->SetNorTextColor(wxColor(98, 211, 97));
		m_pExtrudeBtn->SetHoverTextColor(wxColor(98, 211, 97));
		m_pExtrudeBtn->SetPressedTextColor(wxColor(98, 211, 97));
		m_pExtrudeBtn->SetDisableTextColor(wxColor(98, 98, 100));

		m_pExtrudeBtn->SetBgNorColor(wxColor(38, 50, 41));
		m_pExtrudeBtn->SetBgHoverColor(wxColor(59, 70, 62));
		m_pExtrudeBtn->SetBgPressedColor(wxColor(30, 40, 43));
		m_pExtrudeBtn->SetBgDisableColor(wxColor(39, 40, 43));


		pRightVSizer->Add(m_pLengthPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRightVSizer->AddSpacer(18);
		pRightVSizer->Add(m_pRetractBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRightVSizer->AddSpacer(8);
		pRightVSizer->Add(m_pExtrudeBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

		pHSizer->AddSpacer(38);
		pHSizer->Add(pLeftSizer, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pHSizer->AddSpacer(40);
		pHSizer->Add(pRightVSizer, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pHSizer->AddSpacer(40);

		pMainVSizer->Add(pHSizer);
		m_pLengthPanel->Hide();
	}

	{
		wxSizer* spacingHSizer = new wxBoxSizer(wxHORIZONTAL);

		m_pNumLine = new wxPanel(this);
		m_pNumLine->SetBackgroundColour(wxColour("#3E3F42"));
		m_pNumLine->SetMinSize(AnkerSize(450, 1));
		m_pNumLine->SetMaxSize(AnkerSize(450, 1));
		m_pNumLine->SetSize(AnkerSize(450, 1));

		spacingHSizer->AddSpacer(12);
		spacingHSizer->Add(m_pNumLine, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		spacingHSizer->AddSpacer(12);
		m_pNumLine->Hide();
		
		pMainVSizer->Add(spacingHSizer);
		pMainVSizer->AddSpacer(4);
	}

	//z-offset
	{
		wxBoxSizer* pHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pOffsetPanel = new wxPanel(this);
		m_pOffsetPanel->SetSizer(pHSizer);

		m_pLabel = new wxStaticText(m_pOffsetPanel,wxID_ANY,_L("common_print_control_zoffset"));
		m_pLabel->SetForegroundColour(wxColour("#a9aaab"));
		m_pLabel->SetFont(ANKER_FONT_NO_1);

		m_pOffsetLineEdit = new AnkerLineEditUnit(m_pOffsetPanel, _L("mm"), ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
		m_pOffsetLineEdit->setValue("0.00");
		m_pOffsetLineEdit->getTextEdit()->SetHint("-2~2");

		m_pOffsetLineEdit->Bind(wxEVT_TEXT, [this](wxCommandEvent& event) {
			if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
				return;
			}
		});
		m_pOffsetLineEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED,[this](wxCommandEvent& event) {
			wxString value = m_pOffsetLineEdit->GetValue();
			ANKER_LOG_INFO << "offset edit value changed " << value.ToStdString().c_str();
			accurateCalculation(value);
			double dValue = 0;
			double numEx = 0;
			dValue = std::stod(value.ToStdString().c_str());
	
			if (dValue > 2.0)
			{
				dValue = 2.0;
			}
			else if (dValue < -2.0)
			{
				dValue = - 2.0;
			}
			
			wxString editValue = wxString::Format(wxT("%.2f"), dValue);
			

			ANKER_LOG_INFO << "zoffset edit real value changed " << value.ToStdString().c_str();
			wxVariant numData(editValue);
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ZOFFSET_CHANGED);
			evt.SetClientObject(new wxStringClientData(numData));
			//ProcessEvent(evt);
			wxPostEvent(this, evt);

			m_pOffsetLineEdit->SetValue(editValue);

			});
		int editHeight = 25;
		m_pOffsetLineEdit->SetMaxSize(AnkerSize(250, editHeight));
		m_pOffsetLineEdit->SetMinSize(AnkerSize(230, editHeight));
		m_pOffsetLineEdit->SetSize(AnkerSize(250, editHeight));


#ifdef __WXOSX__
		int shrink = 10;
#else
		int shrink = 4;
#endif
		m_pOffsetLineEdit->getTextEdit()->SetMinSize(wxSize(-1, editHeight - shrink));
		m_pOffsetLineEdit->getTextEdit()->SetMaxSize(wxSize(-1, editHeight - shrink));
		m_pOffsetLineEdit->getTextEdit()->SetSize(wxSize(-1, editHeight - shrink));
		m_pOffsetLineEdit->getUnitEdit()->SetMinSize(wxSize(-1, editHeight - shrink));
		m_pOffsetLineEdit->getUnitEdit()->SetMaxSize(wxSize(-1, editHeight - shrink));
		m_pOffsetLineEdit->getUnitEdit()->SetSize(wxSize(-1, editHeight - shrink));


		m_pAddBtn = new AnkerBtn(m_pOffsetPanel,wxID_ANY);
		m_pAddBtn->SetText("+");
		m_pAddBtn->SetRadius(4);
		m_pAddBtn->SetFont(ANKER_FONT_TEMPERATURE_UNITEX);
		m_pAddBtn->SetMaxSize(AnkerSize(24, 24));
		m_pAddBtn->SetMinSize(AnkerSize(24, 24));
		m_pAddBtn->SetSize(AnkerSize(24, 24));

		m_pAddBtn->SetNorTextColor(wxColor(242, 242, 242));

		m_pAddBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pAddBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pAddBtn->SetborderNorColor(wxColor(67, 68, 71));
		m_pAddBtn->SetborderHoverBGColor(wxColor(169, 170, 171));
		m_pAddBtn->SetborderPressedBGColor(wxColor(169, 170, 171));
		//m_pAddBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pAddBtn->SetborderDisableBGColor(wxColor(67, 68, 71));

		m_pAddBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
			if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
				return;
			}

			wxString value = m_pOffsetLineEdit->GetValue();
			ANKER_LOG_INFO << "increase zoffset" << value.ToStdString().c_str();
			accurateCalculation(value);
			double dValue = 0;

			if (!value.ToDouble(&dValue))
			{
				dValue = 2.0;
			}
			else
			{
				dValue = dValue + 0.01;

				if (dValue > 2.0)
				{
					dValue = 2.0;
				}
			}
			
			wxString editValue = wxString::Format(wxT("%.2f"), dValue);

			ANKER_LOG_INFO << "increase real zoffset" << editValue.ToStdString().c_str();
			wxVariant numData(editValue);
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ZOFFSET_CHANGED);
			evt.SetClientObject(new wxStringClientData(numData));
			//ProcessEvent(evt);
			wxPostEvent(this, evt);

			m_pOffsetLineEdit->SetValue(editValue);
		});

		m_pdeduceBtn = new AnkerBtn(m_pOffsetPanel,wxID_ANY);
		m_pdeduceBtn->SetText("-");
		m_pdeduceBtn->SetRadius(4);
		m_pdeduceBtn->SetFont(ANKER_FONT_TEMPERATURE_UNITEX);
		m_pdeduceBtn->SetMaxSize(AnkerSize(24, 24));
		m_pdeduceBtn->SetMinSize(AnkerSize(24, 24));
		m_pdeduceBtn->SetSize(AnkerSize(24, 24));

		m_pdeduceBtn->SetNorTextColor(wxColor(242,242,242));

		m_pdeduceBtn->SetBgNorColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgHoverColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgPressedColor(wxColor(41, 42, 45));
		m_pdeduceBtn->SetBgDisableColor(wxColor(41, 42, 45));

		m_pdeduceBtn->SetborderNorColor(wxColor(67, 68, 71));
		m_pdeduceBtn->SetborderHoverBGColor(wxColor(169, 170, 171));
		m_pdeduceBtn->SetborderPressedBGColor(wxColor(169, 170, 171));
		//m_pdeduceBtn->SetborderDisableBGColor(wxColor(49, 50, 53));
		m_pdeduceBtn->SetborderDisableBGColor(wxColor(67, 68, 71));


		m_pdeduceBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
			if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
				return;
			}

			wxString value = m_pOffsetLineEdit->GetValue();
			ANKER_LOG_INFO << "reduce zoffset" << value.ToStdString().c_str();
			accurateCalculation(value);
			double dValue = 0;
			if (!value.ToDouble(&dValue))
			{
				dValue = 0;
			}
			dValue = dValue - 0.01;

			if (dValue < -2.0)
			{
				dValue = -2.0;
			}
			wxString editValue = wxString::Format(wxT("%.2f"), dValue);
			

			ANKER_LOG_INFO << "reduce real zoffset" << editValue.ToStdString().c_str();
			wxVariant numData(editValue);
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ZOFFSET_CHANGED);
			evt.SetClientObject(new wxStringClientData(numData));
			//ProcessEvent(evt);
			wxPostEvent(this, evt);
			
			m_pOffsetLineEdit->SetValue(editValue);
		});
		
		pHSizer->AddSpacer(12);
		pHSizer->Add(m_pLabel, wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL,0);		
		pHSizer->AddSpacer(5);
		pHSizer->AddStretchSpacer(1);
		pHSizer->Add(m_pOffsetLineEdit, wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
		pHSizer->AddSpacer(8);		
		pHSizer->Add(m_pdeduceBtn, wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
		pHSizer->AddSpacer(8);
		pHSizer->Add(m_pAddBtn, wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
		pHSizer->AddSpacer(12);

		pMainVSizer->Add(m_pOffsetPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 12);
		m_pOffsetPanel->Hide();
	}

	SetSizer(pMainVSizer);
}


void AnkerControlWidget::showStopBtnStatus(AnkerBtn* pBtn)
{
	if (!pBtn)
		return;

	wxString btnName = pBtn->GetText();

	if (btnName == _L("common_print_controlbutton_stop"))
		return;

	pBtn->SetText(_L("common_print_controlbutton_stop"));
	
	pBtn->SetTextColor(wxColour(255, 54, 54));
	pBtn->SetHoverTextColor(wxColour(255, 54, 54));
	pBtn->SetPressedTextColor(wxColour(255, 54, 54));

	pBtn->SetBgNorColor(wxColour(84, 45, 47));
	pBtn->SetBgHoverColor(wxColour(84, 45, 47));
	pBtn->SetBgPressedColor(wxColour(84, 45, 47));
	pBtn->SetBgDisableColor(wxColour(84, 45, 47));

	Refresh();
	Layout();

}


void AnkerControlWidget::showDisAbelBtnStatus(AnkerBtn* pBtn)
{
	if (!pBtn)
		return;
	
	if (!pBtn->IsEnabled())
		return;

	pBtn->SetTextColor(OFFILE_COLOR);	
	pBtn->SetDisableTextColor(OFFILE_COLOR);
	pBtn->SetBgDisableColor(wxColor(46, 47, 50));
	pBtn->SetBackgroundColour(wxColor(46, 47, 50));	
	pBtn->Enable(false);

	Refresh();
	Layout();
}


void AnkerControlWidget::showNorBtnStatus(AnkerBtn* pBtn, const wxString& btnName)
{
	if (!pBtn)
		return;

	if (pBtn->GetText() == btnName && pBtn->IsEnabled())
		return;
	
	pBtn->SetText(btnName);
	pBtn->SetBackgroundColour(wxColor(38, 50, 47));

	pBtn->SetNorTextColor(wxColor(98, 211, 97));
	pBtn->SetHoverTextColor(wxColor(98, 211, 97));
	pBtn->SetPressedTextColor(wxColor(98, 211, 97));
	pBtn->SetDisableTextColor(wxColor(98, 98, 100));

	pBtn->SetBgNorColor(wxColor(47, 59, 51));
	pBtn->SetBgHoverColor(wxColor(59, 70, 62));
	pBtn->SetBgPressedColor(wxColor(30, 40, 43));
	pBtn->SetBgDisableColor(wxColor(39, 40, 43));

	pBtn->Enable(true);

	Refresh();
	Layout();
}

AnkerTemperatureWidget::AnkerTemperatureWidget(const std::string& deviceSn, wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_currentDeviceSn(deviceSn)
	, m_pNumText(nullptr)
	, m_pNumUnitText(nullptr)
	, m_pNormalPanel(nullptr)
	, m_pTipsText(nullptr)
	, m_pEditPanel(nullptr)
	, m_pTemperatureEdit(nullptr)
	, m_TemperatureUnitText(nullptr)	
{
	initUi();
}


AnkerTemperatureWidget::~AnkerTemperatureWidget()
{

}


void AnkerTemperatureWidget::showEditTempertrueBtn(bool show)
{
	if (show)
	{
		if(!m_isEditStatus)
			m_pEditBtn->Show();
	}
	else
	{
		m_pEditBtn->Hide();
	}
}

void AnkerTemperatureWidget::setMaxRage(const int& maxRange)
{
	m_maxSize = maxRange;
}


void AnkerTemperatureWidget::setMinRage(const int& minRange)
{
	m_minSize = minRange;
}

void AnkerTemperatureWidget::setTipsText(const wxString& text)
{
	m_pTipsText->SetLabelText(text);
	
	wxString str = text;
	wxClientDC dc(this);
	wxCoord width, height;
	dc.GetTextExtent(str, &width, &height);
	
#ifndef __APPLE__
	m_pTipsText->SetMinSize(AnkerSize(120, 20));
#endif
	m_tipsText = text;
}


void AnkerTemperatureWidget::setHintSize(const wxString& text)
{
	m_pTemperatureEdit->SetHint(text);
}

void AnkerTemperatureWidget::setTemperatureNum(const wxString& numText)
{	
	wxString tempValue = numText + _L("#");			
	wxClientDC dc(this);
	dc.SetFont(ANKER_FONT_TEMPERATURE_UNIT);
	wxSize size = dc.GetTextExtent(tempValue);
	int textWidth = size.GetWidth();

	m_pNumText->SetMaxSize(size);
	m_pNumText->SetMinSize(size);
	m_pNumText->SetSize(size);

	m_pNumText->SetLabelText(tempValue);

	Refresh();	
}


void AnkerTemperatureWidget::updateWorkStatus(bool isWork)
{
	m_isheating = isWork;
	if (!m_isheating)
	{
		m_pNumText->SetForegroundColour(wxColour("#999999"));
		m_pNumUnitText->SetForegroundColour(wxColour("#999999"));
	}
	else
	{
		m_pNumText->SetForegroundColour(wxColour("#62D361"));
		m_pNumUnitText->SetForegroundColour(wxColour("#62D361"));
	}

}


wxString AnkerTemperatureWidget::getValue()
{
	return m_pTemperatureEdit->GetValue();
}
void AnkerTemperatureWidget::setOfflineStatus()
{
	m_pNumText->SetLabelText("--"+ _L("#"));

	int pos = m_tipsText.Find(':');
	wxString subStr = m_tipsText.SubString(0, pos);
	wxString tempText = subStr +  _L("--");

	wxClientDC dc(this);
	wxCoord width, height;
	dc.GetTextExtent(tempText, &width, &height);
#ifndef __APPLE__
	m_pTipsText->SetMinSize(AnkerSize(120, 20));
#endif
	m_pTipsText->SetLabelText(tempText);	
	m_pTipsText->SetForegroundColour(OFFILE_COLOR);

	m_pEditBtn->Hide();	
}

void AnkerTemperatureWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));	

	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);

	{
		wxPanel* pNumberPanel = new wxPanel(this,wxID_ANY);
		pNumberPanel->SetMaxSize(AnkerSize(100,45));
		pNumberPanel->SetMinSize(AnkerSize(100,45));
		pNumberPanel->SetSize(AnkerSize(100,45));
		wxBoxSizer* pNumShowHSizer = new wxBoxSizer(wxHORIZONTAL);
		wxString defaultStr = "--" + _L("#");

		m_pNumText = new wxStaticText(pNumberPanel, wxID_ANY, defaultStr);
		wxClientDC dc(this);
		dc.SetFont(ANKER_FONT_TEMPERATURE_NUM);
		wxSize size = dc.GetTextExtent(defaultStr);
		m_pNumText->SetMaxSize(size);
		m_pNumText->SetMinSize(size);
		m_pNumText->SetSize(size);
		
		m_pNumText->SetFont(ANKER_FONT_TEMPERATURE_NUM);
		m_pNumText->SetForegroundColour(wxColour("#999999"));

		m_pNumUnitText = new wxStaticText(pNumberPanel, wxID_ANY, _L("#"));
		m_pNumUnitText->SetMinSize(AnkerSize(28, 28));

		m_pNumUnitText->SetFont(ANKER_FONT_NO_1);
		m_pNumUnitText->SetForegroundColour(wxColour("#999999"));
		m_pNumUnitText->Hide();
		
		pNumShowHSizer->Add(m_pNumText, wxALIGN_LEFT, wxALIGN_LEFT, 0);
		pNumShowHSizer->Add(m_pNumUnitText, wxALIGN_LEFT , wxALIGN_LEFT, 0);
		pNumShowHSizer->AddStretchSpacer(1);
		pNumberPanel->SetSizer(pNumShowHSizer);

		pBodyVSizer->Add(pNumberPanel, wxALIGN_LEFT, wxALIGN_LEFT, 0);
		
	}

	{
		m_pNormalPanel = new wxPanel(this, wxID_ANY);
		m_pNormalPanel->SetMaxSize(AnkerSize(130, 30));
		m_pNormalPanel->SetMinSize(AnkerSize(130, 30));
		m_pNormalPanel->SetSize(AnkerSize(130, 30));

		wxBoxSizer* pNormalHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pNormalPanel->SetBackgroundColour(wxColour("#292A2D"));
		//m_pNormalPanel->SetBackgroundColour(wxColour("#B0E0E6"));
		m_pTipsText = new wxStaticText(m_pNormalPanel, wxID_ANY, "");	
#ifndef __APPLE__
		int tipsLength = 84;
#else
		int tipsLength = 100;
#endif
		m_pTipsText->SetMinSize(AnkerSize(tipsLength, 20));
		m_pTipsText->SetMaxSize(AnkerSize(tipsLength, 20));
		m_pTipsText->SetSize(AnkerSize(tipsLength, 20));

		m_pTipsText->SetFont(ANKER_FONT_NO_1);
		//m_pTipsText->Wrap(500);
		m_pTipsText->SetForegroundColour(wxColour("#999999"));

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("appbar_edit_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(16, 16);
		wxBitmap scaledBitmap(image);
		m_pEditBtn = new wxButton(m_pNormalPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		wxCursor handCursor(wxCURSOR_HAND);
		m_pEditBtn->SetCursor(handCursor);
		m_pEditBtn->SetBitmap(scaledBitmap);
		m_pEditBtn->SetMinSize(AnkerSize(16, 16));
		m_pEditBtn->SetMaxSize(AnkerSize(16, 16));
		m_pEditBtn->SetSize(AnkerSize(16, 16));
		m_pEditBtn->SetBackgroundColour(GetBackgroundColour());
		m_pEditBtn->SetToolTip(_L("common_control_tip_edit_temperature"));

		m_pEditBtn->Bind(wxEVT_BUTTON, [this](wxEvent& event) {
			if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
				return;
			}

			m_isEditStatus = true;
			m_pEditPanel->Show();

			m_pTemperatureEdit->SetFocus();
			m_pNormalPanel->Hide();

			Layout();
			Refresh(true);
			m_pNumUnitText->SetForegroundColour(wxColour("#999999"));
			m_pNumText->SetForegroundColour(wxColour("#999999"));
		});

#ifdef _WIN32
		m_pEditBtn->SetWindowStyle(wxBORDER_NONE);
#endif	
		//pNormalHSizer->AddStretchSpacer(1);
		pNormalHSizer->Add(m_pTipsText, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
		pNormalHSizer->Add(m_pEditBtn, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
		//pNormalHSizer->AddStretchSpacer(1);
		m_pNormalPanel->SetSizer(pNormalHSizer);

		m_pEditPanel = new wxPanel(this, wxID_ANY);		
		m_pEditPanel->SetBackgroundColour(wxColour("#292A2D"));
		//m_pEditPanel->SetBackgroundColour(wxColour("#9932CC"));
		m_pEditPanel->SetMinSize(AnkerSize(100, 30));
		m_pEditPanel->SetMaxSize(AnkerSize(100, 30));
		m_pEditPanel->SetSize(AnkerSize(100, 30));
		wxBoxSizer* pEditHSizer = new wxBoxSizer(wxHORIZONTAL);
#ifdef _WIN32
		m_pTemperatureEdit = new AnkerLineEdit(m_pEditPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_PROCESS_ENTER);
#else
		m_pTemperatureEdit = new AnkerLineEdit(m_pEditPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
		m_pTemperatureEdit->SetWindowStyle(wxBORDER_THEME);
#endif // _WIN32
		
		m_pTemperatureEdit->SetMaxLength(3);
		m_pTemperatureEdit->SetForegroundColour(wxColour("#999999"));
		m_pTemperatureEdit->SetBackgroundColour(wxColour("#292A2D"));		
		m_pTemperatureEdit->SetMinSize(AnkerSize(81, 25));
		m_pTemperatureEdit->SetMaxSize(AnkerSize(81, 25));
		m_pTemperatureEdit->SetSize(AnkerSize(81, 25));
		m_pTemperatureEdit->SetHint("0~260");

		wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
		validator.SetCharIncludes("0123456789");

		m_pTemperatureEdit->SetValidator(validator);

		m_pTemperatureEdit->Bind(wxCUSTOMEVT_EDIT_ENTER, &AnkerTemperatureWidget::OnEnter, this);
		m_pTemperatureEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, &AnkerTemperatureWidget::OnTextCtrlEditFinished, this);

		m_TemperatureUnitText = new wxStaticText(m_pEditPanel, wxID_ANY, _L("#"));
		m_TemperatureUnitText->SetForegroundColour(wxColour("#999999"));
		m_TemperatureUnitText->SetMinSize(AnkerSize(20, 20));
		m_TemperatureUnitText->SetMaxSize(AnkerSize(20, 20));
		m_TemperatureUnitText->SetSize(AnkerSize(20, 20));
		
		m_TemperatureUnitText->SetFont(ANKER_FONT_NO_1);
		
		pEditHSizer->Add(m_pTemperatureEdit, wxALIGN_LEFT | wxALIGN_BOTTOM, wxALIGN_LEFT | wxALIGN_BOTTOM, 0);
		pEditHSizer->Add(m_TemperatureUnitText, wxALIGN_LEFT | wxALIGN_BOTTOM, wxALIGN_LEFT | wxALIGN_BOTTOM, 0);
		pEditHSizer->AddStretchSpacer(1);

		m_pEditPanel->SetSizer(pEditHSizer);
		
		m_pEditPanel->Hide();

		wxBoxSizer* pBodyHSizer = new wxBoxSizer(wxHORIZONTAL);
		pBodyHSizer->AddStretchSpacer(1);
		pBodyHSizer->Add(m_pNormalPanel, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
		pBodyHSizer->Add(m_pEditPanel, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
		pBodyHSizer->AddStretchSpacer(1);

		pBodyVSizer->Add(pBodyHSizer, wxALIGN_LEFT , wxALIGN_LEFT , 0);

	}
	pMainHSizer->Add(pBodyVSizer, wxALIGN_LEFT , wxALIGN_LEFT , 0);


	SetSizer(pMainHSizer);

}


void AnkerTemperatureWidget::OnTextCtrlEditFinished(wxCommandEvent& event)
{
	wxString numValue = m_pTemperatureEdit->GetValue();	

	if (numValue.IsEmpty())
	{
 		m_isEditStatus = false;
 		//m_pTemperatureEdit->Clear();
 		m_pEditPanel->Hide();
 		m_pNormalPanel->Show();
 		//m_pTemperatureEdit->Hide();
 		//m_TemperatureUnitText->Hide();
 
 		m_pEditBtn->Show(true);
 		m_pTipsText->Show(true);
 
 		Layout();
 		Refresh();
 		m_pNumText->SetForegroundColour(wxColour("#999999"));
		return;
	}

	double resValue = 0;
	numValue.ToDouble(&resValue);
	bool bUpdateEdit = false;
	if (resValue > m_maxSize)
	{
		numValue = wxString::Format(wxT("%d"), m_maxSize);
		bUpdateEdit = true;
	}
	else if (resValue < m_minSize)
	{
		numValue = wxString::Format(wxT("%d"), m_minSize);
		bUpdateEdit = true;
	}

	if (!m_pTemperatureEdit->GetValue().IsEmpty())
	{
		wxVariant numData(numValue);
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_UPDATE);
		evt.SetClientObject(new wxStringClientData(numData));
		ProcessEvent(evt);

		int pos = m_tipsText.Find(':');
		wxString subStr = m_tipsText.SubString(0, pos);
		wxString tempText = subStr + numValue + _L("#");

		wxClientDC dc(this);
		wxCoord width, height;
		dc.GetTextExtent(tempText, &width, &height);
#ifndef __APPLE__
		m_pTipsText->SetMinSize(AnkerSize(120, 20));
#endif
		m_pTipsText->SetLabelText(tempText);
	}

	if(bUpdateEdit)
		m_pTemperatureEdit->SetValue(numValue);
	
	m_isEditStatus = false;
	//m_pTemperatureEdit->Clear();
	m_pEditPanel->Hide();
	m_pNormalPanel->Show();
	//m_pTemperatureEdit->Hide();
	//m_TemperatureUnitText->Hide();

	m_pEditBtn->Show(true);
	m_pTipsText->Show(true);

	Layout();
	Refresh();
	m_pNumText->SetForegroundColour(wxColour("#999999"));
}


void AnkerTemperatureWidget::OnEnter(wxCommandEvent& event)
{
	//trigger m_pTemperatureEdit's kill focus event
	m_pNumText->SetFocus();
	event.Skip();
}

AnkerDeviceControl::AnkerDeviceControl(std::string currentDeviceID, wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_currentDeviceId(currentDeviceID) , m_halfDialog(nullptr)
{
	initUi();
	Bind(wxEVT_SHOW, &AnkerDeviceControl::OnShow, this);
}

AnkerDeviceControl::~AnkerDeviceControl()
{
	//tab,crashed when close,one time
	Unbind(wxEVT_SHOW, &AnkerDeviceControl::OnShow, this);
}

int AnkerDeviceControl::getCurrentWorkNozzle()
{
	return m_pControlWidget->getCurrentWorkNozzle();
}

void AnkerDeviceControl::updateDeviceStatus(const std::string& sn, aknmt_command_type_e type)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	if (m_currentDeviceId.empty())
		return;
	DeviceObjectBasePtr currentDev = ankerNet->getDeviceObjectFromSn(m_currentDeviceId);

	if (!currentDev)
		return;
	
	wxWindowUpdateLocker updateLocker(this);
	auto statusMapSize = currentDev->GetMultiCutoffCloggingMapSize();
	bool bJammedOrOutOfSupplies = (statusMapSize > 0) ?  true :false;
	if (bJammedOrOutOfSupplies)
	{
		if (false)
		{
			using namespace Slic3r::GUI;

			static bool res = false;
			if (res)
				return;
			res = true;
			//listen lack of material or blocking material event
			AnkerRoundDialog* testDilaog = new AnkerRoundDialog(this, "Info", "");
			testDilaog->SetSize(AnkerSize(400, 370));

			AnkerNozzlesStausPanel* pContentPanel = new AnkerNozzlesStausPanel(testDilaog);
			AnkerBasePanel* panelTmp = testDilaog->GetContentPanel();
			testDilaog->GetSizer()->Detach(panelTmp);
			panelTmp->Destroy();
			testDilaog->SetContentPanel(pContentPanel);
			testDilaog->GetSizer()->Insert(2, pContentPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 24);
			pContentPanel->SetMinSize(AnkerSize(-1, 280));

			//reset bottom panel
			if (pContentPanel->getDevcieStatus() == type_jammed)
			{
				AnkerBasePanel* pBottomPanel = new AnkerBasePanel(testDilaog);
				AnkerBasePanel* defaultBottomPanel = testDilaog->GetBottomPanel();
				testDilaog->GetSizer()->Detach(defaultBottomPanel);
				defaultBottomPanel->Destroy();
				testDilaog->SetBottomPanel(pBottomPanel);
				AnkerBtn* pOKBtn = new AnkerBtn(pBottomPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
				pOKBtn->SetText(_L("common_button_ok"));
				pOKBtn->SetMinSize(AnkerSize(352, 32));
				pOKBtn->SetBackgroundColour("#62D361");
				pOKBtn->SetForegroundColour(wxColour("#FFFFFF"));
				pOKBtn->SetRadius(3);
				pOKBtn->SetTextColor(wxColor("#FFFFFF"));
				pOKBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
				pOKBtn->Bind(wxEVT_BUTTON, &AnkerRoundDialog::OnOKButtonClicked, testDilaog);
				wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
				pBottomPanel->SetSizer(btnHSizer);
				btnHSizer->Add(pOKBtn, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
				testDilaog->GetSizer()->Insert(3, pBottomPanel, 0, wxEXPAND | wxBOTTOM, 16);
				pBottomPanel->SetMinSize(AnkerSize(-1, 32));
			}

			testDilaog->GetSizer()->Layout();
			int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
			int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
			testDilaog->SetPosition(wxPoint((screenW - 400) / 2, (screenH - 370) / 2));
			testDilaog->ShowModal();
			res = false;
		}
	}

	if(type == ::AKNMT_CMD_NOZZLE_TEMP || type == ::AKNMT_CMD_HOTBED_TEMP)
		m_pControlWidget->updateTemperatrue(sn);

	m_pControlWidget->updateNozzleStatus(sn);	

	//update edit tempertrue btn status
	if(currentDev->GetDeviceStatus()==AKNMT_PRINT_EVENT_LEVELING ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_LEVEL_HEATING||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_LOAD_MATERIAL ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_UNLOAD_MATERIAL)
	{
		m_pControlWidget->showEditTempertrueBtn(false);
	}
	else
	{
		m_pControlWidget->showEditTempertrueBtn(true);
	}

	//update bed nozzol work status.
	if (currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_IDLE)
	{
		auto extrusionValue = currentDev->GetExtrusionValue();
		ANKER_LOG_DEBUG << "extrusionInfo value: " << extrusionValue;
		if (extrusionValue > 0) {
			m_pControlWidget->updateWorkStatus(true);
		}
		else {
			m_pControlWidget->updateWorkStatus(false);
		}
	}
	else
	{
		m_pControlWidget->updateWorkStatus(true);
	}		

	//update z-offset work status
	if (currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_LEVELING ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_CALIBRATION||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_LEVEL_HEATING ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_LOAD_MATERIAL ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_UNLOAD_MATERIAL)
	{
		//disable zoffset
		updateZoffset(false);
	}
	else
	{
		//able zoffset
		updateZoffset(true);
	}

	if (currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_IDLE ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_PAUSED ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_COMPLETED ||
		currentDev->GetDeviceStatus() == AKNMT_PRINT_EVENT_STOPPED)
	{
		auto extrusionValue = currentDev->GetExtrusionValue();
		if (extrusionValue > 0)
			updateExtrude(true, extrusionValue);
		else
			updateExtrude(true);
	}
	else
	{
		updateExtrude(false);
	}

	m_pStatusWidget->updateStatus(sn, type);
	m_pStatusWidget->requestCallback(type);	

	auto temp = static_cast<int>(currentDev->GetNozzleMaxTemp());
	setNozzleMaxTemp(temp);
}


void AnkerDeviceControl::updateZoffsetValue(const std::string& sn)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	if (m_currentDeviceId.empty())
		return;
	DeviceObjectBasePtr currentDev = ankerNet->getDeviceObjectFromSn(m_currentDeviceId);

	if (!currentDev)
		return;
	
	wxWindowUpdateLocker updateLocker(this);
	m_pControlWidget->updateZoffesetValue(sn);
}

void AnkerDeviceControl::updateFileTransferStatus(const std::string& sn, int progress, FileTransferResult result)
{
	m_pStatusWidget->updateFileTransferStatus(sn, progress, result);
}


void AnkerDeviceControl::showMulitColorNozzle(bool isShow)
{
	wxWindowUpdateLocker updateLocker(this);
	m_pControlWidget->showMulitColorNozzle(isShow);
}


void AnkerDeviceControl::setIsSingleNozOfflineStatus(bool isSimpleNozOffline)
{
	wxWindowUpdateLocker updateLocker(this);
	m_pControlWidget->setIsSingleNozOfflineStatus(isSimpleNozOffline);	
	m_pVideoWidget->SetOnlineOfflineState(false);
	//hide adjust by alves
	//m_pAdjustWidget->setItemStatus(false, AnkerAdjustWidget::ITEM_LEVEL);
	m_pStatusWidget->setOfflineStatus();
}

void AnkerDeviceControl::displayVideoFrame(const unsigned char* imgData, int width, int height)
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->displayVideoFrame(imgData, width, height);
	}
}

void AnkerDeviceControl::onP2pvideoStreamSessionInit()
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onP2pvideoStreamSessionInit();
	}
}

void AnkerDeviceControl::onRecvVideoStreamClosing()
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onRecvVideoStreamClosing();
	}
}

void AnkerDeviceControl::onP2pvideoStreamSessionClosed()
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onP2pvideoStreamSessionClosed();
	}
}

void AnkerDeviceControl::onRcvP2pVideoStreamCtrlAbnomal()
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onRcvP2pVideoStreamCtrlAbnomal();
	}
}

void AnkerDeviceControl::onRecvCameraLightStateNotify(bool onOff)
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onRecvCameraLightStateNotify(onOff);
	}
}

void AnkerDeviceControl::onRecVideoModeNotify(int mode)
{
	AnkerVideo* AnkerVideoWidget = dynamic_cast<AnkerVideo*>(m_pVideoWidget);
	if (AnkerVideoWidget) {
		AnkerVideoWidget->onRecVideoModeNotify(mode);
	}
}

std::string AnkerDeviceControl::getCurrentDeviceId() const
{
	return m_currentDeviceId;
}

void AnkerDeviceControl::showMsgFrame(const NetworkMsg& msg)
{
	if (m_halfDialog) {
		return;
	}

	std::string sn = msg.sn;
	if (m_currentDeviceId != msg.sn) {
		ANKER_LOG_WARNING << "sn not equal cur sn: " << m_currentDeviceId << ", come sn: " << msg.sn;
		sn = m_currentDeviceId;
	}

	bool haveIcon = false;
	if (!msg.imagePath.empty()) {
		haveIcon = true;
	}

	wxWindow* parentWindow = GetParent();
	wxPoint mfPoint = GetPosition();
	wxSize mfSize = GetClientSize();
	bool setCenter = false;
	if (parentWindow) {
		mfSize = parentWindow->GetSize();
		auto pparentWindow = parentWindow->GetParent();
		if (pparentWindow) {
			mfSize = pparentWindow->GetSize();
			mfPoint = pparentWindow->GetPosition();
		}
	}
	wxSize dialogSize = AnkerSize(400, 250);
	if (haveIcon) {
		dialogSize = AnkerSize(400, 300);
	}
	
	wxPoint dialogPos = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, 
		mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
	if (mfSize.GetX() < 100 || mfSize.GetY() < 80) {
		setCenter = true;
		dialogPos = wxDefaultPosition;
	}
	ANKER_LOG_INFO << "half dialog show alert msg: " << msg.sn << ", " << msg.type << ", level: " << msg.level;

	m_halfDialog = new HalfModalDialog(this, wxID_ANY, wxString::FromUTF8(msg.title), 
		wxString::FromUTF8(msg.context), msg.imagePath, dialogPos, dialogSize);
	if (setCenter) {
		m_halfDialog->Center();
	}

	m_halfDialog->SetOkBtnCallBack([msg](wxCommandEvent&) {
		if (msg.clear) {
			auto ankerNet = AnkerNetInst();
			if (!ankerNet) {
				return;
			}
			if (!msg.sn.empty()) {
				DeviceObjectBasePtr deviceObject = ankerNet->getDeviceObjectFromSn(msg.sn);
				if (deviceObject) {
					ANKER_LOG_INFO << "clear the exception for " << msg.sn << ", " 
						<< msg.type << ", " << msg.context;
					deviceObject->clearDeviceExceptionInfo();
				}
			}
		}
	});
	m_halfDialog->Bind(wxEVT_CLOSE_WINDOW, [this, &sn](wxCloseEvent& event) {
		ANKER_LOG_INFO << "destroy half dialog start";
		m_halfDialog->Destroy();
		m_halfDialog = nullptr;
		ANKER_LOG_INFO << "destroy half dialog end";

		DatamangerUi::GetInstance().ShowNextAlertMsg();
	});

	if (parentWindow) {
		parentWindow->Bind(wxEVT_DESTROY, [this, parentWindow](wxWindowDestroyEvent& event) {
			ANKER_LOG_INFO << "parentWindow destroy.";
			parentWindow->Unbind(wxEVT_UPDATE_UI, &AnkerDeviceControl::updateWindowDisplay, this);
			});
		parentWindow->Bind(wxEVT_UPDATE_UI, &AnkerDeviceControl::updateWindowDisplay, this);
	}

	if (haveIcon) {
		m_halfDialog->ShowAnker(PartialModal_IMAGE_OK);
	}
	else if (msg.haveCancel) {
		m_halfDialog->ShowAnker(PartialModal_CANCEL_OK);
	}
	else {
		m_halfDialog->ShowAnker(PartialModal_OK);
	}
	m_halfDialog->Show(false);
}

void AnkerDeviceControl::activate(bool active)
{
	if (m_pStatusWidget)
	{
		m_pStatusWidget->activate(active);
	}
}

void AnkerDeviceControl::updateExtrude(bool isAble, int statusValue)
{
	wxWindowUpdateLocker updateLocker(this);
	m_pControlWidget->setExtrudeAble(isAble, statusValue);
}

void AnkerDeviceControl::updateZoffset(bool isAble)
{
	wxWindowUpdateLocker updateLocker(this);
	m_pControlWidget->setZoffsetDisabel(isAble);
}


void AnkerDeviceControl::OnStopExtrude(wxCommandEvent& event)
{
	DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceId);

	if (!currentDev)
		return;

	currentDev->setStopExtrusion();
}


void AnkerDeviceControl::OnExtrude(wxCommandEvent& event)
{
	DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceId);

	if (!currentDev)
		return;

	wxVariant* pData = (wxVariant*)event.GetClientData();
	if (pData)
	{
		wxVariantList list = pData->GetList();
		int setpLength = 0;
		list[0]->GetString().ToInt(&setpLength);

		int temperature = 0;
		int targetTemperature = 0;
		
		int nozzleNum = getCurrentWorkNozzle();

		//single color index is 0
		auto nozzData = currentDev->GetNozzleTemperature(nozzleNum);

		if (!nozzData.isNull)
		{
			temperature = nozzData.currentTemp;
			targetTemperature = nozzData.targetTemp;
		}
		
		currentDev->setDischargeExtrusion(setpLength, targetTemperature, nozzleNum);
	}
}

void AnkerDeviceControl::OnRetract(wxCommandEvent& event)
{
	DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceId);

	if (!currentDev)
		return;

	wxVariant* pData = (wxVariant*)event.GetClientData();
	if (pData)
	{
		wxVariantList list = pData->GetList();
		int setpLength = 0;
		int nozzleNum = 0;
		int temperature = 0;
		int targetTemperature = 0;

		nozzleNum = getCurrentWorkNozzle();

		list[0]->GetString().ToInt(&setpLength);
		
		//single color index is 0
		auto nozzData = currentDev->GetNozzleTemperature(nozzleNum);

		if (!nozzData.isNull)
		{
			temperature = nozzData.currentTemp;
			targetTemperature = nozzData.targetTemp;
		}

		currentDev->setMaterialReturnExtrusion(setpLength, targetTemperature, nozzleNum);
	}
}

void AnkerDeviceControl::updateWindowDisplay(wxUpdateUIEvent& event)
{
	if (m_halfDialog == nullptr) {
		return;
	}
	
	Slic3r::GUI::TabMode tabMode = Slic3r::GUI::TAB_DEVICE;
	if (Slic3r::GUI::wxGetApp().mainframe) {
		tabMode = Slic3r::GUI::wxGetApp().mainframe->get_current_tab_mode();
	}
	// Restrict the print control interface to pop-up check.
	if (tabMode == Slic3r::GUI::TAB_DEVICE) {
		m_halfDialog->CheckWindowShow();
	}
	else {
		m_halfDialog->Show(false);
	}
}

void AnkerDeviceControl::setNozzleMaxTemp(int temp)
{
	m_pControlWidget->setNozzleMaxTemp(temp);
}

void AnkerDeviceControl::initUi()
{
	SetBackgroundColour(wxColour("#1F2022"));
	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);
	{
		m_pVideoWidget = new AnkerVideo(this, m_currentDeviceId);
		m_pVideoWidget->SetMinSize(AnkerSize(760, 495));
		m_pVideoWidget->SetSize(AnkerSize(760, 495));
		m_pVideoWidget->SetBackgroundColour(wxColour("#292A2D"));		
		pBodyVSizer->AddSpacer(12);
		pBodyVSizer->Add(m_pVideoWidget, 23, wxEXPAND, 0);
		pBodyVSizer->AddSpacer(12);
	}

	{
		wxBoxSizer* pStatusHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pStatusWidget = new AnkerTaskPanel(m_currentDeviceId, this);
		//m_pStatusWidget->setOfflineStatus();
		m_pStatusWidget->SetMinSize(AnkerSize(760, 241));		
		m_pStatusWidget->SetMaxSize(AnkerSize(-1, 241));		

		m_pStatusWidget->SetSize(AnkerSize(760, 241));
		m_pStatusWidget->SetBackgroundColour(wxColour(41, 42, 45));		
		pBodyVSizer->Add(m_pStatusWidget, 0, wxEXPAND, 0);
		pBodyVSizer->AddSpacer(12);

		//hide adjust by alves
		//m_pStatusWidget->Bind(wxANKEREVT_LEVELING_STOPPED, [this](wxCommandEvent& event) {
		//	m_pAdjustWidget->setItemStatus(true, AnkerAdjustWidget::ITEM_LEVEL);
		//	});
		//m_pStatusWidget->Bind(wxANKEREVT_PRINTING_STARTED, [this](wxCommandEvent& event) {
		//	m_pAdjustWidget->setItemStatus(false, AnkerAdjustWidget::ITEM_LEVEL);
		//	});
	}

	pMainHSizer->Add(pBodyVSizer, 1 , wxEXPAND, 0);
	pMainHSizer->AddSpacer(12);

	{
		wxBoxSizer* pControlVSizer = new wxBoxSizer(wxVERTICAL);

		m_pControlWidget = new AnkerControlWidget(m_currentDeviceId, this);
		m_pControlWidget->Bind(wxCUSTOMEVT_EXTRUDEX, &AnkerDeviceControl::OnExtrude, this);
		m_pControlWidget->Bind(wxCUSTOMEVT_RETRACT, &AnkerDeviceControl::OnRetract, this);
		m_pControlWidget->Bind(wxCUSTOMEVT_STOP_EXTRUDEX, &AnkerDeviceControl::OnStopExtrude, this);
		m_pControlWidget->Bind(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE, [this](wxCommandEvent& event) {
				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_NOZZLE_UPDATE);
				wxVariant* pData =(wxVariant*) event.GetClientData();
				if (pData)
				{
					wxVariantList list = pData->GetList();
					auto tpValue = list[0]->GetString().ToStdString();
					auto tpStyle = list[1]->GetInteger();

					wxVariant eventData;
					eventData.ClearList();
					eventData.Append(wxVariant(tpValue));
					eventData.Append(wxVariant(tpStyle));
					evt.SetClientData(new wxVariant(eventData));				
					ProcessEvent(evt);
				}		
			});
		m_pControlWidget->Bind(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, [this](wxCommandEvent& event) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE);
			wxVariant* pData = (wxVariant*)event.GetClientData();
			if (pData)
				{
					wxVariantList list = pData->GetList();
					auto tpValue = list[0]->GetString().ToStdString();
					auto tpStyle = list[1]->GetInteger();

					wxVariant eventData;
					eventData.ClearList();
					eventData.Append(wxVariant(tpValue));
					eventData.Append(wxVariant(tpStyle));
					evt.SetClientData(new wxVariant(eventData));
					ProcessEvent(evt);
				}
			});
		m_pControlWidget->Bind(wxCUSTOMEVT_ZOFFSET_CHANGED, [this](wxCommandEvent &event) {

			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
			wxString zaxisData = wxString("0");
			double dvalue = 0;

			if (pData)
			{
				zaxisData = pData->GetData().ToStdString();
				if (!zaxisData.ToCDouble(&dvalue))
				{
					dvalue = 0;
				}
			}

			if (m_currentDeviceId.empty())
				return;
						
			auto currentDev = CurDevObject(m_currentDeviceId);

			if (!currentDev)
				return;

			currentDev->setZAxisCompensation(dvalue);
		});		
		m_pControlWidget->SetBackgroundColour(wxColour("#292A2D"));
		m_pControlWidget->SetMinSize(AnkerSize(450, 375));		
		m_pControlWidget->SetMaxSize(AnkerSize(450, 375));
		//m_pControlWidget->SetSize(AnkerSize(450, 375));
		
		//hide adjust by alves
//		m_pAdjustWidget = new AnkerAdjustWidget(m_currentDeviceId, this);
//		m_pAdjustWidget->SetBackgroundColour(wxColour("#292A2D"));
//#ifdef __APPLE__
//		m_pAdjustWidget->SetSize(AnkerSize(418, 146));
//		m_pAdjustWidget->SetMaxSize(AnkerSize(418, 146));
//#else
//		m_pAdjustWidget->SetSize(AnkerSize(450, 126));
//		m_pAdjustWidget->SetMaxSize(AnkerSize(450, 126));
//#endif

		Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
			auto ankerNet = AnkerNetInst();
			if (!ankerNet) {
				return;
			}
			if (m_currentDeviceId.empty())
				return;
			DeviceObjectBasePtr currentDev = ankerNet->getDeviceObjectFromSn(m_currentDeviceId);

			if (m_pControlWidget)
				m_pControlWidget->showEditTempertrueBtn(false);

			if (currentDev)
			{
				currentDev->setLevelBegin();
			}
		});

		m_pOtherWidget = new AnkerOtherWidget(this);		
		m_pOtherWidget->SetMinSize(AnkerSize(450, 330));
		//m_pOtherWidget->SetMinSize(AnkerSize(450, 330));
		//m_pOtherWidget->SetSize(AnkerSize(450, 330));
		
		
		pControlVSizer->AddSpacer(12);
		pControlVSizer->Add(m_pControlWidget,53, wxEXPAND, 0);
		pControlVSizer->AddSpacer(12);
		//hide adjust by alves
		//pControlVSizer->Add(m_pAdjustWidget, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, 0);
		//pControlVSizer->AddSpacer(12);
		pControlVSizer->Add(m_pOtherWidget, wxEXPAND | wxALL, wxEXPAND|wxALL, 0);
		pControlVSizer->AddSpacer(12);

		pMainHSizer->Add(pControlVSizer, 0, wxEXPAND, 0);
		pMainHSizer->AddSpacer(12);
	}


	SetSizer(pMainHSizer);
}

void AnkerDeviceControl::OnShow(wxShowEvent& event)
{
	activate(event.IsShown());
}

void AnkerControlWidget::onExtrudeBtnClicked(wxCommandEvent& event)
{
	if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
		Refresh();
		return;
	}

	wxString value = m_pLengthLineEdit->GetValue();
	wxString btnName = m_pExtrudeBtn->GetText();
	double dValue = 0;
	if (!value.ToDouble(&dValue))
	{
		dValue = 0;
	}

	if (dValue > 200)
	{
		dValue = 200;
	}
	else if (dValue < 20)
	{
		dValue = 20;
	}

	ANKER_LOG_INFO << "handle name " << btnName.ToStdString().c_str();
	ANKER_LOG_INFO << "handle value " << value.ToStdString().c_str();

	if (btnName == _L("common_print_controlbutton_stop"))
	{
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_STOP_EXTRUDEX);
		wxPostEvent(this, evt);
		resetExtrudeBtns();
	}
	else
	{
		wxString nozValue = m_pNozWidegt->getValue();
		int temperatureValue = 0;
		int nozzLeNum = -1;

		nozValue.ToInt(&temperatureValue);

		if (temperatureValue < 200)
			temperatureValue = 200;

		if (m_isMulitColor)
		{
			auto iter = m_btnList.begin();
			while (iter != m_btnList.end())
			{
				if ((*iter)->getBtnStatus() == BOX_SELECT)
				{
					wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
					if (pData)
					{
						wxString numTpData = pData->GetData();
						numTpData.ToInt(&nozzLeNum);
						break;
					}
				}
				++iter;
			}
		}

		ANKER_LOG_INFO << "handle length " << dValue;
		ANKER_LOG_INFO << "handle nozzle " << nozzLeNum;
		ANKER_LOG_INFO << "handle temperature " << temperatureValue;

		showStopBtnStatus(m_pExtrudeBtn);
		showDisAbelBtnStatus(m_pRetractBtn);

		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EXTRUDEX);
		wxVariant eventData;
		eventData.ClearList();
		eventData.Append(wxVariant(dValue));
		eventData.Append(wxVariant(temperatureValue));
		eventData.Append(wxVariant(nozzLeNum));
		evt.SetClientData(new wxVariant(eventData));
		wxPostEvent(this,evt);
	}
}

void AnkerControlWidget::onRetractBtnClicked(wxCommandEvent& event)
{
	if (PrintCheckHint::StopForV6UnInited(m_currentDeviceSn, this)) {
		Refresh();
		return;
	}

	wxString value = m_pLengthLineEdit->GetValue();
	wxString btnName = m_pRetractBtn->GetText();
	double dValue = 0;
	if (!value.ToDouble(&dValue))
	{
		dValue = 0;
	}

	if (dValue > 200)
	{
		dValue = 200;
	}
	else if (dValue < 20)
	{
		dValue = 20;
	} 
	ANKER_LOG_INFO << "handle name " << btnName.ToStdString().c_str();
	ANKER_LOG_INFO << "handle value " << value.ToStdString().c_str();

	wxString nozValue = m_pNozWidegt->getValue();
	int temperatureValue = 0;
	int nozzLeNum = -1;
	if (btnName == _L("common_print_controlbutton_stop"))
	{
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_STOP_EXTRUDEX);		
		wxPostEvent(this, evt);
		resetExtrudeBtns();
	}
	else
	{
		nozValue.ToInt(&temperatureValue);
		if (temperatureValue < 200)
			temperatureValue = 200;
		if (m_isMulitColor)
		{
			auto iter = m_btnList.begin();
			while (iter != m_btnList.end())
			{
				if ((*iter)->getBtnStatus() == BOX_SELECT)
				{
					wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
					if (pData)
					{
						wxString numTpData = pData->GetData();
						numTpData.ToInt(&nozzLeNum);
						break;
					}
				}
				++iter;
			}
		}
		ANKER_LOG_INFO << "handle length " << dValue;
		ANKER_LOG_INFO << "handle nozzle " << nozzLeNum;
		ANKER_LOG_INFO << "handle temperature " << temperatureValue;
		

		showStopBtnStatus(m_pRetractBtn);
		showDisAbelBtnStatus(m_pExtrudeBtn);
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_RETRACT);
		wxVariant eventData;
		eventData.ClearList();
		eventData.Append(wxVariant(dValue));
		eventData.Append(wxVariant(temperatureValue));
		eventData.Append(wxVariant(nozzLeNum));
		evt.SetClientData(new wxVariant(eventData));
		wxPostEvent(this, evt);
	}
}
void AnkerControlWidget::setNozzleMaxTemp(int temp)
{
	m_pNozWidegt->setMaxRage(temp);
}

AnkerOtherWidget::AnkerOtherWidget(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerOtherWidget::~AnkerOtherWidget()
{

}

void AnkerOtherWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));

	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pTitleVSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel* pTitleWidget = new wxPanel(this);
	pTitleWidget->SetBackgroundColour(wxColour("#292A2D"));
	pTitleWidget->SetMinSize(AnkerSize(450, 30));
	pTitleWidget->SetMaxSize(AnkerSize(2000, 30));

	m_title = new wxStaticText(this, wxID_ANY, _L("common_print_others_title"));
	m_title->SetFont(ANKER_BOLD_FONT_NO_1);
	m_title->SetSize(AnkerSize(100,30));
	m_title->SetMinSize(AnkerSize(100,30));
	m_title->SetForegroundColour(wxColour("#FFFFFF"));
	pTitleHSizer->AddSpacer(5);
	pTitleHSizer->Add(m_title, wxEXPAND, wxEXPAND | wxALL, 7);
	pTitleHSizer->AddStretchSpacer(1);
	pTitleWidget->SetSizer(pTitleHSizer);

	wxPanel* pLine = new wxPanel(this);
	pLine->SetBackgroundColour(wxColour("#3E3F42"));
	pLine->SetMinSize(wxSize(2000, 1));
	pLine->SetMaxSize(wxSize(2000, 1));

	pTitleVSizer->Add(pTitleWidget, wxEXPAND, wxEXPAND, 0);
	pTitleVSizer->Add(pLine, wxEXPAND, wxEXPAND, 0);
	pMainVSizer->Add(pTitleVSizer);

	wxPanel* pBodyPanel = new wxPanel(this);
	pBodyPanel->SetBackgroundColour(wxColour("#292A2D"));

	pBodyPanel->SetMinSize(AnkerSize(450, 450));
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* pTips = new wxStaticText(pBodyPanel, wxID_ANY, _L("common_print_others_comingsoon"));

	pTips->SetForegroundColour(wxColour("#999999"));
	pTips->SetFont(ANKER_FONT_NO_1);
	pBodyVSizer->AddStretchSpacer(1);
	pBodyVSizer->Add(pTips, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBodyVSizer->AddStretchSpacer(1);
	pBodyPanel->SetSizer(pBodyVSizer);

	pMainVSizer->Add(pBodyPanel, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	SetSizer(pMainVSizer);
}


BEGIN_EVENT_TABLE(AnkerAdjustWidget, wxControl)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerAdjustWidget, wxControl)

AnkerAdjustWidget::AnkerAdjustWidget(wxString sn, 
	wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_sn(sn)
{
	initUi();
}


AnkerAdjustWidget::AnkerAdjustWidget()
{

}

AnkerAdjustWidget::~AnkerAdjustWidget()
{

}


void AnkerAdjustWidget::setItemStatus(bool isAvailable, ItemStyle Style)
{
	switch (Style)
	{
	case AnkerAdjustWidget::ITEM_LEVEL:
		m_pAdjustItemWidget->setStatus(isAvailable);
		break;
	default:
		break;
	}
}

void AnkerAdjustWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));

	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	//title	;
	wxBoxSizer* pTitleVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel* pTitleWidget = new wxPanel(this);
	pTitleWidget->SetBackgroundColour(wxColour("#292A2D"));
	pTitleWidget->SetMinSize(AnkerSize(450, 35));

	m_title = new wxStaticText(this, wxID_ANY, _L("Adjustment"));
	m_title->SetForegroundColour(wxColour("#FFFFFF"));
	m_title->SetFont(ANKER_BOLD_FONT_NO_1);
	m_title->SetSize(AnkerSize(100, 30));
	m_title->SetMinSize(AnkerSize(100, 30));
	pTitleHSizer->AddSpacer(12);
	pTitleHSizer->Add(m_title, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxEXPAND | wxALL, 0);
	pTitleHSizer->AddStretchSpacer(1);

	pTitleVSizer->AddSpacer(7);
	pTitleVSizer->Add(pTitleHSizer, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxEXPAND | wxALL, 0);
	pTitleVSizer->AddSpacer(7);

	wxPanel* pLine = new wxPanel(this);
	pLine->SetBackgroundColour(wxColour("#3E3F42"));
	pLine->SetMinSize(wxSize(2000, 1));
	pLine->SetMaxSize(wxSize(2000, 1));

	pTitleWidget->SetSizer(pTitleVSizer);
	pMainVSizer->Add(pTitleWidget, 0, wxEXPAND, 0);
	pMainVSizer->Add(pLine, 0, wxEXPAND, 0);

	//content
	{
		m_pAdjustItemWidget = new AnkerAdjustItemWidget(this, wxID_ANY);		
		
		m_pAdjustItemWidget->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {

			wxString title = _L("Auto-level");

			auto deviceObj = CurDevObject(m_sn.ToStdString());
			if (!deviceObj)
				return;

			wxString time = wxString::Format("%d", /*deviceObj->time / 60*/10);			
			wxString content = _L("Auto-level will takes about " + time + " minutes, are you sure you want to continue");
			AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, content.ToStdString(wxConvUTF8), title.ToStdString(wxConvUTF8), true);
			if (result != AnkerMsgDialog::MSG_OK)
				return;

			setItemStatus(false, AnkerAdjustWidget::ITEM_LEVEL);

			wxCommandEvent evt = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED);
			wxPostEvent(this->GetParent(), evt);
			});		
		m_pAdjustItemWidget->setLogo("arrow");
		m_pAdjustItemWidget->setTitle(_L("Auto-level"));
		m_pAdjustItemWidget->setContent(_L("Leveling has 49 points and takes \nabout 10 minutes"));		
		m_pAdjustItemWidget->SetMinSize(AnkerSize(388, 85));
		m_pAdjustItemWidget->SetSize(AnkerSize(388, 85));
	}

	pMainVSizer->Add(m_pAdjustItemWidget, 96, wxEXPAND | wxALL, 10);

	SetSizer(pMainVSizer);
}

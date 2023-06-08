#include "AnkerDevice.hpp"
#include "AnkerVideo.hpp"
#include "AnkerLoadingMask.hpp"

#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "wx/sizer.h"
#include <wx/stattext.h>
#include "../Utils/DataManger.hpp"
#include <boost/bind.hpp>
#include <boost/signals2/connection.hpp>
#include <wx/clntdata.h>
#include "libslic3r/Utils.hpp"
#include "I18N.hpp"
#include "AnkerMsgDialog.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_DEVICE_LIST_REFRESH, wxCommandEvent);

AnkerDevice::AnkerDevice(wxWindow* parent,
						wxWindowID winid /*= wxID_ANY*/,
						const wxPoint& pos /*= wxDefaultPosition*/,
						const wxSize& size /*= wxDefaultSize*/)
						: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)						
	                    , m_pLoadingMask(nullptr)
{	
	setVideoCallBacks();
	initUi();
}

AnkerDevice::~AnkerDevice()
{
	PrintLog("AnkerDevice::~AnkerDevice()");
	unsetVideoCallBacks();
}


void AnkerDevice::showUnlogin()
{	
	m_pUnloginPanel->Show(true);
	setLoadingVisible(false);
	m_pNavBarWidget->Hide();
	m_pEmptyDeviceWidget->Hide();
	m_pOfflineDeviceWidget->Hide();
	
	clearWidegt();

	Refresh();
	Layout();
}


bool AnkerDevice::loadDeviceList(bool isFresh)
{
	bool res = false;	
	
	auto deviceList = Datamanger::GetInstance().getDeviceList();

	if (deviceList.size() > 0)
		res = true;
	else  //(deviceList.size() <= 0)
	{
		showNoDevice();
		return false;
	}

	if (isFresh && m_deviceWidgetList.size() > 0)
	{		
		removeExpiredDevice();
	}

	for (auto iter : deviceList) {
		
		//if exist and do not add.
		if(m_pNavBarWidget->checkTabExist(iter->m_sn))
			continue;

		if (m_pNavBarWidget->getCount() <= 0)
			m_currentDeviceId = iter->m_sn;

		//tab
		m_pNavBarWidget->addItem(iter->station_name, iter->m_sn);

		//device control		
		wxVariant snID(iter->m_sn);
		AnkerDeviceControl* pWidget = new AnkerDeviceControl(iter->m_sn, this);
		pWidget->SetClientObject(new wxStringClientData(snID));

		pWidget->Bind(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE, [this, pWidget](wxCommandEvent& event) {
						
			wxVariant* pData = (wxVariant*)event.GetClientData();
			std::string devSn = std::string();

			if (pWidget)
			{			
				wxStringClientData* pSnData = static_cast<wxStringClientData*>(pWidget->GetClientObject());
				if (pSnData)
					devSn = pSnData->GetData().ToStdString();
			}			

			if (pData)
			{
				wxVariantList list = pData->GetList();
				auto tpValue = list[0]->GetString().ToStdString();
				auto tpStyle = list[1]->GetInteger();

				Datamanger::GetInstance().updateTemperture(devSn, tpValue, tpStyle);
			}
			});

		pWidget->Bind(wxCUSTOMEVT_TEMPERATURE_BED_UPDATE, [this, pWidget](wxCommandEvent& event) {

			wxVariant* pData = (wxVariant*)event.GetClientData();
			std::string devSn = std::string();

			if (pWidget)
			{
				wxStringClientData* pSnData = static_cast<wxStringClientData*>(pWidget->GetClientObject());
				if (pSnData)
					devSn = pSnData->GetData().ToStdString();
			}

			if (pData)
			{
				wxVariantList list = pData->GetList();
				auto tpValue = list[0]->GetString().ToStdString();
				auto tpStyle = list[1]->GetInteger();

				Datamanger::GetInstance().updateTemperture(devSn, tpValue, tpStyle);
			}
			});

		pWidget->SetMinSize(wxSize(1200, 750));

		//add layout		
		int couts = m_pDeviceHSizer->GetItemCount();
		m_pDeviceHSizer->Add(pWidget, wxEXPAND, wxEXPAND | wxALIGN_RIGHT | wxALL, 12);	
		pWidget->Hide();
		m_deviceWidgetList.push_back(pWidget);
	}

	if (res)
		showDeviceList();
	else
		showNoDevice();	

	setLoadingVisible(false);
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
	Layout();
}


void AnkerDevice::showDeviceList()
{
 	m_pUnloginPanel->Hide();

 	m_pEmptyDeviceWidget->Hide();
 	m_pOfflineDeviceWidget->Hide();
	
	m_pNavBarWidget->Show(true);
	m_pNavBarWidget->showEmptyPanel(false);

	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == m_currentDeviceId)
		{
			wxString widgetId = pIterSnId->GetData().ToStdString();
			if (Datamanger::GetInstance().checkDeviceStatusFromSn(widgetId.ToStdString()))
				iter->Show(true);
			else
			{
				if(!m_pOfflineDeviceWidget->IsShown())
					m_pOfflineDeviceWidget->Show();
			}
			break;
		}
	}
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

	Layout();	
	Refresh();
}


void AnkerDevice::updateAboutMqttStatus(std::string sn)
{
	if (sn != m_currentDeviceId)
		return;

	updateLevel(sn);	
	updateTemperatrue(sn);
}

void AnkerDevice::updateLevel(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->updateLevel(sn);
		}

	}
}

void AnkerDevice::updateDeviceList(bool rc)
{
	std::size_t id = std::hash<std::thread::id>()(std::this_thread::get_id());
	PrintLog("AnkerDevice::updateDeviceList: " + std::to_string(rc) + ", thread id: " + std::to_string(id));
	loadDeviceList(rc);
}

void AnkerDevice::updateFileTransferStatus(std::string sn, int progress)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->updateFileTransferStatus(sn, progress);
		}
	}
}


void AnkerDevice::showMsgDialog(const std::string& title, const std::string& content, bool isCancel /*= false*/)
{
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, content, title, isCancel);
	if (result != AnkerMsgDialog::MSG_OK)
		return;
}


void AnkerDevice::showWarningDialog(const std::string& title, const std::string& content, const std::string& sn, WARNING_STYLE warningStyle)
{
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, content, title, true);
	if (result != AnkerMsgDialog::MSG_OK)
		return;

	if(warningStyle == WARNING_STYLE::LEVEL_WARNING)
		updateLevel(sn);
	else {

	}
}


void AnkerDevice::updateTemperatrue(const std::string& sn)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == sn)
		{
			iter->updateTemperatrue(sn);
		}
		
	}
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
	AnkerNetBase* ankerNet = Datamanger::GetInstance().getAnkerNetBase();
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

void AnkerDevice::unsetVideoCallBacks()
{
	AnkerNetBase* ankerNet = Datamanger::GetInstance().getAnkerNetBase();
	if (ankerNet) {
		ankerNet->unsetAllVideoCallBacks();
	}
}

void AnkerDevice::showDevice(const std::string& snId)
{
	for (auto iter : m_deviceWidgetList)
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>(iter->GetClientObject());
		std::string widgetId = std::string();

		if (pIterSnId)
			widgetId = pIterSnId->GetData().ToStdString();

		if (widgetId == snId)
		{
			if (Datamanger::GetInstance().checkDeviceStatusFromSn(widgetId))
			{
				m_pOfflineDeviceWidget->Hide();
				iter->Show(true);
			}
			else
				showOfflineDevice();
		}
		else
			iter->Hide();
	}	

	Layout();
	Refresh();
}


void AnkerDevice::clearWidegt()
{
	m_pNavBarWidget->clearItem();	

	int widgetCount = m_deviceWidgetList.size();

	for(int i=0; i < widgetCount; i++)
		if(m_pDeviceHSizer->GetItemCount() >= 4)
			m_pDeviceHSizer->Remove(4);

	for (auto iter : m_deviceWidgetList)
	{
		delete iter;
	}

	m_deviceWidgetList.clear();
}

void AnkerDevice::initUi()
{
	SetBackgroundColour(wxColour("#18191B"));
	m_pMainHSizer = new wxBoxSizer(wxHORIZONTAL);


	//don't login and show the panel
	{
 		m_pUnloginPanel = new AnkerUnLoginPanel(this);
		m_pUnloginPanel->Bind(wxCUSTOMEVT_LOGIN_CLCIKED, [this](wxCommandEvent& event) {			
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_LOGIN_CLCIKED);			
			wxPostEvent(this->GetParent(), evt);
			});
		
 		m_pMainHSizer->Add(m_pUnloginPanel, wxEXPAND | wxALL,  wxEXPAND|wxALL, 0);

	}

	//logined and no device, has device by the navbarwdiget control
	{
		m_pDeviceHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pNavBarWidget = new AnkerNavBar(this);
		
		m_pNavBarWidget->Bind(wxCUSTOMEVT_SWITCH_DEVICE, [this](wxCommandEvent& event) {									
			
			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
			if(pData)
			{
				std::string snId = pData->GetData().ToStdString();
				m_currentDeviceId = snId;
				showDevice(snId);
			}
			});

		//update device list
		m_pNavBarWidget->Bind(wxCUSTOMEVT_BTN_CLICKED, [this](wxCommandEvent& event){
			
			Datamanger::GetInstance().getMachineListEx(true);
			});

		m_pNavBarWidget->SetMinSize(wxSize(215, 750));		
		m_pNavBarWidget->Hide();
		m_pNavBarWidget->showEmptyPanel(true);

		m_pEmptyDeviceWidget = new AnkerEmptyDevice(this);		
		m_pEmptyDeviceWidget->Hide();
		m_pOfflineDeviceWidget = new AnkerOfflineDevice(this);
		m_pOfflineDeviceWidget->Hide();

		m_pDeviceHSizer->Add(m_pNavBarWidget, 0, wxEXPAND | wxALL, 12);				
		m_pDeviceHSizer->Add(m_pEmptyDeviceWidget, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL, 0);		
		m_pDeviceHSizer->Add(m_pOfflineDeviceWidget, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);

		m_pMainHSizer->Add(m_pDeviceHSizer, wxEXPAND | wxALL, wxEXPAND | wxALL, 2);
	}

	SetSizer(m_pMainHSizer);
}


void AnkerDevice::removeExpiredDevice()
{
	//clear ExpiredDevice tab Widget
	m_pNavBarWidget->clearExpiredTab(m_currentDeviceId);

	auto tempDeviceList = m_deviceWidgetList;

	//clear ExpiredDevice Widget
	for (auto iter = m_deviceWidgetList.begin(); iter != m_deviceWidgetList.end(); ) 
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		std::string widgetId = std::string();
		if (pIterSnId)
		{
			widgetId = pIterSnId->GetData().ToStdString();
			if (!Datamanger::GetInstance().checkSnExist(widgetId))
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
	
	if (m_deviceWidgetList.size() > 0)
	{
		auto iter = m_deviceWidgetList.begin();
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		if (pIterSnId)
		{
			wxString widgetId = pIterSnId->GetData().ToStdString();
			m_currentDeviceId = widgetId.ToStdString();
			if (Datamanger::GetInstance().checkDeviceStatusFromSn(widgetId.ToStdString()))
				(*iter)->Show();
			else
				m_pOfflineDeviceWidget->Show();
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
AnkerControlWidget::AnkerControlWidget(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerControlWidget::~AnkerControlWidget()
{
	
}


void AnkerControlWidget::updateTemperatrue(const std::string& sn)
{
	auto devceiObj = Datamanger::GetInstance().getDeviceObjectFromSn(sn);
	if (!devceiObj)
		return;
	wxString nozzleNum = wxString::Format(wxT("%d"), devceiObj->nozzle[0]); 
	wxString hotdBedNum = wxString::Format(wxT("%d"), devceiObj->hotdBed[0]);

	wxString desNozzleNum = wxString::Format(wxT("%d"), devceiObj->nozzle[1]);
	wxString desHotdBedNum = wxString::Format(wxT("%d"), devceiObj->hotdBed[1]);

	m_pNozWidegt->setTemperatureNum(nozzleNum);
	m_pHeatBedWidegt->setTemperatureNum(hotdBedNum);

	if (devceiObj->deviceStatus == MQTT_PRINT_EVENT_IDLE)
	{
		m_pNozWidegt->setTipsText("Nozzle:--"  + _L("#"));
		m_pHeatBedWidegt->setTipsText("Heatbed:--" +  _L("#"));
	}
	else
	{
		m_pNozWidegt->setTipsText("Nozzle:" + desNozzleNum + _L("#"));
		m_pHeatBedWidegt->setTipsText("Heatbed:" + desHotdBedNum + _L("#"));
	}

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


void AnkerControlWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));

	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	//title	;
	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel* pTitleWidget = new wxPanel(this);
	pTitleWidget->SetBackgroundColour(wxColour("#292A2D"));
	pTitleWidget->SetMinSize(wxSize(420, 30));

	m_title = new wxStaticText(this, wxID_ANY, L"Control");	
	wxFont titleFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
	titleFont.SetUnderlined(false);
	m_title->SetFont(titleFont);
	m_title->SetSize(wxSize(100, 30));
	m_title->SetMinSize(wxSize(100, 30));
	m_title->SetForegroundColour(wxColour("#FFFFFF"));
	pTitleHSizer->Add(m_title, wxEXPAND, wxEXPAND | wxALL, 7);
	pTitleHSizer->AddStretchSpacer(1);

	wxPanel* pLine = new wxPanel(this);
	pLine->SetBackgroundColour(wxColour("#3E3F42"));
	pLine->SetMinSize(wxSize(2000, 1));
	pLine->SetMaxSize(wxSize(2000, 1));

	pTitleWidget->SetSizer(pTitleHSizer);
	pMainVSizer->Add(pTitleWidget, 0, wxEXPAND, 0);
	pMainVSizer->Add(pLine, 0, wxEXPAND, 0);

	wxPanel* pBodyPanel = new wxPanel(this);
	pBodyPanel->SetMinSize(wxSize(420, 90));
	pBodyPanel->SetBackgroundColour(wxColour("#292A2D"));
	wxBoxSizer* pBodyHSizer = new wxBoxSizer(wxHORIZONTAL);

	//content
	{
		m_pNozWidegt = new AnkerTemperatureWidget(pBodyPanel);	
		m_pNozWidegt->setMinRage(0);
		m_pNozWidegt->setMaxRage(260);
		
		m_pNozWidegt->Bind(wxCUSTOMEVT_TEMPERATURE_UPDATE, [this](wxCommandEvent& event) {

				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE);
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

		m_pNozWidegt->setTipsText("Nozzle:--");						
		m_pNozWidegt->SetMinSize(wxSize(165, 110));

		m_pHeatBedWidegt = new AnkerTemperatureWidget(pBodyPanel);
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

		m_pHeatBedWidegt->setTipsText("Heatbed:--");		
		m_pHeatBedWidegt->SetMinSize(wxSize(165, 110));

		pBodyHSizer->AddStretchSpacer(1);
		pBodyHSizer->Add(m_pNozWidegt, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pBodyHSizer->AddSpacer(100);
		pBodyHSizer->Add(m_pHeatBedWidegt, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pBodyHSizer->AddStretchSpacer(1);
	}

	pBodyPanel->SetSizer(pBodyHSizer);

	pMainVSizer->Add(pBodyPanel, 95, wxEXPAND | wxALL, 0);

	SetSizer(pMainVSizer);
}


AnkerTemperatureWidget::AnkerTemperatureWidget(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
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
	m_pTipsText->SetMinSize(wxSize(120, 20));
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
	
	wxFont textFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_BOLD);
	wxClientDC dc(this);
	dc.SetFont(textFont);
	wxSize size = dc.GetTextExtent(tempValue);
	int textWidth = size.GetWidth();

	m_pNumText->SetMinSize(wxSize(textWidth, 43));
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

void AnkerTemperatureWidget::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));	

	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);

	{
		wxBoxSizer* pNumShowHSizer = new wxBoxSizer(wxHORIZONTAL);
		wxString defaultStr = "--" + _L("#");

		m_pNumText = new wxStaticText(this, wxID_ANY, defaultStr);
		wxFont textFont(25, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_BOLD);
		wxClientDC dc(this);
		dc.SetFont(textFont);
		wxSize size = dc.GetTextExtent(defaultStr);		
		m_pNumText->SetMinSize(size);
		
		textFont.SetUnderlined(false);
		m_pNumText->SetFont(textFont);
		m_pNumText->SetForegroundColour(wxColour("#999999"));

		m_pNumUnitText = new wxStaticText(this, wxID_ANY, _L("#"));
		m_pNumUnitText->SetMinSize(wxSize(28, 28));
		wxFont unitFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_BOLD);
		unitFont.SetUnderlined(false);

		m_pNumUnitText->SetFont(unitFont);
		m_pNumUnitText->SetForegroundColour(wxColour("#999999"));
		m_pNumUnitText->Hide();

		pNumShowHSizer->AddStretchSpacer(1);
		pNumShowHSizer->Add(m_pNumText, wxSizerFlags().Expand().Border(wxTOP | wxLEFT, 10));
		pNumShowHSizer->Add(m_pNumUnitText, wxEXPAND | wxALL, wxEXPAND| wxALL, 0);
		pNumShowHSizer->AddStretchSpacer(1);

		pBodyVSizer->Add(pNumShowHSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	}

	{
		m_pNormalPanel = new wxPanel(this, wxID_ANY);
		m_pNormalPanel->SetMinSize(wxSize(180, 35));
		m_pNormalPanel->SetSize(wxSize(180, 35));
		wxBoxSizer* pNormalHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pNormalPanel->SetBackgroundColour(wxColour("#292A2D"));
		m_pTipsText = new wxStaticText(m_pNormalPanel, wxID_ANY, "");		
		m_pTipsText->SetMinSize(wxSize(100, 20));

#ifdef _WIN32
		wxFont tipsFont(10, wxFONTSTYLE_NORMAL, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
#else
		wxFont tipsFont(14, wxFONTSTYLE_NORMAL, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
		tipsFont.SetUnderlined(false);
		m_pTipsText->SetFont(tipsFont);
		m_pTipsText->Wrap(500);
		m_pTipsText->SetForegroundColour(wxColour("#999999"));

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("appbar_edit_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(16, 16);
		wxBitmap scaledBitmap(image);
		m_pEditBtn = new wxButton(m_pNormalPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_pEditBtn->SetBitmap(scaledBitmap);
		m_pEditBtn->SetMinSize(wxSize(20, 20));
		m_pEditBtn->SetSize(wxSize(20, 20));
		m_pEditBtn->SetBackgroundColour(GetBackgroundColour());

		m_pEditBtn->Bind(wxEVT_BUTTON, [this](wxEvent& event) {

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
		pNormalHSizer->AddStretchSpacer(1);
		pNormalHSizer->Add(m_pTipsText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pNormalHSizer->Add(m_pEditBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pNormalHSizer->AddStretchSpacer(1);
		m_pNormalPanel->SetSizer(pNormalHSizer);

		m_pEditPanel = new wxPanel(this, wxID_ANY);
		m_pEditPanel->SetBackgroundColour(wxColour(255, 0, 0));
		m_pEditPanel->SetMinSize(wxSize(180, 30));
		m_pEditPanel->SetSize(wxSize(180, 30));
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
		m_pTemperatureEdit->SetMinSize(wxSize(55, 25));
		m_pTemperatureEdit->SetSize(wxSize(55, 25));
		m_pTemperatureEdit->SetHint("0~260");

		wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
		validator.SetCharIncludes("0123456789");

		m_pTemperatureEdit->SetValidator(validator);

		m_pTemperatureEdit->Bind(wxEVT_TEXT_ENTER, &AnkerTemperatureWidget::OnEnter, this);
		m_pTemperatureEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, &AnkerTemperatureWidget::OnTextCtrlEditFinished, this);

		m_TemperatureUnitText = new wxStaticText(m_pEditPanel, wxID_ANY, _L("#"));
		m_TemperatureUnitText->SetForegroundColour(wxColour("#999999"));
		m_TemperatureUnitText->SetMinSize(wxSize(20, 20));

#ifdef _WIN32
		wxFont unitFont(14, wxFONTSTYLE_NORMAL, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
#else
		wxFont unitFont(16, wxFONTSTYLE_NORMAL, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
		unitFont.SetUnderlined(false);
		m_TemperatureUnitText->SetFont(unitFont);

		pEditHSizer->AddStretchSpacer(1);
		pEditHSizer->Add(m_pTemperatureEdit, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pEditHSizer->Add(m_TemperatureUnitText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pEditHSizer->AddStretchSpacer(1);

		m_pEditPanel->SetSizer(pEditHSizer);
		m_pEditPanel->SetBackgroundColour(wxColour("#292A2D"));
		m_pEditPanel->Hide();

		wxBoxSizer* pBodyHSizer = new wxBoxSizer(wxHORIZONTAL);
		pBodyHSizer->Add(m_pNormalPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pBodyHSizer->Add(m_pEditPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);				
		pBodyVSizer->Add(pBodyHSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	}
	pMainHSizer->Add(pBodyVSizer, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);


	SetSizer(pMainHSizer);

}


void AnkerTemperatureWidget::OnTextCtrlEditFinished(wxCommandEvent& event)
{
	wxString numValue = m_pTemperatureEdit->GetValue();
	int resValue = 0;
	numValue.ToInt(&resValue);
	if (resValue > m_maxSize)
	{
		numValue = wxString::Format(wxT("%d"), m_maxSize);
	}
	else if (resValue < m_minSize)
	{
		numValue = wxString::Format(wxT("%d"), m_minSize);
	}

	if (!m_pTemperatureEdit->GetValue().IsEmpty())
	{
		wxVariant numData(numValue);
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_UPDATE);
		evt.SetClientObject(new wxStringClientData(numData));
		ProcessEvent(evt);

		wxString tempText = m_tipsText;
		tempText = tempText.Left(tempText.length() - 2) + numValue + _L("#");

		wxClientDC dc(this);
		wxCoord width, height;
		dc.GetTextExtent(tempText, &width, &height);
#ifndef __APPLE__
		m_pTipsText->SetMinSize(wxSize(120, 20));
#endif
		m_pTipsText->SetLabelText(tempText);
	}
	m_isEditStatus = false;
	m_pTemperatureEdit->Clear();
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
	wxString numValue = m_pTemperatureEdit->GetValue();

	int resValue = 0;
	numValue.ToInt(&resValue);

	if (resValue > m_maxSize)
	{
		numValue = wxString::Format(wxT("%d"), m_maxSize);
	}
	else if (resValue < m_minSize)
	{
		numValue = wxString::Format(wxT("%d"), m_minSize);
	}

	if(!m_pTemperatureEdit->GetValue().IsEmpty())
	{
		wxVariant numData(numValue);
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_UPDATE);
		evt.SetClientObject(new wxStringClientData(numData));
		ProcessEvent(evt);
		
		wxString tempText = m_tipsText;
		tempText = tempText.Left(tempText.length() - 2) + numValue + _L("#");		
		wxClientDC dc(this);
		wxCoord width, height;
		dc.GetTextExtent(tempText, &width, &height);
#ifndef __APPLE__
		m_pTipsText->SetMinSize(wxSize(120, 20));
#endif
		m_pTipsText->SetLabelText(tempText);
	}

	m_pTemperatureEdit->Clear();

	m_pEditPanel->Hide();
	m_isEditStatus = false;
	m_pNormalPanel->Show();
	m_pEditBtn->Show(true);
	m_pTipsText->Show(true);
	
	Refresh();
	Layout();
	m_pNumText->SetForegroundColour(wxColour("#999999"));		
	m_pNumUnitText->SetForegroundColour(wxColour("#999999"));

}

AnkerDeviceControl::AnkerDeviceControl(std::string currentDeviceID, wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_currentDeviceId(currentDeviceID)
{
	initUi();
}

AnkerDeviceControl::~AnkerDeviceControl()
{

}


void AnkerDeviceControl::updateTemperatrue(const std::string &sn)
{
	m_pControlWidget->updateTemperatrue(sn);
}


void AnkerDeviceControl::updateLevel(const std::string& sn)
{
	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceId);

	if (!currentDev)
		return;

	if ((currentDev->deviceStatus == MQTT_PRINT_EVENT_IDLE)
		|| currentDev->deviceStatus == MQTT_PRINT_EVENT_COMPLETED
		|| currentDev->deviceStatus == MQTT_PRINT_EVENT_STOPPED
		|| currentDev->getCustomDeviceStatus() == CustomDeviceStatus::CustomDeviceStatus_File_Transfer
		|| currentDev->generalException == GeneralException2Gui_Auto_Level_Error)
	{
		m_pAdjustWidget->setItemStatus(true, AnkerAdjustWidget::ItemStyle::ITEM_LEVEL);	
	}
	else
	{	
		m_pAdjustWidget->setItemStatus(false, AnkerAdjustWidget::ItemStyle::ITEM_LEVEL);		
	}
	
	if(currentDev->deviceStatus==MQTT_PRINT_EVENT_LEVELING ||
		currentDev->deviceStatus == MQTT_PRINT_EVENT_HEATING)		
	{
		m_pControlWidget->showEditTempertrueBtn(false);
	}
	else
	{
		m_pControlWidget->showEditTempertrueBtn(true);
	}

	if (currentDev->deviceStatus == MQTT_PRINT_EVENT_IDLE)
	{
		m_pControlWidget->updateWorkStatus(false);
	}
	else
	{
		m_pControlWidget->updateWorkStatus(true);
	}		

	m_pStatusWidget->updateStatus(sn);
	m_pStatusWidget->requestCallback();
}

void AnkerDeviceControl::updateFileTransferStatus(const std::string& sn, int progress)
{
	m_pStatusWidget->updateFileTransferStatus(sn, progress);
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



void AnkerDeviceControl::initUi()
{
	SetBackgroundColour(wxColour("#1F2022"));
	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);
	{
		m_pVideoWidget = new AnkerVideo(this, m_currentDeviceId);
		m_pVideoWidget->SetMinSize(wxSize(760, 495));
		m_pVideoWidget->SetBackgroundColour(wxColour("#292A2D"));
		//pBodyVSizer->AddSpacer(6);
		pBodyVSizer->Add(m_pVideoWidget, 495, wxEXPAND, 0);
		pBodyVSizer->AddSpacer(12);
	}

	{
		m_pStatusWidget = new AnkerTaskPanel(m_currentDeviceId, this);
		m_pStatusWidget->SetMinSize(wxSize(760, 237));
		m_pStatusWidget->SetBackgroundColour(wxColour(41, 42, 45));
		m_pStatusWidget->Bind(wxANKEREVT_LEVELING_STOPPED, [this](wxCommandEvent& event) {
			m_pAdjustWidget->setItemStatus(true, AnkerAdjustWidget::ITEM_LEVEL);
		});
		pBodyVSizer->Add(m_pStatusWidget, 237, wxEXPAND, 0);
		pBodyVSizer->AddSpacer(6);
	}

	pMainHSizer->Add(pBodyVSizer, 760, wxEXPAND, 0);
	pMainHSizer->AddSpacer(12);

	{
		wxBoxSizer* pControlVSizer = new wxBoxSizer(wxVERTICAL);

		m_pControlWidget = new AnkerControlWidget(this);
		m_pControlWidget->Bind(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE, [this](wxCommandEvent& event) {
				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TEMPERATURE_NO_UPDATE);
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
		m_pControlWidget->SetBackgroundColour(wxColour("#292A2D"));
		m_pControlWidget->SetMinSize(wxSize(418, 125));

		m_pAdjustWidget = new AnkerAdjustWidget(m_currentDeviceId, this);
		Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {

			Datamanger& dm = Datamanger::GetInstance();
			DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceId);

			if (m_pControlWidget)
				m_pControlWidget->showEditTempertrueBtn(false);

			if (currentDev)
			{
				currentDev->setLevelBegin();
			}
			});
		m_pAdjustWidget->SetBackgroundColour(wxColour("#292A2D"));
#ifdef __APPLE__
		m_pAdjustWidget->SetMinSize(wxSize(418, 146));
#else
		m_pAdjustWidget->SetMinSize(wxSize(418, 126));
#endif

		m_pOtherWidget = new AnkerOtherWidget(this);		
		m_pOtherWidget->SetMinSize(wxSize(418, 475));

		
		pControlVSizer->Add(m_pControlWidget, 125, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);
		pControlVSizer->AddSpacer(12);
		pControlVSizer->Add(m_pAdjustWidget, 126, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);
		pControlVSizer->AddSpacer(12);
		pControlVSizer->Add(m_pOtherWidget, 475, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);		
				
		pMainHSizer->Add(pControlVSizer, 418, wxEXPAND, 0);
	}


	SetSizer(pMainHSizer);
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
	pTitleWidget->SetMinSize(wxSize(420, 30));
	pTitleWidget->SetMaxSize(wxSize(2000, 30));

	m_title = new wxStaticText(this, wxID_ANY, L"Others");
	wxFont titleFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
	titleFont.SetUnderlined(false);
	m_title->SetFont(titleFont);
	m_title->SetSize(wxSize(100,30));
	m_title->SetMinSize(wxSize(100,30));
	m_title->SetForegroundColour(wxColour("#FFFFFF"));
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

	pBodyPanel->SetMinSize(wxSize(420, 450));
	wxBoxSizer* pBodyVSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* pTips = new wxStaticText(pBodyPanel, wxID_ANY, L"Coming soon");

	pTips->SetForegroundColour(wxColour("#999999"));

	pBodyVSizer->AddStretchSpacer(1);
	pBodyVSizer->Add(pTips, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBodyVSizer->AddStretchSpacer(1);
	pBodyPanel->SetSizer(pBodyVSizer);

	pMainVSizer->Add(pBodyPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

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
	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel* pTitleWidget = new wxPanel(this);
	pTitleWidget->SetBackgroundColour(wxColour("#292A2D"));
	pTitleWidget->SetMinSize(wxSize(420, 33));

	m_title = new wxStaticText(this, wxID_ANY, L"Adjustment");
	wxFont titleFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
	titleFont.SetUnderlined(false);
	m_title->SetForegroundColour(wxColour("#FFFFFF"));
	m_title->SetFont(titleFont);
	m_title->SetSize(wxSize(100,33));
	m_title->SetMinSize(wxSize(100, 33));
	pTitleHSizer->Add(m_title, wxEXPAND, wxEXPAND | wxALL, 7);
	pTitleHSizer->AddStretchSpacer(1);

	wxPanel* pLine = new wxPanel(this);
	pLine->SetBackgroundColour(wxColour("#3E3F42"));
	pLine->SetMinSize(wxSize(2000, 1));
	pLine->SetMaxSize(wxSize(2000, 1));

	pTitleWidget->SetSizer(pTitleHSizer);
	pMainVSizer->Add(pTitleWidget, 0, wxEXPAND, 0);
	pMainVSizer->Add(pLine, 0, wxEXPAND, 0);

	//content
	{
		m_pAdjustItemWidget = new AnkerAdjustItemWidget(this, wxID_ANY);		
		
		m_pAdjustItemWidget->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {

			wxString title = _L("Auto-level");
			auto deviceObj = Datamanger::GetInstance().getDeviceObjectFromSn(m_sn.ToStdString());
			wxString time = wxString::Format("%s", deviceObj->time / 60);			
			wxString content = _L("Auto-level will takes about" + time + "munutes, are you sure you want to continue");
			AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, content.ToStdString(), title.ToStdString(), true);
			if (result != AnkerMsgDialog::MSG_OK)
				return;

			wxCommandEvent evt = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED);
			wxPostEvent(this->GetParent(), evt);
			});		
		m_pAdjustItemWidget->setLogo("arrow");
		m_pAdjustItemWidget->setTitle("Auto-level");
		m_pAdjustItemWidget->setContent("Leveling has 49 points and takes \nabout 10 minutes");		
		m_pAdjustItemWidget->SetMinSize(wxSize(388, 64));
	}


	pMainVSizer->Add(m_pAdjustItemWidget, 96, wxEXPAND | wxALL, 16);

	SetSizer(pMainVSizer);
}

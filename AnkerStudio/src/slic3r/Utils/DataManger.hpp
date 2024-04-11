#ifndef _DATAMANAGER_H_
#define _DATAMANAGER_H_

#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include "../GUI/Network/basetype.hpp"
#include "AnkerNetBase.h" 
#include "../GUI/Network/DeviceObject.h"
#include "../GUI/Network/SystemInfo.hpp"

#include <atomic>

#include <boost/signals2.hpp>
#include <thread>
#include <wx/event.h>
#include <wx/timer.h>
#include <list>

#define ANKER_MAKE_NAME "AnkerMake Studio"


typedef AnkerNetBase* (*AnkerNetInstance)();

wxDECLARE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_WARNING_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE, wxCommandEvent);	 
wxDECLARE_EVENT(wxCUSTOMEVT_ACCOUNT_EXTRUSION, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_OTA_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_Z_AXIS_COMPENSATION, wxCommandEvent);

using namespace HttpsType;
using namespace MqttType;
using namespace P2pType;


class wxVectorClientData : public wxClientData
{
public:
	wxVectorClientData() : m_datas() { }
	wxVectorClientData(const std::vector<std::pair<wxVariant, wxVariant>>& datas) : m_datas(datas) { }
	void SetData(const std::vector<std::pair<wxVariant, wxVariant>>& datas) { m_datas = datas; }
	const std::vector<std::pair<wxVariant, wxVariant>>& GetData() const { return m_datas; }

	void Append(const std::pair<wxVariant, wxVariant>& data) { m_datas.push_back(data); }

private:
	std::vector<std::pair<wxVariant, wxVariant>> m_datas;
};

struct AnkerMakeVersion {
	std::string appname = "";
	std::string version = "";
	std::string localLanguage = "";
	std::string localCountry = "";
	int code = 0;
};

enum OtaCheckType {
	OtaCheckType_Unknown,
	OtaCheckType_Manual,
	OtaCheckType_AppBegin,
	OtaCheckType_24Hours,
};

class Datamanger
{
public:
	static Datamanger& GetInstance();

	~Datamanger();
	AnkerNetBase* getAnkerNetBase();

	static int versionToCode(const std::string& version);
	

	void setMainObj(wxWindow *pWindow);
	void destroy();
	void clearAllDeviceObjects();
	LOGIN_DATA			m_loginData;
	PRINT_MACHINE_LIST  m_printMachineList;
	MultiColor_MACHINE_LIST m_multiColorMachineList;

	DSK_INFO			m_dskInfo;
	PRINT_LOG			m_printLog;

	OTA_DEVICE_VERSION_INFO    m_otaVersionInfo;

	QUEUE_SYS_MSG		m_sysMsg;

	COUNTRY_INFO		m_countyInfo;
	USER_INFO			m_userInfo;

	UPDATE_USER_INFO    m_updateUserInfo;//m_loginData install of it

	VEDIO_LIST_INFO		m_vedioLsitInfo;
			
	std::vector<VEDIO_CFG> m_vedioCfgList;

	std::vector<QUEUE_WAIT_INVITE>	m_waitInviteList;

	std::vector < DEVICE_SHARED_MEMBER_LIST> m_deviceSharedMemberList;

	CLIENT_OTA_INFO		m_otaInfo;
	SysInfoCollector m_sysInfo;
    FeedBackInfo m_currentFeedBackInfo;
    
	static AnkerMakeVersion version;
	static OtaCheckType m_otaCheckType;
	//P2pType::RequestP2pType m_p2pType = P2pType::RequestP2pType_Unknown;
	std::string m_aKeyPrintGcodePath;

	std::string getAuthToken();
	void setAuthToken(const std::string token = "");

	std::string getUuid() const;
	void accountExtrusion();
	void logout();

	static void uploadLog(const std::string& logPath, const std::string& zipName, FeedBackInfo info = FeedBackInfo());
	static void removeLogZip(const std::string& logPathZip);

	void postRequestLog(const std::string& logPath);
	void postFeedBack(FeedBackInfo info = FeedBackInfo());
    
	void onGetUserInfo();
	void queryOTAInformation();
	int onLoginMqtt();
	void setHeaderList(std::vector<std::string>& headerList);
	void getMachineList(void* pUsr = nullptr);
	void getMachineListEx(bool refreshDevice);
	void getAKeyPrintMachineListEx(wxWindow* window, bool refreshDevice = true);
	void getMultiColorMachineList();
	void postMachineCollector();
	bool checkSnExist(const std::string& sn);
	void getDeviceDsk(const std::string& sn, void* pUsr);
	bool getTransferFileStatus();
	bool getTransferFileStatus(const std::string& sn);
	void setTransferFileStatus(bool value);

	bool deviceCanPrint(const std::string& sn);
	void onAKeyPrint();
	void setCurrentSn(const std::string& sn);
	std::string getCurrentSn();

	void updateProgressValue(int value);
	void mqttRecovery(bool connected);
	void setPlaterPtr(wxWindow* palter);

	boost::signals2::signal<void(int)> transferFileSig;
	boost::signals2::connection transferFilecConnect(const boost::signals2::slot<void(int)>& slot);

	void sendSigToOTAUpdate(const std::vector<std::pair<wxVariant, wxVariant>>& data);
	void sendSigAccountExtrusion();
	void sendSigToSwitchPrintPage();
	void sendSigToUpdateDevice();
	void setRefreshDeviceStatus(bool refreshStatus);
	void sendSigToUpdateDeviceStatus(const std::string& sn, int type = -1);
	void sendSigToTransferFileProgressValue(const std::string& sn, int value);
	void sendSigToMessageBox(const std::string& title, const std::string& text, const std::string& sn);
	void sendSigToMessageBox(const NetworkMsgText::NetworkMsg& msg, const std::string &sn);
	void sendSigToUpdateZAxisCompensation(const std::string& sn);

	void sendMessage2Mqtt(const std::string& topic, const std::string& msg);
	void sendShowDeviceListDialog();

	// Widgets DeviveObject info
	std::vector<std::string> getAllDeviceSn();
	bool checkDeviceStatusFromSn(const std::string& sn);	
	DeviceObjectPtr getDeviceObjectFromSn(const std::string& sn);
	bool checkIsSingleColorFromSn(const std::string& sn);
	mqtt_device_type getDeviceTypeFromSn(const std::string& sn);
	void updateDeviceObject(const DeviceObject& deviceObject, const std::string& sn);
	void updateDeviceObject(const std::string& sn);
	void updateDeviceObjectPrintFileName(const std::string& sn, const std::string& fileName);
	void appendDeviceObject(const DeviceObjectPtr& deviceObjectPtr);
	void deleteDeviceObject(const DeviceObjectPtr& deviceObjectPtr);
	void deleteDeviceObjectFromSn(const std::string& sn);
	std::list<DeviceObjectPtr> getDeviceList()const;
	std::list<DeviceObjectPtr>::iterator& getDeviceObjectIterFromSn(const std::string& sn);
	std::list<DeviceObjectPtr> getMultiColorPartsDeviceList() const;
	int findDeviceObjectIndex(const std::string& sn, std::list<DeviceObjectPtr>::iterator& deviceObjectIt);
	int getDeviceObjectsSize() const;
	int getMultiColorDeviceObjectsSize() const;
	void setCurrentDeviceObject(const std::string& sn);
	void updateTemperture(const std::string& sn, std::string temperture, const int& tempertureStye);
	DeviceObjectPtr getCurrentObject() const;
	std::string getCurrentDeviceSn() const;
	PRINT_MACHINE* getMachine(std::string& sn);
	DSK_CFG* getDskCfg(std::string& sn);

	// Mqtt device info
	bool isContainDevice(const std::string& sn);
	void queryAllDeviceStatus();
	void updataData(const std::string& sn, std::map<mqtt_command_type_e, CmdTypePtr> data);
	void updateCmdData(const std::string& sn, mqtt_command_type_e cmdType, CmdTypePtr);
	void appendData(const std::string& sn, std::map<mqtt_command_type_e, CmdTypePtr> data);
	void appendCmdData(const std::string& sn, mqtt_command_type_e cmdType, CmdTypePtr);
	void deleteData(const std::string& sn, mqtt_command_type_e cmdType);
	void deleteDevice(const std::string& sn);

	void handleMqttData(const std::string& sn, const std::string& dataStr, int dataLength);
	MqttDataMapPtr getMqttDataMap() { return m_mqttDatasPtr; }

	inline void setP2POperationType(P2pType::P2POperationType type) { m_P2POperationType = type; };
	inline P2pType::P2POperationType getP2POperationType() { return m_P2POperationType; };

	void boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, const std::string strFuncName, 
		const unsigned int lineNumber);

	std::string getMqttCertificateName();
	std::string  getMqttAddress();

protected:
	Datamanger(); 
	
	Datamanger(const Datamanger&) = delete;
	Datamanger& operator=(const Datamanger&) = delete;


private:
	MqttDataMapPtr  m_mqttDatasPtr = nullptr;
	

	// All print ctrl widget DeviceObject.
	std::list<DeviceObjectPtr> m_deviceObjects;
	DeviceObjectPtr m_currentDeviceObject = nullptr;
	mqtt_command_type_e currentCmdType = MQTT_CMD_MAX;
	
	static bool m_isInit;	

	AnkerNetBase* pAnkerNet = nullptr;
	std::string m_currentSn; 
	std::mutex m_snMutex;

	wxWindow* m_palter = nullptr;
	wxWindow* pMainWindow = nullptr;
	bool m_transfering = false;
	std::mutex m_transfer_mutex;
	bool   m_isRefreshDevice = false;
	static P2POperationType m_P2POperationType;

	std::atomic_bool  m_pcOnlined = true;
	std::mutex m_tokenMutex;
};

#endif

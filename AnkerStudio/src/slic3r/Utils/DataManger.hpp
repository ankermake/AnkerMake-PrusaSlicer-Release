#ifndef _DATAMANAGER_H_
#define _DATAMANAGER_H_

#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include "../GUI/Network/basetype.hpp"
#include "../GUI/Network/AnkerNetBase.h" 
#include "../GUI/Network/DeviceObject.h"
#include "../GUI/Network/NetworkAes.h"


#include <boost/signals2.hpp>
#include <thread>
#include <wx/event.h>

typedef AnkerNetBase* (*AnkerNetInstance)();

wxDECLARE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_WARNING_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);

using namespace HttpsType;
using namespace MqttType;
using namespace P2pType;

class Datamanger
{
public:
	static Datamanger& GetInstance();

	~Datamanger();
	AnkerNetBase* getAnkerNetBase();
	
	void setMainObj(wxWindow *pWindow);
	void destroy();
	void clearAllDeviceObjects();
	EnvironmentType m_environment;
	LOGIN_DATA			m_loginData;
	PRINT_MACHINE_LIST  m_printMachineList;

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

	//P2pType::RequestP2pType m_p2pType = P2pType::RequestP2pType_Unknown;
	std::string m_aKeyPrintGcodePath;
	EnvironmentType getEnvironmentType(const std::string& loginUrl);
	void setEnvironmentType(const EnvironmentType& type);
	void setEnvironmentType(const std::string& loginUrl);
	void setEnvironmentType();

	static std::string utf8_to_string(const wxString& wxstr);
	static size_t find_last(const std::string& source, const std::string& sub);
	static size_t find_path_filename_index(const std::string& path);
	static size_t onGetDskInfoCallBack(char* dest, size_t size, size_t nmemb, void* userp);
	static size_t onGetMachineListCallBack(char* dest, size_t size, size_t nmemb, void* userp);
	static int recvMqttMessageCallBack(void* context, char* topicName, int topiclen);
	static size_t onGetUserInfoCallBack(char* dest, size_t size, size_t nmemb, void* userp);
	static std::string getUuid();
	static std::string getSnFromTopic(const std::string& topicStr);

	void logout();

	void onGetUserInfo();
	int onLoginMqtt();
	void setHeaderList(std::vector<std::string>& headerList);
	void getMachineList(void* pUsr = nullptr);
	void getMachineListEx(bool refreshDevice);
	bool checkSnExist(const std::string& sn);
	void getDeviceDsk(const std::string& sn, void* pUsr);
	bool getTransferFileStatus();
	void setTransferFileStatus(bool value);

	bool deviceCanPrint(const std::string& sn);
	void onAKeyPrint();
	void setCurrentSn(const std::string& sn);
	std::string getCurrentSn();

	void updateProgressValue(int value);
	void setPlaterPtr(wxWindow* palter);

	boost::signals2::signal<void(int)> transferFileSig;
	boost::signals2::connection transferFilecConnect(const boost::signals2::slot<void(int)>& slot);

	void sendSigToUpdateDevice();
	void setRefreshDeviceStatus(bool refreshStatus);
	void sendSigToUpdateDeviceStatus(const std::string& sn);
	void sendSigToTransferFileProgressValue(const std::string& sn, int value);
	void sendSigToMessageBox(const std::string& title, const std::string& text, const std::string& sn);

	void sendMessage2Mqtt(const std::string& topic, const std::string& msg);
	void sendShowDeviceListDialog();

	// Widgets DeviveObject info
	std::vector<std::string> getAllDeviceSn();
	bool checkDeviceStatusFromSn(const std::string& sn);	
	DeviceObjectPtr getDeviceObjectFromSn(const std::string& sn);
	void updateDeviceObject(const DeviceObject& deviceObject, const std::string& sn);
	void updateDeviceObject(const std::string& sn);
	void appendDeviceObject(const DeviceObjectPtr& deviceObjectPtr);
	void deleteDeviceObject(const DeviceObjectPtr& deviceObjectPtr);
	void deleteDeviceObjectFromSn(const std::string& sn);
	std::list<DeviceObjectPtr> getDeviceList()const;
	std::list<DeviceObjectPtr>::iterator& getDeviceObjectIterFromSn(const std::string& sn);
	int findDeviceObjectIndex(const std::string& sn, std::list<DeviceObjectPtr>::iterator& deviceObjectIt);
	int getDeviceObjectsSize() const;
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

	void setP2POperationType(P2POperationType type) { };
	P2POperationType getP2POperationType() { return P2P_IDLE; };

	void boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, const std::string strFuncName, 
		const unsigned int lineNumber);

protected:
	Datamanger() {

	};
	
	Datamanger(const Datamanger&) = delete;
	Datamanger& operator=(const Datamanger&) = delete;

private:
	std::string getMqttCertificateName();
	std::string  getMqttAddress();

private:
	MqttDataMapPtr  m_mqttDatasPtr = nullptr;

	// All print ctrl widget DeviceObject.
	std::list<DeviceObjectPtr> m_deviceObjects;
	DeviceObjectPtr m_currentDeviceObject = nullptr;
	mqtt_command_type_e currentCmdType;

	AnkerNetBase* pAnkerNet = nullptr;
	std::string m_currentSn; 

	wxWindow* m_palter = nullptr;
	wxWindow* pMainWindow = nullptr;
	bool m_transfering = false;
	std::mutex m_transfer_mutex;
	bool   m_isRefreshDevice = false;
};

#endif
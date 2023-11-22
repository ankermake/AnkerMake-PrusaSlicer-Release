#include "DataManger.hpp"
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/dynlib.h>
#include <wx/dynload.h>
#include <jansson.h>
#include "../GUI/Plater.hpp"
#include <boost/bind.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/log/trivial.hpp>
#include <ctime>

#include "libslic3r/Utils.hpp"


#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#elif __APPLE__

#endif // _WIN32


#define ANKER_NETWORK_LOG(level) BOOST_LOG_TRIVIAL(level)

bool Datamanger::m_isInit = false;
AnkerMakeVersion Datamanger::version;
OtaCheckType Datamanger::m_otaCheckType = OtaCheckType_Unknown;

std::mutex g_mutex;
using namespace Slic3r::GUI;
wxDEFINE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_WARNING_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ACCOUNT_EXTRUSION, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_OTA_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_Z_AXIS_COMPENSATION, wxCommandEvent);


using namespace P2pType;
P2POperationType Datamanger::m_P2POperationType = P2P_IDLE;


Datamanger::Datamanger() {
};

Datamanger& Datamanger::GetInstance()
{
	if (!m_isInit) {
		m_isInit = true;

	}
	static Datamanger instance;
	return instance;
}

AnkerNetBase* Datamanger::getAnkerNetBase()
{
	return pAnkerNet;
}

void Datamanger::setMainObj(wxWindow* pWindow)
{

}

void Datamanger::destroy()
{

}

void Datamanger::clearAllDeviceObjects()
{

}


std::string Datamanger::getUuid() const
{
	return "";
}


void Datamanger::accountExtrusion()
{

}

void Datamanger::logout()
{

}

int Datamanger::versionToCode(const std::string& version)
{

		return 0;

}

void Datamanger::uploadLog(const std::string& logPath, const std::string& logZipFullPath, FeedBackInfo info)
{

}

void Datamanger::removeLogZip(const std::string& logPathZip)
{

}

void Datamanger::postRequestLog(const std::string& logPath)
{

}


void Datamanger::postFeedBack(FeedBackInfo info)
{

}

void Datamanger::onGetUserInfo()
{

}

void Datamanger::queryOTAInformation()
{

}

int Datamanger::onLoginMqtt()
{
	return 0;
}

void Datamanger::setHeaderList(std::vector<std::string>& headerList)
{

}

void Datamanger::getMachineList(void* pUsr)
{

}
void Datamanger::getMachineListEx(bool refreshDevice)
{

}

void Datamanger::getAKeyPrintMachineListEx(wxWindow* window, bool refreshDevice)
{

}

void Datamanger::getMultiColorMachineList()
{

}

void Datamanger::postMachineCollector()
{

}

bool Datamanger::checkSnExist(const std::string& sn)
{


	return false;
}

void Datamanger::setCurrentSn(const std::string& sn)
{

}

std::string Datamanger::getCurrentSn()
{
	std::string sn;
	return sn;
}

void Datamanger::updateProgressValue(int value)
{

}

void Datamanger::mqttRecovery(bool connected)
{

}

void Datamanger::setPlaterPtr(wxWindow* palter)
{

}

boost::signals2::connection Datamanger::transferFilecConnect(const boost::signals2::slot<void(int)>& slot)
{
	return transferFileSig.connect(slot);
}


void Datamanger::sendSigToOTAUpdate(const std::vector<std::pair<wxVariant, wxVariant>>& data)
{

}

void Datamanger::sendSigAccountExtrusion()
{

}

void Datamanger::sendSigToSwitchPrintPage()
{

}

void Datamanger::sendSigToUpdateDevice()
{

}

void Datamanger::setRefreshDeviceStatus(bool refreshStatus)
{

}

void Datamanger::sendSigToUpdateDeviceStatus(const std::string& sn, int type)
{

}

void Datamanger::sendSigToTransferFileProgressValue(const std::string& sn, int value)
{

}

void Datamanger::sendSigToMessageBox(const std::string& title, const std::string& text, const std::string& sn)
{

}

void Datamanger::sendSigToMessageBox(const NetworkMsgText::NetworkMsg& msg, const std::string& sn)
{

}

void Datamanger::sendSigToUpdateZAxisCompensation(const std::string& sn)
{

}


void Datamanger::sendMessage2Mqtt(const std::string& topic, const std::string& msg)
{

}

void Datamanger::sendShowDeviceListDialog()
{

}

std::vector<std::string> Datamanger::getAllDeviceSn()
{
	std::vector<std::string> snVec;
	return snVec;
}


bool Datamanger::checkDeviceStatusFromSn(const std::string& sn)
{

	return false;
}

DeviceObjectPtr Datamanger::getDeviceObjectFromSn(const std::string& sn)
{

	return nullptr;
}


bool Datamanger::checkIsSingleColorFromSn(const std::string& sn)
{
		return true;
}

mqtt_device_type Datamanger::getDeviceTypeFromSn(const std::string& sn)
{
	return DEVICE_UNKNOWN_TYPE;
}

void Datamanger::updateDeviceObject(const DeviceObject& deviceObject, const std::string& sn)
{

}

void Datamanger::updateDeviceObject(const std::string& sn)
{

}

void Datamanger::updateDeviceObjectPrintFileName(const std::string& sn, const std::string& fileName)
{

}

void Datamanger::appendDeviceObject(const DeviceObjectPtr& deviceObjectPtr)
{

}

void Datamanger::deleteDeviceObject(const DeviceObjectPtr& deviceObjectPtr)
{

}

void Datamanger::deleteDeviceObjectFromSn(const std::string& sn)
{

}


std::list<DeviceObjectPtr> Datamanger::getDeviceList() const
{
	return m_deviceObjects;
}

std::list<DeviceObjectPtr>::iterator& Datamanger::getDeviceObjectIterFromSn(const std::string& sn)
{
	std::list<DeviceObjectPtr>::iterator deviceObjectIt = m_deviceObjects.end();
	return deviceObjectIt;
}

std::list<DeviceObjectPtr> Datamanger::getMultiColorPartsDeviceList() const
{
	std::list<DeviceObjectPtr> multiColorDeviceObjects;
	return multiColorDeviceObjects;
}

std::string Datamanger::getAuthToken()
{
	return "";
}

void Datamanger::setAuthToken(const std::string token)
{

}

int Datamanger::findDeviceObjectIndex(const std::string& sn, std::list<DeviceObjectPtr>::iterator& deviceObjectIt)
{

	return -1;
}

int Datamanger::getDeviceObjectsSize() const
{
	return m_deviceObjects.size();
}

int Datamanger::getMultiColorDeviceObjectsSize() const
{
	int count = 0;

	return count;
}

void Datamanger::setCurrentDeviceObject(const std::string& sn)
{

}


void Datamanger::updateTemperture(const std::string& sn, std::string temperture, const int& tempertureStye)
{

}

DeviceObjectPtr Datamanger::getCurrentObject() const
{
	return m_currentDeviceObject;
}

PRINT_MACHINE* Datamanger::getMachine(std::string& sn)
{

	return nullptr;
}

DSK_CFG* Datamanger::getDskCfg(std::string& sn)
{

	return nullptr;
}

std::string Datamanger::getCurrentDeviceSn() const
{

	return std::string();
}

bool Datamanger::deviceCanPrint(const std::string& sn)
{


	return false;
}

void Datamanger::onAKeyPrint()
{

}

void Datamanger::setTransferFileStatus(bool value)
{

}

bool Datamanger::getTransferFileStatus()
{
	bool transfering = false;
	return transfering;
}

bool Datamanger::getTransferFileStatus(const std::string& sn)
{

	return false;
}

void Datamanger::getDeviceDsk(const std::string& sn, void* pUsr)
{

}

Datamanger::~Datamanger()
{
}


std::string Datamanger::getMqttCertificateName()
{
	std::string certificateName = "";
	return certificateName;
}

std::string Datamanger::getMqttAddress()
{
	std::string address = "";
	return address;
}

bool Datamanger::isContainDevice(const std::string& sn)
{
	return false;
}

void Datamanger::queryAllDeviceStatus()
{
}

void Datamanger::updataData(const std::string& sn, std::map<mqtt_command_type_e, CmdTypePtr> data)
{
}

void Datamanger::updateCmdData(const std::string& sn, mqtt_command_type_e cmdType, CmdTypePtr data)
{

}

void Datamanger::appendData(const std::string& sn, std::map<mqtt_command_type_e, CmdTypePtr> data)
{

}

void Datamanger::appendCmdData(const std::string& sn, mqtt_command_type_e cmdType, CmdTypePtr data)
{

}

void Datamanger::deleteData(const std::string& sn, mqtt_command_type_e cmdType)
{

}

void Datamanger::deleteDevice(const std::string& sn)
{

}

void Datamanger::handleMqttData(const std::string& sn, const std::string& dataStr, int dataLength)
{

}

void Datamanger::boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, const std::string strFuncName, const unsigned int lineNumber)
{
}

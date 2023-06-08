#include "DataManger.hpp"
#include <wx/string.h>
#include <wx/msgdlg.h>
#include "../GUI/Plater.hpp"
#include "../GUI/Network/Ak_zip.h"
#include <boost/bind.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/log/trivial.hpp>

#include "libslic3r/Utils.hpp"

#define ANKER_NETWORK_LOG(level) BOOST_LOG_TRIVIAL(level)

using namespace Slic3r::GUI;
wxDEFINE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_WARNING_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);


Datamanger& Datamanger::GetInstance()
{
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
	if (m_deviceObjects.size() > 0) {
		m_deviceObjects.clear();
	}
}

EnvironmentType Datamanger::getEnvironmentType(const std::string& loginUrl)
{
	EnvironmentType type = EnvironmentType_Unknown;
	return type;
}

void Datamanger::setEnvironmentType()
{
	setEnvironmentType(getEnvironmentType(m_loginData.domain));
}

void Datamanger::setEnvironmentType(const std::string& loginUrl)
{
	setEnvironmentType(getEnvironmentType(loginUrl));
}

void Datamanger::setEnvironmentType(const EnvironmentType& type)
{

}

std::string Datamanger::getUuid()
{
	return "uuid";
}

std::string Datamanger::getSnFromTopic(const std::string& topicStr)
{
	size_t pos1 = topicStr.find("");
	std::string tmpSn = "";
	if (pos1 != std::string::npos) {
		pos1 += 6;
		tmpSn = topicStr.substr(pos1, topicStr.length() - pos1);
		size_t pos2 = tmpSn.find("/");
		if (pos2 != std::string::npos) {
			tmpSn = tmpSn.substr(0, pos2);
		}
	}
	return tmpSn;
}



std::string Datamanger::utf8_to_string(const wxString& wxstr)
{
	std::string str;
	str.reserve(wxstr.length());

	for (wxString::const_iterator it = wxstr.begin(); it != wxstr.end(); ++it)
	{
		wxChar c = *it;

		if (c < 0x80)
		{
			str += static_cast<char>(c);
		}
		else if (c < 0x800)
		{
			str += static_cast<char>((c >> 6) | 0xC0);
			str += static_cast<char>((c & 0x3F) | 0x80);
		}
		else if (c < 0x10000)
		{
			str += static_cast<char>((c >> 12) | 0xE0);
			str += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
			str += static_cast<char>((c & 0x3F) | 0x80);
		}
		else
		{
			str += static_cast<char>((c >> 18) | 0xF0);
			str += static_cast<char>(((c >> 12) & 0x3F) | 0x80);
			str += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
			str += static_cast<char>((c & 0x3F) | 0x80);
		}
	}

	return str;
}

size_t Datamanger::find_last(const std::string& source, const std::string& sub)
{
	auto it = std::search(source.rbegin(), source.rend(), sub.rbegin(), sub.rend());
	if (it != source.rend()) {
		return std::distance(it, source.rend()) - sub.length();
	}
	return std::string::npos;
}

size_t Datamanger::find_path_filename_index(const std::string& path)
{
	std::string fullpath = path;
	int index = std::string::npos;
#ifdef _WIN32
	index = fullpath.find_last_of("\\");
#elif __APPLE__
	index = fullpath.find_last_of("/");
#endif
	return index;
}

size_t Datamanger::onGetDskInfoCallBack(char* dest, size_t size, size_t nmemb, void* userp)
{
	return 0;
}

size_t Datamanger::onGetMachineListCallBack(char* dest, size_t size, size_t nmemb, void* userp)
{
	return 0;
}

int Datamanger::recvMqttMessageCallBack(void* context, char* topicName, int topiclen)
{
	return 0;
}

size_t Datamanger::onGetUserInfoCallBack(char* dest, size_t size, size_t nmemb, void* userp)
{

	return 0; 
}

void Datamanger::logout()
{	
}

void Datamanger::onGetUserInfo()
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

bool Datamanger::checkSnExist(const std::string& sn)
{
	return false;
}

void Datamanger::setCurrentSn(const std::string& sn)
{

}

std::string Datamanger::getCurrentSn()
{
	return m_currentSn;
}

void Datamanger::updateProgressValue(int value)
{

}

void Datamanger::setPlaterPtr(wxWindow* palter)
{
	m_palter = palter;
}

boost::signals2::connection Datamanger::transferFilecConnect(const boost::signals2::slot<void(int)>& slot)
{
	return transferFileSig.connect(slot);
}


void Datamanger::sendSigToUpdateDevice()
{
}

void Datamanger::setRefreshDeviceStatus(bool refreshStatus)
{
	m_isRefreshDevice = refreshStatus;
}

void Datamanger::sendSigToUpdateDeviceStatus(const std::string& sn)
{
}

void Datamanger::sendSigToTransferFileProgressValue(const std::string& sn, int value)
{
}

void Datamanger::sendSigToMessageBox(const std::string& title, const std::string& text, const std::string& sn)
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
	if (m_deviceObjects.empty()) {
		return snVec;
	}
	//std::unique_lock<std::mutex> lock(g_mutex2);
	for (auto it = m_deviceObjects.begin(); it != m_deviceObjects.end(); it++) {
		snVec.push_back((*it)->m_sn);
	}
	return snVec;
}


bool Datamanger::checkDeviceStatusFromSn(const std::string& sn)
{
	for (auto iter : m_deviceObjects)
	{
		if (iter->m_sn == sn)
		{
			if (iter->onlined)
				return true;
			else
				return false;
		}
	}
	return false;
}

DeviceObjectPtr Datamanger::getDeviceObjectFromSn(const std::string& sn)
{
	for (auto iter : m_deviceObjects)
	{
		if (iter->m_sn == sn)
		{	
			return iter;
		}
	}
	
	return nullptr;
}

void Datamanger::updateDeviceObject(const DeviceObject& deviceObject, const std::string& sn)
{
	//std::lock_guard<std::mutex>	lock(g_mutex);
	std::list<DeviceObjectPtr> ::iterator iter;
	if (findDeviceObjectIndex(sn, iter) < 0) {
		return;
	}

	*(*iter) = deviceObject;
}

void Datamanger::updateDeviceObject(const std::string& sn)
{
}

void Datamanger::appendDeviceObject(const DeviceObjectPtr& deviceObjectPtr)
{
	if (deviceObjectPtr == nullptr)
	{
		return;
	}
	m_deviceObjects.push_back(deviceObjectPtr);
}

void Datamanger::deleteDeviceObject(const DeviceObjectPtr& deviceObjectPtr)
{
	if (deviceObjectPtr == nullptr || m_deviceObjects.empty()) {
		return;
	}
	m_deviceObjects.remove(deviceObjectPtr);
}

void Datamanger::deleteDeviceObjectFromSn(const std::string& sn)
{
	if (m_deviceObjects.empty()) {
		return;
	}
	std::list<DeviceObjectPtr>::iterator it;
	if (findDeviceObjectIndex(sn, it) >= 0) {
		m_deviceObjects.erase(it);
	}
}


std::list<DeviceObjectPtr> Datamanger::getDeviceList() const
{
	return m_deviceObjects;
}

std::list<DeviceObjectPtr>::iterator& Datamanger::getDeviceObjectIterFromSn(const std::string& sn)
{
	std::list<DeviceObjectPtr>::iterator deviceObjectIt = m_deviceObjects.end();
	findDeviceObjectIndex(sn, deviceObjectIt);
	return deviceObjectIt;
}

int Datamanger::findDeviceObjectIndex(const std::string& sn, std::list<DeviceObjectPtr>::iterator& deviceObjectIt)
{
	return -1;
}

int Datamanger::getDeviceObjectsSize() const
{
	return m_deviceObjects.size();
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

void Datamanger::boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, const std::string strFuncName,
    const unsigned int lineNumber) {
	
}


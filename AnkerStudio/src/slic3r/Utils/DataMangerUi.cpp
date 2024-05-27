#include "DataMangerUi.hpp"
#include "libslic3r/Utils.hpp"
#include "../GUI/Plater.hpp"
#include <wx/protocol/http.h>
#include <functional>
#include <slic3r/GUI/Network/SystemInfo.hpp>

#include "slic3r/GUI/FilamentMaterialConvertor.hpp"
#include "slic3r/GUI/FilamentMaterialManager.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/Utils/JsonHelp.hpp"
#include "slic3r/GUI/format.hpp"
#include "../../AnkerComFunction.hpp"
#include "AnkerNetBase.h"
#include <boost/bind.hpp>

wxDEFINE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ACCOUNT_EXTRUSION, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_HTTP_CONNECT_ERROR, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_OTA_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ACCOUNT_LOGOUT, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_GET_CONMENT_FLAGS, wxCommandEvent);

#define ANKER_NET_TRACE BOOST_LOG_TRIVIAL(trace)	<< BASE_INFO + "[trace]"
#define ANKER_NET_DEBUG BOOST_LOG_TRIVIAL(debug)	<< BASE_INFO + "[debug]"
#define ANKER_NET_INFO	BOOST_LOG_TRIVIAL(info)		<< BASE_INFO + "[info]"
#define ANKER_NET_WARN	BOOST_LOG_TRIVIAL(warning)	<< BASE_INFO + "[warn]"
#define ANKER_NET_ERROR BOOST_LOG_TRIVIAL(error)	<< BASE_INFO + "[error]"
#define ANKER_NET_FATAL BOOST_LOG_TRIVIAL(fatal)	<< BASE_INFO + "[fatal]"

#include "AnkerNetBase.h"
#include <slic3r/Utils/GcodeInfo.hpp>

using namespace AnkerNet;
typedef AnkerNet::AnkerNetBase* (*AnkerNetInstance)();

using namespace NetworkMsgText;

#define ANKER_MAKE_NAME "AnkerMake Studio"

DatamangerUi::DatamangerUi() 
{
}

bool DatamangerUi::LoadNetLibrary(wxWindow* pWindow, bool silence)
{
	if (pAnkerNet) {
		return true;
	}

	auto ret = m_netModuleMgr.LoadLibrary(pWindow, silence);
	ANKER_LOG_INFO << "init ret: " << ret << ", silence: " << silence;
	return ret;
}

void DatamangerUi::SetMainWindow(wxWindow* pWindow)
{
	if (pAnkerNet) {
		pMainWindow = pWindow;
		auto para = GetNetPara();

		LogOutputCallBackFunc logOutputCb = std::bind(&DatamangerUi::boostLogPrint, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
		pAnkerNet->setLogOutputCallBack(logOutputCb);
		pAnkerNet->Init(para);

		pAnkerNet->SetCallback_RecoverCurl(DatamangerUi::Callback_RecoverCurl);
		pAnkerNet->SetCallback_OtaInfoRecv(DatamangerUi::Callback_OtaUpdate);
		pAnkerNet->SetCallback_FilamentRecv(DatamangerUi::Callback_FilamentUpdate);
		pAnkerNet->SetCallback_ConmentFlagsRecv(DatamangerUi::Callback_GetConmentFlags);

		InitAllCallBacks();
	}
}

size_t find_path_filename_index(const std::string& path)
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

AKNMT_LOG_LEVEL DatamangerUi::GetMqttLogLevel()
{
	Slic3r::GUI::GUI_App* gui = dynamic_cast<Slic3r::GUI::GUI_App*>(Slic3r::GUI::GUI_App::GetInstance());
	AKNMT_LOG_LEVEL logLevel = AKNMT_LOG_LEVEL::FATAL;
	if (gui != nullptr) {
		int level = (int)AKNMT_LOG_LEVEL::FATAL;
		auto strLevel = gui->app_config->get("mqtt_trace_level");
		// protect the level when read from config file
		try {
			level = std::stoi(strLevel);
		}
		catch (...) {
			level = (int)logLevel;
		}
		if (level > (int)AKNMT_LOG_LEVEL::FATAL) {
			level = (int)AKNMT_LOG_LEVEL::FATAL;
		}
		if (level < (int)AKNMT_LOG_LEVEL::MAX) {
			level = (int)AKNMT_LOG_LEVEL::MAX;
		}
		logLevel = static_cast<AKNMT_LOG_LEVEL>(level);
	}
	return logLevel;
}

AnkerNetInitPara DatamangerUi::GetNetPara()
{
	AnkerNetInitPara para;

	auto sysInfo = GetSysInfo();

	wxString languageCode = Slic3r::GUI::wxGetApp().current_language_code_safe();
	int index = languageCode.find('_');

	para.App_name = std::string(ANKER_MAKE_NAME);
	para.Model_type = "PC";
	para.App_version_V = std::string("V") + std::string(SLIC3R_VERSION);
	para.App_version = std::string(SLIC3R_VERSION);
	para.Version_code = versionToCode(std::string(SLIC3R_VERSION));
	para.Country = languageCode.substr(index + 1, languageCode.Length() - index).ToStdString(wxConvUTF8);
	para.Language = languageCode.substr(0, index).ToStdString(wxConvUTF8);
	para.Openudid = sysInfo.m_machineid;
	para.Os_version = sysInfo.m_os_version;
#ifdef _WIN32
	para.Os_type = "Windows";
#elif __APPLE__
	para.Os_type = "MacOS";
#endif
	para.Content_Type = "Content-Type:application/json;charset=UTF-8";


	std::string exePath = wxStandardPaths::Get().GetExecutablePath().ToStdString(wxConvUTF8);
	int nameIndex = find_path_filename_index(exePath) + 1;
	para.exeDir = exePath;
	para.dataDir = Slic3r::data_dir();
	para.resourcesDir = Slic3r::resources_dir();
	para.certDir = exePath.substr(0, nameIndex);
	para.logDir = Slic3r::getLogDirPath().string();

	
	para.aknmtLogLevel = GetMqttLogLevel();

	para.sysInfo = sysInfo;

	return para;
}

void DatamangerUi::InitAllCallBacks()
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	using namespace std;
	using namespace std::placeholders;
	ankerNet->SetsendSigHttpError(bind(&DatamangerUi::sendSigHttpError, this, ::_1));
	ankerNet->SetsendSigToSwitchPrintPage(bind(&DatamangerUi::sendSigToSwitchPrintPage, this, ::_1));
	ankerNet->SetsendSigToUpdateDevice(bind(&DatamangerUi::sendSigToUpdateDevice, this));
	ankerNet->SetsendSigToUpdateDeviceStatus(bind(&DatamangerUi::sendSigToUpdateDeviceStatus, this, ::_1, ::_2));
	ankerNet->SetsendSigToTransferFileProgressValue(bind(&DatamangerUi::sendSigToTransferFileProgressValue, this, ::_1, ::_2, ::_3));
	ankerNet->SetsendShowDeviceListDialog(bind(&DatamangerUi::sendShowDeviceListDialog, this));
	ankerNet->SetGeneralExceptionMsgBox(bind(&DatamangerUi::GeneralExceptionMsgBox, this, ::_1));
	ankerNet->SetSendSigAccountLogout(bind(&DatamangerUi::SendSigAccountLogout, this));

	ankerNet->setWebLoginCallBack([this](const std::string& ab_code) {
		wxCommandEvent event(wxCUSTOMEVT_DEAL_PRIVACY_CHOICES);
		event.SetClientData(new wxIntPtr(ab_code == "US"));
		wxPostEvent(this->pMainWindow,event);
		}, nullptr);
}

int DatamangerUi::versionToCode(const std::string& version)
{
	if (version.empty()) {
		return 0;
	}

	int major, minor, patch;
	char dot;

	std::istringstream iss(version);

	iss >> major >> dot >> minor >> dot >> patch;
	//TODO:  1.5.3 ==> 1 * 100000 + 5 * 1000 + 3
	int code = major * 100000 + minor * 1000 + patch;
	return code;
}

void DatamangerUi::boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, const std::string strFuncName,
	const unsigned int lineNumber) {
	std::string strPrefixInfo = "[" + strFileName + ":" + std::to_string(lineNumber) + " " + strFuncName + "]";
	switch (logLevel) {
	case boost::log::trivial::trace:
		ANKER_NET_TRACE << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	case boost::log::trivial::debug:
		ANKER_NET_DEBUG << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	case boost::log::trivial::info:
		ANKER_NET_INFO << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	case boost::log::trivial::warning:
		ANKER_NET_WARN << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	case boost::log::trivial::error:
		ANKER_NET_ERROR << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	case boost::log::trivial::fatal:
		ANKER_NET_FATAL << strPrefixInfo.c_str() << strLogMsg.c_str();
		break;
	default:
		break;
	}
}

DatamangerUi& DatamangerUi::GetInstance()
{
	static DatamangerUi instance;
	return instance;
}

AnkerNetBase* DatamangerUi::getAnkerNetBase()
{
	return pAnkerNet;
}

SysInfo DatamangerUi::GetSysInfo()
{
	static bool isGet = false;
	static SysInfo info;
	if (isGet)
		return info;
	info = SysInfoCollector::GetSysInfo();
	isGet = true;
	return info;
}

void DatamangerUi::ResetMainObj()
{
	pMainWindow = nullptr;
	if (pAnkerNet) {
		pAnkerNet->logout();
	}
}

void DatamangerUi::setPlaterPtr(wxWindow* palter)
{
	m_palter = palter;
}

void DatamangerUi::sendSigAccountExtrusion()
{
	ANKER_LOG_INFO << "sendSigAccountExtrusion.";
	if (pMainWindow != nullptr) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ACCOUNT_EXTRUSION);
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::sendSigHttpError(HttpError error)
{
	ANKER_LOG_INFO << "sendSigHttpConnectError.";

	if (error == UserNotIdentified)
	{
		sendSigAccountExtrusion();
	}
	else if (error != NoError)
	{
		if (pMainWindow != nullptr) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_HTTP_CONNECT_ERROR);
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant(error));
			evt.SetClientData(new wxVariant(eventData));
			wxPostEvent(pMainWindow, evt);
		}
	}
}

void DatamangerUi::sendSigToSwitchPrintPage(const std::string& sn)
{
	if (pMainWindow != nullptr) {
		wxVariant varData(sn);
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE);
		evt.SetClientObject(new wxStringClientData(varData));

		ANKER_LOG_INFO << "sendSigToSwitchPrintPage sn: " << sn;
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::sendSigToUpdateDevice()
{
	if (pMainWindow != nullptr) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_DEVICE_LIST_UPDATE);
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::sendSigToUpdateDeviceStatus(const std::string& sn, aknmt_command_type_e type)
{
	if (pMainWindow != nullptr) {
		wxVariant eventData;
		eventData.ClearList();
		eventData.Append(wxVariant(sn));
		eventData.Append(wxVariant(type));
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_UPDATE_MACHINE);
		evt.SetClientData(new wxVariant(eventData));
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::sendSigToTransferFileProgressValue(const std::string& sn, int progress, FileTransferResult result)
{
	if (pMainWindow != nullptr) {
		wxVariant eventData;
		eventData.ClearList();
		eventData.Append(wxVariant(sn));
		eventData.Append(wxVariant(progress));
		eventData.Append(wxVariant((int)result));
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TRANSFER_PROGRESS);
		evt.SetClientData(new wxVariant(eventData));
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::sendShowDeviceListDialog()
{
	ANKER_LOG_INFO << "sendShowDeviceListDialog---------------";
	if (m_palter) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG);
		wxPostEvent(m_palter, evt);
	}
}

void DatamangerUi::GeneralExceptionMsgBox(const AnkerNet::ExceptionInfo& infoin)
{
	NetworkMsg msg;
	switch (infoin.type)
	{
	case GeneralException2Gui_No_Error: {
		break;
	}												
	case GeneralException2Gui_One_Mos: {
		msg = getGeneralException2Gui_One_Mos_Msg_Text(infoin.stationName);
		break;
	}
	case GeneralException2Gui_Two_Mos: {
		msg = getGeneralException2Gui_Two_Mos_Msg_Text(infoin.stationName);
		break;
	}
	case GeneralException2Gui_Nozzle_Temp_Too_High: {
		msg = getGeneralException2Gui_Nozzle_Temp_Too_High_Msg_Text();
		break;
	}
	case GeneralException2Gui_HotBed_Temp_Too_High: {
		msg = getGeneralException2Gui_HotBed_Temp_Too_High_Msg_Text();
		break;
	}
	case GeneralException2Gui_Nozzle_Heating_Error: {
		msg = getGeneralException2Gui_Nozzle_Heating_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_HotBed_Heating_Error: {
		msg = getGeneralException2Gui_HotBed_Heating_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_Filament_Broken: {
		msg = getGeneralException2Gui_Filament_Broken_Error_Msg_Text();

		msg.filamentName = Slic3r::GcodeInfo::GetFilamentName(infoin.external);
		msg.context = Slic3r::GUI::format(msg.context, msg.filamentName);
		ANKER_LOG_INFO << "external: " << infoin.external << ", filamentName: " << msg.filamentName;
		break;
	}
	case GeneralException2Gui_Type_C_Transmission_Error: {
		msg = getGeneralException2Gui_Type_C_Transmission_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_Auto_Level_Error: {
		msg = getGeneralException2Gui_Auto_Level_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_Auto_Level_Anomalies: {
		msg = getgetGeneralException2Gui_Auto_Level_Failed_Msg_Text();
		break;
	}
	case GeneralException2Gui_System_Error: {
		msg = getGeneralException2Gui_System_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_Advance_Pause: {
		msg = getGeneralException2Gui_Advance_Pause_Error_Msg_Text();
		break;
	}
	case GeneralException2Gui_Bed_Adhesion_Failure: {
		msg = getGeneralException2Gui_Bed_Adhesion_Failure_Msg_Text();
		break;
	}
	case GeneralException2Gui_Spaghetti_Mess: {
		msg = getGeneralException2Gui_Spaghetti_Mess_Msg_Text();
		break;
	}
	case GeneralException2Gui_HomingFailed: {
		msg = getGeneralException2Gui_HomingFailed_Msg_Text();
		break;
	}
	case GeneralException2Gui_Break_Point: {
		break;
	}
	case GeneralException2Gui_Level_100_Times: {
		msg = getGeneralException2Gui_Level_100_Times_Msg_Text();
		break;
	}
	case GeneralException2Gui_LowTemperature: {
		msg = getGeneralException2Gui_LowTemperature_Msg_Text();
		break;
	}
	default:
		return;
	}

	msg.sn = infoin.sn;
	msg.type = infoin.type;
	
	ANKER_LOG_WARNING << "exception msg: " << msg.context << ", sn: " << msg.sn << ", " << msg.type;
	InsertAndShowAlertMsg(msg);

	// test
#if 0
	std::thread thread([this, msg]() {
		// almost 8's alert msg dialog
		static int aar[] = { 1, 2, 3, 1, 1, 6, 7, 2, 7, 6, 3, 4, 5, 6};
		int index = 0;
		NetworkMsg tmpMsg = msg;
		while (index < sizeof(aar) / sizeof(aar[0])) {
			std::this_thread::sleep_for(std::chrono::milliseconds(800));

			tmpMsg.type = (GeneralException2Gui)aar[index];
			InsertAndShowAlertMsg(tmpMsg);
			index++;			
		}
	});
	thread.detach();
#endif
}

void DatamangerUi::InsertAndShowAlertMsg(const NetworkMsg& msg)
{
	// search
	bool insert = true;
	std::queue<NetworkMsg> tempMsgs = m_alertMsgQueue;
	while (!tempMsgs.empty()) {
		NetworkMsg qmsg = tempMsgs.front();
		tempMsgs.pop();
		ANKER_LOG_DEBUG << "tttt come msg: " << msg.sn << ", " << msg.type << 
			" | qmsg: " << qmsg.sn << ", " << qmsg.type;
		if (qmsg == msg) {
			insert = false;
			break;
		}
	}

	if (!insert) {
		ANKER_LOG_INFO << "same alert msg come, ignore it. " << msg.sn << ", " << msg.type 
			<< ", size: " << m_alertMsgQueue.size();
		return;
	}

	// insert
	ANKER_LOG_INFO << "push alert msg: " << msg.sn << ", " << msg.type << ", size: " << m_alertMsgQueue.size();
	m_alertMsgQueue.push(msg);

	if (m_alertMsgQueue.size() == 1) {
		SendAlertMsgSig();
	}
}

void DatamangerUi::SendAlertMsgSig()
{
	if (m_alertMsgQueue.empty()) {
		return;
	}
	auto msg = m_alertMsgQueue.front();
	
	auto isPrintShow = GetIsPrintFinishFailedDialogShow();
	ANKER_LOG_INFO << "isPrintShow: " << isPrintShow;
	if (pMainWindow != nullptr && !isPrintShow) {
		wxVariant data;
		data.ClearList();
		data.Append(msg.haveCancel);
		data.Append(msg.level);
		data.Append(msg.clear);
		data.Append(msg.type);
		data.Append(wxVariant(msg.sn));
		data.Append(wxVariant(msg.title));
		data.Append(wxVariant(msg.context));								
		data.Append(msg.btn1Text);
		data.Append(msg.btn2Text);
		data.Append(msg.imagePath);
		data.Append(msg.filamentName);

		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SHOW_MSG_DIALOG);
		evt.SetClientData(new wxVariant(data));
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::ShowNextAlertMsg()
{
	if (m_alertMsgQueue.empty()) {
		return;
	}

	// pop up 
	auto msg = m_alertMsgQueue.front();
	ANKER_LOG_INFO << "pop alert msg: " << msg.sn << ", " << msg.type << ", size: " << m_alertMsgQueue.size();
	m_alertMsgQueue.pop();

	// show
	SendAlertMsgSig();
}

void DatamangerUi::SendSigAccountLogout()
{
	ANKER_LOG_INFO << "SendSigAccountLogout.";
	if (pMainWindow != nullptr) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ACCOUNT_LOGOUT);
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::Callback_RecoverCurl()
{
	wxHTTP http;
	http.SetHeader(wxT("Content-type"), wxT("text/html; charset=utf-8"));
	http.Connect(wxT("www.baidu.com"));

	wxInputStream* httpStream = http.GetInputStream(wxT("/"));
	wxString res;
	auto err = http.GetError();
	if (err == wxPROTO_NOERR)
	{
		wxStringOutputStream out_stream(&res);
		httpStream->Read(out_stream);
	}
	if (res.size() > 100)
		res = res.substr(0, 100);
	ANKER_LOG_ERROR << "wxHttp www.baidu.com Get , wxProtocolError = " << err << ",content = " << res;
	wxDELETE(httpStream);
	http.Close();
}

void DatamangerUi::Callback_OtaUpdate(OtaInfo info)
{
	ANKER_LOG_INFO << "sendSigToOTAUpdate.";

	auto pMainWindow = DatamangerUi::GetInstance().pMainWindow;
	if (pMainWindow != nullptr) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_OTA_UPDATE);
		auto pInfo = new OtaInfo(info);
		evt.SetClientData(pInfo);
		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::Callback_GetConmentFlags(std::vector<std::string> dataList)
{
	auto pMainWindow = DatamangerUi::GetInstance().pMainWindow;
	if (pMainWindow != nullptr) 
	{
		bool flags = false;
		wxVariant eventData;
		eventData.ClearList();

		if (dataList.size() > 0)
			flags = true;

		eventData.Append(wxVariant(flags));


		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_GET_CONMENT_FLAGS);

		for (auto data: dataList) {
			eventData.Append(wxVariant(wxString::FromUTF8(data)));
		}

		evt.SetClientData(new wxVariant(eventData));

		wxPostEvent(pMainWindow, evt);
	}
}

void DatamangerUi::Callback_FilamentUpdate()
{
	Slic3r::GUI::wxGetApp().filamentMaterialManager()->Load();
	Slic3r::GUI::FilamentMaterialConvertor::ResetCache();
}

AnkerNetBase* AnkerNetInst()
{
	return DatamangerUi::GetInstance().getAnkerNetBase();
}

DeviceObjectBasePtr CurDevObject(const std::string& sn)
{
	auto ankerNetInst = AnkerNetInst();
	if (!ankerNetInst) {
		return nullptr;
	}
	DeviceObjectBasePtr currentDev = ankerNetInst->getDeviceObjectFromSn(sn);
	return currentDev;
}

void CloseVideoStream(int reason)
{
	auto ankerNet = AnkerNetInst();
	if (ankerNet) {
		ankerNet->closeVideoStream(reason);
	}
}

void BuryAddEvent(const std::string& eventName, const std::map<std::string, std::string>& eventMap)
{
	reportBuryEvent(eventName, eventMap);
}

#pragma once

#include <wx/event.h>
#include "slic3r/GUI/Network/MsgText.hpp"
#include "AnkerNetDefines.h"
#include <slic3r/GUI/AnkerNetModule/AnkerNetModuleManager.h>
#include "slic3r/GUI/Network/MsgText.hpp"

namespace AnkerNet
{
	class AnkerNetBase;
	DEF_PTR(DeviceObjectBase)
}
using namespace AnkerNet;

wxDECLARE_EVENT(wxCUSTOMEVT_DEVICE_LIST_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_UPDATE_MACHINE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_MSG_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_TRANSFER_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ACCOUNT_EXTRUSION, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_HTTP_CONNECT_ERROR, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_OTA_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ACCOUNT_LOGOUT, wxCommandEvent);


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

class DatamangerUi
{
public:
	static DatamangerUi& GetInstance();

	~DatamangerUi() {};
	AnkerNetBase* getAnkerNetBase();

	AnkerNetInitPara GetNetPara();

	static SysInfo GetSysInfo();

	bool LoadNetLibrary(wxWindow* pWindow = nullptr, bool silence = false);
	void SetMainWindow(wxWindow* pWindow = nullptr);

	void ResetMainObj();
	void setPlaterPtr(wxWindow* palter);
	
	void sendSigAccountExtrusion();
	void sendSigHttpError(HttpError error);
	void sendSigToSwitchPrintPage(const std::string& sn);
	void sendSigToUpdateDevice();
	void sendSigToUpdateDeviceStatus(const std::string& sn, aknmt_command_type_e type);
	void sendSigToTransferFileProgressValue(const std::string& sn, int progress, FileTransferResult result);	
	void sendShowDeviceListDialog();
	void GeneralExceptionMsgBox(GeneralException2Gui type, 
		const std::string& stationName, const std::string& sn);
	void SendSigAccountLogout();
	
	void ShowNextAlertMsg();

	//used for callback
	static void Callback_RecoverCurl();	
	static void Callback_OtaUpdate(OtaInfo info);
	static void Callback_FilamentUpdate();

	void SetIsPrintFinishFailedDialogShow(bool isShow) {
#ifdef __APPLE__
		m_isPrintFinishFailedDialogShow = isShow;
#endif
	}
	bool GetIsPrintFinishFailedDialogShow() {
#ifdef __APPLE__
		return m_isPrintFinishFailedDialogShow;
#else
		return false;
#endif
	}
	AnkerNetBase* pAnkerNet = nullptr;

protected:
	DatamangerUi();

	DatamangerUi(const DatamangerUi&) = delete;
	DatamangerUi& operator=(const DatamangerUi&) = delete;

private:
	std::atomic_bool m_isPrintFinishFailedDialogShow{ false };
	
	void InsertAndShowAlertMsg(const NetworkMsg& msg);
	void SendAlertMsgSig();

	AKNMT_LOG_LEVEL GetMqttLogLevel();
	
	void InitAllCallBacks();

	int versionToCode(const std::string& version);

	void boostLogPrint(unsigned int logLevel, const std::string strLogMsg, const std::string strFileName, 
		const std::string strFuncName, const unsigned int lineNumber);

private:	
	wxWindow* m_palter		= nullptr;
	wxWindow* pMainWindow	= nullptr;

	std::queue<NetworkMsg> m_alertMsgQueue;
	AnkerNetModuleManager m_netModuleMgr;
};

AnkerNetBase* AnkerNetInst();
AnkerNet::DeviceObjectBasePtr CurDevObject(const std::string& sn);
void CloseVideoStream(int reason);
void BuryAddEvent(const std::string& eventName, const std::map<std::string, std::string>& eventMap);
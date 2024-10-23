#ifdef WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <psapi.h>
#include <iostream>
#else
#endif

#include "AnkerWebView.hpp"
#include "jansson.h"
#include "jansson_config.h"
#include <wx/wx.h>
#include <wx/webviewfshandler.h>
#include <wx/webviewarchivehandler.h>
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r/AppConfig.hpp"
#include "AnkerNetBase.h"

#ifdef __APPLE__
#include <sys/utsname.h>
#endif

#include "libslic3r/Utils.hpp"
#include <slic3r/GUI/Common/AnkerDialog.hpp>
#include "slic3r/Utils/DataMangerUi.hpp"
#include "AnkerNetModule/BuryDefines.h"
#include "slic3r/Utils/wxFileTool.hpp"
#include "wx/process.h"

#define WEBVIEW_EXE "msedgewebview2.exe"

wxDEFINE_EVENT(wxCUSTOMEVT_WEB_LOGIN_FINISH, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_WEB_LOGOUT_FINISH, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_DEAL_PRIVACY_CHOICES, wxCommandEvent);

AnkerWebView::AnkerWebView(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxString& url,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	bool background,
	long style /*= wxDEFAULT_DIALOG_STYLE*/,
	const wxString& name /*= wxDialogNameStr*/) :
	wxDialog(parent, id, title, pos, size, style, name),
	m_webBegin(false),
	m_url(url),
	m_background(background),
	m_webView(nullptr),
	m_pMainVSizer(nullptr)
{	
	this->AlwaysShowScrollbars(false, false);
	m_showErrorEnable = true;
	m_loadTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &AnkerWebView::OnLoadTimer, this);
	ANKER_LOG_INFO << "webview create. start load timer: " << m_loadTimeMs << ", background: " << m_background;
	m_loadTimer->StartOnce(m_loadTimeMs);	
	initUi();
	BuryEvent(LoginStatus::LoginStart, "start");
}


AnkerWebView::~AnkerWebView()
{
	ANKER_LOG_INFO << "start webview destroy";
	m_loadTimer->Stop();
	delete m_loadTimer;
	m_loadTimer = nullptr;

	ANKER_LOG_INFO << "end webview destroy";
}

void AnkerWebView::onLogOut()
{
	if (m_forceClose) return;
	if (!m_webBegin)
		return;

	if (m_webView)
	{
		CallAfter([=] {
#ifdef _WIN32
			m_webView->RunScript("localStorage.clear();");
#else
			m_webView->RunScriptAsync("localStorage.clear();");
#endif
		});
	}		
}

void AnkerWebView::SendMsgToWeb(const wxString& msg)
{
	if (m_forceClose) return;
	if (!m_webBegin)
		return;	

	if (m_webView)
	{
		CallAfter([=] {
			m_webView->RunScriptAsync(msg);
		});
	}		
}

void AnkerWebView::SetWebViewSize(const wxSize &size)
{
	if (!m_webView)
		return;

	m_webView->SetMinSize(size);

	m_pMainVSizer->Remove(0);
	m_pMainVSizer->Add(m_webView, 0, wxEXPAND, 0);
	SetSizer(m_pMainVSizer);
	SetSizerAndFit(m_pMainVSizer);
}

void AnkerWebView::SetWebViewSize(const int& width, const int& height)
{
	SetWebViewSize(wxSize(width, height));
}

void AnkerWebView::setUrl(const wxString& url)
{
	if (m_forceClose) return;
	if (!m_webBegin)
		return;
	
	m_webView->LoadURL(url);
	m_url = url;
	reLoadUrl();
}

void AnkerWebView::Clear()
{
	if (m_forceClose) return;
	if (!m_webBegin)
		return;

	m_webView->ClearHistory();
}

void AnkerWebView::SetShowErrorEnable(bool enable)
{
	m_showErrorEnable = enable;
}

void AnkerWebView::reLoadUrl()
{
	if (m_forceClose) return;
	if (!m_webBegin)
		return;

	m_webView->Reload();
}

void AnkerWebView::OnNavigationRequest(wxWebViewEvent& evt)
{
	//wxLogMessage("OnNavigationRequest");
}

void AnkerWebView::OnNavigationComplete(wxWebViewEvent& evt)
{
	//wxLogMessage("OnNavigationComplete");
}

void AnkerWebView::onWebLoadFinished(wxWebViewEvent& evt)
{	
	//m_loadTimer->Stop();
	m_webBegin = true;	
	ANKER_LOG_INFO << "web load finished";
	//m_webView->RunScriptAsync("localStorage.clear();");
}

void AnkerWebView::OnTitleChanged(wxWebViewEvent& evt)
{
	//wxLogMessage("OnTitleChanged");
}

void AnkerWebView::OnNewWindow(wxWebViewEvent& evt)
{
	//wxLogMessage("OnNewWindow");
}

void AnkerWebView::OnScriptMessage(wxWebViewEvent& evt)
{
	if(m_loadTimer->IsRunning())
		m_loadTimer->Stop();

	if (m_forceClose) return;

	wxString webJsonData = evt.GetString();	
	std::string utf8Str = webJsonData.ToUTF8().data();

	auto anker_net = AnkerNetInst();
	if (!anker_net) {
		return;
	}


	WebJsProcessRet result;
	anker_net->ProcessWebScriptMessage(utf8Str, result);
	ANKER_LOG_INFO << "get login callback data, action: " << (int)result.action <<
		", status: " << (int)result.status << ", callBackName: " <<
		result.callBackName << ", content" << result.content << ", utf8Str: " << utf8Str;
	if (result.action == WEB_ACTION::EM_LOGIN || result.action == WEB_ACTION::EM_LOGINBACK) {
		if (result.status == LOGIN_STATUS::EM_NO_DATA) {
			wxCommandEvent logoutEvent(wxCUSTOMEVT_WEB_LOGOUT_FINISH, GetId());
			logoutEvent.SetEventObject(this);
			GetEventHandler()->ProcessEvent(logoutEvent);
			return;
		}
		else if (result.status == LOGIN_STATUS::EM_USER_ID_NULL) {
			ANKER_LOG_ERROR << "user id is null.";
			wxCommandEvent logoutEvent(wxCUSTOMEVT_WEB_LOGOUT_FINISH, GetId());
			logoutEvent.SetEventObject(this);
			GetEventHandler()->ProcessEvent(logoutEvent);
			//BuryEvent(LoginStatus::LoginFailed, "login Failed, " + utf8Str);
			return;
		}

		std::string action;
		if (result.action == WEB_ACTION::EM_LOGIN) {
			action = "login";
		}
		else if (result.action == WEB_ACTION::EM_LOGINBACK) {
			action = "login back";
		}
		
		ANKER_LOG_INFO << "send wxCUSTOMEVT_WEB_LOGIN_FINISH event start2";
#ifdef _WIN32
		Hide();
		//Close();

		/*wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_WEB_LOGIN_FINISH);
		wxQueueEvent(Slic3r::GUI::wxGetApp().mainframe, evt.Clone());*/
		Slic3r::GUI::wxGetApp().mainframe->loginFinishHandle();

		ANKER_LOG_INFO << "post wxCUSTOMEVT_WEB_LOGIN_FINISH event end not Close";
		//Close();
#else
		wxCommandEvent testEvent(wxCUSTOMEVT_WEB_LOGIN_FINISH, GetId());
		testEvent.SetEventObject(this);
		GetEventHandler()->ProcessEvent(testEvent);
		ANKER_LOG_INFO << "send wxCUSTOMEVT_WEB_LOGIN_FINISH event end";

		ANKER_LOG_INFO << "LOGIN SUCESS,START CALL EndModal(wxID_ANY)";
		this->EndModal(wxID_ANY);
		ANKER_LOG_INFO << "LOGIN SUCESS,END CALL EndModal(wxID_ANY)";
#endif
		BuryEvent(LoginStatus::LoginSuccess, "login success, " + action);
	}
	else if (result.action == WEB_ACTION::EM_LOGINOUT) {
		wxCommandEvent testEvent(wxCUSTOMEVT_WEB_LOGOUT_FINISH, GetId());
		testEvent.SetEventObject(this);
		GetEventHandler()->ProcessEvent(testEvent);
	}
	else if (result.action == WEB_ACTION::EM_OPEN_BROWSER) {
		if (!result.content.empty()) {
			wxLaunchDefaultBrowser(result.content.c_str());
		}
	}
	else if (result.action == WEB_ACTION::EM_GET_HEADLIST) {
		wxString jsCode = wxString::Format("window[\"%s\"]('%s')", result.callBackName, result.content);
		CallAfter([=] { m_webView->RunScriptAsync(jsCode); });
	}
	else
	{
		BuryEvent(LoginStatus::LoginLog,utf8Str);


		if(webJsonData.Contains("failed"))
		{
			m_loadTimer->Stop();
			ShowErrorDialog();
		}
	}
}

void AnkerWebView::OnError(wxWebViewEvent& evt)
{
	std::string e = "unknown error";
	switch (evt.GetInt()) {
	case wxWEBVIEW_NAV_ERR_CONNECTION:
		e = "wxWEBVIEW_NAV_ERR_CONNECTION";
		break;
	case wxWEBVIEW_NAV_ERR_CERTIFICATE:
		e = "wxWEBVIEW_NAV_ERR_CERTIFICATE";
		break;
	case wxWEBVIEW_NAV_ERR_AUTH:
		e = "wxWEBVIEW_NAV_ERR_AUTH";
		break;
	case wxWEBVIEW_NAV_ERR_SECURITY:
		e = "wxWEBVIEW_NAV_ERR_SECURITY";
		break;
	case wxWEBVIEW_NAV_ERR_NOT_FOUND:
		e = "wxWEBVIEW_NAV_ERR_NOT_FOUND";
		break;
	case wxWEBVIEW_NAV_ERR_REQUEST:
		e = "wxWEBVIEW_NAV_ERR_REQUEST";
		break;
	case wxWEBVIEW_NAV_ERR_USER_CANCELLED:
		e = "wxWEBVIEW_NAV_ERR_USER_CANCELLED";
		break;
	case wxWEBVIEW_NAV_ERR_OTHER:
		e = "wxWEBVIEW_NAV_ERR_OTHER";
		break;
	}


	std::string errorInfo = std::string("webview error , url = ") + evt.GetURL().ToStdString(wxConvUTF8) + 
		",target = " + evt.GetTarget().ToStdString(wxConvUTF8) + ",description = " + e + 
		"," + evt.GetString().ToStdString(wxConvUTF8);
	ANKER_LOG_ERROR << errorInfo;


	m_loadTimer->Stop();
	ANKER_LOG_ERROR << errorInfo;
	std::cout << errorInfo << std::endl;

	BuryEvent(LoginStatus::LoadError, errorInfo);

	if (m_forceClose) return;

	// Running a message loop synchronously in an event handler in Webview can cause reentrancy issue. 
	// Please refer to https://docs.microsoft.com/en-us/microsoft-edge/webview2/concepts/threading-model#re-entrancy 
	// for more information about threading model in WebView2 and how to enable native code debugging for this scenario.
	//ShowErrorDialog();
	CallAfter([this] {
		ShowErrorDialog();
	});
}

void AnkerWebView::SetForceClose(bool close)
{
	m_forceClose = close;
}


void AnkerWebView::initUi()
{
	if (m_forceClose) return;
	SetBackgroundColour(wxColour("#111111"));
	m_pMainVSizer = new wxBoxSizer(wxVERTICAL);
	if (m_webView != nullptr)
	{
		delete m_webView;
		m_webView = nullptr;
	}
	m_webView = std::move(CreateWebView(this, m_url));

	if (m_webView)
	{
		//m_webView->EnableContextMenu(true);
		//m_webView->EnableAccessToDevTools();
		m_webView->SetBackgroundColour(wxColour("#111111"));
		m_webView->AlwaysShowScrollbars(false, false);
		m_pMainVSizer->Remove(0);
		m_pMainVSizer->Add(m_webView, 0, wxEXPAND, 0);
		Bind(wxEVT_WEBVIEW_NAVIGATING, &AnkerWebView::OnNavigationRequest, this, m_webView->GetId());
		Bind(wxEVT_WEBVIEW_NAVIGATED, &AnkerWebView::OnNavigationComplete, this, m_webView->GetId());
		Bind(wxEVT_WEBVIEW_TITLE_CHANGED, &AnkerWebView::OnTitleChanged, this, m_webView->GetId());
		Bind(wxEVT_WEBVIEW_ERROR, &AnkerWebView::OnError, this, m_webView->GetId());
		Bind(wxEVT_WEBVIEW_NEWWINDOW, &AnkerWebView::OnNewWindow, this, m_webView->GetId());

		Bind(wxEVT_WEBVIEW_LOADED, &AnkerWebView::onWebLoadFinished, this, m_webView->GetId());
		Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &AnkerWebView::OnScriptMessage, this, m_webView->GetId());

		ANKER_LOG_INFO << "init webview finished";
		m_webView->Reload(wxWEBVIEW_RELOAD_NO_CACHE);

		auto appConfig = Slic3r::GUI::wxGetApp().app_config;
		if (appConfig && appConfig->get_bool("Debug", "open_webview_f12")) {
			m_webView->EnableAccessToDevTools();
			bool ret = m_webView->IsAccessToDevToolsEnabled();
			ANKER_LOG_INFO << "enable f12 for webview, ret: " << ret;
		}
	}

	SetSizer(m_pMainVSizer);
	SetSizerAndFit(m_pMainVSizer);
}

std::string AnkerWebView::getBrowserVersion()const
{
	std::string version = "";
	if (!m_webView)
		return version;
	
	auto backenVersioninfo = m_webView->GetBackendVersionInfo();
	version = std::to_string(backenVersioninfo.GetMajor())+"."+
		std::to_string(backenVersioninfo.GetMinor()) + "." +
		std::to_string(backenVersioninfo.GetMicro()) + "." +
		std::to_string(backenVersioninfo.GetRevision()) ;

	return version;
}


std::string AnkerWebView::getSysVersion()
{
#ifdef _WIN32
	OSVERSIONINFOEX info;
	ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&info);
	return std::to_string(info.dwMajorVersion);
#elif  __APPLE__
	struct utsname info;
	uname(&info);
	return info.release;	
#endif

}

wxWebView* AnkerWebView::CreateWebView(wxWindow* parent, wxString const& webUrl)
{
#if wxUSE_WEBVIEW_EDGE
	wxFileName edgeFixedDir(wxStandardPaths::Get().GetExecutablePath());
	edgeFixedDir.SetFullName("");
	edgeFixedDir.AppendDir("edge_fixed");
	if (edgeFixedDir.DirExists())
	{
		//wxWebViewEdge::MSWSetBrowserExecutableDir(edgeFixedDir.GetFullPath());
		wxLogMessage("Using fixed edge version");
	}
#endif

	auto url = webUrl;
	
#ifdef __WIN32__
	url.Replace("\\", "/");
#endif
	auto webView = wxWebView::New();
	
	if (webView)
	{		
		webView->Create(parent, wxID_ANY, url, wxDefaultPosition, wxDefaultSize);
#ifdef __WIN32__				
		webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("anker")));
#else			
		webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("wxfs")));
#endif
		webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));

		if (!webView->AddScriptMessageHandler("anker_msg"))
		{
			ANKER_LOG_ERROR << "add js handler error.";
		}

		if (wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
		{
			ANKER_LOG_INFO << "edge available";
		}

		if (wxWebView::IsBackendAvailable(wxWebViewBackendIE))
		{
			ANKER_LOG_INFO << "ie available";
		}

		if (wxWebView::IsBackendAvailable(wxWebViewBackendWebKit))
		{
			ANKER_LOG_INFO << "webkit available";
		}

		webView->EnableContextMenu(false);		
	}
	else
	{
		ANKER_LOG_ERROR << "create log widget fail.";
		return nullptr;
	}
	return webView;
}

void AnkerWebView::OnLoadTimer(wxTimerEvent& event)
{
	wxString info = m_url + " load timeout, backgroud: " + std::to_string(m_background);
	ANKER_LOG_INFO << info;

	BuryEvent(LoginStatus::LoadTimeout, info.ToStdString());

	if (m_forceClose) return;
	ShowErrorDialog();
}

void AnkerWebView::BuryEvent(AnkerWebView::LoginStatus status, const std::string& statusInfo)
{
	ANKER_LOG_INFO << "webview bury: " << (int)status << " " << m_url.ToStdString() << ", " << statusInfo;
	std::map<std::string, std::string> map;
	map.insert(std::make_pair(c_login_type, m_background ? "0" : "1"));
	map.insert(std::make_pair(c_retry_count, std::to_string(m_retry_time)));
	map.insert(std::make_pair(c_login_status, std::to_string((int)status)));
	map.insert(std::make_pair(c_load_url, m_url.ToUTF8()));
	map.insert(std::make_pair(c_status_info, statusInfo));
#ifdef _WIN32
	map.insert(std::make_pair(c_login_webview2_version, Slic3r::GUI::wxGetApp().getWebview2Version()));
#endif
	std::string version = getBrowserVersion();
	if(!version.empty())
		map.insert(std::make_pair(c_browser_version, version));
	BuryAddEvent(e_webview_event, map);
}

#ifdef WIN32

wxString GetProcessExePath(DWORD processID) {
	TCHAR szExePath[MAX_PATH] = { 0 };
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess != NULL) {
		HMODULE hMod;
		DWORD cbNeeded;
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
			if (GetModuleFileNameEx(hProcess, hMod, szExePath, sizeof(szExePath) / sizeof(TCHAR))) {
				return wxString(szExePath);
			}
		}
		CloseHandle(hProcess);
	}
	return wxString("");
}

std::vector<DWORD> GetChildIdRecursively(DWORD parentId)
{
	std::vector<DWORD> childIds;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				if (pe32.th32ParentProcessID == parentId) {
					auto ids =  GetChildIdRecursively(pe32.th32ProcessID);
					for (auto id : ids)
					{
						childIds.push_back(id);
					}
					childIds.push_back(pe32.th32ProcessID);
				}
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return childIds;
}


void RemoveChildIds(std::vector<DWORD> childPids,wxString match = wxString())
{
	ANKER_LOG_INFO << "start remove pid";
	for (auto pid : childPids)
	{
		auto path = GetProcessExePath(pid);

		if(match.empty() || path.Contains(match))
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
			if (hProcess == NULL) {
				ANKER_LOG_ERROR << "OpenProcess errpr , pid = " << pid << ",last error , =" << GetLastError();
				continue;
			}
			BOOL result = TerminateProcess(hProcess, 1);
			if (result == false)
			{
				ANKER_LOG_ERROR << "TerminateProcess error , pid = " << pid << ",last error , =" << GetLastError();
				continue;
			}
			CloseHandle(hProcess);
		}
	}
}

std::vector<DWORD> GetProcessIdsMatch(wxString match)
{
	std::vector<DWORD> childIds;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				auto path = GetProcessExePath(pe32.th32ProcessID);
				if (path.Contains(match))
					childIds.push_back(pe32.th32ProcessID);
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return childIds;
}

#endif

void AnkerWebView::RemoveBrowserCache()
{

#ifdef WIN32
	//step 1. close web browser process
	if(m_retry_time <= 2)
	{
		auto processID = wxGetProcessId();
		//step 1.1. find child pids
		auto childPids = GetChildIdRecursively(processID);
		//step 1.2. close child pids
		RemoveChildIds(childPids, WEBVIEW_EXE);
	}
	else
	{
		//step 1.1. find all matched pids
		std::vector<DWORD> matchedPids = GetProcessIdsMatch(WEBVIEW_EXE);
		//step 1.2. close all matched pids
		RemoveChildIds(matchedPids);
	}
	
#else

#endif

	wxString cacheDir;
#ifdef _WIN32
	cacheDir = wxStandardPaths::Get().GetUserLocalDataDir();
	cacheDir += "/EBWebView";
#else
	wxString homeDir = wxStandardPaths::Get().GetUserConfigDir();
	cacheDir = homeDir + "/../Caches/com.anker.pcankermake/Webkit/";
#endif
	Slic3r::Utils::wxFileTool::RemoveDirRecursivelyAMAP(cacheDir);
}

void AnkerWebView::ShowErrorDialog()
{
	if (m_forceClose) return;

	if (m_showErrorEnable == false)
		return;
	m_showErrorEnable = false;

	ANKER_LOG_INFO << "ShowErrorDialog";
	if (m_background) {
		ANKER_LOG_INFO << "no show or background: " << m_background;
		return;
	}

	wxSize dialogSize = AnkerSize(400, 185);
	wxPoint parentCenterPoint(this->GetPosition().x + this->GetSize().GetWidth() / 2,
		this->GetPosition().y + this->GetSize().GetHeight() / 2);
	wxPoint dialogPos = wxPoint(parentCenterPoint.x - dialogSize.x / 2, parentCenterPoint.y - dialogSize.y / 2);
	AnkerDialog dialog(this, wxID_ANY,
		_L("common_net_connect_error"),
		_L("common_webview_req_failed"),
		dialogPos, dialogSize);
	auto result = dialog.ShowAnkerModalOkCancel(_L("common_button_retry"));

	if (result == wxID_OK) {

		auto size = m_webView->GetMinSize();
		if (m_webView)
		{
			m_webView->Close(true);
			delete m_webView;
			m_webView = nullptr;
		}

		ANKER_LOG_INFO << "reload webview clicked, start load timer: " << m_loadTimeMs;
		m_retry_time++;
		RemoveBrowserCache();
		initUi();
		SetWebViewSize(size);

		m_loadTimer->StartOnce(m_loadTimeMs);
		BuryEvent(LoginStatus::LoginStart, "retry start");
		m_showErrorEnable = true;
	}
	
}
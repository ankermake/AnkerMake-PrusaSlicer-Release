#ifndef _ANKER_WEB_VIEW_H_
#define _ANKER_WEB_VIEW_H_

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/webview.h>


wxDECLARE_EVENT(wxCUSTOMEVT_WEB_LOGIN_FINISH, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_WEB_LOGOUT_FINISH, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_DEAL_PRIVACY_CHOICES, wxCommandEvent);

class AnkerWebView :public wxDialog
{
	enum class LoginStatus {
		LoginSuccess = 0,
		LoginFailed = 1,
		LoadError = 2,
		LoadTimeout = 3,
		LoginStart = 4,
		LoginLog = 10,
	};

public:
	AnkerWebView(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& url,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		bool background = false,
		long style = wxDEFAULT_DIALOG_STYLE,
		const wxString& name = wxDialogNameStr);
	~AnkerWebView();

	void onLogOut();
	void SendMsgToWeb(const wxString& msg);
	void SetWebViewSize(const wxSize& size);
	void SetWebViewSize(const int& width,const int& height);
	void setUrl(const wxString& url);
	void reLoadUrl();
	void Clear();
	void SetShowErrorEnable(bool enable);

	void OnNavigationRequest(wxWebViewEvent& evt);
	void OnNavigationComplete(wxWebViewEvent& evt);
	void onWebLoadFinished(wxWebViewEvent& evt);
	void OnTitleChanged(wxWebViewEvent& evt);
	void OnNewWindow(wxWebViewEvent& evt);
	void OnScriptMessage(wxWebViewEvent& evt);
	void OnError(wxWebViewEvent& evt);
	void SetForceClose(bool close);
protected:
	void initUi();	
	std::string getSysVersion();
	wxWebView* CreateWebView(wxWindow* parent, wxString const& url);

private:
	void OnLoadTimer(wxTimerEvent& event);
	void BuryEvent(LoginStatus status, const std::string& statusInfo);

	void RemoveBrowserCache();
	void ShowErrorDialog();
private:
	bool		m_background{false};
	bool		m_webBegin;
	wxString	m_url;
	wxWebView*  m_webView{ nullptr };
	wxBoxSizer* m_pMainVSizer{ nullptr };
	std::string m_getHeaderJsonCallBack;
	wxTimer*	m_loadTimer{nullptr};
	const int	m_loadTimeMs = 20000;
	bool m_isErrorDialogShow = false;
	int m_retry_time = 0;
	bool m_showErrorEnable = false;
	bool m_forceClose = false;
};
#endif 
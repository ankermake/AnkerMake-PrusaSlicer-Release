#ifndef _Web_DownloadController_H  
#define _Web_DownloadController_H

#include <wx/wx.h>
#include <boost/filesystem/path.hpp>
#include "WebDownload.hpp"
#include "DownLoadMessage.hpp"

namespace Slic3r {
namespace GUI {

wxDECLARE_EVENT(EVT_DOWNLOAD_FILE_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOWNLOAD_ERROR_ID, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOWNLOAD_ERRORINFO_ID, wxCommandEvent);

class WebDownloadController : public wxEvtHandler
{
public:
	WebDownloadController();
	~WebDownloadController() = default;

	void Init(const std::string& download_info);

	void StartDownLoad();

	void StopDownLoad();

	void CancelDownLoad();

	void OnDownLoadComplete();
private:
	void InitCallBack();
	void ShowMessage(const std::string& title, const wxString& msg, bool bError = true);
	void OnMessage(MessageType type, const wxString& msg, float percent);
    void ProcessMessage(wxCommandEvent& event);

private:
	DownLoadMsgDialog* m_dlg { nullptr };
	std::unique_ptr<WebDownload> m_webDownPtr{ std::make_unique<WebDownload>() };
	std::unique_ptr<DownLoadMessageManager> m_manager { std::make_unique<DownLoadMessageManager>() };
};

}
}
#endif // ! 






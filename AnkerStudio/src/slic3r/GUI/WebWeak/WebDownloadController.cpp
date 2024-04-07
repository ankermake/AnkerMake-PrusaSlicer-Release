#include "WebDownloadController.hpp"
#include "libslic3r/Utils.hpp"
namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_DOWNLOAD_FILE_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOWNLOAD_ERROR_ID, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOWNLOAD_ERRORINFO_ID, wxCommandEvent);

WebDownloadController::WebDownloadController()
	: wxEvtHandler()
{
	Bind(EVT_DOWNLOAD_ERROR_ID, [this](wxCommandEvent& evt) { StopDownLoad(); }); 
    Bind(EVT_DOWNLOAD_ERRORINFO_ID, &WebDownloadController::ProcessMessage, this);
	InitCallBack();
}

void WebDownloadController::Init(const std::string& download_info)
{
	if (m_webDownPtr) {
		m_webDownPtr->Init(download_info);
	}
}

void WebDownloadController::InitCallBack()
{
	if (m_webDownPtr) {
		m_webDownPtr->SetDownCallBack([&](MessageType type, const wxString& msg, float percent) {
			OnMessage(type, msg, percent);
		});
	}

	if (m_manager) {
		m_manager->SetCancelCb([&](CancelType type) {
			m_webDownPtr->Cancel();
		});
	}
}

void WebDownloadController::StartDownLoad()
{
	if (m_manager) {
		m_manager->Init();
	}

	if (m_webDownPtr) {
		m_webDownPtr->Start();
	}
}

void WebDownloadController::StopDownLoad()
{
	if (m_webDownPtr) {
		m_webDownPtr->Stop();
	}
}

void WebDownloadController::CancelDownLoad()
{
	if (m_webDownPtr) {
		m_webDownPtr->Cancel();
	}
}

void WebDownloadController::OnDownLoadComplete()
{
	if (m_manager) {
		m_manager->onDownLoadComplete();
	}
}

void WebDownloadController::OnMessage(MessageType type, const wxString& msg, float percent)
{
    wxCommandEvent* evt = new  wxCommandEvent(EVT_DOWNLOAD_ERRORINFO_ID);
    evt->SetId(static_cast<int>(type));
    evt->SetString(msg);
    wxQueueEvent(this, evt);
}

void WebDownloadController::ProcessMessage(wxCommandEvent& event)
{
    auto type = event.GetId();
    auto msg = event.GetString();
    
    switch (static_cast<MessageType>(type))
    {
    case Slic3r::GUI::MessageType::Message_Process:
        m_manager->SetPercent(msg);
        break;
    case Slic3r::GUI::MessageType::Message_HttpError:
        break;
    case Slic3r::GUI::MessageType::Message_Complete:
        ShowMessage("", msg, false);
        break;
    case Slic3r::GUI::MessageType::Message_InvalidParam:
        ShowMessage(_L("common_web_invalid_parameters_title").ToStdString(wxConvUTF8), msg);
        break;
    case Slic3r::GUI::MessageType::Message_ConnectError:
        ShowMessage(_L("common_web_failed_to_connect_network_title").ToStdString(wxConvUTF8), msg);
        break;
    case Slic3r::GUI::MessageType::Message_TimeOut:
        ShowMessage(_L("common_web_down_timeout_title").ToStdString(wxConvUTF8), msg);
        break;
    default:
        break;
    }
}

void WebDownloadController::ShowMessage(const std::string& title, const wxString& msg, bool bError)
{
	if (!m_manager) {
		return;
	}

	wxCommandEvent* event = nullptr;
	if (bError) {
		m_manager->ShowError(title, msg);
		event = new wxCommandEvent(EVT_DOWNLOAD_ERROR_ID);
		wxQueueEvent(this, event);
	}
	else {
		m_manager->onHideProcess();
		event = new wxCommandEvent(EVT_DOWNLOAD_FILE_COMPLETE);
		event->SetString(msg);
		wxQueueEvent(this, event);
	}
}

}
}

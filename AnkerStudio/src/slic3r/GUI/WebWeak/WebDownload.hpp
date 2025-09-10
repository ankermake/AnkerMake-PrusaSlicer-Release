#ifndef Web_Download_hpp_
#define Web_Download_hpp_

#include <boost/filesystem/path.hpp>
#include "slic3r/Utils/Http.hpp"

#include <wx/wx.h>
#include <thread>
#include <functional>
namespace Slic3r {
namespace GUI {

enum class DownloadStates
{
    DownloadUnknown = 0,
    DownloadOngoing,
    DownloadCancel,
    DownloadStopped,
};

enum class MessageType
{
    Message_Process = 0,
    Message_HttpError,
    Message_Complete,
    Message_InvalidParam,
    Message_ConnectError,
    Message_TimeOut
};

class WebDownload 
{
    using DownLoadCallBack = std::function<void(MessageType type, const wxString& msg, float percent) > ;
public:
	WebDownload() = default;
	~WebDownload();

    void Init(const std::string& download_info);

    void Start();

    void Cancel();

    void Stop();

    void SetDownCallBack(DownLoadCallBack cb) {
        m_downLoadCb = std::move(cb);
    }

private:
    bool GetGetUseDownPath();

    bool OnDownReady();

    void DoDownLoading();

    void OnHttpProgress();

    void ParseHttpError(unsigned int http_status);

    void SetProcessCallBack(MessageType type, const wxString &msg, float percent);

private:
    wxString m_down_info;
    wxString m_down_url;
    std::string m_strDownInfo;

    boost::filesystem::path m_dest_folder;
    boost::filesystem::path m_down_filePath;
    boost::filesystem::path m_target_path;

    constexpr static int32_t m_max_retries { 3 };
    DownLoadCallBack m_downLoadCb{ nullptr };

    // download thread 
    std::thread	       m_thread_;
    std::atomic_bool   m_cancel { false };
    std::atomic<DownloadStates> m_downState{ DownloadStates::DownloadUnknown };
};

}
}





#endif
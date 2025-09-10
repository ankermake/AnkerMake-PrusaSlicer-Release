#include "WebDownload.hpp"

#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/format.hpp"

#include <curl/curl.h>
#include <thread>
#include <slic3r/Config/AnkerCommonConfig.hpp>

namespace Slic3r {
namespace GUI {

std::string url_decode(const std::string& unescaped)
{
    std::string ret_val;
    CURL* curl = curl_easy_init();
    if (curl) {
        int decodelen;
        char* decoded = curl_easy_unescape(curl, unescaped.c_str(), unescaped.size(), &decodelen);
        if (decoded) {
            ret_val = std::string(decoded);
            curl_free(decoded);
        }
        curl_easy_cleanup(curl);
    }
    return ret_val;
}

std::vector<std::string> split_str_(std::string src, std::string separator)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    src += separator;
    int size = src.size();

    for (int i = 0; i < size; i++)
    {
        pos = src.find(separator, i);
        if (pos < size)
        {
            std::string s = src.substr(i, pos - i);
            result.push_back(s);
            i = pos + separator.size() - 1;
        }
    }
    return result;
}

WebDownload::~WebDownload()
{
    Stop();
}

bool WebDownload::GetGetUseDownPath()
{
    std::string user_down_path = wxStandardPaths::Get().GetUserDir(wxStandardPaths::Dir_Downloads).ToUTF8().data();
    if (boost::filesystem::exists(user_down_path)) {
        ANKER_LOG_INFO << "Downloads folder path: " << user_down_path;
        m_dest_folder = user_down_path;
    }
    else {
        ANKER_LOG_INFO << "Downloads folder does not exist at the expected location.";
        return false;
    }

    return true;
}

void WebDownload::Init(const std::string& download_info)
{
    m_strDownInfo = download_info;
}

void WebDownload::Start()
{
    if (m_thread_.joinable()) {
        m_cancel = true;
        m_thread_.join();
    }

    m_cancel = false;
    m_thread_ = std::thread([&]() {
        DoDownLoading();
    });
}

void WebDownload::Cancel()
{
    m_cancel = true;
    m_downState = DownloadStates::DownloadCancel;
}

void WebDownload::Stop()
{
    m_downState = DownloadStates::DownloadStopped;
    if (m_thread_.joinable()) {
        m_cancel = true;
        m_thread_.join();
    }

    m_cancel = true;
}

#if 0
void WebDownload::OnHttpProgress()
{
    m_downState = DownloadStates::DownloadOngoing;
    wxString msg;
    int percent = 1;
    auto http = Http::get(m_down_url.ToStdString());
        http.on_progress([&](Http::Progress progress, bool& cancel) {
                if (m_cancel) {
                    cancel = true;
                    ANKER_LOG_INFO << " cancel = true;" << percent;
                    return;
                }
                if (progress.dltotal != 0) {
                    percent = progress.dlnow * 100 / progress.dltotal;
                    msg = wxString::Format(_L(/*"File downloaded %d%%..."*/ "common_web_file_download_content"), percent);
                    SetProcessCallBack(MessageType::Message_Process, msg, percent);
                }
        })
        .on_error([&](std::string body, std::string error, unsigned http_status) {
            ANKER_LOG_INFO << "http_error: " << error << " status: " << http_status;
            ParseHttpError(http_status);
        })
        .on_complete([&](std::string body, unsigned /* http_status */) {
            boost::filesystem::fstream file(m_down_filePath, std::ios::out | std::ios::binary | std::ios::trunc);
            file.write(body.c_str(), body.size());
            file.close();
            boost::filesystem::rename(m_down_filePath, m_target_path);
            SetProcessCallBack(MessageType::Message_Complete, m_target_path.wstring(), percent);
         }).perform_sync();
}

#else
void WebDownload::OnHttpProgress()
{
    m_downState = DownloadStates::DownloadOngoing;
    wxString msg;
    int percent = 1;
    auto http = Http::get(m_down_url.ToStdString());
    while (!m_cancel) {
        http.on_progress([&](Http::Progress progress, bool& cancel) {
                if (m_cancel) {
                    cancel = true;
                    ANKER_LOG_INFO << " cancel = true;" << percent;
                    return;
                }
                if (progress.dltotal != 0) {
                    percent = progress.dlnow * 100 / progress.dltotal;
                    msg = wxString::Format(_L("common_web_file_download_content"), percent);
                    SetProcessCallBack(MessageType::Message_Process, msg, percent);
                }
            })
            .on_error([&](std::string body, std::string error, unsigned http_status) {
                    ANKER_LOG_INFO << "http_error: " << error << " status: " << http_status;
                    m_cancel = true;
                    ParseHttpError(http_status);
                })
                .on_complete([&](std::string body, unsigned /* http_status */) {
                    boost::filesystem::fstream file(m_down_filePath, std::ios::out | std::ios::binary | std::ios::trunc);
                    file.write(body.c_str(), body.size());
                    file.close();
                    boost::filesystem::rename(m_down_filePath, m_target_path);
                    m_cancel = true;
                    SetProcessCallBack(MessageType::Message_Complete, m_target_path.wstring(), percent);
                    }).perform_sync();
    }
}

#endif
void WebDownload::DoDownLoading()
{
    do {
        if (!OnDownReady())
            break;

        OnHttpProgress();
    } while (false);
}

void WebDownload::ParseHttpError(unsigned int http_status)
{
    wxString msg;
    if (http_status == CURLE_COULDNT_RESOLVE_HOST) {
        msg = _L("common_web_connect_content");
        SetProcessCallBack(MessageType::Message_ConnectError, msg, 0.0);
    }
    else if(http_status == CURLE_OPERATION_TIMEDOUT) {
        msg = _L("common_web_timeout_content");
        SetProcessCallBack(MessageType::Message_TimeOut, msg, 0.0);
    }
    else {
        msg = _L("common_web_connect_content");
        SetProcessCallBack(MessageType::Message_ConnectError, msg, 0.0);
    }
}

void WebDownload::SetProcessCallBack(MessageType type, const wxString& msg, float percent)
{
    if (m_downLoadCb) { 
        m_downLoadCb(type, msg, percent);
    }
}

bool WebDownload::OnDownReady()
{
    wxString filename;
    wxString strDownInfo;
    wxString msg = _L("common_web_invalid_parameters_content");
    if (m_strDownInfo.empty()) {
        SetProcessCallBack(MessageType::Message_InvalidParam, msg, 0.0);
        return false;
    }

    if (!GetGetUseDownPath()) {
        wxString cur_msg = _L("common_web_download_error_content");
        SetProcessCallBack(MessageType::Message_InvalidParam, msg, 0.0);
        return false;
    }

    if (boost::starts_with(m_strDownInfo, Slic3r::WebConfig::UrlProtocol)) {
        std::string download_params_url = url_decode(m_strDownInfo);
        auto input_str_arr = split_str_(download_params_url, "file=");
        std::string download_url;
        for (auto input_str : input_str_arr) {
            if (boost::starts_with(input_str, "http://") || boost::starts_with(input_str, "https://")) {
                download_url = input_str;
            }
        }

        if (!download_url.empty()) {
            strDownInfo = from_u8(download_url);
        }
    }
    else {
        SetProcessCallBack(MessageType::Message_InvalidParam, msg, 0.0);
        return false;
    }

    if (strDownInfo.empty()) {
        SetProcessCallBack(MessageType::Message_InvalidParam, msg, 0.0);
        return false;
    }

    wxString download_origin_url = strDownInfo;
    wxString separator = "&name=";

    size_t namePos = strDownInfo.Find(separator);
    if (namePos != wxString::npos) {
        m_down_url = strDownInfo.Mid(0, namePos);
        filename = strDownInfo.Mid(namePos + separator.Length());

    }
    else {
        boost::filesystem::path download_path = boost::filesystem::path(download_origin_url.wx_str());
        m_down_url = download_origin_url;
        filename = download_path.filename().string();
    }

    m_target_path = m_dest_folder;
    //gets the number of files with the same name
    std::vector<wxString>   vecFiles;
    bool  is_already_exist = false;

    try
    {
        vecFiles.clear();
        wxString extension = boost::filesystem::path(filename.wx_str()).extension().c_str();
        auto name = filename.substr(0, filename.length() - extension.length() - 1);

        for (const auto& iter : boost::filesystem::directory_iterator(m_target_path)) {
            if (boost::filesystem::is_directory(iter.path()))
                continue;

            wxString sFile = iter.path().filename().string().c_str();
            if (strstr(sFile.c_str(), name.c_str()) != NULL) {
                vecFiles.push_back(sFile);
            }

            if (sFile == filename) is_already_exist = true;
        }
    }
    catch (const std::exception& error)
    {
        //wxString sError = error.what();
    }

    //update filename
    if (is_already_exist && vecFiles.size() >= 1) {
        wxString extension = boost::filesystem::path(filename.wx_str()).extension().c_str();
        wxString name = filename.substr(0, filename.length() - extension.length());
        filename = wxString::Format("%s(%d)%s", name, vecFiles.size() + 1, extension).ToStdString();
    }

    if (filename.empty()) {
        filename = "untitled.3mf";
    }

    //target_path /= (boost::format("%1%_%2%.3mf") % filename % unique).str();
    m_target_path /= boost::filesystem::path(filename.wc_str());

    m_down_filePath = m_target_path;
    m_down_filePath += format(".%1%", ".download");
    return true;
}

}
}

#include "AnkerNetNative.hpp"
#include "DeviceObjectNative.hpp"
#include "AnkerMqtt.hpp"
#include "AnkerPppp.hpp"
#include "AnkerH264Decoder.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <map>
#include <sstream>
#include <thread>
#include <vector>

#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <curl/curl.h>

namespace pt = boost::property_tree;

namespace AnkerNet {

namespace {
size_t curl_write_to_file(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* ofs = static_cast<std::ofstream*>(userdata);
    ofs->write(static_cast<const char*>(ptr), static_cast<std::streamsize>(size * nmemb));
    return size * nmemb;
}

size_t curl_write_to_string(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* s = static_cast<std::string*>(userdata);
    s->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

// The login page percent-encodes string fields (e.g. "bobbyb%40mac.com",
// avatar URLs with %2F). Decode in place; malformed escapes are passed through.
std::string url_decode(const std::string& in)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            auto hex = in.substr(i + 1, 2);
            char* end = nullptr;
            long v = std::strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() + 2) {
                out.push_back(static_cast<char>(v));
                i += 2;
                continue;
            }
        }
        if (in[i] == '+')
            out.push_back(' ');
        else
            out.push_back(in[i]);
    }
    return out;
}

// Random RFC-4122-ish v4 UUID string (36 chars), used as the outgoing frame guid.
std::string makeGuid()
{
    static const char* hexd = "0123456789abcdef";
    std::srand(static_cast<unsigned>(::time(nullptr) ^ ::getpid()));
    const char* fmt = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    std::string s(36, '0');
    for (int i = 0; i < 36; ++i) {
        char c = fmt[i];
        if (c == '-') s[i] = '-';
        else if (c == '4') s[i] = '4';
        else if (c == 'y') s[i] = hexd[(std::rand() & 0x3) | 0x8];
        else s[i] = hexd[std::rand() & 0xf];
    }
    return s;
}
} // namespace

bool AnkerNetNativeImpl::Init(AnkerNetInitPara para)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dataDir = para.dataDir;
    m_init = true;
    if (m_clientGuid.empty())
        m_clientGuid = makeGuid();
    loadLoginCache();
    return true;
}

bool AnkerNetNativeImpl::IsInit()
{
    return m_init;
}

void AnkerNetNativeImpl::UnInit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_init = false;
}

void AnkerNetNativeImpl::ResetLanguage(std::string /*country*/, std::string /*language*/) {}
void AnkerNetNativeImpl::setLogOutputCallBack(LogOutputCallBackFunc /*callBackFunc*/) {}

void AnkerNetNativeImpl::AsyAddEvent(const std::string& /*eventName*/, const std::map<std::string, std::string>& /*eventMap*/) {}

int AnkerNetNativeImpl::AsyDownLoad(const std::string& url,
    const std::string& localFilePath,
    void* userData,
    CallBackFunction /*callbackfunc*/,
    ProgressCallback /*progressCallbackFunc*/,
    bool /*isBlock*/,
    unsigned int nTimeOut)
{
    // Minimal, self-contained blocking download (used today only for the post-login
    // avatar image). Deliberately doesn't depend on the host app's Http wrapper
    // (slic3r/Utils/Http.cpp) since this is a standalone plugin library.
    (void)userData;
    CURL* curl = curl_easy_init();
    if (!curl)
        return -1;

    std::ofstream ofs(localFilePath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(nTimeOut));

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ofs.close();

    return res == CURLE_OK ? 0 : -1;
}

int AnkerNetNativeImpl::getCurrentEnvironmentType()
{
    return 0;
}

void AnkerNetNativeImpl::AsyPostFeedBack(FeedBackInfo /*info*/) {}
void AnkerNetNativeImpl::PostGetPrintStopReasons(PrintStopReasons& /*reasons*/, const std::string& /*station_sn*/) {}

bool AnkerNetNativeImpl::PostGetSliceTips(SliceTips& /*sliceTips*/)
{
    return true;
}

void AnkerNetNativeImpl::SetCallback_RecoverCurl(NormalCallBack /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_OtaInfoRecv(CallBack_OtaInfoRecv /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_FilamentRecv(NormalCallBack /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_GetMsgCenterConfig(CallBack_MsgCenterCfg /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_GetMsgCenterRecords(CallBack_MsgCenterRecords /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_GetMsgCenterErrorCodeInfo(CallBack_MsgCenterErrCodeInfo /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_GetMsgCenterStatus(CallBack_MsgCenterStatus /*callback*/) {}
void AnkerNetNativeImpl::SetCallback_CommentFlagsRecv(CommentFlagsCallBack /*callback*/) {}
void AnkerNetNativeImpl::SetsendSigHttpError(sendSigHttpError_T /*function*/) {}

void AnkerNetNativeImpl::GetMsgCenterRecords(const int& /*newType*/, const int& /*num*/, const int& /*page*/, bool /*isSyn*/) {}
void AnkerNetNativeImpl::GetMsgCenterStatus() {}

std::string AnkerNetNativeImpl::GetNickName()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_login.nick_name;
}

std::string AnkerNetNativeImpl::GetAvatar()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_login.avatar;
}

std::string AnkerNetNativeImpl::GetUserId()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_login.user_id;
}

std::string AnkerNetNativeImpl::GetUserEmail()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_login.email;
}

void AnkerNetNativeImpl::SetOtaCheckType(OtaCheckType type) { m_otaCheckType = type; }
OtaCheckType AnkerNetNativeImpl::GetOtaCheckType() { return m_otaCheckType; }
void AnkerNetNativeImpl::queryOTAInformation() {}

void AnkerNetNativeImpl::SetGcodePath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gcodePath = path;
}

std::string AnkerNetNativeImpl::GetGcodePath()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_gcodePath;
}

bool AnkerNetNativeImpl::IsLogined()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_login.logined;
}

void AnkerNetNativeImpl::removeMsgByIds(const std::vector<int>& /*msgList*/, bool /*isSyn*/) {}

void AnkerNetNativeImpl::logout()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_login = LoginState{};
        boost::filesystem::remove(loginCachePath());
    }
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        m_devices.clear();
    }
}

void AnkerNetNativeImpl::logoutToServer()
{
    logout();
}

void AnkerNetNativeImpl::AsyRefreshDeviceList()
{
    std::string authToken;
    std::string baseUrl;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_login.logined)
            return;
        authToken = m_login.auth_token;
        baseUrl = apiBaseUrl();
    }

    // Coalesce overlapping refreshes (the UI fires one on every device-tab click).
    bool expected = false;
    if (!m_refreshing.compare_exchange_strong(expected, true))
        return;

    std::thread([this, authToken, baseUrl]() {
        std::list<DeviceObjectBasePtr> devices = fetchDeviceList(authToken, baseUrl);

        sendSigToUpdateDevice_T cb;
        {
            std::lock_guard<std::mutex> lock(m_deviceMutex);
            m_devices = std::move(devices);
            cb = m_updateDeviceCb;
        }
        m_refreshing = false;

        // Always notify so the UI re-renders (populated list or "no device").
        if (cb)
            cb();

        // Bring up MQTT monitoring for the freshly-fetched devices (idempotent).
        startMonitoring();
    }).detach();
}

void AnkerNetNativeImpl::AsyOneKeyPrint() {}

std::list<DeviceObjectBasePtr> AnkerNetNativeImpl::GetDeviceList() const
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    return m_devices;
}

std::list<DeviceObjectBasePtr> AnkerNetNativeImpl::GetMultiColorPartsDeviceList() const
{
    return {};
}

DeviceObjectBasePtr AnkerNetNativeImpl::getDeviceObjectFromSn(const std::string& sn)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    for (const auto& dev : m_devices) {
        if (dev && dev->GetSn() == sn)
            return dev;
    }
    return nullptr;
}

void AnkerNetNativeImpl::SetsendSigToSwitchPrintPage(sendSigToSwitchPrintPage_T /*function*/) {}
void AnkerNetNativeImpl::SetsendSigToUpdateDevice(sendSigToUpdateDevice_T function)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_updateDeviceCb = function;
}
void AnkerNetNativeImpl::SetsendSigToUpdateDeviceStatus(sendSigToUpdateDeviceStatus_T function)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_updateStatusCb = function;
}
void AnkerNetNativeImpl::SetsendSigToTransferFileProgressValue(sendSigToTransferFileProgressValue_T function)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_transferProgressCb = function;
}
void AnkerNetNativeImpl::SetsendShowDeviceListDialog(sendShowDeviceListDialog_T /*function*/) {}
void AnkerNetNativeImpl::SetGeneralExceptionMsgBox(GeneralExceptionMsgBox_T /*function*/) {}
void AnkerNetNativeImpl::SetSendSigAccountLogout(SendSigAccountLogout_T /*function*/) {}

void AnkerNetNativeImpl::PostSetBuryPointSwitch(bool /*isForbideenDataShared*/) {}

std::vector<std::tuple<int, std::string>> AnkerNetNativeImpl::PostQueryDataShared(const std::vector<int>& /*param_type*/)
{
    return {};
}

std::tuple<int, std::string> AnkerNetNativeImpl::PostUpdateDataShared(const std::vector<std::pair<int, std::string>>& /*param_type*/)
{
    return { 0, std::string() };
}

std::vector<std::tuple<std::string, int>> AnkerNetNativeImpl::PostGetMemberType()
{
    return {};
}

void AnkerNetNativeImpl::StartP2pOperator(P2POperationType type, const std::string& sn, const std::string& filePath)
{
    // Camera live view: connect P2P, request the stream, decode H.264 with
    // VideoToolbox, and hand RGB frames to the UI via setVideoFrameDataReadyCallBack.
    if (type == P2P_TRANSFER_VIDEO_STREAM) {
        if (m_videoRunning.exchange(true)) {
            std::fprintf(stderr, "[AnkerNetNative] video already running\n");
            return;
        }
        std::string ip, duid;
        {
            std::lock_guard<std::mutex> lock(m_deviceMutex);
            m_videoSn = sn;
            for (const auto& d : m_devices) {
                if (d && d->GetSn() == sn) {
                    auto dn = std::static_pointer_cast<DeviceObjectNative>(d);
                    ip = dn->ipAddr;
                    duid = dn->p2pDuid;
                    break;
                }
            }
        }

        std::thread([this, sn, ip, duid]() {
            AnkerPpppClient pppp;
            if (duid.empty() || !pppp.connectLan(duid, ip, 20)) {
                std::fprintf(stderr, "[AnkerNetNative] video PPPP connect failed\n");
                m_videoRunning.store(false);
                return;
            }

            AnkerH264Decoder decoder;
            decoder.setFrameCallback([this, sn](const uint8_t* rgb, int w, int h) {
                SnImgCallBackFunc cb;
                {
                    std::lock_guard<std::mutex> lock(m_deviceMutex);
                    cb = m_videoFrameCb;
                }
                if (cb) {
                    std::string s = sn;
                    cb(s, rgb, static_cast<short>(w), static_cast<short>(h));
                }
            });

            // Tell the UI the session is up (connecting -> p2pInit_OK); the first
            // decoded frame then flips it to Recving_Frame.
            {
                SnCallBackFunc initCb;
                {
                    std::lock_guard<std::mutex> lock(m_deviceMutex);
                    initCb = m_videoInitedCb;
                }
                if (initCb)
                    initCb(sn);
            }

            pppp.startLive();
            std::vector<uint8_t> chunk;
            while (m_videoRunning.load() && pppp.isConnected()) {
                if (pppp.recvVideoChunk(chunk, 3000) && !chunk.empty())
                    decoder.feed(chunk.data(), chunk.size());
            }

            pppp.stopLive();
            pppp.close();
            m_videoRunning.store(false);

            SnCallBackFunc closedCb;
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                closedCb = m_videoClosedCb;
            }
            if (closedCb)
                closedCb(sn);
            std::fprintf(stderr, "[AnkerNetNative] video session ended\n");
        }).detach();
        return;
    }

    // Only local gcode file transfer (send-to-printer) is implemented here.
    if (type != P2P_TRANSFER_LOCAL_FILE)
        return;

    // Resolve the device's LAN address + DUID.
    std::string ip, duid, name;
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        for (const auto& d : m_devices) {
            if (d && d->GetSn() == sn) {
                auto dn = std::static_pointer_cast<DeviceObjectNative>(d);
                ip = dn->ipAddr;
                duid = dn->p2pDuid;
                name = dn->name;
                break;
            }
        }
    }

    std::string userId;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        userId = m_login.user_id;
    }
    std::string machineId = m_clientGuid;

    bool expected = false;
    if (!m_transferring.compare_exchange_strong(expected, true)) {
        std::fprintf(stderr, "[AnkerNetNative] transfer already in progress\n");
        return;
    }

    std::thread([this, sn, ip, duid, name, userId, machineId, filePath]() {
        auto fireProgress = [this, sn](int progress, FileTransferResult result) {
            sendSigToTransferFileProgressValue_T cb;
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                cb = m_transferProgressCb;
            }
            if (cb)
                cb(sn, progress, result);
        };

        // Read the gcode file the app already exported.
        std::string data;
        {
            std::ifstream ifs(filePath, std::ios::binary);
            if (!ifs) {
                std::fprintf(stderr, "[AnkerNetNative] cannot open %s\n", filePath.c_str());
                fireProgress(0, FileTransferResult::OpenFileFailed);
                m_transferring = false;
                return;
            }
            std::ostringstream ss;
            ss << ifs.rdbuf();
            data = ss.str();
        }

        std::string fileName = boost::filesystem::path(filePath).filename().string();
        std::string userName = name.empty() ? "eufyStudio" : name;

        // ip may be empty -- connectLan() falls back to LAN broadcast discovery by DUID.
        AnkerPpppClient pppp;
        if (duid.empty() || !pppp.connectLan(duid, ip, 20)) {
            std::fprintf(stderr, "[AnkerNetNative] PPPP connect failed (ip='%s' duid='%s')\n",
                ip.c_str(), duid.c_str());
            fireProgress(0, FileTransferResult::InitFailed);
            m_transferring = false;
            return;
        }

        fireProgress(0, FileTransferResult::Transfering);
        bool ok = pppp.uploadFile(fileName, data, userName, userId, machineId,
            [&](size_t sent, size_t total) {
                int pct = total ? static_cast<int>(sent * 100 / total) : 0;
                fireProgress(pct, FileTransferResult::Transfering);
            });

        fireProgress(ok ? 100 : 0, ok ? FileTransferResult::Succeed : FileTransferResult::Failed);
        pppp.close();
        m_transferring = false;
    }).detach();
}

bool AnkerNetNativeImpl::closeVideoStream(int /*reason*/)
{
    m_videoRunning.store(false); // signals the video thread to stop + tear down
    return true;
}
void AnkerNetNativeImpl::setCameraLightState(bool /*onOff*/) {}
void AnkerNetNativeImpl::setVideoMode(P2P_Video_Mode_t /*mode*/) {}
std::string AnkerNetNativeImpl::getVideoCtrlSn()
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    return m_videoSn;
}
int AnkerNetNativeImpl::getVideoCtrlState() { return m_videoRunning.load() ? 1 : 0; }

void AnkerNetNativeImpl::setVideoFrameDataReadyCallBack(SnImgCallBackFunc callBackFunc)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_videoFrameCb = callBackFunc;
}
void AnkerNetNativeImpl::setP2PVideoStreamSessionInitedCallBack(SnCallBackFunc callBackFunc)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_videoInitedCb = callBackFunc;
}
void AnkerNetNativeImpl::setP2PVideoStreamSessionClosingCallBack(SnCallBackFunc /*callBackFunc*/) {}
void AnkerNetNativeImpl::setP2PVideoStreamSessionClosedCallBack(SnCallBackFunc callBackFunc)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    m_videoClosedCb = callBackFunc;
}
void AnkerNetNativeImpl::setP2PVideoStreamCtrlAbnormalCallBack(SnCallBackFunc /*callBackFunc*/) {}
void AnkerNetNativeImpl::setCameraLightStateCallBack(SnStateCallBackFunc /*callBackFunc*/) {}
void AnkerNetNativeImpl::setVideoModeCallBack(SnStateCallBackFunc /*callBackFunc*/) {}

void AnkerNetNativeImpl::setWebLoginCallBack(PrivayChoiceCb privayCb, LoginFinishCb /*loginCb*/)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_privayChoiceCb = privayCb;
}

void AnkerNetNativeImpl::ProcessWebScriptMessage(const std::string& webContent, WebJsProcessRet& JsProcessRet)
{
    // Default: not a message we recognize. AnkerWebView::OnScriptMessage treats this
    // as a bury/log event rather than a login failure, matching the real plugin's
    // observed behavior for messages outside the login flow.
    JsProcessRet = WebJsProcessRet{};

    // The login page always posts {"functionName":"<name>","data":{...}} shaped
    // messages via the anker_msg bridge (confirmed by reading the page's own
    // bundled JS, static/js/1.*.chunk.js). Dispatch on functionName.
    pt::ptree root;
    try {
        std::istringstream iss(webContent);
        pt::read_json(iss, root);
    } catch (const pt::json_parser_error&) {
        return;
    }

    auto functionName = root.get_optional<std::string>("functionName");
    if (!functionName)
        return;

    if (*functionName == "login" || *functionName == "loginback") {
        auto dataNode = root.get_child_optional("data");
        JsProcessRet.action = (*functionName == "login") ? WEB_ACTION::EM_LOGIN
                                                         : WEB_ACTION::EM_LOGINBACK;

        if (!dataNode || !parseLoginData(*dataNode)) {
            // Page reports logged-out / no usable identity.
            JsProcessRet.status = LOGIN_STATUS::EM_NO_DATA;
            return;
        }

        bool logined = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            logined = m_login.logined;
        }
        JsProcessRet.status = logined ? LOGIN_STATUS::EM_LOGIN_UNKNOW
                                      : LOGIN_STATUS::EM_USER_ID_NULL;

        if (logined) {
            saveLoginCache();
            PrivayChoiceCb cb;
            std::string abCode;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_privayChoiceCb;
                abCode = m_login.ab_code;
            }
            if (cb)
                cb(abCode);

            // Auto-load devices as soon as login is confirmed (covers the silent
            // startup login, which completes after the Device tab's own refresh has
            // already run and been skipped for not-yet-logged-in).
            AsyRefreshDeviceList();
        }
        return;
    }

    if (*functionName == "getHeaderList") {
        // Mirrors the exact header set the page's own JS falls back to when no
        // native bridge is present at all (i.e. running as a plain web page),
        // which is by construction a combination the backend already accepts.
        pt::ptree headers;
        headers.put("Accept", "application/json, text/plain, */*");
        headers.put("Content-Type", "application/json, text/plain, */*");
        headers.put("App_name", "anker_make");
        headers.put("Model_type", "PC");
        headers.put("App_version", "14");
        headers.put("Country", "US");
        headers.put("Language", "en");
        headers.put("Openudid", "4f917785-ab9d-4ba7-9820-bbcf3d0b4e21");
        headers.put("Os_type", "Mac");
        headers.put("Os_version", "26");

        std::ostringstream oss;
        pt::write_json(oss, headers, false /*pretty*/);

        // AnkerWebView embeds this content into a single-quoted JS string literal
        // (window["cb"]('<content>')). boost::write_json appends a trailing newline
        // and may emit \r; a raw newline inside the JS string literal is a syntax
        // error ("Unexpected EOF"), so strip them.
        std::string headerStr = oss.str();
        headerStr.erase(std::remove(headerStr.begin(), headerStr.end(), '\n'), headerStr.end());
        headerStr.erase(std::remove(headerStr.begin(), headerStr.end(), '\r'), headerStr.end());

        JsProcessRet.action = WEB_ACTION::EM_GET_HEADLIST;
        JsProcessRet.callBackName = root.get<std::string>("callbackName", "");
        JsProcessRet.content = headerStr;
    }
    else if (*functionName == "openBrowser") {
        auto url = root.get_optional<std::string>("data.url");
        if (url) {
            JsProcessRet.action = WEB_ACTION::EM_OPEN_BROWSER;
            JsProcessRet.content = *url;
        }
    }
}

std::string AnkerNetNativeImpl::GetUserInfo()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    pt::ptree root;
    root.put("user_id", m_login.user_id);
    root.put("email", m_login.email);
    root.put("nick_name", m_login.nick_name);
    std::ostringstream oss;
    pt::write_json(oss, root);
    return oss.str();
}

void AnkerNetNativeImpl::reportCommentData(StarCommentData /*data*/) {}
void AnkerNetNativeImpl::ProcessWebLoginFinish() {}

// ---------------------------------------------------------------------------
// Login payload parsing / persistence
// ---------------------------------------------------------------------------

bool AnkerNetNativeImpl::parseLoginData(const pt::ptree& dataNode)
{
    // Fields live under the payload's "data" object and are percent-encoded.
    auto authToken = dataNode.get_optional<std::string>("auth_token");
    auto userId = dataNode.get_optional<std::string>("user_id");
    if (!authToken || !userId || authToken->empty() || userId->empty())
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_login.auth_token = url_decode(*authToken);
    m_login.user_id = url_decode(*userId);
    m_login.email = url_decode(dataNode.get<std::string>("email", ""));
    m_login.nick_name = url_decode(dataNode.get<std::string>("nick_name", ""));
    m_login.avatar = url_decode(dataNode.get<std::string>("avatar", ""));
    m_login.ab_code = url_decode(dataNode.get<std::string>("ab_code", ""));
    m_login.token_expires_at = dataNode.get<std::string>("token_expires_at", "");
    m_login.domain = url_decode(dataNode.get<std::string>("domain", ""));
    m_login.public_key = dataNode.get<std::string>("server_secret_info.public_key", "");
    m_login.logined = true;

    return true;
}

std::string AnkerNetNativeImpl::loginCachePath() const
{
    return (boost::filesystem::path(m_dataDir) / "ankernet_native_login.json").string();
}

void AnkerNetNativeImpl::saveLoginCache() const
{
    if (m_dataDir.empty())
        return;

    pt::ptree root;
    root.put("auth_token", m_login.auth_token);
    root.put("user_id", m_login.user_id);
    root.put("email", m_login.email);
    root.put("nick_name", m_login.nick_name);
    root.put("avatar", m_login.avatar);
    root.put("ab_code", m_login.ab_code);
    root.put("token_expires_at", m_login.token_expires_at);
    root.put("domain", m_login.domain);
    root.put("public_key", m_login.public_key);

    try {
        boost::filesystem::create_directories(boost::filesystem::path(m_dataDir));
        std::ofstream ofs(loginCachePath());
        pt::write_json(ofs, root);
    } catch (const std::exception&) {
        // Non-fatal: worst case the user has to log in again next launch.
    }
}

void AnkerNetNativeImpl::loadLoginCache()
{
    if (m_dataDir.empty() || !boost::filesystem::exists(loginCachePath()))
        return;

    try {
        pt::ptree root;
        pt::read_json(loginCachePath(), root);
        m_login.auth_token = root.get<std::string>("auth_token", "");
        m_login.user_id = root.get<std::string>("user_id", "");
        m_login.email = root.get<std::string>("email", "");
        m_login.nick_name = root.get<std::string>("nick_name", "");
        m_login.avatar = root.get<std::string>("avatar", "");
        m_login.ab_code = root.get<std::string>("ab_code", "");
        m_login.token_expires_at = root.get<std::string>("token_expires_at", "");
        m_login.domain = root.get<std::string>("domain", "");
        m_login.public_key = root.get<std::string>("public_key", "");
        m_login.logined = !m_login.user_id.empty() && !m_login.auth_token.empty();
    } catch (const std::exception&) {
        m_login = LoginState{};
    }
}

// ---------------------------------------------------------------------------
// Device list (Phase 2): query_fdm_list + get_dsk_keys
// ---------------------------------------------------------------------------

std::string AnkerNetNativeImpl::apiBaseUrl() const
{
    // Caller holds m_mutex. The login payload's "domain" is the per-user API host;
    // fall back to the region hosts the community client uses (libflagship).
    if (!m_login.domain.empty())
        return "https://" + m_login.domain;
    if (m_login.ab_code == "EU" || m_login.ab_code == "eu")
        return "https://make-app-eu.ankermake.com";
    return "https://make-app.ankermake.com";
}

bool AnkerNetNativeImpl::httpPostV1(const std::string& baseUrl, const std::string& path,
    const std::string& authToken, const std::string& jsonBody,
    std::string& outResponse) const
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    const std::string url = baseUrl + path;
    const std::string authHeader = "X-Auth-Token: " + authToken;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authHeader.c_str());
    // Client-identification headers, matching the values the login web page uses
    // (see getHeaderList in ProcessWebScriptMessage). Harmless if unneeded.
    headers = curl_slist_append(headers, "App_name: anker_make");
    headers = curl_slist_append(headers, "Model_type: PC");
    headers = curl_slist_append(headers, "App_version: 14");
    headers = curl_slist_append(headers, "Country: US");
    headers = curl_slist_append(headers, "Language: en");
    headers = curl_slist_append(headers, "Os_type: Mac");
    headers = curl_slist_append(headers, "Os_version: 26");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonBody.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outResponse);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200) {
        std::fprintf(stderr, "[AnkerNetNative] POST %s failed: curl=%d http=%ld\n",
            path.c_str(), static_cast<int>(res), httpCode);
        return false;
    }
    return true;
}

std::list<DeviceObjectBasePtr> AnkerNetNativeImpl::fetchDeviceList(
    const std::string& authToken, const std::string& baseUrl) const
{
    std::list<DeviceObjectBasePtr> result;

    std::string listResp;
    if (!httpPostV1(baseUrl, "/v1/app/query_fdm_list", authToken, "{}", listResp))
        return result;

    pt::ptree root;
    try {
        std::istringstream iss(listResp);
        pt::read_json(iss, root);
    } catch (const pt::json_parser_error&) {
        std::fprintf(stderr, "[AnkerNetNative] query_fdm_list: bad JSON\n");
        return result;
    }

    if (root.get<int>("code", -1) != 0) {
        std::fprintf(stderr, "[AnkerNetNative] query_fdm_list code=%d msg=%s\n",
            root.get<int>("code", -1), root.get<std::string>("msg", "").c_str());
        return result;
    }

    auto dataNode = root.get_child_optional("data");
    if (!dataNode)
        return result;

    std::vector<std::shared_ptr<DeviceObjectNative>> devs;
    for (const auto& kv : *dataNode) {
        const pt::ptree& pr = kv.second;
        auto d = std::make_shared<DeviceObjectNative>();
        d->sn = pr.get<std::string>("station_sn", "");
        d->name = pr.get<std::string>("station_name", "");
        d->model = pr.get<std::string>("station_model", "");
        d->ipAddr = pr.get<std::string>("ip_addr", "");
        d->mqttKey = pr.get<std::string>("secret_key", "");
        d->p2pDuid = pr.get<std::string>("p2p_did", "");
        d->stationId = pr.get<int>("station_id", 0);
        d->online = true;
        d->setCommandSender([this](const std::string& sn, const std::string& json) {
            publishCommand(sn, json);
        });
        if (!d->sn.empty())
            devs.push_back(d);
    }

    if (devs.empty())
        return result;

    // get_dsk_keys returns the PPPP key per station (needed for Phase 3 transfer).
    // Body is hand-built so invalid_dsks renders as a real empty object "{}".
    std::string body = "{\"station_sns\":[";
    for (size_t i = 0; i < devs.size(); ++i) {
        if (i)
            body += ",";
        body += "\"" + devs[i]->sn + "\"";
    }
    body += "],\"invalid_dsks\":{}}";

    std::string dskResp;
    if (httpPostV1(baseUrl, "/v1/app/equipment/get_dsk_keys", authToken, body, dskResp)) {
        try {
            pt::ptree droot;
            std::istringstream iss(dskResp);
            pt::read_json(iss, droot);
            if (droot.get<int>("code", -1) == 0) {
                if (auto keys = droot.get_child_optional("data.dsk_keys")) {
                    std::map<std::string, std::string> keyBySn;
                    for (const auto& kv : *keys) {
                        keyBySn[kv.second.get<std::string>("station_sn", "")] =
                            kv.second.get<std::string>("dsk_key", "");
                    }
                    for (auto& d : devs)
                        d->p2pKey = keyBySn[d->sn];
                }
            }
        } catch (const pt::json_parser_error&) {
            std::fprintf(stderr, "[AnkerNetNative] get_dsk_keys: bad JSON\n");
        }
    }

    std::fprintf(stderr, "[AnkerNetNative] fetched %zu device(s)\n", devs.size());
    for (auto& d : devs)
        result.push_back(d);
    return result;
}

// ---------------------------------------------------------------------------
// MQTT monitoring (Phase 4)
// ---------------------------------------------------------------------------

namespace {
std::string makeClientId(const std::string& userId)
{
    // Unique-enough client id (broker rejects collisions). Short + user-scoped.
    unsigned long r = static_cast<unsigned long>(::time(nullptr)) ^ static_cast<unsigned long>(::getpid());
    char buf[64];
    std::snprintf(buf, sizeof(buf), "eufypc_%.8s_%lx",
        userId.empty() ? "anon" : userId.c_str(), r);
    return buf;
}
} // namespace

void AnkerNetNativeImpl::startMonitoring()
{
    // Only one connect attempt at a time; skip if already connected.
    if (m_mqtt && m_mqtt->isConnected())
        return;
    bool expected = false;
    if (!m_mqttStarting.compare_exchange_strong(expected, true))
        return;

    std::string userId, email, region;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_login.logined) {
            m_mqttStarting = false;
            return;
        }
        userId = m_login.user_id;
        email = m_login.email;
        region = m_login.ab_code;
    }

    std::vector<std::string> sns;
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        for (const auto& d : m_devices) {
            if (d)
                sns.push_back(d->GetSn());
        }
    }
    if (sns.empty()) {
        m_mqttStarting = false;
        return;
    }

    const bool eu = (region == "EU" || region == "eu");
    const std::string host = eu ? "make-mqtt-eu.ankermake.com" : "make-mqtt.ankermake.com";

    auto client = std::make_unique<AnkerMqttClient>();
    client->setMessageCallback(
        [this](const std::string& topic, const std::string& payload) {
            onMqttMessage(topic, payload);
        });

    if (!client->connect(host, 8789, "eufy_" + userId, email, makeClientId(userId))) {
        std::fprintf(stderr, "[AnkerNetNative] MQTT connect failed\n");
        m_mqttStarting = false;
        return;
    }

    for (const auto& sn : sns) {
        client->subscribe("/phone/maker/" + sn + "/notice");
        client->subscribe("/phone/maker/" + sn + "/command/reply");
        client->subscribe("/phone/maker/" + sn + "/query/reply");
        std::fprintf(stderr, "[AnkerNetNative] subscribed device %s\n", sn.c_str());
    }
    client->startLoop();

    m_mqtt = std::move(client);
    m_mqttStarting = false;

    // Ask each device for a full status snapshot so state the idle /notice omits
    // (e.g. z-offset) arrives on /query/reply.
    for (const auto& sn : sns)
        publishFramed(sn, "{\"commandType\":1027}", "/query");
}

void AnkerNetNativeImpl::publishFramed(const std::string& sn, const std::string& json,
    const std::string& topicSuffix) const
{
    std::string keyHex;
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        for (const auto& d : m_devices) {
            if (d && d->GetSn() == sn) {
                keyHex = std::static_pointer_cast<DeviceObjectNative>(d)->mqttKey;
                break;
            }
        }
    }
    if (keyHex.empty() || !m_mqtt || !m_mqtt->isConnected()) {
        std::fprintf(stderr, "[AnkerNetNative] publish dropped (offline) sn=%s\n", sn.c_str());
        return;
    }

    std::string guid;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        guid = m_clientGuid;
    }

    std::string frame = mqttframe::encode(json, keyHex, guid);
    if (frame.empty())
        return;

    m_mqtt->publish("/device/maker/" + sn + topicSuffix, frame);
    std::fprintf(stderr, "[AnkerNetNative] published sn=%s topic=%s json=%s\n",
        sn.c_str(), topicSuffix.c_str(), json.c_str());
}

void AnkerNetNativeImpl::publishCommand(const std::string& sn, const std::string& json) const
{
    publishFramed(sn, json, "/command");
}

void AnkerNetNativeImpl::onMqttMessage(const std::string& topic, const std::string& payload)
{
    // Topic: /phone/maker/<sn>/<kind>. Extract <sn> (4th path component).
    std::string sn;
    {
        size_t p1 = topic.find("/maker/");
        if (p1 != std::string::npos) {
            size_t start = p1 + 7;
            size_t end = topic.find('/', start);
            sn = topic.substr(start, end == std::string::npos ? std::string::npos : end - start);
        }
    }

    std::shared_ptr<DeviceObjectNative> dev;
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        for (const auto& d : m_devices) {
            if (d && d->GetSn() == sn) {
                dev = std::static_pointer_cast<DeviceObjectNative>(d);
                break;
            }
        }
    }
    if (!dev)
        return;

    std::string json = mqttframe::decode(payload, dev->mqttKey);
    if (json.empty()) {
        std::fprintf(stderr, "[AnkerNetNative] MQTT decode failed sn=%s topic=%s bytes=%zu\n",
            sn.c_str(), topic.c_str(), payload.size());
        return;
    }

    // Log command/query replies (low volume) so uncertain command formats -- z-offset
    // scaling, extrude flag values -- can be refined against the real printer.
    if (topic.find("/reply") != std::string::npos) {
        std::fprintf(stderr, "[AnkerNetNative] REPLY sn=%s topic=%s json=%s\n",
            sn.c_str(), topic.c_str(), json.c_str());
    }

    pt::ptree root;
    try {
        std::istringstream iss(json);
        pt::read_json(iss, root);
    } catch (const pt::json_parser_error&) {
        return;
    }

    dev->online = true;

    // /notice payloads are a JSON array of {"commandType":N,...}; /command/reply and
    // /query/reply are a single such object. Normalize to a list of objects.
    std::vector<const pt::ptree*> objs;
    if (root.get_optional<int>("commandType"))
        objs.push_back(&root);
    else
        for (const auto& kv : root)
            objs.push_back(&kv.second);

    std::vector<int> touched;
    for (const pt::ptree* objp : objs) {
        const pt::ptree& obj = *objp;
        int cmd = obj.get<int>("commandType", -1);
        switch (cmd) {
        case AKNMT_CMD_NOZZLE_TEMP: { // 1003, temps are x100
            int cur = obj.get<int>("currentTemp", 0), tgt = obj.get<int>("targetTemp", 0);
            dev->setNozzleTemp(cur / 100, tgt / 100);
            if (tgt != 0)
                std::fprintf(stderr, "[AnkerNetNative] STATUS nozzle cur=%d tgt=%d sn=%s topic=%s\n",
                    cur, tgt, sn.c_str(), topic.c_str());
            touched.push_back(cmd);
            break;
        }
        case AKNMT_CMD_HOTBED_TEMP: { // 1004
            int cur = obj.get<int>("currentTemp", 0), tgt = obj.get<int>("targetTemp", 0);
            dev->setBedTemp(cur / 100, tgt / 100);
            if (tgt != 0)
                std::fprintf(stderr, "[AnkerNetNative] STATUS bed cur=%d tgt=%d sn=%s topic=%s\n",
                    cur, tgt, sn.c_str(), topic.c_str());
            touched.push_back(cmd);
            break;
        }
        case AKNMT_CMD_Z_AXIS_RECOUP: { // 1021, z-offset (best guess: value is 1/100 mm)
            int v = obj.get<int>("value", 0);
            dev->setZOffsetValue(v / 100.0f);
            touched.push_back(cmd);
            break;
        }
        case 1081: { // print status/progress. value<0 => idle; else a print event.
            int value = obj.get<int>("value", -1);
            int progress = obj.get<int>("progress", 0);
            aknmt_print_event_e ev = (value < 0)
                ? AKNMT_PRINT_EVENT_IDLE
                : static_cast<aknmt_print_event_e>(value);
            dev->setPrintStatus(ev, progress);
            touched.push_back(cmd);
            break;
        }
        default:
            break;
        }
    }

    sendSigToUpdateDeviceStatus_T cb;
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);
        cb = m_updateStatusCb;
    }
    if (cb) {
        for (int cmd : touched)
            cb(sn, static_cast<aknmt_command_type_e>(cmd));
    }
}

} // namespace AnkerNet

extern "C" DLL_EXPORT AnkerNet::AnkerNetBase* GetAnkerNet()
{
    static AnkerNet::AnkerNetNativeImpl instance;
    return &instance;
}

extern "C" DLL_EXPORT int GetAnkerNetMappingVersion()
{
    return MappingVersion;
}

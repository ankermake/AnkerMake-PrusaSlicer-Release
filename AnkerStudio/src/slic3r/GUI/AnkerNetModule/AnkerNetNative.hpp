#ifndef ANKER_NET_NATIVE_HPP
#define ANKER_NET_NATIVE_HPP

#include "Interface Files/AnkerNetBase.h"
#include <atomic>
#include <ctime>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <boost/property_tree/ptree_fwd.hpp>

namespace AnkerNet {

class AnkerMqttClient;
class DeviceObjectNative;

// Native (open-source) replacement for the proprietary libAnkerNet plugin.
//
// Phase 1 scope: real login only. Login itself is performed by the real Anker/eufy
// web login page already loaded in AnkerWebView; this class's job is to parse the
// JSON payload that page posts via window.anker_msg.postMessage(...) (see
// ProcessWebScriptMessage) and expose the resulting identity through the getters
// below. Everything else (device list, P2P transfer, MQTT monitoring, message
// center, OTA, feedback) is a safe no-op for now, matching the null/empty-safe
// calling convention already used throughout the app.
class AnkerNetNativeImpl : public AnkerNetBase
{
public:
    bool Init(AnkerNetInitPara para) override;
    bool IsInit() override;
    void UnInit() override;

    void ResetLanguage(std::string country, std::string language) override;
    void setLogOutputCallBack(LogOutputCallBackFunc callBackFunc) override;

    void AsyAddEvent(const std::string& eventName, const std::map<std::string, std::string>& eventMap) override;
    int AsyDownLoad(const std::string& url,
        const std::string& localFilePath,
        void* userData,
        CallBackFunction callbackfunc,
        ProgressCallback progressCallbackFunc,
        bool isBlock = false,
        unsigned int nTimeOut = 600) override;

    int getCurrentEnvironmentType() override;
    void AsyPostFeedBack(FeedBackInfo info) override;
    void PostGetPrintStopReasons(PrintStopReasons& reasons, const std::string& station_sn) override;
    bool PostGetSliceTips(SliceTips& sliceTips) override;

    void SetCallback_RecoverCurl(NormalCallBack callback) override;
    void SetCallback_OtaInfoRecv(CallBack_OtaInfoRecv callback) override;
    void SetCallback_FilamentRecv(NormalCallBack callback) override;
    void SetCallback_GetMsgCenterConfig(CallBack_MsgCenterCfg callback) override;
    void SetCallback_GetMsgCenterRecords(CallBack_MsgCenterRecords callback) override;
    void SetCallback_GetMsgCenterErrorCodeInfo(CallBack_MsgCenterErrCodeInfo callback) override;
    void SetCallback_GetMsgCenterStatus(CallBack_MsgCenterStatus callback) override;
    void SetCallback_CommentFlagsRecv(CommentFlagsCallBack callback) override;
    void SetsendSigHttpError(sendSigHttpError_T function) override;

    void GetMsgCenterRecords(const int& newType, const int& num, const int& page, bool isSyn = false) override;
    void GetMsgCenterStatus() override;

    std::string GetNickName() override;
    std::string GetAvatar() override;
    std::string GetUserId() override;
    std::string GetUserEmail() override;

    void SetOtaCheckType(OtaCheckType type) override;
    OtaCheckType GetOtaCheckType() override;
    void queryOTAInformation() override;

    void SetGcodePath(const std::string& path) override;
    std::string GetGcodePath() override;

    bool IsLogined() override;
    void removeMsgByIds(const std::vector<int>& msgList, bool isSyn = false) override;
    void logout() override;
    void logoutToServer() override;

    void AsyRefreshDeviceList() override;
    void AsyOneKeyPrint() override;

    std::list<DeviceObjectBasePtr> GetDeviceList() const override;
    std::list<DeviceObjectBasePtr> GetMultiColorPartsDeviceList() const override;
    DeviceObjectBasePtr getDeviceObjectFromSn(const std::string& sn) override;

    void SetsendSigToSwitchPrintPage(sendSigToSwitchPrintPage_T function) override;
    void SetsendSigToUpdateDevice(sendSigToUpdateDevice_T function) override;
    void SetsendSigToUpdateDeviceStatus(sendSigToUpdateDeviceStatus_T function) override;
    void SetsendSigToTransferFileProgressValue(sendSigToTransferFileProgressValue_T function) override;
    void SetsendShowDeviceListDialog(sendShowDeviceListDialog_T function) override;
    void SetGeneralExceptionMsgBox(GeneralExceptionMsgBox_T function) override;
    void SetSendSigAccountLogout(SendSigAccountLogout_T function) override;

    void PostSetBuryPointSwitch(bool isForbideenDataShared) override;
    std::vector<std::tuple<int, std::string>> PostQueryDataShared(const std::vector<int>& param_type) override;
    std::tuple<int, std::string> PostUpdateDataShared(const std::vector<std::pair<int, std::string>>& param_type) override;
    std::vector<std::tuple<std::string, int>> PostGetMemberType() override;

    void StartP2pOperator(P2POperationType type, const std::string& sn, const std::string& filePath) override;

    bool closeVideoStream(int reason = 0) override;
    void setCameraLightState(bool onOff) override;
    void setVideoMode(P2P_Video_Mode_t mode) override;
    std::string getVideoCtrlSn() override;
    int getVideoCtrlState() override;

    void setVideoFrameDataReadyCallBack(SnImgCallBackFunc callBackFunc) override;
    void setP2PVideoStreamSessionInitedCallBack(SnCallBackFunc callBackFunc) override;
    void setP2PVideoStreamSessionClosingCallBack(SnCallBackFunc callBackFunc) override;
    void setP2PVideoStreamSessionClosedCallBack(SnCallBackFunc callBackFunc) override;
    void setP2PVideoStreamCtrlAbnormalCallBack(SnCallBackFunc callBackFunc) override;
    void setCameraLightStateCallBack(SnStateCallBackFunc callBackFunc) override;
    void setVideoModeCallBack(SnStateCallBackFunc callBackFunc) override;

    void setWebLoginCallBack(PrivayChoiceCb privayCb, LoginFinishCb loginCb) override;
    void ProcessWebScriptMessage(const std::string& webContent, WebJsProcessRet& JsProcessRet) override;
    std::string GetUserInfo() override;

    void reportCommentData(StarCommentData data) override;
    void ProcessWebLoginFinish() override;

private:
    // Parses the login page's {"functionName":"login"|"loginback","data":{...}}
    // postMessage payload (fields are percent-encoded) and updates m_login in place.
    // Returns false if dataNode doesn't carry a usable auth_token/user_id.
    bool parseLoginData(const boost::property_tree::ptree& dataNode);

    std::string loginCachePath() const;
    void saveLoginCache() const;
    void loadLoginCache();

    // Authenticated API base (https://<domain>, falling back to region host).
    // Caller must hold m_mutex (reads m_login).
    std::string apiBaseUrl() const;
    // Blocking POST to <baseUrl><path> with X-Auth-Token + client-id headers.
    // Returns true only on transport success + HTTP 200.
    bool httpPostV1(const std::string& baseUrl, const std::string& path,
        const std::string& authToken, const std::string& jsonBody,
        std::string& outResponse) const;
    // Fetches query_fdm_list + get_dsk_keys and builds DeviceObjectNative list.
    std::list<DeviceObjectBasePtr> fetchDeviceList(
        const std::string& authToken, const std::string& baseUrl) const;

    // MQTT monitoring (Phase 4): connect to the broker and subscribe to each
    // device's status topics. onMqttMessage decrypts + dispatches telemetry.
    void startMonitoring();
    void onMqttMessage(const std::string& topic, const std::string& payload);
    // Frames + encrypts a JSON payload and publishes it to /device/maker/<sn><suffix>.
    void publishFramed(const std::string& sn, const std::string& json,
        const std::string& topicSuffix) const;
    // Frames + encrypts a JSON command and publishes it to the device's command
    // topic. Looked up by sn for the per-device AES key. Safe no-op if offline.
    void publishCommand(const std::string& sn, const std::string& json) const;

private:
    mutable std::mutex m_mutex;

    bool m_init = false;
    std::string m_dataDir;

    // Populated from the LOGIN_DATA JSON the real web login page posts back.
    struct LoginState {
        bool logined = false;
        std::string user_id;
        std::string email;
        std::string nick_name;
        std::string auth_token;
        std::string token_expires_at;
        std::string avatar;
        std::string ab_code;
        // Per-user API host (e.g. "make-app.ankermake.com") and the account's
        // ECDH public key -- both needed for the authenticated Phase 2 API calls.
        std::string domain;
        std::string public_key;
    } m_login;

    std::string m_gcodePath;
    OtaCheckType m_otaCheckType = OtaCheckType_Unknown;

    PrivayChoiceCb m_privayChoiceCb;

    // Device list + refresh plumbing (Phase 2). Guarded by m_deviceMutex, kept
    // separate from m_mutex so a refresh can read login state (m_mutex) and then
    // publish the list (m_deviceMutex) without lock-ordering hazards.
    mutable std::mutex m_deviceMutex;
    std::list<DeviceObjectBasePtr> m_devices;
    sendSigToUpdateDevice_T m_updateDeviceCb;
    std::atomic<bool> m_refreshing{ false };

    // MQTT client + the per-device status callback the UI registers.
    std::unique_ptr<AnkerMqttClient> m_mqtt;
    std::atomic<bool> m_mqttStarting{ false };
    sendSigToUpdateDeviceStatus_T m_updateStatusCb;
    std::string m_clientGuid; // sender guid stamped into outgoing MA frames

    // Phase 3: PPPP file-transfer progress callback + a guard against concurrent sends.
    sendSigToTransferFileProgressValue_T m_transferProgressCb;
    std::atomic<bool> m_transferring{ false };

    // Camera video (Stage 2): callbacks the UI registers + live-session state.
    SnImgCallBackFunc m_videoFrameCb;
    SnCallBackFunc m_videoInitedCb;
    SnCallBackFunc m_videoClosedCb;
    std::atomic<bool> m_videoRunning{ false };
    std::string m_videoSn;

    // Local print-usage log (no cloud "message center" -- just a durable record of
    // start/finish/duration/filament/layers per print, appended as CSV). Tracks the
    // start time+filename per device between the PRINTING/PRINT_HEATING transition in
    // and the COMPLETED/IDLE transition out, guarded by m_deviceMutex.
    struct PrintHistoryStart { time_t startTime; std::string fileName; };
    std::map<std::string, PrintHistoryStart> m_printHistoryStart;
    void logPrintHistoryTransition(const std::string& sn, DeviceObjectNative* dev,
        aknmt_print_event_e prevEv, aknmt_print_event_e newEv);
};

} // namespace AnkerNet

#endif // !ANKER_NET_NATIVE_HPP

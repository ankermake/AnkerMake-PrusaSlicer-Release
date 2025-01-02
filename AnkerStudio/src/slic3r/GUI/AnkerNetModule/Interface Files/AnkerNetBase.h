#ifndef ANKER_NET_BASE_H
#define ANKER_NET_BASE_H

#include <map>
#include <list>
#include <functional>
#include <boost/signals2.hpp>
#include "AnkerNetDefines.h"

namespace AnkerNet
{
	
DEF_PTR(DeviceObjectBase)
class DLL_EXPORT AnkerNetBase
{
public:
	//init module and create data
	virtual bool Init(AnkerNetInitPara para) = 0;
	//check whether module is init
	virtual bool IsInit() = 0;
	//uninit module and release data
	virtual void UnInit() = 0;

	//reset language and country.
	//it affects the prompt messages of some network interfaces.
	virtual void ResetLanguage(std::string country, std::string language) = 0;

	//set callback of log
    virtual void setLogOutputCallBack(LogOutputCallBackFunc callBackFunc) = 0;

	//post Bury Point by http.
	//it is async interface
	virtual void AsyAddEvent(const std::string& eventName, const std::map<std::string, std::string>& eventMap) = 0;

	//download file  by http.
	//it is a async interface
	virtual int AsyDownLoad(const std::string& url,
		const std::string& localFilePath,
		void* userData,
		CallBackFunction callbackfunc,
		ProgressCallback progressCallbackFunc,
		bool isBlock = false,
		unsigned int nTimeOut = 600) = 0;

	//get current env type
	virtual int getCurrentEnvironmentType() = 0;
	//post feedback by http
	//it is a async interface
	virtual void AsyPostFeedBack(FeedBackInfo info) = 0;
	virtual void PostGetPrintStopReasons(PrintStopReasons &reasons, const std::string& station_sn) = 0;
	// post get slice tips by http
	virtual inline bool PostGetSliceTips(SliceTips& sliceTips) = 0;

	//set callback for Recover Curl , triggered when curl return CURLE_COULDNT_RESOLVE_HOST
	virtual void SetCallback_RecoverCurl(NormalCallBack callback) = 0;
	//set callback for process ota info , triggered when receive ota info 
	virtual void SetCallback_OtaInfoRecv(CallBack_OtaInfoRecv callback) = 0;
	//set callback for update filament info , triggered when receive filament info and is newer than local
	virtual void SetCallback_FilamentRecv(NormalCallBack callback) = 0;
	virtual void SetCallback_GetMsgCenterConfig(CallBack_MsgCenterCfg callback) = 0;
	virtual void SetCallback_GetMsgCenterRecords(CallBack_MsgCenterRecords callback) = 0;
	virtual void SetCallback_GetMsgCenterErrorCodeInfo(CallBack_MsgCenterErrCodeInfo callback) = 0;
	virtual void SetCallback_GetMsgCenterStatus(CallBack_MsgCenterStatus callback) = 0;	
	virtual void SetCallback_CommentFlagsRecv(CommentFlagsCallBack callback) = 0;
	//set callback for process http error , triggered when http interface return error
	virtual void SetsendSigHttpError(sendSigHttpError_T function) = 0;

	virtual void GetMsgCenterRecords(const int& newType, const int& num, const int& page,bool isSyn = false) = 0;
	virtual void GetMsgCenterStatus() = 0;
	//get nick name of user
	virtual std::string GetNickName() = 0;
	//get download url of Avatar of user
	virtual std::string GetAvatar() = 0;
	//get user id
	virtual std::string GetUserId() = 0;
	//get email url of user
	virtual std::string GetUserEmail() = 0;

	//set ota check type
	virtual void SetOtaCheckType(OtaCheckType type) = 0;
	//get ota check type
	virtual OtaCheckType GetOtaCheckType() = 0;
	//query OTA Info by http
	//it is  a [sync] interface
	virtual void queryOTAInformation() = 0;

	//set gcode path for P2P transfer
	virtual void SetGcodePath(const std::string& path) = 0;
	//get gcode path
	virtual std::string GetGcodePath() = 0;

	//check whether user is logined
	virtual bool IsLogined() = 0;
	//remove msg item by msgids
	virtual void removeMsgByIds(const std::vector<int>& msgList,bool isSyn = false) = 0;
	//log out and clear data and status
	virtual void logout() = 0;
	virtual void logoutToServer() = 0;
	//refresh device list
	//it is a async interface
	virtual void AsyRefreshDeviceList() = 0;
	//start one key print.
	//In fact, it is a combination of refreshing the device and send sig for displaying the pop-up box.
	//it is a async interface
	virtual void AsyOneKeyPrint() = 0;

	//get device list from cache
	virtual std::list<DeviceObjectBasePtr> GetDeviceList() const = 0;
	//get color device list from cache
	//it is used for color device.
	virtual std::list<DeviceObjectBasePtr> GetMultiColorPartsDeviceList() const = 0;

	//get device object by sn
	virtual DeviceObjectBasePtr getDeviceObjectFromSn(const std::string& sn) = 0;

	//set callback for switch main page to print , triggered when start printing after transfer complete
	virtual void SetsendSigToSwitchPrintPage(sendSigToSwitchPrintPage_T function) = 0;
	//set callback for update device list on ui , triggered when login ,AsyRefreshDeviceList,and AsyOneKeyPrint
	virtual void SetsendSigToUpdateDevice(sendSigToUpdateDevice_T function) = 0;
	//set callback for update device status on ui , triggered when detect device is not online .
	virtual void SetsendSigToUpdateDeviceStatus(sendSigToUpdateDeviceStatus_T function) = 0;
	//set callback for update transfer status on ui , triggered when transfer is running or stopped.
	virtual void SetsendSigToTransferFileProgressValue(sendSigToTransferFileProgressValue_T function) = 0;
	//set callback for show device list dialog ,triggered when exec AsyOneKeyPrint and device is found
	virtual void SetsendShowDeviceListDialog(sendShowDeviceListDialog_T function) = 0;
	//set callback for get general exception msg from outside , triggered before trigger sendSigToMessageBox_T
	virtual void SetGeneralExceptionMsgBox(GeneralExceptionMsgBox_T function) = 0;
	//set callback for log out and modify status on ui , triggered when login init failed
	virtual void SetSendSigAccountLogout(SendSigAccountLogout_T function) = 0;
	virtual inline void PostSetBuryPointSwitch(bool isForbideenDataShared) = 0;
	virtual inline std::vector<std::tuple<int, std::string>> PostQueryDataShared(const std::vector<int>& param_type) = 0;
	virtual inline std::tuple<int, std::string> PostUpdateDataShared(const std::vector<std::pair<int, std::string>>& param_type) = 0;
	virtual std::vector<std::tuple<std::string, int>> PostGetMemberType() = 0;


	// p2p operator start
	virtual void StartP2pOperator(P2POperationType type, const std::string& sn, const std::string& filePath) = 0;

	// p2p video
	//close video stream and release data
	virtual bool closeVideoStream(int reason = 0) = 0;
	//set light state of device for image brightness
	virtual void setCameraLightState(bool onOff) = 0;
	//set video mode for image clarity
	virtual void setVideoMode(P2P_Video_Mode_t mode) = 0;
	//get device sn of current video stream
	virtual std::string getVideoCtrlSn() = 0;
	//get video state .
	virtual int getVideoCtrlState() = 0;

	//set callback for show frame , triggered when receive data and decode to frame
	virtual void setVideoFrameDataReadyCallBack(SnImgCallBackFunc callBackFunc) = 0;
	virtual void setP2PVideoStreamSessionInitedCallBack(SnCallBackFunc callBackFunc) = 0;
	virtual void setP2PVideoStreamSessionClosingCallBack(SnCallBackFunc callBackFunc) = 0;
	virtual void setP2PVideoStreamSessionClosedCallBack(SnCallBackFunc callBackFunc) = 0;
	virtual void setP2PVideoStreamCtrlAbnormalCallBack(SnCallBackFunc callBackFunc) = 0;
	virtual void setCameraLightStateCallBack(SnStateCallBackFunc callBackFunc) = 0;
	virtual void setVideoModeCallBack(SnStateCallBackFunc callBackFunc) = 0;

	//set callback for update PrivayChoice and update login status on ui,trigger when user login success
	virtual void setWebLoginCallBack(PrivayChoiceCb privayCb, LoginFinishCb loginCb) = 0;
	//process js msg from web page ,mainly for login,and return process result
	virtual void ProcessWebScriptMessage(const std::string& webContent, WebJsProcessRet& JsProcessRet) = 0;

	//get user info
	virtual std::string GetUserInfo() = 0;

	virtual void reportCommentData(StarCommentData data) = 0;

	// process something after web login finish 
	virtual void ProcessWebLoginFinish() = 0;
};

}

typedef AnkerNet::AnkerNetBase* (*GetAnkerNet_T)();
typedef int (*GetAnkerNetMappingVersion_T)();

#endif // !ANKER_NET_BASE_H

#ifndef ANKER_NET_BASE_H
#define ANKER_NET_BASE_H

#include <iostream>
#include <map>
#include <list>
#include <functional>
#include <boost/signals2.hpp>
#ifdef __APPLE__
#define DLL_EXPORT  __attribute__((visibility("default")))
#else
#define  DLL_EXPORT _declspec(dllexport)
#endif

typedef size_t(*CallBackFunction)(char* dest, size_t size, size_t nmemb, void* userp);
#define bindHttpCallback(func) std::bind(&func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,std::placeholders::_4)
#define	HttpCallBackFunc std::function<size_t(char* dest, size_t size, size_t nmemb, void* userp)>
#define ProgressCallback std::function<void (double dltotal, double dlnow, double ultotal, double ulnow)>
typedef int(*UploadFileProgressCallBack)(void* userdata, long long dltotal, long long dlnow, long long ultotal, long long ulnow);

#define StrMsgCallBackFunc std::function<void (std::string data)>
#define IndexCallBackFunc std::function<void (int index)>
#define ResCallBackFunc std::function<void (std::string data,bool isTrue)>
#define ResStrCallBackFunc std::function<void (std::string data,std::string msg)>
#define ResIndexCallBackFunc std::function<void (std::string data,int index)>


#define SnCallBackFunc std::function<void (const std::string& sn)>
#define SnStateCallBackFunc std::function<void (const std::string& sn, const int state)>
#define SnImgCallBackFunc std::function<void (std::string& sn, const unsigned char* imgDate, short frameWith, short frameHeight)>
#define PlayStopStateCallBackFunc std::function<void (bool playStopState)>

#define LogOutputCallBackFunc std::function<void (unsigned int logLevel, const std::string strLogMsg,  const std::string strFileName,\
            const std::string strFuncName, const unsigned int lineNumber)>

class DLL_EXPORT AnkerNetBase
{
public:
	AnkerNetBase() {};
	virtual ~AnkerNetBase() {};
    // You can use the log printing macro only after calling this interface, see the ReadMe documentation for details
    virtual void setLogOutputCallBack(LogOutputCallBackFunc callBackFunc) {};

	//http api
	virtual int AsyPost(const std::string& url,
						const std::string& data,
						const std::vector<std::string>& headrList,
						void* userData,
						CallBackFunction callbackfunc,
						bool isBlock = false,
						unsigned int nTimeOut = 0) { 
		return 0; 
	};

	virtual int AsyGet(const std::string& url,
					   const std::string& data,
					   const std::vector<std::string>& headrList,
					   void* userData,
					   CallBackFunction callbackfunc,
					   bool isBlock = false,
					   unsigned int nTimeOut = 0) {
		return 0;
	};

	virtual int AsyPut(const std::string& url,
					   const std::string& data,
					   const std::vector<std::string>& headrList,
					   void* userData,
					   CallBackFunction callbackfunc,
					   bool isBlock = false,
					   unsigned int nTimeOut = 0) {
		return 0;
	};

	virtual int AsyDelete(const std::string& url,
						  const std::string& data,
						  const std::vector<std::string>& headrList,
						  void* userData,
						  CallBackFunction callbackfunc,
						  bool isBlock = false,
						  unsigned int nTimeOut = 0) {
		return 0;
	};


	virtual int AsyUpload(const std::string& url,
						  const std::string& localFilePath,
						  const std::vector<std::string>& headrList,
						  CallBackFunction callbackfunc,
						  ProgressCallback progressCallbackFunc,
						  bool isBlock = false,
						  unsigned int nTimeOut = 0) {
		return 0;
	};

	virtual int PostUploadFile(const std::string& url,
		const std::string& data,
		const std::string& localFilePath,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunction callbackfunc,
		UploadFileProgressCallBack progressCallbackFunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) {
		return 0;
	};

	virtual int PostUploadFeedBack(const std::string& url,
		const std::string& data,
		const std::string& localFilePath,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunction callbackfunc,
		UploadFileProgressCallBack progressCallbackFunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) {
		return 0;
	};

	virtual int AsyDownLoad(const std::string& url,
							const std::string& localFilePath,
							const std::vector<std::string>& headrList,
							void* userData,
							CallBackFunction callbackfunc,
							ProgressCallback progressCallbackFunc,
							bool isBlock = false,
							unsigned int nTimeOut = 0) {
		return 0;
	};

	//mqtt api
	//virtual void setMqttCfg(ASYN_MQTT_DATA data);
	virtual void setLoginState(bool login) {};
	virtual void setMqttCfg(void* data) {};
	virtual bool getMqttState() { return true; };
	virtual bool disConnectMqtt() { return true; };
	virtual int startConnectMqtt() { return 0; };
	virtual int sendMsgToMqtt(const std::string& topic, const std::string& msg) { return 0; };
	virtual void setSubscribeMqtt(std::list<std::string> topicList) {};
	virtual void setSubscribeMqtt(const std::string& topic) {};
	virtual void setMqttKeys(const std::map<std::string, std::string>& keys) {};

	//P2P api	
	//p2pWriteData
	virtual void p2pWriteData(void* data) {};
	
	//p2pReadData
	virtual void p2pReadData(void* data) {};
	
	// p2pBreakPointCtrl
	virtual void initBreakPointCtrlData(void* data) {};
	virtual void setBreakPointSn(const std::string& sn) {};
	virtual void setBeakPointHttpData(const std::map<int, std::string>& data) {};
	virtual void startBreakPointWork(const std::string& usrName, const std::string& fileName) {};

	//p2pReadWriteData
	virtual void setDescriptor(int descriptor) {};
	virtual void setSelectDisplayCmdStr(const std::string& cmdStr) {};
	virtual int getDescriptor() const { return 0; };
	virtual std::string getSelectDisplayCmdStr() const { return ""; };

	virtual int initWrite() { return 0; };
	virtual int initP2P(const std::string& didStr,
					    const std::string& initStr,
						int mode) {
		return 0;
	};

	virtual int initP2P(void* data) { return 0; };
	virtual void setSn(const std::string& sn) {};
	virtual std::string getSn() const { return ""; };
	virtual void setDsk(const std::string& dsk) {};
	virtual std::string getDsk() const { return ""; };
	virtual void setHttpsSelectData(std::map<int, std::string> data) {};
	virtual void startWorkThreadRead(const std::string& fileName) {};
	virtual void startWorkThreadWrite(const std::string& usrName, const std::string& fileName) {};
	virtual void startWriteWork(const std::string& usrName, const std::string& fullpathName, const std::string& fileName) {};
	virtual boost::signals2::connection transferFileConnect(const boost::signals2::slot<void(int)>& slot) {
		return boost::signals2::connection();
	};
	virtual boost::signals2::connection mqttRecoveryConnect(const boost::signals2::slot<void(bool)>& slot) {
		return boost::signals2::connection();
	};

	//setCallBackFunction
	//signal p2pWriteData
	virtual void setP2pWDResReadCallBack(StrMsgCallBackFunc callBackFunc) {};
	virtual void setP2pWDResSendCallBack(StrMsgCallBackFunc callBackFunc) {};
	virtual void setP2pWDResErrMsgCallBack(StrMsgCallBackFunc callBackFunc) {};
	virtual void setP2pWDTransferFileValueCallBack(IndexCallBackFunc callBackFunc) {};

	//signal p2pReadData
	virtual void setP2pRDFinishedCallBack(StrMsgCallBackFunc callBackFunc) {};

	//signal p2pBreakPointCtrl
	virtual void setP2pBPCWriteResCallBack(ResStrCallBackFunc callBackFunc) {};
	virtual void setP2pBPTransferFileValueCallBack(IndexCallBackFunc callBackFunc) {};
	virtual void setP2pBPCThreadStatusChangedCallBack(IndexCallBackFunc callBackFunc) {};
	virtual void setP2pBPCIsNeedLevelCallBack(ResCallBackFunc callBackFunc) {};

	//signal p2pReadWriteData
	virtual void setP2pWRFileResCallBack(ResStrCallBackFunc callBackFunc) {};
	virtual void setP2pWRErrMsgCallBack(StrMsgCallBackFunc callBackFunc) {};
	virtual void setP2pWRTransferFileValueCallBack(IndexCallBackFunc callBackFunc) {};
	virtual void setP2pWRTransferFileFinishedCallBack(ResIndexCallBackFunc callBackFunc) {};


	// p2p video 
	virtual void openVideoStream(void* pData) {};
	virtual bool closeVideoStream(int reason = 0) { return true; };
	virtual void setCameraLightState(bool onOff) {};
	virtual void setVideoMode(int mode) {};
	virtual std::string getVideoCtrlSn() { return ""; };
	virtual int getVideoCtrlState() { return 0; };

	virtual void setVideoFrameDataReadyCallBack(SnImgCallBackFunc callBackFunc) {};
	virtual void setVideoPlayStopStateChangeCallBack(PlayStopStateCallBackFunc callBackFunc) {};
	virtual void setP2PVideoStreamSessionInitedCallBack(SnCallBackFunc callBackFunc) {};
	virtual void setP2PVideoStreamSessionClosingCallBack(SnCallBackFunc callBackFunc) {};
	virtual void setP2PVideoStreamSessionClosedCallBack(SnCallBackFunc callBackFunc) {};
	virtual void setP2PVideoStreamCtrlAbnormalCallBack(SnCallBackFunc callBackFunc) {};
	virtual void setVideoStreamStateCallBack(SnStateCallBackFunc callBackFunc) {};
	virtual void setCameraLightStateCallBack(SnStateCallBackFunc callBackFunc) {};
	virtual void setVideoModeCallBack(SnStateCallBackFunc callBackFunc) {};
	virtual void unsetAllVideoCallBacks() {};
	virtual boost::signals2::connection videoStopSussConnect(const boost::signals2::slot<void(const std::string&)>& slot) {
		return boost::signals2::connection();
	};

protected:
	//forward slot to signal P2PWriteData
	//slot p2pWriteData
	virtual void onP2pWDResReadSlot(const std::string& data) {};
	virtual void onP2pWDResSendSlot(const std::string& data) {};
	virtual void onP2pWDResErrMsgSlot(const std::string& data) {};
	virtual void onP2pWDTransferFileValueSlot(const int& index) {};

	//slot p2pReadData
	virtual void onP2pRDFinishedSlot(const std::string& data) {};

	//slot p2pBreakPointCtrl
	virtual void onP2pBPCWriteResSlot(const std::string& data, const std::string& msg) {};
	virtual void onP2pBPTransferFileValueSlot(const int& index) {};
	virtual void onP2pBPCThreadStatusChangedSlot(const int& index) {};
	virtual void onP2pBPCIsNeedLevelSlot(const std::string& data,bool isTrue) {};

	//slot p2pReadWriteData
	virtual void onP2pWRFileResSlot(const std::string& data, const std::string& msg) {};
	virtual void onP2pWRErrMsgSlot(const std::string& data) {};
	virtual void onP2pWRTransferFileValueSlot(const int& index) {};
	virtual void onP2pWRTransferFileFinishedSlot(const std::string& data, int index) {};
private:
};

#endif // !ANKER_NET_BASE_H


#include "AnkerNetBase.h"


AnkerNetBase::AnkerNetBase()
{

}

AnkerNetBase::~AnkerNetBase()
{

}

int AnkerNetBase::AsyPost(const std::string& url, 
						  const std::string& data,
						  const std::vector<std::string>& headrList,
						  void* userData, 
						  CallBackFunction callbackfunc,
						  bool isBlock,/*false*/
						  unsigned int nTimeOut /*= 0*/)
{
	return 0;
}

int AnkerNetBase::AsyGet(const std::string& url,
						 const std::string& data, 
						 const std::vector<std::string>& headrList,
						 void* userData, 
						 CallBackFunction callbackfunc, 
						 bool isBlock,/*fasle*/
						 unsigned int nTimeOut /*= 0*/)
{
	return 0;
}


int AnkerNetBase::AsyPut(const std::string& url,
						const std::string& data,
						const std::vector<std::string>& headrList,
						void* userData,
						CallBackFunction callbackfunc,
						bool isBlock /*= false*/, 
						unsigned int nTimeOut /*= 0*/)
{
	return 0;
}


int AnkerNetBase::AsyDelete(const std::string& url,
							const std::string& data,
							const std::vector<std::string>& headrList,
							void* userData,
							CallBackFunction callbackfunc, 
							bool isBlock /*= false*/, 
							unsigned int nTimeOut /*= 0*/)
{
	return 0;
}

int AnkerNetBase::AsyUpload(const std::string& url,
							const std::string& localFilePath, 
							const std::vector<std::string>& headrList,
							CallBackFunction callbackfunc,
							ProgressCallback progressCallbackFunc,
							bool isBlock /*= false*/, 
							unsigned int nTimeOut /*= 0*/)
{
	return 0;
}

int AnkerNetBase::AsyDownLoad(const std::string& url,
							  const std::string& localFilePath,
							  const std::vector<std::string>& headrList,
							  void *userData,
							  CallBackFunction callbackfunc, 
							  ProgressCallback progressCallbackFunc,
							  bool isBlock /*= false*/,
							  unsigned int nTimeOut /*= 0*/)
{ 
	return 0;
}


void AnkerNetBase::setLoginState(bool login)
{
}

void AnkerNetBase::setMqttCfg(void* data)
{

}

bool AnkerNetBase::getMqttState()
{
	return true;
}

bool AnkerNetBase::disConnectMqtt()
{
	return true;
}

int AnkerNetBase::startConnectMqtt()
{
	return 0;
}

int AnkerNetBase::sendMsgToMqtt(const std::string& topic, const std::string& msg)
{
	return 0;
}

void AnkerNetBase::setSubscribeMqtt(std::list<std::string> topicList)
{

}

void AnkerNetBase::setSubscribeMqtt(const std::string& topic)
{

}

void AnkerNetBase::setMqttKeys(const std::map<std::string, std::string>& keys)
{
}

void AnkerNetBase::p2pWriteData(void* data)
{

}

void AnkerNetBase::p2pReadData(void* data)
{

}

void AnkerNetBase::initBreakPointCtrlData(void* data)
{

}

void AnkerNetBase::setBreakPointSn(const std::string& sn)
{

}

void AnkerNetBase::setBeakPointHttpData(const std::map<int, std::string>& data)
{

}

void AnkerNetBase::startBreakPointWork(const std::string& usrName, const std::string& fileName)
{

}

void AnkerNetBase::setDescriptor(int descriptor)
{

}

void AnkerNetBase::setSelectDisplayCmdStr(const std::string& cmdStr)
{

}

int AnkerNetBase::getDescriptor() const
{
	return 0;
}

std::string AnkerNetBase::getSelectDisplayCmdStr() const
{
	return std::string();
}

int AnkerNetBase::initWrite()
{
	return 0;
}

int AnkerNetBase::initP2P(const std::string& didStr, const std::string& initStr, int mode)
{
	return 0;
}

int AnkerNetBase::initP2P(void* data)
{
	return 0;
}

void AnkerNetBase::setSn(const std::string& sn)
{

}

std::string AnkerNetBase::getSn() const
{
	return std::string();
}

void AnkerNetBase::setDsk(const std::string& dsk)
{

}

std::string AnkerNetBase::getDsk() const
{
	return std::string();
}

void AnkerNetBase::setHttpsSelectData(std::map<int, std::string> data)
{

}

void AnkerNetBase::startWorkThreadRead(const std::string& fileName)
{

}

void AnkerNetBase::startWorkThreadWrite(const std::string& usrName, const std::string& fileName)
{

}

void AnkerNetBase::startWriteWork(const std::string& usrName, const std::string& fullpathName, const std::string& fileName)
{

}

boost::signals2::connection AnkerNetBase::transferFileConnect(const boost::signals2::slot<void(int)>& slot)
{
	return boost::signals2::connection();
}


void AnkerNetBase::setP2pWDResReadCallBack(StrMsgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWDResSendCallBack(StrMsgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWDResErrMsgCallBack(StrMsgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWDTransferFileValueCallBack(IndexCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pRDFinishedCallBack(StrMsgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pBPCWriteResCallBack(ResStrCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pBPTransferFileValueCallBack(IndexCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pBPCThreadStatusChangedCallBack(IndexCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pBPCIsNeedLevelCallBack(ResCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWRFileResCallBack(ResStrCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWRErrMsgCallBack(StrMsgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWRTransferFileValueCallBack(IndexCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2pWRTransferFileFinishedCallBack(ResIndexCallBackFunc callBackFunc)
{

}

void AnkerNetBase::onP2pWDResReadSlot(const std::string& data)
{

}

void AnkerNetBase::onP2pWDResSendSlot(const std::string& data)
{

}

void AnkerNetBase::onP2pWDResErrMsgSlot(const std::string& data)
{

}

void AnkerNetBase::onP2pWDTransferFileValueSlot(const int& index)
{

}

void AnkerNetBase::onP2pRDFinishedSlot(const std::string& data)
{

}

void AnkerNetBase::onP2pBPCWriteResSlot(const std::string& data, const std::string& msg)
{

}

void AnkerNetBase::onP2pBPTransferFileValueSlot(const int& index)
{

}

void AnkerNetBase::onP2pBPCThreadStatusChangedSlot(const int& index)
{

}

void AnkerNetBase::onP2pBPCIsNeedLevelSlot(const std::string& data, bool isTrue)
{

}

void AnkerNetBase::onP2pWRFileResSlot(const std::string& data, const std::string& msg)
{

}

void AnkerNetBase::onP2pWRErrMsgSlot(const std::string& data)
{

}

void AnkerNetBase::onP2pWRTransferFileValueSlot(const int& index)
{

}

void AnkerNetBase::onP2pWRTransferFileFinishedSlot(const std::string& data, int index)
{

}

void AnkerNetBase::openVideoStream(void* pData)
{

}
bool AnkerNetBase::closeVideoStream(int reason )
{
	return true;
}
void AnkerNetBase::setCameraLightState(bool onOff)
{

}
void AnkerNetBase::setVideoMode(int mode)
{

}

std::string AnkerNetBase::getVideoCtrlSn()
{
	return "";
}

int AnkerNetBase::getVideoCtrlState()
{
	//return STATE_NONE;
	return 0;
}

void AnkerNetBase::setVideoFrameDataReadyCallBack(SnImgCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setVideoPlayStopStateChangeCallBack(PlayStopStateCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2PVideoStreamSessionInitedCallBack(SnCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2PVideoStreamSessionClosingCallBack(SnCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2PVideoStreamSessionClosedCallBack(SnCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setP2PVideoStreamCtrlAbnormalCallBack(SnCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setVideoStreamStateCallBack(SnStateCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setCameraLightStateCallBack(SnStateCallBackFunc callBackFunc)
{

}

void AnkerNetBase::setVideoModeCallBack(SnStateCallBackFunc callBackFunc)
{

}

void AnkerNetBase::unsetAllVideoCallBacks()
{

}

boost::signals2::connection AnkerNetBase::videoStopSussConnect(const boost::signals2::slot<void(const std::string& )>& slot)
{
	return boost::signals2::connection();
}

void AnkerNetBase::setLogOutputCallBack(LogOutputCallBackFunc callBackFunc) {

}


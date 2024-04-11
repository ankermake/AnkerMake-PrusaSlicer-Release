#ifndef ANKER_NET_MODULE_H
#define ANKER_NET_MODULE_H

#include "../AnkerNet.h"

class AnkerNetModule:public AnkerNet
{
public:
	AnkerNetModule() {}
	~AnkerNetModule() {}
	int AsyPost(std::string url,
		std::string data,
		std::string token,
		void* userData,
		HttpCallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0);

	int AsyGet(std::string url,
		std::string data,
		std::string token,
		void* userData,
		HttpCallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0);

	int AsyUpload(std::string url,
		std::string localFilePath,
		HttpCallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0);

	int AsyDownLoad(std::string url,
		std::string localFilePath,
		HttpCallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0);

	//mqtt api
	void setMqttCfg(ASYN_MQTT_DATA data);
	bool getMqttState();
	bool disConnectMqtt();
	int startConnectMqtt();
	int sendMsgToMqtt(const std::string& topic, const std::string& msg);
	void setSubscribeMqtt(std::list<std::string> topicList);
	void setSubscribeMqtt(const std::string& topic);

	//P2P api
	void p2pWriteData(const WriteData& data);
	//signal p2pWriteData
	AnkerSignal<const std::string&> p2pResultReadySig;
	AnkerSignal<const std::string&> p2pResultSendSig;
	AnkerSignal<const std::string&> p2pErrorMessageSig;
	AnkerSignal<const int&> p2pTransferFileValueSig;

	void p2pReadData(const ReadData& data);
	//signal p2pReadData
	AnkerSignal<const std::string&> p2pFinishedSig;

	void initBreakPointCtrlData(const BreadkPointCtrlData& data);
	void setBreakPointSn(const std::string& sn);
	void setBeakPointHttpData(const std::map<SELECT_CMD_TYPE, std::string>& data);
	void startBreakPointWork(const std::string& usrName, const std::string& fileName);

	//signal p2pBreakPointCtrl
	AnkerSignal<const std::string&, const std::string&> p2pWriteFileResultSig;
	AnkerSignal<const int&> p2pThreadStatusChangedSig;
	AnkerSignal<const std::string&, const bool&> p2pIsNeedLevelSig;

	//p2pReadWriteData
	void setDescriptor(int descriptor);
	void setSelectDisplayCmdStr(const std::string& cmdStr);
	int getDescriptor() const;
	std::string getSelectDisplayCmdStr() const;

	int initWrite();
	int initP2P(const std::string& didStr, const std::string& initStr, int mode);
	int initP2P(ReadWriteData data);

	void setSn(const std::string& sn);
	std::string getSn() const;
	void setDsk(const std::string& dsk);
	std::string getDsk() const;
	void setHttpsSelectData(std::map<SELECT_CMD_TYPE, std::string> data);

	void startWorkThreadRead(const std::string& fileName);
	void startWorkThreadWrite(const std::string& usrName, const std::string& fileName);
	void startWriteWork(const std::string& usrName, const std::string& fileName);

	//signal p2pReadWriteData
	AnkerSignal <const std::string&, int> p2pTransferFileFinishedSig;
protected:

private:

	AsynMqttClient m_mqttClient;
	P2PBreakPointCtrl m_p2pBreakPointCtrl;
	P2PReadWriteData m_p2pReadWriteData;
};

#endif // !ANKER_NET_MODULE_H

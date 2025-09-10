#ifndef _ANKER_PLUGIN_
#define _ANKER_PLUGIN_

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#define DLL_EXPORT  __attribute__((visibility("default")))
#else
#define  DLL_EXPORT _declspec(dllexport)
#endif

typedef size_t(*CallBackFunc)(char* dest, size_t size, size_t nmemb, void* userp);
#define ProgressInfoCallback std::function<void (double dltotal, double dlnow, double ultotal, double ulnow)>
typedef int(*UploadProgressInfoCallBack)(void* userdata, long long dltotal, long long dlnow, long long ultotal, long long ulnow);

struct DLL_EXPORT RequestRes
{
	// curl perform result.
	// 0 means success
	int curlCode = -1;

	// Http Status Code
	// Relevant details can be found on Baidu HTTP status code
	// 200 means OK
	int httpCode = 0;

	// Http Request Content
	std::vector<char> httpContent;
};

enum DLL_EXPORT RequestType
{
	Request_Get = 0,
	Request_Post = 1,
	Request_Put = 2,
	Request_Delete = 3,
};

class DLL_EXPORT AnkerPlugin
{
public:

	virtual void setHeaderList(const std::vector<std::string>& headerList) = 0;

	virtual void setPluginParameter(const std::string& token,
									 const std::string& envirment,
									 const std::string& userid,
									 const std::string& openudid) = 0;

	virtual void getSentryDsn(std::string &sentryDsn) = 0;

	virtual void encryptText(std::string strContent, int& strLength, std::string &outStr) = 0;

	virtual int createKeyBlock(uint8_t** data, uint32_t& len) = 0;

	virtual int reportBuryPoint(const std::string& eventName, const std::map<std::string, std::string>& eventMap, bool isBlock = false) = 0;

	//http api
	virtual int AsyPost(const std::string& url,
		const std::string& data,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) = 0;

	virtual int AsyGet(const std::string& url,
		const std::string& data,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) = 0;

	virtual int AsyPut(const std::string& url,
		const std::string& data,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) = 0;

	virtual int AsyDelete(const std::string& url,
		const std::string& data,
		const std::vector<std::string>& headrList,
		void* userData,
		CallBackFunc callbackfunc,
		bool isBlock = false,
		unsigned int nTimeOut = 0) = 0;


	virtual int AsyUpload(const std::string& url,
						  const std::string& localFilePath,
						  const std::vector<std::string>& headrList,
						  CallBackFunc callbackfunc,
						  ProgressInfoCallback progressCallbackFunc,
						  bool isBlock = false,
						  unsigned int nTimeOut = 0) = 0;

	virtual int PostUploadFile(const std::string& url,
								const std::string& data,
								const std::string& localFilePath,
								const std::vector<std::string>& headrList,
								void* userData,
								CallBackFunc callbackfunc,
								UploadProgressInfoCallBack progressCallbackFunc,
								bool isBlock = false,
								unsigned int nTimeOut = 0) = 0;

	virtual int PostUploadFeedBack(const std::string& url,
									const std::string& data,
									const std::string& localFilePath,
									const std::vector<std::string>& headrList,
									void* userData,
									CallBackFunc callbackfunc,
									UploadProgressInfoCallBack progressCallbackFunc,
									bool isBlock = false,
									unsigned int nTimeOut = 0) = 0;

	virtual int AsyDownLoad(const std::string& url,
							const std::string& localFilePath,
							const std::vector<std::string>& headrList,
							void* userData,
							CallBackFunc callbackfunc,
							ProgressInfoCallback progressCallbackFunc,
							bool isBlock = false,
							unsigned int nTimeOut = 0) = 0;

protected:
private:
	
};

#endif
#ifndef slic3r_HttpTool_hpp_
#define slic3r_HttpTool_hpp_
#include "AnkerNetDefines.h"

//typedef void(*CallBack_DownloadProgress)(long curSize,long totalSize);
using CallBack_DownloadProgress = std::function<void(long curSize, long totalSize)>;

using namespace AnkerNet;
class HttpTool
{
public:
	enum class RetStatus
	{
		Success,
		Canceled,
		NetworkError,
		Failed,

		JsonParseFailed,
		JsonFormatFailed,
		CodeError,
		JsonContentError,
		ComputeMd5Failed,
		Md5NotEqual,
	};

	static void SetDownloadCancel(bool cancel = true);

	static RetStatus DownloadFileSync(std::string url,
		std::string filename, 
		CallBack_DownloadProgress callback,
		int timeOut);
	static HttpRequestResult HttpRequestSync(const std::string& url, const std::string& data, const HttpRequestType type);

	static RetStatus DownloadAnkerNetZipSync(CallBack_DownloadProgress callback, std::string filename);
};


#endif

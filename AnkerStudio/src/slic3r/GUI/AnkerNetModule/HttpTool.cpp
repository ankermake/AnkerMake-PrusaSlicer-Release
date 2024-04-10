#include "HttpTool.h"

#include "curl/curl.h"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/Utils/DataMangerUi.hpp"
#include "slic3r/Utils/JsonHelp.hpp"
#include "slic3r/Utils/wxFileTool.hpp"
#include <slic3r/GUI/AnkerConfig.hpp>
#ifdef _WIN32
#include <codecvt>
#include <xlocbuf>
#include <xstring>
#endif


typedef int(*CallBack_DownloadProgressInterval)(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);


class HttpToolBase
{
public:
	static bool IsNetworkError(int result)
	{
		if (result == CURLE_COULDNT_RESOLVE_PROXY ||
			result == CURLE_COULDNT_RESOLVE_HOST ||
			result == CURLE_COULDNT_CONNECT ||
			result == CURLE_OPERATION_TIMEDOUT) {
			return true;
		}
		return false;
	}

	static std::vector<std::string> GetHeaderList()
	{
		wxString languageCode = Slic3r::GUI::wxGetApp().current_language_code_safe();
		int index = languageCode.find('_');

		auto sysInfo = DatamangerUi::GetSysInfo();

		std::vector<std::string> headerList;
		headerList.push_back(std::string("App_name: ") + "AnkerMake Studio");
		headerList.push_back(std::string("Model_type: ") + "PC");
		headerList.push_back(std::string("App_version: ") + "V" + SLIC3R_VERSION);
		headerList.push_back(std::string("Country: ") + languageCode.substr(index + 1, languageCode.Length() - index).ToUTF8().data());
		headerList.push_back(std::string("Language: ") + languageCode.substr(0, index).ToUTF8().data());
		headerList.push_back(std::string("Expect:"));
		headerList.push_back(std::string("Openudid: ") + sysInfo.m_machineid);	// client id
		headerList.push_back(std::string("Os_version: ") + sysInfo.m_os_version);

#ifdef _WIN32
		headerList.push_back(std::string("Os_type: ") + "Windows");
#elif __APPLE__
		headerList.push_back(std::string("Os_type: ") + "MacOS");
#endif
		headerList.push_back(std::string("Content-Type:application/json;charset=UTF-8"));
		return headerList;
	}

	static int HttpDownloadFile(const std::string& url, 
		const std::string& filename, 
		CallBack_DownloadProgress callback,
		int &http_code,
		int timeOut)
	{
		CURL* curl = nullptr;
		CURLcode res = CURLE_OK;

		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res != CURLE_OK) {
			ANKER_LOG_ERROR << "curl_global_init() failed: " + std::to_string(res);
			return res;
		}

		curl = curl_easy_init();
		if (!curl)
		{
			ANKER_LOG_ERROR << "curl_easy_init() failed ";
			return -1;
		}

		FILE* fp = nullptr;

#ifdef _WIN32
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto name = converter.from_bytes(filename);
		auto mode = converter.from_bytes("wb");
		_wfopen_s(&fp, name.c_str(), mode.c_str());
#else
		fp = fopen( filename.c_str(), "wb");
#endif

		if (!fp)
			return -1;

		auto headerList = GetHeaderList();

		curl_slist* pHeaders = nullptr;
		if (headerList.size() > 0)
		{
			for (auto iter = headerList.begin(); iter != headerList.end(); iter++)
				pHeaders = curl_slist_append(pHeaders, (*iter).c_str());
		}
		else
			pHeaders = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pHeaders);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeOut);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, 0);

		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, reinterpret_cast<void*>(&callback));
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, reinterpret_cast<void*>(&DownloadProgress));

		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		if (res != CURLE_OK) {
			if (res == CURLE_ABORTED_BY_CALLBACK) {
				ANKER_LOG_INFO << "download canceled";
			}
			else {
				ANKER_LOG_ERROR << "curl_easy_perform failed: " << res << ", http_code: " << http_code;
			}
		}
		fclose(fp);
		curl_slist_free_all(pHeaders);
		curl_easy_cleanup(curl);

		return res;
	}

	static int HttpRequestCallback(char* dest, size_t size, size_t nmemb, void* userp)
		{
			if (!userp) {
				ANKER_LOG_ERROR << "userp is null";
				return -1;
			}

			auto content = static_cast<std::vector<char>*>(userp);
			int len = size * nmemb;
			content->insert(content->end(), dest, dest + len);
			return len;
		}

	static HttpRequestResult HttpRequest(const std::string& url, const std::string& data,
			const std::vector<std::string>& headrList, const HttpRequestType type, unsigned nTimeOut)
		{
			HttpRequestResult result;
			result.curlCode = -1;
			result.httpCode = -1;

			CURL* curl = nullptr;
			CURLcode res = CURLE_OK;

			res = curl_global_init(CURL_GLOBAL_DEFAULT);
			if (res != CURLE_OK) {
				ANKER_LOG_ERROR << "curl_global_init() failed: " + std::to_string(res);
				return result;
			}

			curl = curl_easy_init();
			if (!curl)
			{
				ANKER_LOG_ERROR << "curl_easy_init() failed ";
				return result;
			}

			curl_slist* pHeaders = nullptr;
			if (headrList.size() > 0) {
				for (auto iter = headrList.begin(); iter != headrList.end(); iter++) {
					pHeaders = curl_slist_append(pHeaders, (*iter).c_str());
				}
			}
			else {
				pHeaders = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
			}

			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pHeaders);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeOut);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

			if (type == HttpRequest_Post)
			{
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
				curl_easy_setopt(curl, CURLOPT_POST, 1);
			}
			else
			{
				curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
				if (type == HttpRequest_Get)
				{
					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				else if (type == HttpRequest_Delete)
				{
					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "Delete");
				}
				else if (type == HttpRequest_Put)
				{
					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "Put");
				}
			}


			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpRequestCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&result.httpContent));
			result.curlCode = curl_easy_perform(curl);

			int http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			result.httpCode = http_code;
			if (pHeaders) {
				curl_slist_free_all(pHeaders);
			}
			curl_easy_cleanup(curl);

			if (result.httpContent.size() > 0)
			{
				result.httpContent.push_back(0);
			}
			else
			{
				//when http error or curl error ,keep content valid
				result.httpContent.push_back(' ');
				result.httpContent.push_back(0);
			}
			return result;
		}

	static inline std::atomic_bool m_canceled{ false };

private:
	static int DownloadProgress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
	{
		if (m_canceled) {
			ANKER_LOG_INFO << "download progress canceled";
			return CURLE_ABORTED_BY_CALLBACK;
		}

		CallBack_DownloadProgress callback = *(CallBack_DownloadProgress*)clientp;
		if (callback && dltotal > 0) {
			callback(dlnow, dltotal);
		}

		return 0;
	}
};

void HttpTool::SetDownloadCancel(bool cancel)
{
	HttpToolBase::m_canceled = cancel;
}

HttpTool::RetStatus HttpTool::DownloadFileSync(std::string url, 
	std::string filename, 
	CallBack_DownloadProgress callback,
	int timeOut)
{
	RetStatus downRet = RetStatus::Failed;
	//create tmp file
	auto now1 = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now1);
	std::tm* now = std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(now, "%Y-%m-%d-%H-%M-%S");
	std::string currenTime = oss.str();
	auto tmpPath = filename + "." + currenTime + ".tmp";

	int http_code = -1;
	auto ret = HttpToolBase::HttpDownloadFile(url, tmpPath, callback, http_code, timeOut);
	if (ret == CURLE_OK && http_code == 200)
	{
		ANKER_LOG_ERROR << "download success,url = " << url;
		//download success
		if (Slic3r::Utils::wxFileTool::wxCopyFile(wxString::FromUTF8( tmpPath), wxString::FromUTF8(filename)) == true)
		{
			ANKER_LOG_INFO << "Copy File success";
			downRet = RetStatus::Success;
		}
		else
		{
			ANKER_LOG_ERROR << "Copy File failed";
		}
	}
	else if (ret == CURLE_ABORTED_BY_CALLBACK) {
		downRet = RetStatus::Canceled;
	}
	else
	{
		ANKER_LOG_ERROR << "download failed, " << "ret: " << ret << 
			", http_code: "<< http_code << ", url: " << url;
	}
	wxRemoveFile(wxString::FromUTF8(tmpPath));
	
	return downRet;
}

HttpTool::RetStatus HttpTool::DownloadAnkerNetZipSync(CallBack_DownloadProgress callback, std::string filename)
{
	HttpTool::SetDownloadCancel(false);

	const std::string url = AnkerConfig::GetAnkerNetUrls().at(0);

	ANKER_LOG_INFO << "start to get download url for " << url;
	auto httpResult = HttpTool::HttpRequestSync(url, "", HttpRequest_Post);
	bool valid = httpResult.httpCode == HTTP_CODE_OK && httpResult.curlCode == CURL_CODE_OK;
	if (!valid) {
		ANKER_LOG_ERROR << "http error , http_code = " << httpResult.httpCode << ",curl_code = " 
			<< httpResult.curlCode << ",content = " << httpResult.httpContent.data();
		//if (HttpToolBase::IsNetworkError(httpResult.curlCode)) {
		//	return RetStatus::NetworkError;
		//}
		return RetStatus::NetworkError;
	}

	std::string content;
	if (httpResult.httpContent.empty() == false)
		content = httpResult.httpContent.data();

	json_error_t error;
	json_t* root = json_loads(content.c_str(), 0, nullptr);
	if (root == nullptr) {
		ANKER_LOG_ERROR << "Error parsing JSON: " << error.text << ", content: " << content;
		return RetStatus::JsonParseFailed;
	}

	//top level
	auto jsonCode = json_object_get(root, "code");
	auto jsonMsg = json_object_get(root, "msg");
	auto jsonData = json_object_get(root, "data");
	if (jsonCode == nullptr || jsonMsg == nullptr || jsonData == nullptr)
	{
		ANKER_LOG_ERROR << "json header format error " << error.text << ", content: " << content;
		return RetStatus::JsonFormatFailed;
	}
	auto code = json_integer_value(jsonCode);
	auto msg = json_string_value(jsonMsg);
	if (code != 0)
	{
		ANKER_LOG_ERROR << "api request error. code = " << code << ",msg = " << msg 
			<< ", content: " << content;
		return RetStatus::CodeError;
	}

	//data level
#ifdef _WIN32
	int timeOut = 3 * 60;
	auto jsonUrl = json_object_get(jsonData, "download_link_win");
	auto jsonMd5 = json_object_get(jsonData, "md5_win");
#else
	int timeOut = 8 * 60;
	auto jsonUrl = json_object_get(jsonData, "download_link_mac");
	auto jsonMd5 = json_object_get(jsonData, "md5_mac");
#endif

	if (jsonUrl == nullptr ||  jsonData == nullptr)
	{
		ANKER_LOG_ERROR << "json content format error, content: " << content;
		return RetStatus::JsonContentError;
	}
	std::string downUrl = json_string_value(jsonUrl);
	std::string downMd5 = json_string_value(jsonMd5);

	ANKER_LOG_INFO << "Start download file,filename = " << filename;
	auto downRet = HttpTool::DownloadFileSync(downUrl, filename, callback, timeOut);
	if (downRet != RetStatus::Success)
	{
		ANKER_LOG_ERROR << "Download file Failed: " << (int)downRet << ", filename = " << filename;
		return downRet;
	}
	wxString newMd5;
	if (Slic3r::Utils::wxFileTool::CalcFileMD5(wxString::FromUTF8(filename), newMd5) == false)
	{
		ANKER_LOG_ERROR << "Calc MD5 Failed.filename = " << filename ;
		return RetStatus::ComputeMd5Failed;
	}
	if (downMd5 != newMd5)
	{
		ANKER_LOG_ERROR << "Check MD5 Failed. cms md5: " << downMd5 << ", download file md5: " << newMd5;
		return RetStatus::Md5NotEqual;
	}

	ANKER_LOG_INFO << "Download file success";
	return RetStatus::Success;
}

HttpRequestResult HttpTool::HttpRequestSync(const std::string& url, const std::string& data, const HttpRequestType type)
{
	HttpRequestResult result;

	std::vector<std::string> headerList = HttpToolBase::GetHeaderList();
	auto sysInfo = DatamangerUi::GetSysInfo();	
	wxString languageCode = Slic3r::GUI::wxGetApp().current_language_code_safe();
	int index = languageCode.find('_');
	auto langSet = std::string("Language: ") + languageCode.substr(0, index).ToUTF8().data();
	headerList.push_back(langSet);

	result = HttpToolBase::HttpRequest(url, data, headerList, HttpRequest_Post, 15);

	return result;
}

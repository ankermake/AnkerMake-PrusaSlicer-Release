#ifndef slic3r_AnkerNetModuleManager_hpp_
#define slic3r_AnkerNetModuleManager_hpp_
#include "AnkerNetBase.h" 
#include <boost/dll.hpp>
#include "HttpTool.h"


class AnkerNetDownloadDialog;
class AnkerNetModuleManager
{
public:
	enum class LoadRet
	{
		Success,
		VersionNotMatch,
		LoadFailed,
		FileNotFound
	};

	enum class DownloadResult
	{
		Success = 0,
		Canceled = -1,
		Network_Error = -2,
		
		Unzip_Failed = -10,
		FileCopy_Failed = -11,
		Load_Failed = -12,
		UnzipLoad_Failed = -13,

		JsonParseFailed = -20,
		JsonFormatFailed = -21,
		CodeError = -22,
		JsonContentError = -23,
		ComputeMd5Failed = -24,
		Md5NotEqual = -25,

		Download_Failed = -30,
		MostNeed_Failed = -31
	};

	void ReleaseLibrary();

	bool LoadLibrary(wxWindow* pWindow = nullptr, bool silence = false);

private:
	void SetIsDownLoaded();
	bool GetIsDownLoaded();
	DownloadResult UnzipCopyLoad();
	LoadRet LoadLibraryDefault();
	bool GetPluginData(const boost::filesystem::path& libPath, AnkerNet::AnkerNetBase** netBase, int& netVersion);
	bool MostInNeed();

	void StartDownloadThread();
	void DownloadThread(AnkerNetDownloadDialog* dialog);
	void BuryDownloadResult(DownloadResult reason);
	DownloadResult HttpStatus2Reason(HttpTool::RetStatus httpStatus);
private:
	AnkerNetDownloadDialog* m_dialog = nullptr;
	boost::dll::shared_library m_netLibHandle;
};
#endif

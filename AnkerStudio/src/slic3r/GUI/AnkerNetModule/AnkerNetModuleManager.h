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
		Success,
		Canceled,
		Network_Error,
		
		Unzip_Failed,
		FileCopy_Failed,
		Load_Failed,
		UnzipLoad_Failed,

		JsonParseFailed,
		JsonFormatFailed,
		CodeError,
		JsonContentError,
		ComputeMd5Failed,
		Md5NotEqual,

		Download_Failed,
		MostNeed_Failed
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

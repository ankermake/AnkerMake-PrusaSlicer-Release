#include "AnkerNetModuleManager.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include "DeviceObjectBase.h"
#include "AnkerNetDefines.h"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/FileArchiveDialog.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "slic3r/GUI/GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include <wx/wx.h>
#include <wx/sizer.h>
#include <slic3r/GUI/AnkerNetModule/AnkerNetDownloadDialog.h>
#include <AnkerComFunction.hpp>
#include "../../Utils/wxFileTool.hpp"
#include <slic3r/GUI/AnkerConfig.hpp>


#define OnlineAnkerNetDir "OnlineAnkerNet"
#define CurDir "CurDir"
#define CurrentDir "Current"
#define AnkerNetZipFile "AnkerNet.zip"
#ifdef _WIN32
#define AnkerNetFile "AnkerNet.dll"
const std::string NetLibPatternStart = "AnkerNet_";
const std::string NetLibPatternEnd = "dll";
#else
#define AnkerNetFile "libAnkerNet.dylib"
const std::string NetLibPatternStart = "libAnkerNet_";
const std::string NetLibPatternEnd = "dylib";
#endif
#define netplugin_download "netplugin_download"

using namespace AnkerNet;
using bfpath = boost::filesystem::path;

class AnkerNetModuleHelp
{
public:
	static std::string PathString(const bfpath& pathi)
	{
		return (pathi).string();
	}

	static std::string PathCombine(const std::string& a, const std::string& b)
	{
		boost::filesystem::path path1(a);
		boost::filesystem::path path2(b);
		boost::filesystem::path combined = path1 / path2;
		return combined.string();
	}

	static bool FileExist(const bfpath& filePath)
	{
		return boost::filesystem::exists(filePath);
	}

	static bool DirExist(const bfpath& dir)
	{
		namespace fs = boost::filesystem;
		if (fs::exists(dir) && fs::is_directory(dir)) {
			return true;
		}
		return false;
	}

	static bool RemoveFilesInDir(const bfpath& dirPath)
	{
		namespace fs = boost::filesystem;

		if (!fs::is_directory(dirPath)) {
			return false;
		}

		try {
			if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
				for (const auto& entry : fs::directory_iterator(dirPath)) {
					fs::remove_all(entry.path());
				}
			}
		}
		catch (const fs::filesystem_error& e) {
			ANKER_LOG_ERROR << "error: " << e.what();
			return false;
		}
		return true;
	}

	static bool FileCopy(const bfpath& srcFile, const bfpath& destFile)
	{
		try {
			if (!AnkerNetModuleHelp::FileExist(srcFile)) {
				ANKER_LOG_INFO << "src file not exist, " << srcFile;
				return false;
			}

			if (boost::filesystem::exists(destFile)) {
				ANKER_LOG_INFO << "delete " << destFile;
				wxString md5value;
				Slic3r::Utils::wxFileTool::CalcFileMD5(destFile.string(), md5value);
				
				bool removeRet = boost::filesystem::remove(destFile);
				ANKER_LOG_INFO << "destFile md5value: " << md5value.ToStdString() << ", ret: " << removeRet;
			}

			{
				wxString md5value;
				Slic3r::Utils::wxFileTool::CalcFileMD5(srcFile.string(), md5value);
				ANKER_LOG_INFO << "srcFile md5value: " << md5value.ToStdString();
			}

			bool copyRet = boost::filesystem::copy_file(srcFile, destFile,
				boost::filesystem::copy_option::overwrite_if_exists);
			ANKER_LOG_INFO << "copy " << srcFile << " to " << destFile << " success, ret: " << copyRet;
		}
		catch (const boost::filesystem::filesystem_error& ex) {
			ANKER_LOG_ERROR << "file system error: " << ex.what();
			return false;
		}
		catch (const std::system_error& e) {
			ANKER_LOG_ERROR << "system error: " << e.what();
			return false;
		}

		return true;
	}

	static bfpath GetAnkerNetDir()
	{
		return (boost::filesystem::path(Slic3r::data_dir()) / OnlineAnkerNetDir / SLIC3R_VERSION);
	}

	static bfpath GetExeDir()
	{
		return boost::dll::program_location().parent_path();
	}

	static bfpath GetDefaultDir()
	{
		bfpath ret;
#if defined(_DEBUG) || !defined(NDEBUG)
		ret = GetExeDir();
#else
		ret = boost::filesystem::path(Slic3r::data_dir()) / OnlineAnkerNetDir / CurrentDir;
		// create default dir
		if (!DirExist(ret)) {
			CheckCreateDir(ret);
		}
#endif
		ANKER_LOG_DEBUG << "default dir: " << ret;
		return ret;
	}
	
	static std::string GetAnkerNetFile()
	{
		std::string fileName;
		std::string suffix;
#if defined(_DEBUG) || !defined(NDEBUG)
		return AnkerNetFile;
#else
#ifdef _WIN32
		fileName = "AnkerNet_";
		suffix = ".dll";
#else
		fileName = "libAnkerNet_";
		suffix = ".dylib";
#endif
#endif

		fileName = fileName + std::to_string(MappingVersion) + suffix;
		ANKER_LOG_INFO << "anker net file name: " << fileName;
		return fileName;
	}

	static bfpath GetDefaultFile()
	{
		auto ret = GetDefaultDir() / GetAnkerNetFile();
		return ret;
	}

	static bfpath GetCacheDir()
	{
		return (bfpath(Slic3r::data_dir()) / OnlineAnkerNetDir / SLIC3R_VERSION / CurDir);
	}

	static bfpath GetCacheFile()
	{
		return (bfpath(Slic3r::data_dir()) / OnlineAnkerNetDir / SLIC3R_VERSION / CurDir / GetAnkerNetFile());
	}

	static bfpath GetCacheZipFile()
	{
		return (bfpath(Slic3r::data_dir()) / OnlineAnkerNetDir / SLIC3R_VERSION / AnkerNetZipFile);
	}

	static bfpath GetNeedFile()
	{
		bfpath ret;
		bfpath executablePath = boost::dll::program_location().parent_path();
#ifdef _WIN32
		ret = (executablePath / "resources" / "plugin" / AnkerNetFile);
#else
		ret = executablePath.parent_path() / "Frameworks" / AnkerNetFile;
#endif
		ANKER_LOG_INFO << "need file: " << ret;
		return ret;
	}

	static bool CheckCreateDir(bfpath dir)
	{
		try {
			if (!boost::filesystem::exists(dir)) {
				if (!boost::filesystem::create_directories(dir)) {
					ANKER_LOG_ERROR << "mkdir " << dir.string() << " failed";
					return false;
				}
			}
		}
		catch (const boost::filesystem::filesystem_error& e) {
			ANKER_LOG_ERROR << "error occurred: " << e.what();
			return false;
		}

		return true;
	}	

	static int GetVersionFromFileName(bfpath filePath)
	{
#if defined(_DEBUG) || !defined(NDEBUG)
		return MappingVersion;
#endif

		int version = -1;
		std::string fileName = filePath.filename().string();

		size_t underscore_pos = fileName.find('_');
		if (underscore_pos == std::string::npos) {
			ANKER_LOG_ERROR << "find _ error: " << fileName << ", path: " << filePath;
			return version;
		}

		size_t dot_pos = fileName.find('.', underscore_pos);
		if (dot_pos == std::string::npos) {
			ANKER_LOG_ERROR << "find . error: " << fileName << ", path: " << filePath;
			return version;
		}

		std::string numberStr = fileName.substr(underscore_pos + 1, dot_pos - underscore_pos - 1);

		try {
			version = std::stoi(numberStr);
		}
		catch (const std::invalid_argument& e) {
			ANKER_LOG_ERROR << "invalid argument: " << e.what() << ", " << fileName << ", path: " << filePath;
		}
		catch (const std::out_of_range& e) {
			ANKER_LOG_ERROR << "out of range: " << e.what() << ", " << fileName << ", path: " << filePath;
		}
		catch (...) {
			ANKER_LOG_ERROR << "other error, " << fileName << ", path: " << filePath;
		}

		ANKER_LOG_INFO << "version: " << version;
		return version;
	}

	static void DeleteDefaultFiles()
	{
		namespace fs = boost::filesystem;
		auto defaultDir = GetDefaultDir();

		if (!fs::exists(defaultDir) || !fs::is_directory(defaultDir)) {
			ANKER_LOG_ERROR << "does not exist or is not a directory: " << defaultDir;
			return;
		}

		try {
			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_iter(defaultDir); dir_iter != end_iter; ++dir_iter) {
				if (fs::is_regular_file(dir_iter->status())) {
					std::string filename = dir_iter->path().filename().string();
					if (filename.find(NetLibPatternStart) == 0 &&
						filename.rfind(NetLibPatternEnd) == (filename.length() - NetLibPatternEnd.length())) {
						ANKER_LOG_INFO << "delete " << dir_iter->path();
						fs::remove(dir_iter->path());
					}
				}
			}
		}
		catch (const boost::filesystem::filesystem_error& ex) {
			ANKER_LOG_ERROR << "file system error: " << ex.what();
		}
		catch (const std::system_error& e) {
			ANKER_LOG_ERROR << "system error: " << e.what();
		}
	}	

	static bool HaveNetFileInDir(bfpath& checkDir)
	{
		namespace fs = boost::filesystem;		
		if (!fs::exists(checkDir) || !fs::is_directory(checkDir)) {
			ANKER_LOG_ERROR << "does not exist or is not a directory: " << checkDir;
			return false;
		}

		try {
			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_iter(checkDir); dir_iter != end_iter; ++dir_iter) {
				if (fs::is_regular_file(dir_iter->status())) {
					std::string filename = dir_iter->path().filename().string();
					if (filename.find(NetLibPatternStart) == 0 &&
						filename.rfind(NetLibPatternEnd) == (filename.length() - NetLibPatternEnd.length())) {
						ANKER_LOG_INFO << "find anker net file: " << filename;
						return true;
					}
				}
			}
		}
		catch (const boost::filesystem::filesystem_error& ex) {
			ANKER_LOG_ERROR << "file system error: " << ex.what();
		}
		catch (const std::system_error& e) {
			ANKER_LOG_ERROR << "system error: " << e.what();
		}

		return false;
	}

	static bool UnZipNet(const bfpath& zipfile)
	{		
		if (!FileExist(zipfile)) {
			return false;
		}

		auto dir = GetCacheDir();
		ANKER_LOG_INFO << "unzip " << zipfile << " to " << dir;
		return UnZipToDir(zipfile.string().c_str(), dir.string().c_str());
	}

	static bool UnZipToDir(const char* zipfilename, const char* dirname) 
	{
		if (wxDirExists(wxString::FromUTF8(dirname)) == false)
		{
			Slic3r::Utils::wxFileTool::CreateDirRecursively(wxString::FromUTF8(dirname));
		}

		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		mz_bool status = mz_zip_reader_init_file(&zip_archive, zipfilename, 0);
		if (!status) {
			ANKER_LOG_ERROR << "Unable to open ZIP archive " << zipfilename;
			return false;
		}

		int file_count = (int)mz_zip_reader_get_num_files(&zip_archive);

		for (int i = 0; i < file_count; i++) {
			mz_zip_archive_file_stat file_stat;
			if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
				ANKER_LOG_ERROR << "Unable to stat file at index " << i << " in ZIP archive " << zipfilename;
				mz_zip_reader_end(&zip_archive);
				return false;
			}

			char outfilename[260];
			snprintf(outfilename, sizeof(outfilename), "%s/%s", dirname, file_stat.m_filename);

			if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
				Slic3r::Utils::wxFileTool::CreateDirRecursively(wxString::FromUTF8(outfilename));
			}
			else {
				Slic3r::Utils::wxFileTool::CreateFileDirRecursively(wxString::FromUTF8(outfilename));
				if (!mz_zip_reader_extract_to_file(&zip_archive, i, outfilename, 0)) {
					ANKER_LOG_ERROR << "Unable to extract file at index " << i << " in ZIP archive " << zipfilename;;
					mz_zip_reader_end(&zip_archive);
					return false;
				}
			}
		}

		mz_zip_reader_end(&zip_archive);
		return true;
	}
};


void AnkerNetModuleManager::ReleaseLibrary()
{
	if (m_netLibHandle.is_loaded()) {
		ANKER_LOG_INFO << "unload net dll";
		m_netLibHandle.unload();
	}
}

void AnkerNetModuleManager::SetIsDownLoaded()
{
	Slic3r::GUI::GUI_App* gui = dynamic_cast<Slic3r::GUI::GUI_App*>(Slic3r::GUI::GUI_App::GetInstance());
	if (!gui || !gui->app_config) {
		ANKER_LOG_INFO << "gui is null or app config is null";
		return;
	}
	gui->app_config->set("NetPlugin", "download_netplugin", "1");
}

bool AnkerNetModuleManager::GetIsDownLoaded()
{
	// read auto download config
	bool isUpdate = false;
	Slic3r::GUI::GUI_App* gui = dynamic_cast<Slic3r::GUI::GUI_App*>(Slic3r::GUI::GUI_App::GetInstance());
	if (!gui || !gui->app_config) {
		ANKER_LOG_INFO << "gui is null or app config is null";
		return false;
	}
	int intValue = 0;
	auto configDownload = gui->app_config->get("download_netplugin");
	try {
		intValue = std::stoi(configDownload);
	}
	catch (...) {
		intValue = 0;
	}
	isUpdate = intValue == 1 ? true : false;
	ANKER_LOG_INFO << "is update: " << isUpdate;
	return isUpdate;
}

AnkerNetModuleManager::DownloadResult AnkerNetModuleManager::UnzipCopyLoad()
{
	auto cacheFile = AnkerNetModuleHelp::GetCacheFile();
	if (!AnkerNetModuleHelp::FileExist(cacheFile)) {
		// unzip
		auto zipfile = AnkerNetModuleHelp::GetCacheZipFile();

		if (!AnkerNetModuleHelp::FileExist(zipfile)) {
			ANKER_LOG_INFO << "file not exist, " << zipfile;
			return DownloadResult::Unzip_Failed;
		}
		if (!AnkerNetModuleHelp::UnZipNet(zipfile)) {
			ANKER_LOG_ERROR << "unzip file failed, " << zipfile;
			return DownloadResult::Unzip_Failed;
		}
	}
	if (!AnkerNetModuleHelp::FileExist(cacheFile)) {
		ANKER_LOG_INFO << "cache file not exist, " << cacheFile;
		return DownloadResult::Unzip_Failed;
	}

	// copy unzip file to default dir
	auto defaultFile = AnkerNetModuleHelp::GetDefaultFile();
	ANKER_LOG_INFO << "try copy unzip file to default dir, delete default file first";
	AnkerNetModuleHelp::DeleteDefaultFiles();
	if (!AnkerNetModuleHelp::FileCopy(cacheFile, defaultFile)) {
		ANKER_LOG_INFO << "default file copy failed";
		return DownloadResult::FileCopy_Failed;
	}
	
	// delete the cache file
	auto curDir = AnkerNetModuleHelp::GetCacheDir();
	ANKER_LOG_INFO << "delete curdir: " << curDir;
	AnkerNetModuleHelp::RemoveFilesInDir(curDir);

	auto loadRet = LoadLibraryDefault();
	if (loadRet == LoadRet::Success) {
		return DownloadResult::Success;
	} else if (loadRet == LoadRet::LoadFailed) {
		return DownloadResult::Load_Failed;
	}

	return DownloadResult::UnzipLoad_Failed;
}

bool AnkerNetModuleManager::LoadLibrary(wxWindow* pWindow, bool silence)
{
	LoadRet loadRet = LoadRet::LoadFailed;
	AnkerNetDownloadDialog::Status dailogStatus = AnkerNetDownloadDialog::Status::InstallHint;

	// 1.try load
	loadRet = LoadLibraryDefault();
	if (loadRet == LoadRet::Success) {
		return true;
	}
	// 2.unzip copy and load
	if (UnzipCopyLoad() == DownloadResult::Success) {
		return true;
	}

	if (silence) {
		return false;
	}

	// 3.popup download progress
	ANKER_LOG_INFO << "load ret: " << (int)loadRet;
	if (loadRet == LoadRet::VersionNotMatch) {
		dailogStatus = AnkerNetDownloadDialog::Status::UpdateHint;
		ANKER_LOG_INFO << "set dialog status to: " << (int)dailogStatus;
	}
	
	wxSize dialogSize = AnkerSize(AnkerNetDownloadDialog::defualtWidth, AnkerNetDownloadDialog::defualtHeight);
	wxPoint dialogPos = wxDefaultPosition;
	wxSize parentSize;
	if (pWindow) {
		parentSize = pWindow->GetClientSize();
	}
	else {
		wxDisplay display;
		wxRect screenRect = display.GetGeometry();
		parentSize.SetWidth(screenRect.GetWidth());
		parentSize.SetHeight(screenRect.GetHeight());
	}
	dialogPos.x = (parentSize.GetWidth() - dialogSize.GetWidth()) / 2;
	dialogPos.y = (parentSize.GetHeight() - dialogSize.GetHeight()) / 2;
	ANKER_LOG_INFO << dialogPos.x << "," << dialogPos.y
		<< ", parent: " << parentSize.GetWidth() << ", " << parentSize.GetHeight();

	if (!m_dialog) {
		delete m_dialog;
		m_dialog = nullptr;
	}
	try {
		ANKER_LOG_INFO << "start to new download dialog";
		m_dialog = new AnkerNetDownloadDialog(pWindow, wxID_ANY, dialogPos, dialogSize);
	}
	catch (const std::exception& e) {
		ANKER_LOG_ERROR << "standard exception caught: " << e.what();
		return false;
	}
	catch (...) {
		ANKER_LOG_ERROR << "unknown exception has occurred!";
		return false;
	}
	if (!m_dialog) {
		ANKER_LOG_INFO << "new dialog failed";
		return false;
	}

	ANKER_LOG_INFO << "start to show dialog";
	auto dialogRet = m_dialog->ShowHint(dailogStatus,
		// install click
		[this]() {
			// start download thread
			StartDownloadThread();
		}
	);

	ANKER_LOG_INFO << "download dialog ret: " << (int)dialogRet;
	return dialogRet == (int)AnkerNetDownloadDialog::Result::Success;
}

AnkerNetModuleManager::LoadRet AnkerNetModuleManager::LoadLibraryDefault()
{
	if (DatamangerUi::GetInstance().pAnkerNet) {
		return LoadRet::Success;
	}

	auto filePath = AnkerNetModuleHelp::GetDefaultFile();

	if (!AnkerNetModuleHelp::FileExist(filePath)) {
		// search if have ankernet file
		auto defaultDir = AnkerNetModuleHelp::GetDefaultDir();
		if (AnkerNetModuleHelp::HaveNetFileInDir(defaultDir)) {
			return LoadRet::VersionNotMatch;
		}
		auto exeDir = AnkerNetModuleHelp::GetExeDir();
		if (AnkerNetModuleHelp::HaveNetFileInDir(exeDir)) {
			return LoadRet::VersionNotMatch;
		}
		return LoadRet::FileNotFound;
	}

	// get version from file name
	int fileVersion = AnkerNetModuleHelp::GetVersionFromFileName(filePath);
	if (fileVersion == -1 || fileVersion != MappingVersion) {
		ANKER_LOG_WARNING << "net plugin file version " << fileVersion << " != " <<
			" main net version " << MappingVersion;
		AnkerNetModuleHelp::DeleteDefaultFiles();
		return LoadRet::VersionNotMatch;
	}

	// get version from lib file
	int netVersion = -1;
	AnkerNetBase* netBase = nullptr;
	if (!GetPluginData(filePath, &netBase, netVersion)) {
		ANKER_LOG_WARNING << "get plugin data failed";
		AnkerNetModuleHelp::DeleteDefaultFiles();
		return LoadRet::LoadFailed;
	}

	// version check	
	if (netVersion != MappingVersion) {
		ANKER_LOG_WARNING << "net plugin version " << netVersion << " != " <<
			" main net version " << MappingVersion;
		AnkerNetModuleHelp::DeleteDefaultFiles();
		ReleaseLibrary();
		return LoadRet::VersionNotMatch;
	}

	DatamangerUi::GetInstance().pAnkerNet = netBase;
	ANKER_LOG_INFO << "net plugin load success";
	return LoadRet::Success;
}

bool AnkerNetModuleManager::GetPluginData(const boost::filesystem::path& libPath,
	AnkerNetBase** netBase, int& netVersion)
{
	wxString md5value;
	Slic3r::Utils::wxFileTool::CalcFileMD5(libPath.string(), md5value);

	ANKER_LOG_INFO << "libpath: " << libPath << ", md5: " << md5value;

	do {
		try {
			std::function<AnkerNet::AnkerNetBase* ()> netInterface = nullptr;
			std::function<int()> netVersionIf = nullptr;
			boost::dll::shared_library netLib(libPath);
			if (!netLib.is_loaded()) {
				ANKER_LOG_ERROR << "load lib failed";
				break;
			}

			m_netLibHandle = netLib;

			auto hasNet = netLib.has("GetAnkerNet");
			auto hasVersion = netLib.has("GetAnkerNetMappingVersion");
			ANKER_LOG_INFO << "load lib success, hasNet: " << hasNet << ", hasVersion: " << hasVersion;

			if (!hasNet || !hasVersion) {
				ANKER_LOG_ERROR << "no net interface and version interface";
				break;
			}

			netInterface = netLib.get<AnkerNet::AnkerNetBase* ()>("GetAnkerNet");
			if (!netInterface) {
				ANKER_LOG_ERROR << "get net interface failed";
				break;
			}
			netVersionIf = netLib.get<int()>("GetAnkerNetMappingVersion");
			if (!netVersionIf) {
				ANKER_LOG_ERROR << "get version interface failed";
				break;
			}

			netVersion = netVersionIf();
			*netBase = netInterface();
			ANKER_LOG_INFO << "get net version: " << netVersion;
			return true;
		} catch (const std::exception& e) {
			ANKER_LOG_ERROR << "get plugin error: " << e.what();
		}
		catch (...) {
			ANKER_LOG_ERROR << "get plugin error";
		}
	} while (false);

	ANKER_LOG_INFO << "load failed to release lib";
	ReleaseLibrary();
	return false;
}

bool AnkerNetModuleManager::MostInNeed()
{
	// copy need file to default dir
	auto needFile = AnkerNetModuleHelp::GetNeedFile();
	if (!AnkerNetModuleHelp::FileExist(needFile)) {
		ANKER_LOG_ERROR << "need file not found, " << needFile;
		return false;
	}

	auto defaultFile = AnkerNetModuleHelp::GetDefaultFile();
	ANKER_LOG_INFO << "try copy need file, delete default file first";
	AnkerNetModuleHelp::DeleteDefaultFiles();
	if (!AnkerNetModuleHelp::FileCopy(needFile, defaultFile)) {
		ANKER_LOG_ERROR << "file copy failed for most in need, why?";
		return false;
	}

	auto loadRet = LoadLibraryDefault();
	return loadRet == LoadRet::Success;
}

void AnkerNetModuleManager::StartDownloadThread()
{
	// write config file
	SetIsDownLoaded();

	std::thread thread([this]() {
		DownloadThread(m_dialog);
	});
	thread.detach();
}

void AnkerNetModuleManager::DownloadThread(AnkerNetDownloadDialog* dialog)
{
	ANKER_LOG_INFO << "net plugin download thread start";
	if (!dialog) {
		ANKER_LOG_ERROR << "net download dialog is null";
		return;
	}
	dialog->Change(AnkerNetDownloadDialog::Status::DownLoading, nullptr, [this, dialog]() {
		// download cancel (cancel button)
		ANKER_LOG_INFO << "downloading, cancel click";
		HttpTool::SetDownloadCancel(true);
		BuryDownloadResult(DownloadResult::Canceled);
		dialog->EndModal((int)AnkerNetDownloadDialog::Result::Cancel);		
	});

	DownloadResult buryResult = DownloadResult::Download_Failed;
	auto filename = AnkerNetModuleHelp::GetCacheZipFile();
	auto netDir = AnkerNetModuleHelp::GetAnkerNetDir();
	HttpTool::RetStatus downRet = HttpTool::RetStatus::Failed;
	bool mostInNeed = false;
	int resultCode = 0;
	do {
		if (AnkerNetModuleHelp::DirExist(netDir)) {
			ANKER_LOG_INFO << "delete dir " << netDir;
			AnkerNetModuleHelp::RemoveFilesInDir(netDir);
		}
		ANKER_LOG_INFO << "try download file, delete default file first";
		AnkerNetModuleHelp::DeleteDefaultFiles();
		if (!AnkerNetModuleHelp::CheckCreateDir(netDir)) {
			mostInNeed = true;
			break;
		}

		downRet = HttpTool::DownloadAnkerNetZipSync([dialog](long curSize, long totalSize) {
			int progress = 100 * ((float)curSize / (float)totalSize);
			ANKER_LOG_DEBUG << "download curSize = " << curSize << ",totalSize = " << totalSize << ", " << progress;
			if (dialog) {
				dialog->UpdateProgress(progress);
			}
		}, filename.string());
	} while (false);

	bool isSuccess = false;
	do {
		ANKER_LOG_INFO << "download ret: " << (int)downRet << ", " << filename;
		if (downRet == HttpTool::RetStatus::Canceled) {
			ANKER_LOG_INFO << "user canceled, net plugin download thread over";			
			return;
		}
		if (downRet == HttpTool::RetStatus::NetworkError) {
			buryResult = DownloadResult::Network_Error;
			break;
		}

		buryResult = HttpStatus2Reason(downRet);
		if (downRet == HttpTool::RetStatus::Success) {
			// unzip copy and load
			auto unzipLoadRet = UnzipCopyLoad();
			if (unzipLoadRet == DownloadResult::Success) {
				ANKER_LOG_INFO << "unzip copy load success";
				isSuccess = true;
				buryResult = DownloadResult::Success;
				break;
			}
			buryResult = unzipLoadRet;
		}
		
		ANKER_LOG_INFO << "try to most need, downRet: " << (int)downRet << 
			", buryResult: " << (int)buryResult;
		resultCode = 1;
		if (MostInNeed()) {
			isSuccess = true;
			break;
		}
		else {
			buryResult = DownloadResult::MostNeed_Failed;
		}
	} while (false);

	if (isSuccess) {
		if (dialog) {
			dialog->UpdateProgress(100);
		}

		ANKER_LOG_INFO << "download success";	
		dialog->Change(AnkerNetDownloadDialog::Status::DownLoadSucc, nullptr,
			[this, buryResult, dialog]() {
				// download finish (cancel button)
				ANKER_LOG_INFO << "download success, finish click";
				BuryDownloadResult(buryResult);
				dialog->EndModal((int)AnkerNetDownloadDialog::Result::Success);
		}, resultCode);
	}
	else {
		dialog->Change(AnkerNetDownloadDialog::Status::DownLoadFailed, [this, dialog]() {
			// download failed, retry (ok button)
			ANKER_LOG_INFO << "download failed, retry click";
			StartDownloadThread();
		}, [this, buryResult, dialog]() {
			// download failed, cancel (cancel button)
			ANKER_LOG_INFO << "download failed, cancel click";
			BuryDownloadResult(buryResult);
			dialog->EndModal((int)AnkerNetDownloadDialog::Result::Failed);
		}, (int)buryResult);
	}

	ANKER_LOG_INFO << "net plugin download thread over";
	return;
}

void AnkerNetModuleManager::BuryDownloadResult(DownloadResult reason)
{
	static std::unordered_map<int, std::string> errorMsgMap = {
		{(int)DownloadResult::Success, "success"},
		{(int)DownloadResult::Canceled, "canceled"},
		{(int)DownloadResult::Network_Error, "network error"},
		{(int)DownloadResult::Unzip_Failed, "unzip failed"},
		{(int)DownloadResult::FileCopy_Failed, "file copy failed"},
		{(int)DownloadResult::Load_Failed, "load dll failed"},
		{(int)DownloadResult::UnzipLoad_Failed, "download success but unzip and load failed"},
		{(int)DownloadResult::JsonParseFailed, "json parse failed"},
		{(int)DownloadResult::JsonFormatFailed, "json format failed"},
		{(int)DownloadResult::CodeError, "code error"},
		{(int)DownloadResult::JsonContentError, "json content error"},
		{(int)DownloadResult::ComputeMd5Failed, "compute md5 failed"},
		{(int)DownloadResult::Md5NotEqual, "md5 not equal"},
		{(int)DownloadResult::Download_Failed, "download failed"},
		{(int)DownloadResult::MostNeed_Failed, "backup plan failed"},
	};

	std::map<std::string, std::string> buryMap;
	std::string errorMsg;
	try {
		errorMsg = errorMsgMap.at((int)reason);
	}
	catch (std::exception) {
		ANKER_LOG_ERROR << "map catch exception for " << (int)reason;
		return;
	}
	
	buryMap.insert(std::make_pair("result", std::to_string((int)reason)));
	buryMap.insert(std::make_pair("error_msg", errorMsg));
	ANKER_LOG_INFO << "Report bury event is "<< netplugin_download << ",bury msg : " << errorMsg;

	reportBuryEvent(netplugin_download, buryMap);
}

AnkerNetModuleManager::DownloadResult AnkerNetModuleManager::HttpStatus2Reason(HttpTool::RetStatus httpStatus)
{
	switch (httpStatus)
	{
	case HttpTool::RetStatus::JsonParseFailed:
		return DownloadResult::JsonParseFailed;
	case HttpTool::RetStatus::JsonFormatFailed:
		return DownloadResult::JsonFormatFailed;
	case HttpTool::RetStatus::CodeError:
		return DownloadResult::CodeError;
	case HttpTool::RetStatus::JsonContentError:
		return DownloadResult::JsonContentError;
	case HttpTool::RetStatus::ComputeMd5Failed:
		return DownloadResult::ComputeMd5Failed;
	case HttpTool::RetStatus::Md5NotEqual:
		return DownloadResult::Md5NotEqual;
	default:
		break;
	}

	return DownloadResult::Download_Failed;
}

#include "OnlinePresetManager.hpp"
#include "OnlinePresetInfo.hpp"
#include "OnlinePresetInfoTool.hpp"
#include "VersionTool.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MsgDialog.hpp"
#include "slic3r/GUI/Common/AnkerDialog.hpp"
#include "slic3r/Utils/wxFileTool.hpp"
#include <jansson.h>
#include "miniz.h"
#include "slic3r/GUI/AnkerNetModule/BuryDefines.h"
#include "slic3r/Utils/DataMangerUi.hpp"
#include "wx/filename.h"

namespace Slic3r {
namespace GUI {


#define PRESET_INFO_FILE "OnlinePresetInfo.json"
#define PRESET_FILE "Anker.zip"
#define PRESET_DIR "Anker-ini"
#define LOCAL_ANKER_INI_VERSION "0.0.0"

	wxString GetOnlineDir()
	{
		return wxString::FromUTF8((boost::filesystem::path(Slic3r::data_dir()) / "OnlinePreset" / SLIC3R_VERSION).make_preferred().string());
	}

	wxString GetOnlineInfoFilePath()
	{
		return wxString::FromUTF8( (boost::filesystem::path(Slic3r::data_dir()) / "OnlinePreset" / SLIC3R_VERSION / PRESET_INFO_FILE).make_preferred().string());
	}

	wxString GetOnlineFilePath()
	{
		return wxString::FromUTF8((boost::filesystem::path(Slic3r::data_dir()) / "OnlinePreset" / SLIC3R_VERSION / PRESET_FILE).make_preferred().string());
	}

	wxString GetLocalFileDir()
	{
		return wxString::FromUTF8((boost::filesystem::path(Slic3r::resources_dir()) / "profiles" / PRESET_DIR).make_preferred().string());
	}

	wxString GetLocalVersion()
	{
		return wxString::FromUTF8(LOCAL_ANKER_INI_VERSION);
	}

	bool CheckCreateDir(wxString dir)
	{
		try
		{
			if (!wxFileName::DirExists(dir))
			{
				//gpt,may throw expection
				if (!wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
				{
					return false;
				}
			}
		}
		catch (...)
		{
			return false;
		}
		
		return true;
	}

	bool UnZipToDir(const char* zipfilename, const char* dirname) {

		if (wxDirExists(wxString::FromUTF8(dirname)) == false)
		{
			Utils::wxFileTool::CreateDirRecursively(wxString::FromUTF8(dirname));
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
				Utils::wxFileTool::CreateDirRecursively(wxString::FromUTF8(outfilename));
			}
			else {
				Utils::wxFileTool::CreateFileDirRecursively(wxString::FromUTF8(outfilename));
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

	enum OnlinePresetStatus
	{
		Download_Failed = 0,
		Download_Success = 1,
		Update_Failed = 2,
		Update_Success = 3,
	};

	void BuryEvent(OnlinePresetStatus status, const std::string& statusInfo)
	{
		std::map<std::string, std::string> map;
		map.insert(std::make_pair(std::string(c_online_preset_status), std::to_string(status)));
		map.insert(std::make_pair(std::string(c_online_preset_status_info), statusInfo));
		std::string eventName = e_online_preset_event;
		BuryAddEvent(eventName, map);
	}

	bool OnlinePresetManager::CheckUpdateable()
	{
		const wxString infoFile = GetOnlineInfoFilePath();
		bool ret = OnlinePresetInfoTool::Read(infoFile, info);
		if (ret == false)
		{
			ANKER_LOG_ERROR << "read file error ,file = " << infoFile;
			return false;
		}
		int vc = VersionTool::Compare(info.newVersion, info.curVersion);
		ANKER_LOG_INFO << "newVersion = " << info.newVersion << ",curVersion = " << info.curVersion << ",compare = " << vc;
		if (vc <= 0)
			return false;
		const wxString newFile = GetOnlineFilePath();
		wxString md5;
		ret = Utils::wxFileTool::CalcFileMD5(newFile, md5);
		if (ret == false)
		{
			ANKER_LOG_ERROR << "failed to calc md5 of file ,file = " << newFile;
			return false;
		}
		if (md5 != info.newMD5)
		{
			ANKER_LOG_ERROR << "failed to check md5 of file width config ,md5-file = " << md5<<",md5-config = " << info.newMD5;
			return false;
		}
		return true;
	}

	bool OnlinePresetManager::ChooseUpdate()
	{
		wxString title = _L("common_ota_update_preset_title");
		wxSize size(480, 320);
		auto rect = wxDisplay().GetGeometry();
		wxPoint position((rect.width - size.GetWidth()) / 2 + rect.x, (rect.height - size.GetHeight()) / 2 + rect.y);
		AnkerDialog dialog(nullptr, wxID_ANY, title, wxString::FromUTF8(info.newTips.data()), position, size);
		auto ret = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextNoYesDialog);
		return ret == wxID_OK;
	}

	void OnlinePresetManager::ExecuteUpdate()
	{
		const wxString newFile = GetOnlineFilePath();
		const wxString oldDir = GetLocalFileDir();
		const wxDateTime time = wxDateTime::Now();

		//1.backup
		const wxString backupFileDir = GetOnlineDir() + "//" + PRESET_DIR + "-backup-v=" + info.curVersion	+"-time=" + time.Format("%Y-%m-%d-%H-%M-%S");
		bool ret = Utils::wxFileTool::CopyDirRecursively(oldDir, backupFileDir);
		if(ret == false)
		{
			ANKER_LOG_ERROR << "Failed to Execute Update since Failed to backup cur dir .";
			BuryEvent(Update_Failed, "Backup cur dir Failed.");
			return;
		}

		//2.remove
		ret = Utils::wxFileTool::RemoveDirRecursively(oldDir);
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Failed to Execute Update since Failed to remove cur dir .";

			//roll back
			Utils::wxFileTool::RemoveDirRecursively(oldDir);
			ret = Utils::wxFileTool::CopyDirRecursively(backupFileDir, oldDir);
			if (ret == false)
			{
				ANKER_LOG_ERROR << "Failed to Execute Update since Failed to roll back after remove error .";
			}

			BuryEvent(Update_Failed, "Remove cur dir Failed.");
			return;
		}

		//3.unzip
		ret = UnZipToDir(newFile.ToUTF8().data(), oldDir.ToUTF8().data());
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Failed to Execute Update since Failed to unzip to cur dir .";

			//roll back
			Utils::wxFileTool::RemoveDirRecursively(oldDir);
			ret = Utils::wxFileTool::CopyDirRecursively(backupFileDir, oldDir);
			if (ret == false)
			{
				ANKER_LOG_ERROR << "Failed to Execute Update since Failed to roll back after unzip error .";
			}

			BuryEvent(Update_Failed, "Unzip to cur dir Failed.");
			return;
		}

		//4.modify json
		info.curVersion = info.newVersion;
		const wxString infoFile = GetOnlineInfoFilePath();
		ret = OnlinePresetInfoTool::Write(infoFile, info);
		if(ret)
		{
			ANKER_LOG_INFO << "Execute Update Succeed.";
			BuryEvent(Update_Success, "Execute Update Succeed.");
		}
		else
		{
			ANKER_LOG_INFO << "Failed to Execute Update since Failed to modify json file .";
			BuryEvent(Update_Failed, "Modify Json Succeed.");
		}

	}

	void OnlinePresetManager::CheckAndUpdate()
	{
		ANKER_LOG_INFO << "CheckAndUpdate start";
		auto updateable = CheckUpdateable();
		ANKER_LOG_INFO << "CheckUpdateable ret = " << updateable;
		if (!updateable)
			return;
		bool ret = ChooseUpdate();
		ANKER_LOG_INFO << "ChooseUpdate ret = " << ret;
		if (!ret)
			return;
		ExecuteUpdate();
	}

} // GUI
} // Slic3r

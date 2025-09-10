#include "wxFileTool.hpp"
#include <wx/file.h>
#include "libslic3r/Utils.hpp"
#include "libslic3r/md5.h"

namespace Slic3r {
namespace Utils {
	bool wxFileTool::ReadFileContent(wxString filename, wxString& content)
	{
		bool ret = wxFile::Exists(filename);
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Can't find " << filename;
			return false;
		}

		wxFile file;
		ret = file.Open(filename);
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Can't open file : " << filename;
			return false;
		}

		ret = file.ReadAll(&content);
		file.Close();
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Can't read file : " << filename;
			return false;
		}

		return true;
	}

	bool wxFileTool::WriteFileContent(wxString filename, wxString& content)
	{
		wxFile file(filename, wxFile::write);
		bool ret = file.IsOpened();
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Can't open file : " << filename;
			return false;
		}

		ret = file.Write(content);
		file.Close();
		if (ret == false)
		{
			ANKER_LOG_ERROR << "Can't write file : " << filename;
			return false;
		}
		return true;
	}

	std::string wxFileTool::CalcStrMD5(const std::string& str)
	{
		MD5 md5;
		md5.update(str.c_str(), str.size());
		string md5Str = md5.toString();
		return md5Str;
	}

	bool wxFileTool::CalcFileMD5(wxString filename, wxString& md5Str)
	{
		ANKER_LOG_INFO << "try calc md5 : " << filename;
		wxFile file(filename);
		if (!file.IsOpened()) {
			wxLogError("Cannot open file!");
			return false;
		}

		const size_t bufferSize = 4096;
		unsigned char buffer[bufferSize];
		size_t bytesRead = 0;

		MD5 md5;
		while (!file.Eof()) {
			bytesRead = file.Read(buffer, bufferSize);
			if (bytesRead > 0) {
				md5.update((const void*)buffer, bytesRead);
			}
		}
		md5Str = md5.toString();
		return true;
	}

	bool wxFileTool::wxCopyFile(wxString src, wxString dst, bool delete_dst_if_existed)
	{
		if(!wxFileName::Exists(src))
		{
			ANKER_LOG_INFO << "Src file is not existed";
			return false;
		}

		if (wxFileName::Exists(dst))
		{
			if(delete_dst_if_existed)
			{
				wxRemoveFile(dst);
				if(wxFileName::Exists(dst))
				{
					ANKER_LOG_ERROR << "Failed to Remove drc file";
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		wxFileInputStream input(src);
		if (!input.IsOk())
		{
			ANKER_LOG_ERROR << "Failed to open src file";
			return false;
		}

		wxFileOutputStream output(dst);
		if (!output.IsOk())
		{
			ANKER_LOG_ERROR << "Failed to open dst file";
			return false;
		}

		input.Read(output);
		auto size = input.LastRead();//size will be 0 when file is empty
		auto eof = input.Eof();
		return eof;
	}

	void wxFileTool::GetFilesRecursively(const wxString& directory, std::vector<wxString>& files)
	{
		wxDir dir(directory);
		if (!dir.IsOpened())
		{
			return;
		}

		wxString filename;
		bool cont = dir.GetFirst(&filename, wxT("."), wxDIR_FILES);
		while (cont)
		{
			files.push_back(directory + "/" + filename); 
			cont = dir.GetNext(&filename);
		}

		cont = dir.GetFirst(&filename, wxT("."), wxDIR_DIRS);
		while (cont)
		{
			GetFilesRecursively(directory + "/" + filename, files);
			cont = dir.GetNext(&filename);
		}
	}

	bool wxFileTool::RemoveDirRecursively(const wxString& directory)
	{
		wxDir dir(directory);
		wxString filename;

		if (!dir.IsOpened())
		{
			return false;
		}

		bool cont = dir.GetFirst(&filename);
		while (cont)
		{
			wxString fullPath = directory + "/" + filename;
			if (wxDirExists(fullPath))
			{
				if (!RemoveDirRecursively(fullPath))
				{
					return false;
				}
			}
			else
			{
				if (!wxRemoveFile(fullPath))
				{
					return false; 
				}
			}
			cont = dir.GetNext(&filename);
		}
		return wxRmdir(directory);
	}

	void wxFileTool::RemoveDirRecursivelyAMAP(const wxString& directory)
	{
		try
		{
			wxDir dir(directory);
			wxString filename;

			if (!dir.IsOpened())
			{
				return;
			}

			bool cont = dir.GetFirst(&filename);
			while (cont)
			{
				wxString fullPath = directory + "/" + filename;
				if (wxDirExists(fullPath))
				{
					RemoveDirRecursivelyAMAP(fullPath);
				}
				else
				{
					try
					{
						if(wxFileName::IsFileWritable(fullPath))
						{
							boost::filesystem::path path(fullPath.ToStdWstring());
							if(boost::filesystem::remove(path) == false)
								ANKER_LOG_ERROR << "boost::filesystem::remove error,path = " << path;
						}
					}
					catch (...)
					{
					}
				}
				cont = dir.GetNext(&filename);
			}
			if (wxFileName::IsDirWritable(directory))
			{
				boost::filesystem::path path(directory.ToStdWstring());
				if(boost::filesystem::remove(path) == false)
					ANKER_LOG_ERROR << "boost::filesystem::remove error,path = " << path;
			}
		}
		catch (...)
		{
		}
	}

	bool wxFileTool::CopyDirRecursively(const wxString& dirSrc, const wxString& dirDst)
	{
		wxDir dir(dirSrc);
		wxString filename;
		if(wxDirExists(dirDst) == false)
		{
			CreateDirRecursively(dirDst);
		}

		if (!dir.IsOpened())
		{
			return false;
		}

		bool cont = dir.GetFirst(&filename);
		while (cont)
		{
			wxString fullPathSrc = dirSrc + "/" + filename;
			wxString fullPathDst = dirDst + "/" + filename;
			if (wxDirExists(fullPathSrc))
			{
				if (CopyDirRecursively(fullPathSrc, fullPathDst) == false)
				{
					return false;
				}
			}
			else
			{
				if (wxCopyFile(fullPathSrc, fullPathDst) == false)
				{
					return false;
				}
			}
			cont = dir.GetNext(&filename);
		}
		return true;
	}


	bool wxFileTool::CreateDirRecursively(const wxString& dirName)
	{
		try
		{
			auto filePath = boost::filesystem::path(dirName.ToStdWstring());
			if (boost::filesystem::exists(filePath))
				return true;
			return boost::filesystem::create_directories(filePath);
		}
		catch (...)
		{
			ANKER_LOG_ERROR << "Create Directories Error: " << dirName;
		}
		return false;
	}

	bool wxFileTool::CreateFileDirRecursively(const wxString& fileName)
	{
		wxFileName filename(fileName);
		auto dir = filename.GetPath();
		return CreateDirRecursively(dir);
	}
}
}
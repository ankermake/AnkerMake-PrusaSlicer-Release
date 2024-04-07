#ifndef slic3r_WxFileTool_hpp_
#define slic3r_WxFileTool_hpp_
#include <wx/string.h>

namespace Slic3r {
namespace Utils {

class wxFileTool
{
public:
	//return true if read successfully
	//return false if error.error include file not exist ,open file error ,read file error
	static bool ReadFileContent(wxString filename, wxString& content);
	//return true if write successfully
	//return false if error.error include open file error ,write file error
	static bool WriteFileContent(wxString filename, wxString& content);
	//return true if calc successfully
	//return false if error.error include ReadFileContent error
	static bool CalcFileMD5(wxString filename, wxString& md5Str);
	//return true if copy successfully
	//return false if error.
	static bool wxCopyFile(wxString src, wxString dst,bool delete_dst_if_existed = true);
	//Get all files in a folder recursively
	static void GetFilesRecursively(const wxString& dir, std::vector<wxString>& files);
	//Remove all files and dirs in a folder recursively
	static bool RemoveDirRecursively(const wxString& directory);
	//Remove all files and dirs in a folder recursively,as much as possible
	static void RemoveDirRecursivelyAMAP(const wxString& directory);
	//Copy all files in dirSrc to dirDst
	static bool CopyDirRecursively(const wxString& dirSrc, const wxString& dirDst);
	//Create dir Recursively
	static bool CreateDirRecursively(const wxString& dirName);
	static bool CreateFileDirRecursively(const wxString& fileName);
};


}
}

#endif

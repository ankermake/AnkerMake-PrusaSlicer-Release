#include "AnkerConfig.hpp"
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "wx/wx.h"

#ifdef _WIN32
#include <codecvt>
#include <xlocbuf>
#include <xstring>
#endif


namespace AnkerConfig
{

std::string get_current_dir()
{

#ifdef _WIN32
//	WCHAR buffer[FILENAME_MAX];
//	GetModuleFileName(NULL, buffer, FILENAME_MAX);
//
//	int strLength = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);
//
//	char* chPath = new char[strLength * sizeof(char)];
//
//	WideCharToMultiByte(CP_ACP, 0, buffer, -1, chPath, strLength, NULL, NULL);
//
//	std::string processAbsolPath = chPath;
//
//	std::string::size_type pos = processAbsolPath.find_last_of("\\/");
//	processAbsolPath = processAbsolPath.substr(0, pos);
//
//	wxString processDir = processAbsolPath;
//	processDir.Replace("\\", "/");
//
//	return processDir.ToStdString();
	auto paths = wxStandardPaths::Get();
	auto execPath = paths.GetExecutablePath();
	auto filename = wxFileName(execPath);
	auto dir = filename.GetPath();
	auto path = boost::filesystem::current_path();
	return dir.ToStdString(wxConvUTF8);
#else
	char buffer[FILENAME_MAX];
	if (getcwd(buffer, sizeof(buffer)) != NULL) {
		return std::string(buffer);
	}
	else {
		return "";
	}
#endif
}


std::map<std::string, std::string> read_ini_file(const std::string& filename)
{
	std::map<std::string, std::string> ini;
	std::string line;
	std::string section;
#ifdef _WIN32
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	auto name = converter.from_bytes(filename);
	std::ifstream file(name);
#else
	std::ifstream file(filename);
#endif

	if (file.is_open()) {
		while (getline(file, line)) {

			if (line.empty() || line[0] == '#' || line[0] == ';') {
				continue;
			}

			if (line[0] == '[' && line[line.size() - 1] == ']') {
				section = line.substr(1, line.size() - 2);
				continue;
			}

			size_t pos = line.find('=');
			if (pos != std::string::npos) {
				std::string key = line.substr(0, pos);
				std::string value = line.substr(pos + 1);
				ini[key] = value;
			}
		}
		file.close();
	}
	return ini;
}

std::map<std::string, std::string> GetAnkerConfigIni()
{
	static std::map<std::string, std::string> ini;
	if (!ini.empty()) {
		return ini;
	}

	std::string current_dir = AnkerConfig::get_current_dir();
	if (current_dir.empty()) {
		std::cout << "Failed to get current directory." << std::endl;
		return ini;
	}
	std::string filename = std::string();

#ifdef _WIN32
	filename = current_dir + "/AnkerConfig.ini";
#else
	filename = "/tmp/AnkerConfig.ini";
#endif
	std::cout << filename << std::endl;

	ini = AnkerConfig::read_ini_file(filename);
	return ini;
}

std::string getankerDomainUrl()
{
	auto ini = AnkerConfig::GetAnkerConfigIni();

	std::map<std::string, std::string>::iterator iter = ini.find("AnkerDomainUrl");
	if (iter == ini.end()) {
		return std::string("https://community.ankermake.com/");
	}

	std::string strValue = iter->second;
	return iter->second;
}

std::vector<std::string> GetAnkerNetUrls()
{
	auto ini = AnkerConfig::GetAnkerConfigIni();
	std::vector<std::string> defaultUrls = {
		"https://make-app.ankermake.com/v1/slicer/get_net",
		//"https://make-app-eu.ankermake.com/v1/slicer/get_net",
	};

	std::map<std::string, std::string>::iterator iter = ini.find("NetPluginDomain");
	if (iter == ini.end()) {
		return defaultUrls;
	}
	std::string strValue = iter->second;
	if (strValue == "QA") {
		defaultUrls.clear();
		defaultUrls.push_back("https://make-app-us-qa.eufylife.com/v1/slicer/get_net");
	}
	
	return defaultUrls;
}

}
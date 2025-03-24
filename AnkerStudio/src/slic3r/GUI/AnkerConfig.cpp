#include "AnkerConfig.hpp"
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#include <codecvt>
#include <xlocbuf>
#include <xstring>
#else
#include <unistd.h>
#endif
#include "wx/wx.h"
#include "libslic3r/Utils.hpp"
#include "slic3r/Config/AnkerCommonConfig.hpp"


namespace AnkerConfig
{

std::string get_current_dir()
{
#ifdef _WIN32
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

	std::string dir;
#ifdef _WIN32
	dir = AnkerConfig::get_current_dir();
#else	
	dir = Slic3r::data_dir();
#endif
	
	if (dir.empty()) {
		return ini;
	}
	ini = AnkerConfig::read_ini_file(dir + "/AnkerConfig.ini");
	return ini;
}

std::string getankerDomainUrl()
{
	auto ini = AnkerConfig::GetAnkerConfigIni();

	std::map<std::string, std::string>::iterator iter = ini.find("AnkerDomainUrl");
	if (iter == ini.end()) {
		return Slic3r::UrlConfig::OldLoginUrl;
	}

	std::string strValue = iter->second;
	return iter->second;
}

std::vector<std::string> GetAnkerNetUrls()
{
	auto ini = AnkerConfig::GetAnkerConfigIni();
	std::vector<std::string> defaultUrls = {
		Slic3r::UrlConfig::NetUrl,
		//Slic3r::UrlConfig::NetUrl_EU,
	};

	std::map<std::string, std::string>::iterator iter = ini.find("NetPluginDomain");
	if (iter == ini.end()) {
		return defaultUrls;
	}
	std::string strValue = iter->second;
	if (strValue == "QA") {
		defaultUrls.clear();
		defaultUrls.push_back(Slic3r::UrlConfig::NetUrl_QA);
	}
	
	return defaultUrls;
}

}
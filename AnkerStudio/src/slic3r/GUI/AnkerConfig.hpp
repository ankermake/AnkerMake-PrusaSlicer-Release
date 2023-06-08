#ifndef _ANKER_CONFIG_HPP_
#define _ANKER_CONFIG_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "wx/wx.h"



namespace AnkerConfig
{
std::string ankerDomainUrl = "https://community.ankermake.com/";


std::string get_current_dir()
{

#ifdef _WIN32
	WCHAR buffer[FILENAME_MAX];
	GetModuleFileName(NULL, buffer, FILENAME_MAX);

	int strLength = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);

	char* chPath = new char[strLength * sizeof(char)];

	WideCharToMultiByte(CP_ACP, 0, buffer, -1, chPath, strLength, NULL, NULL);

	std::string processAbsolPath = chPath;

	std::string::size_type pos = processAbsolPath.find_last_of("\\/");
	processAbsolPath = processAbsolPath.substr(0, pos);

	wxString processDir = processAbsolPath;
	processDir.Replace("\\", "/");

	return processDir.ToStdString();
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
	std::ifstream file(filename);
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

std::string getankerDomainUrl()
{
	bool res = false;
	if (res)
		return ankerDomainUrl;

	std::string current_dir = get_current_dir();
	if (current_dir.empty()) {
		std::cout << "Failed to get current directory." << std::endl;
		return std::string("https://community.ankermake.com/");
	}
	std::string filename = std::string();

#ifdef _WIN32
	filename = current_dir + "/AnkerConfig.ini";
#else
	filename = "/tmp/AnkerConfig.ini";
#endif
	std::cout<< filename <<std::endl;

	std::map<std::string, std::string> ini = read_ini_file(filename);

	std::map<std::string, std::string>::iterator iter = ini.find("AnkerDomainUrl");
	if(iter == ini.end())
		return std::string("https://community.ankermake.com/");

	std::string strValue = iter->second;
	return iter->second;
}

#define LoginWebUrl	getankerDomainUrl() + "passport-ct/#/login"

}
#endif 
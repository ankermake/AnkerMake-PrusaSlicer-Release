#ifndef ANKER_COM_FUNCTION_HPP
#define ANKER_COM_FUNCTION_HPP

#include <string>
#include <map>
#include "anker_plungin/Interface Files/AnkerPlugin.hpp"
#include "slic3r/GUI/AnkerNetModule/BuryDefines.h"


void initBuryPoint();

void loadThePlugin();

void setBuryHeaderList(const std::vector<std::string>& headerList);

void setPluginParameter(const std::string& userInfo,
						 const std::string& envirment,
						 const std::string& userid,
						 const std::string& openudid);

void getSentryDsn(std::string &dsn);

void reportBuryEvent(std::string eventStr,std::map<std::string,std::string> infoMap, bool isBlock = false);

void logCryptText(std::string logContent, int& strLength, std::string& cryptStr);

int logCreateKeyBlock(uint8_t** data, uint32_t &len);

std::string getPcName();
std::string getMacAddress();

#endif // !ANKER_COM_FUNCTION_HPP
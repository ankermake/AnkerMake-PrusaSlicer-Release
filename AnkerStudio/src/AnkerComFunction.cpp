#include "AnkerComFunction.hpp"

#include <iostream>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <time.h>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

//#include "libslic3r_version.h"
#ifdef _WIN32
#include <windows.h>
#include <netlistmgr.h>
#include <atlbase.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#elif __APPLE__
#import <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <dlfcn.h>

#include <CoreServices/CoreServices.h>
#else // Linux/BSD
#include <charconv>
#endif

#ifdef _WIN32
#include <WinSock2.h>
#include <dxgi.h>
#include <iphlpapi.h>
#include <Windows.h>
#include <comdef.h>
#include <WbemIdl.h>
#pragma comment(lib, "DXGI.lib")
#endif // _WIN32

#include "libslic3r_version.h"
using namespace boost;
using namespace boost::asio;

AnkerPlugin* pAnkerPlugin = nullptr;
void* pInstancPlugin = nullptr;
typedef AnkerPlugin* (*AnkerPluginInstanse)();


void loadThePlugin()
{
#ifdef OPEN_SOURCE_MODE
    return;
#endif

    static bool isInit = false;

    if (isInit)
        return;
do{
    try {

#ifndef  __APPLE__
        HMODULE ankerPluginHandle = LoadLibrary(L"AnkerPlugin.dll");
        if (ankerPluginHandle)
        {
            AnkerPlugin* pInstance = (AnkerPlugin*)GetProcAddress(ankerPluginHandle, "GetAnkerPlugin");

            if (pInstance)
            {            
                pAnkerPlugin = ((AnkerPluginInstanse)pInstance)();
                if (pAnkerPlugin)
                {
                    std::string dsn = "";
                    pAnkerPlugin->getSentryDsn(dsn);
                    std::cout << "load plubgin success" << std::endl;
                    isInit = true;
                }
                else
                {
                    int res = GetLastError();
                    std::cout << "load plubgin fail:" << res << std::endl;
                }
            }
            else
            {
                int res = GetLastError();
                std::cout << "load plubgin fail:" << res << std::endl;
            }
        }
#else
        void* pInstancPlugin = dlopen("@executable_path/../Frameworks/libAnkerPlugin.dylib", RTLD_NOW| RTLD_GLOBAL);
        std::string errorCode = "0";

        if (pInstancPlugin)
        {
            void* pInstance = dlsym(pInstancPlugin, "GetAnkerPlugin");
            if (pInstance)
            {
                std::cout << "---------load plubgin success" << std::endl;
                isInit = true;
                std::string pluginAddressEx = std::to_string(reinterpret_cast<uintptr_t>(pAnkerPlugin));
                
                pAnkerPlugin = ((AnkerPluginInstanse)pInstance)();
                
                if (pAnkerPlugin)
                {
                    std::string pluginAddress = std::to_string(reinterpret_cast<uintptr_t>(pAnkerPlugin));
                    std::cout << "---------load plubgin success" << std::endl;                
                    pAnkerPlugin->getSentryDsn(errorCode);
                    isInit = true;
                }
                else
                {
                    std::cout << "load no plubgin fail:" << errorCode << std::endl;
                }

            }
            else
            {                
                std::cout << "load no instance fail:" << errorCode << std::endl;
            }
        }
    
#endif
        }
        catch (const std::exception& e) {
            std::cout << "get plugin error: " << e.what();
        }
        catch (...) {                 
            std::cout << "get plugin error";
        }
    } 
    while (false);
}


static std::string getMachineId()
{
    std::string machineId = std::string();
#ifdef _WIN32

    auto wstringTostring = [](std::wstring wTmpStr) -> std::string {
        std::string resStr = std::string();
        int len = WideCharToMultiByte(CP_UTF8, 0, wTmpStr.c_str(), -1, nullptr, 0, nullptr, nullptr);

        if (len <= 0)
            return std::string();
        std::string desStr(len, 0);

        WideCharToMultiByte(CP_UTF8, 0, wTmpStr.c_str(), -1, &desStr[0], len, nullptr, nullptr);

        resStr = desStr;

        return resStr;
    };

    HKEY key = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography",
        0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS) {
        wchar_t buffer[1024];
        memset(buffer, 0, sizeof(wchar_t) * 1024);
        DWORD size = sizeof(buffer);
        bool ok = (RegQueryValueEx(key, L"MachineGuid", NULL, NULL, (LPBYTE)buffer, &size) ==
            ERROR_SUCCESS);
        RegCloseKey(key);
        if (ok) {
            machineId = wstringTostring(buffer);
        }
    }

#elif __APPLE__
    FILE* fp = NULL;
    char buffer[1024];

    memset(buffer, 0, 1024);
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    CFStringRef strRef = (CFStringRef)IORegistryEntryCreateCFProperty(service, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    CFStringGetCString(strRef, buffer, 1024, kCFStringEncodingMacRoman);
    machineId = buffer;

#endif // _WIN32        
    return machineId;
}

void initBuryPoint()
{
    loadThePlugin();
    std::vector<std::string> headerList;
        
    std::string appModelType = "PC";
    std::string appVersion = SLIC3R_VERSION;    
    std::string appCountry = "";
    std::string appLanguage = "";
    std::string machinId = getMachineId();

    headerList.push_back(std::string("App_name: ") + "AnkerMake Studio");

    headerList.push_back("Model_type: " + appModelType);
    headerList.push_back("App_version: V" + appVersion);

    headerList.push_back("Country: " + appCountry);
    headerList.push_back("Language: " + appLanguage);

    headerList.push_back("Expect:");
    headerList.push_back("Openudid: " + machinId);
    //to be stable init ""
    headerList.push_back(std::string("Os_version: ") + "");    

    std::string osType = std::string("Windows");
#ifdef _WIN32
    osType = "Windows";
#elif __APPLE__
    osType = "MacOS";
#endif
    headerList.push_back("Os_type: " + osType);
    headerList.push_back("Content-Type:application/json;charset=UTF-8");

    if (pAnkerPlugin)
    {
        pAnkerPlugin->setHeaderList(headerList);
        std::string domain = std::string();
        
        std::string envir = "US";
        if (domain.find("us-qa")!= std::string::npos)
            envir = "QA";
        else if (domain.find("us-ci") != std::string::npos)
            envir = "CI";
        else if (domain.find("eu") != std::string::npos)
            envir = "EU";
        else
            envir = "US";
        pAnkerPlugin->setPluginParameter("", envir, "", machinId);
    }
}
void setBuryHeaderList(const std::vector<std::string>& headerList)
{
    loadThePlugin();

    if (pAnkerPlugin)
    {
        pAnkerPlugin->setHeaderList(headerList);
    }
}

void setPluginParameter(const std::string& userInfo,
    const std::string& envirment,
    const std::string& userid,
    const std::string& openudid)
{
    loadThePlugin();

    if (pAnkerPlugin)
    {
        pAnkerPlugin->setPluginParameter(userInfo, envirment, userid, openudid);
    }
}

void getSentryDsn(std::string &dsn)
{
    loadThePlugin();

    if (pAnkerPlugin)
    {
        pAnkerPlugin->getSentryDsn(dsn);
    }
            
}

void reportBuryEvent(std::string eventStr, std::map<std::string, std::string> infoMap,bool isBlock)
{
    loadThePlugin();
    if (pAnkerPlugin)
    {    
        pAnkerPlugin->reportBuryPoint(eventStr, infoMap, isBlock);
    }
}
void logCryptText(std::string logContent, int& strLength, std::string& cryptStr)
{
    loadThePlugin();    
    if (pAnkerPlugin)
    {
        pAnkerPlugin->encryptText(logContent, strLength, cryptStr);
    }        
}

int logCreateKeyBlock(uint8_t** data, uint32_t& len)
{
    loadThePlugin();

    if (pAnkerPlugin)
    {
        return pAnkerPlugin->createKeyBlock(data, len);
    }

    return -1;
}

std::string getPcName()
{
	std::string pcname = ip::host_name();
	return pcname;
}

std::string getMacAddress()
{
    std::string macAddress = std::string();
#ifndef __APPLE__
    ULONG ulBufferSize = 0;
    DWORD dwResult = ::GetAdaptersInfo(NULL, &ulBufferSize);
    if (ERROR_BUFFER_OVERFLOW != dwResult) {
        return std::string();
    }

    PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulBufferSize];
    if (!pAdapterInfo)
    {
        return std::string();
    }

    dwResult = ::GetAdaptersInfo(pAdapterInfo, &ulBufferSize);
    if (ERROR_SUCCESS != dwResult)
    {
        delete[] pAdapterInfo;
        return std::string();
    }

    BYTE pMac[MAX_ADAPTER_ADDRESS_LENGTH] = { 0 };
    int nLen = MAX_ADAPTER_ADDRESS_LENGTH;

    if (NULL != pAdapterInfo)
    {
        PIP_ADDR_STRING pAddTemp = &(pAdapterInfo->IpAddressList);

        if (NULL != pAddTemp)
        {
            for (int i = 0; i < (int)pAdapterInfo->AddressLength; ++i)
            {
                pMac[i] = pAdapterInfo->Address[i];
            }
            nLen = pAdapterInfo->AddressLength;
        }
    }
    delete[] pAdapterInfo;

    auto Encode16 = [](const BYTE* buf, int len) -> std::wstring {
        const WCHAR strHex[] = L"0123456789ABCDEF";
        std::wstring wstr;
        wstr.resize(len * 2);
        for (int i = 0; i < len; ++i)
        {
            wstr[i * 2 + 0] = strHex[buf[i] >> 4];
            wstr[i * 2 + 1] = strHex[buf[i] & 0xf];
        }
        return wstr;
    };

    auto wstringTostring = [](std::wstring wTmpStr) -> std::string {
        std::string resStr = std::string();
        int len = WideCharToMultiByte(CP_UTF8, 0, wTmpStr.c_str(), -1, nullptr, 0, nullptr, nullptr);

        if (len <= 0)
            return std::string();
        std::string desStr(len, 0);

        WideCharToMultiByte(CP_UTF8, 0, wTmpStr.c_str(), -1, &desStr[0], len, nullptr, nullptr);

        resStr = desStr;

        return resStr;
    };

    std::wstring strMac = Encode16(pMac, nLen);
    macAddress = wstringTostring(strMac);
#else
    //todo get mac macaddress
#endif
    return macAddress;
}

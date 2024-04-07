#ifndef SYSTEM_INFO_HPP
#define SYSTEM_INFO_HPP

#ifdef _WIN32
#include <WinSock2.h>
#include <dxgi.h>
#include <iphlpapi.h>
#include <Windows.h>
#include <comdef.h>
#include <WbemIdl.h>
#endif // _WIN32
#include <string>
#include "AnkerNetDefines.h"
using namespace AnkerNet;
class SysInfoCollector {
public:
    static SysInfo GetSysInfo();
private:
#ifdef _WIN32
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
#endif
    
private:
    static SysInfo m_sysInfo;
};
#endif

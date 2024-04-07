#include "SystemInfo.hpp"
#include "SystemInfo.hpp"
#include <wx/wx.h>
#include <locale>
#include <codecvt>
#include <algorithm>
#include "libslic3r/Utils.hpp"

#include <time.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#endif
#include <slic3r/Utils/DataMangerUi.hpp>

#ifdef _WIN32
#pragma comment(lib, "DXGI.lib")
#endif // _WIN32

SysInfo SysInfoCollector::m_sysInfo;

#ifdef _WIN32
BOOL CALLBACK SysInfoCollector::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) 
{
    MONITORINFOEX mi;
    static int count = 0;
    mi.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &mi)) {
        std::cout << "Display: " << mi.szDevice << std::endl;
        int width = mi.rcMonitor.right - mi.rcMonitor.left;
        int height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        std::cout << "  Width: " << width << std::endl;
        std::cout << "  Height: " << height << std::endl;
        if (dwData) {
            SysInfoCollector* collector = (SysInfoCollector*)(dwData);
            if (collector) {
                if (count == 0) {
                    collector->m_sysInfo.m_primary_screen_resolution =
                        std::to_string(width) + "x" + std::to_string(height);
                }
                else if (count > 0) {
                    collector->m_sysInfo.m_extended_screen_resolution +=
                        std::string("screen") + std::to_string(count + 1) +
                        std::string(":") + std::to_string(width) + "x" +
                        std::to_string(height) + std::string(",");
                }
            }
        }
    }
    return TRUE;
}
#endif

SysInfo SysInfoCollector::GetSysInfo()
{
    ANKER_LOG_INFO << "initSysInfo thread id: " << std::this_thread::get_id();
    m_sysInfo.m_os_version = wxGetOsDescription().ToStdString();

    // Windows 10 (build 19045), 64-bit edition
    // macOS Ventura Version 13.4 (Build 22F66)
    ANKER_LOG_INFO << "osVersion: " << m_sysInfo.m_os_version;
    wxString machineId;
#ifdef _WIN32
    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    for (i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    // string includes manufacturer, model and clockspeed
    m_sysInfo.m_cpu_model = std::string(CPUBrandString);
    // CPU Type: 11th Gen Intel(R) Core(TM) i7-11700K @ 3.60GHz
    ANKER_LOG_INFO << "CPU Type: " << std::string(CPUBrandString);

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    m_sysInfo.m_ram = std::to_string(statex.ullTotalPhys / (1024 * 1024)) + " MB";
    // Total Physical Memory (RAM): 16149 MB
    ANKER_LOG_INFO << "Total Physical Memory (RAM): " << statex.ullTotalPhys / (1024 * 1024) << " MB";

    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);

    IDXGIFactory* pFactory = NULL;
    IDXGIAdapter* pAdapter = NULL;
    std::vector<IDXGIAdapter*> vAdapters;

    int iAdapterNum = 0;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
    if (FAILED(hr)) {
        return m_sysInfo;
    }

    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    std::string gpuModelStr = "";
    int videoRam = 0;
    for (int i = 0; i < vAdapters.size(); i++) {
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);
        std::wstring wstr(adapterDesc.Description);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string str = converter.to_bytes(wstr);
        gpuModelStr += str + ", ";
        if (adapterDesc.DedicatedSystemMemory / (1024 * 1024) > videoRam) {
            videoRam = adapterDesc.DedicatedSystemMemory / (1024 * 1024);
        }
        ANKER_LOG_INFO << "description: " << str;
        ANKER_LOG_INFO << "System Video Memory: " << i + 1 << adapterDesc.DedicatedSystemMemory / (1024 * 1024);
        ANKER_LOG_INFO << "Video card " << i + 1 << ", DedicateVideoMemory: " << adapterDesc.DedicatedSystemMemory / (1024 * 1024);
        ANKER_LOG_INFO << "Video card " << i + 1 << ", SharedSystemMemory: " << adapterDesc.SharedSystemMemory / (1024 * 1024);
        ANKER_LOG_INFO << "Video card " << i + 1 << ", DeviceId: " << adapterDesc.DeviceId;
    }
    m_sysInfo.m_gpu_model = gpuModelStr;
    m_sysInfo.m_video_ram = std::to_string(videoRam);

    HKEY key = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key)
        == ERROR_SUCCESS) {
        wchar_t buffer[1024];
        memset(buffer, 0, sizeof(wchar_t) * 1024);
        DWORD size = sizeof(buffer);
        bool ok = (RegQueryValueEx(key, L"MachineGuid", NULL, NULL, (LPBYTE)buffer, &size) ==
            ERROR_SUCCESS);
        RegCloseKey(key);
        if (ok) {
            machineId = buffer;
        }
    }

#elif __APPLE__
    int mib[2] = { 0 };
    size_t len = 0;

    mib[0] = CTL_HW;
    mib[1] = HW_MACHINE;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    char* machine_model = (char*)malloc(len);
    memset(machine_model, 0, len);
    sysctl(mib, 2, machine_model, &len, NULL, 0);
    //machine: arm64
    ANKER_LOG_INFO << "machine: " << std::string(machine_model);
    free(machine_model);

    FILE* fp = NULL;
    char buffer[1024];

    //Get CPU info.
    memset(buffer, 0, 1024);
    fp = popen("sysctl machdep.cpu.brand_string", "r");
    if (fp == NULL) {
        ANKER_LOG_ERROR << "Failed to run sysctl.";
    }

    ANKER_LOG_INFO << "CPU info:";
    std::string rCpuStr = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        //printf("%s", buffer);
        rCpuStr += std::string(buffer);
    }
    pclose(fp);
    //machdep.cpu.brand_string: Apple M2 Pro
    size_t index1 = rCpuStr.find("brand_string:");
    if (index1 != std::string::npos) {
        index1 += 13;
        m_sysInfo.m_cpu_model = rCpuStr.substr(index1, rCpuStr.length() - index1);
        ANKER_LOG_INFO << m_sysInfo.m_cpu_model;
    }


    // Get GPU info.
    memset(buffer, 0, 1024);
    fp = popen("system_profiler SPDisplaysDataType", "r");
    if (fp == NULL) {
        ANKER_LOG_ERROR << "Failed to run system_profiler.";
    }
    ANKER_LOG_INFO << "gpu info:";
    std::string rGpuStr = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        //printf("%s", buffer);
        rGpuStr += std::string(buffer);
    }
    pclose(fp);
    index1 = rGpuStr.find("Chipset Model:");
    if (index1 != std::string::npos) {
        index1 += 14;
        std::string tmpStr = rGpuStr.substr(index1, rGpuStr.length() - index1);
        size_t index2 = tmpStr.find('\n');
        if (index2 != std::string::npos) {
            m_sysInfo.m_gpu_model = tmpStr.substr(0, index2);
        }
    }
    ANKER_LOG_INFO << "Chipset Model:" << m_sysInfo.m_gpu_model;
    // Get main display.
    ANKER_LOG_INFO << "Display info:";
    std::string subStr = "Resolution:";
    size_t pos = rGpuStr.find(subStr);
    std::string displays;
    int count = 0;
    while (pos != std::string::npos) {
        size_t pos2 = rGpuStr.find('\n', pos);
        if (pos2 != std::string::npos) {
            pos += 12;
            std::string displayStr = rGpuStr.substr(pos, pos2 - pos);
            if (count++ == 0) {
                m_sysInfo.m_primary_screen_resolution = displayStr;
            }
            displays += std::string("\"screen") + std::to_string(count) + "\":\"" + displayStr + "\",";
        }
        pos = rGpuStr.find(subStr, pos + subStr.length());
    }
    index1 = displays.find_last_of(',');
    if (index1 != std::string::npos) {
        displays = displays.substr(0, index1);
    }
    displays = "{" + displays + "}";
    m_sysInfo.m_extended_screen_resolution = displays;
    ANKER_LOG_INFO << "m_primary_screen_resolution: " << m_sysInfo.m_primary_screen_resolution;
    ANKER_LOG_INFO << "m_extended_screen_resolution: " << displays;

    // Get Ram memsize.
    memset(buffer, 0, 1024);
    fp = popen("sysctl hw.memsize", "r");
    if (fp == NULL) {
        ANKER_LOG_ERROR << "Failed to run sysctl.";
    }

    ANKER_LOG_INFO << "Ram info:";
    std::string rRamStr = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        //printf("%s", buffer);
        rRamStr += std::string(buffer);
    }
    pclose(fp);
    //hw.memsize: 17179869184
    size_t index2 = rRamStr.find("hw.memsize:");
    if (index2 != std::string::npos) {
        index2 += 11;
        std::string memSizeStr = rRamStr.substr(index2, rRamStr.length() - index2);
        memSizeStr.erase(std::remove_if(memSizeStr.begin(), memSizeStr.end(), [](unsigned char c) {
            return std::isspace(c);
            }), memSizeStr.end());
        int64_t memSize = std::stoll(memSizeStr);
        int64_t mbSize = memSize / (1024 * 1024);
        m_sysInfo.m_ram = std::to_string(mbSize) + "MB";
        ANKER_LOG_INFO << "m_ram: " << m_sysInfo.m_ram;
    }
    ANKER_LOG_INFO << rRamStr; // totSize / (1024 * 1024)

    memset(buffer, 0, 1024);
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    CFStringRef strRef = (CFStringRef)IORegistryEntryCreateCFProperty(service, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    CFStringGetCString(strRef, buffer, 1024, kCFStringEncodingMacRoman);
    machineId = buffer;

#endif // _WIN32
    m_sysInfo.m_machineid = machineId.ToStdString();
    ANKER_LOG_INFO << "Machine id: " << m_sysInfo.m_machineid;
    return m_sysInfo;
}

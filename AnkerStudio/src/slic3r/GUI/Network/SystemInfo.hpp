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
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class SysInfo {
public:
    std::string m_app_version = "";
    std::string m_os_type = "";
    std::string m_os_version = "";
    std::string m_cpu_model = "";
    std::string m_gpu_model = "";
    std::string m_ram = "";
    std::string m_video_ram = "";
    std::string m_primary_screen_resolution = "";
    std::string m_extended_screen_resolution = "";
    std::string m_opengl_version = "";
};

class SysInfoCollector {
public:
    SysInfoCollector();


#ifdef _WIN32
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
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
                        collector->m_sysInfo.m_primary_screen_resolution = std::to_string(width) + "x" + std::to_string(height);
                    }
                    else if(count > 0){
                        collector->m_sysInfo.m_extended_screen_resolution += std::string("screen") + std::to_string(count + 1) + std::string(":") + std::to_string(width) + "x" + std::to_string(height) + std::string(",");
                    }
                }
            }
        }
        return TRUE;
    }
#endif

    SysInfo getSysInfo() const;
    std::string getMachineUniqueId() const;
    void initSysInfo();
    void setAppVersion(const std::string& version);

    void goGetPCInfo();

private:
    SysInfo m_sysInfo;
    std::string m_Machineid = "";

    std::mutex m_mutex; 
    std::condition_variable m_cv;
    bool m_start = false;
};




#endif

// Why?
#define _WIN32_WINNT 0x0502
// The standard Windows includes.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <shellapi.h>
#include <wchar.h>
#include <Map>

#pragma comment(lib, "version.lib")
#ifdef SLIC3R_GUI
extern "C"
{
    // Let the NVIDIA and AMD know we want to use their graphics card
    // on a dual graphics card system.
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif /* SLIC3R_GUI */

#include <stdlib.h>
#include <stdio.h>

#ifdef SLIC3R_GUI
    #include <GL/GL.h>
#endif /* SLIC3R_GUI */

#include <string>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

using namespace boost;
using namespace boost::asio;

// Capture windows exceptions to generate dump files
#ifdef WIN32
#include <DbgHelp.h>
#include <windef.h>
#pragma comment(lib, "DbgHelp.lib")

#ifndef OPEN_SOURCE_MODE
#include "sentry.h"
#endif
#include "AnkerComFunction.hpp"

#ifdef _WIN32
#include <windows.h>
#include <netlistmgr.h>
#include <atlbase.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#elif __APPLE__
#import <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#else // Linux/BSD
#include <charconv>
#endif

extern "C" {
    typedef int(__stdcall* Slic3rMainFunc)(int argc, wchar_t** argv);
    Slic3rMainFunc slic3r_main = nullptr;

    //
    typedef LONG(__stdcall* AnkerExceptionHandler)(EXCEPTION_POINTERS* pException);
    AnkerExceptionHandler ankerExceptionHandler = nullptr;    
}

#ifndef OPEN_SOURCE_MODE
static sentry_value_t
on_crash_callback(
    const sentry_ucontext_t* uctx, sentry_value_t event, void* closure)
{
    (void)uctx;
    (void)closure;
    //report: crash info
    std::string errorCode = std::string("-1");
    std::string errorMsg = std::string();
#ifndef __APPLE__
    errorMsg = std::string("windows occur crash and the crash info please check server");
#else
    errorMsg = std::string("mac occur crash and the crash info please check server");
#endif

    //report: report crash info
    std::map<std::string, std::string> map;
    map.insert(std::make_pair(c_cr_error_code, errorCode));
    map.insert(std::make_pair(c_cr_error_msg, errorMsg));
    reportBuryEvent(e_crash_report, map, true);
         
    // tell the backend to retain the event
    return event;
}
#endif

void CreateDumpFile(LPCWSTR dumpFilePathName, EXCEPTION_POINTERS* pException) {
    HANDLE hDumpFile = CreateFileW(dumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    // Write minidump
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
}

LONG ExceptionCrashHandler(EXCEPTION_POINTERS* pException) {
    printf("ExceptionCrashHandler Enter\n");
    wchar_t exeDir[MAX_PATH];
    ::GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    std::wstring wsExeDir(exeDir);
    int nPos = wsExeDir.find_last_of('\\');
    std::wstring  wsDmpDir = wsExeDir.substr(0, nPos+1);
    wsDmpDir +=  L"dump";

    printf("wsDmpDir:%S\n", wsDmpDir.c_str());
    ::CreateDirectoryW(wsDmpDir.c_str(), NULL);
    SYSTEMTIME st;
    ::GetLocalTime(&st);
    WCHAR fileName[50] = L"\0";
    wsprintfW(fileName, L"\\eufyMake Studio_%d%02d%2d%_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring wsDmpFile = wsDmpDir + fileName;
    printf("wsDmpDir:%S\n", wsDmpFile.c_str()); 
    CreateDumpFile(wsDmpFile.c_str(), pException);

    if (ankerExceptionHandler != nullptr)
    {
        ankerExceptionHandler(pException);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif

#ifdef SLIC3R_GUI
class OpenGLVersionCheck
{
public:
    std::string version;
    std::string glsl_version;
    std::string vendor;
    std::string renderer;

    HINSTANCE   hOpenGL = nullptr;
    bool 		success = false;

    bool load_opengl_dll()
    {
        MSG      msg     = {0};
        WNDCLASS wc      = {0};
        wc.lpfnWndProc   = OpenGLVersionCheck::supports_opengl2_wndproc;
        wc.hInstance     = (HINSTANCE)GetModuleHandle(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = L"AnkerStudio_opengl_version_check";
        wc.style = CS_OWNDC;
        if (RegisterClass(&wc)) {
            HWND hwnd = CreateWindowW(wc.lpszClassName, L"AnkerStudio_opengl_version_check", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, wc.hInstance, (LPVOID)this);
            if (hwnd) {
                message_pump_exit = false;
                while (GetMessage(&msg, NULL, 0, 0 ) > 0 && ! message_pump_exit)
                    DispatchMessage(&msg);
            }
        }
        return this->success;
    }

    bool unload_opengl_dll()
    {
        if (this->hOpenGL != nullptr) {
            if (::FreeLibrary(this->hOpenGL) != FALSE) {
                if (::GetModuleHandle(L"opengl32.dll") == nullptr) {
                    printf("System OpenGL library successfully released\n");
                    this->hOpenGL = nullptr;
                    return true;
                }
                else
                    printf("System OpenGL library released but not removed\n");
            }
            else
                printf("System OpenGL library NOT released\n");

            return false;
        }

        return true;
    }

    bool is_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
    {
        // printf("is_version_greater_or_equal_to, version: %s\n", version.c_str());
        std::vector<std::string> tokens;
        boost::split(tokens, version, boost::is_any_of(" "), boost::token_compress_on);
        if (tokens.empty())
            return false;

        std::vector<std::string> numbers;
        boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);

        unsigned int gl_major = 0;
        unsigned int gl_minor = 0;
        if (numbers.size() > 0)
            gl_major = ::atoi(numbers[0].c_str());
        if (numbers.size() > 1)
            gl_minor = ::atoi(numbers[1].c_str());
        // printf("Major: %d, minor: %d\n", gl_major, gl_minor);

        std::map<std::string, std::string> map;
        map.insert(std::make_pair(c_major_version, std::to_string(gl_major)));
        map.insert(std::make_pair(c_minor_version, std::to_string(gl_minor)));
        reportBuryEvent(e_opengl_version, map);

        if (gl_major < major)
            return false;
        else if (gl_major > major)
            return true;
        else
            return gl_minor >= minor;
    }

protected:
    static bool message_pump_exit;

    void check(HWND hWnd)
    {
        hOpenGL = LoadLibraryExW(L"opengl32.dll", nullptr, 0);
        if (hOpenGL == nullptr) {
            printf("Failed loading the system opengl32.dll\n");
            return;
        }

        typedef HGLRC 		(WINAPI *Func_wglCreateContext)(HDC);
        typedef BOOL 		(WINAPI *Func_wglMakeCurrent  )(HDC, HGLRC);
        typedef BOOL     	(WINAPI *Func_wglDeleteContext)(HGLRC);
        typedef GLubyte* 	(WINAPI *Func_glGetString     )(GLenum);

        Func_wglCreateContext 	wglCreateContext = (Func_wglCreateContext)GetProcAddress(hOpenGL, "wglCreateContext");
        Func_wglMakeCurrent 	wglMakeCurrent 	 = (Func_wglMakeCurrent)  GetProcAddress(hOpenGL, "wglMakeCurrent");
        Func_wglDeleteContext 	wglDeleteContext = (Func_wglDeleteContext)GetProcAddress(hOpenGL, "wglDeleteContext");
        Func_glGetString 		glGetString 	 = (Func_glGetString)	  GetProcAddress(hOpenGL, "glGetString");

        if (wglCreateContext == nullptr || wglMakeCurrent == nullptr || wglDeleteContext == nullptr || glGetString == nullptr) {
            printf("Failed loading the system opengl32.dll: The library is invalid.\n");
            return;
        }

        PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,            	// The kind of framebuffer. RGBA or palette.
            32,                        	// Color depth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,                        	// Number of bits for the depthbuffer
            8,                        	// Number of bits for the stencilbuffer
            0,                        	// Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        HDC ourWindowHandleToDeviceContext = ::GetDC(hWnd);
        // Gdi32.dll
        int letWindowsChooseThisPixelFormat = ::ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);
        // Gdi32.dll
        SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);
        // Opengl32.dll
        HGLRC glcontext = wglCreateContext(ourWindowHandleToDeviceContext);
        wglMakeCurrent(ourWindowHandleToDeviceContext, glcontext);
        // Opengl32.dll
        const char *data = (const char*)glGetString(GL_VERSION);
        if (data != nullptr)
            this->version = data;
        // printf("check -version: %s\n", version.c_str());
        data = (const char*)glGetString(0x8B8C); // GL_SHADING_LANGUAGE_VERSION
        if (data != nullptr)
            this->glsl_version = data;
        data = (const char*)glGetString(GL_VENDOR);
        if (data != nullptr)
            this->vendor = data;
        data = (const char*)glGetString(GL_RENDERER);
        if (data != nullptr)
            this->renderer = data;
        // Opengl32.dll
        wglDeleteContext(glcontext);
        ::ReleaseDC(hWnd, ourWindowHandleToDeviceContext);
        this->success = true;
    }

    static LRESULT CALLBACK supports_opengl2_wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message)
        {
        case WM_CREATE:
        {
            CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            OpenGLVersionCheck *ogl_data = reinterpret_cast<OpenGLVersionCheck*>(pCreate->lpCreateParams);
            ogl_data->check(hWnd);
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_NCDESTROY:
            message_pump_exit = true;
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
};

bool OpenGLVersionCheck::message_pump_exit = false;
#endif /* SLIC3R_GUI */

std::string getVersinInfo()
{
    std::string ret = "0.0.0";

    //get exe path
    TCHAR productPath[MAX_PATH] = { 0 };
    if (GetModuleFileName(nullptr, productPath, sizeof(productPath) / sizeof(char)) == 0)
    {
        std::cout << "get exe info fail about " << GetLastError() << std::endl;
        return ret;
    }

    // get version
    uint32_t size = 0;
    size = GetFileVersionInfoSize(productPath, NULL);
    if (size == 0)
    {
        std::cout << "get version info fail about " << GetLastError() << std::endl;
        return ret;
    }

    // load version info
    BYTE* pData = new BYTE[size];
    if (!GetFileVersionInfo(productPath, NULL, size, (LPVOID)pData)) {
        std::cout << "load version info fail about " << GetLastError() << std::endl;
        return ret;
    }

    // get product name version info
    LPVOID lpBuffer;
    UINT uLength;
    DWORD dwVerMS;
    DWORD dwVerLS;

    if (VerQueryValue((LPVOID)pData, _T("\\"), &lpBuffer, &uLength)) {
        dwVerMS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionMS;
        dwVerLS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionLS;
    }
    else {
        std::cout << "get version info fail about "<< GetLastError() << std::endl;
        return ret;
    }

    ret = std::to_string(dwVerMS >> 16) + "." + std::to_string(dwVerMS & 0xFFFF) + "."
        + std::to_string(dwVerLS >> 16);
    return ret;

}

//add sentry by alves
#ifdef WIN32
void initSentry()
{
#ifndef OPEN_SOURCE_MODE
    sentry_options_t* options = sentry_options_new();
    {
#ifdef WIN32
        std::string dsn = std::string("");
        getSentryDsn(dsn);
        
        sentry_options_set_dsn(options, dsn.c_str());

        wchar_t exeDir[MAX_PATH];
        ::GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
        std::wstring wsExeDir(exeDir);
        int nPos = wsExeDir.find_last_of('\\');
        std::wstring  wsDmpDir = wsExeDir.substr(0, nPos + 1);

        std::wstring handlerDir = wsDmpDir + L"crashpad_handler.exe";
        wsDmpDir += L"dump";

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

        std::string desDir = wstringTostring(handlerDir);
        if (!desDir.empty())
            sentry_options_set_handler_path(options, desDir.c_str());
        desDir = wstringTostring(wsDmpDir);
        if (!desDir.empty())
            sentry_options_set_database_path(options, desDir.c_str());
#endif        
        std::string softVersion = "AnkerStudio " + getVersinInfo();
        sentry_options_set_release(options, softVersion.c_str());

#if defined(_DEBUG) || !defined(NDEBUG)
        sentry_options_set_debug(options, 1);
#else
        sentry_options_set_debug(options, 0);
#endif
        //release version environment(Testing/production/development/Staging)
        sentry_options_set_environment(options, "production");
        sentry_options_set_auto_session_tracking(options, false);
        sentry_options_set_symbolize_stacktraces(options, true);
        sentry_options_set_on_crash(options, on_crash_callback, NULL);
        sentry_init(options);
        sentry_start_session();

        DWORD processID = GetCurrentProcessId();
        sentry_set_tag("PID", std::to_string(processID).c_str());

        auto pcName = getPcName();
        auto macAddress = getMacAddress();

        sentry_set_tag("computer_name", pcName.c_str());
        sentry_set_tag("mac_address", macAddress.c_str());
    }
#endif
}
#endif

extern "C" {
#ifdef SLIC3R_WRAPPER_NOCONSOLE
int APIENTRY wWinMain(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */, PWSTR /* lpCmdLine */, int /* nCmdShow */)
{
    int 	  argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#else
int wmain(int argc, wchar_t **argv)
{
#endif
    // Allow the asserts to open message box, such message box allows to ignore the assert and continue with the application.
    // Without this call, the seemingly same message box is being opened by the abort() function, but that is too late and
    // the application will be killed even if "Ignore" button is pressed.
    _set_error_mode(_OUT_TO_MSGBOX);
    
    initBuryPoint();
    //report start soft
    std::string errorCode = std::string("0");
    std::string errorMsg = std::string("start soft");

    std::map<std::string, std::string> map;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    long long timestamp = now_ms.count();

    std::string strDuration = std::to_string(timestamp);
    map.insert(std::make_pair(c_ss_status, "start"));
    map.insert(std::make_pair(c_ss_time, strDuration));
    map.insert(std::make_pair(c_ss_error_code, errorCode));
    map.insert(std::make_pair(c_ss_error_msg, errorMsg));

    reportBuryEvent(e_start_soft, map);
#if defined(_DEBUG) || !defined(NDEBUG)
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ExceptionCrashHandler);
#else
    initSentry();
#endif    
    
    std::vector<wchar_t*> argv_extended;
    argv_extended.emplace_back(argv[0]);

#ifdef SLIC3R_WRAPPER_GCODEVIEWER
    wchar_t gcodeviewer_param[] = L"--gcodeviewer";
    argv_extended.emplace_back(gcodeviewer_param);
#endif /* SLIC3R_WRAPPER_GCODEVIEWER */

#ifdef SLIC3R_GUI
    // Here one may push some additional parameters based on the wrapper type.
    bool force_mesa = false;
    bool force_hw   = false;
#endif /* SLIC3R_GUI */
    for (int i = 1; i < argc; ++ i) {
#ifdef SLIC3R_GUI
        if (wcscmp(argv[i], L"--sw-renderer") == 0)
            force_mesa = true;
        else if (wcscmp(argv[i], L"--no-sw-renderer") == 0)
            force_hw = true;
#endif /* SLIC3R_GUI */
        argv_extended.emplace_back(argv[i]);
    }
    argv_extended.emplace_back(nullptr);

#ifdef SLIC3R_GUI
    OpenGLVersionCheck opengl_version_check;
    bool load_mesa =
        // Forced from the command line.
        force_mesa ||
        // Running over a rempote desktop, and the RemoteFX is not enabled, therefore Windows will only provide SW OpenGL 1.1 context.
        // In that case, use Mesa.
        (::GetSystemMetrics(SM_REMOTESESSION) && !force_hw) ||
        // Try to load the default OpenGL driver and test its context version.
        ! opengl_version_check.load_opengl_dll() || ! opengl_version_check.is_version_greater_or_equal_to(2, 0);
#endif /* SLIC3R_GUI */

    wchar_t path_to_exe[MAX_PATH + 1] = { 0 };
    ::GetModuleFileNameW(nullptr, path_to_exe, MAX_PATH);
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];
    wchar_t fname[_MAX_FNAME];
    wchar_t ext[_MAX_EXT];
    _wsplitpath(path_to_exe, drive, dir, fname, ext);
    _wmakepath(path_to_exe, drive, dir, nullptr, nullptr);

#ifdef SLIC3R_GUI
// https://wiki.qt.io/Cross_compiling_Mesa_for_Windows
// http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/
    if (load_mesa) {
        bool res = opengl_version_check.unload_opengl_dll();
        if (!res) {
            MessageBox(nullptr, L"eufyMake Studio was unable to automatically switch to MESA OpenGL library\nPlease, try to run the application using the '--sw-renderer' option.\n",
                L"eufyMake Studio Warning", MB_OK);
            return -1;
        }
        else {
            wchar_t path_to_mesa[MAX_PATH + 1] = { 0 };
            wcscpy(path_to_mesa, path_to_exe);
            wcscat(path_to_mesa, L"mesa\\opengl32.dll");
            printf("Loading MESA OpenGL library: %S\n", path_to_mesa);
            HINSTANCE hInstance_OpenGL = LoadLibraryExW(path_to_mesa, nullptr, 0);
            if (hInstance_OpenGL == nullptr)
                printf("MESA OpenGL library was not loaded\n");
            else
                printf("MESA OpenGL library was loaded sucessfully\n");
        }
    }
#endif /* SLIC3R_GUI */

    wchar_t path_to_slic3r[MAX_PATH + 1] = { 0 };
    wcscpy(path_to_slic3r, path_to_exe);
    wcscat(path_to_slic3r, L"eufyStudio.dll");
    HINSTANCE hInstance_Slic3r = LoadLibraryExW(path_to_slic3r, nullptr, 0);
    if (hInstance_Slic3r == nullptr) {
        DWORD error = GetLastError();
        printf("eufyStudio.dll was not loaded, error: %d\n", error);
#ifndef OPEN_SOURCE_MODE
        sentry_end_session();
        sentry_shutdown();
#endif
        return -1;
    }

    // resolve function address here
    ankerExceptionHandler = (AnkerExceptionHandler)GetProcAddress(hInstance_Slic3r,
#ifdef _WIN64
        // there is just a single calling conversion, therefore no mangling of the function name.
        "AnkerExceptionHandler"
#else	// stdcall calling convention declaration
        "_slic3r_AnkerExceptionHandler@4"
#endif
    );
    if (ankerExceptionHandler == nullptr) {
        printf("could not locate the function ankerExceptionHandler in eufyStudio.dll\n");
#ifndef OPEN_SOURCE_MODE
        sentry_end_session();
        sentry_shutdown();
#endif
        return -1;
    }

    // resolve function address here
    slic3r_main = (Slic3rMainFunc)GetProcAddress(hInstance_Slic3r,
#ifdef _WIN64
        // there is just a single calling conversion, therefore no mangling of the function name.
        "slic3r_main"
#else	// stdcall calling convention declaration
        "_slic3r_main@8"
#endif
        );
    if (slic3r_main == nullptr) {
        printf("could not locate the function slic3r_main in eufyStudio.dll\n");
#ifndef OPEN_SOURCE_MODE
        sentry_end_session();
        sentry_shutdown();
#endif
        return -1;
    }

    // argc minus the trailing nullptr of the argv
    auto res = slic3r_main((int)argv_extended.size() - 1, argv_extended.data());

#ifndef OPEN_SOURCE_MODE
    sentry_end_session();
    sentry_shutdown();
#endif
    return res;
}
}

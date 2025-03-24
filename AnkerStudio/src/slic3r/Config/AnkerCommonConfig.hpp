#pragma once
#include <string>
#include <boost/filesystem.hpp>

namespace Slic3r {
    namespace UrlConfig {
        // support

        const std::string EufyMakerSupport = "https://support.eufymake.com/s/";
        const std::string SupportUrl = "https://support.eufymake.com/";

        const std::string PreviewData = "https://support.eufymake.com/s/article/How-to-Fix-WiFi-Connection-Issue";
        const std::string PrintCheckFailedGuide = "https://support.eufymake.com/s/article/How-to-Fix-WiFi-Connection-Issue";

        const std::string WifiFixUrl = "https://support.eufymake.com/s/article/How-to-Fix-WiFi-Connection-Issue";
        const std::string StudioGuideUrl = "https://support.eufymake.com/s/article/Ankermake-Studio-Guide-for-printer#content6";
        
        // net download
        const std::string NetUrl = "https://make-app.ankermake.com/v1/slicer/get_net";
        const std::string NetUrl_EU = "https://make-app-eu.ankermake.com/v1/slicer/get_net";  
        const std::string NetUrl_QA = "https://make-app-us-qa.eufylife.com/v1/slicer/get_net";
    
        // login & register
        const std::string OldLoginUrl = "https://community.ankermake.com/";
        const std::string MulpassRegisterUrl = "https://passport.eufymake.com/?app=eufymake-us";
         
        // privacy & release
        const std::string PrivacyRequestUrl = "https://passport.eufymake.com/privacy-request?app=eufymake-us";
        const std::string ReleaseUrl = "https://github.com/eufymake/eufyMake-PrusaSlicer-Release/releases";                
        const std::string TermsOfServiceJaUrl = "https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/terms_of_use_jp.html";
        const std::string TermsOfServiceEnUrl = "https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/terms_of_use_en.html";

        const std::string PrivacyNoticeEn = "https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_en.html";
        const std::string PrivacyNoticeJa = "https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_jp.html";
    }

    namespace BrandConfig {
        const std::string BrandName = "eufyMake Studio";
        const std::wstring StudioDllName = L"eufyStudio.dll";
        const std::wstring OldStudioRunName = L"AnkerMake";

        const std::wstring StudioExeName = L"eufymake studio.exe";
        const std::wstring StudioConsoleExeName = L"eufymake studio-console.exe";  
        const std::wstring GcodeViewerExeName = L"anker - gcodeviewer.exe";
    }

    namespace ProfileConfig {
        const std::string ProfileDirName = "eufyMake Studio Profile";
        const std::string OldProfileDirName = "AnkerMake Studio Profile";

        const std::string OldIniName = "AnkerMake Studio_23.ini";
        static std::string iniName = OldIniName;

        const std::string gcodeViewerIniName = "AnkerMake StudioGcodeViewer.ini";
    }

    namespace ServerConfig {
        const std::string AppName = "AnkerMake Studio";

    }

    namespace LogConfig {
        const std::string MacLogPath = "/Users/eufyMake/Logs";
        const std::string MacLogPathPrefix = "/Library/Logs/eufyMake";
    }
 
    namespace DeviceConfig {
        const std::string M5Name = "AnkerMake M5";
        const std::string M5CName = "AnkerMake M5C";
    }

    namespace WebConfig {
        const std::string UrlProtocol = "eufystudio://open";

        const std::string EBWebViewCacheName = "EBWebView";
        static boost::filesystem::path EBWebViewCacheDir;

        // for mac
        const std::string oldMacWebViewCacheName = "com.anker.pcankermake";
        const std::string MacWebViewCacheName = "com.anker.pceufymake";
        const std::string MacWebViewDataName = "eufyStudio";

        // /Users/user/Library/Caches/com.anker.pcankermake
        static boost::filesystem::path MacWebViewCacheDir;
        // /Users/user/Library/WebKit/com.anker.pcankermake
        static boost::filesystem::path MacWebViewDataDir;
    }
}

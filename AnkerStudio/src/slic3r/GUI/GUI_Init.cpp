#include "libslic3r/Technologies.hpp"
#include "GUI_Init.hpp"


#include <string>
#ifdef _WIN32
#include <codecvt>
#include <xlocbuf>
#endif

#include "libslic3r/AppConfig.hpp" 

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/InstanceCheck.hpp" 
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/I18N.hpp"


// To show a message box if GUI initialization ends up with an exception thrown.
#include <wx/msgdlg.h>

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/convert.hpp>

#if __APPLE__
    #include <signal.h>
#endif // __APPLE__

namespace Slic3r {
namespace GUI {

const std::vector<std::string> OpenGLVersions::core_str    = { "3.2", "3.3", "4.0", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6" };
const std::vector<std::string> OpenGLVersions::precore_str = { "2.0", "2.1", "3.0", "3.1" };

const std::vector<std::pair<int, int>> OpenGLVersions::core    = { {3,2}, {3,3}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6} };
const std::vector<std::pair<int, int>> OpenGLVersions::precore = { {2,0}, {2,1}, {3,0}, {3,1} };

void write_url_file(const std::string &url) 
{
    if (url.empty()) {
        ANKER_LOG_INFO << "download url is empty";
        return;
    }

    boost::filesystem::path tempPath(wxStandardPaths::Get().GetTempDir().utf8_str().data());
    ANKER_LOG_INFO << "tempGcodePath is " << tempPath.string().c_str();
    bool bExists = boost::filesystem::exists(tempPath);
    if (!bExists) {
        ANKER_LOG_INFO << "write url file failed " << tempPath.string().c_str() << " is not exist";
        return;
    }

    auto generatePath = []() {
        boost::filesystem::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
        temp_path /= boost::format("download_url.txt").str();
        return temp_path.string();
    };

    std::ofstream file;
    file.open(generatePath(), std::ios::out | std::ios::trunc);
    if (file.is_open()) { 
        file << url;
        file.close();
    }
    else {
        ANKER_LOG_INFO << "file open failed to write" ;
    }
}

void delete_file()
{
    boost::filesystem::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
    temp_path /= boost::format("download_url.txt").str();
    if (boost::filesystem::exists(temp_path)) {
        boost::filesystem::remove(temp_path);
    }
}


int GUI_Run(GUI_InitParams &params)
{
    ANKER_LOG_INFO << "start run  ui ";

#if __APPLE__
    // On OSX, we use boost::process::spawn() to launch new instances of AnkerStudio from another AnkerStudio.
    // boost::process::spawn() sets SIGCHLD to SIGIGN for the child process, thus if a child AnkerStudio spawns another
    // subprocess and the subrocess dies, the child AnkerStudio will not receive information on end of subprocess
    // (posix waitpid() call will always fail).
    // https://jmmv.dev/2008/10/boostprocess-and-sigchld.html
    // The child instance of AnkerStudio has to reset SIGCHLD to its default, so that posix waitpid() and similar continue to work.
    // See GH issue #5507
    signal(SIGCHLD, SIG_DFL);
#endif // __APPLE__

    delete_file();
    ANKER_LOG_INFO << "Single instance detection before startup";
    try {
        GUI::GUI_App* gui = new GUI::GUI_App(params.start_as_gcodeviewer ? GUI::GUI_App::EAppMode::GCodeViewer : GUI::GUI_App::EAppMode::Editor);
        if (gui->get_app_mode() != GUI::GUI_App::EAppMode::GCodeViewer) {
            // G-code viewer is currently not performing instance check, a new G-code viewer is started every time.
            bool gui_single_instance_setting = gui->app_config->get_bool("single_instance");
            if (Slic3r::instance_check(params.argc, params.argv, true)) {
                write_url_file(params.download_url);  //write url
                //TODO: do we have delete gui and other stuff?
                ANKER_LOG_INFO << "The software is configured in single instance mode and a running instance already exists,just exit this instance";
                return -1;
            }
        }

        GUI::GUI_App::SetInstance(gui);
        gui->init_params = &params;
        ANKER_LOG_INFO << "wxEntry call";

#ifdef WIN32
        std::vector<wchar_t*> wptr;
        std::vector<std::wstring> 	argv_narrow;
        for (int i = 0 ; i < params.argc ; i++)
        {
            std::string str = params.argv[i];
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            argv_narrow.push_back(converter.from_bytes(str));
            wptr.push_back(argv_narrow[i].data());
        }
        return wxEntry(params.argc, wptr.data());

#else
        return wxEntry(params.argc, params.argv);
#endif

    } catch (const Slic3r::Exception &ex) {
        boost::nowide::cerr << ex.what() << std::endl;
        wxMessageBox(boost::nowide::widen(ex.what()), _L("eufyMake Studio GUI initialization failed"), wxICON_STOP);
    } catch (const std::exception &ex) {
        boost::nowide::cerr << "eufyMake Studio GUI initialization failed: " << ex.what() << std::endl;
        wxMessageBox(format_wxstr(_L("Fatal error, exception catched: %1%"), ex.what()), _L("eufyMake Studio GUI initialization failed"), wxICON_STOP);
    }

    // error
    return 1;
}
    
}
}

#include "AnkerBase.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>


#ifdef _WIN32
std::string AnkerBase::AnkerResourceIconPath = "/resources/icons/";
#elif __APPLE__
std::string AnkerBase::AnkerResourceIconPath = "/../Resources/icons/";
#endif

AnkerBase::AnkerBase()
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxString applicationPath = wxFileName(exePath).GetPath();
    std::string iconPath = applicationPath.ToStdString();
#ifdef _WIN32
    iconPath += "/resources/icons/";
#elif __APPLE__
    iconPath += "/../Resources/icons/";
#endif
    AnkerResourceIconPath = iconPath;
}

AnkerBase::~AnkerBase()
{
}

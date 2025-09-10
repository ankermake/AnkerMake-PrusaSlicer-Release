#include "AnkerBase.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>


#ifdef _WIN32
wxString AnkerBase::AnkerResourceIconPath = "/resources/icons/";
#elif __APPLE__
wxString AnkerBase::AnkerResourceIconPath = "/../Resources/icons/";
#endif

AnkerBase::AnkerBase()
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxString applicationPath = wxFileName(exePath).GetPath();
#ifdef _WIN32
    applicationPath += "/resources/icons/";
#elif __APPLE__
    applicationPath += "/../Resources/icons/";
#endif
    AnkerResourceIconPath = applicationPath;
}

AnkerBase::~AnkerBase()
{
}

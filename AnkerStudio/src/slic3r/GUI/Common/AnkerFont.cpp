#include "libslic3r/Utils.hpp"
#include "AnkerFont.hpp"



 void AnkerGlobalFont::initSysFont()
 {
#ifdef __linux__
     const std::string& resource_path = Slic3r::resources_dir();
     wxString font_path = wxString::FromUTF8(resource_path + "/fonts/HarmonyOS_Sans_SC_Bold.ttf");
     bool result = wxFont::AddPrivateFont(font_path);
     printf("add font of HarmonyOS_Sans_SC_Bold returns %d\n", result);
     font_path = wxString::FromUTF8(resource_path + "/fonts/HarmonyOS_Sans_SC_Regular.ttf");
     result = wxFont::AddPrivateFont(font_path);
     printf("add font of HarmonyOS_Sans_SC_Regular returns %d\n", result);
#endif

     Font_Body_14 = sysFont(14, 400);
     Font_Body_13 = sysFont(13, 400);
     Font_Body_12 = sysFont(12, 400);
     Font_Body_10 = sysFont(10, 400);

     Font_Head_14 = sysFont(14, 500);
     Font_Head_12 = sysFont(14, 500);
 }



wxFont AnkerGlobalFont::sysFont(int size, bool bold)
{
#ifndef __APPLE__
    size = size * 4 / 5;
#endif

    auto   face = wxString::FromUTF8("HarmonyOS Sans SC");
    wxFont font{ size, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL, false, face };
    font.SetFaceName(face);
    if (!font.IsOk()) 
    {
        font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        if (bold) font.MakeBold();
        font.SetPointSize(size);
    }
    return font;
}

wxFont AnkerGlobalFont::sysFont(int size, int weight)
{
#ifndef __APPLE__
    size = size * 4 / 5;
#endif

    auto   face = wxString::FromUTF8("HarmonyOS Sans SC");
    wxFont font{ size, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, weight, false, face };
    font.SetFaceName(face);
    if (!font.IsOk())
    {
        font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        font.SetPointSize(size);
    }
    return font;
}

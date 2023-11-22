#ifndef ANKER_MENU_BAR_HPP
#define ANKER_MENU_BAR_HPP

#include <wx/menu.h>
#include <wx/bitmap.h>


class AnkerIconMenu : public wxMenu
{
public:
    AnkerIconMenu(const wxBitmap& icon, const std::string& title = "");
    wxBitmap getIcon() const;
private:
    wxBitmap m_icon;
};

class AnkerMenuBar : public wxMenuBar
{
public:
    AnkerMenuBar();
    // unused under MSW
    AnkerMenuBar(long style);
    // menubar takes ownership of the menus arrays but copies the titles
    AnkerMenuBar(size_t n, wxMenu* menus[], const wxString titles[], long style = 0);
    
private:
    void OnPaint(wxPaintEvent& event);
};



#endif // !ANKER_MENU_BAR_HPP

#include "AnkerMenuBar.hpp"

AnkerIconMenu::AnkerIconMenu(const wxBitmap& icon, const std::string& title) : 
    wxMenu(title),
    m_icon(icon)
{
}

wxBitmap AnkerIconMenu::getIcon() const
{
    return m_icon;
}


AnkerMenuBar::AnkerMenuBar() : wxMenuBar()
{
    Bind(wxEVT_PAINT, &AnkerMenuBar::OnPaint, this);
}

AnkerMenuBar::AnkerMenuBar(long style) : wxMenuBar(style)
{
}

AnkerMenuBar::AnkerMenuBar(size_t n, wxMenu* menus[], const wxString titles[], long style) :
    wxMenuBar(n, menus, titles, style)
{
}

void AnkerMenuBar::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    dc.SetBackground(wxBrush(wxColour(GetBackgroundColour())));
    dc.Clear();

    int menuCount = GetMenuCount();
    int menuItemWidth = GetSize().GetWidth() / menuCount;
    int menuItemHeight = GetSize().GetHeight();
    for (int i = 0; i < menuCount; i++) {
        wxMenu* menu = GetMenu(i);
        if (menu) {
            AnkerIconMenu* iconMenu = dynamic_cast<AnkerIconMenu*>(menu);
            if (iconMenu) {
                wxBitmap icon = iconMenu->getIcon();
                dc.DrawBitmap(icon, wxPoint(menuItemWidth * i, 0), true);
                dc.SetTextForeground(GetForegroundColour());
                dc.DrawText(iconMenu->GetTitle(), wxPoint(menuItemWidth * i + icon.GetWidth(), 0));
            }
            else {
                dc.SetTextForeground(GetForegroundColour());
                dc.DrawText(iconMenu->GetTitle(), wxPoint(menuItemWidth * i, 0));
            }
        }
    }


}

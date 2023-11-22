#include "AnkerButton.hpp"

AnkerButton::AnkerButton()
{
}

AnkerButton::AnkerButton(wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size, long style, 
    const wxValidator& validator, const wxString& name) :
    wxButton(parent, id, label, pos, size, style, validator, name)
{
    SetWindowStyleFlag(wxNO_BORDER);
}

AnkerButton::~AnkerButton()
{
}

void AnkerButton::setIcon(const wxIcon& icon)
{
    SetBitmap(icon);
}

void AnkerButton::setBitmapPressed(const wxIcon& icon)
{
    SetBitmapPressed(icon);
}

void AnkerButton::setBitmapCurrent(const wxIcon& icon)
{
    SetBitmapCurrent(icon);
}

AnkerOkButton::AnkerOkButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size)  :
    AnkerButton(parent, id, label, pos, size, wxBORDER_NONE)
{
    SetBackgroundColour(wxColour("#62D361"));
    SetForegroundColour(wxColour("#FFFFFF"));
    SetOwnBackgroundColour(wxColour("#62D361"));
    wxFont font = ANKER_BOLD_FONT_NO_1;
    SetFont(font);
}

AnkerCancelButton::AnkerCancelButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size) :
    AnkerButton(parent, id, label, pos, size, wxBORDER_NONE)
{
    wxFont cancelFont = ANKER_BOLD_FONT_NO_1;
    SetFont(cancelFont);
    SetForegroundColour(wxColour("#FFFFFF"));
    SetBackgroundColour(wxColour(89, 90, 94));
}

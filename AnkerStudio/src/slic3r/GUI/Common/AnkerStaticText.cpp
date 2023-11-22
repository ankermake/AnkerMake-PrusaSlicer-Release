#include "AnkerStaticText.hpp"

AnkerStaticText::AnkerStaticText()
{
}

AnkerStaticText::AnkerStaticText(
    wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
    wxStaticText(parent, id, label, pos, size, style, name)
{
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(wxColour("#292A2D"));
    }
}

AnkerStaticText::~AnkerStaticText()
{
}

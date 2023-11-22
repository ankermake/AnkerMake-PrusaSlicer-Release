#ifndef ANKER_BUTTON_HPP
#define ANKER_BUTTON_HPP

#include <wx/wx.h>
#include "AnkerBase.hpp"

class AnkerButton : public wxButton, public AnkerBase
{
public:
    AnkerButton();
	AnkerButton(wxWindow* parent,
        wxWindowID id,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxButtonNameStr));
	~AnkerButton();

    void setIcon(const wxIcon& icon);
    void setBitmapPressed(const wxIcon& icon);
    void setBitmapCurrent(const wxIcon& icon);

private:


};


class AnkerOkButton : public AnkerButton
{
public:
    AnkerOkButton(wxWindow* parent, wxWindowID id,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);
};

class AnkerCancelButton : public AnkerButton
{
public:
    AnkerCancelButton(wxWindow* parent, wxWindowID id,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);
};



#endif // !ANKER_BUTTON_HPP

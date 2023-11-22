#ifndef ANKER_STATIC_TEXT_HPP_
#define ANKER_STATIC_TEXT_HPP_


#include <wx/wx.h> 
class AnkerStaticText : public wxStaticText
{
public:
    AnkerStaticText();
	AnkerStaticText(wxWindow* parent,
        wxWindowID id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxASCII_STR(wxStaticTextNameStr));
	~AnkerStaticText();

private:

};



#endif // !ANKER_STATIC_TEXT_HPP_

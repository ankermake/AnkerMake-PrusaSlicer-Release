#ifndef _ANKER_LINE_EDIT_HPP_
#define _ANKER_LINE_EDIT_HPP_

#include <wx/textctrl.h>

wxDECLARE_EVENT(wxCUSTOMEVT_EDIT_FINISHED, wxCommandEvent);

class AnkerLineEdit :public wxTextCtrl
{
public:
	AnkerLineEdit(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = 0, const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxTextCtrlNameStr);
	~AnkerLineEdit();
protected:
	void OnKillFocus(wxFocusEvent& event);
private:
};


#endif
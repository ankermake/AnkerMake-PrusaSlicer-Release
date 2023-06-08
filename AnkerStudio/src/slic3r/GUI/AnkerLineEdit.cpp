#include "AnkerLineEdit.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_EDIT_FINISHED, wxCommandEvent);
AnkerLineEdit::AnkerLineEdit(wxWindow* parent, 
	wxWindowID id, 
	const wxString& value /*= wxEmptyString*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/, 
	long style /*= 0*/,
	const wxValidator& validator /*= wxDefaultValidator*/,
	const wxString& name /*= wxTextCtrlNameStr*/)
	: wxTextCtrl(parent, id, value, pos, size, style, validator, name)
{
	Bind(wxEVT_KILL_FOCUS, &AnkerLineEdit::OnKillFocus, this);
}

AnkerLineEdit::~AnkerLineEdit()
{

}

void AnkerLineEdit::OnKillFocus(wxFocusEvent& event)
{	

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_FINISHED);
	ProcessEvent(evt);
	
	event.Skip();
}

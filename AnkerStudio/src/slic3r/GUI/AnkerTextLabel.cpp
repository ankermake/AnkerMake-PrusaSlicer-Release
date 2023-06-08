#include "AnkerTextLabel.hpp"
#include "AnkerCustomEvent.hpp"

BEGIN_EVENT_TABLE(AnkerTextLabel, wxStaticText)
EVT_ENTER_WINDOW(AnkerTextLabel::OnEnter)
EVT_LEFT_DOWN(AnkerTextLabel::OnClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerTextLabel, wxStaticText)

AnkerTextLabel::AnkerTextLabel(wxWindow* parent,
	wxWindowID id,
	const wxString& label,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	long style /*= 0*/)
	: wxStaticText(parent, id, label, pos, size, style)
{

}

AnkerTextLabel::AnkerTextLabel()
{

}

void AnkerTextLabel::OnEnter(wxMouseEvent& event)
{
	wxCommandEvent evt(ANKER_CUSTOM_ENTER, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}

void AnkerTextLabel::OnClick(wxMouseEvent& event)
{
	wxCommandEvent evt(wxEVT_BUTTON, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}
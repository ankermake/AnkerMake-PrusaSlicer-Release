#include "AnkerLineEdit.hpp"
#include "Common/AnkerValidator.hpp"
#include "Common/AnkerFont.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_EDIT_FINISHED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_EDIT_ENTER, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_EDIT_FOCUS, wxCommandEvent);
AnkerLineEdit::AnkerLineEdit(wxWindow* parent, 
	wxWindowID id, 
	const wxString& value /*= wxEmptyString*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/, 
	long style /*= 0*/,
	const wxValidator& validator /*= wxDefaultValidator*/,
	const wxString& name /*= wxTextCtrlNameStr*/)
	: wxRichTextCtrl(parent, id, value, pos, size, style, validator, name)
	, m_background_color(wxColour(41, 42, 45)), m_text_color(wxColour(*wxWHITE))
{
	//EnableVerticalScrollbar(false);  //Do not do this, it will cause the program to crash
	ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
	SetTextColor(*wxWHITE);

	Unbind(wxEVT_PAINT, &wxRichTextCtrl::OnPaint, this);
	Bind(wxEVT_KILL_FOCUS, &AnkerLineEdit::OnKillFocus, this);
	Bind(wxEVT_SET_FOCUS, &AnkerLineEdit::OnSetFocus, this);
	Bind(wxEVT_KEY_DOWN, &AnkerLineEdit::OnKeyDown, this);
	Bind(wxEVT_PAINT, &AnkerLineEdit::OnPaint, this);
}

AnkerLineEdit::~AnkerLineEdit()
{

}

void AnkerLineEdit::SetTextColor(wxColour color)
{
	wxRichTextAttr attr{};
	attr.SetTextColour(color);
	SetBasicStyle(attr);
	m_text_color = color;
}

void AnkerLineEdit::SetForegroundColour(wxColour color)
{
	SetTextColor(color);
}

void AnkerLineEdit::SetBackgroundColour(wxColour color)
{
	m_background_color = color;
}

void AnkerLineEdit::AddValidatorInt(uint32_t min, uint32_t max)
{
	RichTextIntegerValidator<uint32_t> validator;
	validator.SetMin(min);
	validator.SetMax(max);
	wxRichTextCtrl::SetValidator(validator);
}

void AnkerLineEdit::AddValidatorFloat(float min, float max, int precision)
{
	RichTextFloatingPointValidator<float> validator;
	validator.SetMin(min);
	validator.SetMax(max);
	validator.SetPrecision(precision);
	SetValidator(validator);
}

wxString AnkerLineEdit::GetValue() const
{
	if (GetHint() == wxRichTextCtrl::GetValue()) {		
		return "";
	}

	return wxRichTextCtrl::GetValue();
}

bool AnkerLineEdit::SetFont(const wxFont& font)
{
	wxTextAttr textAttr;
	textAttr.SetFont(font);
	return SetDefaultStyle(textAttr);
}

void AnkerLineEdit::OnKillFocus(wxFocusEvent& event)
{	

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_FINISHED);
	ProcessEvent(evt);
	
	event.Skip();
}

void AnkerLineEdit::OnSetFocus(wxFocusEvent& event)
{
	//clean hint text
	if (GetHint() == wxRichTextCtrl::GetValue()) {
		wxEventBlocker blocker(this, wxEVT_TEXT);
		SetValue("");
	}
	SetInsertionPointEnd();
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_FOCUS);
	ProcessEvent(evt);
	event.Skip();
}

void AnkerLineEdit::OnKeyDown(wxKeyEvent& event)
{
	int keycode = event.GetKeyCode();
	if (keycode == WXK_RETURN || keycode == WXK_NUMPAD_ENTER)
	{
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_ENTER);
		ProcessEvent(evt);
#ifdef __APPLE__
		Navigate(); //trigger kill focus
#endif // __APPLE__
	}
	else
	{
		event.Skip();
	}
}

void AnkerLineEdit::OnPaint(wxPaintEvent& event)
{
	if (m_text_color != GetBasicStyle().GetTextColour()) {
		wxRichTextAttr attr{};
		attr.SetTextColour(m_text_color);
		SetBasicStyle(attr);
	}
	if (m_background_color != GetBackgroundColour()) {
		wxRichTextCtrl::SetBackgroundColour(m_background_color);
	}
	wxRichTextCtrl::OnPaint(event);
}
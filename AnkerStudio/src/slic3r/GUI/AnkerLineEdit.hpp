#ifndef _ANKER_LINE_EDIT_HPP_
#define _ANKER_LINE_EDIT_HPP_

#include <wx/richtext/richtextctrl.h>

wxDECLARE_EVENT(wxCUSTOMEVT_EDIT_FINISHED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_EDIT_ENTER, wxCommandEvent);
class AnkerLineEdit :public wxRichTextCtrl
{
public:
	AnkerLineEdit(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = 0, const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxTextCtrlNameStr);
	~AnkerLineEdit();

	void SetTextColor(wxColour color);
	void SetForegroundColour(wxColour color);
	void SetBackgroundColour(wxColour color);
	void AddValidatorInt(uint32_t min, uint32_t max);
	void AddValidatorFloat(float min, float max, int precision);
	wxString GetValue()const;
	bool SetFont(const wxFont& font);


protected:
	void OnKillFocus(wxFocusEvent& event);
	void OnSetFocus(wxFocusEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnPaint(wxPaintEvent& event);
private:
	wxColour m_background_color;
	wxColour m_text_color;
};


#endif
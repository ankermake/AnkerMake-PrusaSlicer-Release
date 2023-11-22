#ifndef _ANKER_HYPER_LINK_HPP_
#define _ANKER_HYPER_LINK_HPP_
#include "wx/wx.h"

class AnkerHyperlink : public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerHyperlink)
	DECLARE_EVENT_TABLE()
public:
	AnkerHyperlink();
	AnkerHyperlink(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxString& name = wxString(""),
		const wxString& urlLink = wxString(""),
		const wxColour& backgroudColor = wxColour("#1F2022"),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerHyperlink() {}

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnClick(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);

private:

	wxString m_link = wxString("");
	wxString m_text = wxString("");

};

#endif // !_ANKER_HYPER_LINK_HPP_



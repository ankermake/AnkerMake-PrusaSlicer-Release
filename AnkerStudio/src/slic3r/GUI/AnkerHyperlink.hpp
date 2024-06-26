#ifndef _ANKER_HYPER_LINK_HPP_
#define _ANKER_HYPER_LINK_HPP_
#include "wx/wx.h"


#define CustomActionFun std::function<void()>

enum TextAlignType {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
};

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
		const wxSize& size = wxDefaultSize,
		const TextAlignType& align = ALIGN_LEFT
	);

	~AnkerHyperlink() {}

	void SetCustumAction(CustomActionFun f);
	void SetWrapWidth(int w);

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnClick(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);
	void drawWrapText(wxPaintDC& dc, wxString& text, int wrapWidth);

private:

	wxString m_link = wxString("");
	wxString m_text = wxString("");
	int m_wrapWidth = -1;
	TextAlignType m_alignType{ ALIGN_LEFT };
	CustomActionFun CustomAction{ nullptr };
};

#endif // !_ANKER_HYPER_LINK_HPP_



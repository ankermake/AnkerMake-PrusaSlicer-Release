#ifndef _ANKER_HYPER_LINK_HPP_
#define _ANKER_HYPER_LINK_HPP_
#include "wx/wx.h"
#include "Common/AnkerGUIConfig.hpp"

#define CustomActionFun std::function<void()>

enum TextAlignType {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
};


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
	void SetCustomFont(const wxFont& font);
	void SetText(const wxString& text) { m_text = text; }
    
    void SetPrintFailFlag(bool f){ m_printflag = f; }


	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnClick(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);
	void drawWrapText(wxPaintDC& dc, wxString& text, int wrapWidth);
    void drawPrintFailWrapText(wxPaintDC& dc, wxString& text, int wrapWidth);

private:
	wxFont m_font { ANKER_FONT_NO_1 };
	wxString m_link = wxString("");
	wxString m_text = wxString("");
	int m_wrapWidth = -1;
    bool m_printflag = false;
	TextAlignType m_alignType{ ALIGN_LEFT };
	CustomActionFun CustomAction{ nullptr };
};

#endif // !_ANKER_HYPER_LINK_HPP_



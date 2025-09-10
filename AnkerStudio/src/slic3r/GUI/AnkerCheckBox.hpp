#ifndef _ANKER_CHECKBOX_HPP_
#define _ANKER_CHECKBOX_HPP_

#include "wx/wx.h"
#include <wx/control.h>

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, wxCommandEvent);

class AnkerCheckBox: public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerCheckBox)
	DECLARE_EVENT_TABLE()
public:
	AnkerCheckBox();
	AnkerCheckBox(wxWindow* parent,
				  wxImage uncheckImg,
				  wxImage checkImg,
				  wxImage disuncheckImg,
				  wxImage discheckImg,
				  wxString text,
				  wxFont   font,
				  wxColour color,
				  wxWindowID winid = wxID_ANY,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize);
	~AnkerCheckBox();


	void setBgColor(wxColour bgColor);
	void SetFont(wxFont font);
	bool Enable(bool enable = true);
	void Disable();
	void setText(const wxString& text);
	void setCheckStatus(bool isCheck);
	void SetValue(bool isCheck);
	bool GetValue();
	bool getCheckStatus();
	void openSupport();
	virtual void OnPressed(wxMouseEvent& event);
	void OnDelayTimer(wxTimerEvent& event);
	void EnabelDelayTimer(bool bEnabel) { m_bUseDelayTimer = bEnabel;}
protected:
	void OnPaint(wxPaintEvent& event);
private:
	bool	 m_ischeck = false;
	wxImage  m_uncheckImg;
	wxImage  m_checkImg;
	wxImage  m_disUncheckImg;
	wxImage  m_disCheckImg;
	wxString m_text;
	wxFont   m_textFont;
	wxColour m_Color;
	wxColour m_bgColor;
	//by samuel, add a delay timer to avoid response click event frequently
	wxTimer delayTimer;
	bool	m_bUseDelayTimer = true;

};
#endif


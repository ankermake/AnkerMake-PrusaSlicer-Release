#ifndef _ANKER_BTN_H_
#define _ANKER_BTN_H_

#include "wx/wx.h"

enum BtnStatus
{
	NormalBtnStatus,
	EnterBtnStatus,
	PressedBtnStatus,
	UpBtnStatus,
	LeaveBtnStatus,
	DClickBtnStatus,
	DisableBtnStatus
};

class AnkerBtn : public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerBtn)
	DECLARE_EVENT_TABLE()
public:
	AnkerBtn();
	virtual ~AnkerBtn();

	AnkerBtn(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxValidator& validator = wxDefaultValidator);

	void SetRadius(const double &radius);

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
	virtual void OnUp(wxMouseEvent& event);
	virtual bool Enable(bool enable = true);		
	
	void SetText(const wxString& text);
	bool SetFont(const wxFont& font);
	void SetTextColor(const wxColor& textColor);

	void SetNorImg(wxBitmap* bitmap);
	void SetPressedImg(wxBitmap* bitmap);
	void SetEnterImg(wxBitmap* bitmap);
	void SetDisableImg(wxBitmap* bitmap);
	void SetBackground(const wxBitmap& bitmap);

	bool SetBackgroundColour(const wxColour& colour);

protected:
	bool Create(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator& validator = wxDefaultValidator);

	void DrawImg(wxDC* dc, wxBitmap* image1, wxBitmap* exist_image);
	void OnPaint(wxPaintEvent& event);
private:
	void DrawBackground(wxDC* dc);

	wxBitmap m_backgroundImg;
	bool m_isUsedBg;

	wxBitmap* m_norImg;
	wxBitmap* m_pressedImg;
	wxBitmap* m_enterImg;
	wxBitmap* m_disableImg;

	double m_radius;
	int m_btnStatus;
	wxString m_btnText;
	wxFont m_btnTextFont;
	wxColor m_textColor;
};

#endif

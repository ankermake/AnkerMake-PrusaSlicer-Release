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

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_BTN_CLICKED, wxCommandEvent);
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

	void SetStatus(BtnStatus status);
	void SetRadius(const double& radius);
	void clearImg();
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
	virtual void OnUp(wxMouseEvent& event);
	virtual bool Enable(bool enable = true);
	virtual void OnSize(wxSizeEvent& event);

	void SetText(const wxString& text);
	wxString GetText();

	bool SetFont(const wxFont& font);
	void SetTextColor(const wxColor& textColor);
	bool SetBackgroundColour(const wxColour& colour);
	void SetBackRectColour(const wxColour& colour);

	void SetNorImg(wxBitmap* bitmap);
	void SetPressedImg(wxBitmap* bitmap);
	void SetEnterImg(wxBitmap* bitmap);
	void SetDisableImg(wxBitmap* bitmap);
	void SetBackground(const wxBitmap& bitmap);

	void SetNorTextColor(const wxColor& norColor);
	void SetHoverTextColor(const wxColor& hoverColor);
	void SetPressedTextColor(const wxColor& pressColor);
	void SetDisableTextColor(const wxColor& disableColor);

	void SetBgNorColor(const wxColor& norColor);
	void SetBgHoverColor(const wxColor& hoverColor);
	void SetBgPressedColor(const wxColor& pressColor);
	void SetBgDisableColor(const wxColor& disableColor);

	void SetborderNorColor(const wxColor& norColor);
	void SetborderHoverBGColor(const wxColor& hoverColor);
	void SetborderPressedBGColor(const wxColor& pressColor);
	void SetborderDisableBGColor(const wxColor& disableColor);

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
	bool IsEnglishString(const wxString& str);

	wxBitmap m_backgroundImg;
	bool m_isUsedBg;

	wxBitmap* m_norImg;
	wxBitmap* m_pressedImg;
	wxBitmap* m_enterImg;
	wxBitmap* m_disableImg;

	wxColor m_textNorColor = wxColor(255, 255, 255);
	wxColor m_textHoverColor = wxColor(255, 255, 255);
	wxColor m_textPressColor = wxColor(255, 255, 255);
	wxColor m_textDisableColor = wxColor(105, 105, 108);

	wxColor m_BgNorColor;
	wxColor m_BgHoverColor;
	wxColor m_BgPressColor;
	wxColor m_BgDisableColor;

	wxColor m_BorderNorColor;
	wxColor m_BorderHoverColor;
	wxColor m_BorderPressColor;
	wxColor m_BorderDisableColor;

	double m_radius;
	int m_btnStatus;
	wxString m_btnText;
	wxFont m_btnTextFont;

	wxColor m_backRectColor;
};

enum AnkerChooseBtnStatus
{
	ChooseBtn_Normal = 1,
	ChooseBtn_Hover,
	ChooseBtn_Select,
	ChooseBtn_Disable,

};

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, wxCommandEvent);

class AnkerChooseBtn :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerChooseBtn)
	DECLARE_EVENT_TABLE()
public:
	AnkerChooseBtn();
	AnkerChooseBtn(wxWindow* parent, wxWindowID id,
		wxString text = "",
		wxString textColor = "",
		wxFont textFont = *wxNORMAL_FONT,
		wxString textNormalColor = "",
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxValidator& validator = wxDefaultValidator);

	~AnkerChooseBtn();

	wxString getText();

	bool Create(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator& validator = wxDefaultValidator);
	void setBtnStatus(AnkerChooseBtnStatus status);
	void setText(const wxString& text);
	void setTextColor(const wxString& textColor);
	void setTextSelectColor(const wxString& textSelectColor);

	void setTextFont(const wxFont& textFont);
	void setTextSelectFont(const wxFont& textSelectFont);
	void setNormalBGColor(const wxString& normalColor);
	void setHoverBGColor(const wxString& hoverColor);
	void setDisAbleBGColor(const wxString& disableColor);

	AnkerChooseBtnStatus getBtnStatus();
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
protected:
	virtual void OnPaint(wxPaintEvent& event);
	void DrawBackground(wxDC* dc);
private:

	AnkerChooseBtnStatus m_btnStatus;

	wxString			 m_Text = { "" };
	wxString			 m_TextColor = { "" };
	wxString			 m_selectTextColor = { "" };

	wxFont				 m_TextFont;
	wxFont				 m_TextSelectFont;

	wxString			 m_normalColor = { "" };
	wxString			 m_hoverColor = { "" };
	wxString		     m_disAbleColor = { "" };
};

class AnkerTextBtn : public wxControl
{
public:
	AnkerTextBtn(wxWindow* parent);
	~AnkerTextBtn();

	void setName(wxString name, bool refresh = true) { m_name = name; }
	wxString getName() { return m_name; }
	void SetName(wxString name, bool refresh = true) { m_name = name; }
	wxString GetName() { return m_name; }

	void setText(wxString text, bool refresh = true) { m_text = text; }
	wxString getText() { return m_text; }
	void SetText(wxString text, bool refresh = true) { m_text = text; }
	wxString GetText() { return m_text; }

	void setBackgroundColour(wxColour bgColor, bool refresh = true) { m_bgColour = bgColor; if (refresh) Refresh(); }
	void setForegroundColour(wxColour fgColor, bool refresh = true) { m_fgColour = fgColor; if (refresh) Refresh(); }


private:
	void initUI();

	void OnPaint(wxPaintEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);

private:

	int m_drawingNameLenMax;
	//wxString m_drawingName;

	wxString m_name;
	wxString m_text;

	wxColour m_bgColour;
	wxColour m_fgColour;

};


class AnkerToggleBtn : public wxControl
{
public:
	AnkerToggleBtn(wxWindow* parent);
	~AnkerToggleBtn();

	void SetStateColours(bool state, wxColour roundedRectClr, wxColour circleClr);

	void SetState(bool state);
	bool GetState();
private:
	void initUI();

	void OnPaint(wxPaintEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);

private:
	bool m_state = { false };

	wxColour m_onStateRoundedRectClr ;
	wxColour m_offStateRoundedRectClr ;

	wxColour m_onStateCircleClr ;
	wxColour m_offStateCircleClr ;
};



#endif

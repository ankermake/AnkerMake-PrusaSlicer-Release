#include "AnkerBtn.hpp"
#include "wx/dc.h"
#include "GUI_App.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include <wx/graphics.h>

BEGIN_EVENT_TABLE(AnkerBtn, wxControl)
EVT_PAINT(AnkerBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerBtn::OnLeave)
EVT_LEFT_DOWN(AnkerBtn::OnPressed)
EVT_LEFT_DCLICK(AnkerBtn::OnDClick)
EVT_LEFT_UP(AnkerBtn::OnUp)
EVT_SIZE(AnkerBtn::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerBtn, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_BTN_CLICKED, wxCommandEvent);
AnkerBtn::AnkerBtn() : m_norImg(nullptr), m_pressedImg(nullptr),
					   m_enterImg(nullptr), m_disableImg(nullptr),
					   m_radius(0), m_btnStatus(NormalBtnStatus), m_isUsedBg(false)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

AnkerBtn::AnkerBtn(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
	: m_norImg(nullptr), m_pressedImg(nullptr),
	m_enterImg(nullptr), m_disableImg(nullptr),
	m_radius(0), m_btnStatus(NormalBtnStatus), m_isUsedBg(false),
	m_backRectColor(wxColour(43, 43, 43))
{
	Create(parent, id, pos, size, style, validator);
}


void AnkerBtn::SetStatus(BtnStatus status)
{
	m_btnStatus = status;
	Refresh();
}

AnkerBtn::~AnkerBtn()
{
	if(m_norImg)
		wxDELETE(m_norImg);
	if (m_enterImg)
		wxDELETE(m_enterImg);
	if (m_pressedImg)
		wxDELETE(m_pressedImg);
	if (m_disableImg)
		wxDELETE(m_disableImg);	
}

bool AnkerBtn::Create(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
{
	m_norImg = NULL;
	m_pressedImg = NULL;
	m_enterImg = NULL;
	m_disableImg = NULL;

	if (!wxControl::Create(parent, id, pos, size, style, validator))
	{
		return false;
	}

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_isUsedBg = false;
	return true;
}

void AnkerBtn::clearImg()
{
	m_norImg = nullptr;
	m_pressedImg = nullptr;
	m_enterImg = nullptr;
	m_disableImg = nullptr;
}
void AnkerBtn::SetRadius(const double& radius)
{
	m_radius = radius;
}

void AnkerBtn::DrawImg(wxDC* dc,
					   wxBitmap* image1,
					   wxBitmap* exist_image)
{
	if (image1)
	{
		dc->DrawBitmap(*image1, 0, 0, true);
	}
	else if(exist_image)
	{
		dc->DrawBitmap(*exist_image, 0, 0, true);
	}
	else
	{

	}
}

void AnkerBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	DrawBackground(&dc);

	////draw bg img
	//switch (m_btnStatus)
	//{
	//case NormalBtnStatus:
	//	if(m_norImg)
	//		dc.DrawBitmap(*m_norImg, 0, 0, true);
	//	break;
	//case EnterBtnStatus:
	//	if (!m_enterImg)
	//	{
	//		int width = dc.GetSize().GetWidth();
	//		int height = dc.GetSize().GetHeight();
	//		wxClientDC dc(this);
	//		dc.SetPen(*wxTRANSPARENT_PEN);
	//		dc.SetBrush(*wxTRANSPARENT_BRUSH);
 //
	//		dc.DrawRectangle(0, 0, width, height);
	//		break;
	//	}
	//	if(m_enterImg)
	//		dc.DrawBitmap(*m_enterImg, 0, 0, true);
	//	break;
	//case PressedBtnStatus:
	//	
	//	if (m_pressedImg && m_norImg)
	//		DrawImg(&dc, m_pressedImg, m_norImg);
	//	break;
	//case UpBtnStatus:
	//	if(m_norImg)
	//		dc.DrawBitmap(*m_norImg, 0, 0, true);
	//	break;
	//case LeaveBtnStatus:
	//	if (m_norImg)
	//		dc.DrawBitmap(*m_norImg, 0, 0, true);
	//	break;
	//case DClickBtnStatus:
	//	if(m_pressedImg && m_norImg)
	//		DrawImg(&dc, m_pressedImg, m_norImg);
	//	break;
	//case DisableBtnStatus:
	//	
	//	if (m_disableImg && m_norImg)
	//		DrawImg(&dc, m_disableImg, m_norImg);
	//	break;
	//default:
	//	if(m_norImg)
	//		dc.DrawBitmap(*m_norImg, 0, 0, true);
	//	break;
	//}

	//draw color draw border
	{
		wxRect roundeRect;
		wxRect borderRoundeRect = wxRect(0, 0, dc.GetSize().x, dc.GetSize().y);
		
		if (m_BorderNorColor.IsOk())		
#ifndef __APPLE__
			roundeRect = wxRect(1, 1, dc.GetSize().x - 2, dc.GetSize().y - 2);		
#else
			roundeRect = wxRect(2, 2, dc.GetSize().x - 4, dc.GetSize().y - 4);
#endif
		else
			roundeRect = wxRect(0, 0, dc.GetSize().x, dc.GetSize().y);
		switch (m_btnStatus)
		{
		case NormalBtnStatus:

			if (m_BorderNorColor.IsOk())
			{
				wxBrush brush(m_BorderNorColor);
				wxPen pen(m_BorderNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgNorColor.IsOk())
			{
				wxBrush brush(m_BgNorColor);
				wxPen pen(m_BgNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case EnterBtnStatus:
			if (m_BorderHoverColor.IsOk())
			{
				wxBrush brush(m_BorderHoverColor);
				wxPen pen(m_BorderHoverColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgHoverColor.IsOk())
			{
				wxBrush brush(m_BgHoverColor);
				wxPen pen(m_BgHoverColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case PressedBtnStatus:

			if (m_BorderPressColor.IsOk())
			{
				wxBrush brush(m_BorderPressColor);
				wxPen pen(m_BorderPressColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgPressColor.IsOk())
			{
				wxBrush brush(m_BgPressColor);
				wxPen pen(m_BgPressColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case UpBtnStatus:
			if (m_BorderNorColor.IsOk())
			{
				wxBrush brush(m_BorderNorColor);
				wxPen pen(m_BorderNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgNorColor.IsOk())
			{
				wxBrush brush(m_BgNorColor);
				wxPen pen(m_BgNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case LeaveBtnStatus:
			if (m_BorderNorColor.IsOk())
			{
				wxBrush brush(m_BorderNorColor);
				wxPen pen(m_BorderNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgNorColor.IsOk())
			{
				wxBrush brush(m_BgNorColor);
				wxPen pen(m_BgNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case DClickBtnStatus:
			if (m_BorderPressColor.IsOk())
			{
				wxBrush brush(m_BorderPressColor);
				wxPen pen(m_BorderPressColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgPressColor.IsOk())
			{
				wxBrush brush(m_BgPressColor);
				wxPen pen(m_BgPressColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		case DisableBtnStatus:

			if (m_BorderDisableColor.IsOk())
			{
				wxBrush brush(m_BorderDisableColor);
				wxPen pen(m_BorderDisableColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgDisableColor.IsOk())
			{
				wxBrush brush(m_BgDisableColor);
				wxPen pen(m_BgDisableColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		default:
			if (m_BorderNorColor.IsOk())
			{
				wxBrush brush(m_BorderNorColor);
				wxPen pen(m_BorderNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(borderRoundeRect, m_radius);
			}

			if (m_BgNorColor.IsOk())
			{
				wxBrush brush(m_BgNorColor);
				wxPen pen(m_BgNorColor);
				dc.SetBrush(brush);
				dc.SetPen(pen);
				dc.DrawRoundedRectangle(roundeRect, m_radius);
			}
			break;
		}
	}

	//draw bg img
	switch (m_btnStatus)
	{
	case NormalBtnStatus:
		if (m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case EnterBtnStatus:
		if (!m_enterImg)
		{
			int width = dc.GetSize().GetWidth();
			int height = dc.GetSize().GetHeight();
			wxClientDC dc(this);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);

			dc.DrawRectangle(0, 0, width, height);
			break;
		}
		if (m_enterImg)
			dc.DrawBitmap(*m_enterImg, 0, 0, true);
		break;
	case PressedBtnStatus:

		if (m_pressedImg && m_norImg)
			DrawImg(&dc, m_pressedImg, m_norImg);
		break;
	case UpBtnStatus:
		if (m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case LeaveBtnStatus:
		if (m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case DClickBtnStatus:
		if (m_pressedImg && m_norImg)
			DrawImg(&dc, m_pressedImg, m_norImg);
		break;
	case DisableBtnStatus:

		if (m_disableImg && m_norImg)
			DrawImg(&dc, m_disableImg, m_norImg);
		break;
	default:
		if (m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	}


	//draw text
	if (!m_btnText.IsEmpty())
	{
		wxSize widgetSize = dc.GetSize();
		int x = 0;
		int y = 0;

		if (!m_btnTextFont.IsNull())
			dc.SetFont(m_btnTextFont);

		wxSize textSize = dc.GetTextExtent(m_btnText);

		if ((widgetSize.GetWidth() - textSize.GetWidth()) % 2 == 0)
			x = (widgetSize.GetWidth() - textSize.GetWidth()) / 2;
		else
			x = (widgetSize.GetWidth() - textSize.GetWidth()) / 2 + 1;

		int textHeight = textSize.GetHeight();

#ifdef __APPLE__
		wxFontMetrics metrics = dc.GetFontMetrics();
		textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

		if ((widgetSize.GetHeight() - textHeight) % 2 == 0)
			y = (widgetSize.GetHeight() - textHeight) / 2;
		else
			y = (widgetSize.GetHeight() - textHeight) / 2 - 1;


		wxPoint realPoint(x, y);
		dc.SetBackgroundMode(wxTRANSPARENT);
		if(DisableBtnStatus != m_btnStatus)
			dc.SetTextForeground(m_textNorColor);
		else
			dc.SetTextForeground(m_textDisableColor);
		dc.DrawText(m_btnText, realPoint.x, realPoint.y);
	}
}

bool AnkerBtn::IsEnglishString(const wxString& str) {
	for (size_t i = 0; i < str.length(); ++i) {
		if (!wxIsalpha(str[i]) && !wxIspunct(str[i]) && !wxIsspace(str[i]) && !wxIsascii(str[i])) {
			return false;
		}
	}
	return true;
}

void AnkerBtn::DrawBackground(wxDC* dc)
{
	if (m_isUsedBg)
	{
		dc->DrawBitmap(m_backgroundImg, 0, 0, true);
	}
	else
	{
		dc->SetBrush(wxBrush(m_backRectColor));
		dc->SetPen(wxPen(m_backRectColor));
		dc->DrawRectangle(0, 0, dc->GetSize().x, dc->GetSize().y);

		wxBrush brush(GetBackgroundColour());
		wxPen pen(GetBackgroundColour());
		dc->SetBrush(brush);
		dc->SetPen(pen);
		dc->DrawRoundedRectangle(0, 0, dc->GetSize().x, dc->GetSize().y, m_radius);
	}
}

void AnkerBtn::OnEnter(wxMouseEvent& event)
{
	if (m_btnStatus == DisableBtnStatus)
		return;

	m_btnStatus = EnterBtnStatus;
	SetCursor(wxCursor(wxCURSOR_HAND));
	Refresh();
	Update();
}

void AnkerBtn::OnLeave(wxMouseEvent& event)
{
	if (m_btnStatus == DisableBtnStatus)
		return;

	if (!IsEnabled())
	{
		return;
	}

	m_btnStatus = LeaveBtnStatus;
	SetCursor(wxCursor(wxCURSOR_NONE));
	Refresh();
	Update();
}

void AnkerBtn::OnDClick(wxMouseEvent& event)
{
	if (m_btnStatus == DisableBtnStatus)
		return;

	m_btnStatus = PressedBtnStatus;

	wxCommandEvent myEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	myEvent.SetEventObject(this);
	wxPostEvent(this, myEvent);

	Refresh(false);
	Update();
}

void AnkerBtn::OnPressed(wxMouseEvent& event)
{
	if (m_btnStatus == DisableBtnStatus)
		return;

	wxCommandEvent myEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	myEvent.SetEventObject(this);
	wxPostEvent(this, myEvent);

	m_btnStatus = PressedBtnStatus;
	Refresh();
	Update();
}

void AnkerBtn::OnUp(wxMouseEvent& event)
{
	if (PressedBtnStatus != m_btnStatus)
	{
		return;
	}

	m_btnStatus = UpBtnStatus;
	Refresh();
	Update();

}

void AnkerBtn::SetNorImg(wxBitmap* bitmap)
{
	if(bitmap)
		m_norImg = bitmap;	
}

void AnkerBtn::SetPressedImg(wxBitmap* bitmap)
{
	if (bitmap)
		m_pressedImg = bitmap;	
}

void AnkerBtn::SetEnterImg(wxBitmap* bitmap)
{
	if (bitmap)
		m_enterImg = bitmap;	
}

bool AnkerBtn::Enable(bool enable)
{
	if (enable)
	{
		m_btnStatus = NormalBtnStatus;
	}
	else
	{
		m_btnStatus = DisableBtnStatus;
	}

	Refresh(false);
	Update();
	return wxControl::Enable(enable);
}


void AnkerBtn::OnSize(wxSizeEvent& event)
{
	Refresh();
}

void AnkerBtn::SetText(const wxString& text)
{
	m_btnText = text;
}

wxString AnkerBtn::GetText()
{
	return m_btnText;
}

bool AnkerBtn::SetFont(const wxFont& font)
{
	m_btnTextFont = font;
	return true;
}

void AnkerBtn::SetTextColor(const wxColor& textColor)
{
	m_textNorColor = textColor;
}

void AnkerBtn::SetDisableImg(wxBitmap* bitmap)
{
	if (bitmap)
		m_disableImg = bitmap;	
}

void AnkerBtn::SetBackground(const wxBitmap& bitmap)
{
	if (!bitmap.IsNull())
	{
		m_isUsedBg = true;
		m_backgroundImg = bitmap;
	}	
}


void AnkerBtn::SetNorTextColor(const wxColor& norColor)
{
	m_textNorColor = norColor;
}


void AnkerBtn::SetHoverTextColor(const wxColor& hoverColor)
{
	m_textHoverColor = hoverColor;
}


void AnkerBtn::SetPressedTextColor(const wxColor& pressColor)
{
	m_textPressColor = pressColor;
}


void AnkerBtn::SetDisableTextColor(const wxColor& disableColor)
{
	m_textDisableColor = disableColor;
}


void AnkerBtn::SetBgNorColor(const wxColor& norColor)
{
	m_BgNorColor = norColor;
}


void AnkerBtn::SetBgHoverColor(const wxColor& hoverColor)
{
	m_BgHoverColor = hoverColor;
}


void AnkerBtn::SetBgPressedColor(const wxColor& pressColor)
{
	m_BgPressColor = pressColor;
}


void AnkerBtn::SetBgDisableColor(const wxColor& disableColor)
{
	m_BgDisableColor = disableColor;
}


void AnkerBtn::SetborderNorColor(const wxColor& norColor)
{
	m_BorderNorColor = norColor;
}


void AnkerBtn::SetborderHoverBGColor(const wxColor& hoverColor)
{
	m_BorderHoverColor = hoverColor;
}


void AnkerBtn::SetborderPressedBGColor(const wxColor& pressColor)
{
	m_BorderPressColor = pressColor;
}


void AnkerBtn::SetborderDisableBGColor(const wxColor& disableColor)
{
	m_BorderDisableColor = disableColor;
}

bool AnkerBtn::SetBackgroundColour(const wxColour& colour)
{
	if(colour.IsOk())
	{
		m_isUsedBg = false;
		m_BgNorColor = colour;
		return wxControl::SetBackgroundColour(colour);
	}
	
	return false;
}

void AnkerBtn::SetBackRectColour(const wxColour& colour)
{
	m_backRectColor = colour;
}

AnkerChooseBtn::AnkerChooseBtn()
{

}
BEGIN_EVENT_TABLE(AnkerChooseBtn, wxControl)
EVT_PAINT(AnkerChooseBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerChooseBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerChooseBtn::OnLeave)
EVT_LEFT_DOWN(AnkerChooseBtn::OnPressed)
EVT_LEFT_DCLICK(AnkerChooseBtn::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerChooseBtn, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, wxCommandEvent);
AnkerChooseBtn::AnkerChooseBtn(wxWindow* parent,
	wxWindowID id,
	wxString text /*= ""*/,
	wxString textColor /*= ""*/,
	wxFont   textFont /*= *wxNORMAL_FONT*/,
	wxString textNormalColor /*= ""*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	long style /*= wxBORDER_NONE*/,
	const wxValidator& validator /*= wxDefaultValidator*/)		
	: m_btnStatus(ChooseBtn_Normal)
	, m_Text(text)
	, m_TextColor(textColor)
	, m_TextFont(textFont)
	, m_normalColor(textNormalColor)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

AnkerChooseBtn::~AnkerChooseBtn()
{

}


wxString AnkerChooseBtn::getText()
{
	return m_Text;
}

bool AnkerChooseBtn::Create(wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	long style /*= wxSUNKEN_BORDER*/,
	const wxValidator& validator /*= wxDefaultValidator*/)
{


	if (!wxControl::Create(parent, id, pos, size, style, validator))
		return false;

	return true;
}

void AnkerChooseBtn::setBtnStatus(AnkerChooseBtnStatus status)
{
	m_btnStatus = status;
	wxClientDC dc(this);
	if (m_btnStatus != ChooseBtn_Select)
	{
		dc.SetFont(m_TextFont);
	}
	else
	{
		dc.SetFont(m_TextSelectFont);
	}
	wxSize textSize = dc.GetTextExtent(m_Text);
	int textHeight = textSize.GetHeight();

#ifdef __APPLE__
	wxFontMetrics metrics = dc.GetFontMetrics();
	textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

	wxSize winSize;
	winSize.SetWidth(textSize.GetWidth() + 16);
	winSize.SetHeight(textHeight + 8);
	SetMaxSize(winSize);
	SetMinSize(winSize);
	SetSize(winSize);
	Refresh();
}


void AnkerChooseBtn::setText(const wxString& text)
{
	m_Text = text;
	Refresh();
}


void AnkerChooseBtn::setTextColor(const wxString& textColor)
{
	m_TextColor = textColor;
	Refresh();
}


void AnkerChooseBtn::setTextSelectColor(const wxString& textSelectColor)
{
	m_selectTextColor = textSelectColor;
	Refresh();
}


void AnkerChooseBtn::setTextFont(const wxFont& textFont)
{
	m_TextFont = textFont;
	Refresh();
}


void AnkerChooseBtn::setTextSelectFont(const wxFont& textSelectFont)
{
	m_TextSelectFont = textSelectFont;
	Refresh();
}


void AnkerChooseBtn::setNormalBGColor(const wxString& normalColor)
{
	m_normalColor = normalColor;
	Refresh();
}


void AnkerChooseBtn::setHoverBGColor(const wxString& hoverColor)
{
	m_hoverColor = hoverColor;
	Refresh();
}


void AnkerChooseBtn::setDisAbleBGColor(const wxString& disableColor)
{
	m_disAbleColor = disableColor;
	Refresh();
}

AnkerChooseBtnStatus AnkerChooseBtn::getBtnStatus()
{
	return m_btnStatus;
}


void AnkerChooseBtn::OnEnter(wxMouseEvent& event)
{
	if (m_btnStatus == ChooseBtn_Select)
		return;

	m_btnStatus = ChooseBtn_Hover;
	Refresh();
}


void AnkerChooseBtn::OnLeave(wxMouseEvent& event)
{
	if (m_btnStatus == ChooseBtn_Select)
		return;

	m_btnStatus = ChooseBtn_Normal;
	Refresh();
}


void AnkerChooseBtn::OnPressed(wxMouseEvent& event)
{
	if (m_btnStatus == ChooseBtn_Select)
		return;
	m_btnStatus = ChooseBtn_Select;

	wxClientDC dc(this);
	dc.SetFont(m_TextSelectFont);
	wxSize textSize = dc.GetTextExtent(m_Text);
	int textHeight = textSize.GetHeight();

#ifdef __APPLE__
	wxFontMetrics metrics = dc.GetFontMetrics();
	textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

	wxSize winSize;
	winSize.SetWidth(textSize.GetWidth() + 16);
	winSize.SetHeight(textHeight + 8);
	SetMaxSize(winSize);
	SetMinSize(winSize);
	SetSize(winSize);

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Refresh();
}


void AnkerChooseBtn::OnDClick(wxMouseEvent& event)
{
	if (m_btnStatus == ChooseBtn_Select)
		return;
	m_btnStatus = ChooseBtn_Select;

	wxClientDC dc(this);
	dc.SetFont(m_TextSelectFont);
	wxSize textSize = dc.GetTextExtent(m_Text);
	int textHeight = textSize.GetHeight();

#ifdef __APPLE__
	wxFontMetrics metrics = dc.GetFontMetrics();
	textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

	wxSize winSize;
	winSize.SetWidth(textSize.GetWidth() + 16);
	winSize.SetHeight(textHeight + 8);
	SetMaxSize(winSize);
	SetMinSize(winSize);
	SetSize(winSize);
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED);
	ProcessEvent(evt);

	Refresh();
}

void AnkerChooseBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rectArea = GetClientRect();
	
	rectArea.SetHeight(rectArea.height + 1);
	rectArea.SetWidth(rectArea.width + 1);

	switch (m_btnStatus)
	{
	case ChooseBtn_Normal:
	{
		dc.Clear();

		dc.SetBrush(wxBrush(wxColour(m_normalColor)));
		dc.SetPen(wxPen(wxColour(m_normalColor)));
		dc.DrawRectangle(rectArea);
	}
	break;
	case ChooseBtn_Hover:
	{
		dc.Clear();
		dc.SetBrush(wxBrush(wxColour(m_hoverColor)));
		dc.SetPen(wxPen(wxColour(m_hoverColor)));
		dc.DrawRectangle(rectArea);
	}
	break;
	case ChooseBtn_Select:
	{
		dc.Clear();
		dc.SetBrush(wxBrush(wxColour(m_hoverColor)));
		dc.SetPen(wxPen(wxColour(m_hoverColor)));
		dc.DrawRectangle(rectArea);
	}
	break;
	case ChooseBtn_Disable:
	{
		dc.Clear();
		dc.SetBrush(wxBrush(wxColour(m_disAbleColor)));
		dc.SetPen(wxColour(m_disAbleColor));
		dc.DrawRectangle(rectArea);
	}
	break;

	default:

		break;
	}

	if (!m_Text.IsEmpty())
	{
		wxSize widgetSize = GetSize();

		if (m_btnStatus == ChooseBtn_Select || m_btnStatus == ChooseBtn_Hover)
		{
			dc.SetTextForeground(wxColour(m_selectTextColor));
			dc.SetPen(wxPen(wxColour(m_selectTextColor)));
			dc.SetFont(m_TextFont);
			if (!m_TextSelectFont.IsNull() && m_btnStatus != ChooseBtn_Hover)
				dc.SetFont(m_TextSelectFont);
		}
		else
		{
			dc.SetTextForeground(wxColour(m_TextColor));
			dc.SetPen(wxPen(wxColour(m_TextColor)));
			if (!m_TextFont.IsNull())
				dc.SetFont(m_TextFont);
		}
		
		int x = 0;
		int y = 0;

		wxSize textSize = dc.GetTextExtent(m_Text);

		if ((widgetSize.GetWidth() - textSize.GetWidth()) % 2 == 0)
			x = (widgetSize.GetWidth() - textSize.GetWidth()) / 2;
		else
			x = (widgetSize.GetWidth() - textSize.GetWidth()) / 2 + 1;
		int textHeight = textSize.GetHeight();

#ifdef __APPLE__
		wxFontMetrics metrics = dc.GetFontMetrics();
		textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

		if ((widgetSize.GetHeight() - textSize.GetHeight()) % 2 == 0)
			y = (widgetSize.GetHeight() - textHeight) / 2;
		else
			y = (widgetSize.GetHeight() - textHeight) / 2 - 1;

		wxPoint realPoint(x, y);

		dc.DrawText(m_Text, realPoint);
	}
}

void AnkerChooseBtn::DrawBackground(wxDC* dc)
{
	dc->SetBrush(wxBrush(wxColour(43, 43, 43)));
	dc->SetPen(wxPen(wxColour(43, 43, 43)));
	dc->DrawRectangle(0, 0, GetSize().x, GetSize().y);

	wxBrush brush(GetBackgroundColour());
	wxPen pen(GetBackgroundColour());
	dc->SetBrush(brush);
	dc->SetPen(pen);
	dc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 2);
}

AnkerTextBtn::AnkerTextBtn(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
	, m_drawingNameLenMax(16)
	, m_text("")
	, m_bgColour(41, 42, 45)
	, m_fgColour(41, 42, 45)

{
	initUI();

	Bind(wxEVT_PAINT, &AnkerTextBtn::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &AnkerTextBtn::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &AnkerTextBtn::OnLeftUp, this);
	Bind(wxEVT_MOTION, &AnkerTextBtn::OnMouseMove, this);

	Bind(wxEVT_ENTER_WINDOW, &AnkerTextBtn::OnMouseEnter, this);
}

AnkerTextBtn::~AnkerTextBtn()
{

}

void AnkerTextBtn::initUI()
{
	SetMinSize(AnkerSize(18, 18));
	SetMaxSize(AnkerSize(18, 18));
	SetSize(AnkerSize(18, 18));

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerTextBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxColour bgColor = m_bgColour;
	auto size = GetSize();

	if (!m_text.IsSameAs(""))
	{
		bgColor = m_fgColour;
		wxBrush brush(bgColor);
		wxPen pen(wxColour(41, 42, 45));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		wxPoint drawPoint = wxPoint(0, 0);
		dc.DrawRoundedRectangle(drawPoint, size, 3);

		wxColour foreColor = wxColour(255, 255, 255);
		if (bgColor.GetLuminance() > 0.6)
			foreColor = wxColour(0, 0, 0);

		{ // draw text
			wxBrush brush(bgColor);
			wxPen pen(foreColor);
			dc.SetBrush(brush);
			dc.SetPen(pen);
			dc.SetFont(ANKER_BOLD_FONT_NO_2);
			dc.SetTextForeground(foreColor);
#ifdef __APPLE__
			wxPoint textPoint = wxPoint(drawPoint.x + size.GetWidth() / 2 - 5, drawPoint.y + size.GetHeight() / 2 - 5);
#else
			wxPoint textPoint = wxPoint(drawPoint.x + size.GetWidth() / 2 - 4, drawPoint.y + size.GetHeight() / 2 - 8);
#endif
			dc.DrawText(m_text, textPoint);
		}

		// draw bottom right triangle
		{
			wxBrush brush(foreColor);
			wxPen pen(foreColor);
			dc.SetBrush(brush);
			dc.SetPen(pen);
			wxPoint triPoints[3];
			triPoints[0] = wxPoint(drawPoint.x + size.GetWidth() - 8, drawPoint.y + size.GetHeight() - 3);
			triPoints[1] = wxPoint(drawPoint.x + size.GetWidth() - 3, drawPoint.y + size.GetHeight() - 3);
			triPoints[2] = wxPoint(drawPoint.x + size.GetWidth() - 3, drawPoint.y + size.GetHeight() - 8);
			dc.DrawPolygon(3, triPoints);
		}


	}

	
}

void AnkerTextBtn::OnMouseEnter(wxMouseEvent& event)
{

}

void AnkerTextBtn::OnMouseMove(wxMouseEvent& event)
{
    int mouseX = 0, mouseY = 0;
    event.GetPosition(&mouseX, &mouseY);
    wxPoint mousePos(mouseX, mouseY);

    SetCursor(wxCursor(wxCURSOR_HAND));
}

void AnkerTextBtn::OnLeftDown(wxMouseEvent& event)
{
}

void AnkerTextBtn::OnLeftUp(wxMouseEvent& event)
{
	int mouseX = 0, mouseY = 0;
	event.GetPosition(&mouseX, &mouseY);
	wxPoint mousePos(mouseX, mouseY);

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BTN_CLICKED);
	wxVariant eventData;
	eventData.ClearList();
	evt.SetClientData(new wxVariant(eventData));
	evt.SetEventObject(this);
	ProcessEvent(evt);

}


AnkerToggleBtn::AnkerToggleBtn(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
{
	initUI();

	Bind(wxEVT_PAINT, &AnkerToggleBtn::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &AnkerToggleBtn::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &AnkerToggleBtn::OnLeftUp, this);
	Bind(wxEVT_MOTION, &AnkerToggleBtn::OnMouseMove, this);

	Bind(wxEVT_ENTER_WINDOW, &AnkerToggleBtn::OnMouseEnter, this);
}

AnkerToggleBtn::~AnkerToggleBtn()
{

}

void AnkerToggleBtn::initUI()
{



	SetStateColours(true, wxColour(129, 220, 129), wxColour(250, 250, 250));
	SetStateColours(false, wxColour(83, 83, 83), wxColour(219, 219, 219));

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerToggleBtn::SetStateColours(bool state, wxColour roundedRectClr, wxColour circleClr)
{
	if (state) {
		m_onStateRoundedRectClr = roundedRectClr;
		m_onStateCircleClr = circleClr;
	}
	else {
		m_offStateRoundedRectClr = roundedRectClr;
		m_offStateCircleClr = circleClr;
	}
}


void AnkerToggleBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

	if (gc)
	{
		gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

		auto size = GetSize();
		wxRect rect(wxPoint(0, 0), size);

		gc->SetBrush(GetBackgroundColour());
		gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);

		if (m_state)
			gc->SetBrush(m_onStateRoundedRectClr); 
		else
			gc->SetBrush(m_offStateRoundedRectClr); 
		gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, size.y/2);
		
		float circleRadius = 0.5 * size.y * (8.0f / 10.0f);
		wxPoint circleCenter;
		if (m_state) {
			circleCenter = wxPoint(size.x - (size.y - circleRadius * 2) / 2 - circleRadius, size.y / 2);
			gc->SetBrush(m_onStateCircleClr);
		}
		else {
			circleCenter = wxPoint(0 + (size.y - circleRadius * 2) / 2 + circleRadius, size.y / 2);
			gc->SetBrush(m_offStateCircleClr);
		}
		gc->DrawEllipse(circleCenter.x - circleRadius, circleCenter.y - circleRadius, circleRadius * 2, circleRadius * 2);

		delete gc;
	}
}

void AnkerToggleBtn::OnMouseEnter(wxMouseEvent& event)
{

}

void AnkerToggleBtn::OnMouseMove(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_HAND));
}

void AnkerToggleBtn::OnLeftDown(wxMouseEvent& event)
{

}

void AnkerToggleBtn::OnLeftUp(wxMouseEvent& event)
{
	m_state = !m_state;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BTN_CLICKED);
	wxVariant eventData;
	eventData.ClearList();
	evt.SetClientData(new wxVariant(eventData));
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Refresh();
}

void AnkerToggleBtn::SetState(bool state)
{
	m_state = state;
	Refresh();
}


bool AnkerToggleBtn::GetState()
{
	return m_state;
}


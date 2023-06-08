#include "AnkerBtn.hpp"
#include "wx/dc.h"


BEGIN_EVENT_TABLE(AnkerBtn, wxControl)
EVT_PAINT(AnkerBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerBtn::OnLeave)
EVT_LEFT_DOWN(AnkerBtn::OnPressed)
EVT_LEFT_DCLICK(AnkerBtn::OnDClick)
EVT_LEFT_UP(AnkerBtn::OnUp)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerBtn, wxControl)


AnkerBtn::AnkerBtn() : m_norImg(nullptr), m_pressedImg(nullptr),
					   m_enterImg(nullptr), m_disableImg(nullptr),
					   m_radius(0), m_btnStatus(NormalBtnStatus), m_isUsedBg(false)
{
}

AnkerBtn::AnkerBtn(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
	: m_norImg(nullptr), m_pressedImg(nullptr),
	m_enterImg(nullptr), m_disableImg(nullptr),
	m_radius(0), m_btnStatus(NormalBtnStatus), m_isUsedBg(false)
{
	Create(parent, id, pos, size, style, validator);
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

	switch (m_btnStatus)
	{
	case NormalBtnStatus:
		if(m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case EnterBtnStatus:
		if (!m_enterImg)
		{
			int width = GetSize().GetWidth();
			int height = GetSize().GetHeight();
			wxClientDC dc(this);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
 
			dc.DrawRectangle(0, 0, width, height);
			break;
		}
		if(m_enterImg)
			dc.DrawBitmap(*m_enterImg, 0, 0, true);
		break;
	case PressedBtnStatus:
		
		DrawImg(&dc, m_pressedImg, m_norImg);
		break;
	case UpBtnStatus:
		if(m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case LeaveBtnStatus:
		if (m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	case DClickBtnStatus:
		
		DrawImg(&dc, m_pressedImg, m_norImg);
		break;
	case DisableBtnStatus:
		
		DrawImg(&dc, m_disableImg, m_norImg);
		break;
	default:
		if(m_norImg)
			dc.DrawBitmap(*m_norImg, 0, 0, true);
		break;
	}
 
	if (!m_btnText.IsEmpty())
	{		
		wxSize wixgetSize = GetSize();
		int x = 0;
		int y = 0;
 
		if (!m_btnTextFont.IsNull())		
			dc.SetFont(m_btnTextFont);
 
		wxSize textSize = GetTextExtent(m_btnText);
 
		if((wixgetSize.GetWidth() - textSize.GetWidth()) % 2 == 0)
			x = (wixgetSize.GetWidth() - textSize.GetWidth()) / 2;
		else
			x = (wixgetSize.GetWidth() - textSize.GetWidth()) / 2 + 1;
 
		if((wixgetSize.GetHeight() - textSize.GetHeight()) % 2 == 0)
			y = (wixgetSize.GetHeight() - textSize.GetHeight()) / 2;
		else
			y = (wixgetSize.GetHeight() - textSize.GetHeight()) / 2 + 1;
				
		dc.SetBackgroundMode(wxTRANSPARENT);
		dc.SetTextForeground(m_textColor);
		dc.DrawText(m_btnText, x, y);			
	}
}

void AnkerBtn::DrawBackground(wxDC* dc)
{
	if (m_isUsedBg)
	{
		dc->DrawBitmap(m_backgroundImg, 0, 0, true);
	}
	else
	{
		dc->SetBrush(wxBrush(wxColour(43, 43, 43)));
		dc->SetPen(wxPen(wxColour(43, 43, 43)));
		dc->DrawRectangle(0, 0, GetSize().x, GetSize().y);

		wxBrush brush(GetBackgroundColour());
		wxPen pen(GetBackgroundColour());
		dc->SetBrush(brush);
		dc->SetPen(pen);
		dc->DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, m_radius);
	}
}

void AnkerBtn::OnEnter(wxMouseEvent& event)
{
	m_btnStatus = EnterBtnStatus;
	Refresh();
	Update();
}

void AnkerBtn::OnLeave(wxMouseEvent& event)
{
	if (!IsEnabled())
	{
		return;
	}
	m_btnStatus = LeaveBtnStatus;
	Refresh();
	Update();
}

void AnkerBtn::OnDClick(wxMouseEvent& event)
{
	m_btnStatus = PressedBtnStatus;
	Refresh(false);
	Update();
}

void AnkerBtn::OnPressed(wxMouseEvent& event)
{
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
	wxCommandEvent myEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	myEvent.SetEventObject(this);
	GetEventHandler()->ProcessEvent(myEvent);
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

void AnkerBtn::SetText(const wxString& text)
{
	m_btnText = text;
}

bool AnkerBtn::SetFont(const wxFont& font)
{
	m_btnTextFont = font;
	return true;
}

void AnkerBtn::SetTextColor(const wxColor& textColor)
{
	m_textColor = textColor;
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

bool AnkerBtn::SetBackgroundColour(const wxColour& colour)
{
	if(colour.IsOk())
	{
		m_isUsedBg = false;
		return wxControl::SetBackgroundColour(colour);
	}
	
	return false;
}
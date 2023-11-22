#include "AnkerCheckBox.hpp"
#include "libslic3r/Utils.hpp"

BEGIN_EVENT_TABLE(AnkerCheckBox, wxControl)
EVT_PAINT(AnkerCheckBox::OnPaint)
EVT_LEFT_DOWN(AnkerCheckBox::OnPressed)
EVT_LEFT_DCLICK(AnkerCheckBox::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerCheckBox, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, wxCommandEvent);

AnkerCheckBox::AnkerCheckBox()
{

}

AnkerCheckBox::AnkerCheckBox(wxWindow* parent, 
							wxImage uncheckImg,
							wxImage checkImg,
							wxImage disuncheckImg,
							wxImage discheckImg,
							wxString text,
							wxFont   font,
							wxColour color,
							wxWindowID winid /*= wxID_ANY*/,
							const wxPoint& pos /*= wxDefaultPosition*/,
							const wxSize& size /*= wxDefaultSize*/)
{

	wxControl::Create(parent, winid, pos, size, wxBORDER_NONE, wxDefaultValidator);

	m_uncheckImg = uncheckImg;
	m_checkImg = checkImg;
	m_disUncheckImg = disuncheckImg;
	m_disCheckImg = discheckImg;
	m_text = text;
	m_textFont = font;
	m_Color = color;

#ifndef __APPLE__	
	m_bgColor = wxColour("#292A2D");
#else
	m_bgColor = wxColour("#3F4044");
#endif
}

AnkerCheckBox::~AnkerCheckBox()
{

}

void AnkerCheckBox::setBgColor(wxColour bgColor)
{
	m_bgColor = bgColor;
	Refresh();
}

void AnkerCheckBox::SetFont(wxFont font)
{
	m_textFont = font;
	Refresh();
}

void AnkerCheckBox::Disable()
{
	wxControl::Disable();
	Refresh();
}


void AnkerCheckBox::setText(const wxString& text)
{
	m_text = text;
	Refresh();
}

bool AnkerCheckBox::Enable(bool enable /*= true*/)
{
	bool res = wxControl::Enable(enable);
	Refresh();
	return res;
}

void AnkerCheckBox::setCheckStatus(bool isCheck)
{
	m_ischeck = isCheck;
	Refresh();
}


void AnkerCheckBox::SetValue(bool isCheck)
{
	setCheckStatus(isCheck);
}


bool AnkerCheckBox::GetValue()
{
	return getCheckStatus();
}

bool AnkerCheckBox::getCheckStatus()
{
	return m_ischeck;
}

void AnkerCheckBox::OnPressed(wxMouseEvent& event)
{
	if (!IsEnabled())
	{
		return;
	}

	m_ischeck = !m_ischeck;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);
	Refresh();
}

void AnkerCheckBox::OnDClick(wxMouseEvent& event)
{
// click the AnkerCheckBox quickly an repeatly would have bug(UI display state and m_ischeck value unmatch)
#if 0
	if (!IsEnabled())
	{
		return;
	}

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	m_ischeck = !m_ischeck;
	Refresh();
#endif
}

void AnkerCheckBox::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);	
	
	wxBrush brush(m_bgColor);		
	dc.SetBrush(brush);
	dc.DrawRectangle(-1, -1, GetRect().GetWidth() + 2, GetRect().GetHeight() + 2);

 	if (IsEnabled())
 	{	
 		if (m_ischeck)
 		{	
 			dc.DrawBitmap(m_checkImg, 0, 0, true);
 		}
 		else
 		{			
 			dc.DrawBitmap(m_uncheckImg, 0, 0, true);
 		}	
 	}
 	else
 	{
 		if (m_ischeck)
 		{			
 			dc.DrawBitmap(m_disCheckImg, 0, 0, true);
 		}
 		else
 		{		
 			dc.DrawBitmap(m_disUncheckImg, 0, 0, true);
 		}
 	}

	if (!m_text.IsEmpty())
	{		
		dc.SetBrush(m_bgColor);
		dc.SetPen(m_bgColor);
		dc.SetTextForeground(m_Color);
		dc.SetFont(m_textFont);
		dc.DrawText(m_text, wxPoint(24, 0));
	}

	event.Skip();
}

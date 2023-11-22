#include "AnkerAdjustItemWidget.hpp"

#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "libslic3r/Utils.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "GUI_App.hpp"


BEGIN_EVENT_TABLE(AnkerAdjustItemWidget, wxControl)
EVT_LEFT_DOWN(AnkerAdjustItemWidget::OnClick)
EVT_PAINT(AnkerAdjustItemWidget::OnPaint)
EVT_ENTER_WINDOW(AnkerAdjustItemWidget::OnEnter)
EVT_LEAVE_WINDOW(AnkerAdjustItemWidget::OnLeave)
EVT_LEFT_DOWN(AnkerAdjustItemWidget::OnPressed)
EVT_SIZE(AnkerAdjustItemWidget::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerAdjustItemWidget, wxControl)

AnkerAdjustItemWidget::AnkerAdjustItemWidget(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	SetBackgroundColour(wxColour("#343538"));	
	//m_arrowStatusImg = "arrow_right.png";
	m_arrowStatusImg = "arrow_right.disable.png";
}


AnkerAdjustItemWidget::AnkerAdjustItemWidget()
{

}

AnkerAdjustItemWidget::~AnkerAdjustItemWidget()
{

}


void AnkerAdjustItemWidget::setLogo(const std::string& logo)
{
	m_logo = logo;
}


void AnkerAdjustItemWidget::setTitle(const std::string& title)
{
	m_title = title;
	Refresh();
}


void AnkerAdjustItemWidget::setContent(const std::string& content)
{
	m_content = content;
	Refresh();
}


void AnkerAdjustItemWidget::setStatus(bool isDisabel)
{
	m_status = isDisabel;
	if (m_status)
	{
		m_arrowStatusImg = "arrow_right.png";
		SetBackgroundColour(wxColour("#343538"));
	}
	else
	{
		m_arrowStatusImg = "arrow_right.disable.png";
		SetBackgroundColour(wxColour("#2C2D30"));
	}
	Refresh();
}

void AnkerAdjustItemWidget::OnEnter(wxMouseEvent& event)
{

}


void AnkerAdjustItemWidget::OnSize(wxSizeEvent& event)
{
	Refresh();
}

void AnkerAdjustItemWidget::OnLeave(wxMouseEvent& event)
{

}

void AnkerAdjustItemWidget::OnPressed(wxMouseEvent& event)
{
	if (m_status)
	{
		m_status = false;
		m_arrowStatusImg = "arrow_right.disable.png";
		wxCommandEvent evt = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED);
		ProcessEvent(evt);		
		Refresh();
	}
}

void AnkerAdjustItemWidget::OnClick(wxMouseEvent& event)
{

	if (m_status)
	{
		m_status = false;
		m_arrowStatusImg = "arrow_right.disable.png";
		wxCommandEvent evt = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED);
		ProcessEvent(evt);		
		Refresh();
	}

}

void AnkerAdjustItemWidget::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	if (m_title.IsEmpty() || m_logo.IsEmpty() || m_content.IsEmpty())
		return;

	wxColour backGroundColor = wxColour("#0D0D0D");
	wxColour titleColor = wxColour("#FFFFFF");
	wxColour contentColor = wxColour("#999999");

	if (m_status)
	{
		backGroundColor = wxColour("#343538");
		titleColor = wxColour("#FFFFFF");
		contentColor = wxColour("#999999");
	}
	else
	{		
		backGroundColor = wxColour("#2C2D30");
		titleColor = wxColour("#696969");
		contentColor = wxColour("#545454");
	}

	{
		dc.Clear();
		wxBrush brush(backGroundColor);
		wxPen pen(backGroundColor);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(GetRect());
	}
	wxSize size;
	{
		dc.Clear();		
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		size = dc.GetTextExtent(m_title);
		wxBrush brush(titleColor);
		wxPen pen(titleColor);
		dc.SetTextForeground(titleColor);
		wxPoint textPoint = wxPoint(9, 9);
		dc.DrawText(m_title, textPoint);
	}

	{
		dc.SetFont(ANKER_FONT_NO_2);
		wxBrush brush(contentColor);
		wxPen pen(contentColor);
		dc.SetTextForeground(contentColor);
		
		wxPoint textPoint = wxPoint(9, 1.3* Slic3r::GUI::wxGetApp().em_unit() + size.GetHeight());
		dc.DrawText(m_content, textPoint);
	}

	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var(m_arrowStatusImg.ToStdString())), wxBITMAP_TYPE_PNG);
		image.Rescale(16, 16);
		wxBitmap scaledBitmap(image);

		dc.DrawBitmap(scaledBitmap, GetRect().width - 30, 9);
	}
}

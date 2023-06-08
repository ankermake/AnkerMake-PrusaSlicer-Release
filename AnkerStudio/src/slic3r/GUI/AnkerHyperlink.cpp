#include "AnkerHyperlink.hpp"

BEGIN_EVENT_TABLE(AnkerHyperlink, wxControl)
EVT_LEFT_DOWN(AnkerHyperlink::OnClick)
EVT_PAINT(AnkerHyperlink::OnPaint)
EVT_LEFT_DOWN(AnkerHyperlink::OnPressed)
EVT_ENTER_WINDOW(AnkerHyperlink::OnEnter)
EVT_LEAVE_WINDOW(AnkerHyperlink::OnLeave)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerHyperlink, wxControl)

AnkerHyperlink::AnkerHyperlink(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxString& name /*= wxString("")*/,
	const wxString& urlLink /*= wxString("")*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	m_text = name;
	m_link = urlLink;
	SetBackgroundColour(wxColour("#1F2022"));
}

AnkerHyperlink::AnkerHyperlink()
{

}

void AnkerHyperlink::OnEnter(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_HAND));
}

void AnkerHyperlink::OnLeave(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_NONE));
}

void AnkerHyperlink::OnPressed(wxMouseEvent& event)
{
	if (!m_link.IsEmpty())
		wxLaunchDefaultBrowser(m_link);
}

void AnkerHyperlink::OnClick(wxMouseEvent& event)
{
	if (!m_link.IsEmpty())
		wxLaunchDefaultBrowser(m_link);
}

void AnkerHyperlink::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	if (!m_text.IsEmpty())
	{
		dc.Clear();
		wxBrush brush(wxColour("#62D361"));
		wxPen pen(wxColour("#62D361"));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextForeground(wxColour("#62D361"));
		wxPoint textPoint = wxPoint(0, 0);
		dc.DrawText(m_text, textPoint);
	}
}

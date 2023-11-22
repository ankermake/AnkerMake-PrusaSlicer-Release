#include "AnkerLine.hpp"


AnkerLine::AnkerLine(
	wxWindow* parent, wxWindowID winid,
	const wxPoint& pos,
	const wxSize& size, const wxColour& colour) :
	AnkerBox(parent, winid, pos, size)	, m_liseSize(size)
{
	SetForegroundColour(colour);
	SetBackgroundColour(colour);
	//SetMaxSize(wxSize(width, height));
	//SetMinSize(wxSize(width, height));

	//borderHighEnable();
}

AnkerLine2::AnkerLine2(wxWindow* parent, wxWindowID winid, const wxPoint& pos,
	const wxSize& size, const wxColour& colour)	 : AnkerLine(parent, winid, pos, size, colour)
{
}

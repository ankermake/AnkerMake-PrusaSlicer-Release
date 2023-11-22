#ifndef _ANKER_LINE_HPP
#define _ANKER_LINE_HPP

#include <wx/panel.h>
#include "AnkerBox.hpp"

class AnkerLine : public AnkerBox
{
public:
	AnkerLine(wxWindow* parent, wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxSize(400, 1),
		const wxColour& colour = "#545863");

	wxSize m_liseSize;
private:
	
};


class AnkerLine2 : public  AnkerLine
{
public:
	AnkerLine2(wxWindow* parent, wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxSize(400, 1),
		const wxColour& colour = "#545863");
};



#endif // !_ANKER_LINE_HPP

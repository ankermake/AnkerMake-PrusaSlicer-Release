#include "AnkerBitmapCombox.hpp"
#include "AnkerGUIConfig.hpp"
#include "../GUI_App.hpp"

// AnkerBitmapCombox::AnkerBitmapCombox(wxWindow* parent,
// 									wxWindowID id,
// 									const wxString& value /*= wxEmptyString*/,
// 									const wxPoint& pos /*= wxDefaultPosition*/, 
// 									const wxSize& size /*= wxDefaultSize*/,
// 									int n /*= 0*/, 
// 									const wxString choices[] /*= nullptr*/,
// 									unsigned int style /*= 0*/, 
// 									const wxValidator& validator /*= wxDefaultValidator*/,
// 									const wxString& name /*= wxComboBoxNameStr*/)
// 								   : wxOwnerDrawnComboBox(parent, id, value, pos, size, n, choices, style, validator, name)
// {
// 	m_borderColor = wxColor("#434447");
// 	m_bgColor = wxColor("#292A2D");
// 	m_textColor  = wxColour("#FFFFFF");
// 	m_itemHeight = 24;
// }


// AnkerBitmapCombox::AnkerBitmapCombox()
// {
// 	m_borderColor = wxColor("#434447");
// 	m_bgColor = wxColor("#292A2D");
// 	m_textColor = wxColour("#FFFFFF");
// 	m_itemHeight = 24;
// }

AnkerBitmapCombox::~AnkerBitmapCombox()
{

}


void AnkerBitmapCombox::setColor(const wxColor& borderColor,
                                 const wxColor& bgColor,
                                 const wxColor& textColor)
{
	m_borderColor = borderColor;
	m_bgColor = bgColor;
    m_textColor = textColor;
}


void AnkerBitmapCombox::setColor(const wxString& borderColor, const wxString& bgColor)
{
	m_borderColor = wxColor(borderColor);
	m_bgColor = wxColor(bgColor);
}

void AnkerBitmapCombox::setComBoxItemHeight(const int& height)
{
	m_itemHeight = height;
}

void AnkerBitmapCombox::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
{
	if (item == wxNOT_FOUND)
		return;

	wxRect r(rect);

    // If the current item is selected, then draw a custom background color.
    if (flags & wxODCB_PAINTING_SELECTED) {
		wxColour colour("#506853");
		//wxColour colour("#61D37D");
		//wxColour colour(97,211,125,10);
		//colour.Set(colour.Red(),colour.Green(),colour.Blue(),0x00);
        dc.SetBrush(wxBrush(colour));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rect);
	}
	else {
		dc.SetBrush(wxBrush(m_bgColor));
		dc.SetPen(m_bgColor);
		wxRect newRec(rect);
		newRec.SetWidth(rect.width + 1);
		newRec.SetHeight(rect.height + 1);
		dc.DrawRectangle(newRec);
	}
	
    AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(item);
    if (!comboBoxItem) {
        return;
    }

    // draw colour btn
	int textLeftSpan = 5;
	if (comboBoxItem->color != "") {
		wxPen colorRecPen(wxColour(comboBoxItem->color));
		wxBrush colorRecBrush(wxColour(comboBoxItem->color));
		wxRect square(rect.x + 10, rect.y + (r.height - AnkerLength(18)) / 2,
			AnkerLength(18), AnkerLength(18));
		dc.SetBrush(colorRecBrush);
		dc.SetPen(colorRecPen);
		dc.DrawRectangle(square);

		textLeftSpan = 35;
	}

    
    wxPen pen(dc.GetTextForeground(), 1, wxPENSTYLE_TRANSPARENT);
#ifndef __APPLE__
    wxPoint textPoint(r.x + AnkerLength(textLeftSpan),
        (r.y + 0) + ((r.height / 2) - dc.GetCharHeight() / 2));
#else
    wxPoint textPoint(r.x + AnkerLength(textLeftSpan),
        (r.y + 0) + ((r.height / 2) - dc.GetCharHeight() / 2));
#endif // !__APPLE__


	// Get text colour as pen colour
	dc.SetPen(wxPen(m_textColor));
	dc.SetFont(ANKER_FONT_NO_1);
	dc.SetTextForeground(m_textColor);
	    
	dc.DrawText(comboBoxItem->label.ToStdString(),
		textPoint);
}

void AnkerBitmapCombox::OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const
{
	wxRect rectAreaEx = rect;		
	
	dc.SetBrush(wxBrush(m_borderColor));
	dc.SetPen(wxPen(m_borderColor));
	dc.DrawRectangle(rectAreaEx);

	wxRect rectArea = rect;
	rectArea.SetX(rect.x + 1);
	rectArea.SetY(rect.y + 1);

#ifndef __APPLE__
	rectArea.SetWidth(rect.width + 1);
#else
	rectArea.SetWidth(rect.width - 2);
#endif // !__APPLE__
	
//	rectArea.SetHeight(rect.height - 2);

// 	dc.SetBrush(wxBrush(m_bgColor));
// 	dc.SetPen(wxPen(m_bgColor));
// 	dc.DrawRectangle(rectArea);

	wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);

}

void AnkerBitmapCombox::DrawButton(wxDC& dc, const wxRect& rect, int flags)
{

	dc.SetBrush(m_borderColor);
	dc.SetPen(m_borderColor);
	dc.DrawRectangle(rect);

	wxRect rectArea = rect;
#ifndef __APPLE__
	rectArea.SetX(rect.x - 1);
	rectArea.SetY(rect.y + 1);
	rectArea.SetHeight(rect.height - 2);
#else
	rectArea.SetX(rect.x + 1);
	rectArea.SetY(rect.y + 1);
	rectArea.SetWidth(rect.width - 2);
	rectArea.SetHeight(rect.height - 2);
#endif

	dc.SetBrush(m_bgColor);
	dc.SetPen(m_bgColor);
	dc.DrawRectangle(rectArea);
	m_btnRect = rectArea;
	flags = Button_PaintBackground | Button_BitmapOnly;

	wxOwnerDrawnComboBox::DrawButton(dc, rectArea, flags);
}

wxCoord AnkerBitmapCombox::OnMeasureItem(size_t item) const
{
	// Simply demonstrate the ability to have variable-height items
	//	return FromDIP(item & 1 ? 36 : 24);
	return FromDIP(m_itemHeight);
}

wxCoord AnkerBitmapCombox::OnMeasureItemWidth(size_t WXUNUSED(item)) const
{
	return -1; // default - will be measured from text width
}

const wxRect AnkerBitmapCombox::GetButtonRect() {
	return m_btnRect;
} 
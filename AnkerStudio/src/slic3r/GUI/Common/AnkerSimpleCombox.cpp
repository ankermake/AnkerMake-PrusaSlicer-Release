#include "AnkerSimpleCombox.hpp"
#include "AnkerGUIConfig.hpp"
#include "../Common/AnkerFont.hpp"
// AnkerSimpleCombox::AnkerSimpleCombox(wxWindow* parent,
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


// AnkerSimpleCombox::AnkerSimpleCombox()
// {
// 	m_borderColor = wxColor("#434447");
// 	m_bgColor = wxColor("#292A2D");
// 	m_textColor = wxColour("#FFFFFF");
// 	m_itemHeight = 24;
// }

AnkerSimpleCombox::~AnkerSimpleCombox()
{

}


void AnkerSimpleCombox::setColor(const wxColor& borderColor,
                                 const wxColor& bgColor,
                                 const wxColor& textColor)
{
	m_borderColor = borderColor;
	m_bgColor = bgColor;
    m_textColor = textColor;
	m_textColorSaved = m_textColor;
}


void AnkerSimpleCombox::setColor(const wxString& borderColor, const wxString& bgColor)
{
	m_borderColor = wxColor(borderColor);
	m_bgColor = wxColor(bgColor);
}

void AnkerSimpleCombox::setComBoxItemHeight(const int& height)
{
	m_itemHeight = height;
}

void AnkerSimpleCombox::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
{
	if (item == wxNOT_FOUND)
		return;

	wxRect r(rect);
	wxPen pen(dc.GetTextForeground(), 1, wxPENSTYLE_TRANSPARENT);
#ifndef __APPLE__
	wxPoint textPoint(r.x + 5,
		(r.y + 0) + ((r.height / 2) - dc.GetCharHeight()/2));
#else
	wxPoint textPoint(r.x + 5,
		(r.y + 0) + ((r.height / 2) - dc.GetCharHeight()/2));
#endif // !__APPLE__

	if (flags & wxODCB_PAINTING_CONTROL)
	{
        dc.SetBrush(wxBrush(m_bgColor));
        dc.SetPen(m_bgColor);
        wxRect newRec(rect);
        newRec.SetWidth(rect.width + 1);
        newRec.SetHeight(rect.height + 1);
        dc.DrawRectangle(newRec);
        // Get text colour as pen colour
        dc.SetPen(wxPen(m_textColor));
        dc.SetFont(Head_14);
        dc.SetTextForeground(m_textColor);
	}
	else {
        // If the current item is selected, then draw a custom background color.
        if (flags & wxODCB_PAINTING_SELECTED) {
            wxColour colour("#506853");
            //wxColour colour("#61D37D");
            //wxColour colour(97,211,125,10);
            //colour.Set(colour.Red(),colour.Green(),colour.Blue(),0x00);
            dc.SetBrush(wxBrush(colour));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(rect);
            // Get text colour as pen colour
            dc.SetPen(wxPen("#FFFFFF"));
            dc.SetFont(Head_14);
            dc.SetTextForeground(wxColour("#FFFFFF"));
        }
        else {
            dc.SetBrush(wxBrush(m_bgColor));
            dc.SetPen(m_bgColor);
            wxRect newRec(rect);
            newRec.SetWidth(rect.width + 1);
            newRec.SetHeight(rect.height + 1);
            dc.DrawRectangle(newRec);

            // Get text colour as pen colour
            dc.SetPen(wxPen(m_textColor));
            dc.SetFont(Head_14);
            dc.SetTextForeground(m_textColor);
        }
	}
   
	dc.DrawText(GetString(item),
		textPoint);
}

void AnkerSimpleCombox::OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const
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

void AnkerSimpleCombox::DrawButton(wxDC& dc, const wxRect& rect, int flags)
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

wxCoord AnkerSimpleCombox::OnMeasureItem(size_t item) const
{
	// Simply demonstrate the ability to have variable-height items
	//	return FromDIP(item & 1 ? 36 : 24);
	return FromDIP(m_itemHeight);
}

wxCoord AnkerSimpleCombox::OnMeasureItemWidth(size_t WXUNUSED(item)) const
{
	return -1; // default - will be measured from text width
}

const wxRect AnkerSimpleCombox::GetButtonRect() {
	return m_btnRect;
} 

bool AnkerSimpleCombox::Enable(bool enable)
{
	if (enable) {
		m_textColor = m_textColorSaved;
	}
	else {
		m_textColor = (wxColour(80, 80, 80));
	}
	bool ret = wxControl::Enable(enable);
	Refresh();
	return ret;
}

bool AnkerSimpleCombox::Disable()
{
	bool ret = Enable(false);
	Refresh();
	return ret;
}


//AnkerEditCombox::AnkerEditCombox(wxWindow* parent, 
//								wxWindowID id, 
//								const wxString& value /*= wxEmptyString*/,
//								const wxPoint& pos /*= wxDefaultPosition*/,
//								const wxSize& size /*= wxDefaultSize*/,
//								int n /*= 0*/, 
//								const wxString choices[] /*= NULL*/,
//								long style /*= 0*/)
//								: wxOwnerDrawnComboBox(parent, id, value, pos, size, n, choices, style)
//{
//	SetBackgroundColour(wxColor("#292A2D"));
//	SetEditable(true);
//}
//
//AnkerEditCombox::~AnkerEditCombox()
//{
//
//}
//
//void AnkerEditCombox::SetValue(const wxString& value)
//{
//	if (IsEditable())
//	{
//		wxTextEntry::SetValue(value);
//	}
//	else
//	{
//		wxOwnerDrawnComboBox::SetValue(value);
//	}
//}
//
//wxString AnkerEditCombox::GetValue() const
//{
//	if (IsEditable())
//	{
//		return wxTextEntry::GetValue();
//	}
//	else
//	{
//		return wxOwnerDrawnComboBox::GetValue();
//	}
//}
//
//void AnkerEditCombox::WriteText(const wxString& text)
//{
//	if (IsEditable())
//	{
//		wxTextEntry::WriteText(text);
//	}
//	else
//	{
//		wxOwnerDrawnComboBox::WriteText(text);
//	}
//}
//
//void AnkerEditCombox::AppendText(const wxString& text)
//{
//	if (IsEditable())
//	{
//		wxTextEntry::AppendText(text);
//	}
//	else
//	{
//		wxOwnerDrawnComboBox::AppendText(text);
//	}
//}
//
//void AnkerEditCombox::setColor(const wxColor& borderColor, const wxColor& bgColor)
//{
//	m_borderColor = borderColor;
//	m_bgColor = bgColor;
//	Refresh();
//}
//
//void AnkerEditCombox::setColor(const wxString& borderColor, const wxString& bgColor)
//{
//	m_borderColor = wxColor(borderColor);
//	m_bgColor = wxColor(bgColor);
//	Refresh();
//}
//
//void AnkerEditCombox::setComBoxItemHeight(const int& height)
//{
//	m_itemHeight = height;
//}
//
//void AnkerEditCombox::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
//{
//	if (item == wxNOT_FOUND)
//		return;
//
//	wxRect r(rect);
//	wxPen pen(dc.GetTextForeground(), 1, wxPENSTYLE_TRANSPARENT);
//#ifndef __APPLE__

//	wxPoint textPoint(r.x + 5,
//		(r.y + 10) + ((r.height / 2) - dc.GetCharHeight()));
//#else

//	wxPoint textPoint(r.x + 5,
//		(r.y + 12) + ((r.height / 2) - dc.GetCharHeight()));
//#endif // !__APPLE__
//
//
//	// If the current item is selected, then draw a custom background color.
//	if (flags & wxODCB_PAINTING_SELECTED) {
//		dc.SetBrush(wxBrush(wxColour("#506853")));
//		dc.SetPen(*wxTRANSPARENT_PEN);
//		dc.DrawRectangle(rect);
//	}
//	else {
//		dc.SetBrush(wxBrush(m_bgColor));
//	}
//	// Get text colour as pen colour
//	dc.SetPen(wxPen(m_textColor));
//	dc.SetFont(ANKER_FONT_NO_1);
//	dc.SetTextForeground(m_textColor);
//
//	dc.DrawText(GetString(item),
//		textPoint);
//
//}
//
//void AnkerEditCombox::OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const
//{
//	wxRect rectAreaEx = rect;
//
//	dc.SetBrush(wxBrush(m_borderColor));
//	dc.SetPen(wxPen(m_borderColor));
//	dc.DrawRectangle(rectAreaEx);
//
//	wxRect rectArea = rect;
//	rectArea.SetX(rect.x + 1);
//	rectArea.SetY(rect.y + 1);
//
//#ifndef __APPLE__
//	rectArea.SetWidth(rect.width + 1);
//#else
//	rectArea.SetWidth(rect.width - 2);
//#endif // !__APPLE__
//
//	//rectArea.SetHeight(rect.height - 2);
//
//	//dc.SetBrush(wxBrush(m_bgColor));
//	//dc.SetPen(wxPen(m_bgColor));
//	//dc.DrawRectangle(rectArea);
//
//	wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);
//
//}
//
//void AnkerEditCombox::DrawButton(wxDC& dc, const wxRect& rect, int flags)
//{
//	dc.SetBrush(m_borderColor);
//	dc.SetPen(m_borderColor);
//	dc.DrawRectangle(rect);
//
//	wxRect rectArea = rect;
//#ifndef __APPLE__
//	rectArea.SetX(rect.x - 1);
//	rectArea.SetY(rect.y + 1);
//	rectArea.SetHeight(rect.height - 2);
//#else
//	rectArea.SetX(rect.x + 1);
//	rectArea.SetY(rect.y + 1);
//	rectArea.SetWidth(rect.width - 2);
//	rectArea.SetHeight(rect.height - 2);
//#endif
//
//	dc.SetBrush(m_bgColor);
//	dc.SetPen(m_bgColor);
//	dc.DrawRectangle(rectArea);
//
//	flags = Button_PaintBackground | Button_BitmapOnly;
//
//	wxOwnerDrawnComboBox::DrawButton(dc, rectArea, flags);
//}
//
//wxCoord AnkerEditCombox::OnMeasureItem(size_t item) const
//{
//	// Simply demonstrate the ability to have variable-height items
//	//	return FromDIP(item & 1 ? 36 : 24);
//	return FromDIP(m_itemHeight);
//}
//
//wxCoord AnkerEditCombox::OnMeasureItemWidth(size_t WXUNUSED(item)) const
//{
//	return -1; // default - will be measured from text width
//}

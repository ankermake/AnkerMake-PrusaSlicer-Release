#ifndef _ANKER_SIMPLE_COMBOX_HPP_
#define _ANKER_SIMPLE_COMBOX_HPP_

#include "wx/odcombo.h"
#include "wx/wx.h"
#include "wx/dc.h"

class AnkerSimpleCombox : public wxOwnerDrawnComboBox
{
public:
	~AnkerSimpleCombox();

	virtual void setColor(const wxColor& borderColor,
                          const wxColor& bgColor,
                          const wxColor& textColor = wxColour("#FFFFFF"));
	virtual void setColor(const wxString& borderColor, const wxString& bgColor);
	virtual void setComBoxItemHeight(const int& height);

	virtual void OnDrawItem(wxDC& dc,
		const wxRect& rect,
		int item,
		int flags) const wxOVERRIDE;


	virtual void OnDrawBackground(wxDC& dc,
		const wxRect& rect,
		int item,
		int flags) const wxOVERRIDE;

	virtual void DrawButton(wxDC& dc,
		const wxRect&
		rect,
		int flags);


	virtual wxCoord OnMeasureItem(size_t item) const wxOVERRIDE;
	virtual wxCoord OnMeasureItemWidth(size_t WXUNUSED(item)) const wxOVERRIDE;

	virtual const wxRect GetButtonRect();

	bool Enable(bool enable = true);
	bool Disable(); 

protected:

private:
	wxColor		m_borderColor = wxColor("#434447");
	wxColor		m_bgColor = wxColor("#292A2D");
	wxColor		m_textColor = wxColour("#FFFFFF");
	wxColor		m_textColorSaved = wxColour("#FFFFFF");
	int			m_itemHeight = 24;
	wxRect		m_btnRect;
};




#endif

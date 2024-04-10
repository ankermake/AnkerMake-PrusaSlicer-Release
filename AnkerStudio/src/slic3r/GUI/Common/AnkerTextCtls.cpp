#include "AnkerTextCtls.h"

namespace Slic3r {

	namespace GUI {

		 void AnkerTextCtrlEx::OnPaint(wxPaintEvent& event)
		{
		    //wxTextCtrl::OnPaint(event);
			wxSize size = GetSize();
			wxPen pen(m_border_color, 1);
			wxPaintDC dc(this);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			//dc.SetBrush(wxBrush(m_bg_color));
			dc.SetPen(pen);
			dc.DrawRectangle(0, 0, size.GetWidth() + 2, size.GetHeight() + 2);
		}

	 void AnkerTextCtrlEx::OnEraseBackground(wxEraseEvent& event)
	{
		wxDC* dc = event.GetDC();
		if (dc)
		{
			dc->SetBrush(wxBrush(m_bg_color));
			wxRect windowRect(wxPoint(0, 0), GetSize());
			dc->DrawRectangle(windowRect);
		}
	}


	 BEGIN_EVENT_TABLE(AnkerTextCtrlEx, wxTextCtrl)
		 EVT_PAINT(AnkerTextCtrlEx::OnPaint)
		 EVT_ERASE_BACKGROUND(AnkerTextCtrlEx::OnEraseBackground)
	 END_EVENT_TABLE()

	}
}

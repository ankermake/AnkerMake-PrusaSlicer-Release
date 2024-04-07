#include "AnkerBaseCtls.h"

namespace Slic3r {

	namespace GUI {

		BEGIN_EVENT_TABLE(AnkerNoBarScrolledWindow, wxScrolledWindow)
			EVT_MOUSEWHEEL(AnkerNoBarScrolledWindow::OnMouseWheel)
		END_EVENT_TABLE()

		AnkerNoBarScrolledWindow::AnkerNoBarScrolledWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxString& name) : wxScrolledWindow(parent, id, pos, size, style, name)
		{

		}

		void AnkerNoBarScrolledWindow::OnMouseWheel(wxMouseEvent& event)
		{
			int iRotation = event.GetWheelRotation();
			if (iRotation > 0)
			{
				Scroll(0, GetScrollPos(wxVERTICAL) - 1);
			}
			else
			{
				Scroll(0, GetScrollPos(wxVERTICAL) + 1);
			}
			event.Skip();
		}

	}
}
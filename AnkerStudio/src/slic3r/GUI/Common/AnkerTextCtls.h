#pragma once
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/scrolbar.h>


namespace Slic3r {
	namespace GUI {

		class AnkerTextCtrlEx :public wxTextCtrl
		{
		public:
			AnkerTextCtrlEx(wxWindow* parent,
				wxWindowID 	id,
				const wxString& value = wxEmptyString,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long 	style = 0,
				const wxValidator& validator = wxDefaultValidator,
				const wxString& name = wxTextCtrlNameStr
			) :wxTextCtrl(parent, id,value,pos,size,style|wxNO_BORDER,validator,name)
			{
				SetForegroundColour(*wxWHITE);
				m_text_color = wxColor(255, 255, 255);
				//m_border_color = wxColor("#3F4043");
				m_border_color = wxColor(255,0,0);
				m_bg_color = wxColor(41, 42, 45);
				SetBackgroundColour(m_bg_color);
			}

			void setForgeColor(wxColor textColor)
			{ 
				m_text_color = textColor;
				wxTextCtrl::SetForegroundColour(textColor);
				Refresh();
			};

			void setBorderColor(wxColor borderColor) { m_border_color = borderColor; };
			void setBgColor(wxColor bgColor) { m_bg_color = bgColor;};

		virtual void OnEraseBackground(wxEraseEvent& event);
		virtual void OnPaint(wxPaintEvent& event);

			

		private:
			wxColor m_text_color;
			wxColor m_border_color;
			wxColor m_bg_color;
			DECLARE_EVENT_TABLE()
		};
	}
}

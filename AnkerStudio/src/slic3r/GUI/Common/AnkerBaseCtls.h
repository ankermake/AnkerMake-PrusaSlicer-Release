#pragma once
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/scrolbar.h>


namespace Slic3r {
	namespace GUI {

		const wxColour DEFAULT_BG_COLOR(33, 34, 38);

		class AnkerBasePanel :public wxPanel 
		{
		public:
			AnkerBasePanel(wxWindow* parent, 
				wxWindowID 	id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long 	style = wxTAB_TRAVERSAL,
				const wxString& name = wxPanelNameStr
				): wxPanel(parent, id, pos,size,style,name)
			{
				m_bgColor = wxColor("#3A3B3F");
				m_fgColor = wxColor("#FFFFFF");
			}

			void setBgColor(wxColor newColor) { m_bgColor = newColor; }
			void setFgColor(wxColor newColor) { m_fgColor = newColor; }

		protected:
			wxColour m_bgColor;
			wxColour m_fgColor;
		};

		class AnkerBaseButton :public wxButton
		{
		public:
			AnkerBaseButton(wxWindow* parent,
				wxWindowID id, 
				const wxString& label = wxEmptyString, 
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize, 
				long style = 0, 
				const wxValidator& validator = wxDefaultValidator,
				const wxString& name = wxButtonNameStr) : wxButton(parent,id, label,pos,size,style,validator,name)
			{

			}
		};

		class AnkerBaseDialog :public wxDialog
		{

		};

		class AnkerBaseCombbBox :public wxComboBox
		{

		};

		class AnkerStaicText :public wxStaticText
		{
		public:
			AnkerStaicText(wxWindow* parent,
				wxWindowID 	id,
				const wxString& label,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long 	style = 0,
				const wxString& name = wxStaticTextNameStr
			) :wxStaticText(parent,id,label)
			{

			}
		};


		class AnkerNoBarScrolledWindow : public wxScrolledWindow
		{
		public:
			AnkerNoBarScrolledWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);
			void OnMouseWheel(wxMouseEvent& event);
		private:

			DECLARE_EVENT_TABLE()
		};
	}
}

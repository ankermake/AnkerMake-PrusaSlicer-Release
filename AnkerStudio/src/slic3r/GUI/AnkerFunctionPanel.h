#pragma once
#include  <wx/panel.h>
#include  <wx/sizer.h>
#include "AnkerBtn.hpp"
#include "AnkerDevice.hpp"
#include "Common/AnkerCombinButton.h"

wxDECLARE_EVENT(wxCUSTOMEVT_ON_CLICK_LOGON, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ON_TAB_CHANGE, wxCommandEvent);

namespace Slic3r {
	namespace GUI {

		enum tabPanleType
		{
			type_slice = 0, 
			type_devcie,
			type_others
		};



		class AnkerFunctionPanel : public wxPanel
		{
		public:
			AnkerFunctionPanel() {};
			AnkerFunctionPanel(wxWindow* parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxTAB_TRAVERSAL | wxNO_BORDER,
				const wxString& name = wxASCII_STR(wxPanelNameStr));
			void initUI();
			void initEvent();
			void OnPaint(wxPaintEvent& event);
			void SetPrintTab(wxBookCtrlBase* tabControl) { m_pPrintTab = tabControl; };

			DECLARE_EVENT_TABLE()

		private:
			AnkerCombinButton* m_pHomeButton{ nullptr };
			AnkerCombinButton* m_pLoginButton{ nullptr };
			AnkerCombinButton* m_pSliceButton{ nullptr };
			AnkerCombinButton* m_pPrintButton{ nullptr };
			wxBoxSizer* m_pSizer{nullptr};
			wxBookCtrlBase* m_pPrintTab{nullptr};
			//collection of all control function  page buttons
			std::vector<AnkerCombinButton*> m_pTabBtnVec;

		};
	}
}

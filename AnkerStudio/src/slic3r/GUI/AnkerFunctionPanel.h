#pragma once
#include  <wx/panel.h>
#include  <wx/sizer.h>
#include "AnkerBtn.hpp"
#include "AnkerDevice.hpp"
#include "Common/AnkerCombinButton.h"

wxDECLARE_EVENT(wxCUSTOMEVT_ON_CLICK_LOGON, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ON_TAB_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_FEEDBACK, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_MSG_CENTRE, wxCommandEvent);

wxDECLARE_EVENT(wxCUSTOMEVT_FEEDBACK, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SHOW_DOC, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_RELEASE_NOTE, wxCommandEvent);
//wxDECLARE_EVENT(wxCUSTOMEVT_FEEDBACK_HELP_CLICKED , wxCommandEvent);
namespace Slic3r {
	namespace GUI {

		enum tabPanleType
		{
			type_slice = 0, 
			type_devcie,
			type_others
		};

		wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_TABBAR_BTN_CLICKED, wxCommandEvent);
		class AnkerTabBarBtn :public wxControl
		{
			DECLARE_DYNAMIC_CLASS(AnkerMsgPageBtn)
			DECLARE_EVENT_TABLE()
		public:
			AnkerTabBarBtn();
			virtual ~AnkerTabBarBtn();
			AnkerTabBarBtn(wxWindow* parent, wxWindowID id,
				wxImage btnImg,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxBORDER_NONE,
				const wxValidator& validator = wxDefaultValidator);

			void setImg(wxImage img);
			virtual void OnPressed(wxMouseEvent& event);
			virtual void OnEnter(wxMouseEvent& event);
			virtual void OnLeave(wxMouseEvent& event);
		protected:
			void OnPaint(wxPaintEvent& event);
		private:
			wxImage   m_norImg;
			wxColor   m_bgColor;
		};

		class AnkerFunctionPanel : public wxPanel
		{
		public:
			AnkerFunctionPanel() {};
			~AnkerFunctionPanel();
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
			void setMsgEntrenceRedPointStatus(bool hasUnread);
			void setMsgItemRedPointStatus(int officicalNews,int printerNews);
			wxMenu* m_calib_menu = nullptr;

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
			int m_officicalNews = 0;
			int m_printerNews = 0;
			AnkerTabBarBtn* m_msgCentreBtn{ nullptr };
			AnkerTabBarBtn* m_FeedBackHelpBtn{ nullptr };
			wxMenu* m_pFeedbackHelpMenu { nullptr };			
		};
	}
}

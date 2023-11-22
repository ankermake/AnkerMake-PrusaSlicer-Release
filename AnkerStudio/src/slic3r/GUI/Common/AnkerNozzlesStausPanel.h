#pragma once
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/scrolbar.h>
#include "AnkerBaseCtls.h"




namespace Slic3r {
	namespace GUI {

		struct filamentInfo
		{
			wxString strfilamentColor;
			wxString strFilamentName;

			bool operator<(const filamentInfo& rhs) const
			{
				if (strFilamentName < rhs.strFilamentName)
					return true;
				else
					return false;
			}
		};

		struct  printFilamentInfo
		{
			int iIndex;
			filamentInfo infoDetail;
			bool bCanReplace;
		};

		class AnkerNozzlesStausPanel : public AnkerBasePanel
		{
		public:
			AnkerNozzlesStausPanel(	wxWindow* parent,
							wxWindowID id = wxID_ANY,
							const wxPoint& pos = wxDefaultPosition,
							const wxSize& size = wxDefaultSize,
							long style = wxSB_HORIZONTAL,
							const wxString& name = wxScrollBarNameStr);


			virtual void SetFocus() override;
			void OnPaint(wxPaintEvent& event);
			void OnMouseEvent(wxMouseEvent& event);
			void InitData();
			void InitUI();
			void SetNoticeText(std::string strNewText) 
			{
				m_NoticeText = strNewText;
				if (m_pContentText != nullptr)
				{
					m_pContentText->SetLabel(strNewText);
					m_pContentText->Update();
				}
			}

		protected:
			virtual bool ProcessEvent(wxEvent& event) override;

		private:
			void SimulateData();

		protected:
			std::map<filamentInfo, printFilamentInfo> filamentMap;
			std::vector<printFilamentInfo> m_PrinterFilamentVec;

		private:
			std::string m_NoticeText;
			wxStaticText* m_pContentText;

			DECLARE_EVENT_TABLE()
		};
	}
}

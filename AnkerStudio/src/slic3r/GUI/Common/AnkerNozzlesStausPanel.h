#pragma once
#include <vector>
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/scrolbar.h>
#include "AnkerBaseCtls.h"
#include "../Common/AnkerRoundPanel.hpp"
#include "AnkerNetDefines.h"


namespace Slic3r {
	namespace GUI {


		enum NOZZLES_STATUS {
			type_normal = 0,
			type_out_of_supplies,
			type_jammed	
		};

		typedef struct tagNozzlesData
		{
			wxString strMaterialName;
			wxColour ColorRgb;
			int		iNozzlesInx;
			NOZZLES_STATUS nozzlesStatus;
			ROUND_STATE nozzlesBtnState;
		}NozzlesData;

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
			void InitEvent();
			void SetNoticeText(std::string strNewText) 
			{
				m_NoticeText = strNewText;
				if (m_pContentText != nullptr)
				{
					m_pContentText->SetLabel(strNewText);
					m_pContentText->Update();
				}
			}

			NOZZLES_STATUS getDevcieStatus() {return  m_DeviceInterruptType;}

		protected:
			virtual bool ProcessEvent(wxEvent& event) override;

		protected:
			std::map<filamentInfo, printFilamentInfo> filamentMap;
			std::vector<printFilamentInfo> m_PrinterFilamentVec;
			std::vector<NozzlesData> m_NozzlesStateVec;

		private:
			wxString m_NoticeText;
			wxStaticText* m_pContentText;
			NOZZLES_STATUS	m_DeviceInterruptType;
			std::vector<AnkerStateRoundTextPanel*> m_NozzlesPanelVec;
			DECLARE_EVENT_TABLE()
		};
	}
}

#include <wx/event.h>
#include "AnkerGUIConfig.hpp"
#include "../GUI_App.hpp"
#include "AnkerNozzlesStausPanel.h"
#include "../Common/AnkerRoundPanel.hpp"


namespace Slic3r {
	namespace GUI {


		AnkerNozzlesStausPanel::AnkerNozzlesStausPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name /*= wxScrollBarNameStr*/)
			: AnkerBasePanel(parent, id, pos, size, style, name)
		{
			InitData();
			InitUI();
		}

		void AnkerNozzlesStausPanel::OnPaint(wxPaintEvent& event)
		{
			//EVT_MOUSE_EVENTS
		}

		void AnkerNozzlesStausPanel::OnMouseEvent(wxMouseEvent& event)
		{
			wxPoint curPos = event.GetPosition();
			wxPoint ChildScreenPos = ClientToScreen(curPos);
			wxPoint ParentChildPos = GetParent()->ScreenToClient(ChildScreenPos);
			event.SetPosition(ParentChildPos);
			wxPostEvent(this->GetParent(), event);
			event.Skip();
		}

		void AnkerNozzlesStausPanel::InitData()
		{
			m_NoticeText = "Filament transfer interrupted, please pick a new slot to resume print.";
			SimulateData();
		}

		void AnkerNozzlesStausPanel::SimulateData()
		{
			printFilamentInfo info;
			info.iIndex = 0;
			info.bCanReplace = true;
			filamentInfo innerInfo;
			innerInfo.strfilamentColor = "#fffff";//white
			innerInfo.strFilamentName = "PLA";
			info.infoDetail = innerInfo;
			m_PrinterFilamentVec.push_back(info);

			info.iIndex = 1;
			innerInfo.strfilamentColor = "#FF0000";
			innerInfo.strFilamentName = "PLA+";
			m_PrinterFilamentVec.push_back(info);

			info.iIndex = 2;
			innerInfo.strfilamentColor = "#00ff00";
			innerInfo.strFilamentName = "PLA+";
			m_PrinterFilamentVec.push_back(info);


			info.iIndex = 3;
			innerInfo.strfilamentColor = "#0000FF";
			innerInfo.strFilamentName = "PLA+";
			m_PrinterFilamentVec.push_back(info);

			info.iIndex = 4;
			innerInfo.strfilamentColor = "#12ff00";
			innerInfo.strFilamentName = "TPT+";
			info.bCanReplace = false;
			m_PrinterFilamentVec.push_back(info);


			info.iIndex = 5;
			innerInfo.strfilamentColor = "#00ffF3";
			innerInfo.strFilamentName = "?";
			info.bCanReplace = false;
			m_PrinterFilamentVec.push_back(info);


			filamentInfo gcodeFilementInfo;
			gcodeFilementInfo.strfilamentColor = "#fffff";
			gcodeFilementInfo.strFilamentName = "PLA";

			filamentMap.insert(std::make_pair(gcodeFilementInfo, m_PrinterFilamentVec[0]));

			gcodeFilementInfo.strfilamentColor = "#123456";
			gcodeFilementInfo.strFilamentName = "PLA+";
			filamentMap.insert(std::make_pair(gcodeFilementInfo, m_PrinterFilamentVec[1]));
		}

		void AnkerNozzlesStausPanel::InitUI()
		{
			SetBackgroundColour(DEFAULT_BG_COLOR);
			wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
			SetSizer(contentSizer);
			m_pContentText = new wxStaticText(this, wxID_ANY, m_NoticeText);
			m_pContentText->SetBackgroundColour(DEFAULT_BG_COLOR);
			m_pContentText->SetForegroundColour(wxColour("#FFFFFF"));
			m_pContentText->SetFont(ANKER_FONT_SIZE(9));
			m_pContentText->SetMinSize(AnkerSize(352, 32));
			contentSizer->Add(m_pContentText, 0, wxEXPAND | wxBOTTOM, 25);

			wxGridSizer* gridSizer = new wxGridSizer(2, 3, 3, 9);
			AnkerBasePanel* pContentNozzelPanel = new AnkerBasePanel(this);
			pContentNozzelPanel->SetBackgroundColour(DEFAULT_BG_COLOR);
			pContentNozzelPanel->SetSizer(gridSizer);

			for (int i = 0; i < m_PrinterFilamentVec.size(); i++)
			{
				if (i == 1)
				{
					AnkerStateRoundPanel* pPanel = new AnkerStateRoundPanel(pContentNozzelPanel);
					pPanel->SetSizeHints(AnkerSize(63, 60), AnkerSize(63, 60));
					pPanel->SetGapWidth(5);
					pPanel->SetRoundColor(wxColour("#3096FF"));
					pPanel->SetState(state_warning);
					gridSizer->Add(pPanel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);

				}
				else if (i == 2)
				{
					AnkerStateRoundPanel* pPanel = new AnkerStateRoundPanel(pContentNozzelPanel);
					pPanel->SetSizeHints(AnkerSize(63, 60), AnkerSize(63, 60));
					pPanel->SetState(state_unknown);
					pPanel->SetRoundColor(wxColour("#03CE3C"));
					gridSizer->Add(pPanel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
				}
				else if (i == 3)
				{
					AnkerStateRoundTextPanel* pPanel = new AnkerStateRoundTextPanel(pContentNozzelPanel);
					pPanel->SetSizeHints(AnkerSize(63, 77), AnkerSize(63, 77));
					pPanel->SetState(state_selected);
					pPanel->SetRoundColor(wxColour("#5cd8FF"));
					pPanel->SetInnerText("1#");
					pPanel->SetDescText("PLA");
					pPanel->SetTextSpan(3);
					gridSizer->Add(pPanel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
				}
				else
				{
					AnkerStateRoundPanel* pPanel = new AnkerStateRoundPanel(pContentNozzelPanel);
					pPanel->SetSizeHints(AnkerSize(63, 60), AnkerSize(63, 60));
					pPanel->SetRoundColor(wxColour("#3096FF"));
					pPanel->SetState(state_normal);
					gridSizer->Add(pPanel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
				}
			}
			contentSizer->Add(pContentNozzelPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 58);
			contentSizer->AddStretchSpacer();
		}


		void AnkerNozzlesStausPanel::SetFocus()
		{

			wxWindow::SetFocus();
		}

		bool AnkerNozzlesStausPanel::ProcessEvent(wxEvent& event)
		{
			 if (/*event.GetEventType() == wxEVT_MOTION || */
				event.GetEventType() == wxEVT_LEFT_UP || 
				 event.GetEventType() == wxEVT_LEFT_DOWN)
			{
				wxMouseEvent& mouseEvent = static_cast<wxMouseEvent&>(event);
				OnMouseEvent(mouseEvent);
			}

			return wxWindow::ProcessEvent(event);
		}


		BEGIN_EVENT_TABLE(AnkerNozzlesStausPanel, wxPanel)
		EVT_PAINT(AnkerNozzlesStausPanel::OnPaint)
		END_EVENT_TABLE()

	}
}

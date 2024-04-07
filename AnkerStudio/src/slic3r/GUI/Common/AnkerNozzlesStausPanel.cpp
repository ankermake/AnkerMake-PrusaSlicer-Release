#include <wx/event.h>
#include "AnkerGUIConfig.hpp"
#include "../GUI_App.hpp"
#include "AnkerNozzlesStausPanel.h"
#include "../I18N.hpp"
#include "slic3r/Config/AnkerGlobalConfig.hpp"



namespace Slic3r {
	namespace GUI {


		AnkerNozzlesStausPanel::AnkerNozzlesStausPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name /*= wxScrollBarNameStr*/)
			: AnkerBasePanel(parent, id, pos, size, style, name)
		{
			InitData();
			InitUI();
			InitEvent();
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
			m_DeviceInterruptType = type_normal;
			m_NoticeText = (m_DeviceInterruptType == type_out_of_supplies)? _L("common_print_popup_filamentbrokenv6"):
				_L("common_print_popup_filamentblobs");
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
			//TODO: adjust UI 
			// 32  lack of supply
			// 35 jammed
			wxGridSizer* gridSizer = new wxGridSizer(2, 3, 16, 16);
			AnkerBasePanel* pContentNozzelPanel = new AnkerBasePanel(this);
			pContentNozzelPanel->SetBackgroundColour(DEFAULT_BG_COLOR);
			pContentNozzelPanel->SetSizer(gridSizer);

			for (int i = 0; i < m_NozzlesStateVec.size(); i++)
			{
				//0: normal state 1:out of supplies state 2:jammed state 
				AnkerStateRoundTextPanel* pPanel = new AnkerStateRoundTextPanel(pContentNozzelPanel);
				m_NozzlesPanelVec.push_back(pPanel);
				//pPanel->SetSizeHints(AnkerSize(63, 72), AnkerSize(63, 72));
				pPanel->SetSizeHints(AnkerSize(63, 77), AnkerSize(63, 77));
				pPanel->SetRoundColor(m_NozzlesStateVec[i].ColorRgb);
				if (m_DeviceInterruptType == type_jammed && m_NozzlesStateVec[i].nozzlesBtnState != state_warning)
				{
					pPanel->SetState(state_unselected);
				}
				else
				{
					pPanel->SetState(m_NozzlesStateVec[i].nozzlesBtnState);
				}
			
				pPanel->SetInnerText(std::to_string(m_NozzlesStateVec[i].iNozzlesInx));
				pPanel->SetDescText(m_NozzlesStateVec[i].strMaterialName);
				gridSizer->Add(pPanel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
			}
			contentSizer->Add(pContentNozzelPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 58);
			contentSizer->AddStretchSpacer();
		}

		void AnkerNozzlesStausPanel::InitEvent()
		{
			for (int i = 0 ;i < m_NozzlesPanelVec.size();i++)
			{
				AnkerStateRoundPanel* pCurRoundPanel = m_NozzlesPanelVec[i]->getInnnerRoundPanel();
				pCurRoundPanel->Bind(wxEVT_LEFT_DOWN, [this,pCurRoundPanel](wxMouseEvent& event) {
					if (pCurRoundPanel->GetState() == state_selected)
					{
						pCurRoundPanel->SetState(state_normal);
					}
					else if (pCurRoundPanel->GetState() == state_normal)
					{
						pCurRoundPanel->SetState(state_selected);
						//unselect  other panel
						for (int i = 0; i < m_NozzlesPanelVec.size(); i++)
						{
							if (m_NozzlesPanelVec[i]->getInnnerRoundPanel() != pCurRoundPanel && m_NozzlesPanelVec[i]->getInnnerRoundPanel()->GetState() == state_selected)
							{
								m_NozzlesPanelVec[i]->SetState(state_normal);
							}
						}
					}
					Refresh();	
					});
			}
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

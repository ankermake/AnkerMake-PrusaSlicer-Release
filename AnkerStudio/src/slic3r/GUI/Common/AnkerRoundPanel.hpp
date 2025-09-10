#ifndef ANKER_ROUND_PANEL_HPP
#define ANKER_ROUND_PANEL_HPP
#include "AnkerBaseCtls.h"



enum ROUND_STATE
{
	state_normal,
	state_selected,
	state_unselected, //just show ,can not select 
	state_warning,
	state_disabled,
	state_unknown
};


namespace Slic3r {
	namespace GUI {

		class AnkerRoundPanel : public AnkerBasePanel
		{
		public:
			AnkerRoundPanel();
			AnkerRoundPanel(wxWindow* parent);
			~AnkerRoundPanel();


			void InitUi();
			void OnPaint(wxPaintEvent& event);
			void SetRoundColor(wxColour newColor) { m_RoundBgColor = newColor; };
			void SetBorderColor(wxColour newColor) { m_BorderColor = newColor; };
			void SetGapBgColor(wxColour newColor) { m_GapBgColor = newColor; };
			void SetGapWidth(int iNewWidth) { m_GapWidth = iNewWidth; }


		protected:
			wxColour m_RoundBgColor;
			wxColour m_BorderColor;
			wxColour m_PanelBgColor;
			wxColour m_GapBgColor;
			int		m_GapWidth;
		};


		class AnkerStateRoundPanel :public AnkerRoundPanel
		{
		public:
			AnkerStateRoundPanel(wxWindow* parent);
			~AnkerStateRoundPanel();
			void InitUi();
			void OnPaint(wxPaintEvent& event);
			void SetState(ROUND_STATE newState) { m_CurState = newState; }
			ROUND_STATE GetState() { return m_CurState;}
			void SetInnerText(wxString strNewText) { m_InnnerText = strNewText; }
			void OnBtnPressed(wxMouseEvent& event);

		protected:
			ROUND_STATE m_CurState;
			wxString m_InnnerText;
	
			DECLARE_EVENT_TABLE()
		};

		class AnkerStateRoundTextPanel:public AnkerBasePanel
		{
		public:
			AnkerStateRoundTextPanel(wxWindow* parent);
			~AnkerStateRoundTextPanel();
			void InitUi();
			AnkerStateRoundPanel* getInnnerRoundPanel() {return m_RoundPanel;}
			ROUND_STATE GetState() { return m_CurState; }
			void SetState(ROUND_STATE newState)
			{
				if (m_RoundPanel != nullptr)
				{
					m_RoundPanel->SetState(newState);
				}
			}
			void OnPaint(wxPaintEvent& event);
			void SetRoundColor(wxColour newColor)
			{
				if (m_RoundPanel != nullptr)
				{
					m_RoundPanel->SetRoundColor(newColor);
				}
			}
			void SetInnerText(wxString strNewText)
			{ 
				if (m_RoundPanel != nullptr)
				{
					m_RoundPanel->SetInnerText(strNewText);
				}
			}
			void SetDescText(wxString strNewText)
			{
				m_strDesc = strNewText;
				if (m_pTextCtrl != nullptr)
				{
					m_pTextCtrl->SetLabel(strNewText);
				}
				Refresh();
			}
			void SetTextSpan(int iSpan) { m_iTextSpan = iSpan; }
			

		
		protected:
			wxString m_strDesc;
			AnkerStateRoundPanel* m_RoundPanel;
			AnkerStaicText* m_pTextCtrl;
			int m_iTextSpan; //the span between the round panel and teh decribe text
			ROUND_STATE m_CurState;
			DECLARE_EVENT_TABLE()
		};
	}
}









#endif // !ANKER_BASE_HPP

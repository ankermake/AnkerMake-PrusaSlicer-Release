#include "AnkerRoundPanel.hpp"
#include <wx/graphics.h>
#include "../GUI.hpp"
#include "libslic3r/Utils.hpp"
#include <slic3r/GUI/Common/AnkerGUIConfig.hpp>
#include <slic3r/GUI/GUI_App.hpp>
const wxColour DEFAULT_ROUND_BG_COLOR(255, 255, 255, 25);
const wxColour DEFAULT_ROUND_BORDER_COLOR(92, 216, 255, 200);
const wxColour DEFAULT_ROUND_GAP_COLOR(255, 255, 255, 25);
const wxColour DEFAULT_BG_COLOR(33, 34, 38);
#define DEFAULT_ROUND_GAP_WIDTH 5
#define DEFAULT_ROUND_BORDER_WIDTH 1


namespace Slic3r {
	namespace GUI {

	

		AnkerRoundPanel::AnkerRoundPanel():AnkerBasePanel(NULL)
		{
			InitUi();
			Bind(wxEVT_PAINT, &AnkerRoundPanel::OnPaint, this);
		}

		AnkerRoundPanel::AnkerRoundPanel(wxWindow* parent) : AnkerBasePanel(parent)
		{
			InitUi();
			Bind(wxEVT_PAINT, &AnkerRoundPanel::OnPaint, this);
		}

		AnkerRoundPanel::~AnkerRoundPanel()
		{
		}

		void AnkerRoundPanel::InitUi()
		{
			m_RoundBgColor = DEFAULT_ROUND_BG_COLOR;
			m_BorderColor = DEFAULT_ROUND_BORDER_COLOR;
			m_GapBgColor = DEFAULT_ROUND_GAP_COLOR;
			m_GapWidth = DEFAULT_ROUND_GAP_WIDTH;
			m_PanelBgColor = DEFAULT_BG_COLOR;
		}

		void AnkerRoundPanel::OnPaint(wxPaintEvent& event)
		{
			wxAutoBufferedPaintDC dc(this);
			dc.SetBackground(m_PanelBgColor);
			dc.Clear(); // Clear the background
			wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
			if (gc)
			{
				gc->SetAntialiasMode(wxANTIALIAS_DEFAULT); // Enable anti-aliasing
				wxSize size = GetSize();
				//gc->SetPen(*wxRED_PEN);
				gc->SetBrush(m_RoundBgColor);
				//draw spacer area  background
				gc->DrawEllipse(m_GapWidth, m_GapWidth, size.GetWidth()  - 2* m_GapWidth, size.GetWidth() - 2 * m_GapWidth);
				//draw border style
				// no need draw the outer circle
				//gc->SetPen(m_BorderColor);
				//gc->SetBrush(*wxTRANSPARENT_BRUSH);
				////draw Circle  area
				//gc->DrawEllipse(0, 0, size.GetWidth() -1, size.GetWidth() -1);
				delete gc;
			}
		}

		AnkerStateRoundPanel::AnkerStateRoundPanel(wxWindow* parent):AnkerRoundPanel(parent), m_InnnerText("?")
		{
			InitUi();
			Bind(wxEVT_PAINT, &AnkerStateRoundPanel::OnPaint, this);
		}

		AnkerStateRoundPanel::~AnkerStateRoundPanel()
		{

		}

		void AnkerStateRoundPanel::InitUi()
		{
			m_CurState = state_normal;
		}

		void AnkerStateRoundPanel::OnPaint(wxPaintEvent& event)
		{
			wxAutoBufferedPaintDC dc(this);
			dc.SetBackground(m_PanelBgColor);
			dc.Clear(); 
			wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
			if (gc)
			{
				// Enable anti-aliasing
				gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
				wxSize size = GetSize();
				int iRoundDiameter = std::min(size.x,size.y);


				if (m_CurState == state_selected)
				{
					gc->SetBrush(m_RoundBgColor);
					//draw spacer area  background
					gc->DrawEllipse(m_GapWidth, m_GapWidth, iRoundDiameter - 2* m_GapWidth, iRoundDiameter - 2 * m_GapWidth);
					//draw border style
					gc->SetPen(m_BorderColor);
					gc->SetBrush(*wxTRANSPARENT_BRUSH);
					////draw Circle  area
					gc->DrawEllipse(0, 0, iRoundDiameter - 1, iRoundDiameter - 1);
					//draw  inner text
					dc.SetTextForeground(*wxWHITE);
					dc.SetFont(ANKER_BOLD_FONT_SIZE(12));
					wxSize size = GetSize();
					wxCoord w, h;
					dc.GetTextExtent(m_InnnerText, &w, &h);
					dc.DrawText(m_InnnerText, (iRoundDiameter - w) / 2 , (iRoundDiameter - h) / 2);
				}
				else if (m_CurState == state_warning)
				{
					gc->SetBrush(m_RoundBgColor);
					//draw spacer area  background
					gc->DrawEllipse(m_GapWidth, m_GapWidth, iRoundDiameter - 2 * m_GapWidth, iRoundDiameter - 2 * m_GapWidth);
					//add warning image
					//draw border style
					gc->SetPen(*wxRED_PEN);
					gc->SetBrush(*wxTRANSPARENT_BRUSH);
					////draw Circle  area
					gc->DrawEllipse(0, 0, iRoundDiameter - 1, iRoundDiameter - 1);
					//draw images  warning.png
					wxBitmap image;
					image.LoadFile(Slic3r::GUI::from_u8(Slic3r::var("warning.png")), wxBITMAP_TYPE_PNG);
					if (image.IsOk())
					{
						dc.DrawBitmap(image, size.GetWidth() - image.GetWidth(), 0, false);
					}
					//draw  inner text
					dc.SetTextForeground(*wxWHITE);
					dc.SetFont(ANKER_BOLD_FONT_SIZE(12));
					wxSize size = GetSize();
					wxCoord w, h;
					dc.GetTextExtent(m_InnnerText, &w, &h);
					dc.DrawText(m_InnnerText, (iRoundDiameter  - w) / 2 , (iRoundDiameter - h) / 2);
				}
				else if (m_CurState == state_disabled)
				{
					//add a mask
				}
				else if (m_CurState == state_normal || m_CurState == state_unselected)
				{
					gc->SetBrush(m_RoundBgColor);
					//draw spacer area  background
					gc->DrawEllipse(m_GapWidth, m_GapWidth, iRoundDiameter - 2 * m_GapWidth, iRoundDiameter - 2 * m_GapWidth);
					dc.SetTextForeground(*wxWHITE);
					dc.SetFont(ANKER_BOLD_FONT_SIZE(12));
					wxSize size = GetSize();
					wxCoord w, h;
					dc.GetTextExtent(m_InnnerText, &w, &h);
					dc.DrawText(m_InnnerText, (iRoundDiameter - w) / 2, (iRoundDiameter - h) / 2);
				}
				else if (m_CurState == state_unknown)
				{
					gc->SetPen(m_BorderColor);
					gc->SetBrush(*wxTRANSPARENT_BRUSH);
					////draw Circle  area
					gc->DrawEllipse(m_GapWidth, m_GapWidth, iRoundDiameter - 2 * m_GapWidth, iRoundDiameter - 2 * m_GapWidth);

					int iRadius = (iRoundDiameter - 2 * m_GapWidth) / 2;
					int iQuarter = (iRoundDiameter - 2 * m_GapWidth) * 0.35;

					// draw cross
					gc->StrokeLine(m_GapWidth + iRadius - iQuarter, m_GapWidth + iRadius - iQuarter, m_GapWidth +  iRadius + iQuarter, m_GapWidth + iRadius + iQuarter);
					gc->StrokeLine(m_GapWidth +  iRadius - iQuarter, m_GapWidth +  iRadius + iQuarter, m_GapWidth + iRadius + iQuarter, m_GapWidth + iRadius - iQuarter);
				}


				delete gc;
			}
		}
		void AnkerStateRoundPanel::OnBtnPressed(wxMouseEvent& event)
		{
			if (m_CurState == state_selected)
			{
				SetState(state_normal);
			}
			else if (m_CurState == state_normal)
			{
				SetState(state_selected);
			}
			
			Refresh();
		}
		//define event map
		BEGIN_EVENT_TABLE(AnkerStateRoundPanel, AnkerRoundPanel)
		EVT_LEFT_DOWN(AnkerStateRoundPanel::OnBtnPressed)
		END_EVENT_TABLE()

		
		AnkerStateRoundTextPanel::AnkerStateRoundTextPanel(wxWindow* parent):AnkerBasePanel(parent), m_pTextCtrl(nullptr)
		{
			m_iTextSpan = 3;
			//m_strDesc = "?";
			InitUi();
			//Bind(wxEVT_PAINT, &AnkerStateRoundTextPanel::OnPaint, this);
		}

		AnkerStateRoundTextPanel::~AnkerStateRoundTextPanel()
		{

		}

		void AnkerStateRoundTextPanel::InitUi()
		{
			wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
			m_RoundPanel = new AnkerStateRoundPanel(this);
			//m_RoundPanel->SetMinSize(wxSize(60, 60));
			sizer->Add(m_RoundPanel,1,wxEXPAND);
			sizer->AddSpacer(m_iTextSpan);

			m_pTextCtrl = new AnkerStaicText(this,wxID_ANY, m_strDesc);
			// set Font 
			m_pTextCtrl->SetFont(ANKER_FONT_SIZE(8));

			// set font clolor
			wxColour colour(*wxRED);
			m_pTextCtrl->SetForegroundColour(colour);
			m_pTextCtrl->SetSizeHints(AnkerSize(-1,14), AnkerSize(-1, 14));
			sizer->Add(m_pTextCtrl,0,wxALIGN_CENTRE_VERTICAL|wxALIGN_CENTRE_HORIZONTAL);
			SetSizer(sizer);
		}

		void AnkerStateRoundTextPanel::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC dc(this);
		/*	dc.SetTextForeground(*wxWHITE);
			dc.SetFont(ANKER_BOLD_FONT_SIZE(12));
			wxSize size = GetSize(); 
			wxCoord w, h;
			dc.GetTextExtent(m_InnnerText, &w, &h);
			dc.DrawText(m_InnnerText, (size.x - w) / 2, (size.y - h) / 2);*/

			//draw describe text
			dc.SetTextForeground(*wxWHITE);
			wxSize size = GetSize();
			dc.SetFont(ANKER_BOLD_FONT_SIZE(8));
			wxCoord w2, h2;
			dc.GetTextExtent(m_strDesc, &w2, &h2);
			dc.DrawText(m_strDesc, (size.x - w2) / 2, size.GetWidth() - 14);

			//AnkerStateRoundPanel::OnPaint(event);
		}	

	}
}

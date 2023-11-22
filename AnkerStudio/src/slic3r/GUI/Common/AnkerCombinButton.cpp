#include <wx/wx.h>
#include <wx/window.h>
#include <wx/button.h>
#include <wx/image.h>
#include <wx/gbsizer.h>
#include "AnkerStaticText.hpp"
#include "AnkerCombinButton.h"
#include "AnkerGUIConfig.hpp"


namespace Slic3r {
	namespace GUI {

		AnkerCombinButton::AnkerCombinButton(wxWindow* parent, const wxBitmap& bitmap, const wxString& label)
			: wxButton(parent, wxID_ANY, label), m_Text(label), m_bitMap(bitmap), m_btnState(STATE_NORMAL)
		{
			m_backgroundColour = wxColour(DEFAULT_BG_COLOUR);
			m_foregroundColour = wxColour(DEFAULT_FG_COLOUR);
			//m_font = wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_MEDIUM, false, "Roboto");
			m_font = ANKER_BOLD_FONT_NO_1;
			m_bSelected = false;
			m_imageType = TYPE_BITMAP;
			initUI();
			initEvent();
		}

		AnkerCombinButton::AnkerCombinButton(wxWindow* parent, ScalableBitmap& scalabelBitmap, const wxString& label)
			:wxButton(parent, wxID_ANY, label), m_Text(label), m_normalSvg(scalabelBitmap), m_btnState(STATE_NORMAL)
		{
			m_backgroundColour = wxColour(DEFAULT_BG_COLOUR);
			m_foregroundColour = wxColour(DEFAULT_FG_COLOUR);
			m_font = ANKER_BOLD_FONT_NO_1;
			//m_font = wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_MEDIUM, false, "Roboto");
			m_bSelected = false;
			m_imageType = TYPE_SVG;
			initUI();
			initEvent();
		}

		void AnkerCombinButton::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC dc(this);
			switch (m_btnState)
			{
			case Slic3r::GUI::STATE_NORMAL:
			{
				m_textControl->SetForegroundColour(m_foregroundColour);
				dc.SetBrush(wxBrush(m_backgroundColour));
				m_textControl->SetBackgroundColour(m_backgroundColour);
				if (m_imageType == TYPE_BITMAP)
				{
					m_imageControl->SetBackgroundColour(m_backgroundColour);
				}
				else
				{
					m_usedSvgBitMap->setBitMap(m_normalSvg);
					m_usedSvgBitMap->activate();
				}
				break;
			}
			case Slic3r::GUI::STATE_HOVER:
			{
				dc.SetBrush(wxBrush(wxColour(SELECTED_BG_COLOUR)));
				m_textControl->SetForegroundColour(SELECTED_FG_COLOUR);
				m_textControl->SetBackgroundColour(SELECTED_BG_COLOUR);
				if (m_imageType == TYPE_BITMAP)
				{
					m_imageControl->SetBackgroundColour(wxColour(SELECTED_BG_COLOUR));
				}
				else
				{
					m_usedSvgBitMap->setBitMap(m_activedSvg);
					m_usedSvgBitMap->activate();
				}
				break;
			}
			case Slic3r::GUI::STATE_PRESSED:
			{
				dc.SetBrush(wxBrush(wxColour(SELECTED_BG_COLOUR)));
				m_textControl->SetForegroundColour(SELECTED_FG_COLOUR);
				m_textControl->SetBackgroundColour(SELECTED_BG_COLOUR);
				if (m_imageType == TYPE_BITMAP)
				{
					m_imageControl->SetBackgroundColour(wxColour(SELECTED_BG_COLOUR));
				}
				else
				{
					m_usedSvgBitMap->setBitMap(m_activedSvg);
					m_usedSvgBitMap->activate();
				}
				break;
			}

			case Slic3r::GUI::STATE_DISBALED:
				break;
			default:
				break;
			}


			/*if (m_bSelected)
			{
				dc.SetBrush(wxBrush(wxColour(SELECTED_BG_COLOUR)));
				m_textControl->SetForegroundColour(SELECTED_FG_COLOUR);
				m_textControl->SetBackgroundColour(SELECTED_BG_COLOUR);
				if (m_imageType == TYPE_BITMAP)
				{
					m_imageControl->SetBitmap(m_activeBitMap);
					m_imageControl->SetBackgroundColour(wxColour(SELECTED_BG_COLOUR));
				}
				else
				{
					m_usedSvgBitMap->setBitMap(m_activedSvg);
					m_usedSvgBitMap->activate();
				}
			}
			else
			{
				m_textControl->SetForegroundColour(m_foregroundColour);
				dc.SetBrush(wxBrush(m_backgroundColour));
				m_textControl->SetBackgroundColour(m_backgroundColour);
				if (m_imageType == TYPE_BITMAP)
				{
					m_imageControl->SetBitmap(m_bitMap);
					m_imageControl->SetBackgroundColour(m_backgroundColour);
				}
				else
				{
					m_usedSvgBitMap->setBitMap(m_normalSvg);
					m_usedSvgBitMap->activate();
				}
			}*/


			dc.SetPen(*wxTRANSPARENT_PEN);
			wxSize size = GetSize();
			dc.DrawRectangle(0, 0, size.x, GetSize().y);
		}

		void AnkerCombinButton::initUI()
		{
			wxGridBagSizer* sizer = new wxGridBagSizer();
			//m_usedSvgBitMap = new BlinkingBitmap(this);
			if (m_imageType == TYPE_BITMAP)
			{
				m_imageControl = new wxStaticBitmap(this, wxID_ANY, m_bitMap);
				sizer->Add(m_imageControl, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
				m_imageControl->SetBackgroundColour(m_backgroundColour);
				m_imageControl->Update();
			}
			else
			{
				//use svg display 
				std::string OGIconName = "image_active";
				m_activedSvg = ScalableBitmap(this, OGIconName);
				m_usedSvgBitMap->setBitMap(m_normalSvg);
				sizer->Add(m_usedSvgBitMap, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
			}
			m_textControl = new AnkerStaticText(this, wxID_ANY, m_Text);
			m_textControl->SetBackgroundColour(m_backgroundColour);
			m_textControl->SetForegroundColour(m_foregroundColour);
			m_textControl->SetFont(m_font);
			//m_textControl->Refresh();

			//Set the left margin of the text control to 8px
			sizer->Add(m_textControl, wxGBPosition(0, 1), wxGBSpan(1, 1), wxLEFT | wxALIGN_CENTER_VERTICAL, 8);

			// Set the row and column growth ratio to 1 so that the button stays centered as it gets bigger
			sizer->AddGrowableRow(0, 1);
			sizer->AddGrowableCol(0, 1);
			sizer->AddGrowableCol(1, 1);

			//Set the top and bottom margins to half the height of the button to maintain proportional spacing
			sizer->SetEmptyCellSize(wxSize(0, GetSize().GetHeight() / 2));
			SetLabel("");
			SetSizerAndFit(sizer);
		}


		void AnkerCombinButton::initEvent()
		{
			m_textControl->Bind(wxEVT_LEFT_DOWN, &AnkerCombinButton::OnChildMouseDown, this);
			m_imageControl->Bind(wxEVT_LEFT_DOWN, &AnkerCombinButton::OnChildMouseDown, this);

			m_textControl->Bind(wxEVT_LEAVE_WINDOW, &AnkerCombinButton::OnChildWindowLeave, this);
			m_imageControl->Bind(wxEVT_LEAVE_WINDOW, &AnkerCombinButton::OnChildWindowLeave, this);
		}

		void AnkerCombinButton::OnChildMouseDown(wxMouseEvent& event)
		{
			wxCommandEvent evt(wxEVT_BUTTON, GetId());
			evt.SetEventObject(this);
			ProcessEvent(evt);
			event.Skip();
		}

		void AnkerCombinButton::OnChildWindowLeave(wxMouseEvent& event)
		{
			wxPostEvent(this, event);
			event.Skip();
		}

		void AnkerCombinButton::OnMouseEnter(wxMouseEvent& event)
		{
//only call on windows, CaptureMouse will affect wxEVT_BUTTON handling
#ifdef _WIN32
			if (!HasCapture())
			{
				CaptureMouse();
			}
#endif

			if (m_btnState == STATE_PRESSED || m_btnState == STATE_DISBALED || m_bSelected)
			{
				return;
			}

			m_btnState = STATE_HOVER;
			m_imageControl->SetBitmap(m_activeBitMap);
			m_imageControl->Refresh();
			SetCursor(wxCursor(wxCURSOR_HAND));
			Refresh();
			event.Skip();
		}

		void AnkerCombinButton::OnMouseLeave(wxMouseEvent& event)
		{
//only call on windows, CaptureMouse will affect wxEVT_BUTTON handling
#ifdef _WIN32
			if (HasCapture())
			{
				ReleaseMouse();
			}
#endif // WIN32

			if (m_btnState == STATE_PRESSED || m_btnState == STATE_DISBALED || m_bSelected)
			{
				event.Skip();
				return;
			}

			m_btnState = STATE_NORMAL;
			m_imageControl->SetBitmap(m_bitMap);
			m_imageControl->Refresh();
			SetCursor(wxCursor(wxCURSOR_NONE));
			Refresh();
			event.Skip();
		}

		void AnkerCombinButton::OnBtnPressed(wxMouseEvent& event)
		{
			if (m_btnState == STATE_PRESSED)
			{
				event.Skip();
			}
			else
			{
				m_btnState = STATE_PRESSED;
				m_imageControl->SetBitmap(m_activeBitMap);
				m_imageControl->Refresh();
				Refresh();
				event.Skip();
			}
		}


		BEGIN_EVENT_TABLE(AnkerCombinButton, wxButton)
			EVT_PAINT(AnkerCombinButton::OnPaint)
			EVT_ENTER_WINDOW(AnkerCombinButton::OnMouseEnter)
			EVT_LEAVE_WINDOW(AnkerCombinButton::OnMouseLeave)
			EVT_LEFT_DOWN(AnkerCombinButton::OnBtnPressed)
			END_EVENT_TABLE()
	}
}
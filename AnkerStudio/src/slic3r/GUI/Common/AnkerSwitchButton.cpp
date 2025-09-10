#include <wx/wx.h>
#include <wx/window.h>
#include <wx/button.h>
#include <wx/image.h>
#include <wx/gbsizer.h>
#include "AnkerSwitchButton.hpp"
#include "AnkerGUIConfig.hpp"


namespace Slic3r {
	namespace GUI {
		
		AnkerSwitchButton::AnkerSwitchButton(wxWindow* parent, const wxBitmap& bitmap)
			: wxButton(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxNO_BORDER )
			, m_bitMap(bitmap), m_btnState(STATE_NORMAL)
		{
			m_backgroundColour = wxColour(DEFAULT_BG_COLOUR);
			m_foregroundColour = wxColour(DEFAULT_FG_COLOUR);
			m_bSelected = false;
			m_imageType = TYPE_BITMAP;
			initUI();
			initEvent();
		}

		AnkerSwitchButton::AnkerSwitchButton(wxWindow* parent, ScalableBitmap& scalabelBitmap)
			:wxButton(parent, wxID_ANY), m_normalSvg(scalabelBitmap), m_btnState(STATE_NORMAL)
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

		void AnkerSwitchButton::SetSelected(bool bSelected)
		{
			m_bSelected = bSelected;
			if (m_bSelected)
			{
				m_btnState = STATE_PRESSED;
				m_imageControl->SetBitmap(m_activeBitMap);
				m_imageControl->Refresh();
			}
			else
			{
				m_btnState = STATE_NORMAL;
				m_imageControl->SetBitmap(m_bitMap);
				m_imageControl->Refresh();
			}
		}

		void AnkerSwitchButton::initUI()
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
			
			// Set the row and column growth ratio to 1 so that the button stays centered as it gets bigger
			sizer->AddGrowableRow(0, 1);
			sizer->AddGrowableCol(0, 1);
			sizer->AddGrowableCol(1, 1);
			SetSizerAndFit(sizer);
		}


		void AnkerSwitchButton::initEvent()
		{
			m_imageControl->Bind(wxEVT_LEFT_DOWN, &AnkerSwitchButton::OnChildMouseDown, this);
			Bind(wxEVT_SIZE, &AnkerSwitchButton::OnResize, this);
		}

		void AnkerSwitchButton::OnChildMouseDown(wxMouseEvent& event)
		{
			SetSelected(!m_bSelected);
			wxCommandEvent evt(wxEVT_BUTTON, GetId());
			evt.SetEventObject(this);
			ProcessEvent(evt);
			event.Skip();
		}

		void AnkerSwitchButton::OnMouseEnter(wxMouseEvent& event)
		{
//only call on windows, CaptureMouse will affect wxEVT_BUTTON handling
#ifdef _WIN32
			if (!HasCapture())
			{
				CaptureMouse();
			}
#endif
			SetCursor(wxCursor(wxCURSOR_HAND));
			Refresh();
			event.Skip();
		}

		void AnkerSwitchButton::OnMouseLeave(wxMouseEvent& event)
		{
//only call on windows, CaptureMouse will affect wxEVT_BUTTON handling
#ifdef _WIN32
			if (HasCapture())
			{
				ReleaseMouse();
			}
#endif // WIN32
			SetCursor(wxCursor(wxCURSOR_NONE));
			Refresh();
			event.Skip();
		}

		void AnkerSwitchButton::OnResize(wxSizeEvent& event)
		{
			wxSize newSize = m_imageControl->GetSize();
			SetSize(newSize);
			Refresh();
			event.Skip();
		}

	}
}
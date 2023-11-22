#pragma once
#include  <wx/sizer.h>
#include "../wxExtensions.hpp"
#include "AnkerStaticText.hpp"

namespace Slic3r {
	namespace GUI {

		enum bgImgType {
			TYPE_SVG,
			TYPE_BITMAP
		};


		enum buttonState {
			STATE_NORMAL,
			STATE_HOVER,
			STATE_PRESSED,
			STATE_DISBALED
		};

#define SELECTED_BG_COLOUR  "#292A2D"
#define SELECTED_FG_COLOUR	"#62D361"
#define DEFAULT_BG_COLOUR  "#202124"
#define DEFAULT_FG_COLOUR  "#ffffff"


		class AnkerCombinButton :public wxButton {

		public:
			AnkerCombinButton(wxWindow* parent, const wxBitmap& bitmap, const wxString& label);
			AnkerCombinButton(wxWindow* parent, ScalableBitmap& scalabelBitmap, const wxString& label);
			void OnPaint(wxPaintEvent& event);
			void SetSelected(bool bSelected)
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
			};
			bool GetSelected() { return m_bSelected; }
			void SetActieBitMap(wxBitmap& bitmap) { m_activeBitMap = bitmap; };
			void SetActieSvg(BlinkingBitmap* activeSvg) { m_activedSvg = activeSvg; };
			void OnChildMouseDown(wxMouseEvent& event);
			void OnChildWindowLeave(wxMouseEvent& event);
			void OnMouseEnter(wxMouseEvent& event);
			void OnMouseLeave(wxMouseEvent& event);
			void OnBtnPressed(wxMouseEvent& event);


		private:
			void initUI();
			void initEvent();

		private:
			wxBitmap m_bitMap;
			wxBitmap m_activeBitMap;

			ScalableBitmap m_normalSvg;
			ScalableBitmap m_activedSvg;
			BlinkingBitmap* m_usedSvgBitMap;

			//wxSVGCtrl
			wxString m_Text;
			bool	m_bSelected;
			wxStaticBitmap* m_imageControl;
			AnkerStaticText* m_textControl;
			bgImgType		m_imageType;
			buttonState		m_btnState;
			DECLARE_EVENT_TABLE()
		};
	}
}

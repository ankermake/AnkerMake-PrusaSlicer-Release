#pragma once
#include  <wx/sizer.h>
#include "../wxExtensions.hpp"


namespace Slic3r {
	namespace GUI {


#define SELECTED_BG_COLOUR  "#292A2D"
#define SELECTED_FG_COLOUR	"#62D361"
#define DEFAULT_BG_COLOUR  "#202124"
#define DEFAULT_FG_COLOUR  "#ffffff"


		class AnkerSwitchButton :public wxButton {

		public:
			AnkerSwitchButton(wxWindow* parent, const wxBitmap& bitmap);
			AnkerSwitchButton(wxWindow* parent, ScalableBitmap& scalabelBitmap);
			void OnPaint(wxPaintEvent& event);
			void SetSelected(bool bSelected);
			bool GetSelected() { return m_bSelected; }
			void SetActieBitMap(wxBitmap& bitmap) { m_activeBitMap = bitmap; };
			void SetActieSvg(BlinkingBitmap* activeSvg) { m_activedSvg = activeSvg; };
			void OnChildMouseDown(wxMouseEvent& event);
			void OnChildWindowLeave(wxMouseEvent& event);
			void OnMouseEnter(wxMouseEvent& event);
			void OnMouseLeave(wxMouseEvent& event);
			void OnBtnPressed(wxMouseEvent& event);
			void OnResize(wxSizeEvent& event);


		private:
			void initUI();
			void initEvent();

		private:
		enum bgImgType {
			TYPE_SVG,
			TYPE_BITMAP
		};


		enum buttonState {
			STATE_NORMAL,
			STATE_PRESSED,
			STATE_DISBALED
		};


			wxBitmap m_bitMap;
			wxBitmap m_activeBitMap;

			ScalableBitmap m_normalSvg;
			ScalableBitmap m_activedSvg;
			BlinkingBitmap* m_usedSvgBitMap;

			//wxSVGCtrl
			bool	m_bSelected;
			wxStaticBitmap* m_imageControl;
			bgImgType		m_imageType;
			buttonState		m_btnState;
		
		};
	}
}

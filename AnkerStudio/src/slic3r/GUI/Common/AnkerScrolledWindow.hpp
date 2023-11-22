#pragma once
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/splitter.h>
#include "AnkerScrollbar.hpp"

namespace Slic3r {
    namespace GUI {
        class AnkerScrollbar;

        class AnkerScrolledWindow : public wxScrolled<wxWindow>
        {
        public:
            AnkerScrolledWindow(wxWindow* parent, wxWindowID id, wxPoint position, wxSize size, long style, int marginWidth = 0, int scrollbarWidth = 4, int tipLength = 0);
            void OnMouseWheel(wxMouseEvent& event);
            void SetTipColor(wxColour color);
            void Refresh();
            void SetBackgroundColour(wxColour color);

            void         SetMarginColor(wxColour color);
            void         SetScrollbarColor(wxColour color);
            void         SetScrollbarTip(int len);
            virtual void SetVirtualSize(int x, int y);
            virtual void SetVirtualSize(wxSize& size);
            wxPanel* GetPanel() { return m_userPanel; }
            // wxSplitterWindow* GetVerticalSplitter() { return m_verticalSplitter; }
            // wxSplitterWindow* GetHorizontalSplitter() { return m_horizontalSplitter; }
            bool         IsBothDirections() { return m_bothDirections; }
            virtual void SetScrollbars(int pixelsPerUnitX, int pixelsPerUnitY, int noUnitsX, int noUnitsY, int xPos = 0, int yPos = 0, bool noRefresh = false);

        private:
            wxPanel* m_userPanel{nullptr}; // the panel targeted by the scrolled window
            wxWindow* m_scroll_win{ nullptr };
            AnkerScrollbar* m_rightScrollbar{ nullptr };
            AnkerScrollbar* m_bottomScrollbar{ nullptr };
            // wxSplitterWindow* m_verticalSplitter;
            wxWindow* m_verticalSplitter{ nullptr };
            wxSplitterWindow* m_horizontalSplitter{ nullptr };
            int               m_marginWidth;
            bool              m_bothDirections;

            void OnSize(wxSizeEvent& WXUNUSED(event));
            void OnScroll(wxScrollWinEvent& event);
        };
    }
}

#ifndef slic3r_Anker_OG_CustomCtrl_hpp_
#define slic3r_Anker_OG_CustomCtrl_hpp_

#include <wx/stattext.h>
#include <wx/settings.h>

#include <map>
#include <functional>

#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"

#include "AnkerOptionsGroup.hpp"

// Translate the ifdef 
#ifdef __WXOSX__
    #define wxOSX true
#else
    #define wxOSX false
#endif


// add by dhf for ankerPage
namespace Slic3r { namespace GUI {

//  Static text shown among the options.
class Anker_OG_CustomCtrl :public wxPanel
{
    wxFont  m_font;
    int     m_v_gap;
    int     m_h_gap;
    int     m_em_unit;

    wxSize  m_bmp_mode_sz;
    wxSize  m_bmp_blinking_sz;

    int     m_max_win_width{0};

    struct CtrlLine {
        wxCoord           height  { wxDefaultCoord };
        Anker_OG_CustomCtrl*    ctrl    { nullptr };
        const AnkerLine&       og_line;

        bool draw_just_act_buttons  { false };
        bool draw_mode_bitmap       { true };
        bool is_visible             { true };
        bool is_focused             { false };

        CtrlLine(   wxCoord         height,
                    Anker_OG_CustomCtrl*  ctrl,
                    const AnkerLine&     og_line,
                    bool            draw_just_act_buttons = false,
                    bool            draw_mode_bitmap = true);
        ~CtrlLine() { ctrl = nullptr; }

        int     get_max_win_width();
        void    correct_items_positions();
        void    msw_rescale();
        void    update_visibility(ConfigOptionMode mode);

        void render_separator(wxDC& dc, wxCoord v_pos);

        void    render(wxDC& dc, wxCoord v_pos);
        wxCoord draw_mode_bmp(wxDC& dc, wxCoord v_pos);
        wxCoord draw_text      (wxDC& dc, wxPoint pos, const wxString& text, const wxColour* color, int width, bool is_url = false);
        wxPoint draw_blinking_bmp(wxDC& dc, wxPoint pos, bool is_blinking);
        wxCoord draw_act_bmps(wxDC& dc, wxPoint pos, const wxBitmapBundle& bmp_undo_to_sys, const wxBitmapBundle& bmp_undo, bool is_blinking, size_t rect_id = 0);
        wxCoord draw_act_bmps(wxDC& dc, wxPoint pos, const wxBitmapBundle& bmp_undo, bool is_blinking, size_t rect_id = 0);
        bool    launch_browser() const;
        bool    is_separator() const { return og_line.is_separator(); }

        std::vector<wxRect> rects_undo_icon;
        std::vector<wxRect> rects_undo_to_sys_icon;
        wxRect              rect_label;
    };

    std::vector<CtrlLine> ctrl_lines;

public:
    Anker_OG_CustomCtrl(  wxWindow* parent,
                    AnkerOptionsGroup* og,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    const wxValidator& val = wxDefaultValidator,
                    const wxString& name = wxEmptyString);
    ~Anker_OG_CustomCtrl() {}

    void    OnPaint(wxPaintEvent&);
    void    OnMotion(wxMouseEvent& event);
    void    OnLeftDown(wxMouseEvent& event);
    void    OnLeaveWin(wxMouseEvent& event);
    void    OnSize(wxSizeEvent& event);

    void    init_ctrl_lines();
    bool    update_visibility(ConfigOptionMode mode);
    void    correct_window_position(wxWindow* win, const AnkerLine& line, Field* field = nullptr, bool is_near_label_widget_win = false);
    void    correct_widgets_position(wxSizer* widget, const AnkerLine& line, Field* field = nullptr);
    void    init_max_win_width();
    void    set_max_win_width(int max_win_width);
    int     get_max_win_width() { return m_max_win_width; }

    void    msw_rescale();
    void    sys_color_changed();

    wxPoint get_pos(const AnkerLine& line, Field* field = nullptr);
    int     get_height(const AnkerLine& line);

    AnkerOptionsGroup*  opt_group;

};

}}

#endif /* slic3r_Anker_OG_CustomCtrl_hpp_ */

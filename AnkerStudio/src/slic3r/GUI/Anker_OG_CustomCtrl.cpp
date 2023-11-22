#include "Anker_OG_CustomCtrl.hpp"
#include "AnkerOptionsGroup.hpp"
#include "Plater.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "libslic3r/AppConfig.hpp"

#include <wx/utils.h>
#include <boost/algorithm/string/split.hpp>
#include "libslic3r/Utils.hpp"
#include "I18N.hpp"
#include "format.hpp"

#include "common/AnkerGUIConfig.hpp"

// add by dhf for debug reason
static std::string num2Str(long long num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}

#if 0
#define PrintLog(logString) do { \
    auto now = std::chrono::system_clock::now(); \
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now); \
    auto ms = now_ms.time_since_epoch().count() % 1000; \
    auto t = std::chrono::system_clock::to_time_t(now); \
    std::ostringstream logStream; \
    char timeBuffer[100]; \
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d %H:%M:%S", std::localtime(&t)); \
    logStream << "[" << timeBuffer << "." << std::setfill('0') << std::setw(3) << ms << "][" \
              << std::this_thread::get_id() << "][" << /*__FILE__*/""<< "][" << __FUNCTION__ << "][" << __LINE__ << "]: " << logString; \
    std::cout << logStream.str() << std::endl; \
} while(0)
#else
#define PrintLog(logString) do{}while(0)
#endif

// add by dhf for ankerPage
namespace Slic3r { namespace GUI {

static bool is_point_in_rect(const wxPoint& pt, const wxRect& rect)
{
    return  rect.GetLeft() <= pt.x && pt.x <= rect.GetRight() &&
            rect.GetTop() <= pt.y && pt.y <= rect.GetBottom();
}

static wxSize get_bitmap_size(const wxBitmapBundle* bmp, wxWindow* parent)
{
    if (bmp) {
#ifdef __WIN32__
        return bmp->GetBitmapFor(parent).GetSize();
#else
        return bmp->GetDefaultSize();
#endif
    }

    return wxSize(0,0);
}

Anker_OG_CustomCtrl::Anker_OG_CustomCtrl(   wxWindow*            parent,
                                AnkerOptionsGroup*        og,
                                const wxPoint&       pos /* = wxDefaultPosition*/,
                                const wxSize&        size/* = wxDefaultSize*/,
                                const wxValidator&   val /* = wxDefaultValidator*/,
                                const wxString&      name/* = wxEmptyString*/) :
    wxPanel(parent, wxID_ANY, pos, size, /*wxWANTS_CHARS |*/ wxBORDER_NONE | wxTAB_TRAVERSAL),
    opt_group(og)
{
    if (!wxOSX)
        SetDoubleBuffered(true);// SetDoubleBuffered exists on Win and Linux/GTK, but is missing on OSX

    m_font = ANKER_FONT_NO_1;// wxGetApp().normal_font();
    m_em_unit   = em_unit(m_parent);
    m_v_gap     = lround(1.1 * m_em_unit);
    m_h_gap     = lround(0.2 * m_em_unit);

    m_bmp_mode_sz       = get_bitmap_size(get_bmp_bundle("mode", wxOSX ? 10 : 12), this);
    m_bmp_blinking_sz   = get_bitmap_size(get_bmp_bundle("search_blink"), this);

    init_ctrl_lines();// from og.lines()

    this->Bind(wxEVT_PAINT,     &Anker_OG_CustomCtrl::OnPaint, this);
    this->Bind(wxEVT_MOTION,    &Anker_OG_CustomCtrl::OnMotion, this);
    this->Bind(wxEVT_LEFT_DOWN, &Anker_OG_CustomCtrl::OnLeftDown, this);
    this->Bind(wxEVT_LEAVE_WINDOW, &Anker_OG_CustomCtrl::OnLeaveWin, this);
    this->Bind(wxEVT_SIZE,      &Anker_OG_CustomCtrl::OnSize, this);
}

void Anker_OG_CustomCtrl::init_ctrl_lines()
{
    //std::cout << "          ==Anker_OG_CustomCtrl::init_ctrl_lines, m_max_win_width=" << m_max_win_width << "      OG_ctrl size:" << this->GetSize().GetWidth() << "," << this->GetSize().GetHeight() << "      groupname:" << this->opt_group->title << std::endl;
    const std::vector<AnkerLine>& og_lines = opt_group->get_lines();
    for (const AnkerLine& line : og_lines)
    {
        if (line.is_separator()) {
            ctrl_lines.emplace_back(CtrlLine(0, this, line));
            continue;
        }

        if (line.full_width && (
            // description line
            line.widget != nullptr ||
            // description line with widget (button)
            !line.get_extra_widgets().empty())
            )
            continue;

        const std::vector<AnkerOption>& option_set = line.get_options();
        wxCoord height;

        // if we have a single option with no label, no sidetext just add it directly to sizer
        if (option_set.size() == 1 && opt_group->label_width == 0 && option_set.front().opt.full_width &&
            option_set.front().opt.sidetext.size() == 0 && option_set.front().side_widget == nullptr &&
            line.get_extra_widgets().size() == 0)
        {
            height = m_bmp_blinking_sz.GetHeight() + m_v_gap;
            ctrl_lines.emplace_back(CtrlLine(height, this, line, true));
        }
        else if (opt_group->label_width != 0 && (!line.label.IsEmpty() || option_set.front().opt.gui_type == ConfigOptionDef::GUIType::legend) )
        {
            wxSize label_sz = GetTextExtent(line.label);
            height = label_sz.y * (label_sz.GetWidth() > int(opt_group->label_width * m_em_unit) ? 2 : 1) + m_v_gap;
            ctrl_lines.emplace_back(CtrlLine(height, this, line, false, opt_group->staticbox));
        }
        else
            assert(false);
    }
}

int Anker_OG_CustomCtrl::get_height(const AnkerLine& line)
{
    for (auto ctrl_line : ctrl_lines)
        if (&ctrl_line.og_line == &line)
            return ctrl_line.height;
        
    return 0;
}

wxPoint Anker_OG_CustomCtrl::get_pos(const AnkerLine& line, Field* field_in/* = nullptr*/)
{
    wxCoord v_pos = 0;
    wxCoord h_pos = 0;

    auto correct_line_height = [](int& line_height, wxWindow* win)
    {
        int win_height = win->GetSize().GetHeight();
        if (line_height < win_height)
            line_height = win_height;
    };

    auto correct_horiz_pos = [this](int& h_pos, Field* field) {
        if (m_max_win_width > 0 && field->getWindow()) {
            int win_width = field->getWindow()->GetSize().GetWidth();
            if (dynamic_cast<AnkerCheckBoxField*>(field))
                win_width *= 0.5;
            h_pos += m_max_win_width - win_width;
        }
    };

    for (CtrlLine& ctrl_line : ctrl_lines) {
        if (&ctrl_line.og_line == &line)
        {
            h_pos = m_bmp_mode_sz.GetWidth() + m_h_gap;
            if (line.near_label_widget_win) {
                wxSize near_label_widget_sz = line.near_label_widget_win->GetSize();
                if (field_in)
                    h_pos += near_label_widget_sz.GetWidth() + m_h_gap;
                else
                    break;
            }

            wxString label = line.label;
            if (opt_group->label_width != 0)
                h_pos += opt_group->label_width * m_em_unit + m_h_gap;

            int blinking_button_width = m_bmp_blinking_sz.GetWidth() + m_h_gap;

            if (line.widget) {
                h_pos += (line.has_undo_ui() ? 3 : 1) * blinking_button_width;

                for (auto child : line.widget_sizer->GetChildren())
                    if (child->IsWindow())
                        correct_line_height(ctrl_line.height, child->GetWindow());
                break;
            }

            // If we have a single option with no sidetext
            const std::vector<AnkerOption>& option_set = line.get_options();
            if (option_set.size() == 1 && option_set.front().opt.sidetext.size() == 0 &&
                option_set.front().side_widget == nullptr && line.get_extra_widgets().size() == 0)
            {
                h_pos += 3 * blinking_button_width;
                Field* field = opt_group->get_field(option_set.front().opt_id);
                correct_line_height(ctrl_line.height, field->getWindow());
                correct_horiz_pos(h_pos, field);
                break;
            }

            bool is_multioption_line = option_set.size() > 1;
            for (auto opt : option_set) {
                Field* field = opt_group->get_field(opt.opt_id);
                correct_line_height(ctrl_line.height, field->getWindow());

                ConfigOptionDef option = opt.opt;
                // add label if any
                if (is_multioption_line && !option.label.empty()) {
                    // those two parameter names require localization with context
                    label = (option.label == "Top" || option.label == "Bottom") ?
                        _CTX(option.label, "Layers") : _(option.label);
                    label += ":";

                    wxCoord label_w, label_h;
#ifdef __WXMSW__
                    // when we use 2 monitors with different DPIs, GetTextExtent() return value for the primary display
                    // so, use dc.GetMultiLineTextExtent on Windows 
                    wxClientDC dc(this);
                    dc.SetFont(m_font);
                    dc.GetMultiLineTextExtent(label, &label_w, &label_h);
#else
                    GetTextExtent(label, &label_w, &label_h, 0, 0, &m_font);
#endif //__WXMSW__
                    h_pos += label_w + m_h_gap;
                }                
                h_pos += (opt.opt.gui_type == ConfigOptionDef::GUIType::legend ? 1 : 3) * blinking_button_width;
                
                if (field == field_in) {
                    correct_horiz_pos(h_pos, field);
                    break;
                }
                if (opt.opt.gui_type == ConfigOptionDef::GUIType::legend)
                    h_pos += 2 * blinking_button_width;

                h_pos += field->getWindow()->GetSize().x + m_h_gap;

                if (option_set.size() == 1 && option_set.front().opt.full_width)
                    break;

                // add sidetext if any
                if (!option.sidetext.empty() || opt_group->sidetext_width > 0)
                    h_pos += opt_group->sidetext_width * m_em_unit + m_h_gap;

                if (opt.opt_id != option_set.back().opt_id) //! istead of (opt != option_set.back())
                    h_pos += lround(0.6 * m_em_unit);
            }
            break;
        }
        if (ctrl_line.is_visible)
            v_pos += ctrl_line.height;
    }


    return wxPoint(h_pos, v_pos);
}


void Anker_OG_CustomCtrl::OnPaint(wxPaintEvent&)
{
    // std::cout << "=========================================Anker_OG_CustomCtrl::OnPaint, m_max_win_width=" << m_max_win_width << "      OG_ctrl size:" << this->GetSize().GetWidth() << "," << this->GetSize().GetHeight()<<  "      groupname:"<<this->opt_group->title << std::endl;
    // case, when custom controll is destroyed but doesn't deleted from the evet loop
    if(!this->opt_group->custom_ctrl)
        return;

    wxPaintDC dc(this);
    dc.SetFont(m_font);

    wxCoord v_pos = 0;
    for (CtrlLine& line : ctrl_lines) {
        if (!line.is_visible)
            continue;
        // add by dhf , to update field position
        //line.correct_items_positions();
        line.render(dc, v_pos);
        v_pos += line.height;
    }
}

void Anker_OG_CustomCtrl::OnMotion(wxMouseEvent& event)
{
    //std::cout << " ~~~~~~~~~~~~~~~~~~~~ OnMotion, OG_ctrl size:" + num2Str(this->GetSize().GetWidth()) << "," << num2Str(this->GetSize().GetHeight()) << "    group:" << opt_group->title << std::endl;
    const wxPoint pos = event.GetLogicalPosition(wxClientDC(this));
    std::string opt_key;
    wxString tooltip;

    wxString language = wxGetApp().app_config->get("translation_language");

    // const bool suppress_hyperlinks = get_app_config()->get_bool("suppress_hyperlinks");
    bool suppress_hyperlinks = true;

    for (CtrlLine& line : ctrl_lines) {
        const std::vector<AnkerOption>& option_set = line.og_line.get_options();
        for (int i = 0; i < option_set.size(); ++i) {
            opt_key = option_set[i].opt_id;
        }

        line.is_focused = is_point_in_rect(pos, line.rect_label);
        if (line.is_focused) {
            if (!suppress_hyperlinks && !line.og_line.label_path.empty())
                tooltip = AnkerOptionsGroup::get_url(line.og_line.label_path) +"\n\n";
            tooltip += line.og_line.label_tooltip;
            break;
        }

        size_t undo_icons_cnt = line.rects_undo_icon.size();
        assert(line.rects_undo_icon.size() == line.rects_undo_to_sys_icon.size());


        for (size_t opt_idx = 0; opt_idx < undo_icons_cnt; opt_idx++) {
            const std::string& opt_key = option_set[opt_idx].opt_id;
            if (is_point_in_rect(pos, line.rects_undo_icon[opt_idx])) {
                if (line.og_line.has_undo_ui())
                    tooltip = *line.og_line.undo_tooltip();
                else if (Field* field = opt_group->get_field(opt_key))
                    tooltip = *field->undo_tooltip();
                break;
            }
            if (is_point_in_rect(pos, line.rects_undo_to_sys_icon[opt_idx])) {
                if (line.og_line.has_undo_ui())
                    tooltip = *line.og_line.undo_to_sys_tooltip();
                else if (Field* field = opt_group->get_field(opt_key))
                    tooltip = *field->undo_to_sys_tooltip();
                break;
            }
        }
        if (!tooltip.IsEmpty())
            break;
    }

    Field::modify_tooltip_text(opt_key, tooltip);

    // Set tooltips with information for each icon
    this->SetToolTip(tooltip);

    Refresh();
    Update();
    event.Skip();
}

void Anker_OG_CustomCtrl::OnLeftDown(wxMouseEvent& event)
{
    const wxPoint pos = event.GetLogicalPosition(wxClientDC(this));

    for (const CtrlLine& line : ctrl_lines) {
        bool suppress_hyperlinks = true;
        if (!suppress_hyperlinks && line.launch_browser())
            return;

        size_t undo_icons_cnt = line.rects_undo_icon.size();
        assert(line.rects_undo_icon.size() == line.rects_undo_to_sys_icon.size());

        const std::vector<AnkerOption>& option_set = line.og_line.get_options();
        for (size_t opt_idx = 0; opt_idx < undo_icons_cnt; opt_idx++) {
            const std::string& opt_key = option_set[opt_idx].opt_id;

            if (is_point_in_rect(pos, line.rects_undo_icon[opt_idx])) {
                if (line.og_line.has_undo_ui()) {
                    if (ConfigOptionsGroup* conf_OG = dynamic_cast<ConfigOptionsGroup*>(line.ctrl->opt_group))
                        conf_OG->back_to_initial_value(opt_key);
                }
                else if (Field* field = opt_group->get_field(opt_key))
                    field->on_back_to_initial_value();
                event.Skip();
                return;
            }

            if (is_point_in_rect(pos, line.rects_undo_to_sys_icon[opt_idx])) {
                if (line.og_line.has_undo_ui()) {
                    if (ConfigOptionsGroup* conf_OG = dynamic_cast<ConfigOptionsGroup*>(line.ctrl->opt_group))
                        conf_OG->back_to_sys_value(opt_key);
                }
                else if (Field* field = opt_group->get_field(opt_key))
                    field->on_back_to_sys_value();
                event.Skip();
                return;
            }
        }
    }

}

void Anker_OG_CustomCtrl::OnLeaveWin(wxMouseEvent& event)
{
    for (CtrlLine& line : ctrl_lines)
        line.is_focused = false;

    Refresh();
    Update();
    event.Skip();
}

void Anker_OG_CustomCtrl::OnSize(wxSizeEvent& event) {
    wxWindow* parent = this->GetParent();
    if (parent) {
        update_visibility(comExpert);
    }
    Layout();
    Refresh();
}

bool Anker_OG_CustomCtrl::update_visibility(ConfigOptionMode mode)
{
    //std::cout<<" ~~~~~~~~~~~~~~~~~~~~ OG_ctrl size:" + num2Str(this->GetSize().GetWidth()) << "," << num2Str(this->GetSize().GetHeight())    << "    group:" << opt_group->title <<std::endl;
    wxCoord    v_pos = 0;

    size_t invisible_lines = 0;
    for (CtrlLine& line : ctrl_lines) {
        line.update_visibility(mode);
        if (line.is_visible)
            v_pos += (wxCoord)line.height;
        else
            invisible_lines++;
    }    

    this->SetMinSize(wxSize(wxDefaultCoord, v_pos));

    return invisible_lines != ctrl_lines.size();
}

// modify by dhf for set field position
void Anker_OG_CustomCtrl::correct_window_position(wxWindow* win, const AnkerLine& line, Field* the_field/* = nullptr*/, bool is_near_label_widget_win/* = false */)
{
     PrintLog( " ============= correct_window_position, OG_ctrl size:" + num2Str(this->GetSize().GetWidth()) +"," +num2Str(this->GetSize().GetHeight()) + "    group:" +opt_group->title );
     Field* field = the_field;
     wxPoint pos = get_pos(line, field);

    int old_x = pos.x;
    int line_height = get_height(line);
    pos.y += std::max(0, int(0.5 * (line_height - win->GetSize().y)));

    // modify by dhf for Anker_OG_CustomCtrl line, to set field pos at right side
    const std::vector<AnkerOption>& option_set = line.get_options();
    if (option_set.size() == 1) {
        const int indent = 20;
        if (is_near_label_widget_win) {
            pos.x = indent;
        }
        else {
            if (!win)
                return;

            int right_margin = 10;
            int field_win_width = win->GetSize().GetWidth();
            if (option_set.front().opt.full_width && field->getWindow()) {
                int label_width = GetTextExtent(line.label).x;
                int undo_bmp_width = 16; // field&& field->has_undo_ui() ? (get_bitmap_size(&(field->undo_bitmap()), this).GetWidth()) : 0;

                field_win_width = GetSize().x - indent - label_width - m_v_gap * 2 - undo_bmp_width  - m_v_gap * 2 - right_margin;

                PrintLog("                  ---->is full width ,set size:" + num2Str(field_win_width) + "   lable width:" + num2Str(label_width) + "    lable:" + line.label + "  m_v_gap:" + num2Str(m_v_gap));
                field->getWindow()->SetSize(field_win_width, -1);
            }
            pos.x = this->GetSize().GetWidth() - field_win_width - right_margin;
        }

// for debug
        PrintLog("      correct_window_position field x,y :" + num2Str(pos.x) + " " + num2Str(pos.y)
            + "  field width:" + num2Str(win ? win->GetSize().GetWidth() : -1)
            + "   old field x:" + num2Str(old_x)
            + "    OG_ctrl size:" + num2Str(this->GetSize().GetWidth()) << "," << num2Str(this->GetSize().GetHeight())
            + "    line:" + line.label);
        if (line.label == "First layer height")
        {
            int i = 0;
            ++i;
        }

        if (pos.x >= 0 && pos.x <= this->GetSize().GetWidth() /* && pos.x + (win ? win->GetSize().GetWidth() : -1) <= this->GetSize().GetWidth()*/) {
            win->SetPosition(pos);
            PrintLog("                  ----->setpositon:" + num2Str(pos.x) + "," + num2Str(pos.y));
        }
        win->Layout();
    }
    else
    {
        int og_ctrl_width = this->GetSize().GetWidth();
        int rightPos = og_ctrl_width;
        bool is_multioption_line = option_set.size() > 1;
        for (int i = option_set.size() - 1; i >= 0; --i) {
            const AnkerOption& opt = option_set[i];
            Field* curr_field = opt_group->get_field(opt.opt_id);
            if (!curr_field)
                continue;
            field = curr_field;
            //if (curr_field != field)
            //    continue;

            ConfigOptionDef option = opt.opt;
            PrintLog("             ---------------multi-->opt:" + opt.opt_id + "      option Label:" + option.label +
                "  field:" + num2Str((unsigned long long)field) +
                "   field pos x,y:" + num2Str(field->getWindow() ? field->getWindow()->GetPosition().x : -1) + "," + num2Str(field->getWindow() ? field->getWindow()->GetPosition().y : -1) +
                "  fieldsize w,h:" + num2Str(field->getWindow() ? field->getWindow()->GetSize().x : -1) + "*" + num2Str(field->getWindow() ? field->getWindow()->GetSize().y : -1)+
                "      get_pos() x,y:" + num2Str(pos.x) + " " + num2Str(pos.y)+
                "       og_ctrl_width:" + num2Str(og_ctrl_width)+
                "       rightPos:" + num2Str(rightPos)
            );

             // modify by dhf , some multioption line have no label befor the field
            if (is_multioption_line/* && !option.label.empty()*/) {
                // those two parameter names require localization with context
                wxString label = (option.label == "Top" || option.label == "Bottom") ?
                    _CTX(option.label, "Layers") : _(option.label);
                //label += ":";

                wxPaintDC dc(this);
                int label_width = GetTextExtent(label).x;
                int undo_bmp_width = field && field->has_undo_ui() ? (get_bitmap_size(&(field->undo_bitmap()), this).GetWidth()) : 0;

                wxSize field_win_size;
                wxPoint field_pos;
                if (field->getWindow()) {
                    field_win_size = field->getWindow()->GetSize();
                    field_pos = field->getWindow()->GetPosition();
                }
                else if (field->getSizer()) {
                }

                // field 
                int right_margin = 10;
                field_pos = wxPoint(rightPos - right_margin - field_win_size.GetWidth(), pos.y);
                if (field->getWindow()) {
                    PrintLog("                      ======>setfield pos:"+num2Str(field_pos.x)+" "+num2Str(field_pos.y)
                        +"        (rightpos:"+ num2Str(rightPos)    
                        +"  right_margin:" + num2Str(right_margin)
                        + "  field.Width:" + num2Str(field_win_size.GetWidth())
                    );
                    field->getWindow()->SetPosition(field_pos);
                    field->getWindow()->Layout();
                }
                rightPos -= (right_margin + field_win_size.GetWidth());

                // undo icon
            #if 0
                if (field && field->has_undo_ui()) {
                    //draw_act_bmps(dc, undo_bmp_pos, field->undo_bitmap(), field->blink(), bmp_rect_id++);
                    rightPos = rightPos - (m_h_gap * 5) - undo_bmp_width;
                }
            #else
                // left certain space to display undo
                undo_bmp_width = 16;
                rightPos = rightPos - (m_h_gap * 5) - undo_bmp_width;
            #endif

                // label
                rightPos = rightPos - (m_h_gap * 5) - label_width;
            }

        }
    }
};

void Anker_OG_CustomCtrl::correct_widgets_position(wxSizer* widget, const AnkerLine& line, Field* field/* = nullptr*/) {
    auto children = widget->GetChildren();

    wxPoint line_pos = get_pos(line, field);
    PrintLog("      correct_widgets_position:" + num2Str(line_pos.x) + " " + num2Str(line_pos.y)+"    line:"+ line.label);
    int line_height = get_height(line);

    // modify by dhf to suport right aliagn for when line have widget(line.widget)
#if 0
    // 
    for (auto child : children)
        if (child->IsWindow()) {
            wxPoint pos = line_pos;
            wxSize  sz = child->GetWindow()->GetSize();
            pos.y += std::max(0, int(0.5 * (line_height - sz.y)));
            if (line.extra_widget_sizer && widget == line.extra_widget_sizer)
                pos.x += m_h_gap;
            child->GetWindow()->SetPosition(pos);
            line_pos.x += sz.x + m_h_gap;
        }
#else
    // dhf fix "bed shape" in "general page" ; "compatible printer " in "depandencies" page  , bug will crash in "extruder 1"
    int rightPos = GetSize().GetWidth();
    int right_margin = 10;
    rightPos -= right_margin;
    if (1) {
        for (auto child = children.rbegin(); child != children.rend(); ++child)
            if ((*child)->IsWindow()) {
                wxPoint pos = line_pos;
                wxSize  sz = (*child)->GetWindow()->GetSize();
                pos.y += std::max(0, int(0.5 * (line_height - sz.y)));
                pos.x = rightPos - sz.x;
                (*child)->GetWindow()->SetPosition(pos);
                PrintLog("      ======>widget, setpPos" + num2Str(pos.x) + " " + num2Str(pos.y) + "    rightPos:" + num2Str(rightPos) + "  m_h_gap" + num2Str(m_h_gap));
                rightPos = pos.x - m_h_gap * 5;
            }
    }
#endif
};

void Anker_OG_CustomCtrl::init_max_win_width()
{
    m_max_win_width = 0;

    if (opt_group->ctrl_horiz_alignment == wxALIGN_RIGHT && m_max_win_width == 0)
        for (CtrlLine& line : ctrl_lines) {
            if (int max_win_width = line.get_max_win_width();
                m_max_win_width < max_win_width)
                m_max_win_width = max_win_width;
        }
    //PrintLog("++++++++++++++++m_max_win_width:"+num2Str(m_max_win_width));
}

void Anker_OG_CustomCtrl::set_max_win_width(int max_win_width)
{
    //PrintLog("++++++++++++++++set m_max_win_width:" + num2Str(m_max_win_width));
    if (m_max_win_width == max_win_width)
        return;
    m_max_win_width = max_win_width;
    for (CtrlLine& line : ctrl_lines)
        line.correct_items_positions();

    GetParent()->Layout();
}


void Anker_OG_CustomCtrl::msw_rescale()
{
#ifdef __WXOSX__
    return;
#endif
    m_font      = ANKER_FONT_NO_1;//wxGetApp().normal_font();
    m_em_unit   = em_unit(m_parent);
    m_v_gap     = lround(1.0 * m_em_unit);
    m_h_gap     = lround(0.2 * m_em_unit);

    m_bmp_mode_sz       = get_bitmap_size(get_bmp_bundle("mode", wxOSX ? 10 : 12), this);
    m_bmp_blinking_sz   = get_bitmap_size(get_bmp_bundle("search_blink"), this);

    init_max_win_width();

    wxCoord    v_pos = 0;
    for (CtrlLine& line : ctrl_lines) {
        line.msw_rescale();
        if (line.is_visible)
            v_pos += (wxCoord)line.height;
    }
    this->SetMinSize(wxSize(wxDefaultCoord, v_pos));

    GetParent()->Layout();
}

void Anker_OG_CustomCtrl::sys_color_changed()
{
}

Anker_OG_CustomCtrl::CtrlLine::CtrlLine(  wxCoord         height,
                                    Anker_OG_CustomCtrl*  ctrl,
                                    const AnkerLine&     og_line,
                                    bool            draw_just_act_buttons /* = false*/,
                                    bool            draw_mode_bitmap/* = true*/):
    height(height),
    ctrl(ctrl),
    og_line(og_line),
    draw_just_act_buttons(draw_just_act_buttons),
    draw_mode_bitmap(draw_mode_bitmap)
{

    for (size_t i = 0; i < og_line.get_options().size(); i++) {
        rects_undo_icon.emplace_back(wxRect());
        rects_undo_to_sys_icon.emplace_back(wxRect());
    }
}

int Anker_OG_CustomCtrl::CtrlLine::get_max_win_width()
{
    int max_win_width = 0;
    if (!draw_just_act_buttons) {
        const std::vector<AnkerOption>& option_set = og_line.get_options();
        for (auto opt : option_set) {
            Field* field = ctrl->opt_group->get_field(opt.opt_id);
            if (field && field->getWindow())
                max_win_width = field->getWindow()->GetSize().GetWidth();
        }
    }

    return max_win_width;
}

void Anker_OG_CustomCtrl::CtrlLine::correct_items_positions()
{
    if (draw_just_act_buttons || !is_visible)
        return;

    if (og_line.near_label_widget_win) {
        ctrl->correct_window_position(og_line.near_label_widget_win, og_line, nullptr, true);
    }
    if (og_line.widget_sizer)
        ctrl->correct_widgets_position(og_line.widget_sizer, og_line);
    if (og_line.extra_widget_sizer)
        ctrl->correct_widgets_position(og_line.extra_widget_sizer, og_line);

    const std::vector<AnkerOption>& option_set = og_line.get_options();
    for (auto opt : option_set) {
        Field* field = ctrl->opt_group->get_field(opt.opt_id);
        if (!field)
            continue;
        if (field->getSizer())
            ctrl->correct_widgets_position(field->getSizer(), og_line, field);
        else if (field->getWindow())
            ctrl->correct_window_position(field->getWindow(), og_line, field);
    }
}

void Anker_OG_CustomCtrl::CtrlLine::msw_rescale()
{
    // if we have a single option with no label, no sidetext
    if (draw_just_act_buttons)
        height = get_bitmap_size(get_bmp_bundle("empty"), ctrl).GetHeight();

    if (ctrl->opt_group->label_width != 0 && !og_line.label.IsEmpty()) {
        wxSize label_sz = ctrl->GetTextExtent(og_line.label);
        height = label_sz.y * (label_sz.GetWidth() > int(ctrl->opt_group->label_width * ctrl->m_em_unit) ? 2 : 1) + ctrl->m_v_gap;
    }

    correct_items_positions();
}

void Anker_OG_CustomCtrl::CtrlLine::update_visibility(ConfigOptionMode mode)
{
    if (og_line.is_separator())
        return;
    const std::vector<AnkerOption>& option_set = og_line.get_options();

    const ConfigOptionMode& line_mode = option_set.front().opt.mode;
    is_visible = line_mode <= mode;

    if (draw_just_act_buttons)
        return;

    if (og_line.near_label_widget_win)
        og_line.near_label_widget_win->Show(is_visible);
    if (og_line.widget_sizer)
        og_line.widget_sizer->ShowItems(is_visible);
    if (og_line.extra_widget_sizer)
        og_line.extra_widget_sizer->ShowItems(is_visible);

    for (auto opt : option_set) {
        Field* field = ctrl->opt_group->get_field(opt.opt_id);
        if (!field)
            continue;

        if (field->getSizer()) {
            auto children = field->getSizer()->GetChildren();
            for (auto child : children)
                if (child->IsWindow())
                    child->GetWindow()->Show(is_visible);
        }
        else if (field->getWindow())
            field->getWindow()->Show(is_visible);
    }

   correct_items_positions();
}

void Anker_OG_CustomCtrl::CtrlLine::render_separator(wxDC& dc, wxCoord v_pos)
{
    //std::cout << "=========================================render_separator" << std::endl;
    wxPoint begin(ctrl->m_h_gap, v_pos);
    wxPoint end(ctrl->GetSize().GetWidth() - ctrl->m_h_gap, v_pos);

    wxPen pen, old_pen = pen = dc.GetPen();
    pen.SetColour(*wxLIGHT_GREY);
    dc.SetPen(pen);
    dc.DrawLine(begin, end);
    dc.SetPen(old_pen);
}

// modify by dhf for anker style optiongrop 
void Anker_OG_CustomCtrl::CtrlLine::render(wxDC& dc, wxCoord v_pos)
{
    PrintLog("==========CtrlLine::render , opt_id:"+og_line.get_options().front().opt_id);
    if (is_separator()) {
        render_separator(dc, v_pos);
        return;
    }

    Field* field = ctrl->opt_group->get_field(og_line.get_options().front().opt_id);

    // const bool suppress_hyperlinks = get_app_config()->get_bool("suppress_hyperlinks");
    const bool suppress_hyperlinks = true;
    if (draw_just_act_buttons) {
        if (field && field->has_undo_ui() && field->has_undo_to_sys_ui())
            draw_act_bmps(dc, wxPoint(0, v_pos), field->undo_to_sys_bitmap(), field->undo_bitmap(), field->blink());
        return;
    }

    //wxCoord h_pos = draw_mode_bmp(dc, v_pos);
    const int indent = 20;
    wxCoord h_pos = indent;

    if (og_line.near_label_widget_win) {
        h_pos += og_line.near_label_widget_win->GetSize().x + ctrl->m_h_gap*2;
    }

    const std::vector<AnkerOption>& option_set = og_line.get_options();
    // add by dhf, append side text to lable
    wxString sideText;

    if (option_set.size()) {
        const AnkerOption& opt = option_set.front();
        ConfigOptionDef option = opt.opt;
        if (!option.sidetext.empty()) {
            if (AnkerOptionsGroup::sidetext_is_unit(_(option.sidetext))) {
                // sidetext is unit ,display in field control
            }
            else
            {
                // sidetext is not unit ,display behide label
                if(option.sidetext.find_first_of("(") == 0)
                    sideText = " " + _(option.sidetext) + "";
                else
                    sideText = " (" + _(option.sidetext) + ")";
            }
        }
    }

    wxString label = og_line.label;
    bool is_url_string = false;
    if (ctrl->opt_group->label_width != 0 && !label.IsEmpty()) {
        const wxColour* text_clr = (/*option_set.size() == 1 && */ field ? field->label_color() : og_line.label_color());
        is_url_string = !suppress_hyperlinks && !og_line.label_path.empty();
        // modify by dhf for Anker_OG_CustomCtrl
        int label_width = 500;
        if (option_set.size() == 1)
            h_pos = draw_text(dc, wxPoint(h_pos, v_pos), label + sideText /* + ":"*/, text_clr, label_width/*ctrl->opt_group->label_width * ctrl->m_em_unit*/, is_url_string);
        else
            h_pos = draw_text(dc, wxPoint(h_pos, v_pos), label + sideText /* + ":"*/, text_clr, label_width, is_url_string);
    }

    // If there's a widget, build it and set result to the correct position.
    if (og_line.widget != nullptr) {
        if (og_line.has_undo_ui()) 
            draw_act_bmps(dc, wxPoint(h_pos, v_pos), og_line.undo_to_sys_bitmap(), og_line.undo_bitmap(), og_line.blink());
        else
            draw_blinking_bmp(dc, wxPoint(0, v_pos), og_line.blink());
        return;
    }

    // If we're here, we have more than one option or a single option with sidetext
    // so we need a horizontal sizer to arrange these things


    // If we have a single option
    if (option_set.size() == 1  /*&& option_set.front().opt.sidetext.size() == 0 &&
        option_set.front().side_widget == nullptr && og_line.get_extra_widgets().size() == 0*/)
    {
        if (field && field->getWindow()){
            PrintLog("              ==========line:" + og_line.get_options().front().opt_id +
                "          field show:" + num2Str(field->getWindow()->IsShown()) +
                "    pos x,y:" + num2Str(field->getWindow()->GetPosition().x) +","+num2Str(field->getWindow()->GetPosition().y)+
                "    width:" + num2Str(field->getWindow()->GetSize().x)+
                "    ctrl width:"+num2Str(ctrl->GetSize().x)
            );
        }
        if (field && field->has_undo_ui()) {
            // add by dhf, draw act bmp at left side of fied
            wxPoint field_pos;
            if (field->getWindow()) {
                field_pos = field->getWindow()->GetPosition();
            }
            else if (field->getSizer()) {
            }
            int bmp_width = get_bitmap_size(&(field->undo_bitmap()), ctrl).GetWidth();
            int bmp_x = field_pos.x - (ctrl->m_h_gap * 5) - bmp_width;
            if (field->m_opt.opt_key == "extruder_offset")
                bmp_x -= (ctrl->m_h_gap * 15);
            draw_act_bmps(dc, wxPoint(bmp_x, v_pos), field->undo_bitmap(), field->blink());

        }
        else if (field && !field->has_undo_ui() && field->blink()) {
            draw_blinking_bmp(dc, wxPoint(0, v_pos), field->blink());
        }

        // modify by dhf , full_width setting is move to correct_window_position()
#if 0
        // update width for full_width fields
        if (option_set.front().opt.full_width && field->getWindow()) {
            int label_width = dc.GetTextExtent(label).x;
            PrintLog("                  =====>is full width ,set size:"+num2Str(ctrl->GetSize().x - label_width - ctrl->m_v_gap)+"   lable width:"+num2Str(label_width)+     "    lable:"+ label);
            field->getWindow()->SetSize(ctrl->GetSize().x - label_width - ctrl->m_v_gap, -1);
        }
#endif
        if (field)
        {
           // correct_items_positions(); 
        }
        return;
    }


#if 0
    //PrintLog("      ==========befor loop option_set, h_pos:" + num2Str(h_pos));
    size_t bmp_rect_id = 0;
    bool is_multioption_line = option_set.size() > 1;
    for (const AnkerOption& opt : option_set) {
        field = ctrl->opt_group->get_field(opt.opt_id);
        ConfigOptionDef option = opt.opt;
/*
        PrintLog("             ----------------->opt:" + opt.opt_id + "      option Label:" + option.label + 
            "  field:"+num2Str( (unsigned long long)field )+
            "   field pos x:" + num2Str(field->getWindow() ? field->getWindow()->GetPosition().x : -1) + 
            "  fieldsize x:" + num2Str(field->getWindow() ? field->getWindow()->GetSize().x : -1)
        );
*/
        // add label if any
        if (is_multioption_line && !option.label.empty()) {
            // those two parameter names require localization with context
            label = (option.label == "Top" || option.label == "Bottom") ?
                _CTX(option.label, "Layers") : _(option.label);
            label += ":";

            if (is_url_string)
                is_url_string = false;
            else if(opt == option_set.front())
                is_url_string = !suppress_hyperlinks && !og_line.label_path.empty();
            h_pos = draw_text(dc, wxPoint(h_pos, v_pos), label, field ? field->label_color() : nullptr, ctrl->opt_group->sublabel_width * ctrl->m_em_unit, is_url_string);
        }

        if (field && field->has_undo_ui()) {
            h_pos = draw_act_bmps(dc, wxPoint(h_pos, v_pos), field->undo_bitmap(), field->blink(), bmp_rect_id++);
            if (field->getSizer())
            {
                auto children = field->getSizer()->GetChildren();
                for (auto child : children) {
                    if (child->IsWindow())
                        h_pos += child->GetWindow()->GetSize().x + ctrl->m_h_gap;
                }
            }
            else if (field->getWindow()) {
                h_pos += field->getWindow()->GetSize().x + ctrl->m_h_gap;
                //PrintLog("      ==========field getWindow, h_pos:" + num2Str(h_pos));
            }
        }
#else
    //size_t bmp_rect_id = 0;
    bool is_multioption_line = option_set.size() > 1;
    int rightPos = ctrl->GetSize().GetWidth();
    for (int i = option_set.size() - 1; i >= 0;--i) {
        const AnkerOption& opt = option_set[i];
        field = ctrl->opt_group->get_field(opt.opt_id);
        ConfigOptionDef option = opt.opt;
        PrintLog("             ----------------->opt:" + opt.opt_id + "      option Label:" + option.label +
            "  field:" + num2Str((unsigned long long)field) +
            "   field pos x,y:" + num2Str(field->getWindow() ? field->getWindow()->GetPosition().x : -1) +" " + num2Str(field->getWindow() ? field->getWindow()->GetPosition().y : -1)+
            "  fieldsize w,h:" + num2Str(field->getWindow() ? field->getWindow()->GetSize().x : -1)+ "  " + num2Str(field->getWindow() ? field->getWindow()->GetSize().y : -1)
        );

        // modify by dhf , some multioption line have no label befor the field
        if (is_multioption_line/* && !option.label.empty()*/) {
            // those two parameter names require localization with context
            label = (option.label == "Top" || option.label == "Bottom") ?
                _CTX(option.label, "Layers") : _(option.label);
            //label += ":";

            int og_ctrl_width = ctrl->GetSize().GetWidth();
            int label_width = dc.GetTextExtent(label).x;
            int undo_bmp_width = 16; //field && field->has_undo_ui() ? ( get_bitmap_size(&(field->undo_bitmap()), ctrl).GetWidth()) :0;

            wxSize field_win_size;
            wxPoint field_pos;
            if (field->getWindow()) {
                field_win_size = field->getWindow()->GetSize();
                field_pos = field->getWindow()->GetPosition();
            }
            else if (field->getSizer()) {
            }

            // field 
            int right_margin = 10;
            field_pos = wxPoint(rightPos- right_margin - field_win_size.GetWidth(), v_pos/*field_pos.y*/);
            if (field->getWindow()) {
               // field->getWindow()->SetPosition(field_pos);
            }
            //rightPos -= (right_margin + field_win_size.GetWidth());
            rightPos = field->getWindow()->GetPosition().x;

            // undo icon
            wxPoint undo_bmp_pos = wxPoint(rightPos - (ctrl->m_h_gap * 5) - undo_bmp_width, v_pos);
            if (field && field->has_undo_ui()) {
                h_pos = draw_act_bmps(dc, undo_bmp_pos, field->undo_bitmap(), field->blink(), i);
            }
            rightPos = rightPos - (ctrl->m_h_gap * 5) - undo_bmp_width;

            // label
            if (is_url_string)
                is_url_string = false;
            else if (opt == option_set.front())
                is_url_string = !suppress_hyperlinks && !og_line.label_path.empty();
            wxPoint label_pos = wxPoint(rightPos - (ctrl->m_h_gap * 5) - label_width, v_pos);
            draw_text(dc, label_pos, label, field ? field->label_color() : nullptr, ctrl->opt_group->sublabel_width * ctrl->m_em_unit, is_url_string);
            rightPos = rightPos - (ctrl->m_h_gap * 5) - label_width;
        }
#endif
        // add field
        if (option_set.size() == 1 && option_set.front().opt.full_width)
            break;

        // add sidetext if any
        //if (!option.sidetext.empty() || ctrl->opt_group->sidetext_width > 0)
        //    h_pos = draw_text(dc, wxPoint(h_pos, v_pos), _(option.sidetext), nullptr, ctrl->opt_group->sidetext_width * ctrl->m_em_unit);

        if (opt.opt_id != option_set.back().opt_id) //! istead of (opt != option_set.back())
            h_pos += lround(0.6 * ctrl->m_em_unit);
    }
}

wxCoord Anker_OG_CustomCtrl::CtrlLine::draw_mode_bmp(wxDC& dc, wxCoord v_pos)
{
    if (!draw_mode_bitmap)
        return ctrl->m_h_gap;

    ConfigOptionMode mode = og_line.get_options()[0].opt.mode;
    wxBitmapBundle* bmp = get_bmp_bundle("mode", wxOSX ? 10 : 12, wxGetApp().get_mode_btn_color(mode));
    wxCoord y_draw = v_pos + lround((height - get_bitmap_size(bmp, ctrl).GetHeight()) / 2);

    if (og_line.get_options().front().opt.gui_type != ConfigOptionDef::GUIType::legend)
        dc.DrawBitmap(bmp->GetBitmapFor(ctrl), 0, y_draw);

    return get_bitmap_size(bmp, ctrl).GetWidth() + ctrl->m_h_gap;
}

wxCoord    Anker_OG_CustomCtrl::CtrlLine::draw_text(wxDC& dc, wxPoint pos, const wxString& text, const wxColour* color, int width, bool is_url/* = false*/)
{
    wxString multiline_text;
    if (width > 0 && dc.GetTextExtent(text).x > width) {
        multiline_text = text;

        size_t idx = size_t(-1);
        for (size_t i = 0; i < multiline_text.Len(); i++)
        {
            if (multiline_text[i] == ' ')
            {
                if (dc.GetTextExtent(multiline_text.SubString(0, i)).x < width)
                    idx = i;
                else {
                    if (idx != size_t(-1))
                        multiline_text[idx] = '\n';
                    else
                        multiline_text[i] = '\n';
                    break;
                }
            }
        }

        if (idx != size_t(-1))
            multiline_text[idx] = '\n';
    }

    if (!text.IsEmpty()) {
        const wxString& out_text = multiline_text.IsEmpty() ? text : multiline_text;
        wxCoord text_width, text_height;
        dc.GetMultiLineTextExtent(out_text, &text_width, &text_height);

        pos.y = pos.y + lround((height - text_height) / 2);
        if (rect_label.GetWidth() == 0)
            rect_label = wxRect(pos, wxSize(text_width, text_height));

        wxColour old_clr = dc.GetTextForeground();
        wxFont old_font = dc.GetFont();

        wxColour ankerColor = wxColour(173, 174, 175);
        dc.SetTextForeground(ankerColor);
        // modify by dhf for line font
        dc.SetFont(ANKER_FONT_NO_1);
        dc.DrawText(out_text, pos);

        dc.SetTextForeground(old_clr);
        dc.SetFont(old_font);

        if (width < 1)
            width = text_width;
    }

    return pos.x + width + ctrl->m_h_gap;
}

wxPoint Anker_OG_CustomCtrl::CtrlLine::draw_blinking_bmp(wxDC& dc, wxPoint pos, bool is_blinking)
{
    wxBitmapBundle* bmp_blinking = get_bmp_bundle(is_blinking ? "search_blink" : "empty");
    wxCoord h_pos = pos.x;
    wxCoord v_pos = pos.y + lround((height - get_bitmap_size(bmp_blinking, ctrl).GetHeight()) / 2);

    dc.DrawBitmap(bmp_blinking->GetBitmapFor(ctrl), h_pos, v_pos);

    int bmp_dim = get_bitmap_size(bmp_blinking, ctrl).GetWidth();

    h_pos += bmp_dim + ctrl->m_h_gap;
    return wxPoint(h_pos, v_pos);
}

wxCoord Anker_OG_CustomCtrl::CtrlLine::draw_act_bmps(wxDC& dc, wxPoint pos, const wxBitmapBundle& bmp_undo_to_sys, const wxBitmapBundle& bmp_undo, bool is_blinking, size_t rect_id)
{
    wxPoint blink_pos{0, pos.y};
    draw_blinking_bmp(dc, blink_pos, is_blinking);

    wxCoord h_pos = pos.x;
    wxCoord v_pos = pos.y + lround((height - get_bitmap_size(&bmp_undo, ctrl).GetHeight()) / 2);
    dc.DrawBitmap(bmp_undo_to_sys.GetBitmapFor(ctrl), h_pos, v_pos);

    int bmp_dim = get_bitmap_size(&bmp_undo_to_sys, ctrl).GetWidth();
    rects_undo_to_sys_icon[rect_id] = wxRect(h_pos, v_pos, bmp_dim, bmp_dim);

    h_pos += bmp_dim + ctrl->m_h_gap;
    dc.DrawBitmap(bmp_undo.GetBitmapFor(ctrl), h_pos, v_pos);

    bmp_dim = get_bitmap_size(&bmp_undo, ctrl).GetWidth();
    if (rect_id >= 0 && rect_id < rects_undo_icon.size())
        rects_undo_icon[rect_id] = wxRect(h_pos, v_pos, bmp_dim, bmp_dim);

    h_pos += bmp_dim + ctrl->m_h_gap;

    return h_pos;
}

wxCoord Anker_OG_CustomCtrl::CtrlLine::draw_act_bmps(wxDC& dc, wxPoint pos, const wxBitmapBundle& bmp_undo, bool is_blinking, size_t rect_id)
{
    wxPoint blink_pos{0, pos.y};
    draw_blinking_bmp(dc, blink_pos, is_blinking);

    wxCoord h_pos = pos.x;
    wxCoord v_pos = pos.y + lround((height - get_bitmap_size(&bmp_undo, ctrl).GetHeight()) / 2);;

    dc.DrawBitmap(bmp_undo.GetBitmapFor(ctrl), h_pos, v_pos);

    int bmp_dim = get_bitmap_size(&bmp_undo, ctrl).GetWidth();
    if(rect_id >= 0 && rect_id < rects_undo_icon.size())
        rects_undo_icon[rect_id] = wxRect(h_pos, v_pos, bmp_dim, bmp_dim);

    h_pos += bmp_dim + ctrl->m_h_gap;

    return h_pos;
}

bool Anker_OG_CustomCtrl::CtrlLine::launch_browser() const
{
    if (!is_focused || og_line.label_path.empty())
        return false;

    return AnkerOptionsGroup::launch_browser(og_line.label_path);
}

} // GUI
} // Slic3r

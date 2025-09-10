#ifndef slic3r_GUI_DailyTips_hpp_
#define slic3r_GUI_DailyTips_hpp_

#include "HintNotification.hpp"

//#include <wx/time.h>
#include <string>
#include <vector>
#include <memory>

namespace Slic3r { namespace GUI {

enum class DailyTipsLayout{
    Horizontal,
    Vertical
};

class DailyTipsDataRenderer;
class DailyTipsPanel {
    static int uid;
public:
    DailyTipsPanel(bool can_expand = true, DailyTipsLayout layout = DailyTipsLayout::Vertical);
    void set_position(const ImVec2& pos);
    void set_size(const ImVec2& size);
    void set_can_expand(bool can_expand);
    ImVec2 get_size();
    void render();
    void init_hints();
    int get_hint_cnt();
    void retrieve_data_from_hint_database(HintDataNavigation nav);
    void expand(bool expand = true);
    void collapse();
    bool is_expanded();
    void on_change_color_mode(bool is_dark);
    void set_fade_opacity(float opacity);

protected:
    void set_header_footer_height();
    void render_Header(const ImVec2& pos, const ImVec2& size);
    void render_controller_buttons(const ImVec2& pos, const ImVec2& size);
    void push_styles();
    void pop_styles();

private:
    std::unique_ptr<DailyTipsDataRenderer> m_dailytips_renderer;
    size_t m_page_index{ 0 };
    int m_pages_count;
    bool m_is_expanded{ true };
    bool m_can_expand{ true };
    ImVec2 m_pos;
    float m_width{0.0f};
    float m_height{ 0.0f };   // m_header_height + m_content_height + m_footer_height

    float m_header_height{ 0.0f };
    float m_content_height{ 0.0f };
    float m_footer_height{0.0f};
    int m_uid;
    bool m_first_enter{ false };
    bool m_is_dark{ true };
    DailyTipsLayout m_layout{ DailyTipsLayout::Vertical };
    float m_fade_opacity{ 1.0f };
};

class DailyTipsWindow {
public:
    DailyTipsWindow();
    void open();
    void close();
    void render();
    void on_change_color_mode(bool is_dark);

private:
    DailyTipsPanel* m_panel{ nullptr };
    bool m_show{ false };
    bool m_is_dark{ false };
};

}}

#endif
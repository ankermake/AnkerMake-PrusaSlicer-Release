#ifndef slic3r_GLGizmoSeam_hpp_
#define slic3r_GLGizmoSeam_hpp_

#include "GLGizmoPainterBase.hpp"

#include "slic3r/GUI/I18N.hpp"

namespace Slic3r::GUI {

class GLGizmoSeam : public GLGizmoPainterBase
{
public:
    GLGizmoSeam(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
        : GLGizmoPainterBase(parent, icon_filename, sprite_id) 
        , m_editFlag(false)
        , m_panelVisibleFlag(false)
        , m_currentType(EnforcerBlockerType::ENFORCER)
        , m_pInputWindowSizer(nullptr)
    {}

    void render_painter_gizmo() override;

    void clearSeam();

protected:
    void on_render_input_window(float x, float y, float bottom_limit) override;
    std::string on_get_name(bool i18n = true) const override;
    PainterGizmoType get_painter_type() const override;

    EnforcerBlockerType get_left_button_state_type() const override { return m_currentType; }
    EnforcerBlockerType get_right_button_state_type() const override { return EnforcerBlockerType::NONE; }

    bool get_right_button_enable() const override { return false; }

    wxString handle_snapshot_action_name(bool shift_down, Button button_down) const override;

    std::string get_gizmo_entering_text() const override { return _u8L("Entering Seam painting"); }
    std::string get_gizmo_leaving_text() const override { return _u8L("Leaving Seam painting"); }
    std::string get_action_snapshot_name() const override { return _u8L("Paint-on seam editing"); }

private:
    bool on_init() override;

    void update_model_object() const override;
    void update_from_model_object() override;

    void on_opening() override;
    void on_shutdown() override;

    // Anker
    void set_input_window_state(bool on);

    // This map holds all translated description texts, so they can be easily referenced during layout calculations
    // etc. When language changes, GUI is recreated and this class constructed again, so the change takes effect.
    std::map<std::string, wxString> m_desc;

    bool m_editFlag;
    bool m_panelVisibleFlag;
    EnforcerBlockerType m_currentType;
    wxBoxSizer* m_pInputWindowSizer;
};



} // namespace Slic3r::GUI


#endif // slic3r_GLGizmoSeam_hpp_

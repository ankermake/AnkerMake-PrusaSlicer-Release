#ifndef slic3r_GLGizmoMmuSegmentation_hpp_
#define slic3r_GLGizmoMmuSegmentation_hpp_

#include "GLGizmoPainterBase.hpp"
#include "slic3r/GUI/3DScene.hpp"

#include "slic3r/GUI/I18N.hpp"

namespace Slic3r::GUI {

class TriangleSelectorMmGui : public TriangleSelectorGUI {
public:
    // Plus 1 in the initialization of m_gizmo_scene is because the first position is allocated for non-painted triangles, and the indices above colors.size() are allocated for seed fill.
    TriangleSelectorMmGui(const TriangleMesh& mesh, const std::vector<ColorRGBA>& colors, const ColorRGBA& default_volume_color, const Selection* selection, int object_id = -1, int volume_id = -1)
        : TriangleSelectorGUI(mesh)
        , m_colors(colors)
        , m_default_volume_color(default_volume_color)
        , m_gizmo_scene(std::make_shared<GLSimpleMesh>(2 * (colors.size() + 1)))
        , m_object_id(object_id)
        , m_volume_id(volume_id) 
        , m_selection(selection) {}
    ~TriangleSelectorMmGui() override = default;

    void render(ImGuiWrapper* imgui, const Transform3d& matrix) override;

    std::shared_ptr<Slic3r::GLSimpleMesh>             m_gizmo_scene;
    int m_object_id;
    int m_volume_id;
private:
    void update_render_data();
    void set_mmu_render_data();

    const std::vector<ColorRGBA>&            m_colors;
    const ColorRGBA                          m_default_volume_color;
    const Selection*                         m_selection{ nullptr };
};

class GLGizmoMmuSegmentation : public GLGizmoPainterBase
{
public:
    GLGizmoMmuSegmentation(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
        : GLGizmoPainterBase(parent, icon_filename, sprite_id) 
        , m_pInputWindowSizer(nullptr)
        , m_isEditing(false)
        , m_panelVisibleFlag(false)
    {
    }
    ~GLGizmoMmuSegmentation() override = default;

    void render_painter_gizmo() override;

    void data_changed() override;

    void render_triangles(const Selection& selection) const override;

    // TriangleSelector::serialization/deserialization has a limit to store 19 different states.
    // EXTRUDER_LIMIT + 1 states are used to storing the painting because also uncolored triangles are stored.
    // When increasing EXTRUDER_LIMIT, it needs to ensure that TriangleSelector::serialization/deserialization
    // will be also extended to support additional states, requiring at least one state to remain free out of 19 states.
    static const constexpr size_t EXTRUDERS_LIMIT = 16;

    const float get_cursor_radius_min() const override { return CursorRadiusMin; }

protected:
    ColorRGBA get_cursor_sphere_left_button_color() const override;
    ColorRGBA get_cursor_sphere_right_button_color() const override;

    EnforcerBlockerType get_left_button_state_type() const override { return EnforcerBlockerType(/*m_first_selected_extruder_idx*/m_current_extruder_idx + 1); }
    EnforcerBlockerType get_right_button_state_type() const override { return EnforcerBlockerType(m_second_selected_extruder_idx + 1); }

    bool get_right_button_enable() const override { return false; }

    void on_render_input_window(float x, float y, float bottom_limit) override;
    std::string on_get_name(bool i18n = true) const override;

    bool on_is_selectable() const override;
    bool on_is_activable() const override;

    wxString handle_snapshot_action_name(bool shift_down, Button button_down) const override;

    std::string get_gizmo_entering_text() const override { return _u8L("Entering Multimaterial painting"); }
    std::string get_gizmo_leaving_text() const override { return _u8L("Leaving Multimaterial painting"); }
    std::string get_action_snapshot_name() const override { return _u8L("Multimaterial painting editing"); }

    size_t                            m_first_selected_extruder_idx  = 0;
    size_t                            m_second_selected_extruder_idx = 1;
    std::vector<std::string>          m_original_extruders_names;
    std::vector<ColorRGBA>            m_original_extruders_colors;
    std::vector<ColorRGBA>            m_modified_extruders_colors;
    std::vector<int>                  m_original_volumes_extruder_idxs;

    static const constexpr float      CursorRadiusMin = 0.1f; // cannot be zero

    bool m_isEditing;
    bool m_panelVisibleFlag;
    size_t m_current_extruder_idx = 0;
	ColorRGBA m_currentColor;
	wxBoxSizer* m_pInputWindowSizer;

private:
    bool on_init() override;

    void update_model_object() const override;
    void update_from_model_object() override;

    void on_opening() override;
    void on_shutdown() override;
    PainterGizmoType get_painter_type() const override;

    void init_model_triangle_selectors();
    void init_extruders_data();

    // Anker
    void set_input_window_state(bool on);

    // This map holds all translated description texts, so they can be easily referenced during layout calculations
    // etc. When language changes, GUI is recreated and this class constructed again, so the change takes effect.
    std::map<std::string, wxString> m_desc;
};

} // namespace Slic3r


#endif // slic3r_GLGizmoMmuSegmentation_hpp_

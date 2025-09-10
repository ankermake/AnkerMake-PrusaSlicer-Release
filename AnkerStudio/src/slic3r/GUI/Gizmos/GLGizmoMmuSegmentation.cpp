#include "GLGizmoMmuSegmentation.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/BitmapCache.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/OpenGLManager.hpp"
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerSplitCtrl.hpp"
#include "slic3r/GUI/Common/AnkerSliderCtrl.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Model.hpp"
#include "slic3r/Utils/UndoRedo.hpp"

#include <wx/valnum.h>
#include <GL/glew.h>

#define BTN_PRESSED "pressed"
#define BTN_NORMAL "normal"


namespace Slic3r::GUI {

static inline void show_notification_extruders_limit_exceeded()
{
    wxGetApp()
        .plater()
        ->get_notification_manager()
        ->push_notification(NotificationType::MmSegmentationExceededExtrudersLimit, NotificationManager::NotificationLevel::PrintInfoNotificationLevel,
                            GUI::format(_L("common_slicepopup_painterror1"), GLGizmoMmuSegmentation::EXTRUDERS_LIMIT));
}

void GLGizmoMmuSegmentation::on_opening()
{
    if (wxGetApp().extruders_edited_cnt() > int(GLGizmoMmuSegmentation::EXTRUDERS_LIMIT))
        show_notification_extruders_limit_exceeded();

    set_input_window_state(true);
}

void GLGizmoMmuSegmentation::on_shutdown()
{
    m_parent.use_slope(false);
    m_parent.toggle_model_objects_visibility(true);

    set_input_window_state(false);
}

std::string GLGizmoMmuSegmentation::on_get_name(bool i18n) const
{
    //return i18n ? _u8L("Multimaterial painting") : "Multimaterial painting";
    return i18n ? _u8L("common_slice_tooltips_draw") : "Draw";
}

bool GLGizmoMmuSegmentation::on_is_selectable() const
{
    return (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF
            && wxGetApp().get_mode() != comSimple && wxGetApp().extruders_edited_cnt() > 1);
}

bool GLGizmoMmuSegmentation::on_is_activable() const
{
    return GLGizmoPainterBase::on_is_activable() && wxGetApp().extruders_edited_cnt() > 1;
}

static std::vector<ColorRGBA> get_extruders_colors()
{
    //std::vector<std::string> colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();
    std::vector<std::string> colors = Slic3r::GUI::wxGetApp().plater()->get_filament_colors_from_plater_config();
    std::vector<ColorRGBA> ret;
    decode_colors(colors, ret);
    return ret;
}

static std::vector<std::string> get_extruders_names()
{
    size_t                   extruders_count = wxGetApp().extruders_edited_cnt();
    std::vector<std::string> extruders_out;
    extruders_out.reserve(extruders_count);
    for (size_t extruder_idx = 1; extruder_idx <= extruders_count; ++extruder_idx)
        extruders_out.emplace_back("Extruder " + std::to_string(extruder_idx));

    return extruders_out;
}

static std::vector<int> get_extruder_id_for_volumes(const ModelObject &model_object)
{
    std::vector<int> extruders_idx;
    extruders_idx.reserve(model_object.volumes.size());
    for (const ModelVolume *model_volume : model_object.volumes) {
        if (!model_volume->is_model_part())
            continue;

        extruders_idx.emplace_back(model_volume->extruder_id());
    }

    return extruders_idx;
}

void GLGizmoMmuSegmentation::init_extruders_data()
{
    m_original_extruders_names     = get_extruders_names();
    m_original_extruders_colors    = get_extruders_colors();
    m_modified_extruders_colors    = m_original_extruders_colors;
    m_first_selected_extruder_idx  = 0;
    m_second_selected_extruder_idx = 1;
}

bool GLGizmoMmuSegmentation::on_init()
{
    m_shortcut_key = WXK_CONTROL_N;

    m_desc["reset_direction"]      = _L("Reset direction");
    m_desc["clipping_of_view"]     = _L("Clipping of view") + ": ";
    m_desc["cursor_size"]          = _L("Brush size") + ": ";
    m_desc["cursor_type"]          = _L("Brush shape");
    m_desc["first_color_caption"]  = _L("Left mouse button") + ": ";
    m_desc["first_color"]          = _L("First color");
    m_desc["second_color_caption"] = _L("Right mouse button") + ": ";
    m_desc["second_color"]         = _L("Second color");
    m_desc["remove_caption"]       = _L("Shift + Left mouse button") + ": ";
    m_desc["remove"]               = _L("Remove painted color");
    m_desc["remove_all"]           = _L("Clear all");
    m_desc["circle"]               = _L("Circle");
    m_desc["sphere"]               = _L("Sphere");
    m_desc["pointer"]              = _L("Triangles");

    m_desc["tool_type"]            = _L("Tool type");
    m_desc["tool_brush"]           = _L("Brush");
    m_desc["tool_smart_fill"]      = _L("Smart fill");
    m_desc["tool_bucket_fill"]     = _L("Bucket fill");

    m_desc["smart_fill_angle"]     = _L("Smart fill angle");
    m_desc["split_triangles"]      = _L("Split triangles");

    init_extruders_data();

    wxGetApp().plater()->sidebarnew().Bind(wxCUSTOMEVT_CLICK_FILAMENT_BTN, [this](wxCommandEvent& event) {
        wxVariant* pData = (wxVariant*)event.GetClientData();
        if (pData)
        {
            // TODO
            wxVariantList list = pData->GetList();
            wxString wxStrFilamentColor = list[0]->GetString();
            int strFilamentIndex = list[1]->GetInteger();
            wxColour filamentColor = wxColour(wxStrFilamentColor);
            m_currentColor = ColorRGBA(filamentColor.Red(), filamentColor.Green(), filamentColor.Blue(), filamentColor.Alpha());
            m_currentColor.a(0.25);
            m_current_extruder_idx = strFilamentIndex - 1;

            //this->init_extruders_data();
            //this->init_model_triangle_selectors();
            update_from_model_object();
        }
        });

    return true;
}

void GLGizmoMmuSegmentation::render_painter_gizmo()
{
    const Selection& selection = m_parent.get_selection();

    glsafe(::glEnable(GL_BLEND));
    glsafe(::glEnable(GL_DEPTH_TEST));

    render_triangles(selection);

    m_c->object_clipper()->render_cut();
    m_c->instances_hider()->render_cut();
    render_cursor();

    glsafe(::glDisable(GL_BLEND));
}

void GLGizmoMmuSegmentation::data_changed(bool is_serializing)
{
    GLGizmoPainterBase::data_changed(is_serializing);
    if (m_state != On || wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() != ptFFF || wxGetApp().extruders_edited_cnt() <= 1)
        return;

    ModelObject *model_object         = m_c->selection_info()->model_object();
    if (int prev_extruders_count = int(m_original_extruders_colors.size());
        prev_extruders_count != wxGetApp().extruders_edited_cnt() || get_extruders_colors() != m_original_extruders_colors) {
        if (wxGetApp().extruders_edited_cnt() > int(GLGizmoMmuSegmentation::EXTRUDERS_LIMIT))
            show_notification_extruders_limit_exceeded();

        this->init_extruders_data();
        // Reinitialize triangle selectors because of change of extruder count need also change the size of GLIndexedVertexArray
        if (prev_extruders_count != wxGetApp().extruders_edited_cnt())
            this->init_model_triangle_selectors();
    } else if (model_object != nullptr && get_extruder_id_for_volumes(*model_object) != m_original_volumes_extruder_idxs) {
        this->init_model_triangle_selectors();
    }
}

void GLGizmoMmuSegmentation::render_triangles(const Selection &selection) const
{
    ClippingPlaneDataWrapper clp_data = this->get_clipping_plane_data();
    auto                    *shader   = wxGetApp().get_shader("mm_gouraud");
    if (!shader)
        return;
    shader->start_using();
    shader->set_uniform("clipping_plane", clp_data.clp_dataf);
    shader->set_uniform("z_range", clp_data.z_range);
    ScopeGuard guard([shader]() { if (shader) shader->stop_using(); });

    const ModelObject *mo      = m_c->selection_info()->model_object();
    int                mesh_id = -1;
    for (const ModelVolume *mv : mo->volumes) {
        if (!mv->is_model_part())
            continue;

        ++mesh_id;

        const Transform3d trafo_matrix = mo->instances[selection.get_instance_idx()]->get_transformation().get_matrix() * mv->get_matrix();

        const bool is_left_handed = trafo_matrix.matrix().determinant() < 0.0;
        if (is_left_handed)
            glsafe(::glFrontFace(GL_CW));

        const Camera& camera = wxGetApp().plater()->get_camera();
        const Transform3d& view_matrix = camera.get_view_matrix();
        shader->set_uniform("view_model_matrix", view_matrix * trafo_matrix);
        shader->set_uniform("projection_matrix", camera.get_projection_matrix());
        const Matrix3d view_normal_matrix = view_matrix.matrix().block(0, 0, 3, 3) * trafo_matrix.matrix().block(0, 0, 3, 3).inverse().transpose();
        shader->set_uniform("view_normal_matrix", view_normal_matrix);

        shader->set_uniform("volume_world_matrix", trafo_matrix);
        shader->set_uniform("volume_mirrored", is_left_handed);
        m_triangle_selectors[mesh_id]->render(m_imgui, trafo_matrix);

        if (is_left_handed)
            glsafe(::glFrontFace(GL_CCW));
    }
}

static void render_extruders_combo(const std::string& label,
                                   const std::vector<std::string>& extruders,
                                   const std::vector<ColorRGBA>& extruders_colors,
                                   size_t& selection_idx)
{
    assert(!extruders_colors.empty());
    assert(extruders_colors.size() == extruders_colors.size());

    size_t selection_out = selection_idx;
    // It is necessary to use BeginGroup(). Otherwise, when using SameLine() is called, then other items will be drawn inside the combobox.
    ImGui::BeginGroup();
    ImVec2 combo_pos = ImGui::GetCursorScreenPos();
    if (ImGui::BeginCombo(label.c_str(), "")) {
        for (size_t extruder_idx = 0; extruder_idx < std::min(extruders.size(), GLGizmoMmuSegmentation::EXTRUDERS_LIMIT); ++extruder_idx) {
            ImGui::PushID(int(extruder_idx));
            ImVec2 start_position = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable("", extruder_idx == selection_idx))
                selection_out = extruder_idx;

            ImGui::SameLine();
            ImGuiStyle &style  = ImGui::GetStyle();
            float       height = ImGui::GetTextLineHeight();
            ImGui::GetWindowDrawList()->AddRectFilled(start_position, ImVec2(start_position.x + height + height / 2, start_position.y + height), ImGuiWrapper::to_ImU32(extruders_colors[extruder_idx]));
            ImGui::GetWindowDrawList()->AddRect(start_position, ImVec2(start_position.x + height + height / 2, start_position.y + height), IM_COL32_BLACK);

            ImGui::SetCursorScreenPos(ImVec2(start_position.x + height + height / 2 + style.FramePadding.x, start_position.y));
            ImGui::Text("%s", extruders[extruder_idx].c_str());
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImVec2      backup_pos = ImGui::GetCursorScreenPos();
    ImGuiStyle &style      = ImGui::GetStyle();

    ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + style.FramePadding.x, combo_pos.y + style.FramePadding.y));
    ImVec2 p      = ImGui::GetCursorScreenPos();
    float  height = ImGui::GetTextLineHeight();

    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + height + height / 2, p.y + height), ImGuiWrapper::to_ImU32(extruders_colors[selection_idx]));
    ImGui::GetWindowDrawList()->AddRect(p, ImVec2(p.x + height + height / 2, p.y + height), IM_COL32_BLACK);

    ImGui::SetCursorScreenPos(ImVec2(p.x + height + height / 2 + style.FramePadding.x, p.y));
    ImGui::Text("%s", extruders[selection_out].c_str());
    ImGui::SetCursorScreenPos(backup_pos);
    ImGui::EndGroup();

    selection_idx = selection_out;
}

void GLGizmoMmuSegmentation::on_render_input_window(float x, float y, float bottom_limit)
{
    if (!m_c->selection_info()->model_object())
        return;

    const float approx_height = m_imgui->scaled(22.0f);
                            y = std::min(y, bottom_limit - approx_height);
    m_imgui->set_next_window_pos(x, y, ImGuiCond_Always);

    m_imgui->begin(get_name(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    // First calculate width of all the texts that are could possibly be shown. We will decide set the dialog width based on that:
    const float clipping_slider_left = std::max(m_imgui->calc_text_size(m_desc.at("clipping_of_view")).x,
                                                m_imgui->calc_text_size(m_desc.at("reset_direction")).x) + m_imgui->scaled(1.5f);
    const float cursor_slider_left       = m_imgui->calc_text_size(m_desc.at("cursor_size")).x + m_imgui->scaled(1.f);
    const float smart_fill_slider_left   = m_imgui->calc_text_size(m_desc.at("smart_fill_angle")).x + m_imgui->scaled(1.f);

    const float cursor_type_radio_circle  = m_imgui->calc_text_size(m_desc["circle"]).x + m_imgui->scaled(2.5f);
    const float cursor_type_radio_sphere  = m_imgui->calc_text_size(m_desc["sphere"]).x + m_imgui->scaled(2.5f);
    const float cursor_type_radio_pointer = m_imgui->calc_text_size(m_desc["pointer"]).x + m_imgui->scaled(2.5f);

    const float button_width             = m_imgui->calc_text_size(m_desc.at("remove_all")).x + m_imgui->scaled(1.f);
    const float buttons_width            = m_imgui->scaled(0.5f);
    const float minimal_slider_width     = m_imgui->scaled(4.f);
    const float color_button_width       = m_imgui->scaled(1.75f);
    const float combo_label_width        = std::max(m_imgui->calc_text_size(m_desc.at("first_color")).x,
                                                    m_imgui->calc_text_size(m_desc.at("second_color")).x) + m_imgui->scaled(1.f);

    const float tool_type_radio_brush       = m_imgui->calc_text_size(m_desc["tool_brush"]).x + m_imgui->scaled(2.5f);
    const float tool_type_radio_bucket_fill = m_imgui->calc_text_size(m_desc["tool_bucket_fill"]).x + m_imgui->scaled(2.5f);
    const float tool_type_radio_smart_fill  = m_imgui->calc_text_size(m_desc["tool_smart_fill"]).x + m_imgui->scaled(2.5f);

    const float split_triangles_checkbox_width = m_imgui->calc_text_size(m_desc["split_triangles"]).x + m_imgui->scaled(2.5f);

    float caption_max    = 0.f;
    float total_text_max = 0.f;
    for (const auto &t : std::array<std::string, 3>{"first_color", "second_color", "remove"}) {
        caption_max    = std::max(caption_max, m_imgui->calc_text_size(m_desc[t + "_caption"]).x);
        total_text_max = std::max(total_text_max, m_imgui->calc_text_size(m_desc[t]).x);
    }
    total_text_max += caption_max + m_imgui->scaled(1.f);
    caption_max    += m_imgui->scaled(1.f);

    const float sliders_left_width = std::max(smart_fill_slider_left, std::max(cursor_slider_left, clipping_slider_left));
    const float slider_icon_width  = m_imgui->get_slider_icon_size().x;
    float       window_width       = minimal_slider_width + sliders_left_width + slider_icon_width;
    window_width       = std::max(window_width, total_text_max);
    window_width       = std::max(window_width, button_width);
    window_width       = std::max(window_width, split_triangles_checkbox_width);
    window_width       = std::max(window_width, cursor_type_radio_circle + cursor_type_radio_sphere + cursor_type_radio_pointer);
    window_width       = std::max(window_width, tool_type_radio_brush + tool_type_radio_bucket_fill + tool_type_radio_smart_fill);
    window_width       = std::max(window_width, 2.f * buttons_width + m_imgui->scaled(1.f));

    auto draw_text_with_caption = [this, &caption_max](const wxString &caption, const wxString &text) {
        m_imgui->text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, caption);
        ImGui::SameLine(caption_max);
        m_imgui->text(text);
    };

    for (const auto &t : std::array<std::string, 3>{"first_color", "second_color", "remove"})
        draw_text_with_caption(m_desc.at(t + "_caption"), m_desc.at(t));

    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("first_color"));
    ImGui::SameLine(combo_label_width);
    ImGui::PushItemWidth(window_width - combo_label_width - color_button_width);
    render_extruders_combo("##first_color_combo", m_original_extruders_names, m_original_extruders_colors, m_first_selected_extruder_idx);
    ImGui::SameLine();

    const ColorRGBA& select_first_color = m_modified_extruders_colors[m_first_selected_extruder_idx];
    ImVec4           first_color        = ImGuiWrapper::to_ImVec4(select_first_color);
    if (ImGui::ColorEdit4("First color##color_picker", (float*)&first_color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
        m_modified_extruders_colors[m_first_selected_extruder_idx] = ImGuiWrapper::from_ImVec4(first_color);

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("second_color"));
    ImGui::SameLine(combo_label_width);
    ImGui::PushItemWidth(window_width - combo_label_width - color_button_width);
    render_extruders_combo("##second_color_combo", m_original_extruders_names, m_original_extruders_colors, m_second_selected_extruder_idx);
    ImGui::SameLine();

    const ColorRGBA& select_second_color = m_modified_extruders_colors[m_second_selected_extruder_idx];
    ImVec4           second_color        = ImGuiWrapper::to_ImVec4(select_second_color);
    if (ImGui::ColorEdit4("Second color##color_picker", (float*)&second_color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
        m_modified_extruders_colors[m_second_selected_extruder_idx] = ImGuiWrapper::from_ImVec4(second_color);

    const float max_tooltip_width = ImGui::GetFontSize() * 20.0f;

    ImGui::Separator();

    m_imgui->text(m_desc.at("tool_type"));
    ImGui::NewLine();

    float tool_type_offset = (window_width - tool_type_radio_brush - tool_type_radio_bucket_fill - tool_type_radio_smart_fill + m_imgui->scaled(1.5f)) / 2.f;
    ImGui::SameLine(tool_type_offset);
    ImGui::PushItemWidth(tool_type_radio_brush);
    if (m_imgui->radio_button(m_desc["tool_brush"], m_tool_type == ToolType::BRUSH)) {
        m_tool_type = ToolType::BRUSH;
        for (auto &triangle_selector : m_triangle_selectors) {
            triangle_selector->seed_fill_unselect_all_triangles();
            triangle_selector->request_update_render_data();
        }
    }

    if (ImGui::IsItemHovered())
        m_imgui->tooltip(_L("Paints facets according to the chosen painting brush."), max_tooltip_width);

    ImGui::SameLine(tool_type_offset + tool_type_radio_brush);
    ImGui::PushItemWidth(tool_type_radio_smart_fill);
    if (m_imgui->radio_button(m_desc["tool_smart_fill"], m_tool_type == ToolType::SMART_FILL)) {
        m_tool_type = ToolType::SMART_FILL;
        for (auto &triangle_selector : m_triangle_selectors) {
            triangle_selector->seed_fill_unselect_all_triangles();
            triangle_selector->request_update_render_data();
        }
    }

    if (ImGui::IsItemHovered())
        m_imgui->tooltip(_L("Paints neighboring facets whose relative angle is less or equal to set angle."), max_tooltip_width);

    ImGui::SameLine(tool_type_offset + tool_type_radio_brush + tool_type_radio_smart_fill);
    ImGui::PushItemWidth(tool_type_radio_bucket_fill);
    if (m_imgui->radio_button(m_desc["tool_bucket_fill"], m_tool_type == ToolType::BUCKET_FILL)) {
        m_tool_type = ToolType::BUCKET_FILL;
        for (auto &triangle_selector : m_triangle_selectors) {
            triangle_selector->seed_fill_unselect_all_triangles();
            triangle_selector->request_update_render_data();
        }
    }

    if (ImGui::IsItemHovered())
        m_imgui->tooltip(_L("Paints neighboring facets that have the same color."), max_tooltip_width);

    ImGui::Separator();

    if(m_tool_type == ToolType::BRUSH) {
        m_imgui->text(m_desc.at("cursor_type"));
        ImGui::NewLine();

        float cursor_type_offset = (window_width - cursor_type_radio_sphere - cursor_type_radio_circle - cursor_type_radio_pointer + m_imgui->scaled(1.5f)) / 2.f;
        ImGui::SameLine(cursor_type_offset);
        ImGui::PushItemWidth(cursor_type_radio_sphere);
        if (m_imgui->radio_button(m_desc["sphere"], m_cursor_type == TriangleSelector::CursorType::SPHERE))
            m_cursor_type = TriangleSelector::CursorType::SPHERE;

        if (ImGui::IsItemHovered())
            m_imgui->tooltip(_L("Paints all facets inside, regardless of their orientation."), max_tooltip_width);

        ImGui::SameLine(cursor_type_offset + cursor_type_radio_sphere);
        ImGui::PushItemWidth(cursor_type_radio_circle);

        if (m_imgui->radio_button(m_desc["circle"], m_cursor_type == TriangleSelector::CursorType::CIRCLE))
            m_cursor_type = TriangleSelector::CursorType::CIRCLE;

        if (ImGui::IsItemHovered())
            m_imgui->tooltip(_L("Ignores facets facing away from the camera."), max_tooltip_width);

        ImGui::SameLine(cursor_type_offset + cursor_type_radio_sphere + cursor_type_radio_circle);
        ImGui::PushItemWidth(cursor_type_radio_pointer);

        if (m_imgui->radio_button(m_desc["pointer"], m_cursor_type == TriangleSelector::CursorType::POINTER))
            m_cursor_type = TriangleSelector::CursorType::POINTER;

        if (ImGui::IsItemHovered())
            m_imgui->tooltip(_L("Paints only one facet."), max_tooltip_width);

        m_imgui->disabled_begin(m_cursor_type != TriangleSelector::CursorType::SPHERE && m_cursor_type != TriangleSelector::CursorType::CIRCLE);

        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc.at("cursor_size"));
        ImGui::SameLine(sliders_left_width);
        ImGui::PushItemWidth(window_width - sliders_left_width - slider_icon_width);
        m_imgui->slider_float("##cursor_radius", &m_cursor_radius, CursorRadiusMin, CursorRadiusMax, "%.2f", 1.0f, true, _L("Alt + Mouse wheel"));

        m_imgui->checkbox(m_desc["split_triangles"], m_triangle_splitting_enabled);

        if (ImGui::IsItemHovered())
            m_imgui->tooltip(_L("Split bigger facets into smaller ones while the object is painted."), max_tooltip_width);

        m_imgui->disabled_end();

        ImGui::Separator();
    } else if(m_tool_type == ToolType::SMART_FILL) {
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc["smart_fill_angle"] + ":");
        std::string format_str = std::string("%.f") + I18N::translate_utf8("°", "Degree sign to use in the respective slider in MMU gizmo,"
                                                                                "placed after the number with no whitespace in between.");
        ImGui::SameLine(sliders_left_width);
        ImGui::PushItemWidth(window_width - sliders_left_width - slider_icon_width);
        if (m_imgui->slider_float("##smart_fill_angle", &m_smart_fill_angle, SmartFillAngleMin, SmartFillAngleMax, format_str.data(), 1.0f, true, _L("Alt + Mouse wheel")))
            for (auto &triangle_selector : m_triangle_selectors) {
                triangle_selector->seed_fill_unselect_all_triangles();
                triangle_selector->request_update_render_data();
            }

        ImGui::Separator();
    }

    if (m_c->object_clipper()->get_position() == 0.f) {
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc.at("clipping_of_view"));
    } else {
        if (m_imgui->button(m_desc.at("reset_direction"))) {
            wxGetApp().CallAfter([this]() { m_c->object_clipper()->set_position_by_ratio(-1., false); });
        }
    }

    auto clp_dist = float(m_c->object_clipper()->get_position());
    ImGui::SameLine(sliders_left_width);
    ImGui::PushItemWidth(window_width - sliders_left_width - slider_icon_width);
    if (m_imgui->slider_float("##clp_dist", &clp_dist, 0.f, 1.f, "%.2f", 1.0f, true, _L("Ctrl + Mouse wheel")))
        m_c->object_clipper()->set_position_by_ratio(clp_dist, true);

    ImGui::Separator();
    if (m_imgui->button(m_desc.at("remove_all"))) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Reset selection"),
                                      UndoRedo::SnapshotType::GizmoAction);
        ModelObject *        mo  = m_c->selection_info()->model_object();
        int                  idx = -1;
        for (ModelVolume *mv : mo->volumes)
            if (mv->is_model_part()) {
                ++idx;
                m_triangle_selectors[idx]->reset();
                m_triangle_selectors[idx]->request_update_render_data();
            }

        update_model_object();
        m_parent.set_as_dirty();
    }

    m_imgui->end();
}

void GLGizmoMmuSegmentation::update_model_object() const
{
    bool updated = false;
    ModelObject* mo = m_c->selection_info()->model_object();
    int idx = -1;
    for (ModelVolume* mv : mo->volumes) {
        if (! mv->is_model_part())
            continue;
        ++idx;
        updated |= mv->mmu_segmentation_facets.set(*m_triangle_selectors[idx].get());
    }

    if (updated) {
        const ModelObjectPtrs &mos = wxGetApp().model().objects;
        //wxGetApp().obj_list()->update_info_items(std::find(mos.begin(), mos.end(), mo) - mos.begin());
        m_parent.post_event(SimpleEvent(EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS));
    }
}

void GLGizmoMmuSegmentation::init_model_triangle_selectors()
{
    if (m_c->selection_info() == nullptr)
        return;

    const ModelObject *mo = m_c->selection_info()->model_object();
    m_triangle_selectors.clear();

    // Don't continue when extruders colors are not initialized
    if(m_original_extruders_colors.empty())
        return;
    
    auto object_idx = m_parent.get_object_idx(mo);
    if (object_idx < 0)
        return;

    if (mo == nullptr) {
        ANKER_LOG_ERROR << "init_model_triangle_selectors selection model object is nullptr !";
        return;
    }

    int volume_idx = -1;
    for (const ModelVolume *mv : mo->volumes) {
        if (!mv->is_model_part())
            continue;
        volume_idx ++;
        // This mesh does not account for the possible Z up SLA offset.
        const TriangleMesh *mesh = &mv->mesh();

        int extruder_idx = (mv->extruder_id() > 0) ? mv->extruder_id() - 1 : 0;
        m_triangle_selectors.emplace_back(std::make_unique<TriangleSelectorMmGui>(*mesh, m_modified_extruders_colors, m_original_extruders_colors[size_t(extruder_idx)], 
            &m_parent.get_selection(), object_idx, volume_idx));
        // Reset of TriangleSelector is done inside TriangleSelectorMmGUI's constructor, so we don't need it to perform it again in deserialize().
        m_triangle_selectors.back()->deserialize(mv->mmu_segmentation_facets.get_data(), false);
        m_triangle_selectors.back()->request_update_render_data();
    }

    m_original_volumes_extruder_idxs = get_extruder_id_for_volumes(*mo);
}

void GLGizmoMmuSegmentation::update_from_model_object()
{
    wxBusyCursor wait;

    //// Extruder colors need to be reloaded before calling init_model_triangle_selectors to render painted triangles
    //// using colors from loaded 3MF andsea not from printer profile in Slicer.
    //if (int prev_extruders_count = int(m_original_extruders_colors.size());
    //    prev_extruders_count != wxGetApp().extruders_edited_cnt() || get_extruders_colors() != m_original_extruders_colors)
    this->init_extruders_data();

    this->init_model_triangle_selectors();
}

PainterGizmoType GLGizmoMmuSegmentation::get_painter_type() const
{
    return PainterGizmoType::MMU_SEGMENTATION;
}

ColorRGBA GLGizmoMmuSegmentation::get_cursor_sphere_left_button_color() const
{
    ColorRGBA color = m_modified_extruders_colors[/*m_first_selected_extruder_idx*/m_current_extruder_idx];
    color.a(0.25f);
    return color;
}

ColorRGBA GLGizmoMmuSegmentation::get_cursor_sphere_right_button_color() const
{
    ColorRGBA color = m_modified_extruders_colors[m_second_selected_extruder_idx];
    color.a(0.25f);
    return color;
}

void TriangleSelectorMmGui::render(ImGuiWrapper* imgui, const Transform3d& matrix)
{
    if (m_update_render_data)
        update_render_data();

    auto *shader = wxGetApp().get_current_shader();
    if (!shader)
        return;
    assert(shader->get_name() == "mm_gouraud");
    const Camera& camera = wxGetApp().plater()->get_camera();
    const Transform3d& view_matrix = camera.get_view_matrix();
    shader->set_uniform("view_model_matrix", view_matrix * matrix);
    shader->set_uniform("projection_matrix", camera.get_projection_matrix());
    const Matrix3d view_normal_matrix = view_matrix.matrix().block(0, 0, 3, 3) * matrix.matrix().block(0, 0, 3, 3).inverse().transpose();
    shader->set_uniform("view_normal_matrix", view_normal_matrix);

    for (size_t color_idx = 0; color_idx < m_gizmo_scene->triangle_indices.size(); ++color_idx) {
        if (m_gizmo_scene->has_VBOs(color_idx)) {
            if (color_idx > m_colors.size()) // Seed fill VBO
                shader->set_uniform("uniform_color", TriangleSelectorGUI::get_seed_fill_color(color_idx == (m_colors.size() + 1) ? m_default_volume_color : m_colors[color_idx - (m_colors.size() + 1) - 1]));
            else                             // Normal VBO
                shader->set_uniform("uniform_color", color_idx == 0 ? m_default_volume_color : m_colors[color_idx - 1]);

            m_gizmo_scene->render(color_idx);
        }
    }

    render_paint_contour(matrix);
    m_update_render_data = false;
}

void TriangleSelectorMmGui::set_mmu_render_data()
{
    if (!m_selection) {
        return;
    }

    auto object_idx = m_selection->get_object_idx();
    if (object_idx == -1)
        return;
    // get volume idxs from object, which created by model part or model instance
    const auto& list = m_selection->get_volume_idxs_from_object(object_idx);
    for (auto idx : list) {
        GLVolume* vol = const_cast<GLVolume*>(m_selection->get_volume(idx));
        // traverse all selectors, make sure we get the correct one
        if (m_object_id == vol->composite_id.object_id && m_volume_id == vol
            ->composite_id.volume_id) {
            vol->mmu_mesh = m_gizmo_scene;
        }
    }
}

void TriangleSelectorMmGui::update_render_data()
{
    m_gizmo_scene->release_geometry();
    m_vertices.reserve(m_vertices.size() * 3);
    for (const Vertex &vr : m_vertices) {
        m_gizmo_scene->vertices.emplace_back(vr.v.x());
        m_gizmo_scene->vertices.emplace_back(vr.v.y());
        m_gizmo_scene->vertices.emplace_back(vr.v.z());
    }
    m_gizmo_scene->finalize_vertices();

    for (const Triangle &tr : m_triangles)
        if (tr.valid() && !tr.is_split()) {
            int               color = int(tr.get_state()) <= int(m_colors.size()) ? int(tr.get_state()) : 0;
            assert(m_colors.size() + 1 + color < m_gizmo_scene->triangle_indices.size());

            //fix press ESC color not keep when select bucket fill the color
            std::vector<int>& iva = m_gizmo_scene->triangle_indices[color];
            //std::vector<int> &iva   = m_gizmo_scene->triangle_indices[color + tr.is_selected_by_seed_fill() * (m_colors.size() + 1)];
            if (iva.size() + 3 > iva.capacity())
                iva.reserve(next_highest_power_of_2(iva.size() + 3));

            iva.emplace_back(tr.verts_idxs[0]);
            iva.emplace_back(tr.verts_idxs[1]);
            iva.emplace_back(tr.verts_idxs[2]);
        }

    for (size_t color_idx = 0; color_idx < m_gizmo_scene->triangle_indices.size(); ++color_idx)
        m_gizmo_scene->triangle_indices_sizes[color_idx] = m_gizmo_scene->triangle_indices[color_idx].size();

    m_gizmo_scene->finalize_triangle_indices();
    update_paint_contour();
    set_mmu_render_data();
}

wxString GLGizmoMmuSegmentation::handle_snapshot_action_name(bool shift_down, GLGizmoPainterBase::Button button_down) const
{
    wxString action_name;
    if (shift_down)
        action_name = _L("Remove painted color");
    else {
        size_t extruder_id = (button_down == Button::Left ? m_first_selected_extruder_idx : m_second_selected_extruder_idx) + 1;
        action_name        = GUI::format(_L("Painted using: Extruder %1%"), extruder_id);
    }
    return action_name;
}

void GLGizmoMmuSegmentation::set_input_window_state(bool on)
{
    if (wxGetApp().plater() == nullptr)
        return;

    ANKER_LOG_INFO << "GLGizmoMmuSegmentation: " << on;

    std::string panelFlag = "GLGizmoMmuSegmentation";
    if (on)
    {
        wxGetApp().plater()->sidebarnew().setMainSizer();

		if (m_pInputWindowSizer == nullptr)
		{
			int smartFillAngleInit = 30;
			int smartFillAngleMin = 0;
			int smartFillAngleMax = 90;
			m_smart_fill_angle = smartFillAngleInit;
			double brushSizeInit = 2.0;
			double brushSizeMin = 0.1;
			double brushSizeMax = 8.0;
			double sliderMultiple = 10;
            m_tool_type = ToolType::BRUSH;
			m_cursor_radius = brushSizeInit;
			m_cursor_type = Slic3r::TriangleSelector::CursorType::CIRCLE;
			m_pInputWindowSizer = new wxBoxSizer(wxVERTICAL);

			AnkerTitledPanel* container = new AnkerTitledPanel(&(wxGetApp().plater()->sidebarnew()), 32, 0, wxColour(PANEL_TITLE_BACK_DARK_RGB_INT));
			container->setTitle(/*wxString::FromUTF8(get_name(true, false))*/_("common_slice_tooltips_draw"));
			container->setTitleAlign(AnkerTitledPanel::TitleAlign::LEFT);
			int returnBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("return.png")), true);
			int clearBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("reset.png")), false);
			m_pInputWindowSizer->Add(container, 1, wxEXPAND, 0);

			wxPanel* mmuPanel = new wxPanel(container);
            mmuPanel->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			wxBoxSizer* mmuPanelSizer = new wxBoxSizer(wxVERTICAL);
			mmuPanel->SetSizer(mmuPanelSizer);
			container->setContentPanel(mmuPanel);

			wxBoxSizer* modeSizer = new wxBoxSizer(wxHORIZONTAL);
			mmuPanelSizer->Add(modeSizer, 0, wxALIGN_TOP | wxLEFT | wxRIGHT, 20);

			wxImage smartFillImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill.png")), wxBITMAP_TYPE_PNG);
			smartFillImage.Rescale(46, 46);
			wxImage smartFillImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill_pressed.png")), wxBITMAP_TYPE_PNG);
            smartFillImagePressed.Rescale(46, 46);
			wxButton* smartFillButton = new wxButton(mmuPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			smartFillButton->SetBitmap(smartFillImage);
			smartFillButton->SetBitmapHover(smartFillImagePressed);
			smartFillButton->SetMinSize(smartFillImage.GetSize());
			smartFillButton->SetMaxSize(smartFillImage.GetSize());
			smartFillButton->SetSize(smartFillImage.GetSize());
			smartFillButton->SetName(BTN_NORMAL);
			smartFillButton->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			modeSizer->Add(smartFillButton, 0, wxALIGN_LEFT | wxTOP, 16);

			wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush.png")), wxBITMAP_TYPE_PNG);
			blockerImage.Rescale(46, 46);
			wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush_pressed.png")), wxBITMAP_TYPE_PNG);
			blockerImagePressed.Rescale(46, 46);
			wxButton* brushButton = new wxButton(mmuPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			brushButton->SetBitmap(blockerImagePressed);
			brushButton->SetBitmapHover(blockerImagePressed);
			brushButton->SetMinSize(blockerImage.GetSize());
			brushButton->SetMaxSize(blockerImage.GetSize());
			brushButton->SetSize(blockerImage.GetSize());
			brushButton->SetName(BTN_PRESSED);
			brushButton->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			modeSizer->Add(brushButton, 0, wxALIGN_LEFT | wxTOP | wxLEFT, 16);

			modeSizer->AddStretchSpacer(1);


			AnkerSplitCtrl* splitCtrl = new AnkerSplitCtrl(mmuPanel);
			mmuPanelSizer->Add(splitCtrl, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 20);


			wxStaticText* paramTitleText = new wxStaticText(mmuPanel, wxID_ANY, /*L"Brush Size"*/_("common_slice_toolpanneldraw_title2"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            paramTitleText->SetFont(ANKER_FONT_NO_1);
			paramTitleText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			paramTitleText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
			mmuPanelSizer->Add(paramTitleText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);


			wxBoxSizer* smartFillValueSizer = new wxBoxSizer(wxHORIZONTAL);
			mmuPanelSizer->Add(smartFillValueSizer, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT, 18);

			//wxSlider* smartFillValueSlider = new wxSlider(mmuPanel, wxID_ANY, smartFillAngleInit, smartFillAngleMin, smartFillAngleMax, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			//smartFillValueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			//smartFillValueSlider->SetForegroundColour(wxColour(ANKER_RGB_INT));

            AnkerSlider* smartFillValueSlider = new AnkerSlider(mmuPanel);
            smartFillValueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
            smartFillValueSlider->SetMinSize(AnkerSize(40, 40));
            smartFillValueSlider->SetMaxSize(AnkerSize(1000, 40));
            smartFillValueSlider->setRange(smartFillAngleMin, smartFillAngleMax, 0.01);
            smartFillValueSlider->setCurrentValue(smartFillAngleInit);
            smartFillValueSlider->setTooltipVisible(false);

			smartFillValueSizer->Add(smartFillValueSlider, 1, wxEXPAND | wxALIGN_LEFT | wxTOP, 8);

            smartFillValueSizer->AddSpacer(10);

			char text[10];
			sprintf(text, "%d", smartFillAngleInit);
            AnkerLineEditUnit* smartFillValueTextCtrl = new AnkerLineEditUnit(mmuPanel, _L("°"), ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
            smartFillValueTextCtrl->SetFont(ANKER_FONT_NO_1);
			smartFillValueTextCtrl->SetMinSize(AnkerSize(42, 25));
			smartFillValueTextCtrl->SetMaxSize(AnkerSize(42, 25));
			smartFillValueTextCtrl->SetSize(AnkerSize(42, 25));
			smartFillValueTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			smartFillValueTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
            smartFillValueTextCtrl->SetValue(text);
            smartFillValueTextCtrl->AddValidatorInt(SmartFillAngleMin, SmartFillAngleMax);

			smartFillValueSizer->Add(smartFillValueTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxTOP, 8);

            smartFillValueSizer->Show(false);

			wxBoxSizer* brushValueSizer = new wxBoxSizer(wxHORIZONTAL);
			mmuPanelSizer->Add(brushValueSizer, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT, 18);

			//wxSlider* brushValueSlider = new wxSlider(mmuPanel, wxID_ANY, brushSizeInit * sliderMultiple, brushSizeMin * sliderMultiple, brushSizeMax * sliderMultiple, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			//brushValueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			//brushValueSlider->SetForegroundColour(wxColour(ANKER_RGB_INT));

            AnkerSlider* brushValueSlider = new AnkerSlider(mmuPanel);
            brushValueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
            brushValueSlider->SetMinSize(AnkerSize(40, 40));
            brushValueSlider->SetMaxSize(AnkerSize(1000, 40));
            brushValueSlider->setRange(brushSizeMin, brushSizeMax, 0.01);
            brushValueSlider->setCurrentValue(brushSizeInit);
            brushValueSlider->setTooltipVisible(false);

			brushValueSizer->Add(brushValueSlider, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxTOP, 8);

            brushValueSizer->AddSpacer(10);

			sprintf(text, "%.2f", brushSizeInit);
            AnkerLineEditUnit* brushValueTextCtrl = new AnkerLineEditUnit(mmuPanel, "", ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
            brushValueTextCtrl->SetFont(ANKER_FONT_NO_1);
			brushValueTextCtrl->SetMinSize(AnkerSize(42, 25));
			brushValueTextCtrl->SetMaxSize(AnkerSize(42, 25));
			brushValueTextCtrl->SetSize(AnkerSize(42, 25));
			brushValueTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			brushValueTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
            brushValueTextCtrl->SetValue(text);
            brushValueTextCtrl->AddValidatorFloat(brushSizeMin, brushSizeMax, 2);
            
            brushValueSizer->Add(brushValueTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxTOP, 8);

			brushValueSizer->Show(true);


			mmuPanelSizer->AddStretchSpacer(1);


			container->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, container, returnBtnID, clearBtnID, brushSizeInit, smartFillAngleInit, brushValueTextCtrl, smartFillValueTextCtrl, brushValueSlider, smartFillValueSlider](wxCommandEvent& event) {
				int btnID = event.GetInt();
				if (btnID == returnBtnID)
				{
					wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(get_name(false, false)));
				}
				else if (btnID == clearBtnID)
				{
					Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Reset selection"),
						UndoRedo::SnapshotType::GizmoAction);
					ModelObject* mo = m_c->selection_info()->model_object();
					int                  idx = -1;
					for (ModelVolume* mv : mo->volumes)
						if (mv->is_model_part()) {
							++idx;
							m_triangle_selectors[idx]->reset();
							m_triangle_selectors[idx]->request_update_render_data();
						}

					update_model_object();
					m_parent.set_as_dirty();

                    // reset slider and text editor
                    char text[10];
                    sprintf(text, "%.2f", brushSizeInit);
                    brushValueTextCtrl->SetValue(text);
                    brushValueSlider->setCurrentValue(brushSizeInit);
                    m_cursor_radius = brushSizeInit;
                    sprintf(text, "%d", smartFillAngleInit);
                    smartFillValueTextCtrl->SetValue(text);
                    smartFillValueSlider->setCurrentValue(smartFillAngleInit);
                    m_smart_fill_angle = smartFillAngleInit;
                    container->Refresh();
				}
				});

			smartFillButton->Bind(wxEVT_BUTTON, [this, smartFillButton, brushButton, paramTitleText, smartFillValueSizer, brushValueSizer, container](wxCommandEvent& event) {
                if (smartFillButton->GetName() == BTN_PRESSED)
                    return;
                
                m_tool_type = ToolType::SMART_FILL;

				paramTitleText->SetLabelText(_("common_slice_toolpanneldraw_title1"));
				smartFillValueSizer->Show(true);
				brushValueSizer->Show(false);

				wxImage enforcerImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill.png")), wxBITMAP_TYPE_PNG);
				enforcerImage.Rescale(46, 46);
				wxImage enforcerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill_pressed.png")), wxBITMAP_TYPE_PNG);
				enforcerImagePressed.Rescale(46, 46);
				smartFillButton->SetBitmap(smartFillButton->GetName() == BTN_PRESSED ? enforcerImage : enforcerImagePressed);
				smartFillButton->SetBitmapHover(enforcerImagePressed);
				smartFillButton->SetName(smartFillButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush.png")), wxBITMAP_TYPE_PNG);
				blockerImage.Rescale(46, 46);
				wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush_pressed.png")), wxBITMAP_TYPE_PNG);
				blockerImagePressed.Rescale(46, 46);
				brushButton->SetBitmap(smartFillButton->GetName() == BTN_PRESSED ? blockerImage : blockerImagePressed);
				brushButton->SetBitmapHover(blockerImagePressed);
				brushButton->SetName(smartFillButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

                smartFillButton->SetCursor(wxCURSOR_HAND);

				container->Layout();
				container->Refresh();
				});
            //smartFillButton->Bind(wxEVT_ENTER_WINDOW, [smartFillButton](wxMouseEvent& event) {smartFillButton->SetCursor(wxCursor(wxCURSOR_HAND)); });
            //smartFillButton->Bind(wxEVT_LEAVE_WINDOW, [smartFillButton](wxMouseEvent& event) {smartFillButton->SetCursor(wxCursor(wxCURSOR_NONE)); });

			brushButton->Bind(wxEVT_BUTTON, [this, smartFillButton, brushButton, paramTitleText, smartFillValueSizer, brushValueSizer, container](wxCommandEvent& event) {
                if (brushButton->GetName() == BTN_PRESSED)
                    return;
                
                m_tool_type = ToolType::BRUSH;

                paramTitleText->SetLabelText(_("common_slice_toolpanneldraw_title2"));
				smartFillValueSizer->Show(false);
				brushValueSizer->Show(true);

				wxImage enforcerImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill.png")), wxBITMAP_TYPE_PNG);
				enforcerImage.Rescale(46, 46);
				wxImage enforcerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_smart_fill_pressed.png")), wxBITMAP_TYPE_PNG);
				enforcerImagePressed.Rescale(46, 46);
				smartFillButton->SetBitmap(smartFillButton->GetName() == BTN_PRESSED ? enforcerImage : enforcerImagePressed);
				smartFillButton->SetBitmapHover(enforcerImagePressed);
				smartFillButton->SetName(smartFillButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush.png")), wxBITMAP_TYPE_PNG);
				blockerImage.Rescale(46, 46);
				wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("mmu_brush_pressed.png")), wxBITMAP_TYPE_PNG);
				blockerImagePressed.Rescale(46, 46);
				brushButton->SetBitmap(smartFillButton->GetName() == BTN_PRESSED ? blockerImage : blockerImagePressed);
				brushButton->SetBitmapHover(blockerImagePressed);
				brushButton->SetName(smartFillButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

                brushButton->SetCursor(wxCURSOR_HAND);

				container->Layout();
				container->Refresh();
				});
            //brushButton->Bind(wxEVT_ENTER_WINDOW, [brushButton](wxMouseEvent& event) {brushButton->SetCursor(wxCursor(wxCURSOR_HAND)); });
            //brushButton->Bind(wxEVT_LEAVE_WINDOW, [brushButton](wxMouseEvent& event) {brushButton->SetCursor(wxCursor(wxCURSOR_NONE)); });

			smartFillValueSlider->Bind(wxANKEREVT_SLIDER_VALUE_CHANGED, [this, smartFillValueTextCtrl, smartFillValueSlider, container](wxCommandEvent& event) {
                if (m_isEditing)
                    return;

                m_isEditing = true;
                
                int currentValue = smartFillValueSlider->getCurrentValue();

				m_smart_fill_angle = currentValue;

				char text[10];
				sprintf(text, "%d", currentValue);
				smartFillValueTextCtrl->SetValue(text);

				container->Refresh();

                m_isEditing = false;
				});
			smartFillValueTextCtrl->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this, smartFillValueTextCtrl, smartFillValueSlider, smartFillAngleInit, smartFillAngleMin, smartFillAngleMax, container](wxCommandEvent& event) {
                if (m_isEditing)
                    return;

                m_isEditing = true;
                
                int newValue = smartFillAngleInit;
				wxString newValueStr = smartFillValueTextCtrl->GetValue();
				bool success = newValueStr.ToInt(&newValue);
				if (success)
				{
                    newValue = std::max(smartFillAngleMin, std::min(newValue, smartFillAngleMax));
					m_smart_fill_angle = newValue;
					smartFillValueSlider->setCurrentValue(newValue);
                    container->Refresh();
				}
                
                m_isEditing = false;
				});

			brushValueSlider->Bind(wxANKEREVT_SLIDER_VALUE_CHANGED, [this, brushValueTextCtrl, brushValueSlider, sliderMultiple, container](wxCommandEvent& event) {
                if (m_isEditing)
                    return;

                m_isEditing = true;
                
                double currentValue = brushValueSlider->getCurrentValue();
				m_cursor_radius = currentValue;

				char text[10];
				sprintf(text, "%.2f", currentValue);
				brushValueTextCtrl->SetValue(text);

				container->Refresh();
                m_isEditing = false;
				});
			brushValueTextCtrl->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this, brushValueSlider, brushValueTextCtrl, brushSizeInit, brushSizeMin, brushSizeMax, sliderMultiple, container](wxCommandEvent& event) {
                if (m_isEditing)
                    return;

                m_isEditing = true;
                
                double newValue = brushSizeInit;
				wxString newValueStr = brushValueTextCtrl->GetValue();
				bool success = newValueStr.ToDouble(&newValue);
				if (success)
				{
                    newValue = std::max(brushSizeMin, std::min(newValue, brushSizeMax));
					m_cursor_radius = newValue;
					brushValueSlider->setCurrentValue(newValue);
                    container->Refresh();
				}

                m_isEditing = false;
				});

            wxGetApp().plater()->sidebarnew().Bind(wxCUSTOMEVT_MAIN_SIZER_CHANGED, [this, panelFlag](wxCommandEvent& event) {
                event.Skip();

                if (!m_panelVisibleFlag)
                    return;

                std::string flag = wxGetApp().plater()->sidebarnew().getSizerFlags().ToStdString();
                if (flag != panelFlag)
                {
                    m_panelVisibleFlag = false;

                    wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(get_name(false, false)));
                }
                });
        }

        wxGetApp().plater()->SetCursor(m_tool_type == ToolType::BRUSH ? wxCURSOR_PAINT_BRUSH : wxCURSOR_PENCIL);
        wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer(m_pInputWindowSizer, panelFlag);
        m_panelVisibleFlag = true;
    }
    else
    {
        m_panelVisibleFlag = false;
        if (wxGetApp().plater()->sidebarnew().getSizerFlags() == panelFlag)
        {
            wxGetApp().plater()->SetCursor(wxCURSOR_NONE);
            wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer();
        }
    }
}

} // namespace Slic3r

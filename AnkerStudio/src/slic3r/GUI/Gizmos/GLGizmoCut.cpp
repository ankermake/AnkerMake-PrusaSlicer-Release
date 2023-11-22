#include "GLGizmoCut.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"

#include <GL/glew.h>

#include <algorithm>

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GUI_ObjectManipulation.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/TriangleMeshSlicer.hpp"

#include "imgui/imgui_internal.h"
#include "slic3r/GUI/MsgDialog.hpp"

namespace Slic3r {
namespace GUI {

static const ColorRGBA GRABBER_COLOR = ColorRGBA::YELLOW();
static const ColorRGBA UPPER_PART_COLOR = ColorRGBA::CYAN();
static const ColorRGBA LOWER_PART_COLOR = ColorRGBA::MAGENTA();

// connector colors
static const ColorRGBA PLAG_COLOR           = ColorRGBA::YELLOW();
static const ColorRGBA DOWEL_COLOR          = ColorRGBA::DARK_YELLOW();
static const ColorRGBA HOVERED_PLAG_COLOR   = ColorRGBA::CYAN();
static const ColorRGBA HOVERED_DOWEL_COLOR  = ColorRGBA(0.0f, 0.5f, 0.5f, 1.0f);
static const ColorRGBA SELECTED_PLAG_COLOR  = ColorRGBA::GRAY();
static const ColorRGBA SELECTED_DOWEL_COLOR = ColorRGBA::DARK_GRAY();
static const ColorRGBA CONNECTOR_DEF_COLOR  = ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f);
static const ColorRGBA CONNECTOR_ERR_COLOR  = ColorRGBA(1.0f, 0.3f, 0.3f, 0.5f);
static const ColorRGBA HOVERED_ERR_COLOR    = ColorRGBA(1.0f, 0.3f, 0.3f, 1.0f);

const unsigned int AngleResolution = 64;
const unsigned int ScaleStepsCount = 72;
const float ScaleStepRad = 2.0f * float(PI) / ScaleStepsCount;
const unsigned int ScaleLongEvery = 2;
const float ScaleLongTooth = 0.1f; // in percent of radius
const unsigned int SnapRegionsCount = 8;

const float         UndefFloat = -999.f;
const std::string   UndefLabel = " ";

using namespace Geometry;

// Generates mesh for a line
static GLModel::Geometry its_make_line(Vec3f beg_pos, Vec3f end_pos)
{
    GLModel::Geometry init_data;
    init_data.format = { GLModel::Geometry::EPrimitiveType::Lines, GLModel::Geometry::EVertexLayout::P3 };
    init_data.reserve_vertices(2);
    init_data.reserve_indices(2);

    // vertices
    init_data.add_vertex(beg_pos);
    init_data.add_vertex(end_pos);

    // indices
    init_data.add_line(0, 1);
    return init_data;
}

//! -- #ysFIXME those functions bodies are ported from GizmoRotation
// Generates mesh for a circle 
static void init_from_circle(GLModel& model, double radius)
{
    GLModel::Geometry init_data;
    init_data.format = { GLModel::Geometry::EPrimitiveType::LineLoop, GLModel::Geometry::EVertexLayout::P3 };
    init_data.reserve_vertices(ScaleStepsCount);
    init_data.reserve_indices(ScaleStepsCount);

    // vertices + indices
    for (unsigned int i = 0; i < ScaleStepsCount; ++i) {
        const float angle = float(i * ScaleStepRad);
        init_data.add_vertex(Vec3f(::cos(angle) * float(radius), ::sin(angle) * float(radius), 0.0f));
        init_data.add_index(i);
    }

    model.init_from(std::move(init_data));
    model.set_color(ColorRGBA::WHITE());
}

// Generates mesh for a scale
static void init_from_scale(GLModel& model, double radius)
{
    const float out_radius_long  = float(radius) * (1.0f + ScaleLongTooth);
    const float out_radius_short = float(radius) * (1.0f + 0.5f * ScaleLongTooth);

    GLModel::Geometry init_data;
    init_data.format = { GLModel::Geometry::EPrimitiveType::Lines, GLModel::Geometry::EVertexLayout::P3 };
    init_data.reserve_vertices(2 * ScaleStepsCount);
    init_data.reserve_indices(2 * ScaleStepsCount);

    // vertices + indices
    for (unsigned int i = 0; i < ScaleStepsCount; ++i) {
        const float angle = float(i * ScaleStepRad);
        const float cosa = ::cos(angle);
        const float sina = ::sin(angle);
        const float in_x = cosa * float(radius);
        const float in_y = sina * float(radius);
        const float out_x = (i % ScaleLongEvery == 0) ? cosa * out_radius_long : cosa * out_radius_short;
        const float out_y = (i % ScaleLongEvery == 0) ? sina * out_radius_long : sina * out_radius_short;

        // vertices
        init_data.add_vertex(Vec3f(in_x, in_y, 0.0f));
        init_data.add_vertex(Vec3f(out_x, out_y, 0.0f));

        // indices
        init_data.add_line(i * 2, i * 2 + 1);
    }

    model.init_from(std::move(init_data));
    model.set_color(ColorRGBA::WHITE());
}

// Generates mesh for a snap_radii
static void init_from_snap_radii(GLModel& model, double radius)
{
    const float step = 2.0f * float(PI) / float(SnapRegionsCount);
    const float in_radius = float(radius) / 3.0f;
    const float out_radius = 2.0f * in_radius;

    GLModel::Geometry init_data;
    init_data.format = { GLModel::Geometry::EPrimitiveType::Lines, GLModel::Geometry::EVertexLayout::P3 };
    init_data.reserve_vertices(2 * ScaleStepsCount);
    init_data.reserve_indices(2 * ScaleStepsCount);

    // vertices + indices
    for (unsigned int i = 0; i < ScaleStepsCount; ++i) {
        const float angle = float(i) * step;
        const float cosa = ::cos(angle);
        const float sina = ::sin(angle);
        const float in_x = cosa * in_radius;
        const float in_y = sina * in_radius;
        const float out_x = cosa * out_radius;
        const float out_y = sina * out_radius;

        // vertices
        init_data.add_vertex(Vec3f(in_x, in_y, 0.0f));
        init_data.add_vertex(Vec3f(out_x, out_y, 0.0f));

        // indices
        init_data.add_line(i * 2, i * 2 + 1);
    }

    model.init_from(std::move(init_data));
    model.set_color(ColorRGBA::WHITE());
}

// Generates mesh for a angle_arc
static void init_from_angle_arc(GLModel& model, double angle, double radius)
{
    model.reset();

    const float step_angle = float(angle) / float(AngleResolution);
    const float ex_radius = float(radius);

    GLModel::Geometry init_data;
    init_data.format = { GLModel::Geometry::EPrimitiveType::LineStrip, GLModel::Geometry::EVertexLayout::P3 };
    init_data.reserve_vertices(1 + AngleResolution);
    init_data.reserve_indices(1 + AngleResolution);

    // vertices + indices
    for (unsigned int i = 0; i <= AngleResolution; ++i) {
        const float angle = float(i) * step_angle;
        init_data.add_vertex(Vec3f(::cos(angle) * ex_radius, ::sin(angle) * ex_radius, 0.0f));
        init_data.add_index(i);
    }

    model.init_from(std::move(init_data));
}

//! --

GLGizmoCut3D::GLGizmoCut3D(GLCanvas3D& parent, const std::string& icon_filename, unsigned int sprite_id)
    : GLGizmoBase(parent, icon_filename, sprite_id)
    , m_connectors_group_id (GrabberID::Count)
    , m_connector_type (CutConnectorType::Plug)
    , m_connector_style (size_t(CutConnectorStyle::Prism))
    , m_connector_shape_id (size_t(CutConnectorShape::Circle))
{
//    m_modes = { _u8L("Planar"), _u8L("Grid")
//              , _u8L("Radial"), _u8L("Modular")
//    };

    m_connector_modes = { _u8L("Auto"), _u8L("Manual") };

    std::map<const wchar_t, std::string> connetor_types = {
        {ImGui::PlugMarker , _u8L("Plug")  }, 
        {ImGui::DowelMarker, _u8L("Dowel") },
    };
    for (auto connector : connetor_types) {
        std::string type_label = " " + connector.second + " ";
        type_label += connector.first;
        m_connector_types.push_back(type_label);
    }

    m_connector_styles = { _u8L("Prism"), _u8L("Frustum")
//              , _u8L("Claw")
    };

    m_connector_shapes = { _u8L("Triangle"), _u8L("Square"), _u8L("Hexagon"), _u8L("Circle")
//              , _u8L("D-shape")
    };

    m_axis_names = { "X", "Y", "Z" };

    m_part_orientation_names = {
        {"none",    _L("Keep orientation")},
        {"on_cut",  _L("Place on cut")},
        {"flip",    _L("Flip upside down")},
    };

    m_labels_map = {
        {"Connectors"   , _u8L("Connectors")},
        {"Type"         , _u8L("Type")},
        {"Style"        , _u8L("Style")},
        {"Shape"        , _u8L("Shape")},
        {"Depth ratio"  , _u8L("Depth ratio")},
        {"Size"         , _u8L("Size")},
    };

    update_connector_shape();
}

std::string GLGizmoCut3D::get_tooltip() const
{
    std::string tooltip;
    if (m_hover_id == Z || (m_dragging && m_hover_id == CutPlane)) {
        double koef = m_imperial_units ? ObjectManipulation::mm_to_in : 1.0;
        std::string unit_str = " " + (m_imperial_units ? _u8L("in") : _u8L("mm"));
        const BoundingBoxf3& tbb = m_transformed_bounding_box;

        const std::string name = m_keep_as_parts ? _u8L("Part") : _u8L("Object");
        if (tbb.max.z() >= 0.0) {
            double top = (tbb.min.z() <= 0.0 ? tbb.max.z() : tbb.size().z()) * koef;
            tooltip += format(static_cast<float>(top), 2) + " " + unit_str + " (" + name + " A)";
            if (tbb.min.z() <= 0.0)
                tooltip += "\n";
        }
        if (tbb.min.z() <= 0.0) {
            double bottom = (tbb.max.z() <= 0.0 ? tbb.size().z() : (tbb.min.z() * (-1))) * koef;
            tooltip += format(static_cast<float>(bottom), 2) + " " + unit_str + " (" + name + " B)";
        }
        return tooltip;
    }

    if (!m_dragging && m_hover_id == CutPlane)
        return _u8L("Click to flip the cut plane\n"
                    "Drag to move the cut plane");

    if (tooltip.empty() && (m_hover_id == X || m_hover_id == Y)) {
        std::string axis = m_hover_id == X ? "X" : "Y";
        return axis + ": " + format(float(rad2deg(m_angle)), 1) + _u8L("°");
    }

    return tooltip;
}

bool GLGizmoCut3D::on_mouse(const wxMouseEvent &mouse_event)
{
    Vec2i mouse_coord(mouse_event.GetX(), mouse_event.GetY());
    Vec2d mouse_pos = mouse_coord.cast<double>();

    if (mouse_event.ShiftDown() && mouse_event.LeftDown())
        return gizmo_event(SLAGizmoEventType::LeftDown, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), mouse_event.CmdDown());
    if (mouse_event.CmdDown() && mouse_event.LeftDown())
        return false;
    if (cut_line_processing()) {
        if (mouse_event.ShiftDown()) {
            if (mouse_event.Moving()|| mouse_event.Dragging())
                return gizmo_event(SLAGizmoEventType::Moving, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), mouse_event.CmdDown());
            if (mouse_event.LeftUp())
                return gizmo_event(SLAGizmoEventType::LeftUp, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), mouse_event.CmdDown());
        }
        discard_cut_line_processing();
    }
    else if (mouse_event.Moving())
        return false;

    if (use_grabbers(mouse_event)) {
        if (m_hover_id >= m_connectors_group_id) {
            if (mouse_event.LeftDown() && !mouse_event.CmdDown() && !mouse_event.AltDown())
                unselect_all_connectors();
            if (mouse_event.LeftUp() && !mouse_event.ShiftDown())
                gizmo_event(SLAGizmoEventType::LeftUp, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), mouse_event.CmdDown());
        }
        else if (m_hover_id == CutPlane) {
            if (mouse_event.LeftDown())
                m_was_cut_plane_dragged = false;
            else if (mouse_event.LeftUp() && !m_was_cut_plane_dragged)
                flip_cut_plane();
        }
        return true;
    }

    static bool pending_right_up = false;
    if (mouse_event.LeftDown()) {
        bool grabber_contains_mouse = (get_hover_id() != -1);
        const bool shift_down = mouse_event.ShiftDown();
        if ((!shift_down || grabber_contains_mouse) &&
            gizmo_event(SLAGizmoEventType::LeftDown, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), false))
            return true;
    }
    else if (mouse_event.Dragging()) {
        bool control_down = mouse_event.CmdDown();
        if (m_parent.get_move_volume_id() != -1) {
            // don't allow dragging objects with the Sla gizmo on
            return true;
        }
        if (!control_down &&
            gizmo_event(SLAGizmoEventType::Dragging, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), false)) {
            // the gizmo got the event and took some action, no need to do
            // anything more here
            m_parent.set_as_dirty();
            return true;
        }
        if (control_down && (mouse_event.LeftIsDown() || mouse_event.RightIsDown())) {
            // CTRL has been pressed while already dragging -> stop current action
            if (mouse_event.LeftIsDown())
                gizmo_event(SLAGizmoEventType::LeftUp, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), true);
            else if (mouse_event.RightIsDown())
                pending_right_up = false;
        }
    }
    else if (mouse_event.LeftUp() && !m_parent.is_mouse_dragging()) {
        // in case SLA/FDM gizmo is selected, we just pass the LeftUp event
        // and stop processing - neither object moving or selecting is
        // suppressed in that case
        gizmo_event(SLAGizmoEventType::LeftUp, mouse_pos, mouse_event.ShiftDown(), mouse_event.AltDown(), mouse_event.CmdDown());
        return true;
    }
    else if (mouse_event.RightDown()) {
        if (m_parent.get_selection().get_object_idx() != -1 &&
            gizmo_event(SLAGizmoEventType::RightDown, mouse_pos, false, false, false)) {
            // we need to set the following right up as processed to avoid showing
            // the context menu if the user release the mouse over the object
            pending_right_up = true;
            // event was taken care of by the SlaSupports gizmo
            return true;
        }
    }
    else if (pending_right_up && mouse_event.RightUp()) {
        pending_right_up = false;
        return true;
    }
    return false;
}

void GLGizmoCut3D::shift_cut(double delta)
{
    Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Move cut plane"), UndoRedo::SnapshotType::GizmoAction);
    set_center(m_plane_center + m_cut_normal * delta, true);
    m_ar_plane_center = m_plane_center;
}

void GLGizmoCut3D::rotate_vec3d_around_plane_center(Vec3d&vec)
{
    vec = Transformation(translation_transform(m_plane_center) * m_rotation_m * translation_transform(-m_plane_center)).get_matrix() * vec;
}

void GLGizmoCut3D::put_connectors_on_cut_plane(const Vec3d& cp_normal, double cp_offset)
{
    ModelObject* mo = m_c->selection_info()->model_object();
    if (CutConnectors& connectors = mo->cut_connectors; !connectors.empty()) {
        const float sla_shift        = m_c->selection_info()->get_sla_shift();
        const Vec3d& instance_offset = mo->instances[m_c->selection_info()->get_active_instance()]->get_offset();

        for (auto& connector : connectors) {
            // convert connetor pos to the world coordinates
            Vec3d pos = connector.pos + instance_offset;
            pos[Z] += sla_shift;
            // scalar distance from point to plane along the normal
            double distance = -cp_normal.dot(pos) + cp_offset;
            // move connector
            connector.pos += distance * cp_normal;
        }
    }
}

// returns true if the camera (forward) is pointing in the negative direction of the cut normal
bool GLGizmoCut3D::is_looking_forward() const
{
    const Camera& camera = wxGetApp().plater()->get_camera();
    const double dot = camera.get_dir_forward().dot(m_cut_normal);
    return dot < 0.05;
}

void GLGizmoCut3D::update_clipper()
{
    // update cut_normal
    Vec3d normal = m_rotation_m * Vec3d::UnitZ();
    normal.normalize();
    m_cut_normal = normal;

    // calculate normal and offset for clipping plane
    Vec3d beg = m_bb_center;
    beg[Z] -= m_radius;
    rotate_vec3d_around_plane_center(beg);

    m_clp_normal  = normal;
    double offset = normal.dot(m_plane_center);
    double dist   = normal.dot(beg);

    m_parent.set_color_clip_plane(normal, offset);

    if (!is_looking_forward()) {
        // recalculate normal and offset for clipping plane, if camera is looking downward to cut plane
        normal = m_rotation_m * (-1. * Vec3d::UnitZ());
        normal.normalize();

        beg = m_bb_center;
        beg[Z] += m_radius;
        rotate_vec3d_around_plane_center(beg);

        m_clp_normal = normal;
        offset       = normal.dot(m_plane_center);
        dist         = normal.dot(beg);
    }

    m_c->object_clipper()->set_range_and_pos(normal, offset, dist);

    put_connectors_on_cut_plane(normal, offset);

    if (m_raycasters.empty())
        on_register_raycasters_for_picking();
    else
        update_raycasters_for_picking_transform();
}

void GLGizmoCut3D::set_center(const Vec3d& center, bool update_tbb /*=false*/)
{
    set_center_pos(center, update_tbb);
    update_clipper();
}

bool GLGizmoCut3D::render_combo(const std::string& label, const std::vector<std::string>& lines, size_t& selection_idx)
{
    ImGui::AlignTextToFramePadding();
    m_imgui->text(label);
    ImGui::SameLine(m_label_width);
    ImGui::PushItemWidth(m_control_width);
    
    size_t selection_out = selection_idx;
    // It is necessary to use BeginGroup(). Otherwise, when using SameLine() is called, then other items will be drawn inside the combobox.
    ImGui::BeginGroup();
    ImVec2 combo_pos = ImGui::GetCursorScreenPos();
    if (ImGui::BeginCombo(("##"+label).c_str(), "")) {
        for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
            ImGui::PushID(int(line_idx));
            if (ImGui::Selectable("", line_idx == selection_idx))
                selection_out = line_idx;

            ImGui::SameLine();
            ImGui::Text("%s", lines[line_idx].c_str());
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImVec2      backup_pos = ImGui::GetCursorScreenPos();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + style.FramePadding.x, combo_pos.y + style.FramePadding.y));
    ImGui::Text("%s", selection_out < lines.size() ? lines[selection_out].c_str() : UndefLabel.c_str());
    ImGui::SetCursorScreenPos(backup_pos);
    ImGui::EndGroup();

    bool is_changed = selection_idx != selection_out;
    selection_idx = selection_out;

    if (is_changed)
        update_connector_shape();

    return is_changed;
}

bool GLGizmoCut3D::render_double_input(const std::string& label, double& value_in)
{
    ImGui::AlignTextToFramePadding();
    m_imgui->text(label);
    ImGui::SameLine(m_label_width);
    ImGui::PushItemWidth(m_control_width);

    double value = value_in;
    if (m_imperial_units)
        value *= ObjectManipulation::mm_to_in;
    double old_val = value;
    ImGui::InputDouble(("##" + label).c_str(), &value, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_CharsDecimal);

    ImGui::SameLine();
    m_imgui->text(m_imperial_units ? _L("in") : _L("mm"));

    value_in = value * (m_imperial_units ? ObjectManipulation::in_to_mm : 1.0);
    return !is_approx(old_val, value);
}

bool GLGizmoCut3D::render_slider_double_input(const std::string& label, float& value_in, float& tolerance_in)
{
    constexpr float UndefMinVal = -0.1f;
    const float f_mm_to_in = static_cast<float>(ObjectManipulation::mm_to_in);

    auto render_slider = [this, UndefMinVal, f_mm_to_in]
                         (const std::string& label, float& val, float def_val, float max_val, const wxString& tooltip) {
        float min_val = val < 0.f ? UndefMinVal : def_val;
        float value = val;
        if (m_imperial_units) {
            min_val *= f_mm_to_in;
            value   *= f_mm_to_in;
        }
        const float old_val = value;
        const std::string format = val < 0.f ? UndefLabel : (m_imperial_units ? "%.4f  " + _u8L("in") : "%.2f  " + _u8L("mm"));

        m_imgui->slider_float(label.c_str(), &value, min_val, max_val, format.c_str(), 1.f, true, tooltip);
        val = value * (m_imperial_units ? static_cast<float>(ObjectManipulation::in_to_mm) : 1.f);

        return !is_approx(old_val, value);
    };

    const BoundingBoxf3 bbox = m_bounding_box;
    const float mean_size = float((bbox.size().x() + bbox.size().y() + bbox.size().z()) / 9.0) * (m_imperial_units ? f_mm_to_in : 1.f);

    ImGuiWrapper::text(label);

    ImGui::SameLine(m_label_width);
    ImGui::PushItemWidth(m_control_width * 0.7f);

    const bool is_value_changed = render_slider("##" + label, value_in, 1.f, mean_size, _L("Value"));
    
    ImGui::SameLine();
    ImGui::PushItemWidth(m_control_width * 0.45f);

    const bool is_tolerance_changed = render_slider("##tolerance_" + label, tolerance_in, 0.f, 0.5f * mean_size, _L("Tolerance"));

    return is_value_changed || is_tolerance_changed;
}

void GLGizmoCut3D::render_move_center_input(int axis)
{
    m_imgui->text(m_axis_names[axis]+":");
    ImGui::SameLine();
    ImGui::PushItemWidth(0.3f*m_control_width);

    Vec3d move = m_plane_center;
    double in_val, value = in_val = move[axis];
    if (m_imperial_units)
        value *= ObjectManipulation::mm_to_in;
    ImGui::InputDouble(("##move_" + m_axis_names[axis]).c_str(), &value, 0.0, 0.0, "%.2f", ImGuiInputTextFlags_CharsDecimal);
    ImGui::SameLine();

    double val = value * (m_imperial_units ? ObjectManipulation::in_to_mm : 1.0);

    if (in_val != val) {
        move[axis] = val;
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Move cut plane"), UndoRedo::SnapshotType::GizmoAction);
        set_center(move, true);
        m_ar_plane_center = m_plane_center;
    }
}

bool GLGizmoCut3D::render_connect_type_radio_button(CutConnectorType type)
{
    ImGui::SameLine(type == CutConnectorType::Plug ? m_label_width : 0);
    ImGui::PushItemWidth(m_control_width);
    if (ImGui::RadioButton(m_connector_types[size_t(type)].c_str(), m_connector_type == type)) {
        m_connector_type = type;
        update_connector_shape();
        return true;
    }
    return false;
}

void GLGizmoCut3D::render_connect_mode_radio_button(CutConnectorMode mode)
{
    ImGui::SameLine(mode == CutConnectorMode::Auto ? m_label_width : 2 * m_label_width);
    ImGui::PushItemWidth(m_control_width);
    if (ImGui::RadioButton(m_connector_modes[int(mode)].c_str(), m_connector_mode == mode))
        m_connector_mode = mode;
}

bool GLGizmoCut3D::render_reset_button(const std::string& label_id, const std::string& tooltip) const
{
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, style.ItemSpacing.y });

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.25f, 0.25f, 0.25f, 0.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.4f, 0.4f, 0.4f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.4f, 0.4f, 0.4f, 1.0f });

    std::string btn_label;
    btn_label += ImGui::RevertButton;
    const bool revert = ImGui::Button((btn_label +"##" + label_id).c_str());

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        m_imgui->tooltip(tooltip.c_str(), ImGui::GetFontSize() * 20.0f);

    ImGui::PopStyleVar();

    return revert;
}

void GLGizmoCut3D::render_cut_plane()
{
    if (cut_line_processing())
        return;

    GLShaderProgram* shader = wxGetApp().get_shader("flat");
    if (shader == nullptr)
        return;

    glsafe(::glEnable(GL_DEPTH_TEST));
    glsafe(::glDisable(GL_CULL_FACE));
    glsafe(::glEnable(GL_BLEND));
    glsafe(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    shader->start_using();

    const Camera& camera = wxGetApp().plater()->get_camera();
    const Transform3d view_model_matrix = camera.get_view_matrix() * translation_transform(m_plane_center) * m_rotation_m;

    shader->set_uniform("view_model_matrix", view_model_matrix);
    shader->set_uniform("projection_matrix", camera.get_projection_matrix());

    if (can_perform_cut() && has_valid_contour()) {
        if (m_hover_id == CutPlane)
            m_plane.model.set_color({ 0.9f, 0.9f, 0.9f, 0.5f });
        else
            m_plane.model.set_color({ 0.8f, 0.8f, 0.8f, 0.5f });
    }
    else
        m_plane.model.set_color({ 1.0f, 0.8f, 0.8f, 0.5f });
    m_plane.model.render();

    glsafe(::glEnable(GL_CULL_FACE));
    glsafe(::glDisable(GL_BLEND));

    shader->stop_using();
}

static double get_grabber_mean_size(const BoundingBoxf3& bb)
{
    return (bb.size().x() + bb.size().y() + bb.size().z()) / 30.;
}

static double get_half_size(double size)
{
    return std::max(size * 0.35, 0.05);
}

static double get_dragging_half_size(double size)
{
    return get_half_size(size) * 1.25;
}

void GLGizmoCut3D::render_model(GLModel& model, const ColorRGBA& color, Transform3d view_model_matrix)
{
    GLShaderProgram* shader = wxGetApp().get_shader("gouraud_light");
    if (shader) {
        shader->start_using();

        shader->set_uniform("view_model_matrix", view_model_matrix);
        shader->set_uniform("projection_matrix", wxGetApp().plater()->get_camera().get_projection_matrix());

        model.set_color(color);
        model.render();

        shader->stop_using();
    }
}

void GLGizmoCut3D::render_line(GLModel& line_model, const ColorRGBA& color, Transform3d view_model_matrix, float width)
{
    GLShaderProgram* shader = OpenGLManager::get_gl_info().is_core_profile() ? wxGetApp().get_shader("dashed_thick_lines") : wxGetApp().get_shader("flat");
    if (shader) {
        shader->start_using();

        shader->set_uniform("view_model_matrix", view_model_matrix);
        shader->set_uniform("projection_matrix", wxGetApp().plater()->get_camera().get_projection_matrix());
        shader->set_uniform("width", width);

        line_model.set_color(color);
        line_model.render();

        shader->stop_using();
    }
}

void GLGizmoCut3D::render_rotation_snapping(GrabberID axis, const ColorRGBA& color)
{
    GLShaderProgram* line_shader = OpenGLManager::get_gl_info().is_core_profile() ? wxGetApp().get_shader("dashed_thick_lines") : wxGetApp().get_shader("flat");
    if (!line_shader)
        return;

    const Camera& camera = wxGetApp().plater()->get_camera();
    Transform3d view_model_matrix = camera.get_view_matrix() * translation_transform(m_plane_center) * m_start_dragging_m;

    if (axis == X)
        view_model_matrix = view_model_matrix * rotation_transform(0.5 * PI * Vec3d::UnitY()) * rotation_transform(-PI * Vec3d::UnitZ());
    else
        view_model_matrix = view_model_matrix * rotation_transform(-0.5 * PI * Vec3d::UnitZ()) * rotation_transform(-0.5 * PI * Vec3d::UnitY());

    line_shader->start_using();
    line_shader->set_uniform("projection_matrix", camera.get_projection_matrix());
    line_shader->set_uniform("view_model_matrix", view_model_matrix);
    line_shader->set_uniform("width", 0.25f);

    m_circle.render();
    m_scale.render();
    m_snap_radii.render();
    m_reference_radius.render();
    if (m_dragging) {
        line_shader->set_uniform("width", 1.5f);
        m_angle_arc.set_color(color);
        m_angle_arc.render();
    }

    line_shader->stop_using();
}

void GLGizmoCut3D::render_grabber_connection(const ColorRGBA& color, Transform3d view_matrix)
{
    const Transform3d line_view_matrix = view_matrix * scale_transform(Vec3d(1.0, 1.0, m_grabber_connection_len));

    render_line(m_grabber_connection, color, line_view_matrix, 0.2f);
};

void GLGizmoCut3D::render_cut_plane_grabbers()
{
    glsafe(::glClear(GL_DEPTH_BUFFER_BIT));

    ColorRGBA color = ColorRGBA::GRAY();

    const Transform3d view_matrix = wxGetApp().plater()->get_camera().get_view_matrix() * translation_transform(m_plane_center) * m_rotation_m;

    const double mean_size = get_grabber_mean_size(m_bounding_box);
    double size;

    const bool dragging_by_cut_plane = m_dragging && m_hover_id == CutPlane;

    if (!dragging_by_cut_plane) {
        render_grabber_connection(GRABBER_COLOR, view_matrix);

        // render sphere grabber
        size = m_dragging ? get_dragging_half_size(mean_size) : get_half_size(mean_size);
        color = m_hover_id == Y ? complementary(ColorRGBA::GREEN()) :                
                m_hover_id == X ? complementary(ColorRGBA::RED())   :
                m_hover_id == Z ? GRABBER_COLOR                     :   ColorRGBA::GRAY();
        render_model(m_sphere.model, color, view_matrix * translation_transform(m_grabber_connection_len * Vec3d::UnitZ()) * scale_transform(size));
    }

    const bool no_one_grabber_hovered = !m_dragging && (m_hover_id < 0 || m_hover_id == CutPlane);

    // render X grabber

    if (no_one_grabber_hovered || m_hover_id == X)
    {
        size = m_dragging && m_hover_id == X ? get_dragging_half_size(mean_size) : get_half_size(mean_size);
        const Vec3d cone_scale = Vec3d(0.75 * size, 0.75 * size, 1.8 * size);
        color = m_hover_id == X ? complementary(ColorRGBA::RED()) : ColorRGBA::RED();

        if (m_hover_id == X) {
            render_grabber_connection(color, view_matrix);
            render_rotation_snapping(X, color);
        }

        Vec3d offset = Vec3d(0.0, 1.25 * size, m_grabber_connection_len);
        render_model(m_cone.model, color, view_matrix * translation_transform(offset) * rotation_transform(-0.5 * PI * Vec3d::UnitX()) * scale_transform(cone_scale));
        offset = Vec3d(0.0, -1.25 * size, m_grabber_connection_len);
        render_model(m_cone.model, color, view_matrix * translation_transform(offset) * rotation_transform(0.5 * PI * Vec3d::UnitX()) * scale_transform(cone_scale));
    }

    // render Y grabber

    if (no_one_grabber_hovered || m_hover_id == Y)
    {
        size = m_dragging && m_hover_id == Y ? get_dragging_half_size(mean_size) : get_half_size(mean_size);
        const Vec3d cone_scale = Vec3d(0.75 * size, 0.75 * size, 1.8 * size);
        color = m_hover_id == Y ? complementary(ColorRGBA::GREEN()) : ColorRGBA::GREEN();

        if (m_hover_id == Y) {
            render_grabber_connection(color, view_matrix);
            render_rotation_snapping(Y, color);
        }

        Vec3d offset = Vec3d(1.25 * size, 0.0, m_grabber_connection_len);
        render_model(m_cone.model, color, view_matrix * translation_transform(offset) * rotation_transform(0.5 * PI * Vec3d::UnitY()) * scale_transform(cone_scale));
        offset = Vec3d(-1.25 * size, 0.0, m_grabber_connection_len);
        render_model(m_cone.model, color, view_matrix * translation_transform(offset)* rotation_transform(-0.5 * PI * Vec3d::UnitY()) * scale_transform(cone_scale));
    }
}

void GLGizmoCut3D::render_cut_line()
{
    if (!cut_line_processing() || m_line_end.isApprox(Vec3d::Zero()))
        return;

    glsafe(::glEnable(GL_DEPTH_TEST));
    glsafe(::glClear(GL_DEPTH_BUFFER_BIT));

    m_cut_line.reset();
    m_cut_line.init_from(its_make_line((Vec3f)m_line_beg.cast<float>(), (Vec3f)m_line_end.cast<float>()));

    render_line(m_cut_line, GRABBER_COLOR, wxGetApp().plater()->get_camera().get_view_matrix(), 0.25f);
}

bool GLGizmoCut3D::on_init()
{
    m_grabbers.emplace_back();
    m_shortcut_key = WXK_CONTROL_C;

    // initiate info shortcuts
    const wxString ctrl  = GUI::shortkey_ctrl_prefix();
    const wxString alt   = GUI::shortkey_alt_prefix();
    const wxString shift = "Shift+";

    m_shortcuts.push_back(std::make_pair(_L("Left click"),         _L("Add connector")));
    m_shortcuts.push_back(std::make_pair(_L("Right click"),        _L("Remove connector")));
    m_shortcuts.push_back(std::make_pair(_L("Drag"),               _L("Move connector")));
    m_shortcuts.push_back(std::make_pair(shift + _L("Left click"), _L("Add connector to selection")));
    m_shortcuts.push_back(std::make_pair(alt   + _L("Left click"), _L("Remove connector from selection")));
    m_shortcuts.push_back(std::make_pair(ctrl  + "A",              _L("Select all connectors")));

    return true;
}

void GLGizmoCut3D::on_load(cereal::BinaryInputArchive& ar)
{
    ar( m_keep_upper, m_keep_lower, m_rotate_lower, m_rotate_upper, m_hide_cut_plane, m_mode, m_connectors_editing,
        m_ar_plane_center, m_rotation_m);

    m_transformed_bounding_box = transformed_bounding_box(m_ar_plane_center, m_rotation_m);
    set_center_pos(m_ar_plane_center);

    m_parent.request_extra_frame();
}

void GLGizmoCut3D::on_save(cereal::BinaryOutputArchive& ar) const
{ 
    ar( m_keep_upper, m_keep_lower, m_rotate_lower, m_rotate_upper, m_hide_cut_plane, m_mode, m_connectors_editing,
        m_ar_plane_center, m_start_dragging_m);
}

std::string GLGizmoCut3D::on_get_name() const
{
    return _u8L("Cut");
}

void GLGizmoCut3D::on_set_state()
{
    if (m_state == On) {
        m_parent.set_use_color_clip_plane(true);
        m_parent.set_color_clip_plane_colors({ UPPER_PART_COLOR , LOWER_PART_COLOR });

        update_bb();
        m_connectors_editing = !m_selected.empty();
        m_transformed_bounding_box = transformed_bounding_box(m_plane_center, m_rotation_m);

        // initiate archived values
        m_ar_plane_center   = m_plane_center;
        m_start_dragging_m  = m_rotation_m;

        m_parent.request_extra_frame();
    }
    else {
        if (auto oc = m_c->object_clipper()) {
            oc->set_behavior(true, true, 0.);
            oc->release();
        }
        m_selected.clear();
        m_parent.set_use_color_clip_plane(false);
        m_c->selection_info()->set_use_shift(false);
    }
}

void GLGizmoCut3D::on_register_raycasters_for_picking()
{
    assert(m_raycasters.empty());
    // the gizmo grabbers are rendered on top of the scene, so the raytraced picker should take it into account
    m_parent.set_raycaster_gizmos_on_top(true);

    init_picking_models();

    if (m_connectors_editing) {
        if (CommonGizmosDataObjects::SelectionInfo* si = m_c->selection_info()) {
            const CutConnectors& connectors = si->model_object()->cut_connectors;
            for (int i = 0; i < int(connectors.size()); ++i)
                m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, i + m_connectors_group_id, *(m_shapes[connectors[i].attribs]).mesh_raycaster, Transform3d::Identity()));
        }
    }
    else if (!cut_line_processing()) {
        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, X, *m_cone.mesh_raycaster, Transform3d::Identity()));
        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, X, *m_cone.mesh_raycaster, Transform3d::Identity()));

        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, Y, *m_cone.mesh_raycaster, Transform3d::Identity()));
        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, Y, *m_cone.mesh_raycaster, Transform3d::Identity()));

        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, Z, *m_sphere.mesh_raycaster, Transform3d::Identity()));

        m_raycasters.emplace_back(m_parent.add_raycaster_for_picking(SceneRaycaster::EType::Gizmo, CutPlane, *m_plane.mesh_raycaster, Transform3d::Identity()));
    }

    update_raycasters_for_picking_transform();
}

void GLGizmoCut3D::on_unregister_raycasters_for_picking()
{
    m_parent.remove_raycasters_for_picking(SceneRaycaster::EType::Gizmo);
    m_raycasters.clear();
    // the gizmo grabbers are rendered on top of the scene, so the raytraced picker should take it into account
    m_parent.set_raycaster_gizmos_on_top(false);
}

void GLGizmoCut3D::update_raycasters_for_picking()
{
    on_unregister_raycasters_for_picking();
    on_register_raycasters_for_picking();
}

void GLGizmoCut3D::set_volumes_picking_state(bool state)
{
    std::vector<std::shared_ptr<SceneRaycasterItem>>* raycasters = m_parent.get_raycasters_for_picking(SceneRaycaster::EType::Volume);
    if (raycasters != nullptr) {
        const Selection& selection = m_parent.get_selection();
        const Selection::IndicesList ids = selection.get_volume_idxs();
        for (unsigned int id : ids) {
            const GLVolume* v = selection.get_volume(id);
            auto it = std::find_if(raycasters->begin(), raycasters->end(), [v](std::shared_ptr<SceneRaycasterItem> item) { return item->get_raycaster() == v->mesh_raycaster.get(); });
            if (it != raycasters->end())
                (*it)->set_active(state);
        }
    }
}

void GLGizmoCut3D::update_raycasters_for_picking_transform()
{
    if (m_connectors_editing) {
        CommonGizmosDataObjects::SelectionInfo* si = m_c->selection_info();
        if (!si) 
            return;
        const ModelObject* mo = si->model_object();
        const CutConnectors& connectors = mo->cut_connectors;
        if (connectors.empty())
            return;
        auto inst_id = m_c->selection_info()->get_active_instance();
        if (inst_id < 0)
            return;

        const Vec3d& instance_offset = mo->instances[inst_id]->get_offset();
        const double sla_shift = double(m_c->selection_info()->get_sla_shift());

        const bool looking_forward = is_looking_forward();

        for (size_t i = 0; i < connectors.size(); ++i) {
            const CutConnector& connector = connectors[i];

            float height = connector.height;
            // recalculate connector position to world position
            Vec3d pos = connector.pos + instance_offset;
            if (connector.attribs.type == CutConnectorType::Dowel &&
                connector.attribs.style == CutConnectorStyle::Prism) {
                height = 0.05f;
                if (!looking_forward)
                    pos += 0.05 * m_clp_normal;
            }
            pos[Z] += sla_shift;

            const Transform3d scale_trafo = scale_transform(Vec3f(connector.radius, connector.radius, height).cast<double>());
            m_raycasters[i]->set_transform(translation_transform(pos) * m_rotation_m * scale_trafo);
        }
    }
    else if (!cut_line_processing()){
        const Transform3d trafo = translation_transform(m_plane_center) * m_rotation_m;

        const BoundingBoxf3 box = m_bounding_box;

        const double size = get_half_size(get_grabber_mean_size(box));
        Vec3d scale = Vec3d(0.75 * size, 0.75 * size, 1.8 * size);

        int id = 0;

        Vec3d offset = Vec3d(0.0, 1.25 * size, m_grabber_connection_len);
        m_raycasters[id++]->set_transform(trafo * translation_transform(offset) * rotation_transform(-0.5 * PI * Vec3d::UnitX()) * scale_transform(scale));
        offset = Vec3d(0.0, -1.25 * size, m_grabber_connection_len);
        m_raycasters[id++]->set_transform(trafo * translation_transform(offset) * rotation_transform(0.5 * PI * Vec3d::UnitX()) * scale_transform(scale));

        offset = Vec3d(1.25 * size, 0.0, m_grabber_connection_len);
        m_raycasters[id++]->set_transform(trafo * translation_transform(offset) * rotation_transform(0.5 * PI * Vec3d::UnitY()) * scale_transform(scale));
        offset = Vec3d(-1.25 * size, 0.0, m_grabber_connection_len);
        m_raycasters[id++]->set_transform(trafo * translation_transform(offset) * rotation_transform(-0.5 * PI * Vec3d::UnitY()) * scale_transform(scale));

        offset = 1.25 * size * Vec3d::UnitZ();
        m_raycasters[id++]->set_transform(trafo * translation_transform(m_grabber_connection_len * Vec3d::UnitZ()) * scale_transform(size));

        m_raycasters[id++]->set_transform(trafo);
    }
}

void GLGizmoCut3D::on_set_hover_id() 
{
}

bool GLGizmoCut3D::on_is_activable() const
{
    const Selection& selection = m_parent.get_selection();
    const int object_idx = selection.get_object_idx();
    if (object_idx < 0 || selection.is_wipe_tower())
        return false;

    if (const ModelObject* mo = wxGetApp().plater()->model().objects[object_idx];
        mo->is_cut() && mo->volumes.size() == 1) {
        const ModelVolume* volume = mo->volumes[0];
        if (volume->is_cut_connector() && volume->cut_info.connector_type == CutConnectorType::Dowel)
            return false;
    }

    // This is assumed in GLCanvas3D::do_rotate, do not change this
    // without updating that function too.
    return selection.is_single_full_instance() && !m_parent.is_layers_editing_enabled();
}

bool GLGizmoCut3D::on_is_selectable() const
{
    return wxGetApp().get_mode() != comSimple;
}

Vec3d GLGizmoCut3D::mouse_position_in_local_plane(GrabberID axis, const Linef3& mouse_ray) const
{
    double half_pi = 0.5 * PI;

    Transform3d m = Transform3d::Identity();

    switch (axis)
    {
    case X:
    {
        m.rotate(Eigen::AngleAxisd(half_pi, Vec3d::UnitZ()));
        m.rotate(Eigen::AngleAxisd(-half_pi, Vec3d::UnitY()));
        break;
    }
    case Y:
    {
        m.rotate(Eigen::AngleAxisd(half_pi, Vec3d::UnitY()));
        m.rotate(Eigen::AngleAxisd(half_pi, Vec3d::UnitZ()));
        break;
    }
    default:
    case Z:
    {
        // no rotation applied
        break;
    }
    }

    m = m * m_start_dragging_m.inverse();
    m.translate(-m_plane_center);

    return transform(mouse_ray, m).intersect_plane(0.0);
}

void GLGizmoCut3D::dragging_grabber_z(const GLGizmoBase::UpdateData &data)
{
    const Vec3d grabber_init_pos        = (m_hover_id == CutPlane ? 0. : m_grabber_connection_len) * Vec3d::UnitZ();
    const Vec3d starting_drag_position  = translation_transform(m_plane_center) * m_rotation_m * grabber_init_pos;
    double      projection              = 0.0;

    Vec3d starting_vec = m_rotation_m * Vec3d::UnitZ();
    if (starting_vec.norm() != 0.0) {
        const Vec3d mouse_dir = data.mouse_ray.unit_vector();
        // finds the intersection of the mouse ray with the plane parallel to the camera viewport and passing throught the starting position
        // use ray-plane intersection see i.e. https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection algebric form
        // in our case plane normal and ray direction are the same (orthogonal view)
        // when moving to perspective camera the negative z unit axis of the camera needs to be transformed in world space and used as plane normal
        const Vec3d inters = data.mouse_ray.a + (starting_drag_position - data.mouse_ray.a).dot(mouse_dir) * mouse_dir;
        // vector from the starting position to the found intersection
        const Vec3d inters_vec = inters - starting_drag_position;

        starting_vec.normalize();
        // finds projection of the vector along the staring direction
        projection = inters_vec.dot(starting_vec);
    }
    if (wxGetKeyState(WXK_SHIFT))
        projection = m_snap_step * std::round(projection / m_snap_step);

    const Vec3d shift = starting_vec * projection;

    // move  cut plane center
    set_center(m_plane_center + shift, true);

    m_was_cut_plane_dragged = true;
}

void GLGizmoCut3D::dragging_grabber_xy(const GLGizmoBase::UpdateData &data)
{
    const Vec2d mouse_pos = to_2d(mouse_position_in_local_plane((GrabberID)m_hover_id, data.mouse_ray));

    const Vec2d orig_dir = Vec2d::UnitX();
    const Vec2d new_dir  = mouse_pos.normalized();

    const double two_pi = 2.0 * PI;

    double theta = ::acos(std::clamp(new_dir.dot(orig_dir), -1.0, 1.0));
    if (cross2(orig_dir, new_dir) < 0.0)
        theta = two_pi - theta;

    const double len = mouse_pos.norm();
    // snap to coarse snap region
    if (m_snap_coarse_in_radius <= len && len <= m_snap_coarse_out_radius) {
        const double step = two_pi / double(SnapRegionsCount);
        theta             = step * std::round(theta / step);
    }
    // snap to fine snap region (scale)
    else if (m_snap_fine_in_radius <= len && len <= m_snap_fine_out_radius) {
        const double step = two_pi / double(ScaleStepsCount);
        theta             = step * std::round(theta / step);
    }

    if (is_approx(theta, two_pi))
        theta = 0.0;
    if (m_hover_id == X)
        theta += 0.5 * PI;

    Vec3d rotation = Vec3d::Zero();
    rotation[m_hover_id] = theta;

    const Transform3d rotation_tmp = m_start_dragging_m * rotation_transform(rotation);
    const bool update_tbb = !m_rotation_m.rotation().isApprox(rotation_tmp.rotation());
    m_rotation_m = rotation_tmp;
    if (update_tbb)
        m_transformed_bounding_box = transformed_bounding_box(m_plane_center, m_rotation_m);

    m_angle = theta;
    while (m_angle > two_pi)
        m_angle -= two_pi;
    if (m_angle < 0.0)
        m_angle += two_pi;

    update_clipper();
}

void GLGizmoCut3D::dragging_connector(const GLGizmoBase::UpdateData &data)
{
    CutConnectors&          connectors = m_c->selection_info()->model_object()->cut_connectors;
    Vec3d                   pos;
    Vec3d                   pos_world;

    if (unproject_on_cut_plane(data.mouse_pos.cast<double>(), pos, pos_world)) {
        connectors[m_hover_id - m_connectors_group_id].pos = pos;
        update_raycasters_for_picking_transform();
    }
}

void GLGizmoCut3D::on_dragging(const UpdateData& data)
{
    if (m_hover_id < 0)
        return;
    if (m_hover_id == Z || m_hover_id == CutPlane)
        dragging_grabber_z(data);
    else if (m_hover_id == X || m_hover_id == Y)
        dragging_grabber_xy(data);
    else if (m_hover_id >= m_connectors_group_id && m_connector_mode == CutConnectorMode::Manual)
        dragging_connector(data);
}

void GLGizmoCut3D::on_start_dragging()
{
    m_angle = 0.0;
    if (m_hover_id >= m_connectors_group_id && m_connector_mode == CutConnectorMode::Manual)
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Move connector"), UndoRedo::SnapshotType::GizmoAction);

    if (m_hover_id == X || m_hover_id == Y)
        m_start_dragging_m = m_rotation_m;
}

void GLGizmoCut3D::on_stop_dragging()
{
    if (m_hover_id == X || m_hover_id == Y) {
        m_angle_arc.reset();
        m_angle = 0.0;
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Rotate cut plane"), UndoRedo::SnapshotType::GizmoAction);
        m_start_dragging_m = m_rotation_m;
    }
    else if (m_hover_id == Z || m_hover_id == CutPlane) {
        if (m_was_cut_plane_dragged)
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Move cut plane"), UndoRedo::SnapshotType::GizmoAction);
        m_ar_plane_center = m_plane_center;
    }
}

void GLGizmoCut3D::set_center_pos(const Vec3d& center_pos, bool update_tbb /*=false*/)
{
    BoundingBoxf3 tbb = m_transformed_bounding_box;
    if (update_tbb) {
        Vec3d normal = m_rotation_m.inverse() * Vec3d(m_plane_center - center_pos);
        tbb.translate(normal.z() * Vec3d::UnitZ());
    }

    bool can_set_center_pos = false;
    {
        if (tbb.max.z() > -.5 && tbb.min.z() < .5)
            can_set_center_pos = true;
        else {
            const double old_dist = (m_bb_center - m_plane_center).norm();
            const double new_dist = (m_bb_center - center_pos).norm();
            // check if forcing is reasonable
            if (new_dist < old_dist)
                can_set_center_pos = true;
        }
    }

    if (can_set_center_pos) {
        m_transformed_bounding_box = tbb;
        m_plane_center = center_pos;
        m_center_offset = m_plane_center - m_bb_center;
    }
}

BoundingBoxf3 GLGizmoCut3D::bounding_box() const
{
    BoundingBoxf3 ret;
    const Selection& selection = m_parent.get_selection();
    const Selection::IndicesList& idxs = selection.get_volume_idxs();
    for (unsigned int i : idxs) {
        const GLVolume* volume = selection.get_volume(i);
        // respect just to the solid parts for FFF and ignore pad and supports for SLA
        if (!volume->is_modifier && !volume->is_sla_pad() && !volume->is_sla_support())
            ret.merge(volume->transformed_convex_hull_bounding_box());
    }
    return ret;
}

BoundingBoxf3 GLGizmoCut3D::transformed_bounding_box(const Vec3d& plane_center, const Transform3d& rotation_m/* = Transform3d::Identity()*/) const
{
    const Selection& selection = m_parent.get_selection();

    const auto first_volume = selection.get_first_volume();
    Vec3d instance_offset   = first_volume->get_instance_offset();
    instance_offset[Z]     += first_volume->get_sla_shift_z();

    const auto cut_matrix = Transform3d::Identity() * rotation_m.inverse() * translation_transform(instance_offset - plane_center);

    const Selection::IndicesList& idxs = selection.get_volume_idxs();
    BoundingBoxf3 ret;
    for (unsigned int i : idxs) {
        const GLVolume* volume = selection.get_volume(i);
        // respect just to the solid parts for FFF and ignore pad and supports for SLA
        if (!volume->is_modifier && !volume->is_sla_pad() && !volume->is_sla_support()) {

            const auto instance_matrix = volume->get_instance_transformation().get_matrix_no_offset();
            auto volume_trafo = instance_matrix * volume->get_volume_transformation().get_matrix();
            ret.merge(volume->transformed_convex_hull_bounding_box(cut_matrix * volume_trafo));
        }
    }
    return ret;
}

void GLGizmoCut3D::update_bb()
{
    const BoundingBoxf3 box = bounding_box();
    if (!box.defined)
        return;
    if (!m_max_pos.isApprox(box.max) || !m_min_pos.isApprox(box.min)) {

        m_bounding_box = box;

        invalidate_cut_plane();

        m_max_pos = box.max;
        m_min_pos = box.min;
        m_bb_center = box.center();
        m_transformed_bounding_box = transformed_bounding_box(m_bb_center);
        if (box.contains(m_center_offset))
            set_center_pos(m_bb_center + m_center_offset);
        else
            set_center_pos(m_bb_center);

        m_radius = box.radius();
        m_grabber_connection_len = 0.5 * m_radius;// std::min<double>(0.75 * m_radius, 35.0);
        m_grabber_radius = m_grabber_connection_len * 0.85;

        m_snap_coarse_in_radius   = m_grabber_radius / 3.0;
        m_snap_coarse_out_radius  = m_snap_coarse_in_radius * 2.;
        m_snap_fine_in_radius     = m_grabber_connection_len * 0.85;
        m_snap_fine_out_radius    = m_grabber_connection_len * 1.15;

        m_plane.reset();
        m_cone.reset();
        m_sphere.reset();
        m_grabber_connection.reset();
        m_circle.reset();
        m_scale.reset();
        m_snap_radii.reset();
        m_reference_radius.reset();

        on_unregister_raycasters_for_picking();

        clear_selection();
        if (CommonGizmosDataObjects::SelectionInfo* selection = m_c->selection_info();
            selection && selection->model_object())
            m_selected.resize(selection->model_object()->cut_connectors.size(), false);
    }
}

void GLGizmoCut3D::init_picking_models()
{
    if (!m_cone.model.is_initialized()) {
        indexed_triangle_set its = its_make_cone(1.0, 1.0, PI / 12.0);
        m_cone.model.init_from(its);
        m_cone.mesh_raycaster = std::make_unique<MeshRaycaster>(std::make_shared<const TriangleMesh>(std::move(its)));
    }
    if (!m_sphere.model.is_initialized()) {
        indexed_triangle_set its = its_make_sphere(1.0, PI / 12.0);
        m_sphere.model.init_from(its);
        m_sphere.mesh_raycaster = std::make_unique<MeshRaycaster>(std::make_shared<const TriangleMesh>(std::move(its)));
    }

    if (!m_plane.model.is_initialized() && !m_hide_cut_plane && !m_connectors_editing) {
        const double cp_width = 0.02 * get_grabber_mean_size(m_bounding_box);
        indexed_triangle_set its = its_make_frustum_dowel((double)m_cut_plane_radius_koef * m_radius, cp_width, m_cut_plane_as_circle ? 180 : 4);
        m_plane.model.init_from(its);
        m_plane.mesh_raycaster = std::make_unique<MeshRaycaster>(std::make_shared<const TriangleMesh>(std::move(its)));
    }

    if (m_shapes.empty())
        init_connector_shapes();
}

void GLGizmoCut3D::init_rendering_items()
{
    if (!m_grabber_connection.is_initialized())
        m_grabber_connection.init_from(its_make_line(Vec3f::Zero(), Vec3f::UnitZ()));
    if (!m_circle.is_initialized())
        init_from_circle(m_circle, m_grabber_radius);
    if (!m_scale.is_initialized())
        init_from_scale(m_scale, m_grabber_radius);
    if (!m_snap_radii.is_initialized())
        init_from_snap_radii(m_snap_radii, m_grabber_radius);
    if (!m_reference_radius.is_initialized()) {
        m_reference_radius.init_from(its_make_line(Vec3f::Zero(), m_grabber_connection_len * Vec3f::UnitX()));
        m_reference_radius.set_color(ColorRGBA::WHITE());
    }
    if (!m_angle_arc.is_initialized() || m_angle != 0.0)
        init_from_angle_arc(m_angle_arc, m_angle, m_grabber_connection_len);
}

void GLGizmoCut3D::render_clipper_cut()
{
    if (! m_connectors_editing)
        ::glDisable(GL_DEPTH_TEST);
    m_c->object_clipper()->render_cut();
    if (! m_connectors_editing)
        ::glEnable(GL_DEPTH_TEST);
}

void GLGizmoCut3D::on_render()
{
    if (m_state == On) {
        // This gizmo is showing the object elevated. Tell the common
        // SelectionInfo object to lie about the actual shift.
        m_c->selection_info()->set_use_shift(true);
    }

    update_clipper();

    init_picking_models();

    init_rendering_items();

    render_connectors();

    render_clipper_cut();

    if (!m_hide_cut_plane && !m_connectors_editing) {
        render_cut_plane();
        render_cut_plane_grabbers();
    }

    render_cut_line();

    m_selection_rectangle.render(m_parent);
}

void GLGizmoCut3D::render_debug_input_window(float x)
{
    return;
    m_imgui->begin(wxString("DEBUG"));

    ImVec2 pos = ImGui::GetWindowPos();
    pos.x = x;
    ImGui::SetWindowPos(pos, ImGuiCond_Always);
/*
    static bool  hide_clipped  = false;
    static bool  fill_cut      = false;
    static float contour_width = 0.4f;

    m_imgui->checkbox(_L("Hide cut plane and grabbers"), m_hide_cut_plane);
    if (m_imgui->checkbox("hide_clipped", hide_clipped) && !hide_clipped)
        m_clp_normal = m_c->object_clipper()->get_clipping_plane()->get_normal();
    m_imgui->checkbox("fill_cut", fill_cut);
    m_imgui->slider_float("contour_width", &contour_width, 0.f, 3.f);
    if (auto oc = m_c->object_clipper())
        oc->set_behavior(hide_clipped || m_connectors_editing, fill_cut || m_connectors_editing, double(contour_width));
*/
    ImGui::PushItemWidth(0.5f * m_label_width);
    if (auto oc = m_c->object_clipper(); oc && m_imgui->slider_float("contour_width", &m_contour_width, 0.f, 3.f))
        oc->set_behavior(m_connectors_editing, m_connectors_editing, double(m_contour_width));

    ImGui::Separator();

    if (m_imgui->checkbox(_L("Render cut plane as circle"), m_cut_plane_as_circle))
        m_plane.reset();

    ImGui::PushItemWidth(0.5f * m_label_width);
    if (m_imgui->slider_float("cut_plane_radius_koef", &m_cut_plane_radius_koef, 1.f, 2.f))
        m_plane.reset();

    m_imgui->end();
}

void GLGizmoCut3D::adjust_window_position(float x, float y, float bottom_limit)
{
    static float last_y = 0.0f;
    static float last_h = 0.0f;

    const float win_h = ImGui::GetWindowHeight();
    y                 = std::min(y, bottom_limit - win_h);

    ImGui::SetWindowPos(ImVec2(x, y), ImGuiCond_Always);

    if (!is_approx(last_h, win_h) || !is_approx(last_y, y)) {
        // ask canvas for another frame to render the window in the correct position
        m_imgui->set_requires_extra_frame();
        if (!is_approx(last_h, win_h))
            last_h = win_h;
        if (!is_approx(last_y, y))
            last_y = y;
    }
}

void GLGizmoCut3D::unselect_all_connectors()
{
    std::fill(m_selected.begin(), m_selected.end(), false);
    m_selected_count = 0;
    validate_connector_settings();
}

void GLGizmoCut3D::select_all_connectors()
{
    std::fill(m_selected.begin(), m_selected.end(), true);
    m_selected_count = int(m_selected.size());
}

void GLGizmoCut3D::render_shortcuts()
{
    if (m_imgui->button("? " + (m_show_shortcuts ? wxString(ImGui::CollapseBtn) : wxString(ImGui::ExpandBtn))))
        m_show_shortcuts = !m_show_shortcuts;

    if (m_shortcut_label_width < 0.f) {
        for (const auto& shortcut : m_shortcuts) {
            const float width = m_imgui->calc_text_size(shortcut.first).x;
            if (m_shortcut_label_width < width)
                m_shortcut_label_width = width;
        }
        m_shortcut_label_width += +m_imgui->scaled(1.f);
    }

    if (m_show_shortcuts)
        for (const auto&shortcut : m_shortcuts ){
            m_imgui->text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, shortcut.first);
            ImGui::SameLine(m_shortcut_label_width);
            m_imgui->text(shortcut.second);
        }
}

void GLGizmoCut3D::apply_selected_connectors(std::function<void(size_t idx)> apply_fn)
{
    for (size_t idx = 0; idx < m_selected.size(); idx++)
        if (m_selected[idx])
            apply_fn(idx);

    update_raycasters_for_picking_transform();
}

void GLGizmoCut3D::render_connectors_input_window(CutConnectors &connectors)
{
    // add shortcuts panel
    render_shortcuts();

    // Connectors section

    ImGui::Separator();

    // WIP : Auto : Need to implement
    // m_imgui->text(_L("Mode"));
    // render_connect_mode_radio_button(CutConnectorMode::Auto);
    // render_connect_mode_radio_button(CutConnectorMode::Manual);

    ImGui::AlignTextToFramePadding();
    m_imgui->text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, m_labels_map["Connectors"]);

    m_imgui->disabled_begin(connectors.empty());
    ImGui::SameLine(m_label_width);
    const wxString act_name = _L("Remove connectors");
    if (render_reset_button("connectors", into_u8(act_name))) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), act_name, UndoRedo::SnapshotType::GizmoAction);
        reset_connectors();
    }
    m_imgui->disabled_end();

    render_flip_plane_button(m_connectors_editing && connectors.empty());

    m_imgui->text(m_labels_map["Type"]);
    bool type_changed = render_connect_type_radio_button(CutConnectorType::Plug);
    type_changed     |= render_connect_type_radio_button(CutConnectorType::Dowel);
    if (type_changed)
        apply_selected_connectors([this, &connectors] (size_t idx) { connectors[idx].attribs.type = CutConnectorType(m_connector_type); });

    m_imgui->disabled_begin(m_connector_type == CutConnectorType::Dowel);
    if (type_changed && m_connector_type == CutConnectorType::Dowel) {
        m_connector_style = size_t(CutConnectorStyle::Prism);
        apply_selected_connectors([this, &connectors](size_t idx) { connectors[idx].attribs.style = CutConnectorStyle(m_connector_style); });
    }
    if (render_combo(m_labels_map["Style"], m_connector_styles, m_connector_style))
        apply_selected_connectors([this, &connectors](size_t idx) { connectors[idx].attribs.style = CutConnectorStyle(m_connector_style); });
    m_imgui->disabled_end();

    if (render_combo(m_labels_map["Shape"], m_connector_shapes, m_connector_shape_id))
        apply_selected_connectors([this, &connectors](size_t idx) { connectors[idx].attribs.shape = CutConnectorShape(m_connector_shape_id); });

    if (render_slider_double_input(m_labels_map["Depth ratio"], m_connector_depth_ratio, m_connector_depth_ratio_tolerance))
        apply_selected_connectors([this, &connectors](size_t idx) {
            if (m_connector_depth_ratio > 0)
                connectors[idx].height           = m_connector_depth_ratio;
            if (m_connector_depth_ratio_tolerance >= 0)
                connectors[idx].height_tolerance = m_connector_depth_ratio_tolerance;
        });

    if (render_slider_double_input(m_labels_map["Size"], m_connector_size, m_connector_size_tolerance))
        apply_selected_connectors([this, &connectors](size_t idx) {
            if (m_connector_size > 0)
                connectors[idx].radius           = 0.5f * m_connector_size;
            if (m_connector_size_tolerance >= 0)
                connectors[idx].radius_tolerance = 0.5f * m_connector_size_tolerance;
        });

    ImGui::Separator();

    if (m_imgui->button(_L("Confirm connectors"))) {
        unselect_all_connectors();
        set_connectors_editing(false);
    }

    ImGui::SameLine(m_label_width + 1.15f * m_control_width);

    if (m_imgui->button(_L("Cancel"))) {
        reset_connectors();
        set_connectors_editing(false);
    }
}

void GLGizmoCut3D::render_build_size()
{
    double              koef     = m_imperial_units ? ObjectManipulation::mm_to_in : 1.0;
    wxString            unit_str = " " + (m_imperial_units ? _L("in") : _L("mm"));
            
    Vec3d    tbb_sz = m_transformed_bounding_box.size();
    wxString size   =   "X: " + double_to_string(tbb_sz.x() * koef, 2) + unit_str +
                     ",  Y: " + double_to_string(tbb_sz.y() * koef, 2) + unit_str +
                     ",  Z: " + double_to_string(tbb_sz.z() * koef, 2) + unit_str;

    ImGui::AlignTextToFramePadding();
    m_imgui->text(_L("Build Volume"));
    ImGui::SameLine();
    m_imgui->text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, size);
}

void GLGizmoCut3D::reset_cut_plane()
{
    m_angle_arc.reset();
    m_transformed_bounding_box = transformed_bounding_box(m_bb_center);
    set_center(m_bb_center);
    m_start_dragging_m = m_rotation_m = Transform3d::Identity();
    m_ar_plane_center  = m_plane_center;
}

void GLGizmoCut3D::invalidate_cut_plane()
{
    m_rotation_m    = Transform3d::Identity();
    m_plane_center  = Vec3d::Zero();
    m_min_pos       = Vec3d::Zero();
    m_max_pos       = Vec3d::Zero();
    m_bb_center     = Vec3d::Zero();
    m_center_offset = Vec3d::Zero();
}

void GLGizmoCut3D::set_connectors_editing(bool connectors_editing)
{
    if (m_connectors_editing == connectors_editing)
        return;

    m_connectors_editing = connectors_editing;
    update_raycasters_for_picking();

    m_c->object_clipper()->set_behavior(m_connectors_editing, m_connectors_editing, double(m_contour_width));

    m_parent.request_extra_frame();
}

void GLGizmoCut3D::flip_cut_plane()
{
    m_rotation_m = m_rotation_m * rotation_transform(PI * Vec3d::UnitX());
    m_transformed_bounding_box = transformed_bounding_box(m_plane_center, m_rotation_m);

    Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Flip cut plane"), UndoRedo::SnapshotType::GizmoAction);
    m_start_dragging_m = m_rotation_m;

    update_clipper();
}

void GLGizmoCut3D::render_flip_plane_button(bool disable_pred /*=false*/)
{
    ImGui::SameLine();

    if (m_hover_id == CutPlane)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonHovered));

    m_imgui->disabled_begin(disable_pred);
        if (m_imgui->button(_L("Flip cut plane")))
            flip_cut_plane();
    m_imgui->disabled_end();

    if (m_hover_id == CutPlane)
        ImGui::PopStyleColor();
}

void GLGizmoCut3D::add_vertical_scaled_interval(float interval)
{
    ImGui::GetCurrentWindow()->DC.CursorPos.y += m_imgui->scaled(interval);
}

void GLGizmoCut3D::add_horizontal_scaled_interval(float interval)
{
    ImGui::GetCurrentWindow()->DC.CursorPos.x += m_imgui->scaled(interval);
}

void GLGizmoCut3D::add_horizontal_shift(float shift)
{
    ImGui::GetCurrentWindow()->DC.CursorPos.x += shift;
}

void GLGizmoCut3D::render_color_marker(float size, const ImU32& color)
{
    ImGui::SameLine();
    const float radius = 0.5f * size;
    ImVec2 pos = ImGui::GetCurrentWindow()->DC.CursorPos;
    pos.x += size;
    pos.y += 1.25f * radius;
    ImGui::GetCurrentWindow()->DrawList->AddNgonFilled(pos, radius, color, 6);
    ImGuiWrapper::text(" ");
}

void GLGizmoCut3D::render_cut_plane_input_window(CutConnectors &connectors)
{
    // WIP : cut plane mode
    // render_combo(_u8L("Mode"), m_modes, m_mode);

    if (m_mode == size_t(CutMode::cutPlanar)) {
        ImGui::AlignTextToFramePadding();
        ImGuiWrapper::text(wxString(ImGui::InfoMarkerSmall));
        ImGui::SameLine();
        ImGuiWrapper::text_colored(ImGuiWrapper::COL_ORANGE_LIGHT,
                              get_wraped_wxString(_L("Hold SHIFT key to draw a cut line"), 40));
        ImGui::Separator();

        render_build_size();

        ImGui::AlignTextToFramePadding();
        ImGuiWrapper::text(_L("Cut position") + ": ");
        ImGui::SameLine();
        render_move_center_input(Z);
        ImGui::SameLine();

        const bool has_connectors = !connectors.empty();

        const bool is_cut_plane_init = m_rotation_m.isApprox(Transform3d::Identity()) && m_bb_center.isApprox(m_plane_center);
        m_imgui->disabled_begin(is_cut_plane_init);
            wxString act_name = _L("Reset cutting plane");
            if (render_reset_button("cut_plane", into_u8(act_name))) {
                Plater::TakeSnapshot snapshot(wxGetApp().plater(), act_name, UndoRedo::SnapshotType::GizmoAction);
                reset_cut_plane();
            }
        m_imgui->disabled_end();

//        render_flip_plane_button();

        add_vertical_scaled_interval(0.75f);

        m_imgui->disabled_begin(!m_keep_upper || !m_keep_lower || m_keep_as_parts);
            if (m_imgui->button(has_connectors ? _L("Edit connectors") : _L("Add connectors")))
                set_connectors_editing(true);
        m_imgui->disabled_end();

        ImGui::SameLine(1.5f * m_control_width);

        m_imgui->disabled_begin(is_cut_plane_init && !has_connectors);
            act_name = _L("Reset cut");
            if (m_imgui->button(act_name, _L("Reset cutting plane and remove connectors"))) {
                Plater::TakeSnapshot snapshot(wxGetApp().plater(), act_name, UndoRedo::SnapshotType::GizmoAction);
                reset_cut_plane();
                reset_connectors();
            }
        m_imgui->disabled_end();

        ImGui::Separator();

        // render "After Cut" section

        ImVec2 label_size;
        for (const auto& item : m_part_orientation_names) {
            const ImVec2 text_size = ImGuiWrapper::calc_text_size(item.second);
            if (label_size.x < text_size.x)
                label_size.x = text_size.x;
            if (label_size.y < text_size.y)
                label_size.y = text_size.y;
        }
        const float h_shift     = label_size.x + m_imgui->scaled(3.f);
        const float marker_size = label_size.y;

        auto render_part_name = [this, marker_size, has_connectors](const wxString& name, bool& keep_part, const ImU32& color) {
            bool keep = true;
            add_horizontal_shift(m_imgui->scaled(1.2f));
            m_imgui->checkbox((m_keep_as_parts ? _L("Part") : _L("Object")) + " " + name, has_connectors ? keep : keep_part);
            render_color_marker(marker_size, color);
        };

        auto render_part_actions = [this, h_shift] (const wxString& suffix, const bool& keep_part, bool& place_on_cut_part, bool& rotate_part)
        {
            float shift = m_imgui->scaled(1.2f);
            if (suffix == "##lower")
                shift += h_shift;
            m_imgui->disabled_begin(!keep_part || m_keep_as_parts);
                add_horizontal_shift(shift);
                if (m_imgui->radio_button(m_part_orientation_names.at("none") + suffix, !place_on_cut_part && !rotate_part)) {
                    rotate_part = false;
                    place_on_cut_part = false;
                }

                add_horizontal_shift(shift);
                if (m_imgui->radio_button(m_part_orientation_names.at("on_cut") + suffix, place_on_cut_part)) {
                    place_on_cut_part = !place_on_cut_part;
                    rotate_part = false;
                }

                add_horizontal_shift(shift);
                if (m_imgui->radio_button(m_part_orientation_names.at("flip") + suffix, rotate_part)) {
                    rotate_part = !rotate_part;
                    place_on_cut_part = false;
                }
            m_imgui->disabled_end();
        };

        ImGuiWrapper::text(_L("After cut") + ": ");
        add_vertical_scaled_interval(0.5f);

        m_imgui->disabled_begin(has_connectors || m_keep_as_parts);
            render_part_name("A", m_keep_upper, m_imgui->to_ImU32(UPPER_PART_COLOR));
            ImGui::SameLine(h_shift + ImGui::GetCurrentWindow()->WindowPadding.x);
            render_part_name("B", m_keep_lower, m_imgui->to_ImU32(LOWER_PART_COLOR));
        m_imgui->disabled_end();

        add_vertical_scaled_interval(0.5f);

        const ImVec2 pos = ImGui::GetCurrentWindow()->DC.CursorPos;
        render_part_actions("##upper", m_keep_upper, m_place_on_cut_upper, m_rotate_upper);

        ImGui::GetCurrentWindow()->DC.CursorPos = pos;
        render_part_actions("##lower", m_keep_lower, m_place_on_cut_lower, m_rotate_lower);

        add_vertical_scaled_interval(0.75f);

        m_imgui->disabled_begin(has_connectors);
            ImGuiWrapper::text(_L("Cut to") + ":");

            add_horizontal_scaled_interval(1.2f);
            // TRN CutGizmo: RadioButton Cut to ...
            if (m_imgui->radio_button(_L("Objects"), !m_keep_as_parts))
                m_keep_as_parts = false;
            ImGui::SameLine();
            // TRN CutGizmo: RadioButton Cut to ...
            if (m_imgui->radio_button(_L("Parts"), m_keep_as_parts))
                m_keep_as_parts = true;

            if (m_keep_as_parts) {
                m_keep_upper = m_keep_lower = true;
                m_place_on_cut_upper = m_place_on_cut_lower = false;
                m_rotate_upper = m_rotate_lower = false;
            }
        m_imgui->disabled_end();
    }

    ImGui::Separator();

    m_imgui->disabled_begin(!m_is_contour_changed && !can_perform_cut());
        if(m_imgui->button(_L("Perform cut")))
            perform_cut(m_parent.get_selection());
    m_imgui->disabled_end();
}

void GLGizmoCut3D::validate_connector_settings()
{
    if (m_connector_depth_ratio < 0.f)
        m_connector_depth_ratio = 3.f;
    if (m_connector_depth_ratio_tolerance < 0.f)
        m_connector_depth_ratio_tolerance = 0.1f;
    if (m_connector_size < 0.f)
        m_connector_size = 2.5f;
    if (m_connector_size_tolerance < 0.f)
        m_connector_size_tolerance = 0.f;

    if (m_connector_type == CutConnectorType::Undef)
        m_connector_type = CutConnectorType::Plug;
    if (m_connector_style == size_t(CutConnectorStyle::Undef))
        m_connector_style = size_t(CutConnectorStyle::Prism);
    if (m_connector_shape_id == size_t(CutConnectorShape::Undef))
        m_connector_shape_id = size_t(CutConnectorShape::Circle);
}

void GLGizmoCut3D::init_input_window_data(CutConnectors &connectors)
{
    m_imperial_units = wxGetApp().app_config->get_bool("use_inches");
    m_control_width  = m_imgui->get_font_size() * 9.f;

    if (m_connectors_editing && m_selected_count > 0) {
        float               depth_ratio             { UndefFloat };
        float               depth_ratio_tolerance   { UndefFloat };
        float               radius                  { UndefFloat };
        float               radius_tolerance        { UndefFloat };
        CutConnectorType    type                    { CutConnectorType::Undef };
        CutConnectorStyle   style                   { CutConnectorStyle::Undef };
        CutConnectorShape   shape                   { CutConnectorShape::Undef };

        bool is_init = false;
        for (size_t idx = 0; idx < m_selected.size(); idx++)
            if (m_selected[idx]) {
                const CutConnector& connector = connectors[idx];
                if (!is_init) {
                    depth_ratio             = connector.height;
                    depth_ratio_tolerance   = connector.height_tolerance;
                    radius                  = connector.radius;
                    radius_tolerance        = connector.radius_tolerance;
                    type                    = connector.attribs.type;
                    style                   = connector.attribs.style;
                    shape                   = connector.attribs.shape;

                    if (m_selected_count == 1)
                        break;
                    is_init = true;
                }
                else {
                    if (!is_approx(depth_ratio, connector.height))
                        depth_ratio         = UndefFloat;
                    if (!is_approx(depth_ratio_tolerance, connector.height_tolerance))
                        depth_ratio_tolerance = UndefFloat;
                    if (!is_approx(radius,connector.radius))
                        radius              = UndefFloat;
                    if (!is_approx(radius_tolerance, connector.radius_tolerance))
                        radius_tolerance    = UndefFloat;

                    if (type != connector.attribs.type)
                        type = CutConnectorType::Undef;
                    if (style != connector.attribs.style)
                        style = CutConnectorStyle::Undef;
                    if (shape != connector.attribs.shape)
                        shape = CutConnectorShape::Undef;
                }
            }

        m_connector_depth_ratio             = depth_ratio;
        m_connector_depth_ratio_tolerance   = depth_ratio_tolerance;
        m_connector_size                    = 2.f * radius;
        m_connector_size_tolerance          = 2.f * radius_tolerance;
        m_connector_type                    = type;
        m_connector_style                   = size_t(style);
        m_connector_shape_id                = size_t(shape);
    }

    if (m_label_width == 0.f) {
        for (const auto& item : m_labels_map) {
            const float width = ImGuiWrapper::calc_text_size(item.second).x;
            if (m_label_width < width)
                m_label_width = width;
        }
        m_label_width += m_imgui->scaled(1.f);
    }
}

void GLGizmoCut3D::render_input_window_warning() const
{
    if (m_is_contour_changed)
        return;
    if (m_has_invalid_connector) {
        wxString out = wxString(ImGui::WarningMarkerSmall) + _L("Invalid connectors detected") + ":";
        if (m_info_stats.outside_cut_contour > size_t(0))
            out += "\n - " + format_wxstr(_L_PLURAL("%1$d connector is out of cut contour", "%1$d connectors are out of cut contour", m_info_stats.outside_cut_contour),
                                          m_info_stats.outside_cut_contour);
        if (m_info_stats.outside_bb > size_t(0))
            out += "\n - " + format_wxstr(_L_PLURAL("%1$d connector is out of object", "%1$d connectors are out of object", m_info_stats.outside_bb),
                                           m_info_stats.outside_bb);
        if (m_info_stats.is_overlap)
            out += "\n - " + _L("Some connectors are overlapped");
        m_imgui->text(out);
    }
    if (!m_keep_upper && !m_keep_lower)
        m_imgui->text(wxString(ImGui::WarningMarkerSmall) + _L("Invalid state. \nNo one part is selected for keep after cut"));
    if (!has_valid_contour())
        m_imgui->text(wxString(ImGui::WarningMarkerSmall) + _L("Warning state. \nCut plane is placed out of object"));
}

void GLGizmoCut3D::on_render_input_window(float x, float y, float bottom_limit)
{
    m_imgui->begin(get_name(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // adjust window position to avoid overlap the view toolbar
    adjust_window_position(x, y, bottom_limit);

    CutConnectors& connectors = m_c->selection_info()->model_object()->cut_connectors;

    init_input_window_data(connectors);

    if (m_connectors_editing) // connectors mode
        render_connectors_input_window(connectors); 
    else
        render_cut_plane_input_window(connectors);

    render_input_window_warning();

    m_imgui->end();

    if (!m_connectors_editing) // connectors mode
        render_debug_input_window(x);
}

bool GLGizmoCut3D::is_outside_of_cut_contour(size_t idx, const CutConnectors& connectors, const Vec3d cur_pos)
{
    // check if connector pos is out of clipping plane
    if (m_c->object_clipper() && !m_c->object_clipper()->is_projection_inside_cut(cur_pos)) {
        m_info_stats.outside_cut_contour++;
        return true;
    }

    // check if connector bottom contour is out of clipping plane
    const CutConnector& cur_connector = connectors[idx];
    const CutConnectorShape shape = CutConnectorShape(cur_connector.attribs.shape);
    const int   sectorCount = shape == CutConnectorShape::Triangle  ? 3 :
                              shape == CutConnectorShape::Square    ? 4 :
                              shape == CutConnectorShape::Circle    ? 60: // supposably, 60 points are enough for conflict detection
                              shape == CutConnectorShape::Hexagon   ? 6 : 1 ;

    indexed_triangle_set mesh;
    auto& vertices = mesh.vertices;
    vertices.reserve(sectorCount + 1);

    float fa = 2 * PI / sectorCount;
    auto vec = Eigen::Vector2f(0, cur_connector.radius);
    for (float angle = 0; angle < 2.f * PI; angle += fa) {
        Vec2f p = Eigen::Rotation2Df(angle) * vec;
        vertices.emplace_back(Vec3f(p(0), p(1), 0.f));
    }
    its_transform(mesh, translation_transform(cur_pos) * m_rotation_m);

    for (auto vertex : vertices) {
        if (m_c->object_clipper() && !m_c->object_clipper()->is_projection_inside_cut(vertex.cast<double>())) {
            m_info_stats.outside_cut_contour++;
            return true;
        }
    }

    return false;
}

bool GLGizmoCut3D::is_conflict_for_connector(size_t idx, const CutConnectors& connectors, const Vec3d cur_pos)
{
    if (is_outside_of_cut_contour(idx, connectors, cur_pos))
        return true;

    const CutConnector& cur_connector = connectors[idx];    

    const Transform3d matrix = translation_transform(cur_pos) * m_rotation_m *
                               scale_transform(Vec3f(cur_connector.radius, cur_connector.radius, cur_connector.height).cast<double>());
    const BoundingBoxf3 cur_tbb = m_shapes[cur_connector.attribs].model.get_bounding_box().transformed(matrix);

    // check if connector's bounding box is inside the object's bounding box
    if (!m_bounding_box.contains(cur_tbb)) {
        m_info_stats.outside_bb++;
        return true;
    }

    // check if connectors are overlapping 
    for (size_t i = 0; i < connectors.size(); ++i) {
        if (i == idx)
            continue;
        const CutConnector& connector = connectors[i];

        if ((connector.pos - cur_connector.pos).norm() < double(connector.radius + cur_connector.radius)) {
            m_info_stats.is_overlap = true;
            return true;
        }
    }

    return false;
}

void GLGizmoCut3D::render_connectors()
{
    ::glEnable(GL_DEPTH_TEST);

    if (m_is_contour_changed || cut_line_processing() || 
        m_connector_mode == CutConnectorMode::Auto || !m_c->selection_info())
        return;

    const ModelObject* mo = m_c->selection_info()->model_object();
    auto inst_id = m_c->selection_info()->get_active_instance();
    if (inst_id < 0)
        return;
    const CutConnectors& connectors = mo->cut_connectors;
    if (connectors.size() != m_selected.size()) {
        // #ysFIXME
        clear_selection();
        m_selected.resize(connectors.size(), false);
    }

    ColorRGBA render_color = CONNECTOR_DEF_COLOR;

    const ModelInstance* mi = mo->instances[inst_id];
    const Vec3d& instance_offset = mi->get_offset();
    const double sla_shift       = double(m_c->selection_info()->get_sla_shift());

    m_has_invalid_connector = false;
    m_info_stats.invalidate();

    const bool looking_forward = is_looking_forward();

    for (size_t i = 0; i < connectors.size(); ++i) {
        const CutConnector& connector = connectors[i];

        float height = connector.height;
        // recalculate connector position to world position
        Vec3d pos = connector.pos + instance_offset + sla_shift * Vec3d::UnitZ();

        // First decide about the color of the point.
        const bool conflict_connector = is_conflict_for_connector(i, connectors, pos);
        if (conflict_connector) {
            m_has_invalid_connector = true;
            render_color = CONNECTOR_ERR_COLOR;
        }
        else // default connector color
            render_color = connector.attribs.type == CutConnectorType::Dowel ? DOWEL_COLOR          : PLAG_COLOR;

        if (!m_connectors_editing)
            render_color = CONNECTOR_ERR_COLOR;
        else if (size_t(m_hover_id - m_connectors_group_id) == i)
            render_color = conflict_connector ? HOVERED_ERR_COLOR :
                           connector.attribs.type == CutConnectorType::Dowel ? HOVERED_DOWEL_COLOR  : HOVERED_PLAG_COLOR;
        else if (m_selected[i])
            render_color = connector.attribs.type == CutConnectorType::Dowel ? SELECTED_DOWEL_COLOR : SELECTED_PLAG_COLOR;

        const Camera& camera = wxGetApp().plater()->get_camera();
        if (connector.attribs.type  == CutConnectorType::Dowel &&
            connector.attribs.style == CutConnectorStyle::Prism) {
            if (m_connectors_editing) {
                height = 0.05f;
                if (!looking_forward)
                    pos += 0.05 * m_clp_normal;
            }
            else {
                if (looking_forward)
                    pos -= static_cast<double>(height) * m_clp_normal;
                else
                    pos += static_cast<double>(height) * m_clp_normal;
                height *= 2;
            }
        }
        else if (!looking_forward)
            pos += 0.05 * m_clp_normal;

        const Transform3d view_model_matrix = camera.get_view_matrix() * translation_transform(pos) * m_rotation_m * 
                                              scale_transform(Vec3f(connector.radius, connector.radius, height).cast<double>());

        render_model(m_shapes[connector.attribs].model, render_color, view_model_matrix);
    }
}

bool GLGizmoCut3D::can_perform_cut() const
{
    if (m_has_invalid_connector || (!m_keep_upper && !m_keep_lower) || m_connectors_editing)
        return false;

    return true;// has_valid_contour();
}

bool GLGizmoCut3D::has_valid_contour() const
{
    const auto clipper = m_c->object_clipper();
    return clipper && clipper->has_valid_contour();
}

void GLGizmoCut3D::apply_connectors_in_model(ModelObject* mo, bool &create_dowels_as_separate_object)
{
    if (m_connector_mode == CutConnectorMode::Manual) {
        clear_selection();

        for (CutConnector&connector : mo->cut_connectors) {
            connector.rotation_m = m_rotation_m;

            if (connector.attribs.type == CutConnectorType::Dowel) {
                if (connector.attribs.style == CutConnectorStyle::Prism)
                    connector.height *= 2;
                create_dowels_as_separate_object = true;
            }
            else {
                // calculate shift of the connector center regarding to the position on the cut plane
                connector.pos += m_cut_normal * 0.5 * double(connector.height);
            }
        }
        mo->apply_cut_connectors(_u8L("Connector"));
    }
}

void GLGizmoCut3D::perform_cut(const Selection& selection)
{
    if (!can_perform_cut())
        return;
    const int instance_idx = selection.get_instance_idx();
    const int object_idx = selection.get_object_idx();

    wxCHECK_RET(instance_idx >= 0 && object_idx >= 0, "GLGizmoCut: Invalid object selection");

    Plater* plater = wxGetApp().plater();
    ModelObject* mo = plater->model().objects[object_idx];
    if (!mo)
        return;

    // deactivate CutGizmo and than perform a cut
    m_parent.reset_all_gizmos();

    // m_cut_z is the distance from the bed. Subtract possible SLA elevation.
    const double sla_shift_z    = selection.get_first_volume()->get_sla_shift_z();

    const Vec3d instance_offset = mo->instances[instance_idx]->get_offset();
    Vec3d cut_center_offset     = m_plane_center - instance_offset;
    cut_center_offset[Z] -= sla_shift_z;

    // perform cut
    {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Cut by Plane"));

        bool create_dowels_as_separate_object = false;
        const bool has_connectors = !mo->cut_connectors.empty();
        // update connectors pos as offset of its center before cut performing
        apply_connectors_in_model(mo, create_dowels_as_separate_object);

        plater->cut(object_idx, instance_idx, translation_transform(cut_center_offset) * m_rotation_m,
                    only_if(has_connectors ? true : m_keep_upper, ModelObjectCutAttribute::KeepUpper) |
                    only_if(has_connectors ? true : m_keep_lower, ModelObjectCutAttribute::KeepLower) |
                    only_if(has_connectors ? false: m_keep_as_parts, ModelObjectCutAttribute::KeepAsParts) |
                    only_if(m_place_on_cut_upper, ModelObjectCutAttribute::PlaceOnCutUpper) |
                    only_if(m_place_on_cut_lower, ModelObjectCutAttribute::PlaceOnCutLower) |
                    only_if(m_rotate_upper, ModelObjectCutAttribute::FlipUpper) |
                    only_if(m_rotate_lower, ModelObjectCutAttribute::FlipLower) |
                    only_if(create_dowels_as_separate_object, ModelObjectCutAttribute::CreateDowels) |
                    only_if(!has_connectors, ModelObjectCutAttribute::InvalidateCutInfo));
    }
}



// Unprojects the mouse position on the mesh and saves hit point and normal of the facet into pos_and_normal
// Return false if no intersection was found, true otherwise.
bool GLGizmoCut3D::unproject_on_cut_plane(const Vec2d& mouse_position, Vec3d& pos, Vec3d& pos_world)
{
    const float sla_shift = m_c->selection_info()->get_sla_shift();

    const ModelObject* mo = m_c->selection_info()->model_object();
    const ModelInstance* mi = mo->instances[m_c->selection_info()->get_active_instance()];
    const Camera& camera = wxGetApp().plater()->get_camera();

    // Calculate intersection with the clipping plane.
    const ClippingPlane* cp = m_c->object_clipper()->get_clipping_plane(true);
    Vec3d point;
    Vec3d direction;
    Vec3d hit;
    MeshRaycaster::line_from_mouse_pos(mouse_position, Transform3d::Identity(), camera, point, direction);
    Vec3d normal = -cp->get_normal().cast<double>();
    double den = normal.dot(direction);
    if (den != 0.) {
        double t = (-cp->get_offset() - normal.dot(point))/den;
        hit = (point + t * direction);
    } else
        return false;
    
    if (! m_c->object_clipper()->is_projection_inside_cut(hit))
        return false;

    // recalculate hit to object's local position
    Vec3d hit_d = hit;
    hit_d -= mi->get_offset();
    hit_d[Z] -= sla_shift;

    // Return both the point and the facet normal.
    pos = hit_d;
    pos_world = hit;

    return true; 
}

void GLGizmoCut3D::clear_selection()
{
    m_selected.clear();
    m_selected_count = 0;
}

void GLGizmoCut3D::reset_connectors()
{
    m_c->selection_info()->model_object()->cut_connectors.clear();
    update_raycasters_for_picking();
    clear_selection();
}

void GLGizmoCut3D::init_connector_shapes()
{
    for (const CutConnectorType& type : {CutConnectorType::Dowel, CutConnectorType::Plug})
        for (const CutConnectorStyle& style : {CutConnectorStyle::Frustum, CutConnectorStyle::Prism}) {
            if (type == CutConnectorType::Dowel && style == CutConnectorStyle::Frustum)
                continue;
            for (const CutConnectorShape& shape : {CutConnectorShape::Circle, CutConnectorShape::Hexagon, CutConnectorShape::Square, CutConnectorShape::Triangle}) {
                const CutConnectorAttributes attribs = { type, style, shape };
                const indexed_triangle_set its = ModelObject::get_connector_mesh(attribs);
                m_shapes[attribs].model.init_from(its);
                m_shapes[attribs].mesh_raycaster = std::make_unique<MeshRaycaster>(std::make_shared<const TriangleMesh>(std::move(its)));
            }
        }
}

void GLGizmoCut3D::update_connector_shape()
{
    CutConnectorAttributes attribs = { m_connector_type, CutConnectorStyle(m_connector_style), CutConnectorShape(m_connector_shape_id) };

    const indexed_triangle_set its = ModelObject::get_connector_mesh(attribs);
    m_connector_mesh.clear();
    m_connector_mesh = TriangleMesh(its);
}

bool GLGizmoCut3D::cut_line_processing() const
{
    return !m_line_beg.isApprox(Vec3d::Zero());
}

void GLGizmoCut3D::discard_cut_line_processing()
{
    m_line_beg = m_line_end = Vec3d::Zero();
}

bool GLGizmoCut3D::process_cut_line(SLAGizmoEventType action, const Vec2d& mouse_position)
{
    const Camera& camera = wxGetApp().plater()->get_camera();

    Vec3d pt;
    Vec3d dir;
    MeshRaycaster::line_from_mouse_pos(mouse_position, Transform3d::Identity(), camera, pt, dir);
    dir.normalize();
    pt += dir; // Move the pt along dir so it is not clipped.

    if (action == SLAGizmoEventType::LeftDown && !cut_line_processing()) {
        m_line_beg = pt;
        m_line_end = pt;
        on_unregister_raycasters_for_picking();
        return true;
    }

    if (cut_line_processing()) {
        m_line_end = pt;
        if (action == SLAGizmoEventType::LeftDown || action == SLAGizmoEventType::LeftUp) {
            Vec3d line_dir = m_line_end - m_line_beg;
            if (line_dir.norm() < 3.0)
                return true;

            Vec3d cross_dir = line_dir.cross(dir).normalized();
            Eigen::Quaterniond q;
            Transform3d m = Transform3d::Identity();
            m.matrix().block(0, 0, 3, 3) = q.setFromTwoVectors(Vec3d::UnitZ(), cross_dir).toRotationMatrix();

            const Vec3d new_plane_center = m_bb_center + cross_dir * cross_dir.dot(pt - m_bb_center);
            // update transformed bb
            const auto new_tbb = transformed_bounding_box(new_plane_center, m);
            const GLVolume* first_volume = m_parent.get_selection().get_first_volume();
            Vec3d instance_offset = first_volume->get_instance_offset();
            instance_offset[Z] += first_volume->get_sla_shift_z();

            const Vec3d trans_center_pos = m.inverse() * (new_plane_center - instance_offset) + new_tbb.center();
            if (new_tbb.contains(trans_center_pos)) {
                Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Cut by line"), UndoRedo::SnapshotType::GizmoAction);
                m_transformed_bounding_box = new_tbb;
                set_center(new_plane_center);
                m_start_dragging_m = m_rotation_m = m;
                m_ar_plane_center = m_plane_center;
            }

            m_angle_arc.reset();
            discard_cut_line_processing();
        }
        else if (action == SLAGizmoEventType::Moving)
            this->set_dirty();
        return true;
    }
    return false;
}

bool GLGizmoCut3D::add_connector(CutConnectors& connectors, const Vec2d& mouse_position)
{
    if (!m_connectors_editing)
        return false;

    Vec3d pos;
    Vec3d pos_world;
    if (unproject_on_cut_plane(mouse_position.cast<double>(), pos, pos_world)) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Add connector"), UndoRedo::SnapshotType::GizmoAction);
        unselect_all_connectors();

        connectors.emplace_back(pos, m_rotation_m,
                                m_connector_size * 0.5f, m_connector_depth_ratio,
                                m_connector_size_tolerance * 0.5f, m_connector_depth_ratio_tolerance,
                                CutConnectorAttributes( CutConnectorType(m_connector_type),
                                                        CutConnectorStyle(m_connector_style),
                                                        CutConnectorShape(m_connector_shape_id)));
        m_selected.push_back(true);
        m_selected_count = 1;
        assert(m_selected.size() == connectors.size());
        update_raycasters_for_picking();
        m_parent.set_as_dirty();

        return true;
    }
    return false;
}

bool GLGizmoCut3D::delete_selected_connectors(CutConnectors& connectors)
{
    if (connectors.empty())
        return false;

    Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Delete connector"), UndoRedo::SnapshotType::GizmoAction);

    // remove  connectors
    for (int i = int(connectors.size()) - 1; i >= 0; i--)
        if (m_selected[i])
            connectors.erase(connectors.begin() + i);
    // remove selections
    m_selected.erase(std::remove_if(m_selected.begin(), m_selected.end(), [](const auto& selected) {
        return selected; }), m_selected.end());
    m_selected_count = 0;

    assert(m_selected.size() == connectors.size());
    update_raycasters_for_picking();
    m_parent.set_as_dirty();
    return true;
}

void GLGizmoCut3D::select_connector(int idx, bool select)
{
    m_selected[idx] = select;
    if (select)
        ++m_selected_count;
    else
        --m_selected_count;
}

bool GLGizmoCut3D::is_selection_changed(bool alt_down, bool shift_down)
{
    if (m_hover_id >= m_connectors_group_id) {
        if (alt_down)
            select_connector(m_hover_id - m_connectors_group_id, false);
        else {
            if (!shift_down)
                unselect_all_connectors();
            select_connector(m_hover_id - m_connectors_group_id, true);
        }
        return true;
    }
    return false;
}

void GLGizmoCut3D::process_selection_rectangle(CutConnectors &connectors)
{
    GLSelectionRectangle::EState rectangle_status = m_selection_rectangle.get_state();

    ModelObject* mo          = m_c->selection_info()->model_object();
    int          active_inst = m_c->selection_info()->get_active_instance();

    // First collect positions of all the points in world coordinates.
    Transformation trafo = mo->instances[active_inst]->get_transformation();
    trafo.set_offset(trafo.get_offset() + double(m_c->selection_info()->get_sla_shift()) * Vec3d::UnitZ());

    std::vector<Vec3d> points;
    for (const CutConnector&connector : connectors)
        points.push_back(connector.pos + trafo.get_offset());

    // Now ask the rectangle which of the points are inside.
    std::vector<unsigned int> points_idxs = m_selection_rectangle.contains(points);
    m_selection_rectangle.stop_dragging();

    for (size_t idx : points_idxs)
        select_connector(int(idx), rectangle_status == GLSelectionRectangle::EState::Select);
}

bool GLGizmoCut3D::gizmo_event(SLAGizmoEventType action, const Vec2d& mouse_position, bool shift_down, bool alt_down, bool control_down)
{
    if (is_dragging() || m_connector_mode == CutConnectorMode::Auto)
        return false;

    if ( (m_hover_id < 0 || m_hover_id == CutPlane) && shift_down &&  ! m_connectors_editing &&
        (action == SLAGizmoEventType::LeftDown || action == SLAGizmoEventType::LeftUp || action == SLAGizmoEventType::Moving) )
        return process_cut_line(action, mouse_position);

    if (!m_keep_upper || !m_keep_lower)
        return false;

    if (!m_connectors_editing) {
        if (0 && action == SLAGizmoEventType::LeftDown) {
            // disable / enable current contour
            Vec3d pos;
            Vec3d pos_world;
            if (unproject_on_cut_plane(mouse_position.cast<double>(), pos, pos_world)) {
                // Following would inform the clipper about the mouse click, so it can
                // toggle the respective contour as disabled.
                m_c->object_clipper()->pass_mouse_click(pos_world);
                return true;
            }
        }
        return false;
    }

    CutConnectors& connectors = m_c->selection_info()->model_object()->cut_connectors;

    if (action == SLAGizmoEventType::LeftDown) {
        if (shift_down || alt_down) {
            // left down with shift - show the selection rectangle:
            if (m_hover_id == -1)
                m_selection_rectangle.start_dragging(mouse_position, shift_down ? GLSelectionRectangle::EState::Select : GLSelectionRectangle::EState::Deselect);
        }
        else
            // If there is no selection and no hovering, add new point
            if (m_hover_id == -1 && !shift_down && !alt_down)
                if (!add_connector(connectors, mouse_position))
                    m_ldown_mouse_position = mouse_position;
        return true;
    }

    if (action == SLAGizmoEventType::LeftUp && !m_selection_rectangle.is_dragging()) {
        if ((m_ldown_mouse_position - mouse_position).norm() < 5.)
            unselect_all_connectors();
        return is_selection_changed(alt_down, shift_down);
    }

    // left up with selection rectangle - select points inside the rectangle:
    if ((action == SLAGizmoEventType::LeftUp || action == SLAGizmoEventType::ShiftUp || action == SLAGizmoEventType::AltUp) && m_selection_rectangle.is_dragging()) {
        // Is this a selection or deselection rectangle?
        process_selection_rectangle(connectors);
        return true;
    }

    // dragging the selection rectangle:
    if (action == SLAGizmoEventType::Dragging) {
        if (m_selection_rectangle.is_dragging()) {
            m_selection_rectangle.dragging(mouse_position);
            return true;
        }
        return false;
    }
    
    if (action == SLAGizmoEventType::RightDown && !shift_down) {
        // If any point is in hover state, this should initiate its move - return control back to GLCanvas:
        if (m_hover_id < m_connectors_group_id)
            return false;
        unselect_all_connectors();
        select_connector(m_hover_id - m_connectors_group_id, true);
        return delete_selected_connectors(connectors);
    }
    
    if (action == SLAGizmoEventType::Delete)
        return delete_selected_connectors(connectors);

    if (action == SLAGizmoEventType::SelectAll) {
        select_all_connectors();
        return true;
    }

    return false;
}

CommonGizmosDataID GLGizmoCut3D::on_get_requirements() const {
    return CommonGizmosDataID(
                int(CommonGizmosDataID::SelectionInfo)
              | int(CommonGizmosDataID::InstancesHider)
              | int(CommonGizmosDataID::ObjectClipper));
}

void GLGizmoCut3D::data_changed()
{
    update_bb();
    if (auto oc = m_c->object_clipper())
        oc->set_behavior(m_connectors_editing, m_connectors_editing, double(m_contour_width));
}


} // namespace GUI
} // namespace Slic3r

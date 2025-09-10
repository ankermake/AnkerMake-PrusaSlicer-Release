#include "GLGizmoSeam.hpp"

#include "libslic3r/Model.hpp"

//#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerSplitCtrl.hpp"
#include "slic3r/GUI/Common/AnkerSliderCtrl.hpp"
#include "slic3r/Utils/UndoRedo.hpp"

#include <wx/valnum.h>
#include <GL/glew.h>

#define BTN_PRESSED "pressed"
#define BTN_NORMAL "normal"


namespace Slic3r::GUI {

bool GLGizmoSeam::on_init()
{
    m_shortcut_key = WXK_CONTROL_P;

    m_desc["clipping_of_view"] = _L("Clipping of view") + ": ";
    m_desc["reset_direction"]  = _L("Reset direction");
    m_desc["cursor_size"]      = _L("Brush size") + ": ";
    m_desc["cursor_type"]      = _L("Brush shape") + ": ";
    m_desc["enforce_caption"]  = _L("Left mouse button") + ": ";
    m_desc["enforce"]          = _L("Enforce seam");
    m_desc["block_caption"]    = _L("Right mouse button") + ": ";
    m_desc["block"]            = _L("Block seam");
    m_desc["remove_caption"]   = _L("Shift + Left mouse button") + ": ";
    m_desc["remove"]           = _L("Remove selection");
    m_desc["remove_all"]       = _L("Remove all selection");
    m_desc["circle"]           = _L("Circle");
    m_desc["sphere"]           = _L("Sphere");

    return true;
}



std::string GLGizmoSeam::on_get_name(bool i18n) const
{
    return i18n ? _u8L("Seam painting") : "Seam painting";
}

void GLGizmoSeam::render_painter_gizmo()
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

void GLGizmoSeam::clearSeam()
{
    Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Reset selection"), UndoRedo::SnapshotType::GizmoAction);
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
}

#if USE_OCRA
void GLGizmoSeam::on_render_input_window(float x, float y, float bottom_limit)
{
    if (!m_c->selection_info()->model_object())
        return;

    //auto pos = get_gizmo_render_position(9);
    //x = pos[0]; y = pos[1];

    //const float approx_height = m_imgui->scaled(12.5f);
    //y = std::min(y, bottom_limit - approx_height);

    // GUI refactor: move gizmo to the right
#if TOOLBAR_ON_TOP
    m_imgui->set_next_window_pos(x, y, ImGuiCond_Always, 0.f, 0.f);
#else
    m_imgui->set_next_window_pos(x, y, ImGuiCond_Always, 1.0f, 0.f);
#endif

    wchar_t old_tool = m_current_tool;
    ImGuiWrapper::push_toolbar_style(m_parent.get_scale());
    m_imgui->begin(get_name(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // First calculate width of all the texts that are could possibly be shown. We will decide set the dialog width based on that:
    const float space_size = m_imgui->get_style_scaling() * 8;
    const float clipping_slider_left = std::max(m_imgui->calc_text_size(m_desc.at("clipping_of_view")).x,
        m_imgui->calc_text_size(m_desc.at("reset_direction")).x + ImGui::GetStyle().FramePadding.x * 2)
        + m_imgui->scaled(1.5f);
    const float cursor_size_slider_left = m_imgui->calc_text_size(m_desc.at("cursor_size")).x + m_imgui->scaled(1.f);
    const float empty_button_width = m_imgui->calc_button_size("").x;

    float caption_max = 0.f;
    float total_text_max = 0.f;
    for (const auto& t : std::array<std::string, 6>{"enforce", "block", "remove", "cursor_size", "clipping_of_view"}) {
        caption_max = std::max(caption_max, m_imgui->calc_text_size(m_desc[t + "_caption"]).x);
        total_text_max = std::max(total_text_max, m_imgui->calc_text_size(m_desc[t]).x);
    }

    const float sliders_left_width = std::max(cursor_size_slider_left, clipping_slider_left);
    const float slider_icon_width = m_imgui->get_slider_icon_size().x;

    const float sliders_width = m_imgui->scaled(7.0f);
    const float drag_left_width = ImGui::GetStyle().WindowPadding.x + sliders_left_width + sliders_width - space_size;

    const float max_tooltip_width = ImGui::GetFontSize() * 20.0f;

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("cursor_type"));
    std::array<wchar_t, 2> tool_ids = { ImGui::CircleButtonIcon, ImGui::SphereButtonIcon };
    std::array<wchar_t, 2> icons =  { ImGui::CircleButtonDarkIcon, ImGui::SphereButtonDarkIcon };

    std::array<wxString, 2> tool_tips = { _L("Circle"), _L("Sphere") };
    for (int i = 0; i < tool_ids.size(); i++) {
        std::string  str_label = std::string("##");
        std::wstring btn_name = icons[i] + boost::nowide::widen(str_label);

        if (i != 0) ImGui::SameLine((empty_button_width + m_imgui->scaled(1.75f)) * i + m_imgui->scaled(1.3f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0);
        if (m_current_tool == tool_ids[i]) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(43 / 255.0f, 64 / 255.0f, 54 / 255.0f, 1.00f) ); // r, g, b, a
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(43 / 255.0f, 64 / 255.0f, 54 / 255.0f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(43 / 255.0f, 64 / 255.0f, 54 / 255.0f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.00f, 0.68f, 0.26f, 1.00f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0);
        }

        bool btn_clicked = ImGui::Button(into_u8(btn_name).c_str());
        if (m_current_tool == tool_ids[i])
        {
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
        }
        ImGui::PopStyleVar(1);
        if (btn_clicked && m_current_tool != tool_ids[i]) {
            m_current_tool = tool_ids[i];
            for (auto& triangle_selector : m_triangle_selectors) {
                triangle_selector->seed_fill_unselect_all_triangles();
                triangle_selector->request_update_render_data();
            }
        }

        if (ImGui::IsItemHovered()) {
            m_imgui->tooltip(tool_tips[i], max_tooltip_width);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.1));

    if (m_current_tool == ImGui::CircleButtonIcon) {
        m_cursor_type = TriangleSelector::CursorType::CIRCLE;
        m_tool_type = ToolType::BRUSH;
    }
    else if (m_current_tool == ImGui::SphereButtonIcon) {
        m_cursor_type = TriangleSelector::CursorType::SPHERE;
        m_tool_type = ToolType::BRUSH;
    }

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("cursor_size"));
    ImGui::SameLine(sliders_left_width);

    ImGui::PushItemWidth(sliders_width);
    m_imgui->bbl_slider_float_style("##cursor_radius", &m_cursor_radius, CursorRadiusMin, CursorRadiusMax, "%.2f", 1.0f, true);
    ImGui::SameLine(drag_left_width);
    ImGui::PushItemWidth(1.5 * slider_icon_width);
    ImGui::BBLDragFloat("##cursor_radius_input", &m_cursor_radius, 0.05f, 0.0f, 0.0f, "%.2f");

    ImGui::Separator();
    if (m_c->object_clipper()->get_position() == 0.f) {
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc.at("clipping_of_view"));
    }
    else {
        if (m_imgui->button(m_desc.at("reset_direction"))) {
            wxGetApp().CallAfter([this]() {
                m_c->object_clipper()->set_position_by_ratio(-1., false);
                });
}
    }

    auto clp_dist = float(m_c->object_clipper()->get_position());
    ImGui::SameLine(sliders_left_width);

    ImGui::PushItemWidth(sliders_width);
    bool slider_clp_dist = m_imgui->bbl_slider_float_style("##clp_dist", &clp_dist, 0.f, 1.f, "%.2f", 1.0f, true);

    ImGui::SameLine(drag_left_width);
    ImGui::PushItemWidth(1.5 * slider_icon_width);
    bool b_clp_dist_input = ImGui::BBLDragFloat("##clp_dist_input", &clp_dist, 0.05f, 0.0f, 0.0f, "%.2f");
    if (slider_clp_dist || b_clp_dist_input) { m_c->object_clipper()->set_position_by_ratio(clp_dist, true); }

    ImGui::Separator();

    bool enable = m_enable_seam;
    m_imgui->bbl_checkbox(_L("Block seam"), enable);
    if (enable != m_enable_seam) { this->update_enforcerblocker_type(enable); }

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 10.0f));
    float get_cur_y = ImGui::GetContentRegionMax().y + ImGui::GetFrameHeight() + y;
    //show_tooltip_information(caption_max, x, get_cur_y);

    float f_scale = 1.0f /*m_parent.get_gizmos_manager().get_layout_scale()*/;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f * f_scale));

    //ImGui::SameLine();
    if (m_imgui->button(m_desc.at("remove_all"))) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Reset selection"), UndoRedo::SnapshotType::GizmoAction);
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
    }
    ImGui::PopStyleVar(2);
    m_imgui->end();

    ImGuiWrapper::pop_toolbar_style();
}
#else
void GLGizmoSeam::on_render_input_window(float x, float y, float bottom_limit)
{
    if (! m_c->selection_info()->model_object())
        return;

    const float approx_height = m_imgui->scaled(12.5f);
    y = std::min(y, bottom_limit - approx_height);
    m_imgui->set_next_window_pos(x, y, ImGuiCond_Always);
    m_imgui->begin(get_name(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    // First calculate width of all the texts that are could possibly be shown. We will decide set the dialog width based on that:
    const float clipping_slider_left = std::max(m_imgui->calc_text_size(m_desc.at("clipping_of_view")).x,
                                                m_imgui->calc_text_size(m_desc.at("reset_direction")).x)
                                           + m_imgui->scaled(1.5f);
    const float cursor_size_slider_left = m_imgui->calc_text_size(m_desc.at("cursor_size")).x + m_imgui->scaled(1.f);

    const float cursor_type_radio_left   = m_imgui->calc_text_size(m_desc["cursor_type"]).x + m_imgui->scaled(1.f);
    const float cursor_type_radio_sphere = m_imgui->calc_text_size(m_desc["sphere"]).x + m_imgui->scaled(2.5f);
    const float cursor_type_radio_circle = m_imgui->calc_text_size(m_desc["circle"]).x + m_imgui->scaled(2.5f);

    const float button_width = m_imgui->calc_text_size(m_desc.at("remove_all")).x + m_imgui->scaled(1.f);
    const float minimal_slider_width = m_imgui->scaled(4.f);

    float caption_max    = 0.f;
    float total_text_max = 0.f;
    for (const auto &t : std::array<std::string, 3>{"enforce", "block", "remove"}) {
        caption_max    = std::max(caption_max, m_imgui->calc_text_size(m_desc[t + "_caption"]).x);
        total_text_max = std::max(total_text_max, m_imgui->calc_text_size(m_desc[t]).x);
    }
    total_text_max += caption_max + m_imgui->scaled(1.f);
    caption_max    += m_imgui->scaled(1.f);

    const float sliders_left_width = std::max(cursor_size_slider_left, clipping_slider_left);
    const float slider_icon_width  = m_imgui->get_slider_icon_size().x;
    float       window_width       = minimal_slider_width + sliders_left_width + slider_icon_width;
    window_width = std::max(window_width, total_text_max);
    window_width = std::max(window_width, button_width);
    window_width = std::max(window_width, cursor_type_radio_left + cursor_type_radio_sphere + cursor_type_radio_circle);

    auto draw_text_with_caption = [this, &caption_max](const wxString& caption, const wxString& text) {
        m_imgui->text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, caption);
        ImGui::SameLine(caption_max);
        m_imgui->text(text);
    };

    for (const auto &t : std::array<std::string, 3>{"enforce", "block", "remove"})
        draw_text_with_caption(m_desc.at(t + "_caption"), m_desc.at(t));

    ImGui::Separator();

    const float max_tooltip_width = ImGui::GetFontSize() * 20.0f;

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("cursor_size"));
    ImGui::SameLine(sliders_left_width);
    ImGui::PushItemWidth(window_width - sliders_left_width - slider_icon_width);
    m_imgui->slider_float("##cursor_radius", &m_cursor_radius, CursorRadiusMin, CursorRadiusMax, "%.2f", 1.0f, true, _L("Alt + Mouse wheel"));

    ImGui::AlignTextToFramePadding();
    m_imgui->text(m_desc.at("cursor_type"));

    float cursor_type_offset = cursor_type_radio_left + (window_width - cursor_type_radio_left - cursor_type_radio_sphere - cursor_type_radio_circle + m_imgui->scaled(0.5f)) / 2.f;
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

    ImGui::Separator();
    if (m_c->object_clipper()->get_position() == 0.f) {
        ImGui::AlignTextToFramePadding();
        m_imgui->text(m_desc.at("clipping_of_view"));
    }
    else {
        if (m_imgui->button(m_desc.at("reset_direction"))) {
            wxGetApp().CallAfter([this](){
                    m_c->object_clipper()->set_position_by_ratio(-1., false);
                });
        }
    }

    auto clp_dist = float(m_c->object_clipper()->get_position());
    ImGui::SameLine(sliders_left_width);
    ImGui::PushItemWidth(window_width - sliders_left_width - slider_icon_width);
    if (m_imgui->slider_float("##clp_dist", &clp_dist, 0.f, 1.f, "%.2f", 1.0f, true, _L("Ctrl + Mouse wheel")))
        m_c->object_clipper()->set_position_by_ratio(clp_dist, true);

    ImGui::Separator();
    if (m_imgui->button(m_desc.at("remove_all"))) {
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Reset selection"), UndoRedo::SnapshotType::GizmoAction);
        ModelObject         *mo  = m_c->selection_info()->model_object();
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
#endif

void GLGizmoSeam::update_model_object() const
{
    bool updated = false;
    ModelObject* mo = m_c->selection_info()->model_object();
    int idx = -1;
    for (ModelVolume* mv : mo->volumes) {
        if (! mv->is_model_part())
            continue;
        ++idx;
        updated |= mv->seam_facets.set(*m_triangle_selectors[idx].get());
    }

    if (updated) {
        const ModelObjectPtrs& mos = wxGetApp().model().objects;
        wxGetApp().obj_list()->update_info_items(std::find(mos.begin(), mos.end(), mo) - mos.begin());
        m_parent.post_event(SimpleEvent(EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS));
    }
}



void GLGizmoSeam::update_from_model_object()
{
    wxBusyCursor wait;

    const ModelObject* mo = m_c->selection_info()->model_object();
    m_triangle_selectors.clear();

    int volume_id = -1;
    for (const ModelVolume* mv : mo->volumes) {
        if (! mv->is_model_part())
            continue;

        ++volume_id;

        // This mesh does not account for the possible Z up SLA offset.
        const TriangleMesh* mesh = &mv->mesh();

        m_triangle_selectors.emplace_back(std::make_unique<TriangleSelectorGUI>(*mesh));
        // Reset of TriangleSelector is done inside TriangleSelectorGUI's constructor, so we don't need it to perform it again in deserialize().
        m_triangle_selectors.back()->deserialize(mv->seam_facets.get_data(), false);
        m_triangle_selectors.back()->request_update_render_data();
    }
}


PainterGizmoType GLGizmoSeam::get_painter_type() const
{
    return PainterGizmoType::SEAM;
}

wxString GLGizmoSeam::handle_snapshot_action_name(bool shift_down, GLGizmoPainterBase::Button button_down) const
{
    wxString action_name;
    if (shift_down)
        action_name = _L("Remove selection");
    else {
        if (button_down == Button::Left)
            action_name = _L("Enforce seam");
        else
            action_name = _L("Block seam");
    }
    return action_name;
}


void GLGizmoSeam::on_opening()
{
   // set_input_window_state(true);
}

void GLGizmoSeam::on_shutdown()
{
    m_parent.toggle_model_objects_visibility(true);

   // set_input_window_state(false);
}

void GLGizmoSeam::update_enforcerblocker_type(bool new_value)
{
    if (new_value) {
        m_currentType = EnforcerBlockerType::BLOCKER;
    }
    else {
        m_currentType = EnforcerBlockerType::ENFORCER;
    }

    m_enable_seam = new_value;
}

void GLGizmoSeam::set_input_window_state(bool on)
{
    if (wxGetApp().plater() == nullptr)
        return;

    ANKER_LOG_INFO << "GLGizmoSeam: " << on;

    std::string panelFlag = "GLGizmoSeam";

    if (on)
    {
        wxGetApp().plater()->sidebarnew().setMainSizer();
        
		if (m_pInputWindowSizer == nullptr)
		{
			double brushInitSize = 2.0;
			double brushSizeMin = 0.4;
			double brushSizeMax = 8.0;
			double sliderMultiple = 10;
			m_cursor_radius = brushInitSize;
			m_pInputWindowSizer = new wxBoxSizer(wxVERTICAL);

			AnkerTitledPanel* container = new AnkerTitledPanel(&(wxGetApp().plater()->sidebarnew()), 32, 0, wxColour(PANEL_TITLE_BACK_DARK_RGB_INT));
			container->setTitle(/*wxString::FromUTF8(get_name(true, false))*/_("common_slice_toolpannel_seampaint"));
			container->setTitleAlign(AnkerTitledPanel::TitleAlign::LEFT);
			int returnBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("return.png")), true);
			int clearBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("reset.png")), false);
            m_pInputWindowSizer->Add(container, 1, wxEXPAND, 0);

			wxPanel* zeamPanel = new wxPanel(container);
            zeamPanel->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			wxBoxSizer* zeamPanelSizer = new wxBoxSizer(wxVERTICAL);
			zeamPanel->SetSizer(zeamPanelSizer);
			container->setContentPanel(zeamPanel);

			wxBoxSizer* modeSizer = new wxBoxSizer(wxHORIZONTAL);
			zeamPanelSizer->Add(modeSizer, 0, wxALIGN_TOP | wxLEFT | wxRIGHT, 20);

			wxImage enforcerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer.png")), wxBITMAP_TYPE_PNG);
			enforcerImage.Rescale(46, 46);
			wxImage enforcerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer_pressed.png")), wxBITMAP_TYPE_PNG);
			enforcerImagePressed.Rescale(46, 46);
			wxButton* enforcerButton = new wxButton(zeamPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			enforcerButton->SetBitmap(enforcerImagePressed);
			enforcerButton->SetBitmapHover(enforcerImagePressed);
			enforcerButton->SetMinSize(enforcerImage.GetSize());
			enforcerButton->SetMaxSize(enforcerImage.GetSize());
			enforcerButton->SetSize(enforcerImage.GetSize());
			enforcerButton->SetName(BTN_PRESSED);
			enforcerButton->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			modeSizer->Add(enforcerButton, 0, wxALIGN_LEFT | wxTOP, 16);

			wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker.png")), wxBITMAP_TYPE_PNG);
			blockerImage.Rescale(46, 46);
			wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker_pressed.png")), wxBITMAP_TYPE_PNG);
			blockerImagePressed.Rescale(46, 46);
			wxButton* blockerButton = new wxButton(zeamPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			blockerButton->SetBitmap(blockerImage);
			blockerButton->SetBitmapHover(blockerImagePressed);
			blockerButton->SetMinSize(blockerImage.GetSize());
			blockerButton->SetMaxSize(blockerImage.GetSize());
			blockerButton->SetSize(blockerImage.GetSize());
			blockerButton->SetName(BTN_NORMAL);
			blockerButton->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			modeSizer->Add(blockerButton, 0, wxALIGN_LEFT | wxTOP | wxLEFT, 16);

			modeSizer->AddStretchSpacer(1);


			AnkerSplitCtrl* splitCtrl = new AnkerSplitCtrl(zeamPanel);
			zeamPanelSizer->Add(splitCtrl, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 20);


			wxStaticText* brushTitleText = new wxStaticText(zeamPanel, wxID_ANY, /*L"Brush Thickness"*/_("common_slice_toolpannelseam_title1"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            brushTitleText->SetFont(ANKER_FONT_NO_1);
			brushTitleText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			brushTitleText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
			zeamPanelSizer->Add(brushTitleText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);


			wxBoxSizer* valueHSizer = new wxBoxSizer(wxHORIZONTAL);
			zeamPanelSizer->Add(valueHSizer, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT, 18);

			//wxSlider* valueSlider = new wxSlider(zeamPanel, wxID_ANY, brushInitSize * sliderMultiple, brushSizeMin * sliderMultiple, brushSizeMax * sliderMultiple, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			//valueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			//valueSlider->SetForegroundColour(wxColour(ANKER_RGB_INT));
			
            AnkerSlider* valueSlider = new AnkerSlider(zeamPanel);
            valueSlider->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
            valueSlider->SetMinSize(AnkerSize(40, 40));
            valueSlider->SetMaxSize(AnkerSize(1000, 40));
            valueSlider->setRange(brushSizeMin, brushSizeMax, 0.01);
            valueSlider->setCurrentValue(brushInitSize);
            valueSlider->setTooltipVisible(false);
            
            valueHSizer->Add(valueSlider, 1, wxEXPAND | wxALIGN_CENTER | wxALIGN_LEFT, 0);

            valueHSizer->AddSpacer(6);

            wxFloatingPointValidator<double> doubleValidator;
            doubleValidator.SetMin(0.0);
            doubleValidator.SetMax(brushSizeMax);
            doubleValidator.SetPrecision(2);
			char text[5];
			sprintf(text, "%.2f", brushInitSize); 
			wxTextCtrl* valueTextCtrl = new wxTextCtrl(zeamPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
			valueTextCtrl->SetMinSize(AnkerSize(40, 23));
			valueTextCtrl->SetMaxSize(AnkerSize(40, 23));
			valueTextCtrl->SetSize(AnkerSize(40, 23));
            valueTextCtrl->SetFont(ANKER_FONT_NO_1);
			valueTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
			valueTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
            //valueTextCtrl->SetValidator(doubleValidator);

            valueHSizer->Add(valueTextCtrl, 0, wxALIGN_CENTER | wxALIGN_RIGHT, 0);

			zeamPanelSizer->AddStretchSpacer(1);


			container->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, returnBtnID, clearBtnID](wxCommandEvent& event) {
				int btnID = event.GetInt();
				if (btnID == returnBtnID)
				{
					wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(get_name(false, false)));
				}
				else if (btnID == clearBtnID)
				{
                    clearSeam();
				}
				});

			enforcerButton->Bind(wxEVT_BUTTON, [this, enforcerButton, blockerButton, container](wxCommandEvent& event) {
                if (enforcerButton->GetName() == BTN_PRESSED)
                    return;

				m_currentType = EnforcerBlockerType::ENFORCER;

				wxImage enforcerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer.png")), wxBITMAP_TYPE_PNG);
				enforcerImage.Rescale(46, 46);
				wxImage enforcerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer_pressed.png")), wxBITMAP_TYPE_PNG);
				enforcerImagePressed.Rescale(46, 46);
				enforcerButton->SetBitmap(enforcerButton->GetName() == BTN_PRESSED ? enforcerImage : enforcerImagePressed);
				enforcerButton->SetBitmapHover(enforcerImagePressed);
				enforcerButton->SetName(enforcerButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker.png")), wxBITMAP_TYPE_PNG);
				blockerImage.Rescale(46, 46);
				wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker_pressed.png")), wxBITMAP_TYPE_PNG);
				blockerImagePressed.Rescale(46, 46);
				blockerButton->SetBitmap(enforcerButton->GetName() == BTN_PRESSED ? blockerImage : blockerImagePressed);
				blockerButton->SetBitmapHover(blockerImagePressed);
				blockerButton->SetName(enforcerButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				container->Refresh();
				});
            enforcerButton->Bind(wxEVT_ENTER_WINDOW, [enforcerButton](wxMouseEvent& event) {enforcerButton->SetCursor(wxCursor(wxCURSOR_HAND)); });
            enforcerButton->Bind(wxEVT_LEAVE_WINDOW, [enforcerButton](wxMouseEvent& event) {enforcerButton->SetCursor(wxCursor(wxCURSOR_NONE)); });

			blockerButton->Bind(wxEVT_BUTTON, [this, enforcerButton, blockerButton, container](wxCommandEvent& event) {
                if (blockerButton->GetName() == BTN_PRESSED)
                    return;
                
                m_currentType = EnforcerBlockerType::BLOCKER;

				wxImage enforcerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer.png")), wxBITMAP_TYPE_PNG);
				enforcerImage.Rescale(46, 46);
				wxImage enforcerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_enforcer_pressed.png")), wxBITMAP_TYPE_PNG);
				enforcerImagePressed.Rescale(46, 46);
				enforcerButton->SetBitmap(enforcerButton->GetName() == BTN_PRESSED ? enforcerImage : enforcerImagePressed);
				enforcerButton->SetBitmapHover(enforcerImagePressed);
				enforcerButton->SetName(enforcerButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				wxImage blockerImage = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker.png")), wxBITMAP_TYPE_PNG);
				blockerImage.Rescale(46, 46);
				wxImage blockerImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("zeam_blocker_pressed.png")), wxBITMAP_TYPE_PNG);
				blockerImagePressed.Rescale(46, 46);
				blockerButton->SetBitmap(enforcerButton->GetName() == BTN_PRESSED ? blockerImage : blockerImagePressed);
				blockerButton->SetBitmapHover(blockerImagePressed);
				blockerButton->SetName(enforcerButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

				container->Refresh();
				});
            blockerButton->Bind(wxEVT_ENTER_WINDOW, [blockerButton](wxMouseEvent& event) {blockerButton->SetCursor(wxCursor(wxCURSOR_HAND)); });
            blockerButton->Bind(wxEVT_LEAVE_WINDOW, [blockerButton](wxMouseEvent& event) {blockerButton->SetCursor(wxCursor(wxCURSOR_NONE)); });

			valueSlider->Bind(wxANKEREVT_SLIDER_VALUE_CHANGED, [this, valueTextCtrl, valueSlider, container](wxCommandEvent& event) {
                if (m_editFlag)
                    return;

                m_editFlag = true;
                
                double currentValue = valueSlider->getCurrentValue();
				//double value = currentValue * 1.0 / 10.0;

				m_cursor_radius = currentValue;

				char text[5];
				sprintf(text, "%.2f", currentValue);
				valueTextCtrl->SetValue(text);

				//container->Refresh();

                m_editFlag = false;
				});
            valueTextCtrl->Bind(/*wxEVT_TEXT*/wxEVT_TEXT_ENTER, [this, valueSlider, valueTextCtrl, container, brushSizeMin, brushSizeMax](wxCommandEvent& event) {
                if (m_editFlag)
                    return;

                m_editFlag = true;
                
                double newValue = 10.0;
				wxString newValueStr = valueTextCtrl->GetLineText(0);
				bool success = newValueStr.ToDouble(&newValue);
				if (success && newValue >= brushSizeMin && newValue <= brushSizeMax)
				{
					m_cursor_radius = newValue;
					valueSlider->setCurrentValue(newValue);
					
				}
                else
                {
                    double currentValue = valueSlider->getCurrentValue();

                    m_cursor_radius = currentValue;

                    char text[5];
                    sprintf(text, "%.2f", currentValue);
                    valueTextCtrl->SetValue(text);
                }

                container->Refresh();

                m_editFlag = false;
				});
            valueTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, valueSlider, valueTextCtrl, container, brushSizeMin, brushSizeMax](wxFocusEvent& event) {
                if (m_editFlag)
                    return;

                m_editFlag = true;

                double newValue = 10.0;
                wxString newValueStr = valueTextCtrl->GetLineText(0);
                bool success = newValueStr.ToDouble(&newValue);
                if (success && newValue >= brushSizeMin && newValue <= brushSizeMax)
                {
                    m_cursor_radius = newValue;
                    valueSlider->setCurrentValue(newValue);

                }
                else
                {
                    double currentValue = valueSlider->getCurrentValue();

                    m_cursor_radius = currentValue;

                    char text[5];
                    sprintf(text, "%.2f", currentValue);
                    valueTextCtrl->SetValue(text);
                }

                container->Refresh();

                m_editFlag = false;

                event.Skip();
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

        wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer(m_pInputWindowSizer, panelFlag);
        m_panelVisibleFlag = true;
    }
    else
    {
    m_panelVisibleFlag = false;
    if (wxGetApp().plater()->sidebarnew().getSizerFlags() == panelFlag)
    {
        wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer();
    }
    }
}



} // namespace Slic3r::GUI

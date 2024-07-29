#include "SlicingProgressNotification.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r { namespace GUI {

namespace {
	inline void push_style_color(ImGuiCol idx, const ImVec4& col, bool fading_out, float current_fade_opacity)
	{
		if (fading_out)
			ImGui::PushStyleColor(idx, ImVec4(col.x, col.y, col.z, col.w * current_fade_opacity));
		else
			ImGui::PushStyleColor(idx, col);
	}
}

void NotificationManager::SlicingProgressNotification::on_change_color_mode(bool is_dark)
{
	PopNotification::on_change_color_mode(is_dark);
	m_dailytips_panel->on_change_color_mode(is_dark);
}

void NotificationManager::SlicingProgressNotification::init()
{
	if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
		PopNotification::init();
		if (m_endlines.empty()) {
			m_endlines.push_back(0);
		}
		if (m_lines_count >= 2) {
			m_lines_count = 3;
			m_multiline = true;
			while (m_endlines.size() < 3)
				m_endlines.push_back(m_endlines.back());
		}
		else {
			m_lines_count = 2;
			m_endlines.push_back(m_endlines.back());
			m_multiline = false;
		}
		if (m_state == EState::Shown)
			m_state = EState::NotFading;
	}
	else {
		PopNotification::init();
	}

}

bool NotificationManager::SlicingProgressNotification::set_progress_state(float percent)
{
	if (percent < 0.f)
		return true;//set_progress_state(SlicingProgressState::SP_CANCELLED);
	else if (percent >= 1.f) {
			m_before_complete_start = GLCanvas3D::timestamp_now();
			return set_progress_state(SlicingProgressState::SP_COMPLETED);
	}
	else
		return set_progress_state(SlicingProgressState::SP_PROGRESS, percent);
}

bool NotificationManager::SlicingProgressNotification::set_progress_state(NotificationManager::SlicingProgressNotification::SlicingProgressState state, float percent/* = 0.f*/)
{
	switch (state)
	{
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_NO_SLICING:
        m_state = EState::Hidden;
        set_percentage(-1);
        m_has_print_info = false;
        set_export_possible(false);
        m_sp_state             = state;
        return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_BEGAN:
		m_state = EState::Hidden;
		set_percentage(-1);
		m_has_print_info = false;
		set_export_possible(false);
		m_sp_state = state;
        m_current_fade_opacity = 1;
		m_dailytips_panel->retrieve_data_from_hint_database(HintDataNavigation::Random);
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_PROGRESS:
		if ((m_sp_state != SlicingProgressState::SP_BEGAN && m_sp_state != SlicingProgressState::SP_PROGRESS) || percent < m_percentage)
			return false;
		set_percentage(percent);
		m_sp_state = state;
        m_current_fade_opacity = 1;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_CANCELLED:
		set_percentage(-1);
		m_has_print_info = false;
		set_export_possible(false);
		m_sp_state = state;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_COMPLETED:
		if (m_sp_state != SlicingProgressState::SP_BEGAN && m_sp_state != SlicingProgressState::SP_PROGRESS)
			return false;
		set_percentage(1);
		m_has_print_info = false;
		// m_export_possible is important only for SP_PROGRESS state, thus we can reset it here
		set_export_possible(false);
		m_sp_state = state;
		return true;
	default:
		break;
	}
	return false;
}

void NotificationManager::SlicingProgressNotification::set_status_text(const std::string& text)
{
	switch (m_sp_state)
	{
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_NO_SLICING:
		m_state = EState::Hidden;
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_PROGRESS:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0, text};
		update(data);
		m_state = EState::NotFading;
	}
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_CANCELLED:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0, text };
		//NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0, _u8L("common_slicepopup_slicingcancel") };
		update(data);
		m_state = EState::Shown;
	}
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_COMPLETED:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0,  _u8L("Slicing finished.") };
		update(data);
		m_state = EState::Shown;
	}
		break;
	default:
		break;
	}
}

void NotificationManager::SlicingProgressNotification::set_print_info(const std::string& info)
{
	if (m_sp_state != SlicingProgressState::SP_COMPLETED) {
		set_progress_state (SlicingProgressState::SP_COMPLETED);
	} else {
		m_has_print_info = true;
		m_print_info = info;
	}
}

void NotificationManager::SlicingProgressNotification::set_sidebar_collapsed(bool collapsed)
{
	m_sidebar_collapsed = collapsed;
	if (m_sp_state == SlicingProgressState::SP_COMPLETED && collapsed)
		m_state = EState::NotFading;
}

void NotificationManager::SlicingProgressNotification::on_cancel_button()
{
	if (m_cancel_callback){
		if (!m_cancel_callback()) {
			set_progress_state(SlicingProgressState::SP_NO_SLICING);
		}
	}
}

int NotificationManager::SlicingProgressNotification::get_duration()
{
	if (m_sp_state == SlicingProgressState::SP_CANCELLED)
		return 3;
	else if (m_sp_state == SlicingProgressState::SP_COMPLETED)
		return 3;
	else
		return 0;
}

bool  NotificationManager::SlicingProgressNotification::update_state(bool paused, const int64_t delta)
{
	bool ret = PopNotification::update_state(paused, delta);
	if (m_sp_state == SlicingProgressState::SP_COMPLETED)
		ret = true;

	// sets Estate to hidden
	if (get_state() == PopNotification::EState::ClosePending || get_state() == PopNotification::EState::Finished)
		set_progress_state(SlicingProgressState::SP_NO_SLICING);
	return ret;
}

void NotificationManager::SlicingProgressNotification::render(GLCanvas3D& canvas, float initial_y, bool move_from_overlay, float overlay_width/*, float right_margin*/)
{
	if (m_state == EState::Unknown || m_state == PopNotification::EState::Hovered)
		init();

	ImGuiWrapper& imgui = *wxGetApp().imgui();
	float scale = imgui.get_font_size() / 17.0f;

	bool fading_pop = false;
	if (m_state == EState::FadingOut) {
		push_style_color(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg), true, m_current_fade_opacity);
		push_style_color(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text), true, m_current_fade_opacity);
		push_style_color(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered), true, m_current_fade_opacity);
		fading_pop = true;
	}
	set_theme();
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * scale, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	push_style_color(ImGuiCol_Border, { 0, 0, 0, 0 }, true, m_current_fade_opacity);

	Size cnv_size = canvas.get_canvas_size();

	const ImVec2 progress_child_window_padding = ImVec2(15.f, 0.f) * scale;
	const ImVec2 dailytips_child_window_padding = m_dailytips_panel->is_expanded() ? ImVec2(15.f, 0.f) * scale : ImVec2(15.f, 0.f) * scale;
	const ImVec2 bottom_padding = ImVec2(0.f, 0.f) * scale;
	const float  progress_panel_width = (m_window_width - 2 * progress_child_window_padding.x);
	const float  progress_panel_height = (50.0f * scale);
	const float  dailytips_panel_width = (m_window_width - 2 * dailytips_child_window_padding.x);
//	const float  gcodeviewer_height = wxGetApp().plater()->get_preview_canvas3D()->get_gcode_viewer().get_legend_height();
	//const float  dailytips_panel_height = std::min(380.0f * scale, std::max(90.0f, (cnv_size.get_height() - gcodeviewer_height - progress_panel_height - dailytips_child_window_padding.y - initial_y - m_line_height * 4)));
	const float  dailytips_panel_height = 400.0f * scale;

	int right_margin = 10;
	float right_gap = right_margin + (move_from_overlay ? overlay_width + m_line_height * 5 : 0);
	m_window_pos = ImVec2((float)cnv_size.get_width() - right_gap - m_window_width, (float)cnv_size.get_height() - m_top_y); 
	imgui.set_next_window_pos(m_window_pos.x, m_window_pos.y, ImGuiCond_Always, 0.0f, 0.0f);
	m_window_height = progress_panel_height + m_dailytips_panel->get_size().y + progress_child_window_padding.y + dailytips_child_window_padding.y + bottom_padding.y;
	m_top_y = initial_y + m_window_height;
	ImGui::SetNextWindowSizeConstraints(ImVec2(m_window_width, m_window_height), ImVec2(m_window_width, m_window_height));

	// name of window indentifies window - has to be unique string
	if (m_id == 0)
		m_id = m_id_provider.allocate_id();
	std::string name = "!!Ntfctn" + std::to_string(m_id);
	int window_flags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;
	int child_window_flags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;
	if (imgui.begin(name, window_flags)) {
		ImGuiWindow* parent_window = ImGui::GetCurrentWindow();


		if (m_sp_state == SlicingProgressState::SP_CANCELLED || m_sp_state == SlicingProgressState::SP_PROGRESS || m_sp_state == SlicingProgressState::SP_COMPLETED) {
			std::string child_name = "##SlicingProgressPanel" + std::to_string(parent_window->ID);

			// render status text & progress bar
			ImGui::SetNextWindowPos(parent_window->Pos + progress_child_window_padding);
			if (ImGui::BeginChild(child_name.c_str(), ImVec2(progress_panel_width, progress_panel_height), false, child_window_flags)) {
				float  margin_x = 9.5f * scale;
#ifdef __APPLE__
                float  margin_y = 20;
#else
                float  margin_y = 10;
#endif
				ImVec2 child_window_pos = ImGui::GetWindowPos();
				ImVec2 button_size = ImVec2(30.f, 30.f) * scale;
				ImVec2 button_pos = child_window_pos + ImVec2(progress_panel_width - button_size.x - margin_x, (progress_panel_height - button_size.y - margin_y) / 2.0f);
				ImVec2 progress_bar_pos = child_window_pos + ImVec2(0, progress_panel_height / 2.0f);
				ImVec2 progress_bar_size = ImVec2(progress_panel_width - button_size.x - margin_x, 4.0f * scale);

				if (m_sp_state == SlicingProgressState::SP_CANCELLED || m_sp_state == SlicingProgressState::SP_COMPLETED) {
					ImVec2 text_pos = ImVec2(progress_bar_pos.x, child_window_pos.y + (progress_panel_height - m_line_height) / 2);
					render_progress_panel_text(text_pos);
					render_close_button(button_pos, button_size);
				}
				else if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
					ImVec2 text_pos = ImVec2(progress_bar_pos.x, progress_bar_pos.y - m_line_height * 1.2f);
					render_progress_panel_text(text_pos);
					render_bar(progress_bar_pos, progress_bar_size.x);
					render_cancel_button(button_pos, button_size);
				}
			}
			ImGui::EndChild();

			// render tips
			int hint_cnt = m_dailytips_panel->get_hint_cnt();
			if (hint_cnt > 0) {
				// Separator Line
				ImVec2 separator_min = ImVec2(ImGui::GetCursorScreenPos().x + progress_child_window_padding.x, ImGui::GetCursorScreenPos().y);
				ImVec2 separator_max = ImVec2(ImGui::GetCursorScreenPos().x + progress_child_window_padding.x + progress_panel_width, ImGui::GetCursorScreenPos().y);
				ImGui::GetCurrentWindow()->DrawList->AddLine(separator_min, separator_max, ImColor(63, 65, 68, (int)(255 * m_current_fade_opacity)));

				child_name = "##DailyTipsPanel" + std::to_string(parent_window->ID);
				ImVec2 dailytips_pos = ImGui::GetCursorScreenPos() + dailytips_child_window_padding;
				ImVec2 dailytips_size = ImVec2(dailytips_panel_width, dailytips_panel_height);
				m_dailytips_panel->set_position(dailytips_pos);
				m_dailytips_panel->set_size(dailytips_size);
				m_dailytips_panel->set_fade_opacity(m_current_fade_opacity);
				ImGui::SetNextWindowPos(dailytips_pos);
				if (ImGui::BeginChild(child_name.c_str(), ImVec2(dailytips_panel_width, dailytips_panel_height), false, child_window_flags)) {
					render_dailytips_panel(dailytips_pos, dailytips_size);
				}
				ImGui::EndChild();
			}
		}

		if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), true)) {
			set_hovered();
		}
	}
	imgui.end();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);
	restore_default_theme();
	if (fading_pop)
		ImGui::PopStyleColor(3);
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_progress_panel_text(const ImVec2& pos)
{
	ImGuiWrapper& imgui = *wxGetApp().imgui();
	float scale = imgui.get_font_size() / 15.0f;
	ImVec2 icon_size = ImVec2(30.f, 30.f) * scale;

	imgui.push_bold_font();
	ImGui::SetCursorScreenPos(pos);
	imgui.text(m_text1.substr(0, m_endlines[0]).c_str());
	imgui.pop_bold_font();
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_bar(const ImVec2& pos, const float& total_width)
{
	if (m_sp_state != SlicingProgressState::SP_PROGRESS)
		return;

	ImGuiWrapper& imgui = *wxGetApp().imgui();

	// percentage text
	ImVec2 text_size = ImGui::CalcTextSize("100%");
	ImVec2 text_pos = ImVec2(pos.x + total_width - text_size.x, pos.y);
	float text_center_y = pos.y + text_size.y / 2.0f;
	std::string text;
	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << (int)(m_percentage * 100) << "%";
	text = stream.str();
	ImGui::SetCursorScreenPos(text_pos);
	imgui.push_bold_font();
	imgui.text(text.c_str());
	imgui.pop_bold_font();
	
	// progress bar
	ImColor progress_color = ImColor(0, 174, 66, (int)(255 * m_current_fade_opacity));
	ImColor bg_color = ImColor(217, 217, 217, (int)(255 * m_current_fade_opacity));
	float bar_height = 4.0f;
	ImVec2 lineStart = ImVec2(pos.x, text_center_y - bar_height/2.0f);
	ImVec2 lineEnd = ImVec2(text_pos.x - 5, text_center_y + bar_height / 2.0f);
	ImVec2 midPoint = ImVec2(lineStart.x + (lineEnd.x - lineStart.x) * m_percentage, lineEnd.y);
	ImGui::SetCursorScreenPos(lineStart);
	ImGui::GetWindowDrawList()->AddRectFilled(lineStart, lineEnd, bg_color);
	ImGui::GetWindowDrawList()->AddRectFilled(lineStart, midPoint, progress_color);
}

void NotificationManager::SlicingProgressNotification::render_dailytips_panel(const ImVec2& pos, const ImVec2& size)
{
	m_dailytips_panel->render();
}

void NotificationManager::SlicingProgressNotification::render_show_dailytips(const ImVec2& pos)
{
	if (m_sp_state != SlicingProgressState::SP_COMPLETED && m_sp_state != SlicingProgressState::SP_CANCELLED)
		return;

	ImGuiWrapper& imgui = *wxGetApp().imgui();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(31, 142, 234).Value);

	ImGui::SetCursorScreenPos(pos);
	std::wstring button_text;
	button_text = ImGui::OpenArrowIcon;
	imgui.button(_L("View all Daily tips") + " " + button_text);
	//click behavior
	if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true))
	{
		//underline
		ImVec2 lineEnd = ImGui::GetItemRectMax();
		lineEnd.x -= ImGui::CalcTextSize("A").x / 2;
		lineEnd.y -= 2;
		ImVec2 lineStart = lineEnd;
		lineStart.x = ImGui::GetItemRectMin().x;
		ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, ImColor(31, 142, 234));

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			on_show_dailytips();
	}

	ImGui::PopStyleColor(4);
}

void NotificationManager::SlicingProgressNotification::on_show_dailytips()
{
	//wxGetApp().plater()->get_dailytips()->open();
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_cancel_button(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
		ImGuiWrapper& imgui = *wxGetApp().imgui();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
		push_style_color(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));

		ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.f);
		ImVec2 button_size = size;
		ImVec2 button_pos = pos;
		ImGui::SetCursorScreenPos(button_pos);

		std::wstring button_text;
		button_text = ImGui::CancelButton;
		if (ImGui::IsMouseHoveringRect(button_pos, button_pos + button_size, true))
		{
			text_color = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
			//button_text = ImGui::CancelHoverButton;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				on_cancel_button();
		}

		ImGui::PushStyleColor(ImGuiCol_Text, text_color);
		imgui.button(button_text.c_str());
		ImGui::PopStyleColor();


		ImGui::PopStyleColor(4);
	}
}

void NotificationManager::SlicingProgressNotification::render_close_button(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state == SlicingProgressState::SP_CANCELLED || m_sp_state == SlicingProgressState::SP_COMPLETED) {
		ImGuiWrapper& imgui = *wxGetApp().imgui();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
		push_style_color(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		push_style_color(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));

		ImVec2 button_size = size;
		ImVec2 button_pos = pos;
		ImGui::SetCursorScreenPos(button_pos);

		std::wstring button_text;
		button_text = m_is_dark ? ImGui::CloseNotifButton : ImGui::CloseNotifButton;
		if (ImGui::IsMouseHoveringRect(button_pos, button_pos + button_size, true))
		{
			button_text = m_is_dark ? ImGui::CloseNotifHoverButton : ImGui::CloseNotifHoverButton;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				close();
		}
		imgui.button(button_text.c_str());

		ImGui::PopStyleColor(5);
	}
}

}}

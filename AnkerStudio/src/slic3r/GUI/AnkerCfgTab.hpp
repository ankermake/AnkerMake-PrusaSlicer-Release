#ifndef _ANKER_CFG_TAB_
#define _ANKER_CFG_TAB_

//	 The "Expert" tab at the right of the main tabbed window.
//	
//	 This file implements following packages:
//	   Slic3r::GUI::AnkerTab;
//	       Slic3r::GUI::AnkerTab::Print;
//	       Slic3r::GUI::AnkerTab::Filament;
//	       Slic3r::GUI::AnkerTab::Printer;
//	   Slic3r::GUI::AnkerTab::AnkerPage
//	       - Option page: For example, the Slic3r::GUI::AnkerTab::Print has option pages "Layers and perimeters", "Infill", "Skirt and brim" ...
//	   Slic3r::GUI::SavePresetWindow
//	       - Dialog to select a new preset name to store the configuration.
//	   Slic3r::GUI::AnkerTab::Preset;
//	       - Single preset item: name, file is default or external.

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/listbook.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/bmpcbox.h>
#include <wx/bmpbuttn.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>

#include <map>
#include <vector>
#include <memory>

#include "BedShapeDialog.hpp"
#include "ButtonsDescription.hpp"
#include "Event.hpp"
#include "wxExtensions.hpp"
#include "ConfigManipulation.hpp"
#include "AnkerOptionsGroup.hpp"
#include "libslic3r/Preset.hpp"

#include "AnkerBtn.hpp"
#include "Common/AnkerPopupWidget.hpp"
#include "AnkerLineEdit.hpp"

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PRESET, wxCommandEvent);

namespace Slic3r {
namespace GUI {
// add by allen for ankerCfgDlg to hide some control
#define _HIDE_CONTROL_FOR_ANKERCFGDLG_ 1

class TabPresetComboBox;
// add by allen for ankerCfgDlg
class AnkerTabPresetComboBox;
// add by dhf for ankerPage
class Anker_OG_CustomCtrl;
// G-code substitutions

// Substitution Manager - helper for manipuation of the substitutions
class AnkerSubstitutionManager
{
	DynamicPrintConfig* m_config{ nullptr };
	wxWindow*			m_parent{ nullptr };
	wxFlexGridSizer*	m_grid_sizer{ nullptr };

	int                 m_em{10};
	std::function<void()> m_cb_edited_substitution{ nullptr };
	std::function<void()> m_cb_hide_delete_all_btn{ nullptr };

	void validate_lenth();
	bool is_compatibile_with_ui();
	bool is_valid_id(int substitution_id, const wxString& message);

public:
	AnkerSubstitutionManager() = default;
	~AnkerSubstitutionManager() = default;

	void init(DynamicPrintConfig* config, wxWindow* parent, wxFlexGridSizer* grid_sizer);
	void create_legend();
	void delete_substitution(int substitution_id);
	void add_substitution(	int substitution_id = -1,
							const std::string& plain_pattern = std::string(),
							const std::string& format = std::string(),
							const std::string& params = std::string(),
							const std::string& notes  = std::string());
	void update_from_config();
	void delete_all();
	void edit_substitution(int substitution_id, 
						   int opt_pos, // option position insubstitution [0, 2]
						   const std::string& value);
	void set_cb_edited_substitution(std::function<void()> cb_edited_substitution) {
		m_cb_edited_substitution = cb_edited_substitution;
	}
	void call_ui_update() {
		if (m_cb_edited_substitution)
			m_cb_edited_substitution();
	}
	void set_cb_hide_delete_all_btn(std::function<void()> cb_hide_delete_all_btn) {
		m_cb_hide_delete_all_btn = cb_hide_delete_all_btn;
	}
	void hide_delete_all_btn() {
		if (m_cb_hide_delete_all_btn)
			m_cb_hide_delete_all_btn();
	}
	bool is_empty_substitutions();
};

// Single AnkerTab page containing a{ vsizer } of{ optgroups }
// package Slic3r::GUI::AnkerTab::AnkerPage;
// add by dhf for ankerPage
using AnkerConfigOptionsGroupShp = std::shared_ptr<AnkerConfigOptionsGroup>;
using AnkerConfigOptionsGroupWkp = std::weak_ptr<AnkerConfigOptionsGroup>;

class AnkerPage// : public wxScrolledWindow
{
	wxWindow*		m_parent;
	wxString		m_title;
	size_t			m_iconID;
	wxBoxSizer*		m_vsizer;
    bool            m_show = true;
public:
    AnkerPage(wxWindow* parent, const wxString& title, int iconID);
	~AnkerPage() {}

	bool				m_is_modified_values{ false };
	bool				m_is_nonsys_values{ true };

public:
	std::vector <AnkerConfigOptionsGroupShp> m_optgroups;
	DynamicPrintConfig* m_config;

	wxBoxSizer*	vsizer() const { return m_vsizer; }
	wxWindow*	parent() const { return m_parent; }
	const wxString&	title()	 const { return m_title; }
	size_t		iconID() const { return m_iconID; }
	void		set_config(DynamicPrintConfig* config_in) { m_config = config_in; }
	void		reload_config();
    void        update_visibility(ConfigOptionMode mode, bool update_contolls_visibility);
    void        activate(ConfigOptionMode mode, std::function<void()> throw_if_canceled);
    void        clear();
	void		update_visibility(ConfigOptionMode mode);
    void        msw_rescale();
    void        sys_color_changed();
    void        refresh();
	Field*		get_field(const t_config_option_key& opt_key, int opt_index = -1) const;
	AnkerLine*		get_line(const t_config_option_key& opt_key);
	bool		set_value(const t_config_option_key& opt_key, const boost::any& value);
	AnkerConfigOptionsGroupShp	new_optgroup(const wxString& title, int noncommon_label_width = -1);
	const AnkerConfigOptionsGroupShp	get_optgroup(const wxString& title) const;

	bool		set_item_colour(const wxColour *clr) {
		if (m_item_color != clr) {
			m_item_color = clr;
			return true;
		}
		return false;
	}

	const wxColour	get_item_colour() {
			return *m_item_color;
	}
    bool get_show() const { return m_show; }

protected:
	// Color of TreeCtrlItem. The wxColour will be updated only if the new wxColour pointer differs from the currently rendered one.
	const wxColour*		m_item_color;
};


using AnkerPageShp = std::shared_ptr<AnkerPage>;
class AnkerTab: public wxPanel
{
	wxBookCtrlBase*			m_parent;
#ifdef __WXOSX__
	wxPanel*			m_tmp_panel;
	int					m_size_move = -1;
#endif // __WXOSX__
protected:
    Preset::Type        m_type;
	std::string			m_name;
	const wxString		m_title;
	AnkerTabPresetComboBox* m_presets_choice{nullptr};
	ScalableButton*		m_search_btn;
	ScalableButton*		m_btn_compare_preset;
	ScalableButton*		m_btn_save_preset;
	ScalableButton*		m_btn_rename_preset;
	ScalableButton*		m_btn_delete_preset;
	ScalableButton*		m_btn_edit_ph_printer {nullptr};
	ScalableButton*		m_btn_hide_incompatible_presets;
	wxBoxSizer*			m_hsizer;
	wxBoxSizer*			m_h_buttons_sizer;
	wxBoxSizer*			m_left_sizer;
	//add by dhf, to change page UI
	wxBoxSizer*			m_right_sizer;
	wxScrolledWindow* m_page_buttons_scrolledwin{nullptr};
	wxBoxSizer* m_page_buttons_sizer{ nullptr };
	std::vector<AnkerBtn*> m_page_buttons;

	wxScrolledWindow*	m_page_view {nullptr};
	wxBoxSizer*			m_page_sizer {nullptr};

    ModeSizer*			m_mode_sizer {nullptr};

   	struct PresetDependencies {
		Preset::Type type	  = Preset::TYPE_INVALID;
		AnkerCheckBox *checkbox = nullptr;
		ScalableButton 	*btn  = nullptr;
		std::string  key_list; // "compatible_printers"
		std::string  key_condition;
		wxString     dialog_title;
		wxString     dialog_label;
	};
	PresetDependencies 	m_compatible_printers;
	PresetDependencies 	m_compatible_prints;

    /* Indicates, that default preset or preset inherited from default is selected
     * This value is used for a options color updating 
     * (use green color only for options, which values are equal to system values)
     */
    bool                    m_is_default_preset {false};


	ScalableButton*			m_undo_btn;
	ScalableButton*			m_undo_to_sys_btn;
	ScalableButton*			m_question_btn;

	wxControl*				m_pSearchEdit{ nullptr };
	AnkerLineEdit*			m_pSearchTextCtrl{ nullptr };

	AnkerPopupWidget*		m_popupWidget{ nullptr };

	// Bitmaps to be shown on the "Revert to system" aka "Lock to system" button next to each input field.
	ScalableBitmap 			m_bmp_value_lock;
	ScalableBitmap 			m_bmp_value_unlock;
	ScalableBitmap 			m_bmp_white_bullet;
	ScalableBitmap 			m_bmp_empty;
	// The following bitmap points to either m_bmp_value_unlock or m_bmp_white_bullet, depending on whether the current preset has a parent preset.
	ScalableBitmap 		   *m_bmp_non_system;
	// Bitmaps to be shown on the "Undo user changes" button next to each input field.
	ScalableBitmap 			m_bmp_value_revert;
    
    std::vector<ScalableButton*>	m_scaled_buttons = {};    
    std::vector<ScalableBitmap*>	m_scaled_bitmaps = {};    
    std::vector<ScalableBitmap>     m_scaled_icons_list = {};

	// Colors for ui "decoration"
	wxColour			m_sys_label_clr;
	wxColour			m_modified_label_clr;
	wxColour			m_default_text_clr;

	// Tooltip text for reset buttons (for whole options group)
	wxString			m_ttg_value_lock;
	wxString			m_ttg_value_unlock;
	wxString			m_ttg_white_bullet_ns;
	// The following text points to either m_ttg_value_unlock or m_ttg_white_bullet_ns, depending on whether the current preset has a parent preset.
	wxString			*m_ttg_non_system;
	// Tooltip text to be shown on the "Undo user changes" button next to each input field.
	wxString			m_ttg_white_bullet;
	wxString			m_ttg_value_revert;

	// Tooltip text for reset buttons (for each option in group)
	wxString			m_tt_value_lock;
	wxString			m_tt_value_unlock;
	// The following text points to either m_tt_value_unlock or m_ttg_white_bullet_ns, depending on whether the current preset has a parent preset.
	wxString			*m_tt_non_system;
	// Tooltip text to be shown on the "Undo user changes" button next to each input field.
	wxString			m_tt_white_bullet;
	wxString			m_tt_value_revert;

	int					m_icon_count;
	std::map<std::string, size_t>	m_icon_index;		// Map from an icon file name to its index
	std::map<wxString, std::string>	m_category_icon;	// Map from a category name to an icon file name
	std::vector<AnkerPageShp>			m_pages;
	AnkerPage*				m_active_page {nullptr};
	bool				m_disable_tree_sel_changed_event {false};
	bool				m_show_incompatible_presets;

    std::vector<Preset::Type>	m_dependent_tabs;
	enum OptStatus { osSystemValue = 1, osInitValue = 2 };
	std::map<std::string, int>	m_options_list;
	int							m_opt_status_value = 0;

	std::vector<GUI_Descriptions::ButtonEntry>	m_icon_descriptions = {};

	bool				m_is_modified_values{ false };
	bool				m_is_nonsys_values{ true };
	bool				m_postpone_update_ui {false};

    void                set_type();

    int                 m_em_unit;
    // To avoid actions with no-completed AnkerTab
    bool                m_completed { false };
    ConfigOptionMode    m_mode = comExpert; // to correct first AnkerTab update_visibility() set mode to Expert

	AnkerHighlighterForWx	m_highlighter;

	DynamicPrintConfig 	m_cache_config;


	bool				m_page_switch_running = false;
	bool				m_page_switch_planned = false;
	bool				m_page_valid = false;
public:
	PresetBundle*		m_preset_bundle;
	bool				m_show_btn_incompatible_presets = false;
	PresetCollection*	m_presets;
	DynamicPrintConfig*	m_config = nullptr;
	AnkerogStaticText*		m_parent_preset_description_line = nullptr;
	ScalableButton*		m_detach_preset_btn	= nullptr;
	std::map<std::string, AnkerCheckBox*> m_overrides_options;

    // Counter for the updating (because of an update() function can have a recursive behavior):
    // 1. increase value from the very beginning of an update() function
    // 2. decrease value at the end of an update() function
    // 3. propagate changed configuration to the Plater when (m_update_cnt == 0) only
    int                 m_update_cnt = 0;

	wxTimer timer;
public:
    AnkerTab(wxBookCtrlBase* parent, const wxString& title, Preset::Type type);
	~AnkerTab() { m_page_valid = false; }

	wxWindow*	parent() const { return m_parent; }
	wxString	title()	 const { return m_title; }
	std::string	name()	 const { return m_presets->name(); }
    Preset::Type type()  const { return m_type; }
    // The tab is already constructed.
    bool 		completed() const { return m_completed; }
	virtual bool supports_printer_technology(const PrinterTechnology tech) const = 0;

	void		create_preset_tab();
    void        add_scaled_button(wxWindow* parent, ScalableButton** btn, const std::string& icon_name, 
                                  const wxString& label = wxEmptyString, 
                                  long style = wxBU_EXACTFIT | wxNO_BORDER);
    void        add_scaled_bitmap(wxWindow* parent, ScalableBitmap& btn, const std::string& icon_name);
	void		create_search_edit(wxWindow* parent);
	void		update_ui_items_related_on_parent_preset(const Preset* selected_preset_parent);
    void		load_current_preset();
	// add by dhf, to change page UI
	void		rebuild_page_buttons();
	void		change_page(wxString page_Tab_text);
	void		clear_page_buttons();
	int			get_page_title_gap(bool is_selected_page = false);
    void		update_btns_enabling();
    void		update_preset_choice();
	void		get_current_preset_name(std::string& strPresetName);
    // Select a new preset, possibly delete the current one.
	void		select_preset(std::string preset_name = "", bool delete_current = false, const std::string& last_selected_ph_printer_name = "");
	bool		may_discard_current_dirty_preset(PresetCollection* presets = nullptr, const std::string& new_printer_name = "");
	bool		AnkerMayDiscardCurDirtyPreset(PresetCollection* presets = nullptr, const std::string& new_printer_name = "");
    virtual void    clear_pages();
    virtual void    update_description_lines();
    virtual void    activate_selected_page(std::function<void()> throw_if_canceled);

	void		OnKeyDown(wxKeyEvent& event);

	void		compare_preset();
	void		transfer_options(const std::string&name_from, const std::string&name_to, std::vector<std::string> options);
	void		save_preset(std::string name = std::string(), bool detach = false);
	void		rename_preset();
	void		delete_preset();
	void		toggle_show_hide_incompatible();
	void		update_show_hide_incompatible_button();
	void		update_ui_from_settings();
	void		update_label_colours();
	void		decorate();
	void		update_changed_ui();
	void		get_sys_and_mod_flags(const std::string& opt_key, bool& sys_page, bool& modified_page);
	void		update_changed_tree_ui();
	void		update_undo_buttons();

	void		on_roll_back_value(const bool to_sys = false);

    AnkerPageShp		add_options_page(const wxString& title, const std::string& icon, bool is_extruder_pages = false);
	static wxString translate_category(const wxString& title, Preset::Type preset_type);

	virtual void	OnActivate();
	virtual void	on_preset_loaded() {}
	virtual void	build() = 0;
	virtual void	update() = 0;
	virtual void	toggle_options() = 0;
	virtual void	init_options_list();
	void			emplace_option(const std::string &opt_key, bool respect_vec_values = false);
	void			load_initial_data();
	void			update_dirty();
	void			update_tab_ui();
	void			load_config(const DynamicPrintConfig& config);
	virtual void	reload_config();
    void            update_mode();
    void            update_mode_markers();
    void            update_visibility();
	void            force_update();
    virtual void    msw_rescale();
    virtual void	sys_color_changed();
	Field*			get_field(const t_config_option_key& opt_key, int opt_index = -1) const;
	AnkerLine*			get_line(const t_config_option_key& opt_key);
	std::pair<Anker_OG_CustomCtrl*, bool*> get_custom_ctrl_with_blinking_ptr(const t_config_option_key& opt_key, int opt_index = -1);

    Field*          get_field(const t_config_option_key &opt_key, AnkerPage** selected_page, int opt_index = -1);
	void			toggle_option(const std::string& opt_key, bool toggle, int opt_index = -1);
	wxSizer*		description_line_widget(wxWindow* parent, AnkerogStaticText** StaticText, wxString text = wxEmptyString);
	bool			current_preset_is_dirty() const;
	bool			saved_preset_is_dirty() const;
	void            update_saved_preset_from_current_preset();

	DynamicPrintConfig*	get_config() { return m_config; }
	PresetCollection*	get_presets() { return m_presets; }
	const PresetCollection* get_presets() const { return m_presets; }

	void			on_value_change(const std::string& opt_key, const boost::any& value);

    void            update_wiping_button_visibility();
	void			activate_option(const std::string& opt_key, const wxString& category);
	void			cache_config_diff(const std::vector<std::string>& selected_options, const DynamicPrintConfig* config = nullptr);
	void			apply_config_from_cache();

	const std::map<wxString, std::string>& get_category_icon_map() { return m_category_icon; }

	static bool validate_custom_gcode(const wxString& title, const std::string& gcode);
	bool        validate_custom_gcodes();
    bool        validate_custom_gcodes_was_shown{ false };
	void		onAnkerPresetsChanged();
protected:
	void			create_line_with_widget(AnkerConfigOptionsGroup* optgroup, const std::string& opt_key, const std::string& path, widget_t widget);
	wxSizer*		compatible_widget_create(wxWindow* parent, PresetDependencies &deps);
	void 			compatible_widget_reload(PresetDependencies &deps);
	void			load_key_value(const std::string& opt_key, const boost::any& value, bool saved_value = false);

	// add by dhf, to change page UI
	bool			change_sel_page(wxString page_title);
	std::string     get_preset_type_str(Preset::Type type);
	void			on_presets_changed();
	void			build_preset_description_line(AnkerConfigOptionsGroup* optgroup);
	void			update_preset_description_line();
	void			update_frequently_changed_parameters();
	void			fill_icon_descriptions();
	void			set_tooltips_text();

    ConfigManipulation m_config_manipulation;
    ConfigManipulation get_config_manipulation();

private:
	AnkerTabPresetComboBox* getAnkerTabPresetCombo(const Preset::Type type);

	bool bPrintPresetNeedHide(Preset printPreset);
	void selectNormalPrintPreset();
};

class AnkerTabPrint : public AnkerTab
{
public:
	AnkerTabPrint(wxBookCtrlBase* parent) :
        AnkerTab(parent, _(L("Print Settings")), Slic3r::Preset::TYPE_PRINT) {}
	~AnkerTabPrint() {}

	void		build() override;
	void		update_description_lines() override;
	void		toggle_options() override;
	void		update() override;
	void		clear_pages() override;
	bool 		supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptFFF; }
	wxSizer*	create_manage_substitution_widget(wxWindow* parent);
	wxSizer*	create_substitutions_widget(wxWindow* parent);

private:
	AnkerogStaticText*	m_recommended_thin_wall_thickness_description_line = nullptr;
	AnkerogStaticText*	m_top_bottom_shell_thickness_explanation = nullptr;
	AnkerogStaticText*	m_post_process_explanation = nullptr;
	ScalableButton* m_del_all_substitutions_btn{nullptr};
	AnkerSubstitutionManager m_subst_manager;
};

class AnkerTabFilament : public AnkerTab
{
private:
	AnkerogStaticText*	m_volumetric_speed_description_line {nullptr};
	AnkerogStaticText*	m_cooling_description_line {nullptr};

    void            create_line_with_near_label_widget(AnkerConfigOptionsGroupShp optgroup, const std::string &opt_key, int opt_index = 0);
    void            update_line_with_near_label_widget(AnkerConfigOptionsGroupShp optgroup, const std::string &opt_key, int opt_index = 0, bool is_checked = true);
    void            add_filament_overrides_page();
    void            update_filament_overrides_page();
	void 			update_volumetric_flow_preset_hints();

    //std::map<std::string, AnkerCheckBox*> m_overrides_options;
public:
	AnkerTabFilament(wxBookCtrlBase* parent) :
		AnkerTab(parent, _(L("Filament Settings")), Slic3r::Preset::TYPE_FILAMENT) {}
	~AnkerTabFilament() {}

	void		build() override;
	void		update_description_lines() override;
	void		toggle_options() override;
	void		update() override;
	void		clear_pages() override;
	void        msw_rescale() override;
	bool 		supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptFFF; }
};

class AnkerTabPrinter : public AnkerTab
{
private:
	bool		m_has_single_extruder_MM_page = false;
	bool		m_use_silent_mode = false;
    bool        m_supports_travel_acceleration = false;
	bool        m_supports_min_feedrates = false;
	void		append_option_line(AnkerConfigOptionsGroupShp optgroup, const std::string opt_key);
	bool		m_rebuild_kinematics_page = false;
	AnkerogStaticText* m_machine_limits_description_line {nullptr};
	void 		update_machine_limits_description(const MachineLimitsUsage usage);

	AnkerogStaticText*	m_fff_print_host_upload_description_line {nullptr};
	AnkerogStaticText*	m_sla_print_host_upload_description_line {nullptr};

    std::vector<AnkerPageShp>			m_pages_fff;
    std::vector<AnkerPageShp>			m_pages_sla;

public:
	size_t		m_extruders_count;
	size_t		m_extruders_count_old = 0;
	size_t		m_initial_extruders_count;
	size_t		m_sys_extruders_count;
	size_t		m_cache_extruder_count = 0;

    PrinterTechnology               m_printer_technology = ptFFF;

    AnkerTabPrinter(wxBookCtrlBase* parent) :
        AnkerTab(parent, _L("Printer Settings"), Slic3r::Preset::TYPE_PRINTER) {}
	~AnkerTabPrinter() {}

	void		build() override;
	void		build_print_host_upload_group(AnkerPage* page);
    void		build_fff();
    void		build_sla();
	void		reload_config() override;
	void		activate_selected_page(std::function<void()> throw_if_canceled) override;
	void		clear_pages() override;
	void		toggle_options() override;
    void		update() override;
    void		update_fff();
    void		update_sla();
    void        update_pages(); // update m_pages according to printer technology
	void		extruders_count_changed(size_t extruders_count);
	AnkerPageShp		build_kinematics_page();
	void		build_extruder_pages(size_t n_before_extruders);
	void		build_unregular_pages(bool from_initial_build = false);
	void		on_preset_loaded() override;
	void		init_options_list() override;
	bool 		supports_printer_technology(const PrinterTechnology /* tech */) const override { return true; }

	wxSizer*	create_bed_shape_widget(wxWindow* parent);
	void		cache_extruder_cnt(const DynamicPrintConfig* config = nullptr);
	bool		apply_extruder_cnt_from_cache();
};

class AnkerTabSLAMaterial : public AnkerTab
{
public:
    AnkerTabSLAMaterial(wxBookCtrlBase* parent) :
		AnkerTab(parent, _(L("Material Settings")), Slic3r::Preset::TYPE_SLA_MATERIAL) {}
    ~AnkerTabSLAMaterial() {}

	void		build() override;
	void		toggle_options() override;
	void		update() override;
	bool 		supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptSLA; }
};

class AnkerTabSLAPrint : public AnkerTab
{
    // Methods are a vector of method prefix -> method label pairs
    // method prefix is the prefix whith which all the config values are prefixed
    // for a particular method. The label is the friendly name for the method
    void build_sla_support_params(const std::vector<SamePair<std::string>> &methods,
                                  const Slic3r::GUI::AnkerPageShp &page);

public:
    AnkerTabSLAPrint(wxBookCtrlBase* parent) :
        AnkerTab(parent, _(L("Print Settings")), Slic3r::Preset::TYPE_SLA_PRINT) {}
    ~AnkerTabSLAPrint() {}

	AnkerogStaticText* m_support_object_elevation_description_line = nullptr;

    void		build() override;
	void		update_description_lines() override;
	void		toggle_options() override;
    void		update() override;
	void		clear_pages() override;
	bool 		supports_printer_technology(const PrinterTechnology tech) const override { return tech == ptSLA; }
};

} // GUI
} // Slic3r

#endif /* _ANKER_CFG_TAB_ */

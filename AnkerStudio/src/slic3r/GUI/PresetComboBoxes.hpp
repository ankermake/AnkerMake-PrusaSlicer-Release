#ifndef slic3r_PresetComboBoxes_hpp_
#define slic3r_PresetComboBoxes_hpp_

#include <wx/bmpbndl.h>
#include <wx/gdicmn.h>

#include "libslic3r/Preset.hpp"
#include "wxExtensions.hpp"
#include "BitmapComboBox.hpp"
#include "GUI_Utils.hpp"
#include "Common/AnkerSimpleCombox.hpp"
#include "GUI_App.hpp"

class wxString;
class wxTextCtrl;
class wxStaticText;
class ScalableButton;
class wxBoxSizer;
class wxComboBox;
class wxStaticBitmap;

namespace Slic3r {

namespace GUI {

class BitmapCache;

// ---------------------------------
// ***  PresetComboBox  ***
// ---------------------------------

// BitmapComboBox used to presets list on Sidebar and Tabs
class PresetComboBox : public BitmapComboBox
{
    bool m_show_all { false };
    bool m_show_modif_preset_separately{ false };
public:
    PresetComboBox(wxWindow* parent, Preset::Type preset_type, const wxSize& size = wxDefaultSize, PresetBundle* preset_bundle = nullptr);
    ~PresetComboBox();

	enum LabelItemType {
		LABEL_ITEM_PHYSICAL_PRINTER = 0xffffff01,
		LABEL_ITEM_DISABLED,
		LABEL_ITEM_MARKER,
		LABEL_ITEM_PHYSICAL_PRINTERS,
		LABEL_ITEM_WIZARD_PRINTERS,
        LABEL_ITEM_WIZARD_FILAMENTS,
        LABEL_ITEM_WIZARD_MATERIALS,

        LABEL_ITEM_MAX,
	};

    void set_label_marker(int item, LabelItemType label_item_type = LABEL_ITEM_MARKER);
    bool set_printer_technology(PrinterTechnology pt);

    void set_selection_changed_function(std::function<void(int)> sel_changed) { on_selection_changed = sel_changed; }

    bool is_selected_physical_printer();

    // Return true, if physical printer was selected 
    // and next internal selection was accomplished
    bool selection_is_changed_according_to_physical_printers();

    void update(std::string select_preset);
    // select preset which is selected in PreseBundle
    void update_from_bundle();

    void edit_physical_printer();
    void add_physical_printer();
    void open_physical_printer_url();
    bool del_physical_printer(const wxString& note_string = wxEmptyString);
    void show_modif_preset_separately() { m_show_modif_preset_separately = true; }

    virtual wxString get_preset_name(const Preset& preset); 
    Preset::Type     get_type() { return m_type; }
    void             show_all(bool show_all);
    virtual void update();
    virtual void msw_rescale();
    virtual void sys_color_changed();
    virtual void OnSelect(wxCommandEvent& evt);

protected:
    typedef std::size_t Marker;
    std::function<void(int)>    on_selection_changed { nullptr };

    Preset::Type        m_type;
    std::string         m_main_bitmap_name;

    PresetBundle*       m_preset_bundle {nullptr};
    PresetCollection*   m_collection {nullptr};

    // Caching bitmaps for the all bitmaps, used in preset comboboxes
    static BitmapCache& bitmap_cache();

    // Indicator, that the preset is compatible with the selected printer.
    wxBitmapBundle*      m_bitmapCompatible;
    // Indicator, that the preset is NOT compatible with the selected printer.
    wxBitmapBundle*      m_bitmapIncompatible;

    int m_last_selected;
    int m_em_unit;
    bool m_suppress_change { true };

    // parameters for an icon's drawing
    int icon_height;
    int norm_icon_width;
    int null_icon_width;
    int thin_icon_width;
    int wide_icon_width;
    int space_icon_width;
    int thin_space_icon_width;
    int wide_space_icon_width;

    PrinterTechnology printer_technology {ptAny};

    void invalidate_selection();
    void validate_selection(bool predicate = false);
    void update_selection();

#ifdef __linux__
    static const char* separator_head() { return "------- "; }
    static const char* separator_tail() { return " -------"; }
#else // __linux__ 
    static const char* separator_head() { return "————— "; }
    static const char* separator_tail() { return " —————"; }
#endif // __linux__
    static wxString    separator(const wxString& label);

    wxBitmapBundle* get_bmp(  std::string bitmap_key, bool wide_icons, const std::string& main_icon_name,
                        bool is_compatible = true, bool is_system = false, bool is_single_bar = false,
                        const std::string& filament_rgb = "", const std::string& extruder_rgb = "", const std::string& material_rgb = "");

    wxBitmapBundle* get_bmp(  std::string bitmap_key, const std::string& main_icon_name, const std::string& next_icon_name,
                        bool is_enabled = true, bool is_compatible = true, bool is_system = false);

    wxBitmapBundle NullBitmapBndl();

private:
    void fill_width_height();
};


// add by allen for ankerCfgDlg
class AnkerPresetComboBox : public AnkerSimpleCombox
{
    bool m_show_all{ false };
    bool m_show_modif_preset_separately{ false };
public:
    AnkerPresetComboBox(wxWindow* parent, Preset::Type preset_type, const wxSize& size = wxDefaultSize, PresetBundle* preset_bundle = nullptr);
    ~AnkerPresetComboBox();

    enum LabelItemType {
        LABEL_ITEM_PHYSICAL_PRINTER = 0xffffff01,
        LABEL_ITEM_DISABLED,
        LABEL_ITEM_MARKER,
        LABEL_ITEM_PHYSICAL_PRINTERS,
        LABEL_ITEM_WIZARD_PRINTERS,
        LABEL_ITEM_WIZARD_FILAMENTS,
        LABEL_ITEM_WIZARD_MATERIALS,

        LABEL_ITEM_MAX,
    };

    void set_label_marker(int item, LabelItemType label_item_type = LABEL_ITEM_MARKER);
    bool set_printer_technology(PrinterTechnology pt);

    void set_selection_changed_function(std::function<void(int)> sel_changed) { on_selection_changed = sel_changed; }

    bool is_selected_physical_printer();

    // Return true, if physical printer was selected 
    // and next internal selection was accomplished
    bool selection_is_changed_according_to_physical_printers();

    void update(std::string select_preset);
    // select preset which is selected in PreseBundle
    void update_from_bundle();

    void edit_physical_printer();
    void add_physical_printer();
    void open_physical_printer_url();
    bool del_physical_printer(const wxString& note_string = wxEmptyString);
    void show_modif_preset_separately() { m_show_modif_preset_separately = true; }

    virtual wxString get_preset_name(const Preset& preset);
    Preset::Type     get_type() { return m_type; }
    void             show_all(bool show_all);
    virtual void update();
    virtual void msw_rescale();
    virtual void sys_color_changed();
    virtual void OnSelect(wxCommandEvent& evt);

protected:
    typedef std::size_t Marker;
    std::function<void(int)>    on_selection_changed{ nullptr };

    Preset::Type        m_type;
    std::string         m_main_bitmap_name;

    PresetBundle* m_preset_bundle{ nullptr };
    PresetCollection* m_collection{ nullptr };

    // Caching bitmaps for the all bitmaps, used in preset comboboxes
    static BitmapCache& bitmap_cache();

    // Indicator, that the preset is compatible with the selected printer.
    wxBitmapBundle* m_bitmapCompatible;
    // Indicator, that the preset is NOT compatible with the selected printer.
    wxBitmapBundle* m_bitmapIncompatible;

    int m_last_selected;
    int m_em_unit;
    bool m_suppress_change{ true };

    // parameters for an icon's drawing
    int icon_height;
    int norm_icon_width;
    int null_icon_width;
    int thin_icon_width;
    int wide_icon_width;
    int space_icon_width;
    int thin_space_icon_width;
    int wide_space_icon_width;

    PrinterTechnology printer_technology{ ptAny };

    void invalidate_selection();
    void validate_selection(bool predicate = false);
    void update_selection();
    void update_selection(int topNozzleIndex);

#ifdef __linux__
    static const char* separator_head() { return "------- "; }
    static const char* separator_tail() { return " -------"; }
#else // __linux__ 
    static const char* separator_head() { return "————— "; }
    static const char* separator_tail() { return " —————"; }
#endif // __linux__
    static const wxString  separator(const wxString& label);

    wxBitmapBundle* get_bmp(std::string bitmap_key, bool wide_icons, const std::string& main_icon_name,
        bool is_compatible = true, bool is_system = false, bool is_single_bar = false,
        const std::string& filament_rgb = "", const std::string& extruder_rgb = "", const std::string& material_rgb = "");

    wxBitmapBundle* get_bmp(std::string bitmap_key, const std::string& main_icon_name, const std::string& next_icon_name,
        bool is_enabled = true, bool is_compatible = true, bool is_system = false);

    wxBitmapBundle NullBitmapBndl();
    // mod by allen for Change the interaction for switching print and filament presets.
    bool bPrintPresetNeedHide(Preset preset);

private:
    void fill_width_height();
};


// ---------------------------------
// ***  PlaterPresetComboBox  ***
// ---------------------------------

class PlaterPresetComboBox : public PresetComboBox
{
public:
    PlaterPresetComboBox(wxWindow *parent, Preset::Type preset_type);
    ~PlaterPresetComboBox();

    ScalableButton* edit_btn { nullptr };

    void set_extruder_idx(const int extr_idx)   { m_extruder_idx = extr_idx; }
    int  get_extruder_idx() const               { return m_extruder_idx; }

    void switch_to_tab();
    void change_extruder_color();
    void show_add_menu();
    void show_edit_menu();

    wxString get_preset_name(const Preset& preset) override;
    void update() override;
    void msw_rescale() override;
    void sys_color_changed() override;
    void OnSelect(wxCommandEvent& evt) override;

    std::string get_selected_ph_printer_name() const;

private:
    int     m_extruder_idx = -1;
};

// add by allen for ankerCfgDlg
// ---------------------------------
// ***  AnkerPlaterPresetComboBox  ***
// ---------------------------------

class AnkerPlaterPresetComboBox : public AnkerPresetComboBox
{
public:
    AnkerPlaterPresetComboBox(wxWindow* parent, Preset::Type preset_type,wxSize widgetSize = wxSize(15 * Slic3r::GUI::wxGetApp().em_unit(), -1));
    ~AnkerPlaterPresetComboBox();

    ScalableButton* edit_btn{ nullptr };

    void set_extruder_idx(const int extr_idx) { m_extruder_idx = extr_idx; }
    int  get_extruder_idx() const { return m_extruder_idx; }
    void set_button_clicked_function(std::function<void()> button_clicked) { on_button_clicked = button_clicked; }
    virtual void OnButtonClick();
    void switch_to_tab();
    void change_extruder_color();
    void show_add_menu();
    void show_edit_menu();

    wxString get_preset_name(const Preset& preset) override;
    void update_dirty();
    void update() override;
    void update(int topNozzleIndex);
    void msw_rescale() override;
    void sys_color_changed() override;
    void OnSelect(wxCommandEvent& evt) override;
    Preset::Type        type()      const { return m_type; }
    std::string get_selected_ph_printer_name() const;

private:
    int     m_extruder_idx = -1;
    std::function<void()>    on_button_clicked{ nullptr };
};

// ---------------------------------
// ***  TabPresetComboBox  ***
// ---------------------------------

class TabPresetComboBox : public PresetComboBox
{
    bool show_incompatible {false};
    bool m_enable_all {false};

public:
    TabPresetComboBox(wxWindow *parent, Preset::Type preset_type);
    ~TabPresetComboBox() {}
    void set_show_incompatible_presets(bool show_incompatible_presets) {
        show_incompatible = show_incompatible_presets;
    }

    wxString get_preset_name(const Preset& preset) override;
    void update() override;
    void update_dirty();
    void msw_rescale() override;
    void OnSelect(wxCommandEvent& evt) override;

    void set_enable_all(bool enable=true) { m_enable_all = enable; }

    PresetCollection*   presets()   const { return m_collection; }
    Preset::Type        type()      const { return m_type; }
};

// add by allen for ankerCfgDlg
// ---------------------------------
// ***  AnkerTabPresetComboBox  ***
// ---------------------------------

class AnkerTabPresetComboBox : public AnkerPresetComboBox
{
    bool show_incompatible{ false };
    bool m_enable_all{ false };

public:
    AnkerTabPresetComboBox(wxWindow* parent, Preset::Type preset_type);
    ~AnkerTabPresetComboBox() {}
    void set_show_incompatible_presets(bool show_incompatible_presets) {
        show_incompatible = show_incompatible_presets;
    }
    // add by allen for ankerCfgDlg
    void set_button_clicked_function(std::function<void()> button_clicked) { on_button_clicked = button_clicked; }
    virtual void OnButtonClick();

    wxString get_preset_name(const Preset& preset);
    void update();
    void update_dirty();
    void msw_rescale();
    void OnSelect(wxCommandEvent& evt);

    void set_enable_all(bool enable = true) { m_enable_all = enable; }

    PresetCollection* presets()   const { return m_collection; }
    Preset::Type        type()      const { return m_type; }

private:
    std::function<void()>    on_button_clicked{ nullptr };
};

} // namespace GUI
} // namespace Slic3r

#endif

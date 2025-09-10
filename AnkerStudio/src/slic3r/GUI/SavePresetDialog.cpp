#include "SavePresetDialog.hpp"

#include <cstddef>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h> 

#include "libslic3r/PresetBundle.hpp"

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "Tab.hpp"
// add by allen for ankerCfgDlg AnkerSavePresetDialog
#include "common/AnkerSimpleCombox.hpp"
#include "AnkerBtn.hpp"
#include "common/AnkerGUIConfig.hpp"

using Slic3r::GUI::format_wxstr;

namespace Slic3r {
namespace GUI {

constexpr auto BORDER_W = 10;
// add by allen for ankerCfgDlg AnkerSavePresetDialog
#define ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR  wxColour("#48494F")

//-----------------------------------------------
//          SavePresetDialog::Item
//-----------------------------------------------

std::string SavePresetDialog::Item::get_init_preset_name(const std::string &suffix)
{
    PresetBundle*     preset_bundle = dynamic_cast<SavePresetDialog*>(m_parent)->get_preset_bundle();
    if (!preset_bundle)
        preset_bundle = wxGetApp().preset_bundle;
    m_presets = &preset_bundle->get_presets(m_type);

    const Preset& sel_preset = m_presets->get_selected_preset();
    std::string preset_name = sel_preset.is_default ? "Untitled" :
                              sel_preset.is_system ? (boost::format(("%1% - %2%")) % sel_preset.name % suffix).str() :
                              sel_preset.name;

    // if name contains extension
    if (boost::iends_with(preset_name, ".ini")) {
        size_t len = preset_name.length() - 4;
        preset_name.resize(len);
    }

    return preset_name;
}

void SavePresetDialog::Item::init_input_name_ctrl(wxBoxSizer *input_name_sizer, const std::string preset_name)
{
    if (m_use_text_ctrl) {
#ifdef _WIN32
        long style = wxBORDER_SIMPLE;
#else
        long style = 0L;
#endif
        m_text_ctrl = new wxTextCtrl(m_parent, wxID_ANY, from_u8(preset_name), wxDefaultPosition, wxSize(35 * wxGetApp().em_unit(), -1), style);
        wxGetApp().UpdateDarkUI(m_text_ctrl);
        m_text_ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update(); });

        input_name_sizer->Add(m_text_ctrl,1, wxEXPAND, BORDER_W);
    }
    else {
        std::vector<std::string> values;
        for (const Preset&preset : *m_presets) {
            if (preset.is_default || preset.is_system || preset.is_external)
                continue;
            values.push_back(preset.name);
        }

        m_combo = new wxComboBox(m_parent, wxID_ANY, from_u8(preset_name), wxDefaultPosition, wxSize(35 * wxGetApp().em_unit(), -1));
        for (const std::string&value : values)
            m_combo->Append(from_u8(value));

        m_combo->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update(); });
#ifdef __WXOSX__
        // Under OSX wxEVT_TEXT wasn't invoked after change selection in combobox,
        // So process wxEVT_COMBOBOX too
        m_combo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) { update(); });
#endif //__WXOSX__

        input_name_sizer->Add(m_combo,    1, wxEXPAND, BORDER_W);
    }
}

static std::map<Preset::Type, std::string> TOP_LABELS =
{
    // type                             Save settings    
    { Preset::Type::TYPE_PRINT,         L("Save print settings as")   },
    { Preset::Type::TYPE_SLA_PRINT,     L("Save print settings as")   },
    { Preset::Type::TYPE_FILAMENT,      L("Save filament settings as")},
    { Preset::Type::TYPE_SLA_MATERIAL,  L("Save material settings as")},
    { Preset::Type::TYPE_PRINTER,       L("Save printer settings as") },
};

SavePresetDialog::Item::Item(Preset::Type type, const std::string& suffix, wxBoxSizer* sizer, SavePresetDialog* parent, bool is_for_multiple_save):
    m_type(type),
    m_use_text_ctrl(parent->is_for_rename()),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    m_valid_label->SetFont(wxGetApp().bold_font());

    wxStaticText* label_top = is_for_multiple_save ? new wxStaticText(m_parent, wxID_ANY, _(TOP_LABELS.at(m_type)) + ":") : nullptr;

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, get_init_preset_name(suffix));

    if (label_top)
        sizer->Add(label_top,   0, wxEXPAND | wxTOP| wxBOTTOM, BORDER_W);
    sizer->Add(input_name_sizer,0, wxEXPAND | (label_top ? 0 : wxTOP) | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label,   0, wxEXPAND | wxLEFT,   3*BORDER_W);

    if (m_type == Preset::TYPE_PRINTER)
        parent->add_info_for_edit_ph_printer(sizer);

    update();
}

SavePresetDialog::Item::Item(wxWindow* parent, wxBoxSizer* sizer, const std::string& def_name, PrinterTechnology pt /*= ptFFF*/):
    m_preset_name(def_name),
    m_printer_technology(pt),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    m_valid_label->SetFont(wxGetApp().bold_font());

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, m_preset_name);

    sizer->Add(input_name_sizer,0, wxEXPAND | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label,   0, wxEXPAND | wxLEFT,   3*BORDER_W);

    update();
}

const Preset* SavePresetDialog::Item::get_existing_preset() const 
{
    if (m_presets)
        return m_presets->find_preset(m_preset_name, false);

    for (const Preset::Type& type : PresetBundle::types_list(m_printer_technology)) {
        const PresetCollection& presets = wxGetApp().preset_bundle->get_presets(type);
        if (const Preset* preset = presets.find_preset(m_preset_name, false))
            return preset;
    }

    return nullptr;
}

void SavePresetDialog::Item::update()
{
    m_preset_name = into_u8(m_use_text_ctrl ? m_text_ctrl->GetValue() : m_combo->GetValue());

    m_valid_type = ValidationType::Valid;
    wxString info_line;

    const char* unusable_symbols = "<>[]:/\\|?*\"";

    const std::string unusable_suffix = PresetCollection::get_suffix_modified();//"(modified)";
    for (size_t i = 0; i < std::strlen(unusable_symbols); i++) {
        if (m_preset_name.find_first_of(unusable_symbols[i]) != std::string::npos) {
            info_line = _L("The supplied name is not valid;") + "\n" +
                        _L("the following characters are not allowed:") + " " + unusable_symbols;
            m_valid_type = ValidationType::NoValid;
            break;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find(unusable_suffix) != std::string::npos) {
        info_line = _L("The supplied name is not valid;") + "\n" +
                    _L("the following suffix is not allowed:") + "\n\t" +
                    from_u8(unusable_suffix);
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name == "- default -") {
        info_line = _L("The supplied name is not available.");
        m_valid_type = ValidationType::NoValid;
    }

    const Preset* existing = get_existing_preset();
    if (m_valid_type == ValidationType::Valid && existing && (existing->is_default || existing->is_system)) {
        info_line = m_use_text_ctrl ? _L("This name is used for a system profile name, use another.") :
                             _L("Cannot overwrite a system profile.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && existing && (existing->is_external)) {
        info_line = m_use_text_ctrl ? _L("This name is used for an external profile name, use another.") :
                             _L("Cannot overwrite an external profile.");
        m_valid_type = ValidationType::NoValid;
    }

    SavePresetDialog* dlg = dynamic_cast<SavePresetDialog*>(m_parent);
    if (m_valid_type == ValidationType::Valid && existing)
    {
        if (m_presets && m_preset_name == m_presets->get_selected_preset_name()) {
            if ((!m_use_text_ctrl && m_presets->get_edited_preset().is_dirty) ||
                (dlg && dlg->get_preset_bundle())) // means that we save modifications from the DiffDialog
                info_line = _L("Save preset modifications to existing user profile");
            m_valid_type = ValidationType::Valid;
        }
        else {
            if (existing->is_compatible)
                info_line = from_u8((boost::format(_u8L("Preset with name \"%1%\" already exists.")) % m_preset_name).str());
            else
                info_line = from_u8((boost::format(_u8L("Preset with name \"%1%\" already exists and is incompatible with selected printer.")) % m_preset_name).str());
            info_line += "\n" + _L("Note: This preset will be replaced after saving");
            m_valid_type = ValidationType::Warning;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.empty()) {
        info_line = _L("The name cannot be empty.");
        m_valid_type = ValidationType::NoValid;
    }

#ifdef __WXMSW__
    const int max_path_length = MAX_PATH;
#else
    const int max_path_length = 255;
#endif

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->path_from_name(m_preset_name).length() >= max_path_length) {
        info_line = _L("The name is too long.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_first_of(' ') == 0) {
        info_line = _L("The name cannot start with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_last_of(' ') == m_preset_name.length()-1) {
        info_line = _L("The name cannot end with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->get_preset_name_by_alias(m_preset_name) != m_preset_name) {
        info_line = _L("The name cannot be the same as a preset alias name.");
        m_valid_type = ValidationType::NoValid;
    }

    if ((dlg && !dlg->get_info_line_extention().IsEmpty()) && m_valid_type != ValidationType::NoValid)
        info_line += "\n\n" + dlg->get_info_line_extention();

    m_valid_label->SetLabel(info_line);
    m_valid_label->Show(!info_line.IsEmpty());
    if(!info_line.IsEmpty()) {
        m_parent->SetSize(AnkerSize(500, -1));
        m_parent->Fit();
        m_parent->Layout();
    }

    update_valid_bmp();

    if (dlg && m_type == Preset::TYPE_PRINTER)
        dlg->update_info_for_edit_ph_printer(m_preset_name);

    m_parent->Layout();
}

void SavePresetDialog::Item::update_valid_bmp()
{
    std::string bmp_name =  m_valid_type == ValidationType::Warning ? "exclamation_manifold" :
                            m_valid_type == ValidationType::NoValid ? "exclamation"          : "tick_mark" ;
    m_valid_bmp->SetBitmap(*get_bmp_bundle(bmp_name));
}

void SavePresetDialog::Item::accept()
{
    if (m_valid_type == ValidationType::Warning)
        m_presets->delete_preset(m_preset_name);
}

void SavePresetDialog::Item::Enable(bool enable /*= true*/)
{
    m_valid_label->Enable(enable);
    m_valid_bmp->Enable(enable);
    m_use_text_ctrl ? m_text_ctrl->Enable(enable) : m_combo->Enable(enable);
}


//-----------------------------------------------
//          SavePresetDialog
//-----------------------------------------------

SavePresetDialog::SavePresetDialog(wxWindow* parent, std::vector<Preset::Type> types, std::string suffix, bool template_filament/* =false*/, PresetBundle* preset_bundle/* = nullptr*/)
    : DPIDialog(parent, wxID_ANY, types.size() == 1 ? _L("Save preset") : _L("Save presets"), 
                wxDefaultPosition, wxSize(45 * wxGetApp().em_unit(), 5 * wxGetApp().em_unit()), wxDEFAULT_DIALOG_STYLE | wxICON_WARNING),
    m_preset_bundle(preset_bundle)
{
    build(types, suffix, template_filament);
}

SavePresetDialog::SavePresetDialog(wxWindow* parent, Preset::Type type, const wxString& info_line_extention)
    : DPIDialog(parent, wxID_ANY, _L("common_popup_presetrename_title"), wxDefaultPosition, wxSize(45 * wxGetApp().em_unit(), 5 * wxGetApp().em_unit()), wxDEFAULT_DIALOG_STYLE | wxICON_WARNING),
    m_use_for_rename(true),
    m_info_line_extention(info_line_extention)
{
    build(std::vector<Preset::Type>{type});
}
SavePresetDialog::~SavePresetDialog()
{
    for (auto  item : m_items) {
        delete item;
    }
}

void SavePresetDialog::build(std::vector<Preset::Type> types, std::string suffix, bool template_filament)
{
    this->SetFont(wxGetApp().normal_font());

#if defined(__WXMSW__)
    // ys_FIXME! temporary workaround for correct font scaling
    // Because of from wxWidgets 3.1.3 auto rescaling is implemented for the Fonts,
    // From the very beginning set dialog font to the wxSYS_DEFAULT_GUI_FONT
//    this->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
#else
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif // __WXMSW__

    if (suffix.empty())
        // TRN Suffix for the preset name. Have to be a noun.
        suffix = _CTX_utf8(L_CONTEXT("Copy", "PresetName"), "PresetName");

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    m_presets_sizer = new wxBoxSizer(wxVERTICAL);

    const bool is_for_multiple_save = types.size() > 1;
    for (const Preset::Type& type : types)
        AddItem(type, suffix, is_for_multiple_save);

    // Add dialog's buttons
    wxStdDialogButtonSizer* btns = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    wxButton* btnOK = static_cast<wxButton*>(this->FindWindowById(wxID_OK, this));
    btnOK->Bind(wxEVT_BUTTON,    [this](wxCommandEvent&)        { accept(); });
    btnOK->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& evt)   { evt.Enable(enable_ok_btn()); });

    topSizer->Add(m_presets_sizer,  0, wxEXPAND | wxALL, BORDER_W);
    
    // Add checkbox for Template filament saving
    if (template_filament && types.size() == 1 && *types.begin() == Preset::Type::TYPE_FILAMENT) {
        m_template_filament_checkbox = new wxCheckBox(this, wxID_ANY, _L("Save as profile derived from current printer only."));
        wxBoxSizer* check_sizer = new wxBoxSizer(wxVERTICAL);
        check_sizer->Add(m_template_filament_checkbox);
        topSizer->Add(check_sizer, 0, wxEXPAND | wxALL, BORDER_W);
    }

    topSizer->Add(btns,             0, wxEXPAND | wxALL, BORDER_W);

    SetSizer(topSizer);
    topSizer->SetSizeHints(this);

    this->CenterOnScreen();

#ifdef _WIN32
    wxGetApp().UpdateDlgDarkUI(this);
#endif
}

void SavePresetDialog::AddItem(Preset::Type type, const std::string& suffix, bool is_for_multiple_save)
{
    m_items.emplace_back(new Item{type, suffix, m_presets_sizer, this, is_for_multiple_save});
}

std::string SavePresetDialog::get_name()
{
    return m_items.front()->preset_name();
}

std::string SavePresetDialog::get_name(Preset::Type type)
{
    for (const Item* item : m_items)
        if (item->type() == type)
            return item->preset_name();
    return "";
}

bool SavePresetDialog::get_template_filament_checkbox()
{
    if (m_template_filament_checkbox)
    {
        return m_template_filament_checkbox->GetValue();
    }
    return false;
}

bool SavePresetDialog::enable_ok_btn() const
{
    for (const Item* item : m_items)
        if (!item->is_valid())
            return false;

    return true;
}

void SavePresetDialog::add_info_for_edit_ph_printer(wxBoxSizer* sizer)
{
    PhysicalPrinterCollection& printers = wxGetApp().preset_bundle->physical_printers;
    m_ph_printer_name = printers.get_selected_printer_name();
    m_old_preset_name = printers.get_selected_printer_preset_name();

    wxString msg_text = from_u8((boost::format(_u8L("You have selected physical printer \"%1%\" \n"
                                                    "with related printer preset \"%2%\"")) %
                                                    m_ph_printer_name % m_old_preset_name).str());
    m_label = new wxStaticText(this, wxID_ANY, msg_text);
    m_label->SetFont(wxGetApp().bold_font());

    m_action = ChangePreset;
    m_radio_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBox* action_stb = new wxStaticBox(this, wxID_ANY, "");
    if (!wxOSX) action_stb->SetBackgroundStyle(wxBG_STYLE_PAINT);
    action_stb->SetFont(wxGetApp().bold_font());

    wxStaticBoxSizer* stb_sizer = new wxStaticBoxSizer(action_stb, wxVERTICAL);
    for (int id = 0; id < 3; id++) {
        wxRadioButton* btn = new wxRadioButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
        btn->SetValue(id == int(ChangePreset));
        btn->Bind(wxEVT_RADIOBUTTON, [this, id](wxCommandEvent&) { m_action = (ActionType)id; });
        stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
    }
    m_radio_sizer->Add(stb_sizer, 1, wxEXPAND | wxTOP, 2*BORDER_W);

    sizer->Add(m_label,         0, wxEXPAND | wxLEFT | wxTOP,   3*BORDER_W);
    sizer->Add(m_radio_sizer,   1, wxEXPAND | wxLEFT,           3*BORDER_W);
}

void SavePresetDialog::update_info_for_edit_ph_printer(const std::string& preset_name)
{
    bool show = wxGetApp().preset_bundle->physical_printers.has_selection() && m_old_preset_name != preset_name;

    m_label->Show(show);
    m_radio_sizer->ShowItems(show);
    if (!show) {
        this->SetMinSize(wxSize(100,50));
        return;
    }

    if (wxSizerItem* sizer_item = m_radio_sizer->GetItem(size_t(0))) {
        if (wxStaticBoxSizer* stb_sizer = static_cast<wxStaticBoxSizer*>(sizer_item->GetSizer())) {
            wxString msg_text = format_wxstr(_L("What would you like to do with \"%1%\" preset after saving?"), preset_name);
            stb_sizer->GetStaticBox()->SetLabel(msg_text);

            wxString choices[] = { format_wxstr(_L("Change \"%1%\" to \"%2%\" for this physical printer \"%3%\""), m_old_preset_name, preset_name, m_ph_printer_name),
                                   format_wxstr(_L("Add \"%1%\" as a next preset for the the physical printer \"%2%\""), preset_name, m_ph_printer_name),
                                   format_wxstr(_L("Just switch to \"%1%\" preset"), preset_name) };

            size_t n = 0;
            for (const wxString& label : choices)
                stb_sizer->GetItem(n++)->GetWindow()->SetLabel(label);
        }
        Refresh();
    }
}

bool SavePresetDialog::Layout()
{
    const bool ret = DPIDialog::Layout();
    this->Fit();
    return ret;
}

void SavePresetDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_OK, wxID_CANCEL });

    for (Item* item : m_items)
        item->update_valid_bmp();

    //const wxSize& size = wxSize(45 * em, 35 * em);
    SetMinSize(/*size*/wxSize(100, 50));

    Fit();
    Refresh();
}

void SavePresetDialog::update_physical_printers(const std::string& preset_name)
{
    if (m_action == UndefAction)
        return;

    PhysicalPrinterCollection& physical_printers = wxGetApp().preset_bundle->physical_printers;
    if (!physical_printers.has_selection())
        return;

    std::string printer_preset_name = physical_printers.get_selected_printer_preset_name();

    if (m_action == Switch)
        // unselect physical printer, if it was selected
        physical_printers.unselect_printer();
    else
    {
        PhysicalPrinter printer = physical_printers.get_selected_printer();

        if (m_action == ChangePreset)
            printer.delete_preset(printer_preset_name);

        if (printer.add_preset(preset_name))
            physical_printers.save_printer(printer);

        physical_printers.select_printer(printer.get_full_name(preset_name));
    }    
}

void SavePresetDialog::accept()
{
    for (Item* item : m_items) {
        item->accept();
        if (item->type() == Preset::TYPE_PRINTER)
            update_physical_printers(item->preset_name());
    }

    EndModal(wxID_OK);
}

// add by allen for ankerCfgDlg and AnkerSavePresetDialog
//-----------------------------------------------
//          AnkerSavePresetDialog::Item
//-----------------------------------------------

std::string AnkerSavePresetDialog::Item::get_init_preset_name(const std::string& suffix)
{
    PresetBundle* preset_bundle = dynamic_cast<AnkerSavePresetDialog*>(m_parent)->get_preset_bundle();
    if (!preset_bundle)
        preset_bundle = wxGetApp().preset_bundle;
    m_presets = &preset_bundle->get_presets(m_type);

    const Preset& sel_preset = m_presets->get_selected_preset();
    std::string preset_name = sel_preset.is_default ? "Untitled" :
        sel_preset.is_system ? (boost::format(("%1% - %2%")) % sel_preset.name % suffix).str() :
        sel_preset.name;

    // if name contains extension
    if (boost::iends_with(preset_name, ".ini")) {
        size_t len = preset_name.length() - 4;
        preset_name.resize(len);
    }

    return preset_name;
}

void AnkerSavePresetDialog::Item::init_input_name_ctrl(wxBoxSizer* input_name_sizer, const std::string preset_name)
{
    std::vector<std::string> values;
    for (const Preset& preset : *m_presets) {
        if (preset.is_default || preset.is_system || preset.is_external)
            continue;
        values.push_back(preset.name);
    }

    // when should use combo but item count is less than 1, we should still use text ctrl 
    if (!m_use_text_ctrl && values.size() <= 1) {
        m_use_text_ctrl = true;
    }

    if (m_use_text_ctrl) {
#ifdef _WIN32
        long style = wxBORDER_SIMPLE;
#else
        long style = 0L;
#endif
        m_text_ctrl = new wxTextCtrl(m_parent, wxID_ANY, from_u8(preset_name), wxDefaultPosition, wxSize(35 * wxGetApp().em_unit(), -1), style);
        wxGetApp().UpdateDarkUI(m_text_ctrl);
        m_text_ctrl->SetBackgroundColour(wxColour("#434447"));
        m_text_ctrl->SetForegroundColour(wxColour("#FFFFFF"));
        m_text_ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update(); });

        input_name_sizer->Add(m_text_ctrl, 1, wxEXPAND, BORDER_W);
    }
    else {
        std::vector<std::string> values;
        for (const Preset& preset : *m_presets) {
            if (preset.is_default || preset.is_system || preset.is_external)
                continue;
            values.push_back(preset.name);
        }

        //m_combo = new wxComboBox(m_parent, wxID_ANY, from_u8(preset_name), wxDefaultPosition, wxSize(35 * wxGetApp().em_unit(), -1));
        m_combo = new AnkerSimpleCombox();
        m_combo->Create(m_parent,
            wxID_ANY,
            from_u8(preset_name),
            wxDefaultPosition,
            wxSize(352, AnkerLength(21)),
            wxNO_BORDER,
            wxDefaultValidator,
            "");
        m_combo->SetFont(ANKER_FONT_NO_1);
        m_combo->SetBackgroundColour(ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR);
        m_combo->setColor(wxColour("#434447"), ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR);
        m_combo->SetForegroundColour(wxColour("#FFFFFF"));
        wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
        btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
        wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
        wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
        wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));

        m_combo->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

        for (const std::string& value : values)
            m_combo->Append(from_u8(value));

        m_combo->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update(); });
#ifdef __WXOSX__
        // Under OSX wxEVT_TEXT wasn't invoked after change selection in combobox,
        // So process wxEVT_COMBOBOX too
        m_combo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) { update(); });
#endif //__WXOSX__

        input_name_sizer->Add(m_combo, 1, wxEXPAND, BORDER_W);
    }
}

AnkerSavePresetDialog::Item::Item(Preset::Type type, const std::string& suffix, wxBoxSizer* sizer, AnkerSavePresetDialog* parent, bool is_for_multiple_save) :
    m_type(type),
    m_use_text_ctrl(parent->is_for_rename()),  //mod by allen,use text ctrl instead of combox
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    m_valid_label->SetFont(wxGetApp().bold_font());
    m_valid_label->SetForegroundColour(wxColour("#FF0E00"));

    wxStaticText* label_top = is_for_multiple_save ? new wxStaticText(m_parent, wxID_ANY, _(TOP_LABELS.at(m_type)) + ":") : nullptr;

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, get_init_preset_name(suffix));

    if (label_top)
        sizer->Add(label_top, 0, wxEXPAND | wxTOP | wxBOTTOM, BORDER_W);
    sizer->Add(input_name_sizer, 0, wxEXPAND | (label_top ? 0 : wxTOP) | wxBOTTOM, BORDER_W);
     sizer->Add(m_valid_label, 1, wxEXPAND | wxLEFT, 3 * BORDER_W);

    if (m_type == Preset::TYPE_PRINTER)
        parent->add_info_for_edit_ph_printer(sizer);

    update();
}

AnkerSavePresetDialog::Item::Item(wxWindow* parent, wxBoxSizer* sizer, const std::string& def_name, PrinterTechnology pt /*= ptFFF*/) :
    m_preset_name(def_name),
    m_printer_technology(pt),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    m_valid_label->SetFont(wxGetApp().bold_font());
    m_valid_label->SetForegroundColour(wxColour("#FFFFFF"));
    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, m_preset_name);

    sizer->Add(input_name_sizer, 0, wxEXPAND | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label, 1, wxEXPAND | wxLEFT, 3 * BORDER_W);

    update();
}

const Preset* AnkerSavePresetDialog::Item::get_existing_preset() const
{
    if (m_presets)
        return m_presets->find_preset(m_preset_name, false);

    for (const Preset::Type& type : PresetBundle::types_list(m_printer_technology)) {
        const PresetCollection& presets = wxGetApp().preset_bundle->get_presets(type);
        if (const Preset* preset = presets.find_preset(m_preset_name, false))
            return preset;
    }

    return nullptr;
}

void AnkerSavePresetDialog::Item::update()
{
    m_preset_name = into_u8(m_use_text_ctrl ? m_text_ctrl->GetValue() : m_combo->GetValue());

    m_valid_type = ValidationType::Valid;
    wxString info_line;

    const char* unusable_symbols = "<>[]:/\\|?*\"";

    const std::string unusable_suffix = PresetCollection::get_suffix_modified();//"(modified)";
    for (size_t i = 0; i < std::strlen(unusable_symbols); i++) {
        if (m_preset_name.find_first_of(unusable_symbols[i]) != std::string::npos) {
            info_line = _L("The supplied name is not valid;") + "\n" +
                _L("the following characters are not allowed:") + " " + unusable_symbols;
            m_valid_type = ValidationType::NoValid;
            break;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find(unusable_suffix) != std::string::npos) {
        info_line = _L("The supplied name is not valid;") + "\n" +
            _L("the following suffix is not allowed:") + "\n\t" +
            from_u8(unusable_suffix);
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name == "- default -") {
        info_line = _L("The supplied name is not available.");
        m_valid_type = ValidationType::NoValid;
    }

    const Preset* existing = get_existing_preset();
    if (m_valid_type == ValidationType::Valid && existing && (existing->is_default || existing->is_system)) {
        info_line = m_use_text_ctrl ? _L("The supplied name is used for a system profile.") :
            _L("Cannot overwrite a system profile.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && existing && (existing->is_external)) {
        info_line = m_use_text_ctrl ? _L("The supplied name is used for a external profile.") :
            _L("Cannot overwrite an external profile.");
        m_valid_type = ValidationType::NoValid;
    }

    AnkerSavePresetDialog* dlg = dynamic_cast<AnkerSavePresetDialog*>(m_parent);
    if (m_valid_type == ValidationType::Valid && existing)
    {
        if (m_presets && m_preset_name == m_presets->get_selected_preset_name()) {
            if ((!m_use_text_ctrl && m_presets->get_edited_preset().is_dirty) ||
                (dlg && dlg->get_preset_bundle())) // means that we save modifications from the DiffDialog
                info_line = _L("Save preset modifications to existing user profile");
            m_valid_type = ValidationType::Valid;
        }
        else {
            if (existing->is_compatible)
                info_line = from_u8((boost::format(_u8L("Preset with name \"%1%\" already exists.")) % m_preset_name).str());
            else
                info_line = from_u8((boost::format(_u8L("Preset with name \"%1%\" already exists and is incompatible with selected printer.")) % m_preset_name).str());
            info_line += "\n" + _L("Note: This preset will be replaced after saving");
            m_valid_type = ValidationType::Warning;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.empty()) {
        info_line = _L("The name cannot be empty.");
        m_valid_type = ValidationType::NoValid;
    }

#ifdef __WXMSW__
    const int max_path_length = MAX_PATH;
#else
    const int max_path_length = 255;
#endif

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->path_from_name(m_preset_name).length() >= max_path_length) {
        info_line = _L("The name is too long.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_first_of(' ') == 0) {
        info_line = _L("The name cannot start with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_last_of(' ') == m_preset_name.length() - 1) {
        info_line = _L("The name cannot end with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->get_preset_name_by_alias(m_preset_name) != m_preset_name) {
        info_line = _L("The name cannot be the same as a preset alias name.");
        m_valid_type = ValidationType::NoValid;
    }

    if ((dlg && !dlg->get_info_line_extention().IsEmpty()) && m_valid_type != ValidationType::NoValid)
        info_line += "\n\n" + dlg->get_info_line_extention();

    wxString info_line_wrap;
    wxArrayString arrayStr = wxStringTokenize(info_line, '\n');
    for (size_t i = 0; i < arrayStr.GetCount(); ++i) {
        wxString str = arrayStr.Item(i);
        wxString str_wrap = Slic3r::GUI::WrapEveryCharacter(str, wxGetApp().bold_font(), m_parent->GetSize().GetWidth() - 6 * BORDER_W);
        if (info_line_wrap.empty())
            info_line_wrap = str_wrap;
        else
            info_line_wrap = info_line_wrap + "\n" + str_wrap;
    }


    m_valid_label->Wrap(m_parent->GetSize().GetWidth() - 5 * BORDER_W);
    m_valid_label->SetLabel(info_line_wrap);
    m_valid_label->SetForegroundColour(wxColour("#FF0E00"));
    m_valid_label->Show(!info_line.IsEmpty());
    if (!info_line.IsEmpty() &&  m_parent) {
        m_parent->SetSize(AnkerSize(500, -1));
        m_parent->Fit();
        m_parent->Layout();
    }

    update_valid_bmp();

    if (dlg && m_type == Preset::TYPE_PRINTER)
        dlg->update_info_for_edit_ph_printer(m_preset_name);

    m_parent->Layout();
}

void AnkerSavePresetDialog::Item::update_valid_bmp()
{
    std::string bmp_name = m_valid_type == ValidationType::Warning ? "exclamation_manifold" :
        m_valid_type == ValidationType::NoValid ? "exclamation" : "tick_mark";
    m_valid_bmp->SetBitmap(*get_bmp_bundle(bmp_name));
}

void AnkerSavePresetDialog::Item::accept()
{
    if (m_valid_type == ValidationType::Warning)
        m_presets->delete_preset(m_preset_name);
}

void AnkerSavePresetDialog::Item::Enable(bool enable /*= true*/)
{
    m_valid_label->Enable(enable);
    m_valid_bmp->Enable(enable);
    m_use_text_ctrl ? m_text_ctrl->Enable(enable) : m_combo->Enable(enable);
}


//-----------------------------------------------
//          AnkerSavePresetDialog
//-----------------------------------------------

AnkerSavePresetDialog::AnkerSavePresetDialog(wxWindow* parent, std::vector<Preset::Type> types, std::string suffix, bool template_filament/* =false*/, PresetBundle* preset_bundle/* = nullptr*/)
    : AnkerDPIDialog(parent, wxID_ANY, types.size() == 1 ? _L("Save preset") : _L("Save presets"),
        wxDefaultPosition, wxSize(AnkerSize(500, -1)), wxBORDER_SIMPLE),
    m_preset_bundle(preset_bundle)
{
    m_strTitle = _L("Save preset");
    build(types, suffix, template_filament);
    CenterOnParent();
}

AnkerSavePresetDialog::AnkerSavePresetDialog(wxWindow* parent, Preset::Type type, const wxString& info_line_extention)
    : AnkerDPIDialog(parent, wxID_ANY, _L("common_popup_presetrename_title"), wxDefaultPosition, wxSize(AnkerSize(500, -1)), wxBORDER_SIMPLE),
    m_use_for_rename(true),
    m_info_line_extention(info_line_extention)
{
    m_strTitle = _L("common_popup_presetrename_title");
    build(std::vector<Preset::Type>{type});
    CenterOnParent();
}

AnkerSavePresetDialog::~AnkerSavePresetDialog()
{
    for (auto item : m_items) {
        delete item;
    }
}

void AnkerSavePresetDialog::build(std::vector<Preset::Type> types, std::string suffix, bool template_filament)
{
    this->SetFont(wxGetApp().normal_font());

    // add by allen for ankerCfgDlg AnkerSavePresetDialog
    SetBackgroundColour(ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR);
//#if defined(__WXMSW__)
//    // ys_FIXME! temporary workaround for correct font scaling
//    // Because of from wxWidgets 3.1.3 auto rescaling is implemented for the Fonts,
//    // From the very beginning set dialog font to the wxSYS_DEFAULT_GUI_FONT
////    this->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
//#else
//    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
//#endif // __WXMSW__

    if (suffix.empty())
        // TRN Suffix for the preset name. Have to be a noun.
        suffix = _CTX_utf8(L_CONTEXT("Copy", "PresetName"), "PresetName");

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    // titleHSizer
    {
        m_titlePanel = new wxPanel(this);
        m_titlePanel->SetSize(AnkerSize(500, 40));
        topSizer->Add(m_titlePanel, 0, wxEXPAND | wxALL, 0);

        wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
        m_titlePanel->SetSizer(titleHSizer);

        // title text
        auto titleText = new wxStaticText(m_titlePanel, wxID_ANY, m_strTitle);
        titleText->SetBackgroundColour(ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR);
        titleText->SetForegroundColour(wxColour("#FFFFFF"));
        titleText->SetFont(ANKER_FONT_NO_1);
        

        int iCloseBtnWidth = 30;
        int iSpaceLength = 500 - iCloseBtnWidth;
        wxClientDC dc(titleText);
        wxSize textSize = dc.GetTextExtent(m_strTitle);
        int iMaxTextWidth = 500 - 30 - 5;
        int iTextWith = (textSize.GetWidth() > (iMaxTextWidth)) ? iMaxTextWidth : textSize.GetWidth();
        iSpaceLength = (iSpaceLength - iTextWith) / 2;

        titleText->SetMinSize(wxSize(iTextWith, -1));
        titleText->SetMaxSize(wxSize(iTextWith, -1));

        wxPanel* emptyPanel1 = new wxPanel(this);
       // emptyPanel1->SetBackgroundColour(wxColor(255,0,0));
        emptyPanel1->SetMinSize(wxSize(iSpaceLength,40));
        emptyPanel1->SetMaxSize(wxSize(iSpaceLength, 40));
        titleHSizer->Add(emptyPanel1,0, wxALL,0);
        titleHSizer->Add(titleText, 0, wxALIGN_CENTER_VERTICAL, 0);
        wxPanel* emptyPanel2 = new wxPanel(this);
        //emptyPanel2->SetBackgroundColour(wxColor(255, 0, 0));
        emptyPanel2->SetMinSize(wxSize(iSpaceLength, 40));
        emptyPanel2->SetMaxSize(wxSize(iSpaceLength, 40));
        titleHSizer->Add(emptyPanel2, 0, wxALL, 0);

        // exit button
        auto exitBtn = new ScalableButton(m_titlePanel, wxID_ANY, "ankerConfigDialogExit", "", wxSize(20, 20));
        exitBtn->SetWindowStyleFlag(wxBORDER_NONE);
        exitBtn->SetBackgroundColour(ANKER_SAVEPRESET_DIALOG_BACKGROUD_COLOUR);
        exitBtn->SetForegroundColour(wxColour("#FFFFFF"));
        exitBtn->Bind(wxEVT_BUTTON, &AnkerSavePresetDialog::OnExitButtonClicked, this);
        titleHSizer->Add(exitBtn, 0,  wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 10);

        // split line
        wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        splitLineCtrl->SetBackgroundColour(wxColour("#545863"));
        splitLineCtrl->SetMaxSize(wxSize(1000, 1));
        splitLineCtrl->SetMinSize(wxSize(500, 1));
        topSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALL, 0);
    }

    m_presets_sizer = new wxBoxSizer(wxVERTICAL);

    const bool is_for_multiple_save = types.size() > 1;
    for (const Preset::Type& type : types)
        AddItem(type, suffix, is_for_multiple_save);

    topSizer->Add(m_presets_sizer, 1, wxEXPAND | wxALL, BORDER_W);

    // Add checkbox for Template filament saving
    if (template_filament && types.size() == 1 && *types.begin() == Preset::Type::TYPE_FILAMENT) {
        m_template_filament_checkbox = new wxCheckBox(this, wxID_ANY, _L("Save as profile derived from current printer only."));
        wxBoxSizer* check_sizer = new wxBoxSizer(wxVERTICAL);
        check_sizer->Add(m_template_filament_checkbox);
        topSizer->Add(check_sizer, 0, wxEXPAND | wxALL, BORDER_W);
    }

    // Add dialog's buttons
    //wxStdDialogButtonSizer* btns = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    {
        // button
        wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
        topSizer->Add(btnHSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBottom, 0);

        btnHSizer->AddSpacer(2.4 * wxGetApp().em_unit());

        m_pCancelBtn = new wxButton(this, wxID_CANCEL, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        m_pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
        m_pCancelBtn->SetLabel(_L("common_button_cancel"));
        m_pCancelBtn->SetMaxSize(wxSize(1000, 32));
#ifdef __APPLE__
        m_pCancelBtn->SetMinSize(wxSize(60, 32));
#endif
        m_pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
        //m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnCancelButtonClicked, this);
        btnHSizer->Add(m_pCancelBtn, 170, wxEXPAND | wxALIGN_CENTER | wxRIGHT, 12);

        m_pOKBtn = new wxButton(this, wxID_OK, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        m_pOKBtn->SetForegroundColour(wxColour("#FFFFFF"));
        m_pOKBtn->SetLabel(_L("common_button_ok"));
        m_pOKBtn->SetMaxSize(wxSize(1000, 32));
#ifdef __APPLE__
        m_pOKBtn->SetMinSize(wxSize(60, 32));
#endif
        m_pOKBtn->SetBackgroundColour("#62D361");
        
        m_pOKBtn->SetFont(wxGetApp().normal_font());
        btnHSizer->Add(m_pOKBtn, 170, wxEXPAND | wxALIGN_CENTER, 0);

        btnHSizer->AddSpacer(2.4 * wxGetApp().em_unit());

        topSizer->AddSpacer(1.6 * wxGetApp().em_unit());

        //wxButton* btnOK = static_cast<wxButton*>(this->FindWindowById(wxID_OK, this));
        m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerSavePresetDialog::OnExitButtonClicked, this);
        m_pCancelBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
            SetCursor(wxCursor(wxCURSOR_HAND));
            });
        m_pCancelBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
            SetCursor(wxCursor(wxCURSOR_NONE));
            });
        m_pOKBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
            SetCursor(wxCursor(wxCURSOR_HAND));
            });
        m_pOKBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
            SetCursor(wxCursor(wxCURSOR_NONE));
            });
        m_pOKBtn->Bind(wxEVT_BUTTON, &AnkerSavePresetDialog::OnOKBtnClicked, this);
        m_pOKBtn->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& evt) { evt.Enable(enable_ok_btn()); });
    }

    /*topSizer->Add(btns, 0, wxEXPAND | wxALL, BORDER_W);*/

    SetSizer(topSizer);
    topSizer->SetSizeHints(this);

    this->CenterOnScreen();
    // add by allen for ankerCfgDlg AnkerSavePresetDialog
//#ifdef _WIN32
//    wxGetApp().UpdateDlgDarkUI(this);
//#endif
}

void AnkerSavePresetDialog::AddItem(Preset::Type type, const std::string& suffix, bool is_for_multiple_save)
{
    m_items.emplace_back(new Item{ type, suffix, m_presets_sizer, this, is_for_multiple_save });
}

std::string AnkerSavePresetDialog::get_name()
{
    return m_items.front()->preset_name();
}

std::string AnkerSavePresetDialog::get_name(Preset::Type type)
{
    for (const Item* item : m_items)
        if (item->type() == type)
            return item->preset_name();
    return "";
}

bool AnkerSavePresetDialog::get_template_filament_checkbox()
{
    if (m_template_filament_checkbox)
    {
        return m_template_filament_checkbox->GetValue();
    }
    return false;
}

bool AnkerSavePresetDialog::enable_ok_btn() const
{
    for (const Item* item : m_items)
        if (!item->is_valid()) {
            m_pCancelBtn->SetBackgroundColour(wxColor("#62D361"));
            m_pCancelBtn->SetForegroundColour(wxColor("#FFFFFF"));
            m_pOKBtn->SetBackgroundColour(wxColor("#3F4044"));
            m_pOKBtn->SetForegroundColour(wxColor("#FFFFFF"));
            return false;
        }
    m_pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
    m_pCancelBtn->SetForegroundColour(wxColor("#FFFFFF"));
    m_pOKBtn->SetBackgroundColour(wxColor("#62D361"));
    m_pOKBtn->SetForegroundColour(wxColor("#FFFFFF"));
    return true;
}

void AnkerSavePresetDialog::add_info_for_edit_ph_printer(wxBoxSizer* sizer)
{
    PhysicalPrinterCollection& printers = wxGetApp().preset_bundle->physical_printers;
    m_ph_printer_name = printers.get_selected_printer_name();
    m_old_preset_name = printers.get_selected_printer_preset_name();

    wxString msg_text = from_u8((boost::format(_u8L("You have selected physical printer \"%1%\" \n"
        "with related printer preset \"%2%\"")) %
        m_ph_printer_name % m_old_preset_name).str());
    m_label = new wxStaticText(this, wxID_ANY, msg_text);
    m_label->SetFont(wxGetApp().bold_font());

    m_action = ChangePreset;
    m_radio_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBox* action_stb = new wxStaticBox(this, wxID_ANY, "");
    if (!wxOSX) action_stb->SetBackgroundStyle(wxBG_STYLE_PAINT);
    action_stb->SetFont(wxGetApp().bold_font());

    wxStaticBoxSizer* stb_sizer = new wxStaticBoxSizer(action_stb, wxVERTICAL);
    for (int id = 0; id < 3; id++) {
        wxRadioButton* btn = new wxRadioButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
        btn->SetValue(id == int(ChangePreset));
        btn->Bind(wxEVT_RADIOBUTTON, [this, id](wxCommandEvent&) { m_action = (ActionType)id; });
        stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
    }
    m_radio_sizer->Add(stb_sizer, 1, wxEXPAND | wxTOP, 2 * BORDER_W);

    sizer->Add(m_label, 0, wxEXPAND | wxLEFT | wxTOP, 3 * BORDER_W);
    sizer->Add(m_radio_sizer, 1, wxEXPAND | wxLEFT, 3 * BORDER_W);
}

void AnkerSavePresetDialog::update_info_for_edit_ph_printer(const std::string& preset_name)
{
    bool show = wxGetApp().preset_bundle->physical_printers.has_selection() && m_old_preset_name != preset_name;

    m_label->Show(show);
    m_radio_sizer->ShowItems(show);
    if (!show) {
        this->SetMinSize(wxSize(100, 50));
        return;
    }

    if (wxSizerItem* sizer_item = m_radio_sizer->GetItem(size_t(0))) {
        if (wxStaticBoxSizer* stb_sizer = static_cast<wxStaticBoxSizer*>(sizer_item->GetSizer())) {
            wxString msg_text = format_wxstr(_L("What would you like to do with \"%1%\" preset after saving?"), preset_name);
            stb_sizer->GetStaticBox()->SetLabel(msg_text);

            wxString choices[] = { format_wxstr(_L("Change \"%1%\" to \"%2%\" for this physical printer \"%3%\""), m_old_preset_name, preset_name, m_ph_printer_name),
                                   format_wxstr(_L("Add \"%1%\" as a next preset for the the physical printer \"%2%\""), preset_name, m_ph_printer_name),
                                   format_wxstr(_L("Just switch to \"%1%\" preset"), preset_name) };

            size_t n = 0;
            for (const wxString& label : choices)
                stb_sizer->GetItem(n++)->GetWindow()->SetLabel(label);
        }
        Refresh();
    }
}

bool AnkerSavePresetDialog::Layout()
{
    const bool ret = AnkerDPIDialog::Layout();
    this->Fit();
    return ret;
}

void AnkerSavePresetDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_OK, wxID_CANCEL });

    for (Item* item : m_items)
        item->update_valid_bmp();

    //const wxSize& size = wxSize(45 * em, 35 * em);
    SetMinSize(/*size*/wxSize(100, 50));

    Fit();
    Refresh();
}

void AnkerSavePresetDialog::update_physical_printers(const std::string& preset_name)
{
    if (m_action == UndefAction)
        return;

    PhysicalPrinterCollection& physical_printers = wxGetApp().preset_bundle->physical_printers;
    if (!physical_printers.has_selection())
        return;

    std::string printer_preset_name = physical_printers.get_selected_printer_preset_name();

    if (m_action == Switch)
        // unselect physical printer, if it was selected
        physical_printers.unselect_printer();
    else
    {
        PhysicalPrinter printer = physical_printers.get_selected_printer();

        if (m_action == ChangePreset)
            printer.delete_preset(printer_preset_name);

        if (printer.add_preset(preset_name))
            physical_printers.save_printer(printer);

        physical_printers.select_printer(printer.get_full_name(preset_name));
    }
}

void AnkerSavePresetDialog::accept()
{
    for (Item* item : m_items) {
        item->accept();
        if (item->type() == Preset::TYPE_PRINTER)
            update_physical_printers(item->preset_name());
    }

    EndModal(wxID_OK);
}


void AnkerSavePresetDialog::OnExitButtonClicked(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void AnkerSavePresetDialog::OnOKBtnClicked(wxCommandEvent& event) {
    accept();
}

}}    // namespace Slic3r::GUI

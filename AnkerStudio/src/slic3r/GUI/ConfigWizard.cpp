// FIXME: extract absolute units -> em

#include "ConfigWizard_private.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include <unordered_map>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dcclient.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <wx/listbook.h>
#include <wx/display.h>
#include <wx/filefn.h>
#include <wx/wupdlock.h>
#include <wx/debug.h>

#ifdef WIN32
#include <wx/msw/registry.h>
#include <KnownFolders.h>
#include <Shlobj_core.h>
#endif // WIN32

#ifdef _MSW_DARK_MODE
#include <wx/msw/dark_mode.h>
#endif // _MSW_DARK_MODE

#include "libslic3r/Platform.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/Model.hpp"
#include "libslic3r/Color.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "GUI_Utils.hpp"
#include "GUI_ObjectManipulation.hpp"
#include "AnkerObjectManipulator.hpp"
#include "Field.hpp"
#include "DesktopIntegrationDialog.hpp"
#include "slic3r/Config/Snapshot.hpp"
#include "slic3r/Utils/PresetUpdater.hpp"
#include "format.hpp"
#include "MsgDialog.hpp"
#include "UnsavedChangesDialog.hpp"
#include "slic3r/Utils/AppUpdater.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "AnkerBtn.hpp"
#include "common/AnkerGUIConfig.hpp"
#include "common/AnkerMsgDialog.hpp"
#include "slic3r/Utils/DataMangerUi.hpp"
#include "AnkerNetModule/BuryDefines.h"

#if defined(__linux__) && defined(__WXGTK3__)
#define wxLinux_gtk3 true
#else
#define wxLinux_gtk3 false
#endif //defined(__linux__) && defined(__WXGTK3__)

namespace Slic3r {
namespace GUI {


using Config::Snapshot;
using Config::SnapshotDB;



// Configuration data structures extensions needed for the wizard

bool Bundle::load(fs::path source_path, BundleLocation location, bool ais_anker_bundle)
{
    this->preset_bundle = std::make_unique<PresetBundle>();
    this->location = location;
    this->is_anker_bundle = ais_anker_bundle;

    std::string path_string = source_path.string();
    // Throw when parsing invalid configuration. Only valid configuration is supposed to be provided over the air.
    auto [config_substitutions, presets_loaded] = preset_bundle->load_configbundle(
        path_string, PresetBundle::LoadConfigBundleAttribute::LoadSystem, ForwardCompatibilitySubstitutionRule::Disable);
    UNUSED(config_substitutions);
    // No substitutions shall be reported when loading a system config bundle, no substitutions are allowed.
    assert(config_substitutions.empty());
    auto first_vendor = preset_bundle->vendors.begin();
    if (first_vendor == preset_bundle->vendors.end()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("Vendor bundle: `%1%`: No vendor information defined, cannot install.") % path_string;
        return false;
    }
    if (presets_loaded == 0) {
        BOOST_LOG_TRIVIAL(error) << boost::format("Vendor bundle: `%1%`: No profile loaded.") % path_string;
        return false;
    } 

    BOOST_LOG_TRIVIAL(trace) << boost::format("Vendor bundle: `%1%`: %2% profiles loaded.") % path_string % presets_loaded;
    this->vendor_profile = &first_vendor->second;
    return true;
}

Bundle::Bundle(Bundle &&other)
    : preset_bundle(std::move(other.preset_bundle))
    , vendor_profile(other.vendor_profile)
    , location(other.location)
    , is_anker_bundle(other.is_anker_bundle)
{
    other.vendor_profile = nullptr;
}

BundleMap BundleMap::load()
{
    BundleMap res;

    const auto vendor_dir = (boost::filesystem::path(Slic3r::data_dir()) / "vendor").make_preferred();
    const auto archive_dir = (boost::filesystem::path(Slic3r::data_dir()) / "cache" / "vendor").make_preferred();
    const auto rsrc_vendor_dir = (boost::filesystem::path(resources_dir()) / "profiles" / "Anker-ini").make_preferred();

    // Load anker bundle from the datadir/vendor directory or from datadir/cache/vendor (archive) or from resources/profiles.
    boost::filesystem::path anker_bundle_path;
    BundleLocation anker_bundle_loc;
    //  Change the order @2023-06-15 by ChunLian
    if (!boost::filesystem::exists(anker_bundle_path)) {
        anker_bundle_path = (rsrc_vendor_dir / PresetBundle::PreferredVendorBundle).replace_extension(".ini");
        anker_bundle_loc = BundleLocation::IN_RESOURCES;
    }
    if (!boost::filesystem::exists(anker_bundle_path)){
        anker_bundle_path = (vendor_dir / PresetBundle::PreferredVendorBundle).replace_extension(".ini");
        anker_bundle_loc = BundleLocation::IN_VENDOR;
    }
    if (!boost::filesystem::exists(anker_bundle_path)) {
        anker_bundle_path = (archive_dir / PresetBundle::PreferredVendorBundle).replace_extension(".ini");
        anker_bundle_loc = BundleLocation::IN_ARCHIVE;
    }

    {
        Bundle anker_bundle;
        if (anker_bundle.load(std::move(anker_bundle_path), anker_bundle_loc, true))
            res.emplace(PresetBundle::PreferredVendorBundle, std::move(anker_bundle)); 
    }

    // Load the other bundles in the datadir/vendor directory
    // and then additionally from datadir/cache/vendor (archive) and resources/profiles.
    // Should we concider case where archive has older profiles than resources (shouldnt happen)?
    typedef std::pair<const fs::path&, BundleLocation> DirData;
    std::vector<DirData> dir_list { {vendor_dir, BundleLocation::IN_VENDOR},  {archive_dir, BundleLocation::IN_ARCHIVE},  {rsrc_vendor_dir, BundleLocation::IN_RESOURCES} };
    for ( auto dir : dir_list) {
        if (!fs::exists(dir.first))
            continue;
        for (const auto &dir_entry : boost::filesystem::directory_iterator(dir.first)) {
            if (Slic3r::is_ini_file(dir_entry)) {
                std::string id = dir_entry.path().stem().string();  // stem() = filename() without the trailing ".ini" part

                // Don't load this bundle if we've already loaded it.
                if (res.find(id) != res.end()) { continue; }

                Bundle bundle;
                if (bundle.load(dir_entry.path(), dir.second))
                    res.emplace(std::move(id), std::move(bundle));
            }
        }
    }

    return res;
}

Bundle& BundleMap::anker_bundle()
{
    auto it = find(PresetBundle::PreferredVendorBundle);
    if (it == end()) {
        throw Slic3r::RuntimeError( (boost::format("ConfigWizard: Internal error in BundleMap: %s not loaded") % PresetBundle::PreferredVendorBundle).str());
    }

    return it->second;
}

const Bundle& BundleMap::anker_bundle() const
{
    return const_cast<BundleMap*>(this)->anker_bundle();
}


// Printer model picker GUI control

struct PrinterPickerEvent : public wxEvent
{
    std::string vendor_id;
    std::string model_id;
    std::string variant_name;
    bool enable;

    PrinterPickerEvent(wxEventType eventType, int winid, std::string vendor_id, std::string model_id, std::string variant_name, bool enable)
        : wxEvent(winid, eventType)
        , vendor_id(std::move(vendor_id))
        , model_id(std::move(model_id))
        , variant_name(std::move(variant_name))
        , enable(enable)
    {}

    virtual wxEvent *Clone() const
    {
        return new PrinterPickerEvent(*this);
    }
};

wxDEFINE_EVENT(EVT_PRINTER_PICK, PrinterPickerEvent);

const std::string PrinterPicker::PRINTER_PLACEHOLDER = "printer_placeholder.png";

PrinterPicker::PrinterPicker(wxWindow *parent, const VendorProfile &vendor, wxString title, size_t max_cols, const AppConfig &appconfig, const ModelFilter &filter)
    : wxPanel(parent)
    , vendor_id(vendor.id)
    , width(0)
{
    wxGetApp().UpdateDarkUI(this);
    const auto &models = vendor.models;
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    const auto font_title = GetFont().MakeBold().Scaled(1.3f);
    const auto font_name = GetFont().MakeBold();
    const auto font_alt_nozzle = GetFont().Scaled(0.9f);

    // wxGrid appends widgets by rows, but we need to construct them in columns.
    // These vectors are used to hold the elements so that they can be appended in the right order.
    std::vector<wxStaticText*> titles;
    std::vector<wxStaticBitmap*> bitmaps;
    std::vector<wxPanel*> variants_panels;

    int max_row_width = 0;
    int current_row_width = 0;

    bool is_variants = false;

    const fs::path vendor_dir_path = (fs::path(Slic3r::data_dir()) / "vendor").make_preferred();
    const fs::path cache_dir_path = (fs::path(Slic3r::data_dir()) / "cache").make_preferred();
    const fs::path rsrc_dir_path = (fs::path(resources_dir()) / "profiles").make_preferred();

    // define common image resouce
    wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
    uncheckImage.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
    wxBitmap uncheckScaledBitmap(uncheckImage);
    wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check.png")), wxBITMAP_TYPE_PNG);
    checkImage.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
    wxBitmap checkScaledBitmap(checkImage);

    wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
    disuncheckImage.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
    wxBitmap disUncheckScaledBitmap(disuncheckImage);

    wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
    discheckImage.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
    wxBitmap disCheckScaledBitmap(discheckImage);

    for (const auto &model : models) {
       if (! filter(model)) { continue; }

        wxBitmap bitmap;
        int bitmap_width = 0;
        auto load_bitmap = [](const wxString& bitmap_file, wxBitmap& bitmap, int& bitmap_width) {
            bitmap.LoadFile(bitmap_file, wxBITMAP_TYPE_PNG);
            bitmap_width = bitmap.GetWidth();
        };
        
        bool found = false;
        for (const fs::path& res : { rsrc_dir_path   / vendor.id / model.thumbnail
                                   , vendor_dir_path / vendor.id / model.thumbnail
                                   , cache_dir_path  / vendor.id / model.thumbnail })
        {
            if (!fs::exists(res))
                continue;
            load_bitmap(GUI::from_u8(res.string()), bitmap, bitmap_width);
            found = true;
            break;
        }
        
        if (!found) {
            BOOST_LOG_TRIVIAL(warning) << boost::format("Can't find bitmap file `%1%` for vendor `%2%`, printer `%3%`, using placeholder icon instead")
                % model.thumbnail
                % vendor.id
                % model.id;
            load_bitmap(Slic3r::var(PRINTER_PLACEHOLDER), bitmap, bitmap_width);
        }
        
        wxStaticText* title = new wxStaticText(this, wxID_ANY, from_u8(model.name), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        title->SetFont(font_name);
        title->SetForegroundColour(wxColour(255,255,255));
        const int wrap_width = std::max((int)MODEL_MIN_WRAP, bitmap_width);
        title->Wrap(wrap_width);

        current_row_width += wrap_width;
        if (titles.size() % max_cols == max_cols - 1) {
            max_row_width = std::max(max_row_width, current_row_width);
            current_row_width = 0;
        }

        titles.push_back(title);
        wxStaticBitmap* bitmap_widget = new wxStaticBitmap(this, wxID_ANY, bitmap);
        bitmaps.push_back(bitmap_widget);

        auto *variants_panel = new wxPanel(this);
        wxGetApp().UpdateDarkUI(variants_panel);
        auto *variants_sizer = new wxBoxSizer(wxVERTICAL);
        variants_panel->SetSizer(variants_sizer);
        const auto model_id = model.id;
        for (size_t i = 0; i < model.variants.size(); i++) {
            const auto &variant = model.variants[i];

            const auto label = model.technology == ptFFF
                ? format_wxstr("%1% %2% %3%", variant.name, _L("mm"), _L("common_wizard_pinter_nozlle"))
                : from_u8(model.name);

            if (i == 1) {
                auto *alt_label = new wxStaticText(variants_panel, wxID_ANY, _L("common_wizard_pinter_alter_nozlle"));
                alt_label->SetFont(font_alt_nozzle);
                alt_label->SetForegroundColour(wxColour(255,255,255));
                variants_sizer->Add(alt_label, 0, wxBOTTOM, 3);
                is_variants = true;
            }

            AnkerCheckBoxWrapper* cbox = new AnkerCheckBoxWrapper(
                (wxWindow*)variants_panel,
                uncheckScaledBitmap.ConvertToImage(),
                checkScaledBitmap.ConvertToImage(),
                disUncheckScaledBitmap.ConvertToImage(),
                disCheckScaledBitmap.ConvertToImage(),
                label, 
                model_id,
                variant.name);
            i == 0 ? cboxes.push_back(cbox) : cboxes_alt.push_back(cbox);

            const bool enabled = appconfig.get_variant(vendor.id, model_id, variant.name);
            cbox->SetValue(enabled);
            cbox->SetMinSize(wxSize(wrap_width, AnkerLength(16)));
            cbox->SetFont(font_name);
            cbox->setBgColor(wxColor(43, 43, 43));
            
            variants_sizer->Add(cbox, 0, wxBOTTOM, 3);

            cbox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, [this, cbox](wxCommandEvent &event) {
                on_checkbox(cbox, cbox->getCheckStatus());
            });
        }
        variants_panels.push_back(variants_panel);
    }

    width = std::max(max_row_width, current_row_width);
    const size_t cols = std::min(max_cols, titles.size());

    auto *printer_grid = new wxFlexGridSizer(cols, 0, 20);
    printer_grid->SetFlexibleDirection(wxVERTICAL | wxHORIZONTAL);

    if (titles.size() > 0) {
        const size_t odd_items = titles.size() % cols;

        for (size_t i = 0; i < titles.size() - odd_items; i += cols) {
            for (size_t j = i; j < i + cols; j++) { printer_grid->Add(bitmaps[j], 0, wxBOTTOM, 20); }
            for (size_t j = i; j < i + cols; j++) { printer_grid->Add(titles[j], 0, wxBOTTOM, 3); }
            for (size_t j = i; j < i + cols; j++) { printer_grid->Add(variants_panels[j]); }

            // Add separator space to multiliners
            if (titles.size() > cols) {
                for (size_t j = i; j < i + cols; j++) { printer_grid->Add(1, 30); }
            }
        }
        if (odd_items > 0) {
            const size_t rem = titles.size() - odd_items;

            for (size_t i = rem; i < titles.size(); i++) { printer_grid->Add(bitmaps[i], 0, wxBOTTOM, 20); }
            for (size_t i = 0; i < cols - odd_items; i++) { printer_grid->AddSpacer(1); }
            for (size_t i = rem; i < titles.size(); i++) { printer_grid->Add(titles[i], 0, wxBOTTOM, 3); }
            for (size_t i = 0; i < cols - odd_items; i++) { printer_grid->AddSpacer(1); }
            for (size_t i = rem; i < titles.size(); i++) { printer_grid->Add(variants_panels[i]); }
        }
    }

    auto *title_sizer = new wxBoxSizer(wxHORIZONTAL);
    if (! title.IsEmpty()) {
        auto *title_widget = new wxStaticText(this, wxID_ANY, title);
        title_widget->SetFont(font_title);
        title_widget->SetForegroundColour(wxColour(255,255,255));
        title_sizer->Add(title_widget);
    }
    //title_sizer->AddStretchSpacer();

    if (titles.size() > 1 || is_variants) {
        // It only makes sense to add the All / None buttons if there's multiple printers
        // All Standard button is added when there are more variants for at least one printer
        //auto *sel_all_std = new AnkerButton(this, wxID_ANY, titles.size() > 1 ? _L("All standard") : _L("Standard"));
        auto *sel_all = new AnkerBtn(this, wxID_ANY, wxDefaultPosition,wxSize(95, 24));
        sel_all->SetText(_L("common_button_all"));
        sel_all->SetBackgroundColour(wxColor(96, 96, 96));
        sel_all->SetRadius(4);
        auto *sel_none = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxSize(95, 24));
        sel_none->SetText(_L("common_button_clear"));
        sel_none->SetBackgroundColour(wxColor(96, 96, 96));
        sel_none->SetRadius(4);
       /* if (is_variants) 
            sel_all_std->Bind(wxEVT_BUTTON, [this](const wxCommandEvent& event) { this->select_all(true, false); });*/
        sel_all->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &event) { this->select_all(true, true); });
        sel_none->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &event) { this->select_all(false); });
     /*   if (is_variants) 
            title_sizer->Add(sel_all_std, 0, wxRIGHT, BTN_SPACING);*/

   /*     wxPanel* pEmptyPanel = new wxPanel(this);
        title_sizer->Add(pEmptyPanel, 1,wxALL);*/

        title_sizer->AddStretchSpacer();
        title_sizer->Add(sel_all, 0, wxLEFT, BTN_SPACING);
        title_sizer->Add(sel_none, 0, wxLEFT, BTN_SPACING);
      
    
       // wxGetApp().UpdateDarkUI(sel_all_std);
       // wxGetApp().UpdateDarkUI(sel_all);
        //wxGetApp().UpdateDarkUI(sel_none);

        // fill button indexes used later for buttons rescaling
        if (is_variants)
            m_button_indexes = {  sel_all->GetId(), sel_none->GetId() };
        else {
           // sel_all_std->Destroy();
            m_button_indexes = { sel_all->GetId(), sel_none->GetId() };
        }
    }

    sizer->Add(title_sizer, 0, wxEXPAND | wxBOTTOM, TITLE_CONTENT_SAPCING);
    sizer->Add(printer_grid);

    SetSizer(sizer);
    Fit();
}

PrinterPicker::PrinterPicker(wxWindow *parent, const VendorProfile &vendor, wxString title, size_t max_cols, const AppConfig &appconfig)
    : PrinterPicker(parent, vendor, std::move(title), max_cols, appconfig, [](const VendorProfile::PrinterModel&) { return true; })
{}

void PrinterPicker::select_all(bool select, bool alternates)
{
    for (const auto &cb : cboxes) {
        if (cb->GetValue() != select) {
            cb->SetValue(select);
            on_checkbox(cb, select);
        }
    }

    if (! select) { alternates = false; }

    for (const auto &cb : cboxes_alt) {
        if (cb->GetValue() != alternates) {
            cb->SetValue(alternates);
            on_checkbox(cb, alternates);
        }
    }
}

void PrinterPicker::select_one(size_t i, bool select)
{
    if (i < cboxes.size() && cboxes[i]->GetValue() != select) {
        cboxes[i]->SetValue(select);
        on_checkbox(cboxes[i], select);
    }
}

bool PrinterPicker::any_selected() const
{
    for (const auto &cb : cboxes) {
        if (cb->GetValue()) { return true; }
    }

    for (const auto &cb : cboxes_alt) {
        if (cb->GetValue()) { return true; }
    }

    return false;
}

std::set<std::string> PrinterPicker::get_selected_models() const 
{
    std::set<std::string> ret_set;

    for (const auto& cb : cboxes)
        if (cb->GetValue())
            ret_set.emplace(cb->model);

    for (const auto& cb : cboxes_alt)
        if (cb->GetValue())
            ret_set.emplace(cb->model);

    return ret_set;
}

void PrinterPicker::on_checkbox(const AnkerCheckBoxWrapper*cbox, bool checked)
{
    PrinterPickerEvent evt(EVT_PRINTER_PICK, GetId(), vendor_id, cbox->model, cbox->variant, checked);
    AddPendingEvent(evt);
}


// Wizard page base

ConfigWizardPage::ConfigWizardPage(ConfigWizard *parent, wxString title, wxString shortname, unsigned indent)
    : wxPanel(parent->p->hscroll)
    , parent(parent)
    , shortname(std::move(shortname))
    , indent(indent)
{
    wxGetApp().UpdateDarkUI(this);

    auto *sizer = new wxBoxSizer(wxVERTICAL);

    auto *text = new wxStaticText(this, wxID_ANY, std::move(title), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    const auto font = GetFont().MakeBold().Scaled(1.5);
    text->SetFont(font);
    text->SetForegroundColour(wxColour(255,255,255));
    sizer->Add(text, 0, wxALIGN_LEFT, 0);
    sizer->AddSpacer(10);
    if (shortname != _L("Welcome")) {
        auto* hline = new StaticLine(this);
        sizer->Add(hline, 0, wxEXPAND);
    }
    content = new wxBoxSizer(wxVERTICAL);
    sizer->Add(content, 0, wxEXPAND);

    SetSizer(sizer);

    // There is strange layout on Linux with GTK3, 
    // see https://github.com/prusa3d/PrusaSlicer/issues/5103 and https://github.com/prusa3d/PrusaSlicer/issues/4861
    // So, non-active pages will be hidden later, on wxEVT_SHOW, after completed Layout() for all pages 
    if (!wxLinux_gtk3)
        this->Hide();

    Bind(wxEVT_SIZE, [this](wxSizeEvent &event) {
        this->Layout();
        event.Skip();
    });
}

ConfigWizardPage::~ConfigWizardPage() {}

wxStaticText* ConfigWizardPage::append_text(wxString text)
{
    auto *widget = new wxStaticText(this, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    widget->Wrap(WRAP_WIDTH);
    widget->SetMinSize(wxSize(WRAP_WIDTH, -1));
    append(widget);
    return widget;
}

void ConfigWizardPage::append_spacer(int space)
{
    // FIXME: scaling
    content->AddSpacer(space);
}

// Wizard pages

PageWelcome::PageWelcome(ConfigWizard* parent)
    : ConfigWizardPage(parent, _L(""), _L("Welcome"))
    , config_Wizard(parent)
    , welcome_text(append_text(format_wxstr(
        _L("Hello, welcome to %s! This %s helps you with the initial configuration; just a few settings and you will be ready to print.")
        , SLIC3R_APP_NAME
        , _(ConfigWizard::name()))
    ))
    , cbox_reset(append(
        new wxCheckBox(this, wxID_ANY, _L("Remove user profiles (a snapshot will be taken beforehand)"))
    ))
    , cbox_integrate(append(
        new wxCheckBox(this, wxID_ANY, _L("Perform desktop integration (Sets this binary to be searchable by the system)."))
    ))
{

    welcome_text->Hide();
    cbox_reset->Hide();
    cbox_integrate->Hide();

    auto sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddSpacer(100);
    wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("ankermake_logo.png")), wxBITMAP_TYPE_PNG);  // AnkerStudio_192px.png
    image.Rescale(82, 78);
    wxStaticBitmap* iconImage = new wxStaticBitmap(this, wxID_ANY, image);
    iconImage->SetMinSize(image.GetSize());
    iconImage->SetMaxSize(image.GetSize());
    sizer->Add(iconImage, 0, wxALIGN_CENTER| wxEXPAND, 0);
    sizer->AddSpacer(30);

    auto text = new wxStaticText(this, wxID_ANY, _L("common_wizard_welcome_header"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    text->SetFont(ANKER_BOLD_FONT_NO_1);
    text->SetForegroundColour(wxColour("#FFFFFF"));
    text->Wrap(400);
    sizer->Add(text, 0, wxALIGN_CENTER, 0);
    sizer->AddSpacer(100);

    auto startBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    startBtn->SetMinSize(AnkerSize(200, 30));
    startBtn->SetMaxSize(AnkerSize(200, 30));
    startBtn->SetSize(AnkerSize(200, 30));
    startBtn->SetText(_L("common_button_start"));
    startBtn->SetDisableTextColor(wxColour(105, 105, 108));
    startBtn->SetBackgroundColour("#62d361");
    startBtn->SetTextColor("#FFFFFF");
    startBtn->SetRadius(5);
    startBtn->SetFont(ANKER_BOLD_FONT_NO_1);
    sizer->Add(startBtn, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    startBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        if (config_Wizard)
            config_Wizard->go_next_page();
    });

    this->content->Add(sizer,0,  wxALIGN_CENTER_HORIZONTAL);
    //wxGetApp().UpdateDarkUI(text);

}

void PageWelcome::set_run_reason(ConfigWizard::RunReason run_reason)
{
/*
    const bool data_empty = run_reason == ConfigWizard::RR_DATA_EMPTY;
    welcome_text->Show(data_empty);
    cbox_reset->Show(!data_empty);
#if defined(__linux__) && defined(SLIC3R_DESKTOP_INTEGRATION)
    if (!DesktopIntegrationDialog::is_integrated())
        cbox_integrate->Show(true);
    else
        cbox_integrate->Hide();
#else
    cbox_integrate->Hide();
#endif
*/

}


PagePrinters::PagePrinters(ConfigWizard *parent,
    wxString title,
    wxString shortname,
    const VendorProfile &vendor,
    unsigned indent,
    Technology technology)
    : ConfigWizardPage(parent, std::move(title), std::move(shortname), indent)
    , technology(technology)
    , install(false)   // only used for 3rd party vendors
{
    enum {
        COL_SIZE = 200,
    };

    AppConfig *appconfig = &this->wizard_p()->appconfig_new;

 /*   const auto families = vendor.families();

    for (const auto &family : families) {
        const auto filter = [&](const VendorProfile::PrinterModel &model) {
            return ((model.technology == ptFFF && technology & T_FFF)
                    || (model.technology == ptSLA && technology & T_SLA))
                && model.family == family;
        };

        if (std::find_if(vendor.models.begin(), vendor.models.end(), filter) == vendor.models.end()) {
            continue;
        }
    }*/

    const auto picker_title = wxString(vendor.name);
    auto* picker = new PrinterPicker(this, vendor, picker_title, MAX_COLS, *appconfig, [&](const VendorProfile::PrinterModel& model) {return true; });

    picker->Bind(EVT_PRINTER_PICK, [this, appconfig](const PrinterPickerEvent& evt) {
        appconfig->set_variant(evt.vendor_id, evt.model_id, evt.variant_name, evt.enable);
        wizard_p()->on_printer_pick(this, evt);
        });

    append(picker,1);
    printer_pickers.push_back(picker);
    has_printers = true;

}

void PagePrinters::select_all(bool select, bool alternates)
{
    for (auto picker : printer_pickers) {
        picker->select_all(select, alternates);
    }
}

int PagePrinters::get_width() const
{
    return std::accumulate(printer_pickers.begin(), printer_pickers.end(), 0,
        [](int acc, const PrinterPicker *picker) { return std::max(acc, picker->get_width()); });
}

bool PagePrinters::any_selected() const
{
    for (const auto *picker : printer_pickers) {
        if (picker->any_selected()) { return true; }
    }

    return false;
}

std::set<std::string> PagePrinters::get_selected_models()
{
    std::set<std::string> ret_set;

    for (const auto *picker : printer_pickers)
    {
        std::set<std::string> tmp_models = picker->get_selected_models();
        ret_set.insert(tmp_models.begin(), tmp_models.end());
    }

    return ret_set;
}

void PagePrinters::set_run_reason(ConfigWizard::RunReason run_reason)
{
    // comemnt by Samule, no need to select a default printer when first conifg printer
    //if (is_primary_printer_page
    //    && (run_reason == ConfigWizard::RR_DATA_EMPTY || run_reason == ConfigWizard::RR_DATA_LEGACY)
    //    && printer_pickers.size() > 0 
    //    && printer_pickers[0]->vendor_id == PresetBundle::ANKER_BUNDLE) {
    //    printer_pickers[0]->select_one(0, true);
    //}
}


const std::string PageMaterials::EMPTY;
const std::string PageMaterials::TEMPLATES = "templates";

PageMaterials::PageMaterials(ConfigWizard *parent, Materials *materials, wxString title, wxString shortname, wxString list1name)
    : ConfigWizardPage(parent, std::move(title), std::move(shortname))
    , materials(materials)
	, list_printer(new  StringList(this, wxLB_MULTIPLE))
    , list_type(new StringList(this))
    , list_vendor(new StringList(this))
    , list_profile(new PresetList(this))
{
    append_spacer(VERTICAL_SPACING);

    const int em = parent->em_unit();
    const int list_h = 30*em;


	list_printer->SetMinSize(wxSize(23*em, list_h));
    list_type->SetMinSize(wxSize(13*em, list_h));
    list_vendor->SetMinSize(wxSize(13*em, list_h));
    list_profile->SetMinSize(wxSize(23*em, list_h));



    grid = new wxFlexGridSizer(4, em/2, em);
    grid->AddGrowableCol(3, 1);
    grid->AddGrowableRow(1, 1);

	grid->Add(new wxStaticText(this, wxID_ANY, _L("Printer:")));
    grid->Add(new wxStaticText(this, wxID_ANY, list1name));
    grid->Add(new wxStaticText(this, wxID_ANY, _L("Vendor:")));
    grid->Add(new wxStaticText(this, wxID_ANY, _L("Profile:")));

	grid->Add(list_printer, 0, wxEXPAND);
    grid->Add(list_type, 0, wxEXPAND);
    grid->Add(list_vendor, 0, wxEXPAND);
    grid->Add(list_profile, 1, wxEXPAND);

    auto *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto *sel_all = new wxButton(this, wxID_ANY, _L("All"));
    auto *sel_none = new wxButton(this, wxID_ANY, _L("None"));
    btn_sizer->Add(sel_all, 0, wxRIGHT, em / 2);
    btn_sizer->Add(sel_none);

    wxGetApp().UpdateDarkUI(list_printer);
    wxGetApp().UpdateDarkUI(list_type);
    wxGetApp().UpdateDarkUI(list_vendor);
    wxGetApp().UpdateDarkUI(sel_all);
    wxGetApp().UpdateDarkUI(sel_none);

    grid->Add(new wxBoxSizer(wxHORIZONTAL));
    grid->Add(new wxBoxSizer(wxHORIZONTAL));
    grid->Add(new wxBoxSizer(wxHORIZONTAL));
    grid->Add(btn_sizer, 0, wxALIGN_RIGHT);

    append(grid, 1, wxEXPAND);

    append_spacer(VERTICAL_SPACING);

    html_window = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition,
        wxSize(60 * em, 20 * em), wxHW_SCROLLBAR_AUTO);
    append(html_window, 0, wxEXPAND);

	list_printer->Bind(wxEVT_LISTBOX, [this](wxCommandEvent& evt) {
		update_lists(list_type->GetSelection(), list_vendor->GetSelection(), evt.GetInt());
		});
    list_type->Bind(wxEVT_LISTBOX, [this](wxCommandEvent &) {
        update_lists(list_type->GetSelection(), list_vendor->GetSelection());
    });
    list_vendor->Bind(wxEVT_LISTBOX, [this](wxCommandEvent &) {
        update_lists(list_type->GetSelection(), list_vendor->GetSelection());
    });

    list_profile->Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent &evt) { select_material(evt.GetInt()); });
    list_profile->Bind(wxEVT_LISTBOX, [this](wxCommandEvent& evt) { on_material_highlighted(evt.GetInt()); });

    sel_all->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { select_all(true); });
    sel_none->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { select_all(false); });
    /*
    Bind(wxEVT_PAINT, [this](wxPaintEvent& evt) {on_paint();});

    list_profile->Bind(wxEVT_MOTION, [this](wxMouseEvent& evt) { on_mouse_move_on_profiles(evt); });
    list_profile->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& evt) { on_mouse_enter_profiles(evt); });
    list_profile->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& evt) { on_mouse_leave_profiles(evt); });
    */
    reload_presets();
    set_compatible_printers_html_window(std::vector<std::string>(), false);
}
void PageMaterials::on_paint()
{
}
void PageMaterials::on_mouse_move_on_profiles(wxMouseEvent& evt)
{
    const wxClientDC dc(list_profile);
    const wxPoint pos = evt.GetLogicalPosition(dc);
    int item = list_profile->HitTest(pos);
    on_material_hovered(item);
}
void PageMaterials::on_mouse_enter_profiles(wxMouseEvent& evt)
{}
void PageMaterials::on_mouse_leave_profiles(wxMouseEvent& evt)
{
    on_material_hovered(-1);
}
void PageMaterials::reload_presets()
{
    clear();

	list_printer->append(_L("(All)"), &EMPTY);

    const AppConfig* app_config = wxGetApp().app_config;
    if (materials->technology == T_FFF && app_config->get("no_templates") == "0")
        list_printer->append(_L("(Templates)"), &TEMPLATES);

    //list_printer->SetLabelMarkup("<b>bald</b>");
	for (const Preset* printer : materials->printers) {
		list_printer->append(printer->name, &printer->name);
	}
    sort_list_data(list_printer, true, false);
    if (list_printer->GetCount() > 0) {
        list_printer->SetSelection(0);
        sel_printers_prev.Clear();
        sel_type_prev = wxNOT_FOUND;
        sel_vendor_prev = wxNOT_FOUND;
        update_lists(0, 0, 0);
    }

    presets_loaded = true;
}

void PageMaterials::set_compatible_printers_html_window(const std::vector<std::string>& printer_names, bool all_printers)
{
    const auto bgr_clr = 
#if defined(__APPLE__)
        html_window->GetParent()->GetBackgroundColour();
#else 
#if defined(_WIN32)
        wxGetApp().get_window_default_clr();
#else
        wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
#endif
#endif
    const auto text_clr = wxGetApp().get_label_clr_default();
    const auto bgr_clr_str = encode_color(ColorRGB(bgr_clr.Red(), bgr_clr.Green(), bgr_clr.Blue()));
    const auto text_clr_str = encode_color(ColorRGB(text_clr.Red(), text_clr.Green(), text_clr.Blue()));
    wxString text;
    if (materials->technology == T_FFF && template_shown) {
        // TRN ConfigWizard: Materials : "%1%" = "Filaments"/"SLA materials"
        text = format_wxstr(_L("%1% visible for <b>(\"Template\")</b> printer are universal profiles available for all printers. These might not be compatible with your printer."), materials->technology == T_FFF ? _L("Filaments") : _L("SLA materials"));
    } else {
        // TRN ConfigWizard: Materials : "%1%" = "Filaments"/"SLA materials"
        wxString first_line = format_wxstr(_L("%1% marked with <b>*</b> are <b>not</b> compatible with some installed printers."), materials->technology == T_FFF ? _L("Filaments") : _L("SLA materials"));

        if (all_printers) {
            // TRN ConfigWizard: Materials : "%1%" = "filament"/"SLA material"
            wxString second_line = format_wxstr(_L("All installed printers are compatible with the selected %1%."), materials->technology == T_FFF ? _L("filament") : _L("SLA material"));
            text = wxString::Format(
                "<html>"
                "<style>"
                "table{border-spacing: 1px;}"
                "</style>"
                "<body bgcolor= %s>"
                "<font color=%s>"
                "<font size=\"3\">"
                "%s<br /><br />%s"
                "</font>"
                "</font>"
                "</body>"
                "</html>"
                , bgr_clr_str
                , text_clr_str
                , first_line
                , second_line
            );
        }
        else {
            wxString second_line;
            if (!printer_names.empty())
                second_line = (materials->technology == T_FFF ?
                    _L("Only the following installed printers are compatible with the selected filaments") :
                    _L("Only the following installed printers are compatible with the selected SLA materials")) + ":";
            text = wxString::Format(
                "<html>"
                "<style>"
                "table{border-spacing: 1px;}"
                "</style>"
                "<body bgcolor= %s>"
                "<font color=%s>"
                "<font size=\"3\">"
                "%s<br /><br />%s"
                "<table>"
                "<tr>"
                , bgr_clr_str
                , text_clr_str
                , first_line
                , second_line);
            for (size_t i = 0; i < printer_names.size(); ++i)
            {
                text += wxString::Format("<td>%s</td>", boost::nowide::widen(printer_names[i]));
                if (i % 3 == 2) {
                    text += wxString::Format(
                        "</tr>"
                        "<tr>");
                }
            }
            text += wxString::Format(
                "</tr>"
                "</table>"
                "</font>"
                "</font>"
                "</body>"
                "</html>"
            );
        }
    }
       
   
    wxFont font = get_default_font_for_dpi(this, get_dpi_for_window(this));
    const int fs = font.GetPointSize();
    int size[] = { fs,fs,fs,fs,fs,fs,fs };
    html_window->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
    html_window->SetPage(text);
}

void PageMaterials::clear_compatible_printers_label()
{
    set_compatible_printers_html_window(std::vector<std::string>(), false);
}

void PageMaterials::on_material_hovered(int sel_material)
{

}

void PageMaterials::on_material_highlighted(int sel_material)
{
    if (sel_material == last_hovered_item)
        return;
    if (sel_material == -1) {
        clear_compatible_printers_label();
        return;
    }
    last_hovered_item = sel_material;
    std::vector<std::string> tabs;
    tabs.push_back(std::string());
    tabs.push_back(std::string());
    tabs.push_back(std::string());
    //selected material string
    std::string material_name = list_profile->get_data(sel_material);
    // get material preset
    const std::vector<const Preset*> matching_materials = materials->get_presets_by_alias(material_name);
    if (matching_materials.empty())
    {
        clear_compatible_printers_label();
        return;
    }
    //find matching printers
    std::vector<std::string> names;
    for (const Preset* printer : materials->printers) {
        for (const Preset* material : matching_materials) {
            if (material->vendor && material->vendor->templates_profile)
                continue;
            if (is_compatible_with_printer(PresetWithVendorProfile(*material, material->vendor), PresetWithVendorProfile(*printer, printer->vendor))) {
                names.push_back(printer->name);
                break;
            }
        }
    }
    set_compatible_printers_html_window(names, names.size() == materials->printers.size());
}

void PageMaterials::update_lists(int sel_type, int sel_vendor, int last_selected_printer/* = -1*/)
{
	wxWindowUpdateLocker freeze_guard(this);
	(void)freeze_guard;

	wxArrayInt sel_printers;
	int sel_printers_count = list_printer->GetSelections(sel_printers);

    bool templates_available = list_printer->size() > 1 && list_printer->get_data(1) == TEMPLATES;

    // Does our wxWidgets version support operator== for wxArrayInt ?
    // https://github.com/prusa3d/PrusaSlicer/issues/5152#issuecomment-787208614
#if wxCHECK_VERSION(3, 1, 1)
    if (sel_printers != sel_printers_prev) {
#else
    auto are_equal = [](const wxArrayInt& arr_first, const wxArrayInt& arr_second) {
        if (arr_first.GetCount() != arr_second.GetCount())
            return false;
        for (size_t i = 0; i < arr_first.GetCount(); i++)
            if (arr_first[i] != arr_second[i])
                return false;
        return true;
    };
    if (!are_equal(sel_printers, sel_printers_prev)) {
#endif
        template_shown = false;
        // Refresh type list
		list_type->Clear();
		list_type->append(_L("(All)"), &EMPTY);
		if (sel_printers_count > 1) {
            // If all is selected with other printers
            // unselect "all" or all printers depending on last value
            // same with "templates" 
            if (sel_printers[0] == 0 && sel_printers_count > 1) {
                if (last_selected_printer == 0) {
                    list_printer->SetSelection(wxNOT_FOUND);
                    list_printer->SetSelection(0);
                } else {
                    list_printer->SetSelection(0, false);
                    sel_printers_count = list_printer->GetSelections(sel_printers);
                }
            }
            if (materials->technology == T_FFF && templates_available && (sel_printers[0] == 1 || sel_printers[1] == 1) && sel_printers_count > 1) {
                if (last_selected_printer == 1) {
                    list_printer->SetSelection(wxNOT_FOUND);
                    list_printer->SetSelection(1);
                }
                else  if (last_selected_printer != 0) {
                    list_printer->SetSelection(1, false);
                    sel_printers_count = list_printer->GetSelections(sel_printers);
                }
            }
        }
        if (sel_printers_count > 0 && sel_printers[0] != 0 && ((materials->technology == T_FFF && templates_available && sel_printers[0] != 1) || materials->technology != T_FFF || !templates_available)) {
            for (int i = 0; i < sel_printers_count; i++) {
                const std::string& printer_name = list_printer->get_data(sel_printers[i]);
                const Preset* printer = nullptr;
                for (const Preset* it : materials->printers) {
                    if (it->name == printer_name) {
                        printer = it;
                        break;
                    }
                }
                materials->filter_presets(printer, printer_name, EMPTY, EMPTY, [this](const Preset* p) {
                    const std::string& type = this->materials->get_type(p);
                    if (list_type->find(type) == wxNOT_FOUND) {
                        list_type->append(type, &type);
                    }
                    });
            }
        }
        else if (sel_printers_count > 0 && last_selected_printer == 0) {
            //clear selection except "ALL"
            list_printer->SetSelection(wxNOT_FOUND);
            list_printer->SetSelection(0);
            sel_printers_count = list_printer->GetSelections(sel_printers);

            materials->filter_presets(nullptr, EMPTY, EMPTY, EMPTY, [this](const Preset* p) {
                const std::string& type = this->materials->get_type(p);
                if (list_type->find(type) == wxNOT_FOUND) {
                    list_type->append(type, &type);
                }
                });
        }
        else if (materials->technology == T_FFF && templates_available && sel_printers_count > 0 && last_selected_printer == 1) {
            //clear selection except "TEMPLATES"
            list_printer->SetSelection(wxNOT_FOUND);
            list_printer->SetSelection(1);
            sel_printers_count = list_printer->GetSelections(sel_printers);
            template_shown = true;
            materials->filter_presets(nullptr, TEMPLATES, EMPTY, EMPTY, 
                [this](const Preset* p) {
                    const std::string& type = this->materials->get_type(p);
                    if (list_type->find(type) == wxNOT_FOUND) {
                        list_type->append(type, &type);
                    }
                });
        }
        sort_list_data(list_type, true, true);

		sel_printers_prev = sel_printers;
		sel_type = 0;
		sel_type_prev = wxNOT_FOUND;
		list_type->SetSelection(sel_type);
		list_profile->Clear();
	}
	
	if (sel_type != sel_type_prev) {
		// Refresh vendor list

		// XXX: The vendor list is created with quadratic complexity here,
		// but the number of vendors is going to be very small this shouldn't be a problem.

		list_vendor->Clear();
		list_vendor->append(_L("(All)"), &EMPTY);
		if (sel_printers_count != 0 && sel_type != wxNOT_FOUND) {
			const std::string& type = list_type->get_data(sel_type);
			// find printer preset
            for (int i = 0; i < sel_printers_count; i++) {
				const std::string& printer_name = list_printer->get_data(sel_printers[i]);
				const Preset* printer = nullptr;
				for (const Preset* it : materials->printers) {
					if (it->name == printer_name) {
						printer = it;
						break;
					}
				}
				materials->filter_presets(printer, printer_name, type, EMPTY, [this](const Preset* p) {
					const std::string& vendor = this->materials->get_vendor(p);
					if (list_vendor->find(vendor) == wxNOT_FOUND) {
						list_vendor->append(vendor, &vendor);
					}
					});
			}
            sort_list_data(list_vendor, true, false);
		}

		sel_type_prev = sel_type;
		sel_vendor = 0;
		sel_vendor_prev = wxNOT_FOUND;
		list_vendor->SetSelection(sel_vendor);
		list_profile->Clear();
	}
         
	if (sel_vendor != sel_vendor_prev) {
		// Refresh material list

		list_profile->Clear();
        clear_compatible_printers_label();
		if (sel_printers_count != 0 && sel_type != wxNOT_FOUND && sel_vendor != wxNOT_FOUND) {
			const std::string& type = list_type->get_data(sel_type);
			const std::string& vendor = list_vendor->get_data(sel_vendor);
			// first printer preset
            std::vector<ProfilePrintData> to_list;
            for (int i = 0; i < sel_printers_count; i++) {
				const std::string& printer_name = list_printer->get_data(sel_printers[i]);
				const Preset* printer = nullptr;
				for (const Preset* it : materials->printers) {
					if (it->name == printer_name) {
						printer = it;
						break;
					}
				}
				materials->filter_presets(printer, printer_name, type, vendor, [this, &to_list](const Preset* p) {
					const std::string& section = materials->appconfig_section();
                    bool checked = wizard_p()->appconfig_new.has(section, p->name);
                    bool was_checked = false;

                    int cur_i = list_profile->find(p->alias);
                    if (cur_i == wxNOT_FOUND) {
                        cur_i = list_profile->append(p->alias + (materials->get_omnipresent(p) || template_shown ? "" : " *"), &p->alias);
                        to_list.emplace_back(p->alias, materials->get_omnipresent(p), checked);
                    }
                    else {
                        was_checked = list_profile->IsChecked(cur_i);
                        to_list[cur_i].checked = checked || was_checked;
                    }
                    list_profile->Check(cur_i, checked || was_checked);

					/* Update preset selection in config.
					 * If one preset from aliases bundle is selected,
					 * than mark all presets with this aliases as selected
					 * */
					if (checked && !was_checked)
						wizard_p()->update_presets_in_config(section, p->alias, true);
					else if (!checked && was_checked)
						wizard_p()->appconfig_new.set(section, p->name, "1");
					});
			}
            sort_list_data(list_profile, to_list);
		}

		sel_vendor_prev = sel_vendor;
	}
    wxGetApp().UpdateDarkUI(list_profile);
}

void PageMaterials::sort_list_data(StringList* list, bool add_All_item, bool material_type_ordering)
{
// get data from list
// sort data
// first should be <all>
// then anker profiles
// then the rest
// in alphabetical order
    
    std::vector<std::reference_wrapper<const std::string>> anker_profiles;
    std::vector<std::reference_wrapper<const std::string>> other_profiles;
    bool add_TEMPLATES_item = false;
    for (int i = 0 ; i < list->size(); ++i) {
        const std::string& data = list->get_data(i);
        if (data == EMPTY) // do not sort <all> item 
            continue;
        if (data == TEMPLATES) {// do not sort <templates> item
            add_TEMPLATES_item = true;
            continue;
        }
        if (!material_type_ordering && data.find("Anker") != std::string::npos)
            anker_profiles.push_back(data);
        else 
            other_profiles.push_back(data);
    }
    if (material_type_ordering) {
        
        const ConfigOptionDef* def = print_config_def.get("filament_type");
        size_t end_of_sorted = 0;
        for (const std::string &value : def->enum_def->values()) {
            for (size_t profs = end_of_sorted; profs < other_profiles.size(); profs++)
            {
                // find instead compare because PET vs PETG
                if (other_profiles[profs].get().find(value) != std::string::npos) {
                    //swap
                    if(profs != end_of_sorted) {
                        std::reference_wrapper<const std::string> aux = other_profiles[end_of_sorted];
                        other_profiles[end_of_sorted] = other_profiles[profs];
                        other_profiles[profs] = aux;
                    }
                    end_of_sorted++;
                    break;
                }
            }
        }
    } else {
        std::sort(anker_profiles.begin(), anker_profiles.end(), [](std::reference_wrapper<const std::string> a, std::reference_wrapper<const std::string> b) {
            return a.get() < b.get();
            });
        std::sort(other_profiles.begin(), other_profiles.end(), [](std::reference_wrapper<const std::string> a, std::reference_wrapper<const std::string> b) {
            return a.get() < b.get();
            });
    }
    
    list->Clear();
    if (add_All_item)
        list->append(_L("(All)"), &EMPTY);
    if (materials->technology == T_FFF && add_TEMPLATES_item)
        list->append(_L("(Templates)"), &TEMPLATES);
    for (const auto& item : anker_profiles)
        list->append(item, &const_cast<std::string&>(item.get()));
    for (const auto& item : other_profiles)
        list->append(item, &const_cast<std::string&>(item.get()));
    
}     

void PageMaterials::sort_list_data(PresetList* list, const std::vector<ProfilePrintData>& data)
{
    // sort data
    // then anker profiles
    // then the rest
    // in alphabetical order
    std::vector<ProfilePrintData> anker_profiles;
    std::vector<ProfilePrintData> other_profiles;
    //for (int i = 0; i < data.size(); ++i) {
    for (const auto& item : data) {
        const std::string& name = item.name;
        if (name.find("Anker") != std::string::npos)
            anker_profiles.emplace_back(item);
        else
            other_profiles.emplace_back(item);
    }
    std::sort(anker_profiles.begin(), anker_profiles.end(), [](ProfilePrintData a, ProfilePrintData b) {
        return a.name.get() < b.name.get();
        });
    std::sort(other_profiles.begin(), other_profiles.end(), [](ProfilePrintData a, ProfilePrintData b) {
        return a.name.get() < b.name.get();
        });
    list->Clear();
    for (size_t i = 0; i < anker_profiles.size(); ++i) {
        list->append(std::string(anker_profiles[i].name) + (anker_profiles[i].omnipresent || template_shown ? "" : " *"), &const_cast<std::string&>(anker_profiles[i].name.get()));
        list->Check(i, anker_profiles[i].checked);
    }
    for (size_t i = 0; i < other_profiles.size(); ++i) {
        list->append(std::string(other_profiles[i].name) + (other_profiles[i].omnipresent || template_shown ? "" : " *"), &const_cast<std::string&>(other_profiles[i].name.get()));
        list->Check(i + anker_profiles.size(), other_profiles[i].checked);
    }
}

void PageMaterials::select_material(int i)
{
    const bool checked = list_profile->IsChecked(i);

    const std::string& alias_key = list_profile->get_data(i);
    if (checked && template_shown && !notification_shown) {
        notification_shown = true;
        wxString message = _L("You have selected template filament. Please note that these filaments are available for all printers but are NOT certain to be compatible with your printer. Do you still wish to have this filament selected?\n(This message won't be displayed again.)");
        MessageDialog msg(this, message, _L("Notice"), wxYES_NO);
        if (msg.ShowModal() == wxID_NO) {
            list_profile->Check(i, false);
            return;
        }
    }
    wizard_p()->update_presets_in_config(materials->appconfig_section(), alias_key, checked);
}

void PageMaterials::select_all(bool select)
{
    wxWindowUpdateLocker freeze_guard(this);
    (void)freeze_guard;

    for (unsigned i = 0; i < list_profile->GetCount(); i++) {
        const bool current = list_profile->IsChecked(i);
        if (current != select) {
            list_profile->Check(i, select);
            select_material(i);
        }
    }
}

void PageMaterials::clear()
{
	list_printer->Clear();
    list_type->Clear();
    list_vendor->Clear();
    list_profile->Clear();
	sel_printers_prev.Clear();
    sel_type_prev = wxNOT_FOUND;
    sel_vendor_prev = wxNOT_FOUND;
    presets_loaded = false;
}

void PageMaterials::on_activate()
{
    if (! presets_loaded) {
        wizard_p()->update_materials(materials->technology);
        reload_presets();
    }
    first_paint = true;
}


const char *PageCustom::default_profile_name = "My Settings";

PageCustom::PageCustom(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Custom Printer Setup"), _L("Custom Printer1111111"))
{
    cb_custom = new wxCheckBox(this, wxID_ANY, _L("Define a custom printer profile"));
    auto *label = new wxStaticText(this, wxID_ANY, _L("Custom profile name:"));

    wxBoxSizer* profile_name_sizer = new wxBoxSizer(wxVERTICAL);
    profile_name_editor = new SavePresetDialog::Item{ this, profile_name_sizer, default_profile_name };
    profile_name_editor->Enable(false);

    cb_custom->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        profile_name_editor->Enable(custom_wanted());
        wizard_p()->on_custom_setup(custom_wanted());
    });

    append(cb_custom);
    append(label);
    append(profile_name_sizer);
}

PageUpdate::PageUpdate(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Automatic updates"), _L("Updates"))
    , version_check(true)
    , preset_update(true)
{
    const AppConfig *app_config = wxGetApp().app_config;
    auto boldfont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    boldfont.SetWeight(wxFONTWEIGHT_BOLD);

    auto *box_slic3r = new wxCheckBox(this, wxID_ANY, _L("Check for application updates"));
    box_slic3r->SetValue(app_config->get("notify_release") != "none");
    append(box_slic3r);
    append_text(wxString::Format(_L(
        "If enabled, %s checks for new application versions online. When a new version becomes available, "
         "a notification is displayed at the next application startup (never during program usage). "
         "This is only a notification mechanisms, no automatic installation is done."), SLIC3R_APP_NAME));

    append_spacer(VERTICAL_SPACING);

    auto *box_presets = new wxCheckBox(this, wxID_ANY, _L("Update built-in Presets automatically"));
    box_presets->SetValue(app_config->get_bool("preset_update"));
    append(box_presets);
    append_text(wxString::Format(_L(
        "If enabled, %s downloads updates of built-in system presets in the background."
        "These updates are downloaded into a separate temporary location."
        "When a new preset version becomes available it is offered at application startup."), SLIC3R_APP_NAME));
    const auto text_bold = _L("Updates are never applied without user's consent and never overwrite user's customized settings.");
    auto *label_bold = new wxStaticText(this, wxID_ANY, text_bold);
    label_bold->SetFont(boldfont);
    label_bold->Wrap(WRAP_WIDTH);
    append(label_bold);
    append_text(_L("Additionally a backup snapshot of the whole configuration is created before an update is applied."));

    if (PresetBundle::BoolAnkerMain) {
        this->version_check = false;
        this->preset_update = false;
    } else {
        box_slic3r->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &event) { this->version_check = event.IsChecked(); });
        box_presets->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &event) { this->preset_update = event.IsChecked(); });
    }
}



namespace DownloaderUtils
{
namespace {
#ifdef _WIN32
    wxString get_downloads_path()
    {
        wxString ret;
        PWSTR path = NULL;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path);
        if (SUCCEEDED(hr)) {
            ret = wxString(path);
        }
        CoTaskMemFree(path);
        return ret;
    }
#elif  __APPLE__
    wxString get_downloads_path()
    {
        // call objective-c implementation
        return wxString::FromUTF8(get_downloads_path_mac());
    }
#else
    wxString get_downloads_path()
    {
        wxString command = "xdg-user-dir DOWNLOAD";
        wxArrayString output;
        GUI::desktop_execute_get_result(command, output);
        if (output.GetCount() > 0) {
            return output[0];
        }
        return wxString();
    }
#endif
 }
Worker::Worker(wxWindow* parent)
: wxBoxSizer(wxHORIZONTAL)
, m_parent(parent)
{
    m_input_path = new wxTextCtrl(m_parent, wxID_ANY);
    set_path_name(get_app_config()->get("url_downloader_dest"));

    auto* path_label = new wxStaticText(m_parent, wxID_ANY, _L("Download path") + ":");

    this->Add(path_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    this->Add(m_input_path, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5);

    auto* button_path = new wxButton(m_parent, wxID_ANY, _L("Browse"));
    this->Add(button_path, 0, wxEXPAND | wxTOP | wxLEFT, 5);
    button_path->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        boost::filesystem::path chosen_dest(boost::nowide::narrow(m_input_path->GetValue()));

        wxDirDialog dialog(m_parent, _L("Choose folder") + ":", chosen_dest.string() );
        if (dialog.ShowModal() == wxID_OK)
            this->m_input_path->SetValue(dialog.GetPath());
    });

    for (wxSizerItem* item : this->GetChildren())
        if (item->IsWindow()) {
            wxWindow* win = item->GetWindow();
            wxGetApp().UpdateDarkUI(win);
        }
}

void Worker::set_path_name(wxString path)
{
    if (path.empty())
        path = boost::nowide::widen(get_app_config()->get("url_downloader_dest"));

    if (path.empty()) {
        // What should be default path? Each system has Downloads folder, that could be good one.
        // Other would be program location folder - not so good: access rights, apple bin is inside bundle...
        // default_path = boost::dll::program_location().parent_path().string();
        path = get_downloads_path();
    }

    m_input_path->SetValue(path);
}

void Worker::set_path_name(const std::string& name)
{
    if (!m_input_path)
        return;

    set_path_name(boost::nowide::widen(name));
}

} // DownLoader

PageDownloader::PageDownloader(ConfigWizard* parent)
    : ConfigWizardPage(parent, _L("Downloads from URL"), _L("Downloads"))
{
    const AppConfig* app_config = wxGetApp().app_config;
    auto boldfont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    boldfont.SetWeight(wxFONTWEIGHT_BOLD);

    append_spacer(VERTICAL_SPACING);

    auto* box_allow_downloads = new wxCheckBox(this, wxID_ANY, _L("Allow built-in downloader"));
    // TODO: Do we want it like this? The downloader is allowed for very first time the wizard is run. 
    bool box_allow_value = (app_config->has("downloader_url_registered") ? app_config->get_bool("downloader_url_registered") : true);
    box_allow_value = PresetBundle::BoolAnkerMain ? false : box_allow_value;
    box_allow_downloads->SetValue(box_allow_value);
    append(box_allow_downloads);

    // TRN ConfigWizard : Downloader : %1% = "AnkerStudio"
    append_text(format_wxstr(_L(
        "If enabled, %1% registers to start on custom URL on www.printables.com."
        " You will be able to use button with %1% logo to open models in this %1%."
        " The model will be downloaded into folder you choose bellow."
    ), SLIC3R_APP_NAME));

#ifdef __linux__
    append_text(wxString::Format(_L(
        "On Linux systems the process of registration also creates desktop integration files for this version of application."
    )));
#endif

    PresetBundle::BoolAnkerMain ? void(0) : box_allow_downloads->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) { this->m_downloader->allow(event.IsChecked()); });

    m_downloader = new DownloaderUtils::Worker(this);
    append(m_downloader);
    m_downloader->allow(box_allow_value);
}

bool PageDownloader::on_finish_downloader() const
{
    return m_downloader->on_finish();
}

bool DownloaderUtils::Worker::perform_register(const std::string& path_override/* = {}*/)
{
    boost::filesystem::path aux_dest (GUI::into_u8(path_name()));
    if (!path_override.empty())
        aux_dest = boost::filesystem::path(path_override);
    boost::system::error_code ec;
    boost::filesystem::path chosen_dest = boost::filesystem::absolute(aux_dest, ec);
    if(ec)
        chosen_dest = aux_dest;
    ec.clear();
    if (chosen_dest.empty() || !boost::filesystem::is_directory(chosen_dest, ec) || ec) {
        std::string err_msg = GUI::format("%1%\n\n%2%",_L("Chosen directory for downloads does not exist.") ,chosen_dest.string());
        BOOST_LOG_TRIVIAL(error) << err_msg;
        show_error(m_parent, err_msg);
        return false;
    }
    BOOST_LOG_TRIVIAL(info) << "Downloader registration: Directory for downloads: " << chosen_dest.string();
    wxGetApp().app_config->set("url_downloader_dest", chosen_dest.string());
#ifdef _WIN32
    // Registry key creation for "ankerstudio://" URL

    boost::filesystem::path binary_path(boost::filesystem::canonical(boost::dll::program_location()));
    // the path to binary needs to be correctly saved in string with respect to localized characters
    wxString wbinary = wxString::FromUTF8(binary_path.string());
    std::string binary_string = (boost::format("%1%") % wbinary).str();
    BOOST_LOG_TRIVIAL(info) << "Downloader registration: Path of binary: " << binary_string;

    //std::string key_string = "\"" + binary_string + "\" \"-u\" \"%1\"";
    //std::string key_string = "\"" + binary_string + "\" \"%1\"";
    std::string key_string = "\"" + binary_string + "\" \"--single-instance\" \"%1\"";

    wxRegKey key_first(wxRegKey::HKCU, "Software\\Classes\\ankerstudio");
    wxRegKey key_full(wxRegKey::HKCU, "Software\\Classes\\ankerstudio\\shell\\open\\command");
    if (!key_first.Exists()) {
        key_first.Create(false);
    }
    key_first.SetValue("URL Protocol", "");

    if (!key_full.Exists()) {
        key_full.Create(false);
    }
    //key_full = "\"C:\\Program Files\\Anker3D\\AnkerStudio\\anker-studio-console.exe\" \"%1\"";
    key_full = key_string;
#elif __APPLE__
    // Apple registers for custom url in info.plist thus it has to be already registered since build.
    // The url will always trigger opening of ankerstudio and we have to check that user has allowed it. (GUI_App::MacOpenURL is the triggered method)
#else 
    // the performation should be called later during desktop integration
    perform_registration_linux = true;
#endif
    return true;
}

void DownloaderUtils::Worker::deregister()
{
#ifdef _WIN32
    std::string key_string = "";
    wxRegKey key_full(wxRegKey::HKCU, "Software\\Classes\\ankerstudio\\shell\\open\\command");
    if (!key_full.Exists()) {
        return;
    }
    key_full = key_string;
#elif __APPLE__
    // TODO
#else 
    BOOST_LOG_TRIVIAL(debug) << "DesktopIntegrationDialog::undo_downloader_registration";
    DesktopIntegrationDialog::undo_downloader_registration();
    perform_registration_linux = false;
#endif
}

bool DownloaderUtils::Worker::on_finish() {
    AppConfig* app_config = wxGetApp().app_config;
    bool ac_value = app_config->get_bool("downloader_url_registered");
    BOOST_LOG_TRIVIAL(debug) << "PageDownloader::on_finish_downloader ac_value " << ac_value << " downloader_checked " << downloader_checked;
    if (ac_value && downloader_checked) {
        // Todo: already registered but we need to do it again
        //if (!perform_register())
        //    return false;
        app_config->set("downloader_url_registered", "1");
    } else if (!ac_value && downloader_checked) {
        // Todo: register
        //if (!perform_register())
        //    return false;
        app_config->set("downloader_url_registered", "1");
    } else if (ac_value && !downloader_checked) {
        // Todo: deregister, downloads are banned now  
        //deregister();
        app_config->set("downloader_url_registered", "0");
    } /*else if (!ac_value && !downloader_checked) {
        // not registered and we dont want to do it
        // do not deregister as other instance might be registered
    } */
    return true;
}


PageReloadFromDisk::PageReloadFromDisk(ConfigWizard* parent)
    : ConfigWizardPage(parent, _L("Reload from disk"), _L("Reload from disk"))
    , full_pathnames(false)
{
    auto* box_pathnames = new wxCheckBox(this, wxID_ANY, _L("Export full pathnames of models and parts sources into 3mf and amf files"));
    box_pathnames->SetValue(wxGetApp().app_config->get_bool("export_sources_full_pathnames"));
    append(box_pathnames);
    append_text(_L(
        "If enabled, allows the Reload from disk command to automatically find and load the files when invoked.\n"
        "If not enabled, the Reload from disk command will ask to select each file using an open file dialog."
    ));

    box_pathnames->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) { this->full_pathnames = event.IsChecked(); });
}

#ifdef _WIN32
PageFilesAssociation::PageFilesAssociation(ConfigWizard* parent)
    : ConfigWizardPage(parent, _L("Files association"), _L("Files association"))
{
    cb_3mf = new wxCheckBox(this, wxID_ANY, _L("Associate .3mf files to AnkerStudio"));
    cb_stl = new wxCheckBox(this, wxID_ANY, _L("Associate .stl files to AnkerStudio"));
//    cb_gcode = new wxCheckBox(this, wxID_ANY, _L("Associate .gcode files to AnkerStudio G-code Viewer"));

    append(cb_3mf);
    append(cb_stl);
//    append(cb_gcode);
}
#endif // _WIN32

PageMode::PageMode(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("View mode"), _L("View mode"))
{
    append_text(_L("eufyMake Studio's user interfaces comes in three variants:\nSimple, Advanced, and Expert.\n"
        "The Simple mode shows only the most frequently used settings relevant for regular 3D printing. "
        "The other two offer progressively more sophisticated fine-tuning, "
        "they are suitable for advanced and expert users, respectively."));

    radio_simple = new wxRadioButton(this, wxID_ANY, _L("Simple mode"));
    radio_advanced = new wxRadioButton(this, wxID_ANY, _L("Advanced mode"));
    radio_expert = new wxRadioButton(this, wxID_ANY, _L("Expert mode"));

    std::string mode{ "expert" };   // { "simple" }; change @2023-06-15 by ChunLian
    wxGetApp().app_config->get("", "view_mode", mode);

    if (mode == "advanced") { radio_advanced->SetValue(true); }
    else if (mode == "expert") { radio_expert->SetValue(true); }
    else { radio_simple->SetValue(true); }

    append(radio_simple);
    append(radio_advanced);
    append(radio_expert);

    append_text("\n" + _L("The size of the object can be specified in inches"));
    check_inch = new wxCheckBox(this, wxID_ANY, _L("Use inches"));
    check_inch->SetValue(wxGetApp().app_config->get_bool("use_inches"));
    append(check_inch);

    on_activate();
}

void PageMode::serialize_mode(AppConfig *app_config) const
{
    std::string mode = "";

    if (radio_simple->GetValue()) { mode = "simple"; }
    if (radio_advanced->GetValue()) { mode = "advanced"; }
    if (radio_expert->GetValue()) { mode = "expert"; }

    app_config->set("view_mode", mode);
    app_config->set("use_inches", check_inch->GetValue() ? "1" : "0");
}

PageVendors::PageVendors(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Other Vendors"), _L("Other Vendors"))
{
    const AppConfig &appconfig = this->wizard_p()->appconfig_new;

    append_text(wxString::Format(_L("Pick another vendor supported by %s"), SLIC3R_APP_NAME) + ":");

    auto boldfont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    boldfont.SetWeight(wxFONTWEIGHT_BOLD);

    for (const auto &pair : wizard_p()->bundles) {
        const VendorProfile *vendor = pair.second.vendor_profile;
        if (vendor->id == PresetBundle::ANKER_BUNDLE) { continue; }
        if (vendor && vendor->templates_profile)
            continue;

        auto *cbox = new wxCheckBox(this, wxID_ANY, vendor->name);
        cbox->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent &event) {
            wizard_p()->on_3rdparty_install(vendor, cbox->IsChecked());
        });

        const auto &vendors = appconfig.vendors();
        const bool enabled = vendors.find(pair.first) != vendors.end();
        if (enabled) {
            cbox->SetValue(true);

            auto pages = wizard_p()->pages_3rdparty.find(vendor->id);
            wxCHECK_RET(pages != wizard_p()->pages_3rdparty.end(), "Internal error: 3rd party vendor printers page not created");

            for (PagePrinters* page : { pages->second.first, pages->second.second })
                if (page) page->install = true;
        }

        append(cbox);
    }
}

PageFirmware::PageFirmware(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Firmware Type"), _L("Firmware"), 1)
    , gcode_opt(*print_config_def.get("gcode_flavor"))
    , gcode_picker(nullptr)
{
    append_text(_L("Choose the type of firmware used by your printer."));
    append_text(_(gcode_opt.tooltip));

    wxArrayString choices;
    choices.Alloc(gcode_opt.enum_def->labels().size());
    for (const auto &label : gcode_opt.enum_def->labels()) {
        choices.Add(label);
    }

    gcode_picker = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    wxGetApp().UpdateDarkUI(gcode_picker);
    const auto &enum_values = gcode_opt.enum_def->values();
    auto needle = enum_values.cend();
    if (gcode_opt.default_value) {
        needle = std::find(enum_values.cbegin(), enum_values.cend(), gcode_opt.default_value->serialize());
    }
    if (needle != enum_values.cend()) {
        gcode_picker->SetSelection(needle - enum_values.cbegin());
    } else {
        gcode_picker->SetSelection(0);
    }

    append(gcode_picker);
}

void PageFirmware::apply_custom_config(DynamicPrintConfig &config)
{
    auto sel = gcode_picker->GetSelection();
    if (sel >= 0 && (size_t)sel < gcode_opt.enum_def->labels().size()) {
        auto *opt = new ConfigOptionEnum<GCodeFlavor>(static_cast<GCodeFlavor>(sel));
        config.set_key_value("gcode_flavor", opt);
    }
}

static void focus_event(wxFocusEvent& e, wxTextCtrl* ctrl, double def_value)
{
    e.Skip();
    wxString str = ctrl->GetValue();

    const char dec_sep = is_decimal_separator_point() ? '.' : ',';
    const char dec_sep_alt = dec_sep == '.' ? ',' : '.';
    // Replace the first incorrect separator in decimal number.
    bool was_replaced = str.Replace(dec_sep_alt, dec_sep, false) != 0;

    double val = 0.0;
    if (!str.ToDouble(&val)) {
        if (val == 0.0)
            val = def_value;
        ctrl->SetValue(double_to_string(val));
        show_error(nullptr, _L("Invalid numeric input."));
        // On Windows, this SetFocus creates an invisible marker.
        //ctrl->SetFocus();
    }
    else if (was_replaced)
        ctrl->SetValue(double_to_string(val));
}

class DiamTextCtrl : public wxTextCtrl
{
public:
    DiamTextCtrl(wxWindow* parent)
    {
#ifdef _WIN32
        long style = wxBORDER_SIMPLE;
#else
        long style = 0;
#endif
        Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(Field::def_width_thinner() * wxGetApp().em_unit(), wxDefaultCoord), style);
        wxGetApp().UpdateDarkUI(this);
    }
    ~DiamTextCtrl() {}
};

PageBedShape::PageBedShape(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Bed Shape and Size"), _L("Bed Shape"), 1)
    , shape_panel(new BedShapePanel(this))
{
    append_text(_L("Set the shape of your printer's bed."));

    shape_panel->build_panel(*wizard_p()->custom_config->option<ConfigOptionPoints>("bed_shape"),
        *wizard_p()->custom_config->option<ConfigOptionString>("bed_custom_texture"),
        *wizard_p()->custom_config->option<ConfigOptionString>("bed_custom_model"));

    append(shape_panel);
}

void PageBedShape::apply_custom_config(DynamicPrintConfig &config)
{
    const std::vector<Vec2d>& points = shape_panel->get_shape();
    const std::string& custom_texture = shape_panel->get_custom_texture();
    const std::string& custom_model = shape_panel->get_custom_model();
    config.set_key_value("bed_shape", new ConfigOptionPoints(points));
    config.set_key_value("bed_custom_texture", new ConfigOptionString(custom_texture));
    config.set_key_value("bed_custom_model", new ConfigOptionString(custom_model));
}

PageBuildVolume::PageBuildVolume(ConfigWizard* parent)
    // TRN ConfigWizard : Size of possible print, related on printer size
    : ConfigWizardPage(parent, _L("Build Volume"), _L("Build Volume"), 1)
    , build_volume(new DiamTextCtrl(this))
{
    append_text(_L("Set vertical size of your printer."));

    wxString value = "250";
    build_volume->SetValue(value);

    build_volume->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { 
        double def_value = 250.0;
        double max_value = 1200.0;
        e.Skip();
        wxString str = build_volume->GetValue();

        const char dec_sep = is_decimal_separator_point() ? '.' : ',';
        const char dec_sep_alt = dec_sep == '.' ? ',' : '.';
        // Replace the first incorrect separator in decimal number.
        bool was_replaced = str.Replace(dec_sep_alt, dec_sep, false) != 0;

        double val = 0.0;
        if (!str.ToDouble(&val)) {
            val = def_value;
            build_volume->SetValue(double_to_string(val));
            show_error(nullptr, _L("Invalid numeric input."));
            //build_volume->SetFocus();
        } else if (val < 0.0) {
            val  = def_value;
            build_volume->SetValue(double_to_string(val));
            show_error(nullptr, _L("Invalid numeric input."));
            //build_volume->SetFocus();
        } else if (val > max_value) {
            val = max_value;
            build_volume->SetValue(double_to_string(val));
            show_error(nullptr, _L("Invalid numeric input."));
            //build_volume->SetFocus();
        } else if (was_replaced)
            build_volume->SetValue(double_to_string(val));
    }, build_volume->GetId());

    auto* sizer_volume = new wxFlexGridSizer(3, 5, 5);
    auto* text_volume = new wxStaticText(this, wxID_ANY, _L("Max print height") + ":");
    auto* unit_volume = new wxStaticText(this, wxID_ANY, _L("mm"));
    sizer_volume->AddGrowableCol(0, 1);
    sizer_volume->Add(text_volume, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_volume->Add(build_volume);
    sizer_volume->Add(unit_volume, 0, wxALIGN_CENTRE_VERTICAL);
    append(sizer_volume); 
}

void PageBuildVolume::apply_custom_config(DynamicPrintConfig& config)
{
    double val = 0.0;
    build_volume->GetValue().ToDouble(&val);
    auto* opt_volume = new ConfigOptionFloat(val);
    config.set_key_value("max_print_height", opt_volume);
}

PageDiameters::PageDiameters(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Filament and Nozzle Diameters"), _L("Print Diameters"), 1)
    , diam_nozzle(new DiamTextCtrl(this))
    , diam_filam (new DiamTextCtrl(this))
{
    auto *default_nozzle = print_config_def.get("nozzle_diameter")->get_default_value<ConfigOptionFloats>();
    wxString value = double_to_string(default_nozzle != nullptr && default_nozzle->size() > 0 ? default_nozzle->get_at(0) : 0.5);
    diam_nozzle->SetValue(value);

    auto *default_filam = print_config_def.get("filament_diameter")->get_default_value<ConfigOptionFloats>();
    value = double_to_string(default_filam != nullptr && default_filam->size() > 0 ? default_filam->get_at(0) : 3.0);
    diam_filam->SetValue(value);

    diam_nozzle->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { focus_event(e, diam_nozzle, 0.5); }, diam_nozzle->GetId());
    diam_filam ->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { focus_event(e, diam_filam , 3.0); }, diam_filam->GetId());

    append_text(_L("Enter the diameter of your printer's hot end nozzle."));

    auto *sizer_nozzle = new wxFlexGridSizer(3, 5, 5);
    auto *text_nozzle = new wxStaticText(this, wxID_ANY, _L("Nozzle Diameter") + ":");
    auto *unit_nozzle = new wxStaticText(this, wxID_ANY, _L("mm"));
    sizer_nozzle->AddGrowableCol(0, 1);
    sizer_nozzle->Add(text_nozzle, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_nozzle->Add(diam_nozzle);
    sizer_nozzle->Add(unit_nozzle, 0, wxALIGN_CENTRE_VERTICAL);
    append(sizer_nozzle);

    append_spacer(VERTICAL_SPACING);

    append_text(_L("Enter the diameter of your filament."));
    append_text(_L("Good precision is required, so use a caliper and do multiple measurements along the filament, then compute the average."));

    auto *sizer_filam = new wxFlexGridSizer(3, 5, 5);
    auto *text_filam = new wxStaticText(this, wxID_ANY, _L("Filament Diameter") + ":");
    auto *unit_filam = new wxStaticText(this, wxID_ANY, _L("mm"));
    sizer_filam->AddGrowableCol(0, 1);
    sizer_filam->Add(text_filam, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_filam->Add(diam_filam, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_filam->Add(unit_filam, 0, wxALIGN_CENTRE_VERTICAL);
    append(sizer_filam);
}

void PageDiameters::apply_custom_config(DynamicPrintConfig &config)
{
    double val = 0.0;
    diam_nozzle->GetValue().ToDouble(&val);
    auto *opt_nozzle = new ConfigOptionFloats(1, val);
    config.set_key_value("nozzle_diameter", opt_nozzle);

    val = 0.0;
    diam_filam->GetValue().ToDouble(&val);
    auto * opt_filam = new ConfigOptionFloats(1, val);
    config.set_key_value("filament_diameter", opt_filam);

    auto set_extrusion_width = [&config, opt_nozzle](const char *key, double dmr) {
        char buf[64]; // locales don't matter here (sprintf/atof)
        sprintf(buf, "%.2lf", dmr * opt_nozzle->values.front() / 0.4);
        config.set_key_value(key, new ConfigOptionFloatOrPercent(atof(buf), false));
    };

    set_extrusion_width("support_material_extrusion_width",   0.35);
    set_extrusion_width("top_infill_extrusion_width",		  0.40);
    set_extrusion_width("first_layer_extrusion_width",		  0.42);

    set_extrusion_width("extrusion_width",					  0.45);
    set_extrusion_width("perimeter_extrusion_width",		  0.45);
    set_extrusion_width("external_perimeter_extrusion_width", 0.45);
    set_extrusion_width("infill_extrusion_width",			  0.45);
    set_extrusion_width("solid_infill_extrusion_width",       0.45);
}

class SpinCtrlDouble: public wxSpinCtrlDouble
{
public:
    SpinCtrlDouble(wxWindow* parent)
    {
#ifdef _WIN32
        long style = wxSP_ARROW_KEYS | wxBORDER_SIMPLE;
#else
        long style = wxSP_ARROW_KEYS;
#endif
        Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style);
#ifdef _WIN32
        wxGetApp().UpdateDarkUI(this->GetText());
#endif
        this->Refresh();
    }
    ~SpinCtrlDouble() {}
};

PageTemperatures::PageTemperatures(ConfigWizard *parent)
    : ConfigWizardPage(parent, _L("Nozzle and Bed Temperatures"), _L("Temperatures"), 1)
    , spin_extr(new SpinCtrlDouble(this))
    , spin_bed (new SpinCtrlDouble(this))
{
    spin_extr->SetIncrement(5.0);
    const auto &def_extr = *print_config_def.get("temperature");
    spin_extr->SetRange(def_extr.min, def_extr.max);
    auto *default_extr = def_extr.get_default_value<ConfigOptionInts>();
    spin_extr->SetValue(default_extr != nullptr && default_extr->size() > 0 ? default_extr->get_at(0) : 200);

    spin_bed->SetIncrement(5.0);
    const auto &def_bed = *print_config_def.get("bed_temperature");
    spin_bed->SetRange(def_bed.min, def_bed.max);
    auto *default_bed = def_bed.get_default_value<ConfigOptionInts>();
    spin_bed->SetValue(default_bed != nullptr && default_bed->size() > 0 ? default_bed->get_at(0) : 0);

    append_text(_L("Enter the temperature needed for extruding your filament."));
    append_text(_L("A rule of thumb is 160 to 230 °C for PLA+, and 215 to 250 °C for ABS."));

    auto *sizer_extr = new wxFlexGridSizer(3, 5, 5);
    auto *text_extr = new wxStaticText(this, wxID_ANY, _L("Extrusion Temperature:"));
    auto *unit_extr = new wxStaticText(this, wxID_ANY, _L("°C"));
    sizer_extr->AddGrowableCol(0, 1);
    sizer_extr->Add(text_extr, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_extr->Add(spin_extr);
    sizer_extr->Add(unit_extr, 0, wxALIGN_CENTRE_VERTICAL);
    append(sizer_extr);

    append_spacer(VERTICAL_SPACING);

    append_text(_L("Enter the bed temperature needed for getting your filament to stick to your heated bed."));
    append_text(_L("A rule of thumb is 60 °C for PLA+ and 110 °C for ABS. Leave zero if you have no heated bed."));

    auto *sizer_bed = new wxFlexGridSizer(3, 5, 5);
    auto *text_bed = new wxStaticText(this, wxID_ANY, _L("Bed Temperature") + ":");
    auto *unit_bed = new wxStaticText(this, wxID_ANY, _L("°C"));
    sizer_bed->AddGrowableCol(0, 1);
    sizer_bed->Add(text_bed, 0, wxALIGN_CENTRE_VERTICAL);
    sizer_bed->Add(spin_bed);
    sizer_bed->Add(unit_bed, 0, wxALIGN_CENTRE_VERTICAL);
    append(sizer_bed);
}

void PageTemperatures::apply_custom_config(DynamicPrintConfig &config)
{
    auto *opt_extr = new ConfigOptionInts(1, spin_extr->GetValue());
    config.set_key_value("temperature", opt_extr);
    auto *opt_extr1st = new ConfigOptionInts(1, spin_extr->GetValue());
    config.set_key_value("first_layer_temperature", opt_extr1st);
    auto *opt_bed = new ConfigOptionInts(1, spin_bed->GetValue());
    config.set_key_value("bed_temperature", opt_bed);
    auto *opt_bed1st = new ConfigOptionInts(1, spin_bed->GetValue());
    config.set_key_value("first_layer_bed_temperature", opt_bed1st);
}


// Index

ConfigWizardIndex::ConfigWizardIndex(wxWindow *parent)
    : wxPanel(parent)
    , bg(ScalableBitmap(parent, "AnkerStudio_192px_transparent.png", 192))
    , bullet_black(ScalableBitmap(parent, "bullet_black.png"))
    , bullet_blue(ScalableBitmap(parent, "bullet_blue.png"))
    , bullet_white(ScalableBitmap(parent, "bullet_white.png"))
    , item_active(NO_ITEM)
    , item_hover(NO_ITEM)
    , last_page((size_t)-1)
{
#ifndef __WXOSX__ 
    SetDoubleBuffered(true);// SetDoubleBuffered exists on Win and Linux/GTK, but is missing on OSX
#endif //__WXOSX__
    SetMinSize(bg.GetSize());

    const wxSize size = GetTextExtent("m");
    em_w = size.x;
    em_h = size.y;

    Bind(wxEVT_PAINT, &ConfigWizardIndex::on_paint, this);
    Bind(wxEVT_SIZE, [this](wxEvent& e) { e.Skip(); Refresh(); });
    Bind(wxEVT_MOTION, &ConfigWizardIndex::on_mouse_move, this);

    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent &evt) {
        if (item_hover != -1) {
            item_hover = -1;
            Refresh();
        }
        evt.Skip();
    });

    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &evt) {
        if (item_hover >= 0) { go_to(item_hover); }
    });
}

wxDECLARE_EVENT(EVT_INDEX_PAGE, wxCommandEvent);

void ConfigWizardIndex::add_page(ConfigWizardPage *page)
{
    last_page = items.size();
    items.emplace_back(Item { page->shortname, page->indent, page });
    Refresh();
}

void ConfigWizardIndex::add_label(wxString label, unsigned indent)
{
    items.emplace_back(Item { std::move(label), indent, nullptr });
    Refresh();
}

ConfigWizardPage* ConfigWizardIndex::active_page() const
{
    if (item_active >= items.size()) { return nullptr; }

    return items[item_active].page;
}

void ConfigWizardIndex::go_prev()
{
    // Search for a preceiding item that is a page (not a label, ie. page != nullptr)

    if (item_active == NO_ITEM) { return; }

    for (size_t i = item_active; i > 0; i--) {
        if (items[i - 1].page != nullptr) {
            go_to(i - 1);
            return;
        }
    }
}

void ConfigWizardIndex::go_next()
{
    // Search for a next item that is a page (not a label, ie. page != nullptr)

    if (item_active == NO_ITEM) { return; }

    for (size_t i = item_active + 1; i < items.size(); i++) {
        if (items[i].page != nullptr) {
            go_to(i);
            return;
        }
    }
}

// This one actually performs the go-to op
void ConfigWizardIndex::go_to(size_t i)
{
    if (i != item_active
        && i < items.size()
        && items[i].page != nullptr) {
        auto *new_active = items[i].page;
        auto *former_active = active_page();
        if (former_active != nullptr) {
            former_active->Hide();
        }

        item_active = i;
        new_active->Show();

        wxCommandEvent evt(EVT_INDEX_PAGE, GetId());
        evt.SetString(items[i].label);
        AddPendingEvent(evt);

        Refresh();

        new_active->on_activate();
    }
}

void ConfigWizardIndex::go_to(const ConfigWizardPage *page)
{
    if (page == nullptr) { return; }

    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].page == page) {
            go_to(i);
            return;
        }
    }
}

void ConfigWizardIndex::clear()
{
    auto *former_active = active_page();
    if (former_active != nullptr) { former_active->Hide(); }

    items.clear();
    item_active = NO_ITEM;
}

void ConfigWizardIndex::on_paint(wxPaintEvent & evt)
{
    const auto size = GetClientSize();
    if (size.GetHeight() == 0 || size.GetWidth() == 0) { return; }
   
    wxPaintDC dc(this);
    
    const auto bullet_w = bullet_black.GetWidth();
    const auto bullet_h = bullet_black.GetHeight();
    const int yoff_icon = bullet_h < em_h ? (em_h - bullet_h) / 2 : 0;
    const int yoff_text = bullet_h > em_h ? (bullet_h - em_h) / 2 : 0;
    const int yinc = item_height();
   
    int index_width = 0;

    unsigned y = 0;
    for (size_t i = 0; i < items.size(); i++) {
        const Item& item = items[i];
        unsigned x = em_w/2 + item.indent * em_w;

        if (i == item_active || (item_hover >= 0 && i == (size_t)item_hover)) {
            dc.DrawBitmap(bullet_blue.get_bitmap(), x, y + yoff_icon, false);
        }
        else if (i < item_active)  { dc.DrawBitmap(bullet_black.get_bitmap(), x, y + yoff_icon, false); }
        else if (i > item_active)  { dc.DrawBitmap(bullet_white.get_bitmap(), x, y + yoff_icon, false); }

        x += + bullet_w + em_w/2;
        const auto text_size = dc.GetTextExtent(item.label);
        dc.SetTextForeground(wxGetApp().get_label_clr_default());
        dc.DrawText(item.label, x, y + yoff_text);

        y += yinc;
        index_width = std::max(index_width, (int)x + text_size.x);
    }
    
    //draw logo
    if (int y = size.y - bg.GetHeight(); y>=0) {
        dc.DrawBitmap(bg.get_bitmap(), 0, y, false);
        index_width = std::max(index_width, bg.GetWidth() + em_w / 2);
    }

    if (GetMinSize().x < index_width) {
        CallAfter([this, index_width]() {
            SetMinSize(wxSize(index_width, GetMinSize().y));
            Refresh();
        });
    }
}

void ConfigWizardIndex::on_mouse_move(wxMouseEvent &evt)
{
    const wxClientDC dc(this);
    const wxPoint pos = evt.GetLogicalPosition(dc);

    const ssize_t item_hover_new = pos.y / item_height();

    if (item_hover_new < ssize_t(items.size()) && item_hover_new != item_hover) {
        item_hover = item_hover_new;
        Refresh();
    }

    evt.Skip();
}

void ConfigWizardIndex::msw_rescale()
{
    const wxSize size = GetTextExtent("m");
    em_w = size.x;
    em_h = size.y;

    SetMinSize(bg.GetSize());

    Refresh();
}


// Materials

const std::string Materials::UNKNOWN = "(Unknown)";

void Materials::push(const Preset *preset)
{
    presets.emplace_back(preset);
    types.insert(technology & T_FFF
        ? Materials::get_filament_type(preset)
        : Materials::get_material_type(preset));
}

void  Materials::add_printer(const Preset* preset)
{
	printers.insert(preset);
}

void Materials::clear()
{
    presets.clear();
    types.clear();
	printers.clear();
    compatibility_counter.clear();
}

const std::string& Materials::appconfig_section() const
{
    return (technology & T_FFF) ? AppConfig::SECTION_FILAMENTS : AppConfig::SECTION_MATERIALS;
}

const std::string& Materials::get_type(const Preset *preset) const
{
    return (technology & T_FFF) ? get_filament_type(preset) : get_material_type(preset);
}

const std::string& Materials::get_vendor(const Preset *preset) const
{
    return (technology & T_FFF) ? get_filament_vendor(preset) : get_material_vendor(preset);
}

const std::string& Materials::get_filament_type(const Preset *preset)
{
    const auto *opt = preset->config.opt<ConfigOptionStrings>("filament_type");
    if (opt != nullptr && opt->values.size() > 0) {
        return opt->values[0];
    } else {
        return UNKNOWN;
    }
}

const std::string& Materials::get_filament_vendor(const Preset *preset)
{
    const auto *opt = preset->config.opt<ConfigOptionString>("filament_vendor");
    return opt != nullptr ? opt->value : UNKNOWN;
}

const std::string& Materials::get_material_type(const Preset *preset)
{
    const auto *opt = preset->config.opt<ConfigOptionString>("material_type");
    if (opt != nullptr) {
        return opt->value;
    } else {
        return UNKNOWN;
    }
}

const std::string& Materials::get_material_vendor(const Preset *preset)
{
    const auto *opt = preset->config.opt<ConfigOptionString>("material_vendor");
    return opt != nullptr ? opt->value : UNKNOWN;
}

// priv

static const std::unordered_map<std::string, std::pair<std::string, std::string>> legacy_preset_map {{
    { "Original Anker i3 MK2.ini",                           std::make_pair("MK2S", "0.4") },
    { "Original Anker i3 MK2 MM Single Mode.ini",            std::make_pair("MK2SMM", "0.4") },
    { "Original Anker i3 MK2 MM Single Mode 0.6 nozzle.ini", std::make_pair("MK2SMM", "0.6") },
    { "Original Anker i3 MK2 MultiMaterial.ini",             std::make_pair("MK2SMM", "0.4") },
    { "Original Anker i3 MK2 MultiMaterial 0.6 nozzle.ini",  std::make_pair("MK2SMM", "0.6") },
    { "Original Anker i3 MK2 0.25 nozzle.ini",               std::make_pair("MK2S", "0.25") },
    { "Original Anker i3 MK2 0.6 nozzle.ini",                std::make_pair("MK2S", "0.6") },
    { "Original Anker i3 MK3.ini",                           std::make_pair("MK3",  "0.4") },
}};

void ConfigWizard::priv::load_pages()
{
    wxWindowUpdateLocker freeze_guard(q);
    (void)freeze_guard;

    const ConfigWizardPage *former_active = index->active_page();

    index->clear();

    index->add_page(page_welcome);

   //PresetBundle::BoolAnkerMain ? void(0) : index->add_page(page_welcome);

    // Printers
    if (!only_sla_mode)
        index->add_page(page_fff);

    if (PresetBundle::BoolHasSLA) {
        index->add_page(page_msla);
    }
//    if (!only_sla_mode) {
//        if (PresetBundle::BoolShowOtherVendors) {
//            index->add_page(page_vendors);
//            for (const auto& pages : pages_3rdparty) {
//                for (PagePrinters* page : { pages.second.first, pages.second.second })
//                    if (page && page->install)
//                        index->add_page(page);
//            }
//        }
//
//        index->add_page(page_custom);
//        if (page_custom->custom_wanted()) {
//            index->add_page(page_firmware);
//            index->add_page(page_bed);
//            index->add_page(page_bvolume);
//            index->add_page(page_diams);
//            index->add_page(page_temps);
//        }
//   
//        // Filaments & Materials
//        if (any_fff_selected) { index->add_page(page_filaments); }
//        // Filaments page if only custom printer is selected 
//        const AppConfig* app_config = wxGetApp().app_config;
//        if (!any_fff_selected && (custom_printer_selected || custom_printer_in_bundle) && (app_config->get("no_templates") == "0")) {
//            update_materials(T_ANY);
//            index->add_page(page_filaments);
//        }
//    }
//    if (any_sla_selected) { index->add_page(page_sla_materials); }
//
//    // there should to be selected at least one printer
//    btn_finish->Enable(any_fff_selected || any_sla_selected || custom_printer_selected || custom_printer_in_bundle);
//
//    PresetBundle::BoolAnkerMain ? void(0) : index->add_page(page_update);
//    PresetBundle::BoolAnkerMain ? void(0) : index->add_page(page_downloader);
//    index->add_page(page_reload_from_disk);
//#ifdef _WIN32
//    index->add_page(page_files_association);
//#endif // _WIN32
//    index->add_page(page_mode);

    index->go_to(former_active);   // Will restore the active item/page if possible

    q->Layout();
// This Refresh() is needed to avoid ugly artifacts after printer selection, when no one vendor was selected from the very beginnig
    q->Refresh();
}

void ConfigWizard::priv::init_dialog_size()
{
    // Clamp the Wizard size based on screen dimensions

    const auto idx = wxDisplay::GetFromWindow(q);
    wxDisplay display(idx != wxNOT_FOUND ? idx : 0u);

    const auto disp_rect = display.GetClientArea();
    wxRect window_rect(
        disp_rect.x + disp_rect.width / 20,
        disp_rect.y + disp_rect.height / 20,
        9*disp_rect.width / 10,
        9*disp_rect.height / 10);

    const int width_hint = index->GetSize().GetWidth() + std::max(90 * em(), (only_sla_mode ? page_msla->get_width() : page_fff->get_width()) + 30 * em());    // XXX: magic constant, I found no better solution
    if (width_hint < window_rect.width) {
        window_rect.x += (window_rect.width - width_hint) / 2;
        window_rect.width = width_hint;
    }

    //set ConfigWizard dialog size to  AnkerSize(900,600)
    window_rect.height = AnkerLength(600);
    window_rect.width = AnkerLength(900);
    q->SetSize(window_rect);
}

void ConfigWizard::priv::load_vendors()
{
    bundles = BundleMap::load();

    // Load up the set of vendors / models / variants the user has had enabled up till now
    AppConfig *app_config = wxGetApp().app_config;
    if (! app_config->legacy_datadir()) {
        appconfig_new.set_vendors(*app_config);
    } else {
        // In case of legacy datadir, try to guess the preference based on the printer preset files that are present
        const auto printer_dir = fs::path(Slic3r::data_dir()) / "printer";
        for (auto &dir_entry : boost::filesystem::directory_iterator(printer_dir))
            if (Slic3r::is_ini_file(dir_entry)) {
                auto needle = legacy_preset_map.find(dir_entry.path().filename().string());
                if (needle == legacy_preset_map.end()) { continue; }

                const auto &model = needle->second.first;
                const auto &variant = needle->second.second;
                appconfig_new.set_variant(PresetBundle::PreferredVendorBundle, model, variant, true);
            }
    }

    for (const auto& printer : wxGetApp().preset_bundle->printers) {
        if (!printer.is_default && !printer.is_system && printer.is_visible) {
            custom_printer_in_bundle = true;
            break;
        }
    }

    // Initialize the is_visible flag in printer Presets
    for (auto &pair : bundles) {
        pair.second.preset_bundle->load_installed_printers(appconfig_new);
    }

    // Copy installed filaments and SLA material names from app_config to appconfig_new
    // while resolving current names of profiles, which were renamed in the meantime.
    for (PrinterTechnology technology : { ptFFF, ptSLA }) {
    	const std::string &section_name = (technology == ptFFF) ? AppConfig::SECTION_FILAMENTS : AppConfig::SECTION_MATERIALS;
		std::map<std::string, std::string> section_new;
		if (app_config->has_section(section_name)) {
			const std::map<std::string, std::string> &section_old = app_config->get_section(section_name);
            for (const auto& material_name_and_installed : section_old)
				if (material_name_and_installed.second == "1") {
					// Material is installed. Resolve it in bundles.
                    size_t num_found = 0;
					const std::string &material_name = material_name_and_installed.first;
				    for (auto &bundle : bundles) {
				    	const PresetCollection &materials = bundle.second.preset_bundle->materials(technology);
				    	const Preset           *preset    = materials.find_preset(material_name);
				    	if (preset == nullptr) {
				    		// Not found. Maybe the material preset is there, bu it was was renamed?
							const std::string *new_name = materials.get_preset_name_renamed(material_name);
							if (new_name != nullptr)
								preset = materials.find_preset(*new_name);
				    	}
                        if (preset != nullptr) {
                            // Materal preset was found, mark it as installed.
                            section_new[preset->name] = "1";
                            ++ num_found;
                        }
				    }
                    if (num_found == 0)
            	        BOOST_LOG_TRIVIAL(error) << boost::format("Profile %1% was not found in installed vendor Preset Bundles.") % material_name;
                    else if (num_found > 1)
            	        BOOST_LOG_TRIVIAL(error) << boost::format("Profile %1% was found in %2% vendor Preset Bundles.") % material_name % num_found;
                }
		}
        appconfig_new.set_section(section_name, section_new);
    };
}

void ConfigWizard::priv::add_page(ConfigWizardPage *page)
{
    //const int proportion = (page->shortname == _L("Filaments")) || (page->shortname == _L("SLA Materials")) ? 1 : 0;
    const int proportion = 1;
    if (_L("Welcome") == page->shortname) {
        hscroll_sizer->Add(page, proportion, wxEXPAND |wxALIGN_CENTRE_HORIZONTAL);
    }
    else {
        hscroll_sizer->Add(page, proportion, wxEXPAND);
    }
    all_pages.push_back(page);
}

void ConfigWizard::priv::enable_next(bool enable)
{
    btn_next->Enable(enable);
    btn_finish->Enable(enable);
}

void ConfigWizard::priv::set_start_page(ConfigWizard::StartPage start_page)
{
    switch (start_page) {
        case ConfigWizard::SP_PRINTERS: 
            index->go_to(page_fff); 
            btn_next->SetFocus();
            break;
        case ConfigWizard::SP_FILAMENTS:
            index->go_to(page_filaments);
            btn_finish->SetFocus();
            break;
        case ConfigWizard::SP_MATERIALS:
            index->go_to(page_sla_materials);
            btn_finish->SetFocus();
            break;
        default:
            index->go_to(page_welcome);
            //PresetBundle::BoolAnkerMain ? index->go_to(page_fff) : index->go_to(page_welcome);
            btn_next->SetFocus();
            break;
    }
}

void ConfigWizard::priv::create_3rdparty_pages()
{
    for (const auto &pair : bundles) {
        const VendorProfile *vendor = pair.second.vendor_profile;
        if (vendor->id == PresetBundle::ANKER_BUNDLE) { continue; }

        bool is_fff_technology = false;
        bool is_sla_technology = false;

        for (auto& model: vendor->models)
        {
            if (!is_fff_technology && model.technology == ptFFF)
                 is_fff_technology = true;
            if (!is_sla_technology && model.technology == ptSLA)
                 is_sla_technology = true;
        }

        PagePrinters* pageFFF = nullptr;
        PagePrinters* pageSLA = nullptr;

        if (is_fff_technology) {
            pageFFF = new PagePrinters(q, vendor->name + " " +_L("FFF Technology Printers"), vendor->name+" FFF", *vendor, 1, T_FFF);
            add_page(pageFFF);
        }

        if (is_sla_technology) {
            pageSLA = new PagePrinters(q, vendor->name + " " + _L("SLA Technology Printers"), vendor->name+" MSLA", *vendor, 1, T_SLA);
            add_page(pageSLA);
        }

        pages_3rdparty.insert({vendor->id, {pageFFF, pageSLA}});
    }
}

void ConfigWizard::priv::set_run_reason(RunReason run_reason)
{
    this->run_reason = run_reason;
    for (auto &page : all_pages) {
        page->set_run_reason(run_reason);
    }
}

void ConfigWizard::priv::update_materials(Technology technology)
{
    if ((any_fff_selected || custom_printer_in_bundle || custom_printer_selected) && (technology & T_FFF)) {
        filaments.clear();
        aliases_fff.clear();
        // Iterate filaments in all bundles
        for (const auto &pair : bundles) {
            for (const auto &filament : pair.second.preset_bundle->filaments) {
                // Check if filament is already added
                if (filaments.containts(&filament))
					continue;
                // Iterate printers in all bundles
                for (const auto &printer : pair.second.preset_bundle->printers) {
					if (!printer.is_visible || printer.printer_technology() != ptFFF)
						continue;
                    // Filter out inapplicable printers
					if (is_compatible_with_printer(PresetWithVendorProfile(filament, filament.vendor), PresetWithVendorProfile(printer, printer.vendor))) {
						if (!filaments.containts(&filament)) {
							filaments.push(&filament);
							if (!filament.alias.empty())
								aliases_fff[filament.alias].insert(filament.name); 
						} 
						filaments.add_printer(&printer);
                    }
				}
                // template filament bundle has no printers - filament would be never added
                if(pair.second.vendor_profile&& pair.second.vendor_profile->templates_profile && pair.second.preset_bundle->printers.begin() == pair.second.preset_bundle->printers.end())
                {
                    if (!filaments.containts(&filament)) {
                        filaments.push(&filament);
                        if (!filament.alias.empty())
                            aliases_fff[filament.alias].insert(filament.name);
                    }
                }
            }
        }
        // count compatible printers
        for (const auto& preset : filaments.presets) {
            // skip template filaments
            if (preset->vendor && preset->vendor->templates_profile)
                continue;

            const auto filter = [preset](const std::pair<std::string, size_t> element) {
                return preset->alias == element.first;
            };
            if (std::find_if(filaments.compatibility_counter.begin(), filaments.compatibility_counter.end(), filter) != filaments.compatibility_counter.end()) {
                continue;
            }
            // find all aliases (except templates)
            std::vector<size_t> idx_with_same_alias;
            for (size_t i = 0; i < filaments.presets.size(); ++i) {
                if (preset->alias == filaments.presets[i]->alias && ((filaments.presets[i]->vendor && !filaments.presets[i]->vendor->templates_profile) || !filaments.presets[i]->vendor))
                    idx_with_same_alias.push_back(i);
            }
            // check compatibility with each printer
            size_t counter = 0;
            for (const auto& printer : filaments.printers) {
                if (!(*printer).is_visible || (*printer).printer_technology() != ptFFF)
                    continue;
                bool compatible = false;
                // Test other materials with same alias
                for (size_t i = 0; i < idx_with_same_alias.size() && !compatible; ++i) {
                    const Preset& prst = *(filaments.presets[idx_with_same_alias[i]]);
                    const Preset& prntr = *printer;
                    if (is_compatible_with_printer(PresetWithVendorProfile(prst, prst.vendor), PresetWithVendorProfile(prntr, prntr.vendor))) {
                        compatible = true;
                        break;
                    }
                }
                if (compatible)
                    counter++;
            }
            filaments.compatibility_counter.emplace_back(preset->alias, counter);
        }
    }

    if (PresetBundle::BoolHasSLA && any_sla_selected && (technology & T_SLA)) {
        sla_materials.clear();
        aliases_sla.clear();

        // Iterate SLA materials in all bundles
        for (const auto &pair : bundles) {
            for (const auto &material : pair.second.preset_bundle->sla_materials) {
                // Check if material is already added
                if (sla_materials.containts(&material))
                	continue;
                // Iterate printers in all bundles
				// For now, we only allow the profiles to be compatible with another profiles inside the same bundle.
                for (const auto& printer : pair.second.preset_bundle->printers) {
                    if(!printer.is_visible || printer.printer_technology() != ptSLA)
                        continue;
                    // Filter out inapplicable printers
                    if (is_compatible_with_printer(PresetWithVendorProfile(material, nullptr), PresetWithVendorProfile(printer, nullptr))) {
                        // Check if material is already added
                        if(!sla_materials.containts(&material)) {
                            sla_materials.push(&material);
                            if (!material.alias.empty())
                                aliases_sla[material.alias].insert(material.name);
                        }
                        sla_materials.add_printer(&printer);
                    }
                }
            }
        }
        // count compatible printers        
        for (const auto& preset : sla_materials.presets) {
            
            const auto filter = [preset](const std::pair<std::string, size_t> element) {
                return preset->alias == element.first;
            };
            if (std::find_if(sla_materials.compatibility_counter.begin(), sla_materials.compatibility_counter.end(), filter) != sla_materials.compatibility_counter.end()) {
                continue;
            }
            std::vector<size_t> idx_with_same_alias;
            for (size_t i = 0; i < sla_materials.presets.size(); ++i) {
                if(preset->alias == sla_materials.presets[i]->alias)
                    idx_with_same_alias.push_back(i);
            }
            size_t counter = 0;
            for (const auto& printer : sla_materials.printers) {
                if (!(*printer).is_visible || (*printer).printer_technology() != ptSLA)
                    continue;
                bool compatible = false;
                // Test otrher materials with same alias
                for (size_t i = 0; i < idx_with_same_alias.size() && !compatible; ++i) {
                    const Preset& prst = *(sla_materials.presets[idx_with_same_alias[i]]);
                    const Preset& prntr = *printer;
                    if (is_compatible_with_printer(PresetWithVendorProfile(prst, prst.vendor), PresetWithVendorProfile(prntr, prntr.vendor))) {
                        compatible = true;
                        break;
                    }
                }
                if (compatible)
                    counter++;
            }
            sla_materials.compatibility_counter.emplace_back(preset->alias, counter);
        }
    }
}

void ConfigWizard::priv::on_custom_setup(const bool custom_wanted)
{
	custom_printer_selected = custom_wanted;
    load_pages();
}

void ConfigWizard::priv::on_printer_pick(PagePrinters *page, const PrinterPickerEvent &evt)
{
    if (check_sla_selected() != any_sla_selected ||
        check_fff_selected() != any_fff_selected) {
        any_fff_selected = check_fff_selected();
        any_sla_selected = check_sla_selected();

        load_pages();
    }

    // Update the is_visible flag on relevant printer profiles
    for (auto &pair : bundles) {
        if (pair.first != evt.vendor_id) { continue; }

        for (auto &preset : pair.second.preset_bundle->printers) {
            if (preset.config.opt_string("printer_model") == evt.model_id
                && preset.config.opt_string("printer_variant") == evt.variant_name) {
                preset.is_visible = evt.enable;
            }
        }

        // When a printer model is picked, but there is no material installed compatible with this printer model,
        // install default materials for selected printer model silently.
		check_and_install_missing_materials(page->technology, evt.model_id);
    }

    if (page->technology & T_FFF) {
        page_filaments->clear();
    } else if (page->technology & T_SLA) {
        page_sla_materials->clear();
    }
}

void ConfigWizard::priv::select_default_materials_for_printer_model(const VendorProfile::PrinterModel &printer_model, Technology technology)
{
    PageMaterials* page_materials = technology & T_FFF ? page_filaments : page_sla_materials;
    for (const std::string& material : printer_model.default_materials)
        appconfig_new.set(page_materials->materials->appconfig_section(), material, "1");
}

void ConfigWizard::priv::select_default_materials_for_printer_models(Technology technology, const std::set<const VendorProfile::PrinterModel*> &printer_models)
{
    PageMaterials     *page_materials    = technology & T_FFF ? page_filaments : page_sla_materials;
    const std::string &appconfig_section = page_materials->materials->appconfig_section();
    
    // Following block was unnecessary. Its enough to iterate printer_models once. Not for every vendor printer page. 
    // Filament is selected on same page for all printers of same technology.
    /*
    auto select_default_materials_for_printer_page = [this, appconfig_section, printer_models, technology](PagePrinters *page_printers, Technology technology)
    {
        const std::string vendor_id = page_printers->get_vendor_id();
        for (auto& pair : bundles)
            if (pair.first == vendor_id)
                for (const VendorProfile::PrinterModel *printer_model : printer_models)
                    for (const std::string &material : printer_model->default_materials)
                        appconfig_new.set(appconfig_section, material, "1");
    };

    PagePrinters* page_printers = technology & T_FFF ? page_fff : page_msla;
    select_default_materials_for_printer_page(page_printers, technology);

    for (const auto& printer : pages_3rdparty)
    {
        page_printers = technology & T_FFF ? printer.second.first : printer.second.second;
        if (page_printers)
            select_default_materials_for_printer_page(page_printers, technology);
    }
    */

    // Iterate printer_models and select default materials. If none available -> msg to user.
    std::vector<const VendorProfile::PrinterModel*> models_without_default;
    for (const VendorProfile::PrinterModel* printer_model : printer_models) {
        if (printer_model->default_materials.empty()) {
            models_without_default.emplace_back(printer_model);
        } else {
            for (const std::string& material : printer_model->default_materials)
                appconfig_new.set(appconfig_section, material, "1");
        }
    }

    if (!models_without_default.empty()) {
        std::string printer_names = "\n\n";
        for (const VendorProfile::PrinterModel* printer_model : models_without_default) {
            printer_names += printer_model->name + "\n";
        }
        printer_names += "\n\n";
        std::string message = (technology & T_FFF ?
            GUI::format(_L("Following printer profiles has no default filament: %1%Please select one manually."), printer_names) :
            GUI::format(_L("Following printer profiles has no default material: %1%Please select one manually."), printer_names));
        MessageDialog msg(q, message, _L("Notice"), wxOK);
        msg.ShowModal();
    }

    update_materials(technology);
    ((technology & T_FFF) ? page_filaments : page_sla_materials)->reload_presets();
}

void ConfigWizard::priv::on_3rdparty_install(const VendorProfile *vendor, bool install)
{
    auto it = pages_3rdparty.find(vendor->id);
    wxCHECK_RET(it != pages_3rdparty.end(), "Internal error: GUI page not found for 3rd party vendor profile");

    for (PagePrinters* page : { it->second.first, it->second.second }) 
        if (page) {
            if (page->install && !install)
                page->select_all(false);
            page->install = install;
            // if some 3rd vendor is selected, select first printer for them
            if (install)
                page->printer_pickers[0]->select_one(0, true);
            page->Layout();
        }

    load_pages();
}

std::atomic_bool gIsClickWizardFinish;
bool ConfigWizard::priv::on_bnt_finish()
{
    wxBusyCursor wait;

    if (!page_downloader->on_finish_downloader()) {
        index->go_to(page_downloader);
        return false;
    }
    /* When Filaments or Sla Materials pages are activated, 
     * materials for this pages are automaticaly updated and presets are reloaded.
     * 
     * But, if _Finish_ button was clicked without activation of those pages 
     * (for example, just some printers were added/deleted), 
     * than last changes wouldn't be updated for filaments/materials.
     * SO, do that before close of Wizard
     */
    update_materials(T_ANY);
    if (any_fff_selected)
    {
        page_filaments->reload_presets();
    }
    else
    {
        AnkerMessageBox(nullptr, _u8L("common_wizard_empty_printer_notice"), _u8L("common_popup_titlenotice"),false);
        return false;
    }
    gIsClickWizardFinish.store(true);
    if (any_sla_selected)
        page_sla_materials->reload_presets();

	// theres no need to check that filament is selected if we have only custom printer
    if (custom_printer_selected && !any_fff_selected && !any_sla_selected)
    {
        uploadTraceData();
        return true;
    }

    // check, that there is selected at least one filament/material
    bool bRet = check_and_install_missing_materials(T_ANY);
    if (bRet)
    {
        uploadTraceData();
    }
    return bRet;
}

void ConfigWizard::priv::uploadTraceData()
{
    std::set<std::string> selectedModelVec = page_fff->get_selected_models();
    std::string result;
    auto it = selectedModelVec.begin();
    result += *it;
    ++it;
    for (; it != selectedModelVec.end(); ++it) {

        // Append a comma and the current string to the result
        result += ", " + *it;
    }
    std::map<std::string, std::string> maps;
    maps.insert(std::make_pair(c_config_wizard_printer_added, result));
    BuryAddEvent(e_config_wizard_event, maps);
}

// This allmighty method verifies, whether there is at least a single compatible filament or SLA material installed
// for each Printer preset of each Printer Model installed.
//
// In case only_for_model_id is set, then the test is done for that particular printer model only, and the default materials are installed silently.
// Otherwise the user is quieried whether to install the missing default materials or not.
// 
// Return true if the tested Printer Models already had materials installed.
// Return false if there were some Printer Models with missing materials, independent from whether the defaults were installed for these
// respective Printer Models or not.
bool ConfigWizard::priv::check_and_install_missing_materials(Technology technology, const std::string &only_for_model_id)
{
	// Walk over all installed Printer presets and verify whether there is a filament or SLA material profile installed at the same PresetBundle,
	// which is compatible with it.
    const auto printer_models_missing_materials = [this, only_for_model_id](PrinterTechnology technology, const std::string &section, bool no_templates)
    {
		const std::map<std::string, std::string> &appconfig_presets = appconfig_new.has_section(section) ? appconfig_new.get_section(section) : std::map<std::string, std::string>();
    	std::set<const VendorProfile::PrinterModel*> printer_models_without_material;
        for (const auto &pair : bundles) {
        	const PresetCollection &materials = pair.second.preset_bundle->materials(technology);
        	for (const auto &printer : pair.second.preset_bundle->printers) {
                if (printer.is_visible && printer.printer_technology() == technology) {
	            	const VendorProfile::PrinterModel *printer_model = PresetUtils::system_printer_model(printer);
	            	assert(printer_model != nullptr);
	            	if ((only_for_model_id.empty() || only_for_model_id == printer_model->id) &&
	            		printer_models_without_material.find(printer_model) == printer_models_without_material.end()) {
                    	bool has_material = false;
                        for (const auto& preset : appconfig_presets) {
			            	if (preset.second == "1") {
			            		const Preset *material = materials.find_preset(preset.first, false);
			            		if (material != nullptr && is_compatible_with_printer(PresetWithVendorProfile(*material, nullptr), PresetWithVendorProfile(printer, nullptr))) {
				                	has_material = true;
				                    break;
				                }
                                // find if preset.first is part of the templates profile (up is searching if preset.first is part of printer vendor preset)
                                if (!no_templates) {
                                    for (const auto& bp : bundles) {
                                        if (!bp.second.preset_bundle->vendors.empty() && bp.second.preset_bundle->vendors.begin()->second.templates_profile) {
                                            const PresetCollection& template_materials = bp.second.preset_bundle->materials(technology);
                                            const Preset* template_material = template_materials.find_preset(preset.first, false);
                                            if (template_material && is_compatible_with_printer(PresetWithVendorProfile(*template_material, &bp.second.preset_bundle->vendors.begin()->second), PresetWithVendorProfile(printer, nullptr))) {
                                                has_material = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (has_material)
                                    break;

			                }
			            }
			            if (! has_material)
			            	printer_models_without_material.insert(printer_model);
			        }
                }
            }
        }
        // todo: just workaround so template_profile_selected wont get to false after this function is called for SLA
        // this will work unltil there are no SLA template filaments
        if (technology == ptFFF) {
            // template_profile_selected check
            template_profile_selected = false;
            for (const auto& bp : bundles) {
                if (!bp.second.preset_bundle->vendors.empty() && bp.second.preset_bundle->vendors.begin()->second.templates_profile) {
                    for (const auto& preset : appconfig_presets) {
                        const PresetCollection& template_materials = bp.second.preset_bundle->materials(technology);
                        const Preset* template_material = template_materials.find_preset(preset.first, false);
                        if (template_material){
                            template_profile_selected = true;
                            break;
                        }
                    }
                    if (template_profile_selected) {
                        break;
                    }
                }
            }
        }
        assert(printer_models_without_material.empty() || only_for_model_id.empty() || only_for_model_id == (*printer_models_without_material.begin())->id);
        return printer_models_without_material;
    };

    const auto ask_and_select_default_materials = [this](const wxString &message, const std::set<const VendorProfile::PrinterModel*> &printer_models, Technology technology)
    {
        //wxMessageDialog msg(q, message, _L("Notice"), wxYES_NO);
        MessageDialog msg(q, message, _L("Notice"), wxYES_NO);
        if (msg.ShowModal() == wxID_YES)
            select_default_materials_for_printer_models(technology, printer_models);
    };

    const auto printer_model_list = [](const std::set<const VendorProfile::PrinterModel*> &printer_models) -> wxString {
    	wxString out;
    	for (const VendorProfile::PrinterModel *printer_model : printer_models) {
            wxString name = from_u8(printer_model->name);
    		out += "\t\t";
    		out += name;
    		out += "\n";
    	}
    	return out;
    };

    bool no_templates = wxGetApp().app_config->get("no_templates") == "1";

    if (any_fff_selected && (technology & T_FFF)) {
    	std::set<const VendorProfile::PrinterModel*> printer_models_without_material = printer_models_missing_materials(ptFFF, AppConfig::SECTION_FILAMENTS, no_templates);
    	if (! printer_models_without_material.empty()) {
			if (only_for_model_id.empty())
				ask_and_select_default_materials(
					_L("The following FFF printer models have no filament selected:") +
					"\n\n" +
					printer_model_list(printer_models_without_material) +
					"\n\n" +
					_L("Do you want to select default filaments for these FFF printer models?"),
					printer_models_without_material,
					T_FFF);
			else
				select_default_materials_for_printer_model(**printer_models_without_material.begin(), T_FFF);
			return false;
		}
    }

    if (PresetBundle::BoolHasSLA && any_sla_selected && (technology & T_SLA)) {
    	std::set<const VendorProfile::PrinterModel*> printer_models_without_material = printer_models_missing_materials(ptSLA, AppConfig::SECTION_MATERIALS, no_templates);
    	if (! printer_models_without_material.empty()) {
	        if (only_for_model_id.empty())
	            ask_and_select_default_materials(
					_L("The following SLA printer models have no materials selected:") +
	            	"\n\n" +
				   	printer_model_list(printer_models_without_material) +
					"\n\n" +
					_L("Do you want to select default SLA materials for these printer models?"),
					printer_models_without_material,
	            	T_SLA);
	        else
				select_default_materials_for_printer_model(**printer_models_without_material.begin(), T_SLA);
	        return false;
	    }
    }

    return true;
}

static std::set<std::string> get_new_added_presets(const std::map<std::string, std::string>& old_data, const std::map<std::string, std::string>& new_data) 
{
    auto get_aliases = [](const std::map<std::string, std::string>& data) {
        std::set<std::string> old_aliases;
        for (auto item : data) {
            const std::string& name = item.first;
            size_t pos = name.find("@");
            old_aliases.emplace(pos == std::string::npos ? name : name.substr(0, pos-1));
        }
        return old_aliases;
    };

    std::set<std::string> old_aliases = get_aliases(old_data);
    std::set<std::string> new_aliases = get_aliases(new_data);
    std::set<std::string> diff;
    std::set_difference(new_aliases.begin(), new_aliases.end(), old_aliases.begin(), old_aliases.end(), std::inserter(diff, diff.begin()));

    return diff;
}

static std::string get_first_added_preset(const std::map<std::string, std::string>& old_data, const std::map<std::string, std::string>& new_data)
{
    std::set<std::string> diff = get_new_added_presets(old_data, new_data);
    if (diff.empty())
        return std::string();
    return *diff.begin();
}

bool ConfigWizard::priv::apply_config(AppConfig *app_config, PresetBundle *preset_bundle, const PresetUpdater *updater, bool& apply_keeped_changes)
{
    wxString header, caption = _L("Configuration is edited in ConfigWizard");
    const auto enabled_vendors = appconfig_new.vendors();
    const auto enabled_vendors_old = app_config->vendors();

    bool suppress_sla_printer = model_has_multi_part_objects(wxGetApp().model());
    PrinterTechnology preferred_pt = ptAny;
    auto get_preferred_printer_technology = [enabled_vendors, enabled_vendors_old, suppress_sla_printer](const std::string& bundle_name, const Bundle& bundle) {
        const auto config = enabled_vendors.find(bundle_name);
        PrinterTechnology pt = ptAny;
        if (config != enabled_vendors.end()) {
            for (const auto& model : bundle.vendor_profile->models) {
                if (const auto model_it = config->second.find(model.id);
                    model_it != config->second.end() && model_it->second.size() > 0) {
                    pt = model.technology;
                    const auto config_old = enabled_vendors_old.find(bundle_name);
                    if (config_old == enabled_vendors_old.end() || config_old->second.find(model.id) == config_old->second.end()) {
                        // if preferred printer model has SLA printer technology it's important to check the model for multi-part state
                        if (pt == ptSLA && suppress_sla_printer)
                            continue;
                        return pt;
                    }

                    if (const auto model_it_old = config_old->second.find(model.id);
                        model_it_old == config_old->second.end() || model_it_old->second != model_it->second) {
                        // if preferred printer model has SLA printer technology it's important to check the model for multi-part state
                        if (pt == ptSLA && suppress_sla_printer)
                            continue;
                        return pt;
                    }
                }
            }
        }
        return pt;
    };
    // Anker printers are considered first, then 3rd party.
    if (preferred_pt = get_preferred_printer_technology(PresetBundle::PreferredVendorBundle, bundles.anker_bundle());
        preferred_pt == ptAny || (preferred_pt == ptSLA && suppress_sla_printer)) {
        for (const auto& bundle : bundles) {
            if (bundle.second.is_anker_bundle) { continue; }
            if (PrinterTechnology pt = get_preferred_printer_technology(bundle.first, bundle.second); pt == ptAny)
                continue;
            else if (preferred_pt == ptAny)
                preferred_pt = pt;
            if(!(preferred_pt == ptAny || (preferred_pt == ptSLA && suppress_sla_printer)))
                break;
        }
    }

    if (preferred_pt == ptSLA && !wxGetApp().may_switch_to_SLA_preset(caption))
        return false;

    bool check_unsaved_preset_changes = page_welcome->reset_user_profile();
    if (check_unsaved_preset_changes)
        header = _L("All user presets will be deleted.");
    int act_btns = ActionButtons::KEEP;
    if (!check_unsaved_preset_changes)
        act_btns |= ActionButtons::SAVE;

    // Install bundles from resources or cache / vendor if needed:
    std::vector<std::string> install_bundles;
    for (const auto &pair : bundles) {
        if (pair.second.location == BundleLocation::IN_VENDOR) { continue; }

        if (pair.second.is_anker_bundle) {
            // Always install Anker bundle, because it has a lot of filaments/materials
            // likely to be referenced by other profiles.
            install_bundles.emplace_back(pair.first);
            continue;
        }

        const auto vendor = enabled_vendors.find(pair.first);
        if (vendor == enabled_vendors.end()) {
            // vendor not found
            // if templates vendor and needs to be installed, add it
            // then continue
            if (template_profile_selected && pair.second.vendor_profile && pair.second.vendor_profile->templates_profile)
                install_bundles.emplace_back(pair.first);
            continue;
        }

        size_t size_sum = 0;
        for (const auto &model : vendor->second) { size_sum += model.second.size(); }

        if (size_sum > 0) {
            // This vendor needs to be installed
            install_bundles.emplace_back(pair.first);
        }
    }
    if (!check_unsaved_preset_changes)
        if ((check_unsaved_preset_changes = install_bundles.size() > 0))
            header = _L_PLURAL("A new vendor was installed and one of its printers will be activated", "New vendors were installed and one of theirs printers will be activated", install_bundles.size());

#ifdef __linux__
    // Desktop integration on Linux
    BOOST_LOG_TRIVIAL(debug) << "ConfigWizard::priv::apply_config integrate_desktop" << page_welcome->integrate_desktop()  << " perform_registration_linux " << page_downloader->m_downloader->get_perform_registration_linux();
    if (page_welcome->integrate_desktop())
        DesktopIntegrationDialog::perform_desktop_integration();
    if (page_downloader->m_downloader->get_perform_registration_linux())
        DesktopIntegrationDialog::perform_downloader_desktop_integration();
#endif

    // Decide whether to create snapshot based on run_reason and the reset profile checkbox
    bool snapshot = true;
    Snapshot::Reason snapshot_reason = Snapshot::SNAPSHOT_UPGRADE;
    switch (run_reason) {
        case ConfigWizard::RR_DATA_EMPTY:
            snapshot = false;
            break;
        case ConfigWizard::RR_DATA_LEGACY:
            snapshot = true;
            break;
        case ConfigWizard::RR_DATA_INCOMPAT:
            // In this case snapshot has already been taken by
            // PresetUpdater with the appropriate reason
            snapshot = false;
            break;
        case ConfigWizard::RR_USER:
            snapshot = page_welcome->reset_user_profile();
            snapshot_reason = Snapshot::SNAPSHOT_USER;
            break;
    }

    if (snapshot && ! take_config_snapshot_cancel_on_error(*app_config, snapshot_reason, "", _u8L("Do you want to continue changing the configuration?")))
        return false;

    if (check_unsaved_preset_changes &&
        !wxGetApp().check_and_keep_current_preset_changes(caption, header, act_btns, &apply_keeped_changes))
        return false;

    if (install_bundles.size() > 0) {
        // Install bundles from resources or cache / vendor.
        // Don't create snapshot - we've already done that above if applicable.
        
        bool install_result = updater->install_bundles_rsrc_or_cache_vendor(std::move(install_bundles), false);
        if (!install_result)
            return false;
    } else {
        BOOST_LOG_TRIVIAL(info) << "No bundles need to be installed from resources or cache / vendor";
    }

    if (page_welcome && page_welcome->reset_user_profile()) {
        BOOST_LOG_TRIVIAL(info) << "Resetting user profiles...";
        preset_bundle->reset(true);
    }

    std::string preferred_model;
    std::string preferred_variant;
    auto get_preferred_printer_model = [enabled_vendors, enabled_vendors_old, preferred_pt](const std::string& bundle_name, const Bundle& bundle, std::string& variant) {
        const auto config = enabled_vendors.find(bundle_name);
        if (config == enabled_vendors.end())
            return std::string();
        for (const auto& model : bundle.vendor_profile->models) {
            if (const auto model_it = config->second.find(model.id);
                model_it != config->second.end() && model_it->second.size() > 0 &&
                preferred_pt == model.technology) {
                variant = *model_it->second.begin();
                const auto config_old = enabled_vendors_old.find(bundle_name);
                if (config_old == enabled_vendors_old.end())
                    return model.id;
                const auto model_it_old = config_old->second.find(model.id);
                if (model_it_old == config_old->second.end())
                    return model.id;
                else if (model_it_old->second != model_it->second) {
                    for (const auto& var : model_it->second)
                        if (model_it_old->second.find(var) == model_it_old->second.end()) {
                            variant = var;
                            return model.id;
                        }
                }
            }
        }
        if (!variant.empty())
            variant.clear();
        return std::string();
    };
    // Anker printers are considered first, then 3rd party.
    if (preferred_model = get_preferred_printer_model(PresetBundle::PreferredVendorBundle, bundles.anker_bundle(), preferred_variant);
        preferred_model.empty()) {
        for (const auto& bundle : bundles) {
            if (bundle.second.is_anker_bundle) { continue; }
            if (preferred_model = get_preferred_printer_model(bundle.first, bundle.second, preferred_variant);
                !preferred_model.empty())
                    break;
        }
    }

    // if unsaved changes was not cheched till this moment
    if (!check_unsaved_preset_changes) {
        if ((check_unsaved_preset_changes = !preferred_model.empty())) {
            header = _L("A new Printer was installed and it will be activated.");
            if (!wxGetApp().check_and_keep_current_preset_changes(caption, header, act_btns, &apply_keeped_changes))
                return false;
        }
        else if ((check_unsaved_preset_changes = enabled_vendors_old != enabled_vendors)) {
            header = _L("Some Printers were uninstalled.");
            if (!wxGetApp().check_and_keep_current_preset_changes(caption, header, act_btns, &apply_keeped_changes))
                return false;
        }
    }

    std::string first_added_filament, first_added_sla_material;
    auto get_first_added_material_preset = [this, app_config](const std::string& section_name, std::string& first_added_preset) {
        if (appconfig_new.has_section(section_name)) {
            // get first of new added preset names
            const std::map<std::string, std::string>& old_presets = app_config->has_section(section_name) ? app_config->get_section(section_name) : std::map<std::string, std::string>();
            first_added_preset = get_first_added_preset(old_presets, appconfig_new.get_section(section_name));
        }
    };
    get_first_added_material_preset(AppConfig::SECTION_FILAMENTS, first_added_filament);
    get_first_added_material_preset(AppConfig::SECTION_MATERIALS, first_added_sla_material);

    // if unsaved changes was not cheched till this moment
    if (!check_unsaved_preset_changes) {
        if ((check_unsaved_preset_changes = !first_added_filament.empty() || !first_added_sla_material.empty())) {
            header = !first_added_filament.empty() ? 
                     _L("A new filament was installed and it will be activated.") :
                     _L("A new SLA material was installed and it will be activated.");
            if (!wxGetApp().check_and_keep_current_preset_changes(caption, header, act_btns, &apply_keeped_changes))
                return false;
        }
        else {
            auto changed = [app_config, &appconfig_new = std::as_const(this->appconfig_new)](const std::string& section_name) {
                return (app_config->has_section(section_name) ? app_config->get_section(section_name) : std::map<std::string, std::string>()) != appconfig_new.get_section(section_name);
            };
            bool is_filaments_changed     = changed(AppConfig::SECTION_FILAMENTS);
            bool is_sla_materials_changed = PresetBundle::BoolHasSLA && changed(AppConfig::SECTION_MATERIALS);
            if ((check_unsaved_preset_changes = is_filaments_changed || is_sla_materials_changed)) {
                header = is_filaments_changed ? _L("Some filaments were uninstalled.") : _L("Some SLA materials were uninstalled.");
                if (!wxGetApp().check_and_keep_current_preset_changes(caption, header, act_btns, &apply_keeped_changes))
                    return false;
            }
        }
    }

    // apply materials in app_config
    for (const std::string& section_name : {AppConfig::SECTION_FILAMENTS, AppConfig::SECTION_MATERIALS})
        if (appconfig_new.has_section(section_name))
            app_config->set_section(section_name, appconfig_new.get_section(section_name));

    app_config->set_vendors(appconfig_new);

    app_config->set("notify_release", page_update->version_check ? "all" : "none");
    app_config->set("preset_update", page_update->preset_update ? "1" : "0");
    app_config->set("export_sources_full_pathnames", page_reload_from_disk->full_pathnames ? "1" : "0");

#ifdef _WIN32
    app_config->set("associate_3mf", page_files_association->associate_3mf() ? "1" : "0");
    app_config->set("associate_stl", page_files_association->associate_stl() ? "1" : "0");
//    app_config->set("associate_gcode", page_files_association->associate_gcode() ? "1" : "0");

    if (wxGetApp().is_editor()) {
        if (page_files_association->associate_3mf())
            wxGetApp().associate_3mf_files();
        if (page_files_association->associate_stl())
            wxGetApp().associate_stl_files();
    }
//    else {
//        if (page_files_association->associate_gcode())
//            wxGetApp().associate_gcode_files();
//    }
#endif // _WIN32

    page_mode->serialize_mode(app_config);

    if (check_unsaved_preset_changes)
        preset_bundle->load_presets(*app_config, ForwardCompatibilitySubstitutionRule::EnableSilentDisableSystem, 
                                    {preferred_model, preferred_variant, first_added_filament, first_added_sla_material});

    if (!only_sla_mode && page_custom->custom_wanted() && page_custom->is_valid_profile_name()) {
        // if unsaved changes was not cheched till this moment
        if (!check_unsaved_preset_changes && 
            !wxGetApp().check_and_keep_current_preset_changes(caption, _L("Custom printer was installed and it will be activated."), act_btns, &apply_keeped_changes))
            return false;

        page_firmware->apply_custom_config(*custom_config);
        page_bed->apply_custom_config(*custom_config);
        page_bvolume->apply_custom_config(*custom_config);
        page_diams->apply_custom_config(*custom_config);
        page_temps->apply_custom_config(*custom_config);

        copy_bed_model_and_texture_if_needed(*custom_config);

        const std::string profile_name = page_custom->profile_name();
        preset_bundle->load_config_from_wizard(profile_name, *custom_config);
    }

    // Update the selections from the compatibilty.
    preset_bundle->export_selections(*app_config);

    return true;
}
void ConfigWizard::priv::update_presets_in_config(const std::string& section, const std::string& alias_key, bool add)
{
    const PresetAliases& aliases = section == AppConfig::SECTION_FILAMENTS ? aliases_fff : aliases_sla;

    auto update = [this, add](const std::string& s, const std::string& key) {
    	assert(! s.empty());
        if (add)
            appconfig_new.set(s, key, "1");
        else
            appconfig_new.erase(s, key); 
    };

    // add or delete presets had a same alias 
    auto it = aliases.find(alias_key);
    if (it != aliases.end())
        for (const std::string& name : it->second)
            update(section, name);
}

bool ConfigWizard::priv::check_fff_selected()
{
    bool ret = page_fff->any_selected();
    for (const auto& printer: pages_3rdparty)
        if (printer.second.first)               // FFF page
            ret |= printer.second.first->any_selected();
    return ret;
}

bool ConfigWizard::priv::check_sla_selected()
{
    if (!PresetBundle::BoolHasSLA) { return false; }
    bool ret = page_msla->any_selected();
    for (const auto& printer: pages_3rdparty)
        if (printer.second.second)               // SLA page
            ret |= printer.second.second->any_selected();
    return ret;
}


// Public

ConfigWizard::ConfigWizard(wxWindow *parent, bool can_close)
    : DPIDialog(parent, wxID_ANY, _L("common_wizard_title"), wxDefaultPosition, wxDefaultSize, can_close ? wxDEFAULT_DIALOG_STYLE : wxCAPTION | wxTAB_TRAVERSAL)
    , p(new priv(this))
{


    this->SetFont(wxGetApp().normal_font());
    this->SetForegroundColour(wxColour(255,255,255));
    SetBackgroundColour(wxColor(43, 43, 43));

    p->load_vendors();
    p->custom_config.reset(DynamicPrintConfig::new_from_defaults_keys({
        "gcode_flavor", "bed_shape", "bed_custom_texture", "bed_custom_model", "nozzle_diameter", "filament_diameter", "temperature", "bed_temperature",
    }));

    p->index = new ConfigWizardIndex(this);

    auto *vsizer = new wxBoxSizer(wxVERTICAL);
    auto *topsizer = new wxBoxSizer(wxHORIZONTAL);
    p->bottomSizer = new wxBoxSizer(wxVERTICAL);

    auto* hline = new StaticLine(this);
    p->btnsizer = new wxBoxSizer(wxHORIZONTAL);

    // Initially we _do not_ SetScrollRate in order to figure out the overall width of the Wizard  without scrolling.
    // Later, we compare that to the size of the current screen and set minimum width based on that (see below).
    p->hscroll = new wxScrolledWindow(this);
    p->hscroll_sizer = new wxBoxSizer(wxHORIZONTAL);
    p->hscroll->SetSizer(p->hscroll_sizer);
    p->index->Hide();

    //topsizer->Add(p->index, 0, wxEXPAND);
    //topsizer->AddSpacer(INDEX_MARGIN);
    topsizer->Add(p->hscroll, 1, wxEXPAND);

    //p->btn_sel_all = new wxButton(this, wxID_ANY, _L("Select all standard printers"));
    //p->btnsizer->Add(p->btn_sel_all);

    p->btn_prev = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxSize(95, 24));
    p->btn_prev->SetText(_L("common_button_back"));
    p->btn_prev->SetBackgroundColour(wxColor(96, 96, 96));
    p->btn_prev->SetRadius(4);

    p->btn_next = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxSize(95, 24));
    p->btn_next->SetText(_L("common_button_next"));
    p->btn_next->SetBackgroundColour(wxColor(96, 96, 96));
    p->btn_next->SetRadius(4);

    p->btn_finish = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxSize(95, 24));
    p->btn_finish->SetText(_L("common_print_popupfinished_buttonfinish"));
    p->btn_finish->SetBackgroundColour(wxColor(96, 96, 96));
    p->btn_finish->SetRadius(4);

    p->btn_cancel = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxSize(95, 24));
    p->btn_cancel->SetText(_L("common_button_cancel"));
    p->btn_cancel->SetBackgroundColour(wxColor(96, 96, 96));
    p->btn_cancel->SetRadius(4);

    p->btnsizer->AddStretchSpacer();
    p->btnsizer->Add(p->btn_prev, 0, wxLEFT, BTN_SPACING);
    p->btnsizer->Add(p->btn_next, 0, wxLEFT, BTN_SPACING);
    p->btnsizer->Add(p->btn_finish, 0, wxLEFT, BTN_SPACING);
    p->btnsizer->Add(p->btn_cancel, 0, wxLEFT, BTN_SPACING);

   //wxGetApp().UpdateDarkUI(p->btn_sel_all);
    //wxGetApp().UpdateDarkUI(p->btn_prev);
    //wxGetApp().UpdateDarkUI(p->btn_next);
    //wxGetApp().UpdateDarkUI(p->btn_finish);
    //wxGetApp().UpdateDarkUI(p->btn_cancel);

    const auto anker_it = p->bundles.find(PresetBundle::PreferredVendorBundle);
    wxCHECK_RET(anker_it != p->bundles.cend(), (boost::format("Vendor %s not found") % PresetBundle::PreferredVendorBundle).str());
    const VendorProfile *vendor_anker = anker_it->second.vendor_profile;

    p->page_welcome = new PageWelcome(this);
    p->add_page(p->page_welcome);

    p->page_fff = new PagePrinters(this, _L("common_wizard_pinter_header"), (boost::format("%s FFF") % PresetBundle::PreferredVendorName).str(), *vendor_anker, 0, T_FFF);
    if (!can_close)
    {
        //first time conifg printer should select non printer in page_fff
        p->page_fff->select_all(false);
    }

    p->only_sla_mode = !p->page_fff->has_printers;
    if (!p->only_sla_mode) {
        p->add_page(p->page_fff);
        p->page_fff->is_primary_printer_page = true;
    }

    p->page_msla = new PagePrinters(this, _L("Prusa MSLA Technology Printers"), "Prusa MSLA", *vendor_anker, 0, T_SLA);
    if (PresetBundle::BoolHasSLA) {
        p->add_page(p->page_msla);
        if (p->only_sla_mode) {
            p->page_msla->is_primary_printer_page = true;
        }
    }

    if (!p->only_sla_mode) {
        if (PresetBundle::BoolShowOtherVendors) {
            // Pages for 3rd party vendors
            p->create_3rdparty_pages();   // Needs to be done _before_ creating PageVendors
            p->add_page(p->page_vendors = new PageVendors(this));
        }
	    p->add_page(p->page_custom = new PageCustom(this));
        p->custom_printer_selected = p->page_custom->custom_wanted();
    }

    p->any_sla_selected = p->check_sla_selected();
    p->any_fff_selected = ! p->only_sla_mode && p->check_fff_selected();

    p->update_materials(T_ANY);
    if (!p->only_sla_mode)
        p->add_page(p->page_filaments = new PageMaterials(this, &p->filaments,
            _L("Filament Profiles Selection"), _L("Filaments"), _L("Type:") ));

    //p->add_page(p->page_sla_materials = new PageMaterials(this, &p->sla_materials, _L("SLA Material Profiles Selection") + " ", _L("SLA Materials"), _L("Type:") ));

    p->page_update = new PageUpdate(this);
    PresetBundle::BoolAnkerMain ? void(0) : p->add_page(p->page_update);
    p->page_downloader = new PageDownloader(this);
    PresetBundle::BoolAnkerMain ? void(0) : p->add_page(p->page_downloader);
    p->add_page(p->page_reload_from_disk = new PageReloadFromDisk(this));
#ifdef _WIN32
    p->add_page(p->page_files_association = new PageFilesAssociation(this));
#endif // _WIN32
    p->add_page(p->page_mode     = new PageMode(this));
    p->add_page(p->page_firmware = new PageFirmware(this));
    p->add_page(p->page_bed      = new PageBedShape(this));
    p->add_page(p->page_bvolume  = new PageBuildVolume(this));
    p->add_page(p->page_diams    = new PageDiameters(this));
    p->add_page(p->page_temps    = new PageTemperatures(this));
    
    p->load_pages();
    p->index->go_to(size_t{0});

    p->bottomSizer->Add(hline, 0, wxEXPAND | wxLEFT | wxRIGHT, VERTICAL_SPACING);
    p->bottomSizer->Add(p->btnsizer, 0, wxEXPAND | wxALL, DIALOG_MARGIN);

    vsizer->Add(topsizer, 1, wxEXPAND | wxALL, DIALOG_MARGIN);
    vsizer->Add(p->bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, VERTICAL_SPACING);

    SetSizer(vsizer);
    SetSizerAndFit(vsizer);

    // We can now enable scrolling on hscroll
    p->hscroll->SetScrollRate(30, 30);

    on_window_geometry(this, [this]() {
        p->init_dialog_size();
    });

    p->btn_prev->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &) { this->p->index->go_prev(); });

    p->btn_next->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &)
    {
        // check, that there is selected at least one filament/material
        ConfigWizardPage* active_page = this->p->index->active_page();
        if (// Leaving the filaments or SLA materials page and 
        	(active_page == p->page_filaments || active_page == p->page_sla_materials) && 
        	// some Printer models had no filament or SLA material selected.
        	! p->check_and_install_missing_materials(dynamic_cast<PageMaterials*>(active_page)->materials->technology))
        	// In that case don't leave the page and the function above queried the user whether to install default materials.
            return;
        this->p->index->go_next();
    });

    p->btn_finish->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &)
    {
        if (p->on_bnt_finish())
        {
            this->EndModal(wxID_OK);
        }
    });

    p->btn_cancel->Bind(wxEVT_BUTTON, [this](const wxCommandEvent&)
        {
            this->EndModal(wxID_CANCEL);
        });


    //p->btn_sel_all->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &) {
    //    p->any_sla_selected = PresetBundle::BoolHasSLA;
    //    p->load_pages();
    //    p->page_fff->select_all(true, false);
    //    p->page_msla->select_all(true, false);
    //    p->index->go_to(p->page_mode);
    //});

    p->index->Bind(EVT_INDEX_PAGE, [this](const wxCommandEvent &evt) {
        const bool is_last = p->index->active_is_last();
        const bool is_first = p->index->active_is_first();

        bool is_welcome = false;
        wxString PageName = evt.GetString();
        if (PageName == _L("Welcome")) {
            p->bottomSizer->Show(false);
            is_welcome = true;
        }
        else {
            p->bottomSizer->Show(true);
            is_welcome = false;
        }

        p->btn_next->Show(! is_last && !is_welcome );
        p->btn_finish->Show(is_last);

        p->btn_prev->Show(!is_first  );

        if (is_first)
            p->btn_next->SetFocus();

        if (is_last)
        {
            p->btn_finish->SetFocus();
        }

        if (p->run_reason == ConfigWizard::RR_USER) {
            p->btn_prev->Show(false);
            p->btn_cancel->Show(true);
            p->btn_finish->SetLabelText(_L("common_button_save"));
        }
        else {
            p->btn_cancel->Show(false);
            p->btn_finish->SetLabelText(_L("common_print_popupfinished_buttonfinish"));
        }

        Layout();
    });

    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) {
        if (p->run_reason == ConfigWizard::RR_DATA_EMPTY) {
            evt.Veto();
        }
        else
        {
            this->EndModal(wxID_CLOSE);
        }
    });

    if (wxLinux_gtk3)
        this->Bind(wxEVT_SHOW, [this, vsizer](const wxShowEvent& e) {
            ConfigWizardPage* active_page = p->index->active_page();
            if (!active_page)
                return;
            for (auto page : p->all_pages)
                if (page != active_page)
                    page->Hide();
            // update best size for the dialog after hiding of the non-active pages
            vsizer->SetSizeHints(this);
            // set initial dialog size
            p->init_dialog_size();
        });
}

ConfigWizard::~ConfigWizard() {}

bool ConfigWizard::run(RunReason reason, StartPage start_page)
{
    BOOST_LOG_TRIVIAL(info) << boost::format("Running ConfigWizard, reason: %1%, start_page: %2%") % reason % start_page;

    GUI_App &app = wxGetApp();

    p->set_run_reason(reason);
    p->set_start_page(start_page);

    if (ShowModal() == wxID_OK) {
        bool apply_keeped_changes = false;
        if (! p->apply_config(app.app_config, app.preset_bundle, app.preset_updater, apply_keeped_changes))
            return false;

        if (apply_keeped_changes)
            app.apply_keeped_preset_modifications();

        app.app_config->set_legacy_datadir(false);
        app.update_mode();
        //app.obj_manipul()->update_ui_from_settings();
        app.aobj_manipul()->update_ui_from_settings();
        BOOST_LOG_TRIVIAL(info) << "ConfigWizard applied";
        return true;
    } else {
        BOOST_LOG_TRIVIAL(info) << "ConfigWizard cancelled";
        return false;
    }
}
void ConfigWizard::go_next_page()
{
    p->index->go_next();
}

const wxString& ConfigWizard::name(const bool from_menu/* = false*/)
{
    // A different naming convention is used for the Wizard on Windows & GTK vs. OSX.
    // Note: Don't call _() macro here.
    //       This function just return the current name according to the OS.
    //       Translation is implemented inside GUI_App::add_config_menu()
#if __APPLE__
    static const wxString config_wizard_name =  L("Configuration Assistant");
    static const wxString config_wizard_name_menu = L("Configuration &Assistant");
#else
    static const wxString config_wizard_name = L("Configuration Wizard");
    static const wxString config_wizard_name_menu = L("Configuration &Wizard");
#endif
    return from_menu ? config_wizard_name_menu : config_wizard_name;
}

void ConfigWizard::on_dpi_changed(const wxRect &suggested_rect)
{
    p->index->msw_rescale();

    const int em = em_unit();

    msw_buttons_rescale(this, em, { wxID_APPLY, 
                                    p->btn_next->GetId(),
                                    p->btn_prev->GetId() });

    for (auto printer_picker: p->page_fff->printer_pickers)
        msw_buttons_rescale(this, em, printer_picker->get_button_indexes());

    p->init_dialog_size();

    Refresh();
}

void ConfigWizard::on_sys_color_changed()
{
    wxGetApp().UpdateDlgDarkUI(this);
    Refresh();
}

}
}

#include "Plater.hpp"

#include <cstddef>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <regex>
#include <future>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/convert.hpp>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/bmpcbox.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <wx/dnd.h>
#include <wx/progdlg.h>
#include <wx/wupdlock.h>
#include <wx/numdlg.h>
#include <wx/debug.h>
#include <wx/busyinfo.h>
#include <wx/stdpaths.h>
#include <wx/tarstrm.h>
#ifdef _WIN32
#include <wx/richtooltip.h>
#include <wx/custombgwin.h>
#include <wx/popupwin.h>
#endif

#include "libslic3r/libslic3r.h"
#include "libslic3r/Format/STL.hpp"
#include "libslic3r/Format/AMF.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Format/OBJ.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "libslic3r/GCode/Thumbnails.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/SLA/Hollowing.hpp"
#include "libslic3r/SLA/SupportPoint.hpp"
#include "libslic3r/SLA/ReprojectPointsOnMesh.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/miniz_extension.hpp"
#include "../AnkerComFunction.hpp"

// For stl export
#include "libslic3r/CSGMesh/ModelToCSGMesh.hpp"
#include "libslic3r/CSGMesh/PerformCSGMeshBooleans.hpp"

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "GUI_ObjectList.hpp"
#include "GUI_ObjectManipulation.hpp"
//#include "GUI_ObjectLayers.hpp"
#include "GUI_Utils.hpp"
#include "GUI_Geometry.hpp"
#include "GUI_Factories.hpp"
#include "wxExtensions.hpp"
#include "MainFrame.hpp"
#include "format.hpp"
#include "3DScene.hpp"
#include "GLCanvas3D.hpp"
#include "Selection.hpp"
#include "GLToolbar.hpp"
#include "GUI_Preview.hpp"
#include "3DBed.hpp"
#include "Camera.hpp"
#include "Mouse3DController.hpp"
#include "Tab.hpp"
#include "Jobs/ArrangeJob.hpp"
#include "Jobs/OrientJob.hpp"
#include "Jobs/FillBedJob.hpp"
#include "Jobs/RotoptimizeJob.hpp"
#include "Jobs/SLAImportJob.hpp"
#include "Jobs/SLAImportDialog.hpp"
#include "Jobs/NotificationProgressIndicator.hpp"
#include "Jobs/PlaterWorker.hpp"
#include "Jobs/BoostThreadWorker.hpp"
#include "BackgroundSlicingProcess.hpp"
#include "PrintHostDialogs.hpp"
#include "ConfigWizard.hpp"
#include "../Utils/ASCIIFolding.hpp"
#include "../Utils/PrintHost.hpp"
#include "../Utils/FixModelByWin10.hpp"
#include "../Utils/UndoRedo.hpp"
#include "../Utils/PresetUpdater.hpp"
#include "../Utils/Process.hpp"
#include "RemovableDriveManager.hpp"
#include "InstanceCheck.hpp"
#include "NotificationManager.hpp"
#include "PresetComboBoxes.hpp"
#include "MsgDialog.hpp"
#include "ProjectDirtyStateManager.hpp"
#include "Gizmos/GLGizmoSimplify.hpp" // create suggestion notification
#include "Gizmos/GLGizmoCut.hpp"
#include "FileArchiveDialog.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "Common/AnkerNumberEnterDialog.hpp"
#include "Common/AnkerMsgDialog.hpp"
#include "Network/chooseDeviceFrame.h"

#ifdef __APPLE__
#include "Gizmos/GLGizmosManager.hpp"
#endif // __APPLE__

#include <wx/glcanvas.h>    // Needs to be last because reasons :-/
#include "WipeTowerDialog.hpp"

#include "libslic3r/CustomGCode.hpp"
#include "libslic3r/Platform.hpp"

#include "../Utils/DataMangerUi.hpp"
#include <boost/bind/bind.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/filesystem.hpp>
#include "libslic3r/Utils.hpp"

#include "AnkerGcodePreviewSideBar.hpp"
#include <slic3r/GUI/AnkerHint.h>
#include "slic3r/GUI/WebWeak/WebDownloadController.hpp"
#include "slic3r/GUI/Calibration/FlowCalibration.hpp"
#include "AnkerNetBase.h"

using boost::optional;
namespace fs = boost::filesystem;
using Slic3r::_3DScene;
using Slic3r::Preset;
using Slic3r::PrintHostJob;
using Slic3r::GUI::format_wxstr;
extern AnkerPlugin* pAnkerPlugin;
static const std::pair<unsigned int, unsigned int> THUMBNAIL_SIZE_3MF = { 256, 256 };

wxDEFINE_EVENT(wxCUSTOMEVT_EXPORT_FINISHED_SAFE_QUIT_APP, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SLICE_FOR_COMMENT, wxCommandEvent);

namespace Slic3r {
    namespace GUI {
        // Trigger Plater::schedule_background_process().
        wxDEFINE_EVENT(EVT_SCHEDULE_BACKGROUND_PROCESS, SimpleEvent);
        wxDEFINE_EVENT(EVT_SLICING_BEGAN, wxCommandEvent);
        // BackgroundSlicingProcess updates UI with slicing progress: Status bar / progress bar has to be updated, possibly scene has to be refreshed,
        // see PrintBase::SlicingStatus for the content of the message.
        wxDEFINE_EVENT(EVT_SLICING_UPDATE, SlicingStatusEvent);
        // FDM slicing finished, but G-code was not exported yet. Initial G-code preview shall be displayed by the UI.
        wxDEFINE_EVENT(EVT_SLICING_COMPLETED, wxCommandEvent);
        // BackgroundSlicingProcess finished either with success or error.
        wxDEFINE_EVENT(EVT_PROCESS_COMPLETED, SlicingProcessCompletedEvent);
        wxDEFINE_EVENT(EVT_EXPORT_BEGAN, wxCommandEvent);



        bool Plater::has_illegal_filename_characters(const wxString& wxs_name)
        {
            std::string name = into_u8(wxs_name);
            return has_illegal_filename_characters(name);
        }

        bool Plater::has_illegal_filename_characters(const std::string& name)
        {
            const char* illegal_characters = "<>:/\\|?*\"";
            for (size_t i = 0; i < std::strlen(illegal_characters); i++)
                if (name.find_first_of(illegal_characters[i]) != std::string::npos)
                    return true;

            return false;
        }

        void Plater::show_illegal_characters_warning(wxWindow* parent)
        {
            show_error(parent, _L("The provided name is not valid;") + "\n" +
                _L("the following characters are not allowed:") + " <>:/\\|?*\"");
        }

        // Sidebar widgets

        // struct InfoBox : public wxStaticBox
        // {
        //     InfoBox(wxWindow *parent, const wxString &label) :
        //         wxStaticBox(parent, wxID_ANY, label)
        //     {
        //         SetFont(GUI::small_font().Bold());
        //     }
        // };

        class ObjectInfo : public wxStaticBoxSizer
        {
            std::string m_warning_icon_name{ "exclamation" };
        public:
            ObjectInfo(wxWindow* parent);

            wxStaticBitmap* manifold_warning_icon;
            wxStaticBitmap* info_icon;
            wxStaticText* info_size;
            wxStaticText* info_volume;
            wxStaticText* info_facets;
            //    wxStaticText *info_materials;
            wxStaticText* info_manifold;

            wxStaticText* label_volume;
            //    wxStaticText *label_materials; // ysFIXME - delete after next release if anyone will not complain about this
            std::vector<wxStaticText*> sla_hidden_items;

            bool        showing_manifold_warning_icon;
            void        show_sizer(bool show);
            void        update_warning_icon(const std::string& warning_icon_name);
        };

        ObjectInfo::ObjectInfo(wxWindow* parent) :
            wxStaticBoxSizer(new wxStaticBox(parent, wxID_ANY, _L("Info")), wxVERTICAL)
        {
            GetStaticBox()->SetFont(wxGetApp().bold_font());
            wxGetApp().UpdateDarkUI(GetStaticBox());

            auto* grid_sizer = new wxFlexGridSizer(4, 5, 15);
            grid_sizer->SetFlexibleDirection(wxHORIZONTAL);

            auto init_info_label = [parent, grid_sizer](wxStaticText** info_label, wxString text_label, wxSizer* sizer_with_icon = nullptr) {
                auto* text = new wxStaticText(parent, wxID_ANY, text_label + ":");
                text->SetFont(wxGetApp().small_font());
                *info_label = new wxStaticText(parent, wxID_ANY, "");
                (*info_label)->SetFont(wxGetApp().small_font());
                grid_sizer->Add(text, 0);
                if (sizer_with_icon) {
                    sizer_with_icon->Insert(0, *info_label, 0);
                    grid_sizer->Add(sizer_with_icon, 0, wxEXPAND);
                }
                else
                    grid_sizer->Add(*info_label, 0);
                return text;
            };

            init_info_label(&info_size, _L("Size"));

            info_icon = new wxStaticBitmap(parent, wxID_ANY, *get_bmp_bundle("info"));
            info_icon->SetToolTip(_L("For a multipart object, this value isn't accurate.\n"
                "It doesn't take account of intersections and negative volumes."));
            auto* volume_info_sizer = new wxBoxSizer(wxHORIZONTAL);
            volume_info_sizer->Add(info_icon, 0, wxLEFT, 10);
            label_volume = init_info_label(&info_volume, _L("Volume"), volume_info_sizer);

            init_info_label(&info_facets, _L("Facets"));
            //    label_materials = init_info_label(&info_materials, _L("Materials"));
            Add(grid_sizer, 0, wxEXPAND);

            info_manifold = new wxStaticText(parent, wxID_ANY, "");
            info_manifold->SetFont(wxGetApp().small_font());
            manifold_warning_icon = new wxStaticBitmap(parent, wxID_ANY, *get_bmp_bundle(m_warning_icon_name));
            auto* sizer_manifold = new wxBoxSizer(wxHORIZONTAL);
            sizer_manifold->Add(manifold_warning_icon, 0, wxLEFT, 2);
            sizer_manifold->Add(info_manifold, 0, wxLEFT, 2);
            Add(sizer_manifold, 0, wxEXPAND | wxTOP, 4);

            sla_hidden_items = { label_volume, info_volume, /*label_materials, info_materials*/ };

            // Fixes layout issues on plater, short BitmapComboBoxes with some Windows scaling, see GH issue #7414.
            this->Show(false);
        }

        void ObjectInfo::show_sizer(bool show)
        {
            Show(show);
            if (show)
                manifold_warning_icon->Show(showing_manifold_warning_icon && show);
        }

        void ObjectInfo::update_warning_icon(const std::string& warning_icon_name)
        {
            if ((showing_manifold_warning_icon = !warning_icon_name.empty())) {
                m_warning_icon_name = warning_icon_name;
                manifold_warning_icon->SetBitmap(*get_bmp_bundle(m_warning_icon_name));
            }
        }

        enum SlicedInfoIdx
        {
            siFilament_g,
            siFilament_m,
            siFilament_mm3,
            siMateril_unit,
            siCost,
            siEstimatedTime,
            siWTNumbetOfToolchanges,

            siCount
        };

        class SlicedInfo : public wxStaticBoxSizer
        {
        public:
            SlicedInfo(wxWindow* parent);
            void SetTextAndShow(SlicedInfoIdx idx, const wxString& text, const wxString& new_label = "");

        private:
            std::vector<std::pair<wxStaticText*, wxStaticText*>> info_vec;
        };

        SlicedInfo::SlicedInfo(wxWindow* parent) :
            wxStaticBoxSizer(new wxStaticBox(parent, wxID_ANY, _L("Sliced Info")), wxVERTICAL)
        {
            GetStaticBox()->SetFont(wxGetApp().bold_font());
            wxGetApp().UpdateDarkUI(GetStaticBox());

            auto* grid_sizer = new wxFlexGridSizer(2, 5, 15);
            grid_sizer->SetFlexibleDirection(wxVERTICAL);

            info_vec.reserve(siCount);

            auto init_info_label = [this, parent, grid_sizer](wxString text_label) {
                auto* text = new wxStaticText(parent, wxID_ANY, text_label);
                text->SetFont(wxGetApp().small_font());
                auto info_label = new wxStaticText(parent, wxID_ANY, "N/A");
                info_label->SetFont(wxGetApp().small_font());
                grid_sizer->Add(text, 0);
                grid_sizer->Add(info_label, 0);
                info_vec.push_back(std::pair<wxStaticText*, wxStaticText*>(text, info_label));
            };

            init_info_label(_L("Used Filament (g)"));
            init_info_label(_L("Used Filament (m)"));
            init_info_label(_L("Used Filament (mm³)"));
            init_info_label(_L("Used Material (unit)"));
            init_info_label(_L("Cost (money)"));
            init_info_label(_L("Estimated printing time"));
            init_info_label(_L("Number of tool changes"));

            Add(grid_sizer, 0, wxEXPAND);
            this->Show(false);
        }

        void SlicedInfo::SetTextAndShow(SlicedInfoIdx idx, const wxString& text, const wxString& new_label/*=""*/)
        {
            const bool show = text != "N/A";
            if (show)
                info_vec[idx].second->SetLabelText(text);
            if (!new_label.IsEmpty())
                info_vec[idx].first->SetLabelText(new_label);
            info_vec[idx].first->Show(show);
            info_vec[idx].second->Show(show);
        }

        // Frequently changed parameters

        class FreqChangedParams : public OG_Settings
        {
            double		    m_brim_width = 0.0;
            wxButton* m_wiping_dialog_button{ nullptr };
            wxSizer* m_sizer{ nullptr };

            std::shared_ptr<ConfigOptionsGroup> m_og_sla;
            std::vector<ScalableButton*>        m_empty_buttons;
        public:
            FreqChangedParams(wxWindow* parent);
            ~FreqChangedParams() {}

            wxButton* get_wiping_dialog_button() { return m_wiping_dialog_button; }
            wxSizer* get_sizer() override;
            ConfigOptionsGroup* get_og(const bool is_fff);
            void            Show(const bool is_fff) override;

            void            msw_rescale();
            void            sys_color_changed();
        };

        void FreqChangedParams::msw_rescale()
        {
            m_og->msw_rescale();
            m_og_sla->msw_rescale();
        }

        void FreqChangedParams::sys_color_changed()
        {
            m_og->sys_color_changed();
            m_og_sla->sys_color_changed();

            for (auto btn : m_empty_buttons)
                btn->sys_color_changed();

            wxGetApp().UpdateDarkUI(m_wiping_dialog_button, true);
        }

        FreqChangedParams::FreqChangedParams(wxWindow* parent) :
            OG_Settings(parent, false)
        {
            DynamicPrintConfig* config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;

            // Frequently changed parameters for FFF_technology
            m_og->set_config(config);
            m_og->hide_labels();

            m_og->m_on_change = [config, this](t_config_option_key opt_key, boost::any value) {
                Tab* tab_print = wxGetApp().get_tab(Preset::TYPE_PRINT);
                if (!tab_print) return;

                if (opt_key == "fill_density") {
                    tab_print->update_dirty();
                    tab_print->reload_config();
                    tab_print->update();
                }
                else
                {
                    DynamicPrintConfig new_conf = *config;
                    if (opt_key == "brim") {
                        double new_val;
                        double brim_width = config->opt_float("brim_width");
                        if (boost::any_cast<bool>(value) == true)
                        {
                            new_val = m_brim_width == 0.0 ? 5 :
                                m_brim_width < 0.0 ? m_brim_width * (-1) :
                                m_brim_width;
                        }
                        else {
                            m_brim_width = brim_width * (-1);
                            new_val = 0;
                        }
                        new_conf.set_key_value("brim_width", new ConfigOptionFloat(new_val));
                    }
                    else {
                        assert(opt_key == "support");
                        const wxString& selection = boost::any_cast<wxString>(value);
                        PrinterTechnology printer_technology = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology();

                        auto support_material = selection == _("None") ? false : true;
                        new_conf.set_key_value("support_material", new ConfigOptionBool(support_material));

                        if (selection == _("Everywhere")) {
                            new_conf.set_key_value("support_material_buildplate_only", new ConfigOptionBool(false));
                            if (printer_technology == ptFFF)
                                new_conf.set_key_value("support_material_auto", new ConfigOptionBool(true));
                        }
                        else if (selection == _("Support on build plate only")) {
                            new_conf.set_key_value("support_material_buildplate_only", new ConfigOptionBool(true));
                            if (printer_technology == ptFFF)
                                new_conf.set_key_value("support_material_auto", new ConfigOptionBool(true));
                        }
                        else if (selection == _("For support enforcers only")) {
                            assert(printer_technology == ptFFF);
                            new_conf.set_key_value("support_material_buildplate_only", new ConfigOptionBool(false));
                            new_conf.set_key_value("support_material_auto", new ConfigOptionBool(false));
                        }
                    }
                    tab_print->load_config(new_conf);
                }
            };


            Line line = Line{ "", "" };

            ConfigOptionDef support_def;
            support_def.label = L("Supports");
            support_def.type = coStrings;
            support_def.tooltip = L("Select what kind of support do you need");
            support_def.set_enum_labels(ConfigOptionDef::GUIType::select_open, {
                L("None"),
                L("Support on build plate only"),
                L("For support enforcers only"),
                L("Everywhere")
                });
            support_def.set_default_value(new ConfigOptionStrings{ "None" });
            Option option = Option(support_def, "support");
            option.opt.full_width = true;
            line.append_option(option);

            /* Not a best solution, but
             * Temporary workaround for right border alignment
             */
            auto empty_widget = [this](wxWindow* parent) {
                auto sizer = new wxBoxSizer(wxHORIZONTAL);
                auto btn = new ScalableButton(parent, wxID_ANY, "mirroring_transparent", wxEmptyString,
                    wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER | wxTRANSPARENT_WINDOW);
                sizer->Add(btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, int(0.3 * wxGetApp().em_unit()));
                m_empty_buttons.push_back(btn);
                return sizer;
            };
            line.append_widget(empty_widget);

            m_og->append_line(line);


            line = Line{ "", "" };

            option = m_og->get_option("fill_density");
            option.opt.label = L("Infill");
            option.opt.width = 8;
            option.opt.sidetext = "   ";
            line.append_option(option);

            m_brim_width = config->opt_float("brim_width");
            ConfigOptionDef def;
            def.label = L("Brim");
            def.type = coBool;
            def.tooltip = L("This flag enables the brim that will be printed around each object on the first layer.");
            def.gui_type = ConfigOptionDef::GUIType::undefined;
            def.set_default_value(new ConfigOptionBool{ m_brim_width > 0.0 ? true : false });
            option = Option(def, "brim");
            option.opt.sidetext = "";
            line.append_option(option);

            auto wiping_dialog_btn = [this](wxWindow* parent) {
                m_wiping_dialog_button = new wxButton(parent, wxID_ANY, _L("Purging volumes") + dots, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
                m_wiping_dialog_button->SetFont(wxGetApp().normal_font());
                wxGetApp().UpdateDarkUI(m_wiping_dialog_button, true);

                auto sizer = new wxBoxSizer(wxHORIZONTAL);
                sizer->Add(m_wiping_dialog_button, 0, wxALIGN_CENTER_VERTICAL);
                m_wiping_dialog_button->Bind(wxEVT_BUTTON, ([parent](wxCommandEvent& e)
                    {
                        auto& project_config = wxGetApp().preset_bundle->project_config;
                        const std::vector<double>& init_matrix = (project_config.option<ConfigOptionFloats>("wiping_volumes_matrix"))->values;
                        const std::vector<double>& init_extruders = (project_config.option<ConfigOptionFloats>("wiping_volumes_extruders"))->values;

                        // add by allen for fixing use filament_colour instead of extruder_colour
                        //const std::vector<std::string> extruder_colours = wxGetApp().plater()->get_extruder_colors_from_plater_config();
                        const std::vector<std::string> extruder_colours = wxGetApp().plater()->get_filament_colors_from_plater_config();

                        WipingDialog dlg(parent, cast<float>(init_matrix), cast<float>(init_extruders), extruder_colours);

                        if (dlg.ShowModal() == wxID_OK) {
                            std::vector<float> matrix = dlg.get_matrix();
                            std::vector<float> extruders = dlg.get_extruders();
                            (project_config.option<ConfigOptionFloats>("wiping_volumes_matrix"))->values = std::vector<double>(matrix.begin(), matrix.end());
                            (project_config.option<ConfigOptionFloats>("wiping_volumes_extruders"))->values = std::vector<double>(extruders.begin(), extruders.end());
                            // Update Project dirty state, update application title bar.
                            wxGetApp().plater()->update_project_dirty_from_presets();
                            wxPostEvent(parent, SimpleEvent(EVT_SCHEDULE_BACKGROUND_PROCESS, parent));
                        }
                    }));

                auto btn = new ScalableButton(parent, wxID_ANY, "mirroring_transparent", wxEmptyString,
                    wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER | wxTRANSPARENT_WINDOW);
                sizer->Add(btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    int(0.3 * wxGetApp().em_unit()));
                m_empty_buttons.push_back(btn);

                return sizer;
            };
            line.append_widget(wiping_dialog_btn);
            m_og->append_line(line);

            m_og->activate();

            Choice* choice = dynamic_cast<Choice*>(m_og->get_field("support"));
            choice->suppress_scroll();

            // Frequently changed parameters for SLA_technology
            m_og_sla = std::make_shared<ConfigOptionsGroup>(parent, "");
            m_og_sla->hide_labels();
            DynamicPrintConfig* config_sla = &wxGetApp().preset_bundle->sla_prints.get_edited_preset().config;
            m_og_sla->set_config(config_sla);

            m_og_sla->m_on_change = [config_sla](t_config_option_key opt_key, boost::any value) {
                Tab* tab = wxGetApp().get_tab(Preset::TYPE_SLA_PRINT);
                if (!tab) return;

                DynamicPrintConfig new_conf = *config_sla;
                if (opt_key == "pad") {
                    const wxString& selection = boost::any_cast<wxString>(value);

                    const bool pad_enable = selection == _("None") ? false : true;
                    new_conf.set_key_value("pad_enable", new ConfigOptionBool(pad_enable));

                    if (selection == _("Below object"))
                        new_conf.set_key_value("pad_around_object", new ConfigOptionBool(false));
                    else if (selection == _("Around object"))
                        new_conf.set_key_value("pad_around_object", new ConfigOptionBool(true));
                }
                else
                {
                    assert(opt_key == "support");
                    const wxString& selection = boost::any_cast<wxString>(value);

                    const bool supports_enable = selection == _("None") ? false : true;
                    new_conf.set_key_value("supports_enable", new ConfigOptionBool(supports_enable));

                    std::string treetype = get_sla_suptree_prefix(new_conf);

                    if (selection == _("Everywhere")) {
                        new_conf.set_key_value(treetype + "support_buildplate_only", new ConfigOptionBool(false));
                        new_conf.set_key_value("support_enforcers_only", new ConfigOptionBool(false));
                    }
                    else if (selection == _("Support on build plate only")) {
                        new_conf.set_key_value(treetype + "support_buildplate_only", new ConfigOptionBool(true));
                        new_conf.set_key_value("support_enforcers_only", new ConfigOptionBool(false));
                    }
                    else if (selection == _("For support enforcers only")) {
                        new_conf.set_key_value("support_enforcers_only", new ConfigOptionBool(true));
                    }
                }

                tab->load_config(new_conf);
                tab->update_dirty();
            };

            line = Line{ "", "" };

            ConfigOptionDef support_def_sla = support_def;
            support_def_sla.set_default_value(new ConfigOptionStrings{ "None" });
            option = Option(support_def_sla, "support");
            option.opt.full_width = true;
            line.append_option(option);
            line.append_widget(empty_widget);
            m_og_sla->append_line(line);

            line = Line{ "", "" };

            ConfigOptionDef pad_def;
            pad_def.label = L("Pad");
            pad_def.type = coStrings;
            pad_def.tooltip = L("Select what kind of pad do you need");
            pad_def.set_enum_labels(ConfigOptionDef::GUIType::select_open, {
                L("None"),
                L("Below object"),
                L("Around object")
                });
            pad_def.set_default_value(new ConfigOptionStrings{ "Below object" });
            option = Option(pad_def, "pad");
            option.opt.full_width = true;
            line.append_option(option);
            line.append_widget(empty_widget);

            m_og_sla->append_line(line);

            m_og_sla->activate();
            choice = dynamic_cast<Choice*>(m_og_sla->get_field("support"));
            choice->suppress_scroll();
            choice = dynamic_cast<Choice*>(m_og_sla->get_field("pad"));
            choice->suppress_scroll();

            m_sizer = new wxBoxSizer(wxVERTICAL);
            m_sizer->Add(m_og->sizer, 0, wxEXPAND);
            m_sizer->Add(m_og_sla->sizer, 0, wxEXPAND);
        }


        wxSizer* FreqChangedParams::get_sizer()
        {
            return m_sizer;
        }

        void FreqChangedParams::Show(const bool is_fff)
        {
            const bool is_wdb_shown = m_wiping_dialog_button->IsShown();
            m_og->Show(is_fff);
            m_og_sla->Show(!is_fff);

            // correct showing of the FreqChangedParams sizer when m_wiping_dialog_button is hidden
            if (is_fff && !is_wdb_shown)
                m_wiping_dialog_button->Hide();
        }

        ConfigOptionsGroup* FreqChangedParams::get_og(const bool is_fff)
        {
            return is_fff ? m_og.get() : m_og_sla.get();
        }

        // Sidebar / private

        enum class ActionButtonType : int {
            abReslice,
            abExport,
            abSendGCode
        };

        struct Sidebar::priv
        {
            Plater* plater;

            wxScrolledWindow* scrolled;
            wxPanel* presets_panel; // Used for MSW better layouts

            ModeSizer* mode_sizer{ nullptr };
            wxFlexGridSizer* sizer_presets;
            PlaterPresetComboBox* combo_print;
            std::vector<PlaterPresetComboBox*> combos_filament;
            wxBoxSizer* sizer_filaments;
            PlaterPresetComboBox* combo_sla_print;
            PlaterPresetComboBox* combo_sla_material;
            PlaterPresetComboBox* combo_printer;

            wxBoxSizer* sizer_params;
            FreqChangedParams* frequently_changed_parameters{ nullptr };
            //ObjectList* object_list{ nullptr };
            ObjectManipulation* object_manipulation{ nullptr };
            ObjectSettings* object_settings{ nullptr };
            ObjectLayers* object_layers{ nullptr };
            ObjectInfo* object_info;
            SlicedInfo* sliced_info;

            wxButton* btn_export_gcode;
            wxButton* btn_reslice;
            wxButton* btn_print; // A key print btn add by aden.
            wxGauge* m_transferFileProgress; // File transfer progress.
            ScalableButton* btn_send_gcode;
            int transferFileProgressValue = 0;
            //ScalableButton *btn_eject_device;
            ScalableButton* btn_export_gcode_removable; //exports to removable drives (appears only if removable drive is connected)

            bool                is_collapsed{ false };
            Search::OptionsSearcher     searcher;

            priv(Plater* plater) : plater(plater) {}
            ~priv();

            void show_preset_comboboxes();

#ifdef _WIN32
            wxString btn_reslice_tip;
            void show_rich_tip(const wxString& tooltip, wxButton* btn);
            void hide_rich_tip(wxButton* btn);
#endif
        };

        Sidebar::priv::~priv()
        {
            delete object_manipulation;
            delete object_settings;
            delete frequently_changed_parameters;
            delete object_layers;
        }

        void Sidebar::priv::show_preset_comboboxes()
        {
            const bool showSLA = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptSLA;

            for (size_t i = 0; i < 4; ++i)
                sizer_presets->Show(i, !showSLA);

            for (size_t i = 4; i < 10; ++i)
                sizer_presets->Show(i, showSLA);

            //frequently_changed_parameters->Show(!showSLA);

            scrolled->GetParent()->Layout();
            scrolled->Refresh();
        }

#ifdef _WIN32
        using wxRichToolTipPopup = wxCustomBackgroundWindow<wxPopupTransientWindow>;
        static wxRichToolTipPopup* get_rtt_popup(wxButton* btn)
        {
            auto children = btn->GetChildren();
            for (auto child : children)
                if (child->IsShown())
                    return dynamic_cast<wxRichToolTipPopup*>(child);
            return nullptr;
        }

        void Sidebar::priv::show_rich_tip(const wxString& tooltip, wxButton* btn)
        {
            if (tooltip.IsEmpty())
                return;
            wxRichToolTip tip(tooltip, "");
            tip.SetIcon(wxICON_NONE);
            tip.SetTipKind(wxTipKind_BottomRight);
            tip.SetTitleFont(wxGetApp().normal_font());
            tip.SetBackgroundColour(wxGetApp().get_window_default_clr());

            tip.ShowFor(btn);
            // Every call of the ShowFor() creates new RichToolTip and show it.
            // Every one else are hidden. 
            // So, set a text color just for the shown rich tooltip
            if (wxRichToolTipPopup* popup = get_rtt_popup(btn)) {
                auto children = popup->GetChildren();
                for (auto child : children) {
                    child->SetForegroundColour(wxGetApp().get_label_clr_default());
                    // we neen just first text line for out rich tooltip
                    return;
                }
            }
        }

        void Sidebar::priv::hide_rich_tip(wxButton* btn)
        {
            if (wxRichToolTipPopup* popup = get_rtt_popup(btn))
                popup->Dismiss();
        }
#endif

        // Sidebar / public

        Sidebar::Sidebar(Plater* parent)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(27.4 * wxGetApp().em_unit(), -1)), p(new priv(parent))
        {
            p->scrolled = new wxScrolledWindow(this);
            //    p->scrolled->SetScrollbars(0, 100, 1, 2); // ys_DELETE_after_testing. pixelsPerUnitY = 100 from https://github.com/prusa3d/PrusaSlicer/commit/8f019e5fa992eac2c9a1e84311c990a943f80b01, 
                // but this cause the bad layout of the sidebar, when all infoboxes appear.
                // As a result we can see the empty block at the bottom of the sidebar
                // But if we set this value to 5, layout will be better
            p->scrolled->SetScrollRate(0, 5);

            SetFont(wxGetApp().normal_font());
#ifndef __APPLE__
#ifdef _WIN32
            wxGetApp().UpdateDarkUI(this);
            wxGetApp().UpdateDarkUI(p->scrolled);
#else
            SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif
#endif

            // Sizer in the scrolled area
            auto* scrolled_sizer = new wxBoxSizer(wxVERTICAL);
            p->scrolled->SetSizer(scrolled_sizer);

            // Sizer with buttons for mode changing
            //p->mode_sizer = new ModeSizer(p->scrolled, int(0.5 * wxGetApp().em_unit()));

            // The preset chooser
            p->sizer_presets = new wxFlexGridSizer(10, 1, 1, 2);
            p->sizer_presets->AddGrowableCol(0, 1);
            p->sizer_presets->SetFlexibleDirection(wxBOTH);

            bool is_msw = false;
#ifdef __WINDOWS__
            p->scrolled->SetDoubleBuffered(true);

            p->presets_panel = new wxPanel(p->scrolled, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
            wxGetApp().UpdateDarkUI(p->presets_panel);
            p->presets_panel->SetSizer(p->sizer_presets);

            is_msw = true;
#else
            p->presets_panel = p->scrolled;
#endif //__WINDOWS__

            p->sizer_filaments = new wxBoxSizer(wxVERTICAL);

            const int margin_5 = int(0.5 * wxGetApp().em_unit());// 5;

            auto init_combo = [this, margin_5](PlaterPresetComboBox** combo, wxString label, Preset::Type preset_type, bool filament) {
                auto text_and_btn_sizer = new wxBoxSizer(wxHORIZONTAL);

                auto* text = new wxStaticText(p->presets_panel, wxID_ANY, label);
                text->SetFont(wxGetApp().small_font());
                text_and_btn_sizer->Add(text, 1, wxEXPAND);

                *combo = new PlaterPresetComboBox(p->presets_panel, preset_type);

                if ((*combo)->edit_btn)
                    text_and_btn_sizer->Add((*combo)->edit_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                        int(0.3 * wxGetApp().em_unit()));

                auto* sizer_presets = this->p->sizer_presets;
                auto* sizer_filaments = this->p->sizer_filaments;
                // Hide controls, which will be shown/hidden in respect to the printer technology
                text->Show(preset_type == Preset::TYPE_PRINTER);
                if (!filament) {
                    text_and_btn_sizer->ShowItems(preset_type == Preset::TYPE_PRINTER);
                    sizer_presets->Add(text_and_btn_sizer, 0, wxEXPAND |
#ifdef __WXGTK3__
                        wxRIGHT, margin_5);
#else
                        wxBOTTOM, 1);
                    (void)margin_5; // supress unused capture warning
#endif // __WXGTK3__
                    sizer_presets->Add(*combo, 0, wxALIGN_LEFT | wxEXPAND | wxRIGHT, 4);
                }
                else {
                    sizer_filaments->Add(text_and_btn_sizer, 0, wxEXPAND |
#ifdef __WXGTK3__
                        wxRIGHT, margin_5);
#else
                        wxBOTTOM, 1);
#endif // __WXGTK3__
                    (*combo)->set_extruder_idx(0);
                    sizer_filaments->ShowItems(false);
                    sizer_presets->Add(sizer_filaments, 1, wxEXPAND);
                    sizer_presets->Add(*combo, 0, wxALIGN_LEFT | wxEXPAND | wxRIGHT, 4);
                }
            };

            p->combos_filament.push_back(nullptr);

#if !USE_SIDEBAR_NEW
            init_combo(&p->combo_printer, _L("Printer"), Preset::TYPE_PRINTER, false);
            init_combo(&p->combos_filament[0], _L("Filament"), Preset::TYPE_FILAMENT, true);
            init_combo(&p->combo_print, _L("Print settings"), Preset::TYPE_PRINT, false);
            init_combo(&p->combo_sla_print, _L("SLA print settings"), Preset::TYPE_SLA_PRINT, false);
            init_combo(&p->combo_sla_material, _L("SLA material"), Preset::TYPE_SLA_MATERIAL, false);
#endif

            p->sizer_params = new wxBoxSizer(wxVERTICAL);

            // Frequently changed parameters
            p->frequently_changed_parameters = new FreqChangedParams(p->scrolled);
            p->sizer_params->Add(p->frequently_changed_parameters->get_sizer(), 0, wxEXPAND | wxTOP | wxBOTTOM
#ifdef __WXGTK3__
                | wxRIGHT
#endif // __WXGTK3__
                , wxOSX ? 1 : margin_5);

            //// Object List
            //p->object_list = new ObjectList(p->scrolled);
            //p->sizer_params->Add(p->object_list->get_sizer(), 1, wxEXPAND);

            // Object Manipulations
            p->object_manipulation = new ObjectManipulation(p->scrolled);
            p->object_manipulation->Hide();
            p->sizer_params->Add(p->object_manipulation->get_sizer(), 0, wxEXPAND | wxTOP, margin_5);

            // Frequently Object Settings
            p->object_settings = new ObjectSettings(p->scrolled);
            p->object_settings->Hide();
            p->sizer_params->Add(p->object_settings->get_sizer(), 0, wxEXPAND | wxTOP, margin_5);

            // Object Layers
            //p->object_layers = new ObjectLayers(p->scrolled);
            //p->object_layers->Hide();
            //p->sizer_params->Add(p->object_layers->get_sizer(), 0, wxEXPAND | wxTOP, margin_5);

            // Info boxes
            p->object_info = new ObjectInfo(p->scrolled);
            p->sliced_info = new SlicedInfo(p->scrolled);

            // Sizer in the scrolled area
            /*if (p->mode_sizer)
                scrolled_sizer->Add(p->mode_sizer, 0, wxALIGN_CENTER_HORIZONTAL);*/

            int size_margin = wxGTK3 ? wxLEFT | wxRIGHT : wxLEFT;

            is_msw ?
                scrolled_sizer->Add(p->presets_panel, 0, wxEXPAND | size_margin, margin_5) :
                scrolled_sizer->Add(p->sizer_presets, 0, wxEXPAND | size_margin, margin_5);
            if (p->sizer_params) {
                p->sizer_params->Show(false);
            }
            /* scrolled_sizer->Add(p->sizer_params, 1, wxEXPAND | size_margin, margin_5);
             scrolled_sizer->Add(p->object_info, 0, wxEXPAND | wxTOP | size_margin, margin_5);
             scrolled_sizer->Add(p->sliced_info, 0, wxEXPAND | wxTOP | size_margin, margin_5);*/

             // Buttons underneath the scrolled area

             // rescalable bitmap buttons "Send to printer" and "Remove device" 

            auto init_scalable_btn = [this](ScalableButton** btn, const std::string& icon_name, wxString tooltip = wxEmptyString)
            {
#ifdef __APPLE__
                int bmp_px_cnt = 16;
#else
                int bmp_px_cnt = 32;
#endif //__APPLE__
                ScalableBitmap bmp = ScalableBitmap(this, icon_name, bmp_px_cnt);
                *btn = new ScalableButton(this, wxID_ANY, bmp, "", wxBU_EXACTFIT);

#ifdef _WIN32
                (*btn)->Bind(wxEVT_ENTER_WINDOW, [tooltip, btn, this](wxMouseEvent& event) {
                    p->show_rich_tip(tooltip, *btn);
                    event.Skip();
                    });
                (*btn)->Bind(wxEVT_LEAVE_WINDOW, [btn, this](wxMouseEvent& event) {
                    p->hide_rich_tip(*btn);
                    event.Skip();
                    });
#else
                (*btn)->SetToolTip(tooltip);
#endif // _WIN32
                (*btn)->Hide();
            };

            init_scalable_btn(&p->btn_send_gcode, "export_gcode", _L("Send to printer") + " " + GUI::shortkey_ctrl_prefix() + "Shift+G");
            //    init_scalable_btn(&p->btn_eject_device, "eject_sd"       , _L("Remove device ") + GUI::shortkey_ctrl_prefix() + "T");
            init_scalable_btn(&p->btn_export_gcode_removable, "export_to_sd", _L("Export to SD card / Flash drive") + " " + GUI::shortkey_ctrl_prefix() + "U");

            // regular buttons "Slice now" and "Export G-code" 

        //    const int scaled_height = p->btn_eject_device->GetBitmapHeight() + 4;
#ifdef _WIN32
            const int scaled_height = p->btn_export_gcode_removable->GetBitmapHeight();
#else
            const int scaled_height = p->btn_export_gcode_removable->GetBitmapHeight() + 4;
#endif
            auto init_btn = [this](wxButton** btn, wxString label, const int button_height) {
                *btn = new wxButton(this, wxID_ANY, label, wxDefaultPosition,
                    wxSize(-1, button_height), wxBU_EXACTFIT);
                (*btn)->SetFont(wxGetApp().bold_font());
                wxGetApp().UpdateDarkUI((*btn), true);
            };

#ifdef __APPLE__
            init_btn(&p->btn_export_gcode, _L("Export G-code") + dots, scaled_height);
            init_btn(&p->btn_reslice, _L("Slice now"), scaled_height);
            init_btn(&p->btn_print, _L("Go Print"), scaled_height);
#elif _WIN32
            p->btn_export_gcode = new wxButton(nullptr, wxID_ANY, _L("Export G-code"), wxDefaultPosition,
                wxSize(-1, scaled_height), wxBU_EXACTFIT);
            p->btn_export_gcode->Hide();
            p->btn_export_gcode->Disable();
            p->btn_reslice = new wxButton(nullptr, wxID_ANY, _L("Slice now"), wxDefaultPosition,
                wxSize(-1, scaled_height), wxBU_EXACTFIT);
            p->btn_reslice->Hide();
            p->btn_reslice->Disable();
            p->btn_print = new wxButton(nullptr, wxID_ANY, _L("Go Print"), wxDefaultPosition,
                wxSize(-1, scaled_height), wxBU_EXACTFIT);
            p->btn_print->Hide();
            p->btn_print->Disable();
#endif // __APPLE__



            //init_btn(&p->btn_export_gcode, _L("Export") + dots, scaled_height);
            //init_btn(&p->btn_print, _L("Print Now"), scaled_height);
            p->m_transferFileProgress = new wxGauge(
                this, (wxWindowID)wxID_ANY, 100,
                wxPoint(), wxSize(-1, scaled_height));
            wxGetApp().UpdateDarkUI(p->m_transferFileProgress, true);
            //p->m_transferFileProgress->SetForegroundColour(wxColour(0, 255, 0)); 
            p->m_transferFileProgress->Enable(false);
            p->m_transferFileProgress->Hide();
            enable_buttons(false);

            auto* btns_sizer = new wxBoxSizer(wxVERTICAL);

            auto* complect_btns_sizer = new wxBoxSizer(wxHORIZONTAL);
            //complect_btns_sizer->Add(p->btn_export_gcode, 1, wxEXPAND);
            complect_btns_sizer->AddSpacer(10);
            //complect_btns_sizer->Add(p->btn_print, 1, wxEXPAND);
            //complect_btns_sizer->Add(p->m_transferFileProgress, 1, wxEXPAND);
            btns_sizer->Add(p->btn_reslice, 0, wxEXPAND | wxTOP, margin_5);
            btns_sizer->Add(complect_btns_sizer, 0, wxEXPAND | wxTOP, margin_5);

            auto* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(p->scrolled, 1, wxEXPAND);
            sizer->Add(btns_sizer, 0, wxEXPAND | wxLEFT, margin_5);
            //SetSizer(sizer);
            DatamangerUi::GetInstance().setPlaterPtr(p->plater);

            // Events
            /*p->btn_export_gcode->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p->plater->export_gcode(false); });
            p->btn_print->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
                p->plater->a_key_print_clicked(); });*/
            p->btn_reslice->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                {
                    if (p->plater->canvas3D()->get_gizmos_manager().is_in_editing_mode(true))
                        return;

                    const bool export_gcode_after_slicing = wxGetKeyState(WXK_SHIFT);
                    if (export_gcode_after_slicing)
                        p->plater->export_gcode(true);
                    //else
                    //    p->plater->reslice();
                    p->plater->select_view_3D(VIEW_MODE_PREVIEW);
                });

#ifdef _WIN32
            p->btn_reslice->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
                p->show_rich_tip(p->btn_reslice_tip, p->btn_reslice);
            event.Skip();
                });
            p->btn_reslice->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
                p->hide_rich_tip(p->btn_reslice);
            event.Skip();
                });
#endif // _WIN32

            p->btn_send_gcode->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p->plater->send_gcode(); });
            //    p->btn_eject_device->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p->plater->eject_drive(); });
            p->btn_export_gcode_removable->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p->plater->export_gcode(true); });
        }

        Sidebar::~Sidebar() {}

        void Sidebar::init_filament_combo(PlaterPresetComboBox** combo, const int extr_idx)
        {
            *combo = new PlaterPresetComboBox(p->presets_panel, Slic3r::Preset::TYPE_FILAMENT);
            (*combo)->set_extruder_idx(extr_idx);

            auto combo_and_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
            combo_and_btn_sizer->Add(*combo, 1, wxEXPAND);
            combo_and_btn_sizer->Add((*combo)->edit_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                int(0.3 * wxGetApp().em_unit()));

            this->p->sizer_filaments->Add(combo_and_btn_sizer, 1, wxEXPAND |
#ifdef __WXGTK3__
                wxRIGHT, int(0.5 * wxGetApp().em_unit()));
#else
                wxBOTTOM, 1);
#endif // __WXGTK3__
        }

        void Sidebar::remove_unused_filament_combos(const size_t current_extruder_count)
        {
            if (current_extruder_count >= p->combos_filament.size())
                return;
            auto sizer_filaments = this->p->sizer_filaments;
            while (p->combos_filament.size() > current_extruder_count) {
                const int last = p->combos_filament.size() - 1;
                sizer_filaments->Remove(last);
                (*p->combos_filament[last]).Destroy();
                p->combos_filament.pop_back();
            }
        }

        void Sidebar::update_all_preset_comboboxes()
        {
            PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
            const auto print_tech = preset_bundle.printers.get_edited_preset().printer_technology();

            // Update the print choosers to only contain the compatible presets, update the dirty flags.
            if (print_tech == ptFFF)
                p->combo_print->update();
            else {
                p->combo_sla_print->update();
                p->combo_sla_material->update();
            }
            // Update the printer choosers, update the dirty flags.
            p->combo_printer->update();
            // Update the filament choosers to only contain the compatible presets, update the color preview,
            // update the dirty flags.
            if (print_tech == ptFFF) {
                for (PlaterPresetComboBox* cb : p->combos_filament)
                    cb->update();
            }
        }

        void Sidebar::update_presets(Preset::Type preset_type)
        {
            PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
            const auto print_tech = preset_bundle.printers.get_edited_preset().printer_technology();

            switch (preset_type) {
            case Preset::TYPE_FILAMENT:
            {
                const size_t extruder_cnt = print_tech != ptFFF ? 1 :
                    dynamic_cast<ConfigOptionFloats*>(preset_bundle.printers.get_edited_preset().config.option("nozzle_diameter"))->values.size();
                const size_t filament_cnt = p->combos_filament.size() > extruder_cnt ? extruder_cnt : p->combos_filament.size();

                if (filament_cnt == 1) {
                    // Single filament printer, synchronize the filament presets.
                    const std::string& name = preset_bundle.filaments.get_selected_preset_name();
                    preset_bundle.set_filament_preset(0, name);
                }

                for (size_t i = 0; i < filament_cnt; i++)
                    p->combos_filament[i]->update();

                break;
            }

            case Preset::TYPE_PRINT:
                p->combo_print->update();
                break;

            case Preset::TYPE_SLA_PRINT:
                p->combo_sla_print->update();
                break;

            case Preset::TYPE_SLA_MATERIAL:
                p->combo_sla_material->update();
                break;

            case Preset::TYPE_PRINTER:
            {
                update_all_preset_comboboxes();
                // CallAfter is really needed here to correct layout of the preset comboboxes,
                // when printer technology is changed during a project loading AND/OR switching the application mode.
                // Otherwise, some of comboboxes are invisible 
                CallAfter([this]() { p->show_preset_comboboxes(); });
                break;
            }

            default: break;
            }

            // Synchronize config.ini with the current selections.
            wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
        }

        void Sidebar::update_mode_sizer() const
        {
            if (p->mode_sizer)
                p->mode_sizer->SetMode(m_mode);
        }

        void Sidebar::change_top_border_for_mode_sizer(bool increase_border)
        {
            if (p->mode_sizer) {
                p->mode_sizer->set_items_flag(increase_border ? wxTOP : 0);
                p->mode_sizer->set_items_border(increase_border ? int(0.5 * wxGetApp().em_unit()) : 0);
            }
        }

        void Sidebar::update_reslice_btn_tooltip() const
        {
            wxString tooltip = wxString("Slice") + " [" + GUI::shortkey_ctrl_prefix() + "R]";
            if (m_mode != comSimple)
                tooltip += wxString("\n") + _L("Hold Shift to Slice & Export G-code");
#ifdef _WIN32
            p->btn_reslice_tip = tooltip;
#else
            p->btn_reslice->SetToolTip(tooltip);
#endif
        }

        void Sidebar::msw_rescale()
        {
            SetMinSize(wxSize(40 * wxGetApp().em_unit(), -1));

            for (PlaterPresetComboBox* combo : std::vector<PlaterPresetComboBox*>{ p->combo_print,
                                                                        p->combo_sla_print,
                                                                        p->combo_sla_material,
                                                                        p->combo_printer })
                combo->msw_rescale();
            for (PlaterPresetComboBox* combo : p->combos_filament)
                combo->msw_rescale();

            p->frequently_changed_parameters->msw_rescale();
            //p->object_list->msw_rescale();
            p->object_manipulation->msw_rescale();
            //p->object_layers->msw_rescale();

#ifdef _WIN32
            const int scaled_height = p->btn_export_gcode_removable->GetBitmapHeight();
#else
            const int scaled_height = p->btn_export_gcode_removable->GetBitmapHeight() + 4;
#endif
            //p->btn_export_gcode->SetMinSize(wxSize(-1, scaled_height));
            p->btn_reslice->SetMinSize(wxSize(-1, scaled_height));

            p->scrolled->Layout();

            p->searcher.dlg_msw_rescale();
        }

        void Sidebar::sys_color_changed()
        {
#ifdef _WIN32
            wxWindowUpdateLocker noUpdates(this);

            for (wxWindow* win : std::vector<wxWindow*>{ this, p->sliced_info->GetStaticBox(), p->object_info->GetStaticBox(), p->btn_reslice, p->btn_export_gcode })
                wxGetApp().UpdateDarkUI(win);
            for (wxWindow* win : std::vector<wxWindow*>{ p->scrolled, p->presets_panel })
                wxGetApp().UpdateAllStaticTextDarkUI(win);
            for (wxWindow* btn : std::vector<wxWindow*>{ p->btn_reslice, p->btn_export_gcode })
                wxGetApp().UpdateDarkUI(btn, true);

            if (p->mode_sizer)
                p->mode_sizer->sys_color_changed();
            p->frequently_changed_parameters->sys_color_changed();
            p->object_settings->sys_color_changed();
#endif

            for (PlaterPresetComboBox* combo : std::vector<PlaterPresetComboBox*>{ p->combo_print,
                                                                        p->combo_sla_print,
                                                                        p->combo_sla_material,
                                                                        p->combo_printer })
                combo->sys_color_changed();
            for (PlaterPresetComboBox* combo : p->combos_filament)
                combo->sys_color_changed();

            //p->object_list->sys_color_changed();
            p->object_manipulation->sys_color_changed();
            //p->object_layers->sys_color_changed();

            // btn...->msw_rescale() updates icon on button, so use it
            p->btn_send_gcode->sys_color_changed();
            //    p->btn_eject_device->msw_rescale();
            p->btn_export_gcode_removable->sys_color_changed();

            p->scrolled->Layout();
            p->scrolled->Refresh();

            p->searcher.dlg_sys_color_changed();
        }

        void Sidebar::update_mode_markers()
        {
            if (p->mode_sizer)
                p->mode_sizer->update_mode_markers();
        }

        void Sidebar::search()
        {
            p->searcher.search();
        }

        void Sidebar::jump_to_option(const std::string& opt_key, Preset::Type type, const std::wstring& category)
        {
            //const Search::Option& opt = p->searcher.get_option(opt_key, type);
            wxGetApp().get_tab(type)->activate_option(opt_key, category);
        }

        void Sidebar::jump_to_option(size_t selected)
        {
            const Search::Option& opt = p->searcher.get_option(selected);
            if (opt.type == Preset::TYPE_PREFERENCES)
                wxGetApp().open_preferences(opt.opt_key(), boost::nowide::narrow(opt.group));
            else
                wxGetApp().get_tab(opt.type)->activate_option(opt.opt_key(), boost::nowide::narrow(opt.category));
        }

        ObjectManipulation* Sidebar::obj_manipul()
        {
            return p->object_manipulation;
        }

        //ObjectList* Sidebar::obj_list()
        //{
        //    return p->object_list;
        //}

        ObjectSettings* Sidebar::obj_settings()
        {
            return p->object_settings;
        }

        ObjectLayers* Sidebar::obj_layers()
        {
            return p->object_layers;
        }

        wxScrolledWindow* Sidebar::scrolled_panel()
        {
            return p->scrolled;
        }

        wxPanel* Sidebar::presets_panel()
        {
            return p->presets_panel;
        }

        ConfigOptionsGroup* Sidebar::og_freq_chng_params(const bool is_fff)
        {
            return p->frequently_changed_parameters->get_og(is_fff);
        }

        wxButton* Sidebar::get_wiping_dialog_button()
        {
            return p->frequently_changed_parameters->get_wiping_dialog_button();
        }

        //void Sidebar::update_objects_list_extruder_column(size_t extruders_count)
        //{
        //    p->object_list->update_objects_list_extruder_column(extruders_count);
        //}

        void Sidebar::show_info_sizer()
        {
            Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
            ModelObjectPtrs objects = p->plater->model().objects;
            int obj_idx = selection.get_object_idx();

            if (m_mode < comExpert || objects.empty() || obj_idx < 0 || int(objects.size()) <= obj_idx ||
                objects[obj_idx]->volumes.empty() ||                                            // hack to avoid crash when deleting the last object on the bed
                (selection.is_single_full_object() && objects[obj_idx]->instances.size() > 1) ||
                !(selection.is_single_full_instance() || selection.is_single_volume())) {
                p->object_info->Show(false);
                return;
            }

            const ModelObject* model_object = objects[obj_idx];

            int inst_idx = selection.get_instance_idx();
            assert(inst_idx >= 0);

            bool imperial_units = wxGetApp().app_config->get_bool("use_inches");
            double koef = imperial_units ? ObjectManipulation::mm_to_in : 1.0f;

            ModelVolume* vol = nullptr;
            Transform3d t;
            if (selection.is_single_volume()) {
                std::vector<int> obj_idxs, vol_idxs;
                wxGetApp().obj_list()->get_selection_indexes(obj_idxs, vol_idxs);
                if (vol_idxs.size() != 1)
                    // Case when this fuction is called between update selection in ObjectList and on Canvas
                    // Like after try to delete last solid part in object, the object is selected in ObjectLIst when just a part is still selected on Canvas
                    // see https://github.com/prusa3d/PrusaSlicer/issues/7408
                    return;
                vol = model_object->volumes[vol_idxs[0]];
                t = model_object->instances[inst_idx]->get_matrix() * vol->get_matrix();
            }

            Vec3d size = vol ? vol->mesh().transformed_bounding_box(t).size() : model_object->instance_bounding_box(inst_idx).size();
            p->object_info->info_size->SetLabel(wxString::Format("%.2f x %.2f x %.2f", size(0) * koef, size(1) * koef, size(2) * koef));
            //    p->object_info->info_materials->SetLabel(wxString::Format("%d", static_cast<int>(model_object->materials_count())));
            auto plater = wxGetApp().plater();
            if (plater) {
                //plater->setModelObjectSizeText(wxString::Format("%.2f x %.2f x %.2f mm", size(0) * koef, size(1) * koef, size(2) * koef));
                plater->setModelObjectSizeText(wxString::Format("%.2f X %.2f X %.2f mm", size(0) * koef, size(1) * koef, size(2) * koef));
            }

            const TriangleMeshStats& stats = vol ? vol->mesh().stats() : model_object->get_object_stl_stats();

            double volume_val = stats.volume;
            if (vol)
                volume_val *= std::fabs(t.matrix().block(0, 0, 3, 3).determinant());

            p->object_info->info_volume->SetLabel(wxString::Format("%.2f", volume_val * pow(koef, 3)));
            p->object_info->info_facets->SetLabel(format_wxstr(_L_PLURAL("%1% (%2$d shell)", "%1% (%2$d shells)", stats.number_of_parts),
                static_cast<int>(model_object->facets_count()), stats.number_of_parts));

            //wxString info_manifold_label;
            //auto mesh_errors = obj_list()->get_mesh_errors_info(&info_manifold_label);
            //wxString tooltip = mesh_errors.tooltip;
            //p->object_info->update_warning_icon(mesh_errors.warning_icon_name);
            //p->object_info->info_manifold->SetLabel(info_manifold_label);
            //p->object_info->info_manifold->SetToolTip(tooltip);
            //p->object_info->manifold_warning_icon->SetToolTip(tooltip);

            p->object_info->show_sizer(true);
            if (vol || model_object->volumes.size() == 1)
                p->object_info->info_icon->Hide();

            if (p->plater->printer_technology() == ptSLA) {
                for (auto item : p->object_info->sla_hidden_items)
                    item->Show(false);
            }
        }

        void Sidebar::update_sliced_info_sizer()
        {
            if (p->sliced_info->IsShown(size_t(0)))
            {
                if (p->plater->printer_technology() == ptSLA)
                {
                    const SLAPrintStatistics& ps = p->plater->sla_print().print_statistics();
                    wxString new_label = _L("Used Material (ml)") + ":";
                    const bool is_supports = ps.support_used_material > 0.0;
                    if (is_supports)
                        new_label += format_wxstr("\n    - %s\n    - %s", _L_PLURAL("object", "objects", p->plater->model().objects.size()), _L("supports and pad"));

                    wxString info_text = is_supports ?
                        wxString::Format("%.2f \n%.2f \n%.2f", (ps.objects_used_material + ps.support_used_material) / 1000,
                            ps.objects_used_material / 1000,
                            ps.support_used_material / 1000) :
                        wxString::Format("%.2f", (ps.objects_used_material + ps.support_used_material) / 1000);
                    p->sliced_info->SetTextAndShow(siMateril_unit, info_text, new_label);

                    wxString str_total_cost = "N/A";

                    DynamicPrintConfig* cfg = wxGetApp().get_tab(Preset::TYPE_SLA_MATERIAL)->get_config();
                    if (cfg->option("bottle_cost")->getFloat() > 0.0 &&
                        cfg->option("bottle_volume")->getFloat() > 0.0)
                    {
                        double material_cost = cfg->option("bottle_cost")->getFloat() /
                            cfg->option("bottle_volume")->getFloat();
                        str_total_cost = wxString::Format("%.3f", material_cost * (ps.objects_used_material + ps.support_used_material) / 1000);
                    }
                    p->sliced_info->SetTextAndShow(siCost, str_total_cost, "Cost");

                    wxString t_est = std::isnan(ps.estimated_print_time) ? "N/A" : get_time_dhms(float(ps.estimated_print_time));
                    p->sliced_info->SetTextAndShow(siEstimatedTime, t_est, _L("Estimated printing time") + ":");

                    p->plater->get_notification_manager()->set_slicing_complete_print_time(_u8L("Estimated printing time") + ": " + boost::nowide::narrow(t_est), p->plater->is_sidebar_collapsed());

                    // Hide non-SLA sliced info parameters
                    p->sliced_info->SetTextAndShow(siFilament_m, "N/A");
                    p->sliced_info->SetTextAndShow(siFilament_mm3, "N/A");
                    p->sliced_info->SetTextAndShow(siFilament_g, "N/A");
                    p->sliced_info->SetTextAndShow(siWTNumbetOfToolchanges, "N/A");
                }
                else
                {
                    const PrintStatistics& ps = p->plater->fff_print().print_statistics();
                    const bool is_wipe_tower = ps.total_wipe_tower_filament > 0;

                    bool imperial_units = wxGetApp().app_config->get_bool("use_inches");
                    double koef = imperial_units ? ObjectManipulation::in_to_mm : 1000.0;

                    wxString new_label = imperial_units ? _L("Used Filament (in)") : _L("Used Filament (m)");
                    if (is_wipe_tower)
                        new_label += format_wxstr(":\n    - %1%\n    - %2%", _L("objects"), _L("wipe tower"));

                    wxString info_text = is_wipe_tower ?
                        wxString::Format("%.2f \n%.2f \n%.2f", ps.total_used_filament / koef,
                            (ps.total_used_filament - ps.total_wipe_tower_filament) / koef,
                            ps.total_wipe_tower_filament / koef) :
                        wxString::Format("%.2f", ps.total_used_filament / koef);
                    p->sliced_info->SetTextAndShow(siFilament_m, info_text, new_label);

                    koef = imperial_units ? pow(ObjectManipulation::mm_to_in, 3) : 1.0f;
                    new_label = imperial_units ? _L("Used Filament (in³)") : _L("Used Filament (mm³)");
                    info_text = wxString::Format("%.2f", imperial_units ? ps.total_extruded_volume * koef : ps.total_extruded_volume);
                    p->sliced_info->SetTextAndShow(siFilament_mm3, info_text, new_label);

                    if (ps.total_weight == 0.0)
                        p->sliced_info->SetTextAndShow(siFilament_g, "N/A");
                    else {
                        new_label = _L("Used Filament (g)");
                        info_text = wxString::Format("%.2f", ps.total_weight);

                        const std::vector<std::string>& filament_presets = wxGetApp().preset_bundle->filament_presets;
                        const PresetCollection& filaments = wxGetApp().preset_bundle->filaments;

                        if (ps.filament_stats.size() > 1)
                            new_label += ":";

                        for (auto filament : ps.filament_stats) {
                            const Preset* filament_preset = filaments.find_preset(filament_presets[filament.first], false);
                            if (filament_preset) {
                                double filament_weight;
                                if (ps.filament_stats.size() == 1)
                                    filament_weight = ps.total_weight;
                                else {
                                    double filament_density = filament_preset->config.opt_float("filament_density", 0);
                                    filament_weight = filament.second * filament_density/* *2.4052f*/ * 0.001; // assumes 1.75mm filament diameter;

                                    new_label += "\n    - " + format_wxstr(_L("Filament at extruder %1%"), filament.first + 1);
                                    info_text += wxString::Format("\n%.2f", filament_weight);
                                }

                                double spool_weight = filament_preset->config.opt_float("filament_spool_weight", 0);
                                if (spool_weight != 0.0) {
                                    new_label += "\n      " + _L("(including spool)");
                                    info_text += wxString::Format(" (%.2f)\n", filament_weight + spool_weight);
                                }
                            }
                        }

                        p->sliced_info->SetTextAndShow(siFilament_g, info_text, new_label);
                    }

                    new_label = _L("Cost");
                    if (is_wipe_tower)
                        new_label += format_wxstr(":\n    - %1%\n    - %2%", _L("objects"), _L("wipe tower"));

                    info_text = ps.total_cost == 0.0 ? "N/A" :
                        is_wipe_tower ?
                        wxString::Format("%.2f \n%.2f \n%.2f", ps.total_cost,
                            (ps.total_cost - ps.total_wipe_tower_cost),
                            ps.total_wipe_tower_cost) :
                        wxString::Format("%.2f", ps.total_cost);
                    p->sliced_info->SetTextAndShow(siCost, info_text, new_label);

                    if (ps.estimated_normal_print_time == "N/A" && ps.estimated_silent_print_time == "N/A")
                        p->sliced_info->SetTextAndShow(siEstimatedTime, "N/A");
                    else {
                        info_text = "";
                        new_label = _L("Estimated printing time") + ":";
                        if (ps.estimated_normal_print_time != "N/A") {
                            new_label += format_wxstr("\n   - %1%", _L("normal mode"));
                            info_text += format_wxstr("\n%1%", short_time(ps.estimated_normal_print_time));

                            p->plater->get_notification_manager()->set_slicing_complete_print_time(_u8L("Estimated printing time") + ": " + ps.estimated_normal_print_time, p->plater->is_sidebar_collapsed());

                        }
                        if (ps.estimated_silent_print_time != "N/A") {
                            new_label += format_wxstr("\n   - %1%", _L("stealth mode"));
                            info_text += format_wxstr("\n%1%", short_time(ps.estimated_silent_print_time));
                        }
                        p->sliced_info->SetTextAndShow(siEstimatedTime, info_text, new_label);
                    }

                    // if there is a wipe tower, insert number of toolchanges info into the array:
                    p->sliced_info->SetTextAndShow(siWTNumbetOfToolchanges, is_wipe_tower ? wxString::Format("%.d", ps.total_toolchanges) : "N/A");

                    // Hide non-FFF sliced info parameters
                    p->sliced_info->SetTextAndShow(siMateril_unit, "N/A");
                }
            }

            Layout();
        }

        void Sidebar::show_sliced_info_sizer(const bool show)
        {
            wxWindowUpdateLocker freeze_guard(this);

            p->sliced_info->Show(show);
            if (show)
                update_sliced_info_sizer();

            Layout();
            p->scrolled->Refresh();
        }

        void Sidebar::enable_buttons(bool enable)
        {
            p->btn_reslice->Enable(enable);
            //p->btn_export_gcode->Enable(enable);
            p->btn_send_gcode->Enable(enable);
            //    p->btn_eject_device->Enable(enable);
            p->btn_export_gcode_removable->Enable(enable);
            // p->btn_print->Enable(enable);
        }

        bool Sidebar::show_reslice(bool show)          const { return p->btn_reslice->Show(show); }
        bool Sidebar::show_export(bool show)           const { return false/*p->btn_export_gcode->Show(show)*/; }
        bool Sidebar::show_send(bool show)             const { return p->btn_send_gcode->Show(show); }
        bool Sidebar::show_export_removable(bool show) const { return p->btn_export_gcode_removable->Show(show); }
        //bool Sidebar::show_eject(bool show)            const { return p->btn_eject_device->Show(show); }
        //bool Sidebar::get_eject_shown()                const { return p->btn_eject_device->IsShown(); }
        bool Sidebar::show_print_btn(bool show)             const { return false/*p->btn_print->Show(show)*/; }
        void Sidebar::updateFileTransferProgressValue(int value) { /*p->m_transferFileProgress->SetValue(value);*/ }
        bool Sidebar::show_send_progress(bool show)    const { show_print_btn(!show); return false;/* p->m_transferFileProgress->Show(show);*/ }
        void Sidebar::OnUpdateProgressEvent(wxThreadEvent& event) {
            int value = event.GetInt();
            ANKER_LOG_INFO << "Progress: " << value;
            p->m_transferFileProgress->SetValue(value);
        }

        bool Sidebar::is_multifilament()
        {
            return p->combos_filament.size() > 1;
        }

        void Sidebar::check_and_update_searcher(bool respect_mode /*= false*/)
        {
            std::vector<Search::InputInfo> search_inputs{};

            auto& tabs_list = wxGetApp().tabs_list;
            auto print_tech = wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology();
            for (auto tab : tabs_list)
                if (tab->supports_printer_technology(print_tech))
                    search_inputs.emplace_back(Search::InputInfo{ tab->get_config(), tab->type() });

            p->searcher.check_and_update(wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology(),
                respect_mode ? m_mode : comExpert,Preset::TYPE_COUNT ,search_inputs);
        }

        void Sidebar::update_mode()
        {
            m_mode = wxGetApp().get_mode();

            update_reslice_btn_tooltip();
            update_mode_sizer();

            wxWindowUpdateLocker noUpdates(this);

            if (m_mode == comSimple)
            {
                p->object_manipulation->set_coordinates_type(ECoordinatesType::World);
            }

            //p->object_list->get_sizer()->Show(m_mode > comSimple);

            //p->object_list->unselect_objects();
            //p->object_list->update_selections();

            Layout();
        }

        bool Sidebar::is_collapsed() { return p->is_collapsed; }

        void Sidebar::collapse(bool collapse)
        {
            p->is_collapsed = collapse;

            this->Show(!collapse);
            p->plater->Layout();

            // save collapsing state to the AppConfig
            if (wxGetApp().is_editor())
                wxGetApp().app_config->set("collapsed_sidebar", collapse ? "1" : "0");
        }

#ifdef _MSW_DARK_MODE
        void Sidebar::show_mode_sizer(bool show)
        {
            if (p->mode_sizer) {
                p->mode_sizer->Show(show);
            }
        }
#endif

        void Sidebar::update_ui_from_settings()
        {
            //p->object_manipulation->update_ui_from_settings();
            show_info_sizer();
            update_sliced_info_sizer();
            // update Cut gizmo, if it's open
            p->plater->canvas3D()->update_gizmos_on_off_state();
            p->plater->set_current_canvas_as_dirty();
            p->plater->get_current_canvas3D()->request_extra_frame();
            //p->object_list->apply_volumes_order();
        }

        std::vector<PlaterPresetComboBox*>& Sidebar::combos_filament()
        {
            return p->combos_filament;
        }

        Search::OptionsSearcher& Sidebar::get_searcher()
        {
            return p->searcher;
        }

        int Sidebar::getExportGCodeHeight()
        {
            if (p != nullptr) {
                p->btn_export_gcode_removable->GetBitmapHeight();
            }
            return -1;
        }

        std::string& Sidebar::get_search_line()
        {
            // update searcher before show imGui search dialog on the plater, if printer technology or mode was changed
            check_and_update_searcher(true);
            return p->searcher.search_string();
        }

        // Plater::DropTarget

        class PlaterDropTarget : public wxFileDropTarget
        {
        public:
            PlaterDropTarget(MainFrame& mainframe, Plater& plater) : m_mainframe(mainframe), m_plater(plater) {
                this->SetDefaultAction(wxDragCopy);
            }

            virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
            //if firstlimit gcode/acode file fail,return false; if other file, return true;  
            bool FirstLimitLoadSuccess(const wxArrayString& filenames);
            //if slicepanel can drag file,  device panel forbid drag file.
            bool IsDragPanel(const wxArrayString& filenames);
            bool IsAcodeOrGcodeFiles(const wxArrayString& filenames);

        private:
            MainFrame& m_mainframe;
            Plater& m_plater;
        };

        bool PlaterDropTarget::FirstLimitLoadSuccess(const wxArrayString& filenames){
            //if the file is gcode/acode file, yes to hide sidebarnew, cancel to forbid load 
            if (IsAcodeOrGcodeFiles(filenames)) {
                wxGetApp().mainframe->Raise();
                 //If the current plater doesn't existed Model,hide sidebarnew.
                if (!wxGetApp().plater()->is_plater_dirty()) {
                    //Quickly hide and layout the rightParamPanel
                    wxGetApp().plater()->sidebarnew().Hide();
                    wxGetApp().plater()->Layout();
                    wxGetApp().plater()->SetShowACodeOrGcodePrint(true);
                    return true;
                }

                //If the current plater existed Model,popup a dialog to hint the customer; 
                if (wxGetApp().plater()->new_project() == false) {
                    return false;
                } else {
                    //Quickly hide and layout the rightParamPanel
                    wxGetApp().plater()->sidebarnew().Hide();
                    wxGetApp().plater()->Layout();
                    wxGetApp().plater()->SetShowACodeOrGcodePrint(true);
                    return true;
                }
            }

            //if the file is other, return true indicate firstlimit success.
            return true;
        }

        bool PlaterDropTarget::IsDragPanel(const wxArrayString& filenames) {       
            //drag file to device panel is not allow
            if (!wxGetApp().plater()->IsShown()){
                std::map<std::string, std::string> buryMap;

                std::string handleType = std::string("import");
                buryMap.insert(std::make_pair(c_hm_type, handleType));

                wxString files = "";
                for (auto fileInfo : filenames) {
                    files += fileInfo.ToStdString() + " ";
                }
                std::string fileName = files.ToUTF8().data();
                buryMap.insert(std::make_pair(c_hm_file_name, fileName));

                std::string errorCode = std::string("0"); errorCode = -1;
                buryMap.insert(std::make_pair(c_hm_error_code, errorCode));

                std::string errorMsg = "drag file to device panel is not allow";
                buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                reportBuryEvent(e_hanlde_model, buryMap);
                return false;
            } else { //drag file to slice panel is allow
                return true;
            }
        }

        bool PlaterDropTarget::IsAcodeOrGcodeFiles(const wxArrayString& filenames) {
            for (const auto& filename : filenames) {
                if (is_gcode_file(into_u8(filename)) || is_acode_file(into_u8(filename))) {
                    return true;
                }
            }
            return false;
        }

        bool PlaterDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames){
            //only slice panel can drag file.
            if (!IsDragPanel(filenames)) { return false; }

            bool firstLimitSuccessFlag = FirstLimitLoadSuccess(filenames);
            if (!firstLimitSuccessFlag) { return false; }

#ifdef WIN32
            // hides the system icon
            this->MSWUpdateDragImageOnLeave();
#endif // WIN32
            m_mainframe.Raise();
            m_mainframe.select_tab(size_t(0));
            
            if (wxGetApp().is_editor())
                m_plater.select_view_3D(VIEW_MODE_3D);
            bool res = m_plater.load_files(filenames);
            m_mainframe.update_title();
            m_plater.set_view_drop_file(true);
            m_plater.SetDragGcodeFlag(true);
            m_plater.ShowHintDialogs(false);
            return res;
        }

        // State to manage showing after export notifications and device ejecting
        enum ExportingStatus {
            NOT_EXPORTING,
            EXPORTING_TO_REMOVABLE,
            EXPORTING_TO_LOCAL
        };

        // A key print data
        struct DeviceInfo {
            DeviceInfo() {};
            DeviceInfo(const std::string& sn, const std::string& deviceName, int id, bool canPrint) :
                m_canPrint(canPrint), m_id(id), m_sn(sn), m_deviceName(deviceName)
            {
            };
            bool m_canPrint = false;
            int m_id = -1;
            std::string m_sn;
            std::string m_deviceName;
        };
        struct AkeyPrintData {
            bool logined = false;
            std::string slicer_temp_gcode_path = "";// Gcode Path
            std::vector<DeviceInfo>  devices;
            int selectedDeviceId = -1;
        };

/*
        struct GcodeInfo
        {
            std::string base64_str;
            std::string speed;
            std::string use_filament_weight;
            std::string use_filament_length;
            std::string use_filament_money;
            std::array<float, 3> boxSize;
            float print_time;

            void reset() {
                base64_str.clear();
                speed.clear();
                use_filament_weight.clear();
                use_filament_length.clear();
                use_filament_money.clear();
                print_time = 0.0f;
                boxSize.fill(0.0);
            }
        };
*/
        // Plater / private
        struct Plater::priv
        {
            // PIMPL back pointer ("Q-Pointer")
            Plater* q;
            MainFrame* main_frame;

            MenuFactory menus;

            // Data
            Slic3r::DynamicPrintConfig* config;        // FIXME: leak?
            Slic3r::Print               fff_print;
            Slic3r::SLAPrint            sla_print;
            Slic3r::Model               model;
            PrinterTechnology           printer_technology = ptFFF;
            Slic3r::GCodeProcessorResult gcode_result;

            // GUI elements
            wxSizer* panel_sizer{ nullptr };
            wxPanel* current_panel{ nullptr };
            std::vector<wxPanel*> panels;
            Sidebar* sidebar{ nullptr };
            AnkerSidebarNew* sidebarnew{ nullptr };
            AnkerObjectBar* objectbar;
            AnkerObjectManipulator* aobjectmanipulator;
            AnkerFloatingList* floatinglist;

            AnkerObjectLayers* object_layers{ nullptr };

            AnkerHint* m_machineTypeMatchHint{ nullptr };
            AnkerHint* m_02mmPrinterHint{ nullptr };
            bool m_isPrint = false;
            int m_sliceTimes = -1;
            Bed3D bed;
            Camera camera;
#if ENABLE_ENVIRONMENT_MAP
            GLTexture environment_texture;
#endif // ENABLE_ENVIRONMENT_MAP
            Mouse3DController mouse3d_controller;
            View3D* view3D;
            GLToolbar view_toolbar;
            GLToolbar collapse_toolbar;
            Preview* preview;
            AssembleView* assemble_view{ nullptr };
            bool first_enter_assemble{ true };
            AnkerGcodePreviewSideBar* previewRightSidePanel{ nullptr };
            wxBoxSizer* previewRightSidePanelSizer{ nullptr };
            std::unique_ptr<NotificationManager> notification_manager;
            DynamicPrintConfig m_global_config;
            ProjectDirtyStateManager dirty_state;
            BackgroundSlicingProcess    background_process;
            bool suppressed_backround_processing_update{ false };

            // TODO: A mechanism would be useful for blocking the plater interactions:
            // objects would be frozen for the user. In case of arrange, an animation
            // could be shown, or with the optimize orientations, partial results
            // could be displayed.
            //
            // UIThreadWorker can be used as a replacement for BoostThreadWorker if
            // no additional worker threads are desired (useful for debugging or profiling)
            PlaterWorker<BoostThreadWorker> m_worker;
            SLAImportDialog* m_sla_import_dlg;

            ViewMode               current_view_mode;

            bool                        delayed_scene_refresh;
            std::string                 delayed_error_message;

            wxTimer                     background_process_timer;

            std::string                 label_btn_export;
            std::string                 label_btn_send;

            AkeyPrintData               a_key_print_data;

            bool                        show_render_statistic_dialog{ false };
            bool                        view_drop_file = false;
            progressChangeCallbackFunction export_progress_change_cb = nullptr;
            bool app_closing{ false };
            bool exporting_acode { false };
            bool cancel_export{ false };
            bool create_AI_file = false;
            std::string exported_file_cache;
            static const std::regex pattern_bundle;
            static const std::regex pattern_3mf;
            static const std::regex pattern_zip_amf;
            static const std::regex pattern_any_amf;
            static const std::regex pattern_anker;

            priv(Plater* q, MainFrame* main_frame);
            ~priv();
            bool is_plater_dirty() const { return dirty_state.is_plater_dirty(); }
            bool is_project_dirty() const { return dirty_state.is_dirty(); }
            bool is_presets_dirty() const { return dirty_state.is_presets_dirty(); }
            void update_project_dirty_from_presets() { dirty_state.update_from_presets(); }
            int save_project_if_dirty(const wxString& reason) {
                int res = wxID_NO;
                if (dirty_state.is_dirty()) {
                    MainFrame* mainframe = wxGetApp().mainframe;
                    if (mainframe->can_save_as()) {
                        wxString suggested_project_name;
                        wxString project_name = suggested_project_name = get_project_filename(".3mf");
                        if (suggested_project_name.IsEmpty()) {
                            fs::path output_file = get_export_file_path(FT_3MF);
                            suggested_project_name = output_file.empty() ? _L("Untitled") : from_u8(output_file.stem().string());
                        }

                        std::string act_key = "default_action_on_dirty_project";
                        std::string act = wxGetApp().app_config->get(act_key);
                        if (act.empty()) {
                            RichMessageDialog dialog(mainframe, reason + "\n" + format_wxstr(_L("Do you want to save the changes to \"%1%\"?"), suggested_project_name), wxString(SLIC3R_APP_NAME), wxYES_NO | wxCANCEL);
                            dialog.SetYesNoCancelLabels(_L("common_button_save"), _L("common_button_discard"), _L("common_button_cancel"));
                            dialog.ShowCheckBox(_L("Remember my choice"));
                            res = dialog.ShowModal();
                            if (res != wxID_CANCEL)
                                if (dialog.IsCheckBoxChecked()) {
                                    wxString preferences_item = _L("Ask for unsaved changes in project");
                                    wxString msg =
                                        _L("eufyMake Studio will remember your choice.") + "\n\n" +
                                        _L("You will not be asked about it again, when: \n"
                                            "- Closing eufyMake Studio,\n"
                                            "- Loading or creating a new project") + "\n\n" +
                                        format_wxstr(_L("Visit \"Preferences\" and check \"%1%\"\nto changes your choice."), preferences_item);

                                    MessageDialog msg_dlg(mainframe, msg, _L("eufyMake Studio: Don't ask me again"), wxOK | wxCANCEL | wxICON_INFORMATION);
                                    if (msg_dlg.ShowModal() == wxID_CANCEL)
                                        return wxID_CANCEL;

                                    get_app_config()->set(act_key, res == wxID_YES ? "1" : "0");
                                }
                        }
                        else
                            res = (act == "1") ? wxID_YES : wxID_NO;

                        if (res == wxID_YES)
                            if (!mainframe->save_project_as(project_name)) {
                                // Return Cancel only, when we don't remember a choice for closing the application.
                                // Elsewhere it can causes an impossibility to close the application at all.
                                res = act.empty() ? wxID_CANCEL : wxID_NO;
                            }
                    }
                }
                return res;
            }
            void reset_project_dirty_after_save() { m_undo_redo_stack_main.mark_current_as_saved(); dirty_state.reset_after_save(); }
            void reset_project_dirty_initial_presets() { dirty_state.reset_initial_presets(); }
         
       
#if ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW
            void render_project_state_debug_window() const { dirty_state.render_debug_window(); }
#endif // ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW

            enum class UpdateParams {
                FORCE_FULL_SCREEN_REFRESH = 1,
                FORCE_BACKGROUND_PROCESSING_UPDATE = 2,
                POSTPONE_VALIDATION_ERROR_MESSAGE = 4,
            };
            void update(unsigned int flags = 0);
            void select_view(const std::string& direction);
            void select_view_3D(ViewMode mode);
            void select_next_view_3D();

            wxBoxSizer* CreatePreViewRightSideBar();
            void updatePreViewRightSideBar(bool gcode_valid, RightSidePanelUpdateReason reason = REASON_NONE);
            void UpdateCurrentViewType(GCodeViewer::EViewType type);
            bool is_preview_shown() const { return current_panel == preview; }
            bool is_preview_loaded() const { return preview->is_loaded(); }
            bool is_view3D_shown() const { return current_panel == view3D; }
            bool is_assemble_view_show() const { return current_panel == assemble_view; }

            bool are_view3D_labels_shown() const { return (current_panel == view3D) && view3D->get_canvas3d()->are_labels_shown(); }
            void show_view3D_labels(bool show) { if (current_panel == view3D) view3D->get_canvas3d()->show_labels(show); }

            bool is_legend_shown() const { return (current_panel == preview) && preview->get_canvas3d()->is_legend_shown(); }
            void show_legend(bool show) { if (current_panel == preview) preview->get_canvas3d()->show_legend(show); }
            wxWindow* get_preview_toolbar() { return (preview ? preview->GetGcodeLayerToolbar() : nullptr); }
            void enable_moves_slider(bool enable) { if (preview) { preview->enable_moves_slider(enable); } }

            bool is_sidebar_collapsed() const { return sidebar->is_collapsed(); }
            void collapse_sidebar(bool collapse);

            bool is_view3D_layers_editing_enabled() const { return (current_panel == view3D) && view3D->get_canvas3d()->is_layers_editing_enabled(); }

            void set_current_canvas_as_dirty();
            GLCanvas3D* get_current_canvas3D();
            void unbind_canvas_event_handlers();
            void reset_canvas_volumes();

            bool init_view_toolbar();
            bool init_collapse_toolbar();

            void set_preview_layers_slider_values_range(int bottom, int top);

            void update_preview_moves_slider();
            void enable_preview_moves_slider(bool enable);

            void reset_gcode_toolpaths();

            void reset_all_gizmos();
            void apply_free_camera_correction(bool apply = true);
            void update_ui_from_settings();
            void update_main_toolbar_tooltips();
            //   std::shared_ptr<ProgressStatusBar> statusbar();
            bool get_config_bool(const std::string& key) const;

            void HintForMachine();

            std::vector<size_t> load_files(const std::vector<fs::path>& input_files, bool load_model, bool load_config, bool used_inches = false);
            std::vector<size_t> load_model_objects(const ModelObjectPtrs& model_objects, bool allow_negative_z = false, bool call_selection_changed = true, bool split_object = false);

            fs::path get_export_file_path(GUI::FileType file_type);
            wxString get_export_file(GUI::FileType file_type);

            const Selection& get_selection() const;
            Selection& get_selection();
            Selection& get_curr_selection();

            int get_selected_object_idx() const;
            int get_selected_volume_idx() const;
            void selection_changed();
            void object_list_changed();

            void select_all();
            void deselect_all();
            int get_object_count();
            void remove(size_t obj_idx);
            bool delete_object_from_model(size_t obj_idx);
            void delete_all_objects_from_model();
            void reset(std::string name = "");
            void mirror(Axis axis);
            void split_object();
            void split_volume();
            void scale_selection_to_fit_print_volume();

            // Return the active Undo/Redo stack. It may be either the main stack or the Gimzo stack.
            Slic3r::UndoRedo::Stack& undo_redo_stack() { assert(m_undo_redo_stack_active != nullptr); return *m_undo_redo_stack_active; }
            Slic3r::UndoRedo::Stack& undo_redo_stack_main() { return m_undo_redo_stack_main; }
            void enter_gizmos_stack();
            void leave_gizmos_stack();

            void take_snapshot(const std::string& snapshot_name, UndoRedo::SnapshotType snapshot_type = UndoRedo::SnapshotType::Action);
            void take_snapshot(const wxString& snapshot_name, UndoRedo::SnapshotType snapshot_type = UndoRedo::SnapshotType::Action)
            {
                this->take_snapshot(std::string(snapshot_name.ToUTF8().data()), snapshot_type);
            }
            int  get_active_snapshot_index();

            void undo();
            void redo();
            void undo_redo_to(size_t time_to_load);

            void suppress_snapshots() { m_prevent_snapshots++; }
            void allow_snapshots() { m_prevent_snapshots--; }

            void process_validation_warning(const std::string& warning) const;

            bool background_processing_enabled() const { return this->get_config_bool("background_processing"); }
            void update_print_volume_state();
            void schedule_background_process();
            // Update background processing thread from the current config and Model.
            enum UpdateBackgroundProcessReturnState {
                // update_background_process() reports, that the Print / SLAPrint was updated in a way,
                // that the background process was invalidated and it needs to be re-run.
                UPDATE_BACKGROUND_PROCESS_RESTART = 1,
                // update_background_process() reports, that the Print / SLAPrint was updated in a way,
                // that a scene needs to be refreshed (you should call _3DScene::reload_scene(canvas3Dwidget, false))
                UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE = 2,
                // update_background_process() reports, that the Print / SLAPrint is invalid, and the error message
                // was sent to the status line.
                UPDATE_BACKGROUND_PROCESS_INVALID = 4,
                // Restart even if the background processing is disabled.
                UPDATE_BACKGROUND_PROCESS_FORCE_RESTART = 8,
                // Restart for G-code (or SLA zip) export or upload.
                UPDATE_BACKGROUND_PROCESS_FORCE_EXPORT = 16,
            };
            // returns bit mask of UpdateBackgroundProcessReturnState
            unsigned int update_background_process(bool force_validation = false, bool postpone_error_messages = false);
            // Restart background processing thread based on a bitmask of UpdateBackgroundProcessReturnState.
            bool restart_background_process(unsigned int state);
            // returns bit mask of UpdateBackgroundProcessReturnState
            unsigned int update_restart_background_process(bool force_scene_update, bool force_preview_update);
            void show_delayed_error_message() {
                if (!this->delayed_error_message.empty()) {
                    std::string msg = std::move(this->delayed_error_message);
                    this->delayed_error_message.clear();
                    GUI::show_error(this->q, msg);
                }
            }
            void export_gcode(fs::path output_path, bool output_path_on_removable_media, PrintHostJob upload_job);
            void set_export_progress_change_callback(progressChangeCallbackFunction cb) { export_progress_change_cb = cb; }
            void set_app_closing(bool v) { app_closing = v; }
            bool is_exporting_acode() { return exporting_acode;}
            void cancel_exporting_acode() { cancel_export = true; }
            bool is_cancel_exporting_Gcode() { return cancel_export; }
            void set_create_AI_file(bool value) {
                create_AI_file = value;
                wxString str = wxString::Format("%d", static_cast<int>(create_AI_file));
                Slic3r::GUI::wxGetApp().app_config->set("create_AI_file", str.ToStdString(wxConvUTF8));
                //background_process.set_temp_output_path(create_AI_file);
            }
            bool get_create_AI_file() { return create_AI_file; };

            void update_exported_file_cache(fs::path path);
            void clear_exported_file_cache();

            void set_view_drop_file(bool val) { view_drop_file = val; }
            bool is_view_drop_file() { return view_drop_file; }

            void reload_from_disk();
            bool replace_volume_with_stl(int object_idx, int volume_idx, const fs::path& new_path, const wxString& snapshot = "");
            void replace_with_stl();
            void reload_all_from_disk();
            void set_current_panel(wxPanel* panel);

            void on_select_preset(wxCommandEvent&);
            void on_select_preset_sidebarnew(wxCommandEvent&);
            void on_slicing_update(SlicingStatusEvent&);
            void on_slicing_completed(wxCommandEvent&);
            void on_process_completed(SlicingProcessCompletedEvent&);
            void on_export_began(wxCommandEvent&);
            void on_layer_editing_toggled(bool enable);
            void on_slicing_began();

            void report_slicing_buryevt(const std::string& status, const std::string& errorCode, const std::string& errorMsg);

            void clear_warnings();
            void add_warning(const Slic3r::PrintStateBase::Warning& warning, size_t oid);
            // Update notification manager with the current state of warnings produced by the background process (slicing).
            void actualize_slicing_warnings(const PrintBase& print);
            void actualize_object_warnings(const PrintBase& print);
            // Displays dialog window with list of warnings. 
            // Returns true if user clicks OK.
            // Returns true if current_warnings vector is empty without showning the dialog
            bool warnings_dialog();

            void on_action_add(SimpleEvent&);
            void on_action_split_objects(SimpleEvent&);
            void on_action_split_volumes(SimpleEvent&);
            void on_action_layersediting(SimpleEvent&);

            //// web download model
            void on_action_request_model_download(const std::string& url);
            void on_download_complete(wxCommandEvent& evt);
            ////


            void on_object_select(SimpleEvent&);
            void on_right_click(RBtnEvent&);
            void on_wipetower_moved(Vec3dEvent&);
            void on_wipetower_rotated(Vec3dEvent&);
            void on_update_geometry(Vec3dsEvent<2>&);
            void on_3dcanvas_mouse_dragging_started(SimpleEvent&);
            void on_3dcanvas_mouse_dragging_finished(SimpleEvent&);

            void show_action_buttons(const bool is_ready_to_slice) const;

            // Set the bed shape to a single closed 2D polygon(array of two element arrays),
            // triangulate the bed and store the triangles into m_bed.m_triangles,
            // fills the m_bed.m_grid_lines and sets m_bed.m_origin.
            // Sets m_bed.m_polygon to limit the object placement.
            void set_bed_shape(const Pointfs& shape, const double max_print_height, const std::string& custom_texture, const std::string& custom_model, bool force_as_custom = false);

            bool can_delete() const;
            bool can_delete_all() const;
            bool can_increase_instances() const;
            bool can_decrease_instances(int obj_idx = -1) const;
            bool can_split_to_objects() const;
            bool can_split_to_volumes() const;
            bool can_arrange() const;
            bool can_auto_bed() const;
            bool can_layers_editing() const;
            bool can_fix_through_netfabb() const;
            bool can_simplify() const;
            bool can_set_instance_to_object() const;
            bool can_mirror() const;
            bool can_reload_from_disk() const;
            bool can_replace_with_stl() const;
            bool can_split(bool to_objects) const;
            bool can_scale_to_print_volume() const;

            //assmeble
            bool can_fillcolor() const;
            bool has_assemble_view() const;

            void generate_thumbnail(ThumbnailData& data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, Camera::EType camera_type);
            ThumbnailsList generate_thumbnails(const ThumbnailsParams& params, Camera::EType camera_type);

            void bring_instance_forward() const;

            // returns the path to project file with the given extension (none if extension == wxEmptyString)
            // extension should contain the leading dot, i.e.: ".3mf"
            wxString get_project_filename(const wxString& extension = wxEmptyString) const;
            void set_project_filename(const wxString& filename);
            // Call after plater and Canvas#D is initialized
            void init_notification_manager();

            // Caching last value of show_action_buttons parameter for show_action_buttons(), so that a callback which does not know this state will not override it.
            mutable bool    			ready_to_slice = { false };
            // Flag indicating that the G-code export targets a removable device, therefore the show_action_buttons() needs to be called at any case when the background processing finishes.
            ExportingStatus             exporting_status{ NOT_EXPORTING };
            std::string                 last_output_path;
            std::string                 last_output_dir_path;
            bool                        inside_snapshot_capture() { return m_prevent_snapshots != 0; }
            bool                        process_completed_with_error{ false };
            
            
            void _calib_pa_pattern(const Calib_Params& params);
            void _calib_pa_tower(const Calib_Params& params);
            void calib_flowrate(int pass);
            void calib_temp(const Calib_Params& params);
            void calib_retraction(const Calib_Params& params);
            void calib_max_vol_speed(const Calib_Params& params);
            void calib_VFA(const Calib_Params& params);
        private:
            bool layers_height_allowed() const;

            void update_fff_scene();
            void update_sla_scene();
            void undo_redo_to(std::vector<UndoRedo::Snapshot>::const_iterator it_snapshot);
            void update_after_undo_redo(const UndoRedo::Snapshot& snapshot, bool temp_snapshot_was_taken = false);

            // path to project file stored with no extension
            wxString 					m_project_filename;
            Slic3r::UndoRedo::Stack 	m_undo_redo_stack_main;
            Slic3r::UndoRedo::Stack 	m_undo_redo_stack_gizmos;
            Slic3r::UndoRedo::Stack* m_undo_redo_stack_active = &m_undo_redo_stack_main;
            int                         m_prevent_snapshots = 0;     /* Used for avoid of excess "snapshoting".
                                                                      * Like for "delete selected" or "set numbers of copies"
                                                                      * we should call tack_snapshot just ones
                                                                      * instead of calls for each action separately
                                                                      * */
            std::string 				m_last_fff_printer_profile_name;
            std::string 				m_last_sla_printer_profile_name;

            // vector of all warnings generated by last slicing
            std::vector<std::pair<Slic3r::PrintStateBase::Warning, size_t>> current_warnings;
            bool show_warning_dialog{ false };
            std::unique_ptr<WebDownloadController> m_downLoad_controller{ std::make_unique<WebDownloadController>() };
            std::unique_ptr<FlowCalibration> m_flowCalibration { std::make_unique<FlowCalibration>() };
        };

        const std::regex Plater::priv::pattern_bundle(".*[.](amf|amf[.]xml|zip[.]amf|3mf|akpro)", std::regex::icase);
        const std::regex Plater::priv::pattern_3mf(".*3mf", std::regex::icase);
        const std::regex Plater::priv::pattern_zip_amf(".*[.]zip[.]amf", std::regex::icase);
        const std::regex Plater::priv::pattern_any_amf(".*[.](amf|amf[.]xml|zip[.]amf)", std::regex::icase);
        const std::regex Plater::priv::pattern_anker(".*akpro", std::regex::icase);

        Plater::priv::priv(Plater* q, MainFrame* main_frame)
            : q(q)
            , main_frame(main_frame)
            , config(Slic3r::DynamicPrintConfig::new_from_defaults_keys({
            // add by allen for add anker_filament_id and anker_colour_id
                "anker_filament_id", "anker_colour_id", "bed_shape", "bed_custom_texture", "bed_custom_model", "complete_objects", "duplicate_distance", "extruder_clearance_radius", "skirts", "skirt_distance",
                "brim_width", "brim_separation", "brim_type", "variable_layer_height", "nozzle_diameter", "single_extruder_multi_material",
                "wipe_tower", "wipe_tower_x", "wipe_tower_y", "wipe_tower_width", "wipe_tower_rotation_angle", "wipe_tower_brim_width", "wipe_tower_cone_angle", "wipe_tower_extra_spacing",
                "extruder_colour", "filament_colour", "material_colour", "max_print_height", "printer_model", "printer_technology",
                // These values are necessary to construct SlicingParameters by the Canvas3D variable layer height editor.
                "layer_height", "first_layer_height", "min_layer_height", "max_layer_height",
                "brim_width", "perimeters", "perimeter_extruder", "fill_density", "infill_extruder", "top_solid_layers",
                "support_material", "support_material_extruder", "support_material_interface_extruder",
                "support_material_contact_distance", "support_material_bottom_contact_distance", "raft_layers"
                }))
            , sidebarnew(new AnkerSidebarNew(q))
            , sidebar(new Sidebar(q))
            , objectbar(nullptr)
            , aobjectmanipulator(new AnkerObjectManipulator(sidebarnew))
            , floatinglist(new AnkerFloatingList(q))
            , object_layers(new AnkerObjectLayers(sidebarnew))
            , notification_manager(std::make_unique<NotificationManager>(q))
            , m_worker{ q, std::make_unique<NotificationProgressIndicator>(notification_manager.get()), "ui_worker" }
            , m_sla_import_dlg{ new SLAImportDialog{q} }
            , delayed_scene_refresh(false)
            , view_toolbar(GLToolbar::Radio, "View")
            , collapse_toolbar(GLToolbar::Normal, "Collapse")
            , m_project_filename(wxEmptyString)
            , current_view_mode(VIEW_MODE_3D)
            , m_global_config((wxGetApp().preset_bundle->prints.get_edited_preset().config))
        {
            this->q->SetFont(Slic3r::GUI::wxGetApp().normal_font());
            aobjectmanipulator->Hide();
            floatinglist->Hide();
            main_frame->Bind(wxCUSTOMEVT_ANKER_MAINWIN_MOVE, [this](wxCommandEvent& event) {
                if(sidebarnew)
                    sidebarnew->hidePopupWidget();
             });
            main_frame->Bind(wxCUSTOMEVT_ANKER_RELOAD_DATA, [this](wxCommandEvent& event) {
                if (sidebarnew)
                    sidebarnew->reloadParameterData();
                });
            if (sidebarnew)
            {

                sidebarnew->Bind(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, [this, q](wxCommandEvent& event)
                    {
                        if (q->canvas3D()->get_gizmos_manager().is_in_editing_mode(true))
                            return;

                        const bool export_gcode_after_slicing = wxGetKeyState(WXK_SHIFT);

                        report_slicing_buryevt("start", "0","start slice model");

                        if (export_gcode_after_slicing)
                            q->export_gcode(true);
                        //else
                        //    q->reslice();
                        
                        if (current_view_mode == VIEW_MODE_PREVIEW) {
                            q->reslice();
                        }
                        else {
                            q->select_view_3D(VIEW_MODE_PREVIEW);   // here: switch to PREVIEW mode and reslice()
                        }                        
                    });

                sidebarnew->Bind(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, [this, q](wxCommandEvent& event)
                    {
                        wxString filePath = get_project_filename(".3mf");
                        if (q)
                        {
                            q->export_3mf(Slic3r::GUI::into_path(filePath));
                            wxGetApp().update_saved_preset_from_current_preset();
                            reset_project_dirty_after_save();
                        }
                    });

                sidebarnew->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, [this, q](wxCommandEvent& event)
                    {
                        //todo right parameterpanel value changed and update flags process_completed_with_error
                        //schedule_background_process();
                        static DynamicPrintConfig last_print_config;
                        const auto& current_print_config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
                        auto config = current_print_config;
                        this->sidebarnew->updatePreset(config);  // update config from  right side parameter panel
                        m_global_config.apply(config);

                        if (auto object_list = sidebarnew->object_list()) {
                            if (last_print_config.empty() || !current_print_config.equals(last_print_config)) {
                                object_list->reset_item_model_config(current_print_config);
                                last_print_config = current_print_config;
                            }
                            else {
                                object_list->update_item_model_config(config, last_print_config);
                            }
                        }

                        wxGetApp().mainframe->on_config_changed(&config);

                        sidebarnew->GetGlobalParameterPanel()->checkUIData(&config);
                    });

            }

            background_process.set_fff_print(&fff_print);
            background_process.set_sla_print(&sla_print);
            background_process.set_gcode_result(&gcode_result);
            background_process.set_thumbnail_cb([this](const ThumbnailsParams& params) { return this->generate_thumbnails(params, Camera::EType::Ortho); });
            background_process.set_slicing_began_event(EVT_SLICING_BEGAN);
            background_process.set_slicing_completed_event(EVT_SLICING_COMPLETED);
            background_process.set_finished_event(EVT_PROCESS_COMPLETED);
            background_process.set_export_began_event(EVT_EXPORT_BEGAN);
            // Default printer technology for default config.
            background_process.select_technology(this->printer_technology);
            // Register progress callback from the Print class to the Plater.

            auto statuscb = [this](const Slic3r::PrintBase::SlicingStatus& status) {
                wxQueueEvent(this->q, new Slic3r::SlicingStatusEvent(EVT_SLICING_UPDATE, 0, status));
            };
            fff_print.set_status_callback(statuscb);
            sla_print.set_status_callback(statuscb);
            this->q->Bind(EVT_SLICING_UPDATE, &priv::on_slicing_update, this);

            view3D = new View3D(q, bed, &model, config, &background_process);
            preview = new Preview(q, bed, &model, config, &background_process, &gcode_result, [this]() { schedule_background_process(); });

            assemble_view = new AssembleView(q, bed, &model, config, &background_process);
#ifdef __APPLE__
            // set default view_toolbar icons size equal to GLGizmosManager::Default_Icons_Size
            view_toolbar.set_icons_size(GLGizmosManager::Default_Icons_Size);
#endif // __APPLE__

            panels.push_back(view3D);
            panels.push_back(preview);
            panels.push_back(assemble_view);

            this->background_process_timer.SetOwner(this->q, 0);
            this->q->Bind(wxEVT_TIMER, [this](wxTimerEvent& evt)
                {
                    if (!this->suppressed_backround_processing_update)
                        this->update_restart_background_process(false, false);
                });

            update();

            auto* hsizer = new wxBoxSizer(wxHORIZONTAL);
            panel_sizer = new wxBoxSizer(wxHORIZONTAL);
            panel_sizer->Add(view3D, 1, wxEXPAND | wxALL, 0);
            panel_sizer->Add(preview, 1, wxEXPAND | wxALL, 0);
            panel_sizer->Add(assemble_view, 1, wxEXPAND | wxALL, 0);
            hsizer->Add(panel_sizer, 1, wxEXPAND | wxALL, 0);
#if USE_SIDEBAR_NEW
            hsizer->Add(sidebarnew, 0, wxEXPAND |/* wxLEFT | */wxRIGHT, 0);
            sidebar->Show(false);
#else
            hsizer->Add(sidebar, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
#endif


            q->SetSizer(hsizer);

            menus.init(q);

            // Events:

            if (wxGetApp().is_editor()) {
#if USE_SIDEBAR_NEW
                // Preset change event
                sidebarnew->Bind(wxEVT_COMBOBOX, &priv::on_select_preset_sidebarnew, this);
                //sidebar->Bind(EVT_OBJ_LIST_OBJECT_SELECT, [this](wxEvent&) { priv::selection_changed(); });
                //sidebar->Bind(EVT_SCHEDULE_BACKGROUND_PROCESS, [this](SimpleEvent&) { this->schedule_background_process(); });

                 // add by allen for ankerCfgDlg search
                // jump to found option from SearchDialog
                q->Bind(wxCUSTOMEVT_JUMP_TO_OPTION, [this](wxCommandEvent& evt) { sidebarnew->jump_to_option(evt.GetInt()); });
#else
                // Preset change event
                sidebar->Bind(wxEVT_COMBOBOX, &priv::on_select_preset, this);
                sidebar->Bind(EVT_OBJ_LIST_OBJECT_SELECT, [this](wxEvent&) { priv::selection_changed(); });
                sidebar->Bind(EVT_SCHEDULE_BACKGROUND_PROCESS, [this](SimpleEvent&) { this->schedule_background_process(); });
                // jump to found option from SearchDialog
                q->Bind(wxCUSTOMEVT_JUMP_TO_OPTION, [this](wxCommandEvent& evt) { sidebar->jump_to_option(evt.GetInt()); });
#endif

            }

            wxGLCanvas* view3D_canvas = view3D->get_wxglcanvas();

            if (wxGetApp().is_editor()) {
                // 3DScene events:
                view3D_canvas->Bind(EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS, [this](SimpleEvent&) { this->schedule_background_process(); });
                view3D_canvas->Bind(EVT_GLCANVAS_OBJECT_SELECT, &priv::on_object_select, this);
                view3D_canvas->Bind(EVT_GLCANVAS_RIGHT_CLICK, &priv::on_right_click, this);
                view3D_canvas->Bind(EVT_GLCANVAS_REMOVE_OBJECT, [q](SimpleEvent&) { q->remove_selected(); });
                view3D_canvas->Bind(EVT_GLCANVAS_ARRANGE, [this](SimpleEvent&) { this->q->arrange(); });
                view3D_canvas->Bind(EVT_GLCANVAS_SELECT_ALL, [this](SimpleEvent&) { this->q->select_all(); });
                view3D_canvas->Bind(EVT_GLCANVAS_QUESTION_MARK, [](SimpleEvent&) { wxGetApp().keyboard_shortcuts(); });
                view3D_canvas->Bind(EVT_GLCANVAS_INCREASE_INSTANCES, [this](Event<int>& evt)
                    { if (evt.data == 1) this->q->increase_instances(); else if (this->can_decrease_instances()) this->q->decrease_instances(); });
                view3D_canvas->Bind(EVT_GLCANVAS_INSTANCE_MOVED, [this](SimpleEvent&) { update(); });
                view3D_canvas->Bind(EVT_GLCANVAS_FORCE_UPDATE, [this](SimpleEvent&) { update(); });
                view3D_canvas->Bind(EVT_GLCANVAS_WIPETOWER_MOVED, &priv::on_wipetower_moved, this);
                view3D_canvas->Bind(EVT_GLCANVAS_WIPETOWER_ROTATED, &priv::on_wipetower_rotated, this);
                view3D_canvas->Bind(EVT_GLCANVAS_INSTANCE_ROTATED, [this](SimpleEvent&) { update(); });
                view3D_canvas->Bind(EVT_GLCANVAS_RESET_SKEW, [this](SimpleEvent&) { update(); });
                view3D_canvas->Bind(EVT_GLCANVAS_INSTANCE_SCALED, [this](SimpleEvent&) { update(); });
                view3D_canvas->Bind(EVT_GLCANVAS_ENABLE_ACTION_BUTTONS, [this](Event<bool>& evt) { /*this->sidebar->enable_buttons(evt.data);*/
                                                                                                   sidebarnew->enableSliceBtn(false, evt.data); });
                view3D_canvas->Bind(EVT_GLCANVAS_UPDATE_GEOMETRY, &priv::on_update_geometry, this);
                view3D_canvas->Bind(EVT_GLCANVAS_MOUSE_DRAGGING_STARTED, &priv::on_3dcanvas_mouse_dragging_started, this);
                view3D_canvas->Bind(EVT_GLCANVAS_MOUSE_DRAGGING_FINISHED, &priv::on_3dcanvas_mouse_dragging_finished, this);
                view3D_canvas->Bind(EVT_GLCANVAS_TAB, [this](SimpleEvent&) { select_next_view_3D(); });
                view3D_canvas->Bind(EVT_GLCANVAS_RESETGIZMOS, [this](SimpleEvent&) { reset_all_gizmos(); });
                view3D_canvas->Bind(EVT_GLCANVAS_UNDO, [this](SimpleEvent&) { this->undo(); });
                view3D_canvas->Bind(EVT_GLCANVAS_REDO, [this](SimpleEvent&) { this->redo(); });
                view3D_canvas->Bind(EVT_GLCANVAS_COLLAPSE_SIDEBAR, [this](SimpleEvent&) { this->q->collapse_sidebar(!this->q->is_sidebar_collapsed());  });
                view3D_canvas->Bind(EVT_GLCANVAS_RESET_LAYER_HEIGHT_PROFILE, [this](SimpleEvent&) { this->view3D->get_canvas3d()->reset_layer_height_profile(); });
                view3D_canvas->Bind(EVT_GLCANVAS_ADAPTIVE_LAYER_HEIGHT_PROFILE, [this](Event<float>& evt) { this->view3D->get_canvas3d()->adaptive_layer_height_profile(evt.data); });
                view3D_canvas->Bind(EVT_GLCANVAS_SMOOTH_LAYER_HEIGHT_PROFILE, [this](HeightProfileSmoothEvent& evt) { this->view3D->get_canvas3d()->smooth_layer_height_profile(evt.data); });
                view3D_canvas->Bind(EVT_GLCANVAS_RELOAD_FROM_DISK, [this](SimpleEvent&) { this->reload_all_from_disk(); });

                // 3DScene/Toolbar:
                view3D_canvas->Bind(EVT_GLTOOLBAR_ADD, &priv::on_action_add, this);
                view3D_canvas->Bind(EVT_GLTOOLBAR_DELETE, [q](SimpleEvent&) { q->remove_selected(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_DELETE_ALL, [this](SimpleEvent&) { delete_all_objects_from_model(); });
                //        view3D_canvas->Bind(EVT_GLTOOLBAR_DELETE_ALL, [q](SimpleEvent&) { q->reset_with_confirm(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_ARRANGE, [this](SimpleEvent&) { this->q->arrange(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_AUTO_BED, [this](SimpleEvent&) { this->q->auto_bed(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_COPY, [q](SimpleEvent&) { q->copy_selection_to_clipboard(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_PASTE, [q](SimpleEvent&) { q->paste_from_clipboard(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_MORE, [q](SimpleEvent&) { q->increase_instances(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_FEWER, [q](SimpleEvent&) { q->decrease_instances(); });
                view3D_canvas->Bind(EVT_GLTOOLBAR_SPLIT_OBJECTS, &priv::on_action_split_objects, this);
                view3D_canvas->Bind(EVT_GLTOOLBAR_SPLIT_VOLUMES, &priv::on_action_split_volumes, this);
                view3D_canvas->Bind(EVT_GLTOOLBAR_LAYERSEDITING, &priv::on_action_layersediting, this);
                view3D_canvas->Bind(EVT_GLVIEWTOOLBAR_ASSEMBLE, [q](SimpleEvent&) { q->select_view_3D(VIEW_MODE_ASSEMBLE); });
                view3D_canvas->Bind(EVT_GLCANVAS_INITIALIZED, [this](SimpleEvent&) {
                    bool visible = wxGetApp().is_editor() && current_view_mode == VIEW_MODE_3D && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE;
                    //objectbar->getViewer()->Show(visible);
                 });
            }
            view3D_canvas->Bind(EVT_GLCANVAS_UPDATE_BED_SHAPE, [q](SimpleEvent&) { q->set_bed_shape(); });

            // Preview events:
            preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_QUESTION_MARK, [](SimpleEvent&) { wxGetApp().keyboard_shortcuts(); });
            preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_UPDATE_BED_SHAPE, [q](SimpleEvent&) { q->set_bed_shape(); });
            if (wxGetApp().is_editor()) {
                preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_TAB, [this](SimpleEvent&) { select_next_view_3D(); });
                preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_COLLAPSE_SIDEBAR, [this](SimpleEvent&) { this->q->collapse_sidebar(!this->q->is_sidebar_collapsed());  });
            }

            preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_JUMP_TO, [this](wxKeyEvent& evt) { preview->jump_layers_slider(evt); });
            preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_MOVE_SLIDERS, [this](wxKeyEvent& evt) {
#if !USE_ANKER_SLIDER
                preview->move_layers_slider(evt);
                preview->move_moves_slider(evt);
#else
                preview->move_sliders(evt);
#endif
                });

            preview->get_wxglcanvas()->Bind(EVT_AGCODE_PRINT_EVENT, [q](SimpleEvent& evt) { q->a_key_print_clicked(); });
            preview->get_wxglcanvas()->Bind(EVT_GLCANVAS_EDIT_COLOR_CHANGE, [this](wxKeyEvent& evt) { preview->edit_layers_slider(evt); });
            if (wxGetApp().is_gcode_viewer())
                preview->Bind(EVT_GLCANVAS_RELOAD_FROM_DISK, [this](SimpleEvent&) { this->q->reload_gcode_from_disk(); });

            //assemble
            wxGLCanvas* assemble_canvas = assemble_view->get_wxglcanvas();
            if (wxGetApp().is_editor()) {
                //assemble_canvas->Bind(EVT_GLTOOLBAR_FILLCOLOR, [q](IntEvent& evt) { q->fill_color(evt.get_data()); });
                assemble_canvas->Bind(EVT_GLCANVAS_OBJECT_SELECT, &priv::on_object_select, this);
                assemble_canvas->Bind(EVT_GLVIEWTOOLBAR_3D, [q](SimpleEvent&) { q->select_view_3D(VIEW_MODE_3D); });
                assemble_canvas->Bind(EVT_GLCANVAS_RIGHT_CLICK, &priv::on_right_click, this);
                assemble_canvas->Bind(EVT_GLCANVAS_UNDO, [this](SimpleEvent&) { this->undo(); });
                assemble_canvas->Bind(EVT_GLCANVAS_REDO, [this](SimpleEvent&) { this->redo(); });
                assemble_canvas->Bind(EVT_EXPLOSION_VALUE_CHANGE, [this](SimpleEvent&) { this->assemble_view->reload_scene(false, 0); });
            }

            if (wxGetApp().is_editor()) {
                q->Bind(EVT_SLICING_BEGAN, [q](wxCommandEvent&) { q->sidebarnew().updatePreviewBtn(false, SELECT_VIEW_MODE_PREVIEW); });
                q->Bind(EVT_SLICING_COMPLETED, &priv::on_slicing_completed, this);
                q->Bind(EVT_PROCESS_COMPLETED, &priv::on_process_completed, this);
                q->Bind(EVT_EXPORT_BEGAN, &priv::on_export_began, this);
                q->Bind(EVT_GLVIEWTOOLBAR_3D, [=](SimpleEvent&) {
                    if (preview->is_GcodeImported()) { //If the current project is made from the gcode file, popup a dialog to hint the customer. 
                        MessageDialog dialog(nullptr, _L("The current file will be closed before creating a new model.\nDo you want proceed with the action?"), \
                            _L("Warning"), wxICON_WARNING | wxYES_NO | wxCENTER | wxNO_DEFAULT);
                        if (dialog.ShowModal() == wxID_YES) {
                            //Importing the gcode file results in the sidebarnew hided ,so we must manually show. 
                            //Refer to PlaterDropTarget::OnDropFiles=>wxGetApp().plater()->sidebarnew().Hide();
                            wxGetApp().plater()->sidebarnew().Show();
                            wxGetApp().plater()->SetShowACodeOrGcodePrint(false);
                            q->select_view_3D(VIEW_MODE_3D);
                            q->SetDragGcodeFlag(false);
                            q->ShowHintDialogs(true); 
                            wxGetApp().plater()->Layout();
                        }
                        else {
                            q->select_view_3D(VIEW_MODE_PREVIEW);
                        }
                    }
                    else { //Don't exist gcode file
                        q->select_view_3D(VIEW_MODE_3D);
                    }
                    });
                q->Bind(EVT_GLVIEWTOOLBAR_PREVIEW, [q](SimpleEvent&) { q->select_view_3D(VIEW_MODE_PREVIEW); });
                if (m_downLoad_controller) {
                    m_downLoad_controller->Bind(EVT_DOWNLOAD_FILE_COMPLETE, &priv::on_download_complete, this);
                }
            }

            // Drop target:
            main_frame->SetDropTarget(new PlaterDropTarget(*main_frame, *q));   // if my understanding is right, wxWindow takes the owenership
            q->Layout();

            set_current_panel(wxGetApp().is_editor() ? static_cast<wxPanel*>(view3D) : static_cast<wxPanel*>(preview));
            if (wxGetApp().is_gcode_viewer())
                preview->hide_layers_slider();

            // updates camera type from .ini file
            camera.enable_update_config_on_type_change(true);
            camera.set_type(wxGetApp().app_config->get("use_perspective_camera"));

            // Load the 3DConnexion device database.
            mouse3d_controller.load_config(*wxGetApp().app_config);
            // Start the background thread to detect and connect to a HID device (Windows and Linux).
            // Connect to a 3DConnextion driver (OSX).
            mouse3d_controller.init();
#ifdef _WIN32
            // Register an USB HID (Human Interface Device) attach event. evt contains Win32 path to the USB device containing VID, PID and other info.
            // This event wakes up the Mouse3DController's background thread to enumerate HID devices, if the VID of the callback event
            // is one of the 3D Mouse vendors (3DConnexion or Logitech).
            this->q->Bind(EVT_HID_DEVICE_ATTACHED, [this](HIDDeviceAttachedEvent& evt) {
                mouse3d_controller.device_attached(evt.data);
                });
            this->q->Bind(EVT_HID_DEVICE_DETACHED, [this](HIDDeviceAttachedEvent& evt) {
                mouse3d_controller.device_detached(evt.data);
                });
#endif /* _WIN32 */

            //notification_manager = new NotificationManager(this->q);

            if (wxGetApp().is_editor()) {
                this->q->Bind(EVT_EJECT_DRIVE_NOTIFICAION_CLICKED, [this](EjectDriveNotificationClickedEvent&) { this->q->eject_drive(); });
                this->q->Bind(EVT_EXPORT_GCODE_NOTIFICAION_CLICKED, [this](ExportGcodeNotificationClickedEvent&) { this->q->export_gcode(true); });
                this->q->Bind(EVT_PRESET_UPDATE_AVAILABLE_CLICKED, [](PresetUpdateAvailableClickedEvent&) {  wxGetApp().get_preset_updater()->on_update_notification_confirm(); });
                this->q->Bind(EVT_REMOVABLE_DRIVE_EJECTED, [this, q](RemovableDriveEjectEvent& evt) {
                    if (evt.data.second) {
                        q->show_action_buttons();
                        notification_manager->close_notification_of_type(NotificationType::ExportFinished);
                        notification_manager->push_notification(NotificationType::CustomNotification,
                            NotificationManager::NotificationLevel::RegularNotificationLevel,
                            format(_L("common_slicepopup_deviceremoved1"), evt.data.first.name, evt.data.first.path)
                        );
                    }
                    else {
                        notification_manager->close_notification_of_type(NotificationType::ExportFinished);
                        notification_manager->push_notification(NotificationType::CustomNotification,
                            NotificationManager::NotificationLevel::ErrorNotificationLevel,
                            format(_L("common_slicepopup_deviceremoved2"), evt.data.first.name, evt.data.first.path)
                        );
                    }
                    });
                this->q->Bind(EVT_REMOVABLE_DRIVES_CHANGED, [this, q](RemovableDrivesChangedEvent&) {
                    q->show_action_buttons();
                    // Close notification ExportingFinished but only if last export was to removable
                    notification_manager->device_ejected();
                    });
                // Start the background thread and register this window as a target for update events.
                wxGetApp().removable_drive_manager()->init(this->q);
#ifdef _WIN32
                // Trigger enumeration of removable media on Win32 notification.
                this->q->Bind(EVT_VOLUME_ATTACHED, [this](VolumeAttachedEvent& evt) { wxGetApp().removable_drive_manager()->volumes_changed(); });
                this->q->Bind(EVT_VOLUME_DETACHED, [this](VolumeDetachedEvent& evt) { wxGetApp().removable_drive_manager()->volumes_changed(); });
#endif /* _WIN32 */
            }

            // Initialize the Undo / Redo stack with a first snapshot.
            this->take_snapshot(_L("New Project"), UndoRedo::SnapshotType::ProjectSeparator);
            // Reset the "dirty project" flag.
            m_undo_redo_stack_main.mark_current_as_saved();
            dirty_state.update_from_undo_redo_stack(false);

            this->q->Bind(EVT_LOAD_MODEL_OTHER_INSTANCE, [this](LoadFromOtherInstanceEvent& evt) {
                BOOST_LOG_TRIVIAL(trace) << "Received load from other instance event.";
                wxArrayString input_files;
                for (size_t i = 0; i < evt.data.size(); ++i) {
                    input_files.push_back(from_u8(evt.data[i].string()));
                }
                wxGetApp().mainframe->Raise();
                this->q->load_files(input_files);
                });

            this->q->Bind(EVT_START_DOWNLOAD_OTHER_INSTANCE, [&](StartDownloadOtherInstanceEvent& evt) {
                BOOST_LOG_TRIVIAL(trace) << "Received url from other instance event.";
                wxGetApp().mainframe->Raise();
                for (size_t i = 0; i < evt.data.size(); ++i) {
                    //wxGetApp().start_download(evt.data[i]);
                    //ANKER_LOG_INFO << "other instance download: " << evt.data[i];
                    //on_action_request_model_download(evt.data[i]);
                }

                });

            this->q->Bind(EVT_INSTANCE_GO_TO_FRONT, [this](InstanceGoToFrontEvent&) {
                bring_instance_forward();
                });
            wxGetApp().other_instance_message_handler()->init(this->q);

            // collapse sidebar according to saved value
        /*    if (wxGetApp().is_editor()) {
                bool is_collapsed = get_config_bool("collapsed_sidebar");
                sidebar->collapse(is_collapsed);
            }*/

            //CreatePreViewRightSideBar();

            create_AI_file = false;
#if ENABLE_AI
            // init create_AI_file from config file
            if (wxGetApp().app_config->has("create_AI_file")) {
                bool val = wxGetApp().app_config->get_bool("create_AI_file");
                set_create_AI_file(val);
            }
#endif
        }

        Plater::priv::~priv()
        {
            if (config != nullptr)
                delete config;
            // Saves the database of visited (already shown) hints into hints.ini.
            notification_manager->deactivate_loaded_hints();
            clear_exported_file_cache();
        }

        DynamicPrintConfig& Plater::get_global_config()
        {
            return p->m_global_config;
        }

        void Plater::priv::update(unsigned int flags)
        {
            // the following line, when enabled, causes flickering on NVIDIA graphics cards
        //    wxWindowUpdateLocker freeze_guard(q);
            if (get_config_bool("autocenter"))
                model.center_instances_around_point(this->bed.build_volume().bed_center());

            unsigned int update_status = 0;
            const bool force_background_processing_restart = this->printer_technology == ptSLA || (flags & (unsigned int)UpdateParams::FORCE_BACKGROUND_PROCESSING_UPDATE);
            if (force_background_processing_restart)
                // Update the SLAPrint from the current Model, so that the reload_scene()
                // pulls the correct data.
                update_status = this->update_background_process(false, flags & (unsigned int)UpdateParams::POSTPONE_VALIDATION_ERROR_MESSAGE);
            this->view3D->reload_scene(false, flags & (unsigned int)UpdateParams::FORCE_FULL_SCREEN_REFRESH);
            this->preview->reload_print();
            this->assemble_view->reload_scene(false, flags);

            if (force_background_processing_restart)
                this->restart_background_process(update_status);
            else
                this->schedule_background_process();

            //if (get_config_bool("autocenter") && this->sidebar->obj_manipul()->IsShown())
            //    this->sidebar->obj_manipul()->UpdateAndShow(true);
            if (aobjectmanipulator->IsShown())
            {
                aobjectmanipulator->set_dirty();
                aobjectmanipulator->update_if_dirty();
            }
        }

        wxBoxSizer* Plater::priv::CreatePreViewRightSideBar()
        {
            if (previewRightSidePanelSizer == nullptr) {
                previewRightSidePanelSizer = new wxBoxSizer(wxHORIZONTAL);
                previewRightSidePanel = new AnkerGcodePreviewSideBar(sidebarnew);
                previewRightSidePanel->SetSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
                // previewRightSidePanel = new AnkerGcodePreviewSideBar(preview);
                previewRightSidePanel->SetBackgroundColour(wxColour("#18191b"));
                previewRightSidePanelSizer->Add(previewRightSidePanel, 1, wxEXPAND | wxALL, 0);
            }

            return previewRightSidePanelSizer;
        }

        void Plater::priv::updatePreViewRightSideBar(bool gcode_valid, RightSidePanelUpdateReason reason)
        {
            if (previewRightSidePanel)
            {
                previewRightSidePanel->UpdateGcodePreviewSideBar(gcode_valid, reason);
                previewRightSidePanel->Refresh();
            }
        }


        void Plater::priv::UpdateCurrentViewType(GCodeViewer::EViewType type)
        {
            if (previewRightSidePanel)
            {
                previewRightSidePanel->UpdateCurrentViewType(type);
                previewRightSidePanel->Refresh();
            }
        }

        void Plater::priv::select_view(const std::string& direction)
        {
            if (current_panel == view3D)
                view3D->select_view(direction);
            else if (current_panel == preview)
                preview->select_view(direction);
            else if (current_panel == assemble_view)
                assemble_view->select_view(direction);
        }

        void Plater::priv::apply_free_camera_correction(bool apply/* = true*/)
        {
            camera.set_type(wxGetApp().app_config->get("use_perspective_camera"));
            if (apply && !wxGetApp().app_config->get_bool("use_free_camera"))
                camera.recover_from_free_camera();
        }

        void Plater::priv::select_view_3D(ViewMode mode)
        {
            bool showGcodeLayerToolbar = false;
            current_view_mode = mode;
            if (q != nullptr && !q->isDragGcode()) {
                q->ShowHintDialogs(true);
            }
            if (mode == VIEW_MODE_3D) {
                set_view_drop_file(false);
                set_current_panel(view3D);

                if (wxGetApp().is_editor())
                {
                    //objectbar->getObjectBarView()->Show(wxGetApp().plater()->IsShown() && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE);
                }

                if (sidebarnew) {
                    //sidebarnew->setMainSizer(nullptr, false);
                    sidebarnew->ChangeViewMode(VIEW_MODE_3D);
                    if (wxGetApp().plater()->model().objects.empty())
                        sidebarnew->enableSliceBtn(false, false);
                    else
                        sidebarnew->enableSliceBtn(false, true);
                }

            }
            else if (mode == VIEW_MODE_PREVIEW) {
                set_current_panel(preview);

                if (wxGetApp().is_editor())
                {
                    //objectbar->getObjectBarView()->Show(false);
                    floatinglist->Show(false);
                }

                GLGizmosManager& gizmos = view3D->get_canvas3d()->get_gizmos_manager();
                if (gizmos.is_running()) {
                    gizmos.reset_all_states();
                    gizmos.update_data();
                }
                
                if (sidebarnew) {
                    sidebarnew->ChangeViewMode(VIEW_MODE_PREVIEW);
                    sidebarnew->updatePreviewBtn(wxGetApp().plater()->is_preview_loaded(), SELECT_VIEW_MODE_PREVIEW);
                /*
                    // fix the problem of flickering when switching gcode preview interface.
                    //sidebarnew->Show(false);
                    if (sidebarnew->GetSizer())
                    {
                        sidebarnew->GetSizer()->Show(false);
                    }
                    sidebarnew->setMainSizer(previewRightSidePanelSizer);
                    previewRightSidePanel->UpdateGcodePreviewSideBar(wxGetApp().plater()->is_preview_loaded(), SELECT_VIEW_MODE_PREVIEW);
                    if (sidebarnew->GetSizer())
                    {
                        sidebarnew->GetSizer()->Show(true);
                    }
                  // sidebarnew->Show(true);
                */
                }

                if (is_preview_loaded() && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE)
                {
                    showGcodeLayerToolbar = true;
                }
            }
            else if (mode == VIEW_MODE_ASSEMBLE) {
                set_current_panel(assemble_view);
            }
            if (q->IsShown()) {
                preview->showGcodeLayerToolbar(showGcodeLayerToolbar);
            }

            // update selection
            wxGetApp().obj_list()->update_selections();
            selection_changed();

            apply_free_camera_correction(false);
        }

        void Plater::priv::select_next_view_3D()
        {
            if (current_panel == view3D)
                set_current_panel(preview);
            else if (current_panel == preview)
                set_current_panel(view3D);
            else if (current_panel == assemble_view)
                set_current_panel(view3D);
        }

        void Plater::priv::collapse_sidebar(bool collapse)
        {
            //sidebar->collapse(collapse);

            // Now update the tooltip in the toolbar.
            std::string new_tooltip = collapse
                ? _u8L("Expand sidebar")
                : _u8L("Collapse sidebar");
            new_tooltip += " [Shift+Tab]";
            int id = collapse_toolbar.get_item_id("collapse_sidebar");
            collapse_toolbar.set_tooltip(id, new_tooltip);

            notification_manager->set_sidebar_collapsed(collapse);
        }


        void Plater::priv::reset_all_gizmos()
        {
            view3D->get_canvas3d()->reset_all_gizmos();
        }

        // Called after the Preferences dialog is closed and the program settings are saved.
        // Update the UI based on the current preferences.
        void Plater::priv::update_ui_from_settings()
        {
            apply_free_camera_correction();

            view3D->get_canvas3d()->update_ui_from_settings();
            preview->get_canvas3d()->update_ui_from_settings();

            sidebarnew->update_ui_from_settings();
            //objectbar->update_ui_from_settings();
            aobjectmanipulator->update_ui_from_settings();
        }

        // Called after the print technology was changed.
        // Update the tooltips for "Switch to Settings" button in maintoolbar
        void Plater::priv::update_main_toolbar_tooltips()
        {
            view3D->get_canvas3d()->update_tooltip_for_settings_item_in_main_toolbar();
        }

        //std::shared_ptr<ProgressStatusBar> Plater::priv::statusbar()
        //{
        //      return nullptr;
        //    return main_frame->m_statusbar;
        //}

        bool Plater::priv::get_config_bool(const std::string& key) const
        {
            return wxGetApp().app_config->get_bool(key);
        }

        void Plater::priv::HintForMachine()
        {
            auto appConfig = Slic3r::GUI::wxGetApp().app_config;
            if (appConfig && appConfig->get_bool("Print Check", "machine_type_nohint")) {
                ANKER_LOG_INFO << "machine type nohint for user set.";
                return;
            }

            if (m_machineTypeMatchHint == nullptr) {
                m_machineTypeMatchHint = new AnkerHint(_L("common_popup_guide_printerchoose"), q);
                m_machineTypeMatchHint->SetCallBack([=](bool isChecked) {             
                    q->DelSpecifyHintDialog(MatchHintType);
                    q->SetExistMatchHint(false);

                    if (!q->isDragGcode()) {
                        // Find the other ankerhint and move it up  
                        q->ShowHintDialogs(true);  q->Layout(); /*q->Refresh(); */
                    }

                    m_machineTypeMatchHint = nullptr;
                    if (appConfig) {    
                        appConfig->set("Print Check", "machine_type_nohint", isChecked ? "1" : "0");
                    }
                });
                wxSize uiSize(268, 189);
                m_machineTypeMatchHint->InitUI(uiSize.GetWidth(), uiSize.GetHeight());
                q->updateMatchHint();
            }
        }

        // After loading of the presets from project, check if they are visible.
        // Set them to visible if they are not.
        void Plater::check_selected_presets_visibility(PrinterTechnology loaded_printer_technology)
        {
            auto update_selected_preset_visibility = [](PresetCollection& presets, std::vector<std::string>& names) {
                if (!presets.get_selected_preset().is_visible) {
                    assert(presets.get_selected_preset().name == presets.get_edited_preset().name);
                    presets.get_selected_preset().is_visible = true;
                    presets.get_edited_preset().is_visible = true;
                    names.emplace_back(presets.get_selected_preset().name);
                }
            };

            std::vector<std::string> names;
            PresetBundle* preset_bundle = wxGetApp().preset_bundle;
            if (loaded_printer_technology == ptFFF) {
                update_selected_preset_visibility(preset_bundle->prints, names);
                for (const std::string& filament : preset_bundle->filament_presets) {
                    Preset* preset = preset_bundle->filaments.find_preset(filament);
                    if (preset && !preset->is_visible) {
                        preset->is_visible = true;
                        names.emplace_back(preset->name);
                        if (preset->name == preset_bundle->filaments.get_edited_preset().name)
                            preset_bundle->filaments.get_selected_preset().is_visible = true;
                    }
                }
            }
            else {
                update_selected_preset_visibility(preset_bundle->sla_prints, names);
                update_selected_preset_visibility(preset_bundle->sla_materials, names);
            }
            update_selected_preset_visibility(preset_bundle->printers, names);

            preset_bundle->update_compatible(PresetSelectCompatibleType::Never);

            // show notification about temporarily installed presets

        //comment by Samuel 20231106, Discarded  unused notification text
        /*    if (!names.empty()) {
                std::string notif_text = into_u8(_L_PLURAL("The preset below was temporarily installed on the active instance of eufyMake Studio",
                    "The presets below were temporarily installed on the active instance of eufyMake Studio", names.size())) + ":";
                for (std::string& name : names)
                    notif_text += "\n - " + name;
                get_notification_manager()->push_notification(NotificationType::CustomNotification,
                    NotificationManager::NotificationLevel::PrintInfoNotificationLevel, notif_text);
            }*/
        }

        std::vector<size_t> Plater::priv::load_files(const std::vector<fs::path>& input_files, bool load_model, bool load_config, bool imperial_units/* = false*/)
        {
            if (input_files.empty()) { return std::vector<size_t>(); }

            auto* nozzle_dmrs = config->opt<ConfigOptionFloats>("nozzle_diameter");

            if (model.objects.empty()) {
                HintForMachine();
            }

            PlaterAfterLoadAutoArrange plater_after_load_auto_arrange;

            bool one_by_one = input_files.size() == 1 || printer_technology == ptSLA || nozzle_dmrs->values.size() <= 1;
            if (!one_by_one) {
                for (const auto& path : input_files) {
                    if (std::regex_match(path.string(), pattern_bundle)) {
                        one_by_one = true;
                        break;
                    }
                }
            }

            const auto loading = _L("Loading") + dots;

            // The situation with wxProgressDialog is quite interesting here.
            // On Linux (only), there are issues when FDM/SLA is switched during project file loading (disabling of controls,
            // see a comment below). This can be bypassed by creating the wxProgressDialog on heap and destroying it
            // when loading a project file. However, creating the dialog on heap causes issues on macOS, where it does not
            // appear at all. Therefore, we create the dialog on stack on Win and macOS, and on heap on Linux, which
            // is the only system that needed the workarounds in the first place.
#ifdef __linux__
            auto progress_dlg = new wxProgressDialog(loading, "", 100, find_toplevel_parent(q), wxPD_APP_MODAL | wxPD_AUTO_HIDE);
            Slic3r::ScopeGuard([&progress_dlg]() { if (progress_dlg) progress_dlg->Destroy(); progress_dlg = nullptr; });
#else
            wxProgressDialog progress_dlg_stack(loading, "", 100, find_toplevel_parent(q), wxPD_APP_MODAL | wxPD_AUTO_HIDE);
            wxProgressDialog* progress_dlg = &progress_dlg_stack;
#endif


            wxBusyCursor busy;

            auto* new_model = (!load_model || one_by_one) ? nullptr : new Slic3r::Model();
            std::vector<size_t> obj_idxs;

            int answer_convert_from_meters = wxOK_DEFAULT;
            int answer_convert_from_imperial_units = wxOK_DEFAULT;

            bool in_temp = false;
            const fs::path temp_path = wxStandardPaths::Get().GetTempDir().utf8_str().data();

            size_t input_files_size = input_files.size();
            for (size_t i = 0; i < input_files_size; ++i) {
#ifdef _WIN32
                auto path = input_files[i];
                // On Windows, we swap slashes to back slashes, see GH #6803 as read_from_file() does not understand slashes on Windows thus it assignes full path to names of loaded objects.
                path.make_preferred();
#else // _WIN32
                // Don't make a copy on Posix. Slash is a path separator, back slashes are not accepted as a substitute.
                const auto& path = input_files[i];
#endif // _WIN32
                in_temp = (path.parent_path() == temp_path);
                const auto filename = path.filename();
                if (progress_dlg) {
                    progress_dlg->Update(static_cast<int>(100.0f * static_cast<float>(i) / static_cast<float>(input_files.size())), _L("Loading file") + ": " + from_path(filename));
                    progress_dlg->Fit();
                }

                const bool type_3mf = std::regex_match(path.string(), pattern_3mf);
                const bool type_zip_amf = !type_3mf && std::regex_match(path.string(), pattern_zip_amf);
                const bool type_any_amf = !type_3mf && std::regex_match(path.string(), pattern_any_amf);
                const bool type_anker = std::regex_match(path.string(), pattern_anker);

                Slic3r::Model model;
                bool is_project_file = type_anker;
                try {
                    if (type_3mf || type_zip_amf) {
#ifdef __linux__
                        // On Linux Constructor of the ProgressDialog calls DisableOtherWindows() function which causes a disabling of all children of the find_toplevel_parent(q)
                        // And a destructor of the ProgressDialog calls ReenableOtherWindows() function which revert previously disabled children.
                        // But if printer technology will be changes during project loading, 
                        // then related SLA Print and Materials Settings or FFF Print and Filaments Settings will be unparent from the wxNoteBook
                        // and that is why they will never be enabled after destruction of the ProgressDialog.
                        // So, distroy progress_gialog if we are loading project file
                        if (input_files_size == 1 && progress_dlg) {
                            progress_dlg->Destroy();
                            progress_dlg = nullptr;
                        }
#endif
                        DynamicPrintConfig config;
                        PrinterTechnology loaded_printer_technology{ ptFFF };
                        {
                            DynamicPrintConfig config_loaded;
                            ConfigSubstitutionContext config_substitutions{ ForwardCompatibilitySubstitutionRule::Enable };
                            model = Slic3r::Model::read_from_archive(path.string(), &config_loaded, &config_substitutions, only_if(load_config, Model::LoadAttribute::CheckVersion));
                            if (load_config && !config_loaded.empty()) {
                                // Based on the printer technology field found in the loaded config, select the base for the config,
                                loaded_printer_technology = Preset::printer_technology(config_loaded);

                                // We can't to load SLA project if there is at least one multi-part object on the bed
                                if (loaded_printer_technology == ptSLA) {
                                    const ModelObjectPtrs& objects = q->model().objects;
                                    for (auto object : objects)
                                        if (object->volumes.size() > 1) {
                                            Slic3r::GUI::show_info(nullptr,
                                                _L("You cannot load SLA project with a multi-part object on the bed") + "\n\n" +
                                                _L("Please check your object list before preset changing."),
                                                _L("Attention!"));
                                            return obj_idxs;
                                        }
                                }

                                config.apply(loaded_printer_technology == ptFFF ?
                                    static_cast<const ConfigBase&>(FullPrintConfig::defaults()) :
                                    static_cast<const ConfigBase&>(SLAFullPrintConfig::defaults()));
                                // and place the loaded config over the base.
                                config += std::move(config_loaded);
                            }
                            if (!config_substitutions.empty())
                                show_substitutions_info(config_substitutions.substitutions, filename.string());

                            this->model.custom_gcode_per_print_z = model.custom_gcode_per_print_z;
                        }

                        if (load_config) {
                            if (!config.empty()) {
                                Preset::normalize(config);
                                PresetBundle* preset_bundle = wxGetApp().preset_bundle;
                                preset_bundle->load_config_model(filename.string(), std::move(config));
                                q->check_selected_presets_visibility(loaded_printer_technology);

                                if (loaded_printer_technology == ptFFF)
                                    CustomGCode::update_custom_gcode_per_print_z_from_config(model.custom_gcode_per_print_z, &preset_bundle->project_config);

                                // For exporting from the amf/3mf we shouldn't check printer_presets for the containing information about "Print Host upload"
                                wxGetApp().load_current_presets(false);
                                // Update filament colors for the MM-printer profile in the full config 
                                // to avoid black (default) colors for Extruders in the ObjectList, 
                                // when for extruder colors are used filament colors
                                q->update_filament_colors_in_full_config();
                                is_project_file = true;
                            }
                            if (!in_temp)
                                wxGetApp().app_config->update_config_dir(path.parent_path().string());
                        }
                    }
                    else {
                        model = Slic3r::Model::read_from_file(path.string(), nullptr, nullptr, only_if(load_config, Model::LoadAttribute::CheckVersion));
                        for (auto obj : model.objects)
                            if (obj->name.empty())
                                obj->name = fs::path(obj->input_file).filename().string();
                    }
                }
                catch (const ConfigurationError& e) {
                    std::string message = GUI::format(_L("Failed loading file \"%1%\" due to an invalid configuration."), filename.string()) + "\n\n" + e.what();
                    GUI::show_error(q, message);
                    continue;
                }
                catch (const std::exception& e) {
                    GUI::show_error(q, e.what());
                    continue;
                }

                if (load_model) {
                    // The model should now be initialized

                    auto convert_from_imperial_units = [](Model& model, bool only_small_volumes) {
                        model.convert_from_imperial_units(only_small_volumes);
                        //                wxGetApp().app_config->set("use_inches", "1");
                        //                wxGetApp().sidebar().update_ui_from_settings();
                    };

                    if (!is_project_file) {
                        if (int deleted_objects = model.removed_objects_with_zero_volume(); deleted_objects > 0) {
                            MessageDialog(q, format_wxstr(_L_PLURAL(
                                "Object size from file %s appears to be zero.\n"
                                "This object has been removed from the model",
                                "Objects size from file %s appears to be zero.\n"
                                "These objects have been removed from the model", deleted_objects), from_path(filename)) + "\n",
                                _L("The size of the object is zero"), wxICON_INFORMATION | wxOK).ShowModal();
                        }
                        if (imperial_units)
                            // Convert even if the object is big.
                            convert_from_imperial_units(model, false);
                        else if (model.looks_like_saved_in_meters()) {
                            auto convert_model_if = [](Model& model, bool condition) {
                                if (condition)
                                    //FIXME up-scale only the small parts?
                                    model.convert_from_meters(true);
                            };
                            if (answer_convert_from_meters == wxOK_DEFAULT) {
                                RichMessageDialog dlg(q, format_wxstr(_L_PLURAL(
                                    "The dimensions of the object from file %s seem to be defined in meters.\n"
                                    "The internal unit of eufyMake Studio is a millimeter. Do you want to recalculate the dimensions of the object?",
                                    "The dimensions of some objects from file %s seem to be defined in meters.\n"
                                    "The internal unit of eufyMake Studio is a millimeter. Do you want to recalculate the dimensions of these objects?", model.objects.size()), from_path(filename)) + "\n",
                                    _L("The object is too small"), wxICON_QUESTION | wxYES_NO);
                                dlg.ShowCheckBox(_L("Apply to all the remaining small objects being loaded."));
                                int answer = dlg.ShowModal();
                                if (dlg.IsCheckBoxChecked())
                                    answer_convert_from_meters = answer;
                                else
                                    convert_model_if(model, answer == wxID_YES);
                            }
                            convert_model_if(model, answer_convert_from_meters == wxID_YES);
                        }
                        else if (model.looks_like_imperial_units()) {
                            auto convert_model_if = [convert_from_imperial_units](Model& model, bool condition) {
                                if (condition)
                                    //FIXME up-scale only the small parts?
                                    convert_from_imperial_units(model, true);
                            };
                            if (answer_convert_from_imperial_units == wxOK_DEFAULT) {
                                RichMessageDialog dlg(q, format_wxstr(_L_PLURAL(
                                    "The dimensions of the object from file %s seem to be defined in inches.\n"
                                    "The internal unit of eufyMake Studio is a millimeter. Do you want to recalculate the dimensions of the object?",
                                    "The dimensions of some objects from file %s seem to be defined in inches.\n"
                                    "The internal unit of eufyMake Studio is a millimeter. Do you want to recalculate the dimensions of these objects?", model.objects.size()), from_path(filename)) + "\n",
                                    _L("The object is too small"), wxICON_QUESTION | wxYES_NO);
                                dlg.ShowCheckBox(_L("Apply to all the remaining small objects being loaded."));
                                int answer = dlg.ShowModal();
                                if (dlg.IsCheckBoxChecked())
                                    answer_convert_from_imperial_units = answer;
                                else
                                    convert_model_if(model, answer == wxID_YES);
                            }
                            convert_model_if(model, answer_convert_from_imperial_units == wxID_YES);
                        }

                        if (model.looks_like_multipart_object()) {
                            MessageDialog msg_dlg(q, _L(
                                "This file contains several objects positioned at multiple heights.\n"
                                "Instead of considering them as multiple objects, should \n"
                                "the file be loaded as a single object having multiple parts?") + "\n",
                                _L("Multi-part object detected"), wxICON_WARNING | wxYES | wxNO);
                            if (msg_dlg.ShowModal() == wxID_YES) {
                                model.convert_multipart_object(nozzle_dmrs->values.size());
                            }
                        }
                    }
                    if ((wxGetApp().get_mode() == comSimple) && (type_3mf || type_any_amf) && model_has_advanced_features(model)) {
                        MessageDialog msg_dlg(q, _L("This file cannot be loaded in a simple mode. Do you want to switch to an advanced mode?") + "\n",
                            _L("Detected advanced data"), wxICON_WARNING | wxOK | wxCANCEL);
                        if (msg_dlg.ShowModal() == wxID_OK) {
                            if (wxGetApp().save_mode(comAdvanced))
                                view3D->set_as_dirty();
                        }
                        else
                            return obj_idxs;
                    }

                    for (ModelObject* model_object : model.objects) {
                        if (!type_3mf && !type_zip_amf) {
                            model_object->center_around_origin(false);
                            if (type_any_amf && model_object->instances.empty()) {
                                ModelInstance* instance = model_object->add_instance();
                                instance->set_offset(-model_object->origin_translation);
                            }
                        }
                        if (!model_object->instances.empty())
                            model_object->ensure_on_bed(is_project_file);
                    }

                    if (one_by_one) {
                        if ((type_3mf && !is_project_file) || (type_any_amf && !type_zip_amf))
                            model.center_instances_around_point(this->bed.build_volume().bed_center());
                        auto loaded_idxs = load_model_objects(model.objects, is_project_file);
                        obj_idxs.insert(obj_idxs.end(), loaded_idxs.begin(), loaded_idxs.end());
                    }
                    else {
                        // This must be an .stl or .obj file, which may contain a maximum of one volume.
                        for (const ModelObject* model_object : model.objects) {
                            new_model->add_object(*model_object);
                        }
                    }

                    if (is_project_file)
                        plater_after_load_auto_arrange.disable();
                }
            }

            if (new_model != nullptr && new_model->objects.size() > 1) {
                //wxMessageDialog msg_dlg(q, _L(
                MessageDialog msg_dlg(q, _L(
                    "Multiple objects were loaded for a multi-material printer.\n"
                    "Instead of considering them as multiple objects, should I consider\n"
                    "these files to represent a single object having multiple parts?") + "\n",
                    _L("Multi-part object detected"), wxICON_WARNING | wxYES | wxNO);
                if (msg_dlg.ShowModal() == wxID_YES) {
                    new_model->convert_multipart_object(nozzle_dmrs->values.size());
                }

                auto loaded_idxs = load_model_objects(new_model->objects);
                obj_idxs.insert(obj_idxs.end(), loaded_idxs.begin(), loaded_idxs.end());
            }

            if (load_model && !in_temp) {
                wxGetApp().app_config->update_skein_dir(input_files[input_files.size() - 1].parent_path().make_preferred().string());
                // XXX: Plater.pm had @loaded_files, but didn't seem to fill them with the filenames...
                // statusbar()->set_status_text(_L("Loaded"));
            }

            // automatic selection of added objects
            if (!obj_idxs.empty() && view3D != nullptr) {
                // update printable state for new volumes on canvas3D
                wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_objects(obj_idxs);

                Selection& selection = view3D->get_canvas3d()->get_selection();
                selection.clear();
                for (size_t idx : obj_idxs) {
                    selection.add_object((unsigned int)idx, false);
                }

                if (view3D->get_canvas3d()->get_gizmos_manager().is_enabled())
                    // this is required because the selected object changed and the flatten on face an sla support gizmos need to be updated accordingly
                    view3D->get_canvas3d()->update_gizmos_on_off_state();
            }

            GLGizmoSimplify::add_simplify_suggestion_notification(
                obj_idxs, model.objects, *notification_manager);

            return obj_idxs;
        }

        // #define AUTOPLACEMENT_ON_LOAD

        std::vector<size_t> Plater::priv::load_model_objects(const ModelObjectPtrs& model_objects, bool allow_negative_z, bool call_selection_changed /*= true*/, bool split_object)
        {
            const Vec3d bed_size = Slic3r::to_3d(this->bed.build_volume().bounding_volume2d().size(), 1.0) - 2.0 * Vec3d::Ones();

#ifndef AUTOPLACEMENT_ON_LOAD
            // bool need_arrange = false;
#endif /* AUTOPLACEMENT_ON_LOAD */
            bool scaled_down = false;
            std::vector<size_t> obj_idxs;
            unsigned int obj_count = model.objects.size();

#ifdef AUTOPLACEMENT_ON_LOAD
            ModelInstancePtrs new_instances;
#endif /* AUTOPLACEMENT_ON_LOAD */
            for (ModelObject* model_object : model_objects) {
                auto* object = model.add_object(*model_object);
                object->sort_volumes(get_config_bool("order_volumes"));
                std::string object_name = object->name.empty() ? fs::path(object->input_file).filename().string() : object->name;
                obj_idxs.push_back(obj_count++);

                if (model_object->instances.empty()) {
#ifdef AUTOPLACEMENT_ON_LOAD
                    object->center_around_origin();
                    new_instances.emplace_back(object->add_instance());
#else /* AUTOPLACEMENT_ON_LOAD */
                    // if object has no defined position(s) we need to rearrange everything after loading
                    // need_arrange = true;
                     // add a default instance and center object around origin
                    object->center_around_origin();  // also aligns object to Z = 0
                    ModelInstance* instance = object->add_instance();
                    instance->set_offset(Slic3r::to_3d(this->bed.build_volume().bed_center(), -object->origin_translation(2)));
#endif /* AUTOPLACEMENT_ON_LOAD */
                }

                for (size_t i = 0; i < object->instances.size(); ++i) {
                    ModelInstance* instance = object->instances[i];
                    const Vec3d size = object->instance_bounding_box(i).size();
                    const Vec3d ratio = size.cwiseQuotient(bed_size);
                    const double max_ratio = std::max(ratio(0), ratio(1));
                    if (max_ratio > 10000) {
                        // the size of the object is too big -> this could lead to overflow when moving to clipper coordinates,
                        // so scale down the mesh
                        object->scale_mesh_after_creation(1. / max_ratio);
                        object->origin_translation = Vec3d::Zero();
                        object->center_around_origin();
                        scaled_down = true;
                        break;
                    }
                    else if (max_ratio > 5) {
                        instance->set_scaling_factor(instance->get_scaling_factor() / max_ratio);
                        scaled_down = true;
                    }
                }

                object->ensure_on_bed(allow_negative_z);

                //Assemble
                if (!split_object) {
                    // initial assemble transformation
                    for (ModelObject* model_object : model.objects) {
                        // initialize assemble transformation
                        for (int i = 0; i < model_object->instances.size(); i++) {
                            if (!model_object->instances[i]->is_assemble_initialized()) {
                                auto instance_transformation = model_object->instances[i]->get_transformation();
                                instance_transformation.set_offset(Slic3r::to_3d(this->bed.build_volume().bounding_volume2d().max, 0));
                                model_object->instances[i]->set_assemble_transformation(instance_transformation);
                            }
                        }
                    }
                }
            }

#ifdef AUTOPLACEMENT_ON_LOAD
            // FIXME distance should be a config value /////////////////////////////////
            auto min_obj_distance = static_cast<coord_t>(6 / SCALING_FACTOR);
            const auto* bed_shape_opt = config->opt<ConfigOptionPoints>("bed_shape");
            assert(bed_shape_opt);
            auto& bedpoints = bed_shape_opt->values;
            Polyline bed; bed.points.reserve(bedpoints.size());
            for (auto& v : bedpoints) bed.append(Point::new_scale(v(0), v(1)));

            std::pair<bool, GLCanvas3D::WipeTowerInfo> wti = view3D->get_canvas3d()->get_wipe_tower_info();

            arr::find_new_position(model, new_instances, min_obj_distance, bed, wti);

            // it remains to move the wipe tower:
            view3D->get_canvas3d()->arrange_wipe_tower(wti);

#endif /* AUTOPLACEMENT_ON_LOAD */

            if (scaled_down) {
                GUI::show_info(q,
                    _L("common_slicenotice_model2big_content"),
                    _L("Object too large?"), false);
            }

            notification_manager->close_notification_of_type(NotificationType::UpdatedItemsInfo);
            for (const size_t idx : obj_idxs) {
                wxGetApp().obj_list()->add_object_to_list(idx, call_selection_changed);
            }

            if (call_selection_changed) {
                update();
                // Update InfoItems in ObjectList after update() to use of a correct value of the GLCanvas3D::is_sinking(),
                // which is updated after a view3D->reload_scene(false, flags & (unsigned int)UpdateParams::FORCE_FULL_SCREEN_REFRESH) call
                for (const size_t idx : obj_idxs)
                    wxGetApp().obj_list()->update_info_items(idx);

                object_list_changed();
            }
            this->schedule_background_process();

            return obj_idxs;
        }

        fs::path Plater::priv::get_export_file_path(GUI::FileType file_type)
        {
            // Update printbility state of each of the ModelInstances.
            this->update_print_volume_state();

            const Selection& selection = get_selection();
            int obj_idx = selection.get_object_idx();

            fs::path output_file;
            if (file_type == FT_3MF)
                // for 3mf take the path from the project filename, if any
                output_file = into_path(get_project_filename(".3mf"));

            if (output_file.empty())
            {
                // first try to get the file name from the current selection
                if ((0 <= obj_idx) && (obj_idx < (int)this->model.objects.size()))
                    output_file = this->model.objects[obj_idx]->get_export_filename();

                if (output_file.empty())
                    // Find the file name of the first printable object.
                    output_file = this->model.propose_export_file_name_and_path();

                if (output_file.empty() && !model.objects.empty())
                    // Find the file name of the first object.
                    output_file = this->model.objects[0]->get_export_filename();

                if (output_file.empty())
                    // Use _L("Untitled") name
                    output_file = into_path(_L("Untitled"));
            }
            return output_file;
        }

        wxString Plater::priv::get_export_file(GUI::FileType file_type)
        {
            wxString wildcard;
            switch (file_type) {
            case FT_STL:
            case FT_AMF:
            case FT_3MF:
            case FT_GCODE:
            case FT_ACODE:
            case FT_OBJ:
            case FT_OBJECT:
                wildcard = file_wildcards(file_type);
                break;
            default:
                wildcard = file_wildcards(FT_MODEL);
                break;
            }

            fs::path output_file = get_export_file_path(file_type);

            wxString dlg_title;
            switch (file_type) {
            case FT_STL:
            {
                output_file.replace_extension("stl");
                dlg_title = _L("Export STL file:");
                break;
            }
            case FT_AMF:
            {
                // XXX: Problem on OS X with double extension?
                output_file.replace_extension("zip.amf");
                dlg_title = _L("Export AMF file:");
                break;
            }
            case FT_3MF:
            {
                output_file.replace_extension("3mf");
                dlg_title = _L("Save file as:");
                break;
            }
            case FT_OBJ:
            {
                output_file.replace_extension("obj");
                dlg_title = _L("Export OBJ file:");
                break;
            }
            default: break;
            }

            std::string out_dir = (boost::filesystem::path(output_file).parent_path()).string();
            std::string temp_dir = wxStandardPaths::Get().GetTempDir().utf8_str().data();

            wxFileDialog dlg(q, dlg_title,
                out_dir == temp_dir ? from_u8(wxGetApp().app_config->get("last_output_path")) : (is_shapes_dir(out_dir) ? from_u8(wxGetApp().app_config->get_last_dir()) : from_path(output_file.parent_path())), from_path(output_file.filename()),
                wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

            if (dlg.ShowModal() != wxID_OK)
                return wxEmptyString;

            wxString out_path = dlg.GetPath();
            fs::path path(into_path(out_path));
            wxGetApp().app_config->update_last_output_dir(path.parent_path().string());

            return out_path;
        }

        const Selection& Plater::priv::get_selection() const
        {
            return view3D->get_canvas3d()->get_selection();
        }

        Selection& Plater::priv::get_selection()
        {
            return view3D->get_canvas3d()->get_selection();
        }

        Selection& Plater::priv::get_curr_selection()
        {
            return get_current_canvas3D()->get_selection();
        }

        int Plater::priv::get_selected_object_idx() const
        {
            const int idx = get_selection().get_object_idx();
            return (0 <= idx && idx < int(model.objects.size())) ? idx : -1;
        }

        int Plater::priv::get_selected_volume_idx() const
        {
            auto& selection = get_selection();
            const int idx = selection.get_object_idx();
            if (idx < 0 || int(model.objects.size()) <= idx)
                return-1;
            const GLVolume* v = selection.get_first_volume();
            if (model.objects[idx]->volumes.size() > 1)
                return v->volume_idx();
            return -1;
        }

        void Plater::priv::selection_changed()
        {
            // if the selection is not valid to allow for layer editing, we need to turn off the tool if it is running
            if (!layers_height_allowed() && view3D->is_layers_editing_enabled()) {
                SimpleEvent evt(EVT_GLTOOLBAR_LAYERSEDITING);
                on_action_layersediting(evt);
            }

            if (aobjectmanipulator)
            {
                aobjectmanipulator->set_dirty();
                aobjectmanipulator->update_if_dirty();
            }

            // forces a frame render to update the view (to avoid a missed update if, for example, the context menu appears)
            view3D->render();
        }

        void Plater::priv::object_list_changed()
        {
            const bool export_in_progress = this->background_process.is_export_scheduled(); // || ! send_gcode_file.empty());
            // XXX: is this right?
            const bool model_fits = view3D->get_canvas3d()->check_volumes_outside_state() == ModelInstancePVS_Inside;

            sidebarnew->enableSliceBtn(false, !model.objects.empty() && !export_in_progress && model_fits);
            //sidebar->enable_buttons(!model.objects.empty() && !export_in_progress && model_fits);

            //objectbar->object_list_changed();
        }

        void Plater::priv::select_all()
        {
            view3D->select_all();
            this->sidebarnew->object_list()->update_selections();
        }

        void Plater::priv::deselect_all()
        {
            view3D->deselect_all();
        }

        int Plater::priv::get_object_count()
        {
            return model.objects.size();
        }

        void Plater::priv::remove(size_t obj_idx)
        {
            if (view3D->is_layers_editing_enabled())
                view3D->enable_layers_editing(false);

            m_worker.cancel_all();
            model.delete_object(obj_idx);

            update();
            // Delete object from Sidebar list. Do it after update, so that the GLScene selection is updated with the modified model.
            sidebarnew->object_list()->delete_object_from_list(obj_idx);
            object_list_changed();
        }


        bool Plater::priv::delete_object_from_model(size_t obj_idx)
        {
            // check if object isn't cut
            // show warning message that "cut consistancy" will not be supported any more
            ModelObject* obj = model.objects[obj_idx];
            if (obj->is_cut()) {
                InfoDialog dialog(q, _L("Delete object which is a part of cut object"),
                    _L("You try to delete an object which is a part of a cut object.") + "\n" +
                    _L("This action will break a cut information.\n"
                        "After that eufyMake Studio can't guarantee model consistency"),
                    false, wxYES | wxCANCEL | wxCANCEL_DEFAULT | wxICON_WARNING);
                dialog.SetButtonLabel(wxID_YES, _L("Delete object"));
                if (dialog.ShowModal() == wxID_CANCEL)
                    return false;
            }

            wxString snapshot_label = _L("Delete Object");
            if (!obj->name.empty())
                snapshot_label += ": " + wxString::FromUTF8(obj->name.c_str());
            Plater::TakeSnapshot snapshot(q, snapshot_label);
            m_worker.cancel_all();

            if (obj->is_cut())
                sidebarnew->object_list()->invalidate_cut_info_for_object(obj_idx);

            model.delete_object(obj_idx);
            update();
            object_list_changed();
            return true;
        }

        void Plater::priv::delete_all_objects_from_model()
        {
            Plater::TakeSnapshot snapshot(q, _L("Delete All Objects"));

            if (view3D->is_layers_editing_enabled())
                view3D->enable_layers_editing(false);

            reset_gcode_toolpaths();
            gcode_result.reset();

            view3D->get_canvas3d()->reset_sequential_print_clearance();
            view3D->get_canvas3d()->reset_all_gizmos();

            m_worker.cancel_all();

            // Stop and reset the Print content.
            background_process.reset();
            model.clear_objects();
            update();
            // Delete object from Sidebar list. Do it after update, so that the GLScene selection is updated with the modified model.
            sidebarnew->object_list()->delete_all_objects_from_list();
            //objectbar->delete_all_objects_from_list();
            object_list_changed();

            // The hiding of the slicing results, if shown, is not taken care by the background process, so we do it here
            //sidebar->show_sliced_info_sizer(false);
            //updatePreViewRightSideBar(false, DELETE_ALL_OBJECT);
            sidebarnew->updatePreviewBtn(false, DELETE_ALL_OBJECT);

            model.custom_gcode_per_print_z.gcodes.clear();
        }

        void Plater::priv::reset(std::string name)
        {
            Plater::TakeSnapshot snapshot(q, _L(name.empty() ? "Reset Project" : name), UndoRedo::SnapshotType::ProjectSeparator);

            clear_warnings();

            set_project_filename(name.empty() ? wxString(wxEmptyString) : _L(name));

            if (view3D->is_layers_editing_enabled())
                view3D->enable_layers_editing(false);
            view3D->get_canvas3d()->reset_all_gizmos();

            reset_gcode_toolpaths();
            gcode_result.reset();

            view3D->get_canvas3d()->reset_sequential_print_clearance();

            m_worker.cancel_all();

            // Stop and reset the Print content.
            this->background_process.reset();
            model.clear_objects();
            sidebarnew->GetModelParameterPanel()->resetModelConfig();
            //asseble 
            assemble_view->get_canvas3d()->reset_explosion_ratio();
            update();

            // Delete object from Sidebar list. Do it after update, so that the GLScene selection is updated with the modified model.
            if (wxGetApp().is_editor()) {
                sidebarnew->object_list()->delete_all_objects_from_list();
                object_list_changed();
            }

            // The hiding of the slicing results, if shown, is not taken care by the background process, so we do it here
            //this->sidebar->show_sliced_info_sizer(false);

            model.custom_gcode_per_print_z.gcodes.clear();
        }

        void Plater::priv::mirror(Axis axis)
        {
            view3D->mirror_selection(axis);
        }

        void Plater::priv::split_object()
        {
            int obj_idx = get_selected_object_idx();
            if (obj_idx == -1)
                return;

            // we clone model object because split_object() adds the split volumes
            // into the same model object, thus causing duplicates when we call load_model_objects()
            Model new_model = model;
            ModelObject* current_model_object = new_model.objects[obj_idx];

            //take snapshot befor clear supports, seams, and multimaterial painting.
            Plater::TakeSnapshot snapshot(q, _L("Split to Objects"));

            // Before splitting object we have to remove all custom supports, seams, and multimaterial painting.
            wxGetApp().plater()->clear_before_change_mesh(obj_idx, _u8L("common_slicepopup_split1"));

            wxBusyCursor wait;
            ModelObjectPtrs new_objects;
            current_model_object->split(&new_objects);
            if (new_objects.size() == 1)
                // #ysFIXME use notification
                Slic3r::GUI::warning_catcher(q, _L("The selected object couldn't be split because it contains only one solid part."));
            else
            {
                // If we splited object which is contain some parts/modifiers then all non-solid parts (modifiers) were deleted
                if (current_model_object->volumes.size() > 1 && current_model_object->volumes.size() != new_objects.size())
                    notification_manager->push_notification(NotificationType::CustomNotification,
                        NotificationManager::NotificationLevel::PrintInfoNotificationLevel,
                        _u8L("common_slicepopup_nonsoliddeleted"));

                //Plater::TakeSnapshot snapshot(q, _L("Split to Objects"));

                remove(obj_idx);

                // load all model objects at once, otherwise the plate would be rearranged after each one
                // causing original positions not to be kept
                std::vector<size_t> idxs = load_model_objects(new_objects, false, true, true);

                // clear previosli selection
                get_selection().clear();
                // select newly added objects
                for (size_t idx : idxs)
                {
                    get_selection().add_object((unsigned int)idx, false);
                    // update printable state for new volumes on canvas3D
                    q->canvas3D()->update_instance_printable_state_for_object(idx);
                }
            }
        }

        void Plater::priv::split_volume()
        {
            wxGetApp().obj_list()->split();
        }

        void Plater::priv::scale_selection_to_fit_print_volume()
        {
            this->view3D->get_canvas3d()->get_selection().scale_to_fit_print_volume(this->bed.build_volume());
        }

        void Plater::priv::schedule_background_process()
        {
            delayed_error_message.clear();
            // Trigger the timer event after 0.5s
            this->background_process_timer.Start(500, wxTIMER_ONE_SHOT);
            // Notify the Canvas3D that something has changed, so it may invalidate some of the layer editing stuff.
            this->view3D->get_canvas3d()->set_config(this->config);
        }

        void Plater::priv::update_print_volume_state()
        {
            this->q->model().update_print_volume_state(this->bed.build_volume());
        }

        void Plater::priv::process_validation_warning(const std::string& warning) const
        {
            if (warning.empty())
                notification_manager->close_notification_of_type(NotificationType::ValidateWarning);
            else {
                std::string text = warning;
                std::string hypertext = "";
                std::function<bool(wxEvtHandler*)> action_fn = [](wxEvtHandler*) { return false; };

                if (text == "_SUPPORTS_OFF") {
                    text = _u8L("An object has custom support enforcers which will not be used "
                        "because supports are disabled.") + "\n";
                    hypertext = _u8L("Enable supports for enforcers only");
                    action_fn = [](wxEvtHandler*) {
#if SHOW_OLD_SETTING_DIALOG
                        Tab* print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT);
#endif
                        AnkerTab* print_tab = wxGetApp().getAnkerTab(Preset::TYPE_PRINT);
                        assert(print_tab);
                        DynamicPrintConfig& config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
                        config.set_key_value("support_material", new ConfigOptionBool(true));
                        config.set_key_value("support_material_auto", new ConfigOptionBool(false));
                        print_tab->on_value_change("support_material", config.opt_bool("support_material"));
                        print_tab->on_value_change("support_material_auto", config.opt_bool("support_material_auto"));
                        return true;
                    };
                }

                // comment by Samuel 20231106, Discarded  unused notification text
                //notification_manager->push_notification(
                //    NotificationType::ValidateWarning,
                //    NotificationManager::NotificationLevel::WarningNotificationLevel,
                //    _u8L("WARNING:") + "\n" + text, hypertext, action_fn
                //);
            }
        }


        // Update background processing thread from the current config and Model.
        // Returns a bitmask of UpdateBackgroundProcessReturnState.
        unsigned int Plater::priv::update_background_process(bool force_validation, bool postpone_error_messages)
        {
            // bitmap of enum UpdateBackgroundProcessReturnState
            unsigned int return_state = 0;

            // If the update_background_process() was not called by the timer, kill the timer,
            // so the update_restart_background_process() will not be called again in vain.
            background_process_timer.Stop();
            // Update the "out of print bed" state of ModelInstances.
            update_print_volume_state();
            // Apply new config to the possibly running background task.
            bool               was_running = background_process.running();
            //update by alves, cover right parameters data to the config if fff_print then process
            Print::ApplyStatus invalidated;
            DynamicPrintConfig currentPresetConfig = wxGetApp().preset_bundle->full_config();
            if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
            {
                sidebarnew->updatePreset(currentPresetConfig);
            }
            invalidated = background_process.apply(q->model(), currentPresetConfig);

            // Just redraw the 3D canvas without reloading the scene to consume the update of the layer height profile.
            if (view3D->is_layers_editing_enabled())
                view3D->get_wxglcanvas()->Refresh();

            if (background_process.empty())
                view3D->get_canvas3d()->reset_sequential_print_clearance();

            if (invalidated == Print::APPLY_STATUS_INVALIDATED) {
                // Some previously calculated data on the Print was invalidated.
                // Hide the slicing results, as the current slicing status is no more valid.
                //sidebar->show_sliced_info_sizer(false);
                clear_exported_file_cache();

                if (sidebarnew) {
                    std::string tmpGcodePaht = "";
                    wxGetApp().plater()->setAKeyPrintSlicerTempGcodePath(tmpGcodePaht);

                    //updatePreViewRightSideBar(false, GCODE_INVALID);
                    sidebarnew->updatePreviewBtn(false, GCODE_INVALID);
                    wxGetApp().plater()->set_gcode_valid(false);
                }
                // Reset preview canvases. If the print has been invalidated, the preview canvases will be cleared.
                // Otherwise they will be just refreshed.
                if (preview != nullptr) {
                    // If the preview is not visible, the following line just invalidates the preview,
                    // but the G-code paths or SLA preview are calculated first once the preview is made visible.
                    reset_gcode_toolpaths();
                    preview->reload_print();
                }
                // In FDM mode, we need to reload the 3D scene because of the wipe tower preview box.
                // In SLA mode, we need to reload the 3D scene every time to show the support structures.
                if (printer_technology == ptSLA || (printer_technology == ptFFF && config->opt_bool("wipe_tower")))
                    return_state |= UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE;

                notification_manager->set_slicing_progress_hidden();
            }

            if ((invalidated != Print::APPLY_STATUS_UNCHANGED || force_validation) && !background_process.empty()) {
                // The delayed error message is no more valid.
                delayed_error_message.clear();
                // The state of the Print changed, and it is non-zero. Let's validate it and give the user feedback on errors.
                std::string warning;
                std::string err = background_process.validate(&warning);
                if (err.empty()) {
                    notification_manager->set_all_slicing_errors_gray(true);
                    notification_manager->close_notification_of_type(NotificationType::ValidateError);
                    if (invalidated != Print::APPLY_STATUS_UNCHANGED && background_processing_enabled())
                        return_state |= UPDATE_BACKGROUND_PROCESS_RESTART;

                    // Pass a warning from validation and either show a notification,
                    // or hide the old one.
                    process_validation_warning(warning);
                    if (printer_technology == ptFFF) {
                        view3D->get_canvas3d()->reset_sequential_print_clearance();
                        view3D->get_canvas3d()->set_as_dirty();
                        view3D->get_canvas3d()->request_extra_frame();
                    }
                }
                else {
                    // The print is not valid.
                    // Show error as notification.
                    notification_manager->push_validate_error_notification(err);
                    return_state |= UPDATE_BACKGROUND_PROCESS_INVALID;
                    if (printer_technology == ptFFF) {
                        const Print* print = background_process.fff_print();
                        Polygons polygons;
                        if (print->config().complete_objects)
                            Print::sequential_print_horizontal_clearance_valid(*print, &polygons);
                        view3D->get_canvas3d()->set_sequential_print_clearance_visible(true);
                        view3D->get_canvas3d()->set_sequential_print_clearance_render_fill(true);
                        view3D->get_canvas3d()->set_sequential_print_clearance_polygons(polygons);
                    }
                }
            }
            else {
                if (invalidated == Print::APPLY_STATUS_UNCHANGED && !background_process.empty()) {
                    std::string warning;
                    std::string err = background_process.validate(&warning);
                    if (!err.empty())
                        return return_state;
                }

                if (!this->delayed_error_message.empty())
                    // Reusing the old state.
                    return_state |= UPDATE_BACKGROUND_PROCESS_INVALID;
            }

            //actualizate warnings
            if (invalidated != Print::APPLY_STATUS_UNCHANGED || background_process.empty()) {
                if (background_process.empty())
                    process_validation_warning(std::string());
                actualize_slicing_warnings(*this->background_process.current_print());
                actualize_object_warnings(*this->background_process.current_print());
                show_warning_dialog = false;
                process_completed_with_error = false;
            }

            if (invalidated != Print::APPLY_STATUS_UNCHANGED && was_running && !this->background_process.running() &&
                (return_state & UPDATE_BACKGROUND_PROCESS_RESTART) == 0) {
                // The background processing was killed and it will not be restarted.
                // Post the "canceled" callback message, so that it will be processed after any possible pending status bar update messages.
                wxQueueEvent(GUI::wxGetApp().mainframe->m_plater, new SlicingProcessCompletedEvent(EVT_PROCESS_COMPLETED, 0, SlicingProcessCompletedEvent::Cancelled, std::exception_ptr{}));
            }

            if ((return_state & UPDATE_BACKGROUND_PROCESS_INVALID) != 0)
            {
                // Validation of the background data failed.
                const wxString invalid_str = _L("Invalid data");
                for (auto btn : { ActionButtonType::abReslice, ActionButtonType::abSendGCode, ActionButtonType::abExport })
                    //sidebar->set_btn_label(btn, invalid_str);
                process_completed_with_error = true;
            }
            else
            {
                // Background data is valid.
        //        if ((return_state & UPDATE_BACKGROUND_PROCESS_RESTART) != 0 ||
        //            (return_state & UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE) != 0 )
        //            this->statusbar()->set_status_text(_L("Ready to slice"));
                if ((return_state & UPDATE_BACKGROUND_PROCESS_RESTART) != 0 ||
                    (return_state & UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE) != 0)
                    notification_manager->set_slicing_progress_hidden();

                //sidebar->set_btn_label(ActionButtonType::abExport, _(label_btn_export));
                //sidebar->set_btn_label(ActionButtonType::abSendGCode, _(label_btn_send));
                dirty_state.update_from_preview();

                const wxString slice_string = background_process.running() && wxGetApp().get_mode() == comSimple ?
                    _L("Slicing") + dots : _L("Slice now");
                //sidebar->set_btn_label(ActionButtonType::abReslice, slice_string);

                if (background_process.finished())
                    show_action_buttons(false);
                else if (!background_process.empty() &&
                    !background_process.running()) /* Do not update buttons if background process is running
                                                    * This condition is important for SLA mode especially,
                                                    * when this function is called several times during calculations
                                                    * */
                    show_action_buttons(true);
            }

            return return_state;
        }

        // Restart background processing thread based on a bitmask of UpdateBackgroundProcessReturnState.
        bool Plater::priv::restart_background_process(unsigned int state)
        {
            if (!m_worker.is_idle()) {
                // Avoid a race condition
                return false;
            }
            ANKER_LOG_INFO << "background_process.empty: " << this->background_process.empty() << ", "
                << "background_process.finished: " << this->background_process.finished();
            if (!this->background_process.empty() &&
                (state & priv::UPDATE_BACKGROUND_PROCESS_INVALID) == 0 &&
                (((state & UPDATE_BACKGROUND_PROCESS_FORCE_RESTART) != 0 && !this->background_process.finished()) ||
                    (state & UPDATE_BACKGROUND_PROCESS_FORCE_EXPORT) != 0 ||
                    (state & UPDATE_BACKGROUND_PROCESS_RESTART) != 0)) {
                // The print is valid and it can be started.
                if (this->background_process.start()) {
                    //            this->statusbar()->set_cancel_callback([this]() {
                    //                this->statusbar()->set_status_text(_L("Cancelling"));
                    //                this->background_process.stop();
                    //            });
                    if (!show_warning_dialog)
                        on_slicing_began();
                    return true;
                }
            }
            return false;
        }

        void Plater::priv::update_exported_file_cache(fs::path gcode_path)
        {
            auto get_temp_path = []()-> boost::filesystem::path {
                fs::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
                return temp_path;
            };

            fs::path temp_path = get_temp_path();
            if (fs::exists(temp_path)) {
                try {
                    std::string suffix = ".bak";
                    std::string filename = (/*wxString::FromUTF8 */ (gcode_path.filename().string())/*.ToStdString(wxConvUTF8)*/ + suffix);
                    fs::path tmp_cache_file = temp_path / filename;
                    if (!gcode_path.empty() && fs::exists(gcode_path)) {
                        std::string error_message;
                        CopyFileResult copy_ret_val = copy_file(gcode_path.string(), tmp_cache_file.string(), error_message, true);
                        if (copy_ret_val == CopyFileResult::SUCCESS) {
                            exported_file_cache = tmp_cache_file.string();
                        }
                        else {
                            ANKER_LOG_ERROR << "copy file failed,ret=" << static_cast<int>(copy_ret_val) << " err msg=" << error_message
                                << "  from file=" << gcode_path.string() << "  to file:" << tmp_cache_file.string();
                        }
                    }
                }
                catch(const std::exception& e) {
                    ANKER_LOG_ERROR << "exception:" << e.what();
                }
            }
        }

        void Plater::priv::clear_exported_file_cache()
        {
            if (!exported_file_cache.empty() && fs::exists(fs::path(exported_file_cache))) {
                boost::nowide::remove(exported_file_cache.c_str());
            }

            exported_file_cache.clear();
        }

        void Plater::priv::export_gcode(fs::path output_path, bool output_path_on_removable_media, PrintHostJob upload_job)
        {
            ANKER_LOG_INFO << "output_path :" << output_path.string()<<"     output_path_on_removable_media:"<< output_path_on_removable_media;
            wxCHECK_RET(!(output_path.empty() && upload_job.empty()), "export_gcode: output_path and upload_job empty");
            if (model.objects.empty())
                return;

            if (background_process.is_export_scheduled()) {
                GUI::show_error(q, _L("Another export job is currently running."));
                ANKER_LOG_ERROR << "background_process.is_export_scheduled";
                return;
            }
            // bitmask of UpdateBackgroundProcessReturnState
            unsigned int state = update_background_process(true);
            if (state & priv::UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE)
                view3D->reload_scene(false);

            if ((state & priv::UPDATE_BACKGROUND_PROCESS_INVALID) != 0) {
                ANKER_LOG_ERROR << "UPDATE_BACKGROUND_PROCESS_INVALID";
                return;
            }

            show_warning_dialog = true;
            fs::path gcode_path;
            wxString outputFilePath = wxString::FromUTF8(output_path.make_preferred().string());
            if (outputFilePath.EndsWith(".acode")) {
                fs::path file_name = output_path.filename();
                fs::path sys_temp_dir = fs::temp_directory_path();
                gcode_path = sys_temp_dir / file_name;
                gcode_path.replace_extension(".gcode");
            }
            else
                gcode_path = output_path;

            if (!output_path.empty()) {
                background_process.schedule_export(gcode_path.string(), output_path_on_removable_media);
                notification_manager->push_delayed_notification(NotificationType::ExportOngoing, []() {return true; }, 1000, 0);
            }
            else {
                background_process.schedule_upload(std::move(upload_job));
            }

            //previewRightSidePanel->UpdateGcodePreviewSideBar(true, EXPORT_START);
            sidebarnew->updatePreviewBtn(true, EXPORT_START);
            // If the SLA processing of just a single object's supports is running, restart slicing for the whole object.
            this->background_process.set_task(PrintBase::TaskParams());
            this->restart_background_process(priv::UPDATE_BACKGROUND_PROCESS_FORCE_EXPORT);

            cancel_export = false;
            if (create_AI_file && outputFilePath.EndsWith(".acode"))
            {
                ANKER_LOG_INFO << "start exporting acode";
                auto generateTempOutputPath = []() {
                    boost::filesystem::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
                    temp_path /= boost::format("aiGcode.akpic").str();
                    return temp_path.string();
                };
                auto generateTempGcodePath = []() {
                    boost::filesystem::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
                    temp_path /= boost::format("aiGcode.gcode").str();
                    return temp_path.string();
                };
                auto createTar = [](const wxString& tarFile, const wxString& file1, const wxString& file2) {
                    wxFileOutputStream output(tarFile);
                    wxTarOutputStream tar(output, wxTAR_USTAR);
                    wxFFileInputStream inputFile1(file1);
                    wxFileName fileName1(file1);
                    tar.PutNextEntry(fileName1.GetFullName());
                    tar << inputFile1;
                    tar.CloseEntry();

                    wxFFileInputStream inputFile2(file2);
                    wxFileName fileName2(file2);
                    tar.PutNextEntry(fileName2.GetFullName());
                    tar << inputFile2;
                    tar.CloseEntry();

                    tar.Close();
                };

                exporting_acode = true;
                std::string akpicName = generateTempOutputPath();
                boost::nowide::remove(akpicName.c_str());
                auto picInfo = gcode_result.m_pic_infos;
                {
                    std::string cstring = "aiGcode.gcode";
                    std::vector<double> anker_param_ai_height_array = { 70, 80, 90, 100, 110, 120 };
                    int totalLayer = picInfo.size() > 5 ? picInfo.size() + (5 * anker_param_ai_height_array.size()) : picInfo.size() + (picInfo.size() * anker_param_ai_height_array.size()) ;
                    //int totalLayer = picInfo.size() > 5 ? 35 : picInfo.size() + (5 * anker_param_ai_height_array.size());
                    post_gcode::processAiPicture  picWrite(cstring, akpicName, picInfo.size(), totalLayer);
                    auto floatToString = [](float num) {
                        std::ostringstream oss;
                        if (num == static_cast<int>(num)) {
                            oss << static_cast<int>(num);  // Display as integer
                        }
                        else {
                            oss << std::fixed << std::setprecision(2) << num;  // Display with 2 decimal places
                        }
                        return oss.str();
                        };
                    auto write_single_image = [&picWrite,this, &floatToString](const GCodeProcessorResult::picPosInfo& pi,float curPercentage, float picSize,float extraHeight) {
                        post_gcode::picData tmp_pic_data;
                        preview->get_canvas3d()->render_akpic(tmp_pic_data, 2048, 1196, pi.Y, pi.Z + extraHeight, { 0,pi.layer_id });
                        auto compressed = GCodeThumbnails::compress_akpic(tmp_pic_data);
                        post_gcode::imgInfo tImgInfo;
                        tImgInfo.data = static_cast<char*>(compressed->data);
                        tImgInfo.size = compressed->size;
                        post_gcode::imageHead iHead;
                        iHead.percentage = curPercentage / picSize;
                        iHead.layer_num = (unsigned int)(curPercentage);
                        //std::copy(roi_info.begin(), roi_info.end(), iHead.roi_info);
                        iHead.file_size = tImgInfo.size;

                        std::string  saveName = "mesh_layer" + floatToString(curPercentage+1) + ".jpg";
                        std::copy(saveName.begin(), saveName.end(), iHead.name);
                        picWrite.writeImage(iHead, tImgInfo);
                        return iHead.percentage;
                        };

                    unsigned int i = 0;
                    for (const auto pi : picInfo) {
                        if (app_closing) {
                            ANKER_LOG_INFO << "app_closing, stop exporting task";
                            cancel_export = true;
                            break;
                        }
                        if (cancel_export) {
                            ANKER_LOG_INFO << "export_gcode cancel ,break";
                            break;
                        }
                        float percentage = 0.0;
                        if (pi.layer_id <= 5) {
                            for (int h = 0;  h< anker_param_ai_height_array.size();h++) {
                                percentage = write_single_image(pi, i+(h+1)*0.01, picInfo.size(), anker_param_ai_height_array[h]);
                            }
                        }
                        percentage = write_single_image(pi, i, picInfo.size(), 0.0);
                        i++;
                         //notify UI to update progress
                        if (i % 10 == 1) {
                            if (export_progress_change_cb)
                                export_progress_change_cb(percentage);
                                // to give up cpu for UI updating
                                wxYield();
                        }
                    }
                }

                if (!cancel_export) {
                    // picInfo render is complete , pack .gcode and .akpic file into .acode(tar) file
                    if (!output_path.empty()) {
                        boost::filesystem::path aiGcode_path = generateTempGcodePath();
                        if (fs::exists(gcode_path)) {
                            try {
                                std::string error_message = "";
                                CopyFileResult copy_ret_val = copy_file(gcode_path.string(), aiGcode_path.string(), error_message, output_path_on_removable_media);
                                if (copy_ret_val == CopyFileResult::SUCCESS) {
                                    createTar(wxString::FromUTF8(output_path.string()), wxString::FromUTF8(aiGcode_path.string()), wxString::FromUTF8(akpicName));
                                    boost::nowide::remove(gcode_path.string().c_str());
                                    boost::nowide::remove(aiGcode_path.string().c_str());
                                    boost::nowide::remove(akpicName.c_str());
                                    update_exported_file_cache(output_path);
                                    notification_manager->push_exporting_finished_notification(last_output_path, last_output_dir_path, false);
                                }
                                else {
                                    ANKER_LOG_ERROR<<"copy file failed,ret="<<static_cast<int>(copy_ret_val)<<" err msg="<< error_message << "  from file=" << gcode_path.string() << "  to file:" << aiGcode_path.string();
                                }
                            }
                            catch (...) {
                                ANKER_LOG_ERROR << "Unknown error occured during exporting A-code. ";
                                throw Slic3r::ExportError(_u8L("Unknown error occured during exporting A-code."));
                            }
                        }
                        else {
                            ANKER_LOG_ERROR<<"gcode file have not grenerated:"<< gcode_path.string();
                        }

                        //previewRightSidePanel->UpdateGcodePreviewSideBar(true, EXPORT_ACODE_COMPLETE);
                        sidebarnew->updatePreviewBtn(true, EXPORT_ACODE_COMPLETE);
                    }
                    else
                    {
                        ANKER_LOG_INFO << "output_path is empty";
                    }

                    if (export_progress_change_cb)
                        export_progress_change_cb(1.0f);
                }
                else {
                    // export was interupt, to clean the gcode m_outFile
                    boost::nowide::remove(akpicName.c_str());
                    boost::nowide::remove(output_path.string().c_str());
                    if (fs::exists(gcode_path))
                        boost::nowide::remove(gcode_path.string().c_str());
                    //previewRightSidePanel->UpdateGcodePreviewSideBar(true, EXPORT_ACODE_CANCEL);
                    sidebarnew->updatePreviewBtn(true, EXPORT_ACODE_CANCEL);
                }

                exporting_acode = false;
            }
            else
            {
            }

            if (app_closing) {
                // ANKER_LOG_INFO << "send wxCUSTOMEVT_EXPORT_FINISHED_SAFE_QUIT_APP";
                wxCommandEvent event(wxCUSTOMEVT_EXPORT_FINISHED_SAFE_QUIT_APP);
                GUI::wxGetApp().plater()->GetEventHandler()->ProcessEvent(event);
            }

            ANKER_LOG_INFO << "export_gcode exit";
        }

        unsigned int Plater::priv::update_restart_background_process(bool force_update_scene, bool force_update_preview)
        {
            // bitmask of UpdateBackgroundProcessReturnState
            unsigned int state = this->update_background_process(false);
            if (force_update_scene || (state & UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE) != 0)
                view3D->reload_scene(false);

            if (force_update_preview)
                this->preview->reload_print();
            this->restart_background_process(state);
            return state;
        }

        void Plater::priv::update_fff_scene()
        {

            if (this->preview != nullptr)
                this->preview->reload_print();
            // In case this was MM print, wipe tower bounding box on 3D tab might need redrawing with exact depth:
            view3D->reload_scene(true);
            //add assemble view related logic
            assemble_view->reload_scene(true);
        }

        void Plater::priv::update_sla_scene()
        {
            // Update the SLAPrint from the current Model, so that the reload_scene()
            // pulls the correct data.
            delayed_scene_refresh = false;
            this->update_restart_background_process(true, true);
        }

        // class used to show a wxBusyCursor and a wxBusyInfo
        // and hide them on demand
        class Busy
        {
            wxWindow* m_parent{ nullptr };
            std::unique_ptr<wxBusyCursor> m_cursor;
            std::unique_ptr<wxBusyInfo> m_dlg;

        public:
            Busy(const wxString& message, wxWindow* parent = nullptr) {
                m_parent = parent;
                m_cursor = std::make_unique<wxBusyCursor>();
                m_dlg = std::make_unique<wxBusyInfo>(message, m_parent);
            }

            ~Busy() { reset(); }

            void update(const wxString& message) {
                // this is ugly but necessary because the call to wxBusyInfo::UpdateLabel() is not working [WX 3.1.4]
                m_dlg = std::make_unique<wxBusyInfo>(message, m_parent);
                //        m_dlg->UpdateLabel(message);
            }

            void reset() {
                m_cursor.reset();
                m_dlg.reset();
            }
        };

        bool Plater::priv::replace_volume_with_stl(int object_idx, int volume_idx, const fs::path& new_path, const wxString& snapshot)
        {
            const std::string path = new_path.string();
            Busy busy(_L("Replace from:") + " " + from_u8(path), q->get_current_canvas3D()->get_wxglcanvas());

            Model new_model;
            try {
                new_model = Model::read_from_file(path, nullptr, nullptr, Model::LoadAttribute::AddDefaultInstances);
                for (ModelObject* model_object : new_model.objects) {
                    model_object->center_around_origin();
                    model_object->ensure_on_bed();
                }
            }
            catch (std::exception& e) {
                // error while loading
                busy.reset();
                GUI::show_error(q, e.what());
                return false;
            }

            if (new_model.objects.size() > 1 || new_model.objects.front()->volumes.size() > 1) {
                MessageDialog dlg(q, _L("Unable to replace with more than one volume"), _L("Error during replace"), wxOK | wxOK_DEFAULT | wxICON_WARNING);
                dlg.ShowModal();
                return false;
            }

            if (!snapshot.empty())
                q->take_snapshot(snapshot);

            ModelObject* old_model_object = model.objects[object_idx];
            ModelVolume* old_volume = old_model_object->volumes[volume_idx];

            bool sinking = old_model_object->min_z() < SINKING_Z_THRESHOLD;

            ModelObject* new_model_object = new_model.objects.front();
            old_model_object->add_volume(*new_model_object->volumes.front());
            ModelVolume* new_volume = old_model_object->volumes.back();
            new_volume->set_new_unique_id();
            new_volume->config.apply(old_volume->config);
            new_volume->set_type(old_volume->type());
            new_volume->set_material_id(old_volume->material_id());
            new_volume->set_transformation(old_volume->get_transformation());
            new_volume->translate(new_volume->get_transformation().get_matrix_no_offset() * (new_volume->source.mesh_offset - old_volume->source.mesh_offset));
            assert(!old_volume->source.is_converted_from_inches || !old_volume->source.is_converted_from_meters);
            if (old_volume->source.is_converted_from_inches)
                new_volume->convert_from_imperial_units();
            else if (old_volume->source.is_converted_from_meters)
                new_volume->convert_from_meters();
            new_volume->supported_facets.assign(old_volume->supported_facets);
            new_volume->seam_facets.assign(old_volume->seam_facets);
            new_volume->mmu_segmentation_facets.assign(old_volume->mmu_segmentation_facets);
            std::swap(old_model_object->volumes[volume_idx], old_model_object->volumes.back());
            old_model_object->delete_volume(old_model_object->volumes.size() - 1);
            if (!sinking)
                old_model_object->ensure_on_bed();
            old_model_object->sort_volumes(get_config_bool("order_volumes"));

            // if object has just one volume, rename object too
            if (old_model_object->volumes.size() == 1)
                old_model_object->name = old_model_object->volumes.front()->name;

            // update new name in ObjectList
            //objectbar->update_name_in_list(object_idx, volume_idx);
            sidebarnew->object_list()->update_name_in_list(object_idx, volume_idx);

            sla::reproject_points_and_holes(old_model_object);

            return true;
        }

        void Plater::priv::replace_with_stl()
        {
            if (!q->canvas3D()->get_gizmos_manager().check_gizmos_closed_except(GLGizmosManager::EType::Undefined))
                return;

            const Selection& selection = get_selection();

            if (selection.is_wipe_tower() || get_selection().get_volume_idxs().size() != 1)
                return;

            const GLVolume* v = selection.get_first_volume();
            int object_idx = v->object_idx();
            int volume_idx = v->volume_idx();

            // collects paths of files to load

            const ModelObject* object = model.objects[object_idx];
            ModelVolume* volume = object->volumes[volume_idx];

            // clear old seam/support/mmu info
            wxGetApp().aobj_manipul()->set_dirty();
            q->canvas3D()->reset_main_toolbar_toggled_state();
            q->canvas3D()->get_gizmos_manager().reset_all_states();
            q->canvas3D()->get_gizmos_manager().update_data();
            volume->supported_facets.reset();
            volume->seam_facets.reset();
            volume->mmu_segmentation_facets.reset();

            fs::path input_path;
            if (!volume->source.input_file.empty() && fs::exists(volume->source.input_file))
                input_path = volume->source.input_file;

            wxString title = _L("Select the new file");
            title += ":";
            wxFileDialog dialog(q, title, "", from_u8(input_path.filename().string()), file_wildcards(FT_MODEL), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (dialog.ShowModal() != wxID_OK)
                return;

            fs::path out_path = dialog.GetPath().ToUTF8().data();
            if (out_path.empty()) {
                MessageDialog dlg(q, _L("File for the replace wasn't selected"), _L("Error during replace"), wxOK | wxOK_DEFAULT | wxICON_WARNING);
                dlg.ShowModal();
                return;
            }

            if (!replace_volume_with_stl(object_idx, volume_idx, out_path, _L("Replace with STL")))
                return;

            // update 3D scene
            update();

            // new GLVolumes have been created at this point, so update their printable state
            for (size_t i = 0; i < model.objects.size(); ++i) {
                view3D->get_canvas3d()->update_instance_printable_state_for_object(i);
            }
        }

        static std::vector<std::pair<int, int>> reloadable_volumes(const Model& model, const Selection& selection)
        {
            std::vector<std::pair<int, int>> ret;
            const std::set<unsigned int>& selected_volumes_idxs = selection.get_volume_idxs();
            for (unsigned int idx : selected_volumes_idxs) {
                const GLVolume& v = *selection.get_volume(idx);
                const int o_idx = v.object_idx();
                if (0 <= o_idx && o_idx < int(model.objects.size())) {
                    const ModelObject* obj = model.objects[o_idx];
                    const int v_idx = v.volume_idx();
                    if (0 <= v_idx && v_idx < int(obj->volumes.size())) {
                        const ModelVolume* vol = obj->volumes[v_idx];
                        if (!vol->source.is_from_builtin_objects && !vol->source.input_file.empty() &&
                            !fs::path(vol->source.input_file).extension().string().empty())
                            ret.push_back({ o_idx, v_idx });
                    }
                }
            }
            return ret;
        }

        void Plater::priv::reload_from_disk()
        {
            // collect selected reloadable ModelVolumes
            std::vector<std::pair<int, int>> selected_volumes = reloadable_volumes(model, get_selection());

            // nothing to reload, return
            if (selected_volumes.empty())
                return;

            std::sort(selected_volumes.begin(), selected_volumes.end(), [](const std::pair<int, int>& v1, const std::pair<int, int>& v2) {
                return (v1.first < v2.first) || (v1.first == v2.first && v1.second < v2.second);
                });

            selected_volumes.erase(std::unique(selected_volumes.begin(), selected_volumes.end(), [](const std::pair<int, int>& v1, const std::pair<int, int>& v2) {
                return (v1.first == v2.first) && (v1.second == v2.second); }), selected_volumes.end());

            // clear old seam/support/mmu info
            wxGetApp().aobj_manipul()->set_dirty();
            q->canvas3D()->reset_main_toolbar_toggled_state();
            q->canvas3D()->get_gizmos_manager().reset_all_states();
            q->canvas3D()->get_gizmos_manager().update_data();
            for (int i = 0; i < selected_volumes.size(); i++)
            {
                const ModelObject* object = model.objects[selected_volumes[i].first];
                ModelVolume* volume = object->volumes[selected_volumes[i].second];

                volume->supported_facets.reset();
                volume->seam_facets.reset();
                volume->mmu_segmentation_facets.reset();
            }

            // collects paths of files to load
            std::vector<fs::path> input_paths;
            std::vector<fs::path> missing_input_paths;
            std::vector<std::pair<fs::path, fs::path>> replace_paths;
            for (auto [obj_idx, vol_idx] : selected_volumes) {
                const ModelObject* object = model.objects[obj_idx];
                const ModelVolume* volume = object->volumes[vol_idx];
                if (fs::exists(volume->source.input_file))
                    input_paths.push_back(volume->source.input_file);
                else {
                    // searches the source in the same folder containing the object
                    bool found = false;
                    if (!object->input_file.empty()) {
                        fs::path object_path = fs::path(object->input_file).remove_filename();
                        if (!object_path.empty()) {
                            object_path /= fs::path(volume->source.input_file).filename();
                            if (fs::exists(object_path)) {
                                input_paths.push_back(object_path);
                                found = true;
                            }
                        }
                    }
                    if (!found)
                        missing_input_paths.push_back(volume->source.input_file);
                }
            }

            std::sort(missing_input_paths.begin(), missing_input_paths.end());
            missing_input_paths.erase(std::unique(missing_input_paths.begin(), missing_input_paths.end()), missing_input_paths.end());

            while (!missing_input_paths.empty()) {
                // ask user to select the missing file
                fs::path search = missing_input_paths.back();
                wxString title = _L("Please select the file to reload");
#if defined(__APPLE__)
                title += " (" + from_u8(search.filename().string()) + ")";
#endif // __APPLE__
                title += ":";
                wxFileDialog dialog(q, title, "", from_u8(search.filename().string()), file_wildcards(FT_MODEL), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
                if (dialog.ShowModal() != wxID_OK)
                    return;

                std::string sel_filename_path = dialog.GetPath().ToUTF8().data();
                std::string sel_filename = fs::path(sel_filename_path).filename().string();
                if (boost::algorithm::iequals(search.filename().string(), sel_filename)) {
                    input_paths.push_back(sel_filename_path);
                    missing_input_paths.pop_back();

                    fs::path sel_path = fs::path(sel_filename_path).remove_filename().string();

                    std::vector<fs::path>::iterator it = missing_input_paths.begin();
                    while (it != missing_input_paths.end()) {
                        // try to use the path of the selected file with all remaining missing files
                        fs::path repathed_filename = sel_path;
                        repathed_filename /= it->filename();
                        if (fs::exists(repathed_filename)) {
                            input_paths.push_back(repathed_filename.string());
                            it = missing_input_paths.erase(it);
                        }
                        else
                            ++it;
                    }
                }
                else {
                    wxString message = _L("The selected file") + " (" + from_u8(sel_filename) + ") " +
                        _L("differs from the original file") + " (" + from_u8(search.filename().string()) + ").\n" + _L("Do you want to replace it") + " ?";
                    //wxMessageDialog dlg(q, message, wxMessageBoxCaptionStr, wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
                    MessageDialog dlg(q, message, wxMessageBoxCaptionStr, wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
                    if (dlg.ShowModal() == wxID_YES)
                        replace_paths.emplace_back(search, sel_filename_path);

                    missing_input_paths.pop_back();
                }
            }

            std::sort(input_paths.begin(), input_paths.end());
            input_paths.erase(std::unique(input_paths.begin(), input_paths.end()), input_paths.end());

            std::sort(replace_paths.begin(), replace_paths.end());
            replace_paths.erase(std::unique(replace_paths.begin(), replace_paths.end()), replace_paths.end());

            Plater::TakeSnapshot snapshot(q, _L("Reload from disk"));

            std::vector<wxString> fail_list;
            std::set<int> updated_obj_list;

            Busy busy(_L("Reload from:"), q->get_current_canvas3D()->get_wxglcanvas());

            // load one file at a time
            for (size_t i = 0; i < input_paths.size(); ++i) {
                const auto& path = input_paths[i].string();

                busy.update(_L("Reload from:") + " " + from_u8(path));

                Model new_model;
                try
                {
                    new_model = Model::read_from_file(path, nullptr, nullptr, Model::LoadAttribute::AddDefaultInstances);
                    for (ModelObject* model_object : new_model.objects) {
                        model_object->center_around_origin();
                        model_object->ensure_on_bed();
                    }
                }
                catch (std::exception& e)
                {
                    // error while loading
                    busy.reset();
                    GUI::show_error(q, e.what());
                    return;
                }

                // update the selected volumes whose source is the current file
                for (auto [obj_idx, vol_idx] : selected_volumes) {
                    ModelObject* old_model_object = model.objects[obj_idx];
                    ModelVolume* old_volume = old_model_object->volumes[vol_idx];

                    bool sinking = old_model_object->min_z() < SINKING_Z_THRESHOLD;

                    bool has_source = !old_volume->source.input_file.empty() && boost::algorithm::iequals(fs::path(old_volume->source.input_file).filename().string(), fs::path(path).filename().string());
                    bool has_name = !old_volume->name.empty() && boost::algorithm::iequals(old_volume->name, fs::path(path).filename().string());
                    if (has_source || has_name) {
                        int new_volume_idx = -1;
                        int new_object_idx = -1;
                        bool match_found = false;
                        // take idxs from the matching volume
                        if (has_source && old_volume->source.object_idx < int(new_model.objects.size())) {
                            const ModelObject* obj = new_model.objects[old_volume->source.object_idx];
                            if (old_volume->source.volume_idx < int(obj->volumes.size())) {
                                if (obj->volumes[old_volume->source.volume_idx]->name == old_volume->name) {
                                    new_volume_idx = old_volume->source.volume_idx;
                                    new_object_idx = old_volume->source.object_idx;
                                    match_found = true;
                                }
                            }
                        }

                        if (!match_found && has_name) {
                            // take idxs from the 1st matching volume
                            for (size_t o = 0; o < new_model.objects.size(); ++o) {
                                ModelObject* obj = new_model.objects[o];
                                bool found = false;
                                for (size_t v = 0; v < obj->volumes.size(); ++v) {
                                    if (obj->volumes[v]->name == old_volume->name) {
                                        new_volume_idx = (int)v;
                                        new_object_idx = (int)o;
                                        found = true;
                                        break;
                                    }
                                }
                                if (found)
                                    break;
                            }
                        }

                        if (new_object_idx < 0 || int(new_model.objects.size()) <= new_object_idx) {
                            fail_list.push_back(from_u8(has_source ? old_volume->source.input_file : old_volume->name));
                            continue;
                        }
                        ModelObject* new_model_object = new_model.objects[new_object_idx];
                        if (new_volume_idx < 0 || int(new_model_object->volumes.size()) <= new_volume_idx) {
                            fail_list.push_back(from_u8(has_source ? old_volume->source.input_file : old_volume->name));
                            continue;
                        }

                        old_model_object->add_volume(*new_model_object->volumes[new_volume_idx]);
                        ModelVolume* new_volume = old_model_object->volumes.back();
                        new_volume->set_new_unique_id();
                        new_volume->config.apply(old_volume->config);
                        new_volume->set_type(old_volume->type());
                        new_volume->set_material_id(old_volume->material_id());
                        new_volume->set_transformation(
                            old_volume->get_transformation().get_matrix() *
                            old_volume->source.transform.get_matrix_no_offset() *
                            Geometry::translation_transform(new_volume->source.mesh_offset - old_volume->source.mesh_offset) *
                            new_volume->source.transform.get_matrix_no_offset().inverse()
                        );
                        new_volume->source.object_idx = old_volume->source.object_idx;
                        new_volume->source.volume_idx = old_volume->source.volume_idx;
                        assert(!old_volume->source.is_converted_from_inches || !old_volume->source.is_converted_from_meters);
                        if (old_volume->source.is_converted_from_inches)
                            new_volume->convert_from_imperial_units();
                        else if (old_volume->source.is_converted_from_meters)
                            new_volume->convert_from_meters();
                        std::swap(old_model_object->volumes[vol_idx], old_model_object->volumes.back());
                        old_model_object->delete_volume(old_model_object->volumes.size() - 1);
                        if (!sinking)
                            old_model_object->ensure_on_bed();
                        old_model_object->sort_volumes(get_config_bool("order_volumes"));

                        sla::reproject_points_and_holes(old_model_object);

                        updated_obj_list.insert(obj_idx);

                        // Fix warning icon in object list
                        wxGetApp().obj_list()->update_item_error_icon(obj_idx, vol_idx);
                    }
                }
            }

            busy.reset();

            for (auto [src, dest] : replace_paths) {
                for (auto [obj_idx, vol_idx] : selected_volumes) {
                    if (boost::algorithm::iequals(model.objects[obj_idx]->volumes[vol_idx]->source.input_file, src.string()))
                    {
                        replace_volume_with_stl(obj_idx, vol_idx, dest, "");

                        updated_obj_list.insert(obj_idx);
                    }
                }
            }

            if (!fail_list.empty()) {
                wxString message = _L("Unable to reload:") + "\n";
                for (const wxString& s : fail_list) {
                    message += s + "\n";
                }
                //wxMessageDialog dlg(q, message, _L("Error during reload"), wxOK | wxOK_DEFAULT | wxICON_WARNING);
                MessageDialog dlg(q, message, _L("Error during reload"), wxOK | wxOK_DEFAULT | wxICON_WARNING);
                dlg.ShowModal();
            }

            // update 3D scene
            update();

            // new GLVolumes have been created at this point, so update their printable state
            for (size_t i = 0; i < model.objects.size(); ++i) {
                view3D->get_canvas3d()->update_instance_printable_state_for_object(i);
            }
        }

        void Plater::priv::reload_all_from_disk()
        {
            if (model.objects.empty())
                return;

            Plater::TakeSnapshot snapshot(q, _L("Reload all from disk"));
            Plater::SuppressSnapshots suppress(q);

            Selection& selection = get_selection();
            Selection::IndicesList curr_idxs = selection.get_volume_idxs();
            // reload from disk uses selection
            select_all();
            reload_from_disk();
            // restore previous selection
            selection.clear();
            for (unsigned int idx : curr_idxs) {
                selection.add(idx, false);
            }
        }

        void Plater::priv::set_current_panel(wxPanel* panel)
        {
           
            if (std::find(panels.begin(), panels.end(), panel) == panels.end())
                return;

#ifdef __WXMAC__
            bool force_render = (current_panel != nullptr);
#endif // __WXMAC__

            if (current_panel == panel)
                return;

            wxPanel* old_panel = current_panel;
            current_panel = panel;

            sidebar->Hide();
            // to reduce flickering when changing view, first set as visible the new current panel
            for (wxPanel* p : panels) {
                if (p == current_panel) {
#ifdef __WXMAC__
                    // On Mac we need also to force a render to avoid flickering when changing view
                    if (force_render) {
                        if (p == view3D)
                            dynamic_cast<View3D*>(p)->get_canvas3d()->render();
                        else if (p == preview)
                            dynamic_cast<Preview*>(p)->get_canvas3d()->render();
                    }
#endif // __WXMAC__
                    p->Show();
                }
                else
                {
                    p->Show(false);
                }
            }
            // then set to invisible the other
     /*       wxWindowUpdateLocker noUpdates(q);
            for (wxPanel* p : panels) {
                if (p != current_panel)
                    p->Hide();
            }*/

            panel_sizer->Layout();

            if (wxGetApp().plater()) {
                Camera& cam = wxGetApp().plater()->get_camera();
                if (old_panel == preview || old_panel == view3D) {
                    view3D->get_canvas3d()->get_camera().load_camera_view(cam);
                }
                else if (old_panel == assemble_view) {
                    assemble_view->get_canvas3d()->get_camera().load_camera_view(cam);
                }
                if (current_panel == view3D || current_panel == preview) {
                    cam.load_camera_view(view3D->get_canvas3d()->get_camera());
                }
                else if (current_panel == assemble_view) {
                    cam.load_camera_view(assemble_view->get_canvas3d()->get_camera());
                }
            }

            if (current_panel == view3D) {
                if (old_panel == preview) {
                    if (preview->is_GcodeImported()) {
                        ANKER_LOG_INFO << "reset_last_loaded_gcode.";
                        q->reset_last_loaded_gcode();
                        ANKER_LOG_INFO << "reset_gcode_toolpaths.";
                        this->reset_gcode_toolpaths();
                        ANKER_LOG_INFO << "gcode_result.reset.";
                        this->gcode_result.reset();
                        ANKER_LOG_INFO << "clearGCodeSliceState.";
                        this->background_process.clearGCodeSliceState();
                       
                        // Because dragging GCode changes the current platform, the default platform is restored when you switch back to slicing.
                        GUI::wxGetApp().load_current_presets();
                        GUI::wxGetApp().plater()->set_bed_shape();

                        preview->setLoadStatus(false);
                        preview->setGCodeImportType(false);
                        wxGetApp().set_app_mode();
                    }
                    preview->get_canvas3d()->unbind_event_handlers();
                }

                view3D->get_canvas3d()->bind_event_handlers();

                if (view3D->is_reload_delayed()) {
                    // Delayed loading of the 3D scene.
                    if (printer_technology == ptSLA) {
                        // Update the SLAPrint from the current Model, so that the reload_scene()
                        // pulls the correct data.
                        update_restart_background_process(true, false);
                    }
                    else
                        view3D->reload_scene(true);
                }

                // sets the canvas as dirty to force a render at the 1st idle event (wxWidgets IsShownOnScreen() is buggy and cannot be used reliably)
                view3D->set_as_dirty();
                // reset cached size to force a resize on next call to render() to keep imgui in synch with canvas size
                view3D->get_canvas3d()->reset_old_size();
                view_toolbar.select_item("3D");
                if (notification_manager != nullptr)
                    notification_manager->set_in_preview(false);
            }
            else if (current_panel == preview) {
                if (old_panel == view3D)
                    view3D->get_canvas3d()->unbind_event_handlers();
                else if (old_panel == assemble_view)
                    assemble_view->get_canvas3d()->unbind_event_handlers();

                preview->get_canvas3d()->bind_event_handlers();

                if (wxGetApp().is_editor()) {
                    // see: Plater::priv::object_list_changed()
                    // FIXME: it may be better to have a single function making this check and let it be called wherever needed
                    bool export_in_progress = this->background_process.is_export_scheduled();
                    bool model_fits = view3D->get_canvas3d()->check_volumes_outside_state() != ModelInstancePVS_Partly_Outside;
                    // 					if (view3D->get_canvas3d()->check_volumes_outside_state() == ModelInstancePVS_Fully_Outside ||
                    // 						view3D->get_canvas3d()->check_volumes_outside_state() == ModelInstancePVS_Partly_Outside)
                    // 						sidebarnew->enableSliceBtn(false, false);
                    // 					else
                    // 						sidebarnew->enableSliceBtn(false, true);
                    if (!model.objects.empty() && !export_in_progress && model_fits) {
                        preview->get_canvas3d()->init_gcode_viewer();
                        q->reslice();
                    }
                    // keeps current gcode preview, if any
                    preview->reload_print(true);
                }

                preview->set_as_dirty();
                // reset cached size to force a resize on next call to render() to keep imgui in synch with canvas size
                preview->get_canvas3d()->reset_old_size();
                view_toolbar.select_item("Preview");
                if (notification_manager != nullptr)
                    notification_manager->set_in_preview(true);
            }
            else if (current_panel == assemble_view) {
                if (old_panel == view3D) {
                    view3D->get_canvas3d()->unbind_event_handlers();
                }
                else if (old_panel == preview)
                    preview->get_canvas3d()->unbind_event_handlers();

                assemble_view->get_canvas3d()->bind_event_handlers();
                assemble_view->reload_scene(true);

                // BBS set default view and zoom
                if (first_enter_assemble) {
                    wxGetApp().plater()->get_camera().requires_zoom_to_volumes = true;
                    first_enter_assemble = false;
                }

                assemble_view->set_as_dirty();
            }

            current_panel->SetFocusFromKbd();
        }

        void Plater::priv::on_select_preset(wxCommandEvent& evt)
        {
            PlaterPresetComboBox* combo = static_cast<PlaterPresetComboBox*>(evt.GetEventObject());
            Preset::Type preset_type = combo->get_type();

            // see https://github.com/prusa3d/PrusaSlicer/issues/3889
            // Under OSX: in case of use of a same names written in different case (like "ENDER" and "Ender"),
            // m_presets_choice->GetSelection() will return first item, because search in PopupListCtrl is case-insensitive.
            // So, use GetSelection() from event parameter 
            int selection = evt.GetSelection();

            auto idx = combo->get_extruder_idx();

            //! Because of The MSW and GTK version of wxBitmapComboBox derived from wxComboBox,
            //! but the OSX version derived from wxOwnerDrawnCombo.
            //! So, to get selected string we do
            //!     combo->GetString(combo->GetSelection())
            //! instead of
            //!     combo->GetStringSelection().ToUTF8().data());

            std::string preset_name = wxGetApp().preset_bundle->get_preset_name_by_alias(preset_type,
                Preset::remove_suffix_modified(combo->GetString(selection).ToUTF8().data()));

            if (preset_type == Preset::TYPE_FILAMENT) {
                wxGetApp().preset_bundle->set_filament_preset(idx, preset_name);
            }

            std::string last_selected_ph_printer_name = combo->get_selected_ph_printer_name();

            bool select_preset = !combo->selection_is_changed_according_to_physical_printers();
            // TODO: ?
            if (preset_type == Preset::TYPE_FILAMENT /*&& sidebar->is_multifilament()*/) {
                // Only update the plater UI for the 2nd and other filaments.
                combo->update();
            }
            else if (select_preset) {
                wxWindowUpdateLocker noUpdates(sidebar->presets_panel());
#if SHOW_OLD_SETTING_DIALOG
                wxGetApp().get_tab(preset_type)->select_preset(preset_name, false, last_selected_ph_printer_name);
#endif
                wxGetApp().getAnkerTab(preset_type)->select_preset(preset_name, false, last_selected_ph_printer_name);
            }

            //update by alves
            if (preset_type != Preset::TYPE_PRINTER || select_preset) {
                //update by alves, cover right parameters data to the config if fff_print then process
                DynamicPrintConfig currentPresetConfig = wxGetApp().preset_bundle->full_config();
                if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
                {
                    sidebarnew->updatePreset(currentPresetConfig);
                }
                q->on_config_change(currentPresetConfig);
            }
            if (preset_type == Preset::TYPE_PRINTER) {
                /* Settings list can be changed after printer preset changing, so
                 * update all settings items for all item had it.
                 * Furthermore, Layers editing is implemented only for FFF printers
                 * and for SLA presets they should be deleted
                 */
                 wxGetApp().obj_list()->update_object_list_by_printer_technology();
            }

#ifdef __WXMSW__
            // From the Win 2004 preset combobox lose a focus after change the preset selection
            // and that is why the up/down arrow doesn't work properly
            // (see https://github.com/prusa3d/PrusaSlicer/issues/5531 ).
            // So, set the focus to the combobox explicitly
            combo->SetFocus();
#endif
        }

        void Plater::priv::on_select_preset_sidebarnew(wxCommandEvent& evt) {
            // add by allen for ankerCfgDlg
            //PlaterPresetComboBox* combo = static_cast<PlaterPresetComboBox*>(evt.GetEventObject());
            AnkerPlaterPresetComboBox* combo = static_cast<AnkerPlaterPresetComboBox*>(evt.GetEventObject());

            Preset::Type preset_type = combo->get_type();
            if (-1 == preset_type)
                return;
            // see https://github.com/prusa3d/PrusaSlicer/issues/3889
            // Under OSX: in case of use of a same names written in different case (like "ENDER" and "Ender"),
            // m_presets_choice->GetSelection() will return first item, because search in PopupListCtrl is case-insensitive.
            // So, use GetSelection() from event parameter 
            int selection = evt.GetSelection();

            auto idx = combo->get_extruder_idx();

            //! Because of The MSW and GTK version of wxBitmapComboBox derived from wxComboBox,
            //! but the OSX version derived from wxOwnerDrawnCombo.
            //! So, to get selected string we do
            //!     combo->GetString(combo->GetSelection())
            //! instead of
            //!     combo->GetStringSelection().ToUTF8().data());

            std::string preset_name = wxGetApp().preset_bundle->get_preset_name_by_alias(preset_type,
                Preset::remove_suffix_modified(combo->GetString(selection).ToUTF8().data()));

            if (preset_type == Preset::TYPE_FILAMENT) {
                wxGetApp().preset_bundle->set_filament_preset(idx, preset_name);
            }

            std::string last_selected_ph_printer_name = combo->get_selected_ph_printer_name();

            bool select_preset = !combo->selection_is_changed_according_to_physical_printers();
            // TODO: ?
            if (preset_type == Preset::TYPE_FILAMENT && sidebarnew->isMultiFilament()) {
                // Only update the plater UI for the 2nd and other filaments.
                combo->update();
            }
            else if (select_preset) {
                /*wxWindowUpdateLocker noUpdates(sidebar->presets_panel());*/
                // add by allen for ankerCfgDlg
#if SHOW_OLD_SETTING_DIALOG
                wxGetApp().get_tab(preset_type)->select_preset(preset_name, false, last_selected_ph_printer_name);
#endif
                wxGetApp().getAnkerTab(preset_type)->select_preset(preset_name, false, last_selected_ph_printer_name);
            }
            //update by alves
            if (preset_type != Preset::TYPE_PRINTER || select_preset) {
                //update by alves, cover right parameters data to the config if fff_print then process
                DynamicPrintConfig currentPresetConfig = wxGetApp().preset_bundle->full_config();
                if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
                {
                    sidebarnew->updatePreset(currentPresetConfig);
                }
                q->on_config_change(currentPresetConfig);

            }
            if (preset_type == Preset::TYPE_PRINTER) {
                /* Settings list can be changed after printer preset changing, so
                 * update all settings items for all item had it.
                 * Furthermore, Layers editing is implemented only for FFF printers
                 * and for SLA presets they should be deleted
                 */
                 wxGetApp().obj_list()->update_object_list_by_printer_technology();
                 // update extruder
                 //sidebarnew->onExtrudersChange();
            }

#ifdef __WXMSW__
            // From the Win 2004 preset combobox lose a focus after change the preset selection
            // and that is why the up/down arrow doesn't work properly
            // (see https://github.com/prusa3d/PrusaSlicer/issues/5531 ).
            // So, set the focus to the combobox explicitly
            combo->SetFocus();
#endif
        }

        void Plater::priv::on_slicing_update(SlicingStatusEvent& evt)
        {
            if (evt.status.percent >= -1) {
                if (!m_worker.is_idle()) {
                    // Avoid a race condition
                    return;
                }                
                //        this->statusbar()->set_progress(evt.status.percent);
                //        this->statusbar()->set_status_text(_(evt.status.text) + wxString::FromUTF8("…"));
                notification_manager->set_slicing_progress_percentage(evt.status.text, (float)evt.status.percent / 100.0f);
            }
            if (evt.status.flags & (PrintBase::SlicingStatus::RELOAD_SCENE | PrintBase::SlicingStatus::RELOAD_SLA_SUPPORT_POINTS)) {
                switch (this->printer_technology) {
                case ptFFF:
                    this->update_fff_scene();
                    break;
                case ptSLA:
                    // If RELOAD_SLA_SUPPORT_POINTS, then the SLA gizmo is updated (reload_scene calls update_gizmos_data)
                    if (view3D->is_dragging())
                        delayed_scene_refresh = true;
                    else
                        this->update_sla_scene();
                    break;
                default: break;
                }
            }
            else if (evt.status.flags & PrintBase::SlicingStatus::RELOAD_SLA_PREVIEW) {
                // Update the SLA preview. Only called if not RELOAD_SLA_SUPPORT_POINTS, as the block above will refresh the preview anyways.
                this->preview->reload_print();
            }

            if ((evt.status.flags & PrintBase::SlicingStatus::UPDATE_PRINT_STEP_WARNINGS) &&
                static_cast<PrintStep>(evt.status.warning_step) == psAlertWhenSupportsNeeded &&
                !get_app_config()->get_bool("alert_when_supports_needed")) {
                // This alerts are from psAlertWhenSupportsNeeded and the respective app settings is not Enabled, so discard the alerts.
            }
            else if (evt.status.flags &
                (PrintBase::SlicingStatus::UPDATE_PRINT_STEP_WARNINGS | PrintBase::SlicingStatus::UPDATE_PRINT_OBJECT_STEP_WARNINGS)) {
                // Update notification center with warnings of object_id and its warning_step.
                ObjectID object_id = evt.status.warning_object_id;
                int warning_step = evt.status.warning_step;
                PrintStateBase::StateWithWarnings state;
                if (evt.status.flags & PrintBase::SlicingStatus::UPDATE_PRINT_STEP_WARNINGS) {
                    state = this->printer_technology == ptFFF ?
                        this->fff_print.step_state_with_warnings(static_cast<PrintStep>(warning_step)) :
                        this->sla_print.step_state_with_warnings(static_cast<SLAPrintStep>(warning_step));
                }
                else if (this->printer_technology == ptFFF) {
                    const PrintObject* print_object = this->fff_print.get_object(object_id);
                    if (print_object)
                        state = print_object->step_state_with_warnings(static_cast<PrintObjectStep>(warning_step));
                }
                else {
                    const SLAPrintObject* print_object = this->sla_print.get_object(object_id);
                    if (print_object)
                        state = print_object->step_state_with_warnings(static_cast<SLAPrintObjectStep>(warning_step));
                }
                // Now process state.warnings.
                for (auto const& warning : state.warnings) {
                    if (warning.current) {
                        notification_manager->push_slicing_warning_notification(warning.message, false, object_id, warning_step);
                        add_warning(warning, object_id.id);
                    }
                }
            }
        }

        void Plater::priv::on_slicing_completed(wxCommandEvent& evt)
        {
            if (view3D->is_dragging()) // updating scene now would interfere with the gizmo dragging
                delayed_scene_refresh = true;
            else {
                if (this->printer_technology == ptFFF)
                {
                    report_slicing_buryevt("finish","0","finish slice model");

                    this->update_fff_scene();
                    
                    { //if slice times over than 3, popup a star comment dialog(only one).
                        if (m_sliceTimes <= 0)
                            return;
                        if (m_isPrint)
                            return;

                        auto appConfig = Slic3r::GUI::wxGetApp().app_config;
                        if (nullptr == appConfig) {
                            ANKER_LOG_INFO << "02mmPrinter  nohint for user set.";
                            return;
                        }

                        std::string userId = appConfig->get("user_id");
                        std::string appVersion = appConfig->get("version");
                        
                       
                        int sliceTimes = 0;
                        //static int sliceTimes = 1;
                        std::string slice_key = userId + "_" + appVersion;
                        appConfig->get_slice_times("SliceTimesCheck", slice_key, sliceTimes);
                        
                   
                        if (sliceTimes >= m_sliceTimes - 1) {
                            wxCommandEvent event(wxCUSTOMEVT_ANKER_SLICE_FOR_COMMENT);
                            GUI::wxGetApp().plater()->GetEventHandler()->ProcessEvent(event);
                            if (appConfig) {
                                appConfig->set_slice_times("SliceTimesCheck", slice_key, userId, appVersion, 0);
                            }
                        } else {
                            sliceTimes++;
                            if (appConfig) {
                                appConfig->set_slice_times("SliceTimesCheck", slice_key, userId, appVersion, sliceTimes);                              
                            }
                        }
                    }
                }
                else
                    this->update_sla_scene();
            }
        }

        void Plater::priv::on_export_began(wxCommandEvent& evt)
        {
            if (show_warning_dialog)
                warnings_dialog();
        }
        void Plater::priv::on_slicing_began()
        {
            clear_warnings();
            notification_manager->close_notification_of_type(NotificationType::SignDetected);
            notification_manager->close_notification_of_type(NotificationType::ExportFinished);
            notification_manager->set_slicing_progress_began();
        }

        bool g_flag = true; int g_RandNumber = 0;
        void Plater::priv::report_slicing_buryevt(const std::string& status, const std::string& errorCode, const std::string& errorMsg)
        {
            auto GenRandNum = [=]()->int {
                std::srand(static_cast<unsigned int>(std::time(0)));
                return std::rand();
            };

            do {
                if (status == "start" && errorCode == "-1") {
                    g_flag = true;
                    break;
                }

                if (status == "start") {
                    if (g_flag) {
                        g_RandNumber = GenRandNum();
                        g_flag = false; 
                    }
                    break;
                }

                if (status == "cancel" || status == "finish") {
                    g_flag = true;
                    break;
                }

            } while (false);


            // report: slice_model event
            std::string modelName = std::string();
            for (auto object : model.objects) {
                modelName += object->name + " ";
            }
            std::string modelSize = std::string();
            auto workTime = wxDateTime::Now().GetValue().ToString().ToStdString();

            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_sm_file_name, modelName));
            buryMap.insert(std::make_pair(c_sm_file_size, modelSize));
            buryMap.insert(std::make_pair(c_sm_time, workTime));
            buryMap.insert(std::make_pair(c_sm_status, status));
            buryMap.insert(std::make_pair(c_sm_error_code, errorCode));
            buryMap.insert(std::make_pair(c_sm_error_msg, errorMsg));
            buryMap.insert(std::make_pair(c_sm_unique_id, std::to_string(g_RandNumber)));
            ANKER_LOG_INFO << "Report bury event is " << e_slice_model;
            reportBuryEvent(e_slice_model, buryMap);       
        }

        void Plater::priv::add_warning(const Slic3r::PrintStateBase::Warning& warning, size_t oid)
        {
            for (auto const& it : current_warnings) {
                if (warning.message_id == it.first.message_id) {
                    if (warning.message_id != 0 || (warning.message_id == 0 && warning.message == it.first.message))
                        return;
                }
            }
            current_warnings.emplace_back(std::pair<Slic3r::PrintStateBase::Warning, size_t>(warning, oid));
        }
        void Plater::priv::actualize_slicing_warnings(const PrintBase& print)
        {
            std::vector<ObjectID> ids = print.print_object_ids();
            if (ids.empty()) {
                clear_warnings();
                return;
            }
            ids.emplace_back(print.id());
            std::sort(ids.begin(), ids.end());
            notification_manager->remove_slicing_warnings_of_released_objects(ids);
            notification_manager->set_all_slicing_warnings_gray(true);
        }
        void Plater::priv::actualize_object_warnings(const PrintBase& print)
        {
            std::vector<ObjectID> ids;
            for (const ModelObject* object : print.model().objects)
            {
                ids.push_back(object->id());
            }
            std::sort(ids.begin(), ids.end());
            notification_manager->remove_simplify_suggestion_of_released_objects(ids);
        }
        void Plater::priv::clear_warnings()
        {
            notification_manager->close_slicing_errors_and_warnings();
            this->current_warnings.clear();
        }
        bool Plater::priv::warnings_dialog()
        {
            std::vector<std::pair<Slic3r::PrintStateBase::Warning, size_t>> current_critical_warnings{};
            for (const auto& w : current_warnings) {
                if (w.first.level == PrintStateBase::WarningLevel::CRITICAL) {
                    current_critical_warnings.push_back(w);
                }
            }

            if (current_critical_warnings.empty())
                return true;
            std::string text = _u8L("There are active warnings concerning sliced models:") + "\n";
            for (auto const& it : current_critical_warnings) {
                size_t next_n = it.first.message.find_first_of('\n', 0);
                text += "\n";
                if (next_n != std::string::npos)
                    text += it.first.message.substr(0, next_n);
                else
                    text += it.first.message;
            }
            //text += "\n\nDo you still wish to export?";
            //wxMessageDialog msg_wingow(this->q, from_u8(text), wxString(SLIC3R_APP_NAME " ") + _L("generated warnings"), wxOK);
            MessageDialog msg_wingow(this->q, from_u8(text), wxString(SLIC3R_APP_NAME " ") + _L("generated warnings"), wxOK);
            const auto res = msg_wingow.ShowModal();
            return res == wxID_OK;

        }
        void Plater::priv::on_process_completed(SlicingProcessCompletedEvent& evt)
        {
            // Stop the background task, wait until the thread goes into the "Idle" state.
            // At this point of time the thread should be either finished or canceled,
            // so the following call just confirms, that the produced data were consumed.
            this->background_process.stop();
            //    this->statusbar()->reset_cancel_callback();
            //    this->statusbar()->stop_busy();
            notification_manager->set_slicing_progress_export_possible();

            bool is_export_gcode = q->is_exporting_gcode();
            // Reset the "export G-code path" name, so that the automatic background processing will be enabled again.
            this->background_process.reset_export();
            // This bool stops showing export finished notification even when process_completed_with_error is false
            bool has_error = false;
            if (evt.error()) {
                std::pair<std::string, bool> message = evt.format_error_message();
                if (evt.critical_error()) {
                    if (q->m_tracking_popup_menu) {
                        // We don't want to pop-up a message box when tracking a pop-up menu.
                        // We postpone the error message instead.
                        q->m_tracking_popup_menu_error_message = message.first;
                    }
                    else {
                        show_error(q, message.first, message.second);
                        notification_manager->set_slicing_progress_hidden();
                        notification_manager->stop_delayed_notifications_of_type(NotificationType::ExportOngoing);
                    }
                }
                else
                    notification_manager->push_slicing_error_notification(message.first);
                //        this->statusbar()->set_status_text(from_u8(message.first));
                if (evt.invalidate_plater())
                {
                    const wxString invalid_str = _L("Invalid data");
                /*    for (auto btn : { ActionButtonType::abReslice, ActionButtonType::abSendGCode, ActionButtonType::abExport })
                        sidebar->set_btn_label(btn, invalid_str);*/
                    process_completed_with_error = true;
                }
                has_error = true;
            }
            if (evt.cancelled()) {
                report_slicing_buryevt("cancel","1","cancel slice model");
                //        this->statusbar()->set_status_text(_L("Cancelled"));
                this->notification_manager->set_slicing_progress_canceled(_u8L("common_slicepopup_slicingcancel"));
            }

            // This updates the "Slice now", "Export G-code", "Arrange" buttons status.
            // Namely, it refreshes the "Out of print bed" property of all the ModelObjects, and it enables
            // the "Slice now" and "Export G-code" buttons based on their "out of bed" status.
            this->object_list_changed();

            // refresh preview
            if (view3D->is_dragging()) // updating scene now would interfere with the gizmo dragging
                delayed_scene_refresh = true;
            else {
                if (this->printer_technology == ptFFF)
                    this->update_fff_scene();
                else
                    this->update_sla_scene();
            }

       //    this->sidebar->show_sliced_info_sizer(evt.success());
            if (this->sidebarnew) {
                if (is_export_gcode) { // export g-code complete
                    //updatePreViewRightSideBar(true, PROCCESS_GCODE_COMPLETE);
                    sidebarnew->updatePreviewBtn(true, PROCCESS_GCODE_COMPLETE);
                }
                else { // slice complete
                    if (evt.success() && q) {
                        this->preview->UpdateGcodeInfo(q->get_temp_gcode_output_path());
                    }
                    //updatePreViewRightSideBar(evt.success(), evt.cancelled() ? SLICING_CANCEL : PROCCESS_GCODE_COMPLETE);
                    sidebarnew->updatePreviewBtn(evt.success(), evt.cancelled() ? SLICING_CANCEL : PROCCESS_GCODE_COMPLETE);
                    wxGetApp().plater()->set_gcode_valid(evt.success());
                }
            }

            if (evt.cancelled()) {
              /*  if (wxGetApp().get_mode() == comSimple)
                    sidebar->set_btn_label(ActionButtonType::abReslice, "Slice now");*/
                show_action_buttons(true);
            }
            else {
                if (wxGetApp().get_mode() == comSimple) {
                    show_action_buttons(false);
                }
                if (exporting_status != ExportingStatus::NOT_EXPORTING && !has_error) {
                    notification_manager->stop_delayed_notifications_of_type(NotificationType::ExportOngoing);
                    notification_manager->close_notification_of_type(NotificationType::ExportOngoing);
                }

                bool is_acode_path = wxString::FromUTF8(last_output_path).EndsWith(".acode");
                // If writing to removable drive was scheduled, show notification with eject button
                if (exporting_status == ExportingStatus::EXPORTING_TO_REMOVABLE && !has_error) {
                    show_action_buttons(false);
                    if (!(create_AI_file && is_acode_path)) {
                        notification_manager->push_exporting_finished_notification(last_output_path, last_output_dir_path,
                            // Don't offer the "Eject" button on ChromeOS, the Linux side has no control over it.
                            platform_flavor() != PlatformFlavor::LinuxOnChromium);
                    }
                    wxGetApp().removable_drive_manager()->set_exporting_finished(true);
                }
                else if (exporting_status == ExportingStatus::EXPORTING_TO_LOCAL && !has_error) {
                    if (!(create_AI_file && is_acode_path))
                        notification_manager->push_exporting_finished_notification(last_output_path, last_output_dir_path, false);
                }
            }
            exporting_status = ExportingStatus::NOT_EXPORTING;
        }

        void Plater::priv::on_action_request_model_download(const std::string& url)
        {
            ANKER_LOG_INFO << "on_download_import_model url: " << url;
#ifdef __APPLE__
            if (!wxGetApp().mainframe->IsActive()) {
                wxGetApp().mainframe->Raise();
                wxGetApp().mainframe->SetFocus();
                wxGetApp().mainframe->Maximize();
            }
#endif // 
            if (m_downLoad_controller) {
                m_downLoad_controller->Init(url);
                m_downLoad_controller->StartDownLoad();
            }
        }

        void Plater::priv::on_download_complete(wxCommandEvent& evt)
        {
            auto open_stl = [&](const wxString &file) {
                std::vector<fs::path> paths{ boost::filesystem::path(file.wx_str()) };
                wxString snapshot_label;
                snapshot_label = _L("Download Object");
                snapshot_label += ": ";
                snapshot_label += wxString::FromUTF8(paths.front().filename().string().c_str());
                Plater::TakeSnapshot snapshot(q, snapshot_label);
                if (!load_files(paths, true, false, false).empty()) {
                    wxGetApp().mainframe->update_title();
                }
            };

            auto open_3mf = [&](const wxString& file) {
                if (!wxGetApp().can_load_project())
                    return;

                wxString input_file;
                q->load_project(file);
            };

            auto fileName = evt.GetString();
            if (!fileName.empty()) {
                if (fileName.EndsWith(".3mf")) {
                    open_3mf(fileName);
                }
                else if (fileName.EndsWith(".stl") || fileName.EndsWith(".STL") || fileName.EndsWith(".OBJ") || fileName.EndsWith(".obj")) {
                    open_stl(fileName);
                }
            }

            m_downLoad_controller->OnDownLoadComplete();
            m_downLoad_controller->StopDownLoad();
        }

        void Plater::priv::on_layer_editing_toggled(bool enable)
        {
            view3D->enable_layers_editing(enable);
            view3D->set_as_dirty();
        }

        void Plater::priv::on_action_add(SimpleEvent&)
        {
            if (q != nullptr)
                q->add_model();
        }

        void Plater::priv::on_action_split_objects(SimpleEvent&)
        {
            split_object();
        }

        void Plater::priv::on_action_split_volumes(SimpleEvent&)
        {
            split_volume();
        }

        void Plater::priv::on_action_layersediting(SimpleEvent&)
        {
            view3D->enable_layers_editing(!view3D->is_layers_editing_enabled());
            notification_manager->set_move_from_overlay(view3D->is_layers_editing_enabled());
        }

        void Plater::priv::on_object_select(SimpleEvent& evt)
        {
            if (auto obj_list = wxGetApp().obj_list())
                obj_list->update_selections();
            else
                return;
            selection_changed();
        }

        void Plater::priv::on_right_click(RBtnEvent& evt)
        {
            int obj_idx = get_selected_object_idx();

            wxMenu* menu = nullptr;

            if (obj_idx == -1) { // no one or several object are selected
                if (evt.data.second) { // right button was clicked on empty space
                    if (!get_selection().is_empty()) // several objects are selected in 3DScene
                        return;
                    menu = menus.default_menu();
                }
                else {
                    if (current_panel == assemble_view) {
                        menu = menus.assemble_multi_selection_menu();
                    }
                    else {
                        menu = menus.multi_selection_menu();
                    }
                }
            }
            else {
                // If in 3DScene is(are) selected volume(s), but right button was clicked on empty space
                if (evt.data.second)
                    return;

                // Each context menu respects to the selected item in ObjectList, 
                // so this selection should be updated before menu creation
                wxGetApp().obj_list()->update_selections();

                //        if (printer_technology == ptSLA)
                //            menu = menus.sla_object_menu();
                //        else {
                const Selection& selection = get_selection();
                // show "Object menu" for each one or several FullInstance instead of FullObject
                const bool is_some_full_instances = selection.is_single_full_instance() ||
                    selection.is_single_full_object() ||
                    selection.is_multiple_full_instance();
                const bool is_part = selection.is_single_volume_or_modifier() && !selection.is_any_connector();

                if (current_panel == assemble_view) {
                    menu = is_some_full_instances ? menus.assemble_object_menu() :
                        is_part ? menus.assemble_part_menu() : menus.assemble_multi_selection_menu();
                }
                else {
                    if (is_some_full_instances)
                        menu = printer_technology == ptSLA ? menus.sla_object_menu() : menus.object_menu();
                    else if (is_part) {
                        const GLVolume* gl_volume = selection.get_first_volume();
                        const ModelVolume* model_volume = get_model_volume(*gl_volume, selection.get_model()->objects);
                        menu = (model_volume != nullptr && model_volume->is_text()) ? menus.text_part_menu() :
                            (model_volume != nullptr && model_volume->is_svg()) ? menus.svg_part_menu() :
                            menus.part_menu();
                    }
                    else
                        menu = menus.multi_selection_menu();
                }
                //if (is_some_full_instances)
                //    menu = printer_technology == ptSLA ? menus.sla_object_menu() : menus.object_menu();
                //else if (is_part)
                //    menu = selection.is_single_text() ? menus.text_part_menu() : menus.part_menu();
                //else
                //    menu = menus.multi_selection_menu();
                //        }
            }

            if (q != nullptr && menu) {
                Vec2d mouse_position = evt.data.first;
                wxPoint position(static_cast<int>(mouse_position.x()),
                    static_cast<int>(mouse_position.y()));
#ifdef __linux__
                // For some reason on Linux the menu isn't displayed if position is
                // specified (even though the position is sane).
                position = wxDefaultPosition;
#endif

#ifndef __APPLE__
                if(sidebarnew)
                    sidebarnew->Freeze();
#endif

                GLCanvas3D& canvas = *q->canvas3D();
                canvas.apply_retina_scale(mouse_position);
                canvas.set_popup_menu_position(mouse_position);
                q->PopupMenu(menu, position);
                canvas.clear_popup_menu_position();

#ifndef __APPLE__
                if (sidebarnew) 
                    sidebarnew->Thaw();
#endif
            }
        }

        void Plater::priv::on_wipetower_moved(Vec3dEvent& evt)
        {
//            DynamicPrintConfig cfg;
//            cfg.opt<ConfigOptionFloat>("wipe_tower_x", true)->value = evt.data(0);
//            cfg.opt<ConfigOptionFloat>("wipe_tower_y", true)->value = evt.data(1);
//#if SHOW_OLD_SETTING_DIALOG
//            wxGetApp().get_tab(Preset::TYPE_PRINT)->load_config(cfg);
//#endif
//            wxGetApp().getAnkerTab(Preset::TYPE_PRINT)->load_config(cfg);

            double x = evt.data(0);
            double y = evt.data(1);
            auto angle = config->opt_float("wipe_tower_rotation_angle");
            wxGetApp().sidebarnew().moveWipeTower(x, y, angle);
        }

        void Plater::priv::on_wipetower_rotated(Vec3dEvent& evt)
        {
//            DynamicPrintConfig cfg;
//            cfg.opt<ConfigOptionFloat>("wipe_tower_x", true)->value = evt.data(0);
//            cfg.opt<ConfigOptionFloat>("wipe_tower_y", true)->value = evt.data(1);
//            cfg.opt<ConfigOptionFloat>("wipe_tower_rotation_angle", true)->value = Geometry::rad2deg(evt.data(2));
//#if SHOW_OLD_SETTING_DIALOG
//            wxGetApp().get_tab(Preset::TYPE_PRINT)->load_config(cfg);
//#endif
//            wxGetApp().getAnkerTab(Preset::TYPE_PRINT)->load_config(cfg);

            double x = evt.data(0);
            double y = evt.data(1);
            auto angle = Geometry::rad2deg(evt.data(2));
            wxGetApp().sidebarnew().moveWipeTower(x, y, angle);
        }

        void Plater::priv::on_update_geometry(Vec3dsEvent<2>&)
        {
            // TODO
        }

        void Plater::priv::on_3dcanvas_mouse_dragging_started(SimpleEvent&)
        {
            view3D->get_canvas3d()->reset_sequential_print_clearance();
        }

        // Update the scene from the background processing,
        // if the update message was received during mouse manipulation.
        void Plater::priv::on_3dcanvas_mouse_dragging_finished(SimpleEvent&)
        {
            if (delayed_scene_refresh) {
                delayed_scene_refresh = false;
                update_sla_scene();
            }
        }

        void Plater::priv::generate_thumbnail(ThumbnailData& data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, Camera::EType camera_type)
        {
            view3D->get_canvas3d()->render_thumbnail(data, w, h, thumbnail_params, camera_type);
        }

        ThumbnailsList Plater::priv::generate_thumbnails(const ThumbnailsParams& params, Camera::EType camera_type)
        {
            ThumbnailsList thumbnails;
            for (const Vec2d& size : params.sizes) {
                thumbnails.push_back(ThumbnailData());
                Point isize(size); // round to ints
                generate_thumbnail(thumbnails.back(), isize.x(), isize.y(), params, camera_type);
                if (!thumbnails.back().is_valid())
                    thumbnails.pop_back();
            }
            return thumbnails;
        }

        wxString Plater::priv::get_project_filename(const wxString& extension) const
        {
            return m_project_filename.empty() ? "" : m_project_filename + extension;
        }

        void Plater::priv::set_project_filename(const wxString& filename)
        {
            boost::filesystem::path full_path = into_path(filename);
            boost::filesystem::path ext = full_path.extension();
            if (boost::iequals(ext.string(), ".amf")) {
                // Remove the first extension.
                full_path.replace_extension("");
                // It may be ".zip.amf".
                if (boost::iequals(full_path.extension().string(), ".zip"))
                    // Remove the 2nd extension.
                    full_path.replace_extension("");
            }
            else {
                // Remove just one extension.
                full_path.replace_extension("");
            }

            m_project_filename = from_path(full_path);
            wxGetApp().mainframe->update_title();

            const fs::path temp_path = wxStandardPaths::Get().GetTempDir().utf8_str().data();
            bool in_temp = (temp_path == full_path.parent_path().make_preferred());

            if (!filename.empty() && !in_temp)
                wxGetApp().mainframe->add_to_recent_projects(filename);
        }

        void Plater::priv::init_notification_manager()
        {
            if (!notification_manager)
                return;
            notification_manager->init();

            auto cancel_callback = [this]() {
                if (this->background_process.idle())
                    return false;
                this->background_process.stop();
                return true;
            };
            notification_manager->init_slicing_progress_notification(cancel_callback);
            notification_manager->set_fff(printer_technology == ptFFF);
            notification_manager->init_progress_indicator();
        }

        void Plater::priv::set_current_canvas_as_dirty()
        {
            if (current_panel == view3D)
                view3D->set_as_dirty();
            else if (current_panel == preview)
                preview->set_as_dirty();
            else if (current_panel == assemble_view)
                assemble_view->set_as_dirty();
        }

        GLCanvas3D* Plater::priv::get_current_canvas3D()
        {
            if (current_panel == view3D)
                return view3D->get_canvas3d();
            else if (current_panel == preview)
                return preview->get_canvas3d();
            else if (current_panel == assemble_view)
                return assemble_view->get_canvas3d();
            else
                return nullptr;
            //return (current_panel == view3D) ? view3D->get_canvas3d() : ((current_panel == preview) ? preview->get_canvas3d() : nullptr);
        }

        void Plater::priv::unbind_canvas_event_handlers()
        {
            if (view3D != nullptr)
                view3D->get_canvas3d()->unbind_event_handlers();

            if (preview != nullptr)
                preview->get_canvas3d()->unbind_event_handlers();

            if (assemble_view != nullptr)
                assemble_view->get_canvas3d()->unbind_event_handlers();
        }

        void Plater::priv::reset_canvas_volumes()
        {
            if (view3D != nullptr)
                view3D->get_canvas3d()->reset_volumes();

            if (preview != nullptr)
                preview->get_canvas3d()->reset_volumes();
        }

        bool Plater::priv::init_view_toolbar()
        {
            if (wxGetApp().is_gcode_viewer())
                return true;

            if (view_toolbar.get_items_count() > 0)
                // already initialized
                return true;

            // mod by allen at 20230627
            // Make the background color of the view_toolbar button group consistent with the background color of the main window
            /*BackgroundTexture::Metadata background_data;
            background_data.filename = "toolbar_background.png";
            background_data.left = 16;
            background_data.top = 16;
            background_data.right = 16;
            background_data.bottom = 16;

            if (!view_toolbar.init(background_data))
                return false;*/

            view_toolbar.set_horizontal_orientation(GLToolbar::Layout::HO_Left);
            view_toolbar.set_vertical_orientation(GLToolbar::Layout::VO_Bottom);
            view_toolbar.set_border(5.0f);
            view_toolbar.set_gap_size(1.0f);

            GLToolbarItem::Data item;

            item.name = "3D";
            item.icon_filename = "editor.svg";
            //item.tooltip = _u8L("3D editor view") + " [" + GUI::shortkey_ctrl_prefix() + "5]";
            item.sprite_id = 0;
            item.left.action_callback = [this]() { if (this->q != nullptr) wxPostEvent(this->q, SimpleEvent(EVT_GLVIEWTOOLBAR_3D)); };
            item.toggled_callback = [this]() {return current_view_mode == VIEW_MODE_3D; };
            if (!view_toolbar.add_item(item))
                return false;

            item.name = "Preview";
            item.icon_filename = "preview.svg";
            //item.tooltip = _u8L("Preview") + " [" + GUI::shortkey_ctrl_prefix() + "6]";
            item.sprite_id = 1;
            item.left.action_callback = [this]() { if (this->q != nullptr) wxPostEvent(this->q, SimpleEvent(EVT_GLVIEWTOOLBAR_PREVIEW)); };
            item.toggled_callback = [this]() {return current_view_mode == VIEW_MODE_PREVIEW; };
            if (!view_toolbar.add_item(item))
                return false;

            view_toolbar.select_item("3D");
            view_toolbar.set_enabled(true);

            return true;
        }

        bool Plater::priv::init_collapse_toolbar()
        {
            if (wxGetApp().is_gcode_viewer())
                return true;

            if (collapse_toolbar.get_items_count() > 0)
                // already initialized
                return true;

            BackgroundTexture::Metadata background_data;
            background_data.filename = "toolbar_background.png";
            background_data.left = 16;
            background_data.top = 16;
            background_data.right = 16;
            background_data.bottom = 16;

            if (!collapse_toolbar.init(background_data))
                return false;

            collapse_toolbar.set_layout_type(GLToolbar::Layout::Vertical);
            collapse_toolbar.set_horizontal_orientation(GLToolbar::Layout::HO_Right);
            collapse_toolbar.set_vertical_orientation(GLToolbar::Layout::VO_Top);
            collapse_toolbar.set_border(5.0f);
            collapse_toolbar.set_separator_size(5);
            collapse_toolbar.set_gap_size(2);

            GLToolbarItem::Data item;

            item.name = "collapse_sidebar";
            item.icon_filename = "collapse.svg";
            item.sprite_id = 0;
            item.left.action_callback = []() {
                wxGetApp().plater()->collapse_sidebar(!wxGetApp().plater()->is_sidebar_collapsed());
            };

            if (!collapse_toolbar.add_item(item))
                return false;

            // Now "collapse" sidebar to current state. This is done so the tooltip
            // is updated before the toolbar is first used.
            wxGetApp().plater()->collapse_sidebar(wxGetApp().plater()->is_sidebar_collapsed());
            return true;
        }

        void Plater::priv::set_preview_layers_slider_values_range(int bottom, int top)
        {
            preview->set_layers_slider_values_range(bottom, top);
        }
         
        void Plater::priv::update_preview_moves_slider()
        {
            preview->update_moves_slider();
        }

        void Plater::priv::enable_preview_moves_slider(bool enable)
        {
            preview->enable_moves_slider(enable);
        }

        void Plater::priv::reset_gcode_toolpaths()
        {
            preview->get_canvas3d()->reset_gcode_toolpaths();
        }

        bool Plater::priv::can_set_instance_to_object() const
        {
            const int obj_idx = get_selected_object_idx();
            return 0 <= obj_idx && obj_idx < (int)model.objects.size() && model.objects[obj_idx]->instances.size() > 1;
        }

        bool Plater::priv::can_split(bool to_objects) const
        {
            // return objectbar->is_splittable(to_objects);
            return sidebarnew->object_list()->is_splittable(to_objects);
        }

        bool Plater::priv::can_fillcolor() const
        {
            //BBS TODO
            return true;
        }

        bool Plater::priv::can_scale_to_print_volume() const
        {
            const BuildVolume::Type type = this->bed.build_volume().type();
            return !sidebarnew->object_list()->has_selected_cut_object() &&
                !view3D->get_canvas3d()->get_selection().is_empty() && (type == BuildVolume::Type::Rectangle || type == BuildVolume::Type::Circle);
        }

        bool Plater::priv::has_assemble_view() const
        {
            for (auto object : model.objects)
            {
                for (auto instance : object->instances)
                    if (instance->is_assemble_initialized())
                        return true;

                int part_cnt = 0;
                for (auto volume : object->volumes) {
                    if (volume->is_model_part())
                        part_cnt++;
                }

                if (part_cnt > 1)
                    return true;
            }
            return false;
        }

        bool Plater::priv::layers_height_allowed() const
        {
            if (printer_technology != ptFFF)
                return false;

            int obj_idx = get_selected_object_idx();
            return 0 <= obj_idx && obj_idx < (int)model.objects.size() && model.objects[obj_idx]->max_z() > SINKING_Z_THRESHOLD &&
                config->opt_bool("variable_layer_height") && view3D->is_layers_editing_allowed();
        }

        bool Plater::priv::can_mirror() const
        {
            //return !objectbar->has_selected_cut_object();
            return !sidebarnew->object_list()->has_selected_cut_object()
                && get_selection().is_from_single_instance();
        }


        bool Plater::priv::can_replace_with_stl() const
        {
            //return !objectbar->has_selected_cut_object() && get_selection().get_volume_idxs().size() == 1;
            return !sidebarnew->object_list()->has_selected_cut_object() && get_selection().get_volume_idxs().size() == 1;
        }

        bool Plater::priv::can_reload_from_disk() const
        {
            if (sidebarnew->object_list()->has_selected_cut_object())
                return false;

            // collect selected reloadable ModelVolumes
            std::vector<std::pair<int, int>> selected_volumes = reloadable_volumes(model, get_selection());
            // nothing to reload, return
            if (selected_volumes.empty())
                return false;

            std::sort(selected_volumes.begin(), selected_volumes.end(), [](const std::pair<int, int>& v1, const std::pair<int, int>& v2) {
                return (v1.first < v2.first) || (v1.first == v2.first && v1.second < v2.second);
                });
            selected_volumes.erase(std::unique(selected_volumes.begin(), selected_volumes.end(), [](const std::pair<int, int>& v1, const std::pair<int, int>& v2) {
                return (v1.first == v2.first) && (v1.second == v2.second); }), selected_volumes.end());

            // collects paths of files to load
            std::vector<fs::path> paths;
            for (auto [obj_idx, vol_idx] : selected_volumes) {
                paths.push_back(model.objects[obj_idx]->volumes[vol_idx]->source.input_file);
            }

            std::sort(paths.begin(), paths.end());
            paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

            return !paths.empty();
        }

        void Plater::priv::set_bed_shape(const Pointfs& shape, const double max_print_height, const std::string& custom_texture, const std::string& custom_model, bool force_as_custom)
        {
            bool new_shape = bed.set_shape(shape, max_print_height, custom_texture, custom_model, force_as_custom);
            if (new_shape) {
                if (view3D) view3D->bed_shape_changed();
                if (preview) preview->bed_shape_changed();
            }
        }

        bool Plater::priv::can_delete() const
        {
            return !get_selection().is_empty() && !get_selection().is_wipe_tower();
        }

        bool Plater::priv::can_delete_all() const
        {
            return !model.objects.empty();
        }

        bool Plater::priv::can_fix_through_netfabb() const
        {
            std::vector<int> obj_idxs, vol_idxs;
            sidebarnew->object_list()->get_selection_indexes(obj_idxs, vol_idxs);
            //objectbar->get_selection_indexes(obj_idxs, vol_idxs);

#if FIX_THROUGH_NETFABB_ALWAYS
            // Fixing always.
            return !obj_idxs.empty() || !vol_idxs.empty();
#else // FIX_THROUGH_NETFABB_ALWAYS
            // Fixing only if the model is not manifold.
            if (vol_idxs.empty()) {
                for (auto obj_idx : obj_idxs)
                    if (model.objects[obj_idx]->get_repaired_errors_count() > 0)
                        return true;
                return false;
            }

            int obj_idx = obj_idxs.front();
            for (auto vol_idx : vol_idxs)
                if (model.objects[obj_idx]->get_repaired_errors_count(vol_idx) > 0)
                    return true;
            return false;
#endif // FIX_THROUGH_NETFABB_ALWAYS
        }

        bool Plater::priv::can_simplify() const
        {
            const int obj_idx = get_selected_object_idx();
            // is object for simplification selected
            // cut object can't be simplify
            if (obj_idx < 0 || model.objects[obj_idx]->is_cut())
                return false;

            // is already opened?
            if (q->canvas3D()->get_gizmos_manager().get_current_type() ==
                GLGizmosManager::EType::Simplify)
                return false;
            return true;
        }

        bool Plater::priv::can_increase_instances() const
        {
            if (!m_worker.is_idle()
                || q->canvas3D()->get_gizmos_manager().is_in_editing_mode())
                return false;

            // Disallow arrange and add instance when emboss gizmo is opend 
            // Prevent strobo effect during editing emboss parameters.
            if (q->canvas3D()->get_gizmos_manager().get_current_type() == GLGizmosManager::Emboss) return false;

            const auto obj_idxs = get_selection().get_object_idxs();
            return !obj_idxs.empty() && !get_selection().is_wipe_tower() && !sidebarnew->object_list()->has_selected_cut_object();
        }

        bool Plater::priv::can_decrease_instances(int obj_idx /*= -1*/) const
        {
            if (!m_worker.is_idle()
                || q->canvas3D()->get_gizmos_manager().is_in_editing_mode())
                return false;

            if (obj_idx < 0)
                obj_idx = get_selected_object_idx();

            if (obj_idx < 0) {
                if (const auto obj_ids = get_selection().get_object_idxs(); !obj_ids.empty())
                    for (const size_t obj_id : obj_ids)
                        if (can_decrease_instances(obj_id))
                            return true;
                return false;
            }

            return  obj_idx < (int)model.objects.size() &&
                (model.objects[obj_idx]->instances.size() > 1) &&
                !sidebarnew->object_list()->has_selected_cut_object();
        }

        bool Plater::priv::can_split_to_objects() const
        {
            return q->can_split(true);
        }

        bool Plater::priv::can_split_to_volumes() const
        {
            return q->can_split(false);
        }

        bool Plater::priv::can_arrange() const
        {
            if (model.objects.empty() || !m_worker.is_idle()) return false;
            return q->canvas3D()->get_gizmos_manager().get_current_type() == GLGizmosManager::Undefined;
        }

        bool Plater::priv::can_auto_bed() const
        {
            if (model.objects.empty() || !m_worker.is_idle() || q->canvas3D()->get_selection().is_single_text()) return false;
            return true;
        }

        bool Plater::priv::can_layers_editing() const
        {
            return layers_height_allowed();
        }

        void Plater::priv::show_action_buttons(const bool ready_to_slice_) const
        {
            // Cache this value, so that the callbacks from the RemovableDriveManager may repeat that value when calling show_action_buttons().
            this->ready_to_slice = ready_to_slice_;

            //wxWindowUpdateLocker noUpdater(sidebar);

            DynamicPrintConfig* selected_printer_config = wxGetApp().preset_bundle->physical_printers.get_selected_printer_config();
            const auto print_host_opt = selected_printer_config ? selected_printer_config->option<ConfigOptionString>("print_host") : nullptr;
            const bool send_gcode_shown = false;//print_host_opt != nullptr && !print_host_opt->value.empty();   // Hide send_gcode_btn by aden update.

            // when a background processing is ON, export_btn and/or send_btn are showing
            //if (get_config_bool("background_processing"))
            //{
            //    RemovableDriveManager::RemovableDrivesStatus removable_media_status = wxGetApp().removable_drive_manager()->status();
            //    if (sidebar->show_reslice(false) |
            //        sidebar->show_export(true) |
            //        sidebar->show_print_btn(true) |
            //        sidebar->show_send(send_gcode_shown) |
            //        sidebar->show_export_removable(removable_media_status.has_removable_drives))
            //        //			sidebar->show_eject(removable_media_status.has_eject))
            //        sidebar->Layout();
            //}
            //else
            //{
            //    RemovableDriveManager::RemovableDrivesStatus removable_media_status;
            //    if (!ready_to_slice)
            //        removable_media_status = wxGetApp().removable_drive_manager()->status();
            //    if (sidebar->show_reslice(ready_to_slice) |
            //        sidebar->show_export(!ready_to_slice) |
            //        sidebar->show_print_btn(!ready_to_slice) |
            //        sidebar->show_send(send_gcode_shown && !ready_to_slice) |
            //        sidebar->show_export_removable(!ready_to_slice && removable_media_status.has_removable_drives))
            //        //            sidebar->show_eject(!ready_to_slice && removable_media_status.has_eject))
            //        sidebar->Layout();
            //}
        }

        void Plater::priv::enter_gizmos_stack()
        {
            assert(m_undo_redo_stack_active == &m_undo_redo_stack_main);
            if (m_undo_redo_stack_active == &m_undo_redo_stack_main) {
                m_undo_redo_stack_active = &m_undo_redo_stack_gizmos;
                assert(m_undo_redo_stack_active->empty());
                // Take the initial snapshot of the gizmos.
                // Not localized on purpose, the text will never be shown to the user.
                this->take_snapshot(std::string("Gizmos-Initial"));
            }
        }

        void Plater::priv::leave_gizmos_stack()
        {
            assert(m_undo_redo_stack_active == &m_undo_redo_stack_gizmos);
            if (m_undo_redo_stack_active == &m_undo_redo_stack_gizmos) {
                assert(!m_undo_redo_stack_active->empty());
                m_undo_redo_stack_active->clear();
                m_undo_redo_stack_active = &m_undo_redo_stack_main;
            }
        }

        int Plater::priv::get_active_snapshot_index()
        {
            const size_t active_snapshot_time = this->undo_redo_stack().active_snapshot_time();
            const std::vector<UndoRedo::Snapshot>& ss_stack = this->undo_redo_stack().snapshots();
            const auto it = std::lower_bound(ss_stack.begin(), ss_stack.end(), UndoRedo::Snapshot(active_snapshot_time));
            return it - ss_stack.begin();
        }

        void Plater::priv::take_snapshot(const std::string& snapshot_name, const UndoRedo::SnapshotType snapshot_type)
        {
            if (m_prevent_snapshots > 0)
                return;
            assert(m_prevent_snapshots >= 0);
            UndoRedo::SnapshotData snapshot_data;
            snapshot_data.snapshot_type = snapshot_type;
            snapshot_data.printer_technology = this->printer_technology;
            if (this->view3D->is_layers_editing_enabled())
                snapshot_data.flags |= UndoRedo::SnapshotData::VARIABLE_LAYER_EDITING_ACTIVE;
            if (this->sidebarnew->object_list()->is_selected(itSettings)) {
                snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_SETTINGS_ON_SIDEBAR;
                snapshot_data.layer_range_idx = this->sidebarnew->object_list()->get_selected_layers_range_idx();
            }
            else if (this->sidebarnew->object_list()->is_selected(itLayer)) {
                snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_LAYER_ON_SIDEBAR;
                snapshot_data.layer_range_idx = this->sidebarnew->object_list()->get_selected_layers_range_idx();
            }
            else if (this->sidebarnew->object_list()->is_selected(itLayerRoot))
                snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_LAYERROOT_ON_SIDEBAR;

            // If SLA gizmo is active, ask it if it wants to trigger support generation
            // on loading this snapshot.
            if (view3D->get_canvas3d()->get_gizmos_manager().wants_reslice_supports_on_undo())
                snapshot_data.flags |= UndoRedo::SnapshotData::RECALCULATE_SLA_SUPPORTS;

            //FIXME updating the Wipe tower config values at the ModelWipeTower from the Print config.
            // This is a workaround until we refactor the Wipe Tower position / orientation to live solely inside the Model, not in the Print config.
            if (this->printer_technology == ptFFF) {
                const DynamicPrintConfig& config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
                model.wipe_tower.position = Vec2d(config.opt_float("wipe_tower_x"), config.opt_float("wipe_tower_y"));
                model.wipe_tower.rotation = config.opt_float("wipe_tower_rotation_angle");
            }
#if 0
            const GLGizmosManager& gizmos = view3D->get_canvas3d()->get_gizmos_manager();
#else
            const GLGizmosManager& gizmos = get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_gizmos_manager() : view3D->get_canvas3d()->get_gizmos_manager();
#endif
            if (snapshot_type == UndoRedo::SnapshotType::ProjectSeparator && get_config_bool("clear_undo_redo_stack_on_new_project"))
                this->undo_redo_stack().clear();
#if 0
            this->undo_redo_stack().take_snapshot(snapshot_name, model, view3D->get_canvas3d()->get_selection(), gizmos, snapshot_data);
#else
            this->undo_redo_stack().take_snapshot(snapshot_name, model, get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_selection() : view3D->get_canvas3d()->get_selection(), gizmos, snapshot_data);
#endif
            if (snapshot_type == UndoRedo::SnapshotType::LeavingGizmoWithAction) {
                // Filter all but the last UndoRedo::SnapshotType::GizmoAction in a row between the last UndoRedo::SnapshotType::EnteringGizmo and UndoRedo::SnapshotType::LeavingGizmoWithAction.
                // The remaining snapshot will be renamed to a more generic name,
                // depending on what gizmo is being left.
                if (gizmos.get_current() != nullptr) {
                    std::string new_name = gizmos.get_current()->get_action_snapshot_name();
                    this->undo_redo_stack().reduce_noisy_snapshots(new_name);
                }
            }
            else if (snapshot_type == UndoRedo::SnapshotType::ProjectSeparator) {
                // Reset the "dirty project" flag.
                m_undo_redo_stack_main.mark_current_as_saved();
            }
            this->undo_redo_stack().release_least_recently_used();

            dirty_state.update_from_undo_redo_stack(m_undo_redo_stack_main.project_modified());

            // Save the last active preset name of a particular printer technology.
            ((this->printer_technology == ptFFF) ? m_last_fff_printer_profile_name : m_last_sla_printer_profile_name) = wxGetApp().preset_bundle->printers.get_selected_preset_name();
            BOOST_LOG_TRIVIAL(info) << "Undo / Redo snapshot taken: " << snapshot_name << ", Undo / Redo stack memory: " << Slic3r::format_memsize_MB(this->undo_redo_stack().memsize()) << log_memory_info();
        }

        void Plater::priv::undo()
        {
            const std::vector<UndoRedo::Snapshot>& snapshots = this->undo_redo_stack().snapshots();
            auto it_current = std::lower_bound(snapshots.begin(), snapshots.end(), UndoRedo::Snapshot(this->undo_redo_stack().active_snapshot_time()));
            if (--it_current == snapshots.begin()) return;

            if (get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView) {
                if (it_current->snapshot_data.snapshot_type != UndoRedo::SnapshotType::GizmoAction &&
                    it_current->snapshot_data.snapshot_type != UndoRedo::SnapshotType::EnteringGizmo &&
                    it_current->snapshot_data.snapshot_type != UndoRedo::SnapshotType::LeavingGizmoNoAction &&
                    it_current->snapshot_data.snapshot_type != UndoRedo::SnapshotType::LeavingGizmoWithAction)
                    return;
            }

            this->undo_redo_to(it_current);
            if(wxGetApp().plater()->model().objects.empty())
                sidebarnew->enableSliceBtn(false, false);
            else
                sidebarnew->enableSliceBtn(false, true);
        }

        void Plater::priv::redo()
        {
            const std::vector<UndoRedo::Snapshot>& snapshots = this->undo_redo_stack().snapshots();
            auto it_current = std::lower_bound(snapshots.begin(), snapshots.end(), UndoRedo::Snapshot(this->undo_redo_stack().active_snapshot_time()));
            if (++it_current != snapshots.end())
                this->undo_redo_to(it_current);

            if (wxGetApp().plater()->model().objects.empty())
                sidebarnew->enableSliceBtn(false, false);
            else
                sidebarnew->enableSliceBtn(false, true);
        }

        void Plater::priv::undo_redo_to(size_t time_to_load)
        {
            const std::vector<UndoRedo::Snapshot>& snapshots = this->undo_redo_stack().snapshots();
            auto it_current = std::lower_bound(snapshots.begin(), snapshots.end(), UndoRedo::Snapshot(time_to_load));
            assert(it_current != snapshots.end());
            this->undo_redo_to(it_current);
        }

        void Plater::priv::undo_redo_to(std::vector<UndoRedo::Snapshot>::const_iterator it_snapshot)
        {
            // Make sure that no updating function calls take_snapshot until we are done.
            SuppressSnapshots snapshot_supressor(q);

            bool 				temp_snapshot_was_taken = this->undo_redo_stack().temp_snapshot_active();
            PrinterTechnology 	new_printer_technology = it_snapshot->snapshot_data.printer_technology;
            bool 				printer_technology_changed = this->printer_technology != new_printer_technology;
            if (printer_technology_changed) {
                // Switching the printer technology when jumping forwards / backwards in time. Switch to the last active printer profile of the other type.
                std::string s_pt = (it_snapshot->snapshot_data.printer_technology == ptFFF) ? "FFF" : "SLA";
                if (!wxGetApp().check_and_save_current_preset_changes(_L("Undo / Redo is processing"),
                    //            format_wxstr(_L("%1% printer was active at the time the target Undo / Redo snapshot was taken. Switching to %1% printer requires reloading of %1% presets."), s_pt)))
                    format_wxstr(_L("Switching the printer technology from %1% to %2%.\n"
                        "Some %1% presets were modified, which will be lost after switching the printer technology."), s_pt == "FFF" ? "SLA" : "FFF", s_pt), false))
                    // Don't switch the profiles.
                    return;
            }
            // Save the last active preset name of a particular printer technology.
            ((this->printer_technology == ptFFF) ? m_last_fff_printer_profile_name : m_last_sla_printer_profile_name) = wxGetApp().preset_bundle->printers.get_selected_preset_name();
            //FIXME updating the Wipe tower config values at the ModelWipeTower from the Print config.
            // This is a workaround until we refactor the Wipe Tower position / orientation to live solely inside the Model, not in the Print config.
            if (this->printer_technology == ptFFF) {
                const DynamicPrintConfig& config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
                model.wipe_tower.position = Vec2d(config.opt_float("wipe_tower_x"), config.opt_float("wipe_tower_y"));
                model.wipe_tower.rotation = config.opt_float("wipe_tower_rotation_angle");
            }
            const int layer_range_idx = it_snapshot->snapshot_data.layer_range_idx;
            // Flags made of Snapshot::Flags enum values.
            unsigned int new_flags = it_snapshot->snapshot_data.flags;
            UndoRedo::SnapshotData top_snapshot_data;
            top_snapshot_data.printer_technology = this->printer_technology;
            if (this->view3D->is_layers_editing_enabled())
                top_snapshot_data.flags |= UndoRedo::SnapshotData::VARIABLE_LAYER_EDITING_ACTIVE;
            if (this->view3D->is_layers_editing_enabled())
                top_snapshot_data.flags |= UndoRedo::SnapshotData::VARIABLE_LAYER_EDITING_ACTIVE;
            if (this->sidebarnew->object_list()->is_selected(itSettings)) {
                top_snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_SETTINGS_ON_SIDEBAR;
                top_snapshot_data.layer_range_idx = this->sidebarnew->object_list()->get_selected_layers_range_idx();
            }
            else if (this->sidebarnew->object_list()->is_selected(itLayer)) {
                top_snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_LAYER_ON_SIDEBAR;
                top_snapshot_data.layer_range_idx = this->sidebarnew->object_list()->get_selected_layers_range_idx();
            }
            else if (this->sidebarnew->object_list()->is_selected(itLayerRoot))
                top_snapshot_data.flags |= UndoRedo::SnapshotData::SELECTED_LAYERROOT_ON_SIDEBAR;
            bool   		 new_variable_layer_editing_active = (new_flags & UndoRedo::SnapshotData::VARIABLE_LAYER_EDITING_ACTIVE) != 0;
            bool         new_selected_settings_on_sidebar = (new_flags & UndoRedo::SnapshotData::SELECTED_SETTINGS_ON_SIDEBAR) != 0;
            bool         new_selected_layer_on_sidebar = (new_flags & UndoRedo::SnapshotData::SELECTED_LAYER_ON_SIDEBAR) != 0;
            bool         new_selected_layerroot_on_sidebar = (new_flags & UndoRedo::SnapshotData::SELECTED_LAYERROOT_ON_SIDEBAR) != 0;

            if (this->view3D->get_canvas3d()->get_gizmos_manager().wants_reslice_supports_on_undo())
                top_snapshot_data.flags |= UndoRedo::SnapshotData::RECALCULATE_SLA_SUPPORTS;

            // Disable layer editing before the Undo / Redo jump.
            if (!new_variable_layer_editing_active && view3D->is_layers_editing_enabled())
                view3D->get_canvas3d()->force_main_toolbar_left_action(view3D->get_canvas3d()->get_main_toolbar_item_id("layersediting"));

            // Make a copy of the snapshot, undo/redo could invalidate the iterator
            const UndoRedo::Snapshot snapshot_copy = *it_snapshot;
            // Do the jump in time.
            if (it_snapshot->timestamp < this->undo_redo_stack().active_snapshot_time() ?
#if 0
                this->undo_redo_stack().undo(model, this->view3D->get_canvas3d()->get_selection(), this->view3D->get_canvas3d()->get_gizmos_manager(), top_snapshot_data, it_snapshot->timestamp) :
                this->undo_redo_stack().redo(model, this->view3D->get_canvas3d()->get_gizmos_manager(), it_snapshot->timestamp)) {
#else
                this->undo_redo_stack().undo(model, get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_selection() : this->view3D->get_canvas3d()->get_selection(), get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_gizmos_manager() : this->view3D->get_canvas3d()->get_gizmos_manager(), top_snapshot_data, it_snapshot->timestamp) :
                this->undo_redo_stack().redo(model, get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_gizmos_manager() : this->view3D->get_canvas3d()->get_gizmos_manager(), it_snapshot->timestamp)) {
#endif
                if (printer_technology_changed) {
                    // Switch to the other printer technology. Switch to the last printer active for that particular technology.
                    AppConfig* app_config = wxGetApp().app_config;
                    app_config->set("presets", "printer", (new_printer_technology == ptFFF) ? m_last_fff_printer_profile_name : m_last_sla_printer_profile_name);
                    //FIXME Why are we reloading the whole preset bundle here? Please document. This is fishy and it is unnecessarily expensive.
                    // Anyways, don't report any config value substitutions, they have been already reported to the user at application start up.
                    wxGetApp().preset_bundle->load_presets(*app_config, ForwardCompatibilitySubstitutionRule::EnableSilent);
                    // load_current_presets() calls Tab::load_current_preset() -> TabPrint::update() -> Object_list::update_and_show_object_settings_item(),
                    // but the Object list still keeps pointer to the old Model. Avoid a crash by removing selection first.
                    this->sidebarnew->object_list()->unselect_objects();
                    // Load the currently selected preset into the GUI, update the preset selection box.
                    // This also switches the printer technology based on the printer technology of the active printer profile.
                    wxGetApp().load_current_presets();
                }
                //FIXME updating the Print config from the Wipe tower config values at the ModelWipeTower.
                // This is a workaround until we refactor the Wipe Tower position / orientation to live solely inside the Model, not in the Print config.
                if (this->printer_technology == ptFFF) {
                    const DynamicPrintConfig& current_config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
                    Vec2d 					  current_position(current_config.opt_float("wipe_tower_x"), current_config.opt_float("wipe_tower_y"));
                    double 					  current_rotation = current_config.opt_float("wipe_tower_rotation_angle");
                    if (current_position != model.wipe_tower.position || current_rotation != model.wipe_tower.rotation) {
                        DynamicPrintConfig new_config;
                        new_config.set_key_value("wipe_tower_x", new ConfigOptionFloat(model.wipe_tower.position.x()));
                        new_config.set_key_value("wipe_tower_y", new ConfigOptionFloat(model.wipe_tower.position.y()));
                        new_config.set_key_value("wipe_tower_rotation_angle", new ConfigOptionFloat(model.wipe_tower.rotation));
#if SHOW_OLD_SETTING_DIALOG
                        Tab* tab_print = wxGetApp().get_tab(Preset::TYPE_PRINT);
#endif
                        AnkerTab* tab_print = wxGetApp().getAnkerTab(Preset::TYPE_PRINT);
                        tab_print->load_config(new_config);
                        tab_print->update_dirty();
                    }
                }
                // set selection mode for ObjectList on sidebar
                this->sidebarnew->object_list()->set_selection_mode(new_selected_settings_on_sidebar ? ObjectList::SELECTION_MODE::smSettings :
                    new_selected_layer_on_sidebar ? ObjectList::SELECTION_MODE::smLayer :
                    new_selected_layerroot_on_sidebar ? ObjectList::SELECTION_MODE::smLayerRoot :
                    ObjectList::SELECTION_MODE::smUndef);
                if (new_selected_settings_on_sidebar || new_selected_layer_on_sidebar)
                    this->sidebarnew->object_list()->set_selected_layers_range_idx(layer_range_idx);

                this->update_after_undo_redo(snapshot_copy, temp_snapshot_was_taken);
                // Enable layer editing after the Undo / Redo jump.
                if (!view3D->is_layers_editing_enabled() && this->layers_height_allowed() && new_variable_layer_editing_active)
                    view3D->get_canvas3d()->force_main_toolbar_left_action(view3D->get_canvas3d()->get_main_toolbar_item_id("layersediting"));
            }

            dirty_state.update_from_undo_redo_stack(m_undo_redo_stack_main.project_modified());
        }

        void Plater::priv::update_after_undo_redo(const UndoRedo::Snapshot& snapshot, bool /* temp_snapshot_was_taken */)
        {
#if 0
            this->view3D->get_canvas3d()->get_selection().clear();
#else
            get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ? assemble_view->get_canvas3d()->get_selection().clear() : this->view3D->get_canvas3d()->get_selection().clear();
#endif
            // Update volumes from the deserializd model, always stop / update the background processing (for both the SLA and FFF technologies).
            this->update((unsigned int)UpdateParams::FORCE_BACKGROUND_PROCESSING_UPDATE | (unsigned int)UpdateParams::POSTPONE_VALIDATION_ERROR_MESSAGE);
            // Release old snapshots if the memory allocated is excessive. This may remove the top most snapshot if jumping to the very first snapshot.
            //if (temp_snapshot_was_taken)
            // Release the old snapshots always, as it may have happened, that some of the triangle meshes got deserialized from the snapshot, while some
            // triangle meshes may have gotten released from the scene or the background processing, therefore now being calculated into the Undo / Redo stack size.
            this->undo_redo_stack().release_least_recently_used();
            //YS_FIXME update obj_list from the deserialized model (maybe store ObjectIDs into the tree?) (no selections at this point of time)
#if 0
            this->view3D->get_canvas3d()->get_selection().set_deserialized(GUI::Selection::EMode(this->undo_redo_stack().selection_deserialized().mode), this->undo_redo_stack().selection_deserialized().volumes_and_instances);
            this->view3D->get_canvas3d()->get_gizmos_manager().update_after_undo_redo(snapshot);
#else
            get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ?
                assemble_view->get_canvas3d()->get_selection().set_deserialized(GUI::Selection::EMode(this->undo_redo_stack().selection_deserialized().mode), this->undo_redo_stack().selection_deserialized().volumes_and_instances) :
                this->view3D->get_canvas3d()->get_selection().set_deserialized(GUI::Selection::EMode(this->undo_redo_stack().selection_deserialized().mode), this->undo_redo_stack().selection_deserialized().volumes_and_instances);
            get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasAssembleView ?
                assemble_view->get_canvas3d()->get_gizmos_manager().update_after_undo_redo(snapshot) :
                this->view3D->get_canvas3d()->get_gizmos_manager().update_after_undo_redo(snapshot);
#endif
            wxGetApp().obj_list()->update_after_undo_redo();

            if (wxGetApp().get_mode() == comSimple && model_has_advanced_features(this->model)) {
                // If the user jumped to a snapshot that require user interface with advanced features, switch to the advanced mode without asking.
                // There is a little risk of surprising the user, as he already must have had the advanced or expert mode active for such a snapshot to be taken.
                if (wxGetApp().save_mode(comAdvanced))
                    view3D->set_as_dirty();
            }

            // this->update() above was called with POSTPONE_VALIDATION_ERROR_MESSAGE, so that if an error message was generated when updating the back end, it would not open immediately, 
            // but it would be saved to be show later. Let's do it now. We do not want to display the message box earlier, because on Windows & OSX the message box takes over the message
            // queue pump, which in turn executes the rendering function before a full update after the Undo / Redo jump.
            this->show_delayed_error_message();

            //FIXME what about the state of the manipulators?
            //FIXME what about the focus? Cursor in the side panel?

            BOOST_LOG_TRIVIAL(info) << "Undo / Redo snapshot reloaded. Undo / Redo stack memory: " << Slic3r::format_memsize_MB(this->undo_redo_stack().memsize()) << log_memory_info();
        }

        void Plater::priv::bring_instance_forward() const
        {
#ifdef __APPLE__
            wxGetApp().other_instance_message_handler()->bring_instance_forward();
            return;
#endif //__APPLE__
            if (main_frame == nullptr) {
                BOOST_LOG_TRIVIAL(debug) << "Couldnt bring instance forward - mainframe is null";
                return;
            }
            BOOST_LOG_TRIVIAL(debug) << "studio window going forward";
            //this code maximize app window on Fedora
            {
                main_frame->Iconize(false);
                if (main_frame->IsMaximized())
                    main_frame->Maximize(true);
                else
                    main_frame->Maximize(false);
            }
            //this code maximize window on Ubuntu
            {
                main_frame->Restore();
                wxGetApp().GetTopWindow()->SetFocus();  // focus on my window
                wxGetApp().GetTopWindow()->Raise();  // bring window to front
                wxGetApp().GetTopWindow()->Show(true); // show the window
            }
        }
           
        void Plater::priv::_calib_pa_pattern(const Calib_Params& params)
        {
            m_flowCalibration->_calib_pa_pattern(params);
        }

        void Plater::priv::_calib_pa_tower(const Calib_Params& params)
        {
            m_flowCalibration->_calib_pa_tower(params);
        }
            
        void Plater::priv::calib_flowrate(int pass)
        {
            m_flowCalibration->calib_flowrate(pass);
        }
            
        void Plater::priv::calib_temp(const Calib_Params& params)
        {
            m_flowCalibration->calib_temp(params);
            background_process.fff_print()->set_calib_params(params);
        }
            
        void Plater::priv::calib_retraction(const Calib_Params& params)
        {
            m_flowCalibration->calib_retraction(params);
            background_process.fff_print()->set_calib_params(params);
        }
            
        void Plater::priv::calib_max_vol_speed(const Calib_Params& params)
        {
            Calib_Params new_params;
            m_flowCalibration->calib_max_vol_speed(params, new_params);
            background_process.fff_print()->set_calib_params(new_params);
        }
            
        void Plater::priv::calib_VFA(const Calib_Params& params)
        {
            m_flowCalibration->calib_VFA(params);
            background_process.fff_print()->set_calib_params(params);
        }
        
        void Sidebar::set_btn_label(const ActionButtonType btn_type, const wxString& label) const
        {
            switch (btn_type)
            {
            case ActionButtonType::abReslice:   p->btn_reslice->SetLabelText(label);        break;
            case ActionButtonType::abExport:    p->btn_export_gcode->SetLabelText(label);   break;
            case ActionButtonType::abSendGCode: /*p->btn_send_gcode->SetLabelText(label);*/     break;
            }
        }

        // Plater / Public

        Plater::Plater(wxWindow* parent, MainFrame* main_frame)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxGetApp().get_min_size())
            , p(new priv(this, main_frame))
        {
            // Initialization performed in the private c-tor
            Bind(wxCUSTOMEVT_SHOW_DEVICELIST_DIALOG, [this](wxCommandEvent& event) {
                bool hideList = false;
                int res = 0;
                wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
                if (pData) {                    
                    pData->GetData().ToInt(&res);
                    hideList = res == 0 ? false : true;
                }
                ANKER_LOG_INFO << "hideList: " << hideList << ", res: " << res;
                UpdateDeviceList(hideList);
            });

            Bind(wxEVT_SHOW, &Plater::on_show, this);
            Bind(wxEVT_IDLE, &Plater::on_idle, this);
        }

        Plater::~Plater()
        {
            Unbind(wxEVT_SHOW, &Plater::on_show, this);
            clear_acode_extract_path();
        }
        bool Plater::is_plater_dirty() const { return p->is_plater_dirty(); }
        bool Plater::is_project_dirty() const { return p->is_project_dirty(); }
        bool Plater::is_presets_dirty() const { return p->is_presets_dirty(); }
        void Plater::update_project_dirty_from_presets() { p->update_project_dirty_from_presets(); }
        int  Plater::save_project_if_dirty(const wxString& reason) { return p->save_project_if_dirty(reason); }
        void Plater::reset_project_dirty_after_save() { p->reset_project_dirty_after_save(); }
        void Plater::reset_project_dirty_initial_presets() { p->reset_project_dirty_initial_presets(); }
#if ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW
        void Plater::render_project_state_debug_window() const { p->render_project_state_debug_window(); }
#endif // ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW

        Sidebar& Plater::sidebar() { return *p->sidebar; }
        AnkerSidebarNew& Plater::sidebarnew() { return *p->sidebarnew; }
        AnkerObjectBar* Plater::objectbar() { return p->objectbar; }
        AnkerObjectManipulator* Plater::aobject_manipulator() { return p->aobjectmanipulator; }
        AnkerFloatingList* Plater::floatinglist() { return p->floatinglist; }
        AnkerObjectLayers* Plater::object_layers() { return p->object_layers; }
        const Model& Plater::model() const { return p->model; }
        Model& Plater::model() { return p->model; }
        const Print& Plater::fff_print() const { return p->fff_print; }
        Print& Plater::fff_print() { return p->fff_print; }
        const SLAPrint& Plater::sla_print() const { return p->sla_print; }
        SLAPrint& Plater::sla_print() { return p->sla_print; }

        bool Plater::is_project_temp() const
        {
            return false;
        }

        void Plater::request_model_download(std::string url)
        {
            if (p) {
                p->on_action_request_model_download(url);
            }
        }

        bool Plater::new_project(wxString project_name)
        {
            if (int saved_project = p->save_project_if_dirty(_L("Creating a new project while the current project is modified.")); saved_project == wxID_CANCEL)
                return false;
            else {
                wxString header = _L("Creating a new project while some presets are modified.") + "\n" +
                    (saved_project == wxID_YES ? _L("You can keep presets modifications to the new project or discard them") :
                        _L("You can keep presets modifications to the new project, discard them or save changes as new presets.\n"
                            "Note, if changes will be saved then new project wouldn't keep them"));
                int act_buttons = ActionButtons::KEEP;
                if (saved_project == wxID_NO)
                    act_buttons |= ActionButtons::SAVE;
                if (!wxGetApp().check_and_keep_current_preset_changes(_L("Creating a new project"), header, act_buttons))
                    return false;
            }

            p->select_view_3D(VIEW_MODE_3D);
            take_snapshot(_L("New Project"), UndoRedo::SnapshotType::ProjectSeparator);
            Plater::SuppressSnapshots suppress(this);
            //reset();
            p->reset(project_name.ToStdString()); // TODO: check if this line is useful
            // Save the names of active presets and project specific config into ProjectDirtyStateManager.
            reset_project_dirty_initial_presets();
            // Make a copy of the active presets for detecting changes in preset values.
            wxGetApp().update_saved_preset_from_current_preset();
            // Update Project dirty state, update application title bar.
            update_project_dirty_from_presets();
            return true;
        }

        void Plater::load_project()
        {
            if (!wxGetApp().can_load_project())
                return;

            // Ask user for a project file name.
            wxString input_file;
            wxGetApp().load_project(this, input_file);
            // And finally load the new project.
            load_project(input_file);
        }

        void Plater::load_project(const wxString& filename)
        {
            if (filename.empty())
                return;

            // Take the Undo / Redo snapshot.
            Plater::TakeSnapshot snapshot(this, _L("Load Project") + ": " + wxString::FromUTF8(into_path(filename).stem().string().c_str()), UndoRedo::SnapshotType::ProjectSeparator);

            p->reset();

            if (!load_files({ into_path(filename) }).empty()) {
                // At least one file was loaded.
                p->set_project_filename(filename);
                // Save the names of active presets and project specific config into ProjectDirtyStateManager.
                reset_project_dirty_initial_presets();
                // Make a copy of the active presets for detecting changes in preset values.
                wxGetApp().update_saved_preset_from_current_preset();
                // Update Project dirty state, update application title bar.
                update_project_dirty_from_presets();
            }
        }

        void Plater::add_model(bool imperial_units/* = false*/)
        {
            wxArrayString input_files;
            wxGetApp().import_model(this, input_files);
            
            //report: start import model on menu
            std::string handleType = "import";
            std::string model_name = std::string("");
            std::string modelSize = std::string("0");
            std::string errorCode = "0";
            std::string errorMsg = std::string("Import model from menu");
            wxString wxModelName = "";
            if (input_files.empty())
            {
                errorCode = "-1";
                errorMsg = "no model files";
                std::map<std::string, std::string> buryMap;
                buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                reportBuryEvent(e_hanlde_model, buryMap);
                return;
            }

            std::vector<fs::path> paths;
            for (const auto& file : input_files)
            {
                paths.emplace_back(into_path(file));                
            }
            auto GetFileSize = [](const wxString& filePath) -> wxULongLong {
                wxFileName fname(filePath);
                if (fname.FileExists()) {
                    wxFile file(filePath);
                    if (file.IsOpened()) {
                        wxULongLong fileSize = file.Length();
                        file.Close();
                        return fileSize;
                    }
                    else {
                        ANKER_LOG_ERROR <<("can't open the file: " + filePath);                        
                    }
                }
                else {
                    ANKER_LOG_ERROR << ("file not exist: %s", filePath);
                }
                return 0;
            };
            
            wxULongLong filesSize = 0;
            for (const wxString& filePath : input_files)
            {
                
                wxModelName += filePath.ToStdString() + " ";
                filesSize += GetFileSize(filePath);
            }
            modelSize = filesSize.ToString().ToStdString();
            model_name = wxModelName.ToUTF8().data();
            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_hm_type, handleType));
            buryMap.insert(std::make_pair(c_hm_file_name, model_name));            
            buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
            buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
            ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
            reportBuryEvent(e_hanlde_model, buryMap);

            wxString snapshot_label;
            assert(!paths.empty());
            if (paths.size() == 1) {
                snapshot_label = _L("Import Object");
                snapshot_label += ": ";
                snapshot_label += wxString::FromUTF8(paths.front().filename().string().c_str());
            }
            else {
                snapshot_label = _L("Import Objects");
                snapshot_label += ": ";
                snapshot_label += wxString::FromUTF8(paths.front().filename().string().c_str());
                for (size_t i = 1; i < paths.size(); ++i) {
                    snapshot_label += ", ";
                    snapshot_label += wxString::FromUTF8(paths[i].filename().string().c_str());
                }
            }

            Plater::TakeSnapshot snapshot(this, snapshot_label);
            if (!load_files(paths, true, false, imperial_units).empty())
            {
                wxGetApp().mainframe->update_title();
            }
        }

        void Plater::import_zip_archive()
        {
            wxString input_file;
            wxGetApp().import_zip(this, input_file);
            if (input_file.empty())
                return;

            fs::path path = into_path(input_file);
            preview_zip_archive(path);
        }

        void Plater::import_sl1_archive()
        {
            auto& w = get_ui_job_worker();
            if (w.is_idle() && p->m_sla_import_dlg->ShowModal() == wxID_OK) {
                p->take_snapshot(_L("Import SLA archive"));
                replace_job(w, std::make_unique<SLAImportJob>(p->m_sla_import_dlg));
            }
        }

        void Plater::cut_horizontal(size_t obj_idx, size_t instance_idx, double z, ModelObjectCutAttributes attributes)
        {
            wxCHECK_RET(obj_idx < p->model.objects.size(), "obj_idx out of bounds");
            auto* object = p->model.objects[obj_idx];

            wxCHECK_RET(instance_idx < object->instances.size(), "instance_idx out of bounds");

            if (!attributes.has(ModelObjectCutAttribute::KeepUpper) && !attributes.has(ModelObjectCutAttribute::KeepLower))
                return;

            wxBusyCursor wait;

            const Vec3d instance_offset = object->instances[instance_idx]->get_offset();
            const auto  new_objects = object->cut(instance_idx, Geometry::translation_transform(z * Vec3d::UnitZ() - instance_offset), attributes);

            apply_cut_object_to_model(obj_idx, new_objects);
        }

        void Plater::apply_cut_object_to_model(size_t obj_idx, const ModelObjectPtrs& new_objects)
        {
            model().delete_object(obj_idx);
            sidebarnew().object_list()->delete_object_from_list(obj_idx);

            // suppress to call selection update for Object List to avoid call of early Gizmos on/off update
            p->load_model_objects(new_objects, false, false);

            // now process all updates of the 3d scene
            update();
            // Update InfoItems in ObjectList after update() to use of a correct value of the GLCanvas3D::is_sinking(),
            // which is updated after a view3D->reload_scene(false, flags & (unsigned int)UpdateParams::FORCE_FULL_SCREEN_REFRESH) call
            for (size_t idx = 0; idx < p->model.objects.size(); idx++)
                wxGetApp().obj_list()->update_info_items(idx);

            Selection& selection = p->get_selection();
            size_t last_id = p->model.objects.size() - 1;
            for (size_t i = 0; i < new_objects.size(); ++i)
                selection.add_object((unsigned int)(last_id - i), i == 0);

            arrange();
        }

        void Plater::orient()
        {
            auto& w = get_ui_job_worker();
            if (w.is_idle()) {
                p->take_snapshot(_u8L("Orient"));
                replace_job(w, std::make_unique<OrientJob>(this));
            }
        }

        void Plater::calib_pa(const Calib_Params& params)
        {
            if (!this->new_project())
                return;
            wxGetApp().mainframe->select_tab(size_t(MainFrame::tp3DEditor));
            switch (params.mode) {
            case CalibMode::Calib_PA_Line: {
                Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
                std::vector<size_t> objs_idx = this->load_files(std::vector<std::string>{
                    (boost::filesystem::path(Slic3r::resources_dir() + "/calib/pressure_advance/pressure_advance_test.stl").string())}, true, false, false);
                break;
            }
            case CalibMode::Calib_PA_Pattern:
                _calib_pa_pattern(params);
                p->background_process.fff_print()->set_plates_custom_gcodes(p->model.plates_custom_gcodes[p->model.curr_plate_index]);
                break;
            case CalibMode::Calib_PA_Tower:
                _calib_pa_tower(params);
                break;
            default: break;
            }

            p->background_process.fff_print()->set_calib_params(params);
        }

        void Plater::_calib_pa_pattern(const Calib_Params& params) {
            m_calibModel = params.mode;
            p->_calib_pa_pattern(params);
        }

        void Plater::_calib_pa_tower(const Calib_Params& params) {
            m_calibModel = params.mode;
            p->_calib_pa_tower(params);
        }

        void Plater::_calib_pa_select_added_objects() {
            // update printable state for new volumes on canvas3D
            wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_objects({ 0 });

            Selection& selection = p->view3D->get_canvas3d()->get_selection();
            selection.clear();
            selection.add_object(0, false);

            // BBS: update object list selection
            //p->sidebar->obj_list()->update_selections();
            //selection.notify_instance_update(-1, -1);
            p->select_all();
            if (p->view3D->get_canvas3d()->get_gizmos_manager().is_enabled()) {
                // this is required because the selected object changed and the flatten on face an sla support gizmos need to be updated accordingly
                p->view3D->get_canvas3d()->update_gizmos_on_off_state();
            }
        }

        void Plater::calib_flowrate(int pass) {
            m_calibModel = CalibMode::Calib_Flow_Rate;
            p->calib_flowrate(pass);
        }

        void Plater::calib_temp(const Calib_Params& params) {
            m_calibModel = params.mode;
            p->calib_temp(params);
        }

        void Plater::calib_retraction(const Calib_Params& params)
        {
            m_calibModel = params.mode;
            p->calib_retraction(params);
        }

        void Plater::calib_max_vol_speed(const Calib_Params& params)
        {
            m_calibModel = params.mode;
            p->calib_max_vol_speed(params);
        }

        void Plater::calib_VFA(const Calib_Params& params)
        {
            m_calibModel = params.mode;
            p->calib_VFA(params);
        }

        void Plater::extract_config_from_project()
        {
            wxString input_file;
            wxGetApp().load_project(this, input_file);

            if (!input_file.empty())
                load_files({ into_path(input_file) }, false, true);
        }

        void Plater::load_gcode()
        {
            // Ask user for a gcode file name.
            wxString input_file;
            wxGetApp().load_gcode(this, input_file);
            // And finally load the gcode file.
            load_gcode(input_file);
        }

        void Plater::load_gcode(const wxString& filename)
        {
            if (!is_gcode_file(into_u8(filename)) || m_last_loaded_gcode == filename)
                return;

            m_last_loaded_gcode = filename;

            // cleanup view before to start loading/processing
            p->gcode_result.reset();
            reset_gcode_toolpaths();
            p->preview->reload_print(false);
            p->preview->UpdateGcodeInfo(filename.ToStdString());
            p->get_current_canvas3D()->render();

            wxBusyCursor wait;

            // process gcode
            GCodeProcessor processor;
            try
            {
                processor.process_file(filename.ToUTF8().data());
#ifdef DEBUG
                GCodeProcessorResultExt out;
                processor.process_file_ext(filename.ToUTF8().data(), out);
                wxImage image = Slic3r::GCodeThumbnails::base64ToImage<wxImage, wxMemoryInputStream>(out.base64_str);
                wxBitmap bitmap(image);
                wxFrame* frame = new wxFrame(NULL, wxID_ANY, wxT("Image from Base64"));
                wxStaticBitmap* staticBitmap = new wxStaticBitmap(frame, wxID_ANY, bitmap);
                frame->Show();
#endif // DEBUG
            }
            catch (const std::exception& ex)
            {
                show_error(this, ex.what());
                return;
            }

            p->gcode_result = std::move(processor.extract_result());

            if (!processor.isRecognizedProducer() && p->gcode_result.bed_shape.empty()) {
                p->gcode_result.bed_shape.push_back(Slic3r::Vec2d(0, 0));
                p->gcode_result.bed_shape.push_back(Slic3r::Vec2d(235, 0));
                p->gcode_result.bed_shape.push_back(Slic3r::Vec2d(235, 235));
                p->gcode_result.bed_shape.push_back(Slic3r::Vec2d(0, 235));
            }

            // show results
            try
            {
                p->preview->reload_print(false);
            }
            catch (const std::exception&)
            {
                wxEndBusyCursor();
                p->gcode_result.reset();
                reset_gcode_toolpaths();
                set_default_bed_shape();
                p->preview->reload_print(false);
                p->get_current_canvas3D()->render();
                MessageDialog(this, _L("The selected file") + ":\n" + filename + "\n" + _L("does not contain valid gcode."),
                    wxString(GCODEVIEWER_APP_NAME) + " - " + _L("Error while loading .gcode file"), wxOK | wxICON_WARNING | wxCENTRE).ShowModal();
                set_project_filename(wxEmptyString);
                return;
            }
            p->preview->get_canvas3d()->zoom_to_gcode();

            if (p->preview->get_canvas3d()->get_gcode_layers_zs().empty()) {
                wxEndBusyCursor();
                //wxMessageDialog(this, _L("The selected file") + ":\n" + filename + "\n" + _L("does not contain valid gcode."),
                MessageDialog(this, _L("The selected file") + ":\n" + filename + "\n" + _L("does not contain valid gcode."),
                    wxString(GCODEVIEWER_APP_NAME) + " - " + _L("Error while loading .gcode file"), wxOK | wxICON_WARNING | wxCENTRE).ShowModal();
                set_project_filename(wxEmptyString);
            }
        }

        void Plater::reload_gcode_from_disk()
        {
            wxString filename(m_last_loaded_gcode);
            m_last_loaded_gcode.clear();
            load_gcode(filename);
        }

        void Plater::setStarCommentFlagsTimes(const int& sliceTimes)
        {
            p->m_sliceTimes = sliceTimes;
        }

        void Plater::refresh_print()
        {
            p->preview->refresh_print();
        }

        std::vector<size_t> Plater::load_files(const std::vector<fs::path>& input_files, bool load_model, bool load_config, bool imperial_units /*= false*/, bool calibration)
        { 
            // notify sidebarnew to show the filament colour saved in project file
            if (p->sidebarnew) {
                size_t input_files_size = input_files.size();
                for (size_t i = 0; i < input_files_size; ++i) {
                    auto path = input_files[i];
                    std::string filePath = path.string();
                    if (filePath.find(".3mf") != std::string::npos) {
                        p->sidebarnew->setLoadProjectFileFlag(true);
                        break;
                    }
                }
            }

            std::vector<size_t> partSize = p->load_files(input_files, load_model, load_config, imperial_units);

            if (partSize.size() > 0)
                p->sidebarnew->enableSliceBtn(false, true);
            // 			if (!canvas3D()->get_gizmos_manager().is_in_editing_mode(false) &&is_project_dirty())
            //                 p->sidebarnew->enableSliceBtn(true,  true);

            m_input_model_from_calibration = calibration;
            return partSize;
        }

        // To be called when providing a list of files to the GUI slic3r on command line.
        std::vector<size_t> Plater::load_files(const std::vector<std::string>& input_files, bool load_model, bool load_config, bool imperial_units /*= false*/, bool calibration)
        {
            std::vector<fs::path> paths;
            paths.reserve(input_files.size());
            for (const std::string& path : input_files)
                paths.emplace_back(path);

            m_input_model_from_calibration = calibration;
            return p->load_files(paths, load_model, load_config, imperial_units);
        }


        class LoadProjectsDialog : public DPIDialog
        {
            int m_action{ 0 };
            bool m_all{ false };
            wxComboBox* m_combo_project{ nullptr };
            wxComboBox* m_combo_config{ nullptr };
        public:
            enum class LoadProjectOption : unsigned char
            {
                Unknown,
                AllGeometry,
                AllNewWindow,
                OneProject,
                OneConfig
            };

            LoadProjectsDialog(const std::vector<fs::path>& paths);

            int get_action() const { return m_action + 1; }
            bool get_all() const { return m_all; }
            int get_selected() const
            {
                if (m_combo_project && m_combo_project->IsEnabled())
                    return m_combo_project->GetSelection();
                else if (m_combo_config && m_combo_config->IsEnabled())
                    return m_combo_config->GetSelection();
                else
                    return -1;
            }
        protected:
            void on_dpi_changed(const wxRect& suggested_rect) override;
        };

        LoadProjectsDialog::LoadProjectsDialog(const std::vector<fs::path>& paths)
            : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY,
                format_wxstr(_L("%1% - Multiple projects file"), SLIC3R_APP_NAME), wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        {
            SetFont(wxGetApp().normal_font());

            wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
            bool contains_projects = !paths.empty();
            bool instances_allowed = !wxGetApp().app_config->get_bool("single_instance");
            if (contains_projects)
                main_sizer->Add(new wxStaticText(this, wxID_ANY,
                    get_wraped_wxString(_L("There are several files being loaded, including Project files.") + "\n" + _L("Select an action to apply to all files."))), 0, wxEXPAND | wxALL, 10);
            else
                main_sizer->Add(new wxStaticText(this, wxID_ANY,
                    get_wraped_wxString(_L("There are several files being loaded.") + "\n" + _L("Select an action to apply to all files."))), 0, wxEXPAND | wxALL, 10);

            wxStaticBox* action_stb = new wxStaticBox(this, wxID_ANY, _L("Action"));
            if (!wxOSX) action_stb->SetBackgroundStyle(wxBG_STYLE_PAINT);
            action_stb->SetFont(wxGetApp().normal_font());

            if (contains_projects) {
                wxArrayString filenames;
                for (const fs::path& path : paths) {
                    filenames.push_back(from_u8(path.filename().string()));
                }
                m_combo_project = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, filenames, wxCB_READONLY);
                m_combo_project->SetValue(filenames.front());
                m_combo_project->Enable(false);

                m_combo_config = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, filenames, wxCB_READONLY);
                m_combo_config->SetValue(filenames.front());
                m_combo_config->Enable(false);
            }
            wxStaticBoxSizer* stb_sizer = new wxStaticBoxSizer(action_stb, wxVERTICAL);
            int id = 0;

            // all geometry
            wxRadioButton* btn = new wxRadioButton(this, wxID_ANY, _L("Import geometry"), wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
            btn->SetValue(id == m_action);
            btn->Bind(wxEVT_RADIOBUTTON, [this, id, contains_projects](wxCommandEvent&) {
                m_action = id;
                if (contains_projects) {
                    m_combo_project->Enable(false);
                    m_combo_config->Enable(false);
                }
                });
            stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
            id++;
            // all new window
            if (instances_allowed) {
                btn = new wxRadioButton(this, wxID_ANY, _L("Start new eufyMake Studio instance"), wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
                btn->SetValue(id == m_action);
                btn->Bind(wxEVT_RADIOBUTTON, [this, id, contains_projects](wxCommandEvent&) {
                    m_action = id;
                    if (contains_projects) {
                        m_combo_project->Enable(false);
                        m_combo_config->Enable(false);
                    }
                    });
                stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
            }
            id++; // IMPORTANT TO ALWAYS UP THE ID EVEN IF OPTION IS NOT ADDED!
            if (contains_projects) {
                // one project
                btn = new wxRadioButton(this, wxID_ANY, _L("Select one to load as project"), wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
                btn->SetValue(false);
                btn->Bind(wxEVT_RADIOBUTTON, [this, id](wxCommandEvent&) {
                    m_action = id;
                    m_combo_project->Enable(true);
                    m_combo_config->Enable(false);
                    });
                stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
                stb_sizer->Add(m_combo_project, 0, wxEXPAND | wxTOP, 5);
                // one config
                id++;
                btn = new wxRadioButton(this, wxID_ANY, _L("Select one to load config only"), wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
                btn->SetValue(id == m_action);
                btn->Bind(wxEVT_RADIOBUTTON, [this, id, instances_allowed](wxCommandEvent&) {
                    m_action = id;
                    if (instances_allowed)
                        m_combo_project->Enable(false);
                    m_combo_config->Enable(true);
                    });
                stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
                stb_sizer->Add(m_combo_config, 0, wxEXPAND | wxTOP, 5);
            }


            main_sizer->Add(stb_sizer, 1, wxEXPAND | wxRIGHT | wxLEFT, 10);
            wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
            bottom_sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT, 5);
            main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxALL, 10);
            SetSizer(main_sizer);
            main_sizer->SetSizeHints(this);

            // Update DarkUi just for buttons
            wxGetApp().UpdateDlgDarkUI(this, true);
        }

        void LoadProjectsDialog::on_dpi_changed(const wxRect& suggested_rect)
        {
            const int em = em_unit();
            SetMinSize(wxSize(65 * em, 30 * em));
            Fit();
            Refresh();
        }




        bool Plater::preview_zip_archive(const boost::filesystem::path& archive_path)
        {
            //std::vector<fs::path> unzipped_paths;
            std::vector<fs::path> non_project_paths;
            std::vector<fs::path> project_paths;
            try
            {
                mz_zip_archive archive;
                mz_zip_zero_struct(&archive);

                if (!open_zip_reader(&archive, archive_path.string())) {
                    std::string err_msg = GUI::format(_u8L("Loading of a zip archive on path %1% has failed."), archive_path.string());
                    throw Slic3r::FileIOError(err_msg);
                }
                mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
                mz_zip_archive_file_stat stat;
                // selected_paths contains paths and its uncompressed size. The size is used to distinguish between files with same path.
                std::vector<std::pair<fs::path, size_t>> selected_paths;
                FileArchiveDialog dlg(static_cast<wxWindow*>(wxGetApp().mainframe), &archive, selected_paths);
                if (dlg.ShowModal() == wxID_OK)
                {
                    std::string archive_path_string = archive_path.string();
                    archive_path_string = archive_path_string.substr(0, archive_path_string.size() - 4);
                    fs::path archive_dir(wxStandardPaths::Get().GetTempDir().utf8_str().data());

                    for (auto& path_w_size : selected_paths) {
                        const fs::path& path = path_w_size.first;
                        size_t size = path_w_size.second;
                        // find path in zip archive
                        for (mz_uint i = 0; i < num_entries; ++i) {
                            if (mz_zip_reader_file_stat(&archive, i, &stat)) {
                                if (size != stat.m_uncomp_size) // size must fit
                                    continue;
                                wxString wname = boost::nowide::widen(stat.m_filename);
                                std::string name = boost::nowide::narrow(wname);
                                fs::path archive_path(name);

                                std::string extra(1024, 0);
                                size_t extra_size = mz_zip_reader_get_filename_from_extra(&archive, i, extra.data(), extra.size());
                                if (extra_size > 0) {
                                    archive_path = fs::path(extra.substr(0, extra_size));
                                    name = archive_path.string();
                                }

                                if (archive_path.empty())
                                    continue;
                                if (path != archive_path)
                                    continue;
                                // decompressing
                                try
                                {
                                    std::replace(name.begin(), name.end(), '\\', '/');
                                    // rename if file exists
                                    std::string filename = path.filename().string();
                                    std::string extension = boost::filesystem::extension(path);
                                    std::string just_filename = filename.substr(0, filename.size() - extension.size());
                                    std::string final_filename = just_filename;

                                    size_t version = 0;
                                    while (fs::exists(archive_dir / (final_filename + extension)))
                                    {
                                        ++version;
                                        final_filename = just_filename + "(" + std::to_string(version) + ")";
                                    }
                                    filename = final_filename + extension;
                                    fs::path final_path = archive_dir / filename;
                                    std::string buffer((size_t)stat.m_uncomp_size, 0);
                                    // Decompress action. We already has correct file index in stat structure. 
                                    mz_bool res = mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
                                    if (res == 0) {
                                        wxString error_log = GUI::format_wxstr(_L("Failed to unzip file to %1%: %2% "), final_path.string(), mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
                                        BOOST_LOG_TRIVIAL(error) << error_log;
                                        show_error(nullptr, error_log);
                                        break;
                                    }
                                    // write buffer to file
                                    fs::fstream file(final_path, std::ios::out | std::ios::binary | std::ios::trunc);
                                    file.write(buffer.c_str(), buffer.size());
                                    file.close();
                                    if (!fs::exists(final_path)) {
                                        wxString error_log = GUI::format_wxstr(_L("Failed to find unzipped file at %1%. Unzipping of file has failed."), final_path.string());
                                        BOOST_LOG_TRIVIAL(error) << error_log;
                                        show_error(nullptr, error_log);
                                        break;
                                    }
                                    BOOST_LOG_TRIVIAL(info) << "Unzipped " << final_path;
                                    if (!boost::algorithm::iends_with(filename, ".3mf") && !boost::algorithm::iends_with(filename, ".amf")) {
                                        non_project_paths.emplace_back(final_path);
                                        break;
                                    }
                                    // if 3mf - read archive headers to find project file
                                    if ((boost::algorithm::iends_with(filename, ".3mf") && !is_project_3mf(final_path.string())) ||
                                        (boost::algorithm::iends_with(filename, ".amf") && !boost::algorithm::iends_with(filename, ".zip.amf"))) {
                                        non_project_paths.emplace_back(final_path);
                                        break;
                                    }

                                    project_paths.emplace_back(final_path);
                                    break;
                                }
                                catch (const std::exception& e)
                                {
                                    // ensure the zip archive is closed and rethrow the exception
                                    close_zip_reader(&archive);
                                    throw Slic3r::FileIOError(e.what());
                                }
                            }
                        }
                    }
                    close_zip_reader(&archive);
                    if (non_project_paths.size() + project_paths.size() != selected_paths.size())
                        BOOST_LOG_TRIVIAL(error) << "Decompresing of archive did not retrieve all files. Expected files: "
                        << selected_paths.size()
                        << " Decopressed files: "
                        << non_project_paths.size() + project_paths.size();
                }
                else {
                    close_zip_reader(&archive);
                    return false;
                }

            }
            catch (const Slic3r::FileIOError& e) {
                // zip reader should be already closed or not even opened
                GUI::show_error(this, e.what());
                return false;
            }
            // none selected
            if (project_paths.empty() && non_project_paths.empty())
            {
                return false;
            }
#if 0
            // 1 project, 0 models - behave like drag n drop
            if (project_paths.size() == 1 && non_project_paths.empty())
            {
                wxArrayString aux;
                aux.Add(from_u8(project_paths.front().string()));
                load_files(aux);
                //load_files(project_paths, true, true);
                boost::system::error_code ec;
                fs::remove(project_paths.front(), ec);
                if (ec)
                    BOOST_LOG_TRIVIAL(error) << ec.message();
                return true;
            }
            // 1 model (or more and other instances are not allowed), 0 projects - open geometry
            if (project_paths.empty() && (non_project_paths.size() == 1 || wxGetApp().app_config->get_bool("single_instance")))
            {
                load_files(non_project_paths, true, false);
                boost::system::error_code ec;
                fs::remove(non_project_paths.front(), ec);
                if (ec)
                    BOOST_LOG_TRIVIAL(error) << ec.message();
                return true;
            }

            bool delete_after = true;

            LoadProjectsDialog dlg(project_paths);
            if (dlg.ShowModal() == wxID_OK) {
                LoadProjectsDialog::LoadProjectOption option = static_cast<LoadProjectsDialog::LoadProjectOption>(dlg.get_action());
                switch (option)
                {
                case LoadProjectsDialog::LoadProjectOption::AllGeometry: {
                    load_files(project_paths, true, false);
                    load_files(non_project_paths, true, false);
                    break;
                }
                case LoadProjectsDialog::LoadProjectOption::AllNewWindow: {
                    delete_after = false;
                    for (const fs::path& path : project_paths) {
                        wxString f = from_path(path);
                        start_new_slicer(&f, false);
                    }
                    for (const fs::path& path : non_project_paths) {
                        wxString f = from_path(path);
                        start_new_slicer(&f, false);
                    }
                    break;
                }
                case LoadProjectsDialog::LoadProjectOption::OneProject: {
                    int pos = dlg.get_selected();
                    assert(pos >= 0 && pos < project_paths.size());
                    if (wxGetApp().can_load_project())
                        load_project(from_path(project_paths[pos]));
                    project_paths.erase(project_paths.begin() + pos);
                    load_files(project_paths, true, false);
                    load_files(non_project_paths, true, false);
                    break;
                }
                case LoadProjectsDialog::LoadProjectOption::OneConfig: {
                    int pos = dlg.get_selected();
                    assert(pos >= 0 && pos < project_paths.size());
                    std::vector<fs::path> aux;
                    aux.push_back(project_paths[pos]);
                    load_files(aux, false, true);
                    project_paths.erase(project_paths.begin() + pos);
                    load_files(project_paths, true, false);
                    load_files(non_project_paths, true, false);
                    break;
                }
                case LoadProjectsDialog::LoadProjectOption::Unknown:
                default:
                    assert(false);
                    break;
                }
            }

            if (!delete_after)
                return true;
#else 
            // 1 project file and some models - behave like drag n drop of 3mf and then load models
            if (project_paths.size() == 1)
            {
                wxArrayString aux;
                aux.Add(from_u8(project_paths.front().string()));
                bool loaded3mf = load_files(aux, true);
                load_files(non_project_paths, true, false);
                boost::system::error_code ec;
                if (loaded3mf) {
                    fs::remove(project_paths.front(), ec);
                    if (ec)
                        BOOST_LOG_TRIVIAL(error) << ec.message();
                }
                for (const fs::path& path : non_project_paths) {
                    // Delete file from temp file (path variable), it will stay only in app memory.
                    boost::system::error_code ec;
                    fs::remove(path, ec);
                    if (ec)
                        BOOST_LOG_TRIVIAL(error) << ec.message();
                }
                return true;
            }

            // load all projects and all models as geometry
            load_files(project_paths, true, false);
            load_files(non_project_paths, true, false);
#endif // 0


            for (const fs::path& path : project_paths) {
                // Delete file from temp file (path variable), it will stay only in app memory.
                boost::system::error_code ec;
                fs::remove(path, ec);
                if (ec)
                    BOOST_LOG_TRIVIAL(error) << ec.message();
            }
            for (const fs::path& path : non_project_paths) {
                // Delete file from temp file (path variable), it will stay only in app memory.
                boost::system::error_code ec;
                fs::remove(path, ec);
                if (ec)
                    BOOST_LOG_TRIVIAL(error) << ec.message();
            }

            return true;
        }

        class ProjectDropDialog : public DPIDialog
        {
            int m_action{ 0 };
        public:
            enum class LoadType : unsigned char
            {
                Unknown,
                OpenProject,
                LoadGeometry,
                LoadConfig,
                OpenWindow
            };
            ProjectDropDialog(const std::string& filename);

            int get_action() const { return m_action + 1; }

        protected:
            void on_dpi_changed(const wxRect& suggested_rect) override;
        };

        ProjectDropDialog::ProjectDropDialog(const std::string& filename)
            : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY,
                format_wxstr("%1% - %2%", SLIC3R_APP_NAME, _L("Load project file")), wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        {
            SetFont(wxGetApp().normal_font());

            bool single_instance_only = wxGetApp().app_config->get_bool("single_instance");
            wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
            wxArrayString choices;
            choices.reserve(4);
            choices.Add(_L("Open as project"));
            choices.Add(_L("Import geometry only"));
            choices.Add(_L("Import config only"));
            if (!single_instance_only)
                choices.Add(_L("Start new eufyMake Studio instance"));

            main_sizer->Add(new wxStaticText(this, wxID_ANY,
                get_wraped_wxString(_L("Select an action to apply to the file") + ": " + from_u8(filename))), 0, wxEXPAND | wxALL, 10);

            m_action = std::clamp(std::stoi(wxGetApp().app_config->get("drop_project_action")),
                static_cast<int>(LoadType::OpenProject), single_instance_only ? static_cast<int>(LoadType::LoadConfig) : static_cast<int>(LoadType::OpenWindow)) - 1;

            wxStaticBox* action_stb = new wxStaticBox(this, wxID_ANY, _L("Action"));
            if (!wxOSX) action_stb->SetBackgroundStyle(wxBG_STYLE_PAINT);
            action_stb->SetFont(wxGetApp().normal_font());

            wxStaticBoxSizer* stb_sizer = new wxStaticBoxSizer(action_stb, wxVERTICAL);
            int id = 0;
            for (const wxString& label : choices) {
                wxRadioButton* btn = new wxRadioButton(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
                btn->SetValue(id == m_action);
                btn->Bind(wxEVT_RADIOBUTTON, [this, id](wxCommandEvent&) { m_action = id; });
                stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
                id++;
            }
            main_sizer->Add(stb_sizer, 1, wxEXPAND | wxRIGHT | wxLEFT, 10);

            wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
            wxCheckBox* check = new wxCheckBox(this, wxID_ANY, _L("Don't show again"));
            check->Bind(wxEVT_CHECKBOX, [](wxCommandEvent& evt) {
                wxGetApp().app_config->set("show_drop_project_dialog", evt.IsChecked() ? "0" : "1");
                });

            bottom_sizer->Add(check, 0, wxEXPAND | wxRIGHT, 5);
            bottom_sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT, 5);
            main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxALL, 10);

            SetSizer(main_sizer);
            main_sizer->SetSizeHints(this);

            // Update DarkUi just for buttons
            wxGetApp().UpdateDlgDarkUI(this, true);
        }

        void ProjectDropDialog::on_dpi_changed(const wxRect& suggested_rect)
        {
            const int em = em_unit();
            SetMinSize(wxSize(65 * em, 30 * em));
            Fit();
            Refresh();
        }

        bool Plater::isImportGCode() const
        {
            if (p->preview) {
                return p->preview->is_GcodeImported();
            }
            return false;
        }

        bool Plater::ImportIsACode() const
        {
            if (p->preview) {
                return p->preview->ImportIsACode();
            }
            return false;
        }

        std::string Plater::get_acode_extract_path()
        {
            boost::filesystem::path temp_path(wxStandardPaths::Get().GetTempDir().utf8_str().data());
            temp_path /= "acode_extract";
            return temp_path.string();
        }

        void Plater::clear_acode_extract_path()
        {
            fs::path directory(get_acode_extract_path());
            if (fs::exists(directory) && fs::is_directory(directory)) {
                for (auto& file : fs::directory_iterator(directory)) {
                    try {
                        fs::remove_all(file);
                    }
                    catch (const std::exception& e) {
                        ANKER_LOG_ERROR << "Error removing file: " << file.path().string() << " - " << e.what() ;
                    }
                }
            }
        }

        // extract pattern file from .tar to the outputdir
        bool Plater::ExtractFilesFromTar(const wxString& tarFilePath, const wxString& outputDir, std::string file_pattern_regex) {
            bool ret = false;

            wxFileInputStream tarFile(tarFilePath);
            wxTarInputStream tarStream(tarFile);

            const std::regex pattern_file(file_pattern_regex, std::regex::icase);
            if (tarFile.IsOk() && tarStream.IsOk()) {
                wxTarEntry* entry = nullptr;
                while ((entry = tarStream.GetNextEntry()) != nullptr) {
                    wxString entryName = entry->GetName();

                    if (std::regex_match(entryName.ToStdString(wxConvUTF8), pattern_file))
                    {
                        wxString outputPath = wxFileName(outputDir, entryName).GetFullPath();

                        if (entry->IsDir()) {
                            wxFileName::Mkdir(outputPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
                        }
                        else {
                            wxFileOutputStream outputFile(outputPath);
                            if (outputFile.IsOk()) {
                                outputFile.Write(tarStream);
                                ret = true;
                            }
                            else {
                                ANKER_LOG_ERROR << "Error creating output file: " << outputPath;
                            }
                        }
                    }
                    delete entry;
                }
            }
            else {
                ANKER_LOG_ERROR<<"Error opening tar file.";
            }

            return ret;
        }

        // extract aiGcode.gcode file from .acode(.tar) file
        wxString Plater::extract_aiGcode_file_from_tar(const wxString& tarFilePath)
        {
            auto RenameFile = [&](const wxString& filePath, const wxString& newFileName)->bool {
                wxFileName fileName(filePath);
                if (fileName.FileExists()) {
                    wxString newPath = fileName.GetPath() + wxFILE_SEP_PATH + newFileName;
                    wxRenameFile(filePath, newPath);
                    return true;
                    /*
                    if (wxRenameFile(filePath, newPath)) {
                        return true;
                    }
                    else {
                        ANKER_LOG_ERROR<<"Error renaming file:"<< filePath;
                        return true;
                    }
                    */
                }
                else {
                    ANKER_LOG_ERROR<<"File does not exist:"<< filePath;
                    return false;
                }
            };

            auto DeleteFileIfExist = [&](const wxString& filePath)->bool {
                wxFileName fileName(filePath);
                if (fileName.FileExists()) {
                    if (wxRemoveFile(filePath)) 
                        return true;                    
                    else 
                        return false;
                }
                return true;
            };

            auto GetTimestampString = []() -> std::string {
                auto now = std::chrono::system_clock::now();
                auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                return std::to_string(timestamp);
            };

            wxString aiGcodePath;
            std::string acode_extract_path_str = get_acode_extract_path();
            boost::filesystem::path acode_extract_path(acode_extract_path_str);
            if (!boost::filesystem::exists(boost::filesystem::path(acode_extract_path))) {
                boost::filesystem::create_directory(acode_extract_path);
            }

            std::string aiGcodeFile = "aiGcode.gcode";
            aiGcodePath = wxFileName(acode_extract_path_str, aiGcodeFile).GetFullPath();

            if (ExtractFilesFromTar(tarFilePath, acode_extract_path_str, aiGcodeFile)) {
                wxFileName tarFile(tarFilePath);
                wxString tarFileName = tarFile.GetFullName();
                std::string currTimeStamp = GetTimestampString();
                wxString aiGcodeFileNewName = tarFileName + "_" + currTimeStamp + "_" + aiGcodeFile;

                if (RenameFile(aiGcodePath, aiGcodeFileNewName))
                    return wxFileName(acode_extract_path_str, aiGcodeFileNewName).GetFullPath();
            }

            return "";
        }

        bool Plater::HideSideBarNewForAcodeOrGcode(const wxString& filename) {
            //If the current plater doesn't existed Model,hide sidebarnew.
            if (!wxGetApp().plater()->is_plater_dirty()) {
                //Quickly hide and layout the rightParamPanel
                wxGetApp().plater()->sidebarnew().Hide();
                wxGetApp().plater()->Layout();
                m_isShowPrint.store(true);
                return true;
            }

            //If the current plater existed Model,popup a dialog to hint the customer; 
            if (wxGetApp().plater()->new_project() == false) {
                return false;
            } else {
                //Quickly hide and layout the rightParamPanel
                wxGetApp().plater()->sidebarnew().Hide();
                wxGetApp().plater()->Layout();
                m_isShowPrint.store(true);
                return true;
            }
        }


        bool Plater::load_files(const wxArrayString& filenames, bool delete_after_load/*=false*/)
        {
            const std::regex pattern_drop(".*[.](stl|obj|amf|3mf|akpro|step|stp|zip|svg)", std::regex::icase);
            const std::regex pattern_gcode_drop(".*[.](gcode|g|acode)", std::regex::icase);

            std::vector<fs::path> paths;

            //report import gocde/acode
            std::string handleFileName = std::string();
            std::string fileSize = std::string("0");
            std::string handleType = std::string("import");
            std::string handleDuration = std::string("0");
            auto startTime = wxDateTime::Now();
            std::string errorCode = std::string("0");
            std::string errorMsg = std::string("start to import ");
            wxString files = "";

            for (const auto& filename : filenames)
                files += filename + " ";
            handleFileName = files.ToUTF8().data();
            errorMsg += handleFileName;

            // gcode viewer section
            if (wxGetApp().is_gcode_viewer()) {
                for (const auto& filename : filenames) {
                    fs::path path(into_path(filename));
                    if (std::regex_match(path.string(), pattern_gcode_drop))
                        paths.push_back(std::move(path));
                }

                if (paths.size() > 1) {
                    //wxMessageDialog(static_cast<wxWindow*>(this), _L("You can open only one .gcode file at a time."),
                    MessageDialog(static_cast<wxWindow*>(this), _L("You can open only one .gcode file at a time."),
                        wxString(SLIC3R_APP_NAME) + " - " + _L("Drag and drop G-code file"), wxCLOSE | wxICON_WARNING | wxCENTRE).ShowModal();
                    return false;
                }
                else if (paths.size() == 1) {
                    ANKER_LOG_INFO << "Import path: " << paths.front();
#ifdef _WIN32
                    wxString strPath = wxString::FromUTF8(paths.front().string());
#elif __APPLE__
                    wxString strPath = wxString(paths.front().string());
#endif
                    wxString gcodePath;
                    if (strPath.EndsWith(".acode")) {
                        wxString aiGcodePath = extract_aiGcode_file_from_tar(strPath);
                        if (!aiGcodePath.empty())
                            gcodePath = aiGcodePath;
                        else {
                            ANKER_LOG_ERROR << "extract aiGcode.gcode from failed! .acode(.tar) = " << strPath;

                            errorCode = "-1";
                            errorMsg = "extract aiGcode.gcode from failed! .acode(.tar)";
                            std::map<std::string, std::string> buryMap;
                            buryMap.insert(std::make_pair(c_ag_file_name, handleFileName));
                            buryMap.insert(std::make_pair(c_ag_file_size, fileSize));
                            buryMap.insert(std::make_pair(c_ag_handle_type, handleType));
                            buryMap.insert(std::make_pair(c_ag_handle_duration, "0"));
                            buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                            buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                            ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                            reportBuryEvent(e_ag_handle, buryMap);

                            return false;
                        }
                        this->p->preview->setImportIsACode(true);
                    }
                    else {
                        gcodePath = strPath;
                        this->p->preview->setImportIsACode(false);
                    }

                    bool flag = HideSideBarNewForAcodeOrGcode(gcodePath);
                    if (!flag) { return false; }
                    else { select_view_3D(VIEW_MODE_PREVIEW); }

                    load_gcode(gcodePath);
                    m_last_loaded_gcode = strPath;
                    setAKeyPrintSlicerTempGcodePath(gcodePath.ToStdString(wxConvUTF8));

                    if (p->previewRightSidePanel) {
                        //p->previewRightSidePanel->UpdateGcodePreviewSideBar(true, strPath.EndsWith(".acode") ? LOAD_ACODE_FILE_FOR_PREVIEW : LOAD_GCODE_FILE_FOR_PREVIEW);
                        p->sidebarnew->updatePreviewBtn(true, strPath.EndsWith(".acode") ? LOAD_ACODE_FILE_FOR_PREVIEW : LOAD_GCODE_FILE_FOR_PREVIEW);
                        this->p->get_current_canvas3D()->zoom_to_bed();
                        ANKER_LOG_INFO << "UpdateGcodePreviewSideBar.";
                    }

                    std::map<std::string, std::string> buryMap;
                    buryMap.insert(std::make_pair(c_ag_file_name, handleFileName));
                    buryMap.insert(std::make_pair(c_ag_file_size, fileSize));
                    buryMap.insert(std::make_pair(c_ag_handle_type, handleType));
                    buryMap.insert(std::make_pair(c_ag_handle_duration, "0"));
                    buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                    buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                    ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                    reportBuryEvent(e_ag_handle, buryMap);
                    return true;
                }
                return false;
            }

            // editor section drop to load gcode/acode
            for (const auto& filename : filenames) {
                
                fs::path path(into_path(filename));
                if (std::regex_match(path.string(), pattern_drop))
                    paths.push_back(std::move(path));
                else if (std::regex_match(path.string(), pattern_gcode_drop)) {
                    if (0) {
                        start_new_gcodeviewer(&filename);
                    }
                    else {  
                        ANKER_LOG_INFO << "Import gcode/acode: " << path.string();
                        this->p->preview->setGCodeImportType(true); // Set import gcode type.

#ifdef _WIN32
                        wxString strPath = wxString::FromUTF8(path.string());
#elif __APPLE__
                        wxString strPath = wxString(path.string());
#endif
                        wxString gcodePath;

                        if (strPath.EndsWith(".acode")) {
                            wxString aiGcodePath = extract_aiGcode_file_from_tar(strPath);
                            if (!aiGcodePath.empty())
                                gcodePath = aiGcodePath;
                            else {
                                ANKER_LOG_ERROR << "extract aiGcode.gcode from failed! .acode(.tar) = " << strPath;
                                errorCode = -1;
                                errorMsg = "extract aiGcode.gcode from failed! .acode(.tar) = " + strPath.ToStdString();

                                std::map<std::string, std::string> buryMap;
                                buryMap.insert(std::make_pair(c_ag_file_name, handleFileName));
                                buryMap.insert(std::make_pair(c_ag_file_size, fileSize));
                                buryMap.insert(std::make_pair(c_ag_handle_type, handleType));
                                buryMap.insert(std::make_pair(c_ag_handle_duration, "0"));
                                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                                reportBuryEvent(e_ag_handle, buryMap);

                                return false;
                            }
                            this->p->preview->setImportIsACode(true);
                        }
                        else {
                            gcodePath = strPath;
                            this->p->preview->setImportIsACode(false);
                        }

                        setAKeyPrintSlicerTempGcodePath(gcodePath.ToStdString(wxConvUTF8));

                        //select_view_3D(VIEW_MODE_PREVIEW);
                        wxGetApp().set_app_mode(GUI_App::EAppMode::GCodeViewer);

                        //// When rightclick to open gcode,we also should hide the rightParamPanel
                        //wxGetApp().plater()->sidebarnew().Hide();
                        //wxGetApp().plater()->Layout();

                        bool flag = HideSideBarNewForAcodeOrGcode(filename);
                        if (!flag) { return false; }
                        else { select_view_3D(VIEW_MODE_PREVIEW); }

                        load_gcode(gcodePath);
                        m_last_loaded_gcode = strPath;

                       if (p->sidebarnew) {
                            //p->previewRightSidePanel->UpdateGcodePreviewSideBar(true, strPath.EndsWith(".acode") ? LOAD_ACODE_FILE_FOR_PREVIEW : LOAD_GCODE_FILE_FOR_PREVIEW);
                            p->sidebarnew->updatePreviewBtn(true, strPath.EndsWith(".acode") ? LOAD_ACODE_FILE_FOR_PREVIEW : LOAD_GCODE_FILE_FOR_PREVIEW);
                       }
                        this->p->get_current_canvas3D()->zoom_to_bed();
                        ANKER_LOG_INFO << "Import acode/gcode end.";

                        auto nowTime = wxDateTime::Now();
                        auto timeDifference = nowTime - startTime;
                        handleDuration = timeDifference.GetValue().ToString().ToStdString();
                    }
                }
                else
                    continue;
            }
            if (paths.empty())
            {
                //add by alves import gcode/acode end.
                // Likely all paths processed were gcodes, for which a G-code viewer instance has hopefully been started.
                std::map<std::string, std::string> buryMap;
                buryMap.insert(std::make_pair(c_ag_file_name, handleFileName));
                buryMap.insert(std::make_pair(c_ag_file_size, fileSize));
                buryMap.insert(std::make_pair(c_ag_handle_type, handleType));
                buryMap.insert(std::make_pair(c_ag_handle_duration, handleDuration));
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);
                
                return false;
            }

            // searches for project files  drop to start import 3mf and the other files
            for (std::vector<fs::path>::const_reverse_iterator it = paths.rbegin(); it != paths.rend(); ++it) {
                std::string filename = (*it).filename().string();
                if (boost::algorithm::iends_with(filename, ".3mf") || boost::algorithm::iends_with(filename, ".amf")) {
                    ProjectDropDialog::LoadType load_type = ProjectDropDialog::LoadType::Unknown;
                    {
                        if ((boost::algorithm::iends_with(filename, ".3mf") && !is_project_3mf(it->string())) ||
                            (boost::algorithm::iends_with(filename, ".amf") && !boost::algorithm::iends_with(filename, ".zip.amf")))
                            load_type = ProjectDropDialog::LoadType::LoadGeometry;
                        else {
                            if (wxGetApp().app_config->get_bool("show_drop_project_dialog")) {
                                ProjectDropDialog dlg(filename);
                                if (dlg.ShowModal() == wxID_OK) {
                                    int choice = dlg.get_action();
                                    load_type = static_cast<ProjectDropDialog::LoadType>(choice);
                                    wxGetApp().app_config->set("drop_project_action", std::to_string(choice));
                                }
                            }
                            else
                                load_type = static_cast<ProjectDropDialog::LoadType>(std::clamp(std::stoi(wxGetApp().app_config->get("drop_project_action")),
                                    static_cast<int>(ProjectDropDialog::LoadType::OpenProject), static_cast<int>(ProjectDropDialog::LoadType::LoadConfig)));
                        }
                    }

                    if (load_type == ProjectDropDialog::LoadType::Unknown)
                        return false;

                    switch (load_type) {
                    case ProjectDropDialog::LoadType::OpenProject: {
                        if (wxGetApp().can_load_project())
                            load_project(from_path(*it));
                        break;
                    }
                    case ProjectDropDialog::LoadType::LoadGeometry: {
                        //                Plater::TakeSnapshot snapshot(this, _L("Import Object"));
                        load_files({ *it }, true, false);
                        break;
                    }
                    case ProjectDropDialog::LoadType::LoadConfig: {
                        load_files({ *it }, false, true);
                        break;
                    }
                    case ProjectDropDialog::LoadType::OpenWindow: {
                        wxString f = from_path(*it);
                        start_new_slicer(&f, false, delete_after_load);
                        errorCode = "-1";
                        errorMsg = "did not load anything to this instance";

                        std::map<std::string, std::string> buryMap;
                        buryMap.insert(std::make_pair(c_hm_file_name, filename));                        
                        buryMap.insert(std::make_pair(c_hm_type, handleType));
                        buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                        buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                        ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                        reportBuryEvent(e_hanlde_model, buryMap);
                        return false; // did not load anything to this instance
                    }
                    case ProjectDropDialog::LoadType::Unknown: {
                        assert(false);
                        errorCode = "-1";
                        errorMsg = "unknwon model";
                        break;
                    }
                    }

                    std::map<std::string, std::string> buryMap;
                    buryMap.insert(std::make_pair(c_hm_file_name, filename));                    
                    buryMap.insert(std::make_pair(c_hm_type, handleType));
                    buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                    buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                    ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                    reportBuryEvent(e_hanlde_model, buryMap);
                    return true;
                }
                else if (boost::algorithm::iends_with(filename, ".zip")) {
                    return preview_zip_archive(*it);

                }
            }

            // other files
            wxString snapshot_label;
            assert(!paths.empty());
            if (paths.size() == 1) {
                snapshot_label = _L("Load File");
                snapshot_label += ": ";
                snapshot_label += wxString::FromUTF8(paths.front().filename().string().c_str());
            }
            else {
                snapshot_label = _L("Load Files");
                snapshot_label += ": ";
                snapshot_label += wxString::FromUTF8(paths.front().filename().string().c_str());
                for (size_t i = 1; i < paths.size(); ++i) {
                    snapshot_label += ", ";
                    snapshot_label += wxString::FromUTF8(paths[i].filename().string().c_str());
                }
            }
            Plater::TakeSnapshot snapshot(this, snapshot_label);
            load_files(paths);

            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_hm_file_name, handleFileName));
            buryMap.insert(std::make_pair(c_hm_type, handleType));
            buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
            buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
            ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
            reportBuryEvent(e_hanlde_model, buryMap);

            return true;
        }

        void Plater::update() { p->update(); }

        void Plater::update(int flag) { p->update(flag); }

        Worker& Plater::get_ui_job_worker() { return p->m_worker; }

        const Worker& Plater::get_ui_job_worker() const { return p->m_worker; }

        void Plater::update_ui_from_settings() { p->update_ui_from_settings(); }

        void Plater::select_view(const std::string& direction) { p->select_view(direction); }

        void Plater::select_view_3D(ViewMode mode) { p->select_view_3D(mode); }

        bool Plater::is_preview_shown() const { return p->is_preview_shown(); }
        bool Plater::is_preview_loaded() const { return p->is_preview_loaded(); }
        bool Plater::is_view3D_shown() const { return p->is_view3D_shown(); }


        bool Plater::are_view3D_labels_shown() const { return p->are_view3D_labels_shown(); }
        void Plater::show_view3D_labels(bool show) { p->show_view3D_labels(show); }

        bool Plater::is_legend_shown() const { return p->is_legend_shown(); }
        void Plater::show_legend(bool show) { p->show_legend(show); }
        wxWindow* Plater::get_preview_toolbar() { return p->get_preview_toolbar(); }
        void Plater::enable_moves_slider(bool enable) { return p->enable_moves_slider(enable); }

        bool Plater::is_sidebar_collapsed() const { return p->is_sidebar_collapsed(); }
        void Plater::collapse_sidebar(bool show) { p->collapse_sidebar(show); }

        bool Plater::is_view3D_layers_editing_enabled() const { return p->is_view3D_layers_editing_enabled(); }

        void Plater::select_all() { p->select_all(); }
        void Plater::deselect_all() { p->deselect_all(); }
        int Plater::get_object_count() { return p->get_object_count(); }
        int Plater::get_printable_object_count()
        {
            int cnt = 0;
            auto objects = Slic3r::GUI::wxGetApp().model().objects;
            for (const auto& object : objects) {
                for (const auto& instance : object->instances)
                {
                    if (instance->printable)
                        ++cnt;
                }
            }
            return cnt;
        }

        void Plater::remove(size_t obj_idx) { p->remove(obj_idx); }
        void Plater::reset() { p->reset(); }
        void Plater::reset_with_confirm()
        {
            if (p->model.objects.empty() ||
                //wxMessageDialog(static_cast<wxWindow*>(this), _L("All objects will be removed, continue?"), wxString(SLIC3R_APP_NAME) + " - " + _L("Delete all"), wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxCENTRE).ShowModal() == wxID_YES)
                MessageDialog(static_cast<wxWindow*>(this), _L("All objects will be removed, continue?"), wxString(SLIC3R_APP_NAME) + " - " + _L("Delete all"), wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxCENTRE).ShowModal() == wxID_YES)
                reset();
        }

        bool Plater::delete_object_from_model(size_t obj_idx) { return p->delete_object_from_model(obj_idx); }

        void Plater::remove_selected()
        {
            if (p->get_selection().is_empty())
                return;

            if (!p->can_delete())
                return;

            Plater::TakeSnapshot snapshot(this, _L("Delete Selected Objects"));
            get_ui_job_worker().cancel_all();
            //p->view3D->delete_selected();
            p->get_current_canvas3D()->delete_selected();
            p->sidebarnew->GetModelParameterPanel()->resetModelConfig();
            if (wxGetApp().plater()->model().objects.empty())
            {
                //p->sidebarnew->enableSliceBtn(true, false);
                p->sidebarnew->enableSliceBtn(false, false);
            }
        }

        void Plater::increase_instances(size_t num, int obj_idx/* = -1*/)
        {
            if (!can_increase_instances()) { return; }

            Plater::TakeSnapshot snapshot(this, _L("Increase Instances"));

            if (obj_idx < 0)
                obj_idx = p->get_selected_object_idx();

            if (obj_idx < 0) {
                if (const auto obj_idxs = get_selection().get_object_idxs(); !obj_idxs.empty())
                    for (const size_t obj_id : obj_idxs)
                        increase_instances(1, int(obj_id));
                return;
            }

            ModelObject* model_object = p->model.objects[obj_idx];
            ModelInstance* model_instance = model_object->instances.back();

            bool was_one_instance = model_object->instances.size() == 1;

            double offset_base = canvas3D()->get_size_proportional_to_max_bed_size(0.05);
            double offset = offset_base;
            for (size_t i = 0; i < num; i++, offset += offset_base) {
                Vec3d offset_vec = model_instance->get_offset() + Vec3d(offset, offset, 0.0);
                model_object->add_instance(offset_vec, model_instance->get_scaling_factor(), model_instance->get_rotation(), model_instance->get_mirror());
                //        p->print.get_object(obj_idx)->add_copy(Slic3r::to_2d(offset_vec));
                if (auto instance = model_object->instances[i]) {
                    auto transform = instance->get_transformation();
                    instance->set_assemble_transformation(transform);
                }
            }

            if (p->get_config_bool("autocenter"))
                arrange();

            p->update();

            p->get_selection().add_instance(obj_idx, (int)model_object->instances.size() - 1);

            sidebarnew().object_list()->increase_object_instances(obj_idx, was_one_instance ? num + 1 : num);

            p->selection_changed();
            this->p->schedule_background_process();
        }

        void Plater::decrease_instances(size_t num, int obj_idx/* = -1*/)
        {
            if (!can_decrease_instances(obj_idx)) { return; }

            Plater::TakeSnapshot snapshot(this, _L("Decrease Instances"));

            if (obj_idx < 0)
                obj_idx = p->get_selected_object_idx();

            if (obj_idx < 0) {
                if (const auto obj_ids = get_selection().get_object_idxs(); !obj_ids.empty())
                    for (const size_t obj_id : obj_ids)
                        decrease_instances(1, int(obj_id));
                return;
            }

            ModelObject* model_object = p->model.objects[obj_idx];
            if (model_object->instances.size() > num) {
                for (size_t i = 0; i < num; ++i)
                    model_object->delete_last_instance();
                p->update();
                // Delete object from Sidebar list. Do it after update, so that the GLScene selection is updated with the modified model.
                sidebarnew().object_list()->decrease_object_instances(obj_idx, num);
            }
            else {
                remove(obj_idx);
            }

            if (!model_object->instances.empty())
                p->get_selection().add_instance(obj_idx, (int)model_object->instances.size() - 1);

            p->selection_changed();
            this->p->schedule_background_process();
        }

        static long GetNumberFromUser(const wxString& msg,
            const wxString& prompt,
            const wxString& title,
            long value,
            long min,
            long max,
            wxWindow* parent)
        {

           /* wxNumberEntryDialog dialog(parent, msg, prompt, title, value, min, max, wxDefaultPosition);
            wxGetApp().UpdateDlgDarkUI(&dialog);*/
            AnkerNumberEnterDialog dialog(parent, title.ToStdString(wxConvUTF8), prompt, min, max, value);
            dialog.CenterOnParent();
            if (dialog.ShowModal() == wxID_OK)
                return dialog.GetValue();

            return -1;
        }

        void Plater::set_number_of_copies()
        {
            const auto obj_idxs = get_selection().get_object_idxs();
            if (obj_idxs.empty())
                return;

            const size_t init_cnt = obj_idxs.size() == 1 ? p->model.objects[*obj_idxs.begin()]->instances.size() : 1;
            const int num = GetNumberFromUser(" ", _L("Number of the selected object:"),
                _L("common_popup_setnumberofinstances_title"), init_cnt, 0, 1000, this);
            if (num < 0)
                return;
            TakeSnapshot snapshot(this, wxString::Format(_L("Set numbers of copies to %d"), num));

            for (const auto obj_idx : obj_idxs) {
                ModelObject* model_object = p->model.objects[obj_idx];
                const int diff = num - (int)model_object->instances.size();
                if (diff > 0)
                    increase_instances(diff, int(obj_idx));
                else if (diff < 0)
                    decrease_instances(-diff, int(obj_idx));
            }
        }

        void Plater::fill_bed_with_instances()
        {
            auto& w = get_ui_job_worker();
            if (w.is_idle()) {
                p->take_snapshot(_L("Fill bed"));
                replace_job(w, std::make_unique<FillBedJob>());
            }
        }

        bool Plater::is_selection_empty() const
        {
            return p->get_selection().is_empty() || p->get_selection().is_wipe_tower();
        }

        void Plater::scale_selection_to_fit_print_volume()
        {
            p->scale_selection_to_fit_print_volume();
        }

        void Plater::convert_unit(ConversionType conv_type)
        {
            std::vector<int> obj_idxs, volume_idxs;
            wxGetApp().obj_list()->get_selection_indexes(obj_idxs, volume_idxs);
            if (obj_idxs.empty() && volume_idxs.empty())
                return;

            // We will remove object indexes after convertion 
            // So, resort object indexes descending to avoid the crash after remove 
            std::sort(obj_idxs.begin(), obj_idxs.end(), std::greater<int>());

            TakeSnapshot snapshot(this, conv_type == ConversionType::CONV_FROM_INCH ? _L("Convert from imperial units") :
                conv_type == ConversionType::CONV_TO_INCH ? _L("Revert conversion from imperial units") :
                conv_type == ConversionType::CONV_FROM_METER ? _L("Convert from meters") : _L("Revert conversion from meters"));
            wxBusyCursor wait;

            ModelObjectPtrs objects;
            for (int obj_idx : obj_idxs) {
                ModelObject* object = p->model.objects[obj_idx];
                object->convert_units(objects, conv_type, volume_idxs);
                remove(obj_idx);
            }
            p->load_model_objects(objects);

            Selection& selection = p->view3D->get_canvas3d()->get_selection();
            size_t last_obj_idx = p->model.objects.size() - 1;

            if (volume_idxs.empty()) {
                for (size_t i = 0; i < objects.size(); ++i)
                    selection.add_object((unsigned int)(last_obj_idx - i), i == 0);
            }
            else {
                for (int vol_idx : volume_idxs)
                    selection.add_volume(last_obj_idx, vol_idx, 0, false);
            }
        }

        void Plater::toggle_layers_editing(bool enable)
        {
            if (canvas3D()->is_layers_editing_enabled() != enable)
                canvas3D()->force_main_toolbar_left_action(canvas3D()->get_main_toolbar_item_id("layersediting"));
        }

        void Plater::set_selected_visible(bool visible)
        {
            if (p->get_curr_selection().is_empty())
                return;

            Plater::TakeSnapshot snapshot(this, _L("Set Selected Objects Visible in AssembleView"));
            p->m_worker.cancel_all();

            p->get_current_canvas3D()->set_selected_visible(visible);
        }

        void Plater::cut(size_t obj_idx, size_t instance_idx, const Transform3d& cut_matrix, ModelObjectCutAttributes attributes)
        {
            wxCHECK_RET(obj_idx < p->model.objects.size(), "obj_idx out of bounds");
            auto* object = p->model.objects[obj_idx];

            wxCHECK_RET(instance_idx < object->instances.size(), "instance_idx out of bounds");

            wxBusyCursor wait;

            const auto new_objects = object->cut(instance_idx, cut_matrix, attributes);

            remove(obj_idx);

            // suppress to call selection update for Object List to avoid call of early Gizmos on/off update
            p->load_model_objects(new_objects, false, false);

            // now process all updates of the 3d scene
            update();
            // Update InfoItems in ObjectList after update() to use of a correct value of the GLCanvas3D::is_sinking(),
            // which is updated after a view3D->reload_scene(false, flags & (unsigned int)UpdateParams::FORCE_FULL_SCREEN_REFRESH) call
            //for (size_t idx = 0; idx < p->model.objects.size(); idx++)
            //    wxGetApp().obj_list()->update_info_items(idx);

            Selection& selection = p->get_selection();
            size_t last_id = p->model.objects.size() - 1;
            for (size_t i = 0; i < new_objects.size(); ++i)
                selection.add_object((unsigned int)(last_id - i), i == 0);

            p->get_current_canvas3D()->get_wxglcanvas()->SetFocus();
        }

        bool copyFile(const wxString& sourcePath, const wxString& destinationPath)
        {
            wxFile sourceFile(sourcePath, wxFile::read);
            if (!sourceFile.IsOpened()) {
                ANKER_LOG_ERROR << "Failed to open source file: " << sourcePath.ToStdString(wxConvUTF8);
                return false;
            }

            wxFile destinationFile(destinationPath, wxFile::write);
            if (!destinationFile.IsOpened())
            {
                ANKER_LOG_ERROR << "Failed to open destination file: " << destinationPath.ToStdString(wxConvUTF8);
                return false;
            }

            const size_t bufferSize = 1024 * 4;
            char buffer[bufferSize];

            wxFileOffset fileSize = sourceFile.Length();
            wxFileOffset bytesCopied = 0;

            while (bytesCopied < fileSize) {
                size_t bytesRead = sourceFile.Read(buffer, bufferSize);
                if (bytesRead > 0) {
                    destinationFile.Write(buffer, bytesRead);
                    bytesCopied += bytesRead;
                }
                else {
                    ANKER_LOG_ERROR << "Failed to read from source file: " << sourcePath.ToStdString(wxConvUTF8);
                    sourceFile.Close();
                    destinationFile.Close();
                    return false;
                }
            }
            sourceFile.Close();
            destinationFile.Close();
            return true;
        }

        bool Plater::ExportGacode()
        {
            auto ankerNet = AnkerNetInst();
            if (!ankerNet) {
                return false;
            }

            ANKER_LOG_INFO << std::string("get_last_loaded_gcode: ") + get_last_loaded_gcode().ToStdString(wxConvUTF8);
            if (this->p->preview->is_GcodeImported()) {
                m_currentPrintGcodeFile = get_last_loaded_gcode().ToStdString(wxConvUTF8);
            }
            else {
                m_currentPrintGcodeFile = "";
                export_akeyPrint_gcode(m_currentPrintGcodeFile);
                if (is_cancel_exporting_Gcode()) {
                    ANKER_LOG_INFO << "exporting gcode is cancel";
                    return false;
                }
            }
            
            std::thread::id tid = std::this_thread::get_id();
            std::hash<std::thread::id> hasher;
            int id = hasher(tid);
            ANKER_LOG_INFO << "Plater::showDeviceList thread id: " + std::to_string(id);
            ModelObjectPtrs modelObjects = model().objects;
            ANKER_LOG_INFO << "m_currentPrintGcodeFile: " + m_currentPrintGcodeFile;
                       
            wxString fileName = "";
            if (this->p->preview->is_GcodeImported()) {
                wxString tempGCodeFullName = get_last_loaded_gcode();

                int index = std::string::npos;
#ifdef _WIN32
                index = tempGCodeFullName.find_last_of("\\");
#elif __APPLE__
                index = tempGCodeFullName.find_last_of("/");
#endif
                if (index == std::string::npos) {
                    ANKER_LOG_ERROR << "Not found gcode: " << get_last_loaded_gcode().ToStdString(wxConvUTF8);
                    return false;
                }
                index++;
                fileName = tempGCodeFullName.substr(index, tempGCodeFullName.length() - index);
                //wxString inputFile = wxString::FromUTF8((*modelObjects.begin())->input_file);
                int index1 = m_currentPrintGcodeFile.find_last_of(".") + 1;
                if (index1 < 0 || index1 > m_currentPrintGcodeFile.length()) {
                    ANKER_LOG_ERROR << "m_currentPrintGcodeFile is error, index1: " << index1;
                    return false;
                }
                std::string suffix = m_currentPrintGcodeFile.substr(index1, m_currentPrintGcodeFile.length() - index1);

                int index2 = fileName.find_last_of(".") + 1;
                if (index2 < 0 || index2 > fileName.length()) {
                    ANKER_LOG_ERROR << "m_currentPrintGcodeFile is error, index2: " << index2;
                    return false;
                }
                wxString gCodeName = fileName.substr(0, index2 - 1);
#ifdef _WIN32
                int index3 = m_currentPrintGcodeFile.find_last_of("\\") + 2;
#elif __APPLE__
                int index3 = m_currentPrintGcodeFile.find_last_of("/") + 2;
#endif
                std::string destPath = m_currentPrintGcodeFile.substr(0, index3 - 1);
                wxString wdestPath = wxString::FromUTF8(destPath);
                wdestPath  += gCodeName + wxString("_bak") + wxString(".") + wxString::FromUTF8(suffix) ;
                bool cpRet = copyFile(wxString::FromUTF8(m_currentPrintGcodeFile), wdestPath);
                if (!cpRet) {
                    ANKER_LOG_ERROR << "CopyFile error! m_currentPrintGcodeFile: " << m_currentPrintGcodeFile;
                    return false;
                }
#ifdef _WIN32
                ankerNet->SetGcodePath(wdestPath.ToStdString(wxConvUTF8));
#elif __APPLE__
                ankerNet->SetGcodePath(wdestPath.ToStdString(wxConvUTF8));
#endif // _WIN32
            }
            else {
                ankerNet->SetGcodePath(m_currentPrintGcodeFile);
            }
            ANKER_LOG_INFO << "m_aKeyPrintGcodePath: " + ankerNet->GetGcodePath();

            return true;
        }

        void Plater::UpdateDeviceList(bool hideList)
        {
            if (m_chooseDeviceDialog) {
                if (hideList) {
                    m_chooseDeviceDialog->SetLoadingVisible(false);
                    m_chooseDeviceDialog->EndModal(wxID_CLOSE);
                    return;
                }

                m_chooseDeviceDialog->UpdateGui();
                m_chooseDeviceDialog->SetLoadingVisible(false);
            }
        }

        void Plater::ShowDeviceList()
        {
            p->m_isPrint = true;
            wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
            wxSize mfSize = wxGetApp().mainframe->GetClientSize();
            wxSize dialogSize = AnkerSize(400, 370);
            wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, 
                mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
            m_chooseDeviceDialog = new AnkerChooseDeviceDialog(nullptr, wxID_ANY, 
                _L("common_preview_1print_title"), center);
            ANKER_LOG_INFO << "chooseDeviceDialog show";
            m_chooseDeviceDialog->ShowModal();
            delete m_chooseDeviceDialog;
            m_chooseDeviceDialog = nullptr;
            p->m_isPrint = false;
            // fix: the checkbox UI effect change when chooseDeviceDialog popup.
            //p->previewRightSidePanel->UpdateGcodePreviewSideBar(true, REASON_NONE);
        }

        // A key print clicked.
        void Slic3r::GUI::Plater::a_key_print_clicked()
        {
            auto ankerNet = AnkerNetInst();
            if (ankerNet && ankerNet->IsLogined()) {
                if (ExportGacode()) {
                    ankerNet->AsyOneKeyPrint();
                    ShowDeviceList();
                }
            }
            else 
            {
                wxGetApp().mainframe->ShowAnkerWebView("onekey print button clicked");
            }
        }

        void Slic3r::GUI::Plater::onSliceNow()
        {
            if (canvas3D()->get_gizmos_manager().is_in_editing_mode(true))
                return;

            const bool export_gcode_after_slicing = wxGetKeyState(WXK_SHIFT);
            if (export_gcode_after_slicing)
                export_gcode(true);
            else
                reslice();
            select_view_3D(VIEW_MODE_3D);
            select_view_3D(VIEW_MODE_PREVIEW);
        }

		void Plater::set_export_progress_change_callback(progressChangeCallbackFunction cb)
        {
            if (p)
                p->set_export_progress_change_callback(cb); 
        };

        void Plater::set_app_closing(bool v)
        {
            if (p)
                 p->set_app_closing(v);
        };

        bool Plater::is_exporting()
        {
            return is_exporting_gcode() || is_exporting_acode();
        };

        bool Plater::is_exporting_gcode()
        {
            bool ret = false;
            if (p)
                ret = p->background_process.is_export_scheduled();
            return ret;
        };

        bool Plater::is_exporting_acode()
        {
            bool ret = false;
            if (p)
                ret = p->is_exporting_acode();
            return ret;
        };

        void Plater::stop_exporting_acode()
        {
            if (p)
                p->cancel_exporting_acode();
        };


        bool Plater::is_cancel_exporting_Gcode()
        {
            bool ret = false;
            if (p)
                ret = p->is_cancel_exporting_Gcode();
            return ret;
        };
        

        void Plater::set_create_AI_file_val(bool val)
        {
            if (p)
                p->set_create_AI_file(val);
        };

        bool Plater::get_create_AI_file_val()
        {
            bool ret = false;
            if (p)
                ret = p->get_create_AI_file();
            return ret;
        };

        void Plater::set_view_drop_file(bool val)
        {
            if (p)
                p->set_view_drop_file(val);
        }

        bool Plater::is_view_drop_file()
        {
            bool ret = false;
            if (p)
                ret = p->is_view_drop_file();
            return ret;
        }

        void Plater::export_akeyPrint_gcode(std::string& path, bool isAcode)
        {
            if (p->model.objects.empty())
                return;

            if (canvas3D()->get_gizmos_manager().is_in_editing_mode(true))
                return;


            if (p->process_completed_with_error)
                return;

            // If possible, remove accents from accented latin characters.
            // This function is useful for generating file names to be processed by legacy firmwares.
            fs::path default_output_file;
            try {
                // Update the background processing, so that the placeholder parser will get the correct values for the ouput file template.
                // Also if there is something wrong with the current configuration, a pop-up dialog will be shown and the export will not be performed.
                unsigned int state = this->p->update_restart_background_process(false, false);
                if (state & priv::UPDATE_BACKGROUND_PROCESS_INVALID)
                    return;
                //default_output_file = this->p->background_process.output_filepath_for_project(into_path(get_project_filename(".3mf")));

                //auto wstr_temp = wxStandardPaths::Get().GetUserLocalDataDir() + "\\..\\Temp";
                auto wstr_temp = wxStandardPaths::Get().GetTempDir();
                boost::filesystem::path tmpDir(wstr_temp.ToStdWstring());

                if (model().objects.size() > 0) {
                    std::string name = "";
                    if ((*model().objects.begin())) {
                        name = (*model().objects.begin())->name;
                        while (name.size() > 0 && name[0] == ' ')
                        {
                            name.erase(0, 1);
                        }
                    }
#ifdef _WIN32
                    default_output_file = tmpDir.string() + "\\" + name;
#elif __APPLE__
                    default_output_file = tmpDir.string() + "/" + name;
#endif // _WIN32

                    
                }
            }
            catch (const Slic3r::PlaceholderParserError& ex) {
                // Show the error with monospaced font.
                show_error(this, ex.what(), true);
                return;
            }
            catch (const std::exception& ex) {
                show_error(this, ex.what(), false);
                return;
            }
            std::string tmpGcodePath = default_output_file.string();
            int index = tmpGcodePath.find_last_of(".");
            if (index == std::string::npos) {
                index = tmpGcodePath.length();
            }
            std::string suffix = get_create_AI_file_val() ? ".acode" : ".gcode";
            tmpGcodePath = tmpGcodePath.substr(0, index) + "_bak"+ suffix;
            ANKER_LOG_INFO << "  tmpGcodePath:" << tmpGcodePath;
            default_output_file = fs::path(tmpGcodePath);
            AppConfig& appconfig = *wxGetApp().app_config;
            RemovableDriveManager& removable_drive_manager = *wxGetApp().removable_drive_manager();
            boost::filesystem::path output_path(default_output_file);
            if (!output_path.empty()) {
                bool need_export = true;
                if (!p->exported_file_cache.empty() && fs::exists(fs::path(p->exported_file_cache))) {
                    std::string error_message;
                    CopyFileResult copy_ret_val = copy_file(p->exported_file_cache, output_path.string(), error_message, true);
                    if (copy_ret_val == CopyFileResult::SUCCESS) {        
                        need_export = false;
                    }
                    else {
                        ANKER_LOG_ERROR << "copy file failed,ret=" << static_cast<int>(copy_ret_val) << " err msg=" << error_message
                            << "  from file=" << p->exported_file_cache << "  to file:" << output_path.string();
                    }
                }
                
                if(need_export)
                {
                    bool path_on_removable_media = removable_drive_manager.set_and_verify_last_save_path(output_path.string());
                    p->notification_manager->new_export_began(path_on_removable_media);
                    p->exporting_status = path_on_removable_media ? ExportingStatus::EXPORTING_TO_REMOVABLE : ExportingStatus::EXPORTING_TO_LOCAL;
                    p->last_output_path = output_path.string();
                    p->last_output_dir_path = output_path.parent_path().string();
                    p->export_gcode(output_path, path_on_removable_media, PrintHostJob());
                    // Storing a path to AppConfig either as path to removable media or a path to internal media.
                    // is_path_on_removable_drive() is called with the "true" parameter to update its internal database as the user may have shuffled the external drives
                    // while the dialog was open.
                    appconfig.update_last_output_dir(output_path.parent_path().string(), path_on_removable_media);
                }
                path = output_path.string();
            }
        }

        void Plater::export_gcode(bool prefer_removable, bool disableAI)
        {            
            //report: export gcode
            std::string fileName = std::string();
            std::string fileSize = std::string("0"); 
            std::string handleType = std::string("export");  
            std::string handleDuration = std::string("0");
            std::string errorCode = std::string("0");
            std::string errorMsg = std::string("export gcode");
            if(disableAI == false && get_create_AI_file_val())
                errorMsg = std::string("export acode");
            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_ag_file_name, fileName));
            buryMap.insert(std::make_pair(c_ag_file_size, fileSize));
            buryMap.insert(std::make_pair(c_ag_handle_type, handleType));

            if (p->model.objects.empty()) {
                ANKER_LOG_ERROR << "model.objects.empty";

                errorCode = -1;
                errorMsg = "model.objects.empty";
                             
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);

                return;
            }

            if (canvas3D()->get_gizmos_manager().is_in_editing_mode(true)) {
                ANKER_LOG_ERROR << "is_in_editing_mode";
                errorCode = -1;
                errorMsg = "is_in_editing_mode";
                 
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);
                
                return;
            }


            if (p->process_completed_with_error) {
                ANKER_LOG_ERROR << "process_completed_with_error";
                errorCode = -1;
                errorMsg = "process_completed_with_error";
                   
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);

                return;
            }

            // If possible, remove accents from accented latin characters.
            // This function is useful for generating file names to be processed by legacy firmwares.
            fs::path default_output_file;
            try {
                // Update the background processing, so that the placeholder parser will get the correct values for the ouput file template.
                // Also if there is something wrong with the current configuration, a pop-up dialog will be shown and the export will not be performed.
                unsigned int state = this->p->update_restart_background_process(false, false);
                if (state & priv::UPDATE_BACKGROUND_PROCESS_INVALID) {
                    ANKER_LOG_ERROR << "UPDATE_BACKGROUND_PROCESS_INVALID";
                    errorCode = -1;
                    errorMsg = "UPDATE_BACKGROUND_PROCESS_INVALID";                    
                    buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                    buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                    ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                    reportBuryEvent(e_ag_handle, buryMap);
                    return;
                }
                default_output_file = this->p->background_process.output_filepath_for_project(into_path(get_project_filename(".3mf")));
            }
            catch (const Slic3r::PlaceholderParserError& ex) {
                // Show the error with monospaced font.
                ANKER_LOG_ERROR << "show_error 1"<<ex.what();

                std::string errorWhat = ex.what();
                std::string errorInfo = "show_error 1" + errorWhat;
                errorCode = -1;
                errorMsg = errorInfo;
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);
                
                show_error(this, ex.what(), true);
                return;
            }
            catch (const std::exception& ex) {
                ANKER_LOG_ERROR << "show_error 2" << ex.what();

                std::string errorWhat = ex.what();
                std::string errorInfo = "show_error 1" + errorWhat;
                errorCode = -1;
                errorMsg = errorInfo;
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);
                
                show_error(this, ex.what(), false);
                return;
            }
            default_output_file = fs::path(Slic3r::fold_utf8_to_ascii(default_output_file.string()));
            AppConfig& appconfig = *wxGetApp().app_config;
            RemovableDriveManager& removable_drive_manager = *wxGetApp().removable_drive_manager();
            // Get a last save path, either to removable media or to an internal media.
            std::string      		 start_dir = appconfig.get_last_output_dir(default_output_file.parent_path().string(), prefer_removable);
            if (prefer_removable) {
                // Returns a path to a removable media if it exists, prefering start_dir. Update the internal removable drives database.
                start_dir = removable_drive_manager.get_removable_drive_path(start_dir);
                if (start_dir.empty())
                    // Direct user to the last internal media.
                    start_dir = appconfig.get_last_output_dir(default_output_file.parent_path().string(), false);
            }

            fs::path output_path;
            {
#if !ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR
                std::string ext = default_output_file.extension().string();
#endif // !ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR

                if (disableAI == false && get_create_AI_file_val()) {
                    // export as .acode file
                    if (default_output_file.extension().string() == ".gcode")
                        default_output_file.replace_extension(".acode");
                }

                wxFileDialog dlg(this, (printer_technology() == ptFFF) ? _L("Save G-code file as:") : _L("Save SL1 / SL1S file as:"),
                    start_dir,
                    from_path(default_output_file.filename()),
#if ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR
                    GUI::file_wildcards((printer_technology() == ptFFF) ? (disableAI == false && get_create_AI_file_val() ? FT_ACODE : FT_GCODE) : FT_SL1),
#else
                    GUI::file_wildcards((printer_technology() == ptFFF) ? (disableAI == false && get_create_AI_file_val() ? FT_ACODE : FT_GCODE) : FT_SL1, ext),
#endif // ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                );
                if (dlg.ShowModal() == wxID_OK) {

                    output_path = into_path(dlg.GetPath());
                    fileName = output_path.string();
                    while (has_illegal_filename_characters(output_path.filename().string())) {
                        show_error(this, _L("The provided file name is not valid.") + "\n" +
                            _L("The following characters are not allowed by a FAT file system:") + " <>:/\\|?*\"");
                        dlg.SetFilename(from_path(output_path.filename()));
                        if (dlg.ShowModal() == wxID_OK)
                            output_path = into_path(dlg.GetPath());
                        else {
                            output_path.clear();
                            break;
                        }
                    }
                }
            }

            ANKER_LOG_INFO << "output_path :" << output_path.string();
            if (!output_path.empty()) {

                auto startTime = wxDateTime::Now();

                bool path_on_removable_media = removable_drive_manager.set_and_verify_last_save_path(output_path.string());
                p->notification_manager->new_export_began(path_on_removable_media);
                p->exporting_status = path_on_removable_media ? ExportingStatus::EXPORTING_TO_REMOVABLE : ExportingStatus::EXPORTING_TO_LOCAL;
                p->last_output_path = output_path.string();
                p->last_output_dir_path = output_path.parent_path().string();
                p->export_gcode(output_path, path_on_removable_media, PrintHostJob());
                // Storing a path to AppConfig either as path to removable media or a path to internal media.
                // is_path_on_removable_drive() is called with the "true" parameter to update its internal database as the user may have shuffled the external drives
                // while the dialog was open.
                appconfig.update_last_output_dir(output_path.parent_path().string(), path_on_removable_media);

                auto GetFileSize = [](const wxString& filePath) -> wxULongLong {
                    wxFileName fname(filePath);
                    if (fname.FileExists()) {
                        wxFile file(filePath);
                        if (file.IsOpened()) {
                            wxULongLong fileSize = file.Length();
                            file.Close();
                            return fileSize;
                        }
                        else {
                            ANKER_LOG_ERROR << ("can't open the file: " + filePath);
                        }
                    }
                    else {
                        ANKER_LOG_ERROR << ("file not exist: %s", filePath);
                    }
                    return 0;
                };

                auto nowTime = wxDateTime::Now();
                auto timeDifference = nowTime - startTime;
                handleDuration = timeDifference.GetValue().ToString().ToStdString();
                fileSize = GetFileSize(fileName).ToString().ToStdString();

                buryMap.insert(std::make_pair(c_ag_file_name, fileName));
                buryMap.insert(std::make_pair(c_ag_handle_duration, handleDuration));
                buryMap.insert(std::make_pair(c_ag_error_code, errorCode));
                buryMap.insert(std::make_pair(c_ag_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_ag_handle;
                reportBuryEvent(e_ag_handle, buryMap);
            }
        }

        void Plater::export_stl_obj(bool extended, bool selection_only)
        {
            std::string handleType = std::string("export");
            std::string errorCode = std::string("0");
            std::string errorMsg = std::string("export obj stl");

            std::map<std::string, std::string> buryMap;
            buryMap.insert(std::make_pair(c_hm_type, handleType));
                        
            if (p->model.objects.empty()) { 
                errorCode = -1;
                errorMsg = "model objects is null";
                buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                return; 
            wxString path = p->get_export_file(FT_OBJECT);
            const std::string path_u8 = into_u8(path);
            if (path.empty()) { 
                errorCode = -1;
                errorMsg = "export path is empty";
                buryMap.insert(std::make_pair(c_hm_file_name, path_u8));
                buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                reportBuryEvent(e_hanlde_model, buryMap);
                return; 
            }

            wxBusyCursor wait;

            const auto& selection = p->get_selection();
            const auto obj_idx = selection.get_object_idx();
            if (selection_only && (obj_idx == -1 || selection.is_wipe_tower()))
            {
                errorCode = -1;
                errorMsg = "selection  objects is unvalid";
                buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
                buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));
                buryMap.insert(std::make_pair(c_hm_file_name, path_u8));
                ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
                reportBuryEvent(e_hanlde_model, buryMap);       
                return;
            }

            // Following lambda generates a combined mesh for export with normals pointing outwards.
            auto mesh_to_export_fff = [this](const ModelObject& mo, int instance_id) {
                TriangleMesh mesh;

                std::vector<csg::CSGPart> csgmesh;
                csgmesh.reserve(2 * mo.volumes.size());
                csg::model_to_csgmesh(mo, Transform3d::Identity(), std::back_inserter(csgmesh),
                    csg::mpartsPositive | csg::mpartsNegative | csg::mpartsDoSplits);

                auto csgrange = range(csgmesh);
                if (csg::is_all_positive(csgrange)) {
                    mesh = TriangleMesh{ csg::csgmesh_merge_positive_parts(csgrange) };
                }
                else if (csg::check_csgmesh_booleans(csgrange) == csgrange.end()) {
                    try {
                        auto cgalm = csg::perform_csgmesh_booleans(csgrange);
                        mesh = MeshBoolean::cgal::cgal_to_triangle_mesh(*cgalm);
                    }
                    catch (...) {}
                }

                if (mesh.empty()) {
                    get_notification_manager()->push_plater_error_notification(
                        _u8L("common_slicepopup_boolean"));

                    for (const ModelVolume* v : mo.volumes)
                        if (v->is_model_part()) {
                            TriangleMesh vol_mesh(v->mesh());
                            vol_mesh.transform(v->get_matrix(), true);
                            mesh.merge(vol_mesh);
                        }
                }

                if (instance_id == -1) {
                    TriangleMesh vols_mesh(mesh);
                    mesh = TriangleMesh();
                    for (const ModelInstance* i : mo.instances) {
                        TriangleMesh m = vols_mesh;
                        m.transform(i->get_matrix(), true);
                        mesh.merge(m);
                    }
                }
                else if (0 <= instance_id && instance_id < int(mo.instances.size()))
                    mesh.transform(mo.instances[instance_id]->get_matrix(), true);
                return mesh;
            };

            auto mesh_to_export_sla = [&, this](const ModelObject& mo, int instance_id) {
                TriangleMesh mesh;

                const SLAPrintObject* object = this->p->sla_print.get_print_object_by_model_object_id(mo.id());

                if (auto m = object->get_mesh_to_print(); !m || m->empty())
                    mesh = mesh_to_export_fff(mo, instance_id);
                else {
                    const Transform3d mesh_trafo_inv = object->trafo().inverse();
                    const bool is_left_handed = object->is_left_handed();

                    auto pad_mesh = extended ? object->pad_mesh() : TriangleMesh{};
                    pad_mesh.transform(mesh_trafo_inv);

                    auto supports_mesh = extended ? object->support_mesh() : TriangleMesh{};
                    supports_mesh.transform(mesh_trafo_inv);

                    const std::vector<SLAPrintObject::Instance>& obj_instances = object->instances();
                    for (const SLAPrintObject::Instance& obj_instance : obj_instances) {
                        auto it = std::find_if(object->model_object()->instances.begin(), object->model_object()->instances.end(),
                            [&obj_instance](const ModelInstance* mi) { return mi->id() == obj_instance.instance_id; });
                        assert(it != object->model_object()->instances.end());

                        if (it != object->model_object()->instances.end()) {
                            const bool one_inst_only = selection_only && !selection.is_single_full_object();

                            const int instance_idx = it - object->model_object()->instances.begin();
                            const Transform3d& inst_transform = one_inst_only
                                ? Transform3d::Identity()
                                : object->model_object()->instances[instance_idx]->get_transformation().get_matrix();

                            TriangleMesh inst_mesh;

                            if (!pad_mesh.empty()) {
                                TriangleMesh inst_pad_mesh = pad_mesh;
                                inst_pad_mesh.transform(inst_transform, is_left_handed);
                                inst_mesh.merge(inst_pad_mesh);
                            }

                            if (!supports_mesh.empty()) {
                                TriangleMesh inst_supports_mesh = supports_mesh;
                                inst_supports_mesh.transform(inst_transform, is_left_handed);
                                inst_mesh.merge(inst_supports_mesh);
                            }

                            std::shared_ptr<const indexed_triangle_set> m = object->get_mesh_to_print();
                            TriangleMesh inst_object_mesh;
                            if (m)
                                inst_object_mesh = TriangleMesh{ *m };

                            inst_object_mesh.transform(mesh_trafo_inv);
                            inst_object_mesh.transform(inst_transform, is_left_handed);

                            inst_mesh.merge(inst_object_mesh);

                            // ensure that the instance lays on the bed
                            inst_mesh.translate(0.0f, 0.0f, -inst_mesh.bounding_box().min.z());

                            // merge instance with global mesh
                            mesh.merge(inst_mesh);

                            if (one_inst_only)
                                break;
                        }
                    }
                }

                return mesh;
            };

            std::function<TriangleMesh(const ModelObject& mo, int instance_id)>
                mesh_to_export;

            if (p->printer_technology == ptFFF)
                mesh_to_export = mesh_to_export_fff;
            else
                mesh_to_export = mesh_to_export_sla;

            TriangleMesh mesh;
            if (selection_only) {
                const ModelObject* model_object = p->model.objects[obj_idx];
                if (selection.get_mode() == Selection::Instance)
                    mesh = mesh_to_export(*model_object, (selection.is_single_full_object() && model_object->instances.size() > 1) ? -1 : selection.get_instance_idx());
                else {
                    const GLVolume* volume = selection.get_first_volume();
                    mesh = model_object->volumes[volume->volume_idx()]->mesh();
                    mesh.transform(volume->get_volume_transformation().get_matrix(), true);
                }

                if (!selection.is_single_full_object() || model_object->instances.size() == 1)
                    mesh.translate(-model_object->origin_translation.cast<float>());
            }
            else {
                for (const ModelObject* o : p->model.objects) {
                    mesh.merge(mesh_to_export(*o, -1));
                }
            }

            if (path.Lower().EndsWith(".stl"))
                Slic3r::store_stl(path_u8.c_str(), &mesh, true);
            else if (path.Lower().EndsWith(".obj"))
                Slic3r::store_obj(path_u8.c_str(), &mesh);

            errorMsg = "export success";
            buryMap.insert(std::make_pair(c_hm_file_name, path_u8));
            buryMap.insert(std::make_pair(c_hm_error_code, errorCode));
            buryMap.insert(std::make_pair(c_hm_error_msg, errorMsg));   
            ANKER_LOG_INFO << "Report bury event is " << e_hanlde_model;
            reportBuryEvent(e_hanlde_model, buryMap);
            //    p->statusbar()->set_status_text(format_wxstr(_L("STL file exported to %s"), path));
        }

        void Plater::export_amf()
        {
            if (p->model.objects.empty()) { return; }

            wxString path = p->get_export_file(FT_AMF);
            if (path.empty()) { return; }
            const std::string path_u8 = into_u8(path);

            wxBusyCursor wait;
            bool export_config = true;
            DynamicPrintConfig cfg = wxGetApp().preset_bundle->full_config_secure();
            bool full_pathnames = wxGetApp().app_config->get_bool("export_sources_full_pathnames");
            if (Slic3r::store_amf(path_u8.c_str(), &p->model, export_config ? &cfg : nullptr, full_pathnames)) {
                // Success
        //        p->statusbar()->set_status_text(format_wxstr(_L("AMF file exported to %s"), path));
            }
            else {
                // Failure
        //        p->statusbar()->set_status_text(format_wxstr(_L("Error exporting AMF file %s"), path));
            }
        }

        namespace {
            std::string get_file_name(const std::string& file_path)
            {
                size_t pos_last_delimiter = file_path.find_last_of("/\\");
                size_t pos_point = file_path.find_last_of('.');
                size_t offset = pos_last_delimiter + 1;
                size_t count = pos_point - pos_last_delimiter - 1;
                return file_path.substr(offset, count);
            }
            using SvgFile = EmbossShape::SvgFile;
            using SvgFiles = std::vector<SvgFile*>;
            std::string create_unique_3mf_filepath(const std::string& file, const SvgFiles svgs)
            {
                // const std::string MODEL_FOLDER = "3D/"; // copy from file 3mf.cpp
                std::string path_in_3mf = "3D/" + file + ".svg";
                size_t suffix_number = 0;
                bool is_unique = false;
                do {
                    is_unique = true;
                    path_in_3mf = "3D/" + file + ((suffix_number++) ? ("_" + std::to_string(suffix_number)) : "") + ".svg";
                    for (SvgFile* svgfile : svgs) {
                        if (svgfile->path_in_3mf.empty())
                            continue;
                        if (svgfile->path_in_3mf.compare(path_in_3mf) == 0) {
                            is_unique = false;
                            break;
                        }
                    }
                } while (!is_unique);
                return path_in_3mf;
            }

            bool set_by_local_path(SvgFile& svg, const SvgFiles& svgs)
            {
                // Try to find already used svg file
                for (SvgFile* svg_ : svgs) {
                    if (svg_->path_in_3mf.empty())
                        continue;
                    if (svg.path.compare(svg_->path) == 0) {
                        svg.path_in_3mf = svg_->path_in_3mf;
                        return true;
                    }
                }
                return false;
            }

            /// <summary>
            /// Function to secure private data before store to 3mf
            /// </summary>
            /// <param name="model">Data(also private) to clean before publishing</param>
            void publish(Model& model) {

                // SVG file publishing
                bool exist_new = false;
                SvgFiles svgfiles;
                for (ModelObject* object : model.objects) {
                    for (ModelVolume* volume : object->volumes) {
                        if (!volume->emboss_shape.has_value())
                            continue;
                        if (volume->text_configuration.has_value())
                            continue; // text dosen't have svg path

                        assert(volume->emboss_shape->svg_file.has_value());
                        if (!volume->emboss_shape->svg_file.has_value())
                            continue;

                        SvgFile* svg = &(*volume->emboss_shape->svg_file);
                        if (svg->path_in_3mf.empty())
                            exist_new = true;
                        svgfiles.push_back(svg);
                    }
                }

                //if (exist_new) {
                //    MessageDialog dialog(nullptr,
                //        _L("Are you sure you want to store original SVGs with their local paths into the 3MF file?\n"
                //            "If you hit 'NO', all SVGs in the project will not be editable any more."),
                //        _L("Private protection"), wxYES_NO | wxICON_QUESTION);
                //    if (dialog.ShowModal() == wxID_NO) {
                //        for (ModelObject* object : model.objects)
                //            for (ModelVolume* volume : object->volumes)
                //                if (volume->emboss_shape.has_value())
                //                    volume->emboss_shape.reset();
                //    }
                //}

                for (SvgFile* svgfile : svgfiles) {
                    if (!svgfile->path_in_3mf.empty())
                        continue; // already suggested path (previous save)

                    // create unique name for svgs, when local path differ
                    std::string filename = "unknown";
                    if (!svgfile->path.empty()) {
                        if (set_by_local_path(*svgfile, svgfiles))
                            continue;
                        // check whether original filename is already in:
                        filename = get_file_name(svgfile->path);
                    }
                    svgfile->path_in_3mf = create_unique_3mf_filepath(filename, svgfiles);
                }
            }
        }

        bool Plater::export_3mf(const boost::filesystem::path& output_path)
        {
            if (p->model.objects.empty()) {
                MessageDialog dialog(nullptr, _L("The plater is empty.\nDo you want to save the project?"), _L("Save project"), wxYES_NO);
                if (dialog.ShowModal() != wxID_YES)
                    return false;
            }

            wxString path;
            bool export_config = true;
            if (output_path.empty()) {
                path = p->get_export_file(FT_3MF);
                if (path.empty()) { return false; }
            }
            else
                path = from_path(output_path);

            if (!path.Lower().EndsWith(".3mf"))
                return false;

            // take care about private data stored into .3mf
            // modify model
            publish(p->model);

            DynamicPrintConfig cfg = wxGetApp().preset_bundle->full_config_secure();
            const std::string path_u8 = into_u8(path);
            wxBusyCursor wait;
            bool full_pathnames = wxGetApp().app_config->get_bool("export_sources_full_pathnames");
            ThumbnailData thumbnail_data;
            ThumbnailsParams thumbnail_params = { {}, false, true, true, true };
            p->generate_thumbnail(thumbnail_data, THUMBNAIL_SIZE_3MF.first, THUMBNAIL_SIZE_3MF.second, thumbnail_params, Camera::EType::Ortho);
            SliceModelData slice_data { m_print_time, m_filament};
            bool ret = false;
            try
            {
                ret = Slic3r::store_3mf(path_u8.c_str(), &p->model, export_config ? &cfg : nullptr, full_pathnames, &thumbnail_data, &slice_data);
            }
            catch (boost::filesystem::filesystem_error& e)
            {
                const wxString what = _("Unable to save file") + ": " + path_u8 + "\n" + e.code().message();
                MessageDialog dlg(this, what, _("Error saving 3mf file"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
            }
            if (ret) {
                // Success
        //        p->statusbar()->set_status_text(format_wxstr(_L("3MF file exported to %s"), path));
                p->set_project_filename(path);
            }
            else {
                // Failure
        //        p->statusbar()->set_status_text(format_wxstr(_L("Error exporting 3MF file %s"), path));
            }
            return ret;
        }

        void Plater::reload_from_disk()
        {
            p->reload_from_disk();
        }

        void Plater::replace_with_stl()
        {
            p->replace_with_stl();
        }

        void Plater::reload_all_from_disk()
        {
            p->reload_all_from_disk();
        }

        bool Plater::has_toolpaths_to_export() const
        {
            return  p->preview->get_canvas3d()->has_toolpaths_to_export();
        }

        void Plater::export_toolpaths_to_obj() const
        {
            if ((printer_technology() != ptFFF) || !is_preview_loaded())
                return;

            wxString path = p->get_export_file(FT_OBJ);
            if (path.empty())
                return;

            wxBusyCursor wait;
            p->preview->get_canvas3d()->export_toolpaths_to_obj(into_u8(path).c_str());
        }

        void Plater::reslice()
        {
            p->report_slicing_buryevt("start", "0", "start reslice model");

            ANKER_LOG_INFO << "reslice";
            // First, after importing the stl file without slicing, and then importing gcode, reslice will be called.
            if (p->preview->is_GcodeImported()) {
                return;
            }

            // Anker: Update the state of background process. 
            if (!this->p->suppressed_backround_processing_update)
                this->p->update_restart_background_process(false, false);

            //this->p->background_process.setState(BackgroundSlicingProcess::STATE_RUNNING);
            // There is "invalid data" button instead "slice now"
            if (p->process_completed_with_error)
            {
                p->report_slicing_buryevt("start", "-1", "There is invalid data button instead slice now");
                return;
            }

            // In case SLA gizmo is in editing mode, refuse to continue
            // and notify user that he should leave it first.
            if (canvas3D()->get_gizmos_manager().is_in_editing_mode(true))
                return;

            // Stop the running (and queued) UI jobs and only proceed if they actually
            // get stopped.
            unsigned timeout_ms = 10000;
            if (!stop_queue(this->get_ui_job_worker(), timeout_ms)) {
                BOOST_LOG_TRIVIAL(error) << "Could not stop UI job within "
                    << timeout_ms << " milliseconds timeout!";

                p->report_slicing_buryevt("start", "-1", "milliseconds timeout!");

                return;
            unsigned int state = this->p->update_background_process(true);
            if (state & priv::UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE)
                this->p->view3D->reload_scene(false);
            // If the SLA processing of just a single object's supports is running, restart slicing for the whole object.
            this->p->background_process.set_task(PrintBase::TaskParams());
            // Only restarts if the state is valid.
            this->p->restart_background_process(state | priv::UPDATE_BACKGROUND_PROCESS_FORCE_RESTART);
            
            // update_background_process() reports, that the Print / SLAPrint is invalid, and the error message
            // don't report add by alves
            if ((state & priv::UPDATE_BACKGROUND_PROCESS_INVALID) != 0)
                return;

            bool clean_gcode_toolpaths = true;
            if (p->background_process.running())
            {
                if (wxGetApp().get_mode() == comSimple)
                {
                    // p->sidebar->set_btn_label(ActionButtonType::abReslice, _L("Slicing") + dots);
                }
                else
                {
                   // p->sidebar->set_btn_label(ActionButtonType::abReslice, _L("Slice now"));
                    p->show_action_buttons(false);
                }
            }
            else if (!p->background_process.empty() && !p->background_process.idle())
                p->show_action_buttons(true);
            else
                clean_gcode_toolpaths = false;

            if (clean_gcode_toolpaths)
                reset_gcode_toolpaths();

            p->preview->reload_print(!clean_gcode_toolpaths);
        }

        void Plater::reslice_until_step_inner(int step, const ModelObject& object, bool postpone_error_messages)
        {
            //FIXME Don't reslice if export of G-code or sending to OctoPrint is running.
            // bitmask of UpdateBackgroundProcessReturnState
            unsigned int state = this->p->update_background_process(true, postpone_error_messages);
            if (state & priv::UPDATE_BACKGROUND_PROCESS_REFRESH_SCENE)
                this->p->view3D->reload_scene(false);

            if (this->p->background_process.empty() || (state & priv::UPDATE_BACKGROUND_PROCESS_INVALID))
                // Nothing to do on empty input or invalid configuration.
                return;

            // Limit calculation to the single object only.
            PrintBase::TaskParams task;
            task.single_model_object = object.id();
            // If the background processing is not enabled, calculate supports just for the single instance.
            // Otherwise calculate everything, but start with the provided object.
            if (!this->p->background_processing_enabled()) {
                task.single_model_instance_only = true;
                task.to_object_step = step;
            }
            this->p->background_process.set_task(task);
            // and let the background processing start.
            this->p->restart_background_process(state | priv::UPDATE_BACKGROUND_PROCESS_FORCE_RESTART);
        }

        void Plater::reslice_FFF_until_step(PrintObjectStep step, const ModelObject& object, bool postpone_error_messages)
        {
            this->reslice_until_step_inner(PrintObjectStep(step), object, postpone_error_messages);
        }

        void Plater::reslice_SLA_until_step(SLAPrintObjectStep step, const ModelObject& object, bool postpone_error_messages)
        {
            this->reslice_until_step_inner(SLAPrintObjectStep(step), object, postpone_error_messages);
        }

        void Plater::send_gcode()
        {
            // if physical_printer is selected, send gcode for this printer
            DynamicPrintConfig* physical_printer_config = wxGetApp().preset_bundle->physical_printers.get_selected_printer_config();
            if (!physical_printer_config || p->model.objects.empty())
                return;

            PrintHostJob upload_job(physical_printer_config);
            if (upload_job.empty())
                return;

            // Obtain default output path
            fs::path default_output_file;
            try {
                // Update the background processing, so that the placeholder parser will get the correct values for the ouput file template.
                // Also if there is something wrong with the current configuration, a pop-up dialog will be shown and the export will not be performed.
                unsigned int state = this->p->update_restart_background_process(false, false);
                if (state & priv::UPDATE_BACKGROUND_PROCESS_INVALID)
                    return;
                default_output_file = this->p->background_process.output_filepath_for_project(into_path(get_project_filename(".3mf")));
            }
            catch (const Slic3r::PlaceholderParserError& ex) {
                // Show the error with monospaced font.
                show_error(this, ex.what(), true);
                return;
            }
            catch (const std::exception& ex) {
                show_error(this, ex.what(), false);
                return;
            }
            default_output_file = fs::path(Slic3r::fold_utf8_to_ascii(default_output_file.string()));

            // Repetier specific: Query the server for the list of file groups.
            wxArrayString groups;
            {
                wxBusyCursor wait;
                upload_job.printhost->get_groups(groups);
            }
            // AnkerLink specific: Query the server for the list of file groups.
            wxArrayString storage_paths;
            wxArrayString storage_names;
            {
                wxBusyCursor wait;
                try {
                    upload_job.printhost->get_storage(storage_paths, storage_names);
                }
                catch (const Slic3r::IOError& ex) {
                    show_error(this, ex.what(), false);
                    return;
                }
            }

            PrintHostSendDialog dlg(default_output_file, upload_job.printhost->get_post_upload_actions(), groups, storage_paths, storage_names);
            if (dlg.ShowModal() == wxID_OK) {
                upload_job.upload_data.upload_path = dlg.filename();
                upload_job.upload_data.post_action = dlg.post_action();
                upload_job.upload_data.group = dlg.group();
                upload_job.upload_data.storage = dlg.storage();

                // Show "Is printer clean" dialog for AnkerConnect - Upload and print.
                if (std::string(upload_job.printhost->get_name()) == "AnkerConnect" && upload_job.upload_data.post_action == PrintHostPostUploadAction::StartPrint) {
                    GUI::MessageDialog dlg(nullptr, _L("Is the printer ready? Is the print sheet in place, empty and clean?"), _L("Upload and Print"), wxOK | wxCANCEL);
                    if (dlg.ShowModal() != wxID_OK)
                        return;
                }

                p->export_gcode(fs::path(), false, std::move(upload_job));
            }
        }

        // Called when the Eject button is pressed.
        void Plater::eject_drive()
        {
            wxBusyCursor wait;
            wxGetApp().removable_drive_manager()->eject_drive();
        }

        void Plater::take_snapshot(const std::string& snapshot_name) { p->take_snapshot(snapshot_name); }
        void Plater::take_snapshot(const wxString& snapshot_name) { p->take_snapshot(snapshot_name); }
        void Plater::take_snapshot(const std::string& snapshot_name, UndoRedo::SnapshotType snapshot_type) { p->take_snapshot(snapshot_name, snapshot_type); }
        void Plater::take_snapshot(const wxString& snapshot_name, UndoRedo::SnapshotType snapshot_type) { p->take_snapshot(snapshot_name, snapshot_type); }
        void Plater::suppress_snapshots() { p->suppress_snapshots(); }
        void Plater::allow_snapshots() { p->allow_snapshots(); }
        void Plater::undo() { p->undo(); }
        void Plater::redo() { p->redo(); }
        void Plater::undo_to(int selection)
        {
            if (selection == 0) {
                p->undo();
                return;
            }

            const int idx = p->get_active_snapshot_index() - selection - 1;
            p->undo_redo_to(p->undo_redo_stack().snapshots()[idx].timestamp);
        }
        void Plater::redo_to(int selection)
        {
            if (selection == 0) {
                p->redo();
                return;
            }

            const int idx = p->get_active_snapshot_index() + selection + 1;
            p->undo_redo_to(p->undo_redo_stack().snapshots()[idx].timestamp);
        }
        bool Plater::undo_redo_string_getter(const bool is_undo, int idx, const char** out_text)
        {
            const std::vector<UndoRedo::Snapshot>& ss_stack = p->undo_redo_stack().snapshots();
            const int idx_in_ss_stack = p->get_active_snapshot_index() + (is_undo ? -(++idx) : idx);

            if (0 < idx_in_ss_stack && (size_t)idx_in_ss_stack < ss_stack.size() - 1) {
                *out_text = ss_stack[idx_in_ss_stack].name.c_str();
                return true;
            }

            return false;
        }

        void Plater::undo_redo_topmost_string_getter(const bool is_undo, std::string& out_text)
        {
            const std::vector<UndoRedo::Snapshot>& ss_stack = p->undo_redo_stack().snapshots();
            const int idx_in_ss_stack = p->get_active_snapshot_index() + (is_undo ? -1 : 0);

            if (0 < idx_in_ss_stack && (size_t)idx_in_ss_stack < ss_stack.size() - 1) {
                out_text = ss_stack[idx_in_ss_stack].name;
                return;
            }

            out_text = "";
        }

        bool Plater::search_string_getter(int idx, const char** label, const char** tooltip)
        {
            // add by allen for ankerCfgDlg search
            //const Search::OptionsSearcher& search_list = p->sidebar->get_searcher();
            const Search::OptionsSearcher& search_list = p->sidebarnew->get_searcher();

            if (0 <= idx && (size_t)idx < search_list.size()) {
                search_list[idx].get_marked_label_and_tooltip(label, tooltip);
                return true;
            }

            return false;
        }

        void Plater::on_extruders_change(size_t num_extruders)
        {
#if USE_SIDEBAR_NEW
            sidebarnew().onExtrudersChange(num_extruders);
#else
            auto& choices = sidebar().combos_filament();

            if (num_extruders == choices.size())
                return;

            wxWindowUpdateLocker noUpdates_scrolled_panel(&sidebar()/*.scrolled_panel()*/);

            size_t i = choices.size();
            while (i < num_extruders)
            {
                PlaterPresetComboBox* choice/*{ nullptr }*/;
                sidebar().init_filament_combo(&choice, i);
                choices.push_back(choice);

                // initialize selection
                choice->update();
                ++i;
            }

            // remove unused choices if any
            sidebar().remove_unused_filament_combos(num_extruders);

            sidebar().Layout();
            sidebar().scrolled_panel()->Refresh();
#endif     
        }

        bool Plater::update_filament_colors_in_full_config()
        {
            // There is a case, when we use filament_color instead of extruder_color (when extruder_color == "").
            // Thus plater config option "filament_colour" should be filled with filament_presets values.
            // Otherwise, on 3dScene will be used last edited filament color for all volumes with extruder_color == "".
            const std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
            // add by allen for fixing use filament_colour instead of extruder_colour
            if (/*filament_presets.size() == 1 || */!p->config->has("filament_colour"))
                return false;

            const PresetCollection& filaments = wxGetApp().preset_bundle->filaments;
            std::vector<std::string> filament_colors;
            filament_colors.reserve(filament_presets.size());

            for (const std::string& filament_preset : filament_presets)
                filament_colors.push_back(filaments.find_preset(filament_preset, true)->config.opt_string("filament_colour", (unsigned)0));

            p->config->option<ConfigOptionStrings>("filament_colour")->values = filament_colors;

            // add by allen for add anker_filament_id and anker_colour_id
            if (!p->config->has("anker_filament_id") || !p->config->has("anker_colour_id"))
                return false;

            if (wxGetApp().mainframe && wxGetApp().mainframe->plater()) {
                std::vector<SFilamentInfo> filamentEditInfo = wxGetApp().mainframe->plater()->sidebarnew().getEditFilamentList();
                std::vector<std::string> ankerFilamentId, ankerColourId;
                for (auto iter : filamentEditInfo) {
                    ankerFilamentId.emplace_back(iter.wxStrFilamentId.ToStdString(wxConvUTF8));
                    ankerColourId.emplace_back(iter.wxStrColorId.ToStdString(wxConvUTF8));
                }

                p->config->option<ConfigOptionStrings>("anker_filament_id")->values = ankerFilamentId;
                p->config->option<ConfigOptionStrings>("anker_colour_id")->values = ankerColourId;
            }

            return true;
        }

        void Plater::on_config_change(const DynamicPrintConfig& config)
        {
            bool update_scheduled = false;
            bool bed_shape_changed = false;
            for (auto opt_key : p->config->diff(config)) {
                if (opt_key == "filament_colour") {
                    update_scheduled = true; // update should be scheduled (for update 3DScene) #2738

                    if (update_filament_colors_in_full_config()) {
                        p->sidebarnew->object_list()->update_extruder_colors();
                        continue;
                    }
                }
                if (opt_key == "material_colour") {
                    update_scheduled = true; // update should be scheduled (for update 3DScene)
                }

                p->config->set_key_value(opt_key, config.option(opt_key)->clone());
                if (opt_key == "printer_technology") {
                    this->set_printer_technology(config.opt_enum<PrinterTechnology>(opt_key));
                   // p->sidebar->show_sliced_info_sizer(false);
                    if (p->sidebarnew) {
                        //p->updatePreViewRightSideBar(false, CONFIG_CHANGE);
                        p->sidebarnew->updatePreviewBtn(false, CONFIG_CHANGE);
                    }
                    p->reset_gcode_toolpaths();
                    p->view3D->get_canvas3d()->reset_sequential_print_clearance();
                }
                else if (opt_key == "bed_shape" || opt_key == "bed_custom_texture" || opt_key == "bed_custom_model") {
                    bed_shape_changed = true;
                    update_scheduled = true;
                }
                else if (boost::starts_with(opt_key, "wipe_tower") ||
                    // opt_key == "filament_minimal_purge_on_wipe_tower" // ? #ys_FIXME
                    opt_key == "single_extruder_multi_material") {
                    update_scheduled = true;
                }
                else if (opt_key == "variable_layer_height") {
                    if (p->config->opt_bool("variable_layer_height") != true) {
                        p->view3D->enable_layers_editing(false);
                        p->view3D->set_as_dirty();
                    }
                }
                else if (opt_key == "extruder_colour") {
                    update_scheduled = true;
                    //p->sidebarnew->object_list()->update_extruder_colors();
                }
                else if (opt_key == "max_print_height") {
                    bed_shape_changed = true;
                    update_scheduled = true;
                }
                else if (opt_key == "printer_model") {
                    p->reset_gcode_toolpaths();
                    // update to force bed selection(for texturing)
                    bed_shape_changed = true;
                    update_scheduled = true;
                }
            }

            if (bed_shape_changed)
                set_bed_shape();

            if (update_scheduled)
                update();

            if (p->main_frame->is_loaded())
                this->p->schedule_background_process();
        }

        void Plater::set_bed_shape() const
        {
            set_bed_shape(p->config->option<ConfigOptionPoints>("bed_shape")->values,
                p->config->option<ConfigOptionFloat>("max_print_height")->value,
                p->config->option<ConfigOptionString>("bed_custom_texture")->value,
                p->config->option<ConfigOptionString>("bed_custom_model")->value);
        }

        void Plater::set_bed_shape(const Pointfs& shape, const double max_print_height, const std::string& custom_texture, const std::string& custom_model, bool force_as_custom) const
        {
            p->set_bed_shape(shape, max_print_height, custom_texture, custom_model, force_as_custom);
        }

        void Plater::set_default_bed_shape() const
        {
            set_bed_shape({ { 0.0, 0.0 }, { 200.0, 0.0 }, { 200.0, 200.0 }, { 0.0, 200.0 } }, 0.0, {}, {}, true);
        }

        BoundingBox Plater::get_bed_shape() const
        {
            return p->bed.build_volume().bounding_box();
        }

        void Plater::force_filament_colors_update()
        {
            bool update_scheduled = false;
            DynamicPrintConfig* config = p->config;
            const std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
            if (filament_presets.size() > 1 &&
                p->config->option<ConfigOptionStrings>("filament_colour")->values.size() == filament_presets.size())
            {
                const PresetCollection& filaments = wxGetApp().preset_bundle->filaments;
                std::vector<std::string> filament_colors;
                filament_colors.reserve(filament_presets.size());

                for (const std::string& filament_preset : filament_presets)
                    filament_colors.push_back(filaments.find_preset(filament_preset, true)->config.opt_string("filament_colour", (unsigned)0));

                if (config->option<ConfigOptionStrings>("filament_colour")->values != filament_colors) {
                    config->option<ConfigOptionStrings>("filament_colour")->values = filament_colors;
                    update_scheduled = true;
                }
            }

            if (update_scheduled) {
                update();
                p->sidebarnew->object_list()->update_extruder_colors();
                //p->objectbar->update_objects_list_extruder_column(p->config->option<ConfigOptionStrings>("filament_colour")->values.size());
            }

            if (p->main_frame->is_loaded())
                this->p->schedule_background_process();
        }

        void Plater::force_print_bed_update()
        {
            // Fill in the printer model key with something which cannot possibly be valid, so that Plater::on_config_change() will update the print bed
            // once a new Printer profile config is loaded.
            p->config->opt_string("printer_model", true) = "\x01\x00\x01";
        }

        void Plater::on_activate()
        {
            this->p->show_delayed_error_message();
        }

        void Plater::on_move(wxMoveEvent& event)
        {
            if (this->p->objectbar)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->objectbar->getObjectBarView()->SetPosition(topleft + wxPoint(12, 12));

                int newMaxHeight = GetSize().GetHeight();
                //this->p->objectbar->setListMaxHeight(newMaxHeight / 2.0);
            }

            if (this->p->floatinglist)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->floatinglist->SetPosition(topleft + wxPoint(12 + this->p->objectbar->getObjectBarView()->GetSize().x, 12 + 50));
            }

            if(p && p->preview)
                p->preview->on_move(event);


            if (p && p->sidebarnew)
                p->sidebarnew->onMove(event);

            if (!isDragGcode()) {
                ShowHintDialogs(true);
            }

        }

        void Plater::on_show(wxShowEvent& event)
        {
            // show objectbar after the canvas initialized
            if (this->p->objectbar && p->view3D->get_canvas3d()->is_initialized() && wxGetApp().mainframe)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->objectbar->getObjectBarView()->SetPosition(topleft + wxPoint(12, 12));
                
                bool objectbarVisible = event.IsShown() && wxGetApp().is_editor() && p->current_view_mode == VIEW_MODE_3D && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE;
                //this->p->objectbar->getObjectBarView()->Show(objectbarVisible);

                int newMaxHeight = GetSize().GetHeight(); 
                //this->p->objectbar->setListMaxHeight(newMaxHeight / 2.0);
            }

            if (this->p->floatinglist && this->p->objectbar && wxGetApp().mainframe)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->floatinglist->SetPosition(topleft + wxPoint(12 + this->p->objectbar->getObjectBarView()->GetSize().x, 12 + 50));
                bool visible = event.IsShown() && wxGetApp().is_editor() && p->current_view_mode == VIEW_MODE_3D && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE;
                /*if (!visible)
                    this->p->floatinglist->Show(false);*/
            }

            if (event.IsShown() && is_preview_shown() && is_preview_loaded() && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE) {
                this->p->preview->showGcodeLayerToolbar(true);
            }
            else {
                this->p->preview->showGcodeLayerToolbar(false);
                if (p->previewRightSidePanel) {
                    //p->previewRightSidePanel->UpdateGcodePreviewSideBar(true, PLATER_TAB_HIDE);
                    p->sidebarnew->updatePreviewBtn(true, PLATER_TAB_HIDE);
                }
                // Close the filament edit dialog when plater is toggled hidden
                if (p && p->sidebarnew) {
                    p->sidebarnew->closeFilamentEditDlg();
                }
            }
            if (!isDragGcode()) {
                ShowHintDialogs(true);
            }
        }

        void Plater::on_idle(wxIdleEvent& evt)
        {
            // auto adjust toolbar position so that the toolbar not overlap the first notification msg (NotificationManager::get_notifications_top())
            if (this->p && this->p->preview && this->IsShown() && is_preview_shown() && is_preview_loaded() && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE && wxGetApp().mainframe->IsIconized() == false) {
                this->p->preview->CalGcodePreviewToolbarPos();
            }

            evt.Skip();
        }


        void Plater::updateMatchHint()
        {
            if (p && p->m_machineTypeMatchHint) {
                if (wxGetApp().mainframe->get_current_tab_mode() != TabMode::TAB_SLICE) {
                    p->m_machineTypeMatchHint->Show(false);
                }
                else {
                    wxPoint mfPoint = wxGetApp().mainframe->GetScreenPosition();
                    wxRect mfRect = wxGetApp().mainframe->GetScreenRect();
                    wxPoint sidebarnewPos = p->sidebarnew->GetScreenPosition();
                    wxRect sidebarnewRect = p->sidebarnew->GetScreenRect();

                    wxSize machineSize = p->m_machineTypeMatchHint->GetSize();
                    //if exist matchHint, add later the 0.2mm nozzle's printer hintdialog
                    if (isExist02mmPrinterHint()) {
                        wxSize printerSize = p->m_02mmPrinterHint->GetSize();
                        wxPoint machineHintPos(mfPoint.x + mfRect.GetWidth() - sidebarnewRect.GetWidth() - machineSize.GetWidth() - AnkerLength(26),
                            sidebarnewPos.y + printerSize.GetHeight() + AnkerLength(79));
                        p->m_machineTypeMatchHint->SetPosition(machineHintPos);
                    } else {
                        wxPoint printerHintPos(mfPoint.x + mfRect.GetWidth() - sidebarnewRect.GetWidth() - machineSize.GetWidth() - AnkerLength(26),
                            sidebarnewPos.y + AnkerLength(72));
                        p->m_machineTypeMatchHint->SetPosition(printerHintPos);
                    }

                    bool objectbarVisible = IsShown() && wxGetApp().is_editor() && p->current_view_mode == VIEW_MODE_3D;
                    SetExistMatchHint(objectbarVisible);
                    p->m_machineTypeMatchHint->Show(objectbarVisible);
                    if (objectbarVisible) {
                        ReplaceOldHintDialog(MatchHintType, p->m_machineTypeMatchHint);
                    } else {
                        DelSpecifyHintDialog(MatchHintType);
                    }
                }
            }
        }

        void Plater::update02mmPrinter()
        {
            if (p && p->m_02mmPrinterHint) {
                if (wxGetApp().mainframe->get_current_tab_mode() != TabMode::TAB_SLICE) {
                    p->m_02mmPrinterHint->Show(false); 
                    DelSpecifyHintDialog(PrinterHintType);
                } else {
                    wxPoint mfPoint = wxGetApp().mainframe->GetScreenPosition();
                    wxRect mfRect = wxGetApp().mainframe->GetScreenRect();
                    wxPoint sidebarnewPos = p->sidebarnew->GetScreenPosition();
                    wxRect sidebarnewRect = p->sidebarnew->GetScreenRect();

                    wxSize printerHintSize = p->m_02mmPrinterHint->GetSize();

                    //if exist matchHint, add later the 0.2mm nozzle's printer hintdialog
                    if (isExistMatchHint()) {
                        wxSize machineSize = p->m_machineTypeMatchHint->GetSize();
                        wxPoint printerHintPos(mfPoint.x + mfRect.GetWidth() - sidebarnewRect.GetWidth() - printerHintSize.GetWidth() - AnkerLength(26),
                            sidebarnewPos.y + machineSize.GetHeight() + AnkerLength(77));
                        p->m_02mmPrinterHint->SetPosition(printerHintPos);
                    } else {
                        wxPoint printerHintPos(mfPoint.x + mfRect.GetWidth() - sidebarnewRect.GetWidth() - printerHintSize.GetWidth() - AnkerLength(26),
                            sidebarnewPos.y + AnkerLength(72));
                        p->m_02mmPrinterHint->SetPosition(printerHintPos);
                    }
                    
                    if (mIs02mmPrinterFlag) {
                        ReplaceOldHintDialog(PrinterHintType, p->m_02mmPrinterHint);
                        ShowHintDialogs(true); wxGetApp().plater()->Layout();
                        SetExist02mmPrinterHint(true);
                    } else {
                        p->m_02mmPrinterHint->Show(false);
                        DelSpecifyHintDialog(PrinterHintType);
                        SetExist02mmPrinterHint(false);
                    }
                    
                }
            }
        }

        void Plater::HintFor02mmPrinter(const std::string& printerName) 
        {
            auto appConfig = Slic3r::GUI::wxGetApp().app_config;
            if (appConfig && appConfig->get_bool("Print Check", "machine_nozzle_limit_nohint")) {
                ANKER_LOG_INFO << "02mmPrinter  nohint for user set.";
                return;
            }
            
            auto show_hints = [&]() {
                if (auto config = &wxGetApp().preset_bundle->printers.get_selected_preset().config) {
                    if (auto options = static_cast<const ConfigOptionFloats*>(config->option("nozzle_diameter"))) {
                        if (!options->values.empty()) {
                            auto nozzle_value = options->values.front();
                            if (fabs(nozzle_value - 0.2 ) < EPSILON) {
                                return true;
                            }
                        }
                    }
               }
                            
                return false;
            };
            
            mIs02mmPrinterFlag = show_hints();
            if (!mIs02mmPrinterFlag) {
                ANKER_LOG_INFO << "it is not 02mm Printer.";
                if(p->m_02mmPrinterHint == nullptr){
                    ANKER_LOG_INFO << "p->m_02mmPrinterHint == nullptr."; 
                } else {
                    p->m_02mmPrinterHint->Show(false);
                    DelSpecifyHintDialog(PrinterHintType);
                }
                
                return;
            }

            if (p->m_02mmPrinterHint == nullptr) {
                p->m_02mmPrinterHint = new AnkerHint(_L("common_nozzle_hint_text"), p->q);

                p->m_02mmPrinterHint->SetCallBack([=](bool isChecked) {
                    DelSpecifyHintDialog(PrinterHintType);
                    SetExist02mmPrinterHint(false);

                    if (!isDragGcode()) {
                        // Find the other ankerhint and move it up   
                        ShowHintDialogs(true);  p->m_02mmPrinterHint->Layout(); /*p->m_02mmPrinterHint->Refresh();*/
                    }
                    p->m_02mmPrinterHint = nullptr;
                    if (appConfig) {
                        appConfig->set("Print Check", "machine_nozzle_limit_nohint", isChecked ? "1" : "0");
                    }
                });
                wxSize uiSize(268, 189);
                p->m_02mmPrinterHint->InitUI(uiSize.GetWidth(), uiSize.GetHeight());
                p->q->update02mmPrinter();
            } else {
                p->q->update02mmPrinter();
            }
        }

        void Plater::DelSpecifyHintDialog(int hintTypeId) 
        {
            auto it = std::find_if(m_HintDialogs.begin(), m_HintDialogs.end(),
                [=](const std::pair<int, AnkerHint*>& p) { return p.first == hintTypeId; });
            if (it != m_HintDialogs.end()) {
                it = m_HintDialogs.erase(it);
            }

            ShowHintDialogs(true);
        }

        void Plater::ReplaceOldHintDialog(int hintTypeId, AnkerHint* hint)
        {
            auto it = std::find_if(m_HintDialogs.begin(), m_HintDialogs.end(),
                [=](const std::pair<int, AnkerHint*>& p) { return p.first == hintTypeId; });
            if (it != m_HintDialogs.end()) {
                it = m_HintDialogs.erase(it);
            }
            m_HintDialogs.push_back(std::make_pair(hintTypeId, hint));

            ShowHintDialogs(true);
        }

        void Plater::ShowHintDialogs(bool flag) 
        {
            int i = 0, offLen = 0;
            for (const auto [id, hint] : m_HintDialogs) {
                if (p && hint) {
                    if (wxGetApp().mainframe->get_current_tab_mode() != TabMode::TAB_SLICE) {
                        hint->Show(false);
                    } else {
                        wxPoint mfPoint = wxGetApp().mainframe->GetScreenPosition();
                        wxRect mfRect = wxGetApp().mainframe->GetScreenRect();
                        wxPoint sidebarnewPos = p->sidebarnew->GetScreenPosition();
                        wxRect sidebarnewRect = p->sidebarnew->GetScreenRect();
                        wxSize hintSize = hint->GetSize();
                        offLen = i * (hintSize.GetHeight()+ AnkerLength(17)); ++i;
                        wxPoint hintPos(mfPoint.x + mfRect.GetWidth() - sidebarnewRect.GetWidth() - hintSize.GetWidth() - AnkerLength(26),
                            sidebarnewPos.y + offLen + AnkerLength(72));
                        hint->SetPosition(hintPos);
                        hint->Show(flag);
                    }
                }
            }
        }

        void Plater::on_size(wxSizeEvent& event)
        {
            if (this->p->objectbar)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->objectbar->getObjectBarView()->SetPosition(topleft + wxPoint(12, 12));

                int newMaxHeight = event.GetSize().GetHeight();
                //this->p->objectbar->setListMaxHeight(newMaxHeight / 2.0);
            }

            if (this->p->floatinglist)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->floatinglist->SetPosition(topleft + wxPoint(12 + this->p->objectbar->getObjectBarView()->GetSize().x, 12 + 50));
            }

            if (p && p->sidebarnew)
                p->sidebarnew->onSize(event);
            if (!isDragGcode()) {
                ShowHintDialogs(true);
            }
        }

        void Plater::on_minimize(wxIconizeEvent& event)
        {
            if (wxGetApp().mainframe)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->objectbar->getObjectBarView()->SetPosition(topleft + wxPoint(12, 12));
                bool objectbarVisible = this->IsShown() && !event.IsIconized() && wxGetApp().is_editor() && 
                    p->current_view_mode == VIEW_MODE_3D && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE;
                //this->p->objectbar->getObjectBarView()->Show(objectbarVisible);
                
                if (!isDragGcode()) {
                    if (p->m_machineTypeMatchHint) {
                        p->m_machineTypeMatchHint->Show(objectbarVisible);
                    }

                    if (p->m_02mmPrinterHint) {
                        p->m_02mmPrinterHint->Show(objectbarVisible);
                    }
                }
                
                int newMaxHeight = GetSize().GetHeight();
                //this->p->objectbar->setListMaxHeight(newMaxHeight / 2.0);
            }

            if (this->p->floatinglist && wxGetApp().mainframe)
            {
                wxPoint topleft = GetScreenPosition();
                //this->p->floatinglist->SetPosition(topleft + wxPoint(12 + this->p->objectbar->getObjectBarView()->GetSize().x, 12 + 50));

                bool objectbarVisible = this->IsShown() && !event.IsIconized() && wxGetApp().is_editor() && p->current_view_mode == VIEW_MODE_3D && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE;
                if (!objectbarVisible)
                    this->p->floatinglist->Show(false);
            }

            if (this->IsShown() && !event.IsIconized() && is_preview_shown() && is_preview_loaded() && wxGetApp().mainframe->get_current_tab_mode() == TabMode::TAB_SLICE) {
                this->p->preview->showGcodeLayerToolbar(true);
            }
            else {
                this->p->preview->showGcodeLayerToolbar(false);
            }

            if (p && p->sidebarnew)
                p->sidebarnew->onMinimize(event);
        }

        void Plater::on_maximize(wxMaximizeEvent& event) {
            if (p && p->sidebarnew)
                p->sidebarnew->onMaximize(event);
            if (!isDragGcode()) {
                ShowHintDialogs(true);
            }
        }

        void Plater::shutdown()
        {
            if (p && p->preview)
            {
                p->preview->shutdown();
            }

            if (p && p->sidebarnew)
                p->sidebarnew->shutdown();
        }

        // Get vector of extruder colors considering filament color, if extruder color is undefined.
        std::vector<std::string> Plater::get_extruder_colors_from_plater_config(const GCodeProcessorResult* const result) const
        {
            if (wxGetApp().is_gcode_viewer() && result != nullptr)
                return result->extruder_colors;
            else {
                const Slic3r::DynamicPrintConfig* config = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
                std::vector<std::string> extruder_colors;
                if (!config->has("extruder_colour")) // in case of a SLA print
                    return extruder_colors;

                extruder_colors = (config->option<ConfigOptionStrings>("extruder_colour"))->values;
                if (!wxGetApp().plater())
                    return extruder_colors;

                const std::vector<std::string>& filament_colours = (p->config->option<ConfigOptionStrings>("filament_colour"))->values;
                for (size_t i = 0; i < extruder_colors.size(); ++i)
                    if (extruder_colors[i] == "" && i < filament_colours.size())
                        extruder_colors[i] = filament_colours[i];

                return extruder_colors;
            }
        }

        std::vector<std::string> Plater::get_filament_colors_from_plater_config(const GCodeProcessorResult* const result) const
        {
            std::vector<std::string> extruder_colors;

            const std::vector<SFilamentInfo>& editFilaments = p->sidebarnew->getEditFilamentList();
            for (int i = 0; i < editFilaments.size(); i++)
            {
                extruder_colors.push_back(editFilaments[i].wxStrColor.ToStdString(wxConvUTF8));
            }

            return extruder_colors;
        }

        /* Get vector of colors used for rendering of a Preview scene in "Color print" mode
         * It consists of extruder colors and colors, saved in model.custom_gcode_per_print_z
         */
        std::vector<std::string> Plater::get_colors_for_color_print(const GCodeProcessorResult* const result) const
        {
            // add by allen for fixing use filament_colour instead of extruder_colour
            //std::vector<std::string> colors = get_extruder_colors_from_plater_config(result);
            std::vector<std::string> colors = get_filament_colors_from_plater_config(result);
            colors.reserve(colors.size() + p->model.custom_gcode_per_print_z.gcodes.size());

            if (wxGetApp().is_gcode_viewer() && result != nullptr) {
                for (const CustomGCode::Item& code : result->custom_gcode_per_print_z) {
                    if (code.type == CustomGCode::ColorChange)
                        colors.emplace_back(code.color);
                }
            }
            else {
                for (const CustomGCode::Item& code : p->model.custom_gcode_per_print_z.gcodes) {
                    if (code.type == CustomGCode::ColorChange)
                        colors.emplace_back(code.color);
                }
            }

            return colors;
        }

        wxString Plater::get_project_filename(const wxString& extension) const
        {
            return p->get_project_filename(extension);
        }

        void Plater::set_project_filename(const wxString& filename)
        {
            p->set_project_filename(filename);
        }

        bool Plater::is_export_gcode_scheduled() const
        {
            return p->background_process.is_export_scheduled();
        }

        const Selection& Plater::get_selection() const
        {
            return p->get_selection();
        }

        int Plater::get_selected_object_idx()
        {
            return p->get_selected_object_idx();
        }

        bool Plater::is_single_full_object_selection() const
        {
            return p->get_selection().is_single_full_object();
        }

        GLCanvas3D* Plater::canvas3D()
        {
            return p->view3D->get_canvas3d();
            //return p->get_current_canvas3D();
        }

        const GLCanvas3D* Plater::canvas3D() const
        {
            return p->view3D->get_canvas3d();
            //return p->get_current_canvas3D();
        }

        GLCanvas3D* Plater::canvas_preview()
        {
            if (!p || !(p->preview))
                return nullptr;
            return p->preview->get_canvas3d();
        }

        GLCanvas3D* Plater::get_current_canvas3D()
        {
            return p->get_current_canvas3D();
        }

        GLCanvas3D* Plater::get_assmeble_canvas3D()
        {
            if (p->assemble_view)
                return p->assemble_view->get_canvas3d();
            return nullptr;
        }
        static std::string concat_strings(const std::set<std::string>& strings,
            const std::string& delim = "\n")
        {
            return std::accumulate(
                strings.begin(), strings.end(), std::string(""),
                [delim](const std::string& s, const std::string& name) {
                    return s + name + delim;
                });
        }

        GLCanvas3D* Plater::get_view3D_canvas3D()
        {
            return p->view3D->get_canvas3d();
        }

        GCodeProcessorResultExt* Plater::get_gcode_info()
        {
            if (p && p->preview)
              return p->preview->GetGcodeInfo() ;
            return nullptr;
        }


        void Plater::arrange()
        {
            if (p->can_arrange()) {
                auto& w = get_ui_job_worker();
                p->take_snapshot(_L("Arrange"));
                replace_job(w, std::make_unique<ArrangeJob>());
            }
        }

        void Plater::auto_bed()
        {
            if (p->can_auto_bed()) {
                auto& w = get_ui_job_worker();
                p->take_snapshot(_L("Auto bed"));
                replace_job(w, std::make_unique<OrientJob>(this));
           }
        }

        void Plater::set_current_canvas_as_dirty()
        {
            p->set_current_canvas_as_dirty();
        }

        void Plater::unbind_canvas_event_handlers()
        {
            p->unbind_canvas_event_handlers();
        }

        void Plater::reset_canvas_volumes()
        {
            p->reset_canvas_volumes();
        }

        PrinterTechnology Plater::printer_technology() const
        {
            return p->printer_technology;
        }

        const DynamicPrintConfig* Plater::config() const { return p->config; }

        bool Plater::set_printer_technology(PrinterTechnology printer_technology)
        {
            p->printer_technology = printer_technology;
            bool ret = p->background_process.select_technology(printer_technology);
            if (ret) {
                // Update the active presets.
            }
            //FIXME for SLA synchronize
            //p->background_process.apply(Model)!

            if (printer_technology == ptSLA) {
                for (ModelObject* model_object : p->model.objects) {
                    model_object->ensure_on_bed();
                }
            }

            //p->label_btn_export = printer_technology == ptFFF ? L("Export G-code") : L("Export");
           // p->label_btn_export = printer_technology == ptFFF ? L("Export") : L("Export");
          //  p->label_btn_send = printer_technology == ptFFF ? L("Send G-code") : L("Send to printer");

            if (wxGetApp().mainframe != nullptr)
                wxGetApp().mainframe->update_menubar();

            p->update_main_toolbar_tooltips();

            p->notification_manager->set_fff(printer_technology == ptFFF);
            p->notification_manager->set_slicing_progress_hidden();

            return ret;
        }

        void Plater::clear_before_change_mesh(int obj_idx, const std::string& notification_msg)
        {
            ModelObject* mo = model().objects[obj_idx];

            // If there are custom supports/seams/mmu segmentation, remove them. Fixed mesh
            // may be different and they would make no sense.
            bool paint_removed = false;
            for (ModelVolume* mv : mo->volumes) {
                paint_removed |= !mv->supported_facets.empty() || !mv->seam_facets.empty() || !mv->mmu_segmentation_facets.empty();
                mv->supported_facets.reset();
                mv->seam_facets.reset();
                mv->mmu_segmentation_facets.reset();
            }
            if (paint_removed) {
                // snapshot_time is captured by copy so the lambda knows where to undo/redo to.
                get_notification_manager()->push_notification(
                    NotificationType::CustomSupportsAndSeamRemovedAfterRepair,
                    NotificationManager::NotificationLevel::PrintInfoNotificationLevel,
                    notification_msg);
                //                    _u8L("Undo the repair"),
                //                    [this, snapshot_time](wxEvtHandler*){
                //                        // Make sure the snapshot is still available and that
                //                        // we are in the main stack and not in a gizmo-stack.
                //                        if (undo_redo_stack().has_undo_snapshot(snapshot_time)
                //                         && q->canvas3D()->get_gizmos_manager().get_current() == nullptr)
                //                            undo_redo_to(snapshot_time);
                //                        else
                //                            notification_manager->push_notification(
                //                                NotificationType::CustomSupportsAndSeamRemovedAfterRepair,
                //                                NotificationManager::NotificationLevel::RegularNotificationLevel,
                //                                _u8L("Cannot undo to before the mesh repair!"));
                //                        return true;
                //                    });
            }
        }

        void Plater::changed_mesh(int obj_idx)
        {
            ModelObject* mo = model().objects[obj_idx];
            sla::reproject_points_and_holes(mo);
            update();
            p->object_list_changed();
            p->schedule_background_process();
        }

        void Plater::changed_object(ModelObject* object)
        {
            if (object == nullptr) {
                return;
            }
            sla::reproject_points_and_holes(object);
            update();
            p->object_list_changed();
            p->schedule_background_process();
        }

        void Plater::changed_object(int obj_idx)
        {
            if (obj_idx < 0)
                return;
            // recenter and re - align to Z = 0
            p->model.objects[obj_idx]->ensure_on_bed(p->printer_technology != ptSLA);
            if (this->p->printer_technology == ptSLA) {
                // Update the SLAPrint from the current Model, so that the reload_scene()
                // pulls the correct data, update the 3D scene.
                this->p->update_restart_background_process(true, false);
            }
            else
                p->view3D->reload_scene(false);

            // update print
            this->p->schedule_background_process();

            this->p->object_list_changed();
        }

        void Plater::changed_objects(const std::vector<size_t>& object_idxs)
        {
            if (object_idxs.empty())
                return;

            for (size_t obj_idx : object_idxs) {
                if (obj_idx < p->model.objects.size()) {
                    if (p->model.objects[obj_idx]->min_z() >= SINKING_Z_THRESHOLD)
                        // re - align to Z = 0
                        p->model.objects[obj_idx]->ensure_on_bed();
                }
            }
            if (this->p->printer_technology == ptSLA) {
                // Update the SLAPrint from the current Model, so that the reload_scene()
                // pulls the correct data, update the 3D scene.
                this->p->update_restart_background_process(true, false);
            }
            else {
                p->view3D->reload_scene(false);
                p->view3D->get_canvas3d()->update_instance_printable_state_for_objects(object_idxs);
            }

            // update print
            this->p->schedule_background_process();

            this->p->object_list_changed();
        }

        void Plater::schedule_background_process(bool schedule/* = true*/)
        {
            if (schedule)
                this->p->schedule_background_process();

            this->p->suppressed_backround_processing_update = false;
        }

        bool Plater::is_background_process_update_scheduled() const
        {
            return this->p->background_process_timer.IsRunning();
        }

        void Plater::suppress_background_process(const bool stop_background_process)
        {
            if (stop_background_process)
                this->p->background_process_timer.Stop();

            this->p->suppressed_backround_processing_update = true;
        }

        bool Plater::background_process_running()
        {
            return this->p->background_process.running();
        }

        void Plater::mirror(Axis axis) { p->mirror(axis); }
        void Plater::split_object() { p->split_object(); }
        void Plater::split_volume() { p->split_volume(); }
        void Plater::update_menus() { p->menus.update(); }
        void Plater::show_action_buttons(const bool ready_to_slice) const { p->show_action_buttons(ready_to_slice); }
        void Plater::show_action_buttons() const { p->show_action_buttons(p->ready_to_slice); }

        void Plater::copy_selection_to_clipboard()
        {
            // At first try to copy selected values to the ObjectList's clipboard
            // to check if Settings or Layers are selected in the list
            // and then copy to 3DCanvas's clipboard if not
            //if (can_copy_to_clipboard() && !p->objectbar->getObjectList()->copy_to_clipboard())
            if (can_copy_to_clipboard() && !p->sidebarnew->object_list()->copy_to_clipboard())
                p->view3D->get_canvas3d()->get_selection().copy_to_clipboard();
        }

        void Plater::paste_from_clipboard()
        {
            if (!can_paste_from_clipboard())
                return;

            Plater::TakeSnapshot snapshot(this, _L("Paste From Clipboard"));

            // At first try to paste values from the ObjectList's clipboard
            // to check if Settings or Layers were copied
            // and then paste from the 3DCanvas's clipboard if not
            //if (!p->objectbar->getObjectList()->paste_from_clipboard())
            if (!p->sidebarnew->object_list()->paste_from_clipboard())
                p->view3D->get_canvas3d()->get_selection().paste_from_clipboard();
        }

        void Plater::fill_color(int extruder_id)
        {
            if (can_fillcolor()) {
                p->assemble_view->get_canvas3d()->get_selection().fill_color(extruder_id);
            }
        }

        void Plater::search(bool plater_is_active, wxString defaultSearchData, Preset::Type type )
        {
            if (plater_is_active) {
                if (is_preview_shown())
                    return;
                // plater should be focused for correct navigation inside search window 
                this->SetFocus();

                wxKeyEvent evt;
#ifdef __APPLE__
                evt.m_keyCode = 'f';
#else /* __APPLE__ */
                evt.m_keyCode = WXK_CONTROL_F;
#endif /* __APPLE__ */
                evt.SetControlDown(true);
                canvas3D()->on_char(evt);
            }
            else {
                // add by allen for ankerCfgDlg search
                /* p->sidebar->check_and_update_searcher(true);
                 p->sidebar->get_searcher().show_dialog();*/

                p->sidebarnew->check_and_update_searcher(true, type);
                p->sidebarnew->get_searcher().show_dialog(defaultSearchData);
            }
        }

        void Plater::msw_rescale()
        {
            if (p->preview)
                p->preview->msw_rescale();

            if (p->view3D)
                p->view3D->get_canvas3d()->msw_rescale();

            p->sidebarnew->object_list()->msw_rescale();
            //if (p->objectbar)
            //    p->objectbar->msw_rescale();

            Layout();
            GetParent()->Layout();
        }

        void Plater::sys_color_changed()
        {
            p->preview->sys_color_changed();
            //p->sidebar->sys_color_changed();
            p->sidebarnew->object_list()->sys_color_changed();
            //p->objectbar->sys_color_changed();

            p->menus.sys_color_changed();

            Layout();
            GetParent()->Layout();
        }

        bool Plater::init_view_toolbar()
        {
            return p->init_view_toolbar();
        }

        void Plater::enable_view_toolbar(bool enable)
        {
            p->view_toolbar.set_enabled(enable);
        }

        bool Plater::init_collapse_toolbar()
        {
            return p->init_collapse_toolbar();
        }

        void Plater::enable_collapse_toolbar(bool enable)
        {
            p->collapse_toolbar.set_enabled(enable);
        }

        void Plater::setAKeyPrintSlicerTempGcodePath(std::string gcodePath)
        {
            std::string current_gcode_file = gcodePath;
            ANKER_LOG_INFO << "set currentPrintGcodeFile: " << current_gcode_file;
            m_currentPrintGcodeFile = current_gcode_file;
        }

        std::string Plater::get_temp_gcode_output_path()
        {
            return p->background_process.get_temp_output_path();
        }

        const Camera& Plater::get_camera() const
        {
            return p->camera;
        }

        Camera& Plater::get_camera()
        {
            return p->camera;
        }

#if ENABLE_ENVIRONMENT_MAP
        void Plater::init_environment_texture()
        {
            if (p->environment_texture.get_id() == 0)
                p->environment_texture.load_from_file(resources_dir() + "/icons/Pmetal_001.png", false, GLTexture::SingleThreaded, false);
        }

        unsigned int Plater::get_environment_texture_id() const
        {
            return p->environment_texture.get_id();
        }
#endif // ENABLE_ENVIRONMENT_MAP

        const BuildVolume& Plater::build_volume() const
        {
            return p->bed.build_volume();
        }

        const GLToolbar& Plater::get_view_toolbar() const
        {
            return p->view_toolbar;
        }

        GLToolbar& Plater::get_view_toolbar()
        {
            return p->view_toolbar;
        }

        const GLToolbar& Plater::get_collapse_toolbar() const
        {
            return p->collapse_toolbar;
        }

        GLToolbar& Plater::get_collapse_toolbar()
        {
            return p->collapse_toolbar;
        }

        void Plater::set_preview_layers_slider_values_range(int bottom, int top)
        {
            p->set_preview_layers_slider_values_range(bottom, top);
        }

        void Plater::update_preview_moves_slider()
        {
            p->update_preview_moves_slider();
        }

        void Plater::enable_preview_moves_slider(bool enable)
        {
            p->enable_preview_moves_slider(enable);
        }

        void Plater::update_current_ViewType(GCodeViewer::EViewType type)
        {
            p->UpdateCurrentViewType(type);
        }

        void Plater::reset_gcode_toolpaths()
        {
            p->reset_gcode_toolpaths();
        }

        const Mouse3DController& Plater::get_mouse3d_controller() const
        {
            return p->mouse3d_controller;
        }

        Mouse3DController& Plater::get_mouse3d_controller()
        {
            return p->mouse3d_controller;
        }

        NotificationManager* Plater::get_notification_manager()
        {
            return p->notification_manager.get();
        }

        const NotificationManager* Plater::get_notification_manager() const
        {
            return p->notification_manager.get();
        }

        void Plater::init_notification_manager()
        {
            p->init_notification_manager();
        }

        bool Plater::can_delete() const { return p->can_delete(); }
        bool Plater::can_delete_all() const { return p->can_delete_all(); }
        bool Plater::can_increase_instances() const { return p->can_increase_instances(); }
        bool Plater::can_decrease_instances(int obj_idx/* = -1*/) const { return p->can_decrease_instances(obj_idx); }
        bool Plater::can_set_instance_to_object() const { return p->can_set_instance_to_object(); }
        bool Plater::can_fix_through_netfabb() const { return p->can_fix_through_netfabb(); }
        bool Plater::can_simplify() const { return p->can_simplify(); }
        bool Plater::can_split_to_objects() const { return p->can_split_to_objects(); }
        bool Plater::can_split_to_volumes() const { return p->can_split_to_volumes(); }
        bool Plater::can_arrange() const { return p->can_arrange(); }
        bool Plater::can_auto_bed() const { return p->can_auto_bed(); }
        bool Plater::can_layers_editing() const { return p->can_layers_editing(); }
        bool Plater::can_paste_from_clipboard() const
        {
            const Selection& selection = p->view3D->get_canvas3d()->get_selection();
            const Selection::Clipboard& clipboard = selection.get_clipboard();

            if (clipboard.is_empty() && p->sidebarnew->object_list()->clipboard_is_empty())
                return false;

            if ((wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptSLA) && !clipboard.is_sla_compliant())
                return false;

            Selection::EMode mode = clipboard.get_mode();
            if ((mode == Selection::Volume) && !selection.is_from_single_instance())
                return false;

            if ((mode == Selection::Instance) && (selection.get_mode() != Selection::Instance))
                return false;

            return true;
        }

        bool Plater::can_copy_to_clipboard() const
        {
            if (is_selection_empty())
                return false;

            const Selection& selection = p->view3D->get_canvas3d()->get_selection();
            if ((wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptSLA) && !selection.is_sla_compliant())
                return false;

            return true;
        }

        bool Plater::can_undo() const { return p->undo_redo_stack().has_undo_snapshot(); }
        bool Plater::can_redo() const { return p->undo_redo_stack().has_redo_snapshot(); }
        bool Plater::can_reload_from_disk() const { return p->can_reload_from_disk(); }
        bool Plater::can_replace_with_stl() const { return p->can_replace_with_stl(); }
        bool Plater::can_mirror() const { return p->can_mirror(); }
        bool Plater::can_split(bool to_objects) const { return p->can_split(to_objects); }
        bool Plater::can_scale_to_print_volume() const { return p->can_scale_to_print_volume(); }

        //assmeble
        bool Plater::can_fillcolor() const { return p->can_fillcolor(); }
        bool Plater::has_assmeble_view() const { return p->has_assemble_view(); }

        const UndoRedo::Stack& Plater::undo_redo_stack_main() const { return p->undo_redo_stack_main(); }
        void Plater::clear_undo_redo_stack_main() { p->undo_redo_stack_main().clear(); }
        void Plater::enter_gizmos_stack() { p->enter_gizmos_stack(); }
        void Plater::leave_gizmos_stack() { p->leave_gizmos_stack(); }
        bool Plater::inside_snapshot_capture() { return p->inside_snapshot_capture(); }

        void Plater::toggle_render_statistic_dialog()
        {
            p->show_render_statistic_dialog = !p->show_render_statistic_dialog;
        }

        bool Plater::is_render_statistic_dialog_visible() const
        {
            return p->show_render_statistic_dialog;
        }

        void Plater::set_keep_current_preview_type(bool value)
        {
            p->preview->set_keep_current_preview_type(value);
        }

        Plater::TakeSnapshot::TakeSnapshot(Plater* plater, const std::string& snapshot_name)
            : TakeSnapshot(plater, from_u8(snapshot_name)) {}
        Plater::TakeSnapshot::TakeSnapshot(Plater* plater, const std::string& snapshot_name, UndoRedo::SnapshotType snapshot_type)
            : TakeSnapshot(plater, from_u8(snapshot_name), snapshot_type) {}


        // Wrapper around wxWindow::PopupMenu to suppress error messages popping out while tracking the popup menu.
        bool Plater::PopupMenu(wxMenu* menu, const wxPoint& pos)
        {
            // Don't want to wake up and trigger reslicing while tracking the pop-up menu.
            SuppressBackgroundProcessingUpdate sbpu;
            // When tracking a pop-up menu, postpone error messages from the slicing result.
            m_tracking_popup_menu = true;
            bool out = this->wxPanel::PopupMenu(menu, pos);
            m_tracking_popup_menu = false;
            if (!m_tracking_popup_menu_error_message.empty()) {
                // Don't know whether the CallAfter is necessary, but it should not hurt.
                // The menus likely sends out some commands, so we may be safer if the dialog is shown after the menu command is processed.
                wxString message = std::move(m_tracking_popup_menu_error_message);
                wxTheApp->CallAfter([message, this]() { show_error(this, message); });
                m_tracking_popup_menu_error_message.clear();
            }
            return out;
        }
        void Plater::bring_instance_forward()
        {
            p->bring_instance_forward();
        }

        wxMenu* Plater::object_menu()           { return p->menus.object_menu(); }
        wxMenu* Plater::part_menu()             { return p->menus.part_menu(); }
        wxMenu* Plater::text_part_menu()        { return p->menus.text_part_menu(); }
        wxMenu* Plater::svg_part_menu()         { return p->menus.svg_part_menu(); }
        wxMenu* Plater::sla_object_menu()       { return p->menus.sla_object_menu(); }
        wxMenu* Plater::default_menu()          { return p->menus.default_menu(); }
        wxMenu* Plater::instance_menu()         { return p->menus.instance_menu(); }
        wxMenu* Plater::layer_menu()            { return p->menus.layer_menu(); }
        wxMenu* Plater::multi_selection_menu()  { return p->menus.multi_selection_menu(); }


        wxBoxSizer* Plater::CreatePreViewRightSideBar() {
            return p->CreatePreViewRightSideBar();
        }

        std::string Plater::GetRightSidePanelUpdateReasonString(RightSidePanelUpdateReason reason)
        {
            std::string str = "";
            switch (reason)
            {
            case REASON_NONE:
                str = "REASON_NONE";
                break;
            case LOAD_GCODE_FILE_FOR_PREVIEW:
                str = "LOAD_GCODE_FILE_FOR_PREVIEW";
                break;
            case LOAD_ACODE_FILE_FOR_PREVIEW:
                str = "LOAD_ACODE_FILE_FOR_PREVIEW";
                break;
            case EXPORT_START:
                str = "EXPORT_START";
                break;
            case EXPORT_ACODE_COMPLETE:
                str = "EXPORT_ACODE_COMPLETE";
                break;
            case EXPORT_ACODE_CANCEL:
                str = "EXPORT_ACODE_CANCEL";
                break;
            case PROCCESS_GCODE_COMPLETE:
                str = "PROCCESS_GCODE_COMPLETE";
                break;
            case SLICING_CANCEL:
                str = "SLICING_CANCEL";
                break;
            case GCODE_INVALID:
                str = "GCODE_INVALID";
                break;
            case PRINT_PRESET_DATA_DIRTY:
                str = "PRINT_PRESET_DATA_DIRTY";
                break;
            case PRINTABLE_OBJ_CHANGE:
                str = "PRINTABLE_OBJ_CHANGE";
                break;
            case SELECT_VIEW_MODE_PREVIEW:
                str = "SELECT_VIEW_MODE_PREVIEW";
                break;
            case SELECT_VIEW_MODE_3D:
                str = "SELECT_VIEW_MODE_3D";
                break;
            case PLATER_TAB_HIDE:
                str = "PLATER_TAB_HIDE";
                break;
            case DELETE_ALL_OBJECT:
                str = "DELETE_ALL_OBJECT";
                break;
            case CONFIG_CHANGE:
                str = "CONFIG_CHANGE";
                break;
            default:
                str = "Unknown reason";
                break;
            }

            return str + "(" + std::to_string(reason) + ")";
        }

        void Plater::CalcModelObjectSize()
        {
            Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
            ModelObjectPtrs objects = this->model().objects;
            int obj_idx = selection.get_object_idx();
            int inst_idx = selection.get_instance_idx();

            ConfigOptionMode mode = wxGetApp().get_mode();
            if (mode < comExpert || objects.empty() || obj_idx < 0 || int(objects.size()) <= obj_idx ||
                inst_idx < 0 || int(objects[obj_idx]->instances.size()) <= inst_idx ||
                objects[obj_idx]->volumes.empty() ||                                            // hack to avoid crash when deleting the last object on the bed
                (selection.is_single_full_object() && objects[obj_idx]->instances.size() > 1) ||
                !(selection.is_single_full_instance() || selection.is_single_volume())) {
                //p->object_info->Show(false);
                return;
            }

            const ModelObject* model_object = objects[obj_idx];
            if (model_object == nullptr) {
                ANKER_LOG_INFO << "model_object == nullptr";
                return;
            }

            bool imperial_units = wxGetApp().app_config->get_bool("use_inches");
            double koef = imperial_units ? ObjectManipulation::mm_to_in : 1.0f;

            ModelVolume* vol = nullptr;
            Transform3d t;
            if (selection.is_single_volume()) {
                std::vector<int> obj_idxs, vol_idxs;
                wxGetApp().obj_list()->get_selection_indexes(obj_idxs, vol_idxs);
                if (vol_idxs.size() != 1)
                    // Case when this fuction is called between update selection in ObjectList and on Canvas
                    // Like after try to delete last solid part in object, the object is selected in ObjectLIst when just a part is still selected on Canvas
                    // see https://github.com/prusa3d/PrusaSlicer/issues/7408
                    return;
                vol = model_object->volumes[vol_idxs[0]];
                t = model_object->instances[inst_idx]->get_matrix() * vol->get_matrix();
            }

            Vec3d size = vol ? vol->mesh().transformed_bounding_box(t).size() : model_object->instance_bounding_box(inst_idx).size();
            setModelObjectSizeText(wxString::Format("%.2f X %.2f X %.2f mm", size(0) * koef, size(1) * koef, size(2) * koef));
        }

        SuppressBackgroundProcessingUpdate::SuppressBackgroundProcessingUpdate() :
            m_was_scheduled(wxGetApp().plater()->is_background_process_update_scheduled())
        {
            wxGetApp().plater()->suppress_background_process(m_was_scheduled);
        }

        SuppressBackgroundProcessingUpdate::~SuppressBackgroundProcessingUpdate()
        {
            wxGetApp().plater()->schedule_background_process(m_was_scheduled);
        }

        PlaterAfterLoadAutoArrange::PlaterAfterLoadAutoArrange()
        {
            Plater* plater = wxGetApp().plater();
            m_enabled = plater->model().objects.empty() &&
                plater->printer_technology() == ptFFF &&
                plater->fff_print().config().printer_model.value == "XL";
        }

        PlaterAfterLoadAutoArrange::~PlaterAfterLoadAutoArrange()
        {
            if (m_enabled)
                wxGetApp().plater()->arrange();
        }
    }
}    // namespace Slic3r::GUI



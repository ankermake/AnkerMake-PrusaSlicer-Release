#ifndef slic3r_Plater_hpp_
#define slic3r_Plater_hpp_

#include <memory>
#include <vector>
#include <boost/filesystem/path.hpp>

#include <wx/panel.h>

#include "Selection.hpp"

#include "GCodeViewer.hpp"

#include "libslic3r/enum_bitmask.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/calib.hpp"
#include "Jobs/Job.hpp"
#include "Jobs/Worker.hpp"
#include "Search.hpp"
#include "AnkerObjectBar.hpp"
#include "AnkerObjectManipulator.hpp"
#include "AnkerFloatingList.hpp"
#include "AnkerObjectLayers.hpp"
#include "AnkerSideBarNew.hpp"

#define ENABLE_AI 1

class wxButton;
class ScalableButton;
class wxScrolledWindow;
class wxString;
class AnkerChooseDeviceDialog;

wxDECLARE_EVENT(wxCUSTOMEVT_EXPORT_FINISHED_SAFE_QUIT_APP, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SLICE_FOR_CONMENT, wxCommandEvent);

namespace Slic3r {
    // use sidebar new
    #define USE_SIDEBAR_NEW 1
#ifndef __APPLE__
    #define SIDEBARNEW_WIDGET_WIDTH (420)
#else
    #define SIDEBARNEW_WIDGET_WIDTH (400)
#endif
    // show or hide old setting dialog
    #define SHOW_OLD_SETTING_DIALOG 0

    class BuildVolume;
    class Model;
    class ModelObject;
    enum class ModelObjectCutAttribute : int;
    using ModelObjectCutAttributes = enum_bitmask<ModelObjectCutAttribute>;
    class ModelInstance;
    class Print;
    class SLAPrint;
    enum PrintObjectStep : unsigned int;
    enum SLAPrintObjectStep : unsigned int;
    enum class ConversionType : int;

    using ModelInstancePtrs = std::vector<ModelInstance*>;

    namespace UndoRedo {
        class Stack;
        enum class SnapshotType : unsigned char;
        struct Snapshot;
    }

    namespace GUI {

        class MainFrame;
        class ConfigOptionsGroup;
        class ObjectManipulation;
        class ObjectSettings;
        class ObjectLayers;
        class ObjectList;
        class GLCanvas3D;
        class Mouse3DController;
        class NotificationManager;
        struct Camera;
        class GLToolbar;
        class PlaterPresetComboBox;

        using t_optgroups = std::vector <std::shared_ptr<ConfigOptionsGroup>>;
        using progressChangeCallbackFunction = std::function<void(float)>;

        class Plater;
        enum class ActionButtonType : int;

        enum ViewMode
        {
            VIEW_MODE_3D,
            VIEW_MODE_PREVIEW,
            VIEW_MODE_ASSEMBLE
        };

        enum RightSidePanelUpdateReason {
            REASON_NONE = 0,
            LOAD_GCODE_FILE_FOR_PREVIEW,
            LOAD_ACODE_FILE_FOR_PREVIEW,
            PRINTABLE_OBJ_CHANGE,
            EXPORT_START,
            EXPORT_ACODE_COMPLETE,
            EXPORT_ACODE_CANCEL,
            PROCCESS_GCODE_COMPLETE,
            SLICING_CANCEL,
            GCODE_INVALID,
            PRINT_PRESET_DATA_DIRTY,
            SELECT_VIEW_MODE_3D,
            SELECT_VIEW_MODE_PREVIEW,
            PLATER_TAB_HIDE,
            DELETE_ALL_OBJECT,
            CONFIG_CHANGE,
        };

        class Sidebar : public wxPanel
        {
            ConfigOptionMode    m_mode{ ConfigOptionMode::comSimple };
        public:
            Sidebar(Plater* parent);
            Sidebar(Sidebar&&) = delete;
            Sidebar(const Sidebar&) = delete;
            Sidebar& operator=(Sidebar&&) = delete;
            Sidebar& operator=(const Sidebar&) = delete;
            ~Sidebar();

            void init_filament_combo(PlaterPresetComboBox** combo, const int extr_idx);
            void remove_unused_filament_combos(const size_t current_extruder_count);
            void update_all_preset_comboboxes();
            void update_presets(Slic3r::Preset::Type preset_type);
            void update_mode_sizer() const;
            void change_top_border_for_mode_sizer(bool increase_border);
            void update_reslice_btn_tooltip() const;
            void msw_rescale();
            void sys_color_changed();
            void update_mode_markers();
            void search();
            void jump_to_option(size_t selected);
            void jump_to_option(const std::string& opt_key, Preset::Type type, const std::wstring& category);

            ObjectManipulation* obj_manipul();
            //ObjectList* obj_list();
            ObjectSettings* obj_settings();
            ObjectLayers* obj_layers();
            wxScrolledWindow* scrolled_panel();
            wxPanel* presets_panel();

            void OnUpdateProgressEvent(wxThreadEvent& event);
            void updateFileTransferProgressValue(int value);
            ConfigOptionsGroup* og_freq_chng_params(const bool is_fff);
            wxButton* get_wiping_dialog_button();
            //void                    update_objects_list_extruder_column(size_t extruders_count);
            void                    show_info_sizer();
            void                    show_sliced_info_sizer(const bool show);
            void                    update_sliced_info_sizer();
            void                    enable_buttons(bool enable);
            void                    set_btn_label(const ActionButtonType btn_type, const wxString& label) const;
            bool                    show_reslice(bool show) const;
            bool                    show_export(bool show) const;
            bool                    show_send(bool show) const;
            bool                    show_eject(bool show)const;
            bool                    show_export_removable(bool show) const;
            bool                    show_print_btn(bool show) const;
            bool                    show_send_progress(bool show) const;
            bool                    get_eject_shown() const;
            bool                    is_multifilament();
            void                    update_mode();
            bool                    is_collapsed();
            void                    collapse(bool collapse);
            void                    check_and_update_searcher(bool respect_mode = false);
            void                    update_ui_from_settings();

#ifdef _MSW_DARK_MODE
            void                    show_mode_sizer(bool show);
#endif

            std::vector<PlaterPresetComboBox*>& combos_filament();
            Search::OptionsSearcher& get_searcher();
            std::string& get_search_line();

            int                     getExportGCodeHeight();

        private:
            struct priv;
            std::unique_ptr<priv> p;
        };
        class Plater : public wxPanel
        {
        public:
            using fs_path = boost::filesystem::path;
            

            Plater(wxWindow* parent, MainFrame* main_frame);
            Plater(Plater&&) = delete;
            Plater(const Plater&) = delete;
            Plater& operator=(Plater&&) = delete;
            Plater& operator=(const Plater&) = delete;
            ~Plater();

            bool is_project_dirty() const;
            bool is_presets_dirty() const;
            void update_project_dirty_from_presets();
            int  save_project_if_dirty(const wxString& reason);
            void reset_project_dirty_after_save();
            void reset_project_dirty_initial_presets();
#if ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW
            void render_project_state_debug_window() const;
#endif // ENABLE_PROJECT_DIRTY_STATE_DEBUG_WINDOW

            bool is_project_temp() const;

            Sidebar& sidebar();
            AnkerSidebarNew& sidebarnew();
            AnkerObjectBar* objectbar();
            AnkerObjectManipulator* aobject_manipulator();
            AnkerFloatingList* floatinglist();
            AnkerObjectLayers* object_layers();
            const Model& model() const;
            Model& model();
            const Print& fff_print() const;
            Print& fff_print();
            const SLAPrint& sla_print() const;
            SLAPrint& sla_print();

            ///web download
            void request_model_download(std::string url);

            bool new_project(wxString project_name = "");
            void load_project();
            void load_project(const wxString& filename);
            void add_model(bool imperial_units = false);
            void import_zip_archive();
            void import_sl1_archive();

            void cut_horizontal(size_t obj_idx, size_t instance_idx, double z, ModelObjectCutAttributes attributes);
            void apply_cut_object_to_model(size_t obj_idx, const ModelObjectPtrs& new_objects);
            void orient();

            void extract_config_from_project();
            void load_gcode();
            void load_gcode(const wxString& filename);
            void reload_gcode_from_disk();
            void refresh_print();
            void setStarConmentFlagsTimes(const int& sliceTimes);

            static std::string get_acode_extract_path();
            void clear_acode_extract_path();
            static bool ExtractFilesFromTar(const wxString& tarFilePath, const wxString& outputDir, std::string file_pattern_regex = ".*");
            static wxString extract_aiGcode_file_from_tar(const wxString& tarFilePath);

            std::vector<size_t> load_files(const std::vector<boost::filesystem::path>& input_files, bool load_model = true, bool load_config = true, bool imperial_units = false, bool calibration = false);
            // To be called when providing a list of files to the GUI slic3r on command line.
            std::vector<size_t> load_files(const std::vector<std::string>& input_files, bool load_model = true, bool load_config = true, bool imperial_units = false, bool calibration = false);
            // to be called on drag and drop
            bool load_files(const wxArrayString& filenames, bool delete_after_load = false);
            bool isImportGCode() const;
            bool ImportIsACode() const;
            void check_selected_presets_visibility(PrinterTechnology loaded_printer_technology);

            bool preview_zip_archive(const boost::filesystem::path& input_file);

            const wxString& get_last_loaded_gcode() const { return m_last_loaded_gcode; }

            void update();

            void update(int flag);

            // Get the worker handling the UI jobs (arrange, fill bed, etc...)
            // Here is an example of starting up an ad-hoc job:
            //    queue_job(
            //        get_ui_job_worker(),
            //        [](Job::Ctl &ctl) {
            //            // Executed in the worker thread
            //
            //            CursorSetterRAII cursor_setter{ctl};
            //            std::string msg = "Running";
            //
            //            ctl.update_status(0, msg);
            //            for (int i = 0; i < 100; i++) {
            //                usleep(100000);
            //                if (ctl.was_canceled()) break;
            //                ctl.update_status(i + 1, msg);
            //            }
            //            ctl.update_status(100, msg);
            //        },
            //        [](bool, std::exception_ptr &e) {
            //            // Executed in UI thread after the work is done
            //
            //            try {
            //                if (e) std::rethrow_exception(e);
            //            } catch (std::exception &e) {
            //                BOOST_LOG_TRIVIAL(error) << e.what();
            //            }
            //            e = nullptr;
            //        });
            // This would result in quick run of the progress indicator notification
            // from 0 to 100. Use replace_job() instead of queue_job() to cancel all
            // pending jobs.
            Worker& get_ui_job_worker();
            const Worker& get_ui_job_worker() const;

            void select_view(const std::string& direction);
            void select_view_3D(ViewMode mode);

            bool is_preview_shown() const;
            bool is_preview_loaded() const;
            bool is_view3D_shown() const;

            bool are_view3D_labels_shown() const;
            void show_view3D_labels(bool show);

            bool is_legend_shown() const;
            void show_legend(bool show);

            wxWindow* get_preview_toolbar();
            void enable_moves_slider(bool enable);

            bool is_sidebar_collapsed() const;
            void collapse_sidebar(bool show);

            bool is_view3D_layers_editing_enabled() const;

            // Called after the Preferences dialog is closed and the program settings are saved.
            // Update the UI based on the current preferences.
            void update_ui_from_settings();

            void select_all();
            void deselect_all();
            int get_object_count();
            int get_printable_object_count();

            void remove(size_t obj_idx);
            void reset();
            void reset_with_confirm();
            bool delete_object_from_model(size_t obj_idx);
            void remove_selected();
            void increase_instances(size_t num = 1, int obj_idx = -1);
            void decrease_instances(size_t num = 1, int obj_idx = -1);
            void set_number_of_copies();
            void fill_bed_with_instances();
            bool is_selection_empty() const;
            void scale_selection_to_fit_print_volume();
            void convert_unit(ConversionType conv_type);
            void toggle_layers_editing(bool enable);
            void set_selected_visible(bool visible);

            void cut(size_t obj_idx, size_t instance_idx, const Transform3d& cut_matrix, ModelObjectCutAttributes attributes);

            void export_akeyPrint_gcode(std::string &path, bool isAcode = false);
            void export_gcode(bool prefer_removable, bool disableAI = false);
            void set_export_progress_change_callback(progressChangeCallbackFunction cb);
            void set_app_closing(bool v);
            bool is_exporting();
            bool is_exporting_gcode();
            bool is_exporting_acode();
            void stop_exporting_acode();
            bool is_cancel_exporting_Gcode();
            void set_create_AI_file_val(bool val);
            bool get_create_AI_file_val();
            void set_view_drop_file(bool val);
            bool is_view_drop_file();
            void a_key_print_clicked();
            bool ExportGacode();
            void UpdateDeviceList(bool hideList);
            void ShowDeviceList();
            void export_stl_obj(bool extended = false, bool selection_only = false);
            void export_amf();
            bool export_3mf(const boost::filesystem::path& output_path = boost::filesystem::path());
            void reload_from_disk();
            void replace_with_stl();
            void reload_all_from_disk();
            bool has_toolpaths_to_export() const;
            void export_toolpaths_to_obj() const;
            void reslice();
            void reslice_FFF_until_step(PrintObjectStep step, const ModelObject& object, bool postpone_error_messages = false);
            void reslice_SLA_until_step(SLAPrintObjectStep step, const ModelObject& object, bool postpone_error_messages = false);

            void clear_before_change_mesh(int obj_idx, const std::string& notification_msg);
            void changed_mesh(int obj_idx);

            void changed_object(ModelObject *object);
            void changed_object(int obj_idx);
            void changed_objects(const std::vector<size_t>& object_idxs);
            void schedule_background_process(bool schedule = true);
            bool is_background_process_update_scheduled() const;
            void suppress_background_process(const bool stop_background_process);
            bool background_process_running();
            void send_gcode();
            void eject_drive();

            void take_snapshot(const std::string& snapshot_name);
            void take_snapshot(const wxString& snapshot_name);
            void take_snapshot(const std::string& snapshot_name, UndoRedo::SnapshotType snapshot_type);
            void take_snapshot(const wxString& snapshot_name, UndoRedo::SnapshotType snapshot_type);

            void undo();
            void redo();
            void undo_to(int selection);
            void redo_to(int selection);
            bool undo_redo_string_getter(const bool is_undo, int idx, const char** out_text);
            void undo_redo_topmost_string_getter(const bool is_undo, std::string& out_text);
            bool search_string_getter(int idx, const char** label, const char** tooltip);
            // For the memory statistics. 
            const Slic3r::UndoRedo::Stack& undo_redo_stack_main() const;
            void clear_undo_redo_stack_main();
            // Enter / leave the Gizmos specific Undo / Redo stack. To be used by the SLA support point editing gizmo.
            void enter_gizmos_stack();
            void leave_gizmos_stack();

            void on_extruders_change(size_t extruders_count);
            bool update_filament_colors_in_full_config();
            void on_config_change(const DynamicPrintConfig& config);
            void force_filament_colors_update();
            void force_print_bed_update();
            // On activating the parent window.
            void on_activate();
            void on_move(wxMoveEvent& event);
            void on_show(wxShowEvent& event);
            void on_size(wxSizeEvent& event);
            void on_minimize(wxIconizeEvent& event);
            void on_maximize(wxMaximizeEvent& event);
            void on_idle(wxIdleEvent& evt);
            void onSliceNow();
            void updateMatchHint();

            std::vector<std::string> get_extruder_colors_from_plater_config(const GCodeProcessorResult* const result = nullptr) const;
            std::vector<std::string> get_filament_colors_from_plater_config(const GCodeProcessorResult* const result = nullptr) const;
            std::vector<std::string> get_colors_for_color_print(const GCodeProcessorResult* const result = nullptr) const;

            void update_menus();
            void show_action_buttons(const bool is_ready_to_slice) const;
            void show_action_buttons() const;

            wxString get_project_filename(const wxString& extension = wxEmptyString) const;
            void set_project_filename(const wxString& filename);

            bool is_export_gcode_scheduled() const;

            const Selection& get_selection() const;
            int get_selected_object_idx();
            bool is_single_full_object_selection() const;
            GLCanvas3D* canvas3D();
            const GLCanvas3D* canvas3D() const;
            GLCanvas3D* canvas_preview();
            GLCanvas3D* get_current_canvas3D();
            GLCanvas3D* get_assmeble_canvas3D();
            GLCanvas3D* get_view3D_canvas3D();
            GCodeProcessorResultExt* get_gcode_info();

            void arrange();

            // Aden add.
            void auto_bed();

            void set_current_canvas_as_dirty();
            void unbind_canvas_event_handlers();
            void reset_canvas_volumes();

            PrinterTechnology   printer_technology() const;
            const DynamicPrintConfig* config() const;
            bool                set_printer_technology(PrinterTechnology printer_technology);

            void copy_selection_to_clipboard();
            void paste_from_clipboard();
            void search(bool plater_is_active, wxString defaultSearchData="", Preset::Type type= Preset::TYPE_COUNT);
            void mirror(Axis axis);
            void split_object();
            void split_volume();

            void fill_color(int extruder_id);

            bool can_delete() const;
            bool can_delete_all() const;
            bool can_increase_instances() const;
            bool can_decrease_instances(int obj_idx = -1) const;
            bool can_set_instance_to_object() const;
            bool can_fix_through_netfabb() const;
            bool can_simplify() const;
            bool can_split_to_objects() const;
            bool can_split_to_volumes() const;
            bool can_arrange() const;
            bool can_auto_bed() const;
            bool can_layers_editing() const;
            bool can_paste_from_clipboard() const;
            bool can_copy_to_clipboard() const;
            bool can_undo() const;
            bool can_redo() const;
            bool can_reload_from_disk() const;
            bool can_replace_with_stl() const;
            bool can_mirror() const;
            bool can_split(bool to_objects) const;
            bool can_scale_to_print_volume() const;

            //assmeble
            bool can_fillcolor() const;
            bool has_assmeble_view() const;

            void msw_rescale();
            void sys_color_changed();

            bool init_view_toolbar();
            void enable_view_toolbar(bool enable);
            bool init_collapse_toolbar();
            void enable_collapse_toolbar(bool enable);

            const Camera& get_camera() const;
            Camera& get_camera();

            void shutdown();
#if ENABLE_ENVIRONMENT_MAP
            void init_environment_texture();
            unsigned int get_environment_texture_id() const;
#endif // ENABLE_ENVIRONMENT_MAP

            const BuildVolume& build_volume() const;

            const GLToolbar& get_view_toolbar() const;
            GLToolbar& get_view_toolbar();

            const GLToolbar& get_collapse_toolbar() const;
            GLToolbar& get_collapse_toolbar();

            void set_preview_layers_slider_values_range(int bottom, int top);

            void update_preview_moves_slider();
            void enable_preview_moves_slider(bool enable);

            void update_current_ViewType(GCodeViewer::EViewType type);

            void reset_gcode_toolpaths();
            void reset_last_loaded_gcode() { m_last_loaded_gcode = ""; }

            const Mouse3DController& get_mouse3d_controller() const;
            Mouse3DController& get_mouse3d_controller();

            void set_bed_shape() const;
            void set_bed_shape(const Pointfs& shape, const double max_print_height, const std::string& custom_texture, const std::string& custom_model, bool force_as_custom = false) const;
            void set_default_bed_shape() const;
            BoundingBox get_bed_shape() const;

            NotificationManager* get_notification_manager();
            const NotificationManager* get_notification_manager() const;

            void init_notification_manager();

            void bring_instance_forward();

            // ROII wrapper for suppressing the Undo / Redo snapshot to be taken.
            class SuppressSnapshots
            {
            public:
                SuppressSnapshots(Plater* plater) : m_plater(plater)
                {
                    m_plater->suppress_snapshots();
                }
                ~SuppressSnapshots()
                {
                    m_plater->allow_snapshots();
                }
            private:
                Plater* m_plater;
            };

            // RAII wrapper for taking an Undo / Redo snapshot while disabling the snapshot taking by the methods called from inside this snapshot.
            class TakeSnapshot
            {
            public:
                TakeSnapshot(Plater* plater, const std::string& snapshot_name);
                TakeSnapshot(Plater* plater, const wxString& snapshot_name) : m_plater(plater)
                {
                    m_plater->take_snapshot(snapshot_name);
                    m_plater->suppress_snapshots();
                }
                TakeSnapshot(Plater* plater, const std::string& snapshot_name, UndoRedo::SnapshotType snapshot_type);
                TakeSnapshot(Plater* plater, const wxString& snapshot_name, UndoRedo::SnapshotType snapshot_type) : m_plater(plater)
                {
                    m_plater->take_snapshot(snapshot_name, snapshot_type);
                    m_plater->suppress_snapshots();
                }

                ~TakeSnapshot()
                {
                    m_plater->allow_snapshots();
                }
            private:
                Plater* m_plater;
            };

            bool inside_snapshot_capture();

            void toggle_render_statistic_dialog();
            bool is_render_statistic_dialog_visible() const;

            void set_keep_current_preview_type(bool value);

            // Wrapper around wxWindow::PopupMenu to suppress error messages popping out while tracking the popup menu.
            bool PopupMenu(wxMenu* menu, const wxPoint& pos = wxDefaultPosition);
            bool PopupMenu(wxMenu* menu, int x, int y) { return this->PopupMenu(menu, wxPoint(x, y)); }

            // get same Plater/ObjectList menus
            wxMenu* object_menu();
            wxMenu* part_menu();
            wxMenu* text_part_menu();
            wxMenu* svg_part_menu();
            wxMenu* sla_object_menu();
            wxMenu* default_menu();
            wxMenu* instance_menu();
            wxMenu* layer_menu();
            wxMenu* multi_selection_menu();

            static bool has_illegal_filename_characters(const wxString& name);
            static bool has_illegal_filename_characters(const std::string& name);
            static void show_illegal_characters_warning(wxWindow* parent);

            void setAKeyPrintSlicerTempGcodePath(const std::string& gcodePath);
            // return utf8 file name
            std::string getAKeyPrintSlicerTempGcodePath() {
                return m_currentPrintGcodeFile;
            };

            void set_gcode_valid(bool valid) {
                gcode_valid = valid;
            };

            bool is_gcode_valid() {
                return gcode_valid;
            };

            std::string get_temp_gcode_output_path();

            void CalcModelObjectSize();
            void setModelObjectSizeText(wxString sizeText) { m_model_object_size_text = sizeText;};
            wxString getModelObjectSizeText() {return m_model_object_size_text;};
            void setScaledModelObjectSizeText(wxString sizeText) {m_scaled_model_object_size_text = sizeText;};
            wxString getScaledModelObjectSizeText() { return m_scaled_model_object_size_text; };
            wxBoxSizer* CreatePreViewRightSideBar();
            std::string GetRightSidePanelUpdateReasonString(RightSidePanelUpdateReason reason);
            void set_sliceModel_data(int second, std::string filament) {
                m_print_time = second;
                m_filament = std::move(filament);
            }
            DynamicPrintConfig& get_global_config();

            ////calibration/////
            void calib_pa(const Calib_Params& params);
            void _calib_pa_pattern(const Calib_Params& params);
            void _calib_pa_tower(const Calib_Params& params);
            void _calib_pa_select_added_objects();
            void calib_flowrate(int pass);
            void calib_temp(const Calib_Params& params);
            void calib_retraction(const Calib_Params& params);
            void calib_max_vol_speed(const Calib_Params& params);
            void calib_VFA(const Calib_Params& params);
            void set_calibration(bool bCalibration) { m_input_model_from_calibration = bCalibration; }
            bool get_calibration_enable() { return m_input_model_from_calibration; }
            CalibMode get_calibration_mode() { return m_calibModel; }
            /////////

        private:
            void reslice_until_step_inner(int step, const ModelObject& object, bool postpone_error_messages);

            struct priv;
            std::unique_ptr<priv> p;

            // Set true during PopupMenu() tracking to suppress immediate error message boxes.
            // The error messages are collected to m_tracking_popup_menu_error_message instead and these error messages
            // are shown after the pop-up dialog closes.
            bool 	 m_tracking_popup_menu = false;
            wxString m_tracking_popup_menu_error_message;

            wxString m_last_loaded_gcode;
            std::string m_currentPrintGcodeFile;  // utf8 
            std::string m_onekeyPrintGcodeFile;
            wxString  m_model_object_size_text;
            wxString  m_scaled_model_object_size_text;
            
            bool gcode_valid{ false };

            int m_print_time { 0 };
            std::string m_filament { "--" };

            void suppress_snapshots();
            void allow_snapshots();

            AnkerChooseDeviceDialog* m_chooseDeviceDialog = nullptr;
            bool m_input_model_from_calibration = false;
            CalibMode m_calibModel{ CalibMode::Calib_None };
            friend class SuppressBackgroundProcessingUpdate;
        };

        class SuppressBackgroundProcessingUpdate
        {
        public:
            SuppressBackgroundProcessingUpdate();
            ~SuppressBackgroundProcessingUpdate();
        private:
            bool m_was_scheduled;
        };

        class PlaterAfterLoadAutoArrange
        {
            bool m_enabled{ false };

        public:
            PlaterAfterLoadAutoArrange();
            ~PlaterAfterLoadAutoArrange();
            void disable() { m_enabled = false; }
        };

    } // namespace GUI
} // namespace Slic3r

#endif

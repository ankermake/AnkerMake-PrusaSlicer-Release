#ifndef slic3r_GUI_App_hpp_
#define slic3r_GUI_App_hpp_

#include <memory>
#include <string>
#include "ImGuiWrapper.hpp"
#include "ConfigWizard.hpp"
#include "OpenGLManager.hpp"
#include "libslic3r/Preset.hpp"

#include <wx/app.h>
#include <wx/colour.h>
#include <wx/font.h>
#include <wx/string.h>
#include <wx/snglinst.h>

#include <mutex>
#include <stack>
#include <thread>

#include "OnlinePreset/OnlinePresetManager.hpp"

class AnkerObjectBar;
class AnkerObjectManipulator;
class AnkerFloatingList;
class wxMenuItem;
class wxMenuBar;
class wxTopLevelWindow;
class wxDataViewCtrl;
class wxBookCtrlBase;
struct wxLanguageInfo;

#define  AnkerSize(x, y) Slic3r::GUI::wxGetApp().getRealSize(wxSize(x, y))
#define  AnkerRect(x, y, w, h) Slic3r::GUI::wxGetApp().getRealRect(wxRect(x, y, w, h))
#define  AnkerPoint(x, y) Slic3r::GUI::wxGetApp().getRealPoint(wxPoint(x, y))
#define  AnkerLength(x) Slic3r::GUI::wxGetApp().getRealLength(x)

//enable/ disable V6 ui 
#define ENABLE_V6   true
#define LOCAL_SIMULATE_V6 false


namespace Slic3r {

class AppConfig;
class PresetBundle;
class PresetUpdater;
class ModelObject;
class PrintHostJobQueue;
class Model;
class AppUpdater;

namespace GUI{

class RemovableDriveManager;
class OtherInstanceMessageHandler;
class MainFrame;
class Sidebar;
// add by allen for ankerCfgDlg
class AnkerSidebarNew;
class AnkerObjectLayers;
class ObjectManipulation;
class ObjectSettings;
class ObjectList;
class ObjectLayers;
class Plater;
class NotificationManager;
class Downloader;
struct GUI_InitParams;
class GalleryDialog;
class FilamentMaterialManager;
class AnkerObjectLayerEditor;


enum FileType
{
    FT_STL,
    FT_OBJ,
    FT_OBJECT,
    FT_STEP,
    FT_AMF,
    FT_3MF,
    FT_GCODE,
    FT_ACODE,
    FT_MODEL,
    FT_PROJECT,
    FT_FONTS,
    FT_GALLERY,

    FT_INI,
    FT_SVG,

    FT_TEX,

    FT_SL1,

    FT_ZIP,

    FT_SIZE,
};

#if ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR
extern wxString file_wildcards(FileType file_type);
#else
extern wxString file_wildcards(FileType file_type, const std::string &custom_extension = std::string{});
#endif // ENABLE_ALTERNATIVE_FILE_WILDCARDS_GENERATOR

extern wxString WrapEveryCharacter(const wxString& str, wxFont font, const int& lineLength);
extern wxString WrapFixWidth(const wxString& str, wxFont font, const int& lineLength);
extern wxString WrapFixWidthAdvance(const wxString& str, wxFont font, const int& lineLength, std::string language);
extern wxString WrapFixWidthAdvance(const wxString& str, wxFont font, const int& lineLength);

enum ConfigMenuIDs {
    ConfigMenuWizard,
    ConfigMenuSnapshots,
    ConfigMenuTakeSnapshot,
    ConfigMenuUpdateConf,
    ConfigMenuUpdateApp,
    ConfigMenuDesktopIntegration,
    ConfigMenuPreferences,
    ConfigMenuModeSimple,
    ConfigMenuModeAdvanced,
    ConfigMenuModeExpert,
    ConfigMenuLanguage,
    ConfigMenuFlashFirmware,
    ConfigMenuCnt,
};


enum ENVIR_ID {
	EU_ID = wxID_ANY +10086,
    US_ID = wxID_ANY +10087,
	QA_ID = wxID_ANY +10088,
    CI_ID = wxID_ANY + 10089
};

enum ANKER_ENVIR
{
    EN_UNKNOWN = -1,    
    US_ENVIR = 0,
    EU_ENVIR = 1,
    QA_ENVIR = 2,
    CI_ENVIR = 3
};

// add by allen for ankerCfgDlg
class AnkerTab;
class Tab;

class ConfigWizard;

static wxString dots("…", wxConvUTF8);

// Does our wxWidgets version support markup?
// https://github.com/prusa3d/PrusaSlicer/issues/4282#issuecomment-634676371
#if wxUSE_MARKUP && wxCHECK_VERSION(3, 1, 1)
    #define SUPPORTS_MARKUP
#endif

class GUI_App : public wxApp
{
public:
    enum class EAppMode : unsigned char
    {
        Editor,
        GCodeViewer
    };

private:
    bool            m_initialized { false };
    bool            m_post_initialized { false };
    bool            m_app_conf_exists{ false };
    bool            m_last_app_conf_lower_version{ false };
    EAppMode        m_app_mode{ EAppMode::Editor };
    bool            m_is_recreating_gui{ false };
    bool            m_opengl_initialized{ false };

    wxColour        m_color_label_modified;
    wxColour        m_color_label_sys;
    wxColour        m_color_label_default;
    wxColour        m_color_window_default;
#ifdef _WIN32
    wxColour        m_color_highlight_label_default;
    wxColour        m_color_hovered_btn_label;
    wxColour        m_color_default_btn_label;
    wxColour        m_color_highlight_default;
    wxColour        m_color_selected_btn_bg;
    bool            m_force_colors_update { false };
#endif
    std::vector<std::string>     m_mode_palette;

    wxFont		    m_small_font;
    wxFont		    m_bold_font;
	wxFont			m_normal_font;
	wxFont			m_code_font;
    wxFont		    m_link_font;

    int             m_em_unit; // width of a "m"-symbol in pixels for current system font
                               // Note: for 100% Scale m_em_unit = 10 -> it's a good enough coefficient for a size setting of controls

    std::unique_ptr<wxLocale> 	  m_wxLocale;
    // System language, from locales, owned by wxWidgets.
    const wxLanguageInfo		 *m_language_info_system = nullptr;
    // Best translation language, provided by Windows or OSX, owned by wxWidgets.
    const wxLanguageInfo		 *m_language_info_best   = nullptr;

    OpenGLManager m_opengl_mgr;

    std::unique_ptr<RemovableDriveManager> m_removable_drive_manager;

    std::unique_ptr<ImGuiWrapper> m_imgui;
    std::unique_ptr<PrintHostJobQueue> m_printhost_job_queue;
	std::unique_ptr <OtherInstanceMessageHandler> m_other_instance_message_handler;
    std::unique_ptr <AppUpdater> m_app_updater;
    std::unique_ptr <wxSingleInstanceChecker> m_single_instance_checker;
    std::unique_ptr <Downloader> m_downloader;
    std::unique_ptr <FilamentMaterialManager> m_filament_material_manager;
    std::unique_ptr <OnlinePresetManager> m_onlinePresetManager;

    std::string m_instance_hash_string;
	size_t m_instance_hash_int;

public:
    bool            OnInit() override;
    bool            initialized() const { return m_initialized; }

    explicit GUI_App(EAppMode mode = EAppMode::Editor);
    ~GUI_App() override;
    void set_app_mode(EAppMode mode = EAppMode::Editor) { m_app_mode = mode; }
    EAppMode get_app_mode() const { return m_app_mode; }
    bool is_editor() const { return m_app_mode == EAppMode::Editor; }
    bool is_gcode_viewer() const { return m_app_mode == EAppMode::GCodeViewer; }
    bool is_recreating_gui() const { return m_is_recreating_gui; }
    std::string logo_name() const { return is_editor() ? "AnkerStudio" : "AnkerStudio-gcodeviewer"; }

    ///
    void request_model_download(std::string url);
    ////

    // To be called after the GUI is fully built up.
    // Process command line parameters cached in this->init_params,
    // load configs, STLs etc.
    void            post_init();
    // If formatted for github, plaintext with OpenGL extensions enclosed into <details>.
    // Otherwise HTML formatted for the system info dialog.
    static std::string get_gl_info(bool for_github);
    wxGLContext*    init_glcontext(wxGLCanvas& canvas);
    bool            init_opengl();

    static unsigned get_colour_approx_luma(const wxColour &colour);
    static bool     dark_mode();
    const wxColour  get_label_default_clr_system();
    const wxColour  get_label_default_clr_modified();
    const std::vector<std::string> get_mode_default_palette();
    void            init_ui_colours();
    void            update_ui_colours_from_appconfig();
    void            update_label_colours();
    // update color mode for window
    void            UpdateDarkUI(wxWindow *window, bool highlited = false, bool just_font = false);
    // update color mode for whole dialog including all children
    void            UpdateDlgDarkUI(wxDialog* dlg, bool just_buttons_update = false);
    // update color mode for DataViewControl
    void            UpdateDVCDarkUI(wxDataViewCtrl* dvc, bool highlited = false);
    // update color mode for panel including all static texts controls
    void            UpdateAllStaticTextDarkUI(wxWindow* parent);
    void            init_fonts();
	void            update_fonts(const MainFrame *main_frame = nullptr);
    void            set_label_clr_modified(const wxColour& clr);
    void            set_label_clr_sys(const wxColour& clr);

    const wxColour& get_label_clr_modified(){ return m_color_label_modified; }
    const wxColour& get_label_clr_sys()     { return m_color_label_sys; }
    const wxColour& get_label_clr_default() { return m_color_label_default; }
    const wxColour& get_window_default_clr(){ return m_color_window_default; }

    const std::string&      get_mode_btn_color(int mode_id);
    std::vector<wxColour>   get_mode_palette();
    void                    set_mode_palette(const std::vector<wxColour> &palette);

#ifdef _WIN32
    const wxColour& get_label_highlight_clr()   { return m_color_highlight_label_default; }
    const wxColour& get_highlight_default_clr() { return m_color_highlight_default; }
    const wxColour& get_color_hovered_btn_label() { return m_color_hovered_btn_label; }
    const wxColour& get_color_selected_btn_bg() { return m_color_selected_btn_bg; }
    void            force_colors_update();
#ifdef _MSW_DARK_MODE
    void            force_menu_update();
#endif //_MSW_DARK_MODE
#endif

    const wxFont&   small_font()            { return m_small_font; }
    const wxFont&   bold_font()             { return m_bold_font; }
    const wxFont&   normal_font()           { return m_normal_font; }
    const wxFont&   code_font()             { return m_code_font; }
    const wxFont&   link_font()             { return m_link_font; }
    int             em_unit() const         { return m_em_unit; }
    wxSize          getRealSize(const wxSize& windowSize);
    wxRect          getRealRect(const wxRect& windowRect);
    wxPoint          getRealPoint(const wxPoint& windowPoint);
    int          getRealLength(const int windowsLen);
    bool            tabs_as_menu() const;
    wxSize          get_min_size() const;
    float           toolbar_icon_scale(const bool is_limited = false) const;
    void            set_auto_toolbar_icon_scale(float scale) const;
    void            check_printer_presets();

    void            recreate_GUI(const wxString& message);
    void            system_info();
    void            keyboard_shortcuts();
    void            load_project(wxWindow *parent, wxString& input_file) const;
    void            import_model(wxWindow *parent, wxArrayString& input_files) const;
    void            import_zip(wxWindow* parent, wxString& input_file) const;
    void            load_gcode(wxWindow* parent, wxString& input_file) const;
    void            change_calibration_dialog(const wxDialog* have_to_destroy, wxDialog* new_one);
    void            calib_filament_temperature_dialog(wxWindow* parent, Plater* plater);
    void            calib_pressure_advance_dialog(wxWindow* parent, Plater* plater);
    void            calib_retraction_dialog(wxWindow* parent, Plater* plater);
    void            calib_max_flowrate_dialog(wxWindow* parent, Plater* plater);
    void            calib_vfa_dialog(wxWindow* parent, Plater* plater);

    static bool     catch_error(std::function<void()> cb, const std::string& err);

    void            persist_window_geometry(wxTopLevelWindow *window, bool default_maximized = false);
    void            update_ui_from_settings();

    enum AnkerLanguageType {
        AnkerLanguageType_Unknown = -1,
        AnkerLanguageType_English = 5,
        AnkerLanguageType_Japanese = 10,
        AnkerLanguageType_Chinese = 1
    };
    bool            switch_language();
    bool            switch_language(AnkerLanguageType language);
    bool            load_language(wxString language, bool initial);
    int getCurrentLanguageType() const;

    // add by allen for ankerCfgDlg
    AnkerTab*   getAnkerTab(Preset::Type type);
    Tab*            get_tab(Preset::Type type);

    ConfigOptionMode get_mode();
    bool            save_mode(const /*ConfigOptionMode*/int mode) ;
    void            update_mode();

    void            add_config_menu(wxMenuBar *menu);
    bool            has_unsaved_preset_changes() const;
    bool            has_current_preset_changes() const;
    void            update_saved_preset_from_current_preset();
    std::vector<const PresetCollection*> get_active_preset_collections() const;
    bool            check_and_save_current_preset_changes(const wxString& caption, const wxString& header, bool remember_choice = true, bool use_dont_save_insted_of_discard = false);
    void            apply_keeped_preset_modifications();
    bool            check_and_keep_current_preset_changes(const wxString& caption, const wxString& header, int action_buttons, bool* postponed_apply_of_keeped_changes = nullptr);
    bool            can_load_project();
    bool            check_print_host_queue();
    
    bool            checked_tab(Tab* tab);
    // add by allen for ankerCfgDlg
    bool            checkedAnkerTab(AnkerTab* tab);

    void            load_current_presets(bool check_printer_presets = true);

    wxString        current_language_code() const { return m_wxLocale->GetCanonicalName(); }
	// Translate the language code to a code, for which Anker Research maintains translations. Defaults to "en_US".
    wxString 		current_language_code_safe() const;
    bool            is_localized() const { return m_wxLocale->GetLocale() != "English"; }

    void            open_preferences(const std::string& highlight_option = std::string(), const std::string& tab_name = std::string());

    virtual bool OnExceptionInMainLoop() override;
    // Calls wxLaunchDefaultBrowser if user confirms in dialog.
    // Add "Rememeber my choice" checkbox to question dialog, when it is forced or a "suppress_hyperlinks" option has empty value
    bool            open_browser_with_warning_dialog(const wxString& url, wxWindow* parent = nullptr, bool force_remember_choice = true, int flags = 0);
#ifdef __APPLE__
    void            OSXStoreOpenFiles(const wxArrayString &files) override;
    // wxWidgets override to get an event on open files.
    void            MacOpenFiles(const wxArrayString &fileNames) override;
    void            MacOpenURL(const wxString& url) override;
#endif /* __APPLE */

    Sidebar&             sidebar();
    // add by allen for ankerCfgDlg
    AnkerSidebarNew&             sidebarnew();
    AnkerObjectBar*         objectbar();
    AnkerFloatingList*      floatinglist();
    ObjectManipulation*  obj_manipul();
    AnkerObjectManipulator*  aobj_manipul();
    ObjectSettings*      obj_settings();
    ObjectList*          obj_list();
    AnkerObjectLayers*   obj_layers_();
    AnkerObjectLayerEditor* obj_layers();
    Plater*              plater();
    const Plater*        plater() const;
    Model&      		 model();
    NotificationManager* notification_manager();
    GalleryDialog *      gallery_dialog();
    Downloader*          downloader();
    FilamentMaterialManager* filamentMaterialManager();
    OnlinePresetManager* onlinePresetManager();

    // Parameters extracted from the command line to be passed to GUI after initialization.
    GUI_InitParams* init_params { nullptr };

    AppConfig*      app_config{ nullptr };
    PresetBundle*   preset_bundle{ nullptr };
    PresetUpdater*  preset_updater{ nullptr };
    MainFrame*      mainframe{ nullptr };
    Plater*         plater_{ nullptr };

	PresetUpdater*  get_preset_updater() { return preset_updater; }

    wxBookCtrlBase* tab_panel() const ;
    // add by allen for ankerCfgDlg
    wxBookCtrlBase* ankerTabPanel() const;

    int             extruders_cnt() const;
    int             extruders_edited_cnt() const;
    
    std::vector<Tab*>      tabs_list;
    // add by allen for ankerCfgDlg
    std::vector<AnkerTab*>      ankerTabsList;

	RemovableDriveManager* removable_drive_manager() { return m_removable_drive_manager.get(); }
	OtherInstanceMessageHandler* other_instance_message_handler() { return m_other_instance_message_handler.get(); }
    wxSingleInstanceChecker* single_instance_checker() {return m_single_instance_checker.get();}

	void        init_single_instance_checker(const std::string &name, const std::string &path);
	void        set_instance_hash (const size_t hash) { m_instance_hash_int = hash; m_instance_hash_string = std::to_string(hash); }
    std::string get_instance_hash_string ()           { return m_instance_hash_string; }
	size_t      get_instance_hash_int ()              { return m_instance_hash_int; }

    bool check_privacy_policy();

    std::string getWebview2Version();

    ImGuiWrapper* imgui() { return m_imgui.get(); }

    PrintHostJobQueue& printhost_job_queue() { return *m_printhost_job_queue.get(); }

    void            open_web_page_localized(const std::string &http_address);
    bool            may_switch_to_SLA_preset(const wxString& caption);
    bool            run_wizard(ConfigWizard::RunReason reason, ConfigWizard::StartPage start_page = ConfigWizard::SP_WELCOME);
    void            show_desktop_integration_dialog();
    void            show_downloader_registration_dialog();

#if ENABLE_THUMBNAIL_GENERATOR_DEBUG
    // temporary and debug only -> extract thumbnails from selected gcode and save them as png files
    void            gcode_thumbnails_debug();
#endif // ENABLE_THUMBNAIL_GENERATOR_DEBUG

    GLShaderProgram* get_shader(const std::string& shader_name) { return m_opengl_mgr.get_shader(shader_name); }
    GLShaderProgram* get_current_shader() { return m_opengl_mgr.get_current_shader(); }

    bool is_gl_version_greater_or_equal_to(unsigned int major, unsigned int minor) const { return m_opengl_mgr.get_gl_info().is_version_greater_or_equal_to(major, minor); }
    bool is_glsl_version_greater_or_equal_to(unsigned int major, unsigned int minor) const { return m_opengl_mgr.get_gl_info().is_glsl_version_greater_or_equal_to(major, minor); }
    int  GetSingleChoiceIndex(const wxString& message, const wxString& caption, const wxArrayString& choices, int initialSelection);

#ifdef __WXMSW__
    void            associate_3mf_files();
    void            associate_stl_files();
    void            associate_gcode_files();
#endif // __WXMSW__


    // URL download - AnkerStudio gets system call to open ankerstudio:// URL which should contain address of download
    void            start_download(std::string url);

    
    bool            select_language(AnkerLanguageType type);
    void            on_CrashTrack();

private:
    bool            on_init_inner();
    void            CompatibleProcess();
    void            WebviewCompatibleProcess();
    bool            ProfileCompatibleProcess();
	void            init_app_config();
    // returns old config path to copy from if such exists,
    // returns an empty string if such config path does not exists or if it cannot be loaded.
    std::string     check_older_app_config(Semver current_version, bool backup);
    void            window_pos_save(wxTopLevelWindow* window, const std::string &name);
    void            window_pos_restore(wxTopLevelWindow* window, const std::string &name, bool default_maximized = false);
    void            window_pos_sanitize(wxTopLevelWindow* window);
    bool            select_language();

    

    bool            config_wizard_startup();
    // Returns true if the configuration is fine. 
    // Returns true if the configuration is not compatible and the user decided to rather close the slicer instead of reconfiguring.
	bool            check_updates(const bool verbose);
    void            on_version_read(wxCommandEvent& evt);
    // if the data from version file are already downloaded, shows dialogs to start download of new version of app
    void            app_updater(bool from_user);
    // inititate read of version file online in separate thread
    void            app_version_check(bool from_user);

    // Support regular cleaning of log files and retaining logs of the last few days
    void delLogtimer();
    void autoDeleteLogfile(const unsigned int days);
    void autoDeleteTempGcodeFile(const unsigned int days);
    void autoDeleteUrlFile();
    void readDownloadFile();

    bool            m_datadir_redefined { false }; 
    std::thread     m_thrdDelLog;
    std::atomic<bool> m_bDelLogTimerStop{ true };
 
    std::thread    m_urlThread;
    std::atomic<bool> m_bReadUrlStop { true };
};

DECLARE_APP(GUI_App)

} // GUI
} // Slic3r

#endif // slic3r_GUI_App_hpp_

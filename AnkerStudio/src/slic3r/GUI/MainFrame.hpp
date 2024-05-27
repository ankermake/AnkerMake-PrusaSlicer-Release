#ifndef slic3r_MainFrame_hpp_
#define slic3r_MainFrame_hpp_

#include "libslic3r/PrintConfig.hpp"

#include <wx/frame.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/filehistory.h>
#include <wx/webview.h>
#ifdef __APPLE__
#include <wx/taskbar.h>
#endif // __APPLE__

#include <string>
#include <map>
#include <mutex>

#include "GUI_Utils.hpp"
#include "Event.hpp"
#include "UnsavedChangesDialog.hpp"
#include "AnkerDevice.hpp"
#include "GUI_App.hpp"
#include "AnkerWebView.hpp"
#include "AnkerFunctionPanel.h"
#include "AnkerConfigDialog/AnkerConfigDialog.hpp"
#include "AnkerSliceConmentDialog.hpp"

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_MAINWIN_MOVE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_RELOAD_DATA, wxCommandEvent);

class wxBookCtrlBase;
class wxProgressDialog;
namespace Slic3r {

class ProgressStatusBar;
namespace GUI
{

class Tab;
// add by allen for ankerCfgDlg
class AnkerTab;
class AnkerConfigDlg;
class AnkerTabPresetComboBox;

class PrintHostQueueDialog;
class Plater;
class MainFrame;
class PreferencesDialog;
class GalleryDialog;
class CalibrationMaxFlowrateDialog;
class CalibrationPresAdvDialog;
class CalibrationTempDialog;
class CalibrationRetractionDialog;
class CalibrationVfaDialog;


enum QuickSlice
{
    qsUndef = 0,
    qsReslice = 1,
    qsSaveAs = 2,
    qsExportSVG = 4,
    qsExportPNG = 8
};

enum TabMode
{
    TAB_SLICE,
    TAB_DEVICE,
    TAB_COUNT
};

struct PresetTab {
    std::string       name;
    Tab*              panel;
    PrinterTechnology technology;
};

// ----------------------------------------------------------------------------
// SettingsDialog
// ----------------------------------------------------------------------------

class SettingsDialog : public DPIFrame//DPIDialog
{
    wxBookCtrlBase* m_tabpanel { nullptr };
    MainFrame*      m_main_frame { nullptr };
    wxMenuBar*      m_menubar{ nullptr };
public:
    SettingsDialog(MainFrame* mainframe);
    ~SettingsDialog() = default;
    void set_tabpanel(wxBookCtrlBase* tabpanel) { m_tabpanel = tabpanel; }
    wxMenuBar* menubar() { return m_menubar; }

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;
};


class MainFrame : public DPIFrame
{
    bool        m_loaded {false};

    wxString    m_avatarPath = { "" };
    wxString    m_qs_last_input_file = wxEmptyString;
    wxString    m_qs_last_output_file = wxEmptyString;
    wxString    m_last_config = wxEmptyString;
    wxMenuBar*  m_menubar{ nullptr };
    //AnkerMenuBar* m_menubar{ nullptr };

#if 0
    wxMenuItem* m_menu_item_repeat { nullptr }; // doesn't used now
#endif
    wxMenuItem* m_menu_item_reslice_now { nullptr };
    wxSizer*    m_main_sizer{ nullptr };    
    size_t      m_last_selected_tab;

    // init tab panel
    void initTabPanel();
    void setUserInfoForSentry();
    std::string     get_base_name(const wxString &full_name, const char *extension = nullptr) const;
    std::string     get_dir_name(const wxString &full_name) const;

    void on_presets_changed(SimpleEvent&);
    void on_value_changed(wxCommandEvent&);

    void on_size(wxSizeEvent& event);
    void on_move(wxMoveEvent& event);
    void on_show(wxShowEvent& event);
    void on_minimize(wxIconizeEvent& event);
    void on_maximize(wxMaximizeEvent& event);

    void OnDocumentLoaded(wxWebViewEvent& evt);
	void OnScriptMessage(wxCommandEvent& evt);

    bool can_start_new_project() const;
    bool can_export_model() const;
    bool can_export_toolpaths() const;
    bool can_export_supports() const;
    bool can_export_gcode() const;
    bool can_send_gcode() const;
	bool can_export_gcode_sd() const;
	bool can_eject() const;
    bool can_slice() const;
    bool can_change_view() const;
    bool can_select() const;
    bool can_deselect() const;
    bool can_delete() const;
    bool can_delete_all() const;
    bool can_reslice() const;
    void bind_diff_dialog();
    // add by allen for ankerCfgDlg
    void bind_diff_dialog_ankertab();

    // MenuBar items changeable in respect to printer technology 
    enum MenuItems
    {                   //   FFF                  SLA
        miExport = 0,   // Export G-code        Export
        miSend,         // Send G-code          Send to print
        miMaterialTab,  // Filament Settings    Material Settings
        miPrinterTab,   // Different bitmap for Printer Settings
    };

    // vector of a MenuBar items changeable in respect to printer technology 
    std::vector<wxMenuItem*> m_changeable_menu_items;
    wxMenu* m_calibration_menu = nullptr;

    wxFileHistory m_recent_projects;

    enum class ESettingsLayout
    {
        Unknown,
        Old,
        New,
        Dlg,
        GCodeViewer
    };
    
    ESettingsLayout m_layout{ ESettingsLayout::Unknown };
    static size_t onDownLoadFinishedCallBack(char* dest, size_t size, size_t nmemb, void* userp);
    static void onDownLoadProgress(double dltotal, double dlnow, double ultotal, double ulnow);

    std::string getAppName();
    void updateBuryInfo();
    wxMenu* GetHelpMenu();
    void DealPrivacyChoices(const wxCommandEvent& event);
    void RemovePrivacyChoices();

    void SetWebviewTestItem();
    void TestAnkerWebview();
    void TestLoacalBrowser();

    void LogOut();

protected:
    virtual void on_dpi_changed(const wxRect &suggested_rect) override;
    virtual void on_sys_color_changed() override;

    void ShowLoginedMenu();
    void ShowUnLoginDevice();
    void ShowUnLoginMenu();
    void ClearLoingiMenu();
    void onLogOut();
    void OnMove(wxMoveEvent& event);
    void OnOtaTimer(wxTimerEvent& event);
    void OnHttpConnectError(wxCommandEvent& event);
    void BindEvent();



public:
    enum TabPosition
    {
        tpHome = 0,
        tp3DEditor = 1,
        tpPreview = 2,
        tpMonitor = 3,
        tpProject = 4,
        tpCalibration = 5,
        tpAuxiliary = 6,
        toDebugTool = 7,
    };

    MainFrame(const int font_point_size);
    ~MainFrame();// = default;
    void createAnkerCfgDlg();
    void InitDeviceWidget();
    void ShowAnkerWebView(const std::string& from);
    void update_layout();
    void update_mode_markers();
    void setUrl(std::string webUrl = std::string());
	// Called when closing the application and when switching the application language.
	void 		shutdown(bool restart = false);
    Plater*     plater() { return m_plater; }
    GalleryDialog* gallery_dialog();

    void buryTime();
    std::string getWorkDuration();
    void        selectLanguage(GUI_App::AnkerLanguageType language);
    static bool        languageIsJapanese();
    static std::string GetTranslateLanguage();

    void        update_title();

    void        init_tabpanel();
    void        getwebLoginDataBack(const std::string& from);
    AnkerWebView* CreateWebView(bool background);
    void        InitAnkerDevice();
    void        create_preset_tabs();
    void        add_created_tab(Tab* panel, const std::string& bmp_name = "");
    bool        is_active_and_shown_tab(Tab* tab);
    // add by allen for ankerCfgDlg
    bool        isActiveAndShownAnkerTab(AnkerTab* tab);
    // Register Win32 RawInput callbacks (3DConnexion) and removable media insert / remove callbacks.
    // Called from wxEVT_ACTIVATE, as wxEVT_CREATE was not reliable (bug in wxWidgets?).
    void        register_win32_callbacks();
    void        init_menubar_as_editor();
    void        init_menubar_as_gcodeviewer();
    void        update_menubar();
    // Open item in menu by menu and item name (in actual language)
    void        open_menubar_item(const wxString& menu_name,const wxString& item_name);
#ifdef _WIN32
    void        show_tabs_menu(bool show);
#endif
    void        update_ui_from_settings();
    bool        is_loaded() const { return m_loaded; }
    bool        is_last_input_file() const  { return !m_qs_last_input_file.IsEmpty(); }
    bool        is_dlg_layout() const { return m_layout == ESettingsLayout::Dlg; }

//    void        quick_slice(const int qs = qsUndef);
    void        reslice_now();
    void        repair_stl();
    void        export_config();
    // Query user for the config file and open it.
    void        load_config_file();
    // Open a config file. Return true if loaded.
    bool        load_config_file(const std::string &path);
    void        export_configbundle(bool export_physical_printers = false);
    void        load_configbundle(wxString file = wxEmptyString);
    void        load_config(const DynamicPrintConfig& config);
    // Select tab in m_tabpanel
    // When tab == -1, will be selected last selected tab
    void        select_tab(Tab* tab);
    // add by allen for ankerCfgDlg
    void        selectAnkerTab(AnkerTab* tab);
    void        select_tab(size_t tab = size_t(-1));
    TabMode get_current_tab_mode() { return m_currentTabMode; }
    void        setTabMode(TabMode mode) { m_currentTabMode = mode; }
    void        showAnkerCfgDlg();
    void        select_view(const std::string& direction);
    // Propagate changed configuration from the Tab to the Plater and save changes to the AppConfig
    void        on_config_changed(DynamicPrintConfig* cfg) const ;

    bool can_save() const;
    bool can_save_as() const;
    void save_project();
    bool save_project_as(const wxString& filename = wxString());

    void        add_to_recent_projects(const wxString& filename);
    void        technology_changed();
    void        clearStarConmentData();

    PrintHostQueueDialog* printhost_queue_dlg() { return m_printhost_queue_dlg; }

    // add by allen for ankerCfgDlg
    AnkerTabPresetComboBox* GetAnkerTabPresetCombo(const Preset::Type type);

    AnkerTab* openAnkerTabByPresetType(const Preset::Type type);

    void loginFinishHandle();

    AnkerWebView*         m_loginWebview{ nullptr };    // background
    bool                  m_showConmentWebView{ false };
    Plater*               m_plater { nullptr };
    wxBookCtrlBase*       m_tabpanel { nullptr };
    // add by allen for ankerCfgDlg
    AnkerConfigDlg*       m_ankerCfgDlg{ nullptr };
    TabMode m_currentTabMode{ TAB_COUNT };
    wxBookCtrlBase*       m_printTabPanel{ nullptr };
    SettingsDialog        m_settings_dialog;
    DiffPresetDialog      diff_dialog;
    wxWindow*             m_plater_page{ nullptr };
    PreferencesDialog*    preferences_dialog { nullptr };
    PrintHostQueueDialog* m_printhost_queue_dlg;
    GalleryDialog*        m_gallery_dialog{ nullptr };

    AnkerFunctionPanel*        m_pFunctionPanel;
    AnkerSliceConmentDialog* m_sliceConmentDialog{ nullptr };    

#ifdef __APPLE__
    std::unique_ptr<wxTaskBarIcon> m_taskbar_icon;
#endif // __APPLE__

    wxMenu*             m_pLoginMenu {nullptr};
    AnkerDevice*        m_pDeviceWidget{ nullptr };

    ANKER_ENVIR         m_currentEnvir = EN_ENVIR;   
    wxString            m_loginUrl = {wxString("https://community-qa.eufylife.com/passport-ct/?nocache=%s#/login")};
    wxString            m_backloginUrl = {wxString("https://community-qa.eufylife.com/passport-ct/?nocache=%s#/login?invisible=true")};
    inline std::string getCurTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        return std::to_string(now_time);
    }
    inline  wxString getBackloginUrl()
    {
        return  wxString::Format(m_backloginUrl, getCurTimestamp());
    }
    inline  wxString getLoginUrl()
    {
        return  wxString::Format(m_loginUrl, getCurTimestamp());
    }
#ifdef _WIN32
    void*				m_hDeviceNotify { nullptr };
    uint32_t  			m_ulSHChangeNotifyRegister { 0 };
	static constexpr int WM_USER_MEDIACHANGED { 0x7FFF }; // WM_USER from 0x0400 to 0x7FFF, picking the last one to not interfere with wxWidgets allocation
#endif // _WIN32
    wxTimer* m_otaTimer = nullptr;
    wxTimer* m_extrusionTimer = nullptr;
    
    std::atomic_bool m_bIsOpenWebview {false};
    
    //mutable std::mutex m_ReadWriteMutex;
    
    bool m_normalExit { false };
    wxDateTime m_buryTime;
};

} // GUI
} //Slic3r

#endif // slic3r_MainFrame_hpp_

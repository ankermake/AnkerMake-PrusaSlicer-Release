#include "MainFrame.hpp"

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/listbook.h>
#include <wx/simplebook.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/progdlg.h>
#include <wx/tooltip.h>
#include <wx/filename.h>
#include <wx/debug.h>
#include <wx/protocol/http.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/log/trivial.hpp>

#include "libslic3r/Print.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/PresetBundle.hpp"

#include "Tab.hpp"
// add by allen for ankerCfgDlg
#include "AnkerCfgTab.hpp"

#include "ProgressStatusBar.hpp"
#include "3DScene.hpp"
#include "PrintHostDialogs.hpp"
#include "wxExtensions.hpp"
#include "GUI_ObjectList.hpp"
#include "Mouse3DController.hpp"
#include "RemovableDriveManager.hpp"
#include "InstanceCheck.hpp"
#include "I18N.hpp"
#include "GLCanvas3D.hpp"
#include "Plater.hpp"
#include "../Utils/Process.hpp"
#include "format.hpp"

#include <fstream>
#include <string_view>

#include <jansson.h>
#include "GUI_App.hpp"
#include "UnsavedChangesDialog.hpp"
#include "MsgDialog.hpp"
#include "Notebook.hpp"
#include "GUI_Factories.hpp"
#include "GUI_ObjectList.hpp"
#include "GalleryDialog.hpp"
#include "NotificationManager.hpp"
#include "Preferences.hpp"

#ifdef _WIN32
#include <dbt.h>
#include <shlobj.h>
#ifndef OPEN_SOURCE_MODE
#include "sentry.h"
#endif
#endif // _WIN32
#include "AnkerWebView.hpp"
#include <wx/stream.h>
#include <wx/msw/cursor.h>
#include <algorithm>
#include <wx/url.h>
#include <wx/variant.h>

#include "FilamentMaterialConvertor.hpp"
#include "FilamentMaterialManager.hpp"
#include "Common/AnkerDialog.hpp"
#include "Common/AnkerCopyrightDialog.hpp"
#include "Common/AnkerOTANotesBox.hpp"
#include "Common/AnkerComboBox.hpp"
#include "Common/AnkerSpinBox.hpp"
#include "Common/AnkerFeedbackDialog.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"
#include "../../AnkerComFunction.hpp"
#include "Common/AnkerFont.hpp"
#include "slic3r/GUI/Network/MsgText.hpp"
#include "slic3r/GUI/Calibration/FlowCalibration.hpp"
#include "slic3r/GUI/Calibration/CalibrationMaxFlowrateDialog.hpp"
#include "slic3r/GUI/Calibration/CalibrationPresAdvDialog.hpp"
#include "slic3r/GUI/Calibration/CalibrationTempDialog.hpp"
#include "slic3r/GUI/Calibration/CalibrationRetractionDialog.hpp"
#include "slic3r/GUI/Calibration/CalibrationVfaDialog.hpp"
#include "../AnkerComFunction.hpp"
#include "AnkerConfig.hpp"
#include "HintNotification.hpp"
extern AnkerPlugin* pAnkerPlugin;

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_MAINWIN_MOVE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_RELOAD_DATA, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ON_TAB_CHANGE, wxCommandEvent);

#define SHOW_ERR_DIALOG_CMD 1085
#define HIDE_ERR_DIALOG_CMD 1086

extern "C" void ToggleFullScreen(wxWindow * window);
using namespace AnkerNet;
StarCommentData       g_sliceCommentData;
namespace Slic3r {
namespace GUI {

enum class ERescaleTarget
{
    Mainframe,
    SettingsDialog
};

#ifdef __APPLE__
class AnkerStudioTaskBarIcon : public wxTaskBarIcon
{
public:
    AnkerStudioTaskBarIcon(wxTaskBarIconType iconType = wxTBI_DEFAULT_TYPE) : wxTaskBarIcon(iconType) {}
    wxMenu *CreatePopupMenu() override {
        wxMenu *menu = new wxMenu;
        if(wxGetApp().app_config->get("single_instance") == "0") {
            // Only allow opening a new AnkerStudio instance on OSX if "single_instance" is disabled, 
            // as starting new instances would interfere with the locking mechanism of "single_instance" support.
            append_menu_item(menu, wxID_ANY, _L("Open new instance"), _L("Open a new AnkerMake Studio instance"),
            [](wxCommandEvent&) { start_new_slicer(); }, "", nullptr);
        }
        append_menu_item(menu, wxID_ANY, _L("G-code preview") + dots, _L("Open G-code viewer"),
            [](wxCommandEvent&) { start_new_gcodeviewer_open_file(); }, "", nullptr);
        return menu;
    }
};
class GCodeViewerTaskBarIcon : public wxTaskBarIcon
{
public:
    GCodeViewerTaskBarIcon(wxTaskBarIconType iconType = wxTBI_DEFAULT_TYPE) : wxTaskBarIcon(iconType) {}
    wxMenu *CreatePopupMenu() override {
        wxMenu *menu = new wxMenu;
        append_menu_item(menu, wxID_ANY, _L("Open AnkerMake Studio"), _L("Open a new AnkerMake Studio instance"),
            [](wxCommandEvent&) { start_new_slicer(nullptr, true); }, "", nullptr);
        append_menu_item(menu, wxID_ANY, _L("G-code preview") + dots, _L("Open new G-code viewer"),
            [](wxCommandEvent&) { start_new_gcodeviewer_open_file(); }, "", nullptr);
        return menu;
    }
};
#endif // __APPLE__

// Load the icon either from the exe, or from the ico file.
static wxIcon main_frame_icon(GUI_App::EAppMode app_mode)
{
#if _WIN32
    std::wstring path(size_t(MAX_PATH), wchar_t(0));
    int len = int(::GetModuleFileName(nullptr, path.data(), MAX_PATH));
    if (len > 0 && len < MAX_PATH) {
        path.erase(path.begin() + len, path.end());
        if (app_mode == GUI_App::EAppMode::GCodeViewer) {
            // Only in case the slicer was started with --gcodeviewer parameter try to load the icon from anker-gcodeviewer.exe
            // Otherwise load it from the exe.
            for (const std::wstring_view exe_name : { std::wstring_view(L"ankermake studio.exe"), std::wstring_view(L"ankermake studio-console.exe") })
                if (boost::iends_with(path, exe_name)) {
                    path.erase(path.end() - exe_name.size(), path.end());
                    path += L"anker-gcodeviewer.exe";
                    break;
                }
        }
    }
    return wxIcon(path, wxBITMAP_TYPE_ICO);
#else // _WIN32
    return wxIcon(Slic3r::var(app_mode == GUI_App::EAppMode::Editor ? "AnkerStudio_128px.png" : "AnkerStudio-gcodeviewer_128px.png"), wxBITMAP_TYPE_PNG);
#endif // _WIN32
}


void MainFrame::ShowLoginedMenu()
{
    ANKER_LOG_INFO << "enter ShowLoginedMenu";
    auto ankerNet = AnkerNetInst();
    if (!ankerNet) {
        return;
    }
    if (!m_pLoginMenu)
    {
        ANKER_LOG_ERROR << "show logined menu error";
        return;
    }

    ClearLoingiMenu();
	
    wxString loginName = wxString::FromUTF8(ankerNet->GetNickName());
    if (loginName.IsEmpty())
    {
        ANKER_LOG_ERROR << "nick name format error.";
        loginName = ankerNet->GetUserEmail().substr(0, 3) + "***";
    }

	if (!wxFileExists(m_avatarPath))
	{
		auto userItem = append_menu_item(m_pLoginMenu, wxID_ANY, loginName, loginName,
			[this](wxCommandEvent&) {}, "defaultAvatar", nullptr,
			[]() {return true; }, this);

		userItem->Enable(false);
	}
    else
    {
        
		wxImage defaultAvatarImage(wxString::FromUTF8(Slic3r::var("Avatar.png")), wxBITMAP_TYPE_PNG);
        defaultAvatarImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);

        wxMenuItem* userItem = new wxMenuItem(m_pLoginMenu, wxID_ANY, loginName);

		wxString wxStrImg = m_avatarPath;
		wxBitmap avatarImage;
		if (avatarImage.LoadFile(wxStrImg, wxBITMAP_TYPE_ANY))
		{
			wxImage image = avatarImage.ConvertToImage();
			image.Rescale(16, 16);
			userItem->SetBitmap(image);
		}
        else
        {
            userItem->SetBitmap(defaultAvatarImage);
        }

        m_pLoginMenu->Append(userItem);
	}
	append_menu_item(m_pLoginMenu,
		wxID_ANY,
		_L("common_toptable_logout"),
		_L("Log Out AnkerMake"),
		[=](wxCommandEvent&) {
            ANKER_LOG_INFO << "use log out click";
            auto* ankerNet = AnkerNetInst();
            if (ankerNet) {
                ankerNet->logoutToServer();
            }
            LogOut();

		});

    SetWebviewTestItem();
    ANKER_LOG_INFO << "Leave ShowLoginedMenu";
}

void MainFrame::LogOut()
{    
    ANKER_LOG_INFO << "log out start";
    
    auto* ankerNet = AnkerNetInst();
    if (ankerNet) {
        ankerNet->logout();
        ankerNet->closeVideoStream(VIDEO_CLOSE_BY_LOGOUT);
    }    
    ShowUnLoginMenu();
    onLogOut();

    if (m_MsgCentreDialog)
    {
        m_isMsgCenterIsShow = false;
        m_MsgCentreDialog->Hide();           
    }

    ShowUnLoginDevice();
}

void MainFrame::loginFinishHandle()
{
    ANKER_LOG_INFO << "loginFinishHandle enter";
    auto ankerNet = AnkerNetInst();
    if (!ankerNet) {
        return;
    }

    ANKER_LOG_INFO << "login back start";
    std::string url = ankerNet->GetAvatar();
    wxString filePath = wxString();

    setUserInfoForSentry();

    wxStandardPaths standarPaths = wxStandardPaths::Get();
    filePath = standarPaths.GetUserDataDir();
    filePath = filePath + "/cache/" + wxString::FromUTF8(ankerNet->GetUserId()) + ".png";

    auto appConfig = Slic3r::GUI::wxGetApp().app_config;
    if (nullptr == appConfig) {
        ANKER_LOG_INFO << "02mmPrinter  nohint for user set.";
        return;
    }
    appConfig->set("user_id", ankerNet->GetUserId());


#ifndef __APPLE__
    filePath.Replace("\\", "/");
#endif        
    // AnkerMake Studio Profile/cache
    m_avatarPath = filePath;

    //if avatart not exists
    if (!wxFileExists(m_avatarPath) && url.size() > 0) {
        ankerNet->AsyDownLoad(
            url,
            filePath.ToStdString(wxConvUTF8),
            this,
            onDownLoadFinishedCallBack,
            onDownLoadProgress, true);
    }

    {
        wxLogNull logNo;
        wxFile file(m_avatarPath);
        wxFileOffset size = 0;
        if (file.IsOpened()) {
            size = file.Length();
        }

        if (size <= 0 && url.size() > 0)
        {
            ankerNet->AsyDownLoad(
                url,
                filePath.ToStdString(wxConvUTF8),
                this,
                onDownLoadFinishedCallBack,
                onDownLoadProgress, true);
        }

        if (url.size() <= 0)
        {
            m_avatarPath = "nullptr";
        }

        ShowLoginedMenu();
        ANKER_LOG_INFO << "login back finish0";
    }
    updateCurrentEnvironment();
    updateBuryInfo();

    //wxGetApp().filamentMaterialManager()->AsyncUpdate();
    ANKER_LOG_INFO << "login back finish";

    //loginWebview->Close();
    
#ifdef _WIN32  // for windows
    ankerNet->ProcessWebLoginFinish();
#endif
    if (m_pDeviceWidget)
        m_pDeviceWidget->loadDeviceList();


    HintDatabase::get_instance().reinit();

    QueryDataShared(nullptr);
    ANKER_LOG_INFO << "loginFinishHandle leave";
}


void MainFrame::ShowUnLoginDevice()
{
    if (m_pDeviceWidget) {
        m_pDeviceWidget->showUnlogin();
    }
}

void MainFrame::ShowUnLoginMenu()
{
    ShowUnLoginDevice();
    
	if (!m_pLoginMenu)
		return;

    ClearLoingiMenu();

    auto ankerNet = AnkerNetInst();
    if (ankerNet) {
        ankerNet->logout();
    }    
   
    updateCurrentEnvironment();

    updateBuryInfo();

	append_menu_item(m_pLoginMenu,
		wxID_ANY,
		_L("common_toptable_login"),
		_L("Sign In AnkerMake"),
		[=](wxCommandEvent&) {                     
            ShowAnkerWebView("login menu button clicked");
		});

    SetWebviewTestItem();
}


void MainFrame::ClearLoingiMenu()
{
    if (!m_pLoginMenu) {
        return;
    }

	int count = m_pLoginMenu->GetMenuItemCount();

	for (int i = 0; i <= count; i++) {
		wxMenuItem* menuItem = m_pLoginMenu->FindItemByPosition(0);
		if (menuItem) {			
            m_pLoginMenu->Remove(menuItem);
            delete menuItem;
		}
	}
}

void MainFrame::onLogOut()
{
    m_isMsgCenterIsShow = false;
    m_MsgCentreDialog->Hide();    
    clearStarCommentData();
    RemovePrivacyChoices();
    if (m_loginWebview)
        m_loginWebview->onLogOut();

    if (m_plater)
        m_plater->setStarCommentFlagsTimes(-1);
}


void MainFrame::OnMove(wxMoveEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_MAINWIN_MOVE);
	ProcessEvent(evt);
}

wxMenu* MainFrame::GetHelpMenu()
{
    if (!m_menubar) {
        return nullptr;
    }
    int menuIndex = m_menubar->FindMenu(_L("common_menu_title_help"));
    if (menuIndex == wxNOT_FOUND) {
        ANKER_LOG_WARNING << "help menu not found";
        return nullptr;
    }
    return m_menubar->GetMenu(menuIndex);
}

void MainFrame::DealPrivacyChoices(const wxCommandEvent& event)
{
    wxIntPtr* clientData = static_cast<wxIntPtr*>(event.GetClientData());
    if (!clientData) {
        ANKER_LOG_WARNING << "client data or menubar is nullptr";
        return;
    }
    auto helpMenu = GetHelpMenu();
    if (!helpMenu) {
        ANKER_LOG_WARNING << "help menu is nullptr";
        return;
    }

    bool isShow = static_cast<bool>(*clientData);       
    auto item = helpMenu->FindItem(ID_PRIVACY_CHOICES_ITEM);

    ANKER_LOG_INFO << "get your privacy choices item: " << item <<", isShow: " << isShow;
    if (item && !isShow) {
        helpMenu->Remove(item);
    }
    if (!item && isShow) {
        const int privacyChoicesItemPos = 2;
        append_menu_item(helpMenu, ID_PRIVACY_CHOICES_ITEM, _L("common_menu_help_privacychoices"), "",
            [](wxCommandEvent&) {
                wxString url = wxString("https://passport.ankermake.com/privacy-request?app=ankermake-us");
                wxURI uri(url);
                url = uri.BuildURI();
                bool success = wxLaunchDefaultBrowser(url);
        }, "", nullptr, []() { return true; }, nullptr, privacyChoicesItemPos);
    }
    delete clientData;
}

void MainFrame::RemovePrivacyChoices()
{
    auto helpMenu = GetHelpMenu();
    if (!helpMenu) {
        ANKER_LOG_WARNING << "help menu is nullptr";
        return;
    }
    auto item = helpMenu->FindItem(ID_PRIVACY_CHOICES_ITEM);
    if (item) {
        helpMenu->Remove(item);
    }
}

void MainFrame::SetWebviewTestItem()
{
    auto testOpen = wxGetApp().app_config->get_bool("Debug", "open_webview_test");
    if (testOpen) {
        append_menu_item(m_pLoginMenu,
            wxID_ANY,
            "AnkerWeb Test",
            "AnkerWeb Test",
            [=](wxCommandEvent&) {
                TestAnkerWebview();
            });

        append_menu_item(m_pLoginMenu,
            wxID_ANY,
            "Local Browser Test",
            "Local Browser Test",
            [=](wxCommandEvent&) {
                TestLoacalBrowser();
            });
    }
}

void MainFrame::TestAnkerWebview()
{
    wxString defaultUrl = "https://www.google.com";
    wxTextEntryDialog dialog(this, "Enter your url:", "Input Dialog", defaultUrl);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    defaultUrl = dialog.GetValue();
    ANKER_LOG_INFO << "load " << defaultUrl << " to webview for test";

    wxSize loginWebViewSize = AnkerSize(900, 700);
    wxPoint loginWebViewPos = wxPoint((GetSize().x - loginWebViewSize.x) / 2, (GetSize().y - loginWebViewSize.y) / 2);
    std::shared_ptr<AnkerWebView> loginWebview(new AnkerWebView(this, wxID_ANY,
        _L("common_toptable_login"), defaultUrl, loginWebViewPos, loginWebViewSize));

    loginWebview->SetWebViewSize(AnkerSize(900, 700));
    wxPoint winPoint;
    winPoint.x = this->GetRect().x + (GetRect().GetWidth() - loginWebview->GetRect().GetWidth()) / 2;
    winPoint.y = this->GetRect().y + (GetRect().GetHeight() - loginWebview->GetRect().GetHeight()) / 2;
    loginWebview->Move(winPoint);
    loginWebview->Clear();
    loginWebview->ShowModal();
}

void MainFrame::TestLoacalBrowser()
{
    auto url = getLoginUrl();
    ANKER_LOG_INFO << "loacal browser: " << url;
    std::string realUrl = Slic3r::UrlDecode(url.ToUTF8().data());
    wxLaunchDefaultBrowser(realUrl.c_str());
}

void MainFrame::InitDeviceWidget()
{
    DatamangerUi::GetInstance().SetMainWindow(this);
    if (m_pDeviceWidget) {
        m_pDeviceWidget->Init();
    }
}

void MainFrame::ShowAnkerWebView(const std::string& from)
{    
    ANKER_LOG_INFO << from;
    // download ankernet plugin
    if (!DatamangerUi::GetInstance().LoadNetLibrary()) {
        ANKER_LOG_ERROR << "load ankernet plugin failed";
        return;
    }
    InitDeviceWidget();

    ANKER_LOG_INFO << from << ", start call ShowAnkerWebView";
    {
        ANKER_LOG_INFO << "webview start to lock 01 ...";
        //std::unique_lock lock(m_ReadWriteMutex);
        if(m_bIsOpenWebview)
        {
            ANKER_LOG_INFO << "end  call ShowAnkerWebView for opened";
            return;
        }
        ANKER_LOG_INFO << "set webview is open to true";
        m_bIsOpenWebview = true;
    }

    if (m_loginWebview)
        m_loginWebview->SetForceClose(true);

    auto loginwebview = CreateWebView(false);

    ANKER_LOG_INFO<<"before call loginWebview->ShowModal()";
#ifdef _WIN32
    loginwebview->Show();
#else
    loginwebview->ShowModal();
#endif    
    loginwebview->SetShowErrorEnable(false);
    ANKER_LOG_INFO<<"end call loginWebview->ShowModal()";

    if (m_loginWebview) {
        ANKER_LOG_INFO << "before delete webview";
        delete m_loginWebview;
        m_loginWebview = nullptr;
        ANKER_LOG_INFO << "end delete webview";
    }
    m_loginWebview = std::move(loginwebview);
    {
        ANKER_LOG_INFO << "webview start to lock 02 ...";
        //std::unique_lock lock(m_ReadWriteMutex);
        ANKER_LOG_INFO << "set webview is open to false";
        m_bIsOpenWebview = false;
    }

    ANKER_LOG_INFO << from << ", end call ShowAnkerWebView";
}

MainFrame::MainFrame(const int font_point_size) :
DPIFrame(NULL, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, "mainframe", font_point_size),
    m_printhost_queue_dlg(new PrintHostQueueDialog(this))
    , m_recent_projects(9)
    , m_settings_dialog(this)
    , diff_dialog(this)
{
    // Fonts were created by the DPIFrame constructor for the monitor, on which the window opened.
    wxGetApp().update_fonts(this);

    AnkerBase ankerBase;
    //Bind(wxEVT_MOVE, wxMoveEventHandler(MainFrame::OnMove));
    Connect(wxEVT_MOVE, wxMoveEventHandler(MainFrame::OnMove));

    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        // add by allen, we must new AnkerConfigDlg in here to solve the problem of item showed in taskbar;

        ANKER_LOG_DEBUG << "createAnkerCfgDlg start";
        createAnkerCfgDlg();
        ANKER_LOG_DEBUG << "createAnkerCfgDlg end";
    });
/*
#ifndef __WXOSX__ // Don't call SetFont under OSX to avoid name cutting in ObjectList 
    this->SetFont(this->normal_font());
#endif
    // Font is already set in DPIFrame constructor
*/

#ifdef __APPLE__
    // Initialize the docker task bar icon.
    switch (wxGetApp().get_app_mode()) {
    default:
    case GUI_App::EAppMode::Editor:
        m_taskbar_icon = std::make_unique<AnkerStudioTaskBarIcon>(wxTBI_DOCK);
        m_taskbar_icon->SetIcon(wxIcon(Slic3r::var("AnkerStudio_128px.png"), wxBITMAP_TYPE_PNG), "AnkerStudio");
        break;
    case GUI_App::EAppMode::GCodeViewer:
        m_taskbar_icon = std::make_unique<GCodeViewerTaskBarIcon>(wxTBI_DOCK);
        m_taskbar_icon->SetIcon(wxIcon(Slic3r::var("AnkerStudio-gcodeviewer_128px.png"), wxBITMAP_TYPE_PNG), "G-code Viewer");
        break;
    }
#endif // __APPLE__

    // Load the icon either from the exe, or from the ico file.
    SetIcon(main_frame_icon(wxGetApp().get_app_mode()));
    ANKER_LOG_INFO << "init tab panel";
    initTabPanel();
    BindEvent();
    initAnkerUi();
}

MainFrame::~MainFrame()
{
    if (m_MsgCenterCfg)
    {
        delete m_MsgCenterCfg;
        m_MsgCenterCfg = nullptr;
    }

    if (m_MsgCenterErrCodeInfo)
    {
        delete m_MsgCenterErrCodeInfo;
        m_MsgCenterErrCodeInfo = nullptr;
    }

    // for crash when app exception exit
    if (!m_normalExit) {
        ANKER_LOG_INFO << "Abnormal program exit";
        this->shutdown();
    }
}

void MainFrame::initTabPanel() {
    // initialize status bar
//    m_statusbar = std::make_shared<ProgressStatusBar>(this);
//    m_statusbar->set_font(GUI::wxGetApp().normal_font());
//    if (wxGetApp().is_editor())
//        m_statusbar->embed(this);
//    m_statusbar->set_status_text(_L("Version") + " " +
//        SLIC3R_VERSION + " - " +
//       _L("Remember to check for updates at https://github.com/prusa3d/PrusaSlicer/releases"));

    // initialize tabpanel and menubar
    init_tabpanel();
    if (wxGetApp().is_gcode_viewer())
        init_menubar_as_gcodeviewer();
    else
        init_menubar_as_editor();

#if _WIN32
    // This is needed on Windows to fake the CTRL+# of the window menu when using the numpad
    wxAcceleratorEntry entries[6];
    entries[0].Set(wxACCEL_CTRL, WXK_NUMPAD1, wxID_HIGHEST + 1);
    entries[1].Set(wxACCEL_CTRL, WXK_NUMPAD2, wxID_HIGHEST + 2);
    entries[2].Set(wxACCEL_CTRL, WXK_NUMPAD3, wxID_HIGHEST + 3);
    entries[3].Set(wxACCEL_CTRL, WXK_NUMPAD4, wxID_HIGHEST + 4);
    entries[4].Set(wxACCEL_CTRL, WXK_NUMPAD5, wxID_HIGHEST + 5);
    entries[5].Set(wxACCEL_CTRL, WXK_NUMPAD6, wxID_HIGHEST + 6);
    wxAcceleratorTable accel(6, entries);
    SetAcceleratorTable(accel);
#endif // _WIN32

    // set default tooltip timer in msec
    // SetAutoPop supposedly accepts long integers but some bug doesn't allow for larger values
    // (SetAutoPop is not available on GTK.)
    wxToolTip::SetAutoPop(32767);

    m_loaded = true;

    // initialize layout
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_main_sizer, 1, wxEXPAND);
    SetSizer(sizer);
    //init function panel

    //by Samuel, only Editor mode should show function tab Control 
    GUI::GUI_App* gui = dynamic_cast<GUI::GUI_App*>(GUI::GUI_App::GetInstance());
    if (gui->get_app_mode() != GUI::GUI_App::EAppMode::GCodeViewer)
    {
        wxSize panelSize = GetSize();
        panelSize.SetHeight(36);
        m_pFunctionPanel = new AnkerFunctionPanel(this, wxID_ANY);
        m_pFunctionPanel->Bind(wxCUSTOMEVT_FEEDBACK, [this](wxCommandEvent& event) {
            auto ankerNet = AnkerNetInst();
            if (!ankerNet || !ankerNet->IsLogined()) {
                wxGetApp().mainframe->ShowAnkerWebView("feedback button clicked");
            }
            else {
                wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
                wxSize mfSize = wxGetApp().mainframe->GetClientSize();
                wxSize dialogSize = AnkerSize(400, 404);
                wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
                wxString title = _L("common_feedback_title");
                AnkerFeedbackDialog dialog(nullptr, title.ToStdString(), center, dialogSize);
                if (dialog.ShowModal() == wxID_OK) {
                    auto feedback = dialog.GetFeedBack();
                    ankerNet->AsyPostFeedBack(feedback);
                }
            }
            });
        m_pFunctionPanel->Bind(wxCUSTOMEVT_SHOW_DOC, [this](wxCommandEvent& event) {
            //std::string realUrl = "";
            //wxLaunchDefaultBrowser(realUrl.c_str());
            });
        m_pFunctionPanel->Bind(wxCUSTOMEVT_RELEASE_NOTE, [this](wxCommandEvent& event) {
            std::string realUrl = "https://github.com/ankermake/AnkerMake-PrusaSlicer-Release/releases";
            wxLaunchDefaultBrowser(realUrl.c_str());
            });

        m_pFunctionPanel->SetMinSize(AnkerSize(0, 36));
        m_main_sizer->Add(m_pFunctionPanel, 0, wxEXPAND, 0);
        m_pFunctionPanel->Show();
        m_pFunctionPanel->SetPrintTab(m_printTabPanel);

        m_pFunctionPanel->Bind(wxCUSTOMEVT_SHOW_MSG_CENTRE, [=](wxCommandEvent& ev) {
            ANKER_LOG_INFO << "show msg centre";

            auto ankerNet = AnkerNetInst();
            if (!ankerNet || !ankerNet->IsLogined()) {
                wxGetApp().mainframe->ShowAnkerWebView("msg center request to loiginb ");
            }

            if(ankerNet->IsLogined())
            {
                wxPoint winPoint;
                winPoint.x = this->GetRect().x + (GetRect().GetWidth() - m_MsgCentreDialog->GetRect().GetWidth()) / 2;
                winPoint.y = this->GetRect().y + (GetRect().GetHeight() - m_MsgCentreDialog->GetRect().GetHeight()) / 2;

                wxVariant* pData = (wxVariant*)(ev.GetClientData());
                
                bool isShowOfficical = true;
                if (pData)
                {
                    wxVariantList list = pData->GetList();
                    isShowOfficical = list[0]->GetBool();                    
                }

                if (m_MsgCentreDialog)
                {                            
                    m_MsgCentreDialog->clearMsg();
                    m_MsgCentreDialog->Move(winPoint);                    
                    m_MsgCentreDialog->Raise();                    
                    m_isMsgCenterIsShow = true;                    
                    m_MsgCentreDialog->Show();
                    m_MsgCentreDialog->getMsgCenterRecords(true);
                }
            }
            });
        m_pFunctionPanel->m_calib_menu = m_calibration_menu;
    }
    m_sliceCommentDialog = new AnkerSliceCommentDialog(this, _L("Rate Your Experience"));
    m_sliceCommentDialog->SetMaxSize(AnkerSize(400,420));
    m_sliceCommentDialog->SetMinSize(AnkerSize(400, 420));
    m_sliceCommentDialog->SetSize(AnkerSize(400, 420));
    m_sliceCommentDialog->Hide();
    m_sliceCommentDialog->Bind(wxCUSTOMEVT_ANKER_COMMENT_NOT_ASK, [this](wxCommandEvent& event) {
        g_sliceCommentData.action = 3;        
        auto ankerNet = AnkerNetInst();
        if (ankerNet && ankerNet->IsLogined()) {
            ankerNet->reportCommentData(g_sliceCommentData);
        }
        });
    m_sliceCommentDialog->Bind(wxCUSTOMEVT_ANKER_COMMENT_SUBMIT, [this](wxCommandEvent& event) {
        g_sliceCommentData.action = 1;      
        wxVariant* pData = (wxVariant*)event.GetClientData();
        if (pData) {
            wxVariantList list = pData->GetList();
            g_sliceCommentData.rating = list[0]->GetInteger();
            g_sliceCommentData.reviewData = list[1]->GetString().utf8_str();
        }
        
        auto ankerNet = AnkerNetInst();
        if (ankerNet && ankerNet->IsLogined()) {
            ankerNet->reportCommentData(g_sliceCommentData);
        }
        });
    m_sliceCommentDialog->Bind(wxCUSTOMEVT_ANKER_COMMENT_CLOSE, [this](wxCommandEvent& event) {
        g_sliceCommentData.action = 2;        
        auto ankerNet = AnkerNetInst();
        if (ankerNet && ankerNet->IsLogined()) {
            ankerNet->reportCommentData(g_sliceCommentData);
        }
        });
    // initialize layout from config
    update_layout();
    sizer->SetSizeHints(this);
    Fit();

    const wxSize min_size = wxGetApp().get_min_size(); //wxSize(76*wxGetApp().em_unit(), 49*wxGetApp().em_unit());
#ifdef __APPLE__
    // Using SetMinSize() on Mac messes up the window position in some cases
    // cf. https://groups.google.com/forum/#!topic/wx-users/yUKPBBfXWO0
    SetSize(min_size/*wxSize(760, 490)*/);
    // mod by allen for setting the min size of mainframe
    SetMinSize(min_size/*wxSize(760, 490)*/);
#else

    SetMinSize(min_size/*wxSize(760, 490)*/);
    SetSize(GetMinSize());
#endif
    Layout();

    update_title();

    // declare events
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (event.CanVeto() && m_plater->canvas3D()->get_gizmos_manager().is_in_editing_mode(true)) {
            // prevents to open the save dirty project dialog
            event.Veto();
            return;
        }

        if (m_plater != nullptr) {
            int saved_project = m_plater->save_project_if_dirty(_L("Closing AnkerMake Studio. Current project is modified."));
            if (saved_project == wxID_CANCEL) {
                event.Veto();
                return;
            }
            // check unsaved changes only if project wasn't saved
            else if (plater()->is_project_dirty() && saved_project == wxID_NO && event.CanVeto() &&
                (plater()->is_presets_dirty() && !wxGetApp().check_and_save_current_preset_changes(_L("AnkerMake Studio is closing"), _L("Closing AnkerMake Studio while some presets are modified.")))) {
                event.Veto();
                return;
            }
			else
			{
				plater()->sidebarnew().checkDirtyDataonParameterpanel();
			}
        }

        if (event.CanVeto() && !wxGetApp().check_print_host_queue()) {
            event.Veto();
            return;
        }

        if (false == plater()->is_exporting_acode()) {

            //report: exit soft

            std::string durationStr = getWorkDuration();
            std::string errorCode = std::string("0");
            std::string errorMsg = std::string("exit soft");

            std::map<std::string, std::string> map;
            map.insert(std::make_pair(c_es_error_code, errorCode));
            map.insert(std::make_pair(c_es_error_msg, errorMsg));
            map.insert(std::make_pair(c_exit_startup_duration, durationStr));
            ANKER_LOG_INFO << "Report bury event is " << e_exit_soft;
            reportBuryEvent(e_exit_soft, map, true);
            
            this->shutdown();
            m_normalExit = true;
            // propagate event
            event.Skip();
            wxExit();
        }
        else {
            // not safe to shutdown, stop acode expoting task first
            ANKER_LOG_INFO << "gcode export is runing ,stop it first";
            m_normalExit = true;
            plater()->set_app_closing(true);
            plater()->stop_exporting_acode();

            //event.Skip();
        }
        });

    Bind(wxCUSTOMEVT_EXPORT_FINISHED_SAFE_QUIT_APP, [this](wxCommandEvent& event) {
        if (m_normalExit && false == plater()->is_exporting_acode()) {
            ANKER_LOG_INFO << "gcode export is finished ,safe to shutdown";
            this->shutdown();
            event.Skip();
            wxExit();
        }
        });

    //FIXME it seems this method is not called on application start-up, at least not on Windows. Why?
    // The same applies to wxEVT_CREATE, it is not being called on startup on Windows.
    Bind(wxEVT_ACTIVATE, [this](wxActivateEvent& event) {
        if (m_plater != nullptr && event.GetActive())
            m_plater->on_activate();
        event.Skip();
        });

    Bind(wxEVT_SIZE, &MainFrame::on_size, this);
    Bind(wxEVT_MOVE, &MainFrame::on_move, this);
    Bind(wxEVT_SHOW, &MainFrame::on_show, this);
    Bind(wxEVT_ICONIZE, &MainFrame::on_minimize, this);
    Bind(wxEVT_MAXIMIZE, &MainFrame::on_maximize, this);
    Bind(wxEVT_ACTIVATE, &MainFrame::on_Activate, this);

    //
    m_printTabPanel->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event) {
        int iSelectedPage = event.GetId();

        if (iSelectedPage == 0) {
            m_currentTabMode = TabMode::TAB_SLICE;
            m_pMsgCentrePopWindow->Hide();

        } else if (iSelectedPage == 1) {
            m_currentTabMode = TabMode::TAB_DEVICE;
            if (m_hasErrDialog)
            {
                ShowErrDialogByCenter();
            }
            auto ankerNet = AnkerNetInst();
            if (ankerNet && ankerNet->IsLogined()) {
                ankerNet->AsyRefreshDeviceList();
            }
        }

        m_printTabPanel->SetSelection(iSelectedPage);
        });

    // OSX specific issue:
    // When we move application between Retina and non-Retina displays, The legend on a canvas doesn't redraw
    // So, redraw explicitly canvas, when application is moved
    //FIXME maybe this is useful for __WXGTK3__ as well?
#if __APPLE__
    Bind(wxEVT_MOVE, [](wxMoveEvent& event) {
        wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
        wxGetApp().plater()->get_current_canvas3D()->request_extra_frame();
        event.Skip();
        });
#endif

    wxGetApp().persist_window_geometry(this, true);
    wxGetApp().persist_window_geometry(&m_settings_dialog, true);

    update_ui_from_settings();    // FIXME (?)

    if (m_plater != nullptr) {
        m_plater->get_collapse_toolbar().set_enabled(wxGetApp().app_config->get_bool("show_collapse_button"));
        m_plater->show_action_buttons(true);

        preferences_dialog = new PreferencesDialog(this);
    }

    // bind events from DiffDlg

    bind_diff_dialog();
}

void MainFrame::setUserInfoForSentry()
{    
    
#ifdef WIN32
    auto ankerNet = AnkerNetInst();
    if(ankerNet->IsLogined())
    {    
        std::string userEmail = ankerNet->GetUserEmail();
        std::string nickName = ankerNet->GetNickName();
      
#ifndef OPEN_SOURCE_MODE
        sentry_set_tag("email", userEmail.c_str());
        sentry_set_tag("nick_name", nickName.c_str());
#endif    
    }    

#endif
}


void MainFrame::createAnkerCfgDlg() {
    if (!m_ankerCfgDlg) {
        ANKER_LOG_INFO << "createAnkerCfgDlg enter";
        m_ankerCfgDlg = new AnkerConfigDlg(this);

        // add by allen for ankerCfgDlg
        m_ankerCfgDlg->Bind(wxCUSTOMEVT_UPDATE_PARAMETERS_PANEL, [this](wxCommandEvent& event) {
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_RELOAD_DATA);
            evt.SetEventObject(this);
            ProcessEvent(evt);
            });

        // add by allen for ankerCfgDlg
        auto pAnkerTabPrint = new AnkerTabPrint(m_ankerCfgDlg->m_rightPanel);
        pAnkerTabPrint->Bind(wxCUSTOMEVT_ANKER_SAVE_PRESET, [this](wxCommandEvent& event) {
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_RELOAD_DATA);
            evt.SetEventObject(this);
            ProcessEvent(evt);
            });
        m_ankerCfgDlg->AddCreateTab(pAnkerTabPrint, "cog");
        m_ankerCfgDlg->AddCreateTab(new AnkerTabFilament(m_ankerCfgDlg->m_rightPanel), "spool");
        m_ankerCfgDlg->AddCreateTab(new AnkerTabPrinter(m_ankerCfgDlg->m_rightPanel),
            wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF ? "printer" : "sla_printer");
        // we must call this functions as follows after AnkerConfigDialog created
        if (wxGetApp().is_editor())
            wxGetApp().load_current_presets();
             // Save the active profiles as a "saved into project".
            wxGetApp().update_saved_preset_from_current_preset();
            if (wxGetApp().plater_ != nullptr) {
                // Save the names of active presets and project specific config into ProjectDirtyStateManager.
                wxGetApp().plater_->reset_project_dirty_initial_presets();
                // Update Project dirty state, update application title bar.
                wxGetApp().plater_->update_project_dirty_from_presets();
            }
            select_tab(size_t(0));
    }
}

void MainFrame::OnDocumentLoaded(wxWebViewEvent& evt)
{
    //web load finisehd
}


void MainFrame::OnScriptMessage(wxCommandEvent& evt)
{
    //on rec web request
}

void MainFrame::bind_diff_dialog()
{
    auto get_tab = [](Preset::Type type) {
        Tab* null_tab = nullptr;
        for (Tab* tab : wxGetApp().tabs_list)
            if (tab->type() == type)
                return tab;
        return null_tab;
    };

    auto transfer = [this, get_tab](Preset::Type type) {
        get_tab(type)->transfer_options(diff_dialog.get_left_preset_name(type),
                                        diff_dialog.get_right_preset_name(type),
                                        diff_dialog.get_selected_options(type));
    };

    auto update_presets = [this, get_tab](Preset::Type type) {
        get_tab(type)->update_preset_choice();
        //m_plater->sidebar().update_presets(type);
        m_plater->sidebarnew().updatePresets(type);
    };

    auto process_options = [this](std::function<void(Preset::Type)> process) {
        const Preset::Type diff_dlg_type = diff_dialog.view_type();
        if (diff_dlg_type == Preset::TYPE_INVALID) {
            for (const Preset::Type& type : diff_dialog.types_list() )
                process(type);
        }
        else
            process(diff_dlg_type);
    };

    diff_dialog.Bind(EVT_DIFF_DIALOG_TRANSFER,      [process_options, transfer](SimpleEvent&)         { process_options(transfer); });

    diff_dialog.Bind(EVT_DIFF_DIALOG_UPDATE_PRESETS,[process_options, update_presets](SimpleEvent&)   { process_options(update_presets); });

    // add by allen for ankerCfgDlg
    bind_diff_dialog_ankertab();
}

void MainFrame::bind_diff_dialog_ankertab()
{
    auto transfer = [this](Preset::Type type) {
        AnkerTab* ankerTab = wxGetApp().getAnkerTab(type);
        if (ankerTab) {
            ankerTab->transfer_options(diff_dialog.get_left_preset_name(type),
                diff_dialog.get_right_preset_name(type),
                diff_dialog.get_selected_options(type));
        }  
    };

    auto update_presets = [this](Preset::Type type) {
        AnkerTab* ankerTab = wxGetApp().getAnkerTab(type);
        if (ankerTab) {
            wxGetApp().getAnkerTab(type)->update_preset_choice();
           // m_plater->sidebar().update_presets(type);
            m_plater->sidebarnew().updatePresets(type);
        }
    };

    auto process_options = [this](std::function<void(Preset::Type)> process) {
        const Preset::Type diff_dlg_type = diff_dialog.view_type();
        if (diff_dlg_type == Preset::TYPE_INVALID) {
            for (const Preset::Type& type : diff_dialog.types_list())
                process(type);
        }
        else
            process(diff_dlg_type);
    };

    diff_dialog.Bind(EVT_DIFF_DIALOG_TRANSFER, [process_options, transfer](SimpleEvent&) { process_options(transfer); });

    diff_dialog.Bind(EVT_DIFF_DIALOG_UPDATE_PRESETS, [process_options, update_presets](SimpleEvent&) { process_options(update_presets); });
}


size_t MainFrame::onDownLoadFinishedCallBack(char* dest, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;    
}


void MainFrame::onDownLoadProgress(double dltotal, double dlnow, double ultotal, double ulnow)
{
    
}


std::string MainFrame::getAppName()
{
    std::string strAppname = std::string();

#ifdef _WIN32
	WCHAR buffer[MAX_PATH] = { 0 };
	if (GetCurrentDirectory(MAX_PATH, buffer))
	{
		std::wstring wstrPath = buffer;
		std::string strAppname(wstrPath.begin(), wstrPath.end());
		int pos = strAppname.find_last_of("\\");
        strAppname = strAppname.substr(pos + 1, strAppname.size());
	}
#endif
    return strAppname;
}

void MainFrame::updateCurrentEnvironment()
{
    auto ankerNet = AnkerNetInst();
    if (!ankerNet || !ankerNet->IsLogined()) 
    {
        wxString url = AnkerConfig::getankerDomainUrl();
        if (url.Contains(wxT("qa")))
            m_currentEnvir = QA_ENVIR;        
        else if (url.Contains(wxT("ci")))
            m_currentEnvir = CI_ENVIR;
        else
            m_currentEnvir = US_ENVIR;//DEFAULT
        return;
    }

    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj)
    {
        auto currentEnv = obj->getCurrentEnvironmentType();
        m_currentEnvir = ANKER_ENVIR(currentEnv);
    }

}
void MainFrame::updateBuryInfo()
{
    auto para = DatamangerUi::GetInstance().GetNetPara();
    std::string envir = "US";
    std::string userInfo = std::string();
    std::string userId = std::string();
    if (m_currentEnvir == EU_ENVIR)
        envir = "EU";
    else if (m_currentEnvir == QA_ENVIR)
        envir = "QA";
    else if(m_currentEnvir == US_ENVIR)
        envir = "US";
    else
        envir = "CI";
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj)
    {
        userInfo = obj->GetUserInfo();
        userId = obj->GetUserId();
    }
    setPluginParameter(userInfo, envir, userId, para.Openudid);
}

#ifdef _MSW_DARK_MODE
static wxString pref() { return " [ "; }
static wxString suff() { return " ] "; }
static void append_tab_menu_items_to_menubar(wxMenuBar* bar, PrinterTechnology pt, bool is_mainframe_menu)
{
    if (is_mainframe_menu)
        bar->Append(new wxMenu(), pref() + _L("Plater") + suff());
    for (const wxString& title : { is_mainframe_menu    ? _L("Print Settings")       : pref() + _L("Print Settings") + suff(),
                                   pt == ptSLA          ? _L("Material Settings")    : _L("Filament Settings"),
                                   _L("Printer Settings") })
        bar->Append(new wxMenu(), title);
}

// update markers for selected/unselected menu items
static void update_marker_for_tabs_menu(wxMenuBar* bar, const wxString& title, bool is_mainframe_menu)
{
    if (!bar)
        return;
    size_t items_cnt = bar->GetMenuCount();
    for (size_t id = items_cnt - (is_mainframe_menu ? 4 : 3); id < items_cnt; id++) {
        wxString label = bar->GetMenuLabel(id);
        if (label.First(pref()) == 0) {
            if (label == pref() + title + suff())
                return;
            label.Remove(size_t(0), pref().Len());
            label.RemoveLast(suff().Len());
            bar->SetMenuLabel(id, label);
            break;
        }
    }
    if (int id = bar->FindMenu(title); id != wxNOT_FOUND)
        bar->SetMenuLabel(id, pref() + title + suff());
}

static void add_tabs_as_menu(wxMenuBar* bar, MainFrame* main_frame, wxWindow* bar_parent)
{
    PrinterTechnology pt = main_frame->plater() ? main_frame->plater()->printer_technology() : ptFFF;

    bool is_mainframe_menu = bar_parent == main_frame;
    if (!is_mainframe_menu)
        append_tab_menu_items_to_menubar(bar, pt, is_mainframe_menu);

    bar_parent->Bind(wxEVT_MENU_OPEN, [main_frame, bar, is_mainframe_menu](wxMenuEvent& event) {
        wxMenu* const menu = event.GetMenu();
        if (!menu || menu->GetMenuItemCount() > 0) {
            // If we are here it means that we open regular menu and not a tab used as a menu
            event.Skip(); // event.Skip() is verry important to next processing of the wxEVT_UPDATE_UI by this menu items.
                          // If wxEVT_MENU_OPEN will not be pocessed in next event queue then MenuItems of this menu will never caught wxEVT_UPDATE_UI 
                          // and, as a result, "check/radio value" will not be updated
            return;
        }

        // update tab selection

        const wxString& title = menu->GetTitle();
        if (title == _L("Plater"))
            main_frame->select_tab(size_t(0));
        else if (title == _L("Print Settings"))
            main_frame->select_tab(wxGetApp().get_tab(main_frame->plater()->printer_technology() == ptFFF ? Preset::TYPE_PRINT : Preset::TYPE_SLA_PRINT));
        else if (title == _L("Filament Settings"))
            main_frame->select_tab(wxGetApp().get_tab(Preset::TYPE_FILAMENT));
        else if (title == _L("Material Settings"))
            main_frame->select_tab(wxGetApp().get_tab(Preset::TYPE_SLA_MATERIAL));
        else if (title == _L("Printer Settings"))
            main_frame->select_tab(wxGetApp().get_tab(Preset::TYPE_PRINTER));

        // update markers for selected/unselected menu items
        update_marker_for_tabs_menu(bar, title, is_mainframe_menu);
    });
}

void MainFrame::show_tabs_menu(bool show)
{
    if (show)
        append_tab_menu_items_to_menubar(m_menubar, plater() ? plater()->printer_technology() : ptFFF, true);
    else
        while (m_menubar->GetMenuCount() >= 8) {
            if (wxMenu* menu = m_menubar->Remove(7))
                delete menu;
        }
}
#endif // _MSW_DARK_MODE

void MainFrame::update_layout()
{
    auto restore_to_creation = [this]() {
        auto clean_sizer = [](wxSizer* sizer) {
            while (!sizer->GetChildren().IsEmpty()) {
                sizer->Detach(0);
            }
        };

        // On Linux m_plater needs to be removed from m_tabpanel before to reparent it
        int plater_page_id = m_tabpanel->FindPage(m_plater);
        if (plater_page_id != wxNOT_FOUND)
            m_tabpanel->RemovePage(plater_page_id);

        if (m_plater->GetParent() != this)
            m_plater->Reparent(this);

        if (m_tabpanel->GetParent() != this)
            m_tabpanel->Reparent(this);

        if (m_printTabPanel->GetParent() != this)
            m_printTabPanel->Reparent(this);

        plater_page_id = (m_plater_page != nullptr) ? m_tabpanel->FindPage(m_plater_page) : wxNOT_FOUND;
        if (plater_page_id != wxNOT_FOUND) {
            m_tabpanel->DeletePage(plater_page_id);
            m_plater_page = nullptr;
        }

        clean_sizer(m_main_sizer);
        clean_sizer(m_settings_dialog.GetSizer());

        if (m_settings_dialog.IsShown())
            m_settings_dialog.Close();

        m_tabpanel->Hide();
        m_plater->Hide();

        Layout();
    };

    ESettingsLayout layout = wxGetApp().is_gcode_viewer() ? ESettingsLayout::GCodeViewer :
        (wxGetApp().app_config->get_bool("old_settings_layout_mode") ? ESettingsLayout::Old :
         wxGetApp().app_config->get_bool("new_settings_layout_mode") ? ( wxGetApp().tabs_as_menu() ? ESettingsLayout::Old : ESettingsLayout::New) :
         wxGetApp().app_config->get_bool("dlg_settings_layout_mode") ? ESettingsLayout::Dlg : ESettingsLayout::Old);

    if (m_layout == layout)
        return;

    wxBusyCursor busy;

    Freeze();

    // Remove old settings
    if (m_layout != ESettingsLayout::Unknown)
        restore_to_creation();

#ifdef __WXMSW__
    enum class State {
        noUpdate,
        fromDlg,
        toDlg
    };
    State update_scaling_state = //m_layout == ESettingsLayout::Unknown   ? State::noUpdate   : // don't scale settings dialog from the application start
                                 m_layout == ESettingsLayout::Dlg       ? State::fromDlg    :
                                 layout   == ESettingsLayout::Dlg       ? State::toDlg      : State::noUpdate;
#endif //__WXMSW__
    m_printTabPanel->Show();

    ESettingsLayout old_layout = m_layout;
    m_layout = layout;

    // From the very beginning the Print settings should be selected
    m_last_selected_tab = m_layout == ESettingsLayout::Dlg ? 0 : 1;
   
    // Set new settings
    switch (m_layout)
    {
    case ESettingsLayout::Unknown:
    {
        break;
    }
    case ESettingsLayout::Old:
    {
        m_plater->Reparent(m_tabpanel);
#ifdef _MSW_DARK_MODE
        m_plater->Layout();
        if (!wxGetApp().tabs_as_menu())
            dynamic_cast<Notebook*>(m_tabpanel)->InsertPage(0, m_plater, _L("Plater"), std::string("plater"), true);
        else
#endif
        m_tabpanel->InsertPage(0, m_plater, _L("Plater"));
        m_main_sizer->Add(m_tabpanel, 1, wxEXPAND | wxTOP, 1);
        m_plater->Show();
        m_tabpanel->Show();
        // update Tabs
        if (old_layout == ESettingsLayout::Dlg)
            if (int sel = m_tabpanel->GetSelection(); sel != wxNOT_FOUND)
                m_tabpanel->SetSelection(sel+1);// call SetSelection to correct layout after switching from Dlg to Old mode
#ifdef _MSW_DARK_MODE
        if (wxGetApp().tabs_as_menu())
            show_tabs_menu(true);
#endif
        break;
    }
    case ESettingsLayout::New:
    {
        m_main_sizer->Add(m_plater, 1, wxEXPAND);
        m_tabpanel->Hide();
        m_main_sizer->Add(m_tabpanel, 1, wxEXPAND);
        m_plater_page = new wxPanel(m_tabpanel);
#ifdef _MSW_DARK_MODE
        if (!wxGetApp().tabs_as_menu())
            dynamic_cast<Notebook*>(m_tabpanel)->InsertPage(0, m_plater_page, _L("Plater"), std::string("plater"), true);
        else
#endif
        m_tabpanel->InsertPage(0, m_plater_page, _L("Plater")); // empty panel just for Plater tab */
        m_plater->Show();
        break;
    }
    case ESettingsLayout::Dlg:
    {
        m_plater->Reparent(m_printTabPanel);
//#ifdef _MSW_DARK_MODE
//        m_plater->Layout();
//        if (!wxGetApp().tabs_as_menu())
//            dynamic_cast<Notebook*>(m_printTabPanel)->InsertPage(0, m_plater, _L("Plater"), std::string("plater"), true);
//        else
//#endif
        m_printTabPanel->InsertPage(0, m_plater, _L("Plater"));
        m_currentTabMode = TAB_SLICE;
        if(m_pMsgCentrePopWindow)
            m_pMsgCentrePopWindow->Hide();
        m_printTabPanel->SetSelection(0);
        //by Samuel, printTabPanel no need show default border
        m_main_sizer->Add(m_printTabPanel, 1, wxEXPAND | wxTOP, 0);
        m_printTabPanel->Show();
      
        m_tabpanel->Reparent(&m_settings_dialog);
        m_settings_dialog.GetSizer()->Add(m_tabpanel, 1, wxEXPAND | wxTOP, 2);
        m_tabpanel->Show();
        m_plater->Show();

#ifdef _MSW_DARK_MODE
        if (wxGetApp().tabs_as_menu())
            show_tabs_menu(false);
#endif
        break;
    }
    case ESettingsLayout::GCodeViewer:
    {
        m_main_sizer->Add(m_plater, 1, wxEXPAND);
        m_plater->set_default_bed_shape();
        m_plater->get_collapse_toolbar().set_enabled(false);
        m_plater->collapse_sidebar(true);
        m_plater->Show();
        break;
    }
    }

#ifdef _MSW_DARK_MODE
    // Sizer with buttons for mode changing
    //m_plater->sidebar().show_mode_sizer(wxGetApp().tabs_as_menu() || m_layout != ESettingsLayout::Old);
#endif

#ifdef __WXMSW__
    if (update_scaling_state != State::noUpdate)
    {
        int mainframe_dpi   = get_dpi_for_window(this);
        int dialog_dpi      = get_dpi_for_window(&m_settings_dialog);
        if (mainframe_dpi != dialog_dpi) {
            wxSize oldDPI = update_scaling_state == State::fromDlg ? wxSize(dialog_dpi, dialog_dpi) : wxSize(mainframe_dpi, mainframe_dpi);
            wxSize newDPI = update_scaling_state == State::toDlg   ? wxSize(dialog_dpi, dialog_dpi) : wxSize(mainframe_dpi, mainframe_dpi);

            if (update_scaling_state == State::fromDlg)
                this->enable_force_rescale();
            else
                (&m_settings_dialog)->enable_force_rescale();

            wxWindow* win { nullptr };
            if (update_scaling_state == State::fromDlg)
                win = this;
            else
                win = &m_settings_dialog;

#if wxVERSION_EQUAL_OR_GREATER_THAN(3,1,3)
            m_tabpanel->MSWUpdateOnDPIChange(oldDPI, newDPI);
            win->GetEventHandler()->AddPendingEvent(wxDPIChangedEvent(oldDPI, newDPI));
#else
            win->GetEventHandler()->AddPendingEvent(DpiChangedEvent(EVT_DPI_CHANGED_SLICER, newDPI, win->GetRect()));
#endif // wxVERSION_EQUAL_OR_GREATER_THAN
        }
    }
#endif //__WXMSW__

//#ifdef __APPLE__
//    // Using SetMinSize() on Mac messes up the window position in some cases
//    // cf. https://groups.google.com/forum/#!topic/wx-users/yUKPBBfXWO0
//    // So, if we haven't possibility to set MinSize() for the MainFrame, 
//    // set the MinSize() as a half of regular  for the m_plater and m_tabpanel, when settings layout is in slNew mode
//    // Otherwise, MainFrame will be maximized by height
//    if (m_layout == ESettingsLayout::New) {
//        wxSize size = wxGetApp().get_min_size();
//        size.SetHeight(int(0.5 * size.GetHeight()));
//        m_plater->SetMinSize(size);
//        m_tabpanel->SetMinSize(size);
//    }
//#endif

#ifdef __APPLE__
    //m_plater->sidebar().change_top_border_for_mode_sizer(m_layout != ESettingsLayout::Old);
#endif
    
    Layout();
    Thaw();
}


void MainFrame::setUrl(std::string webUrl)
{
    if (webUrl.empty())
        return;

    auto currentLanguage = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
    wxString languageFlags = "en";
    //if (currentLanguage <= wxLANGUAGE_ENGLISH_ZIMBABWE && currentLanguage >= wxLANGUAGE_ENGLISH)  

    if (currentLanguage <= wxLANGUAGE_JAPANESE_JAPAN && currentLanguage >= wxLANGUAGE_JAPANESE)        
        languageFlags = "ja";
    else if ( (currentLanguage <= wxLANGUAGE_CHINESE_MACAU && currentLanguage >= wxLANGUAGE_CHINESE_SIMPLIFIED)||
              (currentLanguage <= wxLANGUAGE_CHINESE_TRADITIONAL_EXPLICIT && currentLanguage >= wxLANGUAGE_CHINESE)
            )
        languageFlags = "cn";
    

    m_loginUrl = webUrl+"?language="+ languageFlags;
    m_backloginUrl = webUrl + "?invisible=true";
}

// Called when closing the application and when switching the application language.
void MainFrame::shutdown(bool restart)
{
    ANKER_LOG_INFO << "MainFrame::shutdown() begin.";

#ifdef _WIN32
	if (m_hDeviceNotify) {
		::UnregisterDeviceNotification(HDEVNOTIFY(m_hDeviceNotify));
		m_hDeviceNotify = nullptr;
	}
 	if (m_ulSHChangeNotifyRegister) {
        SHChangeNotifyDeregister(m_ulSHChangeNotifyRegister);
        m_ulSHChangeNotifyRegister = 0;
 	}
#endif // _WIN32

    // add by allen for ankerCfgDlg
    if (m_ankerCfgDlg && m_ankerCfgDlg->IsShown()) {
        m_ankerCfgDlg->CloseDlg();
    }

    if (m_plater != nullptr) {
        m_plater->get_ui_job_worker().cancel_all();

        // Unbinding of wxWidgets event handling in canvases needs to be done here because on MAC,
        // when closing the application using Command+Q, a mouse event is triggered after this lambda is completed,
        // causing a crash
        m_plater->unbind_canvas_event_handlers();

        // Cleanup of canvases' volumes needs to be done here or a crash may happen on some Linux Debian flavours
        // see: https://github.com/prusa3d/PrusaSlicer/issues/3964
        m_plater->reset_canvas_volumes();
        m_plater->shutdown();
    }

    // Weird things happen as the Paint messages are floating around the windows being destructed.
    // Avoid the Paint messages by hiding the main window.
    // Also the application closes much faster without these unnecessary screen refreshes.
    // In addition, there were some crashes due to the Paint events sent to already destructed windows.
    this->Show(false);

    if (m_settings_dialog.IsShown())
        // call Close() to trigger call to lambda defined into GUI_App::persist_window_geometry()
        m_settings_dialog.Close();

    if (m_plater != nullptr) {
        // Stop the background thread (Windows and Linux).
        // Disconnect from a 3DConnextion driver (OSX).
        m_plater->get_mouse3d_controller().shutdown();
        // Store the device parameter database back to appconfig.
        m_plater->get_mouse3d_controller().save_config(*wxGetApp().app_config);
    }

    // Stop the background thread of the removable drive manager, so that no new updates will be sent to the Plater.
    wxGetApp().removable_drive_manager()->shutdown();
	//stop listening for messages from other instances
	wxGetApp().other_instance_message_handler()->shutdown(this);
    // Save the slic3r.ini.Usually the ini file is saved from "on idle" callback,
    // but in rare cases it may not have been called yet.
    if (wxGetApp().app_config->dirty())
        wxGetApp().app_config->save();
//         if (m_plater)
//             m_plater->print = undef;
//         Slic3r::GUI::deregister_on_request_update_callback();

    // set to null tabs and a plater
    // to avoid any manipulations with them from App->wxEVT_IDLE after of the mainframe closing 
#if SHOW_OLD_SETTING_DIALOG
    wxGetApp().tabs_list.clear();
#endif
   
    // add by allen for ankerCfgDlg
    wxGetApp().ankerTabsList.clear();

    wxGetApp().plater_ = nullptr;
    m_plater           = nullptr;

    CloseVideoStream(VIDEO_CLOSE_BY_APP_QUIT);
    DatamangerUi::GetInstance().ResetMainObj();

    auto ankerNet = AnkerNetInst();
    if (ankerNet && restart == false) {
        ankerNet->UnInit();
    }
    ANKER_LOG_INFO << "MainFrame::shutdown() end.";
}


GalleryDialog* MainFrame::gallery_dialog()
{
    if (!m_gallery_dialog)
        m_gallery_dialog = new GalleryDialog(this);
    return m_gallery_dialog;
}

void MainFrame::update_title()
{
    wxString title = wxEmptyString;
    if (m_plater != nullptr) {
        // m_plater->get_project_filename() produces file name including path, but excluding extension.
        // Don't try to remove the extension, it would remove part of the file name after the last dot!
        wxString project = from_path(into_path(m_plater->get_project_filename()).filename());
//        wxString dirty_marker = (!m_plater->model().objects.empty() && m_plater->is_project_dirty()) ? "*" : "";
        wxString dirty_marker = m_plater->is_project_dirty() ? "*" : "";
        if (!dirty_marker.empty() || !project.empty()) {
            if (!dirty_marker.empty() && project.empty())
                project = _L("Untitled");
            title = dirty_marker + project + " - ";
        }
    }

    std::string build_id = SLIC3R_BUILD_ID;
    if (! wxGetApp().is_editor())
        boost::replace_first(build_id, SLIC3R_APP_NAME, GCODEVIEWER_APP_NAME);
    size_t 		idx_plus = build_id.find('+');
    if (idx_plus != build_id.npos) {
    	// Parse what is behind the '+'. If there is a number, then it is a build number after the label, and full build ID is shown.
    	int commit_after_label;
    	if (! boost::starts_with(build_id.data() + idx_plus + 1, "UNKNOWN") && 
            (build_id.at(idx_plus + 1) == '-' || sscanf(build_id.data() + idx_plus + 1, "%d-", &commit_after_label) == 0)) {
    		// It is a release build.
    		build_id.erase(build_id.begin() + idx_plus, build_id.end());    		
#if defined(_WIN32) && ! defined(_WIN64)
    		// People are using 32bit slicer on a 64bit machine by mistake. Make it explicit.
            build_id += " 32 bit";
#endif
    	}
    }

    title += wxString(build_id);
    if (wxGetApp().is_editor())
        title += (" " + _L("Based on PrusaSlicer"));

    SetTitle(title);
}


void MainFrame::OnOtaTimer(wxTimerEvent& event)
{
    auto ankerNet = AnkerNetInst();
    if (!ankerNet) {
        return;
    }
    ANKER_LOG_INFO << "MainFrame::OnOtaTimer.";
    ankerNet->SetOtaCheckType(OtaCheckType_24Hours);
    ankerNet->queryOTAInformation();
}


void MainFrame::OnHttpConnectError(wxCommandEvent& event)
{
    if (m_plater)
    {
        m_plater->UpdateDeviceList(true);
    }
    wxVariant* pData = (wxVariant*)event.GetClientData();

    wxSize dialogSize = AnkerSize(400, 185);
    wxPoint parentCenterPoint(this->GetPosition().x + this->GetSize().GetWidth() / 2,
        this->GetPosition().y + this->GetSize().GetHeight() / 2);
    wxPoint dialogPos = wxPoint(parentCenterPoint.x - dialogSize.x / 2, parentCenterPoint.y - dialogSize.y / 2);
    wxString title = _L("common_http_connect_error_title");
    wxString content = _L("common_http_connect_error_content");
    //if (pData)
    //{
    //    wxVariantList list = pData->GetList();
    //    auto key = list[0]->GetString().ToStdString();
    //    content += "\nError Code = " + key;
    //}

    AnkerDialog dialog(this, wxID_ANY, title, content, dialogPos, dialogSize);
    auto result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextOkDialog);
}

void MainFrame::BindEvent()
{
#ifdef WIN32
    Bind(wxCUSTOMEVT_EDIT_ENTER, [this](wxCommandEvent& event) {
        this->SetFocus();
        });
#endif // DEBUG
}

void MainFrame::init_tabpanel()
{
    wxGetApp().update_ui_colours_from_appconfig();        
    m_otaTimer = new wxTimer(this, wxID_ANY);
    m_otaTimer->Start(24 * 60 * 60 * 1000); // 24 hours
    Bind(wxEVT_TIMER, &MainFrame::OnOtaTimer, this, m_otaTimer->GetId());
    m_extrusionTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, [this](wxTimerEvent& event)
    {
            CallAfter([this]()
                {
                    ShowAnkerWebView("timer execute for show webview");
                });
    }, m_extrusionTimer->GetId());

    // wxNB_NOPAGETHEME: Disable Windows Vista theme for the Notebook background. The theme performance is terrible on Windows 10
    // with multiple high resolution displays connected.
#ifdef _MSW_DARK_MODE
    if (wxGetApp().tabs_as_menu()) {
        m_tabpanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
        m_printTabPanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
        wxGetApp().UpdateDarkUI(m_tabpanel);
        wxGetApp().UpdateDarkUI(m_printTabPanel);
    }
    else {
        m_tabpanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
        m_printTabPanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
    }
        
#else
    m_tabpanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
    m_printTabPanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
#endif

#ifndef __WXOSX__ // Don't call SetFont under OSX to avoid name cutting in ObjectList
    m_tabpanel->SetFont(Slic3r::GUI::wxGetApp().normal_font());
    m_printTabPanel->SetFont(Slic3r::GUI::wxGetApp().normal_font());
#endif
    m_tabpanel->Hide();
    m_printTabPanel->Hide();
    m_settings_dialog.set_tabpanel(m_tabpanel);

#ifdef __WXMSW__
    m_tabpanel->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
#else
    m_tabpanel->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
#endif
        if (int old_selection = e.GetOldSelection();
            old_selection != wxNOT_FOUND && old_selection < static_cast<int>(m_tabpanel->GetPageCount())) {
            Tab* old_tab = dynamic_cast<Tab*>(m_tabpanel->GetPage(old_selection));
            if (old_tab)
                old_tab->validate_custom_gcodes();
        }

        wxWindow* panel = m_tabpanel->GetCurrentPage();
        Tab* tab = dynamic_cast<Tab*>(panel);
        // There shouldn't be a case, when we try to select a tab, which doesn't support a printer technology
        if (panel == nullptr || (tab != nullptr && !tab->supports_printer_technology(m_plater->printer_technology())))
            return;

#if SHOW_OLD_SETTING_DIALOG
        auto& tabs_list = wxGetApp().tabs_list;
        if (tab && std::find(tabs_list.begin(), tabs_list.end(), tab) != tabs_list.end()) {
            // On GTK, the wxEVT_NOTEBOOK_PAGE_CHANGED event is triggered
            // before the MainFrame is fully set up.
            tab->OnActivate();
            m_last_selected_tab = m_tabpanel->GetSelection();
#ifdef _MSW_DARK_MODE
            if (wxGetApp().tabs_as_menu())
                tab->SetFocus();
#endif
        }
#endif
        m_last_selected_tab = m_tabpanel->GetSelection();
    });

    m_plater = new Plater(this, this);
    m_plater->Bind(wxCUSTOMEVT_ANKER_SLICE_FOR_COMMENT, [this] (wxCommandEvent & event){

        if (!m_showCommentWebView)
            return;
        m_showCommentWebView = false;
        wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
        wxSize mfSize = wxGetApp().mainframe->GetClientSize();
        wxSize dialogSize = m_sliceCommentDialog->GetBestSize();
        wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
        m_sliceCommentDialog->Move(center);
        m_sliceCommentDialog->ShowModal();        
     });
    m_plater->SetBackgroundColour(wxColour("#18191B"));
    m_plater->Hide();

    wxGetApp().plater_ = m_plater;
    

    if (wxGetApp().is_editor())
    {
        ANKER_LOG_INFO << "create preset tabs ";
        create_preset_tabs();
    }
       

    if (m_plater) {
        // load initial config
        //update by alves, cover right parameters data to the config if fff_print then process		
        DynamicPrintConfig full_config = wxGetApp().preset_bundle->full_config();
        if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
        {
            Slic3r::GUI::wxGetApp().plater()->sidebarnew().updatePreset(full_config);
        }

        m_plater->on_config_change(full_config);

        // Show a correct number of filament fields.
        // nozzle_diameter is undefined when SLA printer is selected
        if (full_config.has("nozzle_diameter")) {
            m_plater->on_extruders_change(full_config.option<ConfigOptionFloats>("nozzle_diameter")->values.size());
        }
    }
}

void MainFrame::getwebLoginDataBack(const std::string& from)
{
    if (!AnkerNetInst()) {
        return;
    }

    ANKER_LOG_INFO << from;
    if (!m_loginWebview) {
        ANKER_LOG_INFO << "create background webview";
        m_loginWebview = std::move(CreateWebView(true));
    }
    m_loginWebview->Hide();
}

AnkerWebView* MainFrame::CreateWebView(bool background)
{
    wxSize loginWebViewSize = AnkerSize(900, 700);
    wxPoint loginWebViewPos = wxPoint((GetSize().x - loginWebViewSize.x) / 2, (GetSize().y - loginWebViewSize.y) / 2);
    auto loginWebview = new AnkerWebView(this, wxID_ANY, _L("common_toptable_login"),
        getLoginUrl(), loginWebViewPos, loginWebViewSize, background);

    loginWebview->Bind(wxCUSTOMEVT_DEAL_PRIVACY_CHOICES, [this](wxCommandEvent& ev) {
        DealPrivacyChoices(ev);
        });

    //use old logic for mac 
    //login finish
    loginWebview->Bind(wxCUSTOMEVT_WEB_LOGIN_FINISH, [=](wxEvent& ev) {
#ifdef _WIN32
        wxObject* eventObject = ev.GetEventObject();
        AnkerWebView* dialog = dynamic_cast<AnkerWebView*>(eventObject);
        if (dialog) {
            ANKER_LOG_INFO << "START CALL WEBVIEW Hide 222";
            //dialog->SetForceClose(true);
            dialog->Hide();
            ANKER_LOG_INFO << "START CALL WEBVIEW Close";
            dialog->Close();
            ANKER_LOG_INFO << "END CALL WEBVIEW Close";
        }
        else {
            ANKER_LOG_INFO << "webview dialog is null";
        }
#endif

        auto ankerNet = AnkerNetInst();
        if (!ankerNet) {
            return;
        }

        ANKER_LOG_INFO << "login back start";
        std::string url = ankerNet->GetAvatar();
        wxString filePath = wxString();

        setUserInfoForSentry();

        wxStandardPaths standarPaths = wxStandardPaths::Get();
        filePath = standarPaths.GetUserDataDir();
        filePath = filePath + "/cache/" + wxString::FromUTF8(ankerNet->GetUserId()) + ".png";

        auto appConfig = Slic3r::GUI::wxGetApp().app_config;
        if (nullptr == appConfig) {
            ANKER_LOG_INFO << "02mmPrinter  nohint for user set.";
            return;
        }
        appConfig->set("user_id",ankerNet->GetUserId());

#ifndef __APPLE__
        filePath.Replace("\\", "/");
#endif        
        // AnkerMake Studio Profile/cache
        m_avatarPath = filePath;

        //if avatart not exists
        if (!wxFileExists(m_avatarPath) && url.size() > 0) {
            ankerNet->AsyDownLoad(
                url,
                filePath.ToStdString(wxConvUTF8),
                this,
                onDownLoadFinishedCallBack,
                onDownLoadProgress, true);
        }

        {
            wxLogNull logNo;
            wxFile file(m_avatarPath);
            wxFileOffset size = 0;
            if (file.IsOpened()) {
                size = file.Length();
            }

            if (size <= 0 && url.size() > 0)
            {
                ankerNet->AsyDownLoad(
                    url,
                    filePath.ToStdString(wxConvUTF8),
                    this,
                    onDownLoadFinishedCallBack,
                    onDownLoadProgress, true);
            }

            if (url.size() <= 0)
            {
                m_avatarPath = "nullptr";
            }

            ShowLoginedMenu();
            ANKER_LOG_INFO << "login back finish0";
        }

        updateCurrentEnvironment();
        updateBuryInfo();

        if (m_pDeviceWidget)
            m_pDeviceWidget->loadDeviceList();

        HintDatabase::get_instance().reinit();
        //wxGetApp().filamentMaterialManager()->AsyncUpdate();
        ANKER_LOG_INFO << "login back finish";
        },
        loginWebview->GetId());

    loginWebview->Bind(wxCUSTOMEVT_WEB_LOGOUT_FINISH, [=](wxEvent& ev) {
        ANKER_LOG_INFO << "BEGIN INVOKE wxCUSTOMEVT_WEB_LOGOUT_FINISH";
        ShowUnLoginMenu();
        ShowUnLoginDevice();
        onLogOut();
        ANKER_LOG_INFO << "END INVOKE wxCUSTOMEVT_WEB_LOGOUT_FINISH";
        }, loginWebview->GetId());

    if (background == false)
    {
        loginWebview->SetWebViewSize(AnkerSize(900, 700));
        wxPoint winPoint;
        winPoint.x = this->GetRect().x + (GetRect().GetWidth() - loginWebview->GetRect().GetWidth()) / 2;
        winPoint.y = this->GetRect().y + (GetRect().GetHeight() - loginWebview->GetRect().GetHeight()) / 2;
        loginWebview->Move(winPoint);
    }

    return loginWebview;
}

#ifdef WIN32
void MainFrame::register_win32_callbacks()
{
    //static GUID GUID_DEVINTERFACE_USB_DEVICE  = { 0xA5DCBF10, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED };
    //static GUID GUID_DEVINTERFACE_DISK        = { 0x53f56307, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b };
    //static GUID GUID_DEVINTERFACE_VOLUME      = { 0x71a27cdd, 0x812a, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f };
    static GUID GUID_DEVINTERFACE_HID           = { 0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 };

    // Register USB HID (Human Interface Devices) notifications to trigger the 3DConnexion enumeration.
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = { 0 };
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_HID;
    m_hDeviceNotify = ::RegisterDeviceNotification(this->GetHWND(), &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

// or register for file handle change?
//      DEV_BROADCAST_HANDLE NotificationFilter = { 0 };
//      NotificationFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
//      NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;

    // Using Win32 Shell API to register for media insert / removal events.
    LPITEMIDLIST ppidl;
    if (SHGetSpecialFolderLocation(this->GetHWND(), CSIDL_DESKTOP, &ppidl) == NOERROR) {
        SHChangeNotifyEntry shCNE;
        shCNE.pidl       = ppidl;
        shCNE.fRecursive = TRUE;
        // Returns a positive integer registration identifier (ID).
        // Returns zero if out of memory or in response to invalid parameters.
        m_ulSHChangeNotifyRegister = SHChangeNotifyRegister(this->GetHWND(),        // Hwnd to receive notification
            SHCNE_DISKEVENTS,                                                       // Event types of interest (sources)
            SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED,
            //SHCNE_UPDATEITEM,                                                     // Events of interest - use SHCNE_ALLEVENTS for all events
            WM_USER_MEDIACHANGED,                                                   // Notification message to be sent upon the event
            1,                                                                      // Number of entries in the pfsne array
            &shCNE);                                                                // Array of SHChangeNotifyEntry structures that 
                                                                                    // contain the notifications. This array should 
                                                                                    // always be set to one when calling SHChnageNotifyRegister
                                                                                    // or SHChangeNotifyDeregister will not work properly.
        assert(m_ulSHChangeNotifyRegister != 0);    // Shell notification failed
    } else {
        // Failed to get desktop location
        assert(false); 
    }

    {
        static constexpr int device_count = 1;
        RAWINPUTDEVICE devices[device_count] = { 0 };
        // multi-axis mouse (SpaceNavigator, etc.)
        devices[0].usUsagePage = 0x01;
        devices[0].usUsage = 0x08;
        if (! RegisterRawInputDevices(devices, device_count, sizeof(RAWINPUTDEVICE)))
            BOOST_LOG_TRIVIAL(error) << "RegisterRawInputDevices failed";
    }
}
#endif // _WIN32

bool MainFrame::writeMsgCenterCfg(const std::string& cfgStr)
{
    bool res = true;

    std::ofstream outfile;
    // open file if it not exist and create it    
#ifdef _WIN32
    wchar_t appDataPath[MAX_PATH] = { 0 };
    auto hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
    char* path = new char[MAX_PATH];
    size_t pathLength;
    wcstombs_s(&pathLength, path, MAX_PATH, appDataPath, MAX_PATH);
    std::string filePath = path;
    std::string appName = "\\" + std::string(SLIC3R_APP_KEY " Profile");
    filePath = filePath + appName + "\\msgCenterCfgVersionInfo.json";

    outfile.open(filePath);
#elif __APPLE__
    outfile.open("/tmp/msgCenterCfgVersionInfo.json");
#else
    outfile.open("/tmp/msgCenterCfgVersionInfo.json");
#endif
    // check open status if it success
    if (!outfile.is_open())
    {
        std::cerr << "can't open the file." << std::endl;
    }
    else
    {
        // write something to file
        outfile << cfgStr;
    }
    // close the file
    outfile.close();
    return res;
}

bool MainFrame::loadMsgCenterCfg()
{
    bool res = true;
    std::ifstream ifs;    
#ifdef _WIN32
    wxStandardPaths standarPaths = wxStandardPaths::Get();
    wxString filePath = standarPaths.GetUserDataDir();
    //std::string appName = std::string("\\")+ SLIC3R_APP_KEY + std::string(" Profile");
    std::string appName = std::string("\\")+ SLIC3R_APP_KEY + std::string(" Profile");
    //filePath = filePath + appName+"\\msgCenterCfgVersionInfo.json";
    filePath = filePath +"\\msgCenterCfgVersionInfo.json";

    std::string cfgFilePath = filePath.ToStdString();
    ifs.open(cfgFilePath, std::ios::in);    
#elif __APPLE__
    ifs.open("/tmp/msgCenterCfgVersionInfo.json", std::ios::in);
#else
    ifs.open("/tmp/msgCenterCfgVersionInfo.json", std::ios::in);
#endif

    if (!ifs.is_open())
    {
        ANKER_LOG_WARNING << "read cfg fail." ;
        return false;
    }

    std::string cfgBuf = "";
    while (std::getline(ifs, cfgBuf))
    {        
    }
    ifs.close();

    json_error_t error;
    json_t* root = json_loads(cfgBuf.c_str(), 0, &error);

    if (!root) {
        ANKER_LOG_ERROR << "load loadMsgCenterCfg json fail: " + std::string(error.text);
        return false;
    }

    std::map<std::string, MsgCenterConfig> msgCenterConfigMap;
    if (auto paramsArray = json_object_get(root, "data")) {
        for (int i = 0; i < json_array_size(paramsArray); ++i)
        {
            MsgCenterConfig configItem;
            json_t* child = json_array_get(paramsArray, i);

            if (auto idObj = json_object_get(child, "id"))
                configItem.id = json_integer_value(idObj);

            if (auto errCodeObj = json_object_get(child, "error_code"))
                configItem.error_code = json_string_value(errCodeObj);

            if (auto errLevelObj = json_object_get(child, "alert_level"))
                configItem.error_level = json_string_value(errLevelObj);

            if (auto valueObj = json_object_get(child, "code_source"))
                configItem.code_source = json_integer_value(valueObj);

            if (auto articleArray = json_object_get(child, "article_list"))
            {
                for (int i = 0; i < json_array_size(articleArray); ++i)
                {
                    MsgCenterConfig::ArticleInfo articleInfoItem;
                    json_t* articleChild = json_array_get(articleArray, i);

                    if (auto languageObj = json_object_get(articleChild, "language"))
                        articleInfoItem.language = json_string_value(languageObj);

                    if (auto articleUrlObj = json_object_get(articleChild, "article_url"))
                        articleInfoItem.article_url = json_string_value(articleUrlObj);

                    if (auto articleTitleObj = json_object_get(articleChild, "article_title"))
                        articleInfoItem.article_title = json_string_value(articleTitleObj);

                    configItem.article_info.push_back(articleInfoItem);
                }

            }
            m_MsgCenterCfg->insert(std::make_pair(configItem.error_code, configItem));
        }
    }
    else
        res = false;
    return res;
}

bool MainFrame::writeMsgCenterMultiLanguageCfg(const std::string& cfgStr)
{
    bool res = true;
    std::ofstream outfile;
    // open file if it not exist and create it
#ifdef _WIN32
    wchar_t appDataPath[MAX_PATH] = { 0 };
    auto hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
    char* path = new char[MAX_PATH];
    size_t pathLength;
    wcstombs_s(&pathLength, path, MAX_PATH, appDataPath, MAX_PATH);
    std::string filePath = path;
    std::string appName = "\\" + std::string(SLIC3R_APP_KEY " Profile");
    filePath = filePath + appName + "\\msgCenterMultiLanguageCfg.json";
    outfile.open(filePath);
#elif __APPLE__
    outfile.open("/tmp/msgCenterMultiLanguageCfg.json");
#else
    outfile.open("/tmp/msgCenterMultiLanguageCfg.json");
#endif

    // check open status if it success
    if (!outfile.is_open())
    {
        std::cerr << "can't open the file." << std::endl;
    }
    else
    {
        // write something to file
        outfile << cfgStr;
    }
    // close the file
    outfile.close();
    return res;
}
bool MainFrame::loadMsgCenterMultiLanguageCfg()
{
    bool res = true;
    std::ifstream ifs;
#ifdef _WIN32
    wxStandardPaths standarPaths = wxStandardPaths::Get();
    wxString fileDir = standarPaths.GetUserDataDir();

    std::string appName = std::string("\\") + SLIC3R_APP_KEY + std::string(" Profile");    
    //fileDir = fileDir + appName + "\\msgCenterMultiLanguageCfg.json";
    fileDir = fileDir + "\\msgCenterMultiLanguageCfg.json";

    std::string cfgFilePath = fileDir.ToStdString();
    ifs.open(cfgFilePath, std::ios::in);    
#elif __APPLE__
    ifs.open("/tmp/msgCenterMultiLanguageCfg.json");
#else
    ifs.open("/tmp/msgCenterMultiLanguageCfg.json");
#endif
    if (!ifs.is_open())
    {
        ANKER_LOG_WARNING << "read MultiLanguageCfg fail.";
        return false;
    }

    std::string cfgBuf = "";
    while (std::getline(ifs, cfgBuf))
    {
    }
    ifs.close();

    json_error_t error;
    json_t* root = json_loads(cfgBuf.c_str(), 0, &error);

    if (!root) {
        ANKER_LOG_ERROR << "load MultiLanguageCfg json fail: " + std::string(error.text);
        return false;
    }

    auto jsonDataObj = json_object_get(root, "data");
    if (!jsonDataObj)
    {
        ANKER_LOG_ERROR << "request GetMsgCenterCfgVersionInfo no data ";
        return false;
    }

    if (auto paramsArray = json_object_get(jsonDataObj, "text_2_data"))
    {
        for (int i = 0; i < json_array_size(paramsArray); ++i)
        {
            json_t* child = json_array_get(paramsArray, i);
            MsgErrCodeInfo Item;
            if (auto languageObj = json_object_get(child, "language"))
                Item.language = json_string_value(languageObj);

            if (auto versionObj = json_object_get(child, "version"))
                Item.version = json_string_value(versionObj);

            if (auto releaseVersionObj = json_object_get(child, "release_version"))
                Item.release_version = json_string_value(releaseVersionObj);

            if (auto text2DataObj = json_object_get(child, "text_2"))
            {
                const char* key;
                json_t* value;
                json_object_foreach(text2DataObj, key, value)
                {
                    if (json_is_string(value))
                    {
                        Item.errorCodeUrlMap[key] = json_string_value(value);
                    }
                    else
                    {
                        ANKER_LOG_ERROR << "Error: value for key " << key << " is not a string";
                    }
                }
            }
            m_MsgCenterErrCodeInfo->push_back(Item);
        }
    }
    else
        return false;

    return res;
}


void MainFrame::ShowErrDialogByCenter()
{
    wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
    wxSize mfSize = wxGetApp().mainframe->GetClientSize();
    wxSize dialogSize = m_pMsgCentrePopWindow->GetBestSize();
    wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
    m_pMsgCentrePopWindow->Move(center);
    m_pMsgCentrePopWindow->Raise();
    m_pMsgCentrePopWindow->Show();
}
void MainFrame::showErrMsgDialog(const std::string& errorCode, const std::string& errorLevel, const std::string& sn, const int& cmdType)
{
    ANKER_LOG_INFO <<"msg code:"<< errorCode<<"msg level:"<< errorLevel;
    if (cmdType == HIDE_ERR_DIALOG_CMD)
    {        
        if (sn != m_pMsgCentrePopWindow->getDialogSn()|| errorCode!= m_pMsgCentrePopWindow->getDialogErrCode())
            return;
        m_pMsgCentrePopWindow->clearData();
        m_pMsgCentrePopWindow->Hide();
        return;
    }

    if (!m_MsgCenterCfg)
    {
        m_MsgCenterCfg = new std::map<std::string, MsgCenterConfig>();
        if (!loadMsgCenterCfg())
        {
            //load local cfg msgCenterCfgVersionInfo
            ANKER_LOG_ERROR << "no any msg center config fail ";
            return;
        }                          
    }

    //std::string currentLanguage = GetTranslateLanguage();
    std::string currentLanguage = "";
    int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
    if (type == wxLanguage::wxLANGUAGE_JAPANESE)
    {
        currentLanguage = "ja";
    }
    else
    {
        currentLanguage = "en";//wxLanguage::wxLANGUAGE_ENGLISH
    }
    auto desCfg = m_MsgCenterCfg->find(errorCode);
    std::string realErrorCode = "fdm_news_center_" + errorCode +"_desc";
    std::string content = "";
    if (desCfg == m_MsgCenterCfg->end())
    {
        ANKER_LOG_ERROR << "no any msg center config for this errorCode: " << errorCode;
        return;
    }
    std::string dialogContent = "";
    if (!m_MsgCenterErrCodeInfo)
    {
        m_MsgCenterErrCodeInfo = new std::vector<MsgErrCodeInfo>();
        if (!loadMsgCenterMultiLanguageCfg())
        {
            //load local cfg msgCenterMultiLanguageCfg
            ANKER_LOG_ERROR << "no any msg center MultiLanguageCfg fail ";
            return;
        }
    }
    
    for (auto it = m_MsgCenterErrCodeInfo->begin(); it != m_MsgCenterErrCodeInfo->end(); ++it) {
        if ((*it).language == currentLanguage)
        {
            auto ErrCodeUrlMap = (*it).errorCodeUrlMap;
            auto resItem = ErrCodeUrlMap.find(realErrorCode);
            if (resItem != ErrCodeUrlMap.end())
                dialogContent = resItem->second;
        }
    }    

    auto articleList = desCfg->second.article_info;
    std::string cfgArticleUrl = "";
    std::string cfgArticleTitle = "";
    std::string cfgErrorLevel = desCfg->second.error_level;
    std::string cfgErrorCode = desCfg->second.error_code;

    //fdm_news_center_0xFE01030001_desc
    for (auto item : articleList)
    {
        if (item.language == currentLanguage)
        {
            cfgArticleUrl = item.article_url;
            //cfgArticleTitle = item.article_title;//server return ""
            break;
        }
    }   

    if (cmdType == SHOW_ERR_DIALOG_CMD)
    {   
        auto ankerNet = AnkerNetInst();
        if (!ankerNet) {
            return;
        }
        DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(sn);
        if (!devceiObj)
            return;

        cfgArticleTitle = devceiObj->GetStationName();
        wxString titleContent = wxString::FromUTF8(cfgArticleTitle.c_str());        
        wxString utfDialogContent = wxString::FromUTF8(dialogContent.c_str());
        if(cfgErrorLevel == LEVEL_S || cfgErrorLevel == LEVEL_P0)
           utfDialogContent = "[" + cfgErrorCode + "] " +  wxString::FromUTF8(dialogContent.c_str());
        m_pMsgCentrePopWindow->setValue(utfDialogContent, cfgArticleUrl,cfgErrorCode,cfgErrorLevel, sn, titleContent);
        if (m_pMsgCentrePopWindow->IsActive())
        {
            ankerNet->GetMsgCenterStatus();
        }
                                               
        if (cfgErrorLevel == LEVEL_S)
            ShowErrDialogByCenter();
        else
        {            
            if (m_currentTabMode == TAB_DEVICE)
                ShowErrDialogByCenter();
            else
                m_hasErrDialog = true;            
        }
    }
}

void MainFrame::InitAnkerDevice()
{    
    m_pDeviceWidget = new AnkerDevice(m_printTabPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    if (AnkerNetInst()) {
        DatamangerUi::GetInstance().SetMainWindow(this);
        m_pDeviceWidget->Init();
    }
    else {
        ShowUnLoginDevice();
    }
    Bind(wxCUSTOMEVT_DEVICE_LIST_UPDATE, [this](wxCommandEvent& event) {
        m_pDeviceWidget->loadDeviceList(true);
        });

    Bind(wxCUSTOMEVT_UPDATE_MACHINE, [this](wxCommandEvent& event) {
        wxVariant* pData = (wxVariant*)(event.GetClientData());
        if (pData)
        {
            wxVariantList list = pData->GetList();
            std::string snID = list[0]->GetString().ToStdString();
            std::string snIDEx = list[0]->GetString().ToStdString();
            int type = AKNMT_CMD_EVENT_NONE;
            if (list.size() >= 2) {
                list[1]->GetString().ToInt(&type);
            }
            m_pDeviceWidget->updateAboutMqttStatus(snID, (AnkerNet::aknmt_command_type_e)type);
            if (type == AnkerNet::AKNMT_CMD_Z_AXIS_RECOUP) {
                m_pDeviceWidget->updateAboutZoffsetStatus(snID);
            }

            auto ankerNet = AnkerNetInst();
            if (!ankerNet) {
                return;
            }
            DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(snID);
            if (!devceiObj)
                return;

            if (type == SHOW_ERR_DIALOG_CMD || type == HIDE_ERR_DIALOG_CMD)
            {
                //show error msg dialog
                std::string errorCode = "";
                std::string errorLevel = "";
                devceiObj->GetMsgCenterInfo(errorCode, errorLevel);    
                showErrMsgDialog(errorCode, errorLevel, snID, type);                
            }

        }
        });

    Bind(wxCUSTOMEVT_SHOW_MSG_DIALOG, [this](wxCommandEvent& event) {

        wxVariant* pData = (wxVariant*)event.GetClientData();
        if (pData)
        {
            wxVariantList list = pData->GetList();
            int i = 0;
            int haveCancel = list[i++]->GetInteger();
            int level = list[i++]->GetInteger();
            int clear = list[i++]->GetInteger();
            int type = list[i++]->GetInteger();
            auto sn = list[i++]->GetString().ToStdString();
            auto msgBoxTitle = list[i++]->GetString().ToStdString(wxConvUTF8);
            auto msgBoxContent = list[i++]->GetString().ToStdString(wxConvUTF8);
            auto btn1Text = list[i++]->GetString().ToStdString(wxConvUTF8);
            auto btn2Text = list[i++]->GetString().ToStdString(wxConvUTF8);
            auto imagePath = list[i++]->GetString().ToStdString(wxConvUTF8);
            auto filamentName = list[i++]->GetString().ToStdString(wxConvUTF8);
            
            NetworkMsg msg;
            msg.sn = sn;
            msg.title = msgBoxTitle;
            msg.context = msgBoxContent;
            msg.clear = clear == 0 ? false : true;
            msg.type = (GeneralException2Gui)type;
            msg.haveCancel = haveCancel == 1 ? true : false;
            msg.level = (NetworkMsgLevel)level;
            msg.btn1Text = btn1Text;
            msg.btn2Text = btn2Text;
            msg.imagePath = imagePath;
            msg.filamentName = filamentName;

            m_pDeviceWidget->showMsgLevelDialog(msg);
        }
        });

    Bind(wxCUSTOMEVT_TRANSFER_PROGRESS, [this](wxCommandEvent& event) {
        wxVariant* pData = (wxVariant*)event.GetClientData();
        if (pData) {
            wxVariantList list = pData->GetList();
            if (list.size() < 3) {
                return;
            }
            std::string snID = list[0]->GetString().ToStdString();
            int progress = list[1]->GetInteger();
            FileTransferResult result = (FileTransferResult)list[2]->GetInteger();
            m_pDeviceWidget->updateFileTransferStatus(snID, progress, result);
        }
        });
    Bind(wxCUSTOMEVT_GET_MSG_CENTER_CFG, [this](wxCommandEvent& event) {        

        std::map<std::string, MsgCenterConfig>* pData = (std::map<std::string, MsgCenterConfig>*)(event.GetClientData());
        if (pData)
        {
            int counts = 0;                
            counts = pData->size();
            if (m_MsgCenterCfg)
            {
                delete m_MsgCenterCfg;
                m_MsgCenterCfg = nullptr;
            }
            ANKER_LOG_INFO << "msg center config counts is: "<< counts;
            m_MsgCenterCfg = pData;         

            auto item = pData->begin();
            while (item != pData->end())
            {
                if (item->first == "originMsg")
                {
                    writeMsgCenterCfg(item->second.originMsg);
                    break;
                }
                ++item;
            }
        }
        });
    Bind(wxCUSTOMEVT_GET_MSG_CENTER_RECORDS, [this](wxCommandEvent& event) {

        std::vector<MsgCenterItem>* pData = (std::vector<MsgCenterItem>*)(event.GetClientData());
        if (pData)
        {
            int counts = 0;
            counts = pData->size();
            
            ANKER_LOG_INFO << "msg center config counts is: " << counts;
            
            //update content
            if (pData->size() <= 0)
                m_MsgCentreDialog->updateCurrentPage();

            updateMsgCenterItemContent(pData);
            m_MsgCentreDialog->updateMsg(pData);
        }
        });

    Bind(wxCUSTOMEVT_GET_MSG_CENTER_ERR_CODE_INFO, [this](wxCommandEvent& event) {

        std::vector<MsgErrCodeInfo>* pData = (std::vector<MsgErrCodeInfo>*)(event.GetClientData());
        if (pData)
        {
            int counts = 0;
            counts = pData->size();
            if (m_MsgCenterErrCodeInfo)
            {
                delete m_MsgCenterErrCodeInfo;
                m_MsgCenterErrCodeInfo = nullptr;
            }
            ANKER_LOG_INFO << "msg center config counts is: " << counts;
            m_MsgCenterErrCodeInfo = pData;


            auto item = pData->begin();
            while (item != pData->end())
            {
                if (!item->originMsg.empty())
                {
                    
                    writeMsgCenterMultiLanguageCfg(item->originMsg);
                    break;
                }
                ++item;
            }
        }
        });

    Bind(wxCUSTOMEVT_GET_MSG_CENTER_STATUS, [this](wxCommandEvent& event) {
        wxVariant* pData = (wxVariant*)(event.GetClientData());
        if (pData)
        {
            wxVariantList list = pData->GetList();
            int officicalNews = list[0]->GetInteger();
            int printerNews = list[1]->GetInteger();
            
            m_pFunctionPanel->setMsgItemRedPointStatus(officicalNews, printerNews);
        }
        });

    if (m_pDeviceWidget) {
        m_pDeviceWidget->Bind(wxCUSTOMEVT_LOGIN_CLCIKED, [this](wxCommandEvent& event) {
            ShowAnkerWebView("device login button clicked");
        });
    }

    if (m_pDeviceWidget) {
        m_printTabPanel->AddPage(m_pDeviceWidget, _L("Print"));
    }

    Bind(wxCUSTOMEVT_SWITCH_TO_PRINT_PAGE, [this](wxCommandEvent& event) {
        wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
        if (pData) {
            int pageCount = m_printTabPanel->GetPageCount();
            m_printTabPanel->ChangeSelection(pageCount - 1);
            std::string sn = pData->GetData().ToStdString();
            m_pDeviceWidget->switchDevicePage(sn);
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ON_TAB_CHANGE);
            evt.SetId(type_devcie);
            wxPostEvent(m_pFunctionPanel, evt);
            //change  Tab  to device 
            if (wxGetApp().mainframe != nullptr)
            {
                wxGetApp().mainframe->setTabMode(TAB_DEVICE);
            }
        }
        });
}

void MainFrame::create_preset_tabs()
{
    add_created_tab(new TabPrint(m_tabpanel), "cog");
    add_created_tab(new TabFilament(m_tabpanel), "spool");
    add_created_tab(new TabSLAPrint(m_tabpanel), "cog");
    add_created_tab(new TabSLAMaterial(m_tabpanel), "resin");    add_created_tab(new TabPrinter(m_tabpanel), 
        wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF ? "printer" : "sla_printer");
        
    InitAnkerDevice();

    Bind(wxCUSTOMEVT_GET_COMMENT_FLAGS, [this](wxCommandEvent& event) {
        wxVariant* pData = (wxVariant*)event.GetClientData();
        if (pData) {
            wxVariantList list = pData->GetList();

            m_showCommentWebView = list[0]->GetBool();
            if (m_showCommentWebView)
            {
                g_sliceCommentData.reviewNameID = list[1]->GetString().ToStdString();
                g_sliceCommentData.reviewName = list[2]->GetString().ToStdString(wxConvUTF8);
                g_sliceCommentData.appVersion = list[3]->GetString().ToStdString();
                g_sliceCommentData.country = list[4]->GetString().ToStdString();
                g_sliceCommentData.sliceCount = list[5]->GetString().ToStdString();
            
                int sliceTimes = std::stoi(g_sliceCommentData.sliceCount);
                if(m_plater)
                    m_plater->setStarCommentFlagsTimes(sliceTimes);
            }
            ANKER_LOG_INFO << "get conment flags success";
        }
    });

    Bind(wxCUSTOMEVT_ACCOUNT_LOGOUT, [this](wxCommandEvent& event) {
        LogOut();
    });

    Bind(wxCUSTOMEVT_ACCOUNT_EXTRUSION, [this](wxCommandEvent& event) {
        
        static bool accountShowed = false;
        if (accountShowed) {
            return;
        }

        CloseVideoStream(VIDEO_CLOSE_BY_LOGOUT);

        if(m_plater)
        {
            m_plater->UpdateDeviceList(true);
        }

        accountShowed = true;
        wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
        wxSize mfSize = wxGetApp().mainframe->GetClientSize();
        wxSize dialogSize = AnkerSize(400, 250);
        wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
        AnkerDialog dialog(this, wxID_ANY, _AnkerL("common_popup_titlenotice"),
            _AnkerL("common_popup_content_accountsqueezed"),
            center, dialogSize);
        dialog.CenterOnParent();
        auto res = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextOkDialog);
        accountShowed = false;
        ShowUnLoginMenu();
        onLogOut();
        if(wxID_CLOSE != res)
        {
            m_extrusionTimer->Start(100, wxTIMER_ONE_SHOT);
        } 
        });

    Bind(wxCUSTOMEVT_HTTP_CONNECT_ERROR,&MainFrame::OnHttpConnectError,this);

    Bind(wxCUSTOMEVT_OTA_UPDATE, [this](wxCommandEvent& event) {
        auto ankerNet = AnkerNetInst();
        if (!ankerNet) {
            return;
        }

        OtaInfo* info = (OtaInfo*)(event.GetClientData());

        if (info)
        {
            wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
            wxSize mfSize = wxGetApp().mainframe->GetClientSize();

            if (info->noUpdate) {
                if (ankerNet->GetOtaCheckType() == OtaCheckType_Manual) {
                    wxSize dialogSize = AnkerSize(400, 180);
                    wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2,
                        mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);

                    AnkerDialog dialog(nullptr, wxID_ANY, _L("common_menu_settings_ota"),
                        _L("common_popup_ota_noticenew"), center, dialogSize);

                    int result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextOkDialog);
                    ANKER_LOG_INFO << "result: " << result;
                }
                return;
            }


            {
                int otaId = m_menubar->FindMenuItem(_L("common_menu_title_settings"), _L("common_menu_settings_ota"));
                wxMenu* tmpSettingsMenu = nullptr;
                wxMenuItem* otaItem = m_menubar->FindItem(otaId, &tmpSettingsMenu);
                if (otaItem) {            
                    wxImage image(AnkerBase::AnkerResourceIconPath + "ota_reddot.png", wxBITMAP_TYPE_PNG);
                    if (image.IsOk()) {
                        wxBitmap bitmap(image);
                        otaItem->SetBitmap(bitmap);
                    }
                }

                wxSize dialogSize = AnkerSize(500, 300);
                wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2,
                    mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);

                AnkerOtaNotesDialog dialog(nullptr, wxID_ANY, _L("common_menu_settings_ota"),
                    wxString::FromUTF8(info->version_name.c_str()), wxString::FromUTF8(info->release_note.c_str()), center, dialogSize);
                int result = 0;
                if (info->is_forced) {
                    result = dialog.ShowAnkerModal(OtaType_Forced);
                }
                else {
                    result = dialog.ShowAnkerModal(OtaType_Normal);
                }
                ANKER_LOG_INFO << "ota click result: " << result << " is_forced: " << info->is_forced;
                if (wxID_OK == result) {
                    wxString url = wxString::FromUTF8(info->download_path.c_str());
                    wxURI uri(url);
                    url = uri.BuildURI();
                    bool success = wxLaunchDefaultBrowser(url, wxBROWSER_NEW_WINDOW);
                    if (!success) {
                        ANKER_LOG_WARNING << "launch browser failed, url: " << url.c_str();
                    }
                }
            }
        }

        });

}

void MainFrame::add_created_tab(Tab* panel,  const std::string& bmp_name /*= ""*/)
{
    panel->create_preset_tab();

    const auto printer_tech = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology();

    if (panel->supports_printer_technology(printer_tech))
//#ifdef _MSW_DARK_MODE
//        if (!wxGetApp().tabs_as_menu())
//            dynamic_cast<Notebook*>(m_tabpanel)->AddPage(panel, panel->title(), bmp_name);
//        else
//#endif
        m_tabpanel->AddPage(panel, panel->title());
}

bool MainFrame::is_active_and_shown_tab(Tab* tab)
{
    int page_id = m_tabpanel->FindPage(tab);

    if (m_tabpanel->GetSelection() != page_id)
        return false;

    if (m_layout == ESettingsLayout::Dlg)
        return m_settings_dialog.IsShown();

    if (m_layout == ESettingsLayout::New)
        return m_main_sizer->IsShown(m_tabpanel);
    
    return true;
}

bool MainFrame::isActiveAndShownAnkerTab(AnkerTab* tab)
{
    if (!m_ankerCfgDlg)
        return false;

    int page_id = m_ankerCfgDlg->m_rightPanel->FindPage(tab);

    if (m_ankerCfgDlg->m_rightPanel->GetSelection() != page_id)
        return false;

    if (m_layout == ESettingsLayout::Dlg) {
        // add by allen for ankerCfgDlg
        // return m_settings_dialog.IsShown();
          return m_ankerCfgDlg->IsShown();
    }
        
    if (m_layout == ESettingsLayout::New)
        return m_main_sizer->IsShown(m_tabpanel);

    return true;
}

bool MainFrame::can_start_new_project() const
{
    return m_plater && (!m_plater->get_project_filename(".3mf").IsEmpty() || 
                        GetTitle().StartsWith('*')||
                        wxGetApp().has_current_preset_changes() || 
                        !m_plater->model().objects.empty() );
}

bool MainFrame::can_save() const
{
    return (m_plater != nullptr) &&
        !m_plater->canvas3D()->get_gizmos_manager().is_in_editing_mode(false) &&
        m_plater->is_project_dirty();
}

bool MainFrame::can_save_as() const
{
    return (m_plater != nullptr) &&
        !m_plater->canvas3D()->get_gizmos_manager().is_in_editing_mode(false);
}

void MainFrame::save_project()
{
    save_project_as(m_plater->get_project_filename(".3mf"));
}

bool MainFrame::save_project_as(const wxString& filename)
{
    bool ret = (m_plater != nullptr) ? m_plater->export_3mf(into_path(filename)) : false;
    if (ret) {
        // Make a copy of the active presets for detecting changes in preset values.
        wxGetApp().update_saved_preset_from_current_preset();
        // Save the names of active presets and project specific config into ProjectDirtyStateManager.
        // Reset ProjectDirtyStateManager's state as saved, mark active UndoRedo step as saved with project.
        m_plater->reset_project_dirty_after_save();
    }
    return ret;
}

bool MainFrame::can_export_model() const
{
    return (m_plater != nullptr) && !m_plater->model().objects.empty();
}

bool MainFrame::can_export_toolpaths() const
{
    return (m_plater != nullptr) && (m_plater->printer_technology() == ptFFF) && m_plater->is_preview_shown() && m_plater->is_preview_loaded() && m_plater->has_toolpaths_to_export();
}

bool MainFrame::can_export_supports() const
{
    if ((m_plater == nullptr) || (m_plater->printer_technology() != ptSLA) || m_plater->model().objects.empty())
        return false;

    bool can_export = false;
    const PrintObjects& objects = m_plater->sla_print().objects();
    for (const SLAPrintObject* object : objects)
    {
        if (!object->support_mesh().empty() || !object->pad_mesh().empty())
        {
            can_export = true;
            break;
        }
    }
    return can_export;
}

bool MainFrame::can_export_gcode() const
{
    if (m_plater == nullptr)
        return false;

    if (m_plater->model().objects.empty())
        return false;

    if (m_plater->is_export_gcode_scheduled())
        return false;

    if (!m_plater->is_gcode_valid())
        return false;

    // TODO:: add other filters

    return true;
}

bool MainFrame::can_send_gcode() const
{
    if (m_plater && ! m_plater->model().objects.empty())
        if (const DynamicPrintConfig *cfg = wxGetApp().preset_bundle->physical_printers.get_selected_printer_config(); cfg)
            if (const auto *print_host_opt = cfg->option<ConfigOptionString>("print_host"); print_host_opt)
                return ! print_host_opt->value.empty();
    return false;
}

bool MainFrame::can_export_gcode_sd() const
{
	if (m_plater == nullptr)
		return false;

	if (m_plater->model().objects.empty())
		return false;

	if (m_plater->is_export_gcode_scheduled())
		return false;

    if (!m_plater->is_gcode_valid())
        return false;

	// TODO:: add other filters

	return wxGetApp().removable_drive_manager()->status().has_removable_drives;
}

bool MainFrame::can_eject() const
{
	return wxGetApp().removable_drive_manager()->status().has_eject;
}

bool MainFrame::can_slice() const
{
    bool bg_proc = wxGetApp().app_config->get_bool("background_processing");
    return (m_plater != nullptr) ? !m_plater->model().objects.empty() && !bg_proc : false;
}

bool MainFrame::can_change_view() const
{
    switch (m_layout)
    {
    default:                   { return false; }
    case ESettingsLayout::New: { return m_plater->IsShown(); }
    case ESettingsLayout::Dlg: { return true; }
    case ESettingsLayout::Old: { 
        int page_id = m_tabpanel->GetSelection();
        return page_id != wxNOT_FOUND && dynamic_cast<const Slic3r::GUI::Plater*>(m_tabpanel->GetPage((size_t)page_id)) != nullptr;
    }
    case ESettingsLayout::GCodeViewer: { return true; }
    }
}

bool MainFrame::can_select() const
{
    return (m_plater != nullptr) && !m_plater->model().objects.empty();
}

bool MainFrame::can_deselect() const
{
    return (m_plater != nullptr) && !m_plater->is_selection_empty();
}

bool MainFrame::can_delete() const
{
    return (m_plater != nullptr) && !m_plater->is_selection_empty();
}

bool MainFrame::can_delete_all() const
{
    return (m_plater != nullptr) && !m_plater->model().objects.empty();
}

bool MainFrame::can_reslice() const
{
    return (m_plater != nullptr) && !m_plater->model().objects.empty();
}

void MainFrame::on_dpi_changed(const wxRect& suggested_rect)
{
    wxGetApp().update_fonts(this);
    this->SetFont(this->normal_font());

#ifdef _MSW_DARK_MODE
    //// update common mode sizer
    //if (!wxGetApp().tabs_as_menu())
    //    dynamic_cast<Notebook*>(m_tabpanel)->Rescale();
#endif

    // update Plater
    wxGetApp().plater()->msw_rescale();

   //// update AnkerConfigDialog
   // if (m_ankerCfgDlg)
   //     m_ankerCfgDlg->msw_rescale();

    // update Tabs
#if SHOW_OLD_SETTING_DIALOG
    if (m_layout != ESettingsLayout::Dlg) // Do not update tabs if the Settings are in the separated dialog
        for (auto tab : wxGetApp().tabs_list)
            tab->msw_rescale();
#endif

    // Workarounds for correct Window rendering after rescale

    /* Even if Window is maximized during moving,
     * first of all we should imitate Window resizing. So:
     * 1. cancel maximization, if it was set
     * 2. imitate resizing
     * 3. set maximization, if it was set
     */
    const bool is_maximized = this->IsMaximized();
    if (is_maximized)
        this->Maximize(false);

    /* To correct window rendering (especially redraw of a status bar)
     * we should imitate window resizing.
     */
    const wxSize& sz = this->GetSize();
    this->SetSize(sz.x + 1, sz.y + 1);
    this->SetSize(sz);

    this->Maximize(is_maximized);
}

void MainFrame::handleErrMsgDialogRes(wxVariant* pData)
{
    if (!pData)
    {
        ANKER_LOG_WARNING << "handleErrMsgDialogRes no data to handle ";
        return;
    }

    std::string snID = "";
    std::string errorCode = "";
    std::string errorLevel = "";

    wxVariantList list = pData->GetList();
    snID = list[0]->GetString().ToStdString();
    errorCode = list[1]->GetString().ToStdString();
    errorLevel = list[2]->GetString().ToStdString();
    

    auto ankerNet = AnkerNetInst();
    if (!ankerNet) {
        return;
    }
    DeviceObjectBasePtr devceiObj = ankerNet->getDeviceObjectFromSn(snID);
    if (!devceiObj)
        return;
    devceiObj->SendErrWinResToMachine(errorCode, errorLevel);
}
void MainFrame::initAnkerUi()
{
    wxSize msgWebViewSize = AnkerSize(900, 700);
    wxPoint msgWebViewPos = wxPoint((GetSize().x - msgWebViewSize.x) / 2, (GetSize().y - msgWebViewSize.y) / 2);

    if (!m_MsgCentreDialog)
        m_MsgCentreDialog = new AnkerMsgCentreDialog(this);
    m_MsgCentreDialog->SetSize(AnkerSize(720, 700));  
    m_MsgCentreDialog->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) { 
        m_isMsgCenterIsShow = false;            
        m_pFunctionPanel->setMsgEntrenceRedPointStatus(false);
        evt.Skip(); });
    m_pMsgCentrePopWindow = new AnkerCustomDialog(this);
    m_pMsgCentrePopWindow->SetMaxSize(AnkerSize(400, 240));
    m_pMsgCentrePopWindow->SetMinSize(AnkerSize(400, 240));
    m_pMsgCentrePopWindow->SetSize(AnkerSize(400, 240));
    m_pMsgCentrePopWindow->Bind(wxCUSTOMEVT_ANKER_CUSTOM_CLOSE, [this](wxCommandEvent& event) {

        wxVariant* pData = (wxVariant*)(event.GetClientData());
        handleErrMsgDialogRes(pData);
        m_hasErrDialog = false;
        m_pMsgCentrePopWindow->Hide();
        });
    m_pMsgCentrePopWindow->Bind(wxCUSTOMEVT_ANKER_CUSTOM_OK, [this](wxCommandEvent& event) {
       
        wxVariant* pData = (wxVariant*)(event.GetClientData());        
        handleErrMsgDialogRes(pData);
        m_hasErrDialog = false;
        m_pMsgCentrePopWindow->Hide();
        });
    m_pMsgCentrePopWindow->Bind(wxCUSTOMEVT_ANKER_CUSTOM_CANCEL, [this](wxCommandEvent& event) {

        wxVariant* pData = (wxVariant*)(event.GetClientData());
        handleErrMsgDialogRes(pData);
        m_hasErrDialog = false;
        m_pMsgCentrePopWindow->Hide();
        });
    m_pMsgCentrePopWindow->Bind(wxCUSTOMEVT_ANKER_CUSTOM_OTHER, [this](wxCommandEvent& event) {

        wxVariant* pData = (wxVariant*)(event.GetClientData());
        handleErrMsgDialogRes(pData);
        m_hasErrDialog = false;
        m_pMsgCentrePopWindow->Hide();
        });
    m_pMsgCentrePopWindow->Hide();
}

void MainFrame::on_sys_color_changed()
{
    wxBusyCursor wait;

    // update label colors in respect to the system mode
    wxGetApp().init_ui_colours();
    // but if there are some ui colors in appconfig, they have to be applied
    wxGetApp().update_ui_colours_from_appconfig();
#ifdef __WXMSW__
    wxGetApp().UpdateDarkUI(m_tabpanel);
 //   m_statusbar->update_dark_ui();
 // 
 // 
 // //TODO: need  do this ?
//#ifdef _MSW_DARK_MODE
//    // update common mode sizer
//    if (!wxGetApp().tabs_as_menu())
//        dynamic_cast<Notebook*>(m_tabpanel)->OnColorsChanged();
//#endif
#endif

    // update Plater
    wxGetApp().plater()->sys_color_changed();

    // update Tabs
#if SHOW_OLD_SETTING_DIALOG
    for (auto tab : wxGetApp().tabs_list)
        tab->sys_color_changed();
#endif
    // add by allen for ankerCfgDlg
    for (auto tab : wxGetApp().ankerTabsList)
        tab->sys_color_changed();

    MenuFactory::sys_color_changed(m_menubar);

    this->Refresh();
}

void MainFrame::update_mode_markers()
{
#ifdef __WXMSW__
#ifdef _MSW_DARK_MODE
    // update markers in common mode sizer
    if (!wxGetApp().tabs_as_menu())
        dynamic_cast<Notebook*>(m_tabpanel)->UpdateModeMarkers();
#endif
#endif

    // update mode markers on side_bar
   // wxGetApp().sidebar().update_mode_markers();

    // update mode markers in tabs
#if SHOW_OLD_SETTING_DIALOG
    for (auto tab : wxGetApp().tabs_list)
        tab->update_mode_markers();
#endif

    // add by allen for ankerCfgDlg
    for (auto tab : wxGetApp().ankerTabsList)
        tab->update_mode_markers();
}

#ifdef _MSC_VER
    // \xA0 is a non-breaking space. It is entered here to spoil the automatic accelerators,
    // as the simple numeric accelerators spoil all numeric data entry.
//static const wxString sep = "\t\xA0";
static const wxString sep = "\t";
static const wxString sep_space = "\xA0";
#else
static const wxString sep = " - ";
static const wxString sep_space = "";
#endif

void DataSharedReport(bool isShareBuryPoint)
{
    std::map<std::string, std::string> buryMap;
    if (isShareBuryPoint) {
        buryMap.insert(std::make_pair(c_current_bury_shared, "1"));
    }
    else {
        buryMap.insert(std::make_pair(c_current_bury_shared, "0"));
    }
    ANKER_LOG_INFO << "Report bury event is " << e_report_bury_shared;
    reportBuryEvent(e_report_bury_shared, buryMap);
}

void SetBuryPointSwitch()
{
    bool burypointSwitch = wxGetApp().app_config->get_bool("burypoint_switch");
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj) {
        if (!burypointSwitch) {
            obj->PostSetBuryPointSwitch(true);
        } else {
            obj->PostSetBuryPointSwitch(false);
        }
    }
}


void QueryDataShared(AnkerToggleBtn* dataSharedButton)
{
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj) {
        std::vector<int> paramTypeList = { static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH) };
        auto result = obj->PostQueryDataShared(paramTypeList);
 
        if (result.empty()) {
           
            bool burypointSwitch = wxGetApp().app_config->get_bool("burypoint_switch");
            ANKER_LOG_INFO << "The query result is empty, "<< "burypoint switch is "<< burypointSwitch;
            if (!burypointSwitch) {
                DataSharedReport(false);
                if (dataSharedButton != nullptr) {
                    dataSharedButton->SetState(false);
                }
            } else {
                DataSharedReport(true);
                if (dataSharedButton != nullptr) {
                    dataSharedButton->SetState(true);
                }
            }

            return;
        }

        for (int i = 0; i < result.size(); ++i) {
            auto [paramType, paramValue] = result[i];
            ANKER_LOG_INFO << "Query burypoint switch is " << paramValue;
            if (paramValue == "0") {  
                wxGetApp().app_config->set("burypoint_switch", "0");
                DataSharedReport(false);
            }else {
                wxGetApp().app_config->set("burypoint_switch", "1");
                DataSharedReport(true);
            }
            SetBuryPointSwitch();
            if (dataSharedButton == nullptr) {
                return;
            }
            if (paramType == static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH)) {
                if (paramValue == "0") {
                    dataSharedButton->SetState(false);
                    return;
                }
                else {
                    dataSharedButton->SetState(true);
                    return;
                }
            }
        }
    }
    if (dataSharedButton != nullptr) {
        dataSharedButton->SetState(true);
    }
}

static std::tuple<int, std::string> PreHandleUpdateDataShared(AnkerToggleBtn* shareBuryPointButton)
{
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj) {
        std::vector<std::tuple<std::string, int>> memberVec = obj->PostGetMemberType();
        for (int i = 0; i < memberVec.size(); ++i) {
            auto [stationSn, memberType] = memberVec[i];
            // memberType: 1 is sharer, 2 is owner
            if (memberType == 1) {
                continue;
            }

            std::vector<std::pair<int, std::string>> paramTypeList;
            if (shareBuryPointButton == nullptr) { 
                return std::make_tuple(-1, "-1"); 
            }
            bool btnState = shareBuryPointButton->GetState();
            if (btnState) {
                paramTypeList = { {static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH),"1"} };
            } else {
                paramTypeList = { {static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH),"0"} };
            }
            auto [code, _1] = obj->PostUpdateDataShared(paramTypeList);
            return std::make_tuple(code, stationSn);
        }
    }
    return std::make_tuple(-1, "-1");
}

static void OnShareBuryPointButtonClick(AnkerToggleBtn* shareBuryPointButton, int code, const std::string& stationSn)
{
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj) {
        if (code == 0) {
            bool shareBuryPointButtonStatus = shareBuryPointButton->GetState();
            ANKER_LOG_INFO << "Update burypoint switch is " << shareBuryPointButtonStatus;
            shareBuryPointButton->SetState(shareBuryPointButtonStatus);
            DataSharedReport(shareBuryPointButtonStatus);
            DeviceObjectBasePtr deviceObjectPtr = obj->getDeviceObjectFromSn(stationSn);
            if (deviceObjectPtr) {
                bool state = shareBuryPointButtonStatus;
                deviceObjectPtr->SendSwitchInfoToDevice("shareAnalytics", state);
            }

            if (shareBuryPointButtonStatus) {
                wxGetApp().app_config->set("burypoint_switch", "1");
            }
            else {
                wxGetApp().app_config->set("burypoint_switch", "0");
            }
            SetBuryPointSwitch();
        }
        else {
            wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
            wxSize mfSize = wxGetApp().mainframe->GetClientSize();
            wxSize dialogSize = AnkerSize(380, 100);
            wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2,
                mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
            AnkerDialog dialog(nullptr, wxID_ANY, _L("share_analytics_tips"), "", center, dialogSize);
            int result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextOkDialog);
            ANKER_LOG_INFO << "result: " << result;

            bool state = !shareBuryPointButton->GetState();
            if (state) {
                wxGetApp().app_config->set("burypoint_switch", "1");
            }
            else {
                wxGetApp().app_config->set("burypoint_switch", "0");
            }
            ANKER_LOG_INFO << "Update burypoint switch is " << state;
            SetBuryPointSwitch();
            shareBuryPointButton->SetState(state);
            DataSharedReport(state);
        }
    }
#ifdef OldOnShareBuryPointButtonClick
    auto obj = DatamangerUi::GetInstance().getAnkerNetBase();
    if (obj) {
        std::vector<std::tuple<std::string, int>> memberVec = obj->PostGetMemberType();
        for (int i = 0; i < memberVec.size(); ++i) {
            auto [stationSn, memberType] = memberVec[i];
            if (memberType == 1) {
                bool btnState = !shareBuryPointButton->GetState();
                shareBuryPointButton->SetState(btnState);
                continue;
            }

            std::vector<std::pair<int, std::string>> paramTypeList;
            if (shareBuryPointButton->GetState()) {
                paramTypeList = { {static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH),"1"} };
            }
            else {
                paramTypeList = { {static_cast<int>(AnkerNet::eUserParams::DATA_SHARE_SWITCH),"0"} };
            }
            auto [code, _1] = obj->PostUpdateDataShared(paramTypeList);
            if (code == 0) {
                bool shareBuryPointButtonStatus = shareBuryPointButton->GetState();
                ANKER_LOG_INFO << "Update burypoint switch is " << shareBuryPointButtonStatus;
                shareBuryPointButton->SetState(shareBuryPointButtonStatus);
                DataSharedReport(shareBuryPointButtonStatus);
                DeviceObjectBasePtr deviceObjectPtr = obj->getDeviceObjectFromSn(stationSn);
                if (deviceObjectPtr) {
                    bool state = shareBuryPointButtonStatus;
                    deviceObjectPtr->SendSwitchInfoToDevice("shareAnalytics", state);
                }

                if (shareBuryPointButtonStatus) {
                    wxGetApp().app_config->set("burypoint_switch", "1");
                }
                else {
                    wxGetApp().app_config->set("burypoint_switch", "0");
                }
            }
            else {
                wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
                wxSize mfSize = wxGetApp().mainframe->GetClientSize();
                wxSize dialogSize = AnkerSize(380, 100);
                wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2,
                    mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
                AnkerDialog dialog(nullptr, wxID_ANY, _L("share_analytics_tips"), "", center, dialogSize);
                int result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextOkDialog);
                ANKER_LOG_INFO << "result: " << result;

                bool state = !shareBuryPointButton->GetState();
                if (state) {
                    wxGetApp().app_config->set("burypoint_switch", "1");
                }
                else {
                    wxGetApp().app_config->set("burypoint_switch", "0");
                }
                ANKER_LOG_INFO << "Update burypoint switch is " << state;
                shareBuryPointButton->SetState(state);
                DataSharedReport(state);
            }
        }
    }
#endif // OldOnShareBuryPointButtonClick

}

static void CreateUserExperienceDialog() 
{
    wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
    wxSize mfSize = wxGetApp().mainframe->GetClientSize();
    wxSize dialogSize = AnkerSize(400, 313);
    wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
    wxString title = _AnkerL("user_experience_program");
    AnkerDialog dialog(nullptr, wxID_ANY, title, "", center, dialogSize);

    wxPanel* contentPanel = new wxPanel(&dialog);
    wxBoxSizer* contenSizer = new wxBoxSizer(wxVERTICAL);
    contentPanel->SetSizer(contenSizer);
    contenSizer->AddSpacer(AnkerLength(42));
    wxBoxSizer* shareBuryPointSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* shareBuryPointLabel = new wxStaticText(contentPanel, wxID_ANY, _L("share_analytics"));
    shareBuryPointLabel->SetForegroundColour(wxColour("#ffffff"));
    shareBuryPointLabel->SetFont(AnkerFontSingleton::getInstance().Font_Head_14);

    AnkerToggleBtn* shareBuryPointButton = new AnkerToggleBtn(contentPanel);
    shareBuryPointButton->SetMinSize(AnkerSize(51, 27));
    shareBuryPointButton->SetMaxSize(AnkerSize(51, 27));
    shareBuryPointButton->SetSize(AnkerSize(51, 27));
    shareBuryPointButton->SetBackgroundColour(wxColour("#333438"));
    shareBuryPointButton->SetStateColours(true, wxColour(129, 220, 129), wxColour(250, 250, 250));
    shareBuryPointButton->SetStateColours(false, wxColour(83, 83, 83), wxColour(219, 219, 219));

    QueryDataShared(shareBuryPointButton);
    shareBuryPointButton->Bind(wxCUSTOMEVT_ANKER_BTN_CLICKED, [shareBuryPointButton, contentPanel](wxCommandEvent& event) {
        auto futureResult = std::async(std::launch::async, [shareBuryPointButton]() {
            return PreHandleUpdateDataShared(shareBuryPointButton);
        });
        if (futureResult.wait_for(std::chrono::seconds(3)) == std::future_status::ready) {
            auto [code, sn] = futureResult.get();
            ANKER_LOG_INFO << "code is "<<code<<", sn  is" << sn;
            OnShareBuryPointButtonClick(shareBuryPointButton, code, sn);
        } else {
            ANKER_LOG_INFO << "PreHandleUpdate handle timeout!";
            OnShareBuryPointButtonClick(shareBuryPointButton, -1, "");
        }
        
        contentPanel->Layout();
        });
    shareBuryPointSizer->AddSpacer(AnkerLength(42));
    shareBuryPointSizer->Add(shareBuryPointLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
    shareBuryPointSizer->AddStretchSpacer();
    shareBuryPointSizer->Add(shareBuryPointButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
    shareBuryPointSizer->AddSpacer(AnkerLength(42));
    contenSizer->Add(shareBuryPointSizer, 0, wxALL | wxEXPAND, 0);
    contenSizer->AddSpacer(AnkerLength(12));

    MultipleLinesStaticText* contentText = new MultipleLinesStaticText(contentPanel);
    contentText->SetControlSize(AnkerSize(360, 160));
    contentText->SetControlColour(wxColour("#A8A8A8"), wxColour("#333438"));
    contentText->SetTextFont(Body_13);
    wxString contentString = _L("share_analytics_description") + _L("share_analytics_ensure");
    contentText->WriteText(contentString);
    wxString url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_en.html");
    if (MainFrame::currentSoftwareLanguageIsJapanese()) {
        url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_jp.html");
    }

    wxURI uri(url);
    url = uri.BuildURI();
    std::string stdUrl = url.ToStdString();
    contentText->ChangeTextToLink(_L("privacy_notice"), stdUrl, wxColour("#62D361"), false);

    contenSizer->Add(contentText, 0, wxLEFT | wxRIGHT | wxEXPAND, AnkerLength(42));
    dialog.SetCustomContent(contentPanel);
    int result = dialog.ShowAnkerModal(AnkerDialogType_CustomContent);
    ANKER_LOG_INFO << "result: " << result;
}

static wxMenu* generate_help_menu()
{
    wxMenu* helpMenu = new wxMenu();
//    append_menu_item(helpMenu, wxID_ANY, _L("Anker 3D &Drivers"), _L("Open the Anker3D drivers download page in your browser"),
//        [](wxCommandEvent&) { wxGetApp().open_web_page_localized("https://www.prusa3d.com/downloads"); });
//    append_menu_item(helpMenu, wxID_ANY, _L("Software &Releases"), _L("Open the software releases page in your browser"),
//        [](wxCommandEvent&) { wxGetApp().open_browser_with_warning_dialog("https://github.com/prusa3d/PrusaSlicer/releases", nullptr, false); });
////#        my $versioncheck = $self->_append_menu_item($helpMenu, "Check for &Updates...", "Check for new Slic3r versions", sub{
////#            wxTheApp->check_version(1);
////#        });
////#        $versioncheck->Enable(wxTheApp->have_version_check);
//    append_menu_item(helpMenu, wxID_ANY, wxString::Format(_L("%s &Website"), SLIC3R_APP_NAME),
//        wxString::Format(_L("Open the %s website in your browser"), SLIC3R_APP_NAME),
//        [](wxCommandEvent&) { wxGetApp().open_web_page_localized("https://www.prusa3d.com/slicerweb"); });
////        append_menu_item(helpMenu, wxID_ANY, wxString::Format(_L("%s &Manual"), SLIC3R_APP_NAME),
////                                             wxString::Format(_L("Open the %s manual in your browser"), SLIC3R_APP_NAME),
////            [this](wxCommandEvent&) { wxGetApp().open_browser_with_warning_dialog("http://manual.slic3r.org/"); });
//    helpMenu->AppendSeparator();
   // append_menu_item(helpMenu, wxID_ANY, _L("System &Info"), _L("Show system information"),
   //     [](wxCommandEvent&) { wxGetApp().system_info(); });
    // append_menu_item(helpMenu, wxID_ANY, _L("Show &Configuration Folder"), _L("Show user configuration folder (datadir)"),
    //    [](wxCommandEvent&) { Slic3r::GUI::desktop_open_datadir_folder(); });
    //append_menu_item(helpMenu, wxID_ANY, _L("Report an I&ssue"), wxString::Format(_L("Report an issue on %s"), SLIC3R_APP_NAME),
    //    [](wxCommandEvent&) { wxGetApp().open_browser_with_warning_dialog("https://github.com/prusa3d/slic3r/issues/new", nullptr, false); });
    //if (wxGetApp().is_editor())
    //    append_menu_item(helpMenu, wxID_ANY, wxString::Format(_L("&About %s"), SLIC3R_APP_NAME), _L("Show about dialog"),
    //        [](wxCommandEvent&) { Slic3r::GUI::about(); });
    //else
    //    append_menu_item(helpMenu, wxID_ANY, wxString::Format(_L("&About %s"), GCODEVIEWER_APP_NAME), _L("Show about dialog"),
    //        [](wxCommandEvent&) { Slic3r::GUI::about(); });
    //append_menu_item(helpMenu, wxID_ANY, _L("Show Tip of the Day") 
//#if 0//debug
//        + "\tCtrl+Shift+T"
//#endif
//        ,_L("Opens Tip of the day notification in bottom right corner or shows another tip if already opened."),
//        [](wxCommandEvent&) { wxGetApp().plater()->get_notification_manager()->push_hint_notification(false); });
//    helpMenu->AppendSeparator();
   // append_menu_item(helpMenu, wxID_ANY, _L("Keyboard Shortcuts") + sep + "&?", _L("Show the list of the keyboard shortcuts"),
   //     [](wxCommandEvent&) { wxGetApp().keyboard_shortcuts(); });
#if ENABLE_THUMBNAIL_GENERATOR_DEBUG
    helpMenu->AppendSeparator();
    append_menu_item(helpMenu, wxID_ANY, "DEBUG gcode thumbnails", "DEBUG ONLY - read the selected gcode file and generates png for the contained thumbnails",
        [](wxCommandEvent&) { wxGetApp().gcode_thumbnails_debug(); });
#endif // ENABLE_THUMBNAIL_GENERATOR_DEBUG

    
    append_menu_item(helpMenu, wxID_ANY, _L("common_menu_help_privacy"), _L("Show privacy policy"),
        [](wxCommandEvent&) {
            wxString url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_en.html");
            if (MainFrame::currentSoftwareLanguageIsJapanese()) {
                url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/privacy_notice_jp.html");
            }

            wxURI uri(url);
            url = uri.BuildURI();

            bool success = wxLaunchDefaultBrowser(url);
            if (success) {
            }
            else {
            
            }
            });
    append_menu_item(helpMenu, wxID_ANY, _L("common_menu_help_termsofuse"), _L("Show terms of use"),
        [](wxCommandEvent&) {
            wxString url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/terms_of_use_en.html");
            if (MainFrame::currentSoftwareLanguageIsJapanese()) {
                url = wxString("https://d7p3a6aivdrwg.cloudfront.net/anker_general/public/agreement/2024/12/13/terms_of_use_jp.html");
            }

            wxURI uri(url);
            url = uri.BuildURI();

            bool success = wxLaunchDefaultBrowser(url, wxBROWSER_NEW_WINDOW);
            if (success) {
            }
            else {

            }
            
            });
    append_menu_item(helpMenu, wxID_ANY, _L("common_tab_documentation_entrance"), _L("common_tab_documentation_entrance"),
        [](wxCommandEvent&) {
            std::string realUrl = "https://support.ankermake.com/s/article/Ankermake-Studio-Guide-for-printer#content6";
            wxLaunchDefaultBrowser(realUrl.c_str());
        });
    append_menu_item(helpMenu, wxID_ANY, _L("common_menu_help_copyright"), _L("Show copyright information"),
        [](wxCommandEvent&) {
            wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
            wxSize mfSize = wxGetApp().mainframe->GetClientSize();
            wxSize dialogSize = AnkerSize(400, 362);
            wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);
            wxString title = _AnkerL("common_popup_copyright_title");                            
            AnkerCopyrightDialog dialog(nullptr, wxID_ANY, title, "", center, dialogSize);
            dialog.ShowAnkerModal();
        });
        append_menu_item(helpMenu, wxID_ANY, _L("user_experience_program"), _L("user_experience_program"),
        [](wxCommandEvent&) {
            CreateUserExperienceDialog();
        });

    return helpMenu;
}

static void add_common_view_menu_items(wxMenu* view_menu, MainFrame* mainFrame, std::function<bool(void)> can_change_view)
{
    // The camera control accelerators are captured by GLCanvas3D::on_char().
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_iso") + sep + "&0", _L("Iso View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("iso"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    //view_menu->AppendSeparator();
    //TRN Main menu: View->Top 
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_top") + sep + "&1", _L("Top View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("top"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    //TRN Main menu: View->Bottom 
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_bottom") + sep + "&2", _L("Bottom View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("bottom"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_front") + sep + "&3", _L("Front View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("front"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_rear") + sep + "&4", _L("Rear View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("rear"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_left") + sep + "&5", _L("Left View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("left"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
    append_menu_item(view_menu, wxID_ANY, _L("common_menu_view_right") + sep + "&6", _L("Right View"), [mainFrame](wxCommandEvent&) { mainFrame->select_view("right"); },
        "", nullptr, [can_change_view]() { return can_change_view(); }, mainFrame);
}

void MainFrame::init_menubar_as_editor()
{
#ifdef __APPLE__
    wxMenuBar::SetAutoWindowMenu(false);
#endif

    // File menu
    wxMenu* fileMenu = new wxMenu;
    {
        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_newproject") + "\tCtrl+N", _L("Start a new project"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->new_project(); }, "", nullptr,
            [this](){return m_plater != nullptr && can_start_new_project(); }, this);
        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_openproject") + "\tCtrl+O", _L("Open a project file"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->load_project(); }, ""/*"open"*/, nullptr,
            [this](){return m_plater != nullptr; }, this);

        wxMenu* recent_projects_menu = new wxMenu();
        wxMenuItem* recent_projects_submenu = append_submenu(fileMenu, recent_projects_menu, wxID_ANY, _L("common_menu_file_recentproject"), "");
        m_recent_projects.UseMenu(recent_projects_menu);
        Bind(wxEVT_MENU, [this](wxCommandEvent& evt) {
            size_t file_id = evt.GetId() - wxID_FILE1;
            wxString filename = m_recent_projects.GetHistoryFile(file_id);
            if (wxFileExists(filename)) {
                if (wxGetApp().can_load_project())
                    m_plater->load_project(filename);
            }
            else
            {
                //wxMessageDialog msg(this, _L("The selected project is no longer available.\nDo you want to remove it from the recent projects list?"), _L("Error"), wxYES_NO | wxYES_DEFAULT);
                MessageDialog msg(this, _L("The selected project is no longer available.\nDo you want to remove it from the recent projects list?"), _L("Error"), wxYES_NO | wxYES_DEFAULT);
                if (msg.ShowModal() == wxID_YES)
                {
                    m_recent_projects.RemoveFileFromHistory(file_id);
                        std::vector<std::string> recent_projects;
                        size_t count = m_recent_projects.GetCount();
                        for (size_t i = 0; i < count; ++i)
                        {
                            recent_projects.push_back(into_u8(m_recent_projects.GetHistoryFile(i)));
                        }
                    wxGetApp().app_config->set_recent_projects(recent_projects);
                }
            }
            }, wxID_FILE1, wxID_FILE9);

        std::vector<std::string> recent_projects = wxGetApp().app_config->get_recent_projects();
        std::reverse(recent_projects.begin(), recent_projects.end());
        for (const std::string& project : recent_projects)
        {
            m_recent_projects.AddFileToHistory(from_u8(project));
        }

        Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& evt) { evt.Enable(m_recent_projects.GetCount() > 0); }, recent_projects_submenu->GetId());

        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_saveproject") + "\tCtrl+S", _L("Save current project file"),
            [this](wxCommandEvent&) { save_project(); }, ""/*"save"*/, nullptr,
            [this](){return m_plater != nullptr && can_save(); }, this);
#ifdef __APPLE__
        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_saveprojectas") + "\tCtrl+Shift+S", _L("Save current project file as"),
#else
        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_saveprojectas") + "\tCtrl+Alt+S", _L("Save current project file as"),
#endif // __APPLE__
            [this](wxCommandEvent&) { save_project_as(); }, ""/*"save"*/, nullptr,
            [this](){return m_plater != nullptr && can_save_as(); }, this);

        fileMenu->AppendSeparator();

        wxMenu* import_menu = new wxMenu();
        append_menu_item(import_menu, wxID_ANY, _L("common_menu_file_Importlist") + "\tCtrl+I", _L("Load a model"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->add_model(); }, ""/*"import_plater"*/, nullptr,
            [this](){return m_plater != nullptr; }, this);
        
        /*append_menu_item(import_menu, wxID_ANY, _L("Import STL (Imperial Units)"), _L("Load an model saved with imperial units"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->add_model(true); }, "import_plater", nullptr,
            [this](){return m_plater != nullptr; }, this);
        
        append_menu_item(import_menu, wxID_ANY, _L("Import SLA Archive") + dots, _L("Load an SLA archive"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->import_sl1_archive(); }, "import_plater", nullptr,
            [this](){return m_plater != nullptr && m_plater->get_ui_job_worker().is_idle(); }, this);  */
    
        /*append_menu_item(import_menu, wxID_ANY, _L("Import ZIP Archive") + dots, _L("Load a ZIP archive"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->import_zip_archive(); }, "import_plater", nullptr,
            [this]() {return m_plater != nullptr; }, this);

        import_menu->AppendSeparator();  */
        append_menu_item(import_menu, wxID_ANY, _L("common_menu_file_Importconfig") + "\tCtrl+L", _L("Load exported configuration file"),
            [this](wxCommandEvent&) { load_config_file(); }, ""/*"import_config",*/, nullptr,
            []() {return true; }, this);
       /*append_menu_item(import_menu, wxID_ANY, _L("Import Config from &Project") + dots + "\tCtrl+Alt+L", _L("Load configuration from project file"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->extract_config_from_project(); }, "import_config", nullptr,
            []() {return true; }, this);*/
        import_menu->AppendSeparator();
        append_menu_item(import_menu, wxID_ANY, _L("common_menu_file_Importconfigbundle") + "\tCtrl+Alt+L", _L("Load presets from a bundle"),
            [this](wxCommandEvent&) { load_configbundle(); }, ""/*"import_config_bundle"*/, nullptr,
            []() {return true; }, this);
        append_submenu(fileMenu, import_menu, wxID_ANY, _L("common_menu_file_import"), "");

        wxMenu* export_menu = new wxMenu();
        wxMenuItem* item_export_gcode = append_menu_item(export_menu, wxID_ANY, _L("common_menu_file_exportgocde") + "\tCtrl+G", _L("Export current plate as G-code"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->export_gcode(false, true); }, ""/*"export_gcode"*/, nullptr,
            [this](){return can_export_gcode(); }, this);
        m_changeable_menu_items.push_back(item_export_gcode);
        /*wxMenuItem* item_send_gcode = append_menu_item(export_menu, wxID_ANY, _L("S&end G-code") + dots + "\tCtrl+Shift+G", _L("Send to print current plate as G-code"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->send_gcode(); }, "export_gcode", nullptr,
            [this](){return can_send_gcode(); }, this);
        m_changeable_menu_items.push_back(item_send_gcode);   */
		append_menu_item(export_menu, wxID_ANY, _L("common_menu_file_exportgocde2sd") + "\tCtrl+U", _L("Export current plate as G-code to SD card / Flash drive"),
			[this](wxCommandEvent&) { if (m_plater) m_plater->export_gcode(true, true); }, ""/*"export_to_sd"*/, nullptr,
			[this]() {return can_export_gcode_sd(); }, this);
        export_menu->AppendSeparator();
        append_menu_item(export_menu, wxID_ANY, _L("common_menu_file_exportas"), _L("Export current plate as STL/OBJ"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->export_stl_obj(); }, ""/*"export_plater"*/, nullptr,
            [this](){return can_export_model(); }, this);
        /*append_menu_item(export_menu, wxID_ANY, _L("Export Plate as STL/OBJ &Including Supports") + dots, _L("Export current plate as STL/OBJ including supports"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->export_stl_obj(true); }, "export_plater", nullptr,
            [this](){return can_export_supports(); }, this);*/
// Deprecating AMF export. Let's wait for user feedback.
//        append_menu_item(export_menu, wxID_ANY, _L("Export Plate as &AMF") + dots, _L("Export current plate as AMF"),
//            [this](wxCommandEvent&) { if (m_plater) m_plater->export_amf(); }, "export_plater", nullptr,
//            [this](){return can_export_model(); }, this);
       /* export_menu->AppendSeparator();
        append_menu_item(export_menu, wxID_ANY, _L("Export &Toolpaths as OBJ") + dots, _L("Export toolpaths as OBJ"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->export_toolpaths_to_obj(); }, "export_plater", nullptr,
            [this]() {return can_export_toolpaths(); }, this);*/
        export_menu->AppendSeparator();
        append_menu_item(export_menu, wxID_ANY, _L("common_menu_file_exportconfig") +"\tCtrl+E", _L("Export current configuration to file"),
            [this](wxCommandEvent&) { export_config(); }, ""/*"export_config"*/, nullptr,
            []() {return true; }, this);
        append_menu_item(export_menu, wxID_ANY, _L("common_menu_file_exportconfigbundle"), _L("Export all presets to file"),
            [this](wxCommandEvent&) { export_configbundle(); }, ""/*"export_config_bundle"*/, nullptr,
            []() {return true; }, this);
        /*append_menu_item(export_menu, wxID_ANY, _L("Export Config Bundle With Physical Printers") + dots, _L("Export all presets including physical printers to file"),
            [this](wxCommandEvent&) { export_configbundle(true); }, "export_config_bundle", nullptr,
            []() {return true; }, this);    */
        append_submenu(fileMenu, export_menu, wxID_ANY, _L("common_menu_file_export"), "");

		/*append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_ejectsd") + dots + "\tCtrl+T", _L("Eject SD card / Flash drive after the G-code was exported to it."),
			[this](wxCommandEvent&) { if (m_plater) m_plater->eject_drive(); }, "eject_sd", nullptr,
			[this]() {return can_eject(); }, this);*/

       // fileMenu->AppendSeparator();

#if 0
        m_menu_item_repeat = nullptr;
        append_menu_item(fileMenu, wxID_ANY, _L("Quick Slice") +dots+ "\tCtrl+U", _L("Slice a file into a G-code"),
            [this](wxCommandEvent&) {
                wxTheApp->CallAfter([this]() {
                    quick_slice();
                    m_menu_item_repeat->Enable(is_last_input_file());
                }); }, "cog_go.png");
        append_menu_item(fileMenu, wxID_ANY, _L("Quick Slice and Save As") +dots +"\tCtrl+Alt+U", _L("Slice a file into a G-code, save as"),
            [this](wxCommandEvent&) {
            wxTheApp->CallAfter([this]() {
                    quick_slice(qsSaveAs);
                    m_menu_item_repeat->Enable(is_last_input_file());
                }); }, "cog_go.png");
        m_menu_item_repeat = append_menu_item(fileMenu, wxID_ANY, _L("Repeat Last Quick Slice") +"\tCtrl+Shift+U", _L("Repeat last quick slice"),
            [this](wxCommandEvent&) {
            wxTheApp->CallAfter([this]() {
                quick_slice(qsReslice);
            }); }, "cog_go.png");
        m_menu_item_repeat->Enable(false);
        fileMenu->AppendSeparator();
#endif
       
        fileMenu->AppendSeparator();
        append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_repairstl"), _L("Automatically repair an STL file"),
            [this](wxCommandEvent&) { repair_stl(); }, ""/*"wrench"*/, nullptr,
            []() { return true; }, this);
        m_menu_item_reslice_now = append_menu_item(fileMenu, wxID_ANY, _L("common_menu_file_slice") + "\tCtrl+R", _L("Start new slicing process"),
            [this](wxCommandEvent&) { reslice_now(); }, ""/*"re_slice"*/, nullptr,
            [this]() { return m_plater != nullptr && can_reslice(); }, this);
       // fileMenu->AppendSeparator();
        //append_menu_item(fileMenu, wxID_ANY, _L("&G-code Preview") + dots, _L("Open G-code viewer"),
        //    [this](wxCommandEvent&) { start_new_gcodeviewer_open_file(this); }, "", nullptr);
        //fileMenu->AppendSeparator();
        //#ifdef _WIN32
        //    append_menu_item(fileMenu, wxID_EXIT, _L("E&xit"), wxString::Format(_L("Exit %s"), SLIC3R_APP_NAME),
        //#else
        //    append_menu_item(fileMenu, wxID_EXIT, _L("&Quit"), wxString::Format(_L("Quit %s"), SLIC3R_APP_NAME),
        //#endif
        //    [this](wxCommandEvent&) { Close(false); }, "exit");
    }

    // Edit menu
    wxMenu* editMenu = nullptr;
    if (m_plater != nullptr)
    {
        editMenu = new wxMenu();
    #ifdef __APPLE__
        // Backspace sign
        wxString hotkey_delete = "\u232b";
    #else
        wxString hotkey_delete = "Del";
    #endif
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_selectall") + sep + GUI::shortkey_ctrl_prefix() + sep_space + "&A",
            _L("Selects all objects"), [this](wxCommandEvent&) { m_plater->select_all(); },
            "", nullptr, [this](){return can_select(); }, this);
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_deselectall") + sep + "&Esc",
            _L("Deselects all objects"), [this](wxCommandEvent&) { m_plater->deselect_all(); },
            "", nullptr, [this](){return can_deselect(); }, this);
       // editMenu->AppendSeparator();
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_delect") + sep + "&" + hotkey_delete,
            _L("Deletes the current selection"),[this](wxCommandEvent&) { m_plater->remove_selected(); },
            ""/*"remove_menu",*/, nullptr, [this]() {return can_delete(); }, this);
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_delectall") + sep + GUI::shortkey_ctrl_prefix() + sep_space + hotkey_delete,
            _L("Deletes all objects"), [this](wxCommandEvent&) { m_plater->reset_with_confirm(); },
           ""/* "delete_all_menu"*/, nullptr, [this]() {return can_delete_all(); }, this);

        editMenu->AppendSeparator();
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_undo") + sep + GUI::shortkey_ctrl_prefix() + sep_space + "&Z",
            _L("Undo"), [this](wxCommandEvent&) { m_plater->undo(); },
            ""/*"undo_menu"*/, nullptr, [this]() {return m_plater->can_undo(); }, this);
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_redo") + sep + GUI::shortkey_ctrl_prefix() + sep_space + "&Y",
            _L("Redo"), [this](wxCommandEvent&) { m_plater->redo(); },
            ""/*"redo_menu"*/, nullptr, [this]() {return m_plater->can_redo(); }, this);

        editMenu->AppendSeparator();
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_copy")+ sep + GUI::shortkey_ctrl_prefix() + sep_space + "&C",
            _L("Copy selection to clipboard"), [this](wxCommandEvent&) { m_plater->copy_selection_to_clipboard(); },
            ""/*"copy_menu"*/, nullptr, [this]() {return m_plater->can_copy_to_clipboard(); }, this);
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_paste") + sep + GUI::shortkey_ctrl_prefix() + sep_space + "&V" ,
            _L("Paste clipboard"), [this](wxCommandEvent&) { m_plater->paste_from_clipboard(); },
            ""/*"paste_menu"*/, nullptr, [this]() {return m_plater->can_paste_from_clipboard(); }, this);
        
        editMenu->AppendSeparator();
#ifdef __APPLE__
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_reload") + "\tCtrl+Shift+R",
            _L("Reload the plater from disk"), [this](wxCommandEvent&) { m_plater->reload_all_from_disk(); },
            "", nullptr, [this]() {return !m_plater->model().objects.empty(); }, this);
#else
        append_menu_item(editMenu, wxID_ANY, _L("common_menu_edit_reload") + sep + "F5",
            _L("Reload the plater from disk"), [this](wxCommandEvent&) { m_plater->reload_all_from_disk(); },
            "", nullptr, [this]() {return !m_plater->model().objects.empty(); }, this);
#endif // __APPLE__

       /* editMenu->AppendSeparator();
        append_menu_item(editMenu, wxID_ANY, _L("Searc&h") + "\tCtrl+F",
            _L("Search in settings"), [this](wxCommandEvent&) { m_plater->search(m_plater->IsShown()); },
            "search", nullptr, []() {return true; }, this);*/
    }

    // Window menu
    auto windowMenu = new wxMenu();
    {
        if (m_plater) {
            append_menu_item(windowMenu, wxID_HIGHEST + 1, _L("&Plater Tab") + "\tCtrl+1", _L("Show the plater"),
                [this](wxCommandEvent&) { select_tab(size_t(0)); }, ""/*"plater"*/, nullptr,
                []() {return true; }, this);
            windowMenu->AppendSeparator();
        }
        append_menu_item(windowMenu, wxID_HIGHEST + 2, _L("P&rint Settings Tab") + "\tCtrl+2", _L("Show the print settings"),
            [this/*, tab_offset*/](wxCommandEvent&) { select_tab(1); }, ""/*"cog"*/, nullptr,
            []() {return true; }, this);
        wxMenuItem* item_material_tab = append_menu_item(windowMenu, wxID_HIGHEST + 3, _L("&Filament Settings Tab") + "\tCtrl+3", _L("Show the filament settings"),
            [this/*, tab_offset*/](wxCommandEvent&) { select_tab(2); }, ""/*"spool"*/, nullptr,
            []() {return true; }, this);
        m_changeable_menu_items.push_back(item_material_tab);
        wxMenuItem* item_printer_tab = append_menu_item(windowMenu, wxID_HIGHEST + 4, _L("Print&er Settings Tab") + "\tCtrl+4", _L("Show the printer settings"),
            [this/*, tab_offset*/](wxCommandEvent&) { select_tab(3); }, ""/*"printer"*/, nullptr,
            []() {return true; }, this);
        m_changeable_menu_items.push_back(item_printer_tab);
        if (m_plater) {
            windowMenu->AppendSeparator();
            append_menu_item(windowMenu, wxID_HIGHEST + 5, _L("3&D") + "\tCtrl+5", _L("Show the 3D editing view"),
                [this](wxCommandEvent&) { m_plater->select_view_3D(ViewMode::VIEW_MODE_3D); }, ""/*"editor_menu"*/, nullptr,
                [this](){return can_change_view(); }, this);
            append_menu_item(windowMenu, wxID_HIGHEST + 6, _L("Pre&view") + "\tCtrl+6", _L("Show the 3D slices preview"),
                [this](wxCommandEvent&) { m_plater->select_view_3D(ViewMode::VIEW_MODE_PREVIEW); }, ""/*"preview_menu"*/, nullptr,
                [this](){return can_change_view(); }, this);
        }

        windowMenu->AppendSeparator();
        append_menu_item(windowMenu, wxID_ANY, _L("Shape Gallery"), _L("Open the dialog to modify shape gallery"),
            [this](wxCommandEvent&) {
                if (gallery_dialog()->show(true) == wxID_OK) {
                    wxArrayString input_files;
                    m_gallery_dialog->get_input_files(input_files);
                    //if (!input_files.IsEmpty())
                    //    m_plater->objectbar().getObjectList()->load_shape_object_from_gallery(input_files);
                }
            }, "shape_gallery", nullptr, []() {return true; }, this);
        
        windowMenu->AppendSeparator();
        append_menu_item(windowMenu, wxID_ANY, _L("Print &Host Upload Queue") + "\tCtrl+J", _L("Display the Print Host Upload Queue window"),
            [this](wxCommandEvent&) { m_printhost_queue_dlg->Show(); }, ""/*"upload_queue"*/, nullptr, []() {return true; }, this);
        
        windowMenu->AppendSeparator();
        append_menu_item(windowMenu, wxID_ANY, _L("Open New Instance") + "\tCtrl+Shift+I", _L("Open a new AnkerMake Studios instance"),
            [](wxCommandEvent&) { start_new_slicer(); }, "", nullptr, [this]() {return m_plater != nullptr && !wxGetApp().app_config->get_bool("single_instance"); }, this);

        windowMenu->AppendSeparator();
        append_menu_item(windowMenu, wxID_ANY, _L("Compare Presets")/* + "\tCtrl+F"*/, _L("Compare presets"), 
            [this](wxCommandEvent&) { diff_dialog.show();}, ""/*"compare"*/, nullptr, []() {return true; }, this);
    }

    // View menu
    wxMenu* viewMenu = nullptr;
    if (m_plater) {
        viewMenu = new wxMenu();
        add_common_view_menu_items(viewMenu, this, std::bind(&MainFrame::can_change_view, this));
        viewMenu->AppendSeparator();
        append_menu_check_item(viewMenu, wxID_ANY, _L("common_menu_view_showlabels") + sep + "&E", _L("Show object/instance labels in 3D scene"),
            [this](wxCommandEvent&) { m_plater->show_view3D_labels(!m_plater->are_view3D_labels_shown()); }, this,
            [this]() { return m_plater->is_view3D_shown(); }, [this]() { return m_plater->are_view3D_labels_shown(); }, this);
        //append_menu_check_item(viewMenu, wxID_ANY, _L("&Collapse Sidebar") + sep + "Shift+" + sep_space + "Tab", _L("Collapse sidebar"),
        //    [this](wxCommandEvent&) { m_plater->collapse_sidebar(!m_plater->is_sidebar_collapsed()); }, this,
        //    []() { return true; }, [this]() { return m_plater->is_sidebar_collapsed(); }, this);
        // OSX adds its own menu item to toggle fullscreen.
        append_menu_check_item(viewMenu, wxID_ANY, _L("common_menu_view_fullscreen") + "\t" + "F11", _L("Fullscreen"),
            [this](wxCommandEvent& event) {

#ifdef  __APPLE__
            ToggleFullScreen(this);
#endif
#ifdef  WIN32
                this->ShowFullScreen(!this->IsFullScreen(),
                    // wxFULLSCREEN_ALL: wxFULLSCREEN_NOMENUBAR | wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION
                    wxFULLSCREEN_NOSTATUSBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
#endif
        },
            this, []() { return true; }, [this]() { return this->IsFullScreen(); }, this);

    }
    // Settings menu
    wxMenu* settingsMenu = nullptr;
    if (m_plater) {
        settingsMenu = new wxMenu();
        wxMenu* languageMenu = new  wxMenu();
        //wxLanguage::wxLANGUAGE_CHINESE_CHINA = 130
       // wxLanguage::wxLANGUAGE_ENGLISH = 175
        //wxLanguage::wxLANGUAGE_JAPANESE = 428
        int type = wxGetApp().getCurrentLanguageType();
        ANKER_LOG_INFO << "Language type: " << type;
        std::string iconPath = "appbar_sure_icon";
        if (wxLanguage::wxLANGUAGE_ENGLISH == type) {
            append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languageen"), _L("Switch language to English"),
                [this](wxCommandEvent&) {  selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_English); },
                iconPath, nullptr, [&]() {return !m_plater->background_process_running(); }, this);
        }
        else {
            append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languageen"), _L("Switch language to English"),
                [this](wxCommandEvent&) {
                    selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_English);
                }, "", nullptr, [&]() {return !m_plater->background_process_running(); },this);
        }

        if (wxLanguage::wxLANGUAGE_JAPANESE_JAPAN == type ||
            wxLanguage::wxLANGUAGE_JAPANESE == type) {
            append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languagejp"), _L("Switch language to Japanese"),
                [this](wxCommandEvent&) { selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_Japanese); },
                iconPath, nullptr, [&]() {return !m_plater->background_process_running(); }, this);
        }
        else {
            append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languagejp"), _L("Switch language to Japanese"),
                [this](wxCommandEvent&) {
                    selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_Japanese);
                }, "", nullptr, [&]() {return !m_plater->background_process_running(); },this);
        }

        //add by Samuel, enable/disable muli-language by config 
        GUI::GUI_App* gui = dynamic_cast<GUI::GUI_App*>(GUI::GUI_App::GetInstance());
        if (gui != nullptr && false /* gui->app_config->get_bool("multi-language_enable")*/)
        {
            //comment by samuel, Due to Chinese translation problems, Chinese is blocked first
            if (wxLanguage::wxLANGUAGE_CHINESE_CHINA == type)
            {
                append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languagecn"), _L("Switch language to Chinese"),
                    [this](wxCommandEvent&) { selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_Chinese); },
                    iconPath, nullptr, [&]() {return !m_plater->background_process_running(); }, this);
            }
            else {
                append_menu_item(languageMenu, wxID_ANY, _L("common_menu_settings_languagecn"), _L("Switch language to Chinese"),
                    [this](wxCommandEvent&) {
                        selectLanguage(GUI_App::AnkerLanguageType::AnkerLanguageType_Chinese);
                    },"", nullptr, [&]() {return !m_plater->background_process_running(); },this);
            }
        }

        append_submenu(settingsMenu, languageMenu, wxID_ANY, _L("common_menu_settings_language"), "");

        append_menu_item(settingsMenu, wxID_ANY, _L("common_menu_settings_ota"), _L("Open update dialog"),
            [this](wxCommandEvent&) {
                auto ankerNet = AnkerNetInst();
                if (!ankerNet) {
                    return;
                }

                ankerNet->SetOtaCheckType(OtaCheckType_Manual);
                ankerNet->queryOTAInformation();
            });
    }

    // Calibration menu
    m_calibration_menu = nullptr;
    if (m_plater) {
        // SoftFever calibrations
        m_calibration_menu = new wxMenu();

        // Temperature
        append_menu_item(m_calibration_menu, wxID_ANY, _L("common_calib_temperature"), _L("Temperature"),
            [this](wxCommandEvent&) { wxGetApp().calib_filament_temperature_dialog((wxWindow*)this, m_plater); });

        // Flowrate
        auto flowrate_menu = new wxMenu();
        append_menu_item(flowrate_menu, wxID_ANY, _L("common_calib_pass_1"), _L("Flow rate test - Pass 1"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->calib_flowrate(1); });
        append_menu_item(flowrate_menu, wxID_ANY, _L("common_calib_pass_2"), _L("Flow rate test - Pass 2"),
            [this](wxCommandEvent&) { if (m_plater) m_plater->calib_flowrate(2); });
        append_submenu(m_calibration_menu, flowrate_menu, wxID_ANY, _L("common_calib_flow_rate"), _L("Flow rate"));

        // PA
        append_menu_item(m_calibration_menu, wxID_ANY, _L("common_calib_pressure_advance"), _L("Pressure advance"),
            [this](wxCommandEvent&) {
                wxGetApp().calib_pressure_advance_dialog((wxWindow*)this, m_plater);
            });

        // Retraction
        append_menu_item(m_calibration_menu, wxID_ANY, _L("common_calib_retraction_test"), _L("Retraction test"),
            [this](wxCommandEvent&) {
                wxGetApp().calib_retraction_dialog((wxWindow*)this, m_plater);
            });

        // Advance calibrations
        auto advance_menu = new wxMenu();
        append_menu_item(
            advance_menu, wxID_ANY, _L("common_calib_max_flowrate"), _L("Max flowrate"),
            [this](wxCommandEvent&) {
                wxGetApp().calib_max_flowrate_dialog((wxWindow*)this, m_plater);
            });
        append_menu_item(
            advance_menu, wxID_ANY, _L("common_calib_VFA"), _L("VFA"),
            [this](wxCommandEvent&) {
                wxGetApp().calib_vfa_dialog((wxWindow*)this, m_plater);
            });

        append_submenu(m_calibration_menu, advance_menu, wxID_ANY, _L("common_calib_more"), _L("More calibrations"));
        
        // Tolerance Test
        //append_menu_item(calib_menu, wxID_ANY, _L("Orca Tolerance Test"), _L("Orca Tolerance Test"),
        //    [this](wxCommandEvent&) {
        //        m_plater->new_project();
        //        m_plater->add_model(false, Slic3r::resources_dir() + "/calib/tolerance_test/OrcaToleranceTest.stl");
        //    }, "", nullptr,
        //    [this]() {return m_plater->is_view3D_shown();; }, this);

        //append_menu_item(m_calibration_menu, wxID_ANY, _(L("Ironing pattern calibration")), _(L("Create a test print to help you to set your over-bridge flow ratio and ironing pattern.")),
        //    [this](wxCommandEvent&) { wxGetApp().over_bridge_dialog(); });
        //// help
        //append_menu_item(m_calibration_menu, wxID_ANY, _L("Tutorial"), _L("Calibration help"),
        //    [this](wxCommandEvent&) { wxLaunchDefaultBrowser("https://github.com/SoftFever/OrcaSlicer/wiki/Calibration", wxBROWSER_NEW_WINDOW); }, "", nullptr,
        //    [this]() {return m_plater->is_view3D_shown();; }, this);

    }

    // Help menu
    auto helpMenu = generate_help_menu();

    // menubar
    // assign menubar to frame after appending items, otherwise special items
    // will not be handled correctly
    m_menubar = new wxMenuBar();
    m_menubar->SetFont(this->normal_font());
    m_menubar->Append(fileMenu, _L("common_menu_title_file"));
    if (editMenu) m_menubar->Append(editMenu, _L("common_menu_title_edit"));  
   // m_menubar->Append(windowMenu, _L("&Window"));
    if (viewMenu) m_menubar->Append(viewMenu, _L("common_menu_title_view"));
    if(settingsMenu) m_menubar->Append(settingsMenu, _L("common_menu_title_settings"));
    
    if (settingsMenu) m_menubar->Append(m_calibration_menu, _L("common_calib_calibration"));
    // Add additional menus from C++
    //wxGetApp().add_config_menu(m_menubar);
    m_menubar->Append(helpMenu, _L("common_menu_title_help"));
    
    m_pLoginMenu = new wxMenu();
    ShowUnLoginMenu();
    m_menubar->Append(m_pLoginMenu, _L("common_menu_account"));

#ifdef _MSW_DARK_MODE
    if (wxGetApp().tabs_as_menu()) {
        // Add separator 
        m_menubar->Append(new wxMenu(), "          ");
        add_tabs_as_menu(m_menubar, this, this);
    }
#endif
    SetMenuBar(m_menubar);

#ifdef _MSW_DARK_MODE
    if (wxGetApp().tabs_as_menu())
        m_menubar->EnableTop(6, false);
#endif

#ifdef __APPLE__
    // This fixes a bug on Mac OS where the quit command doesn't emit window close events
    // wx bug: https://trac.wxwidgets.org/ticket/18328
    wxMenu* apple_menu = m_menubar->OSXGetAppleMenu();
    if (apple_menu != nullptr) {
        apple_menu->Bind(wxEVT_MENU, [this](wxCommandEvent &) {
            Close();
        }, wxID_EXIT);
    }
#endif // __APPLE__

    if (plater()->printer_technology() == ptSLA)
        update_menubar();
}
void MainFrame::buryTime()
{
    m_buryTime = wxDateTime::Now();
}
std::string MainFrame::getWorkDuration()
{
    auto nowTime = wxDateTime::Now();
    auto timeDifference = nowTime - m_buryTime;
    std::string strTimeStamp = timeDifference.GetValue().ToString().ToStdString();
    return strTimeStamp;

}
void MainFrame::selectLanguage(GUI_App::AnkerLanguageType language)
{
    /* Before change application language, let's check unsaved changes on 3D-Scene
           * and draw user's attention to the application restarting after a language change
           */
    {
        wxPoint mfPoint = wxGetApp().mainframe->GetPosition();
        wxSize mfSize = wxGetApp().mainframe->GetClientSize();
        wxSize dialogSize = AnkerSize(400, 200);
        wxPoint center = wxPoint(mfPoint.x + mfSize.GetWidth() / 2 - dialogSize.GetWidth() / 2, mfPoint.y + mfSize.GetHeight() / 2 - dialogSize.GetHeight() / 2);

        AnkerDialog dialog(nullptr, wxID_ANY, _L("common_popup_language_title"),
            _L("common_popup_language_notice"), center, dialogSize);

        int result = dialog.ShowAnkerModal(AnkerDialogType_DisplayTextNoYesDialog);
        ANKER_LOG_INFO << "result: " << result;
        if (result != wxID_OK) {
            return;
        }
    }

    if (true == plater()->is_exporting_acode())
    {
        AnkerMessageBox(this, _u8L("common_reject_switch_language"), _u8L("common_popup_titlenotice"), false);
        return;
    }

    wxGetApp().switch_language(language);
    //by samuel,should upodate language type in  DataManger
    wxString languageCode = wxGetApp().current_language_code_safe();
    int index = languageCode.find('_');

    auto ankerNet = AnkerNetInst();
    if (ankerNet) {
        std::string Language  = languageCode.substr(0, index).ToStdString();
        std::string Country = languageCode.substr(index + 1, languageCode.Length() - index).ToStdString();
        ankerNet->ResetLanguage(Country, Language);
    }
}

bool MainFrame::currentSoftwareLanguageIsJapanese()
{
    // wxLanguage::wxLANGUAGE_CHINESE_CHINA = 130
    // wxLanguage::wxLANGUAGE_ENGLISH = 175
    // wxLanguage::wxLANGUAGE_JAPANESE = 428
    int type = wxGetApp().getCurrentLanguageType();
    ANKER_LOG_INFO << "The current software language type is " << type;
    if (type == wxLanguage::wxLANGUAGE_JAPANESE) {
        return true;
    }
    return false;
}

bool MainFrame::languageIsJapanese()
{
#ifdef _WIN32
    int sysLanguage = wxLocale::GetSystemLanguage();
    wxString sysLanguageName = wxLocale::GetLanguageName(wxLANGUAGE_JAPANESE_JAPAN);
    ANKER_LOG_INFO << "MainFrame::languageIsJapanese: " << ", sysLanguage: " << sysLanguage << ", wxLANGUAGE_JAPANESE: " << wxLANGUAGE_JAPANESE_JAPAN;
    return (sysLanguage == wxLanguage::wxLANGUAGE_JAPANESE_JAPAN ||
        sysLanguage == wxLanguage::wxLANGUAGE_JAPANESE) ;
#elif __APPLE__
    FILE* fp = NULL;
    char buffer[1024];
    //Get Language info.
    memset(buffer, 0, 1024);
    fp = popen("defaults read NSGlobalDomain AppleLanguages", "r");
    if (fp == NULL) {
        ANKER_LOG_ERROR << "Failed to read NSGlobalDomain AppleLanguages.";
        return false;
    }

    ANKER_LOG_INFO << "Language info:";
    std::string languageStr = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        languageStr += std::string(buffer);
    }
    pclose(fp);
    ANKER_LOG_INFO << "Language info:" << languageStr;
    int index1 = languageStr.find('(');
    int index2 = languageStr.find(',');

    // Consider the case where there is only one language
    if (index2 == std::string::npos)
    {
        index2 = languageStr.find(')');
        if (index2 == std::string::npos)
        {
            ANKER_LOG_INFO << "system langage info formate exception,please check";
            return false;
        }
    }

    if (index1 != std::string::npos) {
        index1++;
        std::string defaultLanguage = languageStr.substr(index1, index2 - index1);
        ANKER_LOG_INFO << "defaultLanguage: " << defaultLanguage ;
        if (defaultLanguage.find("ja-") != std::string::npos) {
            return true;
        }
    }

    return false;
#endif

}


std::string MainFrame::GetTranslateLanguage()
{
    bool multi_language_enable = false;
    GUI::GUI_App* gui = dynamic_cast<GUI::GUI_App*>(GUI::GUI_App::GetInstance());
    if (gui != nullptr && gui->app_config->get_bool("multi-language_enable"))
        multi_language_enable = true;

#ifdef _WIN32
    int sysLanguage = wxLocale::GetSystemLanguage();
    wxString sysLanguageName = wxLocale::GetLanguageName(wxLANGUAGE_JAPANESE_JAPAN);
    ANKER_LOG_INFO << "MainFrame::languageIsJapanese: " << ", sysLanguage: " << sysLanguage << ", wxLANGUAGE_JAPANESE: " << wxLANGUAGE_JAPANESE_JAPAN;
    if ((sysLanguage == wxLanguage::wxLANGUAGE_JAPANESE_JAPAN || sysLanguage == wxLanguage::wxLANGUAGE_JAPANESE) && multi_language_enable)
        return std::string("ja");
    else  if (sysLanguage == wxLanguage::wxLANGUAGE_CHINESE_CHINA && multi_language_enable)
        return std::string("zh_CN");
    else
        return std::string("en");

#elif __APPLE__
    FILE* fp = NULL;
    char buffer[1024];
    //Get Language info.
    memset(buffer, 0, 1024);
    fp = popen("defaults read NSGlobalDomain AppleLanguages", "r");
    if (fp == NULL) {
        ANKER_LOG_ERROR << "Failed to read NSGlobalDomain AppleLanguages.";
        return std::string("en");
    }

    ANKER_LOG_INFO << "Language info:";
    std::string languageStr = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        languageStr += std::string(buffer);
    }
    pclose(fp);
    ANKER_LOG_INFO << "Language info:" << languageStr;
    int index1 = languageStr.find('(');
    int index2 = languageStr.find(',');
    // Consider the case where there is only one language
    if (index2 == std::string::npos)
    {
        index2 = languageStr.find(')');
        if (index2 == std::string::npos)
        {
            ANKER_LOG_INFO << "system langage info formate exception,please check";
            return std::string("en");
        }
    }

    if (index1 != std::string::npos) {
        index1++;
        std::string defaultLanguage = languageStr.substr(index1, index2 - index1);
        ANKER_LOG_INFO << "defaultLanguage: " << defaultLanguage;
        if (defaultLanguage.find("ja-") != std::string::npos && multi_language_enable) {
            return std::string("ja");
        }
        else if (defaultLanguage.find("zh-Hans-CN") != std::string::npos && multi_language_enable) {
            return std::string("zh_CN");
        }
        else
            return std::string("en");

    }

    return std::string("en");
#endif
}

void MainFrame::open_menubar_item(const wxString& menu_name,const wxString& item_name)
{
    if (m_menubar == nullptr)
        return;
    // Get menu object from menubar
    int     menu_index = m_menubar->FindMenu(menu_name);
    wxMenu* menu       = m_menubar->GetMenu(menu_index);
    if (menu == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Mainframe open_menubar_item function couldn't find menu: " << menu_name;
        return;
    }
    // Get item id from menu
    int     item_id   = menu->FindItem(item_name);
    if (item_id == wxNOT_FOUND)
    {
        // try adding three dots char
        item_id = menu->FindItem(item_name + dots);
    }
    if (item_id == wxNOT_FOUND)
    {
        BOOST_LOG_TRIVIAL(error) << "Mainframe open_menubar_item function couldn't find item: " << item_name;
        return;
    }
    // wxEVT_MENU will trigger item
    wxPostEvent((wxEvtHandler*)menu, wxCommandEvent(wxEVT_MENU, item_id));
}

void MainFrame::init_menubar_as_gcodeviewer()
{
    wxMenu* fileMenu = new wxMenu;
    {
        append_menu_item(fileMenu, wxID_ANY, _L("&Open G-code") + dots + "\tCtrl+O", _L("Open a G-code file"),
            [this](wxCommandEvent&) { if (m_plater != nullptr) m_plater->load_gcode(); }, "open", nullptr,
            [this]() {return m_plater != nullptr; }, this);
#ifdef __APPLE__
        append_menu_item(fileMenu, wxID_ANY, _L("Re&load from Disk") + dots + "\tCtrl+Shift+R",
            _L("Reload the plater from disk"), [this](wxCommandEvent&) { m_plater->reload_gcode_from_disk(); },
            "", nullptr, [this]() { return !m_plater->get_last_loaded_gcode().empty(); }, this);
#else
        append_menu_item(fileMenu, wxID_ANY, _L("Re&load from Disk") + sep + "F5",
            _L("Reload the plater from disk"), [this](wxCommandEvent&) { m_plater->reload_gcode_from_disk(); },
            "", nullptr, [this]() { return !m_plater->get_last_loaded_gcode().empty(); }, this);
#endif // __APPLE__
        fileMenu->AppendSeparator();
        append_menu_item(fileMenu, wxID_ANY, _L("Export &Toolpaths as OBJ") + dots, _L("Export toolpaths as OBJ"),
            [this](wxCommandEvent&) { if (m_plater != nullptr) m_plater->export_toolpaths_to_obj(); }, "export_plater", nullptr,
            [this]() {return can_export_toolpaths(); }, this);
        append_menu_item(fileMenu, wxID_ANY, _L("Open &AnkerMake Studio") + dots, _L("Open AnkerMake Studio"),
            [](wxCommandEvent&) { start_new_slicer(); }, "", nullptr,
            []() {return true; }, this);
        fileMenu->AppendSeparator();
        append_menu_item(fileMenu, wxID_EXIT, _L("&Quit"), wxString::Format(_L("Quit %s"), SLIC3R_APP_NAME),
            [this](wxCommandEvent&) { Close(false); });
    }

    // View menu
    wxMenu* viewMenu = nullptr;
    if (m_plater != nullptr) {
        viewMenu = new wxMenu();
        add_common_view_menu_items(viewMenu, this, std::bind(&MainFrame::can_change_view, this));
        viewMenu->AppendSeparator();
        append_menu_check_item(viewMenu, wxID_ANY, _L("Show Legen&d") + sep + "L", _L("Show legend"),
            [this](wxCommandEvent&) { m_plater->show_legend(!m_plater->is_legend_shown()); }, this,
            [this]() { return m_plater->is_preview_shown(); }, [this]() { return m_plater->is_legend_shown(); }, this);
    }

    // helpmenu
    auto helpMenu = generate_help_menu();

    m_menubar = new wxMenuBar();
    m_menubar->Append(fileMenu, _L("&File"));
    if (viewMenu != nullptr) m_menubar->Append(viewMenu, _L("&View"));
    // Add additional menus from C++
    wxGetApp().add_config_menu(m_menubar);
    m_menubar->Append(helpMenu, _L("&Help"));
    SetMenuBar(m_menubar);

#ifdef __APPLE__
    // This fixes a bug on Mac OS where the quit command doesn't emit window close events
    // wx bug: https://trac.wxwidgets.org/ticket/18328
    wxMenu* apple_menu = m_menubar->OSXGetAppleMenu();
    if (apple_menu != nullptr) {
        apple_menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            Close();
            }, wxID_EXIT);
    }
#endif // __APPLE__
}

void MainFrame::update_menubar()
{
    if (wxGetApp().is_gcode_viewer())
        return;

    const bool is_fff = plater()->printer_technology() == ptFFF;

    m_changeable_menu_items[miExport]       ->SetItemLabel((is_fff ? _L("Export &G-code")         : _L("E&xport"))        + dots    + "\tCtrl+G");
    //m_changeable_menu_items[miSend]         ->SetItemLabel((is_fff ? _L("S&end G-code")           : _L("S&end to print")) + dots    + "\tCtrl+Shift+G");

    m_changeable_menu_items[miMaterialTab]  ->SetItemLabel((is_fff ? _L("&Filament Settings Tab") : _L("Mate&rial Settings Tab"))   + "\tCtrl+3");
    m_changeable_menu_items[miMaterialTab]  ->SetBitmap(*get_bmp_bundle(is_fff ? "spool"   : "resin"));

    //m_changeable_menu_items[miPrinterTab]   ->SetBitmap(*get_bmp_bundle(is_fff ? "printer" : "sla_printer"));
}

#if 0
// To perform the "Quck Slice", "Quick Slice and Save As", "Repeat last Quick Slice" and "Slice to SVG".
void MainFrame::quick_slice(const int qs)
{
//     my $progress_dialog;
    wxString input_file;
//  eval
//     {
    // validate configuration
	DynamicPrintConfig tempConfig;
    //update by alves, cover right parameters data to the config if fff_print then process
	DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
    {	
        Slic3r::GUI::wxGetApp().plater()->sidebarnew().updatePreset(config);
    }
	       
    auto valid = config.validate();
    if (! valid.empty()) {
        show_error(this, valid);
        return;
    }

    // select input file
    if (!(qs & qsReslice)) {
        wxFileDialog dlg(this, _L("Choose a file to slice (STL/OBJ/AMF/3MF/AKPRO):"),
            wxGetApp().app_config->get_last_dir(), "",
            file_wildcards(FT_MODEL), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK)
            return;
        input_file = dlg.GetPath();
        if (!(qs & qsExportSVG))
            m_qs_last_input_file = input_file;
    }
    else {
        if (m_qs_last_input_file.IsEmpty()) {
            //wxMessageDialog dlg(this, _L("No previously sliced file."),
            MessageDialog dlg(this, _L("No previously sliced file."),
                _L("Error"), wxICON_ERROR | wxOK);
            dlg.ShowModal();
            return;
        }
        if (std::ifstream(m_qs_last_input_file.ToUTF8().data())) {
            //wxMessageDialog dlg(this, _L("Previously sliced file (")+m_qs_last_input_file+_L(") not found."),
            MessageDialog dlg(this, _L("Previously sliced file (")+m_qs_last_input_file+_L(") not found."),
                _L("File Not Found"), wxICON_ERROR | wxOK);
            dlg.ShowModal();
            return;
        }
        input_file = m_qs_last_input_file;
    }
    auto input_file_basename = get_base_name(input_file);
    wxGetApp().app_config->update_skein_dir(get_dir_name(input_file));

    auto bed_shape = Slic3r::Polygon::new_scale(config.option<ConfigOptionPoints>("bed_shape")->values);
//     auto print_center = Slic3r::Pointf->new_unscale(bed_shape.bounding_box().center());
// 
//     auto sprint = new Slic3r::Print::Simple(
//         print_center = > print_center,
//         status_cb = > [](int percent, const wxString& msg) {
//         m_progress_dialog->Update(percent, msg+"...");
//     });

    // keep model around
    auto model = Slic3r::Model::read_from_file(input_file.ToUTF8().data());

//     sprint->apply_config(config);
//     sprint->set_model(model);

    // Copy the names of active presets into the placeholder parser.
//     wxGetApp().preset_bundle->export_selections(sprint->placeholder_parser);

    // select output file
    wxString output_file;
    if (qs & qsReslice) {
        if (!m_qs_last_output_file.IsEmpty())
            output_file = m_qs_last_output_file;
    } 
    else if (qs & qsSaveAs) {
        // The following line may die if the output_filename_format template substitution fails.
        wxFileDialog dlg(this, format_wxstr(_L("Save %s file as:"), ((qs & qsExportSVG) ? _L("SVG") : _L("G-code"))),
            wxGetApp().app_config->get_last_output_dir(get_dir_name(output_file)), get_base_name(input_file), 
            qs & qsExportSVG ? file_wildcards(FT_SVG) : file_wildcards(FT_GCODE),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK)
            return;
        output_file = dlg.GetPath();
        if (!(qs & qsExportSVG))
            m_qs_last_output_file = output_file;
        wxGetApp().app_config->update_last_output_dir(get_dir_name(output_file));
    } 
    else if (qs & qsExportPNG) {
        wxFileDialog dlg(this, _L("Save zip file as:"),
            wxGetApp().app_config->get_last_output_dir(get_dir_name(output_file)),
            get_base_name(output_file), "*.sl1", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK)
            return;
        output_file = dlg.GetPath();
    }

    // show processbar dialog
    m_progress_dialog = new wxProgressDialog(_L("Slicing") + dots,
        // TRN ProgressDialog on reslicing: "input file basename"
        format_wxstr(_L("Processing %s"), (input_file_basename + dots)),
        100, nullptr, wxPD_AUTO_HIDE);
    m_progress_dialog->Pulse();
    {
//         my @warnings = ();
//         local $SIG{ __WARN__ } = sub{ push @warnings, $_[0] };

//         sprint->output_file(output_file);
//         if (export_svg) {
//             sprint->export_svg();
//         }
//         else if(export_png) {
//             sprint->export_png();
//         }
//         else {
//             sprint->export_gcode();
//         }
//         sprint->status_cb(undef);
//         Slic3r::GUI::warning_catcher($self)->($_) for @warnings;
    }
    m_progress_dialog->Destroy();
    m_progress_dialog = nullptr;

    auto message = format(_L("%1% was successfully sliced."), input_file_basename);
//     wxTheApp->notify(message);
    //wxMessageDialog(this, message, _L("Slicing Done!"), wxOK | wxICON_INFORMATION).ShowModal();
    MessageDialog(this, message, _L("Slicing Done!"), wxOK | wxICON_INFORMATION).ShowModal();
//     };
//     Slic3r::GUI::catch_error(this, []() { if (m_progress_dialog) m_progress_dialog->Destroy(); });
}
#endif

void MainFrame::reslice_now()
{
    if (m_plater)
    {
        //update by alves
        //m_plater->reslice();
        m_plater->onSliceNow();
    }
}

void MainFrame::repair_stl()
{
    wxString input_file;
    {
        wxFileDialog dlg(this, _L("Select the STL file to repair:"),
            wxGetApp().app_config->get_last_dir(), "",
            file_wildcards(FT_STL), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK)
            return;
        input_file = dlg.GetPath();
    }

    wxString output_file = input_file;
    {
        wxFileDialog dlg( this, L("Save OBJ file (less prone to coordinate errors than STL) as:"),
                                        get_dir_name(output_file), get_base_name(output_file, ".obj"),
                                        file_wildcards(FT_OBJ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK)
            return;
        output_file = dlg.GetPath();
    }

    Slic3r::TriangleMesh tmesh;
    tmesh.ReadSTLFile(input_file.ToUTF8().data());
    tmesh.WriteOBJFile(output_file.ToUTF8().data());
    Slic3r::GUI::show_info(this, _L("Your file was repaired."), _L("Repair"));
}

void MainFrame::export_config()
{
    // Generate a cummulative configuration for the selected print, filaments and printer.
    //update by alves, cover right parameters data to the config if fff_print then process
	DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
    {		
        Slic3r::GUI::wxGetApp().plater()->sidebarnew().updatePreset(config);
    }	
        
    // Validate the cummulative configuration.
    auto valid = config.validate();
    if (! valid.empty()) {
        show_error(this, valid);
        return;
    }
    // Ask user for the file name for the config file.
    std::string dirName = get_dir_name(m_last_config);
    std::string baseName = get_base_name(m_last_config);
    std::string lastName = wxGetApp().app_config->get_last_dir();

    wxString wxDirName = wxString::FromUTF8(dirName);
    wxString wxBaseName = wxString::FromUTF8(baseName);
    wxString wxLastName = wxString::FromUTF8(lastName);

    wxFileDialog dlg(this, _L("Save configuration as:"),
        !m_last_config.IsEmpty() ? wxDirName : wxLastName,
        !m_last_config.IsEmpty() ? wxBaseName : "config.ini",
        file_wildcards(FT_INI), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    wxString file;
    if (dlg.ShowModal() == wxID_OK)
        file = dlg.GetPath();
    if (!file.IsEmpty()) {
        wxGetApp().app_config->update_config_dir(get_dir_name(file));
        m_last_config = file;
        config.save(file.ToUTF8().data());
    }
}

// Load a config file containing a Print, Filament & Printer preset.
void MainFrame::load_config_file()
{
    if (!wxGetApp().check_and_save_current_preset_changes(_L("Loading of a configuration file"), "", false))
        return;
    wxFileDialog dlg(this, _L("Select configuration to load:"),
        !m_last_config.IsEmpty() ? get_dir_name(m_last_config) : wxGetApp().app_config->get_last_dir(),
        "config.ini", "INI files (*.ini, *.gcode)|*.ini;*.INI;*.gcode;*.g", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	wxString file;
    if (dlg.ShowModal() == wxID_OK)
        file = dlg.GetPath();
    if (! file.IsEmpty() && this->load_config_file(file.ToUTF8().data())) {
        wxGetApp().app_config->update_config_dir(get_dir_name(file));
        m_last_config = file;
    }
}

// Load a config file containing a Print, Filament & Printer preset from command line.
bool MainFrame::load_config_file(const std::string &path)
{
    try {
        ConfigSubstitutions config_substitutions = wxGetApp().preset_bundle->load_config_file(path, ForwardCompatibilitySubstitutionRule::Enable);
        if (!config_substitutions.empty())
            show_substitutions_info(config_substitutions, path);
    } catch (const std::exception &ex) {
        show_error(this, ex.what());
        return false;
    }

    m_plater->check_selected_presets_visibility(ptFFF);
    wxGetApp().load_current_presets();
    return true;
}

void MainFrame::export_configbundle(bool export_physical_printers /*= false*/)
{
    if (!wxGetApp().check_and_save_current_preset_changes(_L("Exporting configuration bundle"),
                                                          _L("Some presets are modified and the unsaved changes will not be exported into configuration bundle."), false, true))
        return;
    // validate current configuration in case it's dirty
    //update by alves, cover right parameters data to the config if fff_print then process
	DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF)
    {		
        Slic3r::GUI::wxGetApp().plater()->sidebarnew().updatePreset(config);        
    }	        

    auto err = config.validate();
    if (! err.empty()) {
        show_error(this, err);
        return;
    }
    // Ask user for a file name.
    wxFileDialog dlg(this, _L("Save presets bundle as:"),
        !m_last_config.IsEmpty() ? get_dir_name(m_last_config) : wxGetApp().app_config->get_last_dir(),
        SLIC3R_APP_KEY "_config_bundle.ini",
        file_wildcards(FT_INI), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    wxString file;
    if (dlg.ShowModal() == wxID_OK)
        file = dlg.GetPath();
    if (!file.IsEmpty()) {
        // Export the config bundle.
        wxGetApp().app_config->update_config_dir(get_dir_name(file));
        try {
            wxGetApp().preset_bundle->export_configbundle(file.ToUTF8().data(), false, export_physical_printers);
        } catch (const std::exception &ex) {
			show_error(this, ex.what());
        }
    }
}

// Loading a config bundle with an external file name used to be used
// to auto - install a config bundle on a fresh user account,
// but that behavior was not documented and likely buggy.
void MainFrame::load_configbundle(wxString file/* = wxEmptyString, const bool reset_user_profile*/)
{
    if (!wxGetApp().check_and_save_current_preset_changes(_L("Loading of a configuration bundle"), "", false))
        return;
    if (file.IsEmpty()) {
        wxFileDialog dlg(this, _L("Select configuration to load:"),
            !m_last_config.IsEmpty() ? get_dir_name(m_last_config) : wxGetApp().app_config->get_last_dir(),
            "config.ini", file_wildcards(FT_INI), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK)
            return;
        file = dlg.GetPath();
	}

    wxGetApp().app_config->update_config_dir(get_dir_name(file));

    size_t presets_imported = 0;
    PresetsConfigSubstitutions config_substitutions;
    try {
        // Report all substitutions.
        std::tie(config_substitutions, presets_imported) = wxGetApp().preset_bundle->load_configbundle(
            file.ToUTF8().data(), PresetBundle::LoadConfigBundleAttribute::SaveImported, ForwardCompatibilitySubstitutionRule::Enable);
    } catch (const std::exception &ex) {
        show_error(this, ex.what());
        return;
    }

    if (! config_substitutions.empty())
        show_substitutions_info(config_substitutions);

    // Load the currently selected preset into the GUI, update the preset selection box.
	wxGetApp().load_current_presets();

    const auto message = wxString::Format(_L("%d presets successfully imported."), presets_imported);
    Slic3r::GUI::show_info(this, message, wxString("Info"));
}

// Load a provied DynamicConfig into the Print / Filament / Printer tabs, thus modifying the active preset.
// Also update the plater with the new presets.
void MainFrame::load_config(const DynamicPrintConfig& config)
{
	PrinterTechnology printer_technology = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology();
	const auto       *opt_printer_technology = config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology");
	if (opt_printer_technology != nullptr && opt_printer_technology->value != printer_technology) {
		printer_technology = opt_printer_technology->value;
		this->plater()->set_printer_technology(printer_technology);
	}
#if 0
	for (auto tab : wxGetApp().tabs_list)
		if (tab->supports_printer_technology(printer_technology)) {
			if (tab->type() == Slic3r::Preset::TYPE_PRINTER)
				static_cast<TabPrinter*>(tab)->update_pages();
			tab->load_config(config);
		}
    if (m_plater)
        m_plater->on_config_change(config);
#else
	// Load the currently selected preset into the GUI, update the preset selection box.
    //FIXME this is not quite safe for multi-extruder printers,
    // as the number of extruders is not adjusted for the vector values.
    // (see PresetBundle::update_multi_material_filament_presets())
    // Better to call PresetBundle::load_config() instead?
#if SHOW_OLD_SETTING_DIALOG
    for (auto tab : wxGetApp().tabs_list)
        if (tab->supports_printer_technology(printer_technology)) {
            // Only apply keys, which are present in the tab's config. Ignore the other keys.
			for (const std::string &opt_key : tab->get_config()->diff(config))
				// Ignore print_settings_id, printer_settings_id, filament_settings_id etc.
				if (! boost::algorithm::ends_with(opt_key, "_settings_id"))
					tab->get_config()->option(opt_key)->set(config.option(opt_key));
        }
#endif

    // add by allen for ankerCfgDlg
    for (auto tab : wxGetApp().ankerTabsList)
        if (tab->supports_printer_technology(printer_technology)) {
            // Only apply keys, which are present in the tab's config. Ignore the other keys.
            for (const std::string& opt_key : tab->get_config()->diff(config))
                // Ignore print_settings_id, printer_settings_id, filament_settings_id etc.
                if (!boost::algorithm::ends_with(opt_key, "_settings_id"))
                    tab->get_config()->option(opt_key)->set(config.option(opt_key));
        }

    wxGetApp().load_current_presets();
#endif
}

void MainFrame::select_tab(Tab* tab)
{
    if (!tab)
        return;
    int page_idx = m_tabpanel->FindPage(tab);
    if (page_idx != wxNOT_FOUND && m_layout == ESettingsLayout::Dlg)
        page_idx++;
    select_tab(size_t(page_idx));
}


void MainFrame::selectAnkerTab(AnkerTab* tab)
{
    if (!tab || !m_ankerCfgDlg)
        return;
    int page_idx = m_ankerCfgDlg->m_rightPanel->FindPage(tab);
    if (page_idx != wxNOT_FOUND && m_layout == ESettingsLayout::Dlg)
        page_idx++;
    select_tab(size_t(page_idx));
}

void MainFrame::showAnkerCfgDlg() {
    if (m_ankerCfgDlg && !m_ankerCfgDlg->IsShown()) {
        m_ankerCfgDlg->CenterOnParent();
        m_ankerCfgDlg->ShowModal();
    }
}

void MainFrame::select_tab(size_t tab/* = size_t(-1)*/)
{
    bool tabpanel_was_hidden = false;

    // Controls on page are created on active page of active tab now.
    // We should select/activate tab before its showing to avoid an UI-flickering
    auto select = [this, tab](bool was_hidden) {
        // when tab == -1, it means we should show the last selected tab
        size_t new_selection = tab == (size_t)(-1) ? m_last_selected_tab : (m_layout == ESettingsLayout::Dlg && tab != 0) ? tab - 1 : tab;

        if (m_tabpanel->GetSelection() != (int)new_selection)
            m_tabpanel->SetSelection(new_selection);
#ifdef _MSW_DARK_MODE
        if (wxGetApp().tabs_as_menu()) {
            if (Tab* cur_tab = dynamic_cast<Tab*>(m_tabpanel->GetPage(new_selection)))
                update_marker_for_tabs_menu((m_layout == ESettingsLayout::Old ? m_menubar : m_settings_dialog.menubar()), cur_tab->title(), m_layout == ESettingsLayout::Old);
            else if (tab == 0 && m_layout == ESettingsLayout::Old)
                m_plater->get_current_canvas3D()->render();
        }
        // add by allen for ankerCfgDlg
        if (wxGetApp().tabs_as_menu()) {
            if (AnkerTab* cur_tab = dynamic_cast<AnkerTab*>(m_ankerCfgDlg->m_rightPanel->GetPage(new_selection)))
                update_marker_for_tabs_menu((m_layout == ESettingsLayout::Old ? m_menubar : m_settings_dialog.menubar()), cur_tab->title(), m_layout == ESettingsLayout::Old);
            else if (tab == 0 && m_layout == ESettingsLayout::Old)
                m_plater->get_current_canvas3D()->render();
        }
#endif
        if (tab == 0 && m_layout == ESettingsLayout::Old)
            m_plater->canvas3D()->render();
        else if (was_hidden) {
            Tab* cur_tab = dynamic_cast<Tab*>(m_tabpanel->GetPage(new_selection));
            if (cur_tab)
                cur_tab->OnActivate();
            // add by allen for ankerCfgDlg
            AnkerTab* curTab = dynamic_cast<AnkerTab*>(m_ankerCfgDlg->m_rightPanel->GetPage(new_selection));
            if (curTab)
                curTab->OnActivate();
        }
        // add by allen for ankerCfgDlg
        AnkerTab* curTab = dynamic_cast<AnkerTab*>(m_ankerCfgDlg->m_rightPanel->GetPage(new_selection));
        if (curTab) {
           m_ankerCfgDlg->resetPresetComBoxHighlight();
           m_ankerCfgDlg->updateAnkerTabComBoxHighlight(curTab->type());
        }
    };

    if (m_layout == ESettingsLayout::Dlg) {
        if (tab==0) {
            if (m_settings_dialog.IsShown())
                this->SetFocus();
            // plater should be focused for correct navigation inside search window
            if (m_plater->canvas3D()->is_search_pressed())
                m_plater->SetFocus();
            return;
        }
        // Show/Activate Settings Dialog
#ifdef __WXOSX__ // Don't call SetFont under OSX to avoid name cutting in ObjectList
        if (m_settings_dialog.IsShown())
            m_settings_dialog.Hide();
        else
            tabpanel_was_hidden = true;
            
        select(tabpanel_was_hidden);
        m_tabpanel->Show();
        // add by allen for ankerCfgDlg to hide old config setting dialog
#if SHOW_OLD_SETTING_DIALOG
        m_settings_dialog.Show();
#endif //SHOW_OLD_SETTING_DIALOG
#else
        if (m_settings_dialog.IsShown()) {
            select(false);
            m_settings_dialog.SetFocus();
        }
        else {
            tabpanel_was_hidden = true;
            select(tabpanel_was_hidden);
            m_tabpanel->Show();
            // add by allen for ankerCfgDlg to hide old config setting dialog
#if SHOW_OLD_SETTING_DIALOG
            m_settings_dialog.Show();
#endif //SHOW_OLD_SETTING_DIALOG
        }
#endif
        if (m_settings_dialog.IsIconized())
            m_settings_dialog.Iconize(false);
    }
    else if (m_layout == ESettingsLayout::New) {
        m_main_sizer->Show(m_plater, tab == 0);
        tabpanel_was_hidden = !m_main_sizer->IsShown(m_tabpanel);
        select(tabpanel_was_hidden);
        m_main_sizer->Show(m_tabpanel, tab != 0);

        // plater should be focused for correct navigation inside search window
        if (tab == 0)
            m_plater->SetFocus();
        Layout();
    }
    else {
        select(false);
#ifdef _MSW_DARK_MODE
        if (wxGetApp().tabs_as_menu() && tab == 0)
            m_plater->SetFocus();
#endif
    }

    // When we run application in ESettingsLayout::New or ESettingsLayout::Dlg mode, tabpanel is hidden from the very beginning
    // and as a result Tab::update_changed_tree_ui() function couldn't update m_is_nonsys_values values,
    // which are used for update TreeCtrl and "revert_buttons".
    // So, force the call of this function for Tabs, if tab panel was hidden
    if (tabpanel_was_hidden) {
#if SHOW_OLD_SETTING_DIALOG
        for (auto cur_tab : wxGetApp().tabs_list)
            cur_tab->update_changed_tree_ui();
#endif
        for (auto cur_tab : wxGetApp().ankerTabsList)
            cur_tab->update_changed_tree_ui();
    }
        

    //// when tab == -1, it means we should show the last selected tab
    //size_t new_selection = tab == (size_t)(-1) ? m_last_selected_tab : (m_layout == ESettingsLayout::Dlg && tab != 0) ? tab - 1 : tab;
    //if (m_tabpanel->GetSelection() != new_selection)
    //    m_tabpanel->SetSelection(new_selection);
    //if (tabpanel_was_hidden)
    //    static_cast<Tab*>(m_tabpanel->GetPage(new_selection))->OnActivate();
}

// Set a camera direction, zoom to all objects.
void MainFrame::select_view(const std::string& direction)
{
     if (m_plater)
         m_plater->select_view(direction);
}

// #ys_FIXME_to_delete
void MainFrame::on_presets_changed(SimpleEvent &event)
{
    auto *tab = dynamic_cast<Tab*>(event.GetEventObject());
    wxASSERT(tab != nullptr);
    if (tab == nullptr) {
        return;
    }

    // Update preset combo boxes(Print settings, Filament, Material, Printer) from their respective tabs.
    auto presets = tab->get_presets();
    if (m_plater != nullptr && presets != nullptr) {

        // FIXME: The preset type really should be a property of Tab instead
        Slic3r::Preset::Type preset_type = tab->type();
        if (preset_type == Slic3r::Preset::TYPE_INVALID) {
            wxASSERT(false);
            return;
        }

        m_plater->on_config_change(*tab->get_config());
       // m_plater->sidebar().update_presets(preset_type);
    }
}

// #ys_FIXME_to_delete
void MainFrame::on_value_changed(wxCommandEvent& event)
{
    auto *tab = dynamic_cast<Tab*>(event.GetEventObject());
    wxASSERT(tab != nullptr);
    if (tab == nullptr)
        return;

    auto opt_key = event.GetString();
    if (m_plater) {
        m_plater->on_config_change(*tab->get_config()); // propagate config change events to the plater
        if (opt_key == "extruders_count") {
            auto value = event.GetInt();
            m_plater->on_extruders_change(value);
        }
    }
}

void MainFrame::on_size(wxSizeEvent& event)
{
    if (m_plater)
        m_plater->on_size(event);

    event.Skip();
}

void MainFrame::on_move(wxMoveEvent& event)
{
    if (m_plater)
        m_plater->on_move(event);

    event.Skip();
}

void MainFrame::on_show(wxShowEvent& event)
{
    if (m_plater)
        m_plater->on_show(event);

    if (m_pDeviceWidget)
        m_pDeviceWidget->activate(event.IsShown());

    if (m_MsgCentreDialog)
    {
        if (m_isMsgCenterIsShow)
        {
            m_MsgCentreDialog->Raise();
            m_MsgCentreDialog->Show();
        }
    }

    event.Skip();
}

void MainFrame::on_minimize(wxIconizeEvent& event)
{
    if (m_plater)
        m_plater->on_minimize(event);

    if (m_pDeviceWidget)
        m_pDeviceWidget->activate(false);

    if (m_pMsgCentrePopWindow)
        m_MsgCentreDialog->Hide();
    event.Skip();
}

void MainFrame::on_Activate(wxActivateEvent& event)
{
    if (m_MsgCentreDialog)
    {
        if (m_isMsgCenterIsShow)
        {
            if (m_MsgCentreDialog->IsShown())
                return;
            m_MsgCentreDialog->Raise();
            m_MsgCentreDialog->Show();
        }
    }
    event.Skip();
}

void MainFrame::on_maximize(wxMaximizeEvent& event)
{
    if (m_plater)
        m_plater->on_maximize(event);

    if (m_pDeviceWidget)
        m_pDeviceWidget->activate(true);

    event.Skip();

    if (m_MsgCentreDialog)
    {
        if (m_isMsgCenterIsShow)
        {
            m_MsgCentreDialog->Raise();
            m_MsgCentreDialog->Show();
        }
    }
}


void MainFrame::on_config_changed(DynamicPrintConfig* config) const
{
    if (m_plater)
        m_plater->on_config_change(*config); // propagate config change events to the plater
}

void MainFrame::add_to_recent_projects(const wxString& filename)
{
    if (wxFileExists(filename))
    {
        m_recent_projects.AddFileToHistory(filename);
        std::vector<std::string> recent_projects;
        size_t count = m_recent_projects.GetCount();
        for (size_t i = 0; i < count; ++i)
        {
            recent_projects.push_back(into_u8(m_recent_projects.GetHistoryFile(i)));
        }
        wxGetApp().app_config->set_recent_projects(recent_projects);
    }
}
void MainFrame::clearStarCommentData()
{
    m_showCommentWebView = false;
    
    g_sliceCommentData.reviewNameID = "";
    g_sliceCommentData.reviewName = "";
    g_sliceCommentData.appVersion = "";
    g_sliceCommentData.country = "";

    g_sliceCommentData.sliceCount = "";

    g_sliceCommentData.action = 2;
    g_sliceCommentData.rating = 0;
    g_sliceCommentData.reviewData = "";
    g_sliceCommentData.clientId = "";
}
void MainFrame::technology_changed()
{
    // update menu titles
    PrinterTechnology pt = plater()->printer_technology();
    if (int id = m_menubar->FindMenu(pt == ptFFF ? _L("Material Settings") : _L("Filament Settings")); id != wxNOT_FOUND)
        m_menubar->SetMenuLabel(id , pt == ptSLA ? _L("Material Settings") : _L("Filament Settings"));

    //if (wxGetApp().tab_panel()->GetSelection() != wxGetApp().tab_panel()->GetPageCount() - 1)
    //    wxGetApp().tab_panel()->SetSelection(wxGetApp().tab_panel()->GetPageCount() - 1);

}

//
// Called after the Preferences dialog is closed and the program settings are saved.
// Update the UI based on the current preferences.
void MainFrame::update_ui_from_settings()
{
//    const bool bp_on = wxGetApp().app_config->get_bool("background_processing");
//     m_menu_item_reslice_now->Enable(!bp_on);
//    m_plater->sidebar().show_reslice(!bp_on);
//    m_plater->sidebar().show_export(bp_on);
//    m_plater->sidebar().Layout();

    if (m_plater)
        m_plater->update_ui_from_settings();
#if SHOW_OLD_SETTING_DIALOG
    for (auto tab: wxGetApp().tabs_list)
        tab->update_ui_from_settings();
#endif
    // add by allen for ankerCfgDlg
    for (auto tab : wxGetApp().ankerTabsList)
        tab->update_ui_from_settings();
}

std::string MainFrame::get_base_name(const wxString &full_name, const char *extension) const 
{
    boost::filesystem::path filename = boost::filesystem::path(full_name.wx_str()).filename();
    if (extension != nullptr)
		filename = filename.replace_extension(extension);
    return filename.string();
}

std::string MainFrame::get_dir_name(const wxString &full_name) const 
{
    return boost::filesystem::path(full_name.wx_str()).parent_path().string();
}

// add by allen for ankerCfgDlg
AnkerTabPresetComboBox* MainFrame::GetAnkerTabPresetCombo(const Preset::Type type) {
    return m_ankerCfgDlg ? m_ankerCfgDlg->GetAnkerTabPresetCombo(type) : nullptr;
}

void MainFrame::updateMsgCenterItemContent(std::vector<MsgCenterItem>* pData)
{
    if (!pData || pData->size() <= 0)
        return;

    //std::string currentLanguage = GetTranslateLanguage();
    std::string currentLanguage = "";
    int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
    if (type == wxLanguage::wxLANGUAGE_JAPANESE)
    {
        currentLanguage = "ja";
    }
    else
    {
        currentLanguage = "en";//wxLanguage::wxLANGUAGE_ENGLISH
    }

    if (!m_MsgCenterCfg)
    {
        m_MsgCenterCfg = new std::map<std::string, MsgCenterConfig>();
        if (!loadMsgCenterCfg())
        {
            //load local cfg msgCenterCfgVersionInfo
            ANKER_LOG_INFO << "no any msg center config fail ";
            return;
        }
    }       

    if (!m_MsgCenterErrCodeInfo)
    {
        m_MsgCenterErrCodeInfo = new std::vector<MsgErrCodeInfo>();
        if (!loadMsgCenterMultiLanguageCfg())
        {
            //load local cfg msgCenterMultiLanguageCfg
            ANKER_LOG_INFO << "no any msg center updateMsgCenterItemContent fail ";
            return;
        }
    }

    for (auto item = pData->begin(); item != pData->end(); ++item)
    {
       std::string errorCode = (*item).msgErrorCode;
       std::string realErrorCode = "fdm_news_center_" + (*item).msgErrorCode + "_desc";
       auto urlItem = m_MsgCenterCfg->find(errorCode);
       if (urlItem != m_MsgCenterCfg->end())
       {
           auto articleList = (*urlItem).second.article_info;
           for (auto articleListItem : articleList)
           {
               if (articleListItem.language == currentLanguage)
               {
                   (*item).msgUrl = articleListItem.article_url;//find url
                   (*item).msgLevel = (*urlItem).second.error_level;//find url
                   break;
               }
           }
       }

       for (auto it = m_MsgCenterErrCodeInfo->begin(); it != m_MsgCenterErrCodeInfo->end(); ++it) {
           if ((*it).language == currentLanguage)
           {
               auto ErrCodeUrlMap = (*it).errorCodeUrlMap;
               auto resItem = ErrCodeUrlMap.find(realErrorCode);
               if (resItem != ErrCodeUrlMap.end())
               {
                   //wxString::FromUTF8((*resItem).second).ToUTF8().data();
                   //(*item).msgContent = (*resItem).second;//find error content
                   (*item).msgContent = wxString::FromUTF8((*resItem).second).data();//find error content
               }
           }
       }       
    }
}

AnkerTab* MainFrame::openAnkerTabByPresetType(const Preset::Type type)
{
#if SHOW_OLD_SETTING_DIALOG
    Slic3r::GUI::Tab* tab = Slic3r::GUI::wxGetApp().get_tab(type);
    if (!tab)
        return nullptr;

    if (int page_id = Slic3r::GUI::wxGetApp().ankerTabPanel()->FindPage(tab); page_id != wxNOT_FOUND)
    {
        Slic3r::GUI::wxGetApp().ankerTabPanel()->SetSelection(page_id);
        // Switch to Settings NotePad
        Slic3r::GUI::wxGetApp().mainframe->select_tab();
    }
#endif
    // add by allen for ankerCfgDlg
    Slic3r::GUI::AnkerTab* ankerTab = Slic3r::GUI::wxGetApp().getAnkerTab(type);
    if (!ankerTab)
        return nullptr;

    if (int page_id = Slic3r::GUI::wxGetApp().ankerTabPanel()->FindPage(ankerTab); page_id != wxNOT_FOUND)
    {
        Slic3r::GUI::wxGetApp().ankerTabPanel()->SetSelection(page_id);
        // Switch to Settings NotePad
        Slic3r::GUI::wxGetApp().mainframe->select_tab();
        // show AnkerConfigDilog
        wxGetApp().mainframe->showAnkerCfgDlg();
    }

    return ankerTab;
}

// ----------------------------------------------------------------------------
// SettingsDialog
// ----------------------------------------------------------------------------

SettingsDialog::SettingsDialog(MainFrame* mainframe)
:DPIFrame(NULL, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _L("Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, "settings_dialog", mainframe->normal_font().GetPointSize()),
//: DPIDialog(mainframe, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _L("Settings"), wxDefaultPosition, wxDefaultSize,
//        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX, "settings_dialog"),
    m_main_frame(mainframe)
{
    if (wxGetApp().is_gcode_viewer())
        return;

#if defined(__WXMSW__)
    // ys_FIXME! temporary workaround for correct font scaling
    // Because of from wxWidgets 3.1.3 auto rescaling is implemented for the Fonts,
    // From the very beginning set dialog font to the wxSYS_DEFAULT_GUI_FONT
    this->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
#else
    this->SetFont(wxGetApp().normal_font());
    this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif // __WXMSW__

    // Load the icon either from the exe, or from the ico file.
#if _WIN32
    {
        TCHAR szExeFileName[MAX_PATH];
        GetModuleFileName(nullptr, szExeFileName, MAX_PATH);
        SetIcon(wxIcon(szExeFileName, wxBITMAP_TYPE_ICO));
    }
#else
    SetIcon(wxIcon(var("AnkerStudio_128px.png"), wxBITMAP_TYPE_PNG));
#endif // _WIN32

    this->Bind(wxEVT_SHOW, [this](wxShowEvent& evt) {

        auto key_up_handker = [this](wxKeyEvent& evt) {
            if ((evt.GetModifiers() & wxMOD_CONTROL) != 0) {
                switch (evt.GetKeyCode()) {
                case '1': { m_main_frame->select_tab(size_t(0)); break; }
                case '2': { m_main_frame->select_tab(1); break; }
                case '3': { m_main_frame->select_tab(2); break; }
                case '4': { m_main_frame->select_tab(3); break; }
#ifdef __APPLE__
                case 'f':
#else /* __APPLE__ */
                case WXK_CONTROL_F:
#endif /* __APPLE__ */
                case 'F': { m_main_frame->plater()->search(false); break; }
                default:break;
                }
            }
        };

        if (evt.IsShown()) {
            if (m_tabpanel != nullptr)
                m_tabpanel->Bind(wxEVT_KEY_UP, key_up_handker);
        }
        else {
            if (m_tabpanel != nullptr)
                m_tabpanel->Unbind(wxEVT_KEY_UP, key_up_handker);
        }
        });

    //just hide the Frame on closing
    this->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) { this->Hide(); });

#ifdef _MSW_DARK_MODE
    if (wxGetApp().tabs_as_menu()) {
        // menubar
        m_menubar = new wxMenuBar();
        add_tabs_as_menu(m_menubar, mainframe, this);
        this->SetMenuBar(m_menubar);
    }
#endif

    // initialize layout
    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetSizeHints(this);
    SetSizer(sizer);
    Fit();

    const wxSize min_size = wxSize(85 * em_unit(), 50 * em_unit());
#ifdef __APPLE__
    // Using SetMinSize() on Mac messes up the window position in some cases
    // cf. https://groups.google.com/forum/#!topic/wx-users/yUKPBBfXWO0
    SetSize(min_size);
#else
    SetMinSize(min_size);
    SetSize(GetMinSize());
#endif
    Layout();
}

void SettingsDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    if (wxGetApp().is_gcode_viewer())
        return;

    const int& em = em_unit();
    const wxSize& size = wxSize(85 * em, 50 * em);

#ifdef _MSW_DARK_MODE
    // update common mode sizer
    /*if (!wxGetApp().tabs_as_menu())
        dynamic_cast<Notebook*>(m_tabpanel)->Rescale();*/
#endif

    // update Tabs
#if SHOW_OLD_SETTING_DIALOG
    for (auto tab : wxGetApp().tabs_list)
        tab->msw_rescale();
#endif
    // add by allen for ankerCfgDlg
    for (auto tab : wxGetApp().ankerTabsList)
        tab->msw_rescale();

    SetMinSize(size);
    Fit();
    Refresh();
}


} // GUI
} // Slic3r

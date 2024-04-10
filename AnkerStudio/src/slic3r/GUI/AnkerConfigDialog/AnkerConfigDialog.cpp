#include "AnkerConfigDialog.hpp"
#include <wx/simplebook.h>

#include "../GUI_App.hpp"
#include "../I18N.hpp"
#include "../wxExtensions.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "../Notebook.hpp"
#include "../MainFrame.hpp"
#include "../common/AnkerGUIConfig.hpp"
#include "../Plater.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_UPDATE_PARAMETERS_PANEL, wxCommandEvent);

namespace Slic3r {
    namespace GUI {
#define ANKER_CONFIG_DIALOG_TITLE "common_menu_settings_preset" //"Parameter Settings"

#define ANKER_CONFIG_DIALOG_WIDTH  950
#define ANKER_CONFIG_DIALOG_HEIGHT 660

#define ANKER_CONFIG_DIALOG_TITLE_PANEL_HEIGHT 0 // 40
#define ANKER_CONFIG_DIALOG_CONTENT_HEIGHT (ANKER_CONFIG_DIALOG_HEIGHT - ANKER_CONFIG_DIALOG_TITLE_PANEL_HEIGHT)
#define ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH 250
#define ANKER_CONFIG_DIALOG_RIGHT_PANEL_WIDTH (ANKER_CONFIG_DIALOG_WIDTH - ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH - 1)
#define ANKER_CONFIG_DIALOG_COMBO_TITLE_SPAN   30
#define ANKER_CONFIG_DIALOG_COMBOBOX_SPAN   5
#define ANKER_CONFIG_DIALOG_TITLE_LEFT_PANLE_SPAN  8
#define ANKER_CONFIG_DIALOG_COMBOBOX_LEFT_PANLE_SPAN  14
#define ANKER_CONFIG_DIALOG_COMBO_WIDTH (ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH - ANKER_CONFIG_DIALOG_COMBOBOX_LEFT_PANLE_SPAN * 2)

        AnkerConfigDlg::AnkerConfigDlg(wxWindow* parent,
            wxWindowID id,
            const wxString& title,
            const wxString& url,
            const wxPoint& pos /*= wxDefaultPosition*/,
            const wxSize& size /*= wxDefaultSize*/,
            long style /*= wxDEFAULT_DIALOG_STYLE*/,
            const wxString& name /*= wxDialogNameStr*/) :
            AnkerDPIDialog(parent, id, title, pos, size, style, name),
            m_parent(parent){
            // init ui
            initUI();
            // init event bind Modify it to a modal dialog box according to product requirements.
            initEventBind();
        }

        AnkerConfigDlg::~AnkerConfigDlg() {

        }

        void AnkerConfigDlg::AddCreateTab(AnkerTab* panel, const std::string& bmp_name) {
            panel->create_preset_tab();

            const auto printer_tech = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology();
            if (panel->supports_printer_technology(printer_tech)) {
#ifdef _MSW_DARK_MODE
                if (!wxGetApp().tabs_as_menu())
#if _HIDE_CONTROL_FOR_ANKERCFGDLG_
                    dynamic_cast<Notebook*>(m_rightPanel)->AddPage(panel, panel->title()/*""*/, /*bmp_name*/"");
#else
                    dynamic_cast<Notebook*>(m_rightPanel)->AddPage(panel, panel->title(), bmp_name);
#endif

                else
#endif
                    m_rightPanel->AddPage(panel, /*""*/panel->title());
            }
            m_rightPanel->SetSelection(0);
            panel->OnActivate();
#ifdef __APPLE__
            Layout();
#endif
        }

        AnkerTabPresetComboBox* AnkerConfigDlg::GetAnkerTabPresetCombo(const Preset::Type type) {
            switch (type) {
            case Preset::TYPE_PRINTER:
                return m_printerPresetsChoice;
                break;
            case Preset::TYPE_FILAMENT:
                return m_filamentPresetsChoice;
                break;
            case Preset::TYPE_PRINT:
                return m_printPresetsChoice;
                break;
            default:
                return nullptr;
                break;
            }
        }

        void AnkerConfigDlg::initUI() {
            // backgroud colour
            SetBackgroundColour(wxColour("#333438"));

            // set size and pos
            SetSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_HEIGHT));
            SetMinSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_HEIGHT));
            CenterOnParent();

            // set dialog icon
            SetIcon(mainFrameIcon());

            // sety dialog title
            SetTitle(_L(ANKER_CONFIG_DIALOG_TITLE));

            // main sizer
            {
                m_mainSizer = new wxBoxSizer(wxVERTICAL);
                SetSizer(m_mainSizer);
            }

            // titleHSizer
            //{
            //    m_titlePanel = new wxPanel(this);
            //    m_titlePanel->SetSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_TITLE_PANEL_HEIGHT));
            //    m_mainSizer->Add(m_titlePanel, 0, wxEXPAND | wxALL, 0);

            //    wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
            //    titleHSizer->SetMinSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_TITLE_PANEL_HEIGHT));
            //    m_titlePanel->SetSizer(titleHSizer);

            //    //titleHSizer->AddSpacer(33.9 * wxGetApp().em_unit());
            //    titleHSizer->AddStretchSpacer(1);
            //    // title text
            //    m_titleText = new wxStaticText(m_titlePanel, wxID_ANY, _L(ANKER_CONFIG_DIALOG_TITLE));
            //    m_titleText->SetBackgroundColour(wxColour("#333438"));
            //    m_titleText->SetForegroundColour(wxColour("#FFFFFF"));
            //    m_titleText->SetFont(ANKER_BOLD_FONT_NO_1);
            //    m_titleText->SetSize(wxSize(150, -1));
            //    titleHSizer->Add(m_titleText, 0, wxALIGN_CENTER_VERTICAL, 0);

            //    //titleHSizer->AddSpacer(30.7 * wxGetApp().em_unit());
            //    titleHSizer->AddStretchSpacer(1);
            //    // exit button
            //    m_exitBtn = new ScalableButton(m_titlePanel, wxID_ANY, "ankerConfigDialogExit", "", wxSize(20, 20));
            //    m_exitBtn->SetWindowStyleFlag(wxBORDER_NONE);
            //    m_exitBtn->SetBackgroundColour(wxColour("#333438"));
            //    m_exitBtn->SetForegroundColour(wxColour("#FFFFFF"));
            //    m_exitBtn->Bind(wxEVT_BUTTON, &AnkerConfigDlg::OnExitButtonClicked, this);
            //    m_exitBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
            //        SetCursor(wxCursor(wxCURSOR_HAND));
            //        event.Skip();
            //        });
            //    /*   m_exitBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
            //           SetCursor(wxCursor(wxCURSOR_NONE));
            //           event.Skip();
            //           });*/
            //    titleHSizer->Add(m_exitBtn, 0, /*wxEXPAND | */wxRIGHT | wxTop | wxALIGN_CENTER_VERTICAL, 10);

            //    // split line
            //    wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            //    splitLineCtrl->SetBackgroundColour(wxColour("#484A51"));
            //    splitLineCtrl->SetMaxSize(wxSize(100000, 1));
            //    splitLineCtrl->SetMinSize(wxSize(1, 1));
            //    m_mainSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 0);
            //}

            // content sizer
            {
                /*m_contentSizer = new wxBoxSizer(wxHORIZONTAL);*/

                // left panel sizer
                {
                    //m_leftPanelSizer = new wxBoxSizer(wxVERTICAL);
                    //m_leftPanelSizer->SetMinSize(wxSize(ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT));
                    //m_contentSizer->Add(m_leftPanelSizer, 0, wxEXPAND | wxALL, 0);

                    //m_leftPanel = new wxPanel(this);
					/* m_leftPanel->SetSize(wxSize(ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT));
					 m_leftPanelSizer->Add(m_leftPanel, 0, wxEXPAND | wxALL, 0);*/

                    //// left main sizer
                    //auto leftMainSizer = new wxBoxSizer(wxVERTICAL);
                    //m_leftPanel->SetSizer(leftMainSizer);

                    //// left preset panel
                    //m_leftPresetPanel = new wxPanel(m_leftPanel);
                   /* m_leftPresetPanel->SetSize(wxSize(ANKER_CONFIG_DIALOG_LFEF_PANEL_WIDTH, -1));*/

                    //// left preset sizer 
                    //leftMainSizer->Add(m_leftPresetPanel, 0, wxTop | wxEXPAND, 8);
                    //auto leftPresetSizer = new wxBoxSizer(wxVERTICAL);
                    //m_leftPresetPanel->SetSizer(leftPresetSizer);

                    //leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBO_TITLE_SPAN / 3));
                    //// printer preset
                    {
                        // printer preset title
						/*auto printerTitle = new wxStaticText(m_leftPresetPanel, wxID_ANY, _L("common_slicepannel_title_printer"));
						printerTitle->SetBackgroundColour(wxColour("#333438"));
						printerTitle->SetForegroundColour(wxColour("#FFFFFF"));
						printerTitle->SetFont(ANKER_BOLD_FONT_NO_1);
						printerTitle->SetMinSize(AnkerSize(-1, 20));
						leftPresetSizer->Add(printerTitle, 0, wxLEFT | wxRIGHT | wxEXPAND, ANKER_CONFIG_DIALOG_TITLE_LEFT_PANLE_SPAN);

						leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBOBOX_SPAN));*/

                        // printer preset combox
                        //m_printerPresetsChoice = new AnkerTabPresetComboBox(m_leftPresetPanel, Preset::TYPE_PRINTER);
                        //m_printerPresetsChoice->Create(m_leftPresetPanel,
                        //    wxID_ANY,
                        //    wxEmptyString,
                        //    wxDefaultPosition,
                        //    wxSize(ANKER_CONFIG_DIALOG_COMBO_WIDTH, ANKER_COMBOBOX_HEIGHT),
                        //    wxNO_BORDER | wxCB_READONLY,
                        //    wxDefaultValidator,
                        //    "");
                        //m_printerPresetsChoice->SetFont(ANKER_FONT_NO_1);
                        //m_printerPresetsChoice->SetBackgroundColour(wxColour("#333438"));
                        //m_printerPresetsChoice->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
                        //wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
                        //btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
                        //wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //m_printerPresetsChoice->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
                        //// add by allen for ankerCfgDlg preset combobox focus highlight
                        //m_printerPresetsChoice->set_button_clicked_function([this]() {
                        //    ANKER_LOG_INFO << "preset combox of printer clicked";
                        //    onComboBoxClick(m_printerPresetsChoice);
                        //    });
                        //m_printerPresetsChoice->Bind(wxEVT_COMBOBOX_CLOSEUP, &AnkerConfigDlg::onComboBoxCloseUp, this);

                        //m_printerPresetsChoice->set_selection_changed_function([this](int selection) {
                        //    ChangeAnkerTabComboSel(m_printerPresetsChoice, selection);
                        //    });
                        //m_printerPresetsChoice->Bind(wxEVT_RIGHT_DOWN, &AnkerConfigDlg::onPresetRightClick, this);
                        ////leftPresetSizer->Add(m_printerPresetsChoice, 0, wxLEFT | wxRIGHT | wxEXPAND, ANKER_CONFIG_DIALOG_COMBOBOX_LEFT_PANLE_SPAN);
                        //m_printerPresetsChoice->update();
                    }

                    //leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBO_TITLE_SPAN));
                    //// Filament preset
                    {
                        // Filament title
                       /* auto filamentTitle = new wxStaticText(m_leftPresetPanel, wxID_ANY, _L("common_slicepannel_title_filament"));
                        filamentTitle->SetBackgroundColour(wxColour("#333438"));
                        filamentTitle->SetForegroundColour(wxColour("#FFFFFF"));
                        filamentTitle->SetFont(ANKER_BOLD_FONT_NO_1);
                        filamentTitle->SetMinSize(AnkerSize(-1, 20));
                        leftPresetSizer->Add(filamentTitle, 0, wxLEFT | wxEXPAND, ANKER_CONFIG_DIALOG_TITLE_LEFT_PANLE_SPAN);

                        leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBOBOX_SPAN));*/
                        // Filament preset combox
                        //m_filamentPresetsChoice = new AnkerTabPresetComboBox(m_leftPresetPanel, Preset::TYPE_FILAMENT);
                        //m_filamentPresetsChoice->Show(false);
                        //m_filamentPresetsChoice->Create(m_leftPresetPanel,
                        //    wxID_ANY,
                        //    wxEmptyString,
                        //    wxDefaultPosition,
                        //    wxSize(ANKER_CONFIG_DIALOG_COMBO_WIDTH, ANKER_COMBOBOX_HEIGHT),
                        //    wxNO_BORDER | wxCB_READONLY,
                        //    wxDefaultValidator,
                        //    "");
                        //m_filamentPresetsChoice->SetFont(ANKER_FONT_NO_1);
                        //m_filamentPresetsChoice->SetBackgroundColour(wxColour("#333438"));
                        //m_filamentPresetsChoice->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
                        //wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
                        //btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
                        //wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //m_filamentPresetsChoice->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
                        //// add by allen for ankerCfgDlg preset combobox focus highlight
                        //m_filamentPresetsChoice->set_button_clicked_function([this]() {
                        //    ANKER_LOG_INFO << "preset combox of filament clicked";
                        //    onComboBoxClick(m_filamentPresetsChoice);
                        //    });
                        //m_filamentPresetsChoice->Bind(wxEVT_COMBOBOX_CLOSEUP, &AnkerConfigDlg::onComboBoxCloseUp, this);

                        //m_filamentPresetsChoice->set_selection_changed_function([this](int selection) {
                        //    ChangeAnkerTabComboSel(m_filamentPresetsChoice, selection);
                        //    // mod by allen for Change the interaction for switching print and filament presets.
                        //    CallAfter([=] {
                        //        int selection = m_printPresetsChoice->GetSelection();
                        //        ChangeAnkerTabComboSel(m_printPresetsChoice, selection);
                        //        });
                        //    });

                        //m_filamentPresetsChoice->Bind(wxEVT_RIGHT_DOWN, &AnkerConfigDlg::onPresetRightClick, this);
                        ////leftPresetSizer->Add(m_filamentPresetsChoice, 0, wxLEFT | wxRIGHT | wxEXPAND, ANKER_CONFIG_DIALOG_COMBOBOX_LEFT_PANLE_SPAN);
                        //m_filamentPresetsChoice->update();
                    }

                    //leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBO_TITLE_SPAN));
                    // print preset
                    {
                        // print title
                       /* auto printTitle = new wxStaticText(m_leftPresetPanel, wxID_ANY, _L("common_slicepannel_printsetting_title"));
                        printTitle->SetBackgroundColour(wxColour("#333438"));
                        printTitle->SetForegroundColour(wxColour("#FFFFFF"));
                        printTitle->SetFont(ANKER_BOLD_FONT_NO_1);
                        printTitle->SetMinSize(AnkerSize(-1, 20));
                        leftPresetSizer->Add(printTitle, 0, wxLEFT | wxEXPAND, ANKER_CONFIG_DIALOG_TITLE_LEFT_PANLE_SPAN);

                        leftPresetSizer->AddSpacer((ANKER_CONFIG_DIALOG_COMBOBOX_SPAN));*/
                        // print preset combox
                        //m_printPresetsChoice = new AnkerTabPresetComboBox(m_leftPresetPanel, Preset::TYPE_PRINT);
                        //m_printPresetsChoice->Show(false);
                        //m_printPresetsChoice->Create(m_leftPresetPanel,
                        //    wxID_ANY,
                        //    wxEmptyString,
                        //    wxDefaultPosition,
                        //    wxSize(ANKER_CONFIG_DIALOG_COMBO_WIDTH, ANKER_COMBOBOX_HEIGHT),
                        //    wxNO_BORDER | wxCB_READONLY,
                        //    wxDefaultValidator,
                        //    "");
                        //m_printPresetsChoice->SetFont(ANKER_FONT_NO_1);
                        //m_printPresetsChoice->SetBackgroundColour(wxColour("#333438"));
                        //// add by allen for ankerCfgDlg default preset combobox focus highlight
                        //m_printPresetsChoice->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
                        ////m_printPresetsChoice->setColor(wxColour("#506853"), wxColour("#506853"), wxColour("#62D361"));
                        //m_lastSelectPreset = m_printPresetsChoice;
                        //wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
                        //btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
                        //wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
                        //m_printPresetsChoice->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
                        //// add by allen for ankerCfgDlg preset combobox focus highlight
                        //m_printPresetsChoice->set_button_clicked_function([this]() {
                        //    ANKER_LOG_INFO << "preset combox of print clicked";
                        //    onComboBoxClick(m_printPresetsChoice);
                        //    });
                        //m_printPresetsChoice->Bind(wxEVT_COMBOBOX_CLOSEUP, &AnkerConfigDlg::onComboBoxCloseUp, this);

                        //m_printPresetsChoice->set_selection_changed_function([this](int selection) {
                        //    ChangeAnkerTabComboSel(m_printPresetsChoice, selection);
                        //    });

                        //m_printPresetsChoice->Bind(wxEVT_RIGHT_DOWN, &AnkerConfigDlg::onPresetRightClick, this);
                        ////leftPresetSizer->Add(m_printPresetsChoice, 0, wxLEFT | wxRIGHT | wxEXPAND, ANKER_CONFIG_DIALOG_COMBOBOX_LEFT_PANLE_SPAN);
                        //m_printPresetsChoice->update();
                    }

                    //// split line
                    //wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
                    //splitLineCtrl->SetBackgroundColour(wxColour("#484A51"));
                    //splitLineCtrl->SetMaxSize(wxSize(1, 10000));
                    //splitLineCtrl->SetMinSize(wxSize(1, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT));
                    //m_contentSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALL, 0);

                    // right panel sizer
                    {
                        m_rightPanelSizer = new wxBoxSizer(wxVERTICAL);
                        m_rightPanelSizer->SetMinSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT));
                        //m_contentSizer->Add(m_rightPanelSizer, 1, wxEXPAND | wxALL, 0);

#ifdef _MSW_DARK_MODE
                        if (wxGetApp().tabs_as_menu()) {
                            m_rightPanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT),
                                wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
                            wxGetApp().UpdateDarkUI(m_rightPanel);
                        }
                        else {
#if _HIDE_CONTROL_FOR_ANKERCFGDLG_
                            m_rightPanel = new Notebook(this, wxID_ANY, wxDefaultPosition, wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT),
                                wxNB_LEFT | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
#else
                            m_rightPanel = new Notebook(this, wxID_ANY, wxDefaultPosition, wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT),
                                wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
#endif
                        }
#else
                        // modify by dhf to disable showing page tab
                        m_rightPanel = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);
#endif
                        m_rightPanel->SetMinSize(wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT));
#ifdef __WXMSW__
                        m_rightPanel->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
#else
                        m_rightPanel->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
#endif
                            if (int old_selection = e.GetOldSelection();
                                old_selection != wxNOT_FOUND && old_selection < static_cast<int>(m_rightPanel->GetPageCount())) {
                                AnkerTab* old_tab = dynamic_cast<AnkerTab*>(m_rightPanel->GetPage(old_selection));
                                if (old_tab)
                                    old_tab->validate_custom_gcodes();
                            }

                            wxWindow* panel = m_rightPanel->GetCurrentPage();
                            AnkerTab* tab = dynamic_cast<AnkerTab*>(panel);
                            if (tab) {
                                tab->force_update();
                                SetTitle(tab->title());
                            }
                                

                            // There shouldn't be a case, when we try to select a tab, which doesn't support a printer technology
                            const auto printer_tech = wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology();
                            if (panel == nullptr || (tab != nullptr && !tab->supports_printer_technology(printer_tech)))
                                return;
                            auto& tabs_list = wxGetApp().ankerTabsList;
                            if (tab && std::find(tabs_list.begin(), tabs_list.end(), tab) != tabs_list.end()) {
                                // On GTK, the wxEVT_NOTEBOOK_PAGE_CHANGED event is triggered
                                // before the MainFrame is fully set up.
                                tab->OnActivate();
                                m_rightPanel->GetSelection();
#ifdef _MSW_DARK_MODE
                                if (wxGetApp().tabs_as_menu())
                                    tab->SetFocus();
#endif
                            }

                            m_rightPanel->GetSelection();
                            });

                        m_rightPanelSizer->Add(m_rightPanel, 1, wxEXPAND | wxALL, 0);
                        m_rightPanelSize = wxSize(ANKER_CONFIG_DIALOG_WIDTH, ANKER_CONFIG_DIALOG_CONTENT_HEIGHT);
                    }

                }
                m_mainSizer->Add(m_rightPanelSizer, 1, wxEXPAND | wxALL, 0);
            }
        }

        wxIcon AnkerConfigDlg::mainFrameIcon() {
#if _WIN32
            std::wstring path(size_t(MAX_PATH), wchar_t(0));
            int len = int(::GetModuleFileName(nullptr, path.data(), MAX_PATH));
            if (len > 0 && len < MAX_PATH) {
                path.erase(path.begin() + len, path.end());
            }
            return wxIcon(path, wxBITMAP_TYPE_ICO);
#else // _WIN32
            return wxIcon(Slic3r::var("AnkerStudio_128px.png"), wxBITMAP_TYPE_PNG);
#endif // _WIN32
        }

        void AnkerConfigDlg::initEventBind() {
            //m_titlePanelRect = m_titlePanel->GetRect();
            //// Implementing borderless dialog drag-and-drop movement
            //m_titlePanel->Bind(wxEVT_LEFT_DOWN, &AnkerConfigDlg::OnLeftDown, this);
            //m_titlePanel->Bind(wxEVT_LEFT_UP, &AnkerConfigDlg::OnLeftUp, this);
            //m_titlePanel->Bind(wxEVT_MOTION, &AnkerConfigDlg::OnMouseMove, this);
            //m_titlePanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &AnkerConfigDlg::OnMouseLost, this);

            //m_titlePanel->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
            //    SetCursor(wxCursor(wxCURSOR_HAND));
            //    this->SetFocus();
            //    });
            //m_titlePanel->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
            //    if (!m_titlePanelRect.Contains(event.GetPosition()) )
            //        SetCursor(wxCursor(wxCURSOR_NONE));
            //    });

            //m_titleText->Bind(wxEVT_LEFT_DOWN, &AnkerConfigDlg::OnLeftDown, this);
            //m_titleText->Bind(wxEVT_LEFT_UP, &AnkerConfigDlg::OnLeftUp, this);
            //m_titleText->Bind(wxEVT_MOTION, &AnkerConfigDlg::OnMouseMove, this);
            //m_titleText->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
            //     SetCursor(wxCursor(wxCURSOR_HAND));
            //     this->SetFocus();
            //});
            Bind(wxEVT_SIZE, &AnkerConfigDlg::OnSize, this);
            Bind(wxEVT_CLOSE_WINDOW, &AnkerConfigDlg::OnClose, this);
        }

        void AnkerConfigDlg::OnSize(wxSizeEvent & event) {
            m_rightPanelSizer->SetMinSize(wxSize(event.GetSize().GetWidth(), event.GetSize().GetHeight()));
            int rightPanelWidth = (this->GetClientRect().GetWidth()/* - m_leftPanel->GetClientRect().GetWidth()*/);
            int contentHeight = (this->GetClientRect().GetHeight());
            m_rightPanelSize = wxSize(rightPanelWidth, contentHeight);

            int pageCount = m_rightPanel->GetPageCount();
            for (int i = 0; i < pageCount; i++) {
                int sel = m_rightPanel->GetSelection();
                if (i == sel) {
                    wxWindow* page = m_rightPanel->GetPage(i);
                    AnkerTab* tab = dynamic_cast<AnkerTab*>(page);
                    tab->force_update();
                }
            }

            Layout();
            Refresh();
        }

        void AnkerConfigDlg::OnClose(wxCloseEvent& event) {
            CloseDlg();
        }

        wxSize AnkerConfigDlg::GetRightPanelSize() {
            return m_rightPanelSize;
        }

        void AnkerConfigDlg::on_dpi_changed(const wxRect& suggested_rect) {
            int curWidth = GetSize().GetWidth();
            int curHeight = GetSize().GetHeight();

            int screenWidth = 0;
            int screenHeight = 0;
            wxWindow* window = (wxWindow*)this;
            int displayIndex = wxDisplay::GetFromWindow(window);
            if (displayIndex != wxNOT_FOUND) {
                wxDisplay display(displayIndex);
                wxRect screenGeometry = display.GetClientArea();
                screenWidth= screenGeometry.GetWidth();
                screenHeight = screenGeometry.GetHeight();
            }

            ANKER_LOG_INFO << "msw_rescale, width is " << curWidth << ",height is "
                << curHeight << ",screenWidth is " << screenWidth << ",screenHeight is " << screenHeight;
             
            msw_rescale();
        }

        void AnkerConfigDlg::OnLeftDown(wxMouseEvent& event) {
            auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
            pFocusPanel ? m_titlePanel->CaptureMouse() : m_titleText->CaptureMouse();

            m_startPos = event.GetPosition();

            bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();
            if (hasCapture) {
                event.Skip(false);
                return;
            }
        }

        void AnkerConfigDlg::OnLeftUp(wxMouseEvent & event) {
            auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
            bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();

            if (!hasCapture) {
                event.Skip(false);
                return;
            }

            pFocusPanel ? m_titlePanel->ReleaseMouse() : m_titleText->ReleaseMouse();
        }

        void AnkerConfigDlg::OnMouseMove(wxMouseEvent & event) {
            //CenterOnParent();
            wxPoint pos = event.GetPosition();
            if (event.LeftIsDown() && event.Dragging()) {
                wxPoint delta = pos - m_startPos;
                wxPoint newPos = GetPosition() + delta;
                Move(newPos);
            }
            event.Skip(false);
        }

        void AnkerConfigDlg::OnMouseLost(wxMouseCaptureLostEvent& event){
            auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
            bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();
            if (!hasCapture) {
                event.Skip(false);
                return;
            }
            pFocusPanel ? m_titlePanel->ReleaseMouse() : m_titleText->ReleaseMouse();
        }

        void AnkerConfigDlg::OnExitButtonClicked(wxCommandEvent & event) {
            CloseDlg();
        }

        void AnkerConfigDlg::ChangeAnkerTabComboSel(AnkerTabPresetComboBox * presetChoice, const int selection) {
            if (!presetChoice->selection_is_changed_according_to_physical_printers()) {
                if (!presetChoice->is_selected_physical_printer())
                    wxGetApp().preset_bundle->physical_printers.unselect_printer();

                // select preset
                std::string preset_name = presetChoice->GetString(selection).ToUTF8().data();
                Preset::Type presetType = presetChoice->type();
                wxGetApp().getAnkerTab(presetType)->select_preset(Preset::remove_suffix_modified(preset_name));

				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_UPDATE_PARAMETERS_PANEL);
				evt.SetEventObject(this);
				ProcessEvent(evt);
            }
        }

        void AnkerConfigDlg::onComboBoxCloseUp(wxCommandEvent& event) {    
            AnkerTabPresetComboBox* comboBox = dynamic_cast<AnkerTabPresetComboBox*>(event.GetEventObject());
            comboBox->setColor(wxColour("#506853"), wxColour("#506853") ,
                               wxColour("#62D361"));
            comboBox->Refresh();
        }

        void AnkerConfigDlg::onPresetRightClick(wxMouseEvent& event) {
            AnkerTabPresetComboBox* comboBox = dynamic_cast<AnkerTabPresetComboBox*>(event.GetEventObject());
            Preset::Type presetType = comboBox->type();
            AnkerTab* ankerTab = wxGetApp().getAnkerTab(presetType);
            if (!ankerTab)
                return;
            bool bRenamePresetOpMenu = false;
            bool bDeletePresetOpMenu = false;

            /*   const Preset& preset = m_presets->get_edited_preset();
               m_btn_delete_preset->Show((m_type == Preset::TYPE_PRINTER && m_preset_bundle->physical_printers.has_selection())
                   || (!preset.is_default && !preset.is_system));
               m_btn_rename_preset->Show(!preset.is_default && !preset.is_system && !preset.is_external &&
                   !wxGetApp().preset_bundle->physical_printers.has_selection());*/

            switch (presetType)
            {
            case Preset::TYPE_PRINTER: {
                const Preset& preset = wxGetApp().preset_bundle->printers.get_edited_preset();
                bDeletePresetOpMenu = ankerTab->m_preset_bundle->physical_printers.has_selection()
                    || (!preset.is_default && !preset.is_system);
                bRenamePresetOpMenu = !preset.is_default && !preset.is_system && !preset.is_external &&
                    !wxGetApp().preset_bundle->physical_printers.has_selection();
                break;
            }
            case Preset::TYPE_FILAMENT: {
                const Preset& preset = wxGetApp().preset_bundle->filaments.get_edited_preset();
                bDeletePresetOpMenu = (!preset.is_default && !preset.is_system);
                bRenamePresetOpMenu = !preset.is_default && !preset.is_system && !preset.is_external &&
                    !wxGetApp().preset_bundle->physical_printers.has_selection();
                break;
            }
            case Preset::TYPE_PRINT: {
                const Preset& preset = wxGetApp().preset_bundle->prints.get_edited_preset();
                bDeletePresetOpMenu = (!preset.is_default && !preset.is_system);
                bRenamePresetOpMenu = !preset.is_default && !preset.is_system && !preset.is_external &&
                    !wxGetApp().preset_bundle->physical_printers.has_selection();
                break;
            }
            default:
                break;
            }

            auto menu = new wxMenu(wxT(""));
            menu->Append(ID_PRESET_RENAME, _L("Rename"));
            menu->Append(ID_PRESET_DELETE, _L("Delete"));
            // enable or disable rename menu item
            if (!bRenamePresetOpMenu) {
                menu->Enable(ID_PRESET_RENAME, false);
            }
            // enable or disable rename menu item
            if (!bDeletePresetOpMenu) {
                menu->Enable(ID_PRESET_DELETE, false);
            }

            wxVariant eventData;
            eventData.ClearList();

            wxPoint Pos;
            comboBox->GetPosition(&Pos.x, &Pos.y);
            eventData.Append(wxVariant(presetType));
            menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &AnkerConfigDlg::onPresetOpPopupClick,
                this, wxID_ANY, wxID_ANY, new wxVariant(eventData));
            m_leftPresetPanel->PopupMenu(menu, wxPoint(Pos.x + 94, Pos.y + 35));
        }

        void AnkerConfigDlg::onPresetOpPopupClick(wxCommandEvent & event) {
            wxVariant* pData = (wxVariant*)(event.m_callbackUserData);
            wxVariantList list = pData->GetList();
            Preset::Type presetType = (Preset::Type)list[0]->GetInteger();
            AnkerTab* ankerTab = wxGetApp().getAnkerTab(presetType);
            if (!ankerTab)
                return;
            switch (event.GetId()) {
            case ID_PRESET_RENAME:
                ankerTab->rename_preset();
                break;
            case ID_PRESET_DELETE:
                ankerTab->delete_preset();
                break;
            default:
                break;
            }
        }

        void AnkerConfigDlg::onComboBoxClick(AnkerTabPresetComboBox* presetComboBox) {
            // we changed preset tab,we should check dirty data in current prest
            handleDirtyPreset();

            // According to the product requirements, clicking on the text control area in the drop - down list 
            // should switch to the next tab, and clicking on the drop - down button in the drop - down list 
            // should display the drop - down box.

            wxRect buttonRect = presetComboBox->GetButtonRect();
            wxPoint comboBoxPos = presetComboBox->GetScreenPosition();
            wxSize  comboBoxSize = presetComboBox->GetSize();
            wxPoint buttonPos = wxPoint(comboBoxPos.x + comboBoxSize.GetWidth() - buttonRect.GetWidth(),
                comboBoxPos.y + comboBoxSize.GetHeight() - buttonRect.GetHeight());
            wxMouseState mouseState = wxGetMouseState();
            if (mouseState.GetPosition().x >= buttonPos.x
                && mouseState.GetPosition().x <= buttonPos.x + buttonRect.width
                && mouseState.GetPosition().y >= buttonPos.y
                && mouseState.GetPosition().y <= buttonPos.y + buttonRect.height) {
                ANKER_LOG_INFO << "click at area of drop button in combox, show popup";
                switchAnkerTab(presetComboBox->type());
                presetComboBox->Popup();
                m_lastSelectPreset = presetComboBox;
            }
            else {
                ANKER_LOG_INFO << "click at area of textctrl in combox, hide popup";
                // after validating the dirty data, we can switch anker tab.
                switchAnkerTab(presetComboBox->type());

                // call Popup before call Dismiss becasue of the highlight reset bug
                presetComboBox->Popup();
                presetComboBox->Dismiss();
                updateAnkerTabComBoxHighlight(presetComboBox->type());
            }
        }

        void AnkerConfigDlg::switchAnkerTab(const Preset::Type type) {
            AnkerTab* ankerTab = wxGetApp().getAnkerTab(type);
            if (!ankerTab)
                return;

            if (int page_id = wxGetApp().ankerTabPanel()->FindPage(ankerTab); page_id != wxNOT_FOUND) {
                wxGetApp().ankerTabPanel()->SetSelection(page_id);
                // Switch to Settings NotePad
                wxGetApp().mainframe->select_tab();

                // update btns enabling
                ankerTab->update_btns_enabling();
            }

            resetPresetComBoxHighlight();
        }

        void AnkerConfigDlg::updateAnkerTabComBoxHighlight(const Preset::Type type) {
            resetPresetComBoxHighlight();

            // open config dialog from sidebarnew or menu bar will highlight the AnkerTabPresetCombo auto
            m_lastSelectPreset = GetAnkerTabPresetCombo(type);
          
            if (m_lastSelectPreset) {
                m_lastSelectPreset->setColor(wxColour("#506853"),
                                             wxColour("#506853"),
                                             wxColour("#62D361"));
                m_lastSelectPreset->Refresh();
            }
            AnkerTab* ankerTab = wxGetApp().getAnkerTab(type);
            if (!ankerTab)
                return;
            // update btns enabling
            ankerTab->update_btns_enabling();
        }

        void AnkerConfigDlg::CloseDlg() {
            ANKER_LOG_INFO << "AnkerConfigDlg CloseDlg enter";
            Hide();
            EndModal(wxID_ANY);
            //bool bHandled = handleDirtyPreset();
            //// if bHandled is true means has saved or discharged,if bHandled is false means has canceled,should not close dialog
            //if (bHandled) {
            //    ANKER_LOG_INFO << "AnkerConfigDlg CloseDlg handleDirtyPreset return true,will exit";
            //    // Modify it to a modal dialog box according to product requirements
            //    Hide();
            //    EndModal(wxID_ANY);
            //    // restrore multicolor filament after dischard extruders removed
            //    if (wxGetApp().plater()->sidebarnew().isMultiFilament()) {
            //        wxGetApp().plater()->sidebarnew().onExtrudersChange();
            //    }
            //    
            //    return;
            //}
            //ANKER_LOG_INFO << "AnkerConfigDlg CloseDlg handleDirtyPreset return false, means canceled, will ignore";
        }

        void AnkerConfigDlg::msw_rescale() {
            // should not call Fit otherwise the size is changed when dpi changed
            //Fit();
            Layout();
            int rightPanelWidth = (this->GetClientRect().GetWidth()/* - m_leftPanel->GetClientRect().GetWidth()*/);
            int contentHeight = (this->GetClientRect().GetHeight());
            m_rightPanelSize = wxSize(rightPanelWidth, contentHeight);
            
            Refresh();
        }

        void AnkerConfigDlg::resetPresetComBoxHighlight() {
            if (m_printerPresetsChoice) {
                m_printerPresetsChoice->setColor(wxColour("#434447"), 
                                                 wxColour("#3A3B3F"));
                m_printerPresetsChoice->Refresh();
            }

            if (m_filamentPresetsChoice) {
                m_filamentPresetsChoice->setColor(wxColour("#434447"),
                                                  wxColour("#3A3B3F"));
                m_filamentPresetsChoice->Refresh();
            }

            if (m_printPresetsChoice) {
                m_printPresetsChoice->setColor(wxColour("#434447"),
                    wxColour("#3A3B3F"));
                m_printPresetsChoice->Refresh();
            }
        }

        bool AnkerConfigDlg::handleDirtyPreset() {
            // According to product requirements, when the parameter details dialog box is closed, 
                   // if the preset configuration has been modified, a save preset pop - up window needs to be displayed.
            bool bRet = true;
            unsigned int selectedIndex = m_lastSelectPreset->GetSelection();
            std::string preset_name = m_lastSelectPreset->GetString(selectedIndex).ToUTF8().data();
            Preset::Type presetType = m_lastSelectPreset->type();
            bool bCurIsDirty = wxGetApp().getAnkerTab(presetType)->current_preset_is_dirty();
            ANKER_LOG_INFO << "handleDirtyPreset enter, bCurIsDirty:" << bCurIsDirty << ",preset_name is " << preset_name.c_str()
                << ",presetType:" << presetType;
            if (bCurIsDirty) {
                //wxGetApp().getAnkerTab(presetType)->select_preset(Preset::remove_suffix_modified(preset_name));
                return wxGetApp().getAnkerTab(presetType)->AnkerMayDiscardCurDirtyPreset(nullptr,
                    Preset::remove_suffix_modified(preset_name));
            }
            return bRet;
        }
    }
}

#include "AnkerSideBarNew.hpp"

#include <vector>
#include <algorithm>
#include <wx/string.h>

#include "GUI_App.hpp"
#include "Plater.hpp"
#include "MainFrame.hpp"
#include "Tab.hpp"
// add by allen for ankerCfgDlg
#include "AnkerCfgTab.hpp"

#include "GLCanvas3D.hpp"
#include "Gizmos/GLGizmosManager.hpp"
#include "Slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "common/AnkerGUIConfig.hpp"
#include "common/AnkerBitmapCombox.hpp"


namespace Slic3r {
    namespace GUI {
#define COLOUR_BTN_SIZE 25

#define SEPARATOR_HEAD "————— "
#define SEPARATOR_TAIL " —————"
#define SYSYTEM_PRESETS "common_slicepannel_parametersselect_title1"
#define MY_PRESETS "common_slicepannel_parametersselect_title2"
#define SYSTEM_PRESETS_SEPARATOR wxString::FromUTF8(SEPARATOR_HEAD) + _L(SYSYTEM_PRESETS) + wxString::FromUTF8(SEPARATOR_TAIL)
#define MY_PRESETS_SEPARATOR wxString::FromUTF8(SEPARATOR_HEAD) + _L(MY_PRESETS) + wxString::FromUTF8(SEPARATOR_TAIL)

#define SIDEBARNEW_PRINTGER_HEIGHT  90  // 109
#define SIDEBARNEW_PRINTGER_TEXTBTN_SIZER   40 //46
#define SIDEBARNEW_PRINTGER_HOR_SPAN  10 // 14
#define SIDEBARNEW_PRINTGER_VER_SPAN  5 // 14
#define SIDEBARNEW_PRINTGER_COMBO_WIDTH  (SIDEBARNEW_WIDGET_WIDTH - 2* SIDEBARNEW_PRINTGER_HOR_SPAN)  // 276

#ifndef __APPLE__
#define SIDEBARNEW_PRINTGER_COMBO_VER_SPAN  10
#define SIDEBARNEW_COMBOBOX_HEIGHT 30
#else
#define SIDEBARNEW_PRINTGER_COMBO_VER_SPAN  7
#define SIDEBARNEW_COMBOBOX_HEIGHT 35
#endif

#define SIDEBARNEW_FILAMENT_HEIGHT  90 // 111 94
#define SIDEBARNEW_FILAMENT_TEXTBTN_SIZER  40 // 46
#define SIDEBARNEW_FILAMENT_PANEL_HEIGHT  (SIDEBARNEW_FILAMENT_HEIGHT - SIDEBARNEW_FILAMENT_TEXTBTN_SIZER)
#define SIDEBARNEW_FILAMENT_HOR_SPAN  10 // 14
#define SIDEBARNEW_FILAMENT_VER_SPAN  8 // 14

        

        const int MAX_FILAMENT_BUTTON_NUMS = 6;
        enum TimerID {
            timer1
        };

        wxBEGIN_EVENT_TABLE(AnkerSidebarNew, wxPanel)
            EVT_TIMER(timer1, AnkerSidebarNew::OnTimer)
            wxEND_EVENT_TABLE()

        wxDEFINE_EVENT(wxCUSTOMEVT_CLICK_FILAMENT_BTN, wxCommandEvent);
        wxDEFINE_EVENT(wxCUSTOMEVT_REMOVE_FILAMENT_BTN, wxCommandEvent);
        wxDEFINE_EVENT(wxCUSTOMEVT_UPDATE_FILAMENT_INFO, wxCommandEvent);
        wxDEFINE_EVENT(wxCUSTOMEVT_CHECK_RIGHT_DIRTY_DATA, wxCommandEvent);
        wxDEFINE_EVENT(wxCUSTOMEVT_MAIN_SIZER_CHANGED, wxCommandEvent);

        struct AnkerSidebarNew::priv
        {
            Plater* plater;

            priv(Plater* plater) : plater(plater) {}
            ~priv() {};

            // Fixed printer configuration panel
            wxPanel* m_printerPanel{ nullptr };
            wxBoxSizer* m_printerSizer{ nullptr };
            wxFlexGridSizer* m_printerPanelSizer{ nullptr };

            // Fixed filament configuration panel
            wxBoxSizer* m_filamentSizer{ nullptr };
            wxPanel* m_filamentPanel{ nullptr };

            // Parameter panel default universal Sizer
            wxBoxSizer* m_pParameterVSizer{ nullptr };
            AnkerParameterPanel* m_pAnkerParameterPanel{ nullptr };

            //right key menu parameter panel defualt universal Sizer
			//wxBoxSizer* m_pRightMenuParameterVSizer{ nullptr };
			AnkerParameterPanel* m_pAnkerRightMenuParameterPanel{ nullptr };

            // Variable universal configuration panel
         /*   wxPanel* universalPanel{ nullptr };
            wxBoxSizer* universalSizer{ nullptr };*/

            // custom universal sizer, you can define your generic sizer to replace it
            wxBoxSizer* m_universalCustomSizer{ nullptr };
            wxPanel* m_universalCustomPanel{ nullptr };

            // m_mainSizer
            wxBoxSizer* m_mainSizer{ nullptr };

            // new main sizer for test
            wxBoxSizer* m_newMainSizer{ nullptr };
            wxPanel* m_newMainPanel{ nullptr };

            // for single and multi color filament
            wxFlexGridSizer* m_filamentPanelSizer{ nullptr };
            wxPanel* m_singleFilamentPanel{ nullptr };
            wxPanel* m_horFilamentPanel{ nullptr };
            wxPanel* m_verFilamentPanel{ nullptr };
            wxMenu* m_filamentMenu{ nullptr };

            ScalableButton* m_filamentListToggleBtn{ nullptr };
            ScalableButton* m_filamentSyncBtn{ nullptr };

            AnkerFilamentEditDlg* m_filamentEditDlg{ nullptr };
            std::atomic<bool> m_bSidebarFirstInit{ true };

            // extruder info
            std::vector<SFilamentInfo> m_editFilamentInfoList;
            // add by allen for add anker_filament_id and anker_colour_id
            std::mutex editFilamentMtx;
            // filament info, key:filament type value: filament info vector
            std::map <wxString, std::vector<SFilamentInfo>> m_filamentInfoList;
            // save for current printer
            std::string m_strTypePrinterEnterName;
            std::atomic_bool bChangedPrinter{false};

            FilamentViewType m_filamentViewType{ multiColorHorView };
            // add by allen for ankerCfgDlg
            //PlaterPresetComboBox* m_comboPrinter{nullptr};
            AnkerPlaterPresetComboBox* m_comboPrinter{ nullptr };

            int m_marginBase{ 1 };

            // example of how to use siderbar
            wxTimer* m_timer{ nullptr };

            Search::OptionsSearcher     searcher;

            // default select first btn
            int m_selectdColorBtnIndex{ 1 };
        };
        wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_FILAMENT_CHANGED, wxCommandEvent);
        AnkerFilamentEditDlg::AnkerFilamentEditDlg(wxWindow* parent, std::string title, SFilamentEditInfo filamentEditInfo, wxButton* btn)
            : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
#ifdef __APPLE__
                wxNO_BORDER | wxSTAY_ON_TOP)
#else
            wxNO_BORDER)
#endif
            , m_title(title)
            , m_pExitBtn(nullptr)
            , m_pEditColorBtn(btn)
            , m_oldFilamentEditInfo(std::move(filamentEditInfo)) {
            m_filamentInfoList = wxGetApp().plater()->sidebarnew().getAllFilamentList();
            m_newFilamentEditInfo = m_oldFilamentEditInfo;

            // init ui
            initUI();

            if (parent == nullptr) {
                int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
                int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
                SetPosition(wxPoint((screenW - 400) / 2, (screenH - 180) / 2));
            }
            else {
                SetPosition(wxPoint(parent->GetScreenPosition().x - GetSize().x,
                    parent->GetScreenPosition().y));
            }
        }

        AnkerFilamentEditDlg::~AnkerFilamentEditDlg() {

        }

        void AnkerFilamentEditDlg::initUI()
        {
            SetSize(AnkerSize(274, 160));
            SetBackgroundColour(wxColour("#292A2D"));

            wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);

            // title
            wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
            titleHSizer->SetMinSize(AnkerSize(274, 40));
            contentVSizer->Add(titleHSizer, 0, wxEXPAND | wxALL, 0);

            titleHSizer->AddSpacer(AnkerLength(12));

            m_pTitleText = new wxStaticText(this, wxID_ANY, m_title);
            m_pTitleText->SetBackgroundColour(wxColour("#292A2D"));
            m_pTitleText->SetForegroundColour(wxColour("#FFFFFF"));
            m_pTitleText->SetFont(ANKER_BOLD_FONT_NO_1);
            m_pTitleText->SetMinSize(AnkerSize(100, -1));
            titleHSizer->AddSpacer(AnkerLength(7));
            titleHSizer->Add(m_pTitleText, 0, wxALIGN_CENTER_VERTICAL, 0);
            titleHSizer->AddStretchSpacer(1);

            // exit button
            m_pExitBtn = new ScalableButton(this, wxID_ANY, "filamentEditExitBtn", "", AnkerSize(16, 16));
            m_pExitBtn->SetWindowStyleFlag(wxBORDER_NONE);
            m_pExitBtn->SetBackgroundColour(wxColour("#292A2D"));
            m_pExitBtn->SetForegroundColour(wxColour("#FFFFFF"));
            m_pExitBtn->Bind(wxEVT_BUTTON, &AnkerFilamentEditDlg::OnExitButtonClicked, this);
            titleHSizer->Add(m_pExitBtn, 0, wxALIGN_CENTER_VERTICAL, 0);
            titleHSizer->AddSpacer(AnkerLength(7));

            // split line
            wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            splitLineCtrl->SetBackgroundColour(wxColour("#38393C"));
            splitLineCtrl->SetMaxSize(wxSize(100000, 1));
            splitLineCtrl->SetMinSize(wxSize(1, 1));
            contentVSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 0);

            contentVSizer->AddSpacer(AnkerLength(5));

            // filament combox
            initFilamentCombox();
            contentVSizer->Add(m_comboFilament, 1, wxEXPAND | wxALL, AnkerLength(12));

            // color combox
            initColorCombox();

#ifndef __APPLE__
            contentVSizer->Add(m_comboColor, 1, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, AnkerLength(12));
#else
            contentVSizer->Add(m_comboColor, 1, wxEXPAND | wxLEFT | wxRIGHT, AnkerLength(12));
#endif
            contentVSizer->AddStretchSpacer(1);

            SetSizer(contentVSizer);
        }

        void AnkerFilamentEditDlg::initFilamentCombox() {
            wxArrayString   m_arrItems{};
            int selectedIndex = 0, currentIndex = 0;
            for (auto iter1 : m_filamentInfoList) {
                m_arrItems.Add(wxString::FromUTF8(iter1.first.ToStdString()));
                for (auto iter2 : iter1.second) {
                    if (0 == m_oldFilamentEditInfo.filamentInfo.wxStrLabelName.Cmp(iter2.wxStrLabelName)) {
                        currentIndex = selectedIndex;
                    }
                }
                selectedIndex++;
            }

            m_comboFilament = new AnkerPlaterPresetComboBox(this, Slic3r::Preset::TYPE_FILAMENT);
            m_comboFilament->Create(this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(250, AnkerLength(ANKER_COMBOBOX_HEIGHT)),
                m_arrItems,
                wxNO_BORDER | wxCB_READONLY);
            m_comboFilament->SetSelection(currentIndex);
            m_comboFilament->SetFont(ANKER_FONT_NO_1);
            m_comboFilament->SetBackgroundColour(wxColour("#434447"));
            m_comboFilament->setColor(wxColour("#434447"), wxColour("#3A3B3F"));

            m_comboFilament->set_button_clicked_function([this]() {
				ANKER_LOG_INFO << "preset combox of print clicked";
                m_comboFilament->SetFocus();
                
                if (!Slic3r::GUI::wxGetApp().plater()->sidebarnew().checkDirtyDataonParameterpanel())
				    onComboBoxClick(m_comboFilament);
				});
            m_comboFilament->set_selection_changed_function([this](int selection) {
				ANKER_LOG_INFO << "preset combox of print clicked";
				onPresetComboSelChanged(m_comboFilament, selection);
				});

            wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
            btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
            wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));

            m_comboFilament->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

            // default select
            if (wxNOT_FOUND == m_comboFilament->GetSelection()) {
                m_comboFilament->SetSelection(0);
            }
            m_comboFilament->Bind(wxEVT_COMBOBOX, &AnkerFilamentEditDlg::OnFilamentComboSelected, this);
            m_comboFilament->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
                SetCursor(wxCursor(wxCURSOR_HAND));
                });
            m_comboFilament->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
                SetCursor(wxCursor(wxCURSOR_NONE));
                });
        }

        void AnkerFilamentEditDlg::initColorCombox() {
            wxArrayString   m_colorArrayItems{};
            m_comboColor = new AnkerBitmapCombox();
            m_comboColor->Create(this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(250, AnkerLength(ANKER_COMBOBOX_HEIGHT)),
                m_colorArrayItems,
                wxNO_BORDER | wxCB_READONLY);
            m_comboColor->SetFont(ANKER_FONT_NO_1);
            m_comboColor->SetBackgroundColour(wxColour("#434447"));
            m_comboColor->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
            wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
            btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
            wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            m_comboColor->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

            // reset color combox
            resetColorCombox();

            m_comboColor->Bind(wxEVT_COMBOBOX, &AnkerFilamentEditDlg::OnColorComboSelected, this);
            m_comboColor->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
                SetCursor(wxCursor(wxCURSOR_HAND));
                });
            m_comboColor->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
                SetCursor(wxCursor(wxCURSOR_NONE));
                });
        }

        void AnkerFilamentEditDlg::resetColorCombox(bool bInitCreate) {
            int filamentSelection = m_comboFilament->GetSelection();
            wxString selectedText = m_comboFilament->GetString(filamentSelection);
            std::vector<SFilamentInfo> vecAllFilamentInfo = m_filamentInfoList[selectedText];
            std::vector<SFilamentInfo> vecUserFilamentInfo;

            // add System presets separator
            AnkerBitmapComboxItem* comboBoxItem = new AnkerBitmapComboxItem(SYSTEM_PRESETS_SEPARATOR, "");
            m_comboColor->SetClientData(m_comboColor->Append(SYSTEM_PRESETS_SEPARATOR), comboBoxItem);
            int selectionIndex = 1;
            for (auto iter : vecAllFilamentInfo) {
                if (systemFilamentType == iter.filamentSrcType) {
                    AnkerBitmapComboxItem* comboBoxItem = new AnkerBitmapComboxItem(
                        wxString::FromUTF8(iter.wxStrColorName.ToStdString()),
                        wxString::FromUTF8(iter.wxStrColor.ToStdString()));
                    m_comboColor->SetClientData(
                        m_comboColor->Append(wxString::FromUTF8(iter.wxStrColorName.ToStdString())),
                        comboBoxItem);
                    if (bInitCreate && 0 == m_oldFilamentEditInfo.filamentInfo.wxStrColor.Cmp(iter.wxStrColor)
                        && 0 == m_oldFilamentEditInfo.filamentInfo.wxStrLabelName.Cmp(iter.wxStrLabelName)) {
                        m_comboColor->SetSelection(selectionIndex);
                        m_comboColorLastSelected = selectionIndex;
                    }
                    selectionIndex++;
                }
                else {
                    vecUserFilamentInfo.emplace_back(iter);
                }
            }
            // add My presets separator
            if (!vecUserFilamentInfo.empty()) {
                AnkerBitmapComboxItem* comboBoxItem = new AnkerBitmapComboxItem(MY_PRESETS_SEPARATOR, "");
                m_comboColor->SetClientData(m_comboColor->Append(MY_PRESETS_SEPARATOR), comboBoxItem);
                selectionIndex++;
            }

            for (auto iter : vecUserFilamentInfo) {
                if (userFilamentType == iter.filamentSrcType) {
                    AnkerBitmapComboxItem* comboBoxItem = new AnkerBitmapComboxItem(
                        wxString::FromUTF8(iter.wxStrColorName.ToStdString()),
                        wxString::FromUTF8(iter.wxStrColor.ToStdString()));
                    m_comboColor->SetClientData(
                        m_comboColor->Append(wxString::FromUTF8(iter.wxStrColorName.ToStdString())),
                        comboBoxItem);
                    if (bInitCreate && 0 == m_oldFilamentEditInfo.filamentInfo.wxStrColor.Cmp(iter.wxStrColor)
                        && 0 == m_oldFilamentEditInfo.filamentInfo.wxStrLabelName.Cmp(iter.wxStrLabelName)) {
                        m_comboColor->SetSelection(selectionIndex);
                        m_comboColorLastSelected = selectionIndex;
                    }
                    selectionIndex++;
                }
            }

            // default select
            if (wxNOT_FOUND == m_comboColor->GetSelection()) {
                // Skip the first index which is SYSTEM_PRESETS_SEPARATOR
                int selectionIndex = 1;
                m_comboColor->SetSelection(selectionIndex);
                m_comboColorLastSelected = selectionIndex;
            }
        }

        wxBitmap AnkerFilamentEditDlg::createBitmapFromColor(int width, int height, wxColour color)
        {
            wxImage image(width, height);
            image.SetRGB(wxRect(0, 0, width, height), color.Red(), color.Green(), color.Blue());
            return wxBitmap(image);
        }

        void AnkerFilamentEditDlg::OnExitButtonClicked(wxCommandEvent& event) {
            ANKER_LOG_INFO << "AnkerFilamentEditDlg OnExitButtonClicked enter";
            Show(false);
            Close();
        }

        void AnkerFilamentEditDlg::SetEditColorBtnPtr(wxButton* btn) {
            if (btn != m_pEditColorBtn) {
                m_pEditColorBtn = btn;
                ANKER_LOG_INFO << "AnkerFilamentEditDlg SetEditColorBtnPtr success";
            }
        }

        void AnkerFilamentEditDlg::OnFilamentComboSelected(wxCommandEvent& event) {
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_CHECK_RIGHT_DIRTY_DATA);
            ProcessEvent(evt);

            int index = m_comboFilament->GetSelection();
            wxString filamentLabelType = m_comboFilament->GetString(index);
            ANKER_LOG_INFO << "OnFilamentComboSelected enter, filamentLabelType is " << 
                filamentLabelType.ToStdString().c_str();
            m_newFilamentEditInfo.filamentInfo.wxStrLabelType = filamentLabelType;
            // update color combo
            m_comboColor->Clear();

            // reset color combox
            resetColorCombox(false);
            // we still use index 0 to ignore SYSTEM_PRESETS_SEPARATOR
            m_newFilamentEditInfo.filamentInfo = m_filamentInfoList[filamentLabelType][0];

            if (!(m_newFilamentEditInfo == m_oldFilamentEditInfo)) {
                m_oldFilamentEditInfo = m_newFilamentEditInfo;
                PostFilamentUpdateEvent();
            } 
        }

        void AnkerFilamentEditDlg::OnColorComboSelected(wxCommandEvent& event) {
            int selectionIndex = m_comboColor->GetSelection();
            std::string strSelColorName = m_comboColor->GetString(selectionIndex).ToStdString();
            ANKER_LOG_INFO << "OnColorComboSelected enter, strSelColorName is " << strSelColorName.c_str();
            // if user click SYSTEM_PRESETS_SEPARATOR or MY_PRESETS_SEPARATOR,ignore it
            if (SYSTEM_PRESETS_SEPARATOR == strSelColorName
                || MY_PRESETS_SEPARATOR == strSelColorName) {
                m_comboColor->SetSelection(m_comboColorLastSelected);
                return;
            }
            else {
                m_comboColorLastSelected = selectionIndex;
            }

            // check this preset is system preset or user preset
            int mypresetIndex = m_comboColor->FindString(MY_PRESETS_SEPARATOR);
            bool bUserPreset = (selectionIndex > mypresetIndex && mypresetIndex > 0) ? true : false;

            int filamentIndex = m_comboFilament->GetSelection();
            wxString filamentLabeltext = m_comboFilament->GetString(filamentIndex);
            for (auto iter : m_filamentInfoList[filamentLabeltext]) {
                std::string strColorName = wxString::FromUTF8(iter.wxStrColorName.ToStdString()).ToStdString();
                if ( 0 == strSelColorName.compare(strColorName)) {
                    // we should also compare the source type 
                    if ((bUserPreset && iter.filamentSrcType == userFilamentType)
                         || (!bUserPreset && iter.filamentSrcType == systemFilamentType)) {
                        m_newFilamentEditInfo.filamentInfo = iter;
                        break;
                    }
                    continue;
                }
            }

            if (!(m_newFilamentEditInfo == m_oldFilamentEditInfo)) {
                m_oldFilamentEditInfo = m_newFilamentEditInfo;
                PostFilamentUpdateEvent();
            }    
        }

        void AnkerFilamentEditDlg::PostFilamentUpdateEvent() {
                wxVariant eventData;
                eventData.ClearList();
                eventData.Append(wxVariant(&m_newFilamentEditInfo));
                eventData.Append(wxVariant(m_pEditColorBtn));
                wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_UPDATE_FILAMENT_INFO);
                evt.SetClientData(new wxVariant(eventData));
                wxPostEvent(&wxGetApp().plater()->sidebarnew(), evt);

				wxCommandEvent comboboxChangedEvt = wxCommandEvent(wxCUSTOMEVT_ANKER_FILAMENT_CHANGED);
                comboboxChangedEvt.SetEventObject(this);
                wxPostEvent(&wxGetApp().plater()->sidebarnew(), comboboxChangedEvt);
        }

		void AnkerFilamentEditDlg::onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox)
		{
            presetComboBox->Popup();
		}

		void AnkerFilamentEditDlg::onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection)
		{
			if (!presetChoice->selection_is_changed_according_to_physical_printers()) {
				if (!presetChoice->is_selected_physical_printer())
					Slic3r::GUI::wxGetApp().preset_bundle->physical_printers.unselect_printer();

				// select preset
				std::string preset_name = presetChoice->GetString(selection).ToUTF8().data();
				Slic3r::Preset::Type presetType = presetChoice->type();
				Slic3r::GUI::wxGetApp().getAnkerTab(presetType)->select_preset(Slic3r::Preset::remove_suffix_modified(preset_name));

			}
		}

        // AnkerSidebarNew / public
        AnkerSidebarNew::AnkerSidebarNew(Plater* parent)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1)), p(new priv(parent)) {
            SetBackgroundColour(wxColour("#18191B"));

            // if we have selected new filament colour, we should update the filament info
            Bind(wxCUSTOMEVT_UPDATE_FILAMENT_INFO, &AnkerSidebarNew::onUpdateFilamentInfo, this);
            // if you want to experience the effect of reset and restore sidebar
            // please open the following third lines of code comments
          /*  p->m_timer = new wxTimer(this, timer1);
            if (p->m_timer) {
                p->m_timer->Start(5000);
            }*/
        }

        AnkerSidebarNew::~AnkerSidebarNew() {
            if (p->m_timer) {
                p->m_timer->Stop();
            }
        }


		void AnkerSidebarNew::reloadParameterData()
		{
            if(p->m_pAnkerParameterPanel)
                p->m_pAnkerParameterPanel->reloadData();
		}


		void AnkerSidebarNew::updatePreset(DynamicPrintConfig& config)
		{
            if(p->m_pAnkerParameterPanel)
                p->m_pAnkerParameterPanel->updatePreset(config);
		}

		void AnkerSidebarNew::hidePopupWidget()
		{
            if (p->m_pAnkerParameterPanel)
                p->m_pAnkerParameterPanel->hidePoupWidget();
		}

		void AnkerSidebarNew::enableSliceBtn(bool isSaveBtn, bool isEnable)
		{
            if(p->m_pAnkerParameterPanel)
                p->m_pAnkerParameterPanel->enableSliceBtn(isSaveBtn, isEnable);
		}

        bool AnkerSidebarNew::replaceUniverseSubSizer(wxSizer* sizer, wxString sizerFlags) {
            this->Freeze();
            // if sizer is nullptr, use default m_pParameterVSizer instead;            
            if (!sizer)
            {
                m_showRightMenuPanel = PANEL_PARAMETER_PANEL;
                sizer = p->m_pParameterVSizer;
            }
            else
                m_showRightMenuPanel = PANEL_OTHER_PARAMETER;
            m_sizerFlags = sizerFlags;
            
            //add by alves
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MAIN_SIZER_CHANGED);
            evt.SetClientObject(new wxStringClientData(m_sizerFlags));
            wxPostEvent(this, evt);

            int sizerCnts = p->m_mainSizer->GetItemCount();
            CallAfter([this, sizer, sizerCnts] {
            wxSizerItem* oldSizerItem = p->m_mainSizer->GetItem(sizerCnts-1);
            oldSizerItem->GetSizer()->Show(false);

            p->m_mainSizer->Detach(sizerCnts-1);

            p->m_mainSizer->Add(sizer, 1, wxEXPAND | wxTop, 8 * p->m_marginBase);
            sizer->Show(true);
            if (m_showRightMenuPanel == PANEL_RIGHT_MENU_PARAMETER)
            {
                p->m_pAnkerParameterPanel->Hide();
                p->m_pAnkerRightMenuParameterPanel->Show();
            }
            else if (m_showRightMenuPanel == PANEL_PARAMETER_PANEL)
            {
                p->m_pAnkerParameterPanel->Show();
                p->m_pAnkerRightMenuParameterPanel->Hide();
            }
            else
            {
                p->m_pAnkerParameterPanel->Hide();
                p->m_pAnkerRightMenuParameterPanel->Hide();
            }

            refreshSideBarLayout();
            this->Thaw();
                });
           
            return true;
        }

        void AnkerSidebarNew::setMainSizer(wxSizer* sizer, bool bForceAllDefault) {

            this->Freeze();
            // close filament Edit dlg
            closeFilamentEditDlg();
            
            // sizer is not change, do not handling
            if (sizer == nullptr && p->m_mainSizer == GetSizer()) {
                this->Thaw();
                return;
            }

            if (GetSizer() == sizer) {
                this->Thaw();
                return;
            }

            // hide old main sizer
            if (GetSizer()) {
                GetSizer()->Show(false);
            }

            // set default mainSizer when sizer is nullptr
            if (!sizer) {
                SetSizer(p->m_mainSizer, false);
                if (bForceAllDefault) {
                    // use all of the default layout includes universe sub sizer
                    m_showRightMenuPanel = PANEL_PARAMETER_PANEL;
                    replaceUniverseSubSizer(p->m_pParameterVSizer);
                }
                else
                    m_showRightMenuPanel = PANEL_OTHER_PARAMETER;
                p->m_mainSizer->Show(true);
            }
            else {
                SetSizer(sizer, false);
                sizer->Show(true);
            }
            
            p->m_pAnkerRightMenuParameterPanel->Hide();
            
            refreshSideBarLayout();                 
            this->Thaw();
        }

        void AnkerSidebarNew::setFixedDefaultSidebar() {
            p->m_marginBase = int(0.1 * wxGetApp().em_unit()); // 1
            // m_mainSizer
            p->m_mainSizer = new wxBoxSizer(wxVERTICAL);

            // m_printerPanel
            createPrinterSiderbar();

            // m_filamentPanel
            createFilamentSidebar();


            const int margin = 8 * p->m_marginBase;
            // 1.top margin sizer
            p->m_mainSizer->AddSpacer(12);
            // 2.printer sizer
            p->m_mainSizer->Add(p->m_printerSizer, 0, wxEXPAND | wxBOTTOM, margin);
            // 3. filament sizer
            p->m_mainSizer->Add(p->m_filamentSizer, 0, wxEXPAND | wxTop | wxBOTTOM, margin);
            // 4.parameterPanel sizer
            
            createPrintParameterSidebar();
			createRightMenuPrintParameterSidebar();
			p->m_pAnkerRightMenuParameterPanel->Hide();
            p->m_mainSizer->Add(p->m_pParameterVSizer, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
                        
            
            SetSizer(p->m_mainSizer);
        }

        void AnkerSidebarNew::setAllDefaultSidebar() {
            // fixed panel
            setFixedDefaultSidebar();

            // universal Panel
            createPrintParameterSidebar(); 

            setMainSizer(p->m_mainSizer);

            // use preview right sidebar default in preview mode
            if (!wxGetApp().is_editor()) {
               setMainSizer(wxGetApp().plater()->CreatePreViewRightSideBar());
            }
        }

        void AnkerSidebarNew::refreshSideBarLayout() {
            Layout();
            Refresh();
        }

        void AnkerSidebarNew::updateAllPresetComboboxes()
        {
            PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
            const auto print_tech = preset_bundle.printers.get_edited_preset().printer_technology();

            // Update the print choosers to only contain the compatible presets, update the dirty flags.
         /*   if (print_tech == ptFFF)
                p->combo_print->update();
            else {
                p->combo_sla_print->update();
                p->combo_sla_material->update();
            }*/
            // Update the printer choosers, update the dirty flags.
            if (p->m_comboPrinter) {
                p->m_comboPrinter->update();
            }
            
            //// Update the extrudes
            //onExtrudersChange();

            // Update the filament choosers to only contain the compatible presets, update the color preview,
            // update the dirty flags.
            /*if (print_tech == ptFFF) {
                for (PlaterPresetComboBox* cb : p->combosFilament)
                    cb->update();
            }*/

        }

        void AnkerSidebarNew::updatePresets(Slic3r::Preset::Type preset_type)
        {
            PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
            const auto print_tech = preset_bundle.printers.get_edited_preset().printer_technology();

            switch (preset_type) {
            case Preset::TYPE_FILAMENT:
            {
                const size_t extruder_cnt = print_tech != ptFFF ? 1 :
                    dynamic_cast<ConfigOptionFloats*>(preset_bundle.printers.get_edited_preset().config.option("nozzle_diameter"))->values.size();
                const size_t filament_cnt = p->m_filamentInfoList.size() > extruder_cnt ? extruder_cnt : p->m_filamentInfoList.size();

                if (filament_cnt == 1) {
                    // Single filament printer, synchronize the filament presets.
                    const std::string& name = preset_bundle.filaments.get_selected_preset_name();
                    preset_bundle.set_filament_preset(0, name);
                }

                // Affiliate filament combox change in filament tab setting
                CallAfter([this] {
                    onExtrudersChange();
                });

                break;
            }

              case Preset::TYPE_PRINT:
                   p->m_pAnkerParameterPanel->reloadDataEx();
                   break;
            /*
               case Preset::TYPE_SLA_PRINT:
                   p->combo_sla_print->update();
                   break;

               case Preset::TYPE_SLA_MATERIAL:
                   p->combo_sla_material->update();
                   break;*/

            case Preset::TYPE_PRINTER:
            {
                p->m_strTypePrinterEnterName = preset_bundle.printers.get_edited_preset().name;
                ANKER_LOG_INFO << "get printer Name is " << p->m_strTypePrinterEnterName.c_str() << ", when enter printer type";
                p->bChangedPrinter.store(true);
                // we should close filament edit dialog when changed printer because of the different filaments contains
                if (p->m_filamentEditDlg && p->m_filamentEditDlg->IsShown()) {
                    closeFilamentEditDlg();
                }

                CallAfter([this] {
                    updateAllPresetComboboxes();
                    });
                
                break;
            }

            default: break;
            }

            // Synchronize config.ini with the current selections.
            //wxGetApp().preset_bundle->export_selections(*wxGetApp().app_config);
        }

        void AnkerSidebarNew::onExtrudersChange(size_t changedExtruderNums) {
            // get filament label and color
            getAllFilamentFromCfg();

            // get extruder info
            getEditFilamentFromCfg(changedExtruderNums);

            // filament view type
            p->m_filamentViewType = isMultiFilament() ? (p->m_filamentViewType == multiColorVerView ? multiColorVerView : multiColorHorView): singleColorView;
            
            // init ui
            if (p->m_bSidebarFirstInit.load()) {
                p->m_bSidebarFirstInit.store(false);
                setAllDefaultSidebar();
            }
            else {
                updateFilamentInfo(true);
            }
        }

        void AnkerSidebarNew::getEditFilamentFromCfg(size_t changedExtruderNums) {
            Slic3r::DynamicPrintConfig* config = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            const size_t extruder_cnt = dynamic_cast<ConfigOptionFloats*>(config->option("nozzle_diameter"))->values.size();
            size_t numExtruders = (changedExtruderNums == 0) ? extruder_cnt : changedExtruderNums;

            // default filament presets
            std::vector<std::string> defaultFilamentPresets;
            defaultFilamentPresets = config->option<ConfigOptionStrings>("default_filament_profile")->values;
            size_t defaultFilamentSize = defaultFilamentPresets.size();
            if (defaultFilamentSize != numExtruders) {
                ANKER_LOG_ERROR << "default_filament_profile size is " << defaultFilamentSize << ", numExtruders size is " << numExtruders;
                defaultFilamentPresets.resize(numExtruders);
                for (int i = defaultFilamentSize; i < numExtruders; i++) {
                    defaultFilamentPresets[i] = defaultFilamentPresets[0];
                }
            }
            // filament presets from preset bundle
            std::vector<std::string > filamentPresets = wxGetApp().preset_bundle->filament_presets;
            // edit filament preset
            std::vector<std::string> editFilamentPresets;
            
            // mod by allen for  using default filament when printer preset selection is change
            // see function of update_multi_material_filament_presets
            auto bNeedUseDefaultFilament = [filamentPresets, changedExtruderNums, this]() -> bool {
                // if changedExtruderNums is equal 0 and printer not changed indicates the filament preset of comboBox is changed,should not use defaultFilament 
                // we should only use default filament  when printer preset of comboBox is changed; 
                if (0 == changedExtruderNums) {
                    // if we load project file we should show the filament as it was saved
                    bool bUseDefaultFilament = (p->bChangedPrinter.load() && (!getLoadProjectFileFlag())) ? true : false;
                    ANKER_LOG_INFO << "bChangePrinter is " << p->bChangedPrinter.load() << ",getLoadProjectFileFlag is "
                        << getLoadProjectFileFlag() << ",bUseDefaultFilament is " << bUseDefaultFilament;
                    setLoadProjectFileFlag(false);
                    return bUseDefaultFilament;
                }

                bool bUseDefaultFilament = true;
                for (size_t i = 0; i < filamentPresets.size(); i++) {
                    // if all item of filamentPreset is equal, we should use default filament of printer 
                    if (filamentPresets[i] == filamentPresets[0]) {
                        continue;
                    }
                    else {
                        bUseDefaultFilament = false;
                        break;
                    }
                } 
               
                return bUseDefaultFilament;  
            };

            // define use default filament or filament from preset bundle
            bool bUseDefaultFilament = bNeedUseDefaultFilament();
            if (!bUseDefaultFilament) {
                // use filament from preset bundle
                editFilamentPresets = filamentPresets;
            }
            else {
                // use default filament presets
                editFilamentPresets = defaultFilamentPresets;
            }
            
            p->bChangedPrinter.store(false);

            size_t i = 0;
            while (i < numExtruders) {
                std::string presetName = editFilamentPresets[i];
                std::string colorName;
                presetNameSplit(presetName, colorName);
                // mod by allen for  using default filament when printer preset selection is change
                if (bUseDefaultFilament) {
                    updateFilamentColourInCfg(presetName, i, false);
                    // we should update the filament combobox in AnkerConfigDialog
                    filamentInfoChanged();
                }
                   
                SFilamentInfo editFilamentInfo;
                editFilamentInfo.wxStrColorName = colorName;
                editFilamentInfo.wxStrLabelName = presetName;

                // fill label type to extruderInfo
                for (auto iter1 : p->m_filamentInfoList) {
                    for (auto iter2 : iter1.second) {
                        if (iter2.wxStrLabelName == editFilamentInfo.wxStrLabelName) {
                            editFilamentInfo.wxStrLabelType = iter1.first;
                            // fix the problem of inconsistent color and name of filaments
                            editFilamentInfo.wxStrColor = iter2.wxStrColor;
                            // add by allen for add anker_filament_id and anker_colour_id
                            editFilamentInfo.wxStrFilamentId = iter2.wxStrFilamentId;
                            editFilamentInfo.wxStrColorId = iter2.wxStrColorId;
                            // add by allen for add filament_type
                            editFilamentInfo.wxStrLabelType = iter2.wxStrLabelType;
                            break;
                        }
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(p->editFilamentMtx);
                    // we should clear p->m_editFilamentInfoList first before emplace_back
                    if (0 == i) {
                        p->m_editFilamentInfoList.clear();
                    }
                    p->m_editFilamentInfoList.emplace_back(editFilamentInfo);
                }
                ++i;
            }
        }

        void AnkerSidebarNew::getAllFilamentFromCfg() {
            p->m_filamentInfoList.clear();

            const std::deque<Preset>& presets = wxGetApp().preset_bundle->filaments.get_presets();
            std::deque<Preset> filamentPresets;
            size_t defaultPresetsNum = wxGetApp().preset_bundle->filaments.num_default_presets();
            for (size_t i = presets.front().is_visible ? 0 : defaultPresetsNum; i < presets.size(); ++i) {
                const Preset& preset = presets[i];
                if (!preset.is_visible || !preset.is_compatible)
                    continue;
                // We not only support the system presets of the filament , but also support the users presets of the filament ,mod by allen.
                if (preset.is_default) {
                    continue;
                }

                if (!preset.vendor || !preset.vendor->templates_profile) {
                    SFilamentInfo filamentInfo;
                    filamentInfo.wxStrColor = preset.config.opt_string("filament_colour", (unsigned int)0);
                    // add by allen for add anker_filament_id and anker_colour_id
                    filamentInfo.wxStrFilamentId = preset.config.opt_string("anker_filament_id", (unsigned int)0);
                    filamentInfo.wxStrColorId = preset.config.opt_string("anker_colour_id", (unsigned int)0);
                    // add by allen for filament_type
                    std::string filamentType = preset.config.opt_string("filament_type", (unsigned int)0);
                    std::string colorName;
                    // only system filament can get colorName from presetname
                    if(preset.is_system) presetNameSplit(preset.name, colorName);
                    //colorName may be empty,use preset.name instead
                    filamentInfo.wxStrColorName = colorName.empty() ? preset.name : colorName;
                    filamentInfo.wxStrLabelType = filamentType;
                    filamentInfo.wxStrLabelName = preset.name;
                    filamentInfo.filamentSrcType = preset.is_system ? systemFilamentType : userFilamentType;
                    auto iter = p->m_filamentInfoList.find(filamentInfo.wxStrLabelType);
                    if (iter == p->m_filamentInfoList.end()) {
                        std::vector<SFilamentInfo> filamentInfoVec(1, filamentInfo);
                        p->m_filamentInfoList.insert(std::make_pair(filamentInfo.wxStrLabelType, filamentInfoVec));
                    }
                    else {
                        iter->second.emplace_back(filamentInfo);
                    }
                }
            }
        }

        void AnkerSidebarNew::presetNameSplit(const std::string presetName, std::string& colorName) {
                size_t nFirstPos = presetName.find_first_of("(");
                if (nFirstPos == std::string::npos) {
                    return;
                }

                size_t nLastPos = presetName.find_first_of(")");
                if (nLastPos == std::string::npos) {
                    colorName = presetName;
                    return;
                }
                
                colorName = presetName.substr(nFirstPos + 1, nLastPos - nFirstPos - 1);
        };

        const std::vector<SFilamentInfo>& AnkerSidebarNew::getEditFilamentList() {
            std::lock_guard<std::mutex> lock(p->editFilamentMtx);
            return p->m_editFilamentInfoList;
        }

        const std::map<wxString, std::vector<SFilamentInfo>>& AnkerSidebarNew::getAllFilamentList() {
            return p->m_filamentInfoList;
        }

        void AnkerSidebarNew::createPrinterSiderbar() {
            p->m_printerSizer = new wxBoxSizer(wxHORIZONTAL);

            p->m_printerPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_PRINTGER_HEIGHT), wxTAB_TRAVERSAL);

            p->m_printerPanel->SetBackgroundColour(wxColour("#292A2D"));
            p->m_printerSizer->Add(p->m_printerPanel, 1, wxEXPAND | wxALL, 0);

            p->m_printerPanelSizer = new wxFlexGridSizer(3, 1, 0, 0);
            p->m_printerPanelSizer->SetFlexibleDirection(wxBOTH);
            p->m_printerPanelSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
            p->m_printerPanel->SetSizer(p->m_printerPanelSizer);

            auto textAndbtnSizer = new wxBoxSizer(wxHORIZONTAL);
            textAndbtnSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_PRINTGER_TEXTBTN_SIZER));

            auto* text = new wxStaticText(p->m_printerPanel, wxID_ANY, _L("common_slicepannel_title_printer"));            
            text->SetFont(ANKER_BOLD_FONT_NO_1);

            text->SetForegroundColour(wxColour("#FFFFFF"));
            text->SetBackgroundColour(wxColour("#292A2D"));
            textAndbtnSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
            textAndbtnSizer->Add(text, 0, wxALIGN_CENTER_VERTICAL, 0);
            textAndbtnSizer->AddStretchSpacer(1);
            // add by allen for ankerCfgDlg
            AnkerPlaterPresetComboBox** comboPrinter = &p->m_comboPrinter;
            *comboPrinter = new AnkerPlaterPresetComboBox(p->m_printerPanel, Preset::TYPE_PRINTER);
            (*comboPrinter)->Create(p->m_printerPanel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                AnkerSize(SIDEBARNEW_PRINTGER_COMBO_WIDTH, SIDEBARNEW_COMBOBOX_HEIGHT),
                wxNO_BORDER | wxCB_READONLY,
                wxDefaultValidator,
                "");
            (*comboPrinter)->SetBackgroundColour(wxColour("#434447"));
            (*comboPrinter)->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
            (*comboPrinter)->SetFont(ANKER_FONT_NO_1);

//             (*comboPrinter)->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& event) {				
//                 if (p->m_pAnkerParameterPanel)
//                 {
//                     p->m_pAnkerParameterPanel->checkDirtyData();
//                     p->m_pAnkerParameterPanel->reloadData();
//                 }
//                 event.Skip(false);
//                 });
            (*comboPrinter)->set_button_clicked_function([this, comboPrinter]() {
				if (!comboPrinter)
					return;
				ANKER_LOG_INFO << "preset combox of printer clicked";
                (*comboPrinter)->SetFocus();
				if (p->m_pAnkerParameterPanel)
				{
					if(!p->m_pAnkerParameterPanel->checkDirtyData())
                        onComboBoxClick(*comboPrinter);
					//p->m_pAnkerParameterPanel->reloadData();
				}

				
				});
            (*comboPrinter)->set_selection_changed_function([this, comboPrinter](int selection) {
                if (!comboPrinter)
                    return;
				ANKER_LOG_INFO << "preset combox of printer clicked";
				onPresetComboSelChanged(*comboPrinter, selection);
				});

            wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
            btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
            wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
            wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));

            (*comboPrinter)->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
            if ((*comboPrinter)->edit_btn) {
                textAndbtnSizer->Add((*comboPrinter)->edit_btn, 0, wxALIGN_CENTER_VERTICAL, 0);
            }
            textAndbtnSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));

            textAndbtnSizer->ShowItems(true);
            
            p->m_printerPanelSizer->Add(textAndbtnSizer, 1, wxALL | wxEXPAND, 0);

            wxPanel* pLine = new wxPanel(p->m_printerPanel, wxID_ANY, wxDefaultPosition, 
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
            pLine->SetBackgroundColour(wxColour("#38393C"));

            p->m_printerPanelSizer->Add(pLine, 0, wxEXPAND | wxALL, 0);
            p->m_printerPanelSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_COMBO_VER_SPAN));
            p->m_printerPanelSizer->Add(*comboPrinter, 0,
                wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
        }

        void AnkerSidebarNew::createFilamentSidebar() {
            p->m_filamentSizer = new wxBoxSizer(wxHORIZONTAL);
            p->m_filamentPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_HEIGHT), wxTAB_TRAVERSAL);

            p->m_filamentPanel->SetBackgroundColour(wxColour("#292A2D"));
            p->m_filamentSizer->Add(p->m_filamentPanel, 1, wxEXPAND | wxALL, 0);
            p->m_filamentPanelSizer = new wxFlexGridSizer(3, 1, 1, 2);
            p->m_filamentPanelSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_HEIGHT));

            p->m_filamentPanel->SetSizer(p->m_filamentPanelSizer);

            auto textAndbtnSizer = new wxBoxSizer(wxHORIZONTAL);
            textAndbtnSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_TEXTBTN_SIZER));

            auto* text = new wxStaticText(p->m_filamentPanel, wxID_ANY, _L("common_slicepannel_title_filament"));
            text->SetFont(ANKER_BOLD_FONT_NO_1);

            text->SetForegroundColour(wxColour("#FFFFFF"));
            text->SetBackgroundColour(wxColour("#292A2D"));
            textAndbtnSizer->AddSpacer(AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));
            textAndbtnSizer->Add(text, 0, wxALIGN_CENTER_VERTICAL, 0);
            textAndbtnSizer->AddStretchSpacer(1);
            textAndbtnSizer->Add(0, 0, 1, wxEXPAND, 5);

            // List toggle button only show when have multi color
            p->m_filamentListToggleBtn = new ScalableButton(p->m_filamentPanel, wxID_ANY, "filamentListToggle", "");
            p->m_filamentListToggleBtn->SetToolTip(_L("common_slicepannel_hover_toggle"));
            p->m_filamentListToggleBtn->Bind(wxEVT_BUTTON, &AnkerSidebarNew::onFilamentListToggle, this);
            textAndbtnSizer->Add(p->m_filamentListToggleBtn, 0, wxTop | wxRIGHT, AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));

            // filament sync button only show when have multi color
            p->m_filamentSyncBtn = new ScalableButton(p->m_filamentPanel, wxID_ANY, "filamentSync", "");
            p->m_filamentSyncBtn->SetToolTip(_L("common_slicepannel_hover_sync"));
            p->m_filamentSyncBtn->Bind(wxEVT_BUTTON, &AnkerSidebarNew::onSyncFilamentInfo, this);

            textAndbtnSizer->Add(p->m_filamentSyncBtn, 0, wxTop | wxRIGHT, AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));

            // combo setting open btn show always
            ScalableButton* filamentEditBtn = new ScalableButton(p->m_filamentPanel, wxID_ANY, "cog", "");
            filamentEditBtn->SetMaxSize(AnkerSize(25, 32));
            filamentEditBtn->SetSize(AnkerSize(25, 32));

            filamentEditBtn->SetToolTip(_L("common_slicepannel_hover_presetsetting"));
            filamentEditBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent) {
#if SHOW_OLD_SETTING_DIALOG
                Tab* tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT);
                if (!tab)
                    return;

                if (int page_id = wxGetApp().tab_panel()->FindPage(tab); page_id != wxNOT_FOUND)
                {
                    wxGetApp().tab_panel()->SetSelection(page_id);
                    // Switch to Settings NotePad
                    wxGetApp().mainframe->select_tab();
                }
#endif
                
                // add by allen for ankerCfgDlg
                AnkerTab* ankerTab = wxGetApp().getAnkerTab(Preset::TYPE_FILAMENT);
                if (!ankerTab)
                    return;

                if (int page_id = wxGetApp().ankerTabPanel()->FindPage(ankerTab); page_id != wxNOT_FOUND)
                {
                    wxGetApp().ankerTabPanel()->SetSelection(page_id);
                    // Switch to Settings NotePad
                    wxGetApp().mainframe->selectAnkerTab(ankerTab);
                    // show AnkerConfigDilog
                    wxGetApp().mainframe->showAnkerCfgDlg();
                }
                });
            textAndbtnSizer->Add(filamentEditBtn, 0, wxTop | wxRIGHT, AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));

            p->m_filamentPanelSizer->Add(textAndbtnSizer, 1, wxALL | wxEXPAND, 0);

            // separate line
            wxPanel* pLine = new wxPanel(p->m_filamentPanel, wxID_ANY, wxDefaultPosition, AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
            pLine->SetBackgroundColour(wxColour("#38393C"));

            p->m_filamentPanelSizer->Add(pLine, 1, wxEXPAND | wxTop, SIDEBARNEW_FILAMENT_VER_SPAN);

            // update filament info
            updateFilamentInfo();
        }

        void AnkerSidebarNew::createPrintParameterSidebar()
        {

            if (!p->m_pParameterVSizer)
                p->m_pParameterVSizer = new wxBoxSizer(wxVERTICAL);

            if (!p->m_pAnkerParameterPanel)
            {   
                p->m_pAnkerParameterPanel = new AnkerParameterPanel(this, wxID_ANY);
                p->m_pAnkerParameterPanel->Bind(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, [this](wxCommandEvent& event) {
                    wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED);
                    evt.SetEventObject(this);                    
                    wxPostEvent(this, evt);
                    });
                p->m_pAnkerParameterPanel->Bind(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, [this](wxCommandEvent& event) {
                    wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED);
                    evt.SetEventObject(this);                    
                    wxPostEvent(this, evt);
                    });
				p->m_pAnkerParameterPanel->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, [this](wxCommandEvent& event) {                    
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS);
					evt.SetEventObject(this);					
                    wxPostEvent(this, evt);
					});
                p->m_pAnkerParameterPanel->SetBackgroundColour(wxColour("#292A2D"));
                p->m_pAnkerParameterPanel->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
                p->m_pAnkerParameterPanel->SetSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
                p->m_pParameterVSizer->Add(p->m_pAnkerParameterPanel, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
                p->m_pParameterVSizer->AddSpacer(4);
            }
        }

		void AnkerSidebarNew::createRightMenuPrintParameterSidebar()
		{
			//if (!p->m_pRightMenuParameterVSizer)
			//	p->m_pRightMenuParameterVSizer = new wxBoxSizer(wxVERTICAL);

			if (!p->m_pAnkerRightMenuParameterPanel)
			{
				p->m_pAnkerRightMenuParameterPanel = new AnkerParameterPanel(this, wxID_ANY);
                p->m_pAnkerRightMenuParameterPanel->showRightParameterpanel();
				p->m_pAnkerRightMenuParameterPanel->Bind(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, [this](wxCommandEvent& event) {
                    this->Freeze();
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT);
                    evt.SetEventObject(this);
                    wxPostEvent(this, evt); 

                    m_showRightMenuPanel = PANEL_PARAMETER_PANEL;
                    p->m_pAnkerRightMenuParameterPanel->Hide();
                    p->m_pAnkerParameterPanel->Show();
                    Refresh();
                    Layout();
                    this->Thaw();
                    });

                p->m_pAnkerRightMenuParameterPanel->Bind(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, [this](wxCommandEvent& event) {
                    this->Freeze();
                    wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL);
                    evt.SetEventObject(this);
                    wxPostEvent(this, evt);

                    m_showRightMenuPanel = PANEL_PARAMETER_PANEL;
                    p->m_pAnkerRightMenuParameterPanel->Hide();
                    p->m_pAnkerParameterPanel->Show();
                    Refresh();
                    Layout();		
                    this->Thaw();
                    });

				p->m_pAnkerRightMenuParameterPanel->SetBackgroundColour(wxColour("#292A2D"));
				p->m_pAnkerRightMenuParameterPanel->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
				p->m_pAnkerRightMenuParameterPanel->SetSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
				p->m_pParameterVSizer->Add(p->m_pAnkerRightMenuParameterPanel, wxEXPAND|wxALL, wxEXPAND | wxALL, 0);
				p->m_pParameterVSizer->AddSpacer(4);
			}
		}

        void AnkerSidebarNew::onFilamentListToggle(wxEvent& event) {
            p->m_filamentViewType = (p->m_filamentViewType == multiColorVerView) ? multiColorHorView : multiColorVerView;
            updateFilamentInfo(true);
        }

        void AnkerSidebarNew::updateFilamentInfo(bool bDestoryPanel) {
            this->Freeze();
            if (p->m_filamentViewType == singleColorView) {
                if (bDestoryPanel && p->m_singleFilamentPanel) {
                    auto* panel = p->m_singleFilamentPanel;
                    p->m_singleFilamentPanel = nullptr;

                    CallAfter([=] {
                        panel->Destroy();
                        });
                }
                onHorSingleColorFilament();
            }
            else if (p->m_filamentViewType == multiColorVerView) {
                if (bDestoryPanel && p->m_verFilamentPanel) {
                    auto* panel = p->m_verFilamentPanel;
                    p->m_verFilamentPanel = nullptr;

                    CallAfter([=] {
                        panel->Destroy();
                        });
                }
                onVerMultiColorFilament();
            }
            else {
                if (bDestoryPanel && p->m_horFilamentPanel) {
                    auto* panel = p->m_horFilamentPanel;
                    p->m_horFilamentPanel = nullptr;

                    CallAfter([=] {
                        panel->Destroy();
                        });

                }
                onHorMultiColorFilament();
            }
#ifdef __APPLE__
            // need call Layout manually to refresh in macos
            if (bDestoryPanel) {
                Layout();
            }
#endif
            this->Thaw();
        }

        void AnkerSidebarNew::updateFilamentBtnColor(SFilamentEditInfo* filamentEditInfo, wxButton* editColorBtn) {
           if (editColorBtn && filamentEditInfo) {
               editColorBtn->SetBackgroundColour(wxColour(filamentEditInfo->filamentInfo.wxStrColor));
               wxColour foreColor = wxColour(255, 255, 255);
               if (wxColour(filamentEditInfo->filamentInfo.wxStrColor).GetLuminance() > 0.6)
                   foreColor = wxColour(0, 0, 0);
               editColorBtn->SetForegroundColour(foreColor);

               editColorBtn->SetToolTip(filamentEditInfo->filamentInfo.wxStrColorName);

               wxWindowList& siblings = editColorBtn->GetParent()->GetChildren();
               for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
               {
                   wxStaticText* colorBtnText = dynamic_cast<wxStaticText*>(*it);
                   if (colorBtnText) {
                       wxString wxStrTitle = wxString::FromUTF8(
                           filamentEditInfo->filamentInfo.wxStrLabelName.ToStdString());
                       if (wxStrTitle.size() > 30)
                           wxStrTitle = wxStrTitle.substr(0, 20) + "..." + wxStrTitle.substr(wxStrTitle.size() - 10);
                       colorBtnText->SetLabelText(wxStrTitle);
                       break;
                   }
               }

               editColorBtn->Refresh();
           }
        }

        bool AnkerSidebarNew::isMultiFilament() {
            std::lock_guard<std::mutex> lock(p->editFilamentMtx);
            return p->m_editFilamentInfoList.size() > 1;
        }

        void AnkerSidebarNew::onMove(wxMoveEvent& evt) {
            updateFilamentEditDlgPos();
            evt.Skip();
        }

        void AnkerSidebarNew::onSize(wxSizeEvent& evt) {
            updateFilamentEditDlgPos();
            evt.Skip();
        }
        
        void AnkerSidebarNew::onMinimize(wxIconizeEvent& evt) {
            closeFilamentEditDlg();

            evt.Skip();
        }
        void AnkerSidebarNew::onMaximize(wxMaximizeEvent& evt) {
            // When maximized will not follow the main window movement, currently hidden first, subsequent optimization
            closeFilamentEditDlg();

            evt.Skip();
        }

        void  AnkerSidebarNew::shutdown() {
            if (p->m_filamentEditDlg) {
                p->m_filamentEditDlg->Show(false);
                p->m_filamentEditDlg->Destroy();
            }
            // for switching multi language
            p->m_bSidebarFirstInit.store(true);
        }

        void AnkerSidebarNew::updateFilamentEditDlgPos() {
            wxPoint pos = p->m_filamentPanel->GetScreenPosition();

            if (p->m_filamentEditDlg) {
                pos = wxPoint(pos.x - p->m_filamentEditDlg->GetSize().x, pos.y);
                p->m_filamentEditDlg->SetPosition(wxPoint(pos.x - 8, pos.y));
                p->m_filamentEditDlg->Refresh();
            }
        }

        void AnkerSidebarNew::closeFilamentEditDlg() {
            if (p->m_filamentEditDlg) {
                ANKER_LOG_INFO << "closeFilamentEditDlg enter, will be close";
                p->m_filamentEditDlg->Hide();
                p->m_filamentEditDlg->Destroy();
                p->m_filamentEditDlg = nullptr;
            }
        }

        void AnkerSidebarNew::syncFilamentInfo(const std::vector<SFilamentInfo> syncFilamentInfo) {
            // update p->m_editFilamentInfoList
            if (syncFilamentInfo.size() <= 0) {
                ANKER_LOG_ERROR << "syncFilamentInfo failed, filament info from sync is empty";
                return;
            }
               
            {
                std::lock_guard<std::mutex> lock(p->editFilamentMtx);
                std::vector<SFilamentInfo> tempEditFilamentInfoList = p->m_editFilamentInfoList;
                p->m_editFilamentInfoList.clear();

                for (auto syncFilamentIter : syncFilamentInfo) {
                    ANKER_LOG_INFO << "syncFilamentInfo enter, filamentId=" << syncFilamentIter.wxStrFilamentId.ToStdString()
                        << ", colorId=" << syncFilamentIter.wxStrColorId.ToStdString();
                    bool bFind = false;
                    for (auto mapIter : p->m_filamentInfoList) {
                        for (auto allFilamentIter : mapIter.second) {
                            if (syncFilamentIter.wxStrFilamentId == allFilamentIter.wxStrFilamentId
                                && syncFilamentIter.wxStrColorId == allFilamentIter.wxStrColorId
                                && allFilamentIter.filamentSrcType == systemFilamentType) {
                                p->m_editFilamentInfoList.emplace_back(allFilamentIter);
                                bFind = true;
                            }
                        }
                    }

                    if (!bFind) {
                        ANKER_LOG_ERROR << "syncFilamentInfo enter, but cannot find filament which filamentId=" << syncFilamentIter.wxStrFilamentId.ToStdString()
                            << ", colorId=" << syncFilamentIter.wxStrColorId.ToStdString();
                    }
                }

                if (p->m_editFilamentInfoList.size() <= 0) {
                    ANKER_LOG_ERROR << "syncFilamentInfo failed, have no valid filament info from sync";
                    p->m_editFilamentInfoList = tempEditFilamentInfoList;
                }
            }

            // we should update filament info when we do filament sync operation;
            updateFilamentInfo(true);

            // we should update colour cfg
            for (int i = 0; i < p->m_editFilamentInfoList.size(); i++) {
                updateFilamentColourInCfg(p->m_editFilamentInfoList[i].wxStrLabelName.ToStdString(), 
                    i, false);
            }

            // update filament colour info  for object list and clour
            filamentInfoChanged();
        }

        // // add by allen for ankerCfgDlg search
        void AnkerSidebarNew::check_and_update_searcher(bool respect_mode /*= false*/)
        {
            std::vector<Search::InputInfo> search_inputs{};

            auto& tabs_list = wxGetApp().ankerTabsList;
            auto print_tech = wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology();
            for (auto tab : tabs_list)
                if (tab->supports_printer_technology(print_tech))
                    search_inputs.emplace_back(Search::InputInfo{ tab->get_config(), tab->type() });

            p->searcher.check_and_update(wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology(),
                respect_mode ? m_mode : comExpert, search_inputs);
        }

        Search::OptionsSearcher& AnkerSidebarNew::get_searcher()
        {
            return p->searcher;
        }

        std::string& AnkerSidebarNew::get_search_line()
        {
            // update searcher before show imGui search dialog on the plater, if printer technology or mode was changed
            check_and_update_searcher(true);
            return p->searcher.search_string();
        }

        void AnkerSidebarNew::update_mode()
        {
            m_mode = wxGetApp().get_mode();

            /*  update_reslice_btn_tooltip();
              update_mode_sizer();*/

            wxWindowUpdateLocker noUpdates(this);

            /* if (m_mode == comSimple)
             {
                 p->object_manipulation->set_coordinates_type(ECoordinatesType::World);
             }*/

            //p->object_list->get_sizer()->Show(m_mode > comSimple);

            //p->object_list->unselect_objects();
            //p->object_list->update_selections();

            Layout();
        }

        void AnkerSidebarNew::jump_to_option(const std::string& opt_key, Preset::Type type, const std::wstring& category)
        {
            //const Search::Option& opt = p->searcher.get_option(opt_key, type);
            wxGetApp().getAnkerTab(type)->activate_option(opt_key, category);
        }

        void AnkerSidebarNew::jump_to_option(size_t selected)
        {
            const Search::Option& opt = p->searcher.get_option(selected);
            if (opt.type == Preset::TYPE_PREFERENCES)
                wxGetApp().open_preferences(opt.opt_key(), boost::nowide::narrow(opt.group));
            else
                wxGetApp().getAnkerTab(opt.type)->activate_option(opt.opt_key(), boost::nowide::narrow(opt.category));
        }

        void AnkerSidebarNew::search()
        {
            p->searcher.search();
        }

        void AnkerSidebarNew::getItemList(wxStringList& list, ControlListType listType)
        {
             p->m_pAnkerParameterPanel->getItemList(list, listType);
        }

        void AnkerSidebarNew::setItemValue(const wxString tabName, const wxString& widgetLabel, wxVariant data)
        {
            p->m_pAnkerParameterPanel->setItemValue(tabName, widgetLabel, data);
        }

        void AnkerSidebarNew::openSupportMaterialPage(wxString itemName, wxString text)
        {
            p->m_pAnkerParameterPanel->openSupportMaterialPage(itemName, text);
        }


        void AnkerSidebarNew::setLoadProjectFileFlag(bool bFlag) {
            m_bLoadProjectFile.store(bFlag);
        }
        
        bool AnkerSidebarNew::getLoadProjectFileFlag() {
            return m_bLoadProjectFile.load();
        }

        void AnkerSidebarNew::modifyExtrudersInfo(const std::vector<SFilamentEditInfo> filamentInfoList, 
            bool bRemove, int removeIndex) {

            DynamicPrintConfig* cfg = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            DynamicPrintConfig cfg_new = *cfg;
            ConfigOptionStrings* colors = static_cast<ConfigOptionStrings*>(cfg->option("extruder_colour")->clone());
            ConfigOptionFloats* nozzleDiameters = dynamic_cast<ConfigOptionFloats*>(cfg->option("nozzle_diameter")->clone());
            if (bRemove) {
                std::vector<std::string> vecColors;
                std::vector<double> vecNozzleDiameters;
                for (int i = 0; i < filamentInfoList.size(); i++) {
                    vecColors.emplace_back(filamentInfoList[i].filamentInfo.wxStrColor.ToStdString());
                }
                cfg_new.set_key_value("extruder_colour", new ConfigOptionStrings(vecColors));


                for (int i = 0; i < nozzleDiameters->values.size(); i++) {
                    if (i == removeIndex) {
                        continue;
                    }
                    vecNozzleDiameters.emplace_back(nozzleDiameters->values[i]);
                }
                cfg_new.set_key_value("nozzle_diameter", new ConfigOptionFloats(vecNozzleDiameters));
            }
            else {
                colors->values[filamentInfoList[0].editIndex - 1] = filamentInfoList[0].filamentInfo.wxStrColor.ToStdString();
                cfg_new.set_key_value("extruder_colour", colors);
            }
#if SHOW_OLD_SETTING_DIALOG
            wxGetApp().get_tab(Preset::TYPE_FILAMENT)->load_config(cfg_new);
#endif
            wxGetApp().getAnkerTab(Preset::TYPE_FILAMENT)->load_config(cfg_new);

            wxGetApp().plater()->on_config_change(cfg_new);

            // update filament layout
            updateFilamentInfo(true);
        }

        void AnkerSidebarNew::singleColorFilamentHorInit() {
            std::lock_guard<std::mutex> lock(p->editFilamentMtx);
            // mod by allen for filament hover
            p->m_singleFilamentPanel = new wxPanel(p->m_filamentPanel, wxID_ANY, wxDefaultPosition, 
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_PANEL_HEIGHT));
            auto* filamentBtnSizer = new wxBoxSizer(wxHORIZONTAL);
            filamentBtnSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_PANEL_HEIGHT));
            p->m_singleFilamentPanel->SetSizer(filamentBtnSizer);

            // hide sync and edit btn
            p->m_filamentListToggleBtn->Show(false);
            p->m_filamentSyncBtn->Show(false);

            // horizontalSizer
            wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
            horizontalSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 
                SIDEBARNEW_FILAMENT_PANEL_HEIGHT));

            // btn
            auto* colorBtn = new wxButton(p->m_singleFilamentPanel, ID_FILAMENT_COLOUR_BTN, wxT(""), wxDefaultPosition,
                AnkerSize(COLOUR_BTN_SIZE, COLOUR_BTN_SIZE), wxBORDER_NONE);
            updateFilamentColorBtnPtr(colorBtn);
            // right mouse click
            colorBtn->Bind(wxEVT_RIGHT_DOWN, &AnkerSidebarNew::onFilamentMenuOpen, this);
            // left mouse click
            colorBtn->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
            // left mouse double click
            colorBtn->Bind(wxEVT_LEFT_DCLICK, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
            colorBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_HAND));
                });

            colorBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_NONE));
                });

            colorBtn->SetToolTip(_L(p->m_editFilamentInfoList[0].wxStrColorName));
            wxColour foreColor = wxColour(255, 255, 255);
            if (wxColour(p->m_editFilamentInfoList[0].wxStrColor).GetLuminance() > 0.6)
                foreColor = wxColour(0, 0, 0);
            colorBtn->SetForegroundColour(foreColor);
            colorBtn->SetBackgroundColour(wxColour(p->m_editFilamentInfoList[0].wxStrColor));

            horizontalSizer->AddSpacer(AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));
            horizontalSizer->Add(colorBtn, 0, wxALIGN_CENTER_VERTICAL, 0);

            horizontalSizer->AddSpacer(6);
            // staticText
            wxString wxStrTitle = wxString::FromUTF8(p->m_editFilamentInfoList[0].wxStrLabelName.ToStdString());
            if (wxStrTitle.size() > 30)
                wxStrTitle = wxStrTitle.substr(0, 20) + "..." + wxStrTitle.substr(wxStrTitle.size() - 10);

            auto* staticText = new wxStaticText(p->m_singleFilamentPanel, wxID_ANY, 
                wxStrTitle, wxDefaultPosition, AnkerSize(270, -1), 0);

            staticText->SetFont(ANKER_FONT_NO_1);
            staticText->SetForegroundColour(wxColour("#FFFFFF"));
            staticText->SetWindowStyle(wxTRANSPARENT_WINDOW);
            // left mouse click
            staticText->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentTextLeftClicked, this);
            // left mouse double click
            staticText->Bind(wxEVT_LEFT_DCLICK, &AnkerSidebarNew::onFilamentTextLeftClicked, this);
            staticText->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_HAND));
                });
            staticText->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_NONE));
                });
            horizontalSizer->Add(staticText, 0, wxALIGN_CENTER_VERTICAL, 0);
            // spacer
            horizontalSizer->AddStretchSpacer(1);
            // btnEdit
            auto* btnEdit = new ScalableButton(p->m_singleFilamentPanel, ID_FILAMENT_EDIT_BTN, "filamentSingleColorEdit", wxEmptyString,
                AnkerSize(COLOUR_BTN_SIZE - 5, COLOUR_BTN_SIZE - 5 ));
            // left mouse click
            btnEdit->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
            horizontalSizer->Add(btnEdit, 0, wxALIGN_CENTER_VERTICAL, 0);
            horizontalSizer->AddSpacer(AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN - 4));
            //btnEdit->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentMenuOpen, this);
            /*filamentBtnSizer->AddSpacer(AnkerLength(SIDEBARNEW_FILAMENT_VER_SPAN));*/
            filamentBtnSizer->Add(horizontalSizer, 1, wxEXPAND | wxALL, 0);
        }

        void AnkerSidebarNew::multiColorFilamentHorInit() {
            std::lock_guard<std::mutex> lock(p->editFilamentMtx);
            //const int horFilamentPanelHeight = 64;
            const int ankerHighlightPanelHeight = COLOUR_BTN_SIZE + 6;  //36;
            // mod by allen for filament hover
            p->m_horFilamentPanel = new wxPanel(p->m_filamentPanel, wxID_ANY, wxDefaultPosition, 
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_PANEL_HEIGHT));
            auto* filamentPanelSizer = new wxBoxSizer(wxHORIZONTAL);
            filamentPanelSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_PANEL_HEIGHT));
            p->m_horFilamentPanel->SetSizer(filamentPanelSizer);

            // show sync and edit btn
            p->m_filamentListToggleBtn->Show(true);
            p->m_filamentSyncBtn->Show(true);

            filamentPanelSizer->AddSpacer(AnkerLength(SIDEBARNEW_FILAMENT_HOR_SPAN));
            for (int i = 0; i < p->m_editFilamentInfoList.size(); i++) {
                // mod by allen for filament hover
                auto horFilamentPanel = new AnkerHighlightPanel(p->m_horFilamentPanel, wxID_ANY, wxDefaultPosition,
                    AnkerSize(ankerHighlightPanelHeight, ankerHighlightPanelHeight));
                auto* filamentBtnSizer = new wxBoxSizer(wxHORIZONTAL);
                horFilamentPanel->SetSizer(filamentBtnSizer);
                auto* colorBtn = new wxButton(horFilamentPanel, ID_FILAMENT_COLOUR_BTN, std::to_string(i + 1),
                    wxDefaultPosition, AnkerSize(COLOUR_BTN_SIZE, COLOUR_BTN_SIZE), wxBORDER_NONE);
                colorBtn->SetFont(ANKER_FONT_NO_2);
                filamentBtnSizer->Add(colorBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 
                    AnkerLength(3));
                wxColour foreColor = wxColour(255, 255, 255);
                if (wxColour(p->m_editFilamentInfoList[i].wxStrColor).GetLuminance() > 0.6)
                    foreColor = wxColour(0, 0, 0);
                colorBtn->SetForegroundColour(foreColor);
                colorBtn->SetBackgroundColour(wxColour(p->m_editFilamentInfoList[i].wxStrColor));
                colorBtn->SetToolTip(_L(p->m_editFilamentInfoList[i].wxStrLabelName));
                // right mouse click
                colorBtn->Bind(wxEVT_RIGHT_DOWN, &AnkerSidebarNew::onFilamentMenuOpen, this);
                // left mouse click
                colorBtn->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
                // left mouse double click
                colorBtn->Bind(wxEVT_LEFT_DCLICK, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
                colorBtn->Bind(wxEVT_ENTER_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_HAND));
                    horFilamentPanel->setMouseEnterStatus(true);
                    });
                colorBtn->Bind(wxEVT_LEAVE_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                    SetCursor(wxCursor(wxCURSOR_NONE));
                    horFilamentPanel->setMouseEnterStatus(false);
                    });
                filamentPanelSizer->Add(horFilamentPanel, 0, wxTOP | wxBOTTOM, 
                    AnkerLength((SIDEBARNEW_FILAMENT_PANEL_HEIGHT - ankerHighlightPanelHeight) / 2));
                filamentPanelSizer->AddSpacer(13);

                // highlight the selected color btn
                if (i + 1 == p->m_selectdColorBtnIndex) {
                    horFilamentPanel->setMouseClickStatus();
                    updateFilamentColorBtnPtr(colorBtn);
                }
            }
            filamentPanelSizer->AddStretchSpacer(1);

            p->m_horFilamentPanel->Show(false);
        }

        void AnkerSidebarNew::multiColorFilamentVerInit() {
            std::lock_guard<std::mutex> lock(p->editFilamentMtx);
            p->m_verFilamentPanel = new wxPanel(p->m_filamentPanel, wxID_ANY, wxDefaultPosition, 
                AnkerSize(SIDEBARNEW_WIDGET_WIDTH, p->m_editFilamentInfoList.size() * 43));
            auto* filamentBtnSizer = new wxBoxSizer(wxVERTICAL);
            p->m_verFilamentPanel->SetSizer(filamentBtnSizer);

            // show sync and edit btn
            p->m_filamentListToggleBtn->Show(true);
            p->m_filamentSyncBtn->Show(true);
          
            for (int i = 0; i < p->m_editFilamentInfoList.size(); i++) {
                // mod by allen for filament hover
                auto horFilamentPanel = new AnkerHighlightPanel(p->m_verFilamentPanel, wxID_ANY, wxDefaultPosition,
                    AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 42));

                // horizontalSizer
                wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
                horizontalSizer->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 42));
                horFilamentPanel->SetSizer(horizontalSizer);

                horizontalSizer->AddSpacer(15);
                // btn
                wxString strBtnText = wxString::Format(wxT("%d"), i + 1);
                auto* colorBtn = new wxButton(horFilamentPanel, ID_FILAMENT_COLOUR_BTN, strBtnText, wxDefaultPosition,
                    AnkerSize(COLOUR_BTN_SIZE, COLOUR_BTN_SIZE), wxBORDER_NONE);
                colorBtn->SetBackgroundColour(wxColour(p->m_editFilamentInfoList[i].wxStrColor));
                horizontalSizer->Add(colorBtn, 0, wxALIGN_CENTER_VERTICAL, 0);
                wxColour foreColor = wxColour(255, 255, 255);
                if (wxColour(p->m_editFilamentInfoList[i].wxStrColor).GetLuminance() > 0.6)
                    foreColor = wxColour(0, 0, 0);
                colorBtn->SetForegroundColour(foreColor);
                // right mouse click
                colorBtn->Bind(wxEVT_RIGHT_DOWN, &AnkerSidebarNew::onFilamentMenuOpen, this);
                // left mouse click
                colorBtn->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
                // left mouse double click
                colorBtn->Bind(wxEVT_LEFT_DCLICK, &AnkerSidebarNew::onFilamentBtnLeftClicked, this);
                colorBtn->Bind(wxEVT_ENTER_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                         SetCursor(wxCursor(wxCURSOR_HAND));
                         horFilamentPanel->setMouseEnterStatus(true);
                    });
                colorBtn->Bind(wxEVT_LEAVE_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                         SetCursor(wxCursor(wxCURSOR_NONE));
                        horFilamentPanel->setMouseEnterStatus(false);
                    });
                colorBtn->SetToolTip(_L(p->m_editFilamentInfoList[i].wxStrLabelName));
                horizontalSizer->AddSpacer(10);
                // staticText1
                wxString wxStrTitle = wxString::FromUTF8(p->m_editFilamentInfoList[i].wxStrLabelName.ToStdString());
                if (wxStrTitle.size() > 30)
                    wxStrTitle = wxStrTitle.substr(0, 20) + "..." + wxStrTitle.substr(wxStrTitle.size() - 10);
                auto* staticText = new wxStaticText(horFilamentPanel, wxID_ANY,
                    wxStrTitle, wxDefaultPosition, AnkerSize(270, -1), 0);
                staticText->SetFont(ANKER_FONT_NO_1);
                staticText->Wrap(-1);
                staticText->SetForegroundColour(wxColour("#FFFFFF"));
                // left mouse click
                staticText->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentTextLeftClicked, this);
                // left mouse double click
                staticText->Bind(wxEVT_LEFT_DCLICK, &AnkerSidebarNew::onFilamentTextLeftClicked, this);
                staticText->Bind(wxEVT_ENTER_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                        SetCursor(wxCursor(wxCURSOR_HAND));
                        horFilamentPanel->setMouseEnterStatus(true);
                    });
                staticText->Bind(wxEVT_LEAVE_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                        SetCursor(wxCursor(wxCURSOR_NONE));
                    });
                horizontalSizer->Add(staticText, 0, wxALIGN_CENTER_VERTICAL , 0);
                // spacer
                horizontalSizer->AddStretchSpacer(1);
                // btnEdit
                ScalableButton* btnEdit = new ScalableButton(horFilamentPanel, wxID_ANY, "filamentMultiColorEdit", "");
                horizontalSizer->Add(btnEdit, 0, wxALIGN_CENTER_VERTICAL, 0);
                btnEdit->Bind(wxEVT_LEFT_DOWN, &AnkerSidebarNew::onFilamentMenuOpen, this);
                btnEdit->Bind(wxEVT_ENTER_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                         SetCursor(wxCursor(wxCURSOR_HAND));
                        horFilamentPanel->setMouseEnterStatus(true);
                    });
                btnEdit->Bind(wxEVT_LEAVE_WINDOW, [this, horFilamentPanel](wxMouseEvent& event) {
                        SetCursor(wxCursor(wxCURSOR_NONE));
                        horFilamentPanel->setMouseEnterStatus(false);
                    });
                horizontalSizer->AddSpacer(20);
                filamentBtnSizer->Add(horFilamentPanel, 1, wxEXPAND | wxALL, 0);

                // line
                auto* line = new wxPanel(p->m_verFilamentPanel, wxID_ANY, wxDefaultPosition, AnkerSize(1000, 1), wxTAB_TRAVERSAL);
                line->SetMinSize(AnkerSize(1000, 1));
                line->SetBackgroundColour(wxColour(56, 57, 60));
                filamentBtnSizer->Add(line, 1, wxLEFT | wxRIGHT | wxEXPAND, 5);

                // highlight the selected color btn
                if (i+ 1 == p->m_selectdColorBtnIndex) {
                    horFilamentPanel->setMouseClickStatus();
                    updateFilamentColorBtnPtr(colorBtn);
                }
            }
            p->m_verFilamentPanel->Show(false);
        }

        // SingleColor horizontal filament list
        void AnkerSidebarNew::onHorSingleColorFilament() {
            if (!p->m_singleFilamentPanel) {
                singleColorFilamentHorInit();
            }

            p->m_filamentPanel->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 
                SIDEBARNEW_FILAMENT_HEIGHT));

            if (p->m_filamentPanelSizer->GetItemCount() >= 3) {
                wxSizerItem* sizeItem = p->m_filamentPanelSizer->GetItem(2);
                sizeItem->Show(false);
                p->m_filamentPanelSizer->Detach(2);
            }

            p->m_filamentPanelSizer->Add(p->m_singleFilamentPanel, wxEXPAND | wxALL, 0);
            p->m_singleFilamentPanel->Show(true);

            refreshSideBarLayout();
        }

        //MultiColor horizontal filament list
        void AnkerSidebarNew::onHorMultiColorFilament() {
            if (!p->m_horFilamentPanel) {
                multiColorFilamentHorInit();
            }

            p->m_filamentPanel->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, SIDEBARNEW_FILAMENT_HEIGHT));

            if (p->m_filamentPanelSizer->GetItemCount() >= 3) {
                wxSizerItem* sizeItem = p->m_filamentPanelSizer->GetItem(2);
                sizeItem->Show(false);
                p->m_filamentPanelSizer->Detach(2);
            }

            p->m_filamentPanelSizer->Add(p->m_horFilamentPanel, wxEXPAND | wxALL, 0);
            p->m_horFilamentPanel->Show(true);
#ifdef __APPLE__
            p->m_horFilamentPanel->Layout();
#endif
            refreshSideBarLayout();
        }

        // MultiColor vertical filament list
        void AnkerSidebarNew::onVerMultiColorFilament() {
            if (!p->m_verFilamentPanel) {
                multiColorFilamentVerInit();
            }

            p->m_filamentPanel->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, p->m_editFilamentInfoList.size() * 43 + 46));

            if (p->m_filamentPanelSizer->GetItemCount() >= 3) {
                wxSizerItem* sizeItem = p->m_filamentPanelSizer->GetItem(2);
                sizeItem->Show(false);
                p->m_filamentPanelSizer->Detach(2);
            }

            p->m_filamentPanelSizer->Add(p->m_verFilamentPanel, wxEXPAND | wxALL, 0);
            p->m_verFilamentPanel->Show(true);
#ifdef __APPLE__
            p->m_verFilamentPanel->Layout();
#endif
            refreshSideBarLayout();
        }

        void AnkerSidebarNew::onFilamentMenuOpen(wxMouseEvent& event) {
            //wxButton* btn = (wxButton*)(event.m_callbackUserData);
            wxButton* btn = dynamic_cast<wxButton*>(event.GetEventObject());
            AnkerHighlightPanel* parentPanel = (AnkerHighlightPanel*)(btn->GetParent());

            uint32_t colorBtnIndex = 0;
            wxButton* siblingBtn = nullptr;
            // get sibling color btn
            wxWindowList& siblings = parentPanel->GetChildren();
            for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
            {
                siblingBtn = dynamic_cast<wxButton*>(*it);
                if (!siblingBtn)
                    continue;
                if (ID_FILAMENT_COLOUR_BTN == siblingBtn->GetId()) {
                    // indicates this btn is color btn, not edit icon btn
                    siblingBtn->GetLabelText().ToUInt(&colorBtnIndex);
                    break;
                }
            }

            // mod by allen for filament hover, highlight parent panel only in multi colour
            if (parentPanel && isMultiFilament() && !parentPanel->getMouseClickStatus())
                parentPanel->setMouseClickStatus();

            wxPoint Pos;
            btn->GetPosition(&Pos.x, &Pos.y);
            if (!p->m_filamentMenu) {
                p->m_filamentMenu = new wxMenu(wxT(""));
                p->m_filamentMenu->Append(ID_FILAMENT_EDIT, _L("common_slicepannel_filamentedit_title"));
                if (p->m_filamentViewType != singleColorView && p->m_editFilamentInfoList.size() > 1) {
                    p->m_filamentMenu->Append(ID_FILAMENT_REMOVE, _L("common_slicepannel_filament_remove"));
                }
            }

            // for single filament
            if (0 == colorBtnIndex) {
                colorBtnIndex = 1;
            }

            wxDELETE(p->m_filamentMenu);
            p->m_filamentMenu = new wxMenu(wxT(""));
            p->m_filamentMenu->Append(ID_FILAMENT_EDIT, _L("common_slicepannel_filamentedit_title"));
            // only multiFilament has remove menu item
            // and only the last button has remove menu item ; add by allen at 20230625 for product manager
            if (p->m_filamentViewType != singleColorView && p->m_editFilamentInfoList.size() > 1 && 
                colorBtnIndex == p->m_editFilamentInfoList.size() ) {
                p->m_filamentMenu->Append(ID_FILAMENT_REMOVE, _L("common_slicepannel_filament_remove"));
            }
            
            wxVariant eventData;
            eventData.ClearList();
            eventData.Append(wxVariant(siblingBtn));
            p->m_filamentMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &AnkerSidebarNew::onFilamentPopupClick, this, wxID_ANY, wxID_ANY, new wxVariant(eventData));
#ifndef __APPLE__
            const int horMoveStep = 119;
            const int verMoveStep = 10;
#else
            const int horMoveStep = 50;
            const int verMoveStep = 10;
#endif
            parentPanel->PopupMenu(p->m_filamentMenu, wxPoint(Pos.x-horMoveStep, Pos.y+verMoveStep));
        }

        void AnkerSidebarNew::onFilamentBtnLeftClicked(wxMouseEvent& event) {
            //wxButton* btn = (wxButton*)(event.m_callbackUserData);
            wxButton* btn = dynamic_cast<wxButton*>(event.GetEventObject());
            AnkerHighlightPanel* parentPanel = (AnkerHighlightPanel*)(btn->GetParent());

            uint32_t colorBtnIndex = 0;
            wxButton* siblingBtn = nullptr;
            // get sibling color btn
            wxWindowList& siblings = parentPanel->GetChildren();
            for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
            {
                siblingBtn = dynamic_cast<wxButton*>(*it);
                if (!siblingBtn)
                    continue;
                if (ID_FILAMENT_COLOUR_BTN == siblingBtn->GetId()) {
                    // indicates this btn is color btn, not edit icon btn
                    siblingBtn->GetLabelText().ToUInt(&colorBtnIndex);
                    break;
                }
            }

            // for single filament
            if (0 == colorBtnIndex) {
                colorBtnIndex = 1;
            }

            // record selected color btn index
            p->m_selectdColorBtnIndex = colorBtnIndex;
            // left double click on colour btn and static text, left down on edit btn
            if (event.LeftDClick() || event.GetId() == ID_FILAMENT_EDIT_BTN) {
                //  highlight parent panel only in multi colour
                if (parentPanel && isMultiFilament() && !parentPanel->getMouseClickStatus())
                    parentPanel->setMouseClickStatus();

                ANKER_LOG_INFO << "onFilamentBtnLeftClicked enter, is double click, index is " << colorBtnIndex;
                onFilmentEditSelected(colorBtnIndex, siblingBtn);
            }
            else { // event.LeftDown()
                 // highlight parent panel only in multi colour
                if (parentPanel && isMultiFilament() && !parentPanel->getMouseClickStatus())
                    parentPanel->setMouseClickStatus();
            }

            // post wxCUSTOMEVT_CLICK_FILAMENT_BTN event
            ANKER_LOG_INFO << "onFilamentBtnLeftClicked enter, is click, index is " << colorBtnIndex;
            wxString filamentColor = p->m_editFilamentInfoList[colorBtnIndex - 1].wxStrColor;

            wxVariant eventData;
            eventData.ClearList();
            eventData.Append(wxVariant(filamentColor));
            eventData.Append(wxVariant((int)colorBtnIndex));
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_CLICK_FILAMENT_BTN);
            evt.SetClientData(new wxVariant(eventData));
            wxPostEvent(this, evt);
        }

        void AnkerSidebarNew::onFilamentTextLeftClicked(wxMouseEvent& event) {
            wxStaticText* staticText = dynamic_cast<wxStaticText*>(event.GetEventObject());
            // mod by allen for filament hover
            AnkerHighlightPanel* parentPanel = (AnkerHighlightPanel*)(staticText->GetParent());
            uint32_t colorBtnIndex = 0;
            wxButton* siblingBtn = nullptr;
            // get sibling color btn
            wxWindowList& siblings = parentPanel->GetChildren();
            for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
            {
                siblingBtn = dynamic_cast<wxButton*>(*it);
                if (!siblingBtn)
                    continue;
                if (ID_FILAMENT_COLOUR_BTN  == siblingBtn->GetId()) {
                    // indicates this btn is color btn, not edit icon btn
                    siblingBtn->GetLabelText().ToUInt(&colorBtnIndex);
                    break;
                }
            }
  
            // for single filament
            if (0 == colorBtnIndex) {
                colorBtnIndex = 1;
            }
            // record selected color btn index
            p->m_selectdColorBtnIndex = colorBtnIndex;

            if (event.LeftDClick()) {
                // left double click, highlight parent panel only in multi colour
                if (parentPanel && isMultiFilament() && !parentPanel->getMouseClickStatus())
                    parentPanel->setMouseClickStatus();

                ANKER_LOG_INFO << "onFilamentTextLeftClicked enter, is double click, index is " << colorBtnIndex;
                onFilmentEditSelected(colorBtnIndex, siblingBtn);
            }
            else { // event.LeftDown()
                // highlight parent panel only in multi colour
                if (parentPanel && isMultiFilament() && !parentPanel->getMouseClickStatus())
                    parentPanel->setMouseClickStatus();

                ANKER_LOG_INFO << "onFilamentTextLeftClicked enter, is click, index is " << colorBtnIndex;
                wxString filamentColor = p->m_editFilamentInfoList[colorBtnIndex - 1].wxStrColor;

                wxVariant eventData;
                eventData.ClearList();
                eventData.Append(wxVariant(filamentColor));
                eventData.Append(wxVariant((int)colorBtnIndex));
                wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_CLICK_FILAMENT_BTN);
                evt.SetClientData(new wxVariant(eventData));
                wxPostEvent(this, evt);
            }
        }

        void AnkerSidebarNew::onFilamentPopupClick(wxCommandEvent& event) {
            wxVariant* pData = (wxVariant*)(event.m_callbackUserData);
            wxVariantList list = pData->GetList();
            unsigned int btnIndex = 0;
            wxButton* btn = (wxButton*)(list[0]->GetWxObjectPtr());
            btn->GetLabelText().ToUInt(&btnIndex);
            // for single filament
            if (0 == btnIndex) {
                btnIndex = 1;
            }
            if (btnIndex <= 0) {
                ANKER_LOG_ERROR << "onFilamentPopupClick enter, but user data is invalid";
                return;
            }
            switch (event.GetId()) {
            case ID_FILAMENT_EDIT:
                onFilmentEditSelected(btnIndex, btn);
                break;
            case ID_FILAMENT_REMOVE:
                onFilamentRemoveSelected(btnIndex, btn);
                break;
            default:
                break;
            }
        }

        void AnkerSidebarNew::onFilmentEditSelected(const uint32_t btnIndex, wxButton* btn) {
            ANKER_LOG_INFO << "onFilmentEditSelected enter,btn index is " << btnIndex <<
                ",labelName is " << p->m_editFilamentInfoList[btnIndex - 1].wxStrLabelName <<
                ",colorName is " << p->m_editFilamentInfoList[btnIndex - 1].wxStrColor;
            SFilamentEditInfo filamentEditInfo;
            filamentEditInfo.editIndex = btnIndex;
            {
                std::lock_guard<std::mutex> lock(p->editFilamentMtx);
                filamentEditInfo.filamentInfo.wxStrLabelName = p->m_editFilamentInfoList[btnIndex - 1].wxStrLabelName;
                filamentEditInfo.filamentInfo.wxStrColorName = p->m_editFilamentInfoList[btnIndex - 1].wxStrColorName;
                filamentEditInfo.filamentInfo.wxStrColor = p->m_editFilamentInfoList[btnIndex - 1].wxStrColor;
                filamentEditInfo.filamentInfo.wxStrLabelType = p->m_editFilamentInfoList[btnIndex - 1].wxStrLabelType;
            }
            
            if (p->m_filamentEditDlg)
                wxDELETE(p->m_filamentEditDlg);
            p->m_filamentEditDlg = new AnkerFilamentEditDlg(p->m_filamentPanel, _L("common_slicepannel_filamentedit_title").ToStdString(), filamentEditInfo, btn);
            p->m_filamentEditDlg->SetPosition(wxPoint(p->m_filamentEditDlg->GetPosition().x - 8,
                p->m_filamentEditDlg->GetPosition().y));
            
            p->m_filamentEditDlg->Bind(wxCUSTOMEVT_ANKER_FILAMENT_CHANGED, [this](wxCommandEvent&event) {

                if(p->m_pAnkerParameterPanel)
                    p->m_pAnkerParameterPanel->reloadData();				

                });
            p->m_filamentEditDlg->Bind(wxCUSTOMEVT_CHECK_RIGHT_DIRTY_DATA, [this](wxCommandEvent& event) {
                p->m_pAnkerParameterPanel->checkDirtyData();
            });

            p->m_filamentEditDlg->Show();
        }

        void AnkerSidebarNew::onFilamentRemoveSelected(const uint32_t btnIndex, wxButton* btn) {
            ANKER_LOG_INFO << "onFilamentRemoveSelected enter,btn index is " << btnIndex;
            std::vector<SFilamentEditInfo> filamentEditInfoList;
            {
                std::lock_guard<std::mutex> lock(p->editFilamentMtx);
                p->m_editFilamentInfoList.erase(p->m_editFilamentInfoList.begin() + btnIndex - 1);

                for (int i = 0; i < p->m_editFilamentInfoList.size(); i++) {
                    SFilamentEditInfo filamentEditInfo(i, p->m_editFilamentInfoList[i]);
                    filamentEditInfoList.emplace_back(filamentEditInfo);
                }
            }
            
            // update filament_presets and filament_color when remove filament.add by allen at 20230625
            //modifyExtrudersInfo(filamentEditInfoList, true, btnIndex-1);
            wxGetApp().preset_bundle->filament_presets.clear();
            for (auto iter : filamentEditInfoList) {
                wxGetApp().preset_bundle->filament_presets.emplace_back(iter.filamentInfo.wxStrLabelName.ToStdString());
            }
            wxGetApp().plater()->update_filament_colors_in_full_config();

            // update filament label to ini
            updateFilamentInfo(true);

            // post wxCUSTOMEVT_REMOVE_FILAMENT_BTN event
            wxVariant eventData;
            eventData.ClearList();
            eventData.Append(wxVariant((int)btnIndex));
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_REMOVE_FILAMENT_BTN);
            evt.SetClientData(new wxVariant(eventData));
            wxPostEvent(this, evt);

            // update data in mmu
            wxGetApp().plater()->get_current_canvas3D()->get_gizmos_manager().update_data();
        }

        void AnkerSidebarNew::onUpdateFilamentInfo(wxCommandEvent& event) {
            wxVariant* pData = (wxVariant*)event.GetClientData();
            if (pData) {
                wxVariantList list = pData->GetList();
                SFilamentEditInfo* filamentEditInfo = (SFilamentEditInfo*)(list[0]->GetVoidPtr());
                wxButton* editColorBtn = (wxButton*)(list[1]->GetVoidPtr());
                if (!filamentEditInfo) {
                    ANKER_LOG_ERROR << "recv wxCUSTOMEVT_UPDATE_FILAMENT_INFO, filamentEditInfo is invalid.";
                    return;
                }
                int editIndex = filamentEditInfo->editIndex;
                SFilamentInfo filamentInfo = filamentEditInfo->filamentInfo;
                ANKER_LOG_INFO << "recv wxCUSTOMEVT_UPDATE_FILAMENT_INFO,editIndex is " << editIndex << ",wxStrFilamentColor is "
                    << filamentInfo.wxStrColor << ",wxStrColorName is " << filamentInfo.wxStrColorName << ",wxStrLabelName is "
                    << filamentInfo.wxStrLabelName << ",filamentType is " << filamentInfo.wxStrLabelType << ",filamentId is " 
                    << filamentInfo.wxStrFilamentId << ", colourId is " << filamentInfo.wxStrColorId;

                // update m_editFilamentInfoList
                {
                    std::lock_guard<std::mutex> lock(p->editFilamentMtx);
                    p->m_editFilamentInfoList[editIndex - 1] = filamentEditInfo->filamentInfo;
                }

                // update filament colour to ini
                updateFilamentColourInCfg(filamentInfo.wxStrLabelName.ToStdString(), editIndex - 1);

                // update filament layout
                updateFilamentBtnColor(filamentEditInfo, editColorBtn);

                // update filament colour info  for object list and clour
                filamentInfoChanged();

                // add by allen for fixing use filament_colour instead of extruder_colour
                // post wxCUSTOMEVT_UPDATE_FILAMENT_INFO event
                ANKER_LOG_INFO << "onFilamentBtnLeftClicked enter, is click, index is " << editIndex;
                wxVariant eventData;
                eventData.ClearList();
                eventData.Append(wxVariant(filamentInfo.wxStrColor));
                eventData.Append(wxVariant((int)editIndex));
                wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_CLICK_FILAMENT_BTN);
                evt.SetClientData(new wxVariant(eventData));
                wxPostEvent(this, evt);
            }
        }

        // add by aden for sync filament info
        void AnkerSidebarNew::onSyncFilamentInfo(wxCommandEvent& event) {
 
        }

        void AnkerSidebarNew::updateCfgComboBoxPresets(Slic3r::Preset::Type presetType) {
            auto getTab = [](Preset::Type type) {
                Tab* null_tab = nullptr;
                for (Tab* tab : wxGetApp().tabs_list)
                    if (tab->type() == type)
                        return tab;
                return null_tab;
            };
            
            auto updateConfigPresets = [this, getTab](Preset::Type type) {
#if SHOW_OLD_SETTING_DIALOG
                getTab(type)->update_preset_choice();
#endif
                AnkerTab* ankerTab = wxGetApp().getAnkerTab(type);
                if(ankerTab)
                    ankerTab->update_preset_choice();
            };

            updateConfigPresets(presetType);
        }

        void AnkerSidebarNew::updateFilamentColourInCfg(const std::string strLabelName, 
            const int selectedIndex, bool bUseSelectedIndex) {
            wxGetApp().preset_bundle->set_filament_preset(selectedIndex, strLabelName);
            if (bUseSelectedIndex) {
                // edit filament colour ,should use selected index
                wxGetApp().preset_bundle->filaments.select_preset_by_name(strLabelName, false);
            }
            else {
                // default filament change,should not use selected index,just select the first index
                if (0 == selectedIndex)
                    wxGetApp().preset_bundle->filaments.select_preset_by_name(strLabelName, false);
            }
            wxGetApp().plater()->update_filament_colors_in_full_config();
        }

        void AnkerSidebarNew::filamentInfoChanged() {
            if (!wxGetApp().mainframe || !wxGetApp().mainframe->m_ankerCfgDlg) {
                return;
            }
            // update selected choice of filament preset in filament tab
            AnkerTab* filamentAnkerTab = wxGetApp().getAnkerTab(Preset::TYPE_FILAMENT);
            if (filamentAnkerTab) {
                updateCfgComboBoxPresets(Preset::TYPE_FILAMENT);
                // reselect filament preset to show the right print config 
                AnkerTabPresetComboBox* ankerComboBox = wxGetApp().mainframe->m_ankerCfgDlg->GetAnkerTabPresetCombo(Preset::TYPE_FILAMENT);
                int selection = ankerComboBox->GetSelection();
                std::string presetName = ankerComboBox->GetString(selection).ToUTF8().data();
                filamentAnkerTab->select_preset(Preset::remove_suffix_modified(presetName));
                ANKER_LOG_INFO << "new selected filament preset name is " << presetName;
            }
         
            AnkerTab* printAnkerTab = wxGetApp().getAnkerTab(Preset::TYPE_PRINT);
            // mod by allen for Change the interaction for switching print and filament presets.
            if (printAnkerTab) {
                updateCfgComboBoxPresets(Preset::TYPE_PRINT);
                // reselect print preset to show the right print config 
                AnkerTabPresetComboBox* ankerComboBox = wxGetApp().mainframe->m_ankerCfgDlg->GetAnkerTabPresetCombo(Preset::TYPE_PRINT);
                int selection = ankerComboBox->GetSelection();
                std::string presetName = ankerComboBox->GetString(selection).ToUTF8().data();
                printAnkerTab->select_preset(Preset::remove_suffix_modified(presetName));
                ANKER_LOG_INFO << "new selected print preset name is " << presetName;
            }

            // update data in mmu
            wxGetApp().plater()->get_current_canvas3D()->get_gizmos_manager().update_data();
            // update data in objectbar
            wxGetApp().plater()->objectbar()->update_objects_list_extruder_column(p->m_editFilamentInfoList.size());

            // reload data by AnkerParameterPanel
            if (p->m_pAnkerParameterPanel) {
                p->m_pAnkerParameterPanel->reloadData();
            }  
        }

        // ensure the edit color btn ptr is valid int m_filamentEditDlg when filament panel is changed
        void AnkerSidebarNew::updateFilamentColorBtnPtr(wxButton* colorBtn) {
            if (p->m_filamentEditDlg && colorBtn && colorBtn->GetId() == ID_FILAMENT_COLOUR_BTN)
                p->m_filamentEditDlg->SetEditColorBtnPtr(colorBtn); 
        }

        // Example of resetting and restoring sidebar
        void AnkerSidebarNew::OnTimer(wxTimerEvent& event) {
            static int index = 0;
            ++index;

            if ((index) % 3 == 2) {
                // 1. use your own new sidebar
                if (!p->m_newMainSizer) {
                    p->m_newMainSizer = new wxBoxSizer(wxHORIZONTAL);
                }

                if (!p->m_newMainPanel) {
                    p->m_newMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                        AnkerSize(30 * wxGetApp().em_unit(), -1), wxTAB_TRAVERSAL);
                    p->m_newMainPanel->SetBackgroundColour(wxColour("#D1D100"));
                    p->m_newMainSizer->Add(p->m_newMainPanel, 0, wxEXPAND | wxALL, 0);
                }

                // 2. set your sizer into main sizer
                if (wxGetApp().plater()) {
                    wxGetApp().plater()->sidebarnew().setMainSizer(p->m_newMainSizer);
                }
            }
            else if ((index) % 3 == 1) {
                // 1. set default main sizer
                if (wxGetApp().plater()) {
                    wxGetApp().plater()->sidebarnew().setMainSizer();
                }

                // 2. create your own sub Panel
                if (!p->m_universalCustomSizer) {
                    p->m_universalCustomSizer = new wxBoxSizer(wxHORIZONTAL);
                }

                if (!p->m_universalCustomPanel) {
                    p->m_universalCustomPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                        AnkerSize(30 * wxGetApp().em_unit(), -1), wxTAB_TRAVERSAL);
                    p->m_universalCustomPanel->SetBackgroundColour(wxColour("#3096FF"));
                    p->m_universalCustomSizer->Add(p->m_universalCustomPanel, 0, wxEXPAND | wxALL, 0);
                }

                // 3. replace your sub sizer in main sizer by index
                if (wxGetApp().plater()) {
                    wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer(p->m_universalCustomSizer);
                }
            }
            else {
                // 1. restore all default sidebar
                if (wxGetApp().plater()) {
                    wxGetApp().plater()->sidebarnew().setMainSizer();
                }
            }
        }

		void AnkerSidebarNew::showRightMenuParameterPanel(const wxString& objName, Slic3r::ModelConfig* config)
		{
            if (!p->m_pAnkerRightMenuParameterPanel||!p->m_pParameterVSizer)
                return;

            replaceUniverseSubSizer();

            m_showRightMenuPanel = PANEL_RIGHT_MENU_PARAMETER;
            p->m_pAnkerParameterPanel->Hide();
            p->m_pAnkerRightMenuParameterPanel->setObjectName(objName, config);                                            
            p->m_pAnkerRightMenuParameterPanel->Show();                        
            refreshSideBarLayout();
                
		}

		void AnkerSidebarNew::exitRightMenuParameterPanel()
		{
            if (m_sizerFlags != "")
                return;

			m_showRightMenuPanel = PANEL_PARAMETER_PANEL;
            p->m_pAnkerRightMenuParameterPanel->setObjectName("", nullptr);
			p->m_pAnkerRightMenuParameterPanel->Hide();
			p->m_pAnkerParameterPanel->Show();
			Refresh();
			Layout();
		}
        bool AnkerSidebarNew::checkDirtyDataonParameterpanel()
        {
           return p->m_pAnkerParameterPanel->checkDirtyData();
        }

		void AnkerSidebarNew::onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox)
		{
            presetComboBox->Popup();
		}

		void AnkerSidebarNew::onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection)
		{
			if (!presetChoice->selection_is_changed_according_to_physical_printers()) {
				if (!presetChoice->is_selected_physical_printer())
					Slic3r::GUI::wxGetApp().preset_bundle->physical_printers.unselect_printer();

				// select preset
				std::string preset_name = presetChoice->GetString(selection).ToUTF8().data();
				Slic3r::Preset::Type presetType = presetChoice->type();
				Slic3r::GUI::wxGetApp().getAnkerTab(presetType)->select_preset(Slic3r::Preset::remove_suffix_modified(preset_name));

			}
		}

		wxString AnkerSidebarNew::getSizerFlags() const
		{
            return m_sizerFlags;
		}

	}
}    // namespace Slic3r::GUI

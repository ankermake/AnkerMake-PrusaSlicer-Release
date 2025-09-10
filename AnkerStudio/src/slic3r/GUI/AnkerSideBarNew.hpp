#ifndef _ANKER_SIDEBAR_NEW_HPP_
#define _ANKER_SIDEBAR_NEW_HPP_

#include "slic3r/GUI/NozzlePriorPresent/NozzlePriorityQueue.hpp"
#include <memory>
#include <vector>

#include <wx/panel.h>
#include <wx/event.h>

#include "libslic3r/Preset.hpp"
#include "PresetComboBoxes.hpp"
#include "./PrinterConfigPlater/AnkerParameterPanel.hpp"
#include "Search.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/GUI_ObjectLayers.hpp"

class wxButton;
class ScalableButton;
class wxScrolledWindow;
class wxString;
class AnkerBitmapCombox;

enum CurrentPanelMode
{
    PANEL_MODEL_PARAMETER = 0,
    PANEL_GLOBAL_PARAMETER = 1,
    PANEL_OTHER_PARAMETER = 2,
};

namespace Slic3r {
    namespace GUI {

#define COLOUR_BTN_SIZE 25

#define SEPARATOR_HEAD "————— "
#define SYSTEM_PRESETS_SEPARATOR_TAIL " —————"
#define MY_PRESETS_SEPARATOR_TAIL " ———————"
#define SYSYTEM_PRESETS "common_slicepannel_parametersselect_title1"
#define MY_PRESETS "common_slicepannel_parametersselect_title2"
#define SYSTEM_PRESETS_SEPARATOR wxString::FromUTF8(SEPARATOR_HEAD) + _L(SYSYTEM_PRESETS) + wxString::FromUTF8(SYSTEM_PRESETS_SEPARATOR_TAIL)
#define MY_PRESETS_SEPARATOR wxString::FromUTF8(SEPARATOR_HEAD) + _L(MY_PRESETS) + wxString::FromUTF8(MY_PRESETS_SEPARATOR_TAIL)

#define SIDEBARNEW_PRINTGER_HEIGHT  90  // 109
#define SIDEBARNEW_PRINTGER_TEXTBTN_SIZER   32 //46
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
#define SIDEBARNEW_FILAMENT_TEXTBTN_SIZER  32 // 46
#define SIDEBARNEW_FILAMENT_PANEL_HEIGHT  (SIDEBARNEW_FILAMENT_HEIGHT - SIDEBARNEW_FILAMENT_TEXTBTN_SIZER)
#define SIDEBARNEW_FILAMENT_HOR_SPAN  10 // 14
#define SIDEBARNEW_FILAMENT_VER_SPAN  8 // 14


#define SIDEBARNEW_PRINT_TEXTBTN_SIZER  32


        class Plater;
        wxDECLARE_EVENT(wxCUSTOMEVT_CLICK_FILAMENT_BTN, wxCommandEvent);
        wxDECLARE_EVENT(wxCUSTOMEVT_REMOVE_FILAMENT_BTN, wxCommandEvent);
        wxDECLARE_EVENT(wxCUSTOMEVT_UPDATE_FILAMENT_INFO, wxCommandEvent);
        wxDECLARE_EVENT(wxCUSTOMEVT_CHECK_RIGHT_DIRTY_DATA, wxCommandEvent);
        wxDECLARE_EVENT(wxCUSTOMEVT_MAIN_SIZER_CHANGED, wxCommandEvent);

        enum FilamentViewType {
            singleColorView = 1,
            multiColorHorView = 2,
            multiColorVerView = 3
        };

        // means filament source type,maybe system or user created
        enum FilamentSourceType {
            systemFilamentType = 1,
            userFilamentType = 2
        };

        typedef struct SFilamentInfo {
            wxString wxStrColor     = "";
            wxString wxStrColorName = "";
            wxString wxStrLabelName = "";
            wxString wxStrLabelType = "";
            wxString wxStrFilamentType = "";
            FilamentSourceType filamentSrcType = systemFilamentType;
            // add by allen for add anker_filament_id and anker_colour_id
            wxString wxStrFilamentId = "";
            wxString wxStrColorId = "";
            SFilamentInfo(wxString color = "", wxString colorName = "", wxString labelname = "", wxString type = "",
                wxString filamentType = "" ,FilamentSourceType srcType = systemFilamentType, wxString filamenId = "",
                wxString colourId = "") :
                wxStrColor(color),
                wxStrColorName(colorName),
                wxStrLabelName(labelname),
                wxStrLabelType(type),
                wxStrFilamentType(filamentType),
                filamentSrcType(systemFilamentType),
                wxStrFilamentId(filamenId),
                wxStrColorId(colourId){
            };
            SFilamentInfo(const SFilamentInfo& info) {
                wxStrColor = info.wxStrColor;
                wxStrColorName = info.wxStrColorName;
                wxStrLabelName = info.wxStrLabelName;
                wxStrLabelType = info.wxStrLabelType;
                wxStrFilamentType = info.wxStrFilamentType;
                filamentSrcType = info.filamentSrcType;
                wxStrFilamentId = info.wxStrFilamentId;
                wxStrColorId = info.wxStrColorId;
            }
        }SFilamentInfo;

        typedef struct SFilamentEditInfo {
            int editIndex = 0;
            SFilamentInfo filamentInfo;
            SFilamentEditInfo(int index, SFilamentInfo info):
            editIndex(index),
            filamentInfo(info) {

            }
            SFilamentEditInfo(const SFilamentEditInfo& info){
                editIndex = info.editIndex;
                filamentInfo = info.filamentInfo;
            }

            SFilamentEditInfo() {

            }

            bool operator == (const SFilamentEditInfo& editInfo) {
                return (editInfo.filamentInfo.wxStrColor == this->filamentInfo.wxStrColor 
                    && editInfo.filamentInfo.wxStrLabelName == this->filamentInfo.wxStrLabelName
                    && editInfo.filamentInfo.wxStrLabelType == this->filamentInfo.wxStrLabelType);
            }
        } SFilamentEditInfo;

        wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_FILAMENT_CHANGED, wxCommandEvent);
        class AnkerFilamentEditDlg : public wxFrame
        {
        public:
            AnkerFilamentEditDlg(wxWindow* parent, std::string title, SFilamentEditInfo filamentEditInfo, AnkerBtn* btn);
            ~AnkerFilamentEditDlg();
            void SetEditColorBtnPtr(AnkerBtn* btn);
            void updateFilamentInfo(SFilamentEditInfo filamentEditInfo);

        private:
            void initUI();

            void initFilamentCombox();
            void initColorCombox();
            void resetFilamentCombox();
            void resetColorCombox(bool bInitCreate = true);
            wxBitmap createBitmapFromColor(int width, int height, wxColour color);
            void OnFilamentComboSelected(wxCommandEvent& event);
            void OnColorComboSelected(wxCommandEvent& event);
            void PostFilamentUpdateEvent();

            void OnExitButtonClicked(wxCommandEvent& event);
			void onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox);
			void onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection);
        private:
            std::string m_title;
            wxStaticText* m_pTitleText{nullptr};
            ScalableButton* m_pExitBtn{nullptr};
            AnkerBtn* m_pEditColorBtn{ nullptr };
            SFilamentEditInfo m_oldFilamentEditInfo;
            SFilamentEditInfo m_newFilamentEditInfo;

            std::set<wxString> m_filamentList;
            std::map<wxString, std::vector<SFilamentInfo>> m_filamentInfoList;

            AnkerPlaterPresetComboBox* m_comboFilament{nullptr};
            AnkerBitmapCombox* m_comboColor{nullptr};
            int m_comboFilamentLastSelected = 1;
            
        };


        class AnkerSidebarNew : public wxPanel
        {
            ConfigOptionMode    m_mode{ ConfigOptionMode::comSimple };
        public:
            AnkerSidebarNew(Plater* parent);
            AnkerSidebarNew(AnkerSidebarNew&&) = delete;
            AnkerSidebarNew(const AnkerSidebarNew&) = delete;
            AnkerSidebarNew& operator=(AnkerSidebarNew&&) = delete;
            AnkerSidebarNew& operator=(const AnkerSidebarNew&) = delete;
            ~AnkerSidebarNew();

            void reloadParameterData();
            void updatePreset(DynamicPrintConfig&config);
            void hidePopupWidget();
            void ChangeViewMode(int mode);
            void enableSliceBtn(bool isSaveBtn, bool isEnable);
            void updatePreviewBtn(bool GcodeValid, int reason = 0);
            // if you want to reset and restore sidebar,please see usage examples in OnTimer function
            // if sizer is nullptr, use default m_pParameterVSizer instead;
            //changed by alves.
            bool replaceUniverseSubSizer(wxSizer* sizer = nullptr,wxString sizerFlags = "");
            // use default main sizer when sizer is nullptr
            void setMainSizer(wxSizer* sizer = nullptr, bool bForceAllDefault = true);
            bool checkDirtyDataonParameterpanel();
            void updateParameterPanel();

            void SetCurrentModelConfig(Slic3r::ModelConfig* config, ObjectDataViewModelNode* node, bool bUpdate = false);
            void HideLocalParamPanel();
            void SwitchParamPanelMode(bool show);
            CurrentPanelMode GetCurrentParamPanelMode() { return m_currentParamPanelMode; }
            AnkerParameterPanel* GetGlobalParameterPanel();
            AnkerParameterPanel* GetModelParameterPanel();
            
            void updatePresets(Slic3r::Preset::Type preset_type);
            void onExtrudersChange(size_t changedExtruderNums = 0);
            //add by alves
            wxString getSizerFlags()const;
            const std::vector<SFilamentInfo>& getEditFilamentList();
            const std::map<wxString, std::vector<SFilamentInfo>>& getAllFilamentList();
              
            bool isMultiFilament();

            void onMove(wxMoveEvent& evt);
            void onSize(wxSizeEvent& evt);
            void onMinimize(wxIconizeEvent& evt);
            void onMaximize(wxMaximizeEvent& evt);

            void shutdown();
            void closeFilamentEditDlg();

            void syncFilamentInfo(const std::vector<SFilamentInfo> syncFilamentInfo);

            // add by allen for ankerCfgDlg search
            void check_and_update_searcher(bool respect_mode = false, Preset::Type type= Preset::TYPE_COUNT);
            Search::OptionsSearcher& get_searcher();
            std::string& get_search_line();
            void update_mode();
            void jump_to_option(size_t selected);
            void jump_to_option(const std::string& opt_key, Preset::Type type, const std::wstring& category);
            void search();
            void getItemList(wxStringList& list, ControlListType listType);
            void openSupportMaterialPage(wxString itemName, wxString text);
            void setGlobalParamItemValue(const wxString tabName, const wxString& widgetLabel, wxVariant data);
            void setObjectParamItemValue(const wxString tabName, const wxString& configOptionKey, wxVariant data);
            void setCalibrationValue(const wxString& configOptionKey, wxVariant value, ConfigOption* option);

            // set load file flag
            void setLoadProjectFileFlag(bool bFlag = true);
            bool getLoadProjectFileFlag();

            void updateCfgTabPreset();
            void renameUserFilament(const std::string& oldFilamentName, const std::string& newFilamentName);
            void setFilamentClickedState(bool bClickedFialment = false);
            bool getFilamentClickedState();
            void moveWipeTower(double x, double y, double rotate);

            /////////object list
            ObjectList* object_list();
            AnkerObjectLayerEditor* object_layer();
            AnkerObjectSettings* object_settings();
            AnkerObjectListControl* GetObjectlistControl();
            void update_ui_from_settings();
            void update_current_item_config();
            void update_objects_list_extruder_column(size_t extruders_count);
            void set_layer_height_sizer(wxBoxSizer* sizer, bool layer_root = true);
            void detach_layer_height_sizer();

            std::map < wxString, PARAMETER_GROUP*> getGlobalParamInfo();
            std::map < wxString, PARAMETER_GROUP*> getModelParamInfo();
        private:
            void updateFilamentEditDlgPos();
            void modifyExtrudersInfo(const std::vector<SFilamentEditInfo> filamentInfoList, 
                bool bRemove = false, int removeIndex = 0);
            void updateNozzle();
            void startUpUpdateNozzle();
            void noStartUpUpdateNozzle();

            void updateAllPresetComboboxes();
            void getEditFilamentFromCfg(size_t changedExtruderNums = 0);
            void getAllFilamentFromCfg();
            void handleUserFilamentColor();
            void presetNameSplit(const std::string presetName, std::string& colorName);
            void refreshSideBarLayout();
            void createPrinterSiderbar();
            void createFilamentSidebar();
            void createObjectlistSidebar();
            void createPrintParameterSidebar();
            void createRightMenuPrintParameterSidebar();
            void setFixedDefaultSidebar();
            void setAllDefaultSidebar();

            void onFilamentListToggle(wxEvent& event);
            void updateFilamentInfo(bool bDestoryPanel = false);
            void updateFilamentBtnColor(SFilamentEditInfo* filamentEditInfo, AnkerBtn* editColorBtn);
    
            void singleColorFilamentHorInit();
            void multiColorFilamentHorInit();
            void multiColorFilamentVerInit();

            // event
            void onHorSingleColorFilament();
            void onHorMultiColorFilament();
            void onVerMultiColorFilament();

            void onFilamentMenuOpen(wxMouseEvent& event);
            void onFilamentBtnLeftClicked(wxMouseEvent& event);
            void onFilamentTextLeftClicked(wxMouseEvent& event);
            void onFilamentPopupClick(wxCommandEvent& event);
            void onFilmentEditSelected(const uint32_t btnIndex, AnkerBtn* btn);
            void onFilamentRemoveSelected(const uint32_t btnIndex, AnkerBtn* btn);
            void onUpdateFilamentInfo(wxCommandEvent& event);

            void removeCfgTabExtruders();
            // add by aden for sync filament info
            void onSyncFilamentInfo(wxCommandEvent& event);

            void OnTimer(wxTimerEvent& event);

            void updateCfgComboBoxPresets(Slic3r::Preset::Type presetType);
            void updateFilamentColourInCfg(const std::string strLabelName, const int selectedIndex, bool bUseSelectedIndex = true);

            // the editIndex = -1 means forced to updateCfgComboBoxPresets
            void filamentInfoChanged(int editIndex = -1);

            void updateFilamentColorBtnPtr(AnkerBtn* colorBtn);
			void onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox);
			void onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection);

            void updateCfgTabTimer(wxTimerEvent&);
            void updateUserFilament();
            void updateItemConfigInfo(bool is_item_reset = false, const wxString& option_key = "");
            void selectDefaultFilament();
        private:
            struct priv;
            std::unique_ptr<priv> p;
            std::string m_multiColorSelectedSn = "";
            wxDECLARE_EVENT_TABLE();

            wxString m_sizerFlags = "";
            CurrentPanelMode m_currentParamPanelMode = PANEL_GLOBAL_PARAMETER;
            std::atomic_bool m_bLoadProjectFile{ false };
            wxTimer m_timer;
        };
    } // namespace GUI
} // namespace Slic3r

#endif

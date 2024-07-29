#ifndef _ANKER_PARAMETER_PANEL_HPP_
#define _ANKER_PARAMETER_PANEL_HPP_

#include "wx/wx.h"
#include "wx/combobox.h"
#include <wx/window.h>
#include <iostream>
#include <map>
#include <vector>
#include "../AnkerBtn.hpp"
#include "../AnkerEasyPanel.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Preset.hpp"
#include "../Tab.hpp"
// add by allen for ankerCfgDlg
#include "../AnkerCfgTab.hpp"

#include "../GUI_App.hpp"
#include "../MainFrame.hpp"
#include "wx/odcombo.h"
#include "wx/dc.h"
#include "../Common/AnkerSimpleCombox.hpp"
#include "../Common/AnkerPopupWidget.hpp"
#include "../Common/AnkerLineEditUnit.hpp"
#include "../AnkerLineEdit.hpp"
#include "../PresetComboBoxes.hpp"
#include "../ConfigManipulation.hpp"
#include "AnkerPrintParaItem.hpp"
#include "AnkerParameterData.hpp"
#include "libslic3r/Model.hpp"
#include "slic3r/GUI/ObjectDataViewModel.hpp"

#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/GUI_ObjectLayers.hpp"
#include "slic3r/GUI/Widgets/SwitchButton.hpp"

using namespace Slic3r::GUI;

class AnkerPrintParaItem;
class AnkerSimpleCombox;

#define DEFAULT_OBJECT_LIST_HEIGHT 180
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVEALL_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_UPDATE_CURRENT_PRESET, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, wxCommandEvent);
//schedule_background_process
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, wxCommandEvent);

class AnkerObjectListControl : public wxControl
{
public:
	AnkerObjectListControl(wxWindow* parent);
	~AnkerObjectListControl() {};

	void Init();

	inline ObjectList* get_object_list() const {
		return m_object_list;
	}

	inline std::shared_ptr<AnkerObjectSettings> get_object_setting() const {
		return m_object_setting;
	}

private:
	ObjectList* m_object_list{ nullptr };
	std::shared_ptr<AnkerObjectSettings> m_object_setting{ std::make_shared<AnkerObjectSettings>() };
};


class AnkerAcodeExportProgressDialog : public wxFrame
{
public:
	AnkerAcodeExportProgressDialog(wxWindow* parent);
	~AnkerAcodeExportProgressDialog() {};
	void InitUI();

private:
	void onProgressChange(float percentage);

private:
	wxStaticText* m_exportingLabel = nullptr;
	wxBitmapButton* m_stopExportBtn = nullptr;
	wxGauge* m_exportProgressBar = nullptr;;
	wxStaticText* m_exportProgressText = nullptr;
};




typedef struct paramItemTag {
	wxString strParamName;
	ControlType controlType;
	ItemDataType dataType;
	std::vector<wxString> contentStrList;
	bool local_show{ true };
	bool layer_height_show{ true };
	bool part_show{ true };
	bool modifer_show{ true };
	GroupParamUIConfig uiConifg = GroupParamUIConfig();
}paramItem;

typedef struct groupItemTag {
	wxString strIconName;
	wxString strGroupName;
	wxString strTabName;
	std::vector<paramItem> paramVec;
}groupItem;

class AnkerParameterPanel:public wxControl
{
public:
	AnkerParameterPanel(wxWindow* parent,
		PrintParamMode printParamMode,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerParameterPanel() 
	{
		// should clear static map when destory
		m_AllPrintParamInfo.clear();
	}


	//void ChangeViewMode(Slic3r::GUI::ViewMode mode);
	void ChangeViewMode(int mode);
	void SetAIVisibilityByPrinterModel();
	void UpdateAI(int reason);
	//void UpdatePreviewModeButtons(bool GcodeValid, Slic3r::GUI::RightSidePanelUpdateReason reason = Slic3r::GUI::REASON_NONE);
	void UpdatePreviewModeButtons(bool GcodeValid, int reason = 0);
	void enableSliceBtn(bool isSaveBtn, bool isEnable);
	void resetAllUi(int iResetType = 0);
	bool saveAllUi();
	bool swtichPresetCheckSave();
	void hidePoupWidget();
	void reloadData();
	void reloadDataEx();
	bool checkDirtyData();
	bool hasDirtyData();
	void updateEasyWidget();
	void updatePreset(Slic3r::DynamicPrintConfig& printConfig);
	void updatePresetEx(Slic3r::DynamicPrintConfig* printConfig);
	void checkUIData(Slic3r::DynamicPrintConfig* printConfig = nullptr);
	void getItemList(wxStringList& list,ControlListType listType);
	void setItemValue(const wxString& optionKey, wxVariant data, bool updateFlag = true);
	void setItemValue(const wxString tabName, const wxString& optionKey, wxVariant data, bool updateFlag = true);
	void setItemValueEx(const wxString tabName, const wxString &optionKey, wxVariant data,bool needReset);
	void openSupportMaterialPage(wxString itemName, wxString text);
	
	AnkerParameterData& getParameterData() { return m_parameterData; }

	bool refreshParamPanel(const Slic3r::DynamicPrintConfig& current_config, std::map<std::string, Slic3r::ConfigOption*> &mp);
	void setItemDataValue(const wxString& tabName, const std::string& key, ItemDataType type);
	void updateItemParamPanel(const std::map<std::string, Slic3r::ConfigOption*>& mp);
	wxString GetTabNameByKey(const std::string& key);

	bool isRightParameterPanel() { return m_isModelParameterPanel; }
	void showModelParamPanel(bool show = true);
	void setCurrentModelConfig(Slic3r::ModelConfig* config, const Slic3r::DynamicPrintConfig& current_config,  Slic3r::GUI::ObjectDataViewModelNode* node, 
		bool bUpdate);
	void resetModelConfig() { m_modelParameterCfg = nullptr; }

	void hideLocalParamPanel();
	void setParamVisible(Slic3r::GUI::ItemType type, Slic3r::ModelVolumeType volume_type, bool bUpdate);
	
	bool getObjConfig(Slic3r::ModelConfig*& config);
	void onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox);
	void onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection);
	void moveWipeTower(double x, double y, double angle);
	void onEasyPanelItemClicked(wxString strItemName);
	std::map<wxString, PARAMETER_GROUP*> getAllPrintParamInfos();
	bool isSelectSystemPreset();

	void onPrinterPresetChange();

	void set_layer_height_sizer(wxBoxSizer* sizer, bool layer_root);
	void detach_layer_height_sizer();
	SwitchButton* get_switch_btn() const { return m_mode_region; }
protected:
	void setItemValueWithToolTips(const wxString tabName, const wxString& optionKey, wxVariant data, const wxString& tipswxValue, bool* p_has_dirty = nullptr);
	void initUi();
	void onBtnClicked(wxCommandEvent &event);
	//void showCurrentWidget(const wxString& tabName);
	//void showCurrentWidget(const wxString &tabName, Slic3r::GUI::ItemType type = Slic3r::GUI::ItemType::itUndef);

	void showCurrentWidget(const wxString& tabName, Slic3r::GUI::ItemType type = Slic3r::GUI::ItemType::itUndef, 
		Slic3r::ModelVolumeType volume_type = Slic3r::ModelVolumeType::INVALID);

	void getSearResMap(std::map<wxString, std::map<wxString, wxString >>& searchResMap, const wxString& searchData);
	void showEasyModel();
	wxString getSelectedPrintMode();
	void showExpertModel();
	void switchToOption(const wxString& strTab,const wxString &optionKey);

	void InitViewMode();

	// iInitType equal 0: init all the data by init UI or revert by revert all group params.
	// iInitType equal 1:  revert all model params.
	void initUiData(int iInitType = 0);

	// iInitType equal 0: init all the data by init UI or revert by revert all group params.
	// iInitType equal 1:  revert all model params.
	void initAllTabData(int iInitType = 0);
	void setParamTooltips();
	void initSearchPopupWidget();
	void saveUiData();
	void hideAllResetBtn();
	ItemInfo getItemValue(const wxString &tabStr, const wxString& optionKey);

	std::vector<std::string> getValidParameter();
	wxString getCurrentModule(const std::vector<std::string>& list);
	void onDatachanged(wxCommandEvent &event);
	void onUpdateResetBtn(wxCommandEvent &event);
private:
	//set the preset of the given property
	void saveSetPresetPropertyValue(Slic3r::DynamicPrintConfig& config, 
									ItemInfo& ItemDataInfo,
									std::string& optionKey,
									wxString& optionLabel);

	void saveSetPresetPropertyValueEx(Slic3r::DynamicPrintConfig* printConfig,
									ItemInfo& ItemDataInfo,
									std::string& optionKey,
									wxString& optionLabel);
	void onResetBtnStatusChanged(bool isAble);
	AnkerPrintParaItem* createGroupItem(groupItem& pGroupItem, bool local = true, bool layer_height = true, bool part = false, bool modifer = false);
	//void onPresetOpPopupClick(wxCommandEvent& event);
	void rename_preset();
	void delete_preset();

	void CreateExportProgressDlg();
	void SetAIValByPrinterModel();
	void OnExportBtnClick(wxCommandEvent& event);
	void OnPrintBtnClick(wxCommandEvent& event);
	void EnableAIUI(bool enable);
	std::string getFormatedFilament(std::string filamentStr);
	void UpdateBtnClr();

	void OnToggled(wxCommandEvent& event);
	void ChangeParamMode(PrintParamMode mode);
	AnkerChooseBtn* GetTabBtnByName(const wxString& name);

private:
	void init_configManipulation();
	void CreateGlobalParamPanel();
	void CreateLocalParamPanel();
	void createObjectlistPanel();
	void UpdateObjectListControlHeigh();

private:
	bool m_easyFlag{ true };
	AnkerEasyPanel* m_pEasyWidget{ nullptr };
	ScalableButton* m_printParameterEditBtn{ nullptr };
	wxStaticText* m_pTitleLabel{ nullptr };
	wxStaticText* m_pExpertLabel{ nullptr };

	SwitchButton* m_mode_region{ nullptr };
	AnkerToggleBtn* m_pExpertModeBtn{ nullptr };
	ScalableButton* m_deleteBtn{ nullptr };
	ScalableButton* m_pExitBtn{ nullptr };
	ScalableButton* m_pulldown_btn{ nullptr };
	Slic3r::GUI::AnkerPlaterPresetComboBox* m_pPresetParameterComBox{nullptr};
	wxStaticText* m_currentObjName{nullptr};

#ifndef __APPLE__	
	wxButton* m_pResetBtn{ nullptr };
	wxButton* m_pRenameRemovePresetBtn{ nullptr };
	wxButton* m_pSaveAllBtn{ nullptr };
	wxButton* m_pSearchBtn{ nullptr };
#else	
	ScalableButton* m_pResetBtn{ nullptr };
    ScalableButton* m_pRenameRemovePresetBtn{ nullptr };
	ScalableButton* m_pSaveAllBtn{ nullptr };	
	ScalableButton* m_pSearchBtn{ nullptr };
#endif // !__APPLE__
	
	wxPanel* m_contentWidget{ nullptr };
	wxBoxSizer* m_contentSizer{ nullptr };

	wxPanel*	m_pDividingLine;
	wxControl*				m_pSearchEdit{ nullptr };
	AnkerPopupWidget*		m_popupWidget{ nullptr };
	AnkerLineEdit* m_pSearchTextCtrl{ nullptr };
	wxBoxSizer* m_pMainVSizer{ nullptr };

	// objectlist
	wxBoxSizer* m_object_sizer{ nullptr };
	AnkerObjectListControl* m_control{ nullptr };
	// layer height
	wxBoxSizer* m_layer_height_sizer{ new wxBoxSizer(wxVERTICAL) };

	wxScrolledWindow* m_pTabBtnScrolledWindow{ nullptr };
	wxScrolledWindow* m_pTabItemScrolledWindow{ nullptr };

	wxPanel* m_pBtnPanel{ nullptr };
	AnkerBtn* m_pSliceBtn{ nullptr };
	AnkerBtn* m_pSaveProjectBtn{ nullptr };

	// preview mode UI
	wxBoxSizer* m_AISizer = nullptr;
	wxStaticText* m_AILabel = { nullptr };
	AnkerToggleBtn* m_createAIFileToggleBtn{ nullptr };
	AnkerBtn* m_pExportBtn{ nullptr };
	AnkerBtn* m_pPrintBtn{ nullptr };
	AnkerAcodeExportProgressDialog* m_exportProgressDlg = {nullptr};
	std::atomic_bool m_onOneKeyPrint{ false };

	std::vector<AnkerChooseBtn*>	m_tabBtnVector;
	std::map<wxString, std::list<AnkerPrintParaItem*>> m_windowTabMap;//tab-groups
	bool m_hasChanged = false;

	bool m_isModelParameterPanel{false};
	Slic3r::ModelConfig* m_modelParameterCfg { nullptr };
	AnkerParameterData m_parameterData;   //  data redundancy, it's wast of memery beacause each option group heve one AnkerParameterData object
	wxBoxSizer* m_pTabItemScrolledVWinSizer;
	std::map<wxString, PARAMETER_GROUP*> m_AllPrintParamInfo;
	const PrintParamMode m_PrintParamMode;

	wxTimer m_slice_delay_timer;
	bool m_isFold{ false };
	wxString m_current_tab_name { " "};
	Slic3r::GUI::ItemType m_current_type { Slic3r::GUI::ItemType::itUndef };
	Slic3r::ModelVolumeType m_volume_type{ Slic3r::ModelVolumeType::INVALID };

	ConfigManipulation m_config_manipulation;
};

#endif


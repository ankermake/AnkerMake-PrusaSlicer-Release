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
#include "AnkerPrintParaItem.hpp"
#include "AnkerParameterData.hpp"

class AnkerPrintParaItem;
class AnkerSimpleCombox;


wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVEALL_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_UPDATE_CURRENT_PRESET, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, wxCommandEvent);
//schedule_background_process
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, wxCommandEvent);

typedef struct paramItemTag {
	wxString strParamName;
	ControlType controlType;
	ItemDataType dataType;
	std::vector<wxString> contentStrList;
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
	void getItemList(wxStringList& list,ControlListType listType);
	void setItemValue(const wxString tabName, const wxString& optionKey, wxVariant data);
	void setItemValueEx(const wxString tabName, const wxString &optionKey, wxVariant data,bool needReset);
	void openSupportMaterialPage(wxString itemName, wxString text);
	void setObjectName(const wxString& objName, Slic3r::ModelConfig* config);
	void showRightParameterpanel();
	
	bool getObjConfig(Slic3r::ModelConfig*& config);
	void onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox);
	void onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection);
	void moveWipeTower(double x, double y, double angle);
	void onEasyPanelItemClicked(wxString strItemName);
	std::map<wxString, PARAMETER_GROUP> getAllPrintParamInfos();
	bool isSelectSystemPreset();




protected:
	void setItemValueWithToolTips(const wxString tabName, const wxString& optionKey, wxVariant data, const wxString& tipswxValue);
	void initUi();
	void onBtnClicked(wxCommandEvent &event);
	void showCurrentWidget(const wxString &tabName);

	void getSearResMap(std::map<wxString, std::map<wxString, wxString >>& searchResMap, const wxString& searchData);
	void showEasyModel();
	wxString getSelectedPrintMode();
	void showExpertModel();
	void switchToOption(const wxString& strTab,const wxString &optionKey);

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
	AnkerPrintParaItem* createGroupItem(groupItem& pGroupItem);
	void onPresetOpPopupClick(wxCommandEvent& event);
	void rename_preset();
	void delete_preset();
private:
	AnkerEasyPanel* m_pEasyWidget{ nullptr };
	ScalableButton* m_printParameterEditBtn{ nullptr };
	wxStaticText* m_pTitleLabel{ nullptr };
	wxStaticText* m_pExpertLabel{ nullptr };
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

	wxScrolledWindow* m_pTabBtnScrolledWindow{ nullptr };
	wxScrolledWindow* m_pTabItemScrolledWindow{ nullptr };

	wxPanel* m_pBtnPanel{ nullptr };
	AnkerBtn* m_pSliceBtn{ nullptr };
	AnkerBtn* m_pSaveProjectBtn{ nullptr };

	std::vector<AnkerChooseBtn*>	m_tabBtnVector;
	std::map<wxString, std::list<AnkerPrintParaItem*>> m_windowTabMap;//tab-groups
	bool m_hasChanged = false;

	bool m_isRightParameterPanel{false};
	Slic3r::ModelConfig* m_rightParameterCfg{};
	AnkerParameterData m_parameterData;   //  data redundancy, it's wast of memery beacause each option group heve one AnkerParameterData object
	wxBoxSizer* m_pTabItemScrolledVWinSizer;
	std::map<wxString, PARAMETER_GROUP> m_AllPrintParamInfo;
	PrintParamMode m_PrintParamMode;
};

#endif


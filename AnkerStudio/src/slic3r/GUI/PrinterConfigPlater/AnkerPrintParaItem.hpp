#ifndef _ANKER_PRINT_PARAITEM_HPP_
#define _ANKER_PRINT_PARAITEM_HPP_

#include "wx/wx.h"
#include <iostream>
#include <vector>
#include "wx/string.h"
#include "../GUI_App.hpp"
#include "../I18N.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "AnkerParameterData.hpp"
#include "slic3r/GUI/ObjectDataViewModel.hpp"

class wxOwnerDrawnComboBox;


typedef struct tagGroupParamUIConfig {
	bool bIsFullWith = false;
	bool bIsMultiLine = false;
	int iMinWidth = 10;
	int iMinHeight = 10;
	bool bIsMultiStr = false;
	bool bIsCode = false;
}GroupParamUIConfig;

enum PrintParamMode
{
	mode_global,
	mode_model
};

class AnkerPrintParaItem :public wxControl
{
public:
	AnkerPrintParaItem(wxWindow* parent,
		wxString icon,
		wxString title,
		wxString tabTitle,
		PrintParamMode printMode,
		wxWindowID winid = wxID_ANY,
	    bool local = true, bool layer_height = true, 
		bool part = false, bool modifer = false,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerPrintParaItem();
	wxString getTitle()const;
	wxArrayString getOptionsList() const;
	wxWindow* createItem(const wxString configOptionKey, ControlType controlType, ItemDataType dataType, wxStringList strList = {}, 
		bool local = true, bool layer_height = false, bool part = false, bool modifer = false, GroupParamUIConfig uiConifg = GroupParamUIConfig());
	void updateModelParams(const PARAMETER_GROUP& paraGroup, wxVariant cfgData, const wxString& prusaKeyString, wxString& strLabel);
	bool hsaDirtyData();

	ItemInfo getWidgetValue(const wxString& optionKey);
	bool isExistOption(const wxString& optionKey);
	void updateUi(const wxString& optionKey, wxVariant data, bool isReset = false);
	void updateUi(const wxString& optionKey, wxVariant data, wxString tooltipsValue, bool isReset = false,bool bEnable = true, bool bShown = true);
	void showOptionHighlight(const wxString& optionKey, wxColour labelColor);
	void hideAllResetBtn();
	void clearDirtyData();
	void showResetBtn(const wxString& optionKey, bool show = true);
	bool isExceptionHadleEditParam(wxString strParamKey);
	void HandleExceptionPram(wxString strParamKey, wxOwnerDrawnComboBox* pEditBox, wxString strValue ,std::map<wxString, PARAMETER_GROUP>::iterator paramInfoIter);
	// 
	// iCheckDependStandard equal 0:  check satndard to be  dependedParamMap in DEPENDENCY_INFO struct
	// iCheckDependStandard equal 1:  check satndard to be  dependedParamNewFormsMap in DEPENDENCY_INFO struct
	// chengeType equal 0: change from param change 
	// chengeType equal 1: change from preset change 
	void SetParamDepedency(PARAMETER_GROUP& paramInfo, int chengeType = 0, int iCheckDependStandard = 0, int iCheckValue = 0);
	// notes: if strParamKey not found in map ,this function will throw exception
	PARAMETER_GROUP& GetParamDepenencyRef(wxString strParamKey);
	
	ItemDataType GetItemDataType(wxString optionKey);
	wxString GetItemUnit(wxString optionKey);

	wxString GetIOptionLabel(wxString optionKey);
	wxString GetOptionTip(wxString optionKey);
	wxString GetOptionSideText(wxString optionKey);
	bool GetOptionMaxMinDefVal(wxString optionKey, float& min, float& max);
	//
	// check edit input 
	// return 0: input is in list range
	// return 1: input type mismatch
	// return 2: input valid but need check value range 
	int checkEditBoxInput(wxString strInput,wxString strParamKey,ItemDataType dataType,bool bHavePercent);
	bool CheckExceptionParamValue(wxString strEditValue, wxString paramKey, wxString strUnit);
	std::map<wxString, wxString> GetExceptionParamMap(wxString strParamKey);
	std::map<wxString, PARAMETER_GROUP> GetGroupparamInfos() { return  m_optionParameterMap; }
	std::shared_ptr<GroupProperty> GetGroupProperty() const { return m_group_property; }
	void SetItemVisible(Slic3r::GUI::ItemType type, Slic3r::ModelVolumeType volume_type);

protected:
	void onDatachanged(bool is_item_reset = false, const wxString& option_key = "");
	void onUpdateResetBtn();
	//wxString getPrusaLabel(const wxString& labelStr);
	void initUi();
	void onResetBtnClicked(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void saveSetPresetPropertyValue(Slic3r::ModelConfig* printConfig,
		ItemInfo& ItemDataInfo,
		std::string& prusaProperty,
		std::string& AnkerPropertyName);
	wxString RemoveTrailingZeros(const wxString& numStr);
	bool isNumber(const std::string& str);

	wxString RemoveDecimal(const wxString& numStr);
	wxString RemoveLeadingAndTrailingZeros(const wxString& strnum);

	bool ValueCheck(const PARAMETER_GROUP& param, const wxString& value, wxString& newValue);
	void RefreashDependParamState(wxString strChangedOptionkey, int iSetValue);
	void ChangeMaxMinValueForPrinter(const wxString& optionKey, float& max, float& min);
private:
	void TestRemoveLeadingAndTrailingZeros();

private:

	wxBoxSizer* m_pMainVSizer;
	wxString m_icon;
	wxString m_title;
	wxString m_tabTitle;
	wxTimer* m_HightLightTimer{ nullptr };
	wxStaticText* m_pCurrentLabel{ nullptr };
	wxColour m_colour{ "#A9AAAB" };
	std::map<wxString, PARAMETER_GROUP> m_optionParameterMap; // option_key - option_parameter map for this group
	//add by alves for check if has changed data and notify to save the changeds
	std::map<wxString, ItemDirtyData> m_dirtyMap;
	AnkerParameterData m_parameterData;  //  data redundancy, it's wast of memery beacause each option group(AnkerPrintParaItem) heve one AnkerParameterData object
	PrintParamMode m_PrintMode;


	std::shared_ptr<GroupProperty> m_group_property;
};
#endif
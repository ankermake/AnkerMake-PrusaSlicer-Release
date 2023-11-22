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

class AnkerPrintParaItem;
class AnkerSimpleCombox;

enum ItemDataType
{
	Item_int,
	Item_bool,
	Item_float,
	Item_floatOrPercent,
	Item_serialize,
	Item_serialize_no_unit,
	Item_serialize_num,
	Item_enum_SeamPosition,
	Item_enum_PerimeterGeneratorType,
	Item_enum_FuzzySkinType,
	Item_enum_InfillPattern,
	Item_enum_IroningType,
	Item_enum_DraftShield,
	Item_enum_BrimType,
	Item_enum_SupportMaterialStyle,
	Item_enum_SupportMaterialPattern,
	Item_enum_SupportMaterialInterfacePattern,
	Item_enum_SlicingMode,
	Item_Percent,	
};

enum ControlType
{
	ItemComBox = 1,
	ItemEditUinit,
	ItemCheckBox,
	ItemSpinBox,
	ItemEditBox
	
};

enum ControlListType
{
	List_Seam_Position = 1,
	List_Perimeter_generator,
	List_Fuzzy_skin,
	List_Fill_density,
	List_Fill_pattern,
	List_Length_of_th_infill_anchor,
	List_Maximum_length_of_the_infill_anchor,
	List_Top_fill_pattern,
	List_Bottom_fill_pattern,
	List_Ironing_Type,
	List_Draft_shield,
	List_Brim_type,
	List_Style,
	List_Top_contact_Z_distance,
	List_Bottom_contact_Z_distance,
	List_Pattern,
	List_Top_interface_layers,
	List_Bottom_interface_layers,
	List_interface_pattern,
	List_Slicing_Mode,	
};

typedef struct tag_ItemInfo
{
	ItemDataType paramDataType;
	wxVariant paramDataValue;
}ItemInfo;

typedef struct UpdateDataStruct
{
	wxString tabName = "";
	wxString titleName = "";
	wxString prusaKey = "";
	wxString ankerKey = "";
	ItemDataType dataType = Item_int;
	wxVariant oldData = wxVariant();
	wxVariant newData = wxVariant();
}ItemDirtyData;

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SAVEALL_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_UPDATE_CURRENT_PRESET, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, wxCommandEvent);
//schedule_background_process
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, wxCommandEvent);


class AnkerParameterPanel:public wxControl
{
public:
	AnkerParameterPanel(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerParameterPanel() {}

	void enableSliceBtn(bool isSaveBtn, bool isEnable);
	void resetAllUi();
	bool saveAllUi();
	bool onSave();
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
	void setItemValue(const wxString tabName, const wxString & widgetLabel, wxVariant data);
	void openSupportMaterialPage(wxString itemName, wxString text);
	void setObjectName(const wxString& objName, Slic3r::ModelConfig* config);
	void showRightParameterpanel();
	
	bool getObjConfig(Slic3r::ModelConfig*& config);
	void onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox);
	void onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection);	
protected:
	void initUi();
	void initDefaultData();
	void onBtnClicked(wxCommandEvent &event);
	void showCurrentWidget(const wxString &tabName);

	void getSearResMap(std::map<wxString, std::vector<wxString>>& searchResMap, const wxString& searchData);
	void showEasyModel();
	void showExpertModel();
	void switchToLaebl(const wxString& strTab,const wxString &label);

	void initUiData();

	void initLapData();
	void initInfillData();
	void initSabData();
	void initSmData();
	void initSpeedData();
	void initMeData();
	void initAdvancedData();

	void saveUiData();

	void hideAllResetBtn();
	ItemInfo getItemValue(const wxString &tabStr, const wxString& labelStr);

	std::vector<std::string> getValidParameter();
	wxString getCurrentModule(const std::vector<std::string>& list);
	void onDatachanged(wxCommandEvent &event);
	void onUpdateResetBtn(wxCommandEvent &event);
private:
	//set the preset of the given property
	void saveSetPresetPropertyValue(Slic3r::DynamicPrintConfig& config, 
									ItemInfo& ItemDataInfo,
									std::string& prusaProperty,
									std::string& AnkerPropertyName);

	void saveSetPresetPropertyValueEx(Slic3r::DynamicPrintConfig* printConfig,
									ItemInfo& ItemDataInfo,
									std::string& prusaProperty,
									std::string& AnkerPropertyName);
	void onResetBtnStatusChanged(bool isAble);
private:
	AnkerEasyPanel* m_pEasyWidget{ nullptr };
	Slic3r::GUI::AnkerPlaterPresetComboBox* m_pHandleModelComBox{ nullptr };
	ScalableButton* m_printParameterEditBtn{ nullptr };

	wxStaticText* m_pTitleLabel{ nullptr };
	ScalableButton* m_deleteBtn{ nullptr };
	ScalableButton* m_pExitBtn{ nullptr };

	Slic3r::GUI::AnkerPlaterPresetComboBox* m_pPresetParameterComBox{nullptr};

	wxStaticText* m_currentObjName{nullptr};

#ifndef __APPLE__	
	wxButton* m_pResetBtn{ nullptr };
	wxButton* m_pSaveAllBtn{ nullptr };
	//wxButton* m_pCfgBtn{ nullptr };
#else	
	ScalableButton* m_pResetBtn{ nullptr };
	ScalableButton* m_pSaveAllBtn{ nullptr };
	//ScalableButton* m_pCfgBtn{ nullptr };
#endif // !__APPLE__
	
	wxPanel*	m_pDividingLine;

	//wxStaticText* m_pHandleParameterLabel{ nullptr };
	//AnkerSimpleCombox*   m_pHandleParameterComBox{ nullptr };

	wxControl*				m_pSearchEdit{ nullptr };
	AnkerPopupWidget*		m_popupWidget{ nullptr };
	AnkerLineEdit* m_pSearchTextCtrl{ nullptr };
	wxBoxSizer* m_pMainVSizer{ nullptr };

	//wxOwnerDrawnComboBox*	m_pSearchComBox;
	
	wxScrolledWindow* m_pTabBtnScrolledWindow{ nullptr };
	wxScrolledWindow* m_pTabItemScrolledWindow{ nullptr };

	wxPanel* m_pBtnPanel{ nullptr };
	AnkerBtn* m_pSliceBtn{ nullptr };
	AnkerBtn* m_pSaveProjectBtn{ nullptr };

	std::vector<AnkerChooseBtn*>	m_tabBtnVector;
	std::map<std::string, std::list<AnkerPrintParaItem*>> m_windowTabMap;//tab-itemList

	std::map<wxString, wxString> m_fillPatternData;
	std::vector<wxString> m_lapVector;
	std::vector<wxString> m_infillVector;
	std::vector<wxString> m_sabVector;
	std::vector<wxString> m_smVector;
	std::vector<wxString> m_speedVector;
	std::vector<wxString> m_meVector;
	std::vector<wxString> m_advancedVector;
	std::map<wxString, std::vector<wxString>> m_gSearchMap;

	bool m_hasChanged = false;

	std::map<wxString, wxString> m_lapMap;
	std::map<wxString, wxString> m_infillMap;
	std::map<wxString, wxString> m_sabMap;
	std::map<wxString, wxString> m_smMap;
	std::map<wxString, wxString> m_speedMap;
	std::map<wxString, wxString> m_meMap;
	std::map<wxString, wxString> m_advancedMap;

	std::map<int, Slic3r::SupportMaterialStyle> m_StyleMap;
	std::map<Slic3r::SupportMaterialStyle, int> m_ReVerStyleMap;
	std::map<int, Slic3r::InfillPattern> m_fillPatternMap;
	std::map<int, Slic3r::InfillPattern> m_tabfillPatternMap;//for top and bottom fill pattern

	bool m_isRightParameterPanel{false};
	Slic3r::ModelConfig* m_rightParameterCfg{};
};

typedef struct _PARAMETER_GROUP
{
	wxStaticText* m_pLabel = nullptr;
	ItemDataType m_dataType = Item_int;
	wxWindow* m_pWindow = nullptr;
	ScalableButton* m_pBtn = nullptr;
	
	ControlType m_type;
}*pPARAMETER_GROUP, PARAMETER_GROUP;

class AnkerPrintParaItem :public wxControl
{
public:
	AnkerPrintParaItem(wxWindow* parent,
		wxString icon,
		wxString title,
		wxString tabTitle,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerPrintParaItem();
	wxString getTitle()const;
	void createItem(const wxString& widgetLabel, ControlType controlType, ItemDataType dataType, wxStringList strList = {});	
	bool hsaDirtyData();

	ItemInfo getWidgetValue(const wxString& labelStr);
	bool isExistLabel(const wxString & widgetLabel);
	void updateUi(const wxString& widgetLabel, wxVariant data, bool isReset = false);
	void showLabelHighlight(const wxString& widgetLabel, wxColour labelColor);		
	void hideAllResetBtn();
	void clearDirtyData();
protected:	
	void onDatachanged();
	void onUpdateResetBtn();
	std::map<wxString, wxString> getItemMap();
	wxString getPrusaLabel(const wxString& labelStr);
	void initDefaultData();
	void initUi();
	void onResetBtnClicked(wxCommandEvent &event);
	void OnTimer(wxTimerEvent& event);
	void saveSetPresetPropertyValue(Slic3r::ModelConfig* printConfig,
									ItemInfo& ItemDataInfo,
									std::string& prusaProperty,
									std::string& AnkerPropertyName);
private:

	wxBoxSizer* m_pMainVSizer;
	wxString m_icon;
	wxString m_title;
	wxString m_tabTitle;
	wxTimer* m_HightLightTimer{nullptr};	
	wxStaticText *m_pCurrentLabel{nullptr};
	wxColour m_colour{ "#A9AAAB" };
	std::map<wxString, PARAMETER_GROUP> m_windowWgtLabelMap;//widget label - item	
	std::map<wxString, wxString> m_fillPatternData;

	std::map<wxString, wxString> m_lapMap;
	std::map<wxString, wxString> m_infillMap;
	std::map<wxString, wxString> m_sabMap;
	std::map<wxString, wxString> m_smMap;
	std::map<wxString, wxString> m_speedMap;
	std::map<wxString, wxString> m_meMap;
	std::map<wxString, wxString> m_advancedMap;

	std::map<Slic3r::SupportMaterialStyle, int> m_StyleMap;
	//add by alves for check if has changed data and notify to save the changeds
	std::map<wxString, ItemDirtyData> m_dirtyMap;

	std::map<int, Slic3r::InfillPattern> m_fillPatternMap;
	std::map<int, Slic3r::InfillPattern> m_tabfillPatternMap;//for top and bottom fill pattern
};

#endif


#ifndef _PARAMETER_DATA_
#define _PARAMETER_DATA_

#include "wx/wx.h"
#include <iostream>
#include <vector>
#include "wx/string.h"
#include "../GUI_App.hpp"
#include "../I18N.hpp"
#include "../wxExtensions.hpp"

#ifndef __APPLE__
#define PARAMETER_ITEM_SIZE wxSize(41 * Slic3r::GUI::wxGetApp().em_unit(), -1)
#define PARAMETER_PANEL_WITH 420
#else
#define PARAMETER_ITEM_SIZE wxSize(40 * Slic3r::GUI::wxGetApp().em_unit(), -1)
#define PARAMETER_PANEL_WITH 400
#endif // !__APPLE__

#define CONTROL_WIDTH 140
#define ANKER_CONTROL_WIDTH 140
#define R_PANEL_CONTROLS_HEIGHT 24
#define ParameterPanelBgColor (wxColor("#292A2D"))

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
	List_Top_surface_single_perimeter,
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
	Item_enum_TopSurfaceSinglePerimeter,
	Item_enum_IroningType,
	Item_enum_DraftShield,
	Item_enum_BrimType,
	Item_enum_SupportMaterialStyle,
	Item_enum_SupportMaterialPattern,
	Item_enum_SupportMaterialInterfacePattern,
	Item_enum_SlicingMode,
	Item_Percent,
	Item_Multi_Strings,
	Item_UNKONWN
};

enum ControlType
{
	ItemComBox = 1,
	ItemEditUinit,
	ItemCheckBox,
	ItemSpinBox,
	ItemEditBox,
	ItemTextCtrl,
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

enum Dependency_Type
{
	Type_Enable,
	Type_Disable,
	Type_Shown,
	Type_Hide,
	Type_None
};

typedef struct _DEPENDENCY_INFO 
{
	std::map<wxString,wxString> dependedParamMap; //key --> condition
	std::map<wxString, int> dependedParamNewFormsMap;//key --> condition but condition as int type
	std::vector<wxString> beDependedParamVec;
	Dependency_Type depencyType = Type_None;
}*pDEPENDENCY_INFO, DEPENDENCY_INFO;

typedef struct _PARAMETER_GROUP
{
	wxString m_optionKey = "";
	wxStaticText* m_pLabel = nullptr;
	ItemDataType m_dataType = Item_int;
	wxWindow* m_pWindow = nullptr;
	wxBoxSizer* pLineSizer= nullptr; // the hole line sizer of the param.
	ScalableButton* m_pBtn = nullptr;
	wxString m_UIvalue = "";
	wxString m_unit = "";
	wxString m_optionTip = "";
	ControlType m_type;
	bool bEnable = true;
	bool bShown = true;
	DEPENDENCY_INFO dependencyInfo;
}*pPARAMETER_GROUP, PARAMETER_GROUP;


class AnkerParameterData
{
public:
	AnkerParameterData();
	~AnkerParameterData() {};
	std::map<wxString, wxString> m_fillPatternData;
	std::map<int, Slic3r::SupportMaterialStyle> m_StyleMap;
	std::map<Slic3r::SupportMaterialStyle, int> m_ReVerStyleMap;
	std::map<int, Slic3r::InfillPattern> m_fillPatternMap;
	std::map<int, Slic3r::InfillPattern> m_tabfillPatternMap;
	std::map<int, Slic3r::SinglePerimeterType> m_singlePerimeterTypeMap;
	void getItemList(wxStringList& list, ControlListType listType);
protected:
	void initData();
};
#endif
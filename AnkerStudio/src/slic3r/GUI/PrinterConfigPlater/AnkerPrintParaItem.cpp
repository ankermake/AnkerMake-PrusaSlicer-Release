#include "../MsgDialog.hpp"
#include "AnkerPrintParaItem.hpp"
#include "AnkerParameterPanel.hpp"
#include "../common/AnkerTextCtls.h"
#include "slic3r/Utils/StringHelper.hpp"
#include "../Common/AnkerFont.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "../GUI.hpp"

std::map<wxString, DEPENDENCY_INFO> parintParamMap = {
	//iron group
	/*
	* paramKey --------->		dependcy map ----------->	dependcy map with 	----------	be depended map	---------------------> depend type
															value to be int		
	*/
	{"ironing_type",	{ {},								{},							{"ironing_speed","ironing_spacing","ironing_pattern","ironing_angle","ironing_flowrate"},				        Type_Shown}},
	{"ironing_speed",	{ {{"ironing_type","no ironing"}},	{{"ironing_type",0}},		{""},																										    Type_Hide}},
	{"ironing_spacing",	{ {{"ironing_type","no ironing"}},	{{"ironing_type",0}},		{""},																											Type_Hide }},
	{"ironing_pattern",	{ {{"ironing_type","no ironing"}},	{{"ironing_type",0}},		{""},																											Type_Hide }},
	{"ironing_angle",	{ {{"ironing_type","no ironing"}},	{{"ironing_type",0}},		{""},																											Type_Hide }},
	{"ironing_flowrate",	{ {{"ironing_type","no ironing"}},	{{"ironing_type",0}},		{""},																									    Type_Hide }},

	 // support tab
	 {"support_material",	{ {},								{},						{"support_material_threshold","raft_contact_distance",
																						 "raft_expansion","support_material_style",
																						"support_material_contact_distance","support_material_bottom_contact_distance",
																						"support_material_pattern","support_material_with_sheath","support_material_spacing",
																						"support_material_angle","support_material_closing_radius","support_material_interface_layers","support_material_bottom_interface_layers",
																						"support_material_interface_pattern","support_material_interface_spacing","support_material_interface_contact_loops","support_material_buildplate_only",
																						"support_material_xy_spacing","dont_support_bridges","support_material_synchronize_layers","support_tree_angle","support_tree_angle_slow","support_tree_branch_diameter",
																						"support_tree_branch_diameter_angle","support_tree_tip_diameter","support_tree_branch_distance","support_tree_top_rate"}, Type_Enable}},

	
	

	{"support_material_threshold",						{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"raft_contact_distance",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"raft_expansion",									{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_style",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_contact_distance",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},


	{"support_material_bottom_contact_distance",		{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_pattern",						{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_with_sheath",					{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_spacing",						{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_angle",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_closing_radius",					{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},


	{"support_material_interface_layers",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_bottom_interface_layers",		{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_interface_pattern",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_interface_spacing",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_interface_contact_loops",		{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_buildplate_only",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},

	{"support_material_synchronize_layers",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_material_xy_spacing",						{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"dont_support_bridges",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_angle",								{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_angle_slow",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_branch_diameter",					{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_branch_diameter_angle",				{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_tip_diameter",						{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_branch_distance",					{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	{"support_tree_top_rate",							{ {{"support_material","1"}},	{{"support_material",1}},		{""},	Type_Enable }},
	
	
	{"role_based_wipe_speed",	                            { {},   {},		           {"wipe_speed"},                          Type_Enable}},
	{"wipe_speed",							{ {{"role_based_wipe_speed","1"}},	      {{"role_based_wipe_speed",1}},   {""},	Type_Disable }},
	
};

AnkerPrintParaItem::AnkerPrintParaItem(wxWindow* parent,
	wxString icon,
	wxString title,
	wxString tabTitle,
	PrintParamMode printMode,
	wxWindowID winid /*= wxID_ANY*/,
	bool local, bool layer_height, bool part, bool modifer,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_icon(icon)
	, m_title(title)
	, m_tabTitle(tabTitle)
	, m_PrintMode(printMode)
	, m_group_property(std::make_shared<GroupProperty>(local, layer_height, part, modifer))
{
	initUi();
}

AnkerPrintParaItem::~AnkerPrintParaItem()
{

}

wxString AnkerPrintParaItem::getTitle() const
{
	return m_title;
}

wxArrayString AnkerPrintParaItem::getOptionsList() const
{
	wxArrayString options;
	for (const auto& item : m_optionParameterMap) {
		if (!item.first.empty())
			options.Add(item.first);
	}
	return options;
}

wxWindow* AnkerPrintParaItem::createItem(const wxString configOptionKey, ControlType controlType, ItemDataType dataType, wxStringList strList,
	bool local, bool layer_height, bool part, bool modifer, GroupParamUIConfig uiConifg)
{
	if (m_PrintMode == mode_global) {
		wxGetApp().sidebarnew().get_searcher().add_key(configOptionKey.ToStdString(), Slic3r::Preset::TYPE_PRINT, m_title, m_tabTitle);
	}

	wxWindow* item{ nullptr };
	wxBoxSizer* pTitleSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pFullLineSizer = new wxBoxSizer(wxHORIZONTAL);
	pTitleSizer->AddSpacer(12);
	pFullLineSizer->AddSpacer(12);

	wxString widgetLabel = GetIOptionLabel(configOptionKey);
	wxString widgetTip = GetOptionTip(configOptionKey);

	int iLabelLength = 0;
#ifndef __APPLE__
	iLabelLength = AnkerLength(206);
#else
	iLabelLength = AnkerLength(186);
#endif


	wxString labelStr = Slic3r::GUI::WrapEveryCharacter(widgetLabel, Body_14, iLabelLength);
	wxStaticText* pTitle = new wxStaticText(this, wxID_ANY, widgetLabel);

	if (!uiConifg.bIsFullWith)
	{
		pTitle->SetFont(Body_14);
		pTitle->SetForegroundColour(wxColour("#A9AAAB"));

		//set the label tooltips
		if (widgetTip.size() > 0)
			pTitle->SetToolTip(widgetTip);
		pTitle->SetLabelText(labelStr);

		wxClientDC dc(this);
		dc.SetFont(pTitle->GetFont());
		wxSize size = dc.GetTextExtent(labelStr);
		int textWidth = size.GetWidth();
		int texthigth = size.GetHeight();
		pTitleSizer->Add(pTitle, 0, wxTOP | wxALIGN_LEFT | wxBOTTOM, 2);
		pTitleSizer->AddSpacer(8);
	}
	else
	{
		pTitle->Hide();
	}

	ScalableButton* resetBtn = new ScalableButton(this, wxID_ANY, "reset_btn", "", AnkerSize(20, 20));
	resetBtn->SetMaxSize(AnkerSize(20, 20));
	resetBtn->SetMinSize(AnkerSize(20, 20));
	resetBtn->SetSize(AnkerSize(20, 20));
	resetBtn->Hide();
	resetBtn->SetWindowStyleFlag(wxBORDER_NONE);
	resetBtn->SetBackgroundColour(wxColour("#292A2D"));
	resetBtn->Bind(wxEVT_BUTTON, &AnkerPrintParaItem::onResetBtnClicked, this);
	pTitleSizer->AddStretchSpacer(1);

	if (!uiConifg.bIsFullWith)
	{
		pTitleSizer->Add(resetBtn, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
		pTitleSizer->AddSpacer(8);
	}
	else
	{
		pTitleSizer->Add(resetBtn, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
		wxStaticText* placeHolder = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, resetBtn->GetSize());
		pTitleSizer->Add(placeHolder, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		pFullLineSizer->AddSpacer(8);
	}

	PARAMETER_GROUP paraGroup;
	paraGroup.m_optionKey = configOptionKey;
	paraGroup.m_optionTip = GetOptionTip(configOptionKey);
	paraGroup.m_unit = GetOptionSideText(configOptionKey);
	paraGroup.m_dataType = dataType;
	paraGroup.m_pBtn = resetBtn;
	paraGroup.m_pLabel = pTitle;
	paraGroup.m_type = controlType;
	SetParamDepedency(paraGroup, 0);

	auto groupItemPtr = std::make_shared<GroupItemProperty>(configOptionKey, pTitle, resetBtn, local, layer_height, part, modifer);

	wxCursor handCursor(wxCURSOR_HAND);
	switch (controlType)
	{
	case ItemComBox:
	{
		wxArrayString   m_arrItems = {};
		for (size_t i = 0; i < strList.GetCount(); i++)
			m_arrItems.Add(strList[i]);

		wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
		btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
		wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
		wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
		wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));

		AnkerSimpleCombox* pCBox = new AnkerSimpleCombox();
		pCBox->Create(this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			AnkerSize(ANKER_CONTROL_WIDTH, R_PANEL_CONTROLS_HEIGHT),
			m_arrItems,
			wxNO_BORDER | wxCB_READONLY);

		pCBox->SetCursor(handCursor);
		pCBox->SetBackgroundColour(wxColor("#292A2D"));
		pCBox->setColor(wxColour("#434447"), wxColour("#292A2D"));
		pCBox->SetMinSize(AnkerSize(ANKER_CONTROL_WIDTH, 30));
		pCBox->SetSize(AnkerSize(ANKER_CONTROL_WIDTH, 30));
		pCBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

		if (strList.GetCount() > 0)
			pCBox->SetSelection(0);

		pCBox->Bind(wxEVT_COMBOBOX, [this, configOptionKey, resetBtn, pCBox, paraGroup](wxCommandEvent& event) {

			if (!resetBtn || !pCBox)
				return;

			int cboxIndex = pCBox->GetSelection();
			std::string cboxText = pCBox->GetString(cboxIndex).ToStdString();
			wxString strLabel = paraGroup.m_pLabel->GetLabelText();

			if (strLabel.Contains('\n'))
				strLabel.Replace('\n', "");

			if (configOptionKey.empty())
				return;
			auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
			int  prusaIndex = 0;
			wxVariant cfgData;

			switch (paraGroup.m_dataType)
			{
			case Item_enum_SeamPosition:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SeamPosition>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_ironing_pattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_print_order:
			{
				auto tempData = printConfig.opt_enum<Slic3r::WallSequence>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_PerimeterGeneratorType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::PerimeterGeneratorType>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_FuzzySkinType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::FuzzySkinType>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_InfillPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(configOptionKey.ToStdString());
				auto tempStrData = printConfig.opt_serialize(configOptionKey.ToStdString());
				std::locale loc;
				tempStrData[0] = std::toupper(tempStrData[0], loc);

				auto realData = m_parameterData.m_fillPatternData[tempStrData];
				if (cboxText != realData)
				{
					resetBtn->Show();
					ItemDirtyData dirtyData;
					dirtyData.tabName = m_tabTitle;
					dirtyData.titleName = m_title;
					dirtyData.prusaKey = configOptionKey.ToStdString();
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;
					m_dirtyMap.insert(std::make_pair(configOptionKey.ToStdString(), dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(configOptionKey.ToStdString());
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);

					resetBtn->Hide();
				}

				wxString lastSelStr = m_optionParameterMap.find(configOptionKey)->second.m_UIvalue;
				wxString cboxSelStr = pCBox->GetString(cboxIndex);
				if (lastSelStr != cboxSelStr)
				{
					updateUi(paraGroup.m_optionKey.ToStdString(), cboxSelStr);

					wxWindowUpdateLocker updateLocker(this);
					updateModelParams(paraGroup, wxVariant(wxString::Format("%d", cboxIndex)), configOptionKey, strLabel);
					onDatachanged(false, configOptionKey);
					onUpdateResetBtn();
					//Refresh();
					Layout();
				}

				return;
			}
			break;
			case Item_enum_TopSurfaceSinglePerimeter:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SinglePerimeterType>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;

			case Item_enum_IroningType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::IroningType>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_DraftShield:
			{
				auto tempData = printConfig.opt_enum<Slic3r::DraftShield>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_BrimType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::BrimType>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_SupportMaterialStyle:
			{
				Slic3r::SupportMaterialStyle emunData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(configOptionKey.ToStdString());
				int tempData = m_parameterData.m_StyleMap[emunData];
				prusaIndex = tempData;
			}
			break;
			case Item_enum_SupportMaterialPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialPattern>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_SupportMaterialInterfacePattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
			}
			break;
			case Item_enum_SlicingMode:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SlicingMode>(configOptionKey.ToStdString());
				prusaIndex = (int)tempData;
				break;
			}
			default:
				break;
			}

			if (cboxIndex != prusaIndex)
			{
				ItemDirtyData dirtyData;
				dirtyData.tabName = m_tabTitle;
				dirtyData.titleName = m_title;
				dirtyData.prusaKey = configOptionKey.ToStdString();
				dirtyData.ankerKey = strLabel;
				dirtyData.dataType = paraGroup.m_dataType;

				m_dirtyMap.insert(std::make_pair(configOptionKey.ToStdString(), dirtyData));
				resetBtn->Show();
			}
			else
			{
				auto iter = m_dirtyMap.find(configOptionKey.ToStdString());
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);
				resetBtn->Hide();
			}

			wxString lastSelStr = m_optionParameterMap.find(configOptionKey)->second.m_UIvalue;
			wxString cboxSelStr = pCBox->GetString(cboxIndex);
			if (lastSelStr != cboxSelStr)
			{
				updateUi(paraGroup.m_optionKey.ToStdString(), cboxSelStr);

				wxWindowUpdateLocker updateLocker(resetBtn);
				RefreashDependParamState(configOptionKey, cboxIndex);
				onDatachanged(false, configOptionKey);
				onUpdateResetBtn();

				updateModelParams(paraGroup, wxVariant(cboxIndex), configOptionKey, strLabel);
				//Refresh();
				Layout();
			}
			});

		pTitleSizer->Add(pCBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxTOP, 2);
		pTitleSizer->AddSpacer(4);
		paraGroup.m_pWindow = pCBox;
		paraGroup.pLineSizer = pTitleSizer;

		groupItemPtr->_window = pCBox;
		groupItemPtr->_boxSizer = pTitleSizer;
	}
	break;
	case ItemEditBox:
	{
		wxArrayString   m_arrItems = {};
		for (size_t i = 0; i < strList.GetCount(); i++)
			m_arrItems.Add(strList[i]);

		wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
		btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
		wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
		wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
		wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));

		AnkerSimpleCombox* pCBox = new AnkerSimpleCombox();
#ifndef __APPLE__
		pCBox->Create(this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			AnkerSize(ANKER_CONTROL_WIDTH, R_PANEL_CONTROLS_HEIGHT),
			m_arrItems,
			wxBORDER_SIMPLE);
#else
		pCBox->Create(this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			AnkerSize(ANKER_CONTROL_WIDTH, R_PANEL_CONTROLS_HEIGHT),
			m_arrItems,
			wxNO_BORDER);

#endif // !__APPLE__			

		pCBox->SetBackgroundColour(wxColor("#292A2D"));
		pCBox->setColor(wxColour("#434447"), wxColour("#292A2D"));
		pCBox->SetForegroundColour(wxColour("#FFFFFF"));
		pCBox->SetMinSize(AnkerSize(ANKER_CONTROL_WIDTH, 30));
		pCBox->SetSize(AnkerSize(ANKER_CONTROL_WIDTH, 30));
		pCBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
		pCBox->SetFont(Head_14);

		if (strList.GetCount() > 0)
			pCBox->SetSelection(0);

		pCBox->Bind(wxEVT_KILL_FOCUS, [this, resetBtn, pCBox, configOptionKey](wxFocusEvent& event) {
			int index = 0;
			wxString text = wxString();
			if (!pCBox || !resetBtn)
				return;

			auto it = this->m_optionParameterMap.find(configOptionKey);
			if (it == this->m_optionParameterMap.end())
				return;

			auto& paraGroup = it->second;
			std::string prusaKeyString = paraGroup.m_optionKey.ToStdString();
			if (prusaKeyString.empty())
			{
				return;
			}
			index = pCBox->GetSelection();
			text = pCBox->GetValue();
			//by samuel, just return when no change
			if (isExceptionHadleEditParam(configOptionKey) && CheckExceptionParamValue(text, configOptionKey, paraGroup.m_unit))
			{
				return;
			}
			else
			{
				wxString strTrimValue = text;
				strTrimValue.Replace(wxT(" "), wxT(""));
				strTrimValue.Replace(paraGroup.m_unit, wxT(""));
				strTrimValue.Replace(" ", "");
				wxString strUIValue = paraGroup.m_UIvalue;
				strUIValue.Replace(wxT(" "), wxT(""));
				strUIValue.Replace(paraGroup.m_unit, wxT(""));
				strUIValue.Replace(" ", "");
				if (text == strUIValue || strTrimValue == strUIValue)
				{
					return;
				}
				text = strTrimValue;
			}
			bool havePercent = false;
			if (text.EndsWith("%")) {
				havePercent = true;
			}
			text.Replace('%', "");

			int iCheckValueType = checkEditBoxInput(text, paraGroup.m_optionKey, paraGroup.m_dataType, havePercent);

			if (1 == iCheckValueType)
			{
				// recover the last valid input
				updateUi(prusaKeyString, paraGroup.m_UIvalue);
				Slic3r::GUI::MessageDialog dialog(m_parent, _L("Invalid numeric input"), _L("Parameter validation") + ": " + prusaKeyString, wxOK);
				dialog.SetSize(wxSize(320, 250));
				dialog.ShowModal();
				return;
			}
			else if (2 == iCheckValueType)
			{
				float min = 0.0f, max = 0.0f;
				GetOptionMaxMinDefVal(prusaKeyString, min, max);
				if (paraGroup.m_dataType == Item_floatOrPercent)
				{
					double value;
					text.ToDouble(&value);
					if (!havePercent) {
						if (value > max) {
							// choose float data or percent data
							const wxString msg_text = wxString::Format("Is it %s%% or %s %s?\nYES for %s%%, \nNO for %s %s.", text, text, "mm", text, text, "mm");
							Slic3r::GUI::MessageDialog dialog(m_parent, msg_text, _L("Parameter validation") + ": " + prusaKeyString, wxYES | wxNO);
							dialog.SetSize(wxSize(400, 250));
							if (dialog.ShowModal() == wxID_YES)
								text = RemoveTrailingZeros(text) + "%";
							else {
								//text = RemoveTrailingZeros(wxString::Format(wxT("%f"), max));  // limit to max
								text = RemoveTrailingZeros(wxString::Format(wxT("%f"), value));  // the user input value
							}
						}
						else if (value < min) {
							//text = RemoveTrailingZeros(wxString::Format(wxT("%f"), min));    // limit to min
							text = RemoveTrailingZeros(wxString::Format(wxT("%f"), value));  // the user input value
						}
					}
				}
				else // if (paraGroup.m_dataType == Item_float || paraGroup.m_dataType == Item_int || paraGroup.m_dataType == Item_Percent)
				{
					// set the focus to the item to avoid another kill focus event to this editing control on Mac -- xavier
					this->SetFocus();

					wxString newRealEditValue;
					if (!ValueCheck(paraGroup, text, newRealEditValue)) {
						// recover the last valid input
						updateUi(prusaKeyString, paraGroup.m_UIvalue);
						return;
					}
					text = newRealEditValue;

					double value;
					text.ToDouble(&value);
					if (value > max)
						text = RemoveTrailingZeros(wxString::Format(wxT("%f"), max));
					else if (value < min)
						text = RemoveTrailingZeros(wxString::Format(wxT("%f"), min));

					//TODO  should do this ?
					if (paraGroup.m_dataType == Item_Percent)
						text = text + "%";
				}

				updateUi(prusaKeyString, text + (havePercent ? "%" : ""));
			}

			wxString strLabel = paraGroup.m_pLabel->GetLabelText();
			if (strLabel.Contains('\n'))
				strLabel.Replace('\n', "");

			//auto dataMap = m_parameterData.getItemMap(m_tabTitle);
			//std::string prusaKeyString = dataMap[strLabel].ToStdString();
			auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
			std::string prusaValue = printConfig.opt_serialize(prusaKeyString);
			wxString wxPrusaValue = prusaValue;

			wxString realData = "";
			bool bdirty = false;
			wxVariant cfgData = text;
			if ("support_material_contact_distance" == prusaKeyString)
			{
				if (text != wxPrusaValue)
				{
					if (pCBox->FindString(wxPrusaValue))
					{
						bdirty = true;
						resetBtn->Show();
					}
					else
					{
						int prusaIndex = wxAtoi(wxPrusaValue);
						if (index == prusaIndex)
						{
							bdirty = false;
							resetBtn->Hide();
						}
						else
						{
							bdirty = true;
							resetBtn->Show();
						}
					}
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else if ("support_material_bottom_contact_distance" == prusaKeyString)
			{

				if (text != wxPrusaValue)
				{
					if (pCBox->FindString(wxPrusaValue))
					{

						if (text == _L("Same as top"))
						{
							bdirty = false;
							resetBtn->Hide();
						}
						else
						{
							bdirty = true;
							resetBtn->Show();
						}
					}
					else
					{
						int prusaIndex = wxAtoi(wxPrusaValue);
						if (index == prusaIndex)
						{
							bdirty = false;
							resetBtn->Hide();
						}
						else
						{
							bdirty = true;
							resetBtn->Show();
						}
					}
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else if ("support_material_interface_layers" == prusaKeyString)
			{

				if (wxPrusaValue == ("0"))
					realData = _L("0 (off)");
				else if (wxPrusaValue == ("2"))
					realData = _L("2 (default)");
				else if (wxPrusaValue == ("3"))
					realData = _L("3 (heavy)");
				else if (wxPrusaValue == ("1"))
					realData = _L("1 (light)");
				else
					realData = wxPrusaValue;

				if (text != realData)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else if ("support_material_bottom_interface_layers" == prusaKeyString)
			{
				int prusaIndex = wxAtoi(wxPrusaValue);
				int iRealSetValue = index - 1;
				if (iRealSetValue == prusaIndex)
				{
					bdirty = false;
					resetBtn->Hide();
				}
				else
				{
					bdirty = true;
					resetBtn->Show();
				}
			}
			else if ("fill_density" == prusaKeyString || "infill_anchor" == prusaKeyString || "infill_anchor_max" == prusaKeyString)
			{
				if (text != prusaValue)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else
			{
				resetBtn->Hide();
			}

			if (bdirty)
			{
				ItemDirtyData dirtyData;
				dirtyData.tabName = m_tabTitle;
				dirtyData.titleName = m_title;
				dirtyData.prusaKey = prusaKeyString;
				dirtyData.ankerKey = strLabel;
				dirtyData.dataType = paraGroup.m_dataType;
				m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
			}
			else
			{
				auto iter = m_dirtyMap.find(prusaKeyString);
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);

			}
			wxWindowUpdateLocker updateLocker(resetBtn);
			onDatachanged(false, prusaKeyString);
			onUpdateResetBtn();
			updateModelParams(paraGroup, wxVariant(cfgData), prusaKeyString, strLabel);
			//Refresh();
			Layout();
			});
		pCBox->Bind(wxEVT_COMBOBOX, [this, resetBtn, pCBox, configOptionKey, paraGroup](wxCommandEvent& event) {
			if (!resetBtn || !pCBox)
				return;

			int index = 0;
			wxString text = wxString();

			index = pCBox->GetSelection();
			text = pCBox->GetValue();

			auto it = this->m_optionParameterMap.find(configOptionKey);
			if (it == this->m_optionParameterMap.end())
				return;

			auto& paraGroup = it->second;
			std::string prusaKeyString = paraGroup.m_optionKey.ToStdString();
			if (prusaKeyString.empty())
			{
				return;
			}
			index = pCBox->GetSelection();
			text = pCBox->GetValue();
			//by samuel, just return when no change
			if (isExceptionHadleEditParam(configOptionKey) && CheckExceptionParamValue(text, configOptionKey, paraGroup.m_unit))
			{
				return;
			}
			else
			{
				wxString strTrimValue = text;
				strTrimValue.Replace(wxT(" "), wxT(""));
				strTrimValue.Replace(paraGroup.m_unit, wxT(""));
				strTrimValue.Replace(" ", "");
				wxString strUIValue = paraGroup.m_UIvalue;
				strUIValue.Replace(wxT(" "), wxT(""));
				strUIValue.Replace(paraGroup.m_unit, wxT(""));
				strUIValue.Replace(" ", "");
				if (text == strUIValue || strTrimValue == strUIValue)
				{
					return;
				}
				text = strTrimValue;
			}

			updateUi(prusaKeyString, text);

			wxString strLabel = paraGroup.m_pLabel->GetLabelText();

			if (strLabel.Contains('\n'))
				strLabel.Replace('\n', "");

			auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
			std::string prusaValue = printConfig.opt_serialize(prusaKeyString);
			wxString wxPrusaValue = prusaValue;

			wxString realData = "";
			bool bdirty = false;

			wxVariant cfgData = text;
			if ("support_material_contact_distance" == prusaKeyString)
			{
				if (wxPrusaValue.size() > 0 && wxPrusaValue.size() <= 3)
				{
					if (wxPrusaValue.Contains("0.1"))
						realData = _L("0.1 (detachable)");
					else if (wxPrusaValue.Contains("0.2"))
						realData = _L("0.2 (detachable)");
					else
						realData = _L("0 (soluble)");

					if (text != realData)
					{
						bdirty = true;
						resetBtn->Show();
					}
					else
					{
						bdirty = false;
						resetBtn->Hide();
					}
				}
				else
				{
					if (text != wxPrusaValue)
					{
						bdirty = true;
						resetBtn->Show();
					}
					else
					{
						bdirty = false;
						resetBtn->Hide();
					}
				}
			}
			else if ("support_material_bottom_contact_distance" == prusaKeyString)
			{
				if (wxPrusaValue == "0")
					realData = _L("Same as top");
				else if (wxPrusaValue == "0.1")
					realData = _L("0.1");
				else if (wxPrusaValue == "0.2")
					realData = _L("0.2");

				if (text != realData)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else if ("support_material_interface_layers" == prusaKeyString)
			{

				if (wxPrusaValue == ("0"))
					realData = _L("0 (off)");
				else if (wxPrusaValue == ("2"))
					realData = _L("2 (default)");
				else if (wxPrusaValue == ("3"))
					realData = _L("3 (heavy)");
				else if (wxPrusaValue == ("1"))
					realData = _L("1 (light)");
				else
					realData = wxPrusaValue;

				if (text != realData)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else if ("support_material_bottom_interface_layers" == prusaKeyString)
			{
				if (wxPrusaValue == ("0"))
					realData = _L("0 (off)");
				else if (wxPrusaValue == ("-1"))
					realData = _L("Same as top");
				else if (wxPrusaValue == ("2"))
					realData = _L("2 (default)");
				else if (wxPrusaValue == ("3"))
					realData = _L("3 (heavy)");
				else if (wxPrusaValue == ("1"))
					realData = _L("1 (light)");
				else
					realData = wxPrusaValue;

				if (text != realData)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}

			else if ("fill_density" == prusaKeyString || "infill_anchor" == prusaKeyString || "infill_anchor_max" == prusaKeyString)
			{
				wxString editValue = text.ToStdString();
				wxString strUnit = ' ' + _L("mm");
				editValue.Replace('%', "");
				editValue.Replace(strUnit, "");

				wxString presetValue = prusaValue;
				presetValue.Replace('%', "");
				presetValue.Replace("mm", "");

				if (editValue != presetValue)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}
			}
			else
			{
				resetBtn->Hide();
			}

			if (bdirty)
			{
				ItemDirtyData dirtyData;
				dirtyData.tabName = m_tabTitle;
				dirtyData.titleName = m_title;
				dirtyData.prusaKey = prusaKeyString;
				dirtyData.ankerKey = strLabel;
				dirtyData.dataType = paraGroup.m_dataType;
				m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
			}
			else
			{
				auto iter = m_dirtyMap.find(prusaKeyString);
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);
			}
			wxWindowUpdateLocker updateLocker(resetBtn);
			onDatachanged(false, prusaKeyString);
			onUpdateResetBtn();
			updateModelParams(paraGroup, wxVariant(cfgData), prusaKeyString, strLabel);
			//Refresh();
			//Layout();
			});

		pTitleSizer->Add(pCBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
		pTitleSizer->AddSpacer(4);
		paraGroup.m_pWindow = pCBox;
		paraGroup.pLineSizer = pTitleSizer;
		groupItemPtr->_window = pCBox;
		groupItemPtr->_boxSizer = pTitleSizer;
	}break;
	case ItemEditUinit:
	{
		wxString strUnit = "";
		if (strList.size() == 1)
			strUnit = *strList.begin();

		AnkerLineEditUnit* pItemEdit = new AnkerLineEditUnit(this, strUnit, Body_10, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
		item = pItemEdit;
		int lineEditHeight = R_PANEL_CONTROLS_HEIGHT;
		pItemEdit->setLineEditFont(Head_14);
		pItemEdit->SetMaxSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
		pItemEdit->SetMinSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
		pItemEdit->SetSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
#ifdef __WXOSX__
		int shrink = 5;
#else
		int shrink = 4;
#endif

		wxClientDC dc(this);
		dc.SetFont(Body_10);
		wxSize textSize = dc.GetTextExtent(strUnit);
		int unitHeight = (textSize.GetHeight() == 0) ? shrink : textSize.GetHeight();
		pItemEdit->getTextEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getTextEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getTextEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getUnitEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - unitHeight));
		pItemEdit->getUnitEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - unitHeight));
		pItemEdit->getUnitEdit()->SetSize(AnkerSize(-1, lineEditHeight - unitHeight));

		pItemEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this, resetBtn, pItemEdit, configOptionKey](wxCommandEvent& event) {
			CallAfter([=]() {
				if (!pItemEdit || !resetBtn)
					return;

				auto it = this->m_optionParameterMap.find(configOptionKey);
				if (it == this->m_optionParameterMap.end())
					return;

				auto& paraGroup = it->second;
				std::string prusaKeyString = paraGroup.m_optionKey.ToStdString();
				if (prusaKeyString.empty())
					return;

				float min = 0.0f, max = 0.0f;
				GetOptionMaxMinDefVal(prusaKeyString, min, max);

				wxString strUnit = _L("mm");
				wxString editValue = pItemEdit->GetValue();
				wxString realEditValue = editValue;

				realEditValue.Replace(" ", "");
				realEditValue.Replace(strUnit, "");

				// by samuel, no need to go ahead if no change with the last value 
				if (editValue == paraGroup.m_UIvalue || realEditValue == paraGroup.m_UIvalue)
				{
					return;
				}

				bool inputHavePercent = false;
				if (editValue.EndsWith("%")) {
					inputHavePercent = true;
				}
				realEditValue.Replace('%', "");

				wxString newRealEditValue;
				if (!ValueCheck(paraGroup, realEditValue, newRealEditValue)) {
					// recover the last valid input
					updateUi(prusaKeyString, paraGroup.m_UIvalue);
					return;
				}
				realEditValue = newRealEditValue;
				if (!isNumber(realEditValue.ToStdString())) {
					//Slic3r::GUI::MessageDialog dialog(m_parent, _L("Invalid numeric input"), _L("Parameter validation") + ": " + prusaKeyString, wxOK );
					//dialog.SetSize(wxSize(320, 250));
					//dialog.ShowModal();
					// recover the last valid input
					updateUi(prusaKeyString, paraGroup.m_UIvalue);
					return;
				}

				if (paraGroup.m_dataType == Item_floatOrPercent)
				{
					double value;
					realEditValue.ToDouble(&value);
					if (!inputHavePercent) {
						if (value > max) {
							// choose float data or percent data
							const wxString msg_text = wxString::Format("Is it %s%% or %s %s?\nYES for %s%%, \nNO for %s %s.", realEditValue, realEditValue, "mm", realEditValue, realEditValue, "mm");
							Slic3r::GUI::MessageDialog dialog(m_parent, msg_text, _L("Parameter validation") + ": " + prusaKeyString, wxYES | wxNO);
							dialog.SetSize(wxSize(400, 250));
							if (dialog.ShowModal() == wxID_YES)
								realEditValue = RemoveTrailingZeros(realEditValue) + "%";
							else {
								//realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), max));    // limit to max
								realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), value));  // the user input value
							}
						}
						else if (value < min) {
							//realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), min));  // limit to min
							realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), value));  // the user input value
						}
					}
				}
				else if (paraGroup.m_dataType == Item_float)
				{
					double value;
					realEditValue.ToDouble(&value);
					if (value > max)
						realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), max));
					else if (value < min)
						realEditValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), min));
				}
				updateUi(prusaKeyString, realEditValue + (inputHavePercent ? "%" : ""));


				wxString strLabel = paraGroup.m_pLabel->GetLabelText();
				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

				wxString prusaValue = printConfig.opt_serialize(prusaKeyString);
				bool prusaValueHavePercent = prusaValue.Contains("%") ? true : false;
				wxString realPrusaValue = prusaValue;
				realPrusaValue.Replace('%', "");
				realPrusaValue.Replace("mm", "");

				//if use invaild data and set default value for layer height.
				if (prusaKeyString == "layer_height")
				{
					double dValue = 0;
					realEditValue.ToDouble(&dValue);

					if (dValue < 0.01)
					{
						realEditValue = "0.01";
						updateUi(_L("layer_height"), realEditValue);
					}
				}

				wxVariant cfgData = (realEditValue + (inputHavePercent ? "%" : ""));
				bool bdirty = false;
				if (realEditValue == realPrusaValue && (inputHavePercent && prusaValueHavePercent || !inputHavePercent && !prusaValueHavePercent))
				{
					bdirty = false;
					resetBtn->Hide();

				}
				else
				{
					bdirty = true;
					resetBtn->Show();
				}

				if (bdirty)
				{
					ItemDirtyData dirtyData;
					dirtyData.tabName = m_tabTitle;
					dirtyData.titleName = m_title;
					dirtyData.prusaKey = prusaKeyString;
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;
					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);
				}
				wxWindowUpdateLocker updateLocker(this);
				onDatachanged(false, prusaKeyString);
				onUpdateResetBtn();
				updateModelParams(paraGroup, wxVariant(cfgData), prusaKeyString, strLabel);
				Refresh();
				Layout();
				});
			});

		pTitleSizer->Add(pItemEdit, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
		pTitleSizer->AddSpacer(4);

		paraGroup.m_pWindow = pItemEdit;
		paraGroup.pLineSizer = pTitleSizer;

		groupItemPtr->_window = pItemEdit;
		groupItemPtr->_boxSizer = pTitleSizer;
	}
	break;
	case ItemCheckBox:
	{
		wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
		uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap uncheckScaledBitmap(uncheckImage);
		wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check.png")), wxBITMAP_TYPE_PNG);
		checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap checkScaledBitmap(checkImage);

		wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
		disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disUncheckScaledBitmap(disuncheckImage);

		wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
		discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disCheckScaledBitmap(discheckImage);

		AnkerCheckBox* pCheckBox = new AnkerCheckBox(this,
			uncheckScaledBitmap.ConvertToImage(),
			checkScaledBitmap.ConvertToImage(),
			disUncheckScaledBitmap.ConvertToImage(),
			disCheckScaledBitmap.ConvertToImage(),
			wxString(""),
			wxFont(),
			wxColour("#FFFFFF"),
			wxID_ANY);
		pCheckBox->SetCursor(handCursor);
		pCheckBox->SetWindowStyleFlag(wxBORDER_NONE);
		pCheckBox->SetBackgroundColour(wxColour("#292A2D"));
		pCheckBox->SetMinSize(AnkerSize(16, 16));
		pCheckBox->SetMaxSize(AnkerSize(16, 16));
		pCheckBox->SetSize(AnkerSize(16, 16));

		pTitleSizer->Add(pCheckBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
		pTitleSizer->AddSpacer(4);
		pCheckBox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, [this, configOptionKey, resetBtn, pCheckBox, paraGroup](wxCommandEvent& event) {
			if (!resetBtn)
				return;

			bool isChecked = pCheckBox->getCheckStatus();
			wxString strLabel = paraGroup.m_pLabel->GetLabelText();

			if (strLabel.Contains('\n'))
				strLabel.Replace('\n', "");


			std::string prusaKeyString = configOptionKey.ToStdString();
			if (prusaKeyString.empty())
				return;
			auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

			bool hasKey = printConfig.has(prusaKeyString);
			if (!hasKey) {
				ANKER_LOG_ERROR << "printConfig have no this option key:" << prusaKeyString << ", cann't set it's value";
				return;
			}

			bool option_value = printConfig.opt_bool(prusaKeyString);
			bool bdirty = false;
			if (isChecked != option_value)
			{
				bdirty = true;
				resetBtn->Show();
			}
			else
			{
				bdirty = false;
				resetBtn->Hide();
			}

			if (bdirty)
			{
				ItemDirtyData dirtyData;
				dirtyData.tabName = m_tabTitle;
				dirtyData.titleName = m_title;
				dirtyData.prusaKey = prusaKeyString;
				dirtyData.ankerKey = strLabel;
				dirtyData.dataType = paraGroup.m_dataType;
				m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
			}
			else
			{
				auto iter = m_dirtyMap.find(prusaKeyString);
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);
			}

			wxWindowUpdateLocker updateLocker(this->GetParent());
			RefreashDependParamState(configOptionKey, (int)isChecked);
			onDatachanged(false, prusaKeyString);
			onUpdateResetBtn();
			updateModelParams(paraGroup, wxVariant(isChecked), prusaKeyString, strLabel);
			//Refresh();
			Layout();
			});

		paraGroup.m_pWindow = pCheckBox;
		paraGroup.pLineSizer = pTitleSizer;

		groupItemPtr->_window = pCheckBox;
		groupItemPtr->_boxSizer = pTitleSizer;
	}
	break;
	case ItemSpinBox:
	{
		wxString unit = "";
		if (("standby_temperature_delta") == configOptionKey)
			unit = _L("∆°C");
		else if (("support_material_threshold") == configOptionKey)
			unit = _L("°");

		AnkerSpinEdit* pItemEdit = new AnkerSpinEdit(this, wxColour(41, 42, 45), wxColour("#3F4043"), 4, unit, wxID_ANY);

		wxBoxSizer* pWidgetVSizer = new wxBoxSizer(wxHORIZONTAL);
		int lineEditHeight = R_PANEL_CONTROLS_HEIGHT;
		pItemEdit->SetMaxSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
		pItemEdit->SetMinSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
		pItemEdit->SetSize(AnkerSize(CONTROL_WIDTH, lineEditHeight));
		pItemEdit->SetFont(Head_14);
#ifdef __WXOSX__
		int shrink = 5;
#else
		int shrink = 4;
#endif
		pItemEdit->getTextEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getTextEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getTextEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getUnitEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getUnitEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
		pItemEdit->getUnitEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));

		pItemEdit->setLineEditFont(Head_14);

		pItemEdit->Bind(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED, [this, configOptionKey, resetBtn, pItemEdit](wxCommandEvent& event) {
			CallAfter([=]() {
				if (!resetBtn)
					return;

				auto it = this->m_optionParameterMap.find(configOptionKey);
				if (it == this->m_optionParameterMap.end())
					return;

				auto& paraGroup = it->second;
				std::string prusaKeyString = paraGroup.m_optionKey.ToStdString();
				if (prusaKeyString.empty())
					return;

				int uValue = 0;
				wxString strValue = pItemEdit->GetValue();
				strValue.Replace(" ", "");
				// by samuel, no need to go ahead if no change with the last value 
				if (strValue == paraGroup.m_UIvalue || pItemEdit->GetValue() == paraGroup.m_UIvalue)
				{
					return;
				}
				wxString newStrValue;
				if (!ValueCheck(paraGroup, strValue, newStrValue)) {
					// recover the last valid input
					updateUi(prusaKeyString, paraGroup.m_UIvalue);
					return;
				}
				strValue = newStrValue;

				if (!isNumber(strValue.ToStdString())) {
					Slic3r::GUI::MessageDialog dialog(m_parent, _L("Invalid numeric input"), _L("Parameter validation") + ": " + prusaKeyString, wxOK);
					dialog.SetSize(wxSize(320, 250));
					dialog.ShowModal();
					// recover the last valid input
					updateUi(prusaKeyString, paraGroup.m_UIvalue);
					return;
				}

				strValue.ToInt(&uValue);
				float min = 0.0f, max = 0.0f;
				GetOptionMaxMinDefVal(prusaKeyString, min, max);
				if (uValue > max) {
					uValue = max;
					//pItemEdit->SetValue(RemoveTrailingZeros(wxString::Format(wxT("%f"), max)));
					strValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), max));
				}
				else if (uValue < min) {
					uValue = min;
					//pItemEdit->SetValue(RemoveTrailingZeros(wxString::Format(wxT("%f"), min)));
					strValue = RemoveTrailingZeros(wxString::Format(wxT("%f"), min));
				}
				updateUi(prusaKeyString, strValue);

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
				float prusasValue = 0;
				if ("top_solid_min_thickness" == prusaKeyString || "bottom_solid_min_thickness" == prusaKeyString)
					prusasValue = printConfig.opt_float(prusaKeyString);
				else
					prusasValue = printConfig.opt_int(prusaKeyString);

				bool bdirty = false;
				if (uValue != prusasValue)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}

				if (bdirty)
				{
					ItemDirtyData dirtyData;
					dirtyData.tabName = m_tabTitle;
					dirtyData.titleName = m_title;
					dirtyData.prusaKey = configOptionKey;
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;

					m_dirtyMap.insert(std::make_pair(configOptionKey, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(configOptionKey);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);
				}

				wxWindowUpdateLocker updateLocker(resetBtn);
				onDatachanged(false, prusaKeyString);
				onUpdateResetBtn();
				updateModelParams(paraGroup, wxVariant(uValue), configOptionKey, strLabel);
				//Refresh();
				Layout();
				});
			});

		pTitleSizer->Add(pItemEdit, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
		pTitleSizer->AddSpacer(4);
		paraGroup.m_pWindow = pItemEdit;
		paraGroup.pLineSizer = pTitleSizer;

		groupItemPtr->_window = pItemEdit;
		groupItemPtr->_boxSizer = pTitleSizer;
		item = pItemEdit;
		break;
	}

	case ItemTextCtrl:
	{
		using namespace Slic3r::GUI;
		long style = uiConifg.bIsMultiLine ? wxTE_MULTILINE : wxTE_PROCESS_ENTER;
#ifdef _WIN32
		style |= wxBORDER_SIMPLE;
#endif
		wxTextCtrl* pTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style);
		pTextCtrl->SetBackgroundColour(wxColor(41, 42, 45));
		pTextCtrl->SetForegroundColour(wxColor(255, 255, 255));
		int lineEditHeight = uiConifg.iMinHeight;
		pTextCtrl->SetMaxSize(AnkerSize(-1, lineEditHeight));
		pTextCtrl->SetMinSize(AnkerSize(-1, lineEditHeight));
		pTextCtrl->SetSize(AnkerSize(-1, lineEditHeight));
		pTextCtrl->SetFont(Head_14);
		wxGetApp().UpdateDarkUI(pTextCtrl);


		if (!uiConifg.bIsFullWith)
		{
			pTitleSizer->Add(pTextCtrl, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
			//pTitleSizer->AddSpacer(20);
		}
		else
		{
			pFullLineSizer->Add(pTextCtrl, 1, wxALL | wxEXPAND, 0);
			pFullLineSizer->AddSpacer(20);
		}



		if (style & wxTE_PROCESS_ENTER) {
			//			pTextCtrl->Bind(wxEVT_TEXT_ENTER, ([this, pTextCtrl](wxEvent& e)
			//				{
			//#if !defined(__WXGTK__)
			//					e.Skip();
			//					pTextCtrl->GetToolTip()->Enable(true);
			//#endif // __WXGTK__
			//					//EnterPressed enter(this);
			//					//propagate_value();
			//				}), pTextCtrl->GetId());
		}
		//
		//		pTextCtrl->Bind(wxEVT_LEFT_DOWN, ([pTextCtrl](wxEvent& event)
		//			{
		//				//! to allow the default handling
		//				event.Skip();
		//				//! eliminating the g-code pop up text description
		//				bool flag = false;
		//#ifdef __WXGTK__
		//				// I have no idea why, but on GTK flag works in other way
		//				flag = true;
		//#endif // __WXGTK__
		//				pTextCtrl->GetToolTip()->Enable(flag);
		//			}), pTextCtrl->GetId());


		pTextCtrl->Bind(wxEVT_KILL_FOCUS, ([this, pTextCtrl, configOptionKey, resetBtn, paraGroup, uiConifg](wxEvent& e)
			{
				e.Skip();
#if !defined(__WXGTK__)
				pTextCtrl->GetToolTip()->Enable(true);
#endif // __WXGTK__
				//if (!bEnterPressed)
				//	propagate_value();

				if (!resetBtn)
					return;
				wxString strValue = pTextCtrl->GetValue();
				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				std::string prusaKeyString = paraGroup.m_optionKey.ToStdString();
				if (prusaKeyString.empty())
					return;
				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
				std::string prusasValue;
				if (uiConifg.bIsMultiStr)
				{
					std::vector<std::string> showTextVec;
					showTextVec = printConfig.opt_strings(prusaKeyString);
					if (showTextVec.size() > 0)
					{
						prusasValue = Slic3r::join_strings_with_newlines(showTextVec);
					}
				}
				else
				{
					prusasValue = printConfig.opt_string(prusaKeyString);
				}


				bool bdirty = false;

				if (strValue != prusasValue)
				{
					bdirty = true;
					resetBtn->Show();
				}
				else
				{
					bdirty = false;
					resetBtn->Hide();
				}

				if (bdirty)
				{
					ItemDirtyData dirtyData;
					dirtyData.tabName = m_tabTitle;
					dirtyData.titleName = m_title;
					dirtyData.prusaKey = configOptionKey;
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;
					m_dirtyMap.insert(std::make_pair(configOptionKey, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(configOptionKey);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);
				}

				pTextCtrl->SelectNone();

				wxWindowUpdateLocker updateLocker(this);
				onDatachanged(false, prusaKeyString);
				onUpdateResetBtn();
				Layout();
			}), pTextCtrl->GetId());
		paraGroup.m_pWindow = pTextCtrl;
		paraGroup.pLineSizer = pTitleSizer;
		groupItemPtr->_window = pTextCtrl;
		groupItemPtr->_boxSizer = pTitleSizer;
		item = pTextCtrl;
		break;
	}
	default:
		break;
	}

	m_optionParameterMap.insert(std::pair<wxString, PARAMETER_GROUP>(configOptionKey, paraGroup));
	if (m_group_property) {
		m_group_property->propertyItems.emplace_back(groupItemPtr);
	}

	pTitleSizer->AddSpacer(20);
	if (!uiConifg.bIsFullWith)
	{
		m_pMainVSizer->Add(pTitleSizer, 0, wxALL | wxEXPAND, 0);
	}
	else
	{
		m_pMainVSizer->Add(pTitleSizer, 0, wxALL | wxEXPAND, 0);
		m_pMainVSizer->Add(pFullLineSizer, 1, wxALL | wxEXPAND, 0);
		m_pMainVSizer->AddSpacer(8);
	}

	return item;
}

void AnkerPrintParaItem::updateModelParams(const PARAMETER_GROUP& paraGroup, wxVariant cfgData, const wxString& prusaKeyString, wxString& strLabel)
{
	ItemInfo dataInfo;
	dataInfo.paramDataType = paraGroup.m_dataType;
	dataInfo.paramDataValue = cfgData;
	wxScrolledWindow* pTabItemScrolledWindow = dynamic_cast<wxScrolledWindow*>(GetParent());    // AnkerParameterPanel.m_pTabItemScrolledWindow
	if (!pTabItemScrolledWindow)
		return;
	wxPanel* pContentWidget = dynamic_cast<wxPanel*>(pTabItemScrolledWindow->GetParent());  // AnkerParameterPanel.m_contentWidget
	if (!pContentWidget)
		return;
	AnkerParameterPanel* pWidget = dynamic_cast<AnkerParameterPanel*>(pContentWidget->GetParent());
	Slic3r::ModelConfig* cfgConfig = nullptr;
	if (pWidget)
	{
		if (pWidget->getObjConfig(cfgConfig))
		{
			std::string stdPrusaKey = prusaKeyString.ToStdString();
			std::string ankerKey = strLabel.ToStdString();
			saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
		}
	}
}

bool AnkerPrintParaItem::hsaDirtyData()
{
	if (m_dirtyMap.size() > 0)
		return true;
	else
		return false;
}

ItemInfo AnkerPrintParaItem::getWidgetValue(const wxString& optionKey)
{
	auto iter = m_optionParameterMap.find(optionKey);
	ItemInfo valueData;
	if (iter != m_optionParameterMap.end())
	{
		valueData.paramDataType = iter->second.m_dataType;
		auto widgetType = iter->second.m_type;
		switch (widgetType)
		{
		case ItemComBox:
		{
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
			int index = pComBox->GetSelection();
			valueData.paramDataValue = wxVariant(pComBox->GetSelection());
			return valueData;
		}
		break;
		case ItemEditUinit:
		{
			AnkerLineEditUnit* pEditRange = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			wxString data = pEditRange->getValue();

			//if use invaild data and set default value for layer height.
			if (optionKey == _L("layer_height"))
			{
				double dValue = 0;
				data.ToDouble(&dValue);

				if (dValue < 0.01)
				{
					data = "0.01";
					updateUi(_L("layer_height"), data);
				}
			}

			valueData.paramDataValue = wxVariant(data);
			return valueData;
		}
		break;
		case ItemCheckBox:
		{
			AnkerCheckBox* pCheckBox = static_cast<AnkerCheckBox*>(iter->second.m_pWindow);
			valueData.paramDataValue = wxVariant(pCheckBox->getCheckStatus());
			return valueData;
		}
		break;
		case ItemSpinBox:
		{
			AnkerLineEditUnit* pEditUnit = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			valueData.paramDataValue = wxVariant(pEditUnit->getValue());
			return valueData;
		}
		break;
		case ItemEditBox:
		{
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
			int index = pComBox->GetSelection();
			auto currentStr = pComBox->GetValue();

			valueData.paramDataValue = wxVariant(currentStr);
			return valueData;
		}

		case ItemTextCtrl:
		{
			wxTextCtrl* pTextCtrl = static_cast<wxTextCtrl*>(iter->second.m_pWindow);
			auto currentStr = pTextCtrl->GetValue();
			valueData.paramDataValue = wxVariant(currentStr);
			return valueData;
		}
		default:
			break;
		}
	}
	return valueData;
}

bool AnkerPrintParaItem::isExistOption(const wxString& optionKey)
{
	bool res = false;
	auto iter = m_optionParameterMap.find(optionKey);

	if (iter != m_optionParameterMap.end())
		return true;
	else
		return false;

	return res;
}
void AnkerPrintParaItem::updateUi(const wxString& optionKey, 
										wxVariant data, 
										wxString tooltipsValue, 
										bool isReset /*= false*/, 
										bool bEnable/* = true*/, 
										bool bShown /*= true*/)
{
	//control tool tips
	wxWindow* pCurrentWidget = nullptr;
	auto iter = m_optionParameterMap.find(optionKey);
	if (iter != m_optionParameterMap.end())
	{
		auto controlType = iter->second.m_type;
		iter->second.pLineSizer->Show(iter->second.bShown);
		if (m_dirtyMap.find("optionKey") != m_dirtyMap.end())
		{
			iter->second.m_pBtn->Show();
		}
		else
		{
			iter->second.m_pBtn->Hide();
		}
		iter->second.m_pWindow->Enable(iter->second.bEnable);
		

		switch (controlType)
		{
		case ItemComBox:
		{
			int Itemvalue = 0;
			wxString itemStrValue = "";
			
			if (!isReset)
			{
				itemStrValue = data.GetString();
				Itemvalue = data.GetInteger();
			}
			else
			{
				itemStrValue = "";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
				Itemvalue = 0;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
			pCurrentWidget = pComBox;
			if (pComBox)
			{
				pCurrentWidget = pComBox;
				int cbIndex = 0;
				tooltipsValue.ToInt(&cbIndex);
				int count = pComBox->GetCount();
				if (data.GetType() == wxT("string")) {
					pComBox->SetStringSelection(itemStrValue);
					iter->second.m_UIvalue = itemStrValue;
				}
				else {
					pComBox->SetSelection(Itemvalue);
					iter->second.m_UIvalue = pComBox->GetString(Itemvalue);
				}
				if (cbIndex < count)
				{
					if (cbIndex < 0)
						cbIndex = cbIndex + 1;

					tooltipsValue = pComBox->GetString(cbIndex);
				}
				RefreashDependParamState(optionKey, Itemvalue);
			}
			break;
		}
		
		case ItemEditBox:
		{
			int Itemvalue = 0;
			wxString itemStrValue = "";

			if (!isReset)
			{
				itemStrValue = data.GetString();
				Itemvalue = data.GetInteger();
			}
			else
			{
				itemStrValue = "";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
				Itemvalue = 0;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
			pCurrentWidget = pComBox;
			if (pComBox)
			{
				if (isExceptionHadleEditParam(optionKey))
				{
					HandleExceptionPram(optionKey, pComBox, itemStrValue, iter);
				}
				else
				{
					pComBox->SetValue(itemStrValue);
					iter->second.m_UIvalue = itemStrValue;
				}
			}
			break;
		}
		case ItemEditUinit:
		{
			wxString Itemvalue = { "" };
			AnkerLineEditUnit* pEditUnit = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if (!isReset)
				Itemvalue = data.GetString();
			else
			{
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			if (pEditUnit)
			{
				pCurrentWidget = pEditUnit;
				// check itemvalue is contain unit flag， if true remove the unit flag
				if (Itemvalue.EndsWith(pEditUnit->getUnit()))
				{
					Itemvalue.RemoveLast(pEditUnit->getUnit().length());
				}

				pEditUnit->setValue(Itemvalue);
				iter->second.m_UIvalue = Itemvalue;
			}
			break;
		}
		
		case ItemCheckBox:
		{
			AnkerCheckBox* pCheckBox = static_cast<AnkerCheckBox*>(iter->second.m_pWindow);
			bool Itemvalue = true;
			if (!isReset)
				Itemvalue = data.GetBool();
			else
			{
				Itemvalue = false;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			if (pCheckBox)
			{
				pCurrentWidget = pCheckBox;
				pCheckBox->setCheckStatus(Itemvalue);
			}
			break;
		}
		
		case ItemSpinBox:
		{
			wxString Itemvalue = { "" };
			if (!isReset)
				Itemvalue = data.GetString();
			else
			{
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			AnkerLineEditUnit* pEditRange = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if (pEditRange)
			{
				pCurrentWidget = pEditRange;
				pEditRange->setValue(Itemvalue);
				iter->second.m_UIvalue = Itemvalue;
			}
			break;
		}
	

		case ItemTextCtrl:
		{
			wxString Itemvalue = { "" };
			if (!isReset)
				Itemvalue = data.GetString();
			else
			{
				Itemvalue = "";
			}

			wxTextCtrl* pTextCtrl = static_cast<wxTextCtrl*>(iter->second.m_pWindow);
			if (pTextCtrl)
			{
				pCurrentWidget = pTextCtrl;
				pTextCtrl->SetValue(Itemvalue);
				iter->second.m_UIvalue = Itemvalue;
			}
			break;
		}

		default:
			break;
		}

		//if (pCurrentWidget)
		//{
		//	auto optiontip = GetOptionTip(optionKey);
		//	if (optiontip.empty())
		//		return;

		//	//auto defaultValue = _L("default value") + "\t: " + tooltipsValue;
		//	auto parameterLabel = _L("parameter name") + "\t: " + optionKey;
		//	//auto realWidgetToolTips = describeStr + "\n" + defaultValue + "\n" + parameterLabel;
		//	auto realWidgetToolTips = optiontip + "\n" + parameterLabel;
		//	if (ItemEditUinit == controlType || ItemSpinBox == controlType)
		//	{
		//		AnkerLineEditUnit* pEdit = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
		//		pEdit->SetToolTip(wxString(realWidgetToolTips));
		//	}
		//	else
		//		iter->second.m_pWindow->SetToolTip(realWidgetToolTips);
		//}
	}
}
void AnkerPrintParaItem::updateUi(const wxString& optionKey, wxVariant data, bool isReset /*= false*/)
{
	auto iter = m_optionParameterMap.find(optionKey);
	if (iter != m_optionParameterMap.end())
	{
		auto controlType = iter->second.m_type;
		switch (controlType)
		{
		case ItemComBox:
		{
			int Itemvalue = 0;
			wxString itemStrValue = "";

			if (!isReset)
			{
				itemStrValue = data.GetString();
				Itemvalue = data.GetInteger();
			}
			else
			{
				itemStrValue = "";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
				Itemvalue = 0;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
			if (pComBox)
			{
				if (data.GetType() == wxT("string")) {
					pComBox->SetStringSelection(itemStrValue);
					iter->second.m_UIvalue = itemStrValue;
				}
				else {
					pComBox->SetSelection(Itemvalue);
					iter->second.m_UIvalue = pComBox->GetString(Itemvalue);
				}
			}
		}
		break;
		case ItemEditBox:
		{
			int Itemvalue = 0;
			wxString itemStrValue = "";

			if (!isReset)
			{
				itemStrValue = data.GetString();
				Itemvalue = data.GetInteger();
			}
			else
			{
				itemStrValue = "";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
				Itemvalue = 0;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}
			wxOwnerDrawnComboBox* pComBox = static_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);

			if (pComBox)
			{
				int comCount = pComBox->GetCount();
				if (data.GetType() == wxT("string")) {
					pComBox->SetValue(itemStrValue);
					iter->second.m_UIvalue = itemStrValue;
				}
				else
				{
					int comBoxCount = pComBox->GetCount();
					if (Itemvalue >= comBoxCount) {
						pComBox->SetValue(itemStrValue);
						iter->second.m_UIvalue = itemStrValue;
					}
					else {
						pComBox->SetSelection(Itemvalue);
						iter->second.m_UIvalue = pComBox->GetString(Itemvalue);
					}
				}
			}
		}
		break;
		case ItemEditUinit:
		{
			wxString Itemvalue = { "" };
			AnkerLineEditUnit* pEditUnit = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if (!isReset)
				Itemvalue = data.GetString();
			else
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);

			if (pEditUnit) {
				pEditUnit->setValue(Itemvalue);
				iter->second.m_UIvalue = Itemvalue;
			}
		}
		break;
		case ItemCheckBox:
		{
			AnkerCheckBox* pCheckBox = static_cast<AnkerCheckBox*>(iter->second.m_pWindow);
			bool Itemvalue = true;
			if (!isReset)
				Itemvalue = data.GetBool();
			else
				Itemvalue = false;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);			

			if (pCheckBox)
				pCheckBox->setCheckStatus(Itemvalue);
		}
		break;
		case ItemSpinBox:
		{
			wxString Itemvalue = { "" };
			if (!isReset)
				Itemvalue = data.GetString();
			else
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);

			AnkerLineEditUnit* pEditRange = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if (pEditRange) {
				pEditRange->setValue(Itemvalue);
				iter->second.m_UIvalue = Itemvalue;
			}
		}
		break;
		default:
			break;
		}
	}
}

void AnkerPrintParaItem::showOptionHighlight(const wxString& optionKey, wxColour labelColor)
{
	auto iter = m_optionParameterMap.find(optionKey);
	static int times = 0;
	if (iter != m_optionParameterMap.end())
	{
		(*iter).second.m_pLabel->SetForegroundColour(m_colour);
		Refresh();
		m_HightLightTimer->Start(1000);
		m_pCurrentLabel = (*iter).second.m_pLabel;
		m_colour = labelColor;
	}
}

void AnkerPrintParaItem::onDatachanged(bool is_reset, const wxString& option_key)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS);
	evt.SetEventObject(this);
	evt.SetInt(is_reset ? 1 : 0);
	evt.SetString(option_key);
	wxPostEvent(this, evt);
}

void AnkerPrintParaItem::onUpdateResetBtn()
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE);
	evt.SetEventObject(this);
	wxPostEvent(this, evt);
}

void AnkerPrintParaItem::initUi()
{
	m_HightLightTimer = new wxTimer(this, wxID_ANY);
	m_HightLightTimer->SetOwner(this, wxID_ANY);
	Connect(m_HightLightTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler(AnkerPrintParaItem::OnTimer), NULL, this);
	SetBackgroundColour(wxColour("#292A2D"));
	m_pMainVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);

	BlinkingBitmap* pIcon = new BlinkingBitmap(this, m_icon.ToStdString());
	pIcon->activate();
	wxStaticText* pTitle = new wxStaticText(this, wxID_ANY, m_title);
	pTitle->SetFont(Head_14);
	pTitle->SetForegroundColour(wxColour("#FFFFFF"));

	wxPanel* pDividingLine = new wxPanel(this, wxID_ANY);
	pDividingLine->SetBackgroundColour(wxColour("#38393C"));
	pDividingLine->SetMinSize(AnkerSize(335, 1));
	pDividingLine->SetMaxSize(AnkerSize(335, 1));

	pTitleHSizer->AddSpacer(12);
	pTitleHSizer->Add(pIcon, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, 0);
	pTitleHSizer->AddSpacer(6);
	pTitleHSizer->Add(pTitle, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddSpacer(4);
	pTitleHSizer->Add(pDividingLine, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	m_pMainVSizer->AddSpacer(10);
	m_pMainVSizer->Add(pTitleHSizer, wxSizerFlags().Expand().Border(wxRight | wxLEFT, 12));
	m_pMainVSizer->AddSpacer(10);
	SetSizer(m_pMainVSizer);

	m_group_property->title_Sizer = pTitleHSizer;
}

void AnkerPrintParaItem::onResetBtnClicked(wxCommandEvent& event)
{
	wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
	if (!button)
		return;

	wxWindowUpdateLocker updateLocker(button);
	button->Hide();
	Refresh();
	Layout();

	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	auto iter = m_optionParameterMap.begin();
	while (iter != m_optionParameterMap.end())
	{
		if (button == iter->second.m_pBtn)
		{
			wxString option_key = iter->first;
			wxString wxidgetLabel = iter->second.m_pLabel->GetLabel();

			if (option_key.IsEmpty())
				return;

			ItemDataType dataTyoe = iter->second.m_dataType;
			wxVariant data;
			switch (dataTyoe)
			{
			case Item_int:
			{
				auto tempData = printConfig.opt_int(option_key.ToStdString());
				data = wxVariant(tempData);
				break;
			}
			case Item_bool:
			{
				bool value = printConfig.opt_bool(option_key.ToStdString());
				data = wxVariant(value);
				break;
			}
			case Item_float:
			{
				double value = printConfig.opt_float(option_key.ToStdString());
				data = wxVariant(value);
				break;
			}
			case Item_floatOrPercent:
			{
				auto strData = printConfig.get_abs_value(option_key.ToStdString());
				if (option_key != ("first_layer_height"))
				{
					data = printConfig.opt_serialize(option_key.ToStdString());
				}
				else
					data = strData;
				break;
			}
			case Item_serialize:
			{
				auto strdata = printConfig.opt_string(option_key.ToStdString());
				if (("infill_anchor") == option_key)
				{
					wxString tempdata = strdata;

					if (tempdata.Contains("1000"))
						data = _L("1000 mm");
					else if (tempdata.Contains("10"))
						data = _L("10 mm");
					else if (tempdata.Contains("5"))
						data = _L("5 mm");
					else if (tempdata.Contains("2"))
						data = _L("2 mm");
					else if (tempdata.Contains("1"))
						data = _L("1 mm");
					else if (tempdata == "0")
						data = _L("0 (no open anchors)");
					else 
						data = tempdata + _L(" mm");
				}
				else
					data = strdata;
				break;
			}
			case Item_serialize_no_unit:
			{
				wxString tempData = printConfig.opt_serialize(option_key.ToStdString());

				size_t pos = tempData.find('%');
				if (pos != wxString::npos) {
					tempData.erase(pos, 1);
				}
				data = tempData;
				break;
			}
	
			case Item_serialize_num:
			{
				std::string strdata = printConfig.opt_serialize(option_key.ToStdString());
				if (("support_material_bottom_interface_layers") == option_key)
				{
					wxString tempdata = strdata;
					if (tempdata == ("0"))
						data = _L("0 (off)");
					else if (tempdata == ("-1"))
						data = _L("Same as top");
					else if (tempdata == ("2"))
						data = _L("2 (default)");
					else if (tempdata == ("3"))
						data = _L("3 (heavy)");
					else if (tempdata == ("1"))
						data = _L("1 (light)");
					else
						data = tempdata;
				}
				else
					data = strdata;

				break;
			}
			case Item_Percent:
			{
				wxString tempData = printConfig.opt_serialize(option_key.ToStdString());
				//remove % 
				if (tempData.EndsWith("%"))
				{
					tempData.RemoveLast(1);
				}
				data = wxVariant(tempData);
				break;
			}
			
			case Item_enum_SeamPosition:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SeamPosition>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_ironing_pattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_print_order:
			{
				auto tempData = printConfig.opt_enum<Slic3r::WallSequence>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_PerimeterGeneratorType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::PerimeterGeneratorType>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_FuzzySkinType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::FuzzySkinType>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_InfillPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(option_key.ToStdString());
				auto tempStrData = printConfig.opt_serialize(option_key.ToStdString());
				std::locale loc;
				tempStrData[0] = std::toupper(tempStrData[0], loc);
				data = m_parameterData.m_fillPatternData[tempStrData];
				break;
			}
			case Item_enum_TopSurfaceSinglePerimeter:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SinglePerimeterType>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_IroningType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::IroningType>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			
			case Item_enum_DraftShield:
			{
				auto tempData = printConfig.opt_enum<Slic3r::DraftShield>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			
			case Item_enum_BrimType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::BrimType>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_SupportMaterialStyle:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_enum_SupportMaterialPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialPattern>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
		
			case Item_enum_SupportMaterialInterfacePattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			
			case Item_enum_SlicingMode:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SlicingMode>(option_key.ToStdString());
				data = wxVariant((int)tempData);
				break;
			}
			case Item_Multi_Strings:
			{
				const auto* post_process = printConfig.opt<Slic3r::ConfigOptionStrings>(option_key.ToStdString());
				if (post_process)
				{
					std::vector<std::string> postprocess_values = post_process->values;
					std::string showText = Slic3r::join_strings_with_newlines(postprocess_values);
					data = showText;
				}
				break;
			}

			default:
				break;
			}

			{
				auto iter = m_dirtyMap.find(option_key.ToStdString());
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);
			}

			switch (iter->second.m_type)
			{
			case ItemComBox:
			{
				wxOwnerDrawnComboBox* pComBox = dynamic_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
				if (!pComBox)
				{
					++iter;
					continue;
				}

				if (data.GetType() == wxT("string")) {
					pComBox->SetStringSelection(data.GetString());
					iter->second.m_UIvalue = data.GetString();
				}
				else{
					int index = data.GetInteger();
					pComBox->SetSelection(index);
					iter->second.m_UIvalue = pComBox->GetString(index);
				}
				wxWindowUpdateLocker updateLocker(this->GetParent());
				RefreashDependParamState(option_key, data.GetInteger());
				break;
			}
			case ItemEditBox:
			{
				wxOwnerDrawnComboBox* pComBox = dynamic_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
				if (!pComBox)
				{
					++iter;
					continue;
				}
				if (data.GetType() == wxT("string")) {
					wxString strValue = data.GetString();
					if (("fill_density") == option_key) {
						strValue = strValue + "%";
					}
					else if (("infill_anchor") == option_key || "infill_anchor_max" == option_key) {
						if (strValue == "0" && option_key == "infill_anchor")
							strValue = _L("0 (no open anchors)");
						else if (strValue == "0" && option_key == "infill_anchor")
							strValue = _L("0 (not anchored)");
						else if (strValue == "1000")
							strValue = _L("1000 (unlimited)");
						else
							strValue = strValue + " " + _L("mm");
					}
					pComBox->SetValue(strValue);
					iter->second.m_UIvalue = strValue;
				}
				else
				{
					int index = data.GetInteger();
					if (data.GetType() == wxT("double")) {
						wxString realData = data.GetString();
						if (("support_material_contact_distance") == option_key)
						{
							wxString strValue = data.GetString();
							if (strValue.size() > 0 && strValue.size() <= 3)
							{
								if (strValue.Contains("0.1"))
									realData = _L("0.1 (detachable)");
								else if (strValue.Contains("0.2"))
									realData = _L("0.2 (detachable)");
								else
									realData = _L("0 (soluble)");
							}
							else
								realData = _L("0 (soluble)");
						}
						if (("support_material_bottom_contact_distance") == option_key)
						{
							wxString strValue = data.GetString();

							if (index == 0)
								realData = _L("Same as top");
							else if (index == 1)
								realData = _L("0.1");
							else if (index == 2)
								realData = _L("0.2");
							else
								realData = _L("Same as top");
						}
						pComBox->SetValue(realData);
						iter->second.m_UIvalue = realData;
					}
					else
					{
						if ("support_material_bottom_interface_layers" == option_key)
						{
							// by samuel, the print param saved value is equal to the combo index + 1
							pComBox->SetSelection(index + 1);
							iter->second.m_UIvalue = pComBox->GetString(index + 1);
						}
						else if ( "support_material_interface_layers" == option_key)
						{
							pComBox->SetSelection(index);
							iter->second.m_UIvalue = pComBox->GetString(index);
						}
					}
				}
				break;
			}
			case ItemEditUinit:
			{
				AnkerLineEditUnit* pEdit = dynamic_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
				if (!pEdit)
				{
					Slic3r::SupportMaterialStyle emunData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(option_key.ToStdString());
					int tempData = m_parameterData.m_StyleMap[emunData];
					data = wxVariant(tempData);
				}

				pEdit->SetValue(data);
				iter->second.m_UIvalue = data.GetString();
				break;

			}
			case ItemCheckBox:
			{
				AnkerCheckBox* pCheckBox = dynamic_cast<AnkerCheckBox*>(iter->second.m_pWindow);
				if (!pCheckBox)
				{
					wxOwnerDrawnComboBox* pComBox = dynamic_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
					if (!pComBox)
					{
						++iter;
						continue;
					}
					if (data.GetType() == wxT("string"))
						pComBox->SetStringSelection(data.GetString());
					else
					{
						int tmpIndex = data.GetInteger();
						float tmpFIndex = data.GetDouble();


						if (("support_material_bottom_interface_layers") == option_key)
						{
							int index = data.GetInteger();
							pComBox->SetSelection(index + 1);
						}
						else
							pComBox->SetSelection(data.GetInteger());
					}
					wxWindowUpdateLocker updateLockerParent(this->GetParent());
					RefreashDependParamState(option_key, data.GetInteger());
					break;
				}

				wxWindowUpdateLocker updateLocker(this->GetParent());
				pCheckBox->setCheckStatus(data);
				RefreashDependParamState(option_key, data.GetInteger());
				break;
			}
			case ItemSpinBox:
			{
				AnkerSpinEdit* pEdit = dynamic_cast<AnkerSpinEdit*>(iter->second.m_pWindow);
				if (!pEdit)
				{
					++iter;
					continue;
				}

				pEdit->setValue(data);
				iter->second.m_UIvalue = data.GetString();
				break;
			}
		
			case ItemTextCtrl:
			{
				wxTextCtrl* pTextCtrl = dynamic_cast<wxTextCtrl*>(iter->second.m_pWindow);
				if (!pTextCtrl)
				{
					++iter;
					continue;
				}
				wxString showStr = wxString(data.GetString());
				pTextCtrl->SetValue(showStr);
				iter->second.m_UIvalue = showStr;
				break;
			}

			default:
				break;
			}

			onDatachanged(true, option_key);
			onUpdateResetBtn();
			updateModelParams(iter->second, wxVariant(data), option_key, wxidgetLabel);
			Refresh();
			Layout();

			return;
		}
		++iter;
	}
}

void AnkerPrintParaItem::OnTimer(wxTimerEvent& event)
{
	static int times = 0;
	if (!m_pCurrentLabel)
		return;

	if (times % 2 != 0)
		m_pCurrentLabel->SetForegroundColour(wxColour("#A9AAAB"));
	else
		m_pCurrentLabel->SetForegroundColour(m_colour);

	times++;
	if (times > 5)
	{
		m_HightLightTimer->Stop();
		times = 0;
	}
	Refresh();
}

void AnkerPrintParaItem::saveSetPresetPropertyValue(Slic3r::ModelConfig* printConfig,
	ItemInfo& ItemDataInfo,
	std::string& prusaProperty,
	std::string& AnkerPropertyName)
{
	auto propertyType = ItemDataInfo.paramDataType;//emun 
	wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
	wxString retrievedString = propertyValue.GetString();
	switch (propertyType)
	{
	case Item_int:
	{
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionInt((int)propertyValue.GetInteger()));
		break;
	}
	case Item_enum_FuzzySkinType:
	{
		Slic3r::FuzzySkinType fuzzySkinType = (Slic3r::FuzzySkinType)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::FuzzySkinType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::FuzzySkinType>(fuzzySkinType);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_PerimeterGeneratorType:
	{
		Slic3r::PerimeterGeneratorType perimeterGeneratorType = (Slic3r::PerimeterGeneratorType)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::PerimeterGeneratorType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::PerimeterGeneratorType>(perimeterGeneratorType);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_InfillPattern:
	{

		Slic3r::InfillPattern infillPattern;
		if (prusaProperty == ("fill_pattern"))
			infillPattern = m_parameterData.m_fillPatternMap[propertyValue.GetInteger()];
		else
			infillPattern = m_parameterData.m_tabfillPatternMap[propertyValue.GetInteger()];

		Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infillPattern);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_TopSurfaceSinglePerimeter:
	{
		Slic3r::SinglePerimeterType singlePerimeterType = (Slic3r::SinglePerimeterType)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SinglePerimeterType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SinglePerimeterType>(singlePerimeterType);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}

	case Item_enum_IroningType:
	{
		Slic3r::IroningType ironingType = (Slic3r::IroningType)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::IroningType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::IroningType>(ironingType);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_DraftShield:
	{
		Slic3r::DraftShield draftShield = (Slic3r::DraftShield)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::DraftShield>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::DraftShield>(draftShield);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_BrimType:
	{
		Slic3r::BrimType brimType = (Slic3r::BrimType)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::BrimType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::BrimType>(brimType);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_SupportMaterialStyle:
	{
		Slic3r::SupportMaterialStyle supportMaterialStyle = (Slic3r::SupportMaterialStyle)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialStyle>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialStyle>(supportMaterialStyle);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_SupportMaterialPattern:
	{
		Slic3r::SupportMaterialPattern supportMaterialPattern = (Slic3r::SupportMaterialPattern)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialPattern>(supportMaterialPattern);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_SupportMaterialInterfacePattern:
	{
		Slic3r::SupportMaterialInterfacePattern supportMaterialInterfacePattern = (Slic3r::SupportMaterialInterfacePattern)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialInterfacePattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialInterfacePattern>(supportMaterialInterfacePattern);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_SeamPosition:
	{
		Slic3r::SeamPosition seamPosition = (Slic3r::SeamPosition)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SeamPosition>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SeamPosition>(seamPosition);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_ironing_pattern:
	{
		Slic3r::InfillPattern infill = (Slic3r::InfillPattern)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infill);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_print_order:
	{
		Slic3r::WallSequence infill = (Slic3r::WallSequence)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::WallSequence>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::WallSequence>(infill);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_enum_SlicingMode:
	{
		Slic3r::SlicingMode sliceMode = (Slic3r::SlicingMode)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>(sliceMode);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_floatOrPercent:
	{
		if (prusaProperty == ("first_layer_height"))
		{
			printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, false));
		}
		else
		{
			wxString data = propertyValue.GetString();
			if (data.Contains('%'))
				printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, true));
			else //if(data.Contains("mm"))				
				printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, false));

		}
		break;
	}
	case Item_bool:
	{
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionBool(propertyValue));
		break;
	}

	case Item_float:
	{
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionFloat(propertyValue));
		break;
	}
	case Item_serialize:
	case Item_serialize_no_unit:
	case Item_serialize_num:
	{
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionString(propertyValue));
		break;
	}
	case Item_Percent:
	{
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionPercent(propertyValue));
		break;
	}

	case Item_Multi_Strings:
	{
		std::vector<std::string> splitStr = Slic3r::split_string_by_carriage_return(propertyValue);
		printConfig->set_key_value(prusaProperty, new Slic3r::ConfigOptionStrings(splitStr));
		break;
	}
	default:
	{
		break;
	}
	}
}

wxString AnkerPrintParaItem::GetIOptionLabel(wxString optionKey)
{
	wxString str = "";
	if (Slic3r::GUI::wxGetApp().preset_bundle) {
		auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
		Slic3r::t_config_option_key key = optionKey.ToStdString();

		bool hasKey = printConfig.has(key);
		if (!hasKey) {
			ANKER_LOG_ERROR << "printConfig have no this option key:" << key << ", please check s_Preset_print_options";
			return "";
		}

		auto def = printConfig.def();
		if (def) {
			Slic3r::t_optiondef_map options = def->options;
			if (options.find(key) != options.end()){
				str = _L(options[key].label);
			}
		}		
	}
	return str;
}

wxString AnkerPrintParaItem::GetOptionTip(wxString optionKey)
{
	wxString str = "";
	if (Slic3r::GUI::wxGetApp().preset_bundle) {
		auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
		Slic3r::t_config_option_key key = optionKey.ToStdString();
		auto def = printConfig.def();
		if (def) {
			Slic3r::t_optiondef_map options = def->options;
			if (options.find(key) != options.end()) {
				str = _L(Slic3r::GUI::from_u8(options[key].tooltip));
			}
		}
	}
	return str;
}

wxString AnkerPrintParaItem::GetOptionSideText(wxString optionKey)
{
	wxString str = "";
	if (Slic3r::GUI::wxGetApp().preset_bundle) {
		auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
		Slic3r::t_config_option_key key = optionKey.ToStdString();
		auto def = printConfig.def();
		if (def) {
			Slic3r::t_optiondef_map options = def->options;
			if (options.find(key) != options.end()) {
				str = options[key].sidetext;
			}
		}
	}
	return str;
}

bool AnkerPrintParaItem::GetOptionMaxMinDefVal(wxString optionKey, float& min, float& max)
{
	if (Slic3r::GUI::wxGetApp().preset_bundle) {
		auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
		Slic3r::t_config_option_key key = optionKey.ToStdString();
		auto def = printConfig.def();
		if (def) {
			Slic3r::t_optiondef_map options = def->options;
			if (options.find(key) != options.end()) {
				min = options[key].min;
				max = options[key].max;
				return true;
			}
		}
	}
	return false;
}

int AnkerPrintParaItem::checkEditBoxInput(wxString strInput,wxString strParamKey,ItemDataType dataType,bool bHavePercent)
{
	if (bHavePercent && dataType != Item_Percent && dataType != Item_floatOrPercent)
	{
		return 1;
	}

	std::vector<wxString> exceptionVec;
	switch (dataType)
	{
	case Item_int:
	{
		if (strParamKey == "support_material_bottom_interface_layers")
		{
			exceptionVec = { _L("Same as top"),_L("0 (off)"),_L("1 (light)"),_L("2 (default)"),_L("3 (heavy)") };
		}
		else if (strParamKey == "support_material_interface_layers")
		{
			exceptionVec = { _L("0 (off)"),_L("1 (light)"),_L("2 (default)"),_L("3 (heavy)") };
		}
		break;
	}
	case Item_float:
	{
		if (strParamKey == "support_material_bottom_contact_distance")
		{
			exceptionVec = { _L("Same as top"),_L("0.1"),_L("0.2") };
		}
		if (strParamKey == "support_material_contact_distance")
		{
			exceptionVec = { _L("0 (soluble)"),_L("0.1 (detachable)"),_L("0.2 (detachable)") };
		}
		break;
	}
	case Item_floatOrPercent:
	{
		if (strParamKey == "infill_anchor_max")
		{			
			exceptionVec = { _L("0 (not anchored)"),_L("1 mm"),_L("2 mm"),_L("5 mm"),_L("10 mm"),_L("1000 (unlimited)") };
		}
		if (strParamKey == "infill_anchor")
		{
			exceptionVec = { _L("0 (no open anchors)"),_L("1 mm"),_L("2 mm"),_L("5 mm"),_L("10 mm"),_L("1000 (unlimited)") };
		}
		break;
	}
	default:
		break;
	}

	for (wxString& strTmp : exceptionVec)
	{
		strTmp.Replace(" ", "");
	}
	if (std::find(exceptionVec.begin(), exceptionVec.end(), strInput) != exceptionVec.end())
	{
		return 0;
	}

	if (strParamKey == "infill_anchor" || strParamKey == "infill_anchor_max") // value not in exceptionVec list
		strInput.Replace("mm", "");

	if (!isNumber(strInput.ToStdString()))
	{
		return 1;
	}

	return 2;
}

bool AnkerPrintParaItem::CheckExceptionParamValue(wxString strEditValue, wxString paramKey,wxString strUnit)
{
	wxString strParamValue,strEditValueBack;
	strEditValueBack = strEditValue;

	strEditValueBack.Replace(wxT(" "), wxT(""));
	strEditValueBack.Replace(strUnit, wxT(""));
	strEditValueBack.Replace(" ", "");

	auto printConfig = Slic3r::GUI::wxGetApp().plater()->get_global_config();
	strParamValue = printConfig.opt_serialize(paramKey.ToStdString());
	std::map<wxString, wxString> comValueVec = GetExceptionParamMap(paramKey);
	wxString strParamMappedValue;
	if (comValueVec.find(strParamValue) != comValueVec.end())
	{
		strParamMappedValue = comValueVec[strParamValue];
		return (strEditValue == strParamMappedValue || strEditValueBack == strParamMappedValue) ? true : false;
	}
	return (strEditValue == strParamValue || strEditValueBack == strParamValue) ? true : false;
}

ItemDataType AnkerPrintParaItem::GetItemDataType(wxString optionKey)
{
	ItemDataType type = Item_UNKONWN;
	if (m_optionParameterMap.find(optionKey) != m_optionParameterMap.end())
		type = m_optionParameterMap[optionKey].m_dataType;
	return type;
}

wxString AnkerPrintParaItem::GetItemUnit(wxString optionKey)
{
	wxString unit;
	if (m_optionParameterMap.find(optionKey) != m_optionParameterMap.end())
		unit = m_optionParameterMap[optionKey].m_unit;
	return unit;
}

void AnkerPrintParaItem::hideAllResetBtn()
{
	auto iter = m_optionParameterMap.begin();
	while (iter != m_optionParameterMap.end())
	{
		auto pResetBtn = iter->second.m_pBtn;
		if (pResetBtn)
			pResetBtn->Hide();
		++iter;
	}
}

void AnkerPrintParaItem::clearDirtyData()
{
	m_dirtyMap.clear();
}

void AnkerPrintParaItem::showResetBtn(const wxString& optionKey, bool show)
{
	auto iter = m_optionParameterMap.find(optionKey);
	if (iter != m_optionParameterMap.end())
	{
		auto group = iter->second;
		group.m_pBtn->Show(show);

		wxString strLabel = group.m_pLabel->GetLabelText();
		if (strLabel.Contains('\n'))
			strLabel.Replace('\n', "");

		std::string prusaKeyString = optionKey.ToStdString();

		if (show)
		{
			ItemDirtyData dirtyData;
			dirtyData.tabName = m_tabTitle;
			dirtyData.titleName = m_title;
			dirtyData.prusaKey = prusaKeyString;
			dirtyData.ankerKey = strLabel;
			dirtyData.dataType = group.m_dataType;
			m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
		}
		else
		{
			auto iter = m_dirtyMap.find(prusaKeyString);
			if (iter != m_dirtyMap.end())
				m_dirtyMap.erase(iter);
		}
	}
	Refresh();
	Layout();
}

bool AnkerPrintParaItem::isExceptionHadleEditParam(wxString strParamKey)
{
	std::vector<wxString> exceptionHnadleVec = {
		"support_material_bottom_contact_distance",
		"support_material_interface_layers",
		"support_material_contact_distance",
		"support_material_bottom_interface_layers",
		"infill_anchor",
		"infill_anchor_max"};
	if (std::find(exceptionHnadleVec.begin(), exceptionHnadleVec.end(), strParamKey) != exceptionHnadleVec.end())
	{
		return true;
	}

	return false;
}

std::map<wxString, wxString> AnkerPrintParaItem::GetExceptionParamMap(wxString strParamKey)
{
	std::map<wxString, wxString> comValueVec;
	do
	{
		if (strParamKey == "support_material_bottom_contact_distance")
		{
			comValueVec =
			{
				{ "0", _L("Same as top") },
				{ "0.1",_L("0.1") },
				{ "0.2",_L("0.2") }
			};
			break;
		}

		if (strParamKey == "support_material_interface_layers")
		{
			comValueVec =
			{
			{ "0", _L("0 (off)") },
			{ "1", _L("1 (light)") },
			{ "2", _L("2 (default)") },
			{ "3", _L("3 (heavy)")}
			};
			break;
		}

		if (strParamKey == "support_material_contact_distance")
		{
			comValueVec =
			{
			{ "0", _L("0 (soluble)") },
			{ "0.1",_L("0.1 (detachable)") },
			{ "0.2",_L("0.2 (detachable)") }
			};
			break;
		}

		if (strParamKey == "support_material_bottom_interface_layers")
		{
			comValueVec =
			{
				//TRN Print Settings: "Bottom interface layers". Have to be as short as possible
				{ "-1", _L("Same as top") },
				{ "0", _L("0 (off)") },
				{ "1", _L("1 (light)") },
				{ "2", _L("2 (default)") },
				{ "3", _L("3 (heavy)") }
			};
			break;
		}
		if (strParamKey == "infill_anchor")
		{
			comValueVec =
			{
				{ "0",      _L("0 (no open anchors)") },
				{ "1",      _L("1 mm") },
				{ "2",      _L("2 mm") },
				{ "5",      _L("5 mm") },
				{ "10",     _L("10 mm") },
				{ "1000",   _L("1000 (unlimited)") }
			};
			break;
		}

		if (strParamKey == "infill_anchor_max")
		{
			comValueVec =
			{
				{ "0",      _L("0 (not anchored)") },
				{ "1",      _L("1 mm") },
				{ "2",      _L("2 mm") },
				{ "5",      _L("5 mm") },
				{ "10",     _L("10 mm") },
				{ "1000",   _L("1000 (unlimited)") }
			};
			break;
		}
	} while (0);

	return comValueVec;
}

void AnkerPrintParaItem::HandleExceptionPram(wxString strParamKey, wxOwnerDrawnComboBox* pEditBox, wxString strValue, std::map<wxString, PARAMETER_GROUP>::iterator paramInfoIter)
{
	std::map<wxString, wxString> comValueVec = GetExceptionParamMap(strParamKey);
	if (comValueVec.find(strValue) != comValueVec.end())
	{
		pEditBox->SetValue(comValueVec[strValue]);
		paramInfoIter->second.m_UIvalue = comValueVec[strValue];
	}
	else
	{
		pEditBox->SetValue(strValue);
		paramInfoIter->second.m_UIvalue = strValue;
	}
}

enum CHECK_TYPE
{
	type_show_hide,
	type_enable_disable,
};

void AnkerPrintParaItem::SetParamDepedency(PARAMETER_GROUP& paramInfo, int chengeType/*= 0*/, int iCheckDependStandard /*= 0*/, int iCheckValue /*= 0*/)
{
	if (parintParamMap.find(paramInfo.m_optionKey) == parintParamMap.end())
	{
		return;
	}
	std::vector<Dependency_Type> showHidePairVec		= { Type_Shown , Type_Hide };
	std::vector<Dependency_Type> EnabelDisablePairVec	= { Type_Enable , Type_Disable };
	std::vector<Dependency_Type> checkStandardVec;
	CHECK_TYPE checkType;
	DEPENDENCY_INFO info = parintParamMap[paramInfo.m_optionKey];
	paramInfo.dependencyInfo = info;
	if (std::find(showHidePairVec.begin(), showHidePairVec.end(), info.depencyType) != showHidePairVec.end())
	{
		checkStandardVec = showHidePairVec;
		checkType = type_show_hide;
	}
	else if (std::find(EnabelDisablePairVec.begin(), EnabelDisablePairVec.end(), info.depencyType) != EnabelDisablePairVec.end())
	{
		checkStandardVec = EnabelDisablePairVec;
		checkType = type_enable_disable;
	}

	bool chekcSate = true;
	if (iCheckDependStandard == 0)
	{
	chekcSate = std::all_of(info.dependedParamMap.begin(), info.dependedParamMap.end(),
			[this, info, checkStandardVec, iCheckDependStandard, iCheckValue, chengeType](const std::pair<wxString, wxString>& element) {
				wxString strKey = element.first;
				// get the config value
				auto printConfig  = (chengeType == 0) ? Slic3r::GUI::wxGetApp().plater()->get_global_config(): Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
				wxString strParamValue = printConfig.opt_serialize(strKey.ToStdString());
				if ((strParamValue == element.second && info.depencyType == checkStandardVec[0]) ||
					(strParamValue != element.second && info.depencyType == checkStandardVec[1]))
				{
					return true;
				}
				else
				{
					return false;
				}
			});
	}
	else if (iCheckDependStandard == 1)
	{
		chekcSate = std::all_of(info.dependedParamNewFormsMap.begin(), info.dependedParamNewFormsMap.end(),
			[this, info, checkStandardVec, iCheckDependStandard, iCheckValue](const std::pair<wxString, int>& element) {
				if ((iCheckValue == element.second && info.depencyType == checkStandardVec[0]) ||
					(iCheckValue != element.second && info.depencyType == checkStandardVec[1]))
				{
					return true;
				}
				else
				{
					return false;
				}
			});
	}
	
	switch (checkType)
	{
	case type_show_hide:
	{
		paramInfo.bEnable = true;
		paramInfo.bShown = chekcSate;
		break;
	}
	
	case type_enable_disable:
	{
		paramInfo.bEnable = chekcSate;
		paramInfo.bShown = true;
		break;
	}

	default:
		break;
	}
}

PARAMETER_GROUP& AnkerPrintParaItem::GetParamDepenencyRef(wxString strParamKey)
{
	if (m_optionParameterMap.find(strParamKey) != m_optionParameterMap.end())
	{
		return m_optionParameterMap[strParamKey];
	}
	else
	{
		throw std::runtime_error("Key not found");
	}
}

// remove float numstr's tailing '0' , eg : "12.30" -> "12.3"
wxString AnkerPrintParaItem::RemoveTrailingZeros(const wxString& numStr) {
	wxString result = numStr;
	while (!result.empty() && result.Contains('.') && (result.Last() == '0' || result.Last() == '.')) {
		result.RemoveLast();
	}
	return result;
}

bool AnkerPrintParaItem::isNumber(const std::string& str) {
	std::regex numberRegex("^-?\\d+(\\.\\d+)?$");
	return std::regex_match(str, numberRegex);
}

// eg: "29999.9999999" -> 29999
wxString AnkerPrintParaItem::RemoveDecimal(const wxString& numStr)
{
	double floatValue;
	numStr.ToDouble(&floatValue);
	long long intValue = static_cast<long long>(floatValue);
	return std::to_string(intValue);
}

// remove leading and trailing zeros
// modify this function should run TestRemoveLeadingAndTrailingZeros
wxString AnkerPrintParaItem::RemoveLeadingAndTrailingZeros(const wxString& strnum)
{
	if (strnum.empty()) {
		return strnum;
	}

	wxString result = strnum;
	bool isNegative = false;
	if (strnum[0] == '-') {
		isNegative = true;
		result = result.substr(1, std::string::npos);
	}

	// remove leading zeros    
	size_t pos = result.find_first_not_of('0');
	if (pos != std::string::npos) {
		result = result.substr(pos);
	}
	else {
		return "0";
	}

	// remove trailing zeros
	if (result.Find('.') != wxNOT_FOUND) {
		// 12300 should not remove trailing zeros
		pos = result.find_last_not_of('0');
		if (pos != std::string::npos) {
			result = result.substr(0, pos + 1);
		}
	}

	if (result.empty() ||
		(result.size() == 1 && result[0] == '.')) {
		return "0";
	}
	do {
		// .123 => 0.123
		if (result.size() > 1 && result[0] == '.') {
			result = "0" + result;
			break;
		}
		// 123. => 123
		if (result.size() > 1 && result[result.size() - 1] == '.') {
			result = result.substr(0, result.size() - 1);
			break;
		}
	} while (false);

	if (result != "0" && isNegative) {
		result = "-" + result;
	}

	return result;

	//std::regex re("^0+(\\d+\\.?\\d*?[1-9])0*$");
	//std::regex reBack("(\\.?0+)$");
	//std::string resulta = std::regex_replace(strnum.ToStdString(), re, "$1");
	//std::string result = std::regex_replace(resulta, reBack, "");
}

bool AnkerPrintParaItem::ValueCheck(const PARAMETER_GROUP& param, const wxString& value, wxString& newValue)
{
	// ex: a12bc3.45, 123..45 return false
	auto isNumeric = [](const std::string& str) {
		bool hasDecimalPoint = false;
		bool hasNegativeSymbol = false;

		for (char ch : str) {
			if (!std::isdigit(ch)) {
				if (ch == '-' && !hasNegativeSymbol) {
					hasNegativeSymbol = true;
					continue;
				}
				if (ch == '.' && !hasDecimalPoint) {
					hasDecimalPoint = true;
				}
				else {
					return false;
				}
			}
		}
		return true;
	};

	auto PartLength = [](const std::string& str, int& intLen, int& decLen) {
		size_t decimalPointPos = str.find('.');

		if (decimalPointPos != std::string::npos) {
			intLen = decimalPointPos;
			decLen = str.length() - decimalPointPos - 1;
			if (decLen < 0) {
				return false;
			}
			return true;
		}
		intLen = str.length();
		decLen = 0;
		return true;
	};

	auto Str2Num = [PartLength](const std::string& str, float& num) {
		try {
			num = std::stof(str);
			return true;
		}
		catch (...) {
			return false;
		}
		return false;
	};	

	auto IsAlmostEqual = [](float a, float b, float tolerance = 0.01) {
		return std::abs(a - b) < tolerance;
	};

	if (param.m_dataType != Item_int &&
		param.m_dataType != Item_bool &&
		param.m_dataType != Item_float &&
		param.m_dataType != Item_floatOrPercent &&
		param.m_dataType != Item_Percent) {
		newValue = value;
		return true;
	}

	bool isFloatValue = false;
	if (param.m_dataType == Item_float ||
		param.m_dataType == Item_floatOrPercent ||
		param.m_dataType == Item_Percent) {
		isFloatValue = true;
	}

	bool valOutOfRange = false;
	newValue = "";
	std::string prusaKeyString = param.m_optionKey.ToStdString();
	do {
		// 1. check illegal
		if (!isNumeric(value.ToStdString(wxConvUTF8))) {
			break;
		}

		// 2. remove zeros
		newValue = RemoveLeadingAndTrailingZeros(value);
		
		// 3. check length
		// check integer part len and decimal part len
		// integer len: 8, float len: 8+1+6 = 15
		const int intMaxLen = 8;
		const int decMaxLen = 6;
		int numLen = intMaxLen;
		if (isFloatValue) {
			numLen = 15;
		}
		if (newValue.length() > numLen) {
			break;
		}
		// check float/int len
		int intLen = 0;
		int decLen = 0;
		if (!PartLength(newValue.ToStdString(wxConvUTF8), intLen, decLen)) {
			break;
		}
		if (intLen > intMaxLen || decLen > decMaxLen) {
			break;
		}

		// 4. integer remove decimal num
		if (!isFloatValue) {
			decLen = 0;
			newValue = RemoveDecimal(newValue);
		}

		// 5. str to float value
		float newFvalue = 0.0;
		if (!Str2Num(newValue.ToStdString(wxConvUTF8), newFvalue)) {
			break;
		}

		// 6. check min/max
		float min = 0.0f, max = 0.0f;
		GetOptionMaxMinDefVal(prusaKeyString, min, max);
		ChangeMaxMinValueForPrinter(prusaKeyString, max, min);

		// if min not equal to 0, is a legal value
		//bool minNon0 = true;
		//if (IsAlmostEqual(min, 0.0)) {
		//	// min equal to 0.0
		//	minNon0 = false;
		//}

		if (newFvalue < min /*|| (!minNon0 && IsAlmostEqual(newFvalue, min))*/) {
			// newFvalue < min
			valOutOfRange = true;
			break;
		}
		if (param.m_dataType != Item_floatOrPercent) {
			// Item_floatOrPercent no need check max value here!!! (todo: bad code)
			if (newFvalue > max /*|| IsAlmostEqual(newFvalue, max)*/) {
				// newFvalue > max
				valOutOfRange = true;
				break;
			}
		}		

		// 7. set to string
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(decLen) << newFvalue;
		newValue = oss.str();
		return true;
	} while (false);

	bool showDialog = true;
	wxString errMsg = _L("Invalid numeric input");
	if (valOutOfRange) {
		errMsg = _L("Value is out of range.");
		if (param.m_type == ItemSpinBox) {
			showDialog = false;
		}
	}

	if (showDialog) {
		wxString errMsg = valOutOfRange ? _L("Value is out of range.") : _L("Invalid numeric input");
		Slic3r::GUI::MessageDialog dialog(m_parent, errMsg, _L("Parameter validation") + ": " + prusaKeyString, wxOK);
		dialog.SetSize(wxSize(350, 250));
		dialog.ShowModal();
	}

	return false;
}

void AnkerPrintParaItem::TestRemoveLeadingAndTrailingZeros()
{
	auto test = [this](const wxString& src, const wxString& dest) {
		auto ret = RemoveLeadingAndTrailingZeros(src);
		if (ret != dest) {
			assert(false);
		}
	};
	{
		test("123", "123");
		test("0.123", "0.123");
		test("0000.12300", "0.123");
		test("123.123", "123.123");
		test("000123", "123");
		test("123000", "123000");
		test("123000.000", "123000");
		test("0001203000.000", "1203000");
		test("00012300.00123", "12300.00123");
		test("00012300.00123000", "12300.00123");
		test(".000123000", "0.000123");
	}
	{
		test("-123", "-123");
		test("-0.123", "-0.123");
		test("-0000.12300", "-0.123");
		test("-123.123", "-123.123");
		test("-000123", "-123");
		test("-123000", "-123000");
		test("-123000.000", "-123000");
		test("-0001203000.000", "-1203000");
		test("-00012300.00123", "-12300.00123");
		test("-00012300.00123000", "-12300.00123");
		test("-.000123000", "-0.000123");		
	}
}

void AnkerPrintParaItem::ChangeMaxMinValueForPrinter(const wxString& optionKey, float& max, float& min)
{
	if (optionKey == "first_layer_height")
	{
		const Slic3r::Preset& printer_preset = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset();
		std::string printModel = printer_preset.config.opt_string("printer_model");
		std::vector<double> nozzle_diameters = static_cast<const Slic3r::ConfigOptionFloats*>(printer_preset.config.option("nozzle_diameter"))->values;
		if (nozzle_diameters.size() == 1) {
			if ((printModel == "M5C" || printModel == "M5") && (nozzle_diameters[0] == 0.4 || nozzle_diameters[0] == 0.2)) {
				min = 0.1f;
			}
		}
	}
}

void AnkerPrintParaItem::RefreashDependParamState(wxString strChangedOptionkey,int iSetValue)
{
	if (parintParamMap.find(strChangedOptionkey) != parintParamMap.end() &&
		parintParamMap[strChangedOptionkey].beDependedParamVec.size() > 0)
	{
		std::map<wxString, PARAMETER_GROUP> allParamInfos;
		if (m_PrintMode == mode_global)
		{
			// todo: need to get the refrence ti refresh the total param info in param panel
			allParamInfos = Slic3r::GUI::wxGetApp().sidebarnew().getGlobalParamInfo();
		}
		else if (m_PrintMode == mode_model)
		{
			allParamInfos = Slic3r::GUI::wxGetApp().sidebarnew().getModelParamInfo();
		}
		else
		{
			ANKER_LOG_ERROR << "m_PrintMode in AnkerPrintParaItem type error, please check the code logic";
			return;
		}
		
		bool bNeedRefreash = false;
		for (int i = 0; i< parintParamMap[strChangedOptionkey].beDependedParamVec.size();i++)
		{
			wxString strDependKey = parintParamMap[strChangedOptionkey].beDependedParamVec[i];
			if (allParamInfos.find(strDependKey) != allParamInfos.end())
			{
				bNeedRefreash = true;
				PARAMETER_GROUP info = allParamInfos[strDependKey];
				SetParamDepedency(info, 0, 1, iSetValue);
				info.m_pWindow->Enable(info.bEnable);
				info.pLineSizer->Show(info.bShown);
				//refresh revert icon state
				if (m_dirtyMap.find(strDependKey) == m_dirtyMap.end())
				{
					info.m_pBtn->Hide();
				}
			}
			else
			{
				//ANKER_LOG_ERROR << "depended param key not found, please check the logic";
				continue;
			}
		}

		// only UI would change will call layout 
		if (bNeedRefreash)
		{
			wxWindowUpdateLocker updateLocker(this->GetParent());
			this->GetParent()->Layout();
			//Refresh();
		}
	}
}

void AnkerPrintParaItem::SetItemVisible(Slic3r::GUI::ItemType type, Slic3r::ModelVolumeType volume_type)
{
	if (!m_group_property)  return;

	auto group_visible = [&](Slic3r::GUI::ItemType type) {
		switch (type)
		{
		case Slic3r::GUI::ItemType::itObject:
		case Slic3r::GUI::ItemType::itInstance:
			return m_group_property->local_show;
			break;
		case Slic3r::GUI::itVolume:
			if (volume_type == Slic3r::ModelVolumeType::MODEL_PART) {
				return m_group_property->part_show;
			}
			else if (volume_type == Slic3r::ModelVolumeType::PARAMETER_MODIFIER) {
				return m_group_property->modifer_show;
			}
			break;
		case Slic3r::GUI::itLayer:
			return m_group_property->layer_height_show;
			break;
		default:
			break;
		}
        return false;
	};

	auto item_visible = [&](Slic3r::GUI::ItemType type, std::shared_ptr<GroupItemProperty>& property) {
		switch (type)
		{
		case Slic3r::GUI::itObject:
		case Slic3r::GUI::ItemType::itInstance:
			return property->local_show;
			break;
		case Slic3r::GUI::itVolume:
			if (volume_type == Slic3r::ModelVolumeType::MODEL_PART) {
				return property->part_show;
			}
			else if (volume_type == Slic3r::ModelVolumeType::PARAMETER_MODIFIER) {
				return property->modifer_show;
			}
			break;
		case Slic3r::GUI::itLayer:
			return property->layer_height_show;
			break;
		default:
			break;
		}
        
        return false;
	};

	auto refreshDepence = [&](const wxString &key) {
		auto info = getWidgetValue(key);
		int value = info.paramDataValue.GetInteger();
		RefreashDependParamState(key, value);
	};

	auto is_dirty = [&](const wxString& key) {
		return m_dirtyMap.find(key) != m_dirtyMap.end();
	};

	if (!group_visible(type)) {
		this->Show(false);
	}
	else {
		m_group_property->title_Sizer->Show(true);
		for (auto& item : m_group_property->propertyItems) {
			item->_boxSizer->Show(item_visible(type, item));
			item->reset_btn->Show(is_dirty(item->_key));
			if (m_title == _L("Ironing")) {
				refreshDepence(_L("ironing_type"));
			}
		}
	}
}

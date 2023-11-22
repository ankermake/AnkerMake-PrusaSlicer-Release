#include "AnkerParameterPanel.hpp"
#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "../GUI_App.hpp"
#include "wx/app.h"
#include "../I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "../AnkerBtn.hpp"
#include "../AnkerCheckBox.hpp"
#include "../wxExtensions.hpp"
#include "../Plater.hpp"
#include "libslic3r/print.hpp"
#include "../SavePresetDialog.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "../common/AnkerGUIConfig.hpp"


#ifndef __APPLE__
#define ItemSize wxSize(35 * Slic3r::GUI::wxGetApp().em_unit(), -1)
#define ItemSizeEX AnkerSize(-1,-1)
#define PANEL_WITH 360
#else
#define ItemSize wxSize(40 * Slic3r::GUI::wxGetApp().em_unit(), -1)
#define ItemSizeEX AnkerSize(-1,-1)
#define PANEL_WITH 400
#endif // !__APPLE__


#define BgColor wxColor("#292A2D")


wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SAVEALL_BTN_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_UPDATE_CURRENT_PRESET, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, wxCommandEvent);

AnkerParameterPanel::AnkerParameterPanel(wxWindow* parent, 
										wxWindowID winid /*= wxID_ANY*/, 
										const wxPoint& pos /*= wxDefaultPosition*/, 
										const wxSize& size /*= wxDefaultSize*/)
										: wxControl(parent, wxID_ANY, wxDefaultPosition, ItemSizeEX, wxBORDER_NONE)
{
	initDefaultData();
	initUi();	
	showCurrentWidget(_L("Layers and perimeters"));
	showEasyModel();	
	initUiData();
}



void AnkerParameterPanel::enableSliceBtn(bool isSaveBtn, bool isEnable)
{
	ANKER_LOG_INFO << "slice btn status changed";
	if (!m_pSliceBtn || !m_pSaveProjectBtn)
		return;
	if (isEnable)
	{	
		if (isSaveBtn)
		{
			m_pSaveProjectBtn->SetBackgroundColour(wxColor("#3A3B3F"));
			m_pSaveProjectBtn->SetTextColor(wxColor("#FFFFFF"));
			m_pSaveProjectBtn->Enable(isEnable);
		}
		else
		{
			m_pSliceBtn->SetBackgroundColour(wxColor("#62D361"));
			m_pSliceBtn->SetTextColor(wxColor("#FFFFFF"));
			m_pSliceBtn->Enable(isEnable);		
		}
	}
	else
	{

		if (isSaveBtn)
		{
			m_pSaveProjectBtn->SetBackgroundColour(wxColor("#2E2F32"));
			m_pSaveProjectBtn->SetTextColor(wxColor("#69696C"));
			m_pSaveProjectBtn->Enable(isEnable);
		}
		else
		{
			m_pSliceBtn->SetBackgroundColour(wxColor("#2E2F32"));
			m_pSliceBtn->SetTextColor(wxColor("#69696C"));
			m_pSliceBtn->Enable(isEnable);
		}
	}

}

void AnkerParameterPanel::resetAllUi()
{
	ANKER_LOG_INFO << "real reset all parameter data";
	initUiData();	
}


bool AnkerParameterPanel::saveAllUi()
{	
	return onSave();
}


bool AnkerParameterPanel::onSave()
{
	Slic3r::GUI::AnkerTab* ankerTab = Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT);
	if (!ankerTab)
		return false;
	
	auto& printPrest = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset();

	bool from_template = false;	
	std::string name =  "";
	auto prestType = Slic3r::Preset::TYPE_PRINT;

	//rename dialog
	Slic3r::GUI::AnkerSavePresetDialog dlg(m_parent->GetParent(), { prestType }, false ? _u8L("Detached") : "", from_template);	
	auto res = dlg.ShowModal();

	if (res == wxID_OK)
	{
		name = dlg.get_name();
		if (from_template)
			from_template = dlg.get_template_filament_checkbox();

		saveUiData();

		ankerTab->save_preset(name, true);

		Slic3r::GUI::wxGetApp().preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);

		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_tab_ui();
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->onAnkerPresetsChanged();
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_changed_ui();

		Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_FILAMENT)->update_tab_ui();
		Slic3r::GUI::wxGetApp().mainframe->diff_dialog.update_presets(prestType);

		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets->save_current_preset(name, false);

		std::string preset_name = name;
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->select_preset(Slic3r::Preset::remove_suffix_modified(preset_name));

		m_pPresetParameterComBox->update();
		return true;
	}
	else
		return false;
}


bool AnkerParameterPanel::swtichPresetCheckSave()
{
	Slic3r::GUI::AnkerTab* ankerTab = Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT);	
	if (!ankerTab)
		return false;
	
	std::string name = "";
	auto prestType = Slic3r::Preset::TYPE_PRINT;

	unsigned int selectedIndex = m_pPresetParameterComBox->GetSelection();
	std::string newPresetName = m_pPresetParameterComBox->GetString(selectedIndex).ToUTF8().data();
	auto tmpconfig = ankerTab->m_config;
	updatePresetEx(tmpconfig);

	Slic3r::GUI::UnsavedChangesDialog savedlg(Slic3r::Preset::TYPE_PRINT, ankerTab->m_presets, newPresetName);
	//std::shared_ptr<Slic3r::GUI::UnsavedChangesDialog> pSavedlg(new Slic3r::GUI::UnsavedChangesDialog(Slic3r::Preset::TYPE_PRINT, ankerTab->m_presets, newPresetName));
	
	if((Slic3r::GUI::wxGetApp().app_config->get("default_action_on_select_preset") == "none" && savedlg.ShowModal() == wxID_CANCEL))		
	{
		ankerTab->m_presets->discard_current_changes();
		return false;
	}

	if (savedlg.save_preset())
	{
		name = savedlg.get_preset_name();

		saveUiData();

		ankerTab->save_preset(name, true);

		Slic3r::GUI::wxGetApp().preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);

		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_tab_ui();
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->onAnkerPresetsChanged();
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_changed_ui();

		Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_FILAMENT)->update_tab_ui();
		Slic3r::GUI::wxGetApp().mainframe->diff_dialog.update_presets(prestType);

		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets->save_current_preset(name, false);

		std::string preset_name = name;
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->select_preset(Slic3r::Preset::remove_suffix_modified(preset_name));

		m_pPresetParameterComBox->update();
		onResetBtnStatusChanged(false);
		return true;
		
	}
	else if (savedlg.discard())
	{
		ankerTab->m_presets->discard_current_changes();		
		onResetBtnStatusChanged(false);	
		initUiData();
		hideAllResetBtn();
	}

	return false;
}

void AnkerParameterPanel::hidePoupWidget()
{
	if (m_popupWidget)
		m_popupWidget->Hide();
}


void AnkerParameterPanel::reloadData()
{
	ANKER_LOG_INFO << "reload parameter data";
	this->Freeze();
	m_pPresetParameterComBox->update();	
	updateEasyWidget();
	initUiData();
	this->Thaw();
}


void AnkerParameterPanel::reloadDataEx()
{
	ANKER_LOG_INFO << "reloadEx parameter data";
	this->Freeze();
	m_pPresetParameterComBox->update();	
	updateEasyWidget();
	initUiData();
	this->Thaw();
}

void AnkerParameterPanel::updateEasyWidget()
{
	auto list = getValidParameter();
	auto currentData = getCurrentModule(list);

	if (list.size() > 0)
		m_pEasyWidget->showWidget(list, currentData);
}

void AnkerParameterPanel::updatePreset(Slic3r::DynamicPrintConfig& printConfig)
{		
	ANKER_LOG_INFO << "update preset";
	auto lapIter = m_lapMap.begin();
	while (lapIter != m_lapMap.end())
	{
		std::string AnkerPropertyName = lapIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = lapIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Layers and perimeters"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++lapIter;
	}

	auto infillIter = m_infillMap.begin();
	while (infillIter != m_infillMap.end())
	{
		std::string AnkerPropertyName = infillIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = infillIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Infill"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++infillIter;
	}

	auto sabIter = m_sabMap.begin();
	while (sabIter != m_sabMap.end())
	{
		std::string AnkerPropertyName = sabIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = sabIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Skirt and brim"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++sabIter;
	}


	auto smIter = m_smMap.begin();
	while (smIter != m_smMap.end())
	{
		std::string AnkerPropertyName = smIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = smIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Support material"), AnkerPropertyName);

		if ("support_material_bottom_interface_layers" == prusaProperty)
		{
			wxString tempData = ItemDataInfo.paramDataValue.GetString();
			if (ItemDataInfo.paramDataValue.GetString() == _L("Same as top"))
				ItemDataInfo.paramDataValue = wxVariant("-1");
		}

		//todo: use validator
		if ("raft_first_layer_density" == prusaProperty)
		{
			double value;
			wxString tempData = ItemDataInfo.paramDataValue.GetString();
			tempData.ToDouble(&value);
			//Dividing by 0 will cause crash
			if (value <= 0.0) {
				ItemDataInfo.paramDataValue = wxVariant(10.0);
			}
			if (value > 100.0) {
				ItemDataInfo.paramDataValue = wxVariant(100.0);
			}
		}


		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++smIter;
	}

	auto speedIter = m_speedMap.begin();
	while (speedIter != m_speedMap.end())
	{
		std::string AnkerPropertyName = speedIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = speedIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Speed"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++speedIter;
	}


	auto meIter = m_meMap.begin();
	while (meIter != m_meMap.end())
	{
		std::string AnkerPropertyName = meIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = meIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Multiple Extruders"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++meIter;
	}


	auto advanceIter = m_advancedMap.begin();
	while (advanceIter != m_advancedMap.end())
	{
		std::string AnkerPropertyName = advanceIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = advanceIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Advanced"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++advanceIter;
	}
}


void AnkerParameterPanel::updatePresetEx(Slic3r::DynamicPrintConfig* printConfig)
{
	ANKER_LOG_INFO << "update preset";
	auto lapIter = m_lapMap.begin();
	while (lapIter != m_lapMap.end())
	{
		std::string AnkerPropertyName = lapIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = lapIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Layers and perimeters"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++lapIter;
	}

	auto infillIter = m_infillMap.begin();
	while (infillIter != m_infillMap.end())
	{
		std::string AnkerPropertyName = infillIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = infillIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Infill"), AnkerPropertyName);
		wxString tmpData = ItemDataInfo.paramDataValue.GetString();

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++infillIter;
	}

	auto sabIter = m_sabMap.begin();
	while (sabIter != m_sabMap.end())
	{
		std::string AnkerPropertyName = sabIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = sabIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Skirt and brim"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++sabIter;
	}


	auto smIter = m_smMap.begin();
	while (smIter != m_smMap.end())
	{
		std::string AnkerPropertyName = smIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = smIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Support material"), AnkerPropertyName);
 		if ("support_material_bottom_interface_layers" == prusaProperty)
 		{
 			wxString tempData = ItemDataInfo.paramDataValue.GetString();
 			if (ItemDataInfo.paramDataValue.GetString() == _L("Same as top"))
 				ItemDataInfo.paramDataValue = wxVariant("-1");
 		}
		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++smIter;
	}

	auto speedIter = m_speedMap.begin();
	while (speedIter != m_speedMap.end())
	{
		std::string AnkerPropertyName = speedIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = speedIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Speed"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++speedIter;
	}


	auto meIter = m_meMap.begin();
	while (meIter != m_meMap.end())
	{
		std::string AnkerPropertyName = meIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = meIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Multiple Extruders"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++meIter;
	}


	auto advanceIter = m_advancedMap.begin();
	while (advanceIter != m_advancedMap.end())
	{
		std::string AnkerPropertyName = advanceIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = advanceIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Advanced"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++advanceIter;
	}
}

void AnkerParameterPanel::setObjectName(const wxString& objName, Slic3r::ModelConfig* config)
{
	if (!config)
	{
		m_rightParameterCfg = nullptr;
		m_isRightParameterPanel = false;
		return;
	}
	else
		m_isRightParameterPanel = true;

	ANKER_LOG_INFO << "success set config";
	m_currentObjName->SetLabelText(objName);
	m_rightParameterCfg = config;
	

	initUiData();
	//m_pSearchTextCtrl->SetLabelText("");

	showExpertModel();
	showRightParameterpanel();
}

void AnkerParameterPanel::showRightParameterpanel()
{
	ANKER_LOG_INFO << "show right menue parameter panel";
	m_pTitleLabel->SetLabelText(_L("common_slice_toolpannel_addprintsettings"));
	m_pResetBtn->Hide();
	m_pSaveAllBtn->Hide();
	
	m_pHandleModelComBox->Hide();
	m_pPresetParameterComBox->Hide();
	m_printParameterEditBtn->Hide();

	m_pBtnPanel->Hide();
	//m_pSliceBtn->Hide();
	//m_pSaveProjectBtn->Hide();
	//m_pCfgBtn->Hide();

	m_currentObjName->Show();
	m_deleteBtn->Show();
	m_pExitBtn->Show();

	Update();
	Layout();
}


bool AnkerParameterPanel::getObjConfig(Slic3r::ModelConfig*& config)
{
	if (m_isRightParameterPanel)
	{
		ANKER_LOG_INFO << "valid print cfg";
		config = m_rightParameterCfg;
		return true;
	}

	return false;
}


void AnkerParameterPanel::onComboBoxClick(Slic3r::GUI::AnkerPlaterPresetComboBox* presetComboBox)
{
	if (hasDirtyData())
		swtichPresetCheckSave();
	else
	{
		presetComboBox->Popup();
	}
}


void AnkerParameterPanel::onPresetComboSelChanged(Slic3r::GUI::AnkerPlaterPresetComboBox* presetChoice, const int selection)
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

void AnkerParameterPanel::initUi()
{	
	SetBackgroundColour(wxColour(BgColor));

	wxImage dropBtnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
	dropBtnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);

	wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));
	wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));
	wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));

	wxStringList dataList = {};
	m_pMainVSizer = new wxBoxSizer(wxVERTICAL);
	
	//title
	{
		wxPanel* pTitleWidget = new wxPanel(this);
		pTitleWidget->SetMaxSize(AnkerSize(PANEL_WITH, ANKER_COMBOBOX_HEIGHT));

		pTitleWidget->SetBackgroundColour(BgColor);
		wxBoxSizer* pTitleVSizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
		
		pTitleWidget->SetSizer(pTitleVSizer);

#ifndef __APPLE__
		pTitleVSizer->AddSpacer(10);
#else 
		pTitleVSizer->AddSpacer(5);
#endif
		pTitleVSizer->Add(pTitleHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);


		m_pExitBtn = new ScalableButton(pTitleWidget, wxID_ANY, "return", "");
		//m_pExitBtn->SetToolTip(_L("Click to exit edit"));
		m_pExitBtn->SetMaxSize(AnkerSize(25, 32));
		m_pExitBtn->SetSize(AnkerSize(25, 32));
		m_pExitBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			wxCommandEvent deleteEvt = wxCommandEvent(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL);
			deleteEvt.SetEventObject(this);
			ProcessEvent(deleteEvt);

			m_currentObjName->SetLabelText("");			
			m_isRightParameterPanel = false;
			});
		m_pExitBtn->Hide();

		m_pTitleLabel = new wxStaticText(pTitleWidget, wxID_ANY, _L("common_slicepannel_printsetting_title"));
		m_pTitleLabel->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pTitleLabel->SetForegroundColour(wxColour("#FFFFFF"));
		m_pTitleLabel->SetMinSize(wxSize(100, 30));
		m_pTitleLabel->SetSize(wxSize(100, 30));
		
		m_deleteBtn = new ScalableButton(pTitleWidget, wxID_ANY, "delete_cfg", "");
		//m_deleteBtn->SetToolTip(_L("Click to close edit"));
		m_deleteBtn->SetMaxSize(AnkerSize(25, 32));
		m_deleteBtn->SetSize(AnkerSize(25, 32));
		m_deleteBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) {
			wxCommandEvent deleteEvt = wxCommandEvent(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT);
			deleteEvt.SetEventObject(this);
			ProcessEvent(deleteEvt);

			m_currentObjName->SetLabelText("");			
			m_isRightParameterPanel = false;
		});
		m_deleteBtn->Hide();

		m_printParameterEditBtn = new ScalableButton(pTitleWidget, wxID_ANY, "cog", "");
		m_printParameterEditBtn->SetToolTip(_L("common_slicepannel_hover_presetsetting"));
		m_printParameterEditBtn->SetMaxSize(AnkerSize(25,32));
		m_printParameterEditBtn->SetSize(AnkerSize(25,32));
		m_printParameterEditBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent) {
#if SHOW_OLD_SETTING_DIALOG
			Slic3r::GUI::Tab* tab = Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT);
			if (!tab)
				return;

			if (int page_id = Slic3r::GUI::wxGetApp().tab_panel()->FindPage(tab); page_id != wxNOT_FOUND)
			{
				Slic3r::GUI::wxGetApp().tab_panel()->SetSelection(page_id);
				Slic3r::GUI::wxGetApp().mainframe->select_tab();
			}
#endif

			Slic3r::GUI::AnkerTab* ankerTab = Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT);
			if (!ankerTab)
				return;

            if (int page_id = Slic3r::GUI::wxGetApp().ankerTabPanel()->FindPage(ankerTab); page_id != wxNOT_FOUND)
            {
                Slic3r::GUI::wxGetApp().ankerTabPanel()->SetSelection(page_id);
				Slic3r::GUI::wxGetApp().mainframe->selectAnkerTab(ankerTab);
				Slic3r::GUI::wxGetApp().mainframe->showAnkerCfgDlg();
            }
			});

		wxArrayString itemList;
		itemList.Add(_L("common_slicepannel_style12_easy"));
		itemList.Add(_L("common_slicepannel_style11_expert"));

		wxCursor handCursor(wxCURSOR_HAND);		
		
		m_pHandleModelComBox = new Slic3r::GUI::AnkerPlaterPresetComboBox(this, Slic3r::Preset::TYPE_PRINT);
		m_pHandleModelComBox->Create(pTitleWidget,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			AnkerSize(90, ANKER_COMBOBOX_HEIGHT),
			itemList,
			wxNO_BORDER | wxCB_READONLY);

		m_pHandleModelComBox->set_button_clicked_function([this]() {
			ANKER_LOG_INFO << "preset combox of easycombox clicked";
			m_pHandleModelComBox->SetFocus();
			onComboBoxClick(m_pHandleModelComBox);
			
			});
		m_pHandleModelComBox->set_selection_changed_function([this](int selection) {
			ANKER_LOG_INFO << "preset combox of easycombox clicked";
			onPresetComboSelChanged(m_pHandleModelComBox, selection);
			});

		m_pHandleModelComBox->SetCursor(handCursor);
		m_pHandleModelComBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
		m_pHandleModelComBox->SetFont(ANKER_FONT_NO_1);
		m_pHandleModelComBox->SetForegroundColour(wxColour("#FFFFFF"));
		m_pHandleModelComBox->SetBackgroundColour(BgColor);
		m_pHandleModelComBox->SetBackgroundColour(wxColour("#434447"));
		m_pHandleModelComBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
		m_pHandleModelComBox->SetMinSize(AnkerSize(90, ANKER_COMBOBOX_HEIGHT));
		m_pHandleModelComBox->SetMaxSize(AnkerSize(90, ANKER_COMBOBOX_HEIGHT));
		m_pHandleModelComBox->SetSize(AnkerSize(90, ANKER_COMBOBOX_HEIGHT));
		m_pHandleModelComBox->SetSelection(0);
		//m_pHandleModelComBox->Enable(true);

		m_pHandleModelComBox->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& event) {

			wxOwnerDrawnComboBox* pComboboxCtrl = dynamic_cast<wxOwnerDrawnComboBox*>(event.GetEventObject());
			int index = 0;
			wxString text = wxString();
			if (pComboboxCtrl)
			{
				index = pComboboxCtrl->GetSelection();
				text = pComboboxCtrl->GetString(index);
			}

			if (text == _L("common_slicepannel_style12_easy"))
			{
				checkDirtyData();
			}

			if (text == _L("common_slicepannel_style12_easy"))
				showEasyModel();
			else
				showExpertModel();
			});


#ifdef __APPLE__
		pTitleHSizer->AddSpacer(15);
		pTitleHSizer->Add(m_pExitBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL| wxTOP, 12);
		pTitleHSizer->AddStretchSpacer(1);
		pTitleHSizer->Add(m_pHandleModelComBox, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddSpacer(12);
		pTitleHSizer->Add(m_printParameterEditBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_deleteBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddSpacer(2);
#else
		pTitleHSizer->AddSpacer(12);
		pTitleHSizer->Add(m_pExitBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddStretchSpacer(1);
		pTitleHSizer->Add(m_pHandleModelComBox, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddSpacer(16);
		pTitleHSizer->Add(m_printParameterEditBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_deleteBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddSpacer(6);
#endif

		//printer settings preset position
		wxPanel* pDividingLine = new wxPanel(this, wxID_ANY);
		pDividingLine->SetBackgroundColour(wxColour("#38393C"));		
		pDividingLine->SetMinSize(wxSize(335, 1));
		
		m_pMainVSizer->Add(pTitleWidget,  wxEXPAND, wxALL|wxEXPAND, 0);
		m_pMainVSizer->AddSpacer(8);
		m_pMainVSizer->Add(pDividingLine, 0, wxEXPAND, 0);
		//m_pMainVSizer->AddSpacer(10);
	}

	{
		wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("preset_check.png")), wxBITMAP_TYPE_PNG);
		checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap checkScaledBitmap(checkImage);

		
		wxImage fastModelLogo = wxImage(wxString::FromUTF8(Slic3r::var("fast.png")), wxBITMAP_TYPE_PNG);
		wxImage nornmalModelLogo = wxImage(wxString::FromUTF8(Slic3r::var("normal.png")), wxBITMAP_TYPE_PNG);
		wxImage precisionModelLogo = wxImage(wxString::FromUTF8(Slic3r::var("precision.png")), wxBITMAP_TYPE_PNG);
		fastModelLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		nornmalModelLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		precisionModelLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		wxBitmap fastModelLogoBitmap(fastModelLogo);
		wxBitmap normalModelLogoBitmap(nornmalModelLogo);
		wxBitmap precisionLogoBitmap(precisionModelLogo);

		wxImage sfastModelLogo = wxImage(wxString::FromUTF8(Slic3r::var("fast_select.png")), wxBITMAP_TYPE_PNG);
		wxImage snornmalModelLogo = wxImage(wxString::FromUTF8(Slic3r::var("normal_select.png")), wxBITMAP_TYPE_PNG);
		wxImage sprecisionLogo = wxImage(wxString::FromUTF8(Slic3r::var("precision_select.png")), wxBITMAP_TYPE_PNG);
		sfastModelLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		snornmalModelLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		sprecisionLogo.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
		wxBitmap sfastModelLogoBitmap(sfastModelLogo);
		wxBitmap snormalModelLogoBitmap(snornmalModelLogo);
		wxBitmap sPrecisionLogoBitmap(sprecisionLogo);


		m_pEasyWidget = new AnkerEasyPanel(this, wxID_ANY);		
		m_pEasyWidget->createrItem(_L("common_slicepannel_easy3_fast"),
			fastModelLogoBitmap.ConvertToImage(),
			sfastModelLogoBitmap.ConvertToImage(),
			checkScaledBitmap.ConvertToImage());

		m_pEasyWidget->createrItem(_L("common_slicepannel_easy2_normal"),
			normalModelLogoBitmap.ConvertToImage(),
			snormalModelLogoBitmap.ConvertToImage(),
			checkScaledBitmap.ConvertToImage());

		m_pEasyWidget->createrItem(_L("common_slicepannel_easy1_smooth"),
			precisionLogoBitmap.ConvertToImage(),
			sPrecisionLogoBitmap.ConvertToImage(),
			checkScaledBitmap.ConvertToImage());
		
		m_pEasyWidget->SetMinSize(AnkerSize(PANEL_WITH, 220));		
		m_pEasyWidget->SetSize(AnkerSize(PANEL_WITH, 220));
		m_pEasyWidget->setCurrentWidget(1);
		m_pEasyWidget->Bind(wxCUSTOMEVT_ANKER_EASY_ITEM_CLICKED, [this](wxCommandEvent&event) {		
			this->Freeze();
			wxStringClientData* pData = static_cast<wxStringClientData*>(event.GetClientObject());
			if (pData)
			{
				wxString numTpData = pData->GetData().ToStdString();
				wxString moduleName = wxString();
				if (numTpData == _L("common_slicepannel_easy3_fast"))
					moduleName = "Fast";
				else if(numTpData == _L("common_slicepannel_easy2_normal"))
					moduleName = "Normal";
				else if(numTpData == _L("common_slicepannel_easy1_smooth"))
					moduleName = "Precision";

				wxString currentPresetName = "";
				auto list = getValidParameter();
				auto iter = list.begin();
				while (iter != list.end())
				{
					wxString presetName = (*iter);
					if (presetName.Contains(moduleName))
					{
						currentPresetName = presetName;
						break;
					}
					++iter;
				}
				if (!currentPresetName.IsEmpty())
				{
					Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT)->select_preset(currentPresetName.ToStdString());
				}
			}
			this->Thaw();
			});
		m_pMainVSizer->Add(m_pEasyWidget, 0, wxEXPAND | wxALL, 12);
	}

	//parameter choose combox
	{
		wxBoxSizer* pParameterHSizer = new wxBoxSizer(wxHORIZONTAL);	

		m_currentObjName = new wxStaticText(this, wxID_ANY, "");
		m_currentObjName->SetFont(ANKER_FONT_NO_1);
		m_currentObjName->SetForegroundColour(wxColor(255,255,255));
		m_currentObjName->SetBackgroundColour(BgColor);		
		m_currentObjName->Hide();

		m_pPresetParameterComBox = new Slic3r::GUI::AnkerPlaterPresetComboBox(this, Slic3r::Preset::TYPE_PRINT, AnkerSize(276, 30));
		m_pPresetParameterComBox->set_button_clicked_function([this]() {
			ANKER_LOG_INFO << "preset combox of print clicked";
			m_pPresetParameterComBox->SetFocus();
			onComboBoxClick(m_pPresetParameterComBox);
			});
		m_pPresetParameterComBox->set_selection_changed_function([this](int selection) {
			ANKER_LOG_INFO << "preset combox of print clicked";
			onPresetComboSelChanged(m_pPresetParameterComBox, selection);
			});
		m_pPresetParameterComBox->Create(this,
								wxID_ANY,
								wxEmptyString,
								wxDefaultPosition,
								AnkerSize(328, ANKER_COMBOBOX_HEIGHT),
								wxNO_BORDER | wxCB_READONLY,
								wxDefaultValidator,
								"");
		m_pPresetParameterComBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
		m_pPresetParameterComBox->SetFont(ANKER_FONT_NO_1);
		m_pPresetParameterComBox->SetMinSize(AnkerSize(328, ANKER_COMBOBOX_HEIGHT));
		//m_pPresetParameterComBox->SetMaxSize(AnkerSize(328, ANKER_COMBOBOX_HEIGHT));
		m_pPresetParameterComBox->SetSize(AnkerSize(328, ANKER_COMBOBOX_HEIGHT));
		m_pPresetParameterComBox->SetBackgroundColour(BgColor);
		m_pPresetParameterComBox->update();
		m_pPresetParameterComBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
		
		//wxImage resetBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("reset_btn.png")), wxBITMAP_TYPE_PNG);
		wxImage resetBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("disabel_reset_btn.png")), wxBITMAP_TYPE_PNG);
		resetBtnImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);

#ifndef __APPLE__
		m_pResetBtn = new wxButton(this, wxID_ANY);
		m_pResetBtn->SetBitmap(resetBtnImage);		
#else
		//m_pResetBtn = new ScalableButton(this, wxID_ANY, "reset_btn", "", wxSize(20, 20));
		m_pResetBtn = new ScalableButton(this, wxID_ANY, "disabel_reset", "", wxSize(20, 20));
#endif // !__APPLE__
		m_pResetBtn->SetBackgroundColour(BgColor);
		m_pResetBtn->SetWindowStyleFlag(wxBORDER_NONE);		
		m_pResetBtn->SetMinSize(wxSize(20,20));
		m_pResetBtn->SetMaxSize(wxSize(20,20));
		m_pResetBtn->SetSize(wxSize(20,20));
		m_pResetBtn->Enable(false);
		wxCursor handCursor(wxCURSOR_HAND);
		m_pResetBtn->SetCursor(handCursor);
		m_pResetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			ANKER_LOG_INFO << "reset all parameter data";
			onResetBtnStatusChanged(false);
			resetAllUi();
			hideAllResetBtn();

			onDatachanged(event);
			});		

		wxImage saveAllBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("save_btn.png")), wxBITMAP_TYPE_PNG);		
		saveAllBtnImage.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
#ifndef __APPLE__
		m_pSaveAllBtn = new wxButton(this, wxID_ANY);
		m_pSaveAllBtn->SetBitmap(saveAllBtnImage);
#else
		m_pSaveAllBtn = new ScalableButton(this, wxID_ANY, "save_btn", "", wxSize(20, 20));
#endif // !__APPLE__
		m_pSaveAllBtn->SetWindowStyleFlag(wxBORDER_NONE);
		m_pSaveAllBtn->SetBackgroundColour(BgColor);
		m_pSaveAllBtn->SetMinSize(wxSize(20, 20));
		m_pSaveAllBtn->SetMaxSize(wxSize(20, 20));
		m_pSaveAllBtn->SetSize(wxSize(20, 20));		
		m_pSaveAllBtn->SetCursor(handCursor);
		m_pSaveAllBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			if (saveAllUi())
			{
				onResetBtnStatusChanged(false);
				hideAllResetBtn();
			}
			});

		m_pDividingLine = new wxPanel(this, wxID_ANY);
		m_pDividingLine->SetBackgroundColour(wxColour("#38393C"));
		m_pDividingLine->SetMinSize(wxSize(335, 1));

		pParameterHSizer->AddSpacer(12);
		pParameterHSizer->Add(m_currentObjName, wxALL | wxEXPAND, wxALL | wxEXPAND|wxLEFT, 3);
		pParameterHSizer->Add(m_pPresetParameterComBox, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pParameterHSizer->AddSpacer(6);
		pParameterHSizer->Add(m_pResetBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pParameterHSizer->AddSpacer(6);
		pParameterHSizer->Add(m_pSaveAllBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pParameterHSizer->AddSpacer(10);

		m_pMainVSizer->Add(pParameterHSizer, 0, wxALL | wxEXPAND, 0);
		m_pMainVSizer->AddSpacer(10);
		m_pMainVSizer->Add(m_pDividingLine, 0, wxEXPAND, 0);		
	}

	//custom general model
	{
		wxBoxSizer* pUserModelHSizer = new wxBoxSizer(wxHORIZONTAL);

// 		m_pHandleParameterLabel = new wxStaticText(this, wxID_ANY, _L("Display:"));
// 		m_pHandleParameterLabel->SetForegroundColour(wxColour("#FFFFFF"));
// 		m_pHandleParameterLabel->SetMinSize(wxSize(50, 25));
// 		//m_pHandleParameterLabel->SetMaxSize(wxSize(50, 25));
// 		m_pHandleParameterLabel->SetSize(wxSize(50, 25));
// 		m_pHandleParameterLabel->Hide();
// 
// 		wxArrayString itemList;
// 		itemList.Add(_L("Custom"));
// 		itemList.Add(_L("General"));
// 		
// 		m_pHandleParameterComBox = new AnkerSimpleCombox();
// 		m_pHandleParameterComBox->Create(this,
// 										wxID_ANY,
// 										wxEmptyString,
// 										wxDefaultPosition,
// 										wxDefaultSize,
// 										itemList,
// 										wxNO_BORDER | wxCB_READONLY);
// 		m_pHandleParameterComBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
// 		m_pHandleParameterComBox->SetBackgroundColour(BgColor);
// 		m_pHandleParameterComBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
// 		m_pHandleParameterComBox->SetMinSize(wxSize(70, 30));
// 		m_pHandleParameterComBox->SetSize(wxSize(70, 30));
// 		m_pHandleParameterComBox->SetSelection(0);
// 		m_pHandleParameterComBox->Hide();
// 
// 		m_pHandleParameterComBox->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& event) {
// 			AnkerSimpleCombox* pComboboxCtrl = dynamic_cast<AnkerSimpleCombox*>(event.GetEventObject());
// 			int index = 0;
// 			wxString text = wxString();
// 
// 			if (pComboboxCtrl)
// 			{
// 				index = pComboboxCtrl->GetSelection();
// 				text = pComboboxCtrl->GetString(index);
// 			}
// 
// 			});		
// 		wxImage btnImage = wxImage(wxString::FromUTF8(Slic3r::var("search_btn.png")), wxBITMAP_TYPE_PNG);
// 		btnImage.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
// #ifndef __APPLE__
// 		m_pCfgBtn = new wxButton(this, wxID_ANY);
// 		m_pCfgBtn->SetBitmap(btnImage);
// #else
// 		m_pCfgBtn = new ScalableButton(this, wxID_ANY, "search_btn", "", wxSize(16, 16));
// #endif // !__APPLE__
// 		
// 		m_pCfgBtn->SetWindowStyleFlag(wxBORDER_NONE);
// 		m_pCfgBtn->SetBackgroundColour(BgColor);
// 		m_pCfgBtn->SetMinSize(wxSize(20, 20));
// 		m_pCfgBtn->SetMaxSize(wxSize(20, 20));
// 		m_pCfgBtn->SetSize(wxSize(20, 20));
// 		
// 		m_pCfgBtn->Hide();
// 
// 		m_pCfgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
// 
// 			});

		wxImage searchBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("search_btn.png")), wxBITMAP_TYPE_PNG);
		searchBtnImage.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);

		{
			wxColour bgColor = wxColour(41, 42, 45);
			wxColour textColor = wxColour("#FFFFFF");

			m_popupWidget = new AnkerPopupWidget(this);
			m_popupWidget->AddItem(m_gSearchMap);

			m_popupWidget->Bind(wxCUSTOMEVT_ANKER_ITEM_CLICKED, [this](wxCommandEvent& event) {
			
				ANKER_LOG_INFO << "show search panel";
				wxVariant* pData = (wxVariant*)event.GetClientData();
				if (pData)
				{
					wxVariantList list = pData->GetList();
					auto key = list[0]->GetString().ToStdString();
					auto strValue = list[1]->GetString().ToStdString();

					m_pSearchTextCtrl->SetValue(strValue);
					switchToLaebl(key, strValue);
					wxString data = "";
				}

				if (m_popupWidget)
					m_popupWidget->Hide();

				});
			m_popupWidget->SetBackgroundColour(wxColour("#00FF00"));
			m_popupWidget->SetMaxSize(AnkerSize(PANEL_WITH - 35, 368));
			m_popupWidget->SetMinSize(AnkerSize(PANEL_WITH - 35, 368));
			m_popupWidget->SetSize(AnkerSize(PANEL_WITH - 35, 368));

			m_pSearchEdit = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
			m_pSearchEdit->SetBackgroundColour(bgColor);
			m_pSearchEdit->SetForegroundColour(textColor);
			m_pSearchEdit->SetMinSize(AnkerSize(PANEL_WITH, 24));
			//m_pSearchEdit->SetMaxSize(AnkerSize(300, 24));

			wxBoxSizer* pSearchHSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pSearchEdit->SetSizer(pSearchHSizer);

			wxButton* pSearchIcon = new wxButton(m_pSearchEdit, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			pSearchIcon->SetBackgroundColour(bgColor);
			pSearchIcon->SetMaxSize(AnkerSize(20, 20));
			pSearchIcon->SetBitmap(searchBtnImage);
			
			m_pSearchTextCtrl = new AnkerLineEdit(m_pSearchEdit, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);			
			m_pSearchTextCtrl->SetMinSize(AnkerSize(PANEL_WITH, 24));
			//m_pSearchTextCtrl->SetMaxSize(AnkerSize(360, 24));
			m_pSearchTextCtrl->SetBackgroundColour(bgColor);
			m_pSearchTextCtrl->SetForegroundColour(textColor);
			
			m_pSearchTextCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent &event) {
				
				wxString searchData = wxString();
				if (m_pSearchTextCtrl)
					searchData = m_pSearchTextCtrl->GetLineText(0).ToStdString();
				else
					return;

				m_popupWidget->Freeze();
				searchData.MakeLower();
				if (m_popupWidget)
				{

					wxPoint childPos = m_pSearchEdit->ClientToScreen(wxPoint(0, 0));
					childPos.y = childPos.y + m_pSearchEdit->GetRect().GetHeight() + 4;
					m_popupWidget->Move(childPos);
					m_popupWidget->Show();

					if (searchData.IsEmpty())
					{
						m_popupWidget->showAllItem();
						m_popupWidget->Thaw();
						return;
					}
				}

				std::map<wxString, std::vector<wxString>> searchResMap;
				getSearResMap(searchResMap,searchData);
				m_popupWidget->showResMap(searchResMap);
				int couts = searchResMap.size();
				m_popupWidget->Thaw();
			});

			m_pSearchTextCtrl->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this](wxCommandEvent& event) {
				if (m_popupWidget)
					m_popupWidget->Hide();

				});

			
			pSearchHSizer->Add(pSearchIcon, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
			pSearchHSizer->AddSpacer(4);
			pSearchHSizer->Add(m_pSearchTextCtrl, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL | wxALL,wxALL|wxEXPAND, 0);
			pSearchHSizer->AddSpacer(4);
		}

		//pUserModelHSizer->AddSpacer(12);
		
		//pUserModelHSizer->Add(m_pHandleParameterLabel);
		//pUserModelHSizer->Add(m_pHandleParameterComBox);
		
		//pUserModelHSizer->Add(m_pCfgBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		//pUserModelHSizer->AddSpacer(4);
		//pUserModelHSizer->Add(m_pSearchBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		//pUserModelHSizer->Add(m_pSearchEdit, 0, wxEXPAND | wxALL, 12);
		//pUserModelHSizer->AddStretchSpacer(1);
		//pUserModelHSizer->AddSpacer(12);

		m_pMainVSizer->Add(m_pSearchEdit, 0, wxEXPAND | wxALL, 12);
	}

	//tab HORIZONTAL scroll bar, 
	{
		wxBoxSizer* pScrolledHWinSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pTabBtnScrolledWindow = new wxScrolledWindow(this, wxID_ANY);
		m_pTabBtnScrolledWindow->SetMinSize(AnkerSize(PANEL_WITH - 30, 50));
		m_pTabBtnScrolledWindow->SetSize(AnkerSize(PANEL_WITH - 30, 50));
		m_pTabBtnScrolledWindow->SetSizer(pScrolledHWinSizer);
		m_pTabBtnScrolledWindow->SetBackgroundColour(BgColor);

		m_pTabBtnScrolledWindow->SetVirtualSize(AnkerSize(PANEL_WITH - 30, 31));
		m_pTabBtnScrolledWindow->SetScrollRate(20, 0);
		m_pTabBtnScrolledWindow->SetScrollbars(20, 0, PANEL_WITH / 20, 1);

		wxString btnBGcolor = "#292A2D";
		wxString btnNormalTextColor = "#A9AAAB";
		wxString btnNormalColor = "#292A2D";
		wxString textSelectColor = "#62D361";
		wxString hoverColor = "#324435";

		AnkerChooseBtn* pLayersAPBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Layers and perimeters"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pInfillBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Infill"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pSkirtABBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Skirt and brim"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pSupperotMBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Support material"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pSpeedBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Speed"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pMultipleEBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Multiple Extruders"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
		AnkerChooseBtn* pAdvancedBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, _L("Advanced"), btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);

		wxCursor handCursor(wxCURSOR_HAND);

		pLayersAPBtn->SetCursor(handCursor);
		pInfillBtn->SetCursor(handCursor);
		pSkirtABBtn->SetCursor(handCursor);
		pSupperotMBtn->SetCursor(handCursor);
		pSpeedBtn->SetCursor(handCursor);
		pMultipleEBtn->SetCursor(handCursor);
		pAdvancedBtn->SetCursor(handCursor);

		pLayersAPBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pInfillBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pSkirtABBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pSupperotMBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pSpeedBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pMultipleEBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
		pAdvancedBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);

		m_tabBtnVector.push_back(pLayersAPBtn);
		m_tabBtnVector.push_back(pInfillBtn);
		m_tabBtnVector.push_back(pSkirtABBtn);
		m_tabBtnVector.push_back(pSupperotMBtn);
		m_tabBtnVector.push_back(pSpeedBtn);
		m_tabBtnVector.push_back(pMultipleEBtn);
		m_tabBtnVector.push_back(pAdvancedBtn);

		pLayersAPBtn->setTextSelectColor(textSelectColor);
		pLayersAPBtn->setNormalBGColor(btnNormalColor);
		pLayersAPBtn->setHoverBGColor(hoverColor);
		pLayersAPBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pLayersAPBtn->setBtnStatus(ChooseBtn_Select);

		pInfillBtn->setTextSelectColor(textSelectColor);
		pInfillBtn->setNormalBGColor(btnNormalColor);
		pInfillBtn->setHoverBGColor(hoverColor);
		pInfillBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pInfillBtn->setBtnStatus(ChooseBtn_Normal);

		pSkirtABBtn->setTextSelectColor(textSelectColor);
		pSkirtABBtn->setNormalBGColor(btnNormalColor);
		pSkirtABBtn->setHoverBGColor(hoverColor);
		pSkirtABBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pSkirtABBtn->setBtnStatus(ChooseBtn_Normal);

		pSupperotMBtn->setTextSelectColor(textSelectColor);
		pSupperotMBtn->setNormalBGColor(btnNormalColor);
		pSupperotMBtn->setHoverBGColor(hoverColor);
		pSupperotMBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pSupperotMBtn->setBtnStatus(ChooseBtn_Normal);

		pSpeedBtn->setTextSelectColor(textSelectColor);
		pSpeedBtn->setNormalBGColor(btnNormalColor);
		pSpeedBtn->setHoverBGColor(hoverColor);
		pSpeedBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pSpeedBtn->setBtnStatus(ChooseBtn_Normal);

		pMultipleEBtn->setTextSelectColor(textSelectColor);
		pMultipleEBtn->setNormalBGColor(btnNormalColor);
		pMultipleEBtn->setHoverBGColor(hoverColor);
		pMultipleEBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pMultipleEBtn->setBtnStatus(ChooseBtn_Normal);

		pAdvancedBtn->setTextSelectColor(textSelectColor);
		pAdvancedBtn->setNormalBGColor(btnNormalColor);
		pAdvancedBtn->setHoverBGColor(hoverColor);
		pAdvancedBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
		pAdvancedBtn->setBtnStatus(ChooseBtn_Normal);

		pScrolledHWinSizer->Add(pLayersAPBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pInfillBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pSkirtABBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pSupperotMBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pSpeedBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pMultipleEBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		pScrolledHWinSizer->Add(pAdvancedBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);

		wxBoxSizer* pScrolledWindHSizer = new wxBoxSizer(wxHORIZONTAL);
		pScrolledWindHSizer->AddSpacer(12);
		pScrolledWindHSizer->Add(m_pTabBtnScrolledWindow, wxALL | wxHORIZONTAL, wxALL | wxEXPAND, 0);
		pScrolledWindHSizer->AddSpacer(12);

		m_pMainVSizer->Add(pScrolledWindHSizer, wxALL | wxHORIZONTAL, wxALL | wxEXPAND, 0);
		m_pMainVSizer->AddSpacer(10);
	}

	m_pTabItemScrolledWindow = new wxScrolledWindow(this, wxID_ANY);
	m_pTabItemScrolledWindow->SetScrollRate(0, 50);

	m_pTabItemScrolledWindow->SetVirtualSize(AnkerSize(PANEL_WITH - 30, 500));
	m_pTabItemScrolledWindow->SetMinSize(AnkerSize(PANEL_WITH,-1));
	m_pTabItemScrolledWindow->SetSize(AnkerSize(PANEL_WITH,-1));
	m_pTabItemScrolledWindow->SetScrollbars(0, 30, PANEL_WITH / 50, 500 / 50);
	wxBoxSizer* pTabItemScrolledVWinSizer = new wxBoxSizer(wxVERTICAL);
	m_pTabItemScrolledWindow->SetSizer(pTabItemScrolledVWinSizer);	
	m_pTabItemScrolledWindow->SetBackgroundColour(BgColor);
	//item VERTICAL
	//Layers and perimeters
	//title---group
	{
		wxString strTab = _L("Layers and perimeters");
		std::list<AnkerPrintParaItem*> itemList;

		wxStringList unitList;
		unitList.Add(_L("mm"));
		AnkerPrintParaItem* pLHItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Layer_height", _L("Layer height"), strTab, wxID_ANY);
		pLHItem->SetMaxSize(ItemSize);		
		pLHItem->createItem(_L("Layer height"), ItemEditUinit, Item_float, unitList);
		pLHItem->createItem(_L("First layer height"), ItemEditUinit, Item_floatOrPercent, unitList);
		pLHItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged,this);
		pLHItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn,this);


		AnkerPrintParaItem* pVSItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Vertical_shells", _L("Vertical shells"), strTab, wxID_ANY);
		pVSItem->SetMaxSize(ItemSize);
		pVSItem->createItem(_L("Perimeters"), ItemSpinBox, Item_int);
		pVSItem->createItem(_L("Spiral vase"), ItemCheckBox, Item_bool);
		pVSItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pVSItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		//Horizontal sheells  
		AnkerPrintParaItem* pHSItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Extrusion_width", _L("Horizontal shells"), strTab, wxID_ANY);
		pHSItem->SetMaxSize(ItemSize);
		pHSItem->createItem(_L("Top solid layers"), ItemSpinBox, Item_int);
		pHSItem->createItem(_L("Bottom solid layers"), ItemSpinBox, Item_int);

		pHSItem->createItem(_L("Top minimum shell thickness"), ItemEditUinit, Item_float, unitList);
		pHSItem->createItem(_L("Bottom minimum shell thickness"), ItemEditUinit, Item_float, unitList);
		pHSItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pHSItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pQSSItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Quality_(slower_slicing)", _L("Quality (slower slicing)"), strTab, wxID_ANY);
		pQSSItem->SetMaxSize(ItemSize);
		pQSSItem->createItem(_L("Extra perimeters if needed"), ItemCheckBox, Item_bool);
		pQSSItem->createItem(_L("Extra perimeters on overhangs(Experimental)"), ItemCheckBox, Item_bool);
		pQSSItem->createItem(_L("Avoid crossing curled overhangs(Experimental)"), ItemCheckBox,Item_bool);
		pQSSItem->createItem(_L("Avoid crossing perimeters"), ItemCheckBox,Item_bool);		
		unitList.Clear();
		unitList.Add(wxT("mm or %"));
		pQSSItem->createItem(_L("Avoid crossing perimeters - Max detour length"), ItemEditUinit, Item_floatOrPercent, unitList);
		pQSSItem->createItem(_L("Detect thin walls"), ItemCheckBox, Item_bool);
		pQSSItem->createItem(_L("Thick bridges"), ItemCheckBox, Item_bool);
		pQSSItem->createItem(_L("Detect bridging perimeters"), ItemCheckBox, Item_bool);
		pQSSItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pQSSItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		getItemList(dataList, List_Seam_Position);
		
		AnkerPrintParaItem* pADItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Advanced", _L("Advanced"), strTab, wxID_ANY);
		pADItem->SetMaxSize(ItemSize);
		pADItem->createItem(_L("Seam position"), ItemComBox, Item_enum_SeamPosition,dataList);
		pADItem->createItem(_L("Staggered inner seams"), ItemCheckBox, Item_bool);
		pADItem->createItem(_L("External perimeters first"), ItemCheckBox, Item_bool);
		pADItem->createItem(_L("Fill gaps"), ItemCheckBox, Item_bool);
		getItemList(dataList, List_Perimeter_generator);
		pADItem->createItem(_L("Perimeter generator"), ItemComBox, Item_enum_PerimeterGeneratorType, dataList);
		pADItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pADItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pFSItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Fuzzy_skin_(experimental)", _L("Fuzzy skin (experimental)"), strTab, wxID_ANY);
		pFSItem->SetMaxSize(ItemSize);
		getItemList(dataList, List_Fuzzy_skin);
		pFSItem->createItem(_L("Fuzzy Skin"), ItemComBox, Item_enum_FuzzySkinType, dataList);
		unitList.Clear();
		unitList.Add(wxT("mm"));
		pFSItem->createItem(_L("Fuzzy skin thickness"), ItemEditUinit, Item_float, unitList);
		pFSItem->createItem(_L("Fuzzy skin point distance"), ItemEditUinit, Item_float, unitList);
		pFSItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pFSItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
		itemList.push_back(pLHItem);
		itemList.push_back(pVSItem);		
		itemList.push_back(pHSItem);
		itemList.push_back(pQSSItem);
		itemList.push_back(pADItem);
		itemList.push_back(pFSItem);

		pTabItemScrolledVWinSizer->Add(pLHItem, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pVSItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pHSItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pQSSItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pADItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pFSItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		
		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}

	//Infill
	{
		wxString strTab = _L("Infill");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("%"));

		AnkerPrintParaItem* pLHItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Infill", _L("Infill"), strTab, wxID_ANY);
		pLHItem->SetMaxSize(ItemSize);
		getItemList(dataList, List_Fill_density);
		pLHItem->createItem(_L("Fill density"), ItemEditBox, Item_Percent, dataList);
		getItemList(dataList, List_Fill_pattern);
		pLHItem->createItem(_L("Fill pattern"), ItemComBox, Item_enum_InfillPattern, dataList);

		getItemList(dataList, List_Length_of_th_infill_anchor);
		pLHItem->createItem(_L("Length of the infill anchor"), ItemEditBox, Item_floatOrPercent, dataList);
		getItemList(dataList, List_Maximum_length_of_the_infill_anchor);
		pLHItem->createItem(_L("Maximum length of the infill anchor"), ItemEditBox, Item_floatOrPercent, dataList);
		getItemList(dataList, List_Top_fill_pattern);
		pLHItem->createItem(_L("Top fill pattern"), ItemComBox, Item_enum_InfillPattern, dataList);
		getItemList(dataList, List_Bottom_fill_pattern);
		pLHItem->createItem(_L("Bottom fill pattern"), ItemComBox, Item_enum_InfillPattern, dataList);
		pLHItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pLHItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pIroningItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Ironing", _L("Ironing"), strTab, wxID_ANY);
		pIroningItem->SetMaxSize(ItemSize);
		pIroningItem->createItem(_L("Enable ironing"), ItemCheckBox, Item_bool);
		getItemList(dataList, List_Ironing_Type);
		pIroningItem->createItem(_L("Ironing Type"), ItemComBox, Item_enum_IroningType, dataList);
		pIroningItem->createItem(_L("Flow rate"), ItemEditUinit, Item_Percent, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pIroningItem->createItem(_L("Spacing between ironing passes"), ItemEditUinit, Item_float, unitList);
		pIroningItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pIroningItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		AnkerPrintParaItem* pRptItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Reducing_printing_time", _L("Reducing printing time"), strTab, wxID_ANY);
		pRptItem->SetMaxSize(ItemSize);
		pRptItem->createItem(_L("Combine infill every"), ItemSpinBox, Item_int);
		pRptItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pRptItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pAdvancedItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Advanced", _L("Advanced"), strTab, wxID_ANY);
		pAdvancedItem->SetMaxSize(ItemSize);
		pAdvancedItem->createItem(_L("Solid infill every"), ItemSpinBox, Item_int);
		unitList.Clear();
		unitList.Add(_L("°"));
		pAdvancedItem->createItem(_L("Fill angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm²"));
		pAdvancedItem->createItem(_L("Solid infill threshold area"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("°"));
		pAdvancedItem->createItem(_L("Bridging angle"), ItemEditUinit, Item_float, unitList);
		pAdvancedItem->createItem(_L("Only retract when crossing perimeters"), ItemCheckBox, Item_bool);
		pAdvancedItem->createItem(_L("Infill before perimeters"), ItemCheckBox, Item_bool);
		pAdvancedItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pAdvancedItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		itemList.push_back(pLHItem);
		itemList.push_back(pIroningItem);
		itemList.push_back(pRptItem);
		itemList.push_back(pAdvancedItem);

		pTabItemScrolledVWinSizer->Add(pLHItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pIroningItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pRptItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pAdvancedItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}

	//Skirt and brim
	{
		wxString strTab = _L("Skirt and brim");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("mm"));
		wxStringList dataList = {};		
		
		AnkerPrintParaItem* pSkirtItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Skirt", _L("Skirt"), strTab, wxID_ANY);
		pSkirtItem->SetMaxSize(ItemSize);
		pSkirtItem->createItem(_L("Loops (minimum)"),ItemSpinBox, Item_int);
		pSkirtItem->createItem(_L("Distance from brim/object"), ItemEditUinit, Item_float, unitList);
		pSkirtItem->createItem(_L("Skirt height"),ItemSpinBox, Item_int);
		getItemList(dataList, List_Draft_shield);
		pSkirtItem->createItem(_L("Draft shield"), ItemComBox, Item_enum_DraftShield, dataList);
		pSkirtItem->createItem(_L("Minimal filament extrusion length"),ItemEditUinit, Item_float,unitList);
		pSkirtItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pSkirtItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pBrimItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Brim", _L("Brim"), strTab, wxID_ANY);
		pBrimItem->SetMaxSize(ItemSize);
		getItemList(dataList, List_Brim_type);
		pBrimItem->createItem(_L("Brim type"), ItemComBox, Item_enum_BrimType,dataList);
		pBrimItem->createItem(_L("Brim width"), ItemEditUinit, Item_float, unitList);
		pBrimItem->createItem(_L("Brim separation gap"), ItemEditUinit, Item_float, unitList);
		pBrimItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pBrimItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		itemList.push_back(pSkirtItem);
		itemList.push_back(pBrimItem);

		pTabItemScrolledVWinSizer->Add(pSkirtItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pBrimItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));

	}

	//Support material
	{
		wxString strTab = _L("Support material");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("%"));
		wxStringList dataList = {};

		AnkerPrintParaItem* pSmItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Support_material", _L("Support material"), strTab, wxID_ANY);
		pSmItem->SetMaxSize(ItemSize);
		pSmItem->createItem(_L("Generate support material"), ItemCheckBox, Item_bool);
		pSmItem->createItem(_L("Auto generated supports"), ItemCheckBox, Item_bool);
		pSmItem->createItem(_L("Overhang threshold"), ItemSpinBox, Item_int);
		pSmItem->createItem(_L("Enforce support for the first"), ItemSpinBox, Item_int);
		pSmItem->createItem(_L("First layer density"), ItemEditUinit, Item_Percent, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pSmItem->createItem(_L("First layer expansion"), ItemEditUinit, Item_float, unitList);
		pSmItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pSmItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
		
		
		AnkerPrintParaItem* pRaftItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Raft", _L("Raft"), strTab, wxID_ANY);
		pRaftItem->SetMaxSize(ItemSize);
		pRaftItem->createItem(_L("Raft layers"), ItemSpinBox, Item_int);
		pRaftItem->createItem(_L("Raft contact Z distance"), ItemEditUinit, Item_float, unitList);
		pRaftItem->createItem(_L("Raft expansion"), ItemEditUinit, Item_float, unitList);
		pRaftItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pRaftItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		AnkerPrintParaItem* pOfsmarItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Options_for_support_material_and_raft", _L("Options for support material and raft"), strTab, wxID_ANY);
		pOfsmarItem->SetMaxSize(ItemSize);
		getItemList(dataList, List_Style);
		pOfsmarItem->createItem(_L("Style"), ItemComBox, Item_enum_SupportMaterialStyle, dataList);
		getItemList(dataList, List_Top_contact_Z_distance);
		pOfsmarItem->createItem(_L("Top contact Z distance"), ItemEditBox, Item_float, dataList);//specail handle
		getItemList(dataList, List_Bottom_contact_Z_distance);
		pOfsmarItem->createItem(_L("Bottom contact Z distance"), ItemEditBox, Item_float, dataList);
		getItemList(dataList, List_Pattern);
		pOfsmarItem->createItem(_L("Pattern"), ItemComBox, Item_enum_SupportMaterialPattern, dataList);
		pOfsmarItem->createItem(_L("With sheath around the support"), ItemCheckBox, Item_bool);
		pOfsmarItem->createItem(_L("Pattern spacing"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("°"));		
		pOfsmarItem->createItem(_L("Pattern angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pOfsmarItem->createItem(_L("Closing radius"), ItemEditUinit, Item_float, unitList);
		getItemList(dataList, List_Top_interface_layers);
		pOfsmarItem->createItem(_L("Top interface layers"), ItemEditBox, Item_int, dataList);
		getItemList(dataList, List_Bottom_interface_layers);
		pOfsmarItem->createItem(_L("Bottom interface layers"), ItemEditBox, Item_int, dataList);
		getItemList(dataList, List_interface_pattern);
		pOfsmarItem->createItem(_L("Interface pattern"), ItemComBox, Item_enum_SupportMaterialInterfacePattern, dataList);
		pOfsmarItem->createItem(_L("Interface pattern spacing"), ItemEditUinit, Item_float, unitList);
		pOfsmarItem->createItem(_L("Interface loops"), ItemCheckBox, Item_bool);
		pOfsmarItem->createItem(_L("Support on build plate only"), ItemCheckBox, Item_bool);
		unitList.Clear();
		unitList.Add(_L("mm or %"));
		pOfsmarItem->createItem(_L("XY separation between an object and its support"), ItemEditUinit, Item_floatOrPercent, unitList);
		pOfsmarItem->createItem(_L("Don't support bridges"), ItemCheckBox, Item_bool);
		pOfsmarItem->createItem(_L("Synchronize with object layers"), ItemCheckBox, Item_bool);
		pOfsmarItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pOfsmarItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pOssmarItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Organic_supports", _L("Organic supports"), strTab, wxID_ANY);
		pOssmarItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("°"));
		pOssmarItem->createItem(_L("Maximum Branch Angle"), ItemEditUinit, Item_float, unitList);
		pOssmarItem->createItem(_L("Preferred Branch Angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pOssmarItem->createItem(_L("Branch Diameter"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("°"));
		pOssmarItem->createItem(_L("Branch Diameter Angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pOssmarItem->createItem(_L("Tip Diameter"), ItemEditUinit, Item_float, unitList);
		pOssmarItem->createItem(_L("Branch Distance"), ItemEditUinit, Item_float);
		unitList.Clear();
		unitList.Add(_L("%"));
		pOssmarItem->createItem(_L("Branch Density"), ItemEditUinit, Item_Percent, unitList);
		pOssmarItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pOssmarItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		itemList.push_back(pSmItem);
		itemList.push_back(pRaftItem);
		itemList.push_back(pOfsmarItem);
		itemList.push_back(pOssmarItem);

		pTabItemScrolledVWinSizer->Add(pSmItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pRaftItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pOfsmarItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pOssmarItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}

	//Speed
	{
		wxString strTab = _L("Speed");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("mm/s"));		

		AnkerPrintParaItem* pSfpmItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Speed_for_non-print_moves", _L("Speed for print moves"), strTab, wxID_ANY);
		pSfpmItem->SetMaxSize(ItemSize);
		pSfpmItem->createItem(_L("Speed for print moves perimeters"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s or %"));
		pSfpmItem->createItem(_L("Small perimeters"), ItemEditUinit, Item_floatOrPercent, unitList);
		pSfpmItem->createItem(_L("Speed for print moves external perimeters"), ItemEditUinit, Item_floatOrPercent, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s"));
		pSfpmItem->createItem(_L("Speed for print moves infill"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s or %"));
		pSfpmItem->createItem(_L("Speed for print moves solid infill"), ItemEditUinit, Item_floatOrPercent, unitList);
		pSfpmItem->createItem(_L("Speed for print moves top solid infill"), ItemEditUinit, Item_floatOrPercent, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s"));
		pSfpmItem->createItem(_L("Support material"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s or %"));
		pSfpmItem->createItem(_L("Support material interface"), ItemEditUinit, Item_floatOrPercent, unitList);
		unitList.Clear();
		unitList.Add(_L("mm/s"));
		pSfpmItem->createItem(_L("Speed for print moves bridges"), ItemEditUinit, Item_float, unitList);
		pSfpmItem->createItem(_L("Gap fill"), ItemEditUinit, Item_float,unitList);
		pSfpmItem->createItem(_L("Ironing"), ItemEditUinit, Item_float, unitList);
		pSfpmItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pSfpmItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		AnkerPrintParaItem* pdosItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Dynamic_overhang_speed", _L("Dynamic overhang speed"), strTab, wxID_ANY);
		pdosItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm/s or %"));
		pdosItem->createItem(_L("Enable dynamic overhang speeds"), ItemCheckBox, Item_bool);
		pdosItem->createItem(_L("speed for 0% overlap (bridge)"), ItemEditUinit, Item_floatOrPercent, unitList);
		pdosItem->createItem(_L("speed for 25% overlap"), ItemEditUinit, Item_floatOrPercent, unitList);
		pdosItem->createItem(_L("speed for 50% overlap"), ItemEditUinit, Item_floatOrPercent, unitList);
		pdosItem->createItem(_L("speed for 75% overlap"), ItemEditUinit, Item_floatOrPercent, unitList);
		pdosItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pdosItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pSfnmItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Speed_for_non-print_moves", _L("Speed for non-print moves"), strTab, wxID_ANY);
		pSfnmItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm/s"));
		pSfnmItem->createItem(_L("Speed for non-print moves travel"), ItemEditUinit, Item_float, unitList);
		pSfnmItem->createItem(_L("Z travel"), ItemEditUinit, Item_float, unitList);
		pSfnmItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pSfnmItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pModifiersItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Modifiers", _L("Modifiers"), strTab, wxID_ANY);
		pModifiersItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm/s or %"));
		pModifiersItem->createItem(_L("First layer speed"), ItemEditUinit, Item_floatOrPercent, unitList);
		pModifiersItem->createItem(_L("Speed of object first layer over raft interface"), ItemEditUinit, Item_floatOrPercent, unitList);
		pModifiersItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pModifiersItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		AnkerPrintParaItem* pAcItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Acceleration_control_(advanced)", _L("Acceleration control (advanced)"), strTab, wxID_ANY);
		pAcItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm/s²"));
		pAcItem->createItem(_L("Acceleration control (advanced) external perimeters"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) perimeters"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) top solid infill"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) solid infill"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) infill"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) bridges"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("First layer"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("First object layer over raft interface"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Acceleration control (advanced) travel"), ItemEditUinit, Item_float, unitList);
		pAcItem->createItem(_L("Default"), ItemEditUinit, Item_float, unitList);
		pAcItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pAcItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		
		AnkerPrintParaItem* pAsItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Autospeed_(advanced)", _L("Auto Speed (advanced)"), strTab, wxID_ANY);
		pAsItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm/s"));
		pAsItem->createItem(_L("Max print speed"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm³/s"));
		pAsItem->createItem(_L("Max volumetric speed"), ItemEditUinit, Item_float, unitList);
		pAsItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pAsItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		AnkerPrintParaItem* pPaItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Pressure_equalizer_(experimental)", _L("Pressure equalizer (experimental)"), strTab, wxID_ANY);
		pPaItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm³/s²"));
		pPaItem->createItem(_L("Max volumetric slope positive"), ItemEditUinit, Item_float, unitList);
		pPaItem->createItem(_L("Max volumetric slope negative"), ItemEditUinit,Item_float, unitList);
		pPaItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pPaItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		itemList.push_back(pSfpmItem);
		itemList.push_back(pdosItem);
		itemList.push_back(pSfnmItem);
		itemList.push_back(pModifiersItem);
		itemList.push_back(pAcItem);
		itemList.push_back(pAsItem);
		itemList.push_back(pPaItem);

		pTabItemScrolledVWinSizer->Add(pSfpmItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pdosItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pSfnmItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pModifiersItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pAcItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pAsItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pPaItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}

	//Multiple Extruders
	{
		wxString strTab = _L("Multiple Extruders");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("mm"));
		
		AnkerPrintParaItem* pExtrudersItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Extruders", _L("Extruders"), strTab, wxID_ANY);
		pExtrudersItem->SetMaxSize(ItemSize);
		pExtrudersItem->createItem(_L("Perimeter extruder"), ItemSpinBox, Item_int);
		pExtrudersItem->createItem(_L("Infill extruder"), ItemSpinBox, Item_int);
		pExtrudersItem->createItem(_L("Solid infill extruder"), ItemSpinBox,Item_int);
		pExtrudersItem->createItem(_L("Support material/raft/skirt extruder"), ItemSpinBox,Item_int);
		pExtrudersItem->createItem(_L("Support material/raft interface extruder"), ItemSpinBox,Item_int);
		pExtrudersItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pExtrudersItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
		

		AnkerPrintParaItem* pOpItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Ooze_prevention", _L("Ooze prevention"), strTab, wxID_ANY);
		pOpItem->SetMaxSize(ItemSize);
		pOpItem->createItem(_L("Enable ooze prevention"), ItemCheckBox, Item_bool);
		pOpItem->createItem(_L("Temperature variation"), ItemSpinBox, Item_int);
		pOpItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pOpItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pWipeTowerItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Advanced", _L("Wipe tower"), strTab, wxID_ANY);
		pWipeTowerItem->SetMaxSize(ItemSize);
		pWipeTowerItem->createItem(_L("Enable wipe tower"), ItemCheckBox, Item_bool);

		pWipeTowerItem->createItem(_L("Position X"), ItemEditUinit, Item_float,unitList);
		pWipeTowerItem->createItem(_L("Position Y"), ItemEditUinit, Item_float, unitList);
		pWipeTowerItem->createItem(_L("Width"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("°"));
		pWipeTowerItem->createItem(_L("Wipe tower rotation angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pWipeTowerItem->createItem(_L("Wipe tower brim width"), ItemEditUinit, Item_float, unitList);
		pWipeTowerItem->createItem(_L("Maximal bridging distance"), ItemEditUinit, Item_float, unitList);

		unitList.Clear();
		unitList.Add(_L("°"));
		pWipeTowerItem->createItem(_L("Stabilization cone apex angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("%"));
		pWipeTowerItem->createItem(_L("Wipe tower purge lines spacing"), ItemEditUinit, Item_Percent, unitList);

		pWipeTowerItem->createItem(_L("No sparse layers (EXPERIMENTAL)"), ItemCheckBox, Item_bool);
		pWipeTowerItem->createItem(_L("Prime all printing extruders"), ItemCheckBox, Item_bool);
		pWipeTowerItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pWipeTowerItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pAdvancedItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Advanced", _L("Advanced"), strTab, wxID_ANY);
		pAdvancedItem->SetMaxSize(ItemSize);
		pAdvancedItem->createItem(_L("Interface shells"), ItemCheckBox, Item_bool);
		pAdvancedItem->createItem(_L("Maximum width of a segmented region"), ItemEditUinit, Item_float, unitList);
		pAdvancedItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pAdvancedItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);


		itemList.push_back(pExtrudersItem);
		itemList.push_back(pOpItem);
		itemList.push_back(pWipeTowerItem);
		itemList.push_back(pAdvancedItem);

		pTabItemScrolledVWinSizer->Add(pExtrudersItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pOpItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pWipeTowerItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pAdvancedItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}

	//Advanced
	{
		wxString strTab = _L("Advanced");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;
		unitList.Add(_L("mm or %"));
		wxStringList dataList = {};

		AnkerPrintParaItem* pEwItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Extrusion_width", _L("Extrusion width"), strTab, wxID_ANY);
		pEwItem->SetMaxSize(ItemSize);
		pEwItem->createItem(_L("Default extrusion width"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("First layer"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("Perimeters"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("External perimeters"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("Infill"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("Solid infill"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("Top solid infill"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->createItem(_L("Support material"), ItemEditUinit, Item_floatOrPercent, unitList);
		pEwItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pEwItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pOverlapItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Overlap", _L("Overlap"), strTab, wxID_ANY);
		pOverlapItem->SetMaxSize(ItemSize);
		pOverlapItem->createItem(_L("Infill/perimeters overlap"), ItemEditUinit, Item_floatOrPercent, unitList);
		pOverlapItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pOverlapItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pFlowItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Flow", _L("Flow"), strTab, wxID_ANY);
		pFlowItem->SetMaxSize(ItemSize);
		pFlowItem->createItem(_L("Bridge flow ratio"), ItemEditUinit, Item_float);
		pFlowItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pFlowItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pSlicingItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Slicing", _L("Slicing"), strTab, wxID_ANY);
		pSlicingItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("mm"));
		pSlicingItem->createItem(_L("Slice gap closing radius"), ItemEditUinit, Item_float, unitList);
		getItemList(dataList, List_Slicing_Mode);
		pSlicingItem->createItem(_L("Slicing Mode"), ItemComBox, Item_enum_SlicingMode,dataList);
		pSlicingItem->createItem(_L("Slice resolution"), ItemEditUinit, Item_float, unitList);
		pSlicingItem->createItem(_L("G-code resolution"), ItemEditUinit, Item_float,unitList);
		pSlicingItem->createItem(_L("XY Size Compensation"), ItemEditUinit, Item_float,unitList);
		pSlicingItem->createItem(_L("Elephant foot compensation"), ItemEditUinit, Item_float, unitList);
		pSlicingItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pSlicingItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		AnkerPrintParaItem* pApgItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Arachne_perimeter_generator", _L("Arachne perimeter generator"), strTab, wxID_ANY);
		pApgItem->SetMaxSize(ItemSize);
		unitList.Clear();
		unitList.Add(_L("°"));
		pApgItem->createItem(_L("Perimeter transitioning threshold angle"), ItemEditUinit, Item_float, unitList);
		unitList.Clear();
		unitList.Add(_L("mm or %"));
		pApgItem->createItem(_L("Perimeter transitioning filter margin"), ItemEditUinit, Item_floatOrPercent, unitList);
		pApgItem->createItem(_L("Perimeter transition length"), ItemEditUinit, Item_floatOrPercent, unitList);
		pApgItem->createItem(_L("Perimeter distribution count"), ItemSpinBox, Item_int);
		pApgItem->createItem(_L("Minimum perimeter width"), ItemEditUinit, Item_floatOrPercent, unitList);
		pApgItem->createItem(_L("Minimum feature size"), ItemEditUinit, Item_floatOrPercent, unitList);
		pApgItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pApgItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);

		itemList.push_back(pEwItem);
		itemList.push_back(pOverlapItem);
		itemList.push_back(pFlowItem);
		itemList.push_back(pSlicingItem);		
		itemList.push_back(pApgItem);


		pTabItemScrolledVWinSizer->Add(pEwItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pOverlapItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pFlowItem, wxSizerFlags().Expand().Border(wxBottom, 4));
		pTabItemScrolledVWinSizer->Add(pSlicingItem, wxSizerFlags().Expand().Border(wxBottom, 4));		
		pTabItemScrolledVWinSizer->Add(pApgItem, wxSizerFlags().Expand().Border(wxBottom, 4));

		m_windowTabMap.insert(std::make_pair(strTab.ToStdString(), itemList));
	}
	m_pMainVSizer->Add(m_pTabItemScrolledWindow, wxEXPAND | wxHORIZONTAL, wxALL | wxEXPAND, 0);
	m_pMainVSizer->AddSpacer(8);
	m_pMainVSizer->AddStretchSpacer(1);	

	m_pBtnPanel = new wxPanel(this);
	
	m_pBtnPanel->SetMaxSize(AnkerSize(-1, 40));	
	m_pBtnPanel->SetMinSize(AnkerSize(-1, 40));	
	m_pBtnPanel->SetSize(AnkerSize(-1, 40));	
	wxBoxSizer* pPanelHsizer = new wxBoxSizer(wxHORIZONTAL);
	m_pBtnPanel->SetSizer(pPanelHsizer);
	pPanelHsizer->AddSpacer(1);
	//pBtnPanel->SetBackgroundColour(wxColor(255,0,0));
	{
		if (!m_pSliceBtn)
		{
			//wxBoxSizer* pBtnHSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pSliceBtn = new AnkerBtn(m_pBtnPanel, wxID_ANY);
			m_pSliceBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {				

				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED);
				evt.SetEventObject(this);
				ProcessEvent(evt);
				});
			m_pSliceBtn->Enable(false);		
			m_pSliceBtn->SetRadius(4);

			m_pSliceBtn->SetText(_L("common_slicepannel_button_slice"));
			m_pSliceBtn->SetFont(ANKER_BOLD_FONT_NO_1);
			m_pSliceBtn->SetMaxSize(AnkerSize(-1, 30));
			m_pSliceBtn->SetMinSize(AnkerSize(-1, 30));
			m_pSliceBtn->SetSize(AnkerSize(-1, 30));

			m_pSliceBtn->SetBackgroundColour(wxColor("#3A3B3F"));

			m_pSliceBtn->SetNorTextColor(wxColor("#FFFFFF"));
			m_pSliceBtn->SetHoverTextColor(wxColor("#FFFFFF"));
			m_pSliceBtn->SetPressedTextColor(wxColor("#FFFFFF"));
			m_pSliceBtn->SetDisableTextColor(wxColor("#69696C"));

			m_pSliceBtn->SetBgNorColor(wxColor("#62D361"));
			m_pSliceBtn->SetBgHoverColor(wxColor("#81DC81"));
			m_pSliceBtn->SetBgPressedColor(wxColor("#4EA94E"));
			m_pSliceBtn->SetBgDisableColor(wxColor("#2E2F32"));

		}

		if (!m_pSaveProjectBtn)
		{
			//wxBoxSizer* pBtnHSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pSaveProjectBtn = new AnkerBtn(m_pBtnPanel, wxID_ANY);
			m_pSaveProjectBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
				
				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED);
				evt.SetEventObject(this);
				ProcessEvent(evt);
				});
			m_pSaveProjectBtn->Enable(true);			
			m_pSaveProjectBtn->SetTextColor(wxColor("#FFFFFF"));
			m_pSaveProjectBtn->SetRadius(4);
			m_pSaveProjectBtn->SetMaxSize(AnkerSize(-1, 30));
			m_pSaveProjectBtn->SetMinSize(AnkerSize(-1, 30));
			m_pSaveProjectBtn->SetSize(AnkerSize(-1, 30));
			m_pSaveProjectBtn->SetText(_L("common_slicepannel_button_saveproject"));
			m_pSaveProjectBtn->SetFont(ANKER_BOLD_FONT_NO_1);


			m_pSaveProjectBtn->SetBackgroundColour(wxColor("#3A3B3F"));

			m_pSaveProjectBtn->SetNorTextColor(wxColor("#62D361"));
			m_pSaveProjectBtn->SetHoverTextColor(wxColor("#62D361"));
			m_pSaveProjectBtn->SetPressedTextColor(wxColor("#62D361"));
			m_pSaveProjectBtn->SetDisableTextColor(wxColor("#69696C"));

			m_pSaveProjectBtn->SetBgNorColor(wxColor("#2F3B33"));
			m_pSaveProjectBtn->SetBgHoverColor(wxColor("#434E47"));
			m_pSaveProjectBtn->SetBgPressedColor(wxColor("#262F29"));
			m_pSaveProjectBtn->SetBgDisableColor(wxColor("#2E2F32"));

		}
	}
	pPanelHsizer->AddSpacer(12);
	pPanelHsizer->Add(m_pSaveProjectBtn, wxBottom, wxALL | wxEXPAND, 0);	
	pPanelHsizer->AddSpacer(8);
	pPanelHsizer->Add(m_pSliceBtn, wxBottom, wxALL | wxEXPAND, 0);
	pPanelHsizer->AddSpacer(12);
	m_pMainVSizer->Add(m_pBtnPanel, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALL | wxEXPAND, 0);
	SetSizer(m_pMainVSizer);
}


void AnkerParameterPanel::initDefaultData()
{
	m_lapVector = { _L("Layer height"),_L("First layer height"), _L("Perimeters"),_L("Spiral vase"), _L("Top solid layers"),_L("Bottom solid layers"),_L("Top minimum shell thickness"),_L("Bottom minimum shell thickness"),
	_L("Extra perimeters if needed"), _L("Extra perimeters on overhangs(Experimental)"), _L("Avoid crossing curled overhangs(Experimental)"),_L("Avoid crossing perimeters"), _L("Avoid crossing perimeters - Max detour length"),_L("Detect thin walls"),_L("Thick bridges"),_L("Detect bridging perimeters"),
	_L("Seam position"),_L("Staggered inner seams"), _L("External perimeters first"), _L("Fill gaps"),_L("Perimeter generator"),
	_L("Fuzzy Skin"),_L("Fuzzy skin thickness"),_L("Fuzzy skin point distance") };

	m_infillVector = { _L("Fill density"), _L("Fill pattern"), _L("Length of the infill anchor"),_L("Maximum length of the infill anchor"),_L("Top fill pattern"),_L("Bottom fill pattern"),
	_L("Enable ironing"),_L("Ironing Type"), _L("Flow rate"),_L("Spacing between ironing passes"),
	_L("Combine infill every"),
	_L("Solid infill every"),_L("Fill angle"), _L("Solid infill threshold area"),_L("Bridging angle"),_L("Only retract when crossing perimeters"),_L("Infill before perimeters") };

	m_sabVector = { _L("Loops (minimum)"), _L("Distance from brim/object"),_L("Skirt height"),_L("Draft shield"),_L("Minimal filament extrusion length"),
	_L("Brim type"),_L("Brim width"),_L("Brim separation gap") };

	m_smVector = { _L("Generate support material"), _L("Auto generated supports"),_L("Overhang threshold"), _L("Enforce support for the first"),_L("First layer density"),_L("First layer expansion"),
	_L("Raft layers"),_L("Raft contact Z distance"),_L("Raft expansion"),
	_L("Style"), _L("Top contact Z distance"),_L("Bottom contact Z distance"),_L("Pattern"), _L("With sheath around the support"),_L("Pattern spacing"),_L("Pattern angle"),_L("Closing radius"),_L("Top interface layers"),_L("Bottom interface layers"),_L("Interface pattern"),_L("Interface pattern spacing"),_L("Interface loops"),
	_L("Support on build plate only"),_L("XY separation between an object and its support"),_L("Don't support bridges"),_L("Synchronize with object layers"),
	_L("Maximum Branch Angle"),_L("Preferred Branch Angle"),_L("Branch Diameter"),_L("Branch Diameter Angle"),_L("Tip Diameter"),_L("Branch Distance"),_L("Branch Density") };

	m_speedVector = { _L("Speed for print moves perimeters"),_L("Small perimeters"),_L("Speed for print moves external perimeters"),_L("Speed for print moves infill"),_L("Speed for print moves solid infill"),_L("Speed for print moves top solid infill"),_L("Support material"),_L("Support material interface"),_L("Speed for print moves bridges"),_L("Gap fill"),_L("Ironing"),
	_L("Enable dynamic overhang speeds"),_L("speed for 0% overlap (bridge)"),_L("speed for 25% overlap"),_L("speed for 50% overlap"),_L("speed for 75% overlap"),
	_L("Speed for non-print moves travel"), _L("Z travel"),
	_L("First layer speed"), _L("Speed of object first layer over raft interface"),
	_L("Acceleration control (advanced) external perimeters"),_L("Acceleration control (advanced) perimeters"),_L("Acceleration control (advanced) top solid infill"),_L("Acceleration control (advanced) solid infill"),_L("Acceleration control (advanced) infill"),_L("Acceleration control (advanced) bridges"),_L("First layer"),_L("First object layer over raft interface"),_L("Acceleration control (advanced) travel"),_L("Default"),
	_L("Max print speed"),_L("Max volumetric speed"),
	_L("Max volumetric slope positive"),_L("Max volumetric slope negative") };

	m_meVector = { _L("Perimeter extruder"),_L("Infill extruder"),_L("Solid infill extruder"),_L("Support material/raft/skirt extruder"),_L("Support material/raft interface extruder"),
	_L("Enable ooze prevention"),_L("Temperature variation"),
	_L("Enable wipe tower"), _L("Position X"),_L("Position Y"), _L("Width"), _L("Wipe tower rotation angle"),_L("Wipe tower brim width"), _L("Maximal bridging distance"),_L("Stabilization cone apex angle"),_L("Wipe tower purge lines spacing"),_L("No sparse layers (EXPERIMENTAL)"),_L("Prime all printing extruders"),
	_L("Interface shells"),_L("Maximum width of a segmented region") };

	m_advancedVector = { _L("Default extrusion width"),_L("First layer"),_L("Perimeters"),_L("External perimeters"),_L("Infill"),_L("Solid infill"),_L("Top solid infill"),_L("Support material"),
	_L("Infill/perimeters overlap"),
	_L("Bridge flow ratio"),
	_L("Slice gap closing radius"),_L("Slicing Mode"), _L("Slice resolution"),_L("G-code resolution"),_L("XY Size Compensation"),_L("Elephant foot compensation"),
	_L("Perimeter transitioning threshold angle"),_L("Perimeter transitioning filter margin"),_L("Perimeter transition length"),_L("Perimeter distribution count"),_L("Minimum perimeter width"),_L("Minimum feature size") };

	m_gSearchMap = {
					{_L("Layers and perimeters"), m_lapVector},
					{_L("Infill"), m_infillVector},
					{_L("Skirt and brim"), m_sabVector},
					{_L("Support material"), m_smVector},
					{_L("Speed"), m_speedVector},
					{_L("Multiple Extruders"), m_meVector},
					{_L("Advanced"), m_advancedVector},
					};

	m_fillPatternData = { {("Rectilinear"), _L("Rectilinear")},
						{("Aligned Rectilinear"), _L("Aligned Rectilinear")},
						{("Alignedrectilinear"), _L("Aligned Rectilinear")},
						{("Grid"), _L("Grid")},
						{("Triangles"), _L("Triangles")},
						{("Stars"), _L("Stars")},
						{("Cubic"), _L("Cubic")},
						{("Line"), _L("Line")},
						{("Concentric"), _L("Concentric")},
						{("Honeycomb"), _L("Honeycomb")},
						{("3D Honeycomb"), _L("3D Honeycomb")},
						{("Gyroid"), _L("Gyroid")},
						{("Hilbert Curve"), _L("Hilbert Curve")},
						{("Hilbertcurve"), _L("Hilbert Curve")},
						{("Archimedean Chords"),_L("Archimedean Chords")},
						{("Archimedeanchords"),_L("Archimedean Chords")},
						{("Octagram Spiral"),_L("Octagram Spiral")},
						{("Octagramspiral"),_L("Octagram Spiral")},
						{("Adaptive Cubic"), _L("Adaptive Cubic")},
						{("Support Cubic"), _L("Support Cubic")},
						{("Lightning"), _L("Lightning")},
						{("Monotonic"), _L("Monotonic")},
						{("Monotonic Lines"), _L("Monotonic Lines")},
						{("Monotoniclines"), _L("Monotonic Lines")},
	};

	m_lapMap = {
											{_L("Layer height"),("layer_height")},
											{_L("First layer height"),("first_layer_height")},
											{_L("Perimeters"),("perimeters")},
											{ _L("Spiral vase"),("spiral_vase")},
											{_L("Top solid layers"),("top_solid_layers")},
											{_L("Top minimum shell thickness"),("top_solid_min_thickness")},
											{_L("Bottom solid layers"),("bottom_solid_layers")},
											{_L("Bottom minimum shell thickness"),("bottom_solid_min_thickness")},
											{_L("Extra perimeters if needed"),("extra_perimeters")},
											{_L("Extra perimeters on overhangs(Experimental)"),("extra_perimeters_on_overhangs")},
											{_L("Avoid crossing curled overhangs(Experimental)"),("avoid_crossing_curled_overhangs")},
											{_L("Avoid crossing perimeters"),("avoid_crossing_perimeters")},
											{_L("Avoid crossing perimeters - Max detour length"),("avoid_crossing_perimeters_max_detour")},
											{_L("Detect thin walls"),("thin_walls")},
											{_L("Thick bridges"),("thick_bridges")},
											{_L("Detect bridging perimeters"),("overhangs")},
											{_L("Seam position"),("seam_position")},
											{_L("Staggered inner seams"),("staggered_inner_seams")},
											{_L("External perimeters first"),("external_perimeters_first")},
											{_L("Fill gaps"),("gap_fill_enabled")},
											{_L("Perimeter generator"),("perimeter_generator")},
											{_L("Fuzzy Skin"),("fuzzy_skin")},
											{_L("Fuzzy skin thickness"),("fuzzy_skin_thickness")},
											{_L("Fuzzy skin point distance"),("fuzzy_skin_point_dist")},
	};
	m_infillMap = { {_L("Fill density"),("fill_density")},
												  {_L("Fill pattern"),("fill_pattern")},
												  {_L("Length of the infill anchor"),("infill_anchor")},
												  {_L("Maximum length of the infill anchor"),("infill_anchor_max")},
												  {_L("Top fill pattern"),("top_fill_pattern")},
												  {_L("Bottom fill pattern"),("bottom_fill_pattern")},
												  {_L("Enable ironing"),("ironing")},
												  {_L("Ironing Type"),("ironing_type")},
												  {_L("Flow rate"),("ironing_flowrate")},
												  {_L("Spacing between ironing passes"),("ironing_spacing")},
												  {_L("Combine infill every"),("infill_every_layers")},
												  {_L("Solid infill every"),("solid_infill_every_layers")},
												  {_L("Fill angle"),("fill_angle")},
												  {_L("Solid infill threshold area"),("solid_infill_below_area")},
												  {_L("Bridging angle"),("bridge_angle")},
												  {_L("Only retract when crossing perimeters"),("only_retract_when_crossing_perimeters")},
												  {_L("Infill before perimeters"),("infill_first")},
	};
	m_sabMap = {
											   {_L("Loops (minimum)"),("skirts")},
											   {_L("Distance from brim/object"),("skirt_distance")},
											   {_L("Skirt height"),("skirt_height")},
											   {_L("Draft shield"),("draft_shield")},
											   {_L("Minimal filament extrusion length"),("min_skirt_length")},
											   {_L("Brim type"),("brim_type")},
											   {_L("Brim width"),("brim_width")},
											   {_L("Brim separation gap"),("brim_separation")},
	};
	m_smMap = {
											{_L("Generate support material"),("support_material")},
											{_L("Auto generated supports"),("support_material_auto")},
											{_L("Overhang threshold"),("support_material_threshold")},
											{_L("Enforce support for the first"),("support_material_enforce_layers")},
											{_L("First layer density"),("raft_first_layer_density")},
											{_L("First layer expansion"),("raft_first_layer_expansion")},
											{_L("Raft layers"),("raft_layers")},
											{_L("Raft contact Z distance"),("raft_contact_distance")},
											{_L("Raft expansion"),("raft_expansion")},
											{_L("Style"),("support_material_style")},
											{_L("Top contact Z distance"),("support_material_contact_distance")},
											{_L("Bottom contact Z distance"),("support_material_bottom_contact_distance")},
											{_L("Pattern"),("support_material_pattern")},
											{_L("With sheath around the support"),("support_material_with_sheath")},
											{_L("Pattern spacing"),("support_material_spacing")},
											{_L("Pattern angle"),("support_material_angle")},
											{_L("Closing radius"),("support_material_closing_radius")},
											{_L("Top interface layers"),("support_material_interface_layers")},
											{_L("Bottom interface layers"),("support_material_bottom_interface_layers")},
											{_L("Interface pattern"),("support_material_interface_pattern")},
											{_L("Interface pattern spacing"),("support_material_interface_spacing")},
											{_L("Interface loops"),("support_material_interface_contact_loops")},
											{_L("Support on build plate only"),("support_material_buildplate_only")},
											{_L("XY separation between an object and its support"),("support_material_xy_spacing")},
											{_L("Don't support bridges"),("dont_support_bridges")},
											{_L("Synchronize with object layers"),("support_material_synchronize_layers")},
											{_L("Maximum Branch Angle"),("support_tree_angle")},
											{_L("Preferred Branch Angle"),("support_tree_angle_slow")},
											{_L("Branch Diameter"),("support_tree_branch_diameter")},
											{_L("Branch Diameter Angle"),("support_tree_branch_diameter_angle")},
											{_L("Tip Diameter"),("support_tree_tip_diameter")},
											{_L("Branch Distance"),("support_tree_branch_distance")},
											{_L("Branch Density"),("support_tree_top_rate")},
	};
	m_speedMap = {
											   {_L("Speed for print moves perimeters"),("perimeter_speed")},
											   {_L("Small perimeters"),("small_perimeter_speed")},
											   {_L("Speed for print moves external perimeters"),("external_perimeter_speed")},
											   {_L("Speed for print moves infill"),("infill_speed")},
											   {_L("Speed for print moves solid infill"),("solid_infill_speed")},
											   {_L("Speed for print moves top solid infill"),("top_solid_infill_speed")},
											   {_L("Support material"),("support_material_speed")},
											   {_L("Support material interface"),("support_material_interface_speed")},
											   {_L("Speed for print moves bridges"),("bridge_speed")},
											   {_L("Gap fill"),("gap_fill_speed")},
											   {_L("Ironing"),("ironing_speed")},
											   {_L("Enable dynamic overhang speeds"),("enable_dynamic_overhang_speeds")},
											   {_L("speed for 0% overlap (bridge)"),("overhang_speed_0")},
											   {_L("speed for 25% overlap"),("overhang_speed_1")},
											   {_L("speed for 50% overlap"),("overhang_speed_2")},
											   {_L("speed for 75% overlap"),("overhang_speed_3")},
											   {_L("Speed for non-print moves travel"),("travel_speed")},
											   {_L("Z travel"),("travel_speed_z")},
											   {_L("First layer speed"),("first_layer_speed")},
											   {_L("Speed of object first layer over raft interface"),("first_layer_speed_over_raft")},
											   {_L("Acceleration control (advanced) external perimeters"),("external_perimeter_acceleration")},
											   {_L("Acceleration control (advanced) perimeters"),("perimeter_acceleration")},
											   {_L("Acceleration control (advanced) top solid infill"),("top_solid_infill_acceleration")},
											   {_L("Acceleration control (advanced) solid infill"),("solid_infill_acceleration")},
											   {_L("Acceleration control (advanced) infill"),("infill_acceleration")},
											   {_L("Acceleration control (advanced) bridges"),("bridge_acceleration")},
											   {_L("First layer"),("first_layer_acceleration")},
											   {_L("First object layer over raft interface"),("first_layer_acceleration_over_raft")},
											   {_L("Acceleration control (advanced) travel"),("travel_acceleration")},
											   {_L("Default"),("default_acceleration")},
											   {_L("Max print speed"),("max_print_speed")},
											   {_L("Max volumetric speed"),("max_volumetric_speed")},
											   {_L("Max volumetric slope positive"),("max_volumetric_extrusion_rate_slope_positive")},
											   {_L("Max volumetric slope negative"),("max_volumetric_extrusion_rate_slope_negative")},
	};
	m_meMap = {
											{_L("Perimeter extruder"),("perimeter_extruder")},
											{_L("Infill extruder"),("infill_extruder")},
											{_L("Solid infill extruder"),("solid_infill_extruder")},
											{_L("Support material/raft/skirt extruder"),("support_material_extruder")},
											{_L("Support material/raft interface extruder"),("support_material_interface_extruder")},
											{_L("Enable ooze prevention"),("ooze_prevention")},
											{_L("Temperature variation"),("standby_temperature_delta")},
											{_L("Enable wipe tower"),("wipe_tower")},
											{_L("Position X"),("wipe_tower_x")},
											{_L("Position Y"),("wipe_tower_y")},
											{_L("Width"),("wipe_tower_width")},
											{_L("Wipe tower rotation angle"),("wipe_tower_rotation_angle")},
											{_L("Wipe tower brim width"),("wipe_tower_brim_width")},
											{_L("Maximal bridging distance"),("wipe_tower_bridging")},
											{_L("Stabilization cone apex angle"),("wipe_tower_cone_angle")},
											{_L("Wipe tower purge lines spacing"),("wipe_tower_extra_spacing")},
											{_L("No sparse layers (EXPERIMENTAL)"),("wipe_tower_no_sparse_layers")},
											{_L("Prime all printing extruders"),("single_extruder_multi_material_priming")},
											{_L("Interface shells"),("interface_shells")},
											{ _L("Maximum width of a segmented region"),("mmu_segmented_region_max_width")},
	};
	m_advancedMap = {
												{_L("Default extrusion width"),("extrusion_width")},
												{_L("First layer"),("first_layer_extrusion_width")},
												{_L("Perimeters"),("perimeter_extrusion_width")},
												{_L("External perimeters"),("external_perimeter_extrusion_width")},
												{_L("Infill"),("infill_extrusion_width")},
												{_L("Solid infill"),("solid_infill_extrusion_width")},
												{_L("Top solid infill"),("top_infill_extrusion_width")},
												{_L("Support material"),("support_material_extrusion_width")},
												{_L("Infill/perimeters overlap"),("infill_overlap")},
												{_L("Bridge flow ratio"),("bridge_flow_ratio")},
												{_L("Slice gap closing radius"),("slice_closing_radius")},
												{_L("Slicing Mode"),("slicing_mode")},
												{_L("Slice resolution"),("resolution")},
												{_L("G-code resolution"),("gcode_resolution")},
												{_L("XY Size Compensation"),("xy_size_compensation")},
												{_L("Elephant foot compensation"),("elefant_foot_compensation")},
												{_L("Perimeter transitioning threshold angle"),("wall_transition_angle")},
												{_L("Perimeter transitioning filter margin"),("wall_transition_filter_deviation")},
												{_L("Perimeter transition length"),("wall_transition_length")},
												{_L("Perimeter distribution count"),("wall_distribution_count")},
												{_L("Minimum perimeter width"),("min_bead_width")},
												{_L("Minimum feature size"),("min_feature_size")},
	};
	m_fillPatternMap = {
		{ 0,        Slic3r::InfillPattern::ipRectilinear },
		{ 1,          Slic3r::InfillPattern::ipAlignedRectilinear },
		{ 2,     			Slic3r::InfillPattern::ipGrid },
		{ 3, 			Slic3r::InfillPattern::ipTriangles },
		{ 4,              Slic3r::InfillPattern::ipStars },
		{ 5,              Slic3r::InfillPattern::ipCubic },
		{ 6,               Slic3r::InfillPattern::ipLine },
		{ 7,         Slic3r::InfillPattern::ipConcentric },
		{ 8,          Slic3r::InfillPattern::ipHoneycomb },
		{ 9,        Slic3r::InfillPattern::ip3DHoneycomb },
		{ 10,            Slic3r::InfillPattern::ipGyroid },
		{ 11,       Slic3r::InfillPattern::ipHilbertCurve },
		{ 12,  Slic3r::InfillPattern::ipArchimedeanChords },
		{ 13,     Slic3r::InfillPattern::ipOctagramSpiral },
		{ 14,      Slic3r::InfillPattern::ipAdaptiveCubic },
		{ 15,       Slic3r::InfillPattern::ipSupportCubic },
		{ 16,          Slic3r::InfillPattern::ipLightning }
	};

	m_tabfillPatternMap = {
		{ 0,        Slic3r::InfillPattern::ipRectilinear },
		{ 1,          Slic3r::InfillPattern::ipMonotonic },
		{ 2,     			Slic3r::InfillPattern::ipMonotonicLines },
		{ 3, 			Slic3r::InfillPattern::ipAlignedRectilinear },
		{ 4,              Slic3r::InfillPattern::ipConcentric },
		{ 5,              Slic3r::InfillPattern::ipHilbertCurve },
		{ 6,               Slic3r::InfillPattern::ipArchimedeanChords },
		{ 7,         Slic3r::InfillPattern::ipOctagramSpiral },
	};


	m_StyleMap = {
		{0, Slic3r::SupportMaterialStyle::smsGrid},
		{1, Slic3r::SupportMaterialStyle::smsSnug},
		{2, Slic3r::SupportMaterialStyle::smsOrganic},
	};
	m_ReVerStyleMap = {
		{Slic3r::SupportMaterialStyle::smsGrid, 0},
		{Slic3r::SupportMaterialStyle::smsSnug, 1},
		{Slic3r::SupportMaterialStyle::smsOrganic, 2},
	};
}

void AnkerParameterPanel::getItemList(wxStringList& list, ControlListType listType)
{
	list.Clear();
	switch (listType)
	{
	case List_Seam_Position:
	{		
		list.Add(_L("Random"));
		list.Add(_L("Nearest"));
		list.Add(_L("Aligned"));
		list.Add(_L("Rear"));
	}
		break;
	case List_Perimeter_generator:
	{
		list.Add(_L("Classic"));
		list.Add(_L("Arachne"));
	}
		break;
	case List_Fuzzy_skin:
	{
		list.Add(_L("None"));
		list.Add(_L("Outside walls"));
		list.Add(_L("All walls"));
	}
		break;
	case List_Fill_density:
	{
		list.Add(_L("0%"));
		list.Add(_L("5%"));
		list.Add(_L("10%"));
		list.Add(_L("15%"));
		list.Add(_L("20%"));
		list.Add(_L("25%"));
		list.Add(_L("30%"));
		list.Add(_L("40%"));
		list.Add(_L("50%"));
		list.Add(_L("60%"));
		list.Add(_L("70%"));
		list.Add(_L("80%"));
		list.Add(_L("90%"));
		list.Add(_L("100%"));
	}
		break;
	case List_Fill_pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Aligned Rectilinear"));
		list.Add(_L("Grid"));
		list.Add(_L("Triangles"));
		list.Add(_L("Stars"));
		list.Add(_L("Cubic"));
		list.Add(_L("Line"));
		list.Add(_L("Concentric"));
		list.Add(_L("Honeycomb"));
		list.Add(_L("3D Honeycomb"));
		list.Add(_L("Gyroid"));
		list.Add(_L("Hilbert Curve"));
		list.Add(_L("Archimedean Chords"));
		list.Add(_L("Octagram Spiral"));
		list.Add(_L("Adaptive Cubic"));
		list.Add(_L("Support Cubic"));
		list.Add(_L("Lightning"));
	}
		break;
	case List_Length_of_th_infill_anchor:
	{
		list.Add(_L("0 (no open anchors)"));
		list.Add(_L("1 mm"));
		list.Add(_L("2 mm"));
		list.Add(_L("5 mm"));
		list.Add(_L("10 mm"));
		list.Add(_L("1000 mm"));
	}
		break;
	case List_Maximum_length_of_the_infill_anchor:
	{
		list.Add(_L("0 (not anchored)"));
		list.Add(_L("1 mm"));
		list.Add(_L("2 mm"));
		list.Add(_L("5 mm"));
		list.Add(_L("10 mm"));
		list.Add(_L("1000 mm"));
	}
		break;
	case List_Top_fill_pattern:
	case List_Bottom_fill_pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Monotonic"));
		list.Add(_L("Monotonic Lines"));
		list.Add(_L("Aligned Rectilinear"));
		list.Add(_L("Concentric"));
		list.Add(_L("Hilbert Curve"));
		list.Add(_L("Archimedean Chords"));
		list.Add(_L("Octagram Spiral"));		
	}
		break;		
	case List_Ironing_Type:
	{
		list.Add(_L("All top surfaces"));
		list.Add(_L("Topmost surface only"));
		list.Add(_L("All solid surfaces"));
	}
		break;
	case List_Draft_shield:
	{
		list.Add(_L("Disabled"));
		list.Add(_L("Limited"));
		list.Add(_L("Enabled"));
	}
		break;
	case List_Brim_type:
	{
		list.Add(_L("No brim"));
		list.Add(_L("Outer brim only"));		
		list.Add(_L("Inner brim only"));
		list.Add(_L("Outer and inner brim"));
	}
		break;
	case List_Style:
	{
		list.Add(_L("Grid"));
		list.Add(_L("Snug"));
		list.Add(_L("Organic"));
	}
		break;
	case List_Top_contact_Z_distance:
	{
		list.Add(_L("0 (soluble)"));
		list.Add(_L("0.1 (detachable)"));
		list.Add(_L("0.2 (detachable)"));
	}
		break;
	case List_Bottom_contact_Z_distance:
	{
		list.Add(_L("Same as top"));
		list.Add(_L("0.1"));
		list.Add(_L("0.2"));
	}
		break;
	case List_Pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Rectilinear grid"));
		list.Add(_L("Honevcomb"));
	}
		break;
	case List_Top_interface_layers:
	{
		list.Add(_L("0 (off)"));
		list.Add(_L("1 (light)"));
		list.Add(_L("2 (default)"));
		list.Add(_L("3 (heavy)"));
	}
		break;
	case List_Bottom_interface_layers:
	{
		list.Add(_L("Same as top"));
		list.Add(_L("0 (off)"));
		list.Add(_L("1 (light)"));
		list.Add(_L("2 (default)"));
		list.Add(_L("3 (heavy)"));
	}
		break;
	case List_interface_pattern:
	{
		list.Add(_L("Default"));
		list.Add(_L("Rectilinear"));
		list.Add(_L("Concentric"));		
	}
		break;
	case List_Slicing_Mode:
	{
		list.Add(_L("Regular"));
		list.Add(_L("Even-odd"));
		list.Add(_L("Close holes"));		
	}
		break;
	default:
		break;
	}
}


void AnkerParameterPanel::onBtnClicked(wxCommandEvent& event)
{
	this->Freeze();
	AnkerChooseBtn* senderObjt = dynamic_cast<AnkerChooseBtn*>(event.GetEventObject());

	auto iter = m_tabBtnVector.begin();
	while (iter != m_tabBtnVector.end())	
	{
		if (senderObjt != *iter)
			(*iter)->setBtnStatus(ChooseBtn_Normal);

		++iter;
	}
	
	if(senderObjt)
		showCurrentWidget(senderObjt->getText());

	Refresh();
	this->Thaw();
}


void AnkerParameterPanel::showCurrentWidget(const wxString& tabName)
{
	auto iter = m_windowTabMap.begin();

	while (iter != m_windowTabMap.end())
	{
		if (iter->first == tabName)
		{
			auto list = iter->second;

			auto listIter = list.begin();
			while (listIter != list.end())
			{
				(*listIter)->Show();
				++listIter;
			}
		}
		else
		{
			auto list = iter->second;

			auto listIter = list.begin();
			while (listIter != list.end())
			{
				(*listIter)->Hide();
				++listIter;
			}
		}
		++iter;
	}
	Update();
// 	Refresh();
 	Layout();
}


void AnkerParameterPanel::getSearResMap(std::map<wxString, std::vector<wxString>>& searchResMap, const wxString& searchData)
{
	if (searchData.IsEmpty())
		return;

	searchResMap.clear();
	wxString searchDataEx = searchData;
	searchDataEx.MakeCapitalized();

	auto iter = m_gSearchMap.begin();
	while (iter != m_gSearchMap.end())//tab-vector map
	{
		auto iterEx = iter->second.begin();//vector
		std::vector<wxString> searchKeyVector;
		
		while (iterEx != iter->second.end())//vector iterator
		{
			wxString iterValue = (*iterEx);

			iterValue.MakeLower();
			if(iterValue.Contains(searchData))
				searchKeyVector.push_back((*iterEx));

			++iterEx;
		}
		if (searchKeyVector.size() > 0)
			searchResMap.insert(std::make_pair(iter->first, searchKeyVector));

		++iter;
	}
	//if first capital letters

}

void AnkerParameterPanel::showEasyModel()
{
	this->Freeze();
	ANKER_LOG_INFO << "show easy model";
	//get easy data
	updateEasyWidget();

	m_pEasyWidget->Show();

	m_pPresetParameterComBox->Hide();
	m_pResetBtn->Hide();
	m_pSaveAllBtn->Hide();
	m_pDividingLine->Hide();
	m_pTabBtnScrolledWindow->Hide();
	m_pTabItemScrolledWindow->Hide();

	//m_pHandleParameterLabel->Hide();
	//m_pHandleParameterComBox->Hide();
	m_pSearchEdit->Hide();
	//m_pCfgBtn->Hide();	
	//add by alves hide spacing
	m_pMainVSizer->Show(6, false);
	m_pMainVSizer->Show(8, false);
	m_pMainVSizer->Show(10, false);

	Refresh();
	Layout();
	this->Thaw();
}


void AnkerParameterPanel::showExpertModel()
{
	this->Freeze();
	ANKER_LOG_INFO << "show expert model";
	m_pEasyWidget->Hide();
		
	m_pPresetParameterComBox->Show();	
	m_pResetBtn->Show();
	m_pSaveAllBtn->Show();
	m_pDividingLine->Show();
	m_pTabBtnScrolledWindow->Show();
	m_pTabItemScrolledWindow->Show();

	m_pSearchEdit->Show();

	//m_pHandleParameterLabel->Show();
	//m_pHandleParameterComBox->Show();
	//m_pCfgBtn->Show();	

	//add by alves show spacing
	m_pMainVSizer->Show(6, true);
	m_pMainVSizer->Show(8, true);
	m_pMainVSizer->Show(10, true);

	Refresh();
	Layout();
	this->Thaw();
}


void AnkerParameterPanel::switchToLaebl(const wxString& strTab, const wxString& label)
{
	ANKER_LOG_INFO << "switch label is " << strTab.c_str();

	auto iter = m_tabBtnVector.begin();
	while (iter != m_tabBtnVector.end())
	{
		if (strTab == (*iter)->getText())
		{
			(*iter)->setBtnStatus(ChooseBtn_Select);
			wxRect rect = (*iter)->GetRect();
			int x = 0, y = 0;
			m_pTabBtnScrolledWindow->GetViewStart(&x, &y);
			int xPos = rect.x - x;
			int yPos = rect.y - y;
			m_pTabBtnScrolledWindow->Scroll(xPos, yPos);			
			showCurrentWidget(strTab);


			auto item = m_windowTabMap.find(strTab.ToStdString());
			if (item != m_windowTabMap.end())
			{

				auto itemList = item->second;

				auto itemIter = itemList.begin();
				while (itemIter != itemList.end())
				{
					if ((*itemIter)->isExistLabel(label))
					{
						(*itemIter)->showLabelHighlight(label,wxColour("#62D361"));

						if (strTab != _L("Skirt and brim"))
						{
							wxRect rect = (*itemIter)->GetRect();
							int x = 0, y = 0;
							m_pTabItemScrolledWindow->GetViewStart(&x, &y);
							int xPos = rect.x - x;
							int yPos = rect.y - y;
							m_pTabItemScrolledWindow->Scroll(xPos, yPos);
						}
						break;
					}
					++itemIter;
				}

			}
			
		}
		else
		{
			(*iter)->setBtnStatus(ChooseBtn_Normal);
		}
		++iter;
	}
}


void AnkerParameterPanel::initUiData()
{
	hideAllResetBtn();
	initLapData();	
	initInfillData();
	initSabData();	
	initSmData();	
	initSpeedData();	
	initMeData();	
	initAdvancedData();	
}


void AnkerParameterPanel::initLapData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();

	{
		auto layer_height_value = printConfig.opt_float("layer_height");
		setItemValue(_L("Layers and perimeters"), _L("Layer height"), layer_height_value);
	
		auto first_layer_height_value = printConfig.get_abs_value("first_layer_height");		
		setItemValue(_L("Layers and perimeters"), _L("First layer height"), first_layer_height_value);
	}

	{
		auto perimeters_value = printConfig.opt_int("perimeters");
		setItemValue(_L("Layers and perimeters"), _L("Perimeters"), perimeters_value);
		auto spiral_vase_value = printConfig.opt_bool("spiral_vase");
		setItemValue(_L("Layers and perimeters"), _L("Spiral vase"), spiral_vase_value);
	}

	{
		auto top_solid_layers_value = printConfig.opt_int("top_solid_layers");
		setItemValue(_L("Layers and perimeters"), _L("Top solid layers"), top_solid_layers_value);
		auto bottom_solid_layers_value = printConfig.opt_int("bottom_solid_layers");
		setItemValue(_L("Layers and perimeters"), _L("Bottom solid layers"), bottom_solid_layers_value);
		

		auto top_solid_min_thickness_value = printConfig.opt_float("top_solid_min_thickness");
		setItemValue(_L("Layers and perimeters"), _L("Top minimum shell thickness"), top_solid_min_thickness_value);
		auto bottom_solid_min_thickness_value = printConfig.opt_float("bottom_solid_min_thickness");
		setItemValue(_L("Layers and perimeters"), _L("Bottom minimum shell thickness"), bottom_solid_min_thickness_value);
	}

	{
		auto extra_perimeters_value = printConfig.opt_bool("extra_perimeters");
		setItemValue(_L("Layers and perimeters"), _L("Extra perimeters if needed"), extra_perimeters_value);
		auto extra_perimeters_on_overhangs_value = printConfig.opt_bool("extra_perimeters_on_overhangs");
		setItemValue(_L("Layers and perimeters"), _L("Extra perimeters on overhangs(Experimental)"), extra_perimeters_on_overhangs_value);
		auto avoid_crossing_curled_overhangs_value = printConfig.opt_bool("avoid_crossing_curled_overhangs");	
		setItemValue(_L("Layers and perimeters"), _L("Avoid crossing curled overhangs(Experimental)"), avoid_crossing_curled_overhangs_value);
		auto avoid_crossing_perimeters_value = printConfig.opt_bool("avoid_crossing_perimeters");	
		setItemValue(_L("Layers and perimeters"), _L("Avoid crossing perimeters"), avoid_crossing_perimeters_value);

		auto avoid_crossing_perimeters_max_detour_value = printConfig.opt_serialize("avoid_crossing_perimeters_max_detour");
		wxString wxavoid_crossing_perimeters_max_detour_value = avoid_crossing_perimeters_max_detour_value;
		size_t pos = wxavoid_crossing_perimeters_max_detour_value.find('%');
		if (pos != wxString::npos) {
			wxavoid_crossing_perimeters_max_detour_value.erase(pos, 1);
		}
		setItemValue(_L("Layers and perimeters"), _L("Avoid crossing perimeters - Max detour length"), wxavoid_crossing_perimeters_max_detour_value);
		auto thin_walls_value = printConfig.opt_bool("thin_walls");
		setItemValue(_L("Layers and perimeters"), _L("Detect thin walls"), thin_walls_value);
		auto thick_bridges_value = printConfig.opt_bool("thick_bridges");
		setItemValue(_L("Layers and perimeters"), _L("Thick bridges"), thick_bridges_value);
		auto overhangs_value = printConfig.opt_bool("overhangs");
		setItemValue(_L("Layers and perimeters"), _L("Detect bridging perimeters"), overhangs_value);
	}
	{
		auto seam_position_value = printConfig.opt_enum<Slic3r::SeamPosition>("seam_position");
		setItemValue(_L("Layers and perimeters"), _L("Seam position"), (int)seam_position_value);
		auto staggered_inner_seams_value = printConfig.opt_bool("staggered_inner_seams");
		setItemValue(_L("Layers and perimeters"), _L("Staggered inner seams"), staggered_inner_seams_value);
		auto external_perimeters_first_value = printConfig.opt_bool("external_perimeters_first");		
		setItemValue(_L("Layers and perimeters"), _L("External perimeters first"), external_perimeters_first_value);
		auto gap_fill_enabled_value = printConfig.opt_bool("gap_fill_enabled");
		setItemValue(_L("Layers and perimeters"), _L("Fill gaps"), gap_fill_enabled_value);
		auto perimeter_generator_value = printConfig.opt_enum<Slic3r::PerimeterGeneratorType>("perimeter_generator");
		setItemValue(_L("Layers and perimeters"), _L("Perimeter generator"), (int)perimeter_generator_value);
	}

	{
		auto fuzzy_skin_value = printConfig.opt_enum<Slic3r::FuzzySkinType>("fuzzy_skin");
		setItemValue(_L("Layers and perimeters"), _L("Fuzzy Skin"), (int)fuzzy_skin_value);
		auto fuzzy_skin_thickness_value = printConfig.opt_float("fuzzy_skin_thickness");
		setItemValue(_L("Layers and perimeters"), _L("Fuzzy skin thickness"), fuzzy_skin_thickness_value);
		auto fuzzy_skin_point_dist_value = printConfig.opt_float("fuzzy_skin_point_dist");
		setItemValue(_L("Layers and perimeters"), _L("Fuzzy skin point distance"), fuzzy_skin_point_dist_value);
	}
	
}


void AnkerParameterPanel::initInfillData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();
	{
		
		auto fill_density_value = printConfig.opt_serialize("fill_density");	
		setItemValue(_L("Infill"), _L("Fill density"), fill_density_value);

		//auto fill_pattern_value = printConfig.opt_enum<Slic3r::InfillPattern>("fill_pattern");	
		auto fill_pattern_value = printConfig.opt_serialize("fill_pattern");	
				
		std::locale loc;
		fill_pattern_value[0] = std::toupper(fill_pattern_value[0], loc);

		auto realFillPatternValue = m_fillPatternData[fill_pattern_value];
		setItemValue(_L("Infill"), _L("Fill pattern"), realFillPatternValue);

		auto infill_anchor_value = printConfig.opt_serialize("infill_anchor");

		wxString dataValue = infill_anchor_value;
		{
			if (infill_anchor_value == "0")
				dataValue = _L("0 (no open anchors)");
			else
			{
				wxString tempdata = wxString::Format(wxT("%f"), infill_anchor_value);

				std::ostringstream out;
				out << std::fixed << std::setprecision(2) << infill_anchor_value;
				std::string str = out.str();

				str = str.substr(0, str.find_last_not_of('0') + 1);
				if (str.back() == '.') {
					str.pop_back();
				}
				wxVariant checkData = infill_anchor_value;
				wxString resType = checkData.GetType();
				if(checkData.GetType() == wxT("double"))
					dataValue = str;
			}
		}
		
		setItemValue(_L("Infill"), _L("Length of the infill anchor"), dataValue);

		//auto infill_anchor_max_value = printConfig.get_abs_value("infill_anchor_max");
		auto infill_anchor_max_value = printConfig.opt_serialize("infill_anchor_max");
		wxString wxInfill_anchor_max_value = infill_anchor_max_value;

		setItemValue(_L("Infill"), _L("Maximum length of the infill anchor"), wxInfill_anchor_max_value);
		//auto top_fill_pattern_value = printConfig.opt_enum<Slic3r::InfillPattern>("top_fill_pattern");
		std::string top_fill_pattern_value = printConfig.opt_serialize("top_fill_pattern");
		std::locale topFillPatternLoc;
		top_fill_pattern_value[0] = std::toupper(top_fill_pattern_value[0], topFillPatternLoc);
		top_fill_pattern_value = m_fillPatternData[top_fill_pattern_value].ToStdString();
		setItemValue(_L("Infill"), _L("Top fill pattern"), top_fill_pattern_value);

		//auto bottom_fill_pattern_value = printConfig.opt_enum<Slic3r::InfillPattern>("bottom_fill_pattern");
		std::string bottom_fill_pattern_value = printConfig.opt_serialize("bottom_fill_pattern");
		std::locale fillPatternLoc;
		bottom_fill_pattern_value[0] = std::toupper(bottom_fill_pattern_value[0], fillPatternLoc);
		bottom_fill_pattern_value = m_fillPatternData[bottom_fill_pattern_value].ToStdString();

		setItemValue(_L("Infill"), _L("Bottom fill pattern"), bottom_fill_pattern_value);
	}

	{
		auto ironing_value = printConfig.opt_bool("ironing");
		setItemValue(_L("Infill"), _L("Enable ironing"), ironing_value);
		auto ironing_type_value = printConfig.opt_enum<Slic3r::IroningType>("ironing_type");
		setItemValue(_L("Infill"), _L("Ironing Type"), (int)ironing_type_value);
		auto ironing_flowrate_value = printConfig.opt_serialize("ironing_flowrate");
		wxString wxIroning_flowrate_value = ironing_flowrate_value;
		size_t pos = wxIroning_flowrate_value.find('%');
		if (pos != wxString::npos) {			
			wxIroning_flowrate_value.erase(pos, 1);
		}

		setItemValue(_L("Infill"), _L("Flow rate"), wxIroning_flowrate_value);
		auto ironing_spacing_value = printConfig.opt_float("ironing_spacing");
		setItemValue(_L("Infill"), _L("Spacing between ironing passes"), ironing_spacing_value);
	}

	{
		auto infill_every_layers_value = printConfig.opt_int("infill_every_layers");
		setItemValue(_L("Infill"), _L("Combine infill every"), infill_every_layers_value);
	}

	{
		auto solid_infill_every_layers_value = printConfig.opt_int("solid_infill_every_layers");
		setItemValue(_L("Infill"), _L("Solid infill every"), solid_infill_every_layers_value);

		auto fill_angle_value = printConfig.opt_float("fill_angle");
		setItemValue(_L("Infill"), _L("Fill angle"), fill_angle_value);

		auto solid_infill_below_area_value = printConfig.opt_float("solid_infill_below_area");
		setItemValue(_L("Infill"), _L("Solid infill threshold area"), solid_infill_below_area_value);

		auto bridge_angle_value = printConfig.opt_float("bridge_angle");
		setItemValue(_L("Infill"), _L("Bridging angle"), bridge_angle_value);

		auto only_retract_when_crossing_perimeters_value = printConfig.opt_bool("only_retract_when_crossing_perimeters");
		setItemValue(_L("Infill"), _L("Only retract when crossing perimeters"), only_retract_when_crossing_perimeters_value);

		auto infill_first_value = printConfig.opt_bool("infill_first");
		setItemValue(_L("Infill"), _L("Infill before perimeters"), infill_first_value);
	}
}


void AnkerParameterPanel::initSabData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;	

	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();

	{
		auto skirts_value = printConfig.opt_int("skirts");
		setItemValue(_L("Skirt and brim"), _L("Loops (minimum)"), skirts_value);
		auto skirt_distance_value = printConfig.opt_float("skirt_distance");
		setItemValue(_L("Skirt and brim"), _L("Distance from brim/object"), skirt_distance_value);
		auto skirt_height_value = printConfig.opt_int("skirt_height");
		setItemValue(_L("Skirt and brim"), _L("Skirt height"), skirt_height_value);
		auto draft_shield_value = printConfig.opt_enum<Slic3r::DraftShield>("draft_shield");
		setItemValue(_L("Skirt and brim"), _L("Draft shield"), (int)draft_shield_value);
		auto min_skirt_length_value = printConfig.opt_float("min_skirt_length");
		setItemValue(_L("Skirt and brim"), _L("Minimal filament extrusion length"), min_skirt_length_value);
	}

	{
		auto brim_type_value = printConfig.opt_enum <Slic3r::BrimType>("brim_type");
		setItemValue(_L("Skirt and brim"), _L("Brim type"), (int)brim_type_value);
		auto brim_width_value = printConfig.opt_float("brim_width");
		setItemValue(_L("Skirt and brim"), _L("Brim width"), brim_width_value);
		auto brim_separation_value = printConfig.opt_float("brim_separation");
		setItemValue(_L("Skirt and brim"), _L("Brim separation gap"), brim_separation_value);
	}
	
}


void AnkerParameterPanel::initSmData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	
	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();

	{
		auto support_material_value = printConfig.opt_bool("support_material");
		setItemValue(_L("Support material"), _L("Generate support material"), support_material_value);
		auto support_material_auto_value = printConfig.opt_bool("support_material_auto");
		setItemValue(_L("Support material"), _L("Auto generated supports"), support_material_auto_value);
		auto support_material_threshold_value = printConfig.opt_int("support_material_threshold");
		setItemValue(_L("Support material"), _L("Overhang threshold"), support_material_threshold_value);
		auto support_material_enforce_layerse_value = printConfig.opt_int("support_material_enforce_layers");
		setItemValue(_L("Support material"), _L("Enforce support for the first"), support_material_enforce_layerse_value);
		auto raft_first_layer_density_value = printConfig.opt_serialize("raft_first_layer_density");
		wxString wxRaft_first_layer_density_value = raft_first_layer_density_value;
		size_t pos = wxRaft_first_layer_density_value.find('%');
		if (pos != wxString::npos) {
			wxRaft_first_layer_density_value.erase(pos, 1);
		}
		setItemValue(_L("Support material"), _L("First layer density"), wxRaft_first_layer_density_value);
		auto raft_first_layer_expansion_value = printConfig.opt_float("raft_first_layer_expansion");
		setItemValue(_L("Support material"), _L("First layer expansion"), raft_first_layer_expansion_value);
	}

	{
		auto raft_layers_value = printConfig.opt_int("raft_layers");
		setItemValue(_L("Support material"), _L("Raft layers"), raft_layers_value);
		auto raft_contact_distance_value = printConfig.opt_float("raft_contact_distance");
		setItemValue(_L("Support material"), _L("Raft contact Z distance"), raft_contact_distance_value);
		auto raft_expansion_value = printConfig.opt_float("raft_expansion");
		setItemValue(_L("Support material"), _L("Raft expansion"), raft_expansion_value);
	}
	
	{
		Slic3r::SupportMaterialStyle support_material_style_value = printConfig.opt_enum<Slic3r::SupportMaterialStyle>("support_material_style");
		int index = m_ReVerStyleMap[support_material_style_value];
		setItemValue(_L("Support material"), _L("Style"), index);
		auto support_material_contact_distance_value = printConfig.opt_serialize("support_material_contact_distance");
		wxString wxSupport_material_contact_distance_value = support_material_contact_distance_value;

		{
			if (support_material_contact_distance_value == "0")
				wxSupport_material_contact_distance_value = _L("0 (soluble)");
			else
			{
				wxString tempdata = wxString::Format(wxT("%f"), support_material_contact_distance_value);

				std::ostringstream out;
				out << std::fixed << std::setprecision(2) << support_material_contact_distance_value;
				std::string str = out.str();

				str = str.substr(0, str.find_last_not_of('0') + 1);
				if (str.back() == '.') {
					str.pop_back();
				}

				wxSupport_material_contact_distance_value = str;
			}
		}
		setItemValue(_L("Support material"), _L("Top contact Z distance"), wxSupport_material_contact_distance_value);

		auto support_material_bottom_contact_distance_value = printConfig.opt_serialize("support_material_bottom_contact_distance");		
		wxString wxSupport_material_bottom_contact_distance_value = support_material_bottom_contact_distance_value;

		{
			if (support_material_bottom_contact_distance_value == "0")
				wxSupport_material_bottom_contact_distance_value = _L("Same as top");
			else
			{
				wxString tempdata = wxString::Format(wxT("%f"), support_material_bottom_contact_distance_value);

				std::ostringstream out;
				out << std::fixed << std::setprecision(2) << support_material_bottom_contact_distance_value;
				std::string str = out.str();

				str = str.substr(0, str.find_last_not_of('0') + 1);
				if (str.back() == '.') {
					str.pop_back();
				}

				wxSupport_material_bottom_contact_distance_value = str;
			}
		}

		setItemValue(_L("Support material"), _L("Bottom contact Z distance"), wxSupport_material_bottom_contact_distance_value);

		auto support_material_pattern_value = printConfig.opt_enum<Slic3r::SupportMaterialPattern>("support_material_pattern");
		setItemValue(_L("Support material"), _L("Pattern"), (int)support_material_pattern_value);
		auto support_material_with_sheath_value = printConfig.opt_bool("support_material_with_sheath");
		setItemValue(_L("Support material"), _L("With sheath around the support"), support_material_with_sheath_value);
		auto support_material_spacing_value = printConfig.opt_float("support_material_spacing");
		setItemValue(_L("Support material"), _L("Pattern spacing"), support_material_spacing_value);
		auto support_material_angle_value = printConfig.opt_float("support_material_angle");
		setItemValue(_L("Support material"), _L("Pattern angle"), support_material_angle_value);
		auto support_material_closing_radius_value = printConfig.opt_float("support_material_closing_radius");
		setItemValue(_L("Support material"), _L("Closing radius"), support_material_closing_radius_value);
		auto support_material_interface_layers_value = printConfig.opt_int("support_material_interface_layers");
		setItemValue(_L("Support material"), _L("Top interface layers"), support_material_interface_layers_value);
		auto support_material_bottom_interface_layers_value = printConfig.opt_serialize("support_material_bottom_interface_layers");

		wxString support_material_bottom_interface_layers_valueEx = support_material_bottom_interface_layers_value;
	
		{
			if (support_material_bottom_interface_layers_valueEx == "0")
				wxSupport_material_bottom_contact_distance_value = _L("0 (off)");
			else if (support_material_bottom_interface_layers_valueEx == "-1")
			{
				support_material_bottom_interface_layers_valueEx = _L("Same as top");
			}

			else
			{
				wxString tempdata = wxString::Format(wxT("%f"), support_material_bottom_interface_layers_value);

				std::ostringstream out;
				out << std::fixed << std::setprecision(2) << support_material_bottom_interface_layers_value;
				std::string str = out.str();

				str = str.substr(0, str.find_last_not_of('0') + 1);
				if (str.back() == '.') {
					str.pop_back();
				}

				support_material_bottom_interface_layers_valueEx = str;
			}

		}

		setItemValue(_L("Support material"), _L("Bottom interface layers"), support_material_bottom_interface_layers_valueEx);
		auto support_material_interface_pattern_value = printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>("support_material_interface_pattern");
		setItemValue(_L("Support material"), _L("Interface pattern"), (int)support_material_interface_pattern_value);
		auto support_material_interface_spacing_value = printConfig.opt_float("support_material_interface_spacing");
		setItemValue(_L("Support material"), _L("Interface pattern spacing"), support_material_interface_spacing_value);
		auto support_material_interface_contact_loops_value = printConfig.opt_bool("support_material_interface_contact_loops");
		setItemValue(_L("Support material"), _L("Interface loops"), support_material_interface_contact_loops_value);
		
		auto support_buildplate_only_value = printConfig.opt_bool("support_material_buildplate_only");		
		setItemValue(_L("Support material"), _L("Support on build plate only"), support_buildplate_only_value);
		auto support_material_xy_spacing_value = printConfig.opt_serialize("support_material_xy_spacing");
		setItemValue(_L("Support material"), _L("XY separation between an object and its support"), support_material_xy_spacing_value);
		auto dont_support_bridges_value = printConfig.opt_bool("dont_support_bridges");
		setItemValue(_L("Support material"), _L("Don't support bridges"), dont_support_bridges_value);
		auto support_material_synchronize_layers_value = printConfig.opt_bool("support_material_synchronize_layers");
		setItemValue(_L("Support material"), _L("Synchronize with object layers"), support_material_synchronize_layers_value);
	}

	{		
		auto support_tree_angle_value = printConfig.opt_float("support_tree_angle");
		setItemValue(_L("Support material"), _L("Maximum Branch Angle"), support_tree_angle_value);
		auto support_tree_angle_slow_value = printConfig.opt_float("support_tree_angle_slow");
		setItemValue(_L("Support material"), _L("Preferred Branch Angle"), support_tree_angle_slow_value);

		auto support_tree_branch_diameter_value = printConfig.opt_float("support_tree_branch_diameter");
		setItemValue(_L("Support material"), _L("Branch Diameter"), support_tree_branch_diameter_value);

		auto support_tree_branch_diameter_angle_value = printConfig.opt_float("support_tree_branch_diameter_angle");
		setItemValue(_L("Support material"), _L("Branch Diameter Angle"), support_tree_branch_diameter_angle_value);

		auto support_tree_tip_diameter_value = printConfig.opt_float("support_tree_tip_diameter");
		setItemValue(_L("Support material"), _L("Tip Diameter"), support_tree_tip_diameter_value);

		auto support_tree_branch_distance_value = printConfig.opt_float("support_tree_branch_distance");
		setItemValue(_L("Support material"), _L("Branch Distance"), support_tree_branch_distance_value);
		auto support_tree_top_rate_value = printConfig.opt_serialize("support_tree_top_rate"); 
		wxString wxSupport_tree_top_rate_value = support_tree_top_rate_value;
		size_t pos = wxSupport_tree_top_rate_value.find('%');
		if (pos != wxString::npos) {
			wxSupport_tree_top_rate_value.erase(pos, 1);
		}
		setItemValue(_L("Support material"), _L("Branch Density"), wxSupport_tree_top_rate_value);
	}
}


void AnkerParameterPanel::initSpeedData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;	

	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();
	auto perimeter_speed_value = printConfig.opt_float("perimeter_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves perimeters"), perimeter_speed_value);	
	auto small_perimeter_speed_value = printConfig.opt_serialize("small_perimeter_speed");
	setItemValue(_L("Speed"), _L("Small perimeters"), small_perimeter_speed_value);	
	auto external_perimeter_speed_value = printConfig.opt_serialize("external_perimeter_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves external perimeters"), external_perimeter_speed_value);
	auto infill_speed_value = printConfig.opt_float("infill_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves infill"), infill_speed_value);

	auto solid_infill_speed_value = printConfig.opt_serialize("solid_infill_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves solid infill"), solid_infill_speed_value);
	auto top_solid_infill_speed_value = printConfig.opt_serialize("top_solid_infill_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves top solid infill"), top_solid_infill_speed_value);
	auto support_material_speed_value = printConfig.opt_float("support_material_speed");
	setItemValue(_L("Speed"), _L("Support material"), support_material_speed_value);
	auto support_material_interface_speed_value = printConfig.opt_serialize("support_material_interface_speed");
 	wxString wxSupport_material_interface_speed_value = support_material_interface_speed_value;//mm/s or %

	setItemValue(_L("Speed"), _L("Support material interface"), wxSupport_material_interface_speed_value);
	auto bridge_speed_value = printConfig.opt_float("bridge_speed");
	setItemValue(_L("Speed"), _L("Speed for print moves bridges"), bridge_speed_value);
	auto gap_fill_speed_value = printConfig.opt_float("gap_fill_speed");
	setItemValue(_L("Speed"), _L("Gap fill"), gap_fill_speed_value);
	auto ironing_speed_value = printConfig.opt_float("ironing_speed");
	setItemValue(_L("Speed"), _L("Ironing"), ironing_speed_value);

	auto enable_dynamic_overhang_speeds_value = printConfig.opt_bool("enable_dynamic_overhang_speeds");
	setItemValue(_L("Speed"), _L("Enable dynamic overhang speeds"), enable_dynamic_overhang_speeds_value);
	auto overhang_speed_0_value = printConfig.opt_serialize("overhang_speed_0");
	setItemValue(_L("Speed"), _L("speed for 0% overlap (bridge)"), overhang_speed_0_value);
	auto overhang_speed_1_value = printConfig.opt_serialize("overhang_speed_1");
	setItemValue(_L("Speed"), _L("speed for 25% overlap"), overhang_speed_1_value);
	auto overhang_speed_2_value = printConfig.opt_serialize("overhang_speed_2");
	setItemValue(_L("Speed"), _L("speed for 50% overlap"), overhang_speed_2_value);
	auto overhang_speed_3_value = printConfig.opt_serialize("overhang_speed_3");
	setItemValue(_L("Speed"), _L("speed for 75% overlap"), overhang_speed_3_value);
 
	auto travel_speed_value = printConfig.opt_float("travel_speed");
	setItemValue(_L("Speed"), _L("Speed for non-print moves travel"), travel_speed_value);
	auto travel_speed_z_value = printConfig.opt_float("travel_speed_z");	
	setItemValue(_L("Speed"), _L("Z travel"), travel_speed_z_value);

	auto first_layer_speed_value = printConfig.opt_serialize("first_layer_speed");
	wxString wxFirst_layer_speed_value = first_layer_speed_value;// mm/s or %

	setItemValue(_L("Speed"), _L("First layer speed"), wxFirst_layer_speed_value);
	auto first_layer_speed_over_raft_value = printConfig.opt_serialize("first_layer_speed_over_raft");
	setItemValue(_L("Speed"), _L("Speed of object first layer over raft interface"), first_layer_speed_over_raft_value);

	auto external_perimeter_acceleration_value = printConfig.opt_float("external_perimeter_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) external perimeters"), external_perimeter_acceleration_value);
	auto perimeter_acceleration_value = printConfig.opt_float("perimeter_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) perimeters"), perimeter_acceleration_value);
	auto top_solid_infill_acceleration_value = printConfig.opt_float("top_solid_infill_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) top solid infill"), top_solid_infill_acceleration_value);
	auto solid_infill_acceleration_value = printConfig.opt_float("solid_infill_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) solid infill"), solid_infill_acceleration_value);
	 
	auto infill_acceleration_value = printConfig.opt_float("infill_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) infill"), infill_acceleration_value);
	auto bridge_acceleration_value = printConfig.opt_float("bridge_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) bridges"), bridge_acceleration_value);
	auto first_layer_acceleration_value = printConfig.opt_float("first_layer_acceleration");
	setItemValue(_L("Speed"), _L("First layer"), first_layer_acceleration_value);
	auto first_layer_acceleration_over_raft_value = printConfig.opt_float("first_layer_acceleration_over_raft");
	setItemValue(_L("Speed"), _L("First object layer over raft interface"), first_layer_acceleration_over_raft_value);
	auto travel_acceleration_value = printConfig.opt_float("travel_acceleration");
	setItemValue(_L("Speed"), _L("Acceleration control (advanced) travel"), travel_acceleration_value);
	auto default_acceleration_value = printConfig.opt_float("default_acceleration");
	setItemValue(_L("Speed"), _L("Default"), default_acceleration_value);

	auto max_print_speed_value = printConfig.opt_float("max_print_speed");
	setItemValue(_L("Speed"), _L("Max print speed"), max_print_speed_value);
	auto max_volumetric_speed_value = printConfig.opt_float("max_volumetric_speed");
	setItemValue(_L("Speed"), _L("Max volumetric speed"), max_volumetric_speed_value);

	auto max_volumetric_extrusion_rate_slope_positive_value = printConfig.opt_float("max_volumetric_extrusion_rate_slope_positive");
	setItemValue(_L("Speed"), _L("Max volumetric slope positive"), max_volumetric_extrusion_rate_slope_positive_value);
	auto max_volumetric_extrusion_rate_slope_negative_value = printConfig.opt_float("max_volumetric_extrusion_rate_slope_negative");
	setItemValue(_L("Speed"), _L("Max volumetric slope negative"), max_volumetric_extrusion_rate_slope_negative_value);
}


void AnkerParameterPanel::initMeData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;	
	
	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();

	auto perimeter_extruder_value = printConfig.opt_int("perimeter_extruder");
	setItemValue(_L("Multiple Extruders"), _L("Perimeter extruder"), perimeter_extruder_value);
	auto infill_extruder_value = printConfig.opt_int("infill_extruder");
	setItemValue(_L("Multiple Extruders"), _L("Infill extruder"), infill_extruder_value);
	auto solid_infill_extruder_value = printConfig.opt_int("solid_infill_extruder");
	setItemValue(_L("Multiple Extruders"), _L("Solid infill extruder"), solid_infill_extruder_value);
	auto support_material_extruder_value = printConfig.opt_int("support_material_extruder");
	setItemValue(_L("Multiple Extruders"), _L("Support material/raft/skirt extruder"), support_material_extruder_value);
	auto support_material_interface_extruder_value = printConfig.opt_int("support_material_interface_extruder");
	setItemValue(_L("Multiple Extruders"), _L("Support material/raft interface extruder"), support_material_interface_extruder_value);

	auto ooze_prevention_value = printConfig.opt_bool("ooze_prevention");
	setItemValue(_L("Multiple Extruders"), _L("Enable ooze prevention"), ooze_prevention_value);
	auto standby_temperature_delta_value = printConfig.opt_int("standby_temperature_delta");
	setItemValue(_L("Multiple Extruders"), _L("Temperature variation"), standby_temperature_delta_value);

	auto wipe_tower_value = printConfig.opt_bool("wipe_tower");
	setItemValue(_L("Multiple Extruders"), _L("Enable wipe tower"), wipe_tower_value);
	auto wipe_tower_x_value = printConfig.opt_float("wipe_tower_x");
	setItemValue(_L("Multiple Extruders"), _L("Position X"), wipe_tower_x_value);
	auto wipe_tower_y_value = printConfig.opt_float("wipe_tower_y");
	setItemValue(_L("Multiple Extruders"), _L("Position Y"), wipe_tower_y_value);
	auto wipe_tower_width_value = printConfig.opt_float("wipe_tower_width");
	setItemValue(_L("Multiple Extruders"), _L("Width"), wipe_tower_width_value);
	auto wipe_tower_rotation_angle_value = printConfig.opt_float("wipe_tower_rotation_angle");
	setItemValue(_L("Multiple Extruders"), _L("Wipe tower rotation angle"), wipe_tower_rotation_angle_value);
	auto wipe_tower_brim_width_value = printConfig.opt_float("wipe_tower_brim_width");
	setItemValue(_L("Multiple Extruders"), _L("Wipe tower brim width"), wipe_tower_brim_width_value);
	auto wipe_tower_bridging_value = printConfig.opt_float("wipe_tower_bridging");
	setItemValue(_L("Multiple Extruders"), _L("Maximal bridging distance"), wipe_tower_bridging_value);

	auto wipe_tower_cone_angle_value = printConfig.opt_float("wipe_tower_cone_angle");
	setItemValue(_L("Multiple Extruders"), _L("Stabilization cone apex angle"), wipe_tower_cone_angle_value);
	auto wipe_tower_extra_spacing_value = printConfig.opt_serialize("wipe_tower_extra_spacing");
	wxString wxWipe_tower_extra_spacing_value = wipe_tower_extra_spacing_value;
	size_t pos = wxWipe_tower_extra_spacing_value.find('%');
	if (pos != wxString::npos) {
		wxWipe_tower_extra_spacing_value.erase(pos, 1);
	}
	setItemValue(_L("Multiple Extruders"), _L("Wipe tower purge lines spacing"), wxWipe_tower_extra_spacing_value);
	auto wipe_tower_no_sparse_layers_value = printConfig.opt_bool("wipe_tower_no_sparse_layers");
	setItemValue(_L("Multiple Extruders"), _L("No sparse layers (EXPERIMENTAL)"), wipe_tower_no_sparse_layers_value);
	auto single_extruder_multi_material_priming_value = printConfig.opt_bool("single_extruder_multi_material_priming");
	setItemValue(_L("Multiple Extruders"), _L("Prime all printing extruders"), single_extruder_multi_material_priming_value);
	
	auto interface_shells_value = printConfig.opt_bool("interface_shells");
	setItemValue(_L("Multiple Extruders"), _L("Interface shells"), interface_shells_value);
	auto mmu_segmented_region_max_width_value = printConfig.opt_float("mmu_segmented_region_max_width");
	setItemValue(_L("Multiple Extruders"), _L("Maximum width of a segmented region"), mmu_segmented_region_max_width_value);
	
}


void AnkerParameterPanel::initAdvancedData()
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	
	if (m_isRightParameterPanel)
		printConfig = m_rightParameterCfg->getPrintCfg();
	auto extrusion_width_value = printConfig.opt_serialize("extrusion_width");
	setItemValue(_L("Advanced"), _L("Default extrusion width"), extrusion_width_value);
	auto first_layer_extrusion_width_value = printConfig.opt_serialize("first_layer_extrusion_width");
	setItemValue(_L("Advanced"), _L("First layer"), first_layer_extrusion_width_value);
	auto perimeter_extrusion_width_value = printConfig.opt_serialize("perimeter_extrusion_width");
	setItemValue(_L("Advanced"), _L("Perimeters"), perimeter_extrusion_width_value);
	auto external_perimeter_extrusion_width_value = printConfig.opt_serialize("external_perimeter_extrusion_width");
	setItemValue(_L("Advanced"), _L("External perimeters"), external_perimeter_extrusion_width_value);
	auto infill_extrusion_width_value = printConfig.opt_serialize("infill_extrusion_width");
	setItemValue(_L("Advanced"), _L("Infill"), infill_extrusion_width_value);
	auto solid_infill_extrusion_width_value = printConfig.opt_serialize("solid_infill_extrusion_width");
	setItemValue(_L("Advanced"), _L("Solid infill"), solid_infill_extrusion_width_value);
	auto top_infill_extrusion_width_value = printConfig.opt_serialize("top_infill_extrusion_width");
	setItemValue(_L("Advanced"), _L("Top solid infill"), top_infill_extrusion_width_value);
	auto support_material_extrusion_width_value = printConfig.opt_serialize("support_material_extrusion_width");
	setItemValue(_L("Advanced"), _L("Support material"), support_material_extrusion_width_value);

	auto infill_overlap_value = printConfig.opt_serialize("infill_overlap");
	setItemValue(_L("Advanced"), _L("Infill/perimeters overlap"), infill_overlap_value);

	auto bridge_flow_ratio_value = printConfig.opt_float("bridge_flow_ratio");
	setItemValue(_L("Advanced"), _L("Bridge flow ratio"), bridge_flow_ratio_value);

	auto slice_closing_radius_value = printConfig.opt_float("slice_closing_radius");
	setItemValue(_L("Advanced"), _L("Slice gap closing radius"), slice_closing_radius_value);
	auto slicing_mode_value = printConfig.opt_enum<Slic3r::SlicingMode>("slicing_mode");
	setItemValue(_L("Advanced"), _L("Slicing Mode"), (int)slicing_mode_value);
	auto resolution_value = printConfig.opt_float("resolution");
	setItemValue(_L("Advanced"), _L("Slice resolution"), resolution_value);
	auto gcode_resolution_value = printConfig.opt_float("gcode_resolution");
	setItemValue(_L("Advanced"), _L("G-code resolution"), gcode_resolution_value);
	auto xy_size_compensation_value = printConfig.opt_float("xy_size_compensation");
	setItemValue(_L("Advanced"), _L("XY Size Compensation"), xy_size_compensation_value);
	auto elefant_foot_compensation_value = printConfig.opt_float("elefant_foot_compensation");
	setItemValue(_L("Advanced"), _L("Elephant foot compensation"), elefant_foot_compensation_value);

	auto wall_transition_angle_value = printConfig.opt_float("wall_transition_angle");
	setItemValue(_L("Advanced"), _L("Perimeter transitioning threshold angle"), wall_transition_angle_value);
	auto wall_transition_filter_deviation_value = printConfig.opt_serialize("wall_transition_filter_deviation");
	setItemValue(_L("Advanced"), _L("Perimeter transitioning filter margin"), wall_transition_filter_deviation_value);
	auto wall_transition_length_value = printConfig.opt_serialize("wall_transition_length");
	setItemValue(_L("Advanced"), _L("Perimeter transition length"), wall_transition_length_value);
	auto wall_distribution_count_value = printConfig.opt_int("wall_distribution_count");
	setItemValue(_L("Advanced"), _L("Perimeter distribution count"), wall_distribution_count_value);
	auto min_bead_width_value = printConfig.opt_serialize("min_bead_width");
	setItemValue(_L("Advanced"), _L("Minimum perimeter width"), min_bead_width_value);
	auto min_feature_size_value = printConfig.opt_serialize("min_feature_size");
	setItemValue(_L("Advanced"), _L("Minimum feature size"), min_feature_size_value);

}


bool AnkerParameterPanel::checkDirtyData()
{
	auto iter = m_windowTabMap.begin();
	bool hasDirtyData = false;
	while (iter != m_windowTabMap.end())
	{
		auto itemIter = iter->second.begin();
		while (itemIter != iter->second.end())
		{
			if ((*itemIter)->hsaDirtyData())
			{
				hasDirtyData = true;
				break;
			}
			++itemIter;
		}
		if (hasDirtyData)
			break;
		++iter;
	}
	if (hasDirtyData)
	{
		if (swtichPresetCheckSave())
		{
			resetAllUi();
			hideAllResetBtn();
		}

	}	
	return hasDirtyData;
}


bool AnkerParameterPanel::hasDirtyData()
{
	
	auto iter = m_windowTabMap.begin();
	bool hasDirtyData = false;
	while (iter != m_windowTabMap.end())
	{
		auto itemIter = iter->second.begin();
		while (itemIter != iter->second.end())
		{
			if ((*itemIter)->hsaDirtyData())
			{
				hasDirtyData = true;
				return hasDirtyData;
			}
			++itemIter;
		}
		++iter;
	}

	ANKER_LOG_INFO << "check dirty data is " << hasDirtyData;
	return hasDirtyData;
}

void AnkerParameterPanel::saveUiData()
{
	ANKER_LOG_INFO << "save ui data";
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	auto lapIter = m_lapMap.begin();
	while (lapIter != m_lapMap.end())
	{
		std::string AnkerPropertyName = lapIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = lapIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Layers and perimeters"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++lapIter;
	}

	auto infillIter = m_infillMap.begin();
	while (infillIter != m_infillMap.end())
	{
		std::string AnkerPropertyName = infillIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = infillIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Infill"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++infillIter;
	}

	auto sabIter = m_sabMap.begin();
	while (sabIter != m_sabMap.end())
	{
		std::string AnkerPropertyName = sabIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = sabIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Skirt and brim"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++sabIter;
	}

	auto smIter = m_smMap.begin();
	while (smIter != m_smMap.end())
	{
		std::string AnkerPropertyName = smIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = smIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Support material"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++smIter;
	}

	auto speedIter = m_speedMap.begin();
	while (speedIter != m_speedMap.end())
	{
		std::string AnkerPropertyName = speedIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = speedIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Speed"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++speedIter;
	}
	
	auto meIter = m_meMap.begin();
	while (meIter != m_meMap.end())
	{
		std::string AnkerPropertyName = meIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = meIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Multiple Extruders"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++meIter;
	}

	auto advanceIter = m_advancedMap.begin();
	while (advanceIter != m_advancedMap.end())
	{
		std::string AnkerPropertyName = advanceIter->first.ToStdString();
		std::string strTest(AnkerPropertyName.c_str());
		std::string prusaProperty = advanceIter->second.ToStdString();
		ItemInfo ItemDataInfo = getItemValue(_L("Advanced"), AnkerPropertyName);

		auto propertyType = ItemDataInfo.paramDataType;//emun 
		wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
		saveSetPresetPropertyValue(printConfig, ItemDataInfo, prusaProperty, AnkerPropertyName);
		++advanceIter;
	}
#if SHOW_OLD_SETTING_DIALOG
	Slic3r::GUI::wxGetApp().get_tab(Slic3r::Preset::TYPE_PRINT)->load_config(printConfig);
#endif
	Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT)->load_config(printConfig);
	Slic3r::GUI::wxGetApp().plater()->on_config_change(printConfig);	
	
}

void AnkerParameterPanel::hideAllResetBtn()
{
	ANKER_LOG_INFO << "hide all reset";
	auto iterItem = m_windowTabMap.begin();
	while (iterItem != m_windowTabMap.end())
	{
		auto itemList = iterItem->second;
		
		auto listIter = itemList.begin();
		while (listIter != itemList.end())
		{
			(*listIter)->hideAllResetBtn();
			(*listIter)->clearDirtyData();
			++listIter;
		}
		++iterItem;
	}

	Refresh();
	Layout();
}

void AnkerParameterPanel::setItemValue(const wxString tabName, const wxString& widgetLabel, wxVariant data)
{	
	//ANKER_LOG_INFO << "update data "<<tabName.ToStdString().c_str()<<" "<< widgetLabel.ToStdString().c_str();
	auto item = m_windowTabMap.find(tabName.ToStdString());

	if (item == m_windowTabMap.end())
		return;

	auto itemList = item->second;

	auto itemListIter = itemList.begin();
	while (itemListIter != itemList.end())
	{
		if ((*itemListIter)->isExistLabel(widgetLabel))
		{
			(*itemListIter)->updateUi(widgetLabel, data);
			return;
		}
		++itemListIter;
	}
}

void AnkerParameterPanel::openSupportMaterialPage(wxString itemName, wxString text)
{
	int index = m_pHandleModelComBox->FindString(itemName);
	if (index != wxNOT_FOUND)
	{
		m_pHandleModelComBox->SetSelection(index);
		wxCommandEvent event(wxEVT_COMBOBOX, m_pHandleModelComBox->GetId());
		event.SetEventObject(m_pHandleModelComBox);
		event.SetInt(index);
		m_pHandleModelComBox->GetEventHandler()->ProcessEvent(event);

		//switch to support material tab
		for (auto button : m_tabBtnVector)
		{
			if (text.IsSameAs(button->getText()))
			{
				m_pTabBtnScrolledWindow->Scroll(10, 0); //TODO: get point by text position on tab
				m_pTabBtnScrolledWindow->Refresh();
				wxMouseEvent mouse_event;
				button->OnPressed(mouse_event);
			}
		}
	}
}

ItemInfo AnkerParameterPanel::getItemValue(const wxString& tabStr, const wxString& labelStr)
{
	ItemInfo info;
	auto tabIter = m_windowTabMap.find(tabStr.ToStdString());
	if (tabIter != m_windowTabMap.end())
	{
		auto itemList = tabIter->second;

		auto itemIter = itemList.begin();
		while (itemIter != itemList.end())
		{
			if ((*itemIter)->isExistLabel(labelStr))
			{
				return (*itemIter)->getWidgetValue(labelStr);
			}
			++itemIter;
		}
	}

	return info;

}


std::vector<std::string> AnkerParameterPanel::getValidParameter()
{
	std::vector<std::string> vaildList = {};

	auto sysPresetList = Slic3r::GUI::wxGetApp().preset_bundle->prints.system_preset_names();

	wxArrayString strings = m_pPresetParameterComBox->GetStrings();

	bool isAdd = false;
	for (size_t i = 0; i < strings.GetCount(); i++)
	{
		std::string str = strings[i].ToStdString();		
		
		std::string strSuff = (" (" + _L("modified") + ")").ToUTF8().data();
		str = boost::algorithm::ends_with(str, strSuff) ?
			str.substr(0, str.size() - strSuff.size()) :
			str;		
		auto tempPreset = Slic3r::GUI::wxGetApp().preset_bundle->prints.find_preset(str,false,false);
		if (tempPreset && tempPreset->is_system)
		{
			vaildList.push_back(str);
		}	
	}	

	return vaildList;
}

wxString AnkerParameterPanel::getCurrentModule(const std::vector<std::string>& list)
{	
	if (list.size() <= 0)
		return "";
	wxString data = m_pPresetParameterComBox->GetValue();
	std::string strData = data.ToStdString();

	std::string strSuff = (" (" + _L("modified") + ")").ToUTF8().data();
	strData = boost::algorithm::ends_with(strData, strSuff) ?
		strData.substr(0, strData.size() - strSuff.size()) :
		strData;

	auto iter = list.begin();	

	while (iter != list.end())
	{
		auto tempPreset = Slic3r::GUI::wxGetApp().preset_bundle->prints.find_preset(strData, false, false);
		if (tempPreset && tempPreset->is_system)
		{		
			if ((*iter) == strData)
			return data;
		}
		++iter;
	}

	return list.at(0);
	
}


void AnkerParameterPanel::onDatachanged(wxCommandEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS);
	evt.SetEventObject(this);
	//ProcessEvent(evt);	
	wxPostEvent(this, evt);
}


void AnkerParameterPanel::onUpdateResetBtn(wxCommandEvent& event)
{
	onResetBtnStatusChanged(hasDirtyData());
}

void AnkerParameterPanel::saveSetPresetPropertyValue(Slic3r::DynamicPrintConfig& printConfig,
													ItemInfo& ItemDataInfo,
													std::string& prusaProperty,
													std::string& AnkerPropertyName)
{
	auto propertyType = ItemDataInfo.paramDataType;//emun 
	wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
	switch (propertyType)
	{
		case Item_int:
		{
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionInt((int)propertyValue.GetInteger()));
			break;
		}
		case Item_enum_FuzzySkinType:
		{
			Slic3r::FuzzySkinType fuzzySkinType = (Slic3r::FuzzySkinType)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::FuzzySkinType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::FuzzySkinType>(fuzzySkinType);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_PerimeterGeneratorType:
		{
			Slic3r::PerimeterGeneratorType perimeterGeneratorType = (Slic3r::PerimeterGeneratorType)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::PerimeterGeneratorType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::PerimeterGeneratorType>(perimeterGeneratorType);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_InfillPattern:
		{
			
			Slic3r::InfillPattern infillPattern;
			if(AnkerPropertyName == _L("Fill pattern"))
				infillPattern = m_fillPatternMap[propertyValue.GetInteger()];
			else
				infillPattern = m_tabfillPatternMap[propertyValue.GetInteger()];

			Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infillPattern);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_IroningType:
		{
			Slic3r::IroningType ironingType = (Slic3r::IroningType)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::IroningType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::IroningType>(ironingType);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_DraftShield:
		{
			Slic3r::DraftShield draftShield = (Slic3r::DraftShield)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::DraftShield>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::DraftShield>(draftShield);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_BrimType:
		{
			Slic3r::BrimType brimType = (Slic3r::BrimType)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::BrimType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::BrimType>(brimType);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_SupportMaterialStyle:
		{
			Slic3r::SupportMaterialStyle supportMaterialStyle = m_StyleMap[propertyValue.GetInteger()];
			Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialStyle>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialStyle>(supportMaterialStyle);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_SupportMaterialPattern:
		{
			Slic3r::SupportMaterialPattern supportMaterialPattern = (Slic3r::SupportMaterialPattern)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialPattern>(supportMaterialPattern);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_SupportMaterialInterfacePattern:
		{
			Slic3r::SupportMaterialInterfacePattern supportMaterialInterfacePattern = (Slic3r::SupportMaterialInterfacePattern)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialInterfacePattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SupportMaterialInterfacePattern>(supportMaterialInterfacePattern);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_SeamPosition:
		{
			Slic3r::SeamPosition seamPosition = (Slic3r::SeamPosition)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::SeamPosition>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SeamPosition>(seamPosition);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_SlicingMode:
		{
			Slic3r::SlicingMode sliceMode = (Slic3r::SlicingMode)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>(sliceMode);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_floatOrPercent:
		{
			if (AnkerPropertyName == _L("First layer height"))
			{
				printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, false));
			}
			else
			{
				wxString data = propertyValue.GetString();
				if(data.Contains('%'))
					printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, true));
				else //if(data.Contains("mm"))				
					printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionFloatOrPercent(propertyValue, false));

			}
			break;
		}
		case Item_bool:
		{
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionBool(propertyValue));
			break;
		}

		case Item_float:
		{
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionFloat(propertyValue));
			break;
		}
		case Item_serialize:
		case Item_serialize_no_unit:
		case Item_serialize_num:
		{
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionString(propertyValue));
			break;
		}
		case Item_Percent:
		{		
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionPercent(propertyValue));
			break;
		}		
		default:
		{
			break;
		}
	}
}


void AnkerParameterPanel::saveSetPresetPropertyValueEx(Slic3r::DynamicPrintConfig* printConfig, ItemInfo& ItemDataInfo, std::string& prusaProperty, std::string& AnkerPropertyName)
{
	auto propertyType = ItemDataInfo.paramDataType;//emun 
	wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
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
		if (AnkerPropertyName == _L("Fill pattern"))
			infillPattern = m_fillPatternMap[propertyValue.GetInteger()];
		else
			infillPattern = m_tabfillPatternMap[propertyValue.GetInteger()];

		Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infillPattern);
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
		Slic3r::SupportMaterialStyle supportMaterialStyle = m_StyleMap[propertyValue.GetInteger()];
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
	case Item_enum_SlicingMode:
	{
		Slic3r::SlicingMode sliceMode = (Slic3r::SlicingMode)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>(sliceMode);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_floatOrPercent:
	{
		if (AnkerPropertyName == _L("First layer height"))
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
	default:
	{
		break;
	}
	}
}

void AnkerParameterPanel::onResetBtnStatusChanged(bool isAble)
{
	ANKER_LOG_INFO << "single control reset status";
#ifndef __APPLE__
	if (isAble)
	{
		wxImage resetBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("reset_btn.png")), wxBITMAP_TYPE_PNG);		
		resetBtnImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		m_pResetBtn->SetBitmap(resetBtnImage);
	}
	else
	{
		wxImage resetBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("disabel_reset_btn.png")), wxBITMAP_TYPE_PNG);
		resetBtnImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		m_pResetBtn->SetBitmap(resetBtnImage);
	}
#else
	if (isAble)
	{
		m_pResetBtn->SetBitmap_("reset_btn");
	}
	else
	{
		m_pResetBtn->SetBitmap_("disabel_reset");
	}
#endif
	m_pResetBtn->Enable(isAble);
}

AnkerPrintParaItem::AnkerPrintParaItem(wxWindow* parent, 
										wxString icon,
										wxString title,
										wxString tabTitle,
										wxWindowID winid /*= wxID_ANY*/, 
										const wxPoint& pos /*= wxDefaultPosition*/, 
										const wxSize& size /*= wxDefaultSize*/)
										: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
										, m_icon(icon)
										, m_title(title)
										, m_tabTitle(tabTitle)
{
	initDefaultData();
	initUi();
}

AnkerPrintParaItem::~AnkerPrintParaItem()
{

}


wxString AnkerPrintParaItem::getTitle() const
{
	return m_title;
}

void AnkerPrintParaItem::createItem(const wxString& widgetLabel, 
									ControlType controlType, 
									ItemDataType dataType,
									wxStringList strList /*= {}*/)
{
	wxBoxSizer* pTitleSizer = new wxBoxSizer(wxHORIZONTAL);
	pTitleSizer->AddSpacer(12);

	wxString labelStr = Slic3r::GUI::WrapEveryCharacter(widgetLabel, ANKER_FONT_NO_1, 140);
	wxStaticText* pTitle = new wxStaticText(this, wxID_ANY, widgetLabel);
 	pTitle->SetFont(ANKER_FONT_NO_1);
	pTitle->SetForegroundColour(wxColour("#A9AAAB"));		

	wxClientDC dc(this);
	dc.SetFont(pTitle->GetFont());
	wxSize size = dc.GetTextExtent(widgetLabel);
	int textWidth = size.GetWidth();
	int texthigth = size.GetHeight();
	
	if (textWidth > 140)
	{
		size.SetHeight(texthigth * 2);
		size.SetWidth(150);
		pTitle->Wrap(150);
	}
	pTitle->SetLabelText(labelStr);
	pTitleSizer->Add(pTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, wxALIGN_CENTER_VERTICAL, 0);
	pTitleSizer->AddSpacer(8);	

	ScalableButton *resetBtn = new ScalableButton(this, wxID_ANY, "reset_btn", "", AnkerSize(20, 20));
	resetBtn->SetMaxSize(AnkerSize(20, 20));
	resetBtn->SetMinSize(AnkerSize(20, 20));
	resetBtn->SetSize(AnkerSize(20, 20));
	resetBtn->Hide();
	resetBtn->SetWindowStyleFlag(wxBORDER_NONE);
	resetBtn->SetBackgroundColour(wxColour("#292A2D"));
	resetBtn->Bind(wxEVT_BUTTON, &AnkerPrintParaItem::onResetBtnClicked, this);
 	pTitleSizer->AddStretchSpacer(1);
		
	pTitleSizer->Add(resetBtn, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
	pTitleSizer->AddSpacer(8);
	PARAMETER_GROUP paraGroup;
	paraGroup.m_dataType = dataType;
	paraGroup.m_pBtn = resetBtn;
	paraGroup.m_pLabel = pTitle;
	paraGroup.m_type = controlType;
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
							wxSize(120, 30),
							m_arrItems,
							wxNO_BORDER | wxCB_READONLY);

			pCBox->SetCursor(handCursor);
			pCBox->SetBackgroundColour(BgColor);
			pCBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
			pCBox->SetMinSize(wxSize(120, 30));
			pCBox->SetSize(wxSize(120, 30));
			pCBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

			if(strList.GetCount() > 0)
				pCBox->SetSelection(0);
			

			pCBox->Bind(wxEVT_COMBOBOX, [this, resetBtn,pCBox, paraGroup](wxCommandEvent& event) {
				
				if (!resetBtn || !pCBox)
					return;
				
				int cboxIndex = pCBox->GetSelection();
				std::string cboxText = pCBox->GetString(cboxIndex).ToStdString();

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

				int  prusaIndex = 0;
				wxVariant cfgData;

				switch (paraGroup.m_dataType)
				{
				case Item_enum_SeamPosition:
				{
					auto tempData = printConfig.opt_enum<Slic3r::SeamPosition>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_PerimeterGeneratorType:
				{
					auto tempData = printConfig.opt_enum<Slic3r::PerimeterGeneratorType>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_FuzzySkinType:
				{
					auto tempData = printConfig.opt_enum<Slic3r::FuzzySkinType>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_InfillPattern:
				{
					auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(prusaKeyString);
					auto tempStrData = printConfig.opt_serialize(prusaKeyString);
					std::locale loc;
					tempStrData[0] = std::toupper(tempStrData[0], loc);

					auto realData = m_fillPatternData[tempStrData];
					if (cboxText != realData)
					{
						resetBtn->Show();
						ItemDirtyData dirtyData;
						dirtyData.tabName = m_tabTitle;
						dirtyData.titleName = m_title;
						dirtyData.prusaKey = prusaKeyString;
						dirtyData.ankerKey = strLabel;
						dirtyData.dataType = paraGroup.m_dataType;
						//dirtyData.oldData = prusaIndex;
						//dirtyData.newData = cboxIndex;

						m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
					}
					else
					{
						auto iter = m_dirtyMap.find(prusaKeyString);
						if (iter != m_dirtyMap.end())
							m_dirtyMap.erase(iter);
													
						resetBtn->Hide();
					}


					{
						ItemInfo dataInfo;
						dataInfo.paramDataType = paraGroup.m_dataType;
						dataInfo.paramDataValue = cboxText;
						AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
						if (!pItemWidget)
							return;
						AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
						Slic3r::ModelConfig* cfgConfig = nullptr;
						if (pWidget)
						{
							if (pWidget->getObjConfig(cfgConfig))
							{
								std::string stdPrusaKey = prusaKeyString;
								std::string ankerKey = strLabel.ToStdString();
								saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
							}
						}						
					}
					onDatachanged();
					onUpdateResetBtn();
					Refresh();
					Layout();

					return;
				}
				break;
				case Item_enum_IroningType:
				{
					auto tempData = printConfig.opt_enum<Slic3r::IroningType>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_DraftShield:
				{
					auto tempData = printConfig.opt_enum<Slic3r::DraftShield>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_BrimType:
				{
					auto tempData = printConfig.opt_enum<Slic3r::BrimType>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_SupportMaterialStyle:
				{
					Slic3r::SupportMaterialStyle emunData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(prusaKeyString);
					int tempData = m_StyleMap[emunData];
					prusaIndex = tempData;
				}
				break;
				case Item_enum_SupportMaterialPattern:
				{
					auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialPattern>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_SupportMaterialInterfacePattern:
				{
					auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				case Item_enum_SlicingMode:
				{
					auto tempData = printConfig.opt_enum<Slic3r::SlicingMode>(prusaKeyString);
					prusaIndex = (int)tempData;
				}
				break;
				default:
					break;
	
				}

				if (cboxIndex != prusaIndex)
				{
					ItemDirtyData dirtyData;
					dirtyData.tabName = m_tabTitle;
					dirtyData.titleName = m_title;
					dirtyData.prusaKey = prusaKeyString;
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;
					 
					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
					resetBtn->Show();
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if(iter!=m_dirtyMap.end())
					 	m_dirtyMap.erase(iter);					
					resetBtn->Hide();
				}
				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = wxVariant(cboxIndex);
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}

				Refresh();
				Layout();				

				});
			
			pTitleSizer->Add(pCBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL,0);
			pTitleSizer->AddSpacer(4);
			paraGroup.m_pWindow = pCBox;
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
				wxSize(120, 30),
				m_arrItems,
				wxBORDER_SIMPLE);
#else
			pCBox->Create(this,
				wxID_ANY,
				wxEmptyString,
				wxDefaultPosition,
				wxSize(120, 30),
				m_arrItems,
				wxNO_BORDER);
			
#endif // !__APPLE__			

			pCBox->SetBackgroundColour(BgColor);
			pCBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
			pCBox->SetForegroundColour(wxColour("#FFFFFF"));
			pCBox->SetMinSize(wxSize(120, 30));
			pCBox->SetSize(wxSize(120, 30));
			pCBox->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);

			if (strList.GetCount() > 0)
				pCBox->SetSelection(0);
			
			pCBox->Bind(wxEVT_KILL_FOCUS, [this, resetBtn, pCBox, paraGroup](wxFocusEvent& event) {
				
				int index = 0;
				wxString text = wxString();
				wxString currentText = wxString();

				if (!pCBox || !resetBtn)
					return;
				
				index = pCBox->GetSelection();
				text = pCBox->GetValue();
				currentText = pCBox->GetValue();

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
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

							if (text == _L("Same as top"))
								index = index - 1;

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
				else if ("fill_density" == prusaKeyString || "infill_anchor" == prusaKeyString || "infill_anchor_max" == prusaKeyString)
				{
					wxString editValue = text;
					wxString strUnit = ' ' + _L("mm");
					editValue.Replace('%', "");
					editValue.Replace(strUnit, "");
					
					wxString presetValue = prusaValue;
					presetValue.Replace('%', "");
					presetValue.Replace("mm", "");

					if (wxPrusaValue.Contains("%") && !text.Contains("%")|| !wxPrusaValue.Contains("%") && text.Contains("%"))
					{
						bdirty = true;
						resetBtn->Show();						
					}
					else if (editValue != presetValue)
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
					//dirtyData.oldData = prusaIndex;
					//dirtyData.newData = cboxIndex;

					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);

				}

				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = cfgData;
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}


				Refresh();
				Layout();
				

				});
			pCBox->Bind(wxEVT_COMBOBOX, [this, resetBtn, pCBox, paraGroup](wxCommandEvent& event) {

				if (!resetBtn || !pCBox)
					return;

				int index = 0;
				wxString text = wxString();

				index = pCBox->GetSelection();
				text = pCBox->GetValue();

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
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

				else if ("fill_density" == prusaKeyString|| "infill_anchor" == prusaKeyString|| "infill_anchor_max" == prusaKeyString)
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
					//dirtyData.oldData = prusaIndex;
					//dirtyData.newData = cboxIndex;

					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);
				}

				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = cfgData;
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}


				Refresh();
				Layout();

				});

			pTitleSizer->Add(pCBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
			pTitleSizer->AddSpacer(4);
			paraGroup.m_pWindow = pCBox;
		}break;
		case ItemEditUinit:
		{
			wxString strUnit = "";

			if (strList.size() == 1)
				strUnit = *strList.begin();
		
			AnkerLineEditUnit* pItemEdit = new AnkerLineEditUnit(this, strUnit, ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);			

            int lineEditHeight = 25;
			pItemEdit->setLineEditFont(ANKER_FONT_NO_1);
			pItemEdit->SetMaxSize(AnkerSize(120, lineEditHeight));
			pItemEdit->SetMinSize(AnkerSize(120, lineEditHeight));
			pItemEdit->SetSize(AnkerSize(120, lineEditHeight));
#ifdef __WXOSX__
			int shrink = 10;
#else
			int shrink = 4;
#endif
			pItemEdit->getTextEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getTextEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getTextEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));

			pItemEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this, resetBtn, pItemEdit, paraGroup](wxCommandEvent& event) {
				
				if (!pItemEdit || !resetBtn)
					return;

				wxString strUnit = ' ' + _L("mm");

				wxString editValue = pItemEdit->GetValue();
				wxVariant cfgData = editValue;
				wxString realEditValue = editValue;
				realEditValue.Replace('%', "");
				realEditValue.Replace(strUnit, "");

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
				
				wxString prusaValue = printConfig.opt_serialize(prusaKeyString);
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
						updateUi(_L("Layer height"), realEditValue);
					}					
				}

				bool bdirty = false;
				if (prusaValue.Contains("%") && !editValue.Contains("%") || !prusaValue.Contains("%") && editValue.Contains("%"))
				{
					bdirty = true;
					resetBtn->Show();
				}
				else if (realEditValue != realPrusaValue)
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
					//dirtyData.oldData = prusaIndex;
					//dirtyData.newData = cboxIndex;

					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);					
				}

				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = cfgData;
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}

				Refresh();
				Layout();
				
			});

			pTitleSizer->Add(pItemEdit, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL,0);
			pTitleSizer->AddSpacer(4);

			paraGroup.m_pWindow = pItemEdit;
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
			pCheckBox->SetMinSize(wxSize(16, 16));
			pCheckBox->SetMaxSize(wxSize(16, 16));
			pCheckBox->SetSize(wxSize(16, 16));			
			
			pTitleSizer->Add(pCheckBox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
			pTitleSizer->AddSpacer(4);
			pCheckBox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, [this, resetBtn, pCheckBox, paraGroup](wxCommandEvent &event) {

				if (!resetBtn)
					return;

				bool isChecked = pCheckBox->getCheckStatus();
				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if(strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
				auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

				bool spiral_vase_value = printConfig.opt_bool(prusaKeyString);

				bool bdirty = false;
				if (isChecked != spiral_vase_value)
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
					//dirtyData.oldData = prusaIndex;
					//dirtyData.newData = cboxIndex;

					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);					
				}

				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = isChecked;
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}

				Refresh();
				Layout();
				
				});

			paraGroup.m_pWindow = pCheckBox;
		}
			break;
		case ItemSpinBox:
		{			
			AnkerSpinEdit* pItemEdit = new AnkerSpinEdit(this, wxColour(41, 42, 45), wxColour("#3F4043"), 4, "", wxID_ANY);

			wxBoxSizer* pWidgetVSizer = new wxBoxSizer(wxHORIZONTAL);
            int lineEditHeight = 25;
            pItemEdit->SetMaxSize(AnkerSize(120, lineEditHeight));
            pItemEdit->SetMinSize(AnkerSize(120, lineEditHeight));
            pItemEdit->SetSize(AnkerSize(120, lineEditHeight));
#ifdef __WXOSX__
			int shrink = 10;
#else
			int shrink = 4;
#endif
            pItemEdit->getTextEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
            pItemEdit->getTextEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
            pItemEdit->getTextEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetMinSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetMaxSize(AnkerSize(-1, lineEditHeight - shrink));
			pItemEdit->getUnitEdit()->SetSize(AnkerSize(-1, lineEditHeight - shrink));

			pItemEdit->setLineEditFont(ANKER_FONT_NO_1);

			pItemEdit->Bind(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED, [this, resetBtn, pItemEdit, paraGroup](wxCommandEvent&event) {
				if (!resetBtn)
					return;
				int uValue = 0;
				wxString strValue = pItemEdit->GetValue();
				strValue.ToInt(&uValue);

				wxString strLabel = paraGroup.m_pLabel->GetLabelText();

				if (strLabel.Contains('\n'))
					strLabel.Replace('\n', "");

				auto dataMap = getItemMap();
				std::string prusaKeyString = dataMap[strLabel].ToStdString();
				if (prusaKeyString.empty())
					return;
				auto printConfig  = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
				float prusasValue = 0;
				if("top_solid_min_thickness" == prusaKeyString||"bottom_solid_min_thickness" == prusaKeyString)
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
					dirtyData.prusaKey = prusaKeyString;
					dirtyData.ankerKey = strLabel;
					dirtyData.dataType = paraGroup.m_dataType;
					//dirtyData.oldData = prusaIndex;
					//dirtyData.newData = cboxIndex;

					m_dirtyMap.insert(std::make_pair(prusaKeyString, dirtyData));
				}
				else
				{
					auto iter = m_dirtyMap.find(prusaKeyString);
					if (iter != m_dirtyMap.end())
						m_dirtyMap.erase(iter);
				}

				onDatachanged();
				onUpdateResetBtn();

				{
					ItemInfo dataInfo;
					dataInfo.paramDataType = paraGroup.m_dataType;
					dataInfo.paramDataValue = wxVariant(uValue);
					AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
					if (!pItemWidget)
						return;
					AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
					Slic3r::ModelConfig* cfgConfig = nullptr;
					if (pWidget)
					{
						if (pWidget->getObjConfig(cfgConfig))
						{
							std::string stdPrusaKey = prusaKeyString;
							std::string ankerKey = strLabel.ToStdString();
							saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
						}
					}
				}

				Refresh();
				Layout();
				
			});

			pTitleSizer->Add(pItemEdit, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_VERTICAL, 0);
			pTitleSizer->AddSpacer(4);
			paraGroup.m_pWindow = pItemEdit;
		}
			break;
		default:
			break;		
	}

	m_windowWgtLabelMap.insert(std::pair<wxString, PARAMETER_GROUP>(widgetLabel, paraGroup));	
	pTitleSizer->AddSpacer(20);
	//m_pMainVSizer->Add(pTitleSizer, wxSizerFlags().Expand().Border(wxBottom, 4));
	m_pMainVSizer->Add(pTitleSizer, 0, wxALL | wxEXPAND, 0);
	m_pMainVSizer->AddSpacer(4);

	m_pMainVSizer->AddSpacer(10);
}


bool AnkerPrintParaItem::hsaDirtyData()
{
	if (m_dirtyMap.size() > 0)
		return true;
	else
		return false;
}

ItemInfo AnkerPrintParaItem::getWidgetValue(const wxString& labelStr)
{
	auto iter = m_windowWgtLabelMap.find(labelStr);
	ItemInfo valueData;
	if (iter != m_windowWgtLabelMap.end())
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
			if (labelStr == _L("Layer height"))
			{
				double dValue = 0;
				data.ToDouble(&dValue);

				if (dValue < 0.01)
				{
					data = "0.01";
					updateUi(_L("Layer height"), data);
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
			break;
		default:
			break;
		}
	}
	return valueData;
}

bool AnkerPrintParaItem::isExistLabel(const wxString& widgetLabel)
{
	bool res = false;
	auto iter = m_windowWgtLabelMap.find(widgetLabel);

	if (iter != m_windowWgtLabelMap.end())
		return true;
	else 
		return false;

	return res;
}

void AnkerPrintParaItem::updateUi(const wxString& widgetLabel, wxVariant data, bool isReset /*= false*/)
{
	auto iter = m_windowWgtLabelMap.find(widgetLabel);
	if (iter != m_windowWgtLabelMap.end())
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
				if (data.GetType() == wxT("string"))					
					pComBox->SetStringSelection(itemStrValue);
				else
					pComBox->SetSelection(Itemvalue);
				
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
				if (data.GetType() == wxT("string"))					
					pComBox->SetValue(itemStrValue);
				else
				{
					if(Itemvalue >= pComBox->GetCount())
						pComBox->SetValue(itemStrValue);
					else
						pComBox->SetSelection(Itemvalue);
				}
			}
		}
		break;
		case ItemEditUinit:
		{
			wxString Itemvalue = {""};
			AnkerLineEditUnit* pEditUnit = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if(!isReset)
				Itemvalue = data.GetString();			
			else
			{
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			if (pEditUnit)
			{
				pEditUnit->setValue(Itemvalue);
			}
		}
		break;
		case ItemCheckBox:
		{
			AnkerCheckBox* pCheckBox = static_cast<AnkerCheckBox*>(iter->second.m_pWindow);
			bool Itemvalue = true;
			if(!isReset)
				Itemvalue = data.GetBool();
			else
			{
				Itemvalue = false;// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			if (pCheckBox)
			{
				pCheckBox->setCheckStatus(Itemvalue);
			}
		}
		break;
		case ItemSpinBox:
		{
			wxString Itemvalue = {""};		
			if(!isReset)
				Itemvalue = data.GetString();
			else
			{
				Itemvalue = "0";// wxGetApp().preset_bundle->prints.get_selected_preset().config.get_abs_value(strKey);
			}

			AnkerLineEditUnit* pEditRange = static_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
			if (pEditRange)
			{
				pEditRange->setValue(Itemvalue);
			}
		}
		break;
		default:
			break;
		}
	}
	
}

void AnkerPrintParaItem::showLabelHighlight(const wxString& widgetLabel, wxColour labelColor)
{
	auto iter = m_windowWgtLabelMap.find(widgetLabel);
	static int times = 0;
	if (iter != m_windowWgtLabelMap.end())
	{
		(*iter).second.m_pLabel->SetForegroundColour(m_colour);
		Refresh();
		m_HightLightTimer->Start(1000);						
		m_pCurrentLabel = (*iter).second.m_pLabel;		
		m_colour = labelColor;	
	}
}

void AnkerPrintParaItem::onDatachanged()
{	
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS);
	evt.SetEventObject(this);
	//ProcessEvent(evt);
	wxPostEvent(this, evt);
}


void AnkerPrintParaItem::onUpdateResetBtn()
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE);
	evt.SetEventObject(this);
	//ProcessEvent(evt);
	wxPostEvent(this, evt);
}

std::map<wxString, wxString> AnkerPrintParaItem::getItemMap()
{
	std::map<wxString, wxString> emptyMap;
	if (m_tabTitle == _L("Layers and perimeters"))
		return m_lapMap;
	else if (m_tabTitle == _L("Infill"))
		return m_infillMap;
	else if (m_tabTitle == _L("Skirt and brim"))
		return m_sabMap;
	else if (m_tabTitle == _L("Support material"))
		return m_smMap;
	else if (m_tabTitle == _L("Speed"))
		return m_speedMap;
	else if (m_tabTitle == _L("Multiple Extruders"))
		return m_meMap;
	else if (m_tabTitle == _L("Advanced"))
		return m_advancedMap;
	else
		return emptyMap;

}

wxString AnkerPrintParaItem::getPrusaLabel(const wxString& labelStr)
{
	if (m_tabTitle == _L("Layers and perimeters"))
	{
		auto iter = m_lapMap.find(labelStr);
		if (iter != m_lapMap.end())
			return iter->second;
		else
			return "";
	}	
	else if(m_tabTitle == _L("Infill"))
	{
		auto iter = m_infillMap.find(labelStr);
		if (iter != m_infillMap.end())
			return iter->second;
		else
			return "";
	}
	else if (m_tabTitle == _L("Skirt and brim"))
	{
		auto iter = m_sabMap.find(labelStr);
		if (iter != m_sabMap.end())
			return iter->second;
		else
			return "";
	}
	else if (m_tabTitle == _L("Support material"))
	{
		auto iter = m_smMap.find(labelStr);
		if (iter != m_smMap.end())
			return iter->second;
		else
			return "";
	}
	else if (m_tabTitle == _L("Speed"))
	{
		auto iter = m_speedMap.find(labelStr);
		if (iter != m_speedMap.end())
			return iter->second;
		else
			return "";
	}
	else if (m_tabTitle == _L("Multiple Extruders"))
	{
		auto iter = m_meMap.find(labelStr);
		if (iter != m_meMap.end())
			return iter->second;
		else
			return "";
	}
	else if (m_tabTitle == _L("Advanced"))
	{
		auto iter = m_advancedMap.find(labelStr);
		if (iter != m_advancedMap.end())
			return iter->second;
		else
			return "";
	}

	return "";
}

void AnkerPrintParaItem::initDefaultData()
{
	m_fillPatternData = { {("Rectilinear"), _L("Rectilinear")},
						{("Aligned Rectilinear"), _L("Aligned Rectilinear")},
						{("Alignedrectilinear"), _L("Aligned Rectilinear")},
						{("Grid"), _L("Grid")},
						{("Triangles"), _L("Triangles")},
						{("Stars"), _L("Stars")},
						{("Cubic"), _L("Cubic")},
						{("Line"), _L("Line")},
						{("Concentric"), _L("Concentric")},
						{("Honeycomb"), _L("Honeycomb")},
						{("3D Honeycomb"), _L("3D Honeycomb")},
						{("Gyroid"), _L("Gyroid")},
						{("Hilbert Curve"), _L("Hilbert Curve")},
						{("Hilbertcurve"), _L("Hilbert Curve")},
						{("Archimedean Chords"),_L("Archimedean Chords")},
						{("Archimedeanchords"),_L("Archimedean Chords")},
						{("Octagram Spiral"),_L("Octagram Spiral")},
						{("Octagramspiral"),_L("Octagram Spiral")},
						{("Adaptive Cubic"), _L("Adaptive Cubic")},
						{("Support Cubic"), _L("Support Cubic")},
						{("Lightning"), _L("Lightning")},
						{("Monotonic"), _L("Monotonic")},
						{("Monotonic Lines"), _L("Monotonic Lines")},
						{("Monotoniclines"), _L("Monotonic Lines")},
	};

	m_lapMap = {
											{_L("Layer height"),("layer_height")},
											{_L("First layer height"),("first_layer_height")},
											{_L("Perimeters"),("perimeters")},
											{ _L("Spiral vase"),("spiral_vase")},
											{_L("Top solid layers"),("top_solid_layers")},
											{_L("Top minimum shell thickness"),("top_solid_min_thickness")},
											{_L("Bottom solid layers"),("bottom_solid_layers")},
											{_L("Bottom minimum shell thickness"),("bottom_solid_min_thickness")},
											{_L("Extra perimeters if needed"),("extra_perimeters")},
											{_L("Extra perimeters on overhangs(Experimental)"),("extra_perimeters_on_overhangs")},
											{_L("Avoid crossing curled overhangs(Experimental)"),("avoid_crossing_curled_overhangs")},
											{_L("Avoid crossing perimeters"),("avoid_crossing_perimeters")},
											{_L("Avoid crossing perimeters - Max detour length"),("avoid_crossing_perimeters_max_detour")},
											{_L("Detect thin walls"),("thin_walls")},
											{_L("Thick bridges"),("thick_bridges")},
											{_L("Detect bridging perimeters"),("overhangs")},
											{_L("Seam position"),("seam_position")},
											{_L("Staggered inner seams"),("staggered_inner_seams")},
											{_L("External perimeters first"),("external_perimeters_first")},
											{_L("Fill gaps"),("gap_fill_enabled")},
											{_L("Perimeter generator"),("perimeter_generator")},
											{_L("Fuzzy Skin"),("fuzzy_skin")},
											{_L("Fuzzy skin thickness"),("fuzzy_skin_thickness")},
											{_L("Fuzzy skin point distance"),("fuzzy_skin_point_dist")},
	};
	m_infillMap = { {_L("Fill density"),("fill_density")},
												  {_L("Fill pattern"),("fill_pattern")},
												  {_L("Length of the infill anchor"),("infill_anchor")},
												  {_L("Maximum length of the infill anchor"),("infill_anchor_max")},
												  {_L("Top fill pattern"),("top_fill_pattern")},
												  {_L("Bottom fill pattern"),("bottom_fill_pattern")},
												  {_L("Enable ironing"),("ironing")},
												  {_L("Ironing Type"),("ironing_type")},
												  {_L("Flow rate"),("ironing_flowrate")},
												  {_L("Spacing between ironing passes"),("ironing_spacing")},
												  {_L("Combine infill every"),("infill_every_layers")},
												  {_L("Solid infill every"),("solid_infill_every_layers")},
												  {_L("Fill angle"),("fill_angle")},
												  {_L("Solid infill threshold area"),("solid_infill_below_area")},
												  {_L("Bridging angle"),("bridge_angle")},
												  {_L("Only retract when crossing perimeters"),("only_retract_when_crossing_perimeters")},
												  {_L("Infill before perimeters"),("infill_first")},
	};
	 m_sabMap = {
												{_L("Loops (minimum)"),("skirts")},
												{_L("Distance from brim/object"),("skirt_distance")},
												{_L("Skirt height"),("skirt_height")},
												{_L("Draft shield"),("draft_shield")},
												{_L("Minimal filament extrusion length"),("min_skirt_length")},
												{_L("Brim type"),("brim_type")},
												{_L("Brim width"),("brim_width")},
												{_L("Brim separation gap"),("brim_separation")},
	};
	 m_smMap = {
											 {_L("Generate support material"),("support_material")},
											 {_L("Auto generated supports"),("support_material_auto")},
											 {_L("Overhang threshold"),("support_material_threshold")},
											 {_L("Enforce support for the first"),("support_material_enforce_layers")},
											 {_L("First layer density"),("raft_first_layer_density")},
											 {_L("First layer expansion"),("raft_first_layer_expansion")},
											 {_L("Raft layers"),("raft_layers")},
											 {_L("Raft contact Z distance"),("raft_contact_distance")},
											 {_L("Raft expansion"),("raft_expansion")},
											 {_L("Style"),("support_material_style")},
											 {_L("Top contact Z distance"),("support_material_contact_distance")},
											 {_L("Bottom contact Z distance"),("support_material_bottom_contact_distance")},
											 {_L("Pattern"),("support_material_pattern")},
											 {_L("With sheath around the support"),("support_material_with_sheath")},
											 {_L("Pattern spacing"),("support_material_spacing")},
											 {_L("Pattern angle"),("support_material_angle")},
											 {_L("Closing radius"),("support_material_closing_radius")},
											 {_L("Top interface layers"),("support_material_interface_layers")},
											 {_L("Bottom interface layers"),("support_material_bottom_interface_layers")},
											 {_L("Interface pattern"),("support_material_interface_pattern")},
											 {_L("Interface pattern spacing"),("support_material_interface_spacing")},
											 {_L("Interface loops"),("support_material_interface_contact_loops")},
											 {_L("Support on build plate only"),("support_material_buildplate_only")},
											 {_L("XY separation between an object and its support"),("support_material_xy_spacing")},
											 {_L("Don't support bridges"),("dont_support_bridges")},
											 {_L("Synchronize with object layers"),("support_material_synchronize_layers")},
											 {_L("Maximum Branch Angle"),("support_tree_angle")},
											 {_L("Preferred Branch Angle"),("support_tree_angle_slow")},
											 {_L("Branch Diameter"),("support_tree_branch_diameter")},
											 {_L("Branch Diameter Angle"),("support_tree_branch_diameter_angle")},
											 {_L("Tip Diameter"),("support_tree_tip_diameter")},
											 {_L("Branch Distance"),("support_tree_branch_distance")},
											 {_L("Branch Density"),("support_tree_top_rate")},
	};
	 m_speedMap = {
												{_L("Speed for print moves perimeters"),("perimeter_speed")},
												{_L("Small perimeters"),("small_perimeter_speed")},
												{_L("Speed for print moves external perimeters"),("external_perimeter_speed")},
												{_L("Speed for print moves infill"),("infill_speed")},
												{_L("Speed for print moves solid infill"),("solid_infill_speed")},
												{_L("Speed for print moves top solid infill"),("top_solid_infill_speed")},
												{_L("Support material"),("support_material_speed")},
												{_L("Support material interface"),("support_material_interface_speed")},
												{_L("Speed for print moves bridges"),("bridge_speed")},
												{_L("Gap fill"),("gap_fill_speed")},
												{_L("Ironing"),("ironing_speed")},
												{_L("Enable dynamic overhang speeds"),("enable_dynamic_overhang_speeds")},
												{_L("speed for 0% overlap (bridge)"),("overhang_speed_0")},
												{_L("speed for 25% overlap"),("overhang_speed_1")},
												{_L("speed for 50% overlap"),("overhang_speed_2")},
												{_L("speed for 75% overlap"),("overhang_speed_3")},
												{_L("Speed for non-print moves travel"),("travel_speed")},
												{_L("Z travel"),("travel_speed_z")},
												{_L("First layer speed"),("first_layer_speed")},
												{_L("Speed of object first layer over raft interface"),("first_layer_speed_over_raft")},
												{_L("Acceleration control (advanced) external perimeters"),("external_perimeter_acceleration")},
												{_L("Acceleration control (advanced) perimeters"),("perimeter_acceleration")},
												{_L("Acceleration control (advanced) top solid infill"),("top_solid_infill_acceleration")},
												{_L("Acceleration control (advanced) solid infill"),("solid_infill_acceleration")},
												{_L("Acceleration control (advanced) infill"),("infill_acceleration")},
												{_L("Acceleration control (advanced) bridges"),("bridge_acceleration")},
												{_L("First layer"),("first_layer_acceleration")},
												{_L("First object layer over raft interface"),("first_layer_acceleration_over_raft")},
												{_L("Acceleration control (advanced) travel"),("travel_acceleration")},
												{_L("Default"),("default_acceleration")},
												{_L("Max print speed"),("max_print_speed")},
												{_L("Max volumetric speed"),("max_volumetric_speed")},
												{_L("Max volumetric slope positive"),("max_volumetric_extrusion_rate_slope_positive")},
												{_L("Max volumetric slope negative"),("max_volumetric_extrusion_rate_slope_negative")},
	};
	 m_meMap = {
											 {_L("Perimeter extruder"),("perimeter_extruder")},
											 {_L("Infill extruder"),("infill_extruder")},
											 {_L("Solid infill extruder"),("solid_infill_extruder")},
											 {_L("Support material/raft/skirt extruder"),("support_material_extruder")},
											 {_L("Support material/raft interface extruder"),("support_material_interface_extruder")},
											 {_L("Enable ooze prevention"),("ooze_prevention")},
											 {_L("Temperature variation"),("standby_temperature_delta")},
											 {_L("Enable wipe tower"),("wipe_tower")},
											 {_L("Position X"),("wipe_tower_x")},
											 {_L("Position Y"),("wipe_tower_y")},
											 {_L("Width"),("wipe_tower_width")},
											 {_L("Wipe tower rotation angle"),("wipe_tower_rotation_angle")},
											 {_L("Wipe tower brim width"),("wipe_tower_brim_width")},
											 {_L("Maximal bridging distance"),("wipe_tower_bridging")},
											 {_L("Stabilization cone apex angle"),("wipe_tower_cone_angle")},
											 {_L("Wipe tower purge lines spacing"),("wipe_tower_extra_spacing")},
											 {_L("No sparse layers (EXPERIMENTAL)"),("wipe_tower_no_sparse_layers")},
											 {_L("Prime all printing extruders"),("single_extruder_multi_material_priming")},
											 {_L("Interface shells"),("interface_shells")},
											 { _L("Maximum width of a segmented region"),("mmu_segmented_region_max_width")},
	};
	m_advancedMap = {
												{_L("Default extrusion width"),("extrusion_width")},
												{_L("First layer"),("first_layer_extrusion_width")},
												{_L("Perimeters"),("perimeter_extrusion_width")},
												{_L("External perimeters"),("external_perimeter_extrusion_width")},
												{_L("Infill"),("infill_extrusion_width")},
												{_L("Solid infill"),("solid_infill_extrusion_width")},
												{_L("Top solid infill"),("top_infill_extrusion_width")},
												{_L("Support material"),("support_material_extrusion_width")},
												{_L("Infill/perimeters overlap"),("infill_overlap")},
												{_L("Bridge flow ratio"),("bridge_flow_ratio")},
												{_L("Slice gap closing radius"),("slice_closing_radius")},
												{_L("Slicing Mode"),("slicing_mode")},
												{_L("Slice resolution"),("resolution")},
												{_L("G-code resolution"),("gcode_resolution")},
												{_L("XY Size Compensation"),("xy_size_compensation")},
												{_L("Elephant foot compensation"),("elefant_foot_compensation")},
												{_L("Perimeter transitioning threshold angle"),("wall_transition_angle")},
												{_L("Perimeter transitioning filter margin"),("wall_transition_filter_deviation")},
												{_L("Perimeter transition length"),("wall_transition_length")},
												{_L("Perimeter distribution count"),("wall_distribution_count")},
												{_L("Minimum perimeter width"),("min_bead_width")},
												{_L("Minimum feature size"),("min_feature_size")},
	};

	m_StyleMap = {
		{Slic3r::SupportMaterialStyle::smsGrid, 0},
		{Slic3r::SupportMaterialStyle::smsSnug, 1},
		{Slic3r::SupportMaterialStyle::smsOrganic, 2},
	};
	m_fillPatternMap = {
	{ 0,        Slic3r::InfillPattern::ipRectilinear },
	{ 1,          Slic3r::InfillPattern::ipAlignedRectilinear },
	{ 2,     			Slic3r::InfillPattern::ipGrid },
	{ 3, 			Slic3r::InfillPattern::ipTriangles },
	{ 4,              Slic3r::InfillPattern::ipStars },
	{ 5,              Slic3r::InfillPattern::ipCubic },
	{ 6,               Slic3r::InfillPattern::ipLine },
	{ 7,         Slic3r::InfillPattern::ipConcentric },
	{ 8,          Slic3r::InfillPattern::ipHoneycomb },
	{ 9,        Slic3r::InfillPattern::ip3DHoneycomb },
	{ 10,            Slic3r::InfillPattern::ipGyroid },
	{ 11,       Slic3r::InfillPattern::ipHilbertCurve },
	{ 12,  Slic3r::InfillPattern::ipArchimedeanChords },
	{ 13,     Slic3r::InfillPattern::ipOctagramSpiral },
	{ 14,      Slic3r::InfillPattern::ipAdaptiveCubic },
	{ 15,       Slic3r::InfillPattern::ipSupportCubic },
	{ 16,          Slic3r::InfillPattern::ipLightning }
	};

	m_tabfillPatternMap = {
		{ 0,        Slic3r::InfillPattern::ipRectilinear },
		{ 1,          Slic3r::InfillPattern::ipMonotonic },
		{ 2,     			Slic3r::InfillPattern::ipMonotonicLines },
		{ 3, 			Slic3r::InfillPattern::ipAlignedRectilinear },
		{ 4,              Slic3r::InfillPattern::ipConcentric },
		{ 5,              Slic3r::InfillPattern::ipHilbertCurve },
		{ 6,               Slic3r::InfillPattern::ipArchimedeanChords },
		{ 7,         Slic3r::InfillPattern::ipOctagramSpiral },
	};
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
 	pTitle->SetFont(ANKER_FONT_NO_1);
	pTitle->SetForegroundColour(wxColour("#FFFFFF"));

 	wxPanel* pDividingLine = new wxPanel(this, wxID_ANY);
 	pDividingLine->SetBackgroundColour(wxColour("#38393C"));
 	pDividingLine->SetMinSize(wxSize(335, 1));
 	pDividingLine->SetMaxSize(wxSize(335, 1));

	pTitleHSizer->AddSpacer(12);
	pTitleHSizer->Add(pIcon,0, wxALIGN_CENTER_VERTICAL| wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER_VERTICAL| wxALIGN_CENTER_HORIZONTAL,0);
	pTitleHSizer->AddSpacer(6);
	pTitleHSizer->Add(pTitle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL |wxEXPAND, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddSpacer(4);
	pTitleHSizer->Add(pDividingLine, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	m_pMainVSizer->Add(pTitleHSizer, wxSizerFlags().Expand().Border(wxRight | wxLEFT, 12));
	m_pMainVSizer->AddSpacer(4);
	SetSizer(m_pMainVSizer);
}


void AnkerPrintParaItem::onResetBtnClicked(wxCommandEvent& event)
{
	wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
	if (!button)
		return;
	
	button->Hide();
	Refresh();
	Layout();
	
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;

	auto iter = m_windowWgtLabelMap.begin();
	while (iter != m_windowWgtLabelMap.end())
	{
		if (button == iter->second.m_pBtn)
		{						
			wxString wxidgetLabel = iter->first;
			wxString prusaLabel = getPrusaLabel(wxidgetLabel);

			if (prusaLabel.IsEmpty())
				return;

			ItemDataType dataTyoe = iter->second.m_dataType;
			wxVariant data;
			switch (dataTyoe)
			{
			case Item_int:
			{
				auto tempData = printConfig.opt_int(prusaLabel.ToStdString());
				data = wxVariant(tempData);
			}
				break;
			case Item_bool:
			{
				data = printConfig.opt_bool(prusaLabel.ToStdString());
			}
				break;
			case Item_float:
			{
				data = printConfig.opt_float(prusaLabel.ToStdString());
			}
				break;
			case Item_floatOrPercent:
			{
				auto strData = printConfig.get_abs_value(prusaLabel.ToStdString());
				if (wxidgetLabel != _L("First layer height"))				
				{		
					data = printConfig.opt_serialize(prusaLabel.ToStdString());
				}
				else
					data = strData;
			}
			break;
			case Item_serialize:
			{
				auto strdata = printConfig.opt_serialize(prusaLabel.ToStdString());
				if (_L("Top contact Z distance") == wxidgetLabel)
				{
					wxString tempdata = strdata;
			
					if (tempdata.Contains("0.1"))
						data = _L("0.1 (detachable)");
					else if (tempdata.Contains("0.2"))
						data = _L("0.2 (detachable)");
					else
						data = _L("0 (soluble)");
				}
				else if (_L("Length of the infill anchor") == wxidgetLabel)
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
					else
						data = _L("0 (no open anchors)");			
				}
				else
					data = strdata;					
			}
				break;
			case Item_serialize_no_unit:
			{
				data = printConfig.opt_serialize(prusaLabel.ToStdString());
				wxString tempData = data;
					
				size_t pos = tempData.find('%');
				if (pos != wxString::npos) {
					tempData.erase(pos, 1);
				}
				data = tempData;
					
			}
				break;
			case Item_serialize_num:
			{
				std::string strdata = printConfig.opt_serialize(prusaLabel.ToStdString());
				if(_L("Bottom interface layers") == wxidgetLabel)
				{
					wxString tempdata = strdata;
					if (tempdata.Contains("0"))
						data = _L("0 (off)");
					else if (tempdata.Contains("-1"))
						data = _L("Same as top");
					else if (tempdata.Contains("2"))
						data = _L("2 (default)");
					else if (tempdata.Contains("3"))
						data = _L("3 (heavy)");
					else if (tempdata == ("1"))						
						data = _L("1 (light)");
						
				}
				else
					data = strdata;
			}
				break;
			case Item_Percent:
			{
				auto tempData = printConfig.opt_serialize(prusaLabel.ToStdString());
					
				data = wxVariant(tempData);
			}
			break;
			case Item_enum_SeamPosition:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SeamPosition>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_PerimeterGeneratorType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::PerimeterGeneratorType>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_FuzzySkinType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::FuzzySkinType>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_InfillPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::InfillPattern>(prusaLabel.ToStdString());
				auto tempStrData = printConfig.opt_serialize(prusaLabel.ToStdString());
				std::locale loc;
				tempStrData[0] = std::toupper(tempStrData[0], loc);
				data = m_fillPatternData[tempStrData];

			}
				break;
			case Item_enum_IroningType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::IroningType>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_DraftShield:
			{
				auto tempData = printConfig.opt_enum<Slic3r::DraftShield>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_BrimType:
			{
				auto tempData = printConfig.opt_enum<Slic3r::BrimType>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			case Item_enum_SupportMaterialStyle:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;				
			case Item_enum_SupportMaterialPattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialPattern>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;				
			case Item_enum_SupportMaterialInterfacePattern:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;				
			case Item_enum_SlicingMode:
			{
				auto tempData = printConfig.opt_enum<Slic3r::SlicingMode>(prusaLabel.ToStdString());
				data = wxVariant((int)tempData);
			}
				break;
			default:
				break;
			}
			
			{
				auto iter = m_dirtyMap.find(prusaLabel.ToStdString());
				if (iter != m_dirtyMap.end())
					m_dirtyMap.erase(iter);

				onDatachanged();
				onUpdateResetBtn();
			}


			switch (iter->second.m_type)
			{
			case ItemComBox:
			{
				wxOwnerDrawnComboBox* pComBox = dynamic_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
				if(!pComBox)
				{
					++iter;
					continue;
				}

				if (data.GetType() == wxT("string"))
					pComBox->SetStringSelection(data.GetString());
				else
				{
					int index = data.GetInteger();
					pComBox->SetSelection(index);
				}
									
				return;
			}
			break;
			case ItemEditBox:
			{
				wxOwnerDrawnComboBox* pComBox = dynamic_cast<wxOwnerDrawnComboBox*>(iter->second.m_pWindow);
				if (!pComBox)
				{
					++iter;
					continue;
				}
				if (data.GetType() == wxT("string"))
					pComBox->SetValue(data.GetString());
				else
				{
					wxString strType = data.GetType();
					int index = data.GetInteger();
					wxString strIndex = data.GetString();

					if(data.GetType() == wxT("double"))
						pComBox->SetValue(data.GetString());
					else
					{
						if ("support_material_bottom_interface_layers" == prusaLabel && index == -1)
							index = 0;

						pComBox->SetSelection(index);										
					}

				}
				return;
			}
			break;
			case ItemEditUinit:
			{
				AnkerLineEditUnit* pEdit = dynamic_cast<AnkerLineEditUnit*>(iter->second.m_pWindow);
				if (!pEdit)
				{
					Slic3r::SupportMaterialStyle emunData = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(prusaLabel.ToStdString());
					int tempData = m_StyleMap[emunData];
					data = wxVariant(tempData);
				}
					
				pEdit->SetValue(data);
				return;
			}
				break;
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

						if(_L("Top contact Z distance") == wxidgetLabel)
						{
 							wxString strValue = data.GetString();
 							wxString realData = "error";
 							
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
							
							pComBox->SetStringSelection(realData);
						}
						else if (_L("Bottom interface layers") == wxidgetLabel)
						{
							int index = data.GetInteger();
							pComBox->SetSelection(index + 1);
						}
						else
							pComBox->SetSelection(data.GetInteger());
					}
					return;
				}
					
				pCheckBox->setCheckStatus(data);
				return;
			}
			break;
			case ItemSpinBox:
			{
				AnkerSpinEdit* pEdit = dynamic_cast<AnkerSpinEdit*>(iter->second.m_pWindow);
				if (!pEdit)
				{
					++iter;
					continue;
				}
					
				pEdit->setValue(data);
				return;
			}
			break;
			default:
				break;
			}

			ItemInfo dataInfo;
			dataInfo.paramDataType = iter->second.m_dataType;
			dataInfo.paramDataValue = data;
			AnkerPrintParaItem* pItemWidget = static_cast<AnkerPrintParaItem*>(GetParent());
			if (!pItemWidget)
				return;

			AnkerParameterPanel* pWidget = static_cast<AnkerParameterPanel*>(pItemWidget->GetParent());
			Slic3r::ModelConfig* cfgConfig = nullptr;
			if (pWidget)
			{
				if (pWidget->getObjConfig(cfgConfig))
				{
					std::string stdPrusaKey = prusaLabel.ToStdString();
					std::string ankerKey = wxidgetLabel.ToStdString();
					saveSetPresetPropertyValue(cfgConfig, dataInfo, stdPrusaKey, ankerKey);
				}
			}
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
	//ANKER_LOG_INFO << "save item data "<< prusaProperty.c_str()<<" "<< AnkerPropertyName.c_str();
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
		if (AnkerPropertyName == _L("Fill pattern"))
			infillPattern = m_fillPatternMap[propertyValue.GetInteger()];
		else
			infillPattern = m_tabfillPatternMap[propertyValue.GetInteger()];

		Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infillPattern);
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
	case Item_enum_SlicingMode:
	{
		Slic3r::SlicingMode sliceMode = (Slic3r::SlicingMode)propertyValue.GetInteger();
		Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SlicingMode>(sliceMode);
		printConfig->set_key_value(prusaProperty, configOption);
		break;
	}
	case Item_floatOrPercent:
	{
		if (AnkerPropertyName == _L("First layer height"))
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
	default:
	{
		break;
	}
	}
}

void AnkerPrintParaItem::hideAllResetBtn()
{
	auto iter = m_windowWgtLabelMap.begin();
	while (iter != m_windowWgtLabelMap.end())
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
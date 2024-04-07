#include "../AnkerSideBarNew.hpp"
#include "AnkerParameterPanel.hpp"
#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "../GUI_App.hpp"
#include "wx/app.h"
#include "../I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "../AnkerCheckBox.hpp"

#include "../Plater.hpp"
#include "libslic3r/print.hpp"
#include "../SavePresetDialog.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "../common/AnkerGUIConfig.hpp"
#include "../SavePresetDialog.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "../GUI.hpp"
#include "../format.hpp"
#include <boost/exception/diagnostic_information.hpp>
#include "libslic3r/Config.hpp"
#include "slic3r/Utils/StringHelper.hpp"



using	namespace Slic3r::GUI;


wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SAVE_PROJECT_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SAVEALL_BTN_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_UPDATE_CURRENT_PRESET, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, wxCommandEvent);


std::vector<wxString> ParamTabTextVec ;

AnkerParameterPanel::AnkerParameterPanel(wxWindow* parent, 
										PrintParamMode printParamMode,
										wxWindowID winid /*= wxID_ANY*/, 
										const wxPoint& pos /*= wxDefaultPosition*/, 
										const wxSize& size /*= wxDefaultSize*/)
										: wxControl(parent, wxID_ANY, wxDefaultPosition, AnkerSize(-1, -1), wxBORDER_NONE),m_PrintParamMode(printParamMode)
{	
	initUi();	
	showCurrentWidget(ParamTabTextVec[0]);
	showEasyModel();
	initSearchPopupWidget();
	initUiData();
	setParamTooltips();
	Layout();
}

void AnkerParameterPanel::setParamTooltips()
{
	for (const auto& tabIter : m_windowTabMap)
	{
		for (const auto& PrintParaItem : tabIter.second) 
		{
			const auto& groupInfos = PrintParaItem->GetGroupparamInfos();
			for (const auto& groupIter : groupInfos)
			{
				auto optionKey = groupIter.first.ToStdString();
				ControlType controlType = groupIter.second.m_type;
				wxWindow* pCurrentWidget = groupIter.second.m_pWindow;
				if (pCurrentWidget)
				{
					auto optiontip = PrintParaItem->GetOptionTip(optionKey);
					if (optiontip.empty())
					{
						continue;
					}
					auto parameterLabel = _L("parameter name") + "\t: " + optionKey;
					auto realWidgetToolTips = optiontip + "\n" + parameterLabel;
					if (ItemEditUinit == controlType || ItemSpinBox == controlType)
					{
						AnkerLineEditUnit* pEdit = static_cast<AnkerLineEditUnit*>(pCurrentWidget);
						pEdit->SetToolTip(wxString(realWidgetToolTips));
					}
					else
					{
						pCurrentWidget->SetToolTip(realWidgetToolTips);
					}
				}
			}
		}
	}
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

void AnkerParameterPanel::resetAllUi(int iResetType /*= 0*/)
{
	ANKER_LOG_INFO << "real reset all parameter data";
	initUiData(iResetType);
}

bool AnkerParameterPanel::saveAllUi()
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
		//ankerTab->save_preset(name, true);
		//Slic3r::GUI::wxGetApp().preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);


		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_tab_ui();
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->onAnkerPresetsChanged();
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_changed_ui();
		//Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_FILAMENT)->update_tab_ui();
		
		Slic3r::GUI::wxGetApp().mainframe->diff_dialog.update_presets(prestType);
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets->save_current_preset(name, false);

		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->select_preset(Slic3r::Preset::remove_suffix_modified(name));
		
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
	
	if((Slic3r::GUI::wxGetApp().app_config->get("default_action_on_select_preset") == "none" && savedlg.ShowModal() == wxID_CANCEL))		
	{
		ankerTab->m_presets->discard_current_changes();
		return false;
	}

	if (savedlg.save_preset())
	{
		name = savedlg.get_preset_name();
		saveUiData();
		//ankerTab->save_preset(name, true);
		//Slic3r::GUI::wxGetApp().preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_preset_bundle->update_compatible(Slic3r::PresetSelectCompatibleType::Never);
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_tab_ui();
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->onAnkerPresetsChanged();
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_changed_ui();
		//Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_FILAMENT)->update_tab_ui();
		Slic3r::GUI::wxGetApp().mainframe->diff_dialog.update_presets(prestType);
		Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets->save_current_preset(name, false);
		//Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->select_preset(Slic3r::Preset::remove_suffix_modified(name));
		//wxWindowUpdateLocker updateLocker(this);
		m_pPresetParameterComBox->update();
		onResetBtnStatusChanged(false);
		hideAllResetBtn();
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

	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;    // groups
		for (const auto& PrintParaItem : PrintParaItems) {
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto label = PrintParaItem->GetIOptionLabel(optionKey);

				ItemInfo ItemDataInfo = getItemValue(tabName, optionKey);
				if ("support_material_bottom_interface_layers" == optionKey)
				{
					wxString tempData = ItemDataInfo.paramDataValue.GetString();
					if (ItemDataInfo.paramDataValue.GetString() == _L("Same as top"))
						ItemDataInfo.paramDataValue = wxVariant("-1");
				}
				
				//comment by samuel, no need to change user set value implicitly
				//if ("raft_first_layer_density" == optionKey)
				//{
				//	double value;
				//	wxString tempData = ItemDataInfo.paramDataValue.GetString();
				//	tempData.ToDouble(&value);
				//	//Dividing by 0 will cause crash
				//	if (value <= 0.0) {
				//		ItemDataInfo.paramDataValue = wxVariant(10.0);
				//	}
				//	if (value > 100.0) {
				//		ItemDataInfo.paramDataValue = wxVariant(100.0);
				//	}
				//}

				auto propertyType = ItemDataInfo.paramDataType;//emun 
				wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
				saveSetPresetPropertyValue(printConfig, ItemDataInfo, optionKey, label);
			}
		}
	}
}

void AnkerParameterPanel::updatePresetEx(Slic3r::DynamicPrintConfig* printConfig)
{
	ANKER_LOG_INFO << "update preset";

	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;    // groups
		for (const auto& PrintParaItem : PrintParaItems) {
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto label = PrintParaItem->GetIOptionLabel(optionKey);


				ItemInfo ItemDataInfo = getItemValue(tabName, optionKey);
				if ("support_material_bottom_interface_layers" == optionKey)
				{
					wxString tempData = ItemDataInfo.paramDataValue.GetString();
					if (ItemDataInfo.paramDataValue.GetString() == _L("Same as top"))
						ItemDataInfo.paramDataValue = wxVariant("-1");
				}
				auto propertyType = ItemDataInfo.paramDataType;			//emun 
				wxVariant propertyValue = ItemDataInfo.paramDataValue;  //wxVariant

				saveSetPresetPropertyValueEx(printConfig, ItemDataInfo, optionKey, label);
			}
		}
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
	showExpertModel();
	showRightParameterpanel();
}

void AnkerParameterPanel::showRightParameterpanel()
{
	ANKER_LOG_INFO << "show right menue parameter panel";
	m_pTitleLabel->SetLabelText(_L("common_slice_toolpannel_addprintsettings"));
	m_pResetBtn->Hide();
	m_pSaveAllBtn->Hide();
	m_pRenameRemovePresetBtn->Hide();
	
	m_pExpertLabel->Hide();
	m_pExpertModeBtn->Hide();
	m_pPresetParameterComBox->Hide();
	m_printParameterEditBtn->Hide();
	m_pBtnPanel->Hide();

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

void AnkerParameterPanel::moveWipeTower(double x, double y, double angle)
{
/* 
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	auto old_x = printConfig.opt_float("wipe_tower_x");
	auto old_y = printConfig.opt_float("wipe_tower_y");
	auto old_angle = printConfig.opt_float("wipe_tower_rotation_angle");

	setItemValueEx(_L("Multiple Extruders"), _L("wipe_tower_x"), x,abs(old_x - x) > 0.000001f );
	setItemValueEx(_L("Multiple Extruders"), _L("wipe_tower_y"), y, abs(old_y - y) > 0.000001f);
	setItemValueEx(_L("Multiple Extruders"), _L("wipe_tower_rotation_angle"), angle, abs(old_angle - angle) > 0.000001f);
	wxCommandEvent event1;
	onDatachanged(event1);
	onUpdateResetBtn(event1);
*/
}

void AnkerParameterPanel::initUi()
{	
	SetBackgroundColour(wxColour(ParameterPanelBgColor));
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
		pTitleWidget->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, SIDEBARNEW_PRINT_TEXTBTN_SIZER));
		pTitleWidget->SetBackgroundColour(ParameterPanelBgColor);
		wxBoxSizer* pTitleVSizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);

		pTitleWidget->SetSizer(pTitleVSizer);
		pTitleVSizer->Add(pTitleHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		m_pExitBtn = new ScalableButton(pTitleWidget, wxID_ANY, "return", "");
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
		m_pTitleLabel->SetMinSize(AnkerSize(100, -1));
		m_pTitleLabel->SetSize(AnkerSize(100, -1));

		m_deleteBtn = new ScalableButton(pTitleWidget, wxID_ANY, "delete_cfg", "");
		m_deleteBtn->SetMaxSize(AnkerSize(25, 32));
		m_deleteBtn->SetSize(AnkerSize(25, 32));
		m_deleteBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			wxCommandEvent deleteEvt = wxCommandEvent(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT);
			deleteEvt.SetEventObject(this);
			ProcessEvent(deleteEvt);

			m_currentObjName->SetLabelText("");
			m_isRightParameterPanel = false;
			});
		m_deleteBtn->Hide();

		m_printParameterEditBtn = new ScalableButton(pTitleWidget, wxID_ANY, "cog", "");
		m_printParameterEditBtn->SetToolTip(_L("common_slicepannel_hover_presetsetting"));
		m_printParameterEditBtn->SetMaxSize(AnkerSize(25, 32));
		m_printParameterEditBtn->SetSize(AnkerSize(25, 32));
		m_printParameterEditBtn->Hide();
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


		m_pExpertLabel = new wxStaticText(pTitleWidget, wxID_ANY, _L("common_slicepannel_style11_expert"));
		m_pExpertLabel->SetFont(ANKER_FONT_NO_1);
		m_pExpertLabel->SetForegroundColour(wxColour("#FFFFFF"));


		m_pExpertModeBtn = new AnkerToggleBtn(pTitleWidget);
		m_pExpertModeBtn->SetMinSize(AnkerSize(26, 14));
		m_pExpertModeBtn->SetMaxSize(AnkerSize(26, 14));
		m_pExpertModeBtn->SetSize(AnkerSize(26, 14));
		m_pExpertModeBtn->SetBackgroundColour(wxColour(41, 42, 45));
		m_pExpertModeBtn->SetStateColours(true, wxColour(129, 220, 129), wxColour(250, 250, 250));
		m_pExpertModeBtn->SetStateColours(false, wxColour(83, 83, 83), wxColour(219, 219, 219));
		m_pExpertModeBtn->Bind(wxCUSTOMEVT_ANKER_BTN_CLICKED, [&, this](wxCommandEvent& event) {
			bool state = this->m_pExpertModeBtn->GetState();
			if (state) {
				showExpertModel();
			}
			else {
				checkDirtyData();
				showEasyModel();
				if (!isSelectSystemPreset())
				{
					onEasyPanelItemClicked(getSelectedPrintMode());
				}
			}
			});

		m_pulldown_btn = new ScalableButton(pTitleWidget, wxID_ANY, "pulldown_48x48", "");
		m_pulldown_btn->SetMaxSize(AnkerSize(25, 32));
		m_pulldown_btn->SetSize(AnkerSize(25, 32));
		m_pulldown_btn->Bind(wxEVT_BUTTON, [&](wxCommandEvent) {
			if (m_contentWidget->IsShown())
			{
				m_pMainVSizer->Hide(m_contentWidget);
				this->m_pulldown_btn->SetBitmap_("pullup_48x48");
				this->SetBackgroundColour(wxColour("#18191B"));
			}
			else
			{
				m_pMainVSizer->Show(m_contentWidget);
				this->m_pulldown_btn->SetBitmap_("pulldown_48x48");
				this->SetBackgroundColour(wxColour("#292A2D"));
			}
			this->Refresh();
			this->Layout();
			});


#ifdef __APPLE__
		pTitleHSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
		pTitleHSizer->Add(m_pExitBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
		pTitleHSizer->AddStretchSpacer(1);
		pTitleHSizer->Add(m_pExpertLabel, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL| wxRIGHT, 5);
		pTitleHSizer->Add(m_pExpertModeBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);

		pTitleHSizer->AddSpacer(10);
		pTitleHSizer->Add(m_printParameterEditBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_deleteBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		pTitleHSizer->Add(m_pulldown_btn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);

		pTitleHSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
#else
		pTitleHSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
		pTitleHSizer->Add(m_pExitBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
		pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		//pTitleHSizer->AddStretchSpacer(10);
		pTitleHSizer->Add(m_pExpertLabel, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		pTitleHSizer->Add(m_pExpertModeBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);

		pTitleHSizer->AddSpacer(10);
		pTitleHSizer->Add(m_printParameterEditBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->Add(m_deleteBtn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		pTitleHSizer->Add(m_pulldown_btn, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
		pTitleHSizer->AddSpacer(AnkerLength(SIDEBARNEW_PRINTGER_HOR_SPAN));
#endif

		m_pMainVSizer->Add(pTitleWidget, 0, wxALL, 0);
		wxPanel* pDividingLine = new wxPanel(this, wxID_ANY);
		pDividingLine->SetBackgroundColour(wxColour("#38393C"));
		pDividingLine->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 1));
		m_pMainVSizer->Add(pDividingLine,0, wxALL, 0);
	}

	// content 
	m_contentWidget = new wxPanel(this);
	m_contentSizer = new wxBoxSizer(wxVERTICAL);

	// easy mode
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

		m_pEasyWidget = new AnkerEasyPanel(m_contentWidget, wxID_ANY);
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
		
		m_pEasyWidget->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 220));		
		m_pEasyWidget->SetSize(AnkerSize(PARAMETER_PANEL_WITH, 220));
		m_pEasyWidget->setCurrentWidget(0);
		m_pEasyWidget->Bind(wxCUSTOMEVT_ANKER_EASY_ITEM_CLICKED, [this](wxCommandEvent&event) {
			wxStringClientData * pData = static_cast<wxStringClientData*>(event.GetClientObject());
			if (pData)
			{
				wxString strItemName = pData->GetData().ToStdString();
				onEasyPanelItemClicked(strItemName);
			}
			});
		m_contentSizer->Add(m_pEasyWidget, 0, wxALL, 12);
	}

	// expert mode
	//parameter choose combox
	{
		wxBoxSizer* pParameterHSizer = new wxBoxSizer(wxHORIZONTAL);	
		m_currentObjName = new wxStaticText(m_contentWidget, wxID_ANY, "");
		m_currentObjName->SetFont(ANKER_FONT_NO_1);
		m_currentObjName->SetForegroundColour(wxColor(255,255,255));
		m_currentObjName->SetBackgroundColour(ParameterPanelBgColor);		
		m_currentObjName->Hide();

		m_pPresetParameterComBox = new Slic3r::GUI::AnkerPlaterPresetComboBox(m_contentWidget, Slic3r::Preset::TYPE_PRINT, AnkerSize(276, 30));
		m_pPresetParameterComBox->set_button_clicked_function([this]() {
			ANKER_LOG_INFO << "preset combox of print clicked";
			m_pPresetParameterComBox->SetFocus();
			onComboBoxClick(m_pPresetParameterComBox);
			});
		m_pPresetParameterComBox->set_selection_changed_function([this](int selection) {
			ANKER_LOG_INFO << "preset combox of print clicked";
			onPresetComboSelChanged(m_pPresetParameterComBox, selection);
			});
		m_pPresetParameterComBox->Create(	m_contentWidget,
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
		m_pPresetParameterComBox->SetBackgroundColour(ParameterPanelBgColor);
		m_pPresetParameterComBox->update();
		m_pPresetParameterComBox->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
		
		wxImage resetBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("disabel_reset_btn.png")), wxBITMAP_TYPE_PNG);
		resetBtnImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);

#ifndef __APPLE__
		//m_pResetBtn = new wxButton(m_contentWidget, wxID_ANY);
		//m_pResetBtn->SetBitmap(resetBtnImage);		
		m_pResetBtn = new ScalableButton(m_contentWidget, wxID_ANY, "reset_btn", "", AnkerSize(20, 20));
#else		
		m_pResetBtn = new ScalableButton(m_contentWidget, wxID_ANY, "reset_btn", "", AnkerSize(20, 20));
#endif // !__APPLE__
		m_pResetBtn->SetBackgroundColour(ParameterPanelBgColor);
		m_pResetBtn->SetWindowStyleFlag(wxBORDER_NONE);		
		m_pResetBtn->SetMinSize(AnkerSize(20,20));
		m_pResetBtn->SetMaxSize(AnkerSize(20,20));
		m_pResetBtn->SetSize(AnkerSize(20,20));
		wxCursor handCursor(wxCURSOR_HAND);
		m_pResetBtn->SetCursor(handCursor);
		m_pResetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			ANKER_LOG_INFO << "reset all parameter data";
			wxWindowUpdateLocker updateLocker(this);
			onResetBtnStatusChanged(false);
			int iRestType = m_isRightParameterPanel ? 1 : 0;
			resetAllUi(iRestType);
			hideAllResetBtn();
			onDatachanged(event);
			});		


		wxImage renameRremovePresetBtn = wxImage(wxString::FromUTF8(Slic3r::var("rename_remove_preset.png")), wxBITMAP_TYPE_PNG);
		renameRremovePresetBtn.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
#ifndef __APPLE__
		//m_pRenameRemovePresetBtn = new wxButton(m_contentWidget, wxID_ANY);
		//m_pRenameRemovePresetBtn->SetBitmap(renameRremovePresetBtn);
		m_pRenameRemovePresetBtn = new ScalableButton(m_contentWidget, wxID_ANY, "rename_remove_preset", "", AnkerSize(16, 16));
#else
		m_pRenameRemovePresetBtn = new ScalableButton(m_contentWidget, wxID_ANY, "rename_remove_preset", "", AnkerSize(16, 16));
#endif // !__APPLE__
		m_pRenameRemovePresetBtn->SetWindowStyleFlag(wxBORDER_NONE);
		m_pRenameRemovePresetBtn->SetBackgroundColour(ParameterPanelBgColor);
		m_pRenameRemovePresetBtn->SetMinSize(AnkerSize(16, 16));
		m_pRenameRemovePresetBtn->SetMaxSize(AnkerSize(16, 16));
		m_pRenameRemovePresetBtn->SetSize(AnkerSize(16, 16));
		m_pRenameRemovePresetBtn->SetCursor(handCursor);
		m_pRenameRemovePresetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			auto menu = new wxMenu(wxT(""));
			menu->Append(ID_PRESET_RENAME, _L("common_right_panel_rename_preset"));
			menu->Append(ID_PRESET_DELETE, _L("common_right_panel_remove_preset"));
			// enable or disable rename menu item
			bool bRenamePresetOpMenu = false;
			bool bDeletePresetOpMenu = false;
			auto printPreset = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset();
			 bDeletePresetOpMenu = (!printPreset.is_default && !printPreset.is_system);
			 bRenamePresetOpMenu = !printPreset.is_default && !printPreset.is_system && !printPreset.is_external &&
				!Slic3r::GUI::wxGetApp().preset_bundle->physical_printers.has_selection();
			if (!bRenamePresetOpMenu) {
				menu->Enable(ID_PRESET_RENAME, false);
			}
			// enable or disable rename menu item
			if (!bDeletePresetOpMenu) {
				menu->Enable(ID_PRESET_DELETE, false);
			}
			menu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent& event) {
				int id = event.GetId();
				if (id == ID_PRESET_RENAME) {
					rename_preset();
				}
				else if (id == ID_PRESET_DELETE) {
					delete_preset();
				}
				});

			wxPoint Pos;
			this->m_pRenameRemovePresetBtn->GetPosition(&Pos.x, &Pos.y);
			this->PopupMenu(menu, wxPoint(Pos.x - 94, Pos.y + 35));
		});


		wxImage saveAllBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("save_btn.png")), wxBITMAP_TYPE_PNG);		
		saveAllBtnImage.Rescale(20, 20, wxIMAGE_QUALITY_HIGH);
#ifndef __APPLE__
		m_pSaveAllBtn = new wxButton(m_contentWidget, wxID_ANY);
		m_pSaveAllBtn->SetBitmap(saveAllBtnImage);
#else
		m_pSaveAllBtn = new ScalableButton(m_contentWidget, wxID_ANY, "save_btn", "", AnkerSize(20, 20));
#endif // !__APPLE__
		m_pSaveAllBtn->SetWindowStyleFlag(wxBORDER_NONE);
		m_pSaveAllBtn->SetBackgroundColour(ParameterPanelBgColor);
		m_pSaveAllBtn->SetMinSize(AnkerSize(20, 20));
		m_pSaveAllBtn->SetMaxSize(AnkerSize(20, 20));
		m_pSaveAllBtn->SetSize(AnkerSize(20, 20));
		m_pSaveAllBtn->SetCursor(handCursor);
		m_pSaveAllBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			if (saveAllUi())
			{
				onResetBtnStatusChanged(false);
				hideAllResetBtn();
			}
			});

		m_pDividingLine = new wxPanel(m_contentWidget, wxID_ANY);
		m_pDividingLine->SetBackgroundColour(wxColour("#38393C"));
		m_pDividingLine->SetMinSize(AnkerSize(335, 1));

		pParameterHSizer->AddSpacer(12);
		pParameterHSizer->Add(m_currentObjName, wxALL | wxEXPAND, wxALL | wxEXPAND|wxLEFT, 3);
		pParameterHSizer->Add(m_pPresetParameterComBox,  wxEXPAND, wxALL , 0);
		pParameterHSizer->Add(m_pRenameRemovePresetBtn, 0 , wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT|wxRIGHT, 3);

		pParameterHSizer->Add(m_pResetBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 3);
		pParameterHSizer->Add(m_pSaveAllBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT , 3);
		pParameterHSizer->AddSpacer(10);

		m_contentSizer->AddSpacer(10);
		m_contentSizer->Add(pParameterHSizer, 0, wxALL | wxEXPAND, 0);
		m_contentSizer->AddSpacer(10);
		m_contentSizer->Add(m_pDividingLine, 0, wxEXPAND, 0);
	}

	//custom general model
	{
		// search input widget
		{
			wxColour ParameterPanelBgColor = wxColour(41, 42, 45);
			wxColour textColor = wxColour("#FFFFFF");

			m_popupWidget = new AnkerPopupWidget(m_contentWidget);
			//popupWidget->AddItem(m_parameterData.m_gSearchMap); // do AddItem() in initSearchPopupWidget()

			m_popupWidget->Bind(wxCUSTOMEVT_ANKER_ITEM_CLICKED, [this](wxCommandEvent& event) {   // todo
			
				ANKER_LOG_INFO << "show search panel";
				wxVariant* pData = (wxVariant*)event.GetClientData();
				if (pData)
				{
					wxVariantList list = pData->GetList();
					auto tab = list[0]->GetString().ToStdString();
					auto optionKey = list[1]->GetString().ToStdString();
					auto OptionLabel = list[2]->GetString().ToStdString();
					m_pSearchTextCtrl->SetValue(OptionLabel);
					switchToOption(tab, optionKey);
					wxString data = "";
				}

				if (m_popupWidget)
					m_popupWidget->Hide();
				this->m_pSearchBtn->Show();
				this->m_pTabBtnScrolledWindow->Show();
				this->m_pSearchEdit->Show(false);
				this->Layout();
				this->Refresh();

				});
			m_popupWidget->SetBackgroundColour(wxColour("#00FF00"));
			m_popupWidget->SetMaxSize(AnkerSize(PARAMETER_PANEL_WITH - 35, 368));
			m_popupWidget->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH - 35, 368));
			m_popupWidget->SetSize(AnkerSize(PARAMETER_PANEL_WITH - 35, 368));

			m_pSearchEdit = new wxControl(m_contentWidget, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
			m_pSearchEdit->SetBackgroundColour(ParameterPanelBgColor);
			m_pSearchEdit->SetForegroundColour(textColor);
			m_pSearchEdit->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 24));

			wxBoxSizer* pSearchHSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pSearchEdit->SetSizer(pSearchHSizer);

			auto pSearchIcon = new ScalableButton(m_pSearchEdit, wxID_ANY, "search_btn", "", AnkerSize(20, 20));
			pSearchIcon->SetWindowStyleFlag(wxBORDER_NONE);
			pSearchIcon->SetBackgroundColour(ParameterPanelBgColor);
			pSearchIcon->SetMinSize(AnkerSize(20, 20));
			pSearchIcon->SetMaxSize(AnkerSize(20, 20));
			pSearchIcon->SetSize(AnkerSize(20, 20));


			m_pSearchTextCtrl = new AnkerLineEdit(m_pSearchEdit, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);			
			m_pSearchTextCtrl->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 24));
			m_pSearchTextCtrl->SetBackgroundColour(ParameterPanelBgColor);
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

				std::map<wxString, std::map<wxString, wxString>> searchResMap;  // {tab,{optoinKey, optionLabel}}
				getSearResMap(searchResMap,searchData);
				m_popupWidget->showResMap(searchResMap);
				m_popupWidget->Thaw();
			});

			m_pSearchTextCtrl->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this](wxCommandEvent& event) {
				if (m_popupWidget)
					m_popupWidget->Hide();
					this->m_pSearchBtn->Show();
					this->m_pTabBtnScrolledWindow->Show();
					this->m_pSearchEdit->Show(false);
					this->Layout();
					this->Refresh();

				});
			
			pSearchHSizer->Add(pSearchIcon, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
			pSearchHSizer->AddSpacer(4);
			pSearchHSizer->Add(m_pSearchTextCtrl, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL | wxALL,wxALL|wxEXPAND, 0);
			pSearchHSizer->AddSpacer(4);
		}
		m_contentSizer->Add(m_pSearchEdit, 0, wxEXPAND | wxALL, 12);
	}

	// tabs & search btn	
	{
		wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL); 
		{
			//tab HORIZONTAL scroll bar, 
			wxBoxSizer* pScrolledHWinSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pTabBtnScrolledWindow = new wxScrolledWindow(m_contentWidget, wxID_ANY);
			m_pTabBtnScrolledWindow->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH - 30, 50));
			m_pTabBtnScrolledWindow->SetSize(AnkerSize(PARAMETER_PANEL_WITH - 30, 50));
			m_pTabBtnScrolledWindow->SetSizer(pScrolledHWinSizer);
			m_pTabBtnScrolledWindow->SetBackgroundColour(ParameterPanelBgColor);

			m_pTabBtnScrolledWindow->SetVirtualSize(AnkerSize(PARAMETER_PANEL_WITH - 30, 31));
			m_pTabBtnScrolledWindow->SetScrollRate(20, 0);
			m_pTabBtnScrolledWindow->SetScrollbars(20, 0, PARAMETER_PANEL_WITH / 20, 1);

			wxString btnBGcolor = "#292A2D";
			wxString btnNormalTextColor = "#A9AAAB";
			wxString btnNormalColor = "#292A2D";
			wxString textSelectColor = "#62D361";
			wxString hoverColor = "#324435";

			ParamTabTextVec.clear();
			ParamTabTextVec.push_back(_L("Quality"));
			ParamTabTextVec.push_back(_L("Strength"));
			ParamTabTextVec.push_back(_L("Speed"));
			ParamTabTextVec.push_back(_L("Support"));
			ParamTabTextVec.push_back(_L("Other"));
			for (int i = 0; i < ParamTabTextVec.size(); i++)
			{
				AnkerChooseBtn* pTabBtn = new AnkerChooseBtn(m_pTabBtnScrolledWindow, wxID_ANY, ParamTabTextVec[i], btnNormalTextColor, ANKER_FONT_NO_1, btnBGcolor);
				wxCursor handCursor(wxCURSOR_HAND);
				pTabBtn->SetCursor(handCursor);
				pTabBtn->Bind(wxCUSTOMEVT_ANKER_CHOOSEBTN_CLICKED, &AnkerParameterPanel::onBtnClicked, this);
				m_tabBtnVector.push_back(pTabBtn);
				pTabBtn->setTextSelectColor(textSelectColor);
				pTabBtn->setNormalBGColor(btnNormalColor);
				pTabBtn->setHoverBGColor(hoverColor);
				pTabBtn->setTextSelectFont(ANKER_BOLD_FONT_NO_1);
				//defaut to select the first tab
				if (i == 0)
				{
					pTabBtn->setBtnStatus(ChooseBtn_Select);
				}
				else
				{
					pTabBtn->setBtnStatus(ChooseBtn_Normal);
				}
				pScrolledHWinSizer->Add(pTabBtn, 0, wxALIGN_CENTER | wxBOTTOM, 5);
			}

			tabSizer->AddSpacer(12);
			tabSizer->Add(m_pTabBtnScrolledWindow, 1, wxALL | wxEXPAND, 0);
		}

		// search btn 
		m_pSearchBtn = new ScalableButton(m_contentWidget, wxID_ANY, "search_btn", "", AnkerSize(20, 20));
		m_pSearchBtn->SetWindowStyleFlag(wxBORDER_NONE);
		m_pSearchBtn->SetBackgroundColour(ParameterPanelBgColor);
		m_pSearchBtn->SetMinSize(AnkerSize(20, 20));
		m_pSearchBtn->SetMaxSize(AnkerSize(20, 20));
		m_pSearchBtn->SetSize(AnkerSize(20, 20));
		m_pSearchBtn->SetCursor(wxCursor(wxCURSOR_HAND));
		m_pSearchBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
#ifndef __APPLE__
			this->m_pSearchTextCtrl->SetFocus();
#else
			//this->m_pSearchTextCtrl->SetFocus(); // mac os: m_pSearchTextCtrl would receive wxEVT_KILL_FOCUS event and then sending wxCUSTOMEVT_EDIT_FINISHED 
			static wxTimer timer;
			timer.Bind(wxEVT_TIMER, [this](wxTimerEvent& event) {this->m_pSearchTextCtrl->SetFocus(); }, wxID_ANY);
			timer.StartOnce(200);
#endif // !__APPLE__
			this->m_pTabBtnScrolledWindow->Hide();
			this->m_pSearchBtn->Hide();
			this->m_pSearchEdit->Show();
			this->Layout();
			this->Refresh();
			});

		tabSizer->Add(m_pSearchBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		tabSizer->AddSpacer(10);
		m_contentSizer->Add(tabSizer, wxALL | wxHORIZONTAL, wxALL | wxEXPAND, 0);
	}

	m_pTabItemScrolledWindow = new wxScrolledWindow(m_contentWidget, wxID_ANY);
	m_pTabItemScrolledWindow->SetScrollRate(0, 50);

	m_pTabItemScrolledWindow->SetVirtualSize(AnkerSize(PARAMETER_PANEL_WITH - 30, 500));
	m_pTabItemScrolledWindow->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH,-1));
	m_pTabItemScrolledWindow->SetSize(AnkerSize(PARAMETER_PANEL_WITH,-1));
	m_pTabItemScrolledWindow->SetScrollbars(0, 30, PARAMETER_PANEL_WITH / 50, 500 / 50);
	m_pTabItemScrolledVWinSizer = new wxBoxSizer(wxVERTICAL);
	m_pTabItemScrolledWindow->SetSizer(m_pTabItemScrolledVWinSizer);	
	m_pTabItemScrolledWindow->SetBackgroundColour(ParameterPanelBgColor);


	// tab: Quality
	{
		wxString strTab = _L("Quality");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;

		// group: Layer height 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Layer_height", _L("Layer height"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem(("layer_height"), ItemEditUinit, Item_float, unitList);	// option:layer_height
			//pItemGroup->createItem(("variable_layer_height"), ItemCheckBox, Item_bool);    // variable_layer_height
			pItemGroup->createItem(("first_layer_height"), ItemEditUinit, Item_floatOrPercent, unitList);		// first_layer_height
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		// group: Width 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Width", _L("Line width"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.clear();
			unitList.Add(_L("mm or %"));
			pItemGroup->createItem(("extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);  // extrusion_width
			pItemGroup->createItem(("first_layer_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);  // first_layer_extrusion_width   // todo label repeate
			pItemGroup->createItem(("external_perimeter_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);	// option:external_perimeter_extrusion_width
			pItemGroup->createItem(("perimeter_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);	// perimeter_extrusion_width
			pItemGroup->createItem("top_infill_extrusion_width", ItemEditUinit, Item_floatOrPercent, unitList);   //  top_infill_extrusion_width
			pItemGroup->createItem(("infill_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);		//infill_extrusion_width
			pItemGroup->createItem(("solid_infill_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);	// solid_infill_extrusion_width
			pItemGroup->createItem(("support_material_extrusion_width"), ItemEditUinit, Item_floatOrPercent, unitList);	// support_material_extrusion_width
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		// group: Seam 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Seam", _L("Seam"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			m_parameterData.getItemList(dataList, List_Seam_Position);
			pItemGroup->createItem(("seam_position"), ItemComBox, Item_enum_SeamPosition, dataList);	// option:seam_position		//todo:dataList not load
			pItemGroup->createItem(("staggered_inner_seams"), ItemCheckBox, Item_bool);	// staggered_inner_seams
			unitList.Clear();
			unitList.Add(_L("mm or %"));
			pItemGroup->createItem(("seam_gap"), ItemEditUinit, Item_floatOrPercent, unitList);	// seam_gap
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		// group: Precision  
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Precision", _L("Precision"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem(("slice_closing_radius"), ItemEditUinit, Item_float, unitList);	// slice_closing_radius
			pItemGroup->createItem(("resolution"), ItemEditUinit, Item_float, unitList);	// resolution
			pItemGroup->createItem(("xy_hole_compensation"), ItemEditUinit, Item_float, unitList);	// xy_hole_compensation
			pItemGroup->createItem(("xy_size_compensation"), ItemEditUinit, Item_float, unitList);  // xy_size_compensation
			pItemGroup->createItem(("elefant_foot_compensation"), ItemEditUinit, Item_float, unitList); // elefant_foot_compensation
			pItemGroup->createItem(("precise_outer_wall"), ItemCheckBox, Item_bool);	// precise_outer_wall
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		// group: Ironing 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Ironing", _L("Ironing"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			m_parameterData.getItemList(dataList, List_Ironing_Type);
			pItemGroup->createItem(("ironing_type"), ItemComBox, Item_enum_IroningType, dataList);  // ironing_type
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("ironing_speed"), ItemEditUinit, Item_float, unitList);	// ironing_speed
			unitList.Clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem(("ironing_spacing"), ItemEditUinit, Item_float, unitList);  // ironing_spacing
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		// group: Perimeter generator 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Perimeter_generator", _L("Perimeter generator"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			m_parameterData.getItemList(dataList, List_Perimeter_generator);
			pItemGroup->createItem(("perimeter_generator"), ItemComBox, Item_enum_PerimeterGeneratorType, dataList); // perimeter_generator
			unitList.Clear();
			unitList.Add(_L("°"));
			pItemGroup->createItem(("wall_transition_angle"), ItemEditUinit, Item_float, unitList); // wall_transition_angle
			unitList.Clear();
			unitList.Add(_L("mm or %"));
			pItemGroup->createItem(("wall_transition_filter_deviation"), ItemEditUinit, Item_floatOrPercent, unitList);	// wall_transition_filter_deviation
			pItemGroup->createItem(("wall_transition_length"), ItemEditUinit, Item_floatOrPercent, unitList);	// wall_transition_length
			pItemGroup->createItem(("wall_distribution_count"), ItemSpinBox, Item_int);		// wall_distribution_count
			pItemGroup->createItem(("min_bead_width"), ItemEditUinit, Item_floatOrPercent, unitList);	// min_bead_width
			pItemGroup->createItem(("min_feature_size"), ItemEditUinit, Item_floatOrPercent, unitList);	// min_feature_size
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this); 
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}


		// group: Walls and surfaces 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Walls_and_surfaces", _L("Walls and Surfaces"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			pItemGroup->createItem(("infill_first"), ItemCheckBox, Item_bool);	// infill_first
			pItemGroup->createItem(("top_infill_flow_ratio"), ItemEditUinit, Item_float);	// top_infill_flow_ratio
			m_parameterData.getItemList(dataList, List_Top_surface_single_perimeter);
			//pItemGroup->createItem(("top_surface_single_perimeter"), ItemComBox, Item_enum_TopSurfaceSinglePerimeter, dataList); //  top_surface_single_perimeter   // feature opon later
			//pItemGroup->createItem(("extrusion_multiplier"), ItemEditUinit, Item_float);	// extrusion_multiplier   // todo : this item crash
			pItemGroup->createItem(("avoid_crossing_perimeters"), ItemCheckBox, Item_bool);	// avoid_crossing_perimeters

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}


		// group:Bridge
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Bridge", _L("common_Parameter_optionGroup_Bridging"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			pItemGroup->createItem(("bridge_flow_ratio"), ItemEditUinit, Item_float); //bridge_flow_ratio
			pItemGroup->createItem(("bridge_infill_density"), ItemEditUinit, Item_Percent); //bridge_infill_density

			

			pItemGroup->createItem(("thick_bridges"), ItemCheckBox, Item_bool); //thick_bridges

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}


		// group:Overhangs
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Overhangs", _L("common_Parameter_optionGroup_Overhang"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			//pItemGroup->createItem(("make_overhang_printable"), ItemCheckBox, Item_bool); // make_overhang_printable   // feature open later
			pItemGroup->createItem(("extra_perimeters_on_overhangs"), ItemCheckBox, Item_bool); // extra_perimeters_on_overhangs

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 4));
		}

		m_windowTabMap.insert(std::make_pair(strTab, itemList));
		std::list<AnkerPrintParaItem*>::iterator iter = itemList.begin();
		for (; iter != itemList.end(); iter++)
		{
			std::map<wxString, PARAMETER_GROUP> groupInfoMap = (*iter)->GetGroupparamInfos();
			AnkerParameterPanel::m_AllPrintParamInfo.insert(groupInfoMap.begin(), groupInfoMap.end());
		}
	}

	// tab: Strength 
	{
		wxString strTab = _L("Strength");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;

		// group:Perimeters   
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, 
																	"OG_Perimeters",
																	_L("common_Parameter_optionGroup_WallsAndSurfaces"),
																	strTab, 
																	m_PrintParamMode,
																	wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			pItemGroup->createItem(("perimeters"), ItemSpinBox, Item_int);	// perimeters // todo repeate lable
			//pItemGroup->createItem(("alternate_extra_wall"), ItemCheckBox, Item_bool); //staggered_inner_seams
			pItemGroup->createItem(("thin_walls"), ItemCheckBox, Item_bool); // thin_walls

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}


		// group:Top/Bottom  
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Top_Bottom", _L("common_Parameter_optionGroup_TopBottom"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			m_parameterData.getItemList(dataList, List_Top_fill_pattern);
			pItemGroup->createItem(("top_fill_pattern"), ItemComBox, Item_enum_InfillPattern, dataList);				//  top_fill_pattern
			pItemGroup->createItem(("top_solid_layers"), ItemSpinBox, Item_int); // top_solid_layers
			unitList.Clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem(("top_solid_min_thickness"), ItemEditUinit, Item_float, unitList); // top_solid_min_thickness
			m_parameterData.getItemList(dataList, List_Bottom_fill_pattern);
			pItemGroup->createItem(("bottom_fill_pattern"), ItemComBox, Item_enum_InfillPattern, dataList);  // bottom_fill_pattern
			pItemGroup->createItem(("bottom_solid_layers"), ItemSpinBox, Item_int); //bottom_solid_layers
			pItemGroup->createItem(("bottom_solid_min_thickness"), ItemEditUinit, Item_float, unitList); // bottom_solid_min_thickness

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}


		// group:Fill
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Fill", _L("Fill"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);

			m_parameterData.getItemList(dataList, List_Fill_density);
			pItemGroup->createItem(("fill_density"), ItemEditBox, Item_Percent, dataList); // fill_density
			m_parameterData.getItemList(dataList, List_Fill_pattern);
			pItemGroup->createItem(("fill_pattern"), ItemComBox, Item_enum_InfillPattern, dataList);  //fill_pattern
			m_parameterData.getItemList(dataList, List_Length_of_th_infill_anchor);
			pItemGroup->createItem(("infill_anchor"), ItemEditBox, Item_floatOrPercent, dataList); // infill_anchor
			m_parameterData.getItemList(dataList, List_Maximum_length_of_the_infill_anchor);
			pItemGroup->createItem(("infill_anchor_max"), ItemEditBox, Item_floatOrPercent, dataList); //infill_anchor_max

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}


		// group:Advance
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Advance", _L("Advance"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.clear();
			unitList.Add(_L("mm or %"));
			pItemGroup->createItem(("infill_overlap"), ItemEditUinit, Item_floatOrPercent, unitList);  // infill_overlap
			unitList.Clear();
			unitList.Add(_L("°"));
			pItemGroup->createItem(("fill_angle"), ItemEditUinit, Item_float, unitList); //fill_angle
			pItemGroup->createItem(("bridge_angle"), ItemEditUinit, Item_float, unitList); //bridge_angle
			unitList.Clear();
			unitList.Add(_L("mm²"));
			pItemGroup->createItem(("solid_infill_below_area"), ItemEditUinit, Item_float, unitList); //solid_infill_below_area
			pItemGroup->createItem(("infill_every_layers"), ItemSpinBox, Item_int); // infill_every_layers

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}

		m_windowTabMap.insert(std::make_pair(strTab, itemList));
		std::list<AnkerPrintParaItem*>::iterator iter = itemList.begin();
		for (; iter != itemList.end(); iter++)
		{
			std::map<wxString, PARAMETER_GROUP> groupInfoMap = (*iter)->GetGroupparamInfos();
			AnkerParameterPanel::m_AllPrintParamInfo.insert(groupInfoMap.begin(), groupInfoMap.end());
		}
	}

	// tab: Speed
	{
		wxString strTab = _L("Speed");
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;

		// group:First layer speed  
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_First_layer_speed", _L("First layer speed"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm/s or %"));
			pItemGroup->createItem(("first_layer_speed"), ItemEditUinit, Item_floatOrPercent, unitList);  // first_layer_speed
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("first_layer_travel_speed"), ItemEditUinit, Item_float, unitList);  // first_layer_travel_speed
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}

		// group:Other layer speed  
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Other_layer_speed", _L("common_Parameter_optionGroup_OtherLayerSpeed"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm/s or %"));
			pItemGroup->createItem(("external_perimeter_speed"), ItemEditUinit, Item_floatOrPercent, unitList); // external_perimeter_speed
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("perimeter_speed"), ItemEditUinit, Item_float, unitList); // perimeter_speed
			unitList.Clear();
			unitList.Add(_L("mm/s or %"));
			pItemGroup->createItem(("small_perimeter_speed"), ItemEditUinit, Item_floatOrPercent, unitList);	// small_perimeter_speed
			unitList.Clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem(("small_perimeter_radius"), ItemEditUinit, Item_float, unitList);// small_perimeter_radius
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("infill_speed"), ItemEditUinit, Item_float, unitList);	// infill_speed
			unitList.Clear();
			unitList.Add(_L("mm/s or %"));
			pItemGroup->createItem(("solid_infill_speed"), ItemEditUinit, Item_floatOrPercent, unitList);	// solid_infill_speed
			unitList.clear();
			unitList.Add(_L("mm or %"));
			pItemGroup->createItem("top_solid_infill_speed", ItemEditUinit, Item_floatOrPercent, unitList);		// top_solid_infill_speed
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("gap_fill_speed"), ItemEditUinit, Item_float, unitList);	// gap_fill_speed
			pItemGroup->createItem(("support_material_speed"), ItemEditUinit, Item_float, unitList);	// support_material_speed

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}


		// group:Overhang speed 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Overhang_speed", _L("Overhang speed"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(wxT("mm or %"));
			pItemGroup->createItem(("enable_dynamic_overhang_speeds"), ItemCheckBox, Item_bool); // overhangs
			unitList.Clear();
			unitList.Add(_L("mm/s or %"));
			pItemGroup->createItem(("overhang_speed_0"), ItemEditUinit, Item_floatOrPercent, unitList); // overhang_speed_0
			pItemGroup->createItem(("overhang_speed_1"), ItemEditUinit, Item_floatOrPercent, unitList);	// overhang_speed_1
			pItemGroup->createItem(("overhang_speed_2"), ItemEditUinit, Item_floatOrPercent, unitList);	// overhang_speed_2
			pItemGroup->createItem(("overhang_speed_3"), ItemEditUinit, Item_floatOrPercent, unitList);	// overhang_speed_3
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("bridge_speed"), ItemEditUinit, Item_float, unitList); // bridge_speed

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}


		// group:Travel speed 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Travel_speed", _L("Travel speed"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("travel_speed"), ItemEditUinit, Item_float, unitList); // travel_speed
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 2));
		}


		// group:Acceleration 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Acceleration", _L("Acceleration"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm/s²"));
			pItemGroup->createItem(("default_acceleration"), ItemEditUinit, Item_float, unitList);	// default_acceleration
			pItemGroup->createItem(("external_perimeter_acceleration"), ItemEditUinit, Item_float, unitList); // external_perimeter_acceleration
			pItemGroup->createItem(("perimeter_acceleration"), ItemEditUinit, Item_float, unitList); // perimeter_acceleration // todo check lable
			pItemGroup->createItem(("bridge_acceleration"), ItemEditUinit, Item_float, unitList);  // bridge_acceleration // todo check lable
			pItemGroup->createItem(("infill_acceleration"), ItemEditUinit, Item_float, unitList);	// infill_acceleration // todo check lable
			pItemGroup->createItem(("solid_infill_acceleration"), ItemEditUinit, Item_float, unitList);	// solid_infill_acceleration // todo check lable
			pItemGroup->createItem(("first_layer_acceleration"), ItemEditUinit, Item_float, unitList);		// first_layer_acceleration  // todo label repeate
			pItemGroup->createItem("top_solid_infill_acceleration", ItemEditUinit, Item_float, unitList);      //  top_solid_infill_acceleration
			pItemGroup->createItem(("travel_acceleration"), ItemEditUinit, Item_float, unitList);	// travel_acceleration

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 2));
		}


		// group:Jerk(XY) 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Jerk_XY", _L("common_Parameter_optionGroup_Jerk"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("mm/s"));
			pItemGroup->createItem(("jerk_outer_wall"), ItemEditUinit, Item_float, unitList);	// jerk_outer_wall
			pItemGroup->createItem(("jerk_inner_wall"), ItemEditUinit, Item_float, unitList);	// jerk_inner_wall
			pItemGroup->createItem(("jerk_infill"), ItemEditUinit, Item_float, unitList);	// jerk_infill  // todo lable repeate
			pItemGroup->createItem(("jerk_travel"), ItemEditUinit, Item_float, unitList);	// jerk_travel

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 2));
		}


		// group: Max volumetric slope 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Reducing_printing_time", _L("common_Parameter_optionGroup_PressureEqualizer"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			//pItemGroup->createItem(("external_perimeters_first"), ItemCheckBox, Item_bool);	// external_perimeters_first
			unitList.Clear();
			unitList.Add(_L("mm³/s²"));
			pItemGroup->createItem(("max_volumetric_extrusion_rate_slope_positive"), ItemEditUinit, Item_float, unitList);	// max_volumetric_extrusion_rate_slope_positive
			pItemGroup->createItem(("max_volumetric_extrusion_rate_slope_negative"), ItemEditUinit, Item_float, unitList);	// max_volumetric_extrusion_rate_slope_negative
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxBottom, 2));
		}

		m_windowTabMap.insert(std::make_pair(strTab, itemList));
		std::list<AnkerPrintParaItem*>::iterator iter = itemList.begin();
		for (; iter != itemList.end(); iter++)
		{
			std::map<wxString, PARAMETER_GROUP> groupInfoMap = (*iter)->GetGroupparamInfos();
			AnkerParameterPanel::m_AllPrintParamInfo.insert(groupInfoMap.begin(), groupInfoMap.end());
		}
	}


	// tab: Support
	{
		wxString strTab = ParamTabTextVec[3];
		std::list<AnkerPrintParaItem*> itemList;
		wxStringList unitList;

		// group:Support
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Support", _L("Support material"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			pItemGroup->createItem("support_material", ItemCheckBox, Item_bool);
			pItemGroup->createItem(("support_material_auto"), ItemCheckBox, Item_bool);	//support_material_auto

			pItemGroup->createItem(("support_material_threshold"), ItemSpinBox, Item_int);	// support_material_threshold
			pItemGroup->createItem("support_material_enforce_layers", ItemSpinBox, Item_int);


			unitList.Clear();
			unitList.Add(_L("%"));
			pItemGroup->createItem(("raft_first_layer_density"), ItemEditUinit, Item_Percent, unitList); // raft_first_layer_density
			pItemGroup->createItem(("raft_first_layer_expansion"), ItemEditUinit, Item_float, unitList);	// raft_first_layer_expansion

			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}

		// group:Raft 
		{
			AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Raft", _L("Raft"), strTab, m_PrintParamMode, wxID_ANY);
			pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);
			pItemGroup->createItem(("raft_layers"), ItemSpinBox, Item_int); // raft_layers
			unitList.Clear();
			unitList.Add(_L("mm"));
			pItemGroup->createItem("raft_contact_distance", ItemEditUinit, Item_float, unitList);
			pItemGroup->createItem("raft_expansion", ItemEditUinit, Item_float, unitList);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pItemGroup);
			m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}

		//Options for support material and raft
		{
			AnkerPrintParaItem* pOfsmarItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Options_for_support_material_and_raft", _L("Options for support material and raft"), strTab, m_PrintParamMode, wxID_ANY);
			pOfsmarItem->SetMaxSize(PARAMETER_ITEM_SIZE);
			getItemList(dataList, List_Style);
			pOfsmarItem->createItem("support_material_style", ItemComBox, Item_enum_SupportMaterialStyle, dataList);
			getItemList(dataList, List_Top_contact_Z_distance);
			pOfsmarItem->createItem("support_material_contact_distance", ItemEditBox, Item_float, dataList);//specail handle
			getItemList(dataList, List_Bottom_contact_Z_distance);
			pOfsmarItem->createItem("support_material_bottom_contact_distance", ItemEditBox, Item_float, dataList);
			getItemList(dataList, List_Pattern);
			pOfsmarItem->createItem("support_material_pattern", ItemComBox, Item_enum_SupportMaterialPattern, dataList);
			pOfsmarItem->createItem("support_material_with_sheath", ItemCheckBox, Item_bool);
			pOfsmarItem->createItem("support_material_spacing", ItemEditUinit, Item_float, unitList);
			unitList.Clear();
			unitList.Add(_L("°"));
			pOfsmarItem->createItem("support_material_angle", ItemEditUinit, Item_float, unitList);
			unitList.Clear();
			unitList.Add(_L("mm"));
			pOfsmarItem->createItem("support_material_closing_radius", ItemEditUinit, Item_float, unitList);
			getItemList(dataList, List_Top_interface_layers);
			pOfsmarItem->createItem("support_material_interface_layers", ItemEditBox, Item_int, dataList);
			getItemList(dataList, List_Bottom_interface_layers);
			pOfsmarItem->createItem("support_material_bottom_interface_layers", ItemEditBox, Item_int, dataList);
			getItemList(dataList, List_interface_pattern);
			pOfsmarItem->createItem("support_material_interface_pattern", ItemComBox, Item_enum_SupportMaterialInterfacePattern, dataList);
			pOfsmarItem->createItem("support_material_interface_spacing", ItemEditUinit, Item_float, unitList);
			pOfsmarItem->createItem("support_material_interface_contact_loops", ItemCheckBox, Item_bool);
			pOfsmarItem->createItem("support_material_buildplate_only", ItemCheckBox, Item_bool);
			unitList.Clear();
			unitList.Add(_L("mm or %"));
			pOfsmarItem->createItem("support_material_xy_spacing", ItemEditUinit, Item_floatOrPercent, unitList);
			pOfsmarItem->createItem("dont_support_bridges", ItemCheckBox, Item_bool);
			pOfsmarItem->createItem("support_material_synchronize_layers", ItemCheckBox, Item_bool);
			pOfsmarItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pOfsmarItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pOfsmarItem);
			m_pTabItemScrolledVWinSizer->Add(pOfsmarItem, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}
		
		// Organic supports 
		{
			AnkerPrintParaItem* pOssmarItem = new AnkerPrintParaItem(m_pTabItemScrolledWindow, "OG_Organic_supports", _L("Organic supports"), strTab, m_PrintParamMode, wxID_ANY);
			pOssmarItem->SetMaxSize(PARAMETER_ITEM_SIZE);
			unitList.Clear();
			unitList.Add(_L("°"));
			pOssmarItem->createItem("support_tree_angle", ItemEditUinit, Item_float, unitList);
			pOssmarItem->createItem("support_tree_angle_slow", ItemEditUinit, Item_float, unitList);
			unitList.Clear();
			unitList.Add(_L("mm"));
			pOssmarItem->createItem("support_tree_branch_diameter", ItemEditUinit, Item_float, unitList);
			unitList.Clear();
			unitList.Add(_L("°"));
			pOssmarItem->createItem("support_tree_branch_diameter_angle", ItemEditUinit, Item_float, unitList);
			unitList.Clear();
			unitList.Add(_L("mm"));
			pOssmarItem->createItem("support_tree_tip_diameter", ItemEditUinit, Item_float, unitList);
			pOssmarItem->createItem("support_tree_branch_distance", ItemEditUinit, Item_float);
			unitList.Clear();
			unitList.Add(_L("%"));
			pOssmarItem->createItem("support_tree_top_rate", ItemEditUinit, Item_Percent, unitList);
			pOssmarItem->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
			pOssmarItem->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
			itemList.push_back(pOssmarItem);
			m_pTabItemScrolledVWinSizer->Add(pOssmarItem, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		}
		m_windowTabMap.insert(std::make_pair(strTab, itemList));
		std::list<AnkerPrintParaItem*>::iterator iter = itemList.begin();
		for (; iter != itemList.end(); iter++)
		{
			std::map<wxString, PARAMETER_GROUP> groupInfoMap = (*iter)->GetGroupparamInfos();
			AnkerParameterPanel::m_AllPrintParamInfo.insert(groupInfoMap.begin(), groupInfoMap.end());
		}
	}


	// tab: Oters
	{
		wxString strTab = ParamTabTextVec[4];
		std::list<AnkerPrintParaItem*> itemList;
		//group => Bed Adhension
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Bed Adhension");
			groupItemVal.strIconName = "OG_Bed_Adhension";
			std::vector<paramItem> paramVec = {
				{"skirts",ItemSpinBox, Item_int, {}},
				{"skirt_distance",ItemEditUinit, Item_float, { _L("mm")}},
				{"skirt_height",ItemSpinBox, Item_int, {}},
				{"brim_type",ItemComBox, Item_enum_BrimType, {_L("Auto"),_L("engin_brim_type_mouse_ear"),_L("No brim"),_L("Outer brim only"),_L("Inner brim only"),_L("Outer and inner brim")}},
				{"brim_width", ItemEditUinit, Item_float,{_L("mm")}},
				{"brim_separation", ItemEditUinit, Item_float,{_L("mm")}},
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}

		//group => Wipe 
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Prime tower");
			groupItemVal.strIconName = "OG_Wipe";
			std::vector<paramItem> paramVec = {
				{"wipe_tower", ItemCheckBox, Item_bool,{}},
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}

		//group => Special mode  
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Special mode");
			groupItemVal.strIconName = "OG_Layer_height";
			std::vector<paramItem> paramVec = {
				{"slicing_mode",ItemComBox, Item_enum_SlicingMode,{_L("Regular"),_L("Even-odd"),_L("Close holes")}},
				{"complete_objects", ItemCheckBox, Item_bool,{}},
				{"spiral_vase", ItemCheckBox, Item_bool,{}},
				{"fuzzy_skin",  ItemComBox, Item_enum_FuzzySkinType,{_L("None"),_L("Outside walls"),_L("All walls")}},	
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}


		//group => Advance
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Advance");
			groupItemVal.strIconName = "OG_Advance";
			std::vector<paramItem> paramVec = {
				{"mmu_segmented_region_max_width",ItemEditUinit, Item_float,{_L("%")}},
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}

		//group => G-code output
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("G-code output");
			groupItemVal.strIconName = "OG_G-code_output";
			GroupParamUIConfig paramUiConfig = { false,true,140,185,false };
			std::vector<paramItem> paramVec = {
				{"gcode_comments",ItemCheckBox, Item_bool,{}},// todo
				{"gcode_label_objects",ItemCheckBox, Item_bool,{}},// todo
				{"output_filename_format",ItemTextCtrl, Item_serialize,{},paramUiConfig},// todo
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}

		//group => Post - processing scripts
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Post-processing scripts");
			groupItemVal.strIconName = "OG_Post-processing_scripts";
			GroupParamUIConfig paramUiConfig = { true,true,50,200,true };
			std::vector<paramItem> paramVec = {
			  {"post_process",ItemTextCtrl, Item_Multi_Strings,{},paramUiConfig},// todo
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}

		//group => Notes
		{
			groupItem groupItemVal;
			groupItemVal.strGroupName = _L("Notes");
			groupItemVal.strIconName = "OG_Notes";
			
			GroupParamUIConfig paramUiConfig = {true,true,50,200,false};

			std::vector<paramItem> paramVec = {
			{"notes",ItemTextCtrl, Item_serialize,{},paramUiConfig},// todo
			};
			groupItemVal.paramVec = paramVec;
			itemList.push_back(createGroupItem(groupItemVal));
		}
		m_windowTabMap.insert(std::make_pair(strTab, itemList));
		std::list<AnkerPrintParaItem*>::iterator iter = itemList.begin();
		for (; iter != itemList.end(); iter++)
		{
			std::map<wxString, PARAMETER_GROUP> groupInfoMap = (*iter)->GetGroupparamInfos();
			m_AllPrintParamInfo.insert(groupInfoMap.begin(), groupInfoMap.end());
		}
	}


	m_contentSizer->Add(m_pTabItemScrolledWindow, wxEXPAND | wxHORIZONTAL, wxALL | wxEXPAND, 0);
	m_contentSizer->AddSpacer(8);
	m_contentSizer->AddStretchSpacer(1);

	m_pBtnPanel = new wxPanel(m_contentWidget);
	
	m_pBtnPanel->SetMaxSize(AnkerSize(-1, 40));	
	m_pBtnPanel->SetMinSize(AnkerSize(-1, 40));	
	m_pBtnPanel->SetSize(AnkerSize(-1, 40));	
	wxBoxSizer* pPanelHsizer = new wxBoxSizer(wxHORIZONTAL);
	m_pBtnPanel->SetSizer(pPanelHsizer);
	pPanelHsizer->AddSpacer(1);
	{
		if (!m_pSliceBtn)
		{
			m_pSliceBtn = new AnkerBtn(m_pBtnPanel, wxID_ANY);
			m_pSliceBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {				
				ANKER_LOG_INFO << "AnkerParameterPanel slice btn clicked.";
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

	m_contentSizer->AddStretchSpacer(10);
	m_contentSizer->Add(m_pBtnPanel, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALL | wxEXPAND, 0);

	m_contentWidget->SetSizer(m_contentSizer);
	m_pMainVSizer->Add(m_contentWidget, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALL| wxEXPAND, 0);

	SetSizer(m_pMainVSizer);
}

void AnkerParameterPanel::onEasyPanelItemClicked(wxString strItemName)
{
		wxString numTpData = strItemName;
		wxString moduleName;
		if (numTpData == _L("common_slicepannel_easy3_fast"))
			moduleName = "Fast";
		else if (numTpData == _L("common_slicepannel_easy2_normal"))
			moduleName = "Normal";
		else if (numTpData == _L("common_slicepannel_easy1_smooth"))
			moduleName = "Precision";

		wxString currentPresetName;
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
			this->Freeze();
			Slic3r::GUI::wxGetApp().getAnkerTab(Slic3r::Preset::TYPE_PRINT)->select_preset(currentPresetName.ToStdString());
			this->Thaw();
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
 	Layout();
}

// return: searchResMap {tab,{optoinKey, optionLabel}}
void AnkerParameterPanel::getSearResMap(std::map<wxString, std::map<wxString, wxString>>& searchResMap, const wxString& searchData)
{
	if (searchData.IsEmpty())
		return;

	searchResMap.clear();
	wxString searchDataEx = searchData;
	searchDataEx.MakeCapitalized();

	//std::map<wxString, std::vector<wxString>> tabOptionsMap;;
	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;    // groups
		std::map<wxString, wxString> keyLableMap;
		for (const auto& PrintParaItem : PrintParaItems) {
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto label = PrintParaItem->GetIOptionLabel(optionKey);
				label.MakeLower();
				if (!optionKey.empty() && !label.empty() && label.Contains(searchData)) {
					keyLableMap[optionKey] = label;
				}
			}
		}
		searchResMap[tabName] = keyLableMap;
	}
}

void AnkerParameterPanel::showEasyModel()
{
	this->Freeze();
	ANKER_LOG_INFO << "show easy model";
	m_pExpertModeBtn->SetState(false);
	updateEasyWidget();
	m_pEasyWidget->Show();
	m_pPresetParameterComBox->Hide();
	m_pResetBtn->Hide();
	m_pSaveAllBtn->Hide();
	m_pRenameRemovePresetBtn->Hide();
	m_pDividingLine->Hide();
	m_pTabBtnScrolledWindow->Hide();
	m_pTabItemScrolledWindow->Hide();
	m_pSearchEdit->Hide();
	m_pSearchBtn->Hide();
	Refresh();
	Layout();
	this->Thaw();
}

wxString AnkerParameterPanel::getSelectedPrintMode()
{
	auto list = getValidParameter();
	auto currentData = getCurrentModule(list);

	wxString strPrintModel;
	if (currentData.Contains("Normal"))
	{
		strPrintModel = _L("common_slicepannel_easy2_normal");
	}
	else if (currentData.Contains("Fast"))
	{
		strPrintModel = _L("common_slicepannel_easy3_fast");
	}
	else if (currentData.Contains("Precision"))
	{
		strPrintModel = _L("common_slicepannel_easy1_smooth");
	}

	return strPrintModel;
}

void AnkerParameterPanel::showExpertModel()
{
	//this->Freeze();
	wxWindowUpdateLocker updateLocker(this);
	ANKER_LOG_INFO << "show expert model";
	m_pExpertModeBtn->SetState(true);
	m_pEasyWidget->Hide();
		
	m_pPresetParameterComBox->Show();	
	m_pResetBtn->Hide(); 
	m_pSaveAllBtn->Show();
	m_pRenameRemovePresetBtn->Show();
	m_pDividingLine->Show();
	m_pTabBtnScrolledWindow->Show();
	m_pTabItemScrolledWindow->Show();

	m_pSearchEdit->Hide();
	m_pSearchBtn->Show();

	Refresh();
	Layout();
	//this->Thaw();
}

void AnkerParameterPanel::switchToOption(const wxString& strTab, const wxString& optionKey)   // 
{
	ANKER_LOG_INFO << "switch optionKey is " << optionKey.c_str();

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

			auto item = m_windowTabMap.find(strTab);
			if (item != m_windowTabMap.end())
			{
				auto itemList = item->second;
				auto itemIter = itemList.begin();
				while (itemIter != itemList.end())
				{
					if ((*itemIter)->isExistOption(optionKey))
					{
						(*itemIter)->showOptionHighlight(optionKey,wxColour("#62D361"));

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

void AnkerParameterPanel::initUiData(int iInitType/* = 0*/)
{
	wxWindowUpdateLocker updateLocker(this);
	hideAllResetBtn();
	initAllTabData(iInitType);
	auto e = wxCommandEvent();
	onDatachanged(e);
}

void AnkerParameterPanel::initSearchPopupWidget()
{
	if (!m_popupWidget)
		return;

	std::map<wxString, int> labelCntMap;
	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;    // groups
		for (const auto& PrintParaItem : PrintParaItems) {
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto label = PrintParaItem->GetIOptionLabel(optionKey);
				if (!label.empty())
					labelCntMap[label]++;
			}
		}
	}

	// if different option have same label, add group name to the search popop item 
	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;    // groups
		for (const auto& PrintParaItem : PrintParaItems) {
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto label = PrintParaItem->GetIOptionLabel(optionKey);
				wxString groupName = wxEmptyString;
				if (labelCntMap.find(label) != labelCntMap.end() && labelCntMap[label] > 1)
					groupName = PrintParaItem->getTitle();
				m_popupWidget->AddItem(tabName, optionKey, label, groupName);
			}
		}
	}
}

void AnkerParameterPanel::initAllTabData(int iInitType /*= 0*/)
{
	auto printConfig = Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
	auto printConfigDefault = Slic3r::GUI::wxGetApp().preset_bundle->prints.default_preset().config;
	if (m_isRightParameterPanel && iInitType == 0)
		printConfig = m_rightParameterCfg->getPrintCfg();

	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		//auto PrintParaItems = tabIter.second;    // groups
		for (const auto& PrintParaItem : tabIter.second) {
			const auto& optionKeyList = PrintParaItem->getOptionsList(); 
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto dataType = PrintParaItem->GetItemDataType(optionKey);
				auto unit = PrintParaItem->GetItemUnit(optionKey);
		

	/*			auto allKeys = printConfig.keys();
				auto it = std::find(allKeys.begin(), allKeys.end(), optionKey);
				if (it == allKeys.end())
				{
					ANKER_LOG_ERROR << "printConfig have no this option key:" << optionKey << ", init failed.";
					continue;
				}*/

				try 
				{
					//update dependency info when change  print presets
					PARAMETER_GROUP& goupInfo = PrintParaItem->GetParamDepenencyRef(optionKey);
					PrintParaItem->SetParamDepedency(goupInfo,1);
				}
				catch (const std::runtime_error& e)
				{
					ANKER_LOG_ERROR << "GetParamDepenencyRef funtion throw Exception: " << e.what();
				}
				wxVariant option_value;
				std::string option_value_default;
				switch (dataType)
				{
					case Item_bool:
					{
						option_value = (printConfig.opt_bool(optionKey));
						break;
					}
					case Item_int:
					{
						option_value = std::to_string(printConfig.opt_int(optionKey));
						break;
					}
					case Item_float:
					{
						// todo : "Top contact Z distance"  and " " need special proccessing?
						option_value =  printConfig.opt_float(optionKey);
						break;
					}
					case Item_Percent:
					{
						//auto option_value_tmp = printConfig.opt_serialize(optionKey);
						option_value = printConfig.opt_serialize(optionKey);
						//option_value_default = printConfigDefault.opt_serialize(optionKey);
						/*if (!unit.empty()) {
							size_t pos = option_value_tmp.find_last_of('%');
							if (unit == "%" && (pos != std::string::npos)) {
								option_value_tmp.erase(pos, 1);
							}
							else {
								ANKER_LOG_ERROR << "Item_Percent option value have unit but unit is not '%', please check, option_key=" << optionKey;
							}
						}
						option_value = option_value_tmp;
						option_value_default = option_value_tmp;*/
						break;
					}
					case Item_floatOrPercent:
					{
						if (optionKey == "infill_anchor" || optionKey == "infill_anchor_max") {
							auto option_value_tmp = printConfig.opt_serialize(optionKey);
							wxString dataValue = option_value_tmp;
							{
								if (option_value_tmp == "0" && optionKey == "infill_anchor")
									dataValue = _L("0 (no open anchors)");
								else if (option_value_tmp == "0" && optionKey == "infill_anchor")
									dataValue = _L("0 (not anchored)");
								else if (option_value_tmp == "1000")
									dataValue = _L("1000 (unlimited)");
								else
									dataValue = dataValue + " " + _L("mm");
							}
							option_value = dataValue;
						}
						else
						{
							option_value = printConfig.opt_serialize(optionKey);
						}
						break;
					}
					case Item_serialize: // not item is create with this type
					{
						option_value = wxVariant((printConfig.opt_string(optionKey)));
						break;
					}
					case Item_serialize_no_unit: // not item is create with this type
					{
						break;
					}
					case Item_serialize_num: // not item is create with this type
					{
						break;
					}
					case Item_enum_FuzzySkinType:
					{
						option_value = wxVariant((int)(printConfig.opt_enum<Slic3r::FuzzySkinType>(optionKey)));
						break;
					}
					case Item_enum_PerimeterGeneratorType:
					{
						option_value = wxVariant ((int)(printConfig.opt_enum<Slic3r::PerimeterGeneratorType>(optionKey)));
						break;
					}
					case Item_enum_InfillPattern:
					{
						std::string value = printConfig.opt_serialize(optionKey);
						std::locale loc;
						value[0] = std::toupper(value[0], loc);
						option_value = m_parameterData.m_fillPatternData[value].ToStdString();
						auto top_fill_pattern_valueDefault = printConfigDefault.opt_enum<Slic3r::InfillPattern>(optionKey);
						break;
					}
					case Item_enum_TopSurfaceSinglePerimeter:
					{
						// todo ,Item_enum_TopSurfaceSinglePerimeter is new add 
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::SinglePerimeterType>(optionKey));
						break;
					}
					case Item_enum_IroningType:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::IroningType>(optionKey));
						break;
					}
					case Item_enum_DraftShield:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::DraftShield>(optionKey));
						break;
					}
					case Item_enum_BrimType:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::BrimType>(optionKey));
						break;
					}
					case Item_enum_SupportMaterialStyle:
					{
						auto idx = printConfig.opt_enum<Slic3r::SupportMaterialStyle>(optionKey);
						option_value = wxVariant( m_parameterData.m_ReVerStyleMap[idx]);
						break;
					}
					case Item_enum_SupportMaterialPattern:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::SupportMaterialPattern>(optionKey));
						break;
					}
					case Item_enum_SupportMaterialInterfacePattern:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::SupportMaterialInterfacePattern>(optionKey));
						break;
					}
					case Item_enum_SeamPosition:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::SeamPosition>(optionKey));
						break;
					}
					case Item_enum_SlicingMode:
					{
						option_value = wxVariant((int)printConfig.opt_enum<Slic3r::SlicingMode>(optionKey));
						break;
					}
					case Item_Multi_Strings:
					{
						const auto* post_process = printConfig.opt<Slic3r::ConfigOptionStrings>(optionKey);
						if (post_process)
						{
							std::vector<std::string> postprocess_values = post_process->values;
							std::string showText = Slic3r::join_strings_with_newlines(postprocess_values);
							option_value = wxVariant(showText);
						}

						const auto* post_process_defult = printConfig.opt<Slic3r::ConfigOptionStrings>(optionKey);
						if (post_process_defult)
						{
							std::vector<std::string> postprocess_values = post_process_defult->values;
						}
						break;
					}


					default:
					{
						ANKER_LOG_ERROR << "option daty type unkonw, can't init, please check. option_key:"<< optionKey;
						break;
					}
				}
				setItemValueWithToolTips(_L(tabName), optionKey, option_value, option_value_default);
			}
		}
	}
}

void AnkerParameterPanel::getItemList(wxStringList& list, ControlListType listType)
{
	return m_parameterData.getItemList(list, listType);
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
	for (const auto& tabIter : m_windowTabMap) {
		auto tabName = tabIter.first;			 // tab
		auto PrintParaItems = tabIter.second;   // groups

		for (const auto& PrintParaItem : PrintParaItems) {   // AnkerPrintParaItem
			auto optionKeyList = PrintParaItem->getOptionsList();
			for (size_t i = 0; i < optionKeyList.GetCount(); i++) {
				auto optionKey = optionKeyList[i].ToStdString();
				auto dataType = PrintParaItem->GetItemDataType(optionKey);
				auto unit = PrintParaItem->GetItemUnit(optionKey);
				auto label = PrintParaItem->GetIOptionLabel(optionKey);
				if (m_isRightParameterPanel)
					printConfig = m_rightParameterCfg->getPrintCfg();    // todo : check this logic used for 


				auto allKeys = printConfig.keys();
				auto it = std::find(allKeys.begin(), allKeys.end(), optionKey);
				if (it == allKeys.end())
				{
					ANKER_LOG_ERROR << "=xxx= printConfig not contain config for key:" << optionKey << " , skip... ";
					continue;
				}
				ItemInfo ItemDataInfo = getItemValue(tabName, optionKey);
				auto propertyType = ItemDataInfo.paramDataType;	//emun 
				wxVariant propertyValue = ItemDataInfo.paramDataValue; //wxVariant
				std::string strTest = propertyValue;
				saveSetPresetPropertyValue(printConfig, ItemDataInfo, optionKey, label);
			}
		}
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

void AnkerParameterPanel::setItemValue(const wxString tabName, const wxString& optionKey, wxVariant data)
{	
	auto item = m_windowTabMap.find(tabName);

	if (item == m_windowTabMap.end())
		return;

	auto itemList = item->second;

	auto itemListIter = itemList.begin();
	while (itemListIter != itemList.end())
	{
		if ((*itemListIter)->isExistOption(optionKey))
		{
			(*itemListIter)->updateUi(optionKey, data);
			return;
		}
		++itemListIter;
	}
}

void AnkerParameterPanel::setItemValueWithToolTips(const wxString tabName, const wxString& optionKey, wxVariant data, const wxString& tipswxValue)
{	
	auto item = m_windowTabMap.find(tabName);
	if (item == m_windowTabMap.end())
		return;

	auto itemList = item->second;
	auto itemListIter = itemList.begin();
	while (itemListIter != itemList.end())
	{
		if ((*itemListIter)->isExistOption(optionKey))
		{
			(*itemListIter)->updateUi(optionKey, data, tipswxValue);
			return;
		}
		++itemListIter;
	}
}

void AnkerParameterPanel::setItemValueEx(const wxString tabName, const wxString& optionKey, wxVariant data,
	bool needReset)
{
	auto item = m_windowTabMap.find(tabName);

	if (item == m_windowTabMap.end())
		return;

	auto itemList = item->second;

	auto itemListIter = itemList.begin();
	while (itemListIter != itemList.end())
	{
		if ((*itemListIter)->isExistOption(optionKey))
		{
			(*itemListIter)->updateUi(optionKey, data);
			(*itemListIter)->showResetBtn(optionKey, needReset);
			return;
		}
		++itemListIter;
	}
}


// open "Support material" tab
void AnkerParameterPanel::openSupportMaterialPage(wxString itemName, wxString text)
{
	wxMouseEvent event;
	m_pExpertModeBtn->SetState(true);
	showExpertModel();
    //switch to support material tab
	for (auto button : m_tabBtnVector)
	{
		if (text.IsSameAs(button->getText()))
		{
			wxMouseEvent mouse_event;
			button->OnPressed(mouse_event);
		}
	}
}

ItemInfo AnkerParameterPanel::getItemValue(const wxString& tabStr, const wxString& optionKey)
{
	ItemInfo info;
	info.paramDataType = Item_UNKONWN;
	auto tabIter = m_windowTabMap.find(tabStr);
	if (tabIter != m_windowTabMap.end())
	{
		auto itemList = tabIter->second;

		auto itemIter = itemList.begin();
		while (itemIter != itemList.end())
		{
			if ((*itemIter)->isExistOption(optionKey))
			{
				return (*itemIter)->getWidgetValue(optionKey);
			}
			++itemIter;
		}
	}
	return info;
}

std::vector<std::string> AnkerParameterPanel::getValidParameter()
{
	std::vector<std::string> vaildList = {};
	//auto sysPresetList = Slic3r::GUI::wxGetApp().preset_bundle->prints.system_preset_names();
	wxArrayString strings = m_pPresetParameterComBox->GetStrings();

	bool isAdd = false;
	for (size_t i = 0; i < strings.GetCount(); i++)
	{
		std::string str = strings[i].ToStdString();		
		
		std::string strSuff = (" (" + _L("modified") + ")").ToUTF8().data();
		str = boost::algorithm::ends_with(str, strSuff) ?
			str.substr(0, str.size() - strSuff.size()) : str;
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
	
	// if exsit  "Normal - M5 0.4mm - PLA+ Basic"  should use the default print setting 
	if (std::find(list.begin(), list.end(), "Normal - M5 0.4mm - PLA+ Basic") != list.end())
	{
		return "Normal - M5 0.4mm - PLA+ Basic";
	}

	return list.at(0);
}

void AnkerParameterPanel::onDatachanged(wxCommandEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS);
	evt.SetEventObject(this);
	wxPostEvent(this, evt);
}

void AnkerParameterPanel::onUpdateResetBtn(wxCommandEvent& event)
{
	onResetBtnStatusChanged(hasDirtyData());
}

void AnkerParameterPanel::saveSetPresetPropertyValue(Slic3r::DynamicPrintConfig& printConfig,
													ItemInfo& ItemDataInfo,
													std::string& prusaProperty,
													wxString& AnkerPropertyName)
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
			if(prusaProperty == ("fill_pattern"))
				infillPattern = m_parameterData.m_fillPatternMap[propertyValue.GetInteger()];
			else
				infillPattern = m_parameterData.m_tabfillPatternMap[propertyValue.GetInteger()];

			Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::InfillPattern>(infillPattern);
			printConfig.set_key_value(prusaProperty, configOption);
			break;
		}
		case Item_enum_TopSurfaceSinglePerimeter:
		{

			Slic3r::SinglePerimeterType singlePerimeterType = (Slic3r::SinglePerimeterType)propertyValue.GetInteger();
			Slic3r::ConfigOptionEnum<Slic3r::SinglePerimeterType>* configOption = new Slic3r::ConfigOptionEnum<Slic3r::SinglePerimeterType>(singlePerimeterType);
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
			Slic3r::SupportMaterialStyle supportMaterialStyle = m_parameterData.m_StyleMap[propertyValue.GetInteger()];
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
			if (prusaProperty == ("first_layer_height"))
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
			std::string strValue = propertyValue;
 			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionString(strValue));
			break;
		}
		case Item_Percent:
		{		
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionPercent(propertyValue));
			break;
		}
		case Item_Multi_Strings:
		{
			std::vector<std::string> splitStr = Slic3r::split_string_by_carriage_return(propertyValue);
			printConfig.set_key_value(prusaProperty, new Slic3r::ConfigOptionStrings(splitStr));
			break;
		}

		default:
		{
			break;
		}
	}
}

void AnkerParameterPanel::saveSetPresetPropertyValueEx(Slic3r::DynamicPrintConfig* printConfig, ItemInfo& ItemDataInfo, std::string& prusaProperty, wxString& AnkerPropertyName)
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
			Slic3r::SupportMaterialStyle supportMaterialStyle = m_parameterData.m_StyleMap[propertyValue.GetInteger()];
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

void AnkerParameterPanel::onResetBtnStatusChanged(bool isAble)
{
	ANKER_LOG_INFO << "single control reset status";
	Freeze();
	if (isAble)
	{
		m_pResetBtn->Show();
	}
	else
	{
		m_pResetBtn->Hide();
	}

	Layout();
	Refresh();
	Thaw();
}

AnkerPrintParaItem* AnkerParameterPanel::createGroupItem(groupItem& pGroupItem)
{
		AnkerPrintParaItem* pItemGroup = new AnkerPrintParaItem(m_pTabItemScrolledWindow, pGroupItem.strIconName, pGroupItem.strGroupName, pGroupItem.strTabName, m_PrintParamMode, wxID_ANY);
		pItemGroup->SetMaxSize(PARAMETER_ITEM_SIZE);

		for (int i= 0; i< pGroupItem.paramVec.size();i++)
		{
			wxStringList itemList;
			for (int j = 0 ;j < pGroupItem.paramVec[i].contentStrList.size();j++)
			{
				itemList.Add(pGroupItem.paramVec[i].contentStrList[j]);
			}
			pItemGroup->createItem(pGroupItem.paramVec[i].strParamName, pGroupItem.paramVec[i].controlType, pGroupItem.paramVec[i].dataType, itemList, pGroupItem.paramVec[i].uiConifg);
		}
		pItemGroup->Bind(wxCUSTOMEVT_ANKER_BACKGROUND_PROCESS, &AnkerParameterPanel::onDatachanged, this);
		pItemGroup->Bind(wxCUSTOMEVT_ANKER_RESETBTN_UPDATE, &AnkerParameterPanel::onUpdateResetBtn, this);
		m_pTabItemScrolledVWinSizer->Add(pItemGroup, wxSizerFlags().Expand().Border(wxTOP | wxBottom, 2));
		return pItemGroup;
}

void AnkerParameterPanel::rename_preset()
{
	auto prestType = Slic3r::Preset::TYPE_PRINT;

	// get new name
	// add by allen for ankerCfgDlg and AnkerSavePresetDialog
	//SavePresetDialog dlg(m_parent, m_type, msg);
	Slic3r::GUI::AnkerSavePresetDialog dlg(Slic3r::GUI::wxGetApp().mainframe, prestType, "");
	if (dlg.ShowModal() != wxID_OK)
		return;

	auto presets1 = Slic3r::GUI::wxGetApp().preset_bundle;
	auto presets = Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets;
	const std::string new_name = dlg.get_name(); //into_u8(dlg.get_name());
	if (new_name.empty() || new_name == presets->get_selected_preset().name)
		return;

	// Note: selected preset can be changed, if in SavePresetDialog was selected name of existing preset
	Slic3r::Preset& selected_preset = presets->get_selected_preset();
	Slic3r::Preset& edited_preset = presets->get_edited_preset();

	const std::string old_name = selected_preset.name;
	const std::string old_file_name = selected_preset.file;

	assert(old_name == edited_preset.name);

	using namespace boost;
	try {
		// rename selected and edited presets
		selected_preset.name = new_name;
		replace_last(selected_preset.file, old_name, new_name);

		edited_preset.name = new_name;
		replace_last(edited_preset.file, old_name, new_name);

		// rename file with renamed preset configuration
		filesystem::rename(old_file_name, selected_preset.file);
	}
	catch (const exception& ex) {
		const std::string exception = diagnostic_information(ex);
		ANKER_LOG_ERROR<<"Can't rename a preset : %s", exception.c_str();
	}

	// sort presets after renaming
	std::sort(presets->begin(), presets->end());
	// update selection
	presets->select_preset_by_name(new_name, true);
	m_pPresetParameterComBox->update();
	Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->update_tab_ui();//    onAnkerPresetsChanged(); 
	wxGetApp().plater()->sidebarnew().renameUserFilament(old_name, new_name);

	Layout();
	Refresh();
}


void AnkerParameterPanel::delete_preset()
{
	auto prestType = Slic3r::Preset::TYPE_PRINT;
	auto presets = Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->m_presets;
	auto current_preset = presets->get_selected_preset();
	std::string strOldFilamentName;
	// Don't let the user delete the ' - default - ' configuration.
	wxString action = current_preset.is_external ? _L("remove") : _L("delete");

	auto& physical_printers = Slic3r::GUI::wxGetApp().preset_bundle->physical_printers;
	wxString msg;

	msg += Slic3r::GUI::from_u8((boost::format(_u8L("Are you sure you want to %1% the selected preset?")) % action).str());

	action = current_preset.is_external ? _L("Remove") : _L("Delete");
	if (msg.empty()) {
		msg = format_wxstr(_L("common_notice_delete2check"), current_preset.name);
	}
	// TRN Settings Tabs: Button in toolbar: "Remove/Delete"
	wxString title = format_wxstr(_L("%1% Preset"), action);
	// TRN "remove/delete"
	if (current_preset.is_default ||
		//wxID_YES != wxMessageDialog(parent(), msg, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal())
		AnkerMsgDialog::MsgResult::MSG_OK != AnkerMessageBox(Slic3r::GUI::wxGetApp().mainframe, msg.ToStdString(wxConvUTF8), title.ToStdString(wxConvUTF8)))
		return;

	// Select will handle of the preset dependencies, of saving & closing the depending profiles, and
	// finally of deleting the preset.
	//this->select_preset("", true);
	Slic3r::GUI::wxGetApp().getAnkerTab(prestType)->select_preset("", true);

}

std::map<wxString, PARAMETER_GROUP> AnkerParameterPanel::getAllPrintParamInfos()
{
	return m_AllPrintParamInfo;
}

bool AnkerParameterPanel::isSelectSystemPreset()
{
	wxString strCurrentPresetName = m_pPresetParameterComBox->GetValue();
	auto tempPreset = Slic3r::GUI::wxGetApp().preset_bundle->prints.find_preset(strCurrentPresetName.ToStdString(wxConvUTF8), false, false);
	if (tempPreset == nullptr)
	{
		return false;
	}
	return tempPreset->is_system ? true : false;
}

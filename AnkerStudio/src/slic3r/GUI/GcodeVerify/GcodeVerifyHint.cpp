#include "GcodeVerifyHint.hpp"
#include "libslic3r/Utils.hpp"
#include <slic3r/Utils/GcodeInfo.hpp>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/Common/AnkerDialog.hpp>
#include "libslic3r/AppConfig.hpp"

bool GcodeVerifyHint::CheckOption()
{
	bool hint = false;
	auto appConfig = Slic3r::GUI::wxGetApp().app_config;
	if (appConfig)
		hint = appConfig->get_bool("Print Check", "machine_gcode_unmatched_nohint") == false;
	return hint;
}

bool GcodeVerifyHint::CheckDeviceType(const std::string& filePath, const anker_device_type deviceType)
{
	bool isAnkerBrand;
	anker_device_type machineType;
	Slic3r::GcodeInfo::GetMachineInfoFromGCode(filePath, isAnkerBrand, machineType);
	bool hint = !isAnkerBrand || machineType != deviceType;
	return hint;
}

bool GcodeVerifyHint::CheckTemperature(const std::string& filePath, const int temperature)
{
	int gCodeTemp = 0;
	const bool ret = Slic3r::GcodeInfo::GetTemperatureFromGCode(filePath, gCodeTemp);
	bool hint = ret && gCodeTemp > temperature;
	return hint;
}

bool GcodeVerifyHint::ShowModalTipsDialog(wxWindow* parent)
{
	assert(parent != nullptr);
	if (parent == nullptr)
		return true;

	// popup in the center of parent
	wxSize childSize = AnkerSize(400, 200);
	wxPoint parentCenterPoint(parent->GetPosition().x + parent->GetSize().GetWidth() / 2,
		parent->GetPosition().y + parent->GetSize().GetHeight() / 2);
	wxPoint childPos = wxPoint(parentCenterPoint.x - childSize.x / 2,parentCenterPoint.y - childSize.y / 2);

	wxWindow* parentTmp = parent;
#if !defined (WIN32)
	parentTmp = nullptr;
#endif
	AnkerDialog dialog(parentTmp, wxID_ANY, _L("common_popup_titlenotice"),
		_L("common_printnotice_limit_mismatch"), childPos, childSize);

	bool isCheck = false;
	auto appConfig = Slic3r::GUI::wxGetApp().app_config;
	int result = dialog.ShowAnkerModal2(AnkerDialogType_DisplayTextCheckBoxNoYesDialog, _L("common_button_donotshowagain"),
		[appConfig, &isCheck](wxCommandEvent& event, wxControl* control) {
			auto checkBox = dynamic_cast<wxCheckBox*>(control);
			if (!checkBox || !appConfig) {
				return;
			}
			isCheck = checkBox->IsChecked();
		});
	if (result != wxID_OK) {
		ANKER_LOG_INFO << "you select not continue to print";
		return false;
	}
	if (appConfig) {
		appConfig->set("Print Check", "machine_gcode_unmatched_nohint", isCheck ? "1" : "0");
	}
	return true;
}

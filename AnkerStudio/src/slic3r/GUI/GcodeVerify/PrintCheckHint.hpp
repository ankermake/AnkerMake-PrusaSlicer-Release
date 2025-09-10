#pragma once
#include "AnkerNetDefines.h"

namespace AnkerNet
{
	DEF_PTR(DeviceObjectBase)
}
using namespace AnkerNet;

class PrintCheckHint
{
public:
	using OkFunc_T = std::function<void(const std::string& sn)>;
	using CancelFunc_T = std::function<void(const std::string& sn)>;

	enum class HintType {
		ONLY_OK,
		CANCEL_CONTINUE,
		NEED_LEVEL,
		GCODE_DEVICE_NO_MATCH,
		GCODE_NOT_ANKERBAND,
		TEMPERATURE_HINT,
	};
	
	static bool StopForV6UnInited(const std::string& sn, wxWindow* parent = nullptr);
	
	void SetFuncs(OkFunc_T okFunc, CancelFunc_T cancelFunc = nullptr);

	// utf8GcodeFile: you should transfer the utf8 string for gcode file path
	bool CheckNeedRemind(DeviceObjectBasePtr currentDev, 
		const std::string& utf8GcodeFile = "");

	bool Hint(DeviceObjectBasePtr currentDev,
		wxWindow* parent = nullptr,
		const wxSize& dialogSize = wxDefaultSize, 
		const wxPoint& dialogPosition = wxDefaultPosition, 
		bool remotePrint = false);
	bool Hint(DeviceObjectBasePtr currentDev,
		HintType type, 
		wxWindow* parent = nullptr, 
		const wxSize& dialogSize = wxDefaultSize, 
		const wxPoint& dialogPosition = wxDefaultPosition);
	
	HintType Type() const { return m_type; }

private:
	bool QueryStatus(DeviceObjectBasePtr currentDev);
	bool IsV6Uninited(DeviceObjectBasePtr currentDev);
	bool IsGcodeMachineMatched(DeviceObjectBasePtr currentDev, 
		const std::string& gcodeFile = "");
	bool IsTempertureHint(DeviceObjectBasePtr currentDev);
	bool GcodeNoMatchHint(wxWindow* parent = nullptr, 
		const wxSize& dialogSize = wxDefaultSize, 
		const wxPoint& dialogPosition = wxDefaultPosition);
	void TemperatureHint(wxWindow* parent = nullptr,
		const wxSize& dialogSize = wxDefaultSize,
		const wxPoint& dialogPosition = wxDefaultPosition);

	void SetInfoByType(DeviceObjectBasePtr currentDev, HintType type);

	bool DevVerMatchForTempHint(DeviceObjectBasePtr currentDev);

private:
	DeviceObjectBasePtr m_currentDev = nullptr;
	HintType m_type = HintType::ONLY_OK;
	std::string m_reminderStr;
	std::string m_title;

	OkFunc_T m_okFunc = nullptr;
	CancelFunc_T m_cancelFunc = nullptr;
};

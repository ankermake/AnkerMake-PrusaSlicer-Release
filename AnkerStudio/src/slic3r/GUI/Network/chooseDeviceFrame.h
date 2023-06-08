#ifndef ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H
#define ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include "DeviceObject.h"

class AnkerChooseDeviceItem : wxControl
{
public:
	AnkerChooseDeviceItem(wxWindow* parent);
	~AnkerChooseDeviceItem();

	enum ChooseDeviceStat {
		Available,
		Busy,
		Offline,
		Unknown,
	};

	void setDeviceName(const std::string& deviceName);
	void setDeviceIcon(const wxImage& icon);
	void setDeviceState(int state);

private:
	void initGUI();
	void OnPaint(wxPaintEvent& event);
	void OnMouseEnterWindow(wxMouseButton& event);
	void OnMouseLeaveWindow(wxMouseButton& event);

private:
	std::string m_deviceName = "";
	wxImage m_deviceImage;
	ChooseDeviceStat m_deviceStatus = Unknown;
	mqtt_device_type m_deviceType = DEVICE_V8111_TYPE;

	wxStaticText* m_pDeviceNameText;
	wxStaticText* m_pStatusText;
};


class ChooseDeviceFrame : public wxDialog
{
public:
	ChooseDeviceFrame(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~ChooseDeviceFrame();
	
	void initGui(const int btnHeight, const wxFont& font, const wxColour& backgroundColor, const wxBackgroundStyle& backgroundStyle);
	void initNoDeviceGui(const int btnHeight, const wxFont& font, const wxColour& backgroundColor, const wxBackgroundStyle& backgroundStyle);
	void addDevices();
	void setTimer(wxTimer* timer);

private:
	wxStaticText* chooseDeviceContent;
	wxListBox* listBox;
	wxButton* cancelBtn;
	wxButton* printBtn;

	const int scaled_height;
	wxTimer* m_timer = nullptr;
};



#endif // !ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H

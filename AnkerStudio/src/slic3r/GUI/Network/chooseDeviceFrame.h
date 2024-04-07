#ifndef ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H
#define ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <boost/signals2.hpp>
#include <boost/bind/bind.hpp>
#include "../Common/AnkerLine.hpp"
#include "../Common/AnkerButton.hpp"
#include "../GUI_App.hpp"
#include "../AnkerBtn.hpp"
#include "AnkerNetDefines.h"

namespace AnkerNet
{
	DEF_PTR(DeviceObjectBase)
}
using namespace AnkerNet;

class AnkerChooseDeviceItem : public wxControl
{
public:
	AnkerChooseDeviceItem(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	~AnkerChooseDeviceItem();

	enum DeviceSelectMode {
		Device_Selected_None,
		Device_Selected,
	};
	enum ChooseDeviceStat {
		Available,
		Busy,
		Offline,
	};

	struct ChooseDeviceItemData {
		std::string m_deviceName = "";
		std::string m_statusText = "";
		std::string m_sn = "";
		wxImage m_deviceImage;
		wxImage m_selectedImage;
		ChooseDeviceStat m_deviceStatus = Offline;
		anker_device_type m_deviceType = DEVICE_V8111_TYPE;
		DeviceSelectMode m_selected = Device_Selected_None;
		wxFont m_font;
		int m_index = -1;
		bool m_gcodeMatch = true;

		ChooseDeviceItemData& operator=	(const ChooseDeviceItemData& data) {
			m_deviceName = data.m_deviceName;
			m_statusText = data.m_statusText;	
			m_sn = data.m_sn;
			m_deviceImage = data.m_deviceImage;
			m_selectedImage = data.m_selectedImage;
			m_deviceStatus = data.m_deviceStatus;
			m_deviceType = data.m_deviceType;
			m_selected = data.m_selected;
			m_font = data.m_font;
			m_index = data.m_index;
			m_gcodeMatch = data.m_gcodeMatch;
			return *this;
		}
	};

	void setData(const ChooseDeviceItemData& data);
	void setFont(const wxFont& font);
	void setDeviceTextState();
	void setSelected(DeviceSelectMode selected);
	void updateSelectedStatus(DeviceSelectMode selected);
	DeviceSelectMode getCurrentDeviceSelectMode() const;
	
	bool IsGcodeMatch() const;
	bool isAvailable() const;
	int getIndex() const;
	std::string getDeviceSn() const;

	boost::signals2::connection updateSelctedConnect(const boost::signals2::slot<void(int)>& slot);
	void updateSelectedSignal();

private:
	void initGUI();
	void OnPaint(wxPaintEvent& event);
	void OnClickItem(wxMouseEvent& event);

private:
	ChooseDeviceItemData m_data;
	boost::signals2::signal<void(int)> selectedSignal;
};


class AnkerChooseDevicePanel;
class AnkerChooseDeviceListBox : public wxListBox
{
public:
	struct ListBoxItemData {
		wxString label;
		wxControl* control;
	};
	AnkerChooseDeviceListBox(wxWindow* parent, wxWindowID id, 
		const wxPoint& pos = wxDefaultPosition, const wxSize &size = wxDefaultSize);
	void setChooseDevicePanel(AnkerChooseDevicePanel* window);

	void AddChooseDeviceItem(const AnkerChooseDeviceItem::ChooseDeviceItemData& data);
	void selectedChooseDeviceItem(int index = 0);

	std::string getSelcetedSn() const;
	
private:
	void AddControlItem(wxControl* control, const wxString& label);
	wxControl* GetControlItem(int index) const;
	void DeleteItemData(size_t index);
	void updateSelectedStatus(int index);

private:
	std::string m_selectedSn;
	AnkerChooseDevicePanel* m_chooseDevicePanel = nullptr;
};


class AnkerChooseDevicePanel : public wxPanel 
{
public:
	AnkerChooseDevicePanel(wxWindow* parent, const wxString& title = "");

	void initDeviceListGui();
	void initNoDeviceGui();
	std::string getSelectedDeviceSn() const;
	void UpdateGcodeHintInfo(bool gcodeMatch = true);
	void EnableOkBtn(bool enable = true);

private:	
	void AddGcodeHintPanel();
	void RemoveGcodeHintPanel();
	void OnOkBtnClicked(wxCommandEvent& event);

private:
	wxWindow* m_parent{ nullptr };
	wxStaticText* m_title = nullptr;
	wxButton* closeBtn = nullptr;
	wxStaticText* chooseDeviceContent = nullptr;
	AnkerChooseDeviceListBox* deviceListBox = nullptr;
	AnkerBtn* cancelBtn = nullptr;
	AnkerBtn* okBtn = nullptr;
	wxSizer* m_mainSizer = nullptr;
	// the index is the pos gocode hint sizer in mainsizer
	const int m_gcodeHintIndex = 3;	
	bool m_haveGcodeHint = false;
};

class AnkerLoadingMask;
class AnkerChooseDeviceDialog : public wxDialog
{
public:
	AnkerChooseDeviceDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerChooseDeviceDialog();

	void UpdateGui();
	void SetLoadingVisible(bool visible);

private:
	void InitGui();
	void OnLoadMaskRectUpdate(wxCommandEvent& event);
	void OnLoadingTimeout(wxCommandEvent& event);

private:
	AnkerChooseDevicePanel* m_panel = nullptr;
	AnkerLoadingMask* m_pLoadingMask = nullptr;
	int scaled_height;
	wxString m_title;
	bool m_uiUpdated = false;
	bool m_isVisible;
};


#endif // !ANKER_NETWORK_CHOOSE_DEVICE_FRAME_H

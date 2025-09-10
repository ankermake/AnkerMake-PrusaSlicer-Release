#ifndef ANKER_MULTI_COLOR_SYSNC_DEVICE_DIALOG_HPP
#define  ANKER_MULTI_COLOR_SYSNC_DEVICE_DIALOG_HPP

#include "AnkerDialog.hpp"
#include <boost/signals2.hpp>
#include <boost/bind/bind.hpp>
#include "../Network/chooseDeviceFrame.h"

wxDECLARE_EVENT(wxCUSTOMEVT_MULTICOLOR_PICKED, wxCommandEvent);

enum DeviceSelectMode {
    Device_Selected_None,
    Device_Selected,
};
struct DeviceItemData {
	std::string m_deviceName = std::string("");
	std::string m_sn = std::string("");
	int m_index = -1;
	bool m_onlined = false;
	bool m_haveFilament = false;
	DeviceSelectMode m_selected = Device_Selected_None;
	std::vector<wxColour> m_filaments;

	DeviceItemData& operator= (const DeviceItemData& data) {
		m_deviceName = data.m_deviceName;
		m_sn = data.m_sn;
		m_onlined = data.m_onlined;
		m_haveFilament = data.m_haveFilament;
		m_selected = data.m_selected;
		m_filaments = data.m_filaments;
		return *this;
	}
};

class AnkerMulticolorSyncDeviceItem : public wxControl, public AnkerBase
{
public:
	AnkerMulticolorSyncDeviceItem(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, DeviceItemData data = DeviceItemData());
	~AnkerMulticolorSyncDeviceItem();


	void setIconPath(const wxString& path = AnkerBase::AnkerResourceIconPath);
	void setData(const DeviceItemData& data);
	void setDeviceName(const std::string& deviceName);
	int getPickStatus() const;
	void setSelected(int selected);
	void updateSelectedStatus(int selected);
	int getIndex() const;
	std::string getDeviceSn() const;

	boost::signals2::connection updateSelctedConnect(const boost::signals2::slot<void(int)>& slot);
	void updataSelectedSignal();

	static double getBrightness(const wxColour& color);

private:
	void initGUI();
	void OnPaint(wxPaintEvent& event);
	void OnMouseEnterWindow(wxMouseEvent& event);
	void OnMouseLeaveWindow(wxMouseEvent& event);


private:
	DeviceItemData m_data;
	boost::signals2::signal<void(int)> selectedSignal;

	wxImage m_checkImage;
	wxImage m_noCheckImage;
	wxImage m_offlineImage;
};

class AnkerListBoxCtrl : public wxListBox
{
public:
	AnkerListBoxCtrl(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		int n = 0, const wxString choices[] = NULL,
		long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxListBoxNameStr));
	~AnkerListBoxCtrl();
};

class AnkerMulticolorSysncFilamentListBox : public AnkerListBoxCtrl
{
public:
	struct ListBoxItemData {
		wxString label;
		wxControl* control;
	};
	AnkerMulticolorSysncFilamentListBox(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);

	virtual void AddChooseDeviceItem(void* pData);
	virtual void selectedChooseDeviceItem(int index = 0);
	virtual void updateSelectedStatus(int index);

	void sendPickedDeviceSig(const wxString& sn);

	std::string getSelcetedSn() const;
    void setSliderBarWindow(wxWindow *window);

protected:
	void AddControlItem(wxControl* control, const wxString& label);
	wxControl* GetControlItem(int index) const;
	void DeleteItemData(size_t index);

	std::string m_selectedSn;
    
private:
    wxWindow *m_sliderBarWindow = nullptr;
};



class AnkerMulticolorSyncDeviceDialogPanel : public AnkerDialogCancelOkPanel
{
public: 
	AnkerMulticolorSyncDeviceDialogPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, wxWindow* sliderBarWindow = nullptr);
	std::string getSelectedDevice() const;
    void setSliderBarWindow(wxWindow *window);
private:
	AnkerMulticolorSysncFilamentListBox* m_listBox;
};


class AnkerMulticolorSyncDeviceDialog : public AnkerDialog
{
public:
	AnkerMulticolorSyncDeviceDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
    void setSliderBarWindow(wxWindow* window);
	virtual void InitDialogPanel(int dialogType = 0);
	virtual int ShowAnkerModal(int dialogType = 0);

private:
    wxWindow *m_sliderBarWindow;
};



#endif // !ANKER_MULTI_COLOR_SYSNC_DEVICE_DIALOG_HPP

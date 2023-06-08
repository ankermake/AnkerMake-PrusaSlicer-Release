#include "chooseDeviceFrame.h"
#include "../../Utils/DataManger.hpp"
#include "../../GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
ChooseDeviceFrame::ChooseDeviceFrame(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxDialog(parent, id, title, pos, size, style, name) , scaled_height(10)
{
    //initGui();
}
ChooseDeviceFrame::~ChooseDeviceFrame()
{
    
}

void ChooseDeviceFrame::initGui(const int btnHeight, const wxFont& font, const wxColour& backgroundColor, const wxBackgroundStyle& backgroundStyle)
{
	SetFont(font);
	SetBackgroundColour(backgroundColor);
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	chooseDeviceContent = new  wxStaticText(this, wxID_ANY,
		_L("Before starting printing, please make sure that the printer is currently in an idle state and has been Auto-Leveled."), wxDefaultPosition, wxDefaultSize);
	chooseDeviceContent->SetFont(font);
	sizer1->Add(chooseDeviceContent, 1, wxEXPAND | wxALL, 5);
	//sizer1->AddSpacer(10);

	listBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	listBox->SetFont(font);
	listBox->SetBackgroundStyle(backgroundStyle);
	listBox->SetBackgroundColour(backgroundColor);

	std::list<DeviceObjectPtr> devices = Datamanger::GetInstance().getDeviceList();
	int count = 0;
	for (auto it = devices.begin(); it != devices.end(); it++) {
		listBox->Insert((*it)->station_name, count++);
		PrintLog("Device name: " + (*it)->station_name);
		PrintLog("Device mqtt_status: " + std::to_string((*it)->onlined));
		PrintLog("Device is_command: " + std::to_string((*it)->is_command));
		PrintLog("\n");
	}

	if (listBox->GetCount() > 0) {
		listBox->SetSelection(0);
	}
	listBox->Refresh();
	//listBox->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) {


	//	});
	sizer1->Add(listBox, 1, wxEXPAND | wxALL, 5);
	
	
	wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);

	cancelBtn = new wxButton(this, wxID_ANY, ("Cancel"), wxDefaultPosition,
		wxSize(-1, 20/*scaled_height*/), wxBU_EXACTFIT);
	cancelBtn->SetFont(font);
	printBtn = new wxButton(this, wxID_ANY, ("Print"), wxDefaultPosition,
		wxSize(-1, 20/*scaled_height*/), wxBU_EXACTFIT);
	printBtn->SetFont(font);

	cancelBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		Close();
		});
	printBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		int index = listBox->GetSelection();
		auto devices = Datamanger::GetInstance().m_printMachineList;
		
		if (devices.size() > 0 && index < devices.size() && index >= 0) {
			if (!Datamanger::GetInstance().getTransferFileStatus()) {
				if (Datamanger::GetInstance().deviceCanPrint(devices[index].station_sn)) {
#ifdef __APPLE__
					if (m_timer && !m_timer->IsRunning()) {
						m_timer->Start(100);
					}
#endif			
					Datamanger::GetInstance().setP2POperationType(P2P_TRANSFER_FILE);
					AnkerNetBase* ankerNet = Datamanger::GetInstance().getAnkerNetBase();
					if (!ankerNet) {
						return;
					}

					if (ankerNet->getVideoCtrlSn() == devices[index].station_sn && ankerNet->getVideoCtrlState() != 0/*STATE_NONE*/) {
						PrintLog("current sn on playing video, stop video ..");
						DeviceObjectPtr pDevObj = Datamanger::GetInstance().getDeviceObjectFromSn(devices[index].station_sn);
						boost::signals2::connection m_videoStopSussConnect = ankerNet->videoStopSussConnect(std::bind(&DeviceObject::videoStopSussSlot, pDevObj, std::placeholders::_1));
						ankerNet->closeVideoStream(VIDEO_CLOSE_BY_TRANSFER_FILE);
					}
					else {
						Datamanger::GetInstance().getDeviceDsk(devices[index].station_sn, nullptr);
					}
				}
				}
	}		//Close();
		});

	// Add a static text control with proportion 1 to push the buttons to the right
	wxStaticText* spacer = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer2->Add(spacer, 1, wxEXPAND);
	sizer2->Add(cancelBtn, 0, wxALIGN_RIGHT, 10);
	sizer2->Add(printBtn, 0, wxALIGN_RIGHT, 10);
	//sizer1->AddSpacer(10);
	sizer1->Add(sizer2, 0, wxEXPAND | wxALL, 5);
}

void ChooseDeviceFrame::initNoDeviceGui(const int btnHeight, const wxFont& font, const wxColour& backgroundColor, const wxBackgroundStyle& backgroundStyle)
{
	SetFont(font);
	SetBackgroundColour(backgroundColor);
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	chooseDeviceContent = new  wxStaticText(this, wxID_ANY,
		_L("No device is available now. You can use the AnkerMake\n APP to view/bind devices."), wxDefaultPosition, wxDefaultSize);
	chooseDeviceContent->SetFont(font);
	sizer1->Add(chooseDeviceContent, 1, wxEXPAND | wxALL, 5);
	
	wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);

	cancelBtn = new wxButton(this, wxID_ANY, ("Cancel"), wxDefaultPosition,
		wxSize(-1, 20/*scaled_height*/), wxBU_EXACTFIT);
	cancelBtn->SetFont(font);
	printBtn = new wxButton(this, wxID_ANY, ("OK"), wxDefaultPosition,
		wxSize(-1, 20/*scaled_height*/), wxBU_EXACTFIT);
	printBtn->SetFont(font);

	cancelBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		Close();
		});
	printBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		Close();
		});

	wxStaticText* spacer = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer2->Add(spacer, 1, wxEXPAND);
	sizer2->Add(cancelBtn, 0, wxALIGN_RIGHT, 10);
	sizer2->Add(printBtn, 0, wxALIGN_RIGHT, 10);
	//sizer1->AddSpacer(10);
	sizer1->Add(sizer2, 0, wxEXPAND | wxALL, 5);
}

void ChooseDeviceFrame::addDevices()
{
}

void ChooseDeviceFrame::setTimer(wxTimer* timer)
{
	m_timer = timer;
}

AnkerChooseDeviceItem::AnkerChooseDeviceItem(wxWindow* parent)
{
}

AnkerChooseDeviceItem::~AnkerChooseDeviceItem()
{
}

void AnkerChooseDeviceItem::setDeviceName(const std::string& deviceName)
{
	m_deviceName = deviceName;
	if (m_pDeviceNameText) {
		m_pDeviceNameText->SetLabelText(deviceName);
	}
}

void AnkerChooseDeviceItem::setDeviceIcon(const wxImage& icon)
{
	m_deviceImage = icon;
	Refresh();
}

void AnkerChooseDeviceItem::setDeviceState(int state)
{
	m_deviceStatus = (ChooseDeviceStat)state;
	if (m_pStatusText) {
		switch (m_deviceStatus)
		{
		case AnkerChooseDeviceItem::Available:
			m_pStatusText->SetLabelText(_L("Available"));
			break;
		case AnkerChooseDeviceItem::Busy:
			m_pStatusText->SetLabelText(_L("Busy"));
			break;
		case AnkerChooseDeviceItem::Offline:
			m_pStatusText->SetLabelText(_L("Offline"));
			break;
		default:
			break;
		}

	}
}

void AnkerChooseDeviceItem::initGUI()
{
	SetBackgroundColour(wxColour(41, 42, 45));
	SetMinSize(wxSize(324, 88));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerChooseDeviceItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxColour bgColor = GetBackgroundColour();
	wxBrush brush(bgColor);
	wxPen pen(wxColour(41, 42, 45));
	wxRect rect = GetClientRect();

	dc.SetBrush(brush);
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

	// draw line
	{
		wxPen pen(wxColour(64, 65, 70));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextBackground(wxColour(255, 255, 255));
		wxPoint startPoint = wxPoint(2, 1);
		wxPoint endPoint = wxPoint(321, 1);
		dc.DrawLine(startPoint, endPoint);
	}

	// draw image
	{
		wxPen pen(wxColour(255, 255, 255));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextBackground(wxColour(255, 255, 255));
		wxPoint imagePos(16, 16);
		dc.DrawBitmap(m_deviceImage, imagePos);
	}

	// 


}

void AnkerChooseDeviceItem::OnMouseEnterWindow(wxMouseButton& event)
{
	SetBackgroundColour(wxColour(77, 78, 82));
	Refresh();
}

void AnkerChooseDeviceItem::OnMouseLeaveWindow(wxMouseButton& event)
{
	SetBackgroundColour(wxColour(41, 42, 45));
	Refresh();
}


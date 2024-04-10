#include "chooseDeviceFrame.h"
#include "../../GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/AppConfig.hpp"
#include <wx/filename.h>
#include <slic3r/GUI/Common/AnkerDialog.hpp>
#include <slic3r/Utils/GcodeInfo.hpp>
#include <slic3r/GUI/GcodeVerify/GcodeVerifyHint.hpp>
#include <slic3r/GUI/GcodeVerify/PrintCheckHint.hpp>
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "../../GUI/AnkerGCodeImportDialog.hpp"
#include "../../GUI/FilamentMaterialConvertor.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"
#include "../Common/AnkerLoadingMask.hpp"
#include <boost/bind.hpp>
#include "../AnkerComFunction.hpp"
extern AnkerPlugin* pAnkerPlugin;

AnkerChooseDeviceItem::AnkerChooseDeviceItem(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
	wxControl(parent, id, pos, size, wxNO_BORDER)
{
	if (parent) {
		SetBackgroundColour(wxColour(parent->GetBackgroundColour()));
	}
	else {
		SetBackgroundColour(wxColour("#333438"));
	}
	Bind(wxEVT_PAINT, &AnkerChooseDeviceItem::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &AnkerChooseDeviceItem::OnClickItem, this);
}

AnkerChooseDeviceItem::~AnkerChooseDeviceItem()
{
}

void AnkerChooseDeviceItem::setData(const ChooseDeviceItemData& data)
{
	m_data = data;
	setDeviceTextState();
}

void AnkerChooseDeviceItem::setFont(const wxFont& font)
{
	m_data.m_font = font;
}

void AnkerChooseDeviceItem::setDeviceTextState()
{
	switch (m_data.m_deviceStatus)
	{
	case AnkerChooseDeviceItem::Available:
		m_data.m_statusText = _L("common_preview_1print_available").ToStdString();
		break;
	case AnkerChooseDeviceItem::Busy:
		m_data.m_statusText = _L("common_preview_1print_busy").ToStdString();
		break;
	case AnkerChooseDeviceItem::Offline:
		m_data.m_statusText = _L("common_preview_1print_disconnected").ToStdString();
		break;
	default:
		break;
	}

}

void AnkerChooseDeviceItem::setSelected(DeviceSelectMode selected)
{
	m_data.m_selected = selected;
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxString iconPath = wxFileName(exePath).GetPath();
#ifdef _WIN32
	iconPath += "/resources/icons/";
#elif __APPLE__
	iconPath += "/../Resources/icons/";
#endif 
	ANKER_LOG_INFO << "iconPath: " << iconPath;
	if (m_data.m_deviceStatus == Available) {
		m_data.m_selectedImage = wxImage(iconPath + "check.png", wxBITMAP_TYPE_PNG);
		if (m_data.m_deviceType == DEVICE_V8111_TYPE) {
			m_data.m_deviceImage = wxImage(iconPath + "V8111_Online.png", wxBITMAP_TYPE_PNG);
		}
		else if (m_data.m_deviceType == DEVICE_V8110_TYPE) {
			m_data.m_deviceImage = wxImage(iconPath + "V8110_Online_n.png", wxBITMAP_TYPE_PNG);
		}
	}
	else {
		if (m_data.m_deviceType == DEVICE_V8111_TYPE) {
			m_data.m_deviceImage = wxImage(iconPath + "V8111_Offline.png", wxBITMAP_TYPE_PNG);
		}
		else if (m_data.m_deviceType == DEVICE_V8110_TYPE) {
			m_data.m_deviceImage = wxImage(iconPath + "V8110_Offline_s.png", wxBITMAP_TYPE_PNG);
		}
	}

	Refresh();
	Update();
}

void AnkerChooseDeviceItem::updateSelectedStatus(DeviceSelectMode selected)
{
	setSelected(selected);
	updateSelectedSignal();
}

AnkerChooseDeviceItem::DeviceSelectMode AnkerChooseDeviceItem::getCurrentDeviceSelectMode() const
{
	return m_data.m_selected;
}

bool AnkerChooseDeviceItem::IsGcodeMatch() const
{
	return m_data.m_gcodeMatch;
}

bool AnkerChooseDeviceItem::isAvailable() const
{
	return m_data.m_deviceStatus == Available;
}

int AnkerChooseDeviceItem::getIndex() const
{
	return m_data.m_index;
}

std::string AnkerChooseDeviceItem::getDeviceSn() const
{
	return m_data.m_sn;
}

boost::signals2::connection AnkerChooseDeviceItem::updateSelctedConnect(const boost::signals2::slot<void(int)>& slot)
{
	return selectedSignal.connect(slot);
}

void AnkerChooseDeviceItem::updateSelectedSignal()
{
	selectedSignal(m_data.m_index);
}

void AnkerChooseDeviceItem::initGUI()
{
	SetBackgroundColour(wxColour("#333438"));
	SetSize(400, 76);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerChooseDeviceItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxColour bgColor = GetBackgroundColour();
	wxBrush brush(bgColor);
	wxPen pen(bgColor);
	wxRect rect = GetClientRect();

	//dc.SetBackground(brush);
	if (m_data.m_selected == Device_Selected)
	{
		// draw background
		{
			wxBrush bgBrush(wxColour(58, 76, 62));
			dc.SetBrush(bgBrush);
			dc.SetBackground(bgBrush);
		}
	}
	else
	{
		dc.SetBrush(brush);
		dc.SetBackground(brush);
	}
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

	// draw device image
	{
		wxPoint imagePos = AnkerPoint(24, 8);
		if (m_data.m_deviceImage.IsOk()) {
			wxImage resizedImage = m_data.m_deviceImage.Scale(60, 60, wxIMAGE_QUALITY_HIGH);
			wxBitmap bitmap(resizedImage);
			dc.DrawBitmap(bitmap, imagePos);
		}
	}

	// draw device name text
	{
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		if (m_data.m_selected == Device_Selected) {
			dc.SetTextForeground(wxColour("#FFFFFF"));
		}
		else {
			if (m_data.m_deviceStatus == Available) {
				dc.SetTextForeground(wxColour("#FFFFFF"));
			}
			else {
				dc.SetTextForeground(wxColour(112, 113, 116));
			}
		}
		wxPoint textPoint = AnkerPoint(100, 10);
#ifdef _WIN32
		dc.DrawText(wxString::FromUTF8(m_data.m_deviceName), textPoint);
#elif __APPLE__
		dc.DrawText(m_data.m_deviceName, textPoint);
#endif // _WIN32

		ANKER_LOG_DEBUG << m_data.m_deviceName;
	}

	// draw device status text
	{
		dc.SetFont(ANKER_FONT_NO_1);
		if (m_data.m_selected == Device_Selected) {
			dc.SetTextForeground(wxColour("#FFFFFF"));
		}
		else {
			if (m_data.m_deviceStatus == Available) {
				dc.SetTextForeground(wxColour("#FFFFFF"));
			}
			else {
				dc.SetTextForeground(wxColour(112, 113, 116));
			}
		}
		wxPoint textPoint = AnkerPoint(100, 40);
		dc.DrawText(m_data.m_statusText, textPoint);
	}

	if (m_data.m_selected == Device_Selected)
	{
		// draw check image
		{
			wxPoint imagePos = AnkerPoint(356, 30);
			if (m_data.m_selectedImage.IsOk()) {
				wxImage resizedImage = m_data.m_selectedImage.Scale(20, 20, wxIMAGE_QUALITY_HIGH);
				dc.DrawBitmap(resizedImage, imagePos);
			}
		}
	}

	event.Skip();
}

void AnkerChooseDeviceItem::OnClickItem(wxMouseEvent& event)
{
	if (m_data.m_deviceStatus == Available) {
		DeviceSelectMode mode = Device_Selected;
		if (getCurrentDeviceSelectMode() == Device_Selected) {
			mode = Device_Selected_None;
		}
		else {
			mode = Device_Selected;
		}
		updateSelectedStatus(mode);
	}

	event.Skip();
}

AnkerChooseDeviceListBox::AnkerChooseDeviceListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
	wxListBox(parent, id, pos, size, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL | wxNO_BORDER)
{
	SetBackgroundColour(wxColour("#333438"));
}

void AnkerChooseDeviceListBox::setChooseDevicePanel(AnkerChooseDevicePanel* window)
{
	m_chooseDevicePanel = window;
}

void AnkerChooseDeviceListBox::AddControlItem(wxControl* control, const wxString& label)
{
	ListBoxItemData itemData;
	itemData.label = label;
	itemData.control = control;
	Append(label, reinterpret_cast<void*>(new ListBoxItemData(itemData)));
}

wxControl* AnkerChooseDeviceListBox::GetControlItem(int index) const
{
	ListBoxItemData* itemData = reinterpret_cast<ListBoxItemData*>(GetClientData(index));
	if (itemData) {
		return itemData->control;
	}
	return nullptr;
}

void AnkerChooseDeviceListBox::DeleteItemData(size_t index)
{
	ListBoxItemData* itemData = reinterpret_cast<ListBoxItemData*>(GetClientData(index));
	if (itemData) {
		delete itemData;
	}
	wxListBox::Delete(index);
}


void AnkerChooseDeviceListBox::updateSelectedStatus(int index)
{
	int itemCount = GetCount();
	bool noOneSlected = true;
	for (int i = 0; i < itemCount; i++)
	{
		wxControl* item = GetControlItem(i);
		AnkerChooseDeviceItem* chooseDeviceItem = dynamic_cast<AnkerChooseDeviceItem*>(item);
		if (chooseDeviceItem) {
			if (index == i && chooseDeviceItem->isAvailable()) {
				m_selectedSn = chooseDeviceItem->getDeviceSn();
				if (chooseDeviceItem->getCurrentDeviceSelectMode() == (int)AnkerChooseDeviceItem::Device_Selected) {
					noOneSlected = false;
					if (m_chooseDevicePanel) {
						m_chooseDevicePanel->UpdateGcodeHintInfo(chooseDeviceItem->IsGcodeMatch());
						m_chooseDevicePanel->EnableOkBtn(chooseDeviceItem->IsGcodeMatch());
					}
				}
				else {
					chooseDeviceItem->setSelected(AnkerChooseDeviceItem::Device_Selected_None);
				}
			}
			else {
				chooseDeviceItem->setSelected(AnkerChooseDeviceItem::Device_Selected_None);
			}
		}
	}

	if (noOneSlected) {
		if (m_chooseDevicePanel) {
			m_chooseDevicePanel->UpdateGcodeHintInfo(true);
			m_chooseDevicePanel->EnableOkBtn(false);
		}
	}

	ANKER_LOG_INFO << "m_selectedSn: " << m_selectedSn;
}


void AnkerChooseDeviceListBox::AddChooseDeviceItem(const AnkerChooseDeviceItem::ChooseDeviceItemData& data)
{
	wxPoint pos = AnkerPoint(0, data.m_index * 76);
	AnkerChooseDeviceItem* chooseDeviceItem = new AnkerChooseDeviceItem(this, wxID_ANY, pos, AnkerSize(400, 76));
	boost::signals2::connection updateSelectedConnection = chooseDeviceItem->updateSelctedConnect(
		boost::bind(&AnkerChooseDeviceListBox::updateSelectedStatus, this, _1));
	chooseDeviceItem->setData(data);
	AddControlItem(chooseDeviceItem, "");
	chooseDeviceItem->updateSelectedStatus(AnkerChooseDeviceItem::Device_Selected_None);
	chooseDeviceItem->Refresh();
}

void AnkerChooseDeviceListBox::selectedChooseDeviceItem(int index)
{
	wxControl* control = GetControlItem(index);
	AnkerChooseDeviceItem* chooseDeviceItem = dynamic_cast<AnkerChooseDeviceItem*>(control);
	if (chooseDeviceItem != nullptr) {
		chooseDeviceItem->updateSelectedStatus(AnkerChooseDeviceItem::Device_Selected);
	}
}

std::string AnkerChooseDeviceListBox::getSelcetedSn() const
{
	return m_selectedSn;
}

AnkerChooseDeviceDialog::AnkerChooseDeviceDialog(
	wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name) :
	wxDialog(parent, id, title, pos, size, style, name),
	scaled_height(10), m_title(title)
{
	m_isVisible = true;
	InitGui();
	CallAfter([this](){
		SetLoadingVisible(m_isVisible);
	});
}

AnkerChooseDeviceDialog::~AnkerChooseDeviceDialog()
{
	SetLoadingVisible(false);
}

void AnkerChooseDeviceDialog::InitGui()
{
	wxColour bkColour("#333438");
	SetBackgroundColour(bkColour);
	
	SetSize(AnkerSize(400, 370));
	m_panel = new AnkerChooseDevicePanel(this, m_title);
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_panel, 1, wxEXPAND);
	SetSizer(sizer);
	Layout();
}

void AnkerChooseDeviceDialog::UpdateGui()
{
	if (m_uiUpdated) {
		return;
	}
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	
	m_uiUpdated = true;
	wxWindowUpdateLocker updateLocker(this);
	if (ankerNet->GetDeviceList().size() > 0) {
		m_panel->initDeviceListGui();
	}
	else {
		SetSize(AnkerSize(400, 220));
		m_panel->initNoDeviceGui();
	}
	Layout();	
}

void AnkerChooseDeviceDialog::SetLoadingVisible(bool visible)
{
	m_isVisible = visible;
	ANKER_LOG_INFO << "SetLoadingVisible,visible = " << visible;
	// loading frame
	if (m_pLoadingMask == nullptr)
	{
		m_pLoadingMask = new AnkerLoadingMask(this, 40000);
		m_pLoadingMask->setText("");
		m_pLoadingMask->Bind(wxANKEREVT_LOADING_TIMEOUT, &AnkerChooseDeviceDialog::OnLoadingTimeout, this);
		m_pLoadingMask->Bind(wxANKEREVT_LOADMASK_RECTUPDATE, &AnkerChooseDeviceDialog::OnLoadMaskRectUpdate, this);
	}

	int x, y;
	GetScreenPosition(&x, &y);
	wxSize clientSize = GetClientSize();

	m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
	m_pLoadingMask->Show(visible);
	if (visible)
		m_pLoadingMask->start();
	else
		m_pLoadingMask->stop();
}

void AnkerChooseDeviceDialog::OnLoadMaskRectUpdate(wxCommandEvent& event)
{
	if (m_pLoadingMask && m_pLoadingMask->IsShown())
	{
		int x, y;
		GetScreenPosition(&x, &y);
		wxSize clientSize = GetClientSize();

		m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
	}
}

void AnkerChooseDeviceDialog::OnLoadingTimeout(wxCommandEvent& event)
{
	ANKER_LOG_INFO << "loading time out";
	SetLoadingVisible(false);
	UpdateGui();
}

AnkerChooseDevicePanel::AnkerChooseDevicePanel(wxWindow* parent, const wxString& title) :
	m_parent(parent),
	wxPanel(parent, wxID_ANY)
{
	SetDoubleBuffered(true);

	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxString iconPath = wxFileName(exePath).GetPath();
#ifdef _WIN32
	iconPath += "/resources/icons/choose_device_close.png";
#elif __APPLE__
	iconPath += "/../Resources/icons/choose_device_close.png";
#endif 

	wxSize closeSize = AnkerSize(12, 12);
	wxImage closeImage(iconPath, wxBITMAP_TYPE_PNG);
	closeImage.Rescale(12, 12);
	wxPoint closePos(GetSize().GetWidth() - closeSize.GetWidth(), closeSize.GetHeight());
	wxBitmap closeBitmap(closeImage);
	closeBtn = new wxButton(this, wxID_ANY, "", closePos, closeSize, wxBORDER_NONE);
	closeBtn->SetBitmap(closeBitmap);

	closeBtn->SetBackgroundColour(wxColour("#333438"));
	closeBtn->Bind(wxEVT_BUTTON, ([parent](wxCommandEvent& e)
		{
			auto ankerNet = AnkerNetInst();
			if (!ankerNet) {
				return;
			}
			std::remove(wxString::FromUTF8(ankerNet->GetGcodePath()).ToStdString().c_str());
			wxDialog* pParentDialog = dynamic_cast<wxDialog*>(parent);
			if (pParentDialog != nullptr)
			{
				pParentDialog->EndModal(wxID_CANCEL);
			}
		}));
	m_title = new wxStaticText(this, wxID_ANY, title);
	wxFont titleFont = ANKER_BOLD_FONT_NO_1;
	m_title->SetFont(titleFont);
	m_title->SetForegroundColour("#FFFFFF");
	m_title->SetBackgroundColour(wxColour("#333438"));
	m_title->SetSize(AnkerSize(108, 16));

	wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
	titleSizer->AddSpacer(AnkerSize(145, 20).GetWidth());	 //145

	titleSizer->Add(m_title, 1);
#ifdef _WIN32
	titleSizer->AddSpacer(AnkerSize(90, 20).GetWidth());	 // 90
#elif __APPLE__
	titleSizer->AddSpacer(AnkerSize(107, 20).GetWidth());	 // 107
#endif
	titleSizer->Add(closeBtn, 1);

	wxBoxSizer* lineSizer = new wxBoxSizer(wxHORIZONTAL);
	AnkerLine* line = new AnkerLine(this);
	line->SetMinSize(AnkerSize(400, 1));
	line->SetMaxSize(AnkerSize(400, 1));
	lineSizer->Add(line, 1, wxEXPAND);

	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->Add(titleSizer, 0, wxTOP | wxBOTTOM, 15);
	m_mainSizer->Add(lineSizer, 0, wxBOTTOM, 12);
	SetSizer(m_mainSizer);
}

void AnkerChooseDevicePanel::initDeviceListGui()
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	
	wxWindowUpdateLocker updateLocker(this);
	
	// list box
	wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, AnkerSize(400, 76 * 3));
	scrolledWindow->SetScrollRate(0, 20);
	//scrolledWindow->SetBackgroundColour(wxColour(255, 0, 0));
	listSizer->Add(scrolledWindow, 1, wxEXPAND | wxALL);

	deviceListBox = new AnkerChooseDeviceListBox(scrolledWindow, wxID_ANY, wxDefaultPosition, AnkerSize(400, 76 * ankerNet->GetDeviceList().size()));
	deviceListBox->setChooseDevicePanel(this);
	scrolledWindow->SetSizer(new wxBoxSizer(wxVERTICAL));
	scrolledWindow->GetSizer()->Add(deviceListBox, 0, wxEXPAND);

	// button sizer
	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->AddSpacer(24);
	cancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, AnkerSize(170, 32), wxBORDER_NONE);
	cancelBtn->SetText(_AnkerL("common_button_cancel"));
	cancelBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	cancelBtn->SetTextColor(wxColour("#FFFFFF"));
	cancelBtn->SetBackgroundColour(wxColour(89, 90, 94));
	cancelBtn->SetRadius(5);
	wxWindow* parent = GetParent();
	cancelBtn->Bind(wxEVT_BUTTON, ([parent](wxCommandEvent& e)
		{
			auto ankerNet = AnkerNetInst();
			if (!ankerNet) {
				return;
			}

			std::remove(wxString::FromUTF8(ankerNet->GetGcodePath()).ToStdString().c_str());
			wxDialog* dialog = dynamic_cast<wxDialog*>(parent);
			if (dialog != nullptr)
			{
				dialog->EndModal(wxID_CANCEL);
			}
		}));
	btnSizer->Add(cancelBtn, 1);
	btnSizer->AddSpacer(12);

	okBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, AnkerSize(170, 32), wxBORDER_NONE);
	okBtn->SetText(_AnkerL("common_preview_button_print"));
	okBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	okBtn->SetTextColor(wxColour("#FFFFFF"));
	okBtn->SetBackgroundColour("#62D361");
	okBtn->SetRadius(5);
	okBtn->Bind(wxEVT_BUTTON, &AnkerChooseDevicePanel::OnOkBtnClicked, this);

	btnSizer->Add(okBtn, 1);
	btnSizer->AddSpacer(24);

	m_mainSizer->Add(listSizer, 0, wxTOP | wxBOTTOM, 10);
	// gcode hint sizer
	m_mainSizer->Add(btnSizer, 0, wxTOP | wxBOTTOM, 12);

	// get gcode and device match
	auto gcodeFile = ankerNet->GetGcodePath();
	bool isAnkerBrand = false;
	anker_device_type gcodeMachineType;
	Slic3r::GcodeInfo::GetMachineInfoFromGCode(gcodeFile, isAnkerBrand, gcodeMachineType);

	int i = 0;
	int defaultSelectedIndex = -1;
	std::list<DeviceObjectBasePtr> deviceObjects = ankerNet->GetDeviceList();
	for (auto it = deviceObjects.begin(); it != deviceObjects.end(); it++) {
		if ((*it) != nullptr) {
			AnkerChooseDeviceItem::ChooseDeviceItemData data;
			data.m_deviceName = (*it)->GetStationName();
			data.m_sn = (*it)->GetSn();
			if ((*it)->GetOnline()) {
				if (!(*it)->IsBusy()) {
					data.m_deviceStatus = AnkerChooseDeviceItem::Available;
					if (defaultSelectedIndex == -1) {
						defaultSelectedIndex = i;
					}
				}
				else {
					data.m_deviceStatus = AnkerChooseDeviceItem::Busy;
				}
			}
			else {
				data.m_deviceStatus = AnkerChooseDeviceItem::Offline;
			}

			data.m_deviceType = (*it)->GetDeviceType();
			ANKER_LOG_INFO << "device type: " << data.m_deviceType << " gcode type: " << gcodeMachineType;
			if (isAnkerBrand && gcodeMachineType != data.m_deviceType) {
				data.m_gcodeMatch = false;
			}
			data.m_index = i++;
			deviceListBox->AddChooseDeviceItem(data);
		}
	}
	scrolledWindow->FitInside();
	if (defaultSelectedIndex >= 0) {
		deviceListBox->selectedChooseDeviceItem(defaultSelectedIndex);
	}
	else {
		EnableOkBtn(false);
	}

	Layout();
}

void AnkerChooseDevicePanel::UpdateGcodeHintInfo(bool gcodeMatch)
{
	if (gcodeMatch) {
		RemoveGcodeHintPanel();		
	}
	else {
		AddGcodeHintPanel();
	}
}

void AnkerChooseDevicePanel::EnableOkBtn(bool enable)
{
	if (enable) {
		okBtn->SetBackgroundColour("#62D361");
	}
	else {
		okBtn->SetBackgroundColour(wxColour(63, 64, 68));
	}
	okBtn->Enable(enable);
	this->Layout();
}

void AnkerChooseDevicePanel::AddGcodeHintPanel()
{
	wxWindowUpdateLocker updateLocker(this);
	if (m_haveGcodeHint) {
		return;
	}
	m_haveGcodeHint = true;
	if (m_parent) {
		m_parent->SetSize(AnkerSize(400, 410));
	}

	// line
	AnkerLine* line = new AnkerLine(this);
	line->SetMinSize(AnkerSize(400, 1));
	line->SetMaxSize(AnkerSize(400, 1));

	// hint text
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
	int contextWidth = this->GetSize().GetWidth() - 2 * 24;
	wxString context = _L("common_print_device_gcode_not_match")
		/*"The chosen printer is not compatible with the G-code's printer settings, please select the correct settings."*/;
	auto notMatchText = new AnkerStaticText(this, wxID_ANY, "");
	notMatchText->SetFont(ANKER_FONT_NO_2);
	notMatchText->SetForegroundColour(wxColour("#FF6262"));
	notMatchText->SetBackgroundColour(wxColour("#333438"));
	wxString wrapText = Slic3r::GUI::WrapEveryCharacter(context,
		ANKER_FONT_NO_2, contextWidth);
	notMatchText->Wrap(contextWidth);
	notMatchText->SetLabelText(wrapText);
	
	wxBoxSizer* contextSizer = new wxBoxSizer(wxVERTICAL);
	contextSizer->Add(line, wxTOP, 12);
	contextSizer->Add(notMatchText, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 6);

	m_mainSizer->Insert(m_gcodeHintIndex, contextSizer, 0);
	this->Layout();
}

void AnkerChooseDevicePanel::RemoveGcodeHintPanel()
{
	wxWindowUpdateLocker updateLocker(this);
	if (!m_haveGcodeHint) {
		return;
	}
	m_haveGcodeHint = false;

	auto item = m_mainSizer->GetItem(m_gcodeHintIndex);
	if (item) {
		wxSizer* gcodeHintSizer = item->GetSizer();
		if (gcodeHintSizer) {
			gcodeHintSizer->Show(false);
		}
	}

	if (m_parent) {
		m_parent->SetSize(AnkerSize(400, 370));
	}

	this->Layout();
}

void AnkerChooseDevicePanel::OnOkBtnClicked(wxCommandEvent& event)
{
	//todo report by alves start transfer file
	std::string fileName = std::string();
	std::string fileSize = std::string();
	std::string errorCode = std::string("0");
	std::string errorMsg = std::string("start to transfer file");

	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {		
		return;
	}

	std::string sn = this->getSelectedDeviceSn();
	DeviceObjectBasePtr deviceObject = ankerNet->getDeviceObjectFromSn(sn);
	if (deviceObject != nullptr) {
		deviceObject->AsyQueryAllInfo();
		if (!deviceObject->GetTransfering()) {
			// non V6 printer should not do CheckHint here
			if (deviceObject->GetDevicePartsType() == DEVICE_PARTS_NO)
			{
				PrintCheckHint printCheckHint;
				if (printCheckHint.CheckNeedRemind(deviceObject, ankerNet->GetGcodePath())) {
					wxDialog* parent = dynamic_cast<wxDialog*>(GetParent());

					wxSize childSize = AnkerSize(400, 200);
					wxPoint childPos = wxDefaultPosition;
					// popup in the center of parent
					if (parent) {
						wxPoint parentCenterPoint(parent->GetPosition().x + parent->GetSize().GetWidth() / 2,
							parent->GetPosition().y + parent->GetSize().GetHeight() / 2);
						childPos = wxPoint(parentCenterPoint.x - childSize.x / 2,
							parentCenterPoint.y - childSize.y / 2);
						parent->EndModal(wxID_OK);
					}

					printCheckHint.SetFuncs([&printCheckHint, sn](const std::string& sn2) {
						if (printCheckHint.Type() == PrintCheckHint::HintType::NEED_LEVEL) {
							DatamangerUi::GetInstance().sendSigToSwitchPrintPage(sn);
						}
						});
					if (!printCheckHint.Hint(deviceObject, nullptr, childSize, childPos)) {
						return;
					}
				}
			}

			//V6 printer should show material mapping dialog 
			if (deviceObject->GetDevicePartsType() == DEVICE_PARTS_MULTI_COLOR)
			{
				using namespace Slic3r;
				AnkerGCodeImportDialog* pGcodeImportDialog = new AnkerGCodeImportDialog(sn, this);
				Slic3r::GUI::AnkerMaterialMappingViewModel* pViewModel = new Slic3r::GUI::AnkerMaterialMappingViewModel();
				//TODO: set viewmodel
				GcodeInfo::ParseGcodeInfoToViewModel(ankerNet->GetGcodePath(), pViewModel);
				std::vector<CardInfo> gcodeInfoVec = GcodeInfo::GetColorMaterialIdInfo(ankerNet->GetGcodePath());
				using namespace Slic3r::GUI;
				std::vector<DeviceFilementInfo> devcieInfoVec;
				auto multiColorData = deviceObject->GetMtSlotData();
				for (int i = 0; i < multiColorData.size(); i++)
				{
					DeviceFilementInfo devcieInfo;
					devcieInfo.iNozzelInx = multiColorData[i].cardIndex;
					if (multiColorData[i].edit_status == 1)
					{
						devcieInfo.bIsEdit = true;
					}
					if (multiColorData[i].rfid == 0 && multiColorData[i].edit_status == 0)
					{
						devcieInfo.iCoLorId = 0;
						devcieInfo.iFilamentId = 0;
						ANKER_LOG_INFO << "unknown material received, nozzel index is : " << multiColorData[i].cardIndex;
					}
					else
					{
						devcieInfo.iCoLorId = multiColorData[i].colorId;
						devcieInfo.iFilamentId = multiColorData[i].materialId;
						devcieInfo.filamentColor = wxColor(FilamentMaterialConvertor::ConvertColorId(devcieInfo.iCoLorId));
						devcieInfo.strMaterialName = FilamentMaterialConvertor::ConvertFilamentIdToCategory(devcieInfo.iFilamentId);
					}
					devcieInfo.nozzleStatus = multiColorData[i].nozzle_status;
					devcieInfoVec.push_back(devcieInfo);
				}

				//get G-CODE inlfo 
				for (int j = 0; j < gcodeInfoVec.size(); j++)
				{
					GcodeFilementInfo gcodeInfo;
					gcodeInfo.iFilamentId = gcodeInfoVec[j].materialId;
					gcodeInfo.iNozzelInx = gcodeInfoVec[j].index;
					gcodeInfo.iCoLorId = gcodeInfoVec[j].colorId;
					gcodeInfo.filamentColor = wxColor(FilamentMaterialConvertor::ConvertColorId(gcodeInfoVec[j].colorId));
					gcodeInfo.strMaterialName = FilamentMaterialConvertor::ConvertFilamentIdToCategory(gcodeInfoVec[j].materialId);
					pViewModel->m_curFilamentMap.insert(std::make_pair(gcodeInfo, devcieInfoVec));
					//set -1 represent have no default selected item
					int iMapInx = pGcodeImportDialog->autoMatchSlotInx(gcodeInfo, devcieInfoVec);
					pViewModel->m_SelectedFilamentInxVec.push_back(iMapInx);
				}
				pGcodeImportDialog->setViewModel(pViewModel);
				pGcodeImportDialog->switch2FileInfo(ankerNet->GetGcodePath());
				int iRet = pGcodeImportDialog->ShowModal();
				if (iRet == wxCANCEL)
				{
					return;
				}
			}
		
			std::map<std::string, std::string> map;
			map.insert(std::make_pair(c_pft_file_name, fileName));
			map.insert(std::make_pair(c_pft_file_size, fileSize));
			map.insert(std::make_pair(c_pft_error_code, errorCode));
			map.insert(std::make_pair(c_pft_error_msg, errorMsg));
			reportBuryEvent(e_print_file_transport, map);

			ankerNet->StartP2pOperator(P2P_TRANSFER_PREVIEW_FILE, sn, "");
		}
	} else {
		ANKER_LOG_ERROR << "deviceObject is nullptr, please check Log";

		std::map<std::string, std::string> buryMap;
		errorCode = "-1";
		errorMsg = "no this device";
		buryMap.insert(std::make_pair(c_pft_device_sn, sn));
		buryMap.insert(std::make_pair(c_pft_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pft_error_msg, errorMsg));
		reportBuryEvent(e_print_file_transport, buryMap);

		// TODO KEY replace
		AnkerMessageBox(this, _u8L("common_net_fail_get_printer_data"), _u8L("common_popup_titlenotice"), false);
		return;
	}

	wxDialog* dialog = dynamic_cast<wxDialog*>(GetParent());
	if (dialog != nullptr) {
		dialog->EndModal(wxID_OK);
	}
}

void AnkerChooseDevicePanel::initNoDeviceGui()
{
	wxWindowUpdateLocker updateLocker(this);

	m_mainSizer->AddSpacer(AnkerSize(20, 24).GetHeight());	// 24
	chooseDeviceContent = new wxStaticText(this, wxID_ANY, _L("common_preview_1print_nodevice"),
		wxDefaultPosition, AnkerSize(352, 42));
	chooseDeviceContent->SetForegroundColour(wxColour("#FFFFFF"));
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
	AnkerBox::setStrWrap(chooseDeviceContent, AnkerSize(352, 42).GetWidth(), _L("common_preview_1print_nodevice"), type);
	wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->AddSpacer(AnkerSize(24, 20).GetWidth()); // 24
	contentSizer->Add(chooseDeviceContent);
	m_mainSizer->Add(contentSizer, 1);
	m_mainSizer->AddSpacer(AnkerSize(24, 20).GetWidth());  // 24

	okBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, AnkerSize(352, 32), wxBORDER_NONE);
	okBtn->SetText(_AnkerL("OK"));
	okBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	okBtn->SetTextColor(wxColour("#FFFFFF"));
	okBtn->SetBackgroundColour(wxColour("#62D361"));
	okBtn->SetRadius(5);

	wxWindow* parent = GetParent();
	okBtn->Bind(wxEVT_BUTTON, ([parent](wxCommandEvent& e)
		{
			wxDialog* dialog = dynamic_cast<wxDialog*>(parent);
			if (dialog) {
				dialog->EndModal(wxID_OK);
			}
		}));
	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->AddSpacer(AnkerSize(24, 20).GetWidth());   // 24
	btnSizer->Add(okBtn);

	m_mainSizer->Add(btnSizer, 1);
	m_mainSizer->AddSpacer(AnkerSize(20, 16).GetHeight());	 // 16

	Layout();
}

std::string AnkerChooseDevicePanel::getSelectedDeviceSn() const
{
	return deviceListBox->getSelcetedSn();
}


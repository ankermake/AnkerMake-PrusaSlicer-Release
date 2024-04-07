#include "AnkerMulticolorSyncDeviceDialog.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"
#include <boost/bind.hpp>

wxDEFINE_EVENT(wxCUSTOMEVT_MULTICOLOR_PICKED, wxCommandEvent);

AnkerMulticolorSyncDeviceItem::AnkerMulticolorSyncDeviceItem(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, DeviceItemData data) :
    wxControl(parent, id, pos, size, wxNO_BORDER), m_data(data)
{
    if (parent) {
        SetBackgroundColour(wxColour(parent->GetBackgroundColour()));
    }
    else {
        SetBackgroundColour(wxColour("#333438"));
    }
    
    Bind(wxEVT_PAINT, &AnkerMulticolorSyncDeviceItem::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerMulticolorSyncDeviceItem::OnMouseEnterWindow, this);
    setIconPath();
}

AnkerMulticolorSyncDeviceItem::~AnkerMulticolorSyncDeviceItem()
{
}

void AnkerMulticolorSyncDeviceItem::setIconPath(const wxString& path)
{
    m_checkImage = wxImage(path + "checkbox_check.png", wxBITMAP_TYPE_PNG);
    m_noCheckImage = wxImage(path + "checkbox_disuncheck.png", wxBITMAP_TYPE_PNG);
    m_offlineImage = wxImage(path + "device_wifi_icon_off.png", wxBITMAP_TYPE_PNG);
}

void AnkerMulticolorSyncDeviceItem::setData(const DeviceItemData& data)
{
    m_data = data;
    m_data.m_index = data.m_index;
}

void AnkerMulticolorSyncDeviceItem::setDeviceName(const std::string& deviceName)
{
    m_data.m_deviceName = deviceName;
}

int AnkerMulticolorSyncDeviceItem::getPickStatus() const
{
    return m_data.m_selected;
}

void AnkerMulticolorSyncDeviceItem::setSelected(int selected)
{
    m_data.m_selected = (DeviceSelectMode)selected;
    Update();
    Refresh();
}

void AnkerMulticolorSyncDeviceItem::updateSelectedStatus(int selected)
{
    selectedSignal(m_data.m_index);
}

int AnkerMulticolorSyncDeviceItem::getIndex() const
{
    return m_data.m_index;
}

std::string AnkerMulticolorSyncDeviceItem::getDeviceSn() const
{
    return m_data.m_sn;
}

boost::signals2::connection AnkerMulticolorSyncDeviceItem::updateSelctedConnect(const boost::signals2::slot<void(int)>& slot)
{
    return selectedSignal.connect(slot);
}

void AnkerMulticolorSyncDeviceItem::updataSelectedSignal()
{
    selectedSignal(m_data.m_index);
}

double AnkerMulticolorSyncDeviceItem::getBrightness(const wxColour& color)
{
    int red = color.GetRed();
    int green = color.GetGreen();
    int blue = color.GetBlue();

    double maxVal = std::max(std::max(red, green), blue);
    double minVal = std::min(std::min(red, green), blue);
    double brightness = (maxVal + minVal) / 2.0;
    return brightness;
}

void AnkerMulticolorSyncDeviceItem::initGUI()
{
}

void AnkerMulticolorSyncDeviceItem::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.Clear();

    wxColour bgColor = wxColour("#333438");//GetBackgroundColour();
    wxBrush brush(bgColor);
    wxPen pen(bgColor);
    wxRect rect = GetClientRect();
    dc.SetBrush(bgColor);
    dc.SetBackground(bgColor);
    dc.SetPen(pen);
    dc.DrawRectangle(rect);

    if (m_data.m_onlined) {
        wxString deviceName = m_data.m_deviceName;

        // draw device name text
        dc.SetFont(ANKER_FONT_NO_1);
        dc.SetTextForeground(wxColour("#FFFFFF"));
        wxPoint textPoint = wxPoint(26, 13);
        dc.DrawText(deviceName, textPoint);

        if (m_data.m_haveFilament) {
            int rectWidth = 32;
            int rectHeight = 32;
            wxPoint textPoint = AnkerPoint(26, 13 + 21 + 10);
            
            for (int i = 0; i < m_data.m_filaments.size(); i++) {

                // draw rectangle background
                dc.SetBrush(wxBrush(m_data.m_filaments[i]));

                // draw rectangle 
                int rectX = (i == 0 ? textPoint.x : (textPoint.x + i * (rectWidth + 12)));
                int rectY = textPoint.y;
                dc.DrawRectangle(rectX, rectY, rectWidth, rectHeight);

                // draw number text
                int number = i + 1;
                wxString text = wxString::Format("%d", number);
                dc.SetFont(ANKER_BOLD_FONT_NO_1);

                if (getBrightness(m_data.m_filaments[i]) > 60) {
                    dc.SetTextForeground(wxColour("#000000"));
                }
                else {
                    dc.SetTextForeground(wxColour("#FFFFFF"));
                }

                wxSize textSize = dc.GetTextExtent(text);
#ifdef _WIN32
                wxPoint textPoint(rectX + 10,rectY + 5);
#elif __APPLE__
                wxPoint textPoint(rectX + 11,rectY + 10);
#endif
                dc.DrawText(text, textPoint);
            }

        }
        else {
            wxString noFilamentLabel = _AnkerL("No Filament Detected");
            dc.SetTextForeground(wxColour(112, 113, 116));
            wxPoint textPoint = wxPoint(26, 50);
            dc.DrawText(noFilamentLabel, textPoint);
        }

        dc.SetBrush(brush);

        // draw selected icon
        wxImage selectedImage;
        if (m_data.m_selected == Device_Selected) {
            selectedImage = m_checkImage;
        }
        else {
            selectedImage = m_noCheckImage;
        }

        if (selectedImage.IsOk()) {
            wxImage resizedImage = selectedImage.Scale(16, 16, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmap(resizedImage);
            wxPoint imagePos(rect.width - 26 - 16, textPoint.y);
            dc.DrawBitmap(bitmap, imagePos);
        }
    }
    else {
        wxString deviceName = m_data.m_deviceName;

        // draw device name text
        dc.SetFont(ANKER_FONT_NO_1);
        dc.SetTextForeground(wxColour(104, 105, 108));
        wxPoint textPoint = wxPoint(26, 13);
        dc.DrawText(deviceName, textPoint);
        
        // draw offline icon
        wxImage offlineImage(m_offlineImage);
        if (offlineImage.IsOk()) {
            wxImage resizedImage = offlineImage.Scale(20, 20, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmap(resizedImage);
            wxSize deviceNameSize = dc.GetTextExtent(deviceName);
            wxPoint imagePos(textPoint.x + deviceNameSize.GetWidth() + 12, textPoint.y + 5);
            dc.DrawBitmap(bitmap, imagePos);
        }

        // draw seleced none icon
        wxImage selectedNoneImage(m_noCheckImage);
        if (selectedNoneImage.IsOk()) {
            wxImage resizedImage = selectedNoneImage.Scale(16, 16, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmap(resizedImage);
            wxPoint imagePos(rect.width - 26 - 16, textPoint.y);
            dc.DrawBitmap(bitmap, imagePos);
        }
    }

    dc.SetPen(wxPen(wxColour("#545863"), 2)); 

    wxSize itemSize = GetClientSize();
    int startX = 0;
    int startY = 86;
    wxPoint startPoint = AnkerPoint(startX, startY);
    int endX = itemSize.GetWidth();// - startPoint.x * 2;
    int endY = startPoint.y;
    dc.DrawLine(startPoint.x, startPoint.y, endX, endY);

    event.Skip();
}

void AnkerMulticolorSyncDeviceItem::OnMouseEnterWindow(wxMouseEvent& event)
{
    if (m_data.m_onlined) {
        updateSelectedStatus(Device_Selected);
    }
    Update();
    Refresh();
}

void AnkerMulticolorSyncDeviceItem::OnMouseLeaveWindow(wxMouseEvent& event)
{
}

AnkerListBoxCtrl::AnkerListBoxCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, int n, const wxString choices[], 
    long style, const wxValidator& validator, const wxString& name) :
    wxListBox(parent, id, pos, size, n, choices, style, validator, name)
{

}

AnkerListBoxCtrl::~AnkerListBoxCtrl()
{
}

AnkerMulticolorSysncFilamentListBox::AnkerMulticolorSysncFilamentListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
    AnkerListBoxCtrl(parent, id, pos, size, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL | wxNO_BORDER)
{
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(wxColour("#333438"));
    }
}

void AnkerMulticolorSysncFilamentListBox::AddChooseDeviceItem(void* pData)
{
    DeviceItemData* pItemData = static_cast<DeviceItemData*>(pData);
    if (pItemData == nullptr) {
        return;
    }
    wxPoint pos(0, pItemData->m_index * AnkerSize(10,87).GetHeight());
    AnkerMulticolorSyncDeviceItem* chooseDeviceItem = new AnkerMulticolorSyncDeviceItem(this, wxID_ANY, pos, AnkerSize(400, 87));
    boost::signals2::connection updateSelectedConnection = chooseDeviceItem->updateSelctedConnect(boost::bind(&AnkerMulticolorSysncFilamentListBox::updateSelectedStatus, this, _1));
    chooseDeviceItem->setData(*pItemData);
    AddControlItem(static_cast<wxControl*>(chooseDeviceItem), "");
    chooseDeviceItem->updateSelectedStatus(Device_Selected_None);
    chooseDeviceItem->Refresh();
}

void AnkerMulticolorSysncFilamentListBox::selectedChooseDeviceItem(int index)
{
    if (index < 0) {
        return;
    }
    wxControl* control = GetControlItem(index);
    AnkerMulticolorSyncDeviceItem* chooseDeviceItem = dynamic_cast<AnkerMulticolorSyncDeviceItem*>(control);
    if (chooseDeviceItem != nullptr) {
        if (chooseDeviceItem->getPickStatus() == Device_Selected_None) {
            chooseDeviceItem->updateSelectedStatus(Device_Selected);
        }
        m_selectedSn = chooseDeviceItem->getDeviceSn();
        sendPickedDeviceSig(m_selectedSn);
    }
}

void AnkerMulticolorSysncFilamentListBox::updateSelectedStatus(int index)
{
    int itemCount = GetCount();
    for (int i = 0; i < itemCount; i++)
    {
        wxControl* item = GetControlItem(i);
        AnkerMulticolorSyncDeviceItem* chooseDeviceItem = dynamic_cast<AnkerMulticolorSyncDeviceItem*>(item);
        if (chooseDeviceItem) {
            if (index == i) {
                int picked = chooseDeviceItem->getPickStatus() == Device_Selected ? Device_Selected_None : Device_Selected;
                chooseDeviceItem->setSelected(picked);
                m_selectedSn = chooseDeviceItem->getDeviceSn();
                if (picked == Device_Selected) {
                    sendPickedDeviceSig(m_selectedSn);
                }
            }
            else {
                chooseDeviceItem->setSelected(Device_Selected_None);
            }
        }
    }
}

void AnkerMulticolorSysncFilamentListBox::sendPickedDeviceSig(const wxString& sn)
{
    if(m_sliderBarWindow){
        wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MULTICOLOR_PICKED);
        wxVariant varData(sn);
        evt.SetClientObject(new wxStringClientData(varData));
        wxPostEvent(m_sliderBarWindow, evt);
    }
}

void AnkerMulticolorSysncFilamentListBox::setSliderBarWindow(wxWindow *window)
{
    m_sliderBarWindow = window;
}


std::string AnkerMulticolorSysncFilamentListBox::getSelcetedSn() const
{
    return m_selectedSn;
}

void AnkerMulticolorSysncFilamentListBox::AddControlItem(wxControl* control, const wxString& label)
{
    ListBoxItemData itemData;
    itemData.label = label;
    itemData.control = control;
    Append(label, reinterpret_cast<void*>(new ListBoxItemData(itemData)));
}

wxControl* AnkerMulticolorSysncFilamentListBox::GetControlItem(int index) const
{
    ListBoxItemData* itemData = reinterpret_cast<ListBoxItemData*>(GetClientData(index));
    if (itemData) {
        return itemData->control;
    }
    return nullptr;
}

void AnkerMulticolorSysncFilamentListBox::DeleteItemData(size_t index)
{
}

AnkerMulticolorSyncDeviceDialogPanel::AnkerMulticolorSyncDeviceDialogPanel(wxWindow* parent, const wxString& title, const wxSize& size, wxWindow* sliderBarWindow) :
    AnkerDialogCancelOkPanel(parent, title, size)
{
    auto ankerNet = AnkerNetInst();
    if (!ankerNet) {
        return;
    }

    wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(m_centerPanel, wxID_ANY, wxDefaultPosition, AnkerSize(400, 87 * 3));
    scrolledWindow->SetScrollRate(0, 20);
    listSizer->Add(scrolledWindow, 1, wxEXPAND | wxALL);
    
    auto objSize = ankerNet->GetMultiColorPartsDeviceList().size();
    m_listBox = new AnkerMulticolorSysncFilamentListBox(scrolledWindow, wxID_ANY, wxDefaultPosition, AnkerSize(400, 87 * objSize));
    m_listBox->setSliderBarWindow(sliderBarWindow);
    scrolledWindow->SetSizer(new wxBoxSizer(wxVERTICAL));
    scrolledWindow->GetSizer()->Add(m_listBox, 0, wxEXPAND);
    m_centerPanel->GetSizer()->Add(listSizer, 1);
    //m_mainSizer->Add(listSizer, 1);
    m_centerPanel->GetSizer()->AddSpacer(24);
    bindBtnEvent();

    std::list<DeviceObjectBasePtr> multiColorDeviceObjects = ankerNet->GetMultiColorPartsDeviceList();
    int k = 0;
    int selectedIndex = -1;
    for (auto iter = multiColorDeviceObjects.begin(); iter != multiColorDeviceObjects.end(); iter++) {
        DeviceItemData data;
        if ((*iter) != nullptr) {
            data.m_deviceName = (*iter)->GetStationName();
            data.m_sn = (*iter)->GetSn();
            data.m_onlined = (*iter)->GetOnline();
            data.m_index = k++;
            auto multiColorSlotNotice = (*iter)->GetMtSlotData();
            if (multiColorSlotNotice.size() > 0) {
                data.m_haveFilament = true;
            }
            if (selectedIndex == -1 && (*iter)->GetOnline()) {
                selectedIndex = data.m_index;
            }
            for (int i = 0; i < multiColorSlotNotice.size(); i++) {
                unsigned long colorValue = multiColorSlotNotice[i].materialColor;
                unsigned char alpha = (colorValue >> 24) & 0xFF;
                unsigned char red = (colorValue >> 16) & 0xFF;
                unsigned char green = (colorValue >> 8) & 0xFF;
                unsigned char blue = colorValue & 0xFF;

                data.m_filaments.push_back(wxColour(red, green, blue, alpha));
            }
        }

        m_listBox->AddChooseDeviceItem((void*)(&data));
    }
    m_listBox->selectedChooseDeviceItem(selectedIndex);
}

std::string AnkerMulticolorSyncDeviceDialogPanel::getSelectedDevice() const
{
    return m_listBox->getSelcetedSn();
}

void AnkerMulticolorSyncDeviceDialogPanel::setSliderBarWindow(wxWindow *window)
{
    m_listBox->setSliderBarWindow(window);
}

AnkerMulticolorSyncDeviceDialog::AnkerMulticolorSyncDeviceDialog(wxWindow* parent, wxWindowID id, const wxString& title,
    const wxString& context, const wxPoint& pos, const wxSize& size, long style, const wxString& name) : 
    AnkerDialog(parent, id, title, context, pos, size, style, name)
{
}

void AnkerMulticolorSyncDeviceDialog::InitDialogPanel(int dialogType)
{
    AnkerMulticolorSyncDeviceDialogPanel* displayPanel = new AnkerMulticolorSyncDeviceDialogPanel(this, m_title, m_size, m_sliderBarWindow);
    displayPanel->setOkBtnText(_AnkerL("common_button_ok"));
    displayPanel->setCancelBtnText(_AnkerL("common_button_cancel"));
    m_panel = displayPanel;
    if (m_panel == nullptr) {
        return;
    }
    
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}

void AnkerMulticolorSyncDeviceDialog::setSliderBarWindow(wxWindow* window)
{
    m_sliderBarWindow = window;
}

int AnkerMulticolorSyncDeviceDialog::ShowAnkerModal(int dialogType)
{
    InitDialogPanel(dialogType);
    return wxDialog::ShowModal();
}

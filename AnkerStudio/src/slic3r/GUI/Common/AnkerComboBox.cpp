#include "AnkerComboBox.hpp"
#include <wx/combobox.h>
#include <wx/dcbuffer.h>
#include <wx/renderer.h>


AnkerComboBoxCtrl::AnkerComboBoxCtrl()
{
}

AnkerComboBoxCtrl::AnkerComboBoxCtrl(wxWindow* parent, wxWindowID id, 
    const wxString& value, const wxPoint& pos, 
    const wxSize& size,
    long style, const wxValidator& validator, const wxString& name) :
    wxComboBox(parent, id, value, pos, size, 0, nullptr, style, validator, name)
{
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(wxColour("#292A2D"));
    }
    //SetWindowStyleFlag(wxNO_BORDER);
    //SetWindowStyle(wxCB_READONLY | wxCB_DROPDOWN | wxCB_SIMPLE | wxHSCROLL);
    //SetWindowStyle(wxHSCROLL, false);
    //SetMaxSize(size);
   // SetMinSize(size);
   
    Bind(wxEVT_PAINT, &AnkerComboBoxCtrl::OnPaint, this);
}

AnkerComboBoxCtrl::~AnkerComboBoxCtrl()
{
}

void AnkerComboBoxCtrl::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    //dc.SetPen(wxPen(GetBackgroundColour()));
    //dc.SetBrush(wxBrush(GetBackgroundColour()));
    //dc.DrawRectangle(rect);
    //event.Skip();
}


AnkerComboBox::AnkerComboBox()
{
}

AnkerComboBox::AnkerComboBox(
    wxWindow* parent, wxWindowID id, const wxString& value, 
    const wxPoint& pos, const wxSize& size,     
    long style, const wxValidator& validator,
    const wxString& name) :
    AnkerBox(parent, id, pos, size, style, name)
{
    wxColour bkcolour = wxColour("#292A2D");
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(bkcolour);
    }
    int interval = 1;
    wxPoint comboboxBoxPos(interval, interval);
    int comboboxWidth = (float)(size.x - interval * 2) / 11.0 * 10.0;
    int height = size.y - interval * 2;
    int btnWidth = (float)(size.x - interval * 2) - comboboxWidth;
    wxPoint btnPos(interval + comboboxWidth, interval);
    m_comboboxCtrl = new AnkerComboBoxCtrl(this, wxID_ANY, wxEmptyString, comboboxBoxPos,
        wxSize(comboboxWidth, height), style, validator, name);

    m_comboboxCtrl->SetInsertionPointEnd();
    m_comboboxCtrl->Bind(wxEVT_COMBOBOX, [&](wxCommandEvent&event) {
		wxCommandEvent evt = wxCommandEvent(wxEVT_COMBOBOX);
        if(m_comboboxCtrl)
		    evt.SetEventObject(m_comboboxCtrl);
		ProcessEvent(evt);
        });

    m_rightButton = new AnkerButton(this, wxID_ANY, wxEmptyString, btnPos, wxSize(btnWidth, height-1));
	wxImage btnImage(AnkerResourceIconPath + "drop_down.png", wxBITMAP_TYPE_PNG);
	btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
	wxIcon icon;
	icon.CopyFromBitmap(wxBitmap(btnImage));
	setRightBtnIcon(icon);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);    
    sizer->Add(m_comboboxCtrl, 0);
    sizer->Add(m_rightButton, 0);
    SetSizer(sizer);
    m_rightButton->Bind(wxEVT_BUTTON, &AnkerComboBox::OnButtonClicked, this);
    borderHighEnable();
}

void AnkerComboBox::borderHighEnable(bool enable)
{
    if (enable) {
        m_comboboxCtrl->Bind(wxEVT_ENTER_WINDOW, &AnkerComboBox::OnMouseEnter, this);
        m_comboboxCtrl->Bind(wxEVT_LEAVE_WINDOW, &AnkerComboBox::OnMouseLeave, this);
        m_rightButton->Bind(wxEVT_ENTER_WINDOW, &AnkerComboBox::OnMouseEnter, this);
        m_rightButton->Bind(wxEVT_LEAVE_WINDOW, &AnkerComboBox::OnMouseLeave, this);
        
        Bind(wxEVT_PAINT, &AnkerComboBox::OnPaint, this);
    }
    else {
        m_comboboxCtrl->Unbind(wxEVT_ENTER_WINDOW, &AnkerComboBox::OnMouseEnter, this);
        m_comboboxCtrl->Unbind(wxEVT_LEAVE_WINDOW, &AnkerComboBox::OnMouseLeave, this);
        m_rightButton->Unbind(wxEVT_ENTER_WINDOW, &AnkerComboBox::OnMouseEnter, this);
        m_rightButton->Unbind(wxEVT_LEAVE_WINDOW, &AnkerComboBox::OnMouseLeave, this);
        m_rightButton->Unbind(wxEVT_BUTTON, &AnkerComboBox::OnButtonClicked, this);
        Unbind(wxEVT_PAINT, &AnkerComboBox::OnPaint, this);
    }
    setBorderColour(wxColour("#FFFFFF"), wxColour("#FFFFFF"));
    setBackgroundColour();
    setTextForegroundColour();
    setTextCtrlFont();
}

void AnkerComboBox::setTextCtrlFont(const wxFont& font)
{
    m_comboboxCtrl->SetFont(font);
    m_rightButton->SetFont(font);
    AnkerBox::SetFont(font);
}

void AnkerComboBox::setTextForegroundColour(const wxColour& colour)
{
    m_comboboxCtrl->SetForegroundColour(colour);
    m_rightButton->SetForegroundColour(colour);
    AnkerBox::SetForegroundColour(colour);
}

void AnkerComboBox::setBackgroundColour(const wxColour& colour)
{
    m_comboboxCtrl->SetBackgroundColour(colour);
    m_rightButton->SetBackgroundColour(colour);
    AnkerBox::SetBackgroundColour(colour);
}

void AnkerComboBox::setRightBtnIcon(const wxIcon& icon)
{
    m_rightButton->SetBitmap(icon);
}


int AnkerComboBox::getCurrentIndex()
{
    return m_comboboxCtrl->GetSelection();
}


wxString AnkerComboBox::getCurrentStr()
{
    return m_comboboxCtrl->GetStringSelection();
}

void AnkerComboBox::setComboxTextFont(const wxFont& textFont)
{
    m_comboboxCtrl->SetFont(textFont);
}

void AnkerComboBox::removeComboxItem(const int& index)
{
    if (!m_comboboxCtrl)
        return;

    if (index >= m_comboboxCtrl->GetCount())
        return;

    m_comboboxCtrl->Delete(index);
}


void AnkerComboBox::removeComboxItem(const wxString& itemName)
{
	if (!m_comboboxCtrl)
		return;

	for (int i = 0; i < m_comboboxCtrl->GetCount(); i++) {
		wxString strName = m_comboboxCtrl->GetString(i);
        if (itemName == strName)
        {
            m_comboboxCtrl->Delete(i);
            return;
        }
	}    
}

void AnkerComboBox::SetSelection(const int& index)
{
    if (m_comboboxCtrl)
        m_comboboxCtrl->SetSelection(index);
}


void AnkerComboBox::readOnly(bool isReadOnly)
{
    if (m_comboboxCtrl)
        m_comboboxCtrl->SetEditable(!isReadOnly);
}


void AnkerComboBox::AppendItem(const wxString& itemNmae)
{
    if (m_comboboxCtrl)
        m_comboboxCtrl->Append(itemNmae);
}


void AnkerComboBox::AppendItem(const wxStringList& itemList)
{
    if (!m_comboboxCtrl)
        return;

    auto iter = itemList.begin();
    while (iter != itemList.end())
    {
        m_comboboxCtrl->Append(*iter);
        ++iter;
    }
}

void AnkerComboBox::ClearAllItem()
{
    if(m_comboboxCtrl)
     m_comboboxCtrl->Clear();
}

void AnkerComboBox::OnButtonClicked(wxCommandEvent& event)
{
    m_comboboxCtrl->Popup();
}

AnkerCustomComboBox::AnkerCustomComboBox(wxWindow* parent, wxWindowID id, const wxString& value, 
    const wxPoint& pos, const wxSize& size,
    long style, const wxValidator& validator, const wxString& name) : AnkerBox(parent)
{
    m_comboBox = new  AnkerPenStyleCombox();
    m_comboBox->Create(this, id, value, pos, size, wxCB_READONLY | wxCB_SIMPLE | wxSIMPLE_BORDER);
    wxImage btnImage(AnkerResourceIconPath + "drop_down.png", wxBITMAP_TYPE_PNG);
    btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
    wxIcon icon;
    icon.CopyFromBitmap(wxBitmap(btnImage));
    wxBitmapBundle bmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
    wxBitmapBundle bmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
    wxBitmapBundle bmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
    if (bmpNormal.IsOk() && bmpPressed.IsOk() && bmpHover.IsOk()) {
        m_comboBox->SetButtonBitmaps(bmpNormal, true, bmpPressed, bmpHover);
    }

    m_comboBox->setTextForegroundColour();
    m_comboBox->setBackgroundColour();
	m_comboBox->Bind(wxEVT_COMBOBOX, [&](wxCommandEvent& event) {
		wxCommandEvent evt = wxCommandEvent(wxEVT_COMBOBOX);
        AnkerPenStyleCombox* pComboboxCtrl = dynamic_cast<AnkerPenStyleCombox*>(event.GetEventObject());
		if (pComboboxCtrl)
			evt.SetEventObject(pComboboxCtrl);
		ProcessEvent(evt);
		});
}


void AnkerCustomComboBox::removeComboxItem(const int& index)
{
	if (!m_comboBox)
		return;
    m_comboBox->removeComboxItem(index);
}


void AnkerCustomComboBox::removeComboxItem(const wxString& itemName)
{
	if (!m_comboBox)
		return;

    m_comboBox->removeComboxItem(itemName);
}

void AnkerCustomComboBox::SetSelection(const int& index)
{
    if (!m_comboBox)
        return;
    m_comboBox->setSelection(index);
}

void AnkerCustomComboBox::setTextForegroundColour(const wxColour& colour)
{
    m_comboBox->setTextForegroundColour(colour);
}

void AnkerCustomComboBox::setBackgroundColour(const wxColour& colour)
{
    m_comboBox->setBackgroundColour(colour);
}


wxString AnkerCustomComboBox::getStrFromIndex(const int& index)
{
    return m_comboBox->GetString(index);
}

//void AnkerCustomComboBox::setBorderColour(const wxColour& enterColour, const wxColour& leaveColour)
//{
//    m_comboBox->setBorderColour(enterColour, leaveColour);
//}


int AnkerCustomComboBox::getCurrentIndex()
{
    return m_comboBox->getCurrentIndex();
}


wxString AnkerCustomComboBox::getCurrentStr()
{
    return m_comboBox->getCurrentStr();
}

void AnkerCustomComboBox::setComboxTextFont(const wxFont& font)
{
    m_comboBox->setComboxTextFont(font);
}

void AnkerCustomComboBox::readOnly(bool isReadOnly)
{
    if (!m_comboBox)
        return;
    m_comboBox->readOnly(isReadOnly);
}

void AnkerCustomComboBox::AppendItem(const wxString& itemName)
{
	if (!m_comboBox)
		return;

    m_comboBox->AppendItem(itemName);
}

void AnkerCustomComboBox::AppendItem(const wxStringList& itemList)
{
	if (!m_comboBox)
		return;
    m_comboBox->AppendItem(itemList);
}

void AnkerCustomComboBox::ClearAllItem()
{
	if (!m_comboBox)
		return;

    m_comboBox->ClearAllItem();
}

void AnkerPenStyleCombox::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
{
    dc.SetTextForeground(m_textForegroundColor);
    dc.SetBrush(m_backgroundColor);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawText(GetString(item), rect.x + 5, rect.y + (rect.height - dc.GetCharHeight()) / 2);
}

void AnkerPenStyleCombox::OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const
{
    wxColour bgCol(m_backgroundColor);
    dc.SetBrush(wxBrush(bgCol));
    dc.SetPen(wxPen(bgCol)); 
    dc.DrawRectangle(rect);
}

void AnkerPenStyleCombox::DrawButton(wxDC& dc, const wxRect& rect, int flags)
{
    dc.SetBrush(m_backgroundColor);
    dc.SetPen(m_backgroundColor);
    dc.DrawRectangle(rect);

    flags = Button_PaintBackground | Button_BitmapOnly;
    
    wxOwnerDrawnComboBox::DrawButton(dc, rect, flags);
}

void AnkerPenStyleCombox::setTextForegroundColour(const wxColour& colour)
{
    SetForegroundColour(colour);
    m_textForegroundColor = colour;
}

void AnkerPenStyleCombox::setBackgroundColour(const wxColour& colour)
{
    SetBackgroundColour(colour);
    m_backgroundColor = colour;
}

int AnkerPenStyleCombox::getCurrentIndex()
{
    return GetSelection();
}

wxString AnkerPenStyleCombox::getCurrentStr() const
{
    return  GetValue();
}


void AnkerPenStyleCombox::setComboxTextFont(const wxFont& textFont)
{
    SetFont(textFont);
}

void AnkerPenStyleCombox::removeComboxItem(const int& index)
{
    if (index >= GetCount())
        return;

    Delete(index);
}

void AnkerPenStyleCombox::removeComboxItem(const wxString& itemName)
{
    for (int i = 0; i < GetCount(); i++) {
        wxString strName = GetString(i);
        if (itemName == strName)
        {
            Delete(i);
            return;
        }
    }
}

void AnkerPenStyleCombox::setSelection(const int& index)
{
    int count = GetCount();
    if (count <= index) {
        return;
    }
    SetSelection(index);
}

void AnkerPenStyleCombox::readOnly(bool isReadOnly)
{
    SetEditable(!isReadOnly);
}

void AnkerPenStyleCombox::AppendItem(const wxString& itemNmae)
{
    Append(itemNmae);
}

void AnkerPenStyleCombox::AppendItem(const wxStringList& itemList)
{
    auto iter = itemList.begin();
    while (iter != itemList.end())
    {
        Append(*iter);
        ++iter;
    }
}

void AnkerPenStyleCombox::ClearAllItem()
{
    Clear();
}

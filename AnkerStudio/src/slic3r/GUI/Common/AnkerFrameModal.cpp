#include "AnkerFrameModal.hpp"

HalfModalDialog::HalfModalDialog(wxWindow* parent, wxWindowID id, const wxString& title, 
    const wxString& context, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
    AnkerDialog(parent, id, title, context, pos, size, style, name)
{
   
}

HalfModalDialog::~HalfModalDialog()
{

}

void HalfModalDialog::InitDialogPanel(int dialogType)
{
    switch (dialogType)
    {
    case  PartialModal_OK:
        m_panel = new PartialModalOkPanel(this, m_title, m_size, m_context);
        break;
    case PartialModal_CANCEL_OK:
        m_panel = new PartialModalCancalOkPanel(this, m_title, m_size, m_context);
        break;
    default: break;
    }

    if (m_panel == nullptr) {
        return;
    }

    m_panel->SetOkBtnCallBack(m_okBtnCallBack);

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}

void HalfModalDialog::setBackgroundColour(const wxColour& color)
{
}

int HalfModalDialog::ShowAnkerModal(int dialogType)
{
    InitDialogPanel(dialogType);
    return wxDialog::ShowModal();
}


void HalfModalDialog::ShowAnker(int dialogType)
{
    InitDialogPanel(dialogType);
    Show(true);
}

void HalfModalDialog::ShowNoTitle(AnkerDialogIconTextOkPanel::EventCallBack_T callback)
{
    auto dialogPanel = new AnkerDialogIconTextOkPanel(callback, m_context, "", m_size, this);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(dialogPanel, 1, wxEXPAND);
    SetSizer(sizer);
    Show(true);
}

void HalfModalDialog::SetOkBtnCallBack(AnkerDialogBtnCallBack_T callback)
{
    m_okBtnCallBack = callback;
}

void HalfModalDialog::HideWindow(bool hide)
{
    Show(!hide);
}

void HalfModalDialog::CheckWindowShow()
{
    wxWindow* parent = GetParent();
    if (parent) {
        if (parent->IsShownOnScreen()) {
            if(!IsShownOnScreen())
                Show(true);
        }
        else {
            if (IsShownOnScreen())
                Show(false);
        }
    }
}


PartialModalCancalOkPanel::PartialModalCancalOkPanel(wxWindow* parent, const wxString& title, 
    const wxSize& size, const wxString& context) : 
    AnkerDialogDisplayTextCancelOkPanel(parent, title, size, context)
{
}

void PartialModalCancalOkPanel::cancelButtonClicked(wxCommandEvent& event)
{
    wxWindow* parent = GetParent();
    HalfModalDialog* dialog = static_cast<HalfModalDialog*>(parent);
    if (dialog) {
        dialog->Close();
    }
}

void PartialModalCancalOkPanel::okButtonClicked(wxCommandEvent& event)
{
    wxWindow* parent = GetParent();
    HalfModalDialog* dialog = static_cast<HalfModalDialog*>(parent);
    if (dialog) {
        dialog->Close();
    }
    if (m_okBtnCallBack) {
        m_okBtnCallBack(event);
    }
}

PartialModalOkPanel::PartialModalOkPanel(wxWindow* parent, const wxString& title, 
    const wxSize& size, const wxString& context) :
    AnkerDialogDisplayTextOkPanel(parent, title, size, context)
{
}

void PartialModalOkPanel::okButtonClicked(wxCommandEvent& event)
{
    wxWindow* parent = GetParent();
    HalfModalDialog* dialog = static_cast<HalfModalDialog*>(parent);
    if (dialog) {
        dialog->Close();
    }
    if (m_okBtnCallBack) {
        m_okBtnCallBack(event);
    }
}
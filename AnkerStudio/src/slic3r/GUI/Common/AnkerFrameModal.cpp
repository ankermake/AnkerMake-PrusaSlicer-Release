#include "AnkerFrameModal.hpp"

HalfModalDialog::HalfModalDialog(
    wxWindow* parent, 
    wxWindowID id, 
    const wxString& title, 
    const wxString& context, 
    const wxString& imageName,
    const wxPoint& pos, 
    const wxSize& size, 
    long style, 
    const wxString& name) :
    m_imageName(imageName),
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
    case PartialModal_OK:
        m_panel = new AnkerDialogDisplayTextOkPanel(this, m_title, m_size, m_context, m_okBtnCallBack);
        break;
    case PartialModal_IMAGE_OK:
        m_panel = new AnkerDialogIconTextOkPanel(this, m_imageName, m_title, m_context, m_size, m_okBtnCallBack);
        break;
    case PartialModal_CANCEL_OK:
        m_panel = new PartialModalCancalOkPanel(this, m_title, m_size, m_context);
        break;
    default: break;
    }

    if (m_panel == nullptr) {
        return;
    }

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}

void HalfModalDialog::setBackgroundColour(const wxColour& color)
{
}

void HalfModalDialog::ShowAnker(int dialogType)
{
    InitDialogPanel(dialogType);
    Show(true);
}

void HalfModalDialog::SetOkBtnCallBack(AnkerDialogBtnCallBack_T callback)
{
    m_okBtnCallBack = callback;
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

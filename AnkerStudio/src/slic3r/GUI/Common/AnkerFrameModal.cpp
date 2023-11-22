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
}

HalfModalPanel::HalfModalPanel(wxWindow* parent, wxWindowID id, const wxString& title,
    const wxString& context, const wxPoint& pos, const wxSize& size, 
    long style, const wxString& name)  : 
    wxPanel(parent, id, pos, size)
{
    m_dialog = new HalfModalDialog(this, id, title, context, pos, size, style, name);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_dialog, 1, wxEXPAND);
    SetSizer(sizer);
}

void HalfModalPanel::InitDialogPanel(int dialogType)
{
    


}

void HalfModalPanel::ShowAnkerModal(int dialogType)
{
    m_dialog->ShowAnker(dialogType);
    Show();
}

void HalfModalPanel::OnMouseEvents(wxMouseEvent& event)
{
    std::cout << "HalfModalPanel::OnMouseEvents.\n";
    event.Skip();
}

void HalfModalPanel::OnKeyDown(wxKeyEvent& event)
{
    std::cout << "HalfModalPanel::OnKeyDown.\n";
    event.Skip();
}

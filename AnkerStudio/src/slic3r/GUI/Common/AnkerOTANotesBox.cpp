#include "AnkerOTANotesBox.hpp"
#include "../GUI_App.hpp"

void SetCenterPanelContent(AnkerBox* m_centerPanel, const wxString& version,
    const wxString& context, const wxSize& size, const wxColour& bgColour) {
    const int interval = AnkerLength(16);
    const int interval2 = AnkerLength(12);

    // version info
    wxSizer* versionSizer = new wxBoxSizer(wxHORIZONTAL);
    wxString versionStr;
    versionStr.Printf(_L("common_popup_ota_newversion"), version);
    wxSize versionSize = AnkerSize(size.GetWidth() - 2 * interval, 20);
    wxStaticText* versionLabel = new wxStaticText(m_centerPanel, wxID_ANY,
        versionStr, wxDefaultPosition, versionSize);
    versionLabel->SetBackgroundColour(bgColour);
    versionLabel->SetForegroundColour(wxColour("#FFFFFF"));
    versionLabel->SetFont(ANKER_FONT_NO_1);
    versionSizer->AddSpacer(interval);
    versionSizer->Add(versionLabel);
    versionSizer->AddSpacer(interval);

    // release notes (content)
    wxSize textSize(size.GetWidth() - 2 * interval, AnkerLength(150));
    wxTextCtrl* textCtrl = new wxTextCtrl(m_centerPanel, wxID_ANY, context,
        wxDefaultPosition, textSize, wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER);
    textCtrl->SetBackgroundColour(wxColour("#292A2D"));
    textCtrl->SetForegroundColour(wxColour("#FFFFFF"));
    textCtrl->SetFont(ANKER_FONT_NO_1);

    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);
    textSizer->AddSpacer(interval);
    textSizer->Add(textCtrl);
    textSizer->AddSpacer(interval);

    m_centerPanel->GetSizer()->AddSpacer(interval2);
    m_centerPanel->GetSizer()->Add(versionSizer);
    m_centerPanel->GetSizer()->AddSpacer(interval2 / 2);
    m_centerPanel->GetSizer()->Add(textSizer);
    m_centerPanel->GetSizer()->AddSpacer(interval2);
    m_centerPanel->GetSizer()->Layout();
}

AnkerOtaNotesNormalPanel::AnkerOtaNotesNormalPanel(
    wxWindow* parent, const wxString& title, 
    const wxString& version, const wxString& context, const wxSize& size) :
    AnkerDialogCancelOkPanel(parent, title, size)
{
    setOkBtnText(_L("common_popup_ota_button1"));
    bindBtnEvent();

    SetCenterPanelContent(m_centerPanel, version, context, size, GetBackgroundColour());
    Layout();
}

AnkerOtaNotesNormalPanel::~AnkerOtaNotesNormalPanel()
{
}


AnkerOtaNotesDialog::AnkerOtaNotesDialog(wxWindow* parent, 
    wxWindowID id, 
    const wxString& title, 
    const wxString& version,
    const wxString& context, 
    const wxPoint& pos, 
    const wxSize& size, 
    long style, 
    const wxString& name)  :
    AnkerDialog(parent, id, title, context, pos, size, style, name) , m_version(version)
{
    setBackgroundColour();
}

AnkerOtaNotesDialog::~AnkerOtaNotesDialog()
{
}

void AnkerOtaNotesDialog::InitDialogPanel(int dialogType)
{
    switch (dialogType)
    {
    case OtaType_Normal:
        m_panel = new AnkerOtaNotesNormalPanel(this, m_title, m_version, m_context,  m_size);
        break;
    case OtaType_Forced:
        m_panel = new AnkerOtaNotesForcedPanel(this, m_title, m_version, m_context, m_size);
        break;
    default:
        m_panel = nullptr;
        break;
    }

    if (m_panel == nullptr) {
        return;
    }
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}

void AnkerOtaNotesDialog::setBackgroundColour(const wxColour& color)
{
    SetBackgroundColour(color);
}

int AnkerOtaNotesDialog::ShowAnkerModal(int dialogType)
{
    InitDialogPanel(dialogType);
    return wxDialog::ShowModal();
}

AnkerOtaNotesForcedPanel::AnkerOtaNotesForcedPanel(wxWindow* parent, const wxString& title, 
    const wxString& version, const wxString& context, const wxSize& size) :
    AnkerDialogOkPanel(parent, title, size)
{
    setOkBtnText(_L("common_popup_ota_button1"));
    hideCloseButton(true);
    disableCloseButton(true);

    SetCenterPanelContent(m_centerPanel, version, context, size, GetBackgroundColour());
    Layout();
}

AnkerOtaNotesForcedPanel::~AnkerOtaNotesForcedPanel()
{
}

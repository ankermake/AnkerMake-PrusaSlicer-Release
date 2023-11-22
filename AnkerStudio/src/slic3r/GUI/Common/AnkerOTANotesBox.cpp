#include "AnkerOTANotesBox.hpp"
#include "../GUI_App.hpp"

AnkerOtaNotesNormalPanel::AnkerOtaNotesNormalPanel(
    wxWindow* parent, const wxString& title, 
    const wxString& version, const wxString& context, const wxSize& size) :
    AnkerDialogCancelOkPanel(parent, title, size)
{
    setOkBtnText(_L("common_popup_ota_button1"));
    bindBtnEvent();

    int interval = 16;
    int versionBtnHeight = 20;
    wxPoint versionPos(interval, interval);
    m_centerPanel->GetSizer()->AddSpacer(interval);
    wxSizer* versionSizer = new wxBoxSizer(wxHORIZONTAL);
    versionSizer->AddSpacer(interval);
    wxString versionStr;
    versionStr.Printf(_L("common_popup_ota_newversion"), version);
    wxStaticText* versionLabel = new wxStaticText(m_centerPanel, wxID_ANY, versionStr, versionPos, AnkerSize(468, versionBtnHeight));
    versionLabel->SetBackgroundColour(GetBackgroundColour());
    versionLabel->SetForegroundColour(wxColour("#FFFFFF"));
    wxFont font = ANKER_FONT_NO_1;
    versionLabel->SetFont(font);
    versionSizer->Add(versionLabel);
    m_centerPanel->GetSizer()->Add(versionSizer);
    //m_centerPanel->GetSizer()->AddSpacer(interval / 2);

    wxPoint contextPos(interval, versionPos.y + versionBtnHeight + interval);
    int scrolledHeight = 140;

    int textWidth = size.GetWidth() - interval * 2 - 10 * 2;
    wxSize textSize = getTextSize(versionLabel, context);
    int textY = (textSize.x * textSize.y) / (textWidth - 2 * interval);
    if (textY < scrolledHeight) {
        textY = 140;
    }
    else {
        (textY % scrolledHeight) ? (textY = textY + textY % scrolledHeight + 20) : (textY = textY + 20);
    }
    
    textSize = AnkerSize(textWidth, textY);

#ifdef _WIN32
    wxTextCtrl* textCtrl = new wxTextCtrl(m_centerPanel, wxID_ANY, context, contextPos, AnkerSize(size.GetWidth(), textY), wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER);
#elif __APPLE__
    wxTextCtrl* textCtrl = new wxTextCtrl(m_centerPanel, wxID_ANY, context, contextPos, AnkerSize(size.GetWidth() - interval * 2, textY), wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER);
#endif
    //->SetWindowStyleFlag(textCtrl->GetWindowStyleFlag() & ~wxBORDER_MASK);
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);
    textSizer->AddSpacer(interval / 2);
    textCtrl->SetBackgroundColour(wxColour("#292A2D"));
    textCtrl->SetForegroundColour(wxColour("#FFFFFF"));
    textCtrl->SetFont(font);
    textSizer->Add(textCtrl, wxSizerFlags(1).Expand().Border(wxALL, interval));

    m_centerPanel->GetSizer()->Add(textSizer);
    m_centerPanel->GetSizer()->AddSpacer(interval);
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

    int interval = 16;
    int versionBtnHeight = 20;
    wxPoint versionPos(interval, interval);
    m_centerPanel->GetSizer()->AddSpacer(interval);
    wxSizer* versionSizer = new wxBoxSizer(wxHORIZONTAL);
    versionSizer->AddSpacer(interval);
    wxString versionStr;
    versionStr.Printf(_L("common_popup_ota_newversion"), version);
    wxStaticText* versionLabel = new wxStaticText(m_centerPanel, wxID_ANY, versionStr, versionPos, AnkerSize(468, versionBtnHeight));
    versionLabel->SetBackgroundColour(GetBackgroundColour());
    versionLabel->SetForegroundColour(wxColour("#FFFFFF"));
    wxFont font = ANKER_FONT_NO_1;
    versionLabel->SetFont(font);
    versionSizer->Add(versionLabel);
    m_centerPanel->GetSizer()->Add(versionSizer);
    m_centerPanel->GetSizer()->AddSpacer(interval);

    wxPoint contextPos(interval, versionPos.y + versionBtnHeight + interval);
    int scrolledHeight = 140;
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(m_centerPanel, wxID_ANY, contextPos, AnkerSize(size.GetWidth(), scrolledHeight));
    scrolledWindow->SetScrollbars(0, interval, 0, 100);

    int textWidth = size.GetWidth() - interval * 2 - 10 * 2;
    wxSize textSize = getTextSize(versionLabel, context);
    int textY = (textSize.x * textSize.y) / (textWidth - 2 * interval);
    if (textY < scrolledHeight) {
        textY = 140;
    }
    else {
        (textY % scrolledHeight) ? (textY = textY + textY % scrolledHeight + 20) : (textY = textY + 20);
    }
    textSize = AnkerSize(textWidth, textY);
    wxStaticText* staticText = new wxStaticText(scrolledWindow, wxID_ANY, context, wxPoint(interval, interval), textSize);
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);
    textSizer->AddSpacer(interval);
    staticText->Wrap(180);
    staticText->SetBackgroundColour(wxColour("#292A2D"));
    staticText->SetForegroundColour(wxColour("#FFFFFF"));
    staticText->SetFont(font);
    textSizer->Add(staticText, 1, wxALIGN_CENTER | wxALL, 10);
    textSizer->AddSpacer(interval);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    scrolledWindow->SetSizer(sizer);
    sizer->Add(textSizer, 1, wxEXPAND);
    m_centerPanel->GetSizer()->Add(scrolledWindow);
    hideCloseButton(true);
    disableCloseButton(true);
}

AnkerOtaNotesForcedPanel::~AnkerOtaNotesForcedPanel()
{
}

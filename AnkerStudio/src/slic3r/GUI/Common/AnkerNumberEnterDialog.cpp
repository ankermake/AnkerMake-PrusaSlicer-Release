#include <wx/graphics.h>
#include <wx/valnum.h>
#include "AnkerNumberEnterDialog.hpp"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "AnkerBase.hpp"

#include "libslic3r/Utils.hpp"
#include "AnkerLoadingMask.hpp"
#include "../GUI_App.hpp"
#include "../Common/AnkerGUIConfig.hpp"



AnkerNumberEnterDialog::AnkerNumberEnterDialog(wxWindow* parent, std::string title, wxString prompt, double min, double max, double value)
    : AnkerDialogBase(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, /*wxDEFAULT_DIALOG_STYLE | *//*wxSTAY_ON_TOP | */wxBORDER_NONE)
    , m_title(title)
    , m_prompt(prompt)
    , m_iconPath("")
    , m_max(max)
    , m_min(min)
    , m_textValue(value)
    , m_pTitleText(nullptr)
    , m_pPromptText(nullptr)
    , m_pExitBtn(nullptr)
    , m_pOKBtn(nullptr)
    , m_pCancelBtn(nullptr)
{
    initUI();
    initEvent();
    setOKText(_L("common_button_ok").ToStdString());
    setCancelText(_L("common_button_cancel").ToStdString());

    if (parent == nullptr)
    {
        int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
        int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
        SetPosition(wxPoint((screenW - 200) / 2, (screenH - 100) / 2));
    }
    else
    {
        SetPosition(wxPoint((parent->GetSize().x - 200) / 2, (parent->GetSize().y - 100) / 2));
    }
}

AnkerNumberEnterDialog::~AnkerNumberEnterDialog()
{
}

void AnkerNumberEnterDialog::setTitle(std::string title)
{
    m_title = title;
    if (m_pTitleText)
    {
        m_pTitleText->SetLabelText(title);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setPrompt(wxString prompt)
{
    m_prompt = prompt;
    if (m_pPromptText)
    {
        m_pPromptText->SetLabel(prompt);
        m_pPromptText->Wrap(352);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setIconPath(std::string path)
{
    m_iconPath = path;

    // update icon
}

void AnkerNumberEnterDialog::setOKText(std::string text)
{
    m_okText = text;
    if (m_pOKBtn)
    {
        m_pOKBtn->SetText(text);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setCancelText(std::string text)
{
    m_cancelText = text;
    if (m_pCancelBtn)
    {
        m_pCancelBtn->SetText(text);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setOKVisible(bool visible)
{
    if (m_pOKBtn)
    {
        m_pOKBtn->Show(visible);
        m_bIsOkShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setCancelVisible(bool visible)
{
    if (m_pCancelBtn)
    {
        m_pCancelBtn->Show(visible);
        m_bIsCancelShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

void AnkerNumberEnterDialog::setEditLineText(wxString value)
{
    m_pLineEdit->getTextEdit()->SetValue(value);
}

void AnkerNumberEnterDialog::initUI()
{
    SetWindowStyleFlag(GetWindowStyleFlag() | wxFRAME_SHAPED);


    SetMinSize(AnkerSize(400, 180));
    SetSize(AnkerSize(400, 200));
    SetBackgroundColour(wxColour(41, 42, 45));

    wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);

    SetTitle(m_title);

    wxBoxSizer* promptHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(promptHSizer, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 0);
    promptHSizer->AddSpacer(24);
    m_pPromptText = new wxStaticText(this, wxID_ANY, m_prompt);
    m_pPromptText->SetMinSize(AnkerSize(352, 21));
    m_pPromptText->SetMaxSize(AnkerSize(352, 21));
    m_pPromptText->SetBackgroundColour(wxColour(41, 42, 45));
    m_pPromptText->SetForegroundColour(wxColour("#FFFFFF"));
    //	font = m_pPromptText->GetFont();
    //#ifdef __APPLE__
    //	font.SetPointSize(14);
    //#else
    //	font.SetPointSize(10);
    //#endif
    //	m_pPromptText->SetFont(font);
    m_pPromptText->SetFont(ANKER_FONT_NO_1);
    promptHSizer->Add(m_pPromptText, 1, wxEXPAND | wxALIGN_CENTER, 17);

    contentVSizer->AddStretchSpacer(1);

    // edit text
    wxBoxSizer* editHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(editHSizer, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 0);
    editHSizer->AddSpacer(24);
    m_pLineEdit = new AnkerLineEditUnit(this, "", ANKER_FONT_NO_1, wxColour(41, 42, 45), wxColour("#3F4043"), 4, wxID_ANY);
    m_pLineEdit->setLineEditFont(ANKER_FONT_NO_1);
    m_pLineEdit->SetMaxSize(AnkerSize(352, 25));
    m_pLineEdit->SetMinSize(AnkerSize(352, 25));
    m_pLineEdit->SetSize(AnkerSize(352, 25));
    m_pLineEdit->setValue(wxString::FromDouble(m_textValue));
    editHSizer->Add(m_pLineEdit, 0, wxALIGN_RIGHT | wxCENTER | wxTOP, 0);

    contentVSizer->AddStretchSpacer(1);

    // button
    wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(btnHSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 16);

    btnHSizer->AddSpacer(24);

    m_pCancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pCancelBtn->SetText(m_cancelText);
    m_pCancelBtn->SetMinSize(AnkerSize(170, 32));
    //m_pCancelBtn->SetMaxSize(wxSize(1000, 32));
//#ifdef __APPLE__
//    m_pCancelBtn->SetMinSize(wxSize(60, 32));
//#endif
    m_pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
    m_pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
    m_pCancelBtn->SetRadius(3);
    m_pCancelBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pCancelBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
    m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerNumberEnterDialog::OnCancelButtonClicked, this);
    btnHSizer->Add(m_pCancelBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

    m_pBtnSpaceItem = btnHSizer->AddSpacer(12);

    m_pOKBtn = new AnkerBtn(this, wxID_OK, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pOKBtn->SetText(m_okText);
    //m_pOKBtn->SetMaxSize(wxSize(1000, 32));
    m_pOKBtn->SetMinSize(AnkerSize(170, 32));
    //#ifdef __APPLE__
    //m_pOKBtn->SetMinSize(wxSize(60, 32));
    //#endif
    m_pOKBtn->SetBackgroundColour("#62D361");
    m_pOKBtn->SetForegroundColour(wxColour("#FFFFFF"));
    m_pOKBtn->SetRadius(3);
    m_pOKBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pOKBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
    m_pOKBtn->Bind(wxEVT_BUTTON, &AnkerNumberEnterDialog::OnOKButtonClicked, this);
    btnHSizer->Add(m_pOKBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

    btnHSizer->AddSpacer(24);

    //contentVSizer->AddSpacer(16);

    SetSizer(contentVSizer);
}

void AnkerNumberEnterDialog::initEvent()
{
    Connect(wxEVT_SIZE, wxSizeEventHandler(AnkerNumberEnterDialog::OnSize));
    Bind(wxEVT_PAINT, &AnkerNumberEnterDialog::OnPaint, this);
}

void AnkerNumberEnterDialog::OnExitButtonClicked(wxCommandEvent& event)
{
    Hide();
    EndModal(wxID_ANY);
}

void AnkerNumberEnterDialog::OnOKButtonClicked(wxCommandEvent& event)
{
    wxString newValueStr = m_pLineEdit->getTextEdit()->GetLineText(0);
    double doubleNum;
    bool success = newValueStr.ToDouble(&doubleNum);
    if (success) {
        m_textValue = doubleNum;
    }
    Hide();
    EndModal(wxID_OK);
}

void AnkerNumberEnterDialog::OnCancelButtonClicked(wxCommandEvent& event)
{
    Hide();
    EndModal(wxID_ANY);
}

void AnkerNumberEnterDialog::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxSize size = GetSize();
    dc.SetPen(wxColour(41, 42, 45));
    dc.SetBrush(wxColour(41, 42, 45));
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
}

void AnkerNumberEnterDialog::OnSize(wxSizeEvent& event)
{
    // Handle the size event here
    wxSize newSize = event.GetSize();
    static bool isProcessing = false;
    if (!isProcessing)
    {
        isProcessing = true;

        wxGraphicsPath path = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
        path.AddRoundedRectangle(0, 0, newSize.GetWidth(), newSize.GetHeight(), 8);
        SetShape(path);
        isProcessing = false;
    }

    event.Skip();
}

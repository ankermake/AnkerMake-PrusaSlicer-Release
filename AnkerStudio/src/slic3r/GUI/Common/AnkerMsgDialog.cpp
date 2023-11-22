#include <wx/graphics.h>
#include "AnkerMsgDialog.hpp"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "slic3r/Utils/WxFontUtils.hpp"

#include "libslic3r/Utils.hpp"
#include "AnkerLoadingMask.hpp"
#include "../GUI_App.hpp"
#include "../Common/AnkerGUIConfig.hpp"



AnkerMsgDialog::AnkerMsgDialog(wxWindow* parent, std::string message, std::string title)
: AnkerDialogBase(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, /*wxDEFAULT_DIALOG_STYLE | *//*wxSTAY_ON_TOP | */wxBORDER_NONE)
, m_title(title)
, m_message(message)
, m_iconPath("")
, m_result(MSG_CLOSE)
, m_pTitleText(nullptr)
, m_pMessageText(nullptr)
, m_pExitBtn(nullptr)
, m_pOKBtn(nullptr)
, m_pCancelBtn(nullptr)
{
    initUI();
    initEvent();
    
    if (parent == nullptr)
    {
        int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
        int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
        SetPosition(wxPoint((screenW - 400) / 2, (screenH - 180) / 2));
    }
    else
    {
        SetPosition(wxPoint((parent->GetSize().x - 400) / 2, (parent->GetSize().y - 180) / 2));
    }
}

AnkerMsgDialog::~AnkerMsgDialog()
{
}

void AnkerMsgDialog::setTitle(std::string title)
{
    m_title = title;
    SetTitle(m_title);
    if (m_pTitleText)
    {
        m_pTitleText->SetLabelText(title);
        Refresh();
    }
}

void AnkerMsgDialog::setMessage(wxString message)
{
    m_message = message;
    if (m_pMessageText)
    {
        int langType = Slic3r::GUI::wxGetApp().getCurrentLanguageType();

        m_pMessageText->SetLabel(message);
        //m_pMessageText->Wrap(352);
        Slic3r::GUI::WxFontUtils::setText_wrap(m_pMessageText, 352, message, langType);
        m_pMessageText->Fit();
        Fit();
        Refresh();
    }
}

void AnkerMsgDialog::setIconPath(std::string path)
{
    m_iconPath = path;
    
    // update icon
}

void AnkerMsgDialog::setOKText(std::string text)
{
    m_okText = text;
    if (m_pOKBtn)
    {
        m_pOKBtn->SetText(text);
        Refresh();
    }
}

void AnkerMsgDialog::setCancelText(std::string text)
{
    m_cancelText = text;
    if (m_pCancelBtn)
    {
        m_pCancelBtn->SetText(text);
        Refresh();
    }
}

void AnkerMsgDialog::setOKVisible(bool visible)
{
    if (m_pOKBtn)
    {
        m_pOKBtn->Show(visible);
        m_bIsOkShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

void AnkerMsgDialog::setCancelVisible(bool visible)
{
    if (m_pCancelBtn)
    {
        m_pCancelBtn->Show(visible);
        m_bIsCancelShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

AnkerMsgDialog::MsgResult AnkerMsgDialog::getMsgResult()
{
    return m_result;
}

void AnkerMsgDialog::initUI()
{
#ifdef _WIN32
    SetWindowStyleFlag(GetWindowStyleFlag() | wxFRAME_SHAPED);
#endif
    
    SetMinSize(AnkerSize(400, 180));
    SetSize(AnkerSize(400, 200));
    SetBackgroundColour(wxColour(41, 42, 45));
    
    wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);
    
    SetTitle(m_title);
    
    m_pMessageText = new wxStaticText(this, wxID_ANY, m_message);
    m_pMessageText->SetMinSize(AnkerSize(352, 21));
    m_pMessageText->SetMaxSize(AnkerSize(352, 90));
    m_pMessageText->SetBackgroundColour(wxColour(41, 42, 45));
    m_pMessageText->SetForegroundColour(wxColour("#FFFFFF"));
    //	font = m_pMessageText->GetFont();
    //#ifdef __APPLE__
    //	font.SetPointSize(14);
    //#else
    //	font.SetPointSize(10);
    //#endif
    //	m_pMessageText->SetFont(font);
    m_pMessageText->SetFont(ANKER_FONT_NO_1);
    contentVSizer->Add(m_pMessageText, 1, wxEXPAND | wxALIGN_CENTER, 0);
    
    contentVSizer->AddStretchSpacer(1);
    
    // button
    wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(btnHSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 16);
    
    btnHSizer->AddSpacer(24);
    
    m_pCancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pCancelBtn->SetText(m_cancelText);
    m_pCancelBtn->SetMinSize(AnkerSize(170, 32));
    //m_pCancelBtn->SetMaxSize(wxSize(1000, 32));
#ifdef __APPLE__
    m_pCancelBtn->SetMinSize(wxSize(60, 32));
#endif
    m_pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
    m_pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
    m_pCancelBtn->SetRadius(3);
    m_pCancelBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pCancelBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
    m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnCancelButtonClicked, this);
    btnHSizer->Add(m_pCancelBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);
    
    m_pBtnSpaceItem = btnHSizer->AddSpacer(12);
    
    m_pOKBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
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
    m_pOKBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnOKButtonClicked, this);
    btnHSizer->Add(m_pOKBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);
    
    btnHSizer->AddSpacer(24);
    
    //contentVSizer->AddSpacer(16);
    
    SetSizer(contentVSizer);
}

void AnkerMsgDialog::initEvent()
{
}

void AnkerMsgDialog::OnExitButtonClicked(wxCommandEvent& event)
{
    m_result = MSG_CLOSE;
    
    EndModal(wxID_OK);
    Hide();
}

void AnkerMsgDialog::OnOKButtonClicked(wxCommandEvent& event)
{
    m_result = MSG_OK;
    
    EndModal(wxID_OK);
    Hide();
}

void AnkerMsgDialog::OnCancelButtonClicked(wxCommandEvent& event)
{
    m_result = MSG_CANCEL;
    
    EndModal(wxID_OK);
    Hide();
}

#include <wx/graphics.h>
#include "AnkerMsgDialog.hpp"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "slic3r/Utils/WxFontUtils.hpp"

#include "libslic3r/Utils.hpp"
#include "AnkerLoadingMask.hpp"
#include "../GUI_App.hpp"
#include "../Common/AnkerGUIConfig.hpp"

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_LEARN_MORE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_OK, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_OTHER, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_CANCEL, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_CLOSE, wxCommandEvent);

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
    SetTitle(wxString::FromUTF8(m_title));
    if (m_pTitleText)
    {
        m_pTitleText->SetLabelText(wxString::FromUTF8(title));
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
        Slic3r::GUI::WxFontUtils::setText_wrap(m_pMessageText, m_pMessageText->GetSize().GetWidth(), message, langType);
        m_pMessageText->Fit();
        Fit();
        Refresh();
    }
}

void AnkerMsgDialog::setIconPath(std::string path)
{
    m_iconPath = path;       
}

void AnkerMsgDialog::setOKText(std::string text)
{
    m_okText = text;
    if (m_pOKBtn)
    {
        m_pOKBtn->SetText(wxString::FromUTF8(text));
        Refresh();
    }
}

void AnkerMsgDialog::setCancelText(std::string text)
{
    m_cancelText = text;
    if (m_pCancelBtn)
    {
        m_pCancelBtn->SetText(wxString::FromUTF8(text));
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

AnkerCustomDialog::AnkerCustomDialog(wxWindow* parent, wxString content)
    : wxDialog(parent, wxID_ANY, wxT(""))
{
    SetBackgroundColour(wxColor(41, 42, 45));
    Bind(wxEVT_CLOSE_WINDOW, &AnkerCustomDialog::OnClose, this);
    initUi();
}

AnkerCustomDialog::~AnkerCustomDialog()
{

}
void AnkerCustomDialog::setValue(const wxString& strContent)
{
    m_pContentTextCtrl->Clear();
    m_pContentTextCtrl->SetValue(strContent);

}

void AnkerCustomDialog::clearData()
{
    m_sn = "";
    m_errorCode = "";
    m_msgLevel = "";
    m_url = "";
}

bool AnkerCustomDialog::checkErrorLevel(const std::string& errorLevel)
{
    if (m_msgLevel.empty())
        return true;

    bool isUpdateDialog = false;
    if (errorLevel == LEVEL_S || errorLevel == m_msgLevel)
        return true;
    else if (errorLevel == LEVEL_P0)
    {
        if (m_msgLevel == LEVEL_S)
            return false;
        else
            return true;
    }
    else if (errorLevel == LEVEL_P1)
    {
        if (m_msgLevel == LEVEL_P0)
            return false;
        else
            return true;
    }

    //LEVEL_P2 not in msg center
    return isUpdateDialog;
}

void AnkerCustomDialog::setValue(const wxString& strContent,                    
                                const std::string& url,
                                const std::string& errorCode,
                                const std::string& msgLevel,
                                const std::string& sn,
                                const wxString& title)
{
    bool isUpdateDialog = checkErrorLevel(msgLevel);
    if (!isUpdateDialog)
    {        
        return;
    }
    SetTitle(title);
    if (strContent.empty())
    {
        ANKER_LOG_ERROR << "no any msg center content: " << errorCode;
        return;
    }

    m_sn = sn;
    m_errorCode = errorCode;
    m_msgLevel = msgLevel;
    m_url = url;

    if (!url.empty())
    {
        m_pLearnMoreBtn->Show();
    }
    else
    {
        m_pLearnMoreBtn->Hide();
    }

    m_pContentTextCtrl->Clear();
    m_pContentTextCtrl->SetValue(strContent);

    Refresh();
    Layout();
}
void AnkerCustomDialog::initUi()
{
    m_pMainVSizer = new wxBoxSizer(wxVERTICAL);

    m_pContentTextCtrl = new wxRichTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_CHARWRAP | wxTE_READONLY | wxBORDER_NONE);
    m_pContentTextCtrl->SetBackgroundColour(wxColour("#292A2D"));
    m_pContentTextCtrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {});    
    m_pContentTextCtrl->SetEditable(false);

    wxRichTextAttr attr;
    attr.SetTextColour(wxColor("#FFFFFF"));
    m_pContentTextCtrl->SetBasicStyle(attr);
    m_pContentTextCtrl->SetFont(ANKER_FONT_NO_1);
    m_pMainVSizer->Add(m_pContentTextCtrl, wxALL | wxEXPAND, wxALL | wxEXPAND, 12);

    m_pLearnMoreBtn = new AnkerBtn(this, wxID_ANY);
    m_pLearnMoreBtn->SetText(_L("msg_error_box_btn_help"));
    m_pLearnMoreBtn->SetFont(ANKER_FONT_NO_1);
    m_pLearnMoreBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        if(!m_url.empty())
        {            
            std::string strUrl = Slic3r::UrlDecode(m_url.c_str());
            wxString utf8Url = wxString::FromUTF8(strUrl);
            wxLaunchDefaultBrowser(utf8Url.c_str());
            ANKER_LOG_INFO << "open help url on browser";
            int id = event.GetId();
            wxVariant eventData;
            eventData.ClearList();
            eventData.Append(wxVariant(m_sn));
            eventData.Append(wxVariant(m_errorCode));
            eventData.Append(wxVariant(m_msgLevel));
            wxCommandEvent otherEvent(wxCUSTOMEVT_ANKER_CUSTOM_OTHER, GetId());
            otherEvent.SetClientData(new wxVariant(eventData));
            otherEvent.SetEventObject(this);
            GetEventHandler()->ProcessEvent(otherEvent);

        }
        else
            ANKER_LOG_ERROR << "open help url error";
    });

    m_pLearnMoreBtn->SetBackgroundColour(wxColor("#62D361"));
    m_pLearnMoreBtn->SetMinSize(AnkerSize(170, 32));
    m_pLearnMoreBtn->SetMaxSize(AnkerSize(170, 32));
    m_pLearnMoreBtn->SetSize(AnkerSize(170, 32));
    m_pLearnMoreBtn->SetRadius(5);
    m_pLearnMoreBtn->SetTextColor(wxColor("#FFFFFF"));

    m_pOkBtn = new AnkerBtn(this, wxID_OK);
    m_pOkBtn->SetText(_L("msg_error_box_btn_ok"));
    m_pOkBtn->SetFont(ANKER_FONT_NO_1);
    //pOkBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
    //});

    m_pOkBtn->SetBackgroundColour(wxColor("#62D361"));
    m_pOkBtn->SetMinSize(AnkerSize(170, 32));
    m_pOkBtn->SetMaxSize(AnkerSize(170, 32));
    m_pOkBtn->SetSize(AnkerSize(170, 32));
    m_pOkBtn->SetRadius(5);
    m_pOkBtn->SetTextColor(wxColor("#FFFFFF"));

    wxBoxSizer* pBtnHSizer = new wxBoxSizer(wxHORIZONTAL);

    pBtnHSizer->AddStretchSpacer();
    pBtnHSizer->Add(m_pLearnMoreBtn);
    pBtnHSizer->AddSpacer(AnkerLength(12));
    pBtnHSizer->Add(m_pOkBtn);
    pBtnHSizer->AddStretchSpacer();

    m_pMainVSizer->Add(pBtnHSizer, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, 0);
    m_pMainVSizer->AddSpacer(12);
    SetSizer(m_pMainVSizer);

    Bind(wxEVT_BUTTON, &AnkerCustomDialog::OnButton, this);
}
void AnkerCustomDialog::OnClose(wxCloseEvent& event)
{
    wxVariant eventData;
    eventData.ClearList();
    eventData.Append(wxVariant(m_sn));
    eventData.Append(wxVariant(m_errorCode));
    eventData.Append(wxVariant(m_msgLevel));

    wxCommandEvent otherEvent(wxCUSTOMEVT_ANKER_CUSTOM_CLOSE, GetId());
    otherEvent.SetClientData(new wxVariant(eventData));
    otherEvent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(otherEvent);
}
void AnkerCustomDialog::OnButton(wxCommandEvent& event)
{
    int id = event.GetId();
    wxVariant eventData;
    eventData.ClearList();
    eventData.Append(wxVariant(m_sn));
    eventData.Append(wxVariant(m_errorCode));
    eventData.Append(wxVariant(m_msgLevel));

    if (id == wxID_OK)
    {
        wxCommandEvent okEvent(wxCUSTOMEVT_ANKER_CUSTOM_OK, GetId());

        okEvent.SetClientData(new wxVariant(eventData));
        okEvent.SetEventObject(this);        
        GetEventHandler()->ProcessEvent(okEvent);
    }
    else if (id == wxID_CANCEL)
    {
        wxCommandEvent cancelEvent(wxCUSTOMEVT_ANKER_CUSTOM_CANCEL, GetId());
        cancelEvent.SetClientData(new wxVariant(eventData));
        cancelEvent.SetEventObject(this);        
        GetEventHandler()->ProcessEvent(cancelEvent);
    }
    else
    {
        wxCommandEvent otherEvent(wxCUSTOMEVT_ANKER_CUSTOM_OTHER, GetId());
        otherEvent.SetClientData(new wxVariant(eventData));
        otherEvent.SetEventObject(this);        
        GetEventHandler()->ProcessEvent(otherEvent);
    }

    //EndModal(id);
}

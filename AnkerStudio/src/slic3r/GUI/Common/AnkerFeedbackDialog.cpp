#include <wx/graphics.h>
#include <wx/valnum.h>
#include <wx/regex.h>
#include "AnkerFeedbackDialog.hpp"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/AnkerCheckBox.hpp"
#include "slic3r/Utils/DataManger.hpp"

#include "libslic3r/Utils.hpp"
#include "AnkerLoadingMask.hpp"
#include "AnkerGUIConfig.hpp"

#include <regex>



AnkerFeedbackDialog::AnkerFeedbackDialog(wxWindow* parent, std::string title, wxPoint position, wxSize size)
    : AnkerDialogBase(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, /*wxDEFAULT_DIALOG_STYLE | *//*wxSTAY_ON_TOP | */wxBORDER_NONE)
    , m_title(title)
    , m_iconPath("")
    , m_pTitleText(nullptr)
    , m_pExitBtn(nullptr)
    , m_pOKBtn(nullptr)
    , m_pCancelBtn(nullptr)
{
    initUI();
    initEvent();
    setOKText(_L("common_feedback_button_submit").ToStdString());
    setCancelText(_L("common_button_cancel").ToStdString());

    SetPosition(position);
    SetSize(size);
    Layout();
}

AnkerFeedbackDialog::~AnkerFeedbackDialog()
{
}

void AnkerFeedbackDialog::setTitle(std::string title)
{
    m_title = title;
    if (m_pTitleText)
    {
        m_pTitleText->SetLabelText(title);
        Refresh();
    }
}

void AnkerFeedbackDialog::setIconPath(std::string path)
{
    m_iconPath = path;

    // update icon
}

void AnkerFeedbackDialog::setOKText(std::string text)
{
    m_okText = text;
    if (m_pOKBtn)
    {
        m_pOKBtn->SetText(text);
        Refresh();
    }
}

void AnkerFeedbackDialog::setCancelText(std::string text)
{
    m_cancelText = text;
    if (m_pCancelBtn)
    {
        m_pCancelBtn->SetText(text);
        Refresh();
    }
}

void AnkerFeedbackDialog::setOKVisible(bool visible)
{
    if (m_pOKBtn)
    {
        m_pOKBtn->Show(visible);
        m_bIsOkShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

void AnkerFeedbackDialog::setCancelVisible(bool visible)
{
    if (m_pCancelBtn)
    {
        m_pCancelBtn->Show(visible);
        m_bIsCancelShow = visible;
        m_pBtnSpaceItem->Show(m_bIsOkShow && m_bIsCancelShow);
        Refresh();
    }
}

void AnkerFeedbackDialog::initUI()
{
    SetBackgroundColour(wxColour("#333438"));

    wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);

    SetTitle(m_title);

    // edit text
    wxBoxSizer* editHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(editHSizer, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 0);
    editHSizer->AddSpacer(24);
    m_pTextEdit = new wxRichTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_CHARWRAP | wxBORDER_NONE);
    m_pTextEdit->SetBackgroundColour(wxColour("#292A2D"));
    //m_pTextEdit->SetForegroundColour(wxColour("#FFFFFF"));
    wxRichTextAttr attr;
    attr.SetTextColour(*wxWHITE);
    m_pTextEdit->SetBasicStyle(attr);
    m_pTextEdit->SetFont(ANKER_FONT_NO_1);
    m_pTextEdit->SetMaxSize(AnkerSize(351, 200));
    m_pTextEdit->SetMinSize(AnkerSize(351, 200));
    m_pTextEdit->SetSize(AnkerSize(351, 200));
    m_pTextEdit->SetHint(_L("common_feedback_notice_description"));
    editHSizer->Add(m_pTextEdit, 0, wxALIGN_RIGHT | wxTOP | wxCENTER, 0);

    // email 
    wxBoxSizer* emailHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(emailHSizer, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 0);
    emailHSizer->AddSpacer(24);
    m_pEmailLineEdit = new AnkerLineEditUnit(this, "", ANKER_FONT_NO_1, wxColour("#333438"), wxColour("#545863"), 4, wxID_ANY);
    m_pEmailLineEdit->setLineEditFont(ANKER_FONT_NO_1);
    m_pEmailLineEdit->SetMaxSize(AnkerSize(352, 30));
    m_pEmailLineEdit->SetMinSize(AnkerSize(352, 30));
    m_pEmailLineEdit->SetSize(AnkerSize(352, 30));
    m_pEmailLineEdit->SetValue(Datamanger::GetInstance().m_userInfo.email);
    m_pEmailLineEdit->getTextEdit()->SetHint(_L("Email"));
    emailHSizer->Add(m_pEmailLineEdit, 0, wxALIGN_RIGHT | wxTOP | wxCENTER, 0);

    // share logs
    wxBoxSizer* shareLogHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(shareLogHSizer, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 0);
    shareLogHSizer->AddSpacer(24);
    //check box
    wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
    uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap uncheckScaledBitmap(uncheckImage);
    wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check.png")), wxBITMAP_TYPE_PNG);
    checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap checkScaledBitmap(checkImage);
    wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
    disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap disUncheckScaledBitmap(disuncheckImage);
    wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
    discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap disCheckScaledBitmap(discheckImage);

    wxWindowID id = wxNewId();
    AnkerCheckBox* shareLogCheckBox = new AnkerCheckBox(this,
        uncheckScaledBitmap.ConvertToImage(),
        checkScaledBitmap.ConvertToImage(),
        disUncheckScaledBitmap.ConvertToImage(),
        disCheckScaledBitmap.ConvertToImage(),
        wxString(),
        ANKER_FONT_NO_1,
        wxColour("#FFFFFF"),
        id);
    shareLogCheckBox->SetWindowStyleFlag(wxBORDER_NONE);
    shareLogCheckBox->SetBackgroundColour(wxColour("#292A2D"));
    shareLogCheckBox->SetMinSize(wxSize(16, 16));
    shareLogCheckBox->SetMaxSize(wxSize(16, 16));
    shareLogCheckBox->SetSize(wxSize(16, 16));
    shareLogCheckBox->setCheckStatus(true);
    shareLogCheckBox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, [shareLogCheckBox, this](wxCommandEvent& event) {
        m_bIsShareLog = shareLogCheckBox->getCheckStatus();
        });
    shareLogHSizer->Add(shareLogCheckBox, 0,  wxALIGN_CENTER_VERTICAL, 0);

    wxStaticText* shareLogText = new wxStaticText(this, wxID_ANY, m_title, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    shareLogText->SetMinSize(AnkerSize(257, 20));
    shareLogText->SetBackgroundColour(wxColour("#333438"));
    shareLogText->SetForegroundColour(wxColour("#FFFFFF"));
    shareLogText->SetFont(ANKER_FONT_NO_1);
    shareLogText->SetLabelText(_L("common_feedback_select_applogs"));
    shareLogHSizer->Add(shareLogText, 107,  wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 0);

    contentVSizer->AddStretchSpacer(1);

    // button
    wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
    contentVSizer->Add(btnHSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 16);

    btnHSizer->AddSpacer(24);

    m_pCancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pCancelBtn->SetText(_L("common_button_cancel"));
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
    m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerFeedbackDialog::OnCancelButtonClicked, this);
    btnHSizer->Add(m_pCancelBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

    m_pBtnSpaceItem = btnHSizer->AddSpacer(12);

    m_pOKBtn = new AnkerBtn(this, wxID_OK, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_pOKBtn->SetText(_L("common_feedback_button_submit"));
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
    m_pOKBtn->Enable(false);
    m_pOKBtn->Bind(wxEVT_BUTTON, &AnkerFeedbackDialog::OnOKButtonClicked, this);
    btnHSizer->Add(m_pOKBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

    btnHSizer->AddSpacer(24);

    //contentVSizer->AddSpacer(16);

    SetSizer(contentVSizer);

    auto size = GetSize();
    auto aa = size;
}

void AnkerFeedbackDialog::initEvent()
{
    Connect(wxEVT_SIZE, wxSizeEventHandler(AnkerFeedbackDialog::OnSize));
    Bind(wxEVT_PAINT, &AnkerFeedbackDialog::OnPaint, this);

    //clear hint text
    m_pTextEdit->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {
        auto hint = m_pTextEdit->GetHint();
        auto text = m_pTextEdit->GetValue();
        if (hint == text) {
            m_pTextEdit->SetValue("");
        }
        event.Skip();
        });
    m_pTextEdit->Bind(wxEVT_TEXT, [this](wxCommandEvent& event) {
        auto text = m_pTextEdit->GetValue();
        if (text.Length() > 5000) {
            text = text.Left(5000);
            m_pTextEdit->SetValue(text);
            m_pTextEdit->SetInsertionPointEnd();
        }
        checkOkBtn();
        });

    m_pEmailLineEdit->Bind(wxEVT_TEXT, [this](wxCommandEvent &event) {
        auto email = m_pEmailLineEdit->GetValue();
		if (isValidEmail(email)) {
            m_pEmailLineEdit->setLineEditTextColor(*wxWHITE);
		}
		else {
            m_pEmailLineEdit->setLineEditTextColor(wxColour("#FF0E00"));
		}
        checkOkBtn();
        });
}

void AnkerFeedbackDialog::OnExitButtonClicked(wxCommandEvent& event)
{
    Hide();
    EndModal(wxID_ANY);
}

void AnkerFeedbackDialog::OnOKButtonClicked(wxCommandEvent& event)
{
    Hide();
    EndModal(wxID_OK);
    m_feedback.sendLogs = m_bIsShareLog;
    m_feedback.email = m_pEmailLineEdit->GetValue().utf8_str();
    m_feedback.content = m_pTextEdit->GetValue().utf8_str();
}

void AnkerFeedbackDialog::OnCancelButtonClicked(wxCommandEvent& event)
{
    Hide();
    EndModal(wxID_ANY);
}

bool AnkerFeedbackDialog::isValidEmail(wxString email)
{
    const std::regex pattern("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}");
    return std::regex_match(email.ToStdString(), pattern);
}

void AnkerFeedbackDialog::checkOkBtn()
{
    auto text = m_pTextEdit->GetValue();
    auto email = m_pEmailLineEdit->GetValue();
    if (text.length() > 0 && isValidEmail(email)) {
        m_pOKBtn->SetBackgroundColour("#62D361");
        m_pOKBtn->Enable(true);
    }
    else {
        m_pOKBtn->SetBackgroundColour("#3F4044");
        m_pOKBtn->Enable(false);
    }
}

void AnkerFeedbackDialog::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxSize size = GetSize();
    dc.SetPen(wxColour("#333438"));
    dc.SetBrush(wxColour("#333438"));
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
    
    checkOkBtn();
}

void AnkerFeedbackDialog::OnSize(wxSizeEvent& event)
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

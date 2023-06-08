#include "AnkerMsgDialog.hpp"
#include "AnkerBtn.hpp"

#include "libslic3r/Utils.hpp"
#include "AnkerLoadingMask.hpp"


AnkerMsgDialog::AnkerMsgDialog(wxWindow* parent, std::string message, std::string title)
	: wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE)
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
	if (m_pTitleText)
	{
		m_pTitleText->SetLabelText(title);
		Refresh();
	}
}

void AnkerMsgDialog::setMessage(std::string message)
{
	m_message = message;
	if (m_pMessageText)
	{
		m_pMessageText->SetLabelText(message);
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
		Refresh();
	}
}

void AnkerMsgDialog::setCancelVisible(bool visible)
{
	if (m_pCancelBtn)
	{
		m_pCancelBtn->Show(visible);
		Refresh();
	}
}

AnkerMsgDialog::MsgResult AnkerMsgDialog::getMsgResult()
{
	return m_result;
}

void AnkerMsgDialog::initUI()
{
	SetMinSize(wxSize(400, 180));
	SetBackgroundColour(wxColour(41, 42, 45));

	wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);

	contentVSizer->AddSpacer(16);

	// title
	wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
	contentVSizer->Add(titleHSizer, 0, wxEXPAND | wxALIGN_TOP | wxALL, 0);

	titleHSizer->AddStretchSpacer(167);

	m_pTitleText = new wxStaticText(this, wxID_ANY, m_title);
	m_pTitleText->SetMinSize(wxSize(67, 30));
	m_pTitleText->SetBackgroundColour(wxColour(41, 42, 45));
	m_pTitleText->SetForegroundColour(wxColour("#FFFFFF"));
	wxFont font = m_pTitleText->GetFont();
	font.SetPointSize(14);
	m_pTitleText->SetFont(font);
	titleHSizer->Add(m_pTitleText, 107, wxEXPAND | wxALIGN_CENTER, 0);

	titleHSizer->AddStretchSpacer(123);

	wxImage exitImage = wxImage(wxString::FromUTF8(Slic3r::var("fdm_nav_del_icon.png")), wxBITMAP_TYPE_PNG);
	exitImage.Rescale(20, 20);
	m_pExitBtn = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_pExitBtn->SetBitmap(exitImage);
	m_pExitBtn->SetMinSize(exitImage.GetSize());
	m_pExitBtn->SetBackgroundColour(wxColour(41, 42, 45));
	m_pExitBtn->SetForegroundColour(wxColour("#FFFFFF"));
	m_pExitBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnExitButtonClicked, this);
	titleHSizer->Add(m_pExitBtn, 0, wxALIGN_RIGHT | wxRIGHT, 26);

	contentVSizer->AddSpacer(18);

	// split line
	wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(64, 65, 70));
	splitLineCtrl->SetMaxSize(wxSize(100000, 1));
	splitLineCtrl->SetMinSize(wxSize(1, 1));
	contentVSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 0);

	contentVSizer->AddSpacer(16);
	
	m_pMessageText = new wxStaticText(this, wxID_ANY, m_message);
	m_pMessageText->SetMinSize(wxSize(352, 21));
	m_pMessageText->SetBackgroundColour(wxColour(41, 42, 45));
	m_pMessageText->SetForegroundColour(wxColour("#FFFFFF"));
	font = m_pMessageText->GetFont();
	font.SetPointSize(10);
	m_pMessageText->SetFont(font);
	contentVSizer->Add(m_pMessageText, 1, wxEXPAND | wxALIGN_TOP | wxLEFT | wxRIGHT, 24);

	//contentVSizer->AddStretchSpacer(1);

	// button
	wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
	contentVSizer->Add(btnHSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBottom, 0);

	btnHSizer->AddStretchSpacer(24);

	m_pCancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pCancelBtn->SetText(m_cancelText);
	m_pCancelBtn->SetMaxSize(wxSize(1000, 32));
	#ifdef __APPLE__
	m_pCancelBtn->SetMinSize(wxSize(60, 32));
	#endif
	m_pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
	m_pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
	m_pCancelBtn->SetRadius(3);
	m_pCancelBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pCancelBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnCancelButtonClicked, this);
	btnHSizer->Add(m_pCancelBtn, 170, wxEXPAND | wxALIGN_CENTER | wxRIGHT, 12);

	m_pOKBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pOKBtn->SetText(m_okText);
	m_pOKBtn->SetMaxSize(wxSize(1000, 32));
	#ifdef __APPLE__
	m_pOKBtn->SetMinSize(wxSize(60, 32));
	#endif
	m_pOKBtn->SetBackgroundColour("#62D361");
	m_pOKBtn->SetForegroundColour(wxColour("#FFFFFF"));
	m_pOKBtn->SetRadius(3);
	m_pOKBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pOKBtn->Bind(wxEVT_BUTTON, &AnkerMsgDialog::OnOKButtonClicked, this);
	btnHSizer->Add(m_pOKBtn, 170, wxEXPAND | wxALIGN_CENTER, 0);

	btnHSizer->AddStretchSpacer(24);

	contentVSizer->AddSpacer(16);

	SetSizer(contentVSizer);
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

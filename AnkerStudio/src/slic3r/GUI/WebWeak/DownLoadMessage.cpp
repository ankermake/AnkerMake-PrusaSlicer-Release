#include "DownLoadMessage.hpp"
#include "libslic3r/Utils.hpp"
#include <wx/graphics.h>
#include <slic3r/Utils/WxFontUtils.hpp>
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"

namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_DOWNLOAD_CANCEL, wxCommandEvent);
DownLoadMsgDialog::DownLoadMsgDialog(wxWindow* parent, std::string message, std::string title)
	: AnkerDialogBase(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, /*wxDEFAULT_DIALOG_STYLE | *//*wxSTAY_ON_TOP | */wxBORDER_NONE)
	, m_title(title)
	, m_message(message)
{
	initUI();

	if (parent == nullptr) {
		int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
		int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
		SetPosition(wxPoint((screenW - 400) / 2, (screenH - 180) / 2));
	}
	else {
		SetPosition(wxPoint((parent->GetSize().x - 400) / 2, (parent->GetSize().y - 180) / 2));
	}
}

DownLoadMsgDialog::~DownLoadMsgDialog() {}

void DownLoadMsgDialog::updateMsg(const wxString& message)
{
	m_message = message;
	if (m_pMessageText) {
		int langType = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
		m_pMessageText->SetLabel(message);
		Slic3r::GUI::WxFontUtils::setText_wrap(m_pMessageText, m_pMessageText->GetSize().GetWidth(), message, langType);
		m_pMessageText->Fit();
		Fit();
	}
}

void DownLoadMsgDialog::setExitVisible(bool visible)
{
	if (m_pExitBtn) {
		m_pExitBtn->Show(visible);
		Refresh();
	}
}

void DownLoadMsgDialog::setCancelText(std::string text)
{
	m_cancelText = text;
	if (m_pCancelBtn) {
		m_pCancelBtn->SetText(text);
		Refresh();
	}
}

void DownLoadMsgDialog::initUI()
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
	m_pCancelBtn->SetBackgroundColour("#62D361");
	m_pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
	m_pCancelBtn->SetRadius(3);
	m_pCancelBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pCancelBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
	m_pCancelBtn->Bind(wxEVT_BUTTON, &DownLoadMsgDialog::OnCancelButtonClicked, this);
	btnHSizer->Add(m_pCancelBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

	m_pBtnSpaceItem = btnHSizer->AddSpacer(12);
	SetSizer(contentVSizer);
}

void DownLoadMsgDialog::OnExitButtonClicked(wxCommandEvent& event)
{
	EndModal(wxID_OK);
	Hide();

	m_bCanShow = false;
	wxCommandEvent* evt = new wxCommandEvent(EVT_DOWNLOAD_CANCEL);
	evt->SetInt(static_cast<int>(CancelType::Close_Type));
	wxQueueEvent(this, evt);
}

void DownLoadMsgDialog::OnCancelButtonClicked(wxCommandEvent& event)
{
	EndModal(wxID_OK);
	Hide();

	m_bCanShow = false;
	wxCommandEvent* evt = new wxCommandEvent(EVT_DOWNLOAD_CANCEL);
	evt->SetInt(static_cast<int>(CancelType::Cancel_Type));
	wxQueueEvent(this, evt);
}

DownLoadMessageManager::~DownLoadMessageManager()
{
	if (m_pdlg) {
		delete m_pdlg;
		m_pdlg = nullptr;
	}
}

void DownLoadMessageManager::Init()
{
	std::string title = _L("common_web_import_model_title").ToStdString();
	std::string cancelText = _L("common_button_cancel").ToStdString();
	if (m_pdlg == nullptr) {
		m_pdlg = new DownLoadMsgDialog(nullptr, "", title);
		m_pdlg->setCancelText(cancelText);
		m_pdlg->setCanShow(true);
		m_pdlg->setExitVisible(false);
		m_pdlg->Bind(EVT_DOWNLOAD_CANCEL, [this](wxCommandEvent& event) {
			if (m_cb) { m_cb(static_cast<CancelType>(event.GetInt())); }
		});
	}
	else {
		m_pdlg->setCanShow(true);
	}
}

void DownLoadMessageManager::onHideProcess()
{
	if (m_pdlg && m_pdlg->IsModal()) {
		m_pdlg->Hide();
	}
}

void DownLoadMessageManager::onDownLoadComplete()
{
	if (m_pdlg) {
		m_pdlg->EndModal(wxID_OK);
	}
}

void DownLoadMessageManager::ShowError(const std::string& title, const wxString& msg)
{
	if (m_pdlg && m_pdlg->IsModal()) {
		m_pdlg->EndModal(wxID_OK);
		m_pdlg->Hide();
	}
    
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, msg.ToStdString(wxConvUTF8), title, false);
}

void DownLoadMessageManager::SetPercent(const wxString& msg)
{	
    if (m_pdlg) {
		if (!m_pdlg->IsModal() && m_pdlg->IsCanShow()) {
			m_pdlg->ShowModal();
		}

		m_pdlg->updateMsg(msg);
	}
}
}
}

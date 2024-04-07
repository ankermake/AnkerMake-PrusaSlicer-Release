#include "AnkerNetDownloadDialog.h"
#include "libslic3r/Utils.hpp"
#include <wx/graphics.h>
#include <wx/wx.h>
#include "slic3r/Utils/WxFontUtils.hpp"
//#include "slic3r/GUI/Common/AnkerSliderCtrl.hpp"
#include "slic3r/GUI/Common/AnkerProgressCtrl.hpp"
#include "../GUI_App.hpp"

AnkerNetBtnPanel::AnkerNetBtnPanel(wxWindow* parent, 
	const wxString& title,
	const wxSize& size, 
	bool hideOkBtn) :
	AnkerDialogPanel(parent, title, size)
{
	int marginWidth = AnkerLength(24);	
	int heightInterval = AnkerLength(16);
	int btnInterval = AnkerLength(12);
	int btnWidth = AnkerLength(80);
	int btnHeight = AnkerLength(32);
	int btnPosY = size.GetHeight() - btnHeight - heightInterval;
	
	btnWidth = size.GetWidth() - 2 * marginWidth;
	// ok button
	if (!hideOkBtn) {
		btnWidth = (btnWidth - btnInterval) / 2 ;

		wxSize okSize(btnWidth, btnHeight);
		m_okBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, okSize, wxBORDER_NONE);
		m_okBtn->SetText(_AnkerL("common_button_ok"));
		m_okBtn->SetFont(ANKER_FONT_NO_1);
		m_okBtn->SetBackgroundColour("#62D361");
		m_okBtn->SetTextColor("#FFFFFF");
		m_okBtn->SetRadius(5);
	}
	
	// cancel button
	wxSize cancelSize(btnWidth, btnHeight);
	m_cancelBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, cancelSize, wxBORDER_NONE);
	m_cancelBtn->SetText(_AnkerL("common_button_cancel"));
	m_cancelBtn->SetFont(ANKER_FONT_NO_1);
	m_cancelBtn->SetTextColor(wxColour("#FFFFFF"));
	m_cancelBtn->SetBackgroundColour("#62D361");
	m_cancelBtn->SetRadius(5);

	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->Add(m_cancelBtn, 1, wxEXPAND);
	if (!hideOkBtn) {
		btnSizer->AddSpacer(btnInterval);
		btnSizer->Add(m_okBtn, 0, wxEXPAND);		
	}	

	int centerPanelPosY = m_line->GetPosition().y + m_line->GetSize().GetHeight();
	wxPoint centerPanelPos(0, centerPanelPosY);
	wxSize centerPanelSize(size.GetWidth(), btnPosY - centerPanelPos.y - AnkerLength(12));

	m_centerPanel = new AnkerBox(this, wxID_ANY, wxDefaultPosition, centerPanelSize);
	wxBoxSizer* centerPanelSizer = new wxBoxSizer(wxVERTICAL);
	m_centerPanel->SetSizer(centerPanelSizer);
	//m_centerPanel->SetBackgroundColour(wxColour(255, 0, 0));

	m_mainSizer->Add(m_centerPanel, 0, wxLEFT | wxRIGHT, marginWidth);
	m_mainSizer->Add(btnSizer, 0, wxLEFT | wxRIGHT, marginWidth);
	m_mainSizer->AddSpacer(heightInterval);

	BindBtnEvent();
	setOkBtnText();
	setCancelBtnText();

	Layout();
}

void AnkerNetBtnPanel::BindBtnEvent(OkFunc_T okFunc, CancelFunc_T cancelFunc)
{
	if (closeBtn) {
		if (cancelFunc) {
			closeBtn->Bind(wxEVT_BUTTON, [cancelFunc](wxCommandEvent& event) {
				cancelFunc();
			});
		}
		else {
			closeBtn->Bind(wxEVT_BUTTON, &AnkerNetBtnPanel::cancelButtonClicked, this);
		}
	}
	if (m_cancelBtn) {
		if (cancelFunc) {
			m_cancelBtn->Bind(wxEVT_BUTTON, [cancelFunc](wxCommandEvent& event) {
				cancelFunc();
			});
		}
		else {
			m_cancelBtn->Bind(wxEVT_BUTTON, &AnkerNetBtnPanel::cancelButtonClicked, this);
		}
	}
	if (m_okBtn) {
		if (okFunc) {
			m_okBtn->Bind(wxEVT_BUTTON, [okFunc](wxCommandEvent& event) {
				okFunc();
			});
		}
		else {
			m_okBtn->Bind(wxEVT_BUTTON, &AnkerNetBtnPanel::okButtonClicked, this);
		}
	}
}

void AnkerNetBtnPanel::setOkBtnText(const wxString& text)
{
	if (m_okBtn) {
		m_okBtn->SetText(text);
	}
}

void AnkerNetBtnPanel::setCancelBtnText(const wxString& text)
{
	if (m_cancelBtn) {
		m_cancelBtn->SetText(text);
	}
}

void AnkerNetBtnPanel::cancelButtonClicked(wxCommandEvent& event)
{
	wxWindow* parent = GetParent();
	wxDialog* parentDialog = dynamic_cast<wxDialog*>(parent);
	if (parentDialog) {
		parentDialog->EndModal(wxID_CANCEL);
	}
	else {
		ANKER_LOG_INFO << "close parent window";
		closeWindow();
	}
}

void AnkerNetBtnPanel::okButtonClicked(wxCommandEvent& event)
{
	wxWindow* parent = GetParent();
	wxDialog* parentDialog = dynamic_cast<wxDialog*>(parent);
	if (parentDialog) {
		ANKER_LOG_INFO << "END EndModal parentDialog";
		parentDialog->EndModal(wxID_OK);
	}
	else {
		ANKER_LOG_INFO << "closeWindow of parentDialog";
		closeWindow();
	}
}

AnkerNetInstallPanel::AnkerNetInstallPanel(wxWindow* parent, 
	const wxString& title,
	const wxString& context, 
	const wxSize& size) :
	AnkerNetBtnPanel(parent, title, size)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(12);

	// text
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();

	int contextWidth = size.GetWidth() - 2 * interval;
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, context, 
		wxDefaultPosition, wxSize(contextWidth, AnkerLength(80)), wxST_NO_AUTORESIZE);
	m_contextText->SetFont(ANKER_FONT_NO_2);
	m_contextText->SetForegroundColour("#FFFFFF");
	m_contextText->SetBackgroundColour(bkColour);
    
#ifdef __WXMAC__
    setStrWrap(m_contextText, contextWidth - 20, context, type);
#endif

	wxBoxSizer* contextSizer = new wxBoxSizer(wxVERTICAL);
	contextSizer->Add(m_contextText, 0, wxALIGN_CENTER_VERTICAL);

	m_centerPanel->GetSizer()->Add(contextSizer, 0, wxTOP | wxBOTTOM, interval);

	setCancelBtnText(_L("common_print_netplugin_notnow") /*"Not Now"*/);
	setOkBtnText(_L("common_print_netplugin_install") /*"Install"*/);
	Layout();
}

AnkerNetProgressPanel::AnkerNetProgressPanel(wxWindow* parent,
	const wxString& title,
	const wxSize& size) :
	AnkerNetBtnPanel(parent, title, size, true)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(12);

	// progress
	m_progressBar = new AnkerProgressCtrl(m_centerPanel, true);
	m_progressBar->SetMinSize(AnkerSize(size.GetWidth() - 2*12, 21));
	m_progressBar->SetMaxSize(AnkerSize(size.GetWidth(), 21));
	m_progressBar->setLineWidth(5);
	m_progressBar->SetBackgroundColor(bkColour);
	m_progressBar->setProgressColor(wxColour(ANKER_RGB_INT));
	m_progressBar->setLabelFont(ANKER_FONT_NO_1);
	m_progressBar->setProgressRange(100.0);
	m_progressBar->updateProgress(0);	

	// text
	const wxString context(_L("common_print_netplugin_downloading") /*"Downloading..."*/);

	int contextWidth = size.GetWidth() - 2 * interval;
	m_progressText = new AnkerStaticText(m_centerPanel, wxID_ANY, context);
	m_progressText->SetFont(ANKER_FONT_NO_2);
	m_progressText->SetForegroundColour("#FFFFFF");
	m_progressText->SetBackgroundColour(bkColour);
#ifdef __WXMAC__
	m_progressText->Wrap(contextWidth);
#endif

	wxBoxSizer* contextSizer = new wxBoxSizer(wxVERTICAL);
	contextSizer->AddSpacer(interval);
	contextSizer->Add(m_progressText);
	contextSizer->Add(m_progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
	contextSizer->AddSpacer(interval);

	m_centerPanel->GetSizer()->Add(contextSizer, 0, wxTOP | wxBOTTOM, interval);

	Layout();
}

void AnkerNetProgressPanel::UpdateProgress(double progress)
{
	if (m_progressBar) {		
		m_progressBar->Refresh();
		m_progressBar->updateProgress(progress);
		m_progressBar->Update();
	}
}

AnkerNetStatusPanel::AnkerNetStatusPanel(
	bool success,
	bool hideOkBtn,
	wxWindow* parent,
	const wxString& title,
	const wxString& context,
	const wxSize& size) :
	m_success(success),
	AnkerNetBtnPanel(parent, title, size, hideOkBtn)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(12);

	// status image
	auto imageName = wxString::FromUTF8(Slic3r::var("result_success_icon.png"));
	if (!m_success) {
		imageName = wxString::FromUTF8(Slic3r::var("result_failed_icon.png"));
	}
	wxImage image = wxImage(imageName, wxBITMAP_TYPE_PNG);
	image.Rescale(120, 120);
	m_statusImage = new wxStaticBitmap(m_centerPanel, wxID_ANY, image);
	m_statusImage->SetMinSize(image.GetSize());
	m_statusImage->SetMaxSize(image.GetSize());
	m_statusImage->SetBackgroundColour(bkColour);

	// text
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
	int contextWidth = size.GetWidth() - 2 * interval;
	m_progressText = new AnkerStaticText(m_centerPanel, wxID_ANY, context);
	m_progressText->SetFont(ANKER_FONT_NO_2);
	m_progressText->SetForegroundColour("#FFFFFF");
	m_progressText->SetBackgroundColour(bkColour);
	setStrWrap(m_progressText, contextWidth - 20, context, type);
#ifdef __WXMAC__
	//m_progressText->Wrap(contextWidth);
#endif

	wxBoxSizer* contextSizer = new wxBoxSizer(wxVERTICAL);
	auto imageInterval = size.GetWidth() - 2 * interval - m_statusImage->GetSize().GetWidth();
	contextSizer->Add(m_statusImage, 0, wxLEFT | wxRIGHT, imageInterval/2);
	contextSizer->Add(m_progressText, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	m_centerPanel->GetSizer()->Add(contextSizer, 0, wxTOP | wxBOTTOM, interval);

	Layout();
}

AnkerNetSuccessPanel::AnkerNetSuccessPanel(
	wxWindow* parent,
	const wxString& title,
	const wxSize& size) :
	AnkerNetStatusPanel(true, true, parent, title, 
		_L("common_print_netplugin_install_success") /*"Installed Successfully"*/, size)
{
	setCancelBtnText(_L("common_print_netplugin_install_finish") /*"Finish"*/);
}

AnkerNetFailedPanel::AnkerNetFailedPanel(
	wxWindow* parent,
	const wxString& title,
	const wxSize& size) :
	AnkerNetStatusPanel(false, false, parent, title, 
		_L("common_print_netplugin_download_failed") /*"Failed to download Network Plug - in, please try again."*/, size)
{
	setOkBtnText(_L("common_button_retry"));
}


AnkerNetDownloadDialog::AnkerNetDownloadDialog(
	wxWindow* parent, 
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size, 
	long style, 
	const wxString& name) :
	m_title(_L("common_print_netplugin_networkplugin") /*"Network Plug-In"*/),
	m_size(size), 
	defaultPos(pos),
	wxDialog(parent, id, "", pos, size, style, name)
{
	ANKER_LOG_INFO << "net download dialog create";
	SetBackgroundColour("#333438");
	Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
		EndModal(wxID_CLOSE);
	});
}

AnkerNetDownloadDialog::~AnkerNetDownloadDialog()
{
	ANKER_LOG_INFO << "net download dialog destory";
}

void AnkerNetDownloadDialog::Change(Status status,
	AnkerNetBtnPanel::OkFunc_T okFunc,
	AnkerNetBtnPanel::CancelFunc_T cancelFunc)
{
	this->CallAfter([this, status, okFunc, cancelFunc]() {
		ANKER_LOG_INFO << "call after...";
		if (!this) {
			ANKER_LOG_INFO << "this is nullptr";
			return;
		}
		ChangeInternal(status, okFunc, cancelFunc);
	});
}

void AnkerNetDownloadDialog::ChangeInternal(Status status,
	AnkerNetBtnPanel::OkFunc_T okFunc,
	AnkerNetBtnPanel::CancelFunc_T cancelFunc)
{
	ANKER_LOG_INFO << "change status to " << (int)status;
	if (this->m_panel) {
		this->GetSizer()->Detach(this->m_panel);
		this->m_panel->Destroy();
	}

	wxPoint newPos = this->defaultPos;
	switch (status) {
	case Status::InstallHint:
		this->m_size.SetHeight(AnkerLength(defualtHeight));
		this->SetSize(this->m_size);
		this->m_panel = new AnkerNetInstallPanel(this, this->m_title,
			_L("common_print_netplugin_install_networkplugin")
			/*"AnkerMake Studio network plug-in is not detected, please install it before using the network feature with your AnkerMake devices."*/,
			AnkerSize(400, 200));
		break;
	case Status::UpdateHint:
		this->m_size.SetHeight(AnkerLength(defualtHeight));
		this->SetSize(this->m_size);
		this->m_panel = new AnkerNetInstallPanel(this, this->m_title,
			_L("common_print_netplugin_install_new_networkplugin")
			/*"Detected a new AnkerMake Studio network plug-in, please install it before using the network feature with your AnkerMake devices."*/,
			AnkerSize(400, 200));
		break;
	case Status::DownLoading:
		this->m_size.SetHeight(AnkerLength(defualtHeight));
		this->SetSize(this->m_size);
		this->m_panel = new AnkerNetProgressPanel(this, this->m_title, this->m_size);
		break;
	case Status::DownLoadSucc:
		this->m_size.SetHeight(AnkerLength(resultHeight));
		this->SetSize(this->m_size);
		newPos.y = newPos.y - (resultHeight - defualtHeight) / 2;
		this->m_panel = new AnkerNetSuccessPanel(this, this->m_title, this->m_size);
		break;
	case Status::DownLoadFailed:
		this->m_size.SetHeight(AnkerLength(resultHeight));
		this->SetSize(this->m_size);
		newPos.y = newPos.y - (resultHeight - defualtHeight) / 2;
		this->m_panel = new AnkerNetFailedPanel(this, this->m_title, this->m_size);
		break;
	}
	this->SetPosition(this->defaultPos);
	this->m_panel->BindBtnEvent(okFunc, cancelFunc);

	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(this->m_panel, 1, wxEXPAND);
	this->SetSizer(sizer);
	this->Layout();
}

int AnkerNetDownloadDialog::ShowHint(Status status, 
	AnkerNetBtnPanel::OkFunc_T okFunc,
	AnkerNetBtnPanel::CancelFunc_T cancelFunc)
{
	ChangeInternal(status, okFunc, cancelFunc);
	return wxDialog::ShowModal();
}

void AnkerNetDownloadDialog::UpdateProgress(int progress)
{
	// post event to update progress
	this->CallAfter([this, progress]() {
		if (!this) {
			return;
		}
		AnkerNetProgressPanel* panel = dynamic_cast<AnkerNetProgressPanel*>(this->m_panel);
		if (panel) {
			ANKER_LOG_DEBUG << "progress: " << progress;
			panel->UpdateProgress(progress);
		}
	});
}

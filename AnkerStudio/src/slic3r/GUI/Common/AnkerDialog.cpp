#include "AnkerDialog.hpp"
#include "libslic3r/Utils.hpp"
#include <wx/graphics.h>
#include <wx/wx.h>
#include <slic3r/Utils/WxFontUtils.hpp>
#include "../GUI_App.hpp"

AnkerDialogPanel::AnkerDialogPanel(
	wxWindow* parent, const wxString& title, const wxSize& size) :
	AnkerBox(parent)
{
	wxColour bkcolour = wxColour("#333438");
	if (parent) {
		bkcolour = parent->GetBackgroundColour();
	}

	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_mainSizer);
	
	int interval = 8;
    m_mainSizer->AddSpacer(interval);
    
	wxSize closeSize(12, 12);
	wxPoint closePos(size.GetWidth() - closeSize.GetWidth() - interval * 2, interval);
	wxImage closeImage(AnkerBase::AnkerResourceIconPath + "choose_device_close.png", wxBITMAP_TYPE_PNG);
	closeImage.Rescale(closeSize.GetWidth(), closeSize.GetHeight());
	wxBitmap closeBitmap(closeImage);
	closeBtn = new AnkerButton(this, wxID_ANY, "", closePos, closeSize, wxBORDER_NONE);
	closeBtn->SetBitmap(closeBitmap);
	closeBtn->SetBackgroundColour(bkcolour);
	closeBtn->Bind(wxEVT_BUTTON, &AnkerDialogPanel::closeButtonClicked, this);

	m_title = new AnkerStaticText(this, wxID_ANY, title);
	wxFont titleFont = ANKER_BOLD_FONT_NO_1;
	m_title->SetFont(titleFont);
	m_title->SetForegroundColour("#FFFFFF");
	m_title->SetBackgroundColour(bkcolour);
	wxSize titleSize = getTextSize(m_title, title);
	wxPoint titlePos((size.GetWidth() - titleSize.GetWidth()) / 2.0, interval);
	m_title->SetPosition(titlePos);
	m_title->SetSize(titleSize);

	wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
	titleSizer->AddSpacer(titlePos.x);
	titleSizer->Add(m_title, 1);
	titleSizer->AddSpacer(closeBtn->GetPosition().x - titlePos.x - titleSize.GetWidth());	 
	titleSizer->Add(closeBtn, 1);
	titleSizer->AddSpacer(interval);
	m_mainSizer->AddSpacer(interval);
	m_mainSizer->Add(titleSizer);

	wxPoint linePos(0, titlePos.y + titleSize.GetHeight() + interval);
	int soild = 2;
	wxSize lineSize(size.GetWidth(), soild);
	m_line = new AnkerLine2(this, wxID_ANY, linePos, lineSize);
	m_line->borderHighEnable(false);
	m_mainSizer->AddSpacer(interval);
	m_mainSizer->Add(m_line, 0);
}

void AnkerDialogPanel::closeButtonClicked(wxCommandEvent& event)
{
	closeWindow();
}

void AnkerDialogPanel::closeWindow()
{
	wxWindow* parent = GetParent();
	if (parent) {
		parent->Close();
	}
	else {
		Close();
	}
}

void AnkerDialogPanel::disableCloseButton(bool disable)
{
	closeBtn->Enable(!disable);
}

void AnkerDialogPanel::hideCloseButton(bool hide)
{
	closeBtn->Show(!hide);
}

AnkerDialog::AnkerDialog(
	wxWindow* parent, wxWindowID id, const wxString& title,
	const wxString& context, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name)  :
	wxDialog(parent, id, title, pos, size, style, name), 
	m_title(title), m_size(size), m_context(context)
{
	setBackgroundColour();
	Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &event) {
		EndModal(wxID_CLOSE);
		});
}

AnkerDialog::~AnkerDialog()
{
	ANKER_LOG_INFO << "delete anker dialog.";
}

void AnkerDialog::InitDialogPanel(int dialogType)
{
	switch (dialogType)
	{ 
	case  AnkerDialogType_Dialog:
		m_panel = new AnkerDialogPanel(this, m_title, m_size);
		break;
	case AnkerDialogType_OkDialog:
		m_panel = new AnkerDialogOkPanel(this, m_title, m_size);
		break;
	case AnkerDialogType_CancelOkDialog:
		m_panel = new AnkerDialogCancelOkPanel(this, m_title, m_size);
		break;
	case AnkerDialogType_DisplayTextOkDialog:
		m_panel = new AnkerDialogDisplayTextOkPanel(this, m_title, m_size, m_context);
		break;
	case AnkerDialogType_DisplayTextCancelOkDialog:
		m_panel = new AnkerDialogDisplayTextCancelOkPanel(this, m_title, m_size, m_context);
		break;
	case AnkerDialogType_DisplayTextNoYesDialog: 
	{
		AnkerDialogDisplayTextCancelOkPanel* displayPanel = new AnkerDialogDisplayTextCancelOkPanel(this, m_title, m_size, m_context);
		displayPanel->setOkBtnText(_AnkerL("common_button_yes"));
		displayPanel->setCancelBtnText(_AnkerL("common_button_no"));
		m_panel = displayPanel;
		break;
	}		
	case AnkerDialogType_CustomContent:
	{
		m_panel = new AnkerDialogCustomSizer(this, nullptr, m_title, m_size);
		if (m_customContent) {
			m_customContent->Reparent(m_panel);
			m_panel->m_mainSizer->Add(m_customContent,0,wxEXPAND);
		}
		break;
	}		
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

void AnkerDialog::InitDialogPanel2(int dialogType, const wxString& otherText, EventCallBack_T eventCallBack)
{
	switch (dialogType)
	{
	case AnkerDialogType_DisplayTextCheckBoxNoYesDialog:
	{
		AnkerDialogDisplayTextCheckBoxCancelOkPanel* displayPanel =
			new AnkerDialogDisplayTextCheckBoxCancelOkPanel(m_title, m_size, m_context, otherText, eventCallBack, this);
		displayPanel->setOkBtnText(_AnkerL("common_button_continue"));
		displayPanel->setCancelBtnText(_AnkerL("common_button_cancel"));
		m_panel = displayPanel;
		break;
	}
	case AnkerDialogType_DisplayTextCheckBoxOkDialog:
	{
		auto displayPanel =
			new AnkerDialogDisplayTextCheckBoxOkPanel(m_title, m_size, m_context, otherText, eventCallBack, this);
		m_panel = displayPanel;		
		break;
	}
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

void AnkerDialog::setBackgroundColour(const wxColour& color)
{
	SetBackgroundColour(color);
}

int AnkerDialog::ShowAnkerModal(int dialogType)
{
	InitDialogPanel(dialogType);
	return wxDialog::ShowModal();
}

int AnkerDialog::ShowAnkerModalOkCancel(const wxString& okText, const wxString& cancelText)
{
	InitDialogPanel(AnkerDialogType_DisplayTextCancelOkDialog);
	auto displayPanel = dynamic_cast<AnkerDialogCancelOkPanel*>(m_panel);
	if (!okText.empty()) {
		displayPanel->setOkBtnText(okText);
	}
	if (!cancelText.empty()) {
		displayPanel->setCancelBtnText(cancelText);
	}
	return wxDialog::ShowModal();
}

bool AnkerDialog::ShowAnker(int dialogType)
{
	InitDialogPanel(dialogType);
	return wxDialog::Show();
}

int AnkerDialog::ShowAnkerModal2(int dialogType, const wxString& otherText, EventCallBack_T eventCallBack)
{
	InitDialogPanel2(dialogType, otherText, eventCallBack);
	return wxDialog::ShowModal();
}

bool AnkerDialog::ShowAnker2(int dialogType, EventCallBack_T eventCallBack)
{
	InitDialogPanel2(dialogType, "", eventCallBack);
	return wxDialog::Show();
}

void AnkerDialog::SetCustomContent(wxWindow* customContent)
{
	m_customContent = customContent;
}

AnkerDialogCancelOkPanel::AnkerDialogCancelOkPanel(wxWindow* parent, const wxString& title,
	const wxSize& size)	:
	AnkerDialogPanel(parent, title, size)
{
	int widthInterval = 24;
	int heightInterval = 16;
	int btnInterval = 12;
	int btnWidth = (size.GetWidth() - widthInterval * 2 - btnInterval) / 2;
	int btnHeight = 32;
	int btnPosY = size.GetHeight() - heightInterval - btnHeight;

	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->AddSpacer(widthInterval);
	wxPoint cancelPos(widthInterval, btnPosY);
	wxSize cancelSize(btnWidth, btnHeight);
	m_cancelBtn = new AnkerBtn(this, wxID_ANY,  cancelPos, cancelSize, wxBORDER_NONE);
	m_cancelBtn->SetText(_AnkerL("common_button_cancel"));	  
	m_cancelBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_cancelBtn->SetTextColor(wxColour("#FFFFFF"));
	m_cancelBtn->SetBackgroundColour(wxColour(89, 90, 94));
	m_cancelBtn->SetRadius(5);

	btnSizer->Add(m_cancelBtn, 0);
	btnSizer->AddSpacer(btnInterval);
	wxPoint okPos(widthInterval + btnWidth + btnInterval, btnPosY);
	wxSize okSize(btnWidth, btnHeight);
	m_okBtn = new AnkerBtn(this, wxID_ANY,  okPos, okSize, wxBORDER_NONE);
	m_okBtn->SetText(_AnkerL("common_button_ok"));
	m_okBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_okBtn->SetBackgroundColour("#62D361");
	m_okBtn->SetTextColor("#FFFFFF");
	m_okBtn->SetRadius(5);
	
	btnSizer->Add(m_okBtn, 0);
	btnSizer->AddSpacer(widthInterval);

	int centerPanelPosY = m_line->GetPosition().y + m_line->GetSize().GetHeight();
	wxPoint centerPanelPos(0, centerPanelPosY);
	wxSize centerPanelSize(size.GetWidth(), btnPosY - centerPanelPos.y - btnInterval);

	m_centerPanel = new AnkerBox(this, wxID_ANY, centerPanelPos, centerPanelSize);
	wxBoxSizer* centerPanelSizer = new wxBoxSizer(wxVERTICAL);
	m_centerPanel->SetSizer(centerPanelSizer);
	m_mainSizer->Add(m_centerPanel, 0);
	m_mainSizer->Add(btnSizer, 0);
	m_mainSizer->AddSpacer(heightInterval);
}

AnkerDialogCancelOkPanel::~AnkerDialogCancelOkPanel()
{
}

void AnkerDialogCancelOkPanel::bindBtnEvent()
{
	m_cancelBtn->Bind(wxEVT_BUTTON, &AnkerDialogCancelOkPanel::cancelButtonClicked, this);
	m_okBtn->Bind(wxEVT_BUTTON, &AnkerDialogCancelOkPanel::okButtonClicked, this);
}

void AnkerDialogCancelOkPanel::setOkBtnText(const wxString& text)
{
	m_okBtn->SetText(text);
}

void AnkerDialogCancelOkPanel::setCancelBtnText(const wxString& text)
{
	m_cancelBtn->SetText(text);
}

void AnkerDialogCancelOkPanel::cancelButtonClicked(wxCommandEvent& event)
{
	wxWindow* parent = GetParent();
	AnkerDialog* parentDialog = dynamic_cast<AnkerDialog*>(parent);
	if (parentDialog) {		
		parentDialog->EndModal(wxID_CANCEL);
	}
	else {
		ANKER_LOG_INFO << "close parent window";
		closeWindow();
	}
}

void AnkerDialogCancelOkPanel::okButtonClicked(wxCommandEvent& event)
{
	wxWindow* parent = GetParent();
	AnkerDialog* parentDialog = dynamic_cast<AnkerDialog*>(parent);
	if (parentDialog) {
        ANKER_LOG_INFO<<"END EndModal parentDialog";
		parentDialog->EndModal(wxID_OK);
	}
	else {
        ANKER_LOG_INFO<<"closeWindow of parentDialog";
		closeWindow();
	}
}

AnkerDialogOkPanel::AnkerDialogOkPanel(
	wxWindow* parent, 
	const wxString& title, 
	const wxSize& size,
	AnkerDialogBtnCallBack_T okBtnCallBack) :
	m_okBtnCallBack(okBtnCallBack),
	AnkerDialogPanel(parent, title, size)
{
	int widthInterval = 24;
	int heightInterval = 16;
	int btnWidth = size.GetWidth() - widthInterval * 2;
	int btnHeight = 32;
	int btnPosY = size.GetHeight() - heightInterval - btnHeight;

	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->AddSpacer(widthInterval);

	wxPoint okPos(widthInterval, btnPosY);
	wxSize okSize(btnWidth, btnHeight);	 
	m_okBtn = new AnkerBtn(this, wxID_ANY, okPos, okSize, wxBORDER_NONE);
	m_okBtn->SetText(_AnkerL("common_button_ok"));
	m_okBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_okBtn->SetBackgroundColour("#62D361");
	m_okBtn->SetTextColor("#FFFFFF");
	m_okBtn->SetRadius(5);
	m_okBtn->Bind(wxEVT_BUTTON, &AnkerDialogOkPanel::okButtonClicked, this);
	btnSizer->Add(m_okBtn, 0);
	btnSizer->AddSpacer(widthInterval);

	int centerPanelPosY = m_line->GetPosition().y + m_line->GetSize().GetHeight();
	wxPoint centerPanelPos(0, centerPanelPosY);
	wxSize centerPanelSize(size.GetWidth(), btnPosY - centerPanelPos.y);

	m_centerPanel = new AnkerBox(this, wxID_ANY, centerPanelPos, centerPanelSize);
	wxBoxSizer* centerPanelSizer = new wxBoxSizer(wxVERTICAL);
	m_centerPanel->SetSizer(centerPanelSizer);

	m_mainSizer->Add(m_centerPanel, 0);
	m_mainSizer->Add(btnSizer, 0);
	m_mainSizer->AddSpacer(heightInterval);
}

AnkerDialogOkPanel::~AnkerDialogOkPanel()
{
}

void AnkerDialogOkPanel::setOkBtnText(const wxString& text)
{
	m_okBtn->SetLabelText(text);
}

void AnkerDialogOkPanel::okButtonClicked(wxCommandEvent& event)
{
	wxWindow* parent = GetParent();
	AnkerDialog* parentDialog = dynamic_cast<AnkerDialog*>(parent);
	if (parentDialog) {
		if (parentDialog->IsModal()) {
			ANKER_LOG_INFO << "CALL parentDialog->EndModal(wxID_OK);";
			parentDialog->EndModal(wxID_OK);
		}
		else {
			parentDialog->Close();
		}
	}
	else {
        ANKER_LOG_INFO<< "CALL closeWindow() of AnkerDialogOkPanel";
		closeWindow();
	}

	if (m_okBtnCallBack) {
		m_okBtnCallBack(event);
	}
}

AnkerDialogCustomSizer::AnkerDialogCustomSizer(wxWindow* parent, wxBoxSizer* contentSizer, const wxString& title,
	const wxSize& size) :
	//m_contentSizer(contentSizer),
	AnkerDialogPanel(parent, title, size)
{
	if (contentSizer)
		m_mainSizer->Add(contentSizer, 0,wxEXPAND);
}

AnkerDialogCustomSizer::~AnkerDialogCustomSizer()
{
}


AnkerDialogDisplayTextOkPanel::AnkerDialogDisplayTextOkPanel(
	wxWindow* parent, 
	const wxString& title, 
	const wxSize& size, 
	const wxString& context,
	AnkerDialogBtnCallBack_T okBtnCallBack) :
	AnkerDialogOkPanel(parent, title, size, okBtnCallBack)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}
	
	int interval = 24;
	m_centerPanel->GetSizer()->AddSpacer(interval);
	wxBoxSizer* contextSizer = new wxBoxSizer(wxHORIZONTAL);
	contextSizer->AddSpacer(interval);
	wxPoint contextPos(interval, interval);
	wxSize contextSize(size.GetWidth() - 2 * interval, m_okBtn->GetPosition().y - contextPos.y - interval);
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, context, contextPos, contextSize, wxST_ELLIPSIZE_MIDDLE);
	wxFont titleFont = ANKER_BOLD_FONT_NO_1;
	m_contextText->SetFont(titleFont);
	m_contextText->SetForegroundColour("#FFFFFF");
	m_contextText->SetBackgroundColour(bkColour);
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
	setStrWrap(m_contextText, contextSize.GetWidth(), context, type);
	contextSizer->Add(m_contextText, 0);
	contextSizer->AddSpacer(interval);
	m_centerPanel->GetSizer()->Add(contextSizer);
	m_centerPanel->GetSizer()->AddSpacer(interval);
}

AnkerDialogIconTextOkPanel::AnkerDialogIconTextOkPanel(
	wxWindow* parent,
	const wxString& iconPath,
	const wxString& title,
	const wxString& context,
	const wxSize& size,
	AnkerDialogBtnCallBack_T okBtnCallBack) :
	AnkerDialogOkPanel(parent, title, size, okBtnCallBack)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(24);

	// icon image
	wxStaticBitmap* iconImage = nullptr;
	if (!iconPath.empty()) {
		auto imageName = wxString::FromUTF8(Slic3r::var(iconPath.ToStdString(wxConvUTF8)));
		wxImage image = wxImage(imageName, wxBITMAP_TYPE_PNG);
		image.Rescale(86, 86);
		iconImage = new wxStaticBitmap(m_centerPanel, wxID_ANY, image);
		iconImage->SetMinSize(image.GetSize());
		iconImage->SetMaxSize(image.GetSize());
		iconImage->SetBackgroundColour(bkColour);
	}

	// text
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
	int contextWidth = size.GetWidth() - 2 * interval;
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, "");
	m_contextText->SetFont(ANKER_FONT_NO_2);
	m_contextText->SetForegroundColour("#FFFFFF");
	m_contextText->SetBackgroundColour(bkColour);

	wxString wrapLabel = Slic3r::GUI::WrapEveryCharacter(context, ANKER_FONT_NO_2, contextWidth);
	m_contextText->SetLabelText(wrapLabel);
	m_contextText->Wrap(contextWidth);

	wxBoxSizer* contextSizer = new wxBoxSizer(wxVERTICAL);
	if (iconImage) {
		auto imageInterval = size.GetWidth() - 2 * interval - iconImage->GetSize().GetWidth();
		contextSizer->Add(iconImage, 0, wxLEFT | wxRIGHT, imageInterval / 2);
		contextSizer->AddSpacer(AnkerLength(24));
	}
	contextSizer->Add(m_contextText, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	m_centerPanel->GetSizer()->Add(contextSizer, 0, wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, interval);

	//Layout();
}

AnkerDialogDisplayTextCheckBoxOkPanel::AnkerDialogDisplayTextCheckBoxOkPanel(
	const wxString& title, 
	const wxSize& size, 
	const wxString& context, 
	const wxString& checkBoxStr,
	AnkerDialog::EventCallBack_T callback, 
	wxWindow* parent,
	AnkerDialogBtnCallBack_T okBtnCallBack)
	: AnkerDialogOkPanel(parent, title, size, okBtnCallBack)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(12);

	// check box
	wxStaticText* checkBoxLabel = new wxStaticText(m_centerPanel, wxID_ANY, checkBoxStr);
	checkBoxLabel->SetFont(ANKER_FONT_NO_2);
	checkBoxLabel->SetForegroundColour(wxColour("#FFFFFF"));
	wxCheckBox* checkBox = new wxCheckBox(m_centerPanel, wxID_ANY, "");
	checkBox->SetSize(size.GetWidth(), AnkerLength(21));

	wxBoxSizer* checkBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	checkBoxSizer->AddSpacer(AnkerLength(24));
	checkBoxSizer->Add(checkBox, 0, wxALIGN_LEFT | wxALL);
	checkBoxSizer->AddSpacer(5);
	checkBoxSizer->Add(checkBoxLabel, 0, wxALL);
	checkBoxSizer->AddSpacer(AnkerLength(24));

	// display text	
	wxPoint contextPos(interval, interval);
	auto centerSize = m_centerPanel->GetSize();
	auto checkBoxSize = checkBox->GetSize();
	auto contextHeight = centerSize.GetHeight() - checkBoxSize.GetHeight() - 3 * interval;

	wxSize contextSize(size.GetWidth() - 2 * AnkerLength(24), contextHeight);
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, "", contextPos, contextSize);
	m_contextText->SetFont(ANKER_FONT_NO_2);
	m_contextText->SetForegroundColour("#FFFFFF");

	int wrapWidth = contextSize.GetWidth();
	wxString wrapText = Slic3r::GUI::WrapEveryCharacter(context, ANKER_FONT_NO_2, wrapWidth);
	m_contextText->Wrap(wrapWidth);
	m_contextText->SetLabelText(wrapText);

	wxBoxSizer* contextSizer = new wxBoxSizer(wxHORIZONTAL);
	contextSizer->Add(m_contextText, 0, wxLEFT | wxRIGHT, AnkerLength(24));

	// center box
	auto centerBox = new wxBoxSizer(wxVERTICAL);
	centerBox->AddStretchSpacer(1);
	centerBox->Add(contextSizer);
	centerBox->AddSpacer(interval);
	centerBox->Add(checkBoxSizer);
	centerBox->AddStretchSpacer(1);

	m_centerPanel->GetSizer()->AddSpacer(interval);
	m_centerPanel->GetSizer()->Add(centerBox);
	m_centerPanel->GetSizer()->AddSpacer(interval);

	setOkBtnText(_AnkerL("common_button_ok"));

	checkBox->Bind(wxEVT_CHECKBOX, [this, checkBox, callback](wxCommandEvent& event) {
		if (callback) {
			callback(event, dynamic_cast<wxControl*>(checkBox));
		}
		});

	Layout();
}


AnkerDialogDisplayTextCancelOkPanel::AnkerDialogDisplayTextCancelOkPanel(wxWindow* parent, const wxString& title, const wxSize& size,
	const wxString& context) : AnkerDialogCancelOkPanel(parent, title, size)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = 12;
	m_centerPanel->GetSizer()->AddSpacer(interval);
	wxBoxSizer* contextSizer = new wxBoxSizer(wxHORIZONTAL);
	contextSizer->AddSpacer(interval);
	wxPoint contextPos(interval, interval);
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();

	wxSize contextSize(size.GetWidth() - 2 * interval, m_okBtn->GetPosition().y - contextPos.y - interval);
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, context, contextPos, contextSize, wxST_ELLIPSIZE_MIDDLE);
	wxFont titleFont = ANKER_FONT_NO_2;
	m_contextText->SetFont(titleFont);
	m_contextText->SetForegroundColour("#FFFFFF");
	m_contextText->SetBackgroundColour(bkColour);
	setStrWrap(m_contextText, contextSize.GetWidth(), context, type);
	contextSizer->Add(m_contextText, 0);
	contextSizer->AddSpacer(interval);
	m_centerPanel->GetSizer()->Add(contextSizer);
	m_centerPanel->GetSizer()->AddSpacer(interval);
	bindBtnEvent();
	setOkBtnText();
	setCancelBtnText();
}


AnkerDialogDisplayTextCheckBoxCancelOkPanel::AnkerDialogDisplayTextCheckBoxCancelOkPanel(
	const wxString& title, const wxSize& size, const wxString& context, const wxString& checkBoxStr, 
	AnkerDialog::EventCallBack_T callback, wxWindow* parent)
	: AnkerDialogCancelOkPanel(parent, title, size)
{
	wxColour bkColour = wxColour("#333438");
	if (parent) {
		bkColour = parent->GetBackgroundColour();
	}

	int interval = AnkerLength(12);

	// check box
    wxStaticText* checkBoxLabel = new wxStaticText(m_centerPanel, wxID_ANY, checkBoxStr);
    checkBoxLabel->SetFont(ANKER_FONT_NO_2);
    checkBoxLabel->SetForegroundColour(wxColour("#FFFFFF"));
    wxCheckBox* checkBox = new wxCheckBox(m_centerPanel, wxID_ANY, "");
    checkBox->SetSize(size.GetWidth(), AnkerLength(21));

    wxBoxSizer* checkBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	checkBoxSizer->AddSpacer(AnkerLength(24));
    checkBoxSizer->Add(checkBox, 0, wxALIGN_LEFT | wxALL);
    checkBoxSizer->AddSpacer(5);
    checkBoxSizer->Add(checkBoxLabel, 0, wxALL);
	checkBoxSizer->AddSpacer(AnkerLength(24));

	// display text	
	wxPoint contextPos(interval, interval);
	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();

	auto centerSize = m_centerPanel->GetSize();
	auto checkBoxSize = checkBox->GetSize();
	auto contextHeight = centerSize.GetHeight() - checkBoxSize.GetHeight() - 3 * interval;

	wxSize contextSize(size.GetWidth() - 2 * interval, contextHeight);
	m_contextText = new AnkerStaticText(m_centerPanel, wxID_ANY, context, contextPos, contextSize);
	m_contextText->SetFont(ANKER_FONT_NO_2);
	m_contextText->SetForegroundColour("#FFFFFF");
	//m_contextText->SetBackgroundColour(wxColour(0, 255, 0));
#ifdef __APPLE__
	int width = AnkerLength(20);
#else
	int width = AnkerLength(30);
#endif
	Slic3r::GUI::WxFontUtils::setText_wrap(m_contextText, contextSize.GetWidth() - width, context, type);
	wxBoxSizer* contextSizer = new wxBoxSizer(wxHORIZONTAL);
	contextSizer->Add(m_contextText, 0, wxLEFT | wxRIGHT, width / 2);

	// center box
	auto centerBox = new wxBoxSizer(wxVERTICAL);
	centerBox->AddStretchSpacer(1);
	centerBox->Add(contextSizer);
	centerBox->AddSpacer(interval);
	centerBox->Add(checkBoxSizer);
	centerBox->AddStretchSpacer(1);

	m_centerPanel->GetSizer()->AddSpacer(interval);
	//m_centerPanel->SetBackgroundColour(wxColour(255, 0, 0));
	m_centerPanel->GetSizer()->Add(centerBox);
	m_centerPanel->GetSizer()->AddSpacer(interval);
    Layout();

	bindBtnEvent();
	setOkBtnText();
	setCancelBtnText();

	checkBox->Bind(wxEVT_CHECKBOX, [this, checkBox, callback](wxCommandEvent& event) {
		if (callback) {
			callback(event, dynamic_cast<wxControl*>(checkBox));
		}
	});
}

AnkerDialogBase::AnkerDialogBase(wxWindow* parent, 
	wxWindowID id, 
	const wxString& title, 
	const wxPoint& pos, 
	const wxSize& size, 
	long style, 
	const wxString& name)
	: wxDialog(parent, id, title, pos, size, style, name) ,
	m_title(title)
{
	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->SetMinSize(size);
	
	m_titlePanel = new wxPanel(this);
	m_titlePanel->SetSize(size.x, 25);
	m_mainSizer->Add(m_titlePanel, 0, wxEXPAND | wxALL, 0);
	m_titleSizer = new wxBoxSizer(wxHORIZONTAL);
	m_titlePanel->SetSizer(m_titleSizer);

	m_titleSizer->AddStretchSpacer(167);

	m_titleText = new wxStaticText(m_titlePanel, wxID_ANY, m_title, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	m_titleText->SetMinSize(wxSize(size.x / 2, 25));
	m_titleText->SetBackgroundColour(wxColour(41, 42, 45));
	m_titleText->SetForegroundColour(wxColour("#FFFFFF"));
	m_titleText->SetFont(ANKER_BOLD_FONT_SIZE(12));
	m_titleSizer->Add(m_titleText, 0, wxEXPAND | wxALIGN_CENTER | wxTOP | wxBOTTOM, 17);

	m_titleSizer->AddStretchSpacer(123);

	wxImage exitImage = wxImage(wxString::FromUTF8(Slic3r::var("fdm_nav_del_icon.png")), wxBITMAP_TYPE_PNG);
	exitImage.Rescale(20, 20);
	m_pExitBtn = new wxButton(m_titlePanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	m_pExitBtn->SetBitmap(exitImage);
	m_pExitBtn->SetMinSize(exitImage.GetSize());
	m_pExitBtn->SetMaxSize(exitImage.GetSize());
	m_pExitBtn->SetBackgroundColour(wxColour(41, 42, 45));
	m_pExitBtn->SetForegroundColour(wxColour("#FFFFFF"));
	m_pExitBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
	m_pExitBtn->Bind(wxEVT_BUTTON, &AnkerDialogBase::OnExitButtonClicked, this);
	m_titleSizer->Add(m_pExitBtn, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM | wxRIGHT, 17);

	// split line
	wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(64, 65, 70));
	splitLineCtrl->SetMaxSize(AnkerSize(100000, 1));
	splitLineCtrl->SetMinSize(AnkerSize(1, 1));
	m_mainSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);

	m_contentSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->Add(m_contentSizer);

	m_titlePanel->Bind(wxEVT_LEFT_DOWN, &AnkerDialogBase::OnDragMouseDown, this);
	m_titlePanel->Bind(wxEVT_MOTION, &AnkerDialogBase::OnDragMouseMove, this);
	m_titlePanel->Bind(wxEVT_LEFT_UP, &AnkerDialogBase::OnDragMouseUp, this);
	m_titlePanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &AnkerDialogBase::OnDragMouseLost, this);
	m_titlePanel->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
	    SetCursor(wxCursor(wxCURSOR_HAND));
	    this->SetFocus();
	    });
	m_titlePanel->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
	    if (!m_titlePanelRect.Contains(event.GetPosition()) )
	        SetCursor(wxCursor(wxCURSOR_NONE));
	    });
	m_titleText->Bind(wxEVT_LEFT_DOWN, &AnkerDialogBase::OnDragMouseDown, this);
	m_titleText->Bind(wxEVT_MOTION, &AnkerDialogBase::OnDragMouseMove, this);
	m_titleText->Bind(wxEVT_LEFT_UP, &AnkerDialogBase::OnDragMouseUp, this);
	m_titleText->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
	     SetCursor(wxCursor(wxCURSOR_HAND));
	     this->SetFocus();
	});

	wxDialog::SetSizer(m_mainSizer);
#ifdef _WIN32
	Connect(wxEVT_SIZE, wxSizeEventHandler(AnkerDialogBase::OnSize));
#endif
	Bind(wxEVT_PAINT, &AnkerDialogBase::OnPaint, this);
	Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event) {
		Layout();
		Refresh();
		});
}

AnkerDialogBase::~AnkerDialogBase()
{
}

void AnkerDialogBase::SetSizer(wxSizer* sizer)
{ 
	sizer->SetMinSize(m_contentSizer->GetMinSize());
	m_contentSizer->Add(sizer);
	Layout();
}

void AnkerDialogBase::SetSize(wxSize size)
{
	wxDialog::SetSize(size);
	m_mainSizer->SetMinSize(size);
	m_titlePanel->SetSize(size.x, 25);
	m_titleSizer->SetMinSize(size.x, 25);
	m_titleText->SetMinSize(wxSize(size.x * 0.75, 25));
	m_contentSizer->SetMinSize(size.x, size.y - 25);

	m_titlePanelRect = m_titlePanel->GetRect();
	
	Layout();
}

void AnkerDialogBase::SetTitle(wxString title)
{
	m_title = title;
	m_titleText->SetLabelText(title);
	//todo: fit to text length
	//m_titleText->SetMinSize(wxSize(text.length(), 25));
}

void AnkerDialogBase::SetBackgroundColour(wxColour colour)
{
#ifdef _WIN32
	wxDialog::SetBackgroundColour(colour);
	m_titlePanel->SetBackgroundColour(colour);
#else
	wxDialog::SetBackgroundColour(wxTransparentColour);
#endif
	m_titleText->SetBackgroundColour(colour);
	m_pExitBtn->SetBackgroundColour(colour);
}

void AnkerDialogBase::OnDragMouseDown(wxMouseEvent& event)
{
	auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
	pFocusPanel ? m_titlePanel->CaptureMouse() : m_titleText->CaptureMouse();

	m_startPos = event.GetPosition();

	bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();
	if (hasCapture) {
		event.Skip(false);
		return;
	}
}

void AnkerDialogBase::OnDragMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	if (event.LeftIsDown() && event.Dragging()) {
		wxPoint delta = pos - m_startPos;
		wxPoint newPos = GetPosition() + delta;
		Move(newPos);
	}
	event.Skip(false);
}

void AnkerDialogBase::OnDragMouseUp(wxMouseEvent& event)
{
	auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
	bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();

	if (!hasCapture) {
		event.Skip(false);
		return;
	}

	pFocusPanel ? m_titlePanel->ReleaseMouse() : m_titleText->ReleaseMouse();
}

void AnkerDialogBase::OnExitButtonClicked(wxCommandEvent& event)
{
	Hide();
	EndModal(wxID_ANY);
}

void AnkerDialogBase::OnDragMouseLost(wxMouseCaptureLostEvent& event)
{
	auto pFocusPanel = dynamic_cast<wxPanel*>(event.GetEventObject());
	bool hasCapture = pFocusPanel ? m_titlePanel->HasCapture() : m_titleText->HasCapture();
	if (!hasCapture) {
		event.Skip(false);
		return;
	}
	pFocusPanel ? m_titlePanel->ReleaseMouse() : m_titleText->ReleaseMouse();
}

void AnkerDialogBase::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxSize size = GetSize();
	dc.SetPen(wxColour(41, 42, 45));
	dc.SetBrush(wxColour(41, 42, 45));

#ifdef _WIN32
	dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
#else 
	dc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), 8);
#endif 
}

void AnkerDialogBase::OnSize(wxSizeEvent& event)
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


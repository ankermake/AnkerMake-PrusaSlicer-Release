#include "AnkerShowWidget.hpp"
#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "libslic3r/Utils.hpp"
#include "GUI_App.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "I18N.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_LOGIN_CLCIKED, wxCommandEvent);

AnkerEmptyDevice::AnkerEmptyDevice(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerEmptyDevice::~AnkerEmptyDevice()
{

}

void AnkerEmptyDevice::initUi()
{
	SetBackgroundColour(wxColour("#18191B"));
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pBodyHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pLeftVSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pRightVSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* pTitle = new wxStaticText(this, wxID_ANY, _L("common_print_statusnotice_nodevicetitle"));
	wxFont font = pTitle->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	pTitle->SetFont(font);
	pTitle->SetForegroundColour(wxColour(255, 255, 255));

	pMainVSizer->AddSpacer(215);
	pMainVSizer->Add(pTitle, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("download_app.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(280, 228);
		wxBitmap scaledBitmap(image);

		wxStaticBitmap* pLeftImg = new wxStaticBitmap(this, wxID_ANY, scaledBitmap);
		pLeftImg->SetMinSize(scaledBitmap.GetSize());

		wxStaticText* pLeftText = new wxStaticText(this, wxID_ANY, _L("common_print_statusnotice_nodevicecontent1"));
		pLeftText->SetWindowStyleFlag(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
		pLeftText->SetForegroundColour(wxColour(153, 153, 153));
		pLeftText->Wrap(280);
		wxClientDC dc(this);
		dc.SetFont(pLeftText->GetFont());
		wxSize size = dc.GetTextExtent(_L("common_print_statusnotice_nodevicecontent1"));
		int textHeight = (size.GetWidth()/280 + 2) * size.GetHeight();

		pLeftText->SetMinSize(wxSize(280, textHeight));
		

		pLeftVSizer->Add(pLeftImg, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pLeftVSizer->AddSpacer(22);
		pLeftVSizer->Add(pLeftText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	}

	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("add_device.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(280, 228);
		wxBitmap scaledBitmap(image);

		wxStaticBitmap* pRightImg = new wxStaticBitmap(this, wxID_ANY, scaledBitmap);
		pRightImg->SetMinSize(scaledBitmap.GetSize());

		wxStaticText* pRightText = new wxStaticText(this, wxID_ANY, _L("common_print_statusnotice_nodevicecontent2"));
		pRightText->SetForegroundColour(wxColour(153, 153, 153));
		pRightText->Wrap(280);

		wxClientDC dc(this);
		dc.SetFont(pRightText->GetFont());
		wxSize size = dc.GetTextExtent(_L("common_print_statusnotice_nodevicecontent2"));
		int textHeight = (size.GetWidth() / 280 + 2) * size.GetHeight();

		pRightText->SetMinSize(wxSize(280, textHeight));

		pRightText->SetWindowStyleFlag(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
		
		pRightVSizer->Add(pRightImg, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pRightVSizer->AddSpacer(22);
		pRightVSizer->Add(pRightText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	}
	pBodyHSizer->AddStretchSpacer(1);
	pBodyHSizer->Add(pLeftVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBodyHSizer->AddSpacer(20);
	pBodyHSizer->Add(pRightVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBodyHSizer->AddStretchSpacer(1);

	pMainVSizer->AddSpacer(43);
	pMainVSizer->Add(pBodyHSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainVSizer->AddSpacer(197);
	SetSizer(pMainVSizer);

}

AnkerOfflineDevice::AnkerOfflineDevice(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerOfflineDevice::~AnkerOfflineDevice()
{

}

void AnkerOfflineDevice::initUi()
{
	SetBackgroundColour(wxColour("#1F2022"));
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("device_wifi_icon_off.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(39, 30);
	wxBitmap scaledBitmap(image);

	wxStaticBitmap* pLogoImg = new wxStaticBitmap(this, wxID_ANY, scaledBitmap);

	wxStaticText* pTipsText = new wxStaticText(this, wxID_ANY, _L("Device is disconnected, "));
	pTipsText->SetForegroundColour(wxColour(153, 153, 153));

	AnkerHyperlink* link = new AnkerHyperlink(this, wxID_ANY, _L("please click to see more help >>"), "https://support.ankermake.com/s/article/How-to-Fix-WiFi-Connection-Issue");
	link->SetMinSize(AnkerSize(210, 25));
	link->SetSize(AnkerSize(230, 25));

	wxBoxSizer* pLinkSizer = new wxBoxSizer(wxVERTICAL);
	pLinkSizer->Add(pTipsText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pLinkSizer->Add(link, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, 0);

	pMainVSizer->AddSpacer(318);
	pMainVSizer->Add(pLogoImg, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainVSizer->AddSpacer(22);
	pMainVSizer->Add(pLinkSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainVSizer->AddSpacer(370);

	SetSizer(pMainVSizer);
}


AnkerUnLoginPanel::AnkerUnLoginPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
	, m_logoBitmap(nullptr)
	, m_loginText(nullptr)
	, m_loginBtn(nullptr)
	, m_tipsText(nullptr)
{
	//this->SetBackgroundColour(wxColour("#1F2022"));
	this->SetBackgroundColour(wxColour("#1B1C1F"));

	initUi();
}

AnkerUnLoginPanel::~AnkerUnLoginPanel()
{

}

void AnkerUnLoginPanel::initUi()
{
	wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("logo_icon.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(82, 78);
	wxBitmap scaledBitmap(image);
	m_logoBitmap = new wxStaticBitmap(this, wxID_ANY, scaledBitmap);
	m_logoBitmap->SetSize(scaledBitmap.GetSize());

	m_loginText = new wxStaticText(this, wxID_ANY, _L("common_print_statusnotice_dislogin"));
	m_loginText->SetFont(ANKER_FONT_NO_1);
	wxFont font = m_loginText->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	m_loginText->SetFont(font);
	m_loginText->SetForegroundColour(wxColour(255, 255, 255));

	m_loginBtn = new AnkerBtn(this,
		wxID_ANY);
	m_loginBtn->SetText(_L("common_print_statusnotice_login"));
	m_loginBtn->SetFont(ANKER_FONT_NO_1);	

	m_loginBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_LOGIN_CLCIKED);
		wxPostEvent(this->GetParent(), evt);
		});

	m_loginBtn->SetBackgroundColour(wxColor("#62D361"));
	m_loginBtn->SetMinSize(AnkerSize(336, 48));
	m_loginBtn->SetRadius(5);
	m_loginBtn->SetTextColor(wxColor("#FFFFFF"));

	m_tipsText = new wxStaticText(this, wxID_ANY, _L("common_print_statusnotice_createaccount") + " ");
	m_tipsText->SetFont(ANKER_FONT_NO_1);
	m_tipsText->SetForegroundColour(wxColour(153, 153, 153));

	AnkerHyperlink* link = new AnkerHyperlink(this, wxID_ANY, _L("common_print_statusnotice_createaccount2"), "https://mulpass.ankermake.com/?app=ankermake&tab=register");
	
	wxClientDC dc(this);
	dc.SetFont(ANKER_FONT_NO_1);
	wxSize linkSize = dc.GetTextExtent(_L("common_print_statusnotice_createaccount2"));
	
	link->SetMinSize(linkSize);
	wxBoxSizer* pLinkSizer = new wxBoxSizer(wxHORIZONTAL);
	pLinkSizer->Add(m_tipsText);
	pLinkSizer->Add(link);

	pMainSizer->AddSpacer(176);
	pMainSizer->Add(m_logoBitmap, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainSizer->AddSpacer(38);
	pMainSizer->Add(m_loginText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainSizer->AddSpacer(93);
	pMainSizer->Add(m_loginBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pMainSizer->AddSpacer(25);
	pMainSizer->Add(pLinkSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	SetSizer(pMainSizer);
}
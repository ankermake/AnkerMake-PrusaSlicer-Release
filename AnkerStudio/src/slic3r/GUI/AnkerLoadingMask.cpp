#include "AnkerLoadingMask.hpp"

#include "libslic3r/Utils.hpp"


AnkerLoadingMask::AnkerLoadingMask(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
	, m_pMumLabel(nullptr)
	, m_loadingCounter(0)
{
	initUI();

	Bind(wxEVT_SHOW, &AnkerLoadingMask::OnShow, this);

	m_deltaCount = 8;
	m_pTimer = new wxTimer;
	m_pTimer->Bind(wxEVT_TIMER, &AnkerLoadingMask::OnTimeEvent, this);
	m_pTimer->Start(1000 / m_deltaCount, false);
}

AnkerLoadingMask::~AnkerLoadingMask()
{

}

void AnkerLoadingMask::initUI()
{
	SetTransparent(200);

	SetBackgroundColour(wxColour(58, 59, 63, 50));

	wxBoxSizer* mumSizer = new wxBoxSizer(wxVERTICAL);

	mumSizer->AddStretchSpacer(1);

	m_mumImage = wxImage(wxString::FromUTF8(Slic3r::var("loading.png")), wxBITMAP_TYPE_PNG);
	m_mumImage.Rescale(40, 40);
	m_pMumLabel = new wxStaticBitmap(this, wxID_ANY, m_mumImage);
	m_pMumLabel->SetMinSize(m_mumImage.GetSize());
	m_pMumLabel->SetMaxSize(m_mumImage.GetSize());
	m_pMumLabel->SetBackgroundColour(wxColour(58, 59, 63, 50));
	m_pMumLabel->SetTransparent(100);
	mumSizer->Add(m_pMumLabel, 0, wxALIGN_CENTER, 0);

	mumSizer->AddSpacer(3);

	wxStaticText* loadingText = new wxStaticText(this, wxID_ANY, L"Loading...");
	loadingText->SetMinSize(wxSize(70, 25));
	loadingText->SetBackgroundColour(wxColour(58, 59, 63, 50));
	loadingText->SetForegroundColour(wxColour(255, 255, 255));
	loadingText->SetTransparent(100);
	wxFont font = loadingText->GetFont();
	font.SetPointSize(10); 
	loadingText->SetFont(font);
	mumSizer->Add(loadingText, 0, wxALIGN_CENTER, 0);

	mumSizer->AddStretchSpacer(1);

	SetSizer(mumSizer);
}

void AnkerLoadingMask::OnShow(wxShowEvent& event)
{
	bool animated = event.IsShown();
	if (animated)
		m_pTimer->Start(1000 / m_deltaCount, false);
	else
		m_pTimer->Stop();
}

void AnkerLoadingMask::OnTimeEvent(wxTimerEvent& event)
{
	m_loadingCounter++;
	if (m_loadingCounter == m_deltaCount)
		m_loadingCounter = 0;

	wxImage image = m_mumImage.Rotate(360 / m_deltaCount * m_loadingCounter, wxPoint(0, 0));
	m_pMumLabel->SetBitmap(image);

	//wxWindow* parent = GetParent();
	//SetSize(parent->GetClientSize());
	//SetPosition(parent->GetScreenPosition() + parent->GetClientAreaOrigin());

	if (IsShown())
		Refresh();
}

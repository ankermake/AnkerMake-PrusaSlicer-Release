#include "AnkerLoadingMask.hpp"
#include "AnkerGUIConfig.hpp"

#include "slic3r/GUI/GUI_App.hpp"
#include "libslic3r/Utils.hpp"


#define PANEL_BACK_COLOR 41, 42, 45
#define TEXT_LIGHT_COLOR 255, 255, 255
#define TEXT_DARK_COLOR 183, 183, 183
#define CONTROL_DISABLE_COLOR 125, 125, 125
#define SYSTEM_COLOR "#62D361"

wxDEFINE_EVENT(wxANKEREVT_LOADING_TIMEOUT, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_LOADMASK_RECTUPDATE, wxCommandEvent);

AnkerLoadingMask::AnkerLoadingMask(wxWindow* parent, int m_timeoutMS, bool rectUpdateFlag)
	: wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW)
	, m_rectUpdateFlag(rectUpdateFlag)
	, m_rectUpdateFreq(2)
	, m_pMumLabel(nullptr)
	, m_loadingCounter(0)
	, m_timeoutMS(m_timeoutMS)
	, m_loadingText(L"Loading ...")
	, m_pLoadingLabel(nullptr)
{
	initUI();

	Bind(wxEVT_SHOW, &AnkerLoadingMask::OnShow, this);

	m_deltaCount = MUM_IMAGE_COUNT;
	m_pAnimationTimer = new wxTimer();
	m_pAnimationTimer->Bind(wxEVT_TIMER, &AnkerLoadingMask::OnTimeEvent, this);
	m_pAnimationTimer->Start(1000 / m_deltaCount, false);

	m_pTimeOutTimer = new wxTimer();
	m_pTimeOutTimer->Bind(wxEVT_TIMER, &AnkerLoadingMask::OnTimeOutEvent, this);

	m_pRectUpdatetTimer = new wxTimer();
	m_pRectUpdatetTimer->Bind(wxEVT_TIMER, &AnkerLoadingMask::OnRectUpdateEvent, this);
}

void AnkerLoadingMask::setText(wxString text)
{
	m_loadingText = text;
	m_pLoadingLabel->SetLabelText("");
}

AnkerLoadingMask::~AnkerLoadingMask()
{
	if (m_pAnimationTimer)
	{
		delete m_pAnimationTimer;
		m_pAnimationTimer = nullptr;
	}

	if (m_pTimeOutTimer)
	{
		delete m_pTimeOutTimer;
		m_pTimeOutTimer = nullptr;
	}

	if (m_pRectUpdatetTimer)
	{
		delete m_pRectUpdatetTimer;
		m_pRectUpdatetTimer = nullptr;
	}
}

void AnkerLoadingMask::updateMaskRect(wxPoint newPos, wxSize newSize)
{
	SetSize(newSize);
	SetPosition(newPos);
}

void AnkerLoadingMask::start()
{
	m_loadingCounter = 0;
	if (m_pAnimationTimer)
		m_pAnimationTimer->Start(1000 / m_deltaCount, false);

	if (m_timeoutMS > 0 && m_pTimeOutTimer)
		m_pTimeOutTimer->StartOnce(m_timeoutMS);
}

void AnkerLoadingMask::stop()
{
	m_loadingCounter = 0;
	if (m_pAnimationTimer)
		m_pAnimationTimer->Stop();
	if (m_pTimeOutTimer)
		m_pTimeOutTimer->Stop();
}

bool AnkerLoadingMask::isLoading()
{
	if (m_pTimeOutTimer)
		return m_pTimeOutTimer->IsRunning();

	return false;
}

void AnkerLoadingMask::initUI()
{
	SetTransparent(200);

	SetBackgroundColour(wxColour(41, 42, 45));

	wxBoxSizer* mumSizer = new wxBoxSizer(wxVERTICAL);

	mumSizer->AddStretchSpacer(1);

	for (int i = 0; i < MUM_IMAGE_COUNT; i++)
	{
		m_mumImage[i] = wxImage(wxString::FromUTF8(Slic3r::var("preparingVideo" + std::to_string(i + 1) + ".png")), wxBITMAP_TYPE_PNG);
		m_mumImage[i].Rescale(40, 40);
	}
	m_pMumLabel = new wxStaticBitmap(this, wxID_ANY, m_mumImage[0]);
	m_pMumLabel->SetMinSize(m_mumImage[0].GetSize());
	m_pMumLabel->SetMaxSize(m_mumImage[0].GetSize());
	m_pMumLabel->SetBackgroundColour(wxColour(PANEL_BACK_COLOR));
	mumSizer->Add(m_pMumLabel, 0, wxALIGN_CENTER, 0);

	mumSizer->AddSpacer(3);

	m_pLoadingLabel = new wxStaticText(this, wxID_ANY, m_loadingText);
	m_pLoadingLabel->SetMinSize(AnkerSize(70, 25));
	m_pLoadingLabel->SetBackgroundColour(wxColour(PANEL_BACK_COLOR));
	m_pLoadingLabel->SetForegroundColour(wxColour(TEXT_LIGHT_COLOR));
	m_pLoadingLabel->SetFont(ANKER_FONT_NO_1);
	mumSizer->Add(m_pLoadingLabel, 0, wxALIGN_CENTER, 0);

	mumSizer->AddStretchSpacer(1);

	SetSizer(mumSizer);
}

void AnkerLoadingMask::OnShow(wxShowEvent& event)
{
	bool shown = event.IsShown();

	if (m_rectUpdateFlag)
	{
		if (shown)
		{
			m_pRectUpdatetTimer->Start(m_rectUpdateFreq);
		}
		else if (!shown)
		{
			m_pRectUpdatetTimer->Stop();
		}
	}
}

void AnkerLoadingMask::OnTimeEvent(wxTimerEvent& event)
{
	m_loadingCounter++;
	if (m_loadingCounter == m_deltaCount)
		m_loadingCounter = 0;

	m_pMumLabel->SetBitmap(m_mumImage[m_loadingCounter]);

	m_pAnimationTimer->Start(1000 / m_deltaCount, false);

	if (IsShown())
		Refresh();
}

void AnkerLoadingMask::OnTimeOutEvent(wxTimerEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxANKEREVT_LOADING_TIMEOUT);
	ProcessEvent(evt);
}

void AnkerLoadingMask::OnRectUpdateEvent(wxTimerEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxANKEREVT_LOADMASK_RECTUPDATE);
	ProcessEvent(evt);
}
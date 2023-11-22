#include "AnkerTitledPanel.hpp"

#include "AnkerGUIConfig.hpp"
#include "AnkerSplitCtrl.hpp"
#include "Slic3r/GUI/GUI_App.hpp"


wxDEFINE_EVENT(wxANKEREVT_ATP_BUTTON_CLICKED, wxCommandEvent);

AnkerTitledPanel::AnkerTitledPanel(wxWindow* parent, int titleHeight, int titleBorder)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
	, m_titleHeight(titleHeight)
	, m_titleBorder(titleBorder)
	, m_titleBeforeBtnCount(0)
	, m_titleAfterBtnCount(0)
	, m_pTitleText(nullptr)
	, m_pTitleBeforeStretch(nullptr)
	, m_pTitleAfterStretch(nullptr)
	, m_pTitleSizer(nullptr)
	, m_pMainSizer(nullptr)
	, m_pTitledPanel(nullptr)
	, m_pContentPanel(nullptr)
{
	initUI();
}

AnkerTitledPanel::~AnkerTitledPanel()
{
	m_pTitleBtnList.clear();
}

void AnkerTitledPanel::setTitle(wxString title)
{
	if (m_pTitleText)
	{
		if (title.size() > 20)
			title = title.substr(0, 10) + "..." + title.substr(title.size() - 10);
		m_pTitleText->SetLabel(title);
		m_pTitleText->Fit();
	}
}

void AnkerTitledPanel::setTitleAlign(TitleAlign align)
{
	if (m_pTitleBeforeStretch && m_pTitleAfterStretch)
	{
		m_pTitleBeforeStretch->SetProportion(align == LEFT ? 0 : 1);
		m_pTitleAfterStretch->SetProportion(align == RIGHT ? 0 : 1);

		m_pTitledPanel->Layout();
	}
}

int AnkerTitledPanel::addTitleButton(wxString iconPath, bool beforeTitle)
{
	if (!iconPath.empty() && m_pTitleSizer)
	{
		wxImage icon = wxImage(iconPath, wxBITMAP_TYPE_PNG);
		if (icon.IsOk())
		{
			int id = m_pTitleBtnList.size();
			//int iconWH = 16;
			//icon.Rescale(iconWH, iconWH);

			wxButton* btn = new wxButton(m_pTitledPanel, wxID_HIGHEST + id, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			btn->SetBitmap(icon);
			btn->SetBackgroundColour(GetBackgroundColour());
			btn->SetMinSize(icon.GetSize());
			btn->SetMaxSize(icon.GetSize());
			btn->SetSize(icon.GetSize());
			//btn->Fit();
			btn->Bind(wxEVT_BUTTON, &AnkerTitledPanel::OnButton, this);
			btn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
			btn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });

			m_pTitleSizer->Insert(beforeTitle ? m_titleBeforeBtnCount : m_titleBeforeBtnCount + 3, btn, 0, (beforeTitle ? wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT : wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxRIGHT), 0);

			Fit();
			Layout();
			Refresh();

			m_pTitleBtnList.push_back({ btn, beforeTitle });
			beforeTitle ? m_titleBeforeBtnCount++ : m_titleAfterBtnCount++;

			return id;
		}
	}

	return -1;
}

bool AnkerTitledPanel::removeTitleButton(int id)
{
	if (id < m_pTitleBtnList.size() && id > -1)
	{
		if (m_pTitleBtnList[id].first)
		{
			m_pTitleSizer->Detach(m_pTitleBtnList[id].first);

			if (m_pTitleBtnList[id].second)
				m_titleBeforeBtnCount--;
			else
				m_titleAfterBtnCount--;

			m_pTitleBtnList[id].first->Unbind(wxEVT_BUTTON, &AnkerTitledPanel::OnButton, this);
			m_pTitleBtnList[id].first->SetParent(nullptr);
			delete m_pTitleBtnList[id].first;
			m_pTitleBtnList[id].first = nullptr;

			return true;
		}
	}

	return false;
}

void AnkerTitledPanel::setTitleButtonVisible(int id, bool visible)
{
	if (id < m_pTitleBtnList.size() && id > -1)
	{
		if (m_pTitleBtnList[id].first)
		{
			m_pTitleBtnList[id].first->Show(visible);
		}
	}
}

void AnkerTitledPanel::setTitleMoveObject(wxWindow* target)
{
	m_pMoveObject = target;
}

wxPanel* AnkerTitledPanel::setContentPanel(wxPanel* content)
{
	wxPanel* oldContent = m_pContentPanel;

	if (m_pContentPanel)
	{
		m_pMainSizer->Detach(m_pContentPanel);
	}

	if (content)
	{
		m_pMainSizer->Add(content, 1, wxEXPAND | wxALIGN_CENTER, 0);
	}

	m_pContentPanel = content;

	return oldContent;
}

void AnkerTitledPanel::initUI()
{
	SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));

	m_pMainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_pMainSizer);

	int titlePanelHeight = m_titleHeight - 2 * m_titleBorder;
	m_pTitledPanel = new wxPanel(this);
	//m_pTitledPanel->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
	m_pTitledPanel->SetMinSize(AnkerSize(300, titlePanelHeight));
	m_pTitledPanel->SetMaxSize(AnkerSize(500, titlePanelHeight));
	m_pTitledPanel->SetSize(AnkerSize(300, titlePanelHeight));
	m_pMainSizer->Add(m_pTitledPanel, 1,  wxEXPAND | wxALIGN_TOP | wxTOP | wxBOTTOM, m_titleBorder);

	auto titleLineSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pTitledPanel->SetSizer(titleLineSizer);

	m_pTitleSizer = new wxBoxSizer(wxHORIZONTAL);
	auto titleLineBefore = titleLineSizer->AddStretchSpacer(0);
	titleLineBefore->SetMinSize(AnkerSize(12	,0));
	titleLineSizer->Add(m_pTitleSizer,1, wxEXPAND);
	//m_pTitledPanel->SetSizer(m_pTitleSizer);

	m_pTitleBeforeStretch = m_pTitleSizer->AddStretchSpacer(1);
	
	m_pTitleText = new wxStaticText(m_pTitledPanel, wxID_ANY, L"Title Center");
	//m_pTitleText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
	m_pTitleText->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
//	wxFont font = m_pTitleText->GetFont();
//#ifdef __APPLE__
//	font.SetPointSize(12);
//#else
//	font.SetPointSize(10);
//#endif // __APPLE___
//	m_pTitleText->SetFont(font);
	m_pTitleText->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pTitleSizer->Add(m_pTitleText, 0, wxALIGN_CENTER , 4);

	m_pTitleAfterStretch = m_pTitleSizer->AddStretchSpacer(1);


	AnkerSplitCtrl* splitCtrl = new AnkerSplitCtrl(this);
	m_pMainSizer->Add(splitCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);


	auto titleLineAfter = titleLineSizer->AddStretchSpacer(0);
	titleLineAfter->SetMinSize(AnkerSize(16, 0));
}

void AnkerTitledPanel::OnButton(wxCommandEvent& event)
{
	int btnID = -1;
	wxButton* eventBtn = dynamic_cast<wxButton*>(event.GetEventObject());
	if (eventBtn)
	{
		btnID = eventBtn->GetId() - wxID_HIGHEST;
		wxCommandEvent evt = wxCommandEvent(wxANKEREVT_ATP_BUTTON_CLICKED);
		evt.SetInt(btnID);
		ProcessEvent(evt);
	}
}

#include "AnkerNavWidget.hpp"
#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "../Utils/DataManger.hpp"
#include "libslic3r/Utils.hpp"
#include "wxExtensions.hpp"

#define BTN_BACKGROUND_COLOR "#292A2D"
#define BTN_HOVER_COLOR		 "#354138"

wxDEFINE_EVENT(wxCUSTOMEVT_SWITCH_DEVICE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_BTN_CLICKED, wxCommandEvent);

AnkerBtnList::AnkerBtnList(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/) :
	wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_pScrolledwinSizer(nullptr),
	m_currentIndex(-1),
	m_pScrolledWindow(nullptr)
{
	SetBackgroundColour(wxColour(BTN_BACKGROUND_COLOR));
	initUi();
}

void AnkerBtnList::clearExpiredTab(const std::string& sn)
{
	for (auto iter = m_tabBtnList.begin(); iter != m_tabBtnList.end(); )
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		std::string widgetId = std::string();
		if (pIterSnId)
		{
			widgetId = pIterSnId->GetData().ToStdString();
			if (!Datamanger::GetInstance().checkSnExist(widgetId))
			{
				delete* iter;
				iter = m_tabBtnList.erase(iter);
			}
			else
			{
				(*iter)->setBtnStatus(NORMAL_BTN);
				auto deviceObj = Datamanger::GetInstance().getDeviceObjectFromSn(widgetId);
				if(deviceObj)
					(*iter)->setTabBtnName(deviceObj->station_name);
				++iter;
			}
		}
	}

	if(m_tabBtnList.size() > 0)
		m_tabBtnList.at(0)->setBtnStatus(SELECT_BTN);
}


bool AnkerBtnList::checkTabExist(const std::string& sn)
{
	for (auto iter = m_tabBtnList.begin(); iter != m_tabBtnList.end(); )
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		std::string widgetId = std::string();
		if (pIterSnId)
		{
			widgetId = pIterSnId->GetData().ToStdString();
			if (widgetId == sn)
				return true;
		}
		iter++;
	}

	return false;
}

void AnkerBtnList::clearList()
{
	int itemCount = m_pScrolledwinSizer->GetItemCount();
	for (int i = 0; i < itemCount; i++)
	{
		m_pScrolledwinSizer->Remove(0);
	}

	for (auto ptr : m_tabBtnList) {
		delete ptr;
	}

	m_tabBtnList.clear();
}

int AnkerBtnList::getCount()
{
	return m_tabBtnList.size();
}

void AnkerBtnList::SetCurrentTab(const int& index)
{
	if (index > m_tabBtnList.size())
		return;

	m_currentIndex = index;
}

bool AnkerBtnList::InsertTab(const size_t& index, const std::string& tabName, const std::string& snID, bool isSelect /*= false*/)
{
	static int btnIndex = 0;
	AnkerTabBtn* pBtn = new AnkerTabBtn(m_pScrolledWindow, wxID_ANY, tabName);
	wxVariant myData(snID);

	pBtn->SetClientObject(new wxStringClientData(myData));
	pBtn->SetMinSize(wxSize(275, 30));
	pBtn->SetId(btnIndex);
	m_pScrolledwinSizer->Add(pBtn, 0, wxALIGN_CENTER, 5);

	if (m_tabBtnList.size() <= 0)
		pBtn->setBtnStatus(SELECT_BTN);

	m_tabBtnList.push_back(pBtn);

	pBtn->Bind(wxEVT_BUTTON, [this, pBtn](wxCommandEvent& event) {

		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SWITCH_DEVICE);
		evt.SetClientObject(pBtn->GetClientObject());
		wxPostEvent(this->GetParent(), evt);

		pBtn->setBtnStatus(SELECT_BTN);
		auto iter = m_tabBtnList.begin();
		while (iter != m_tabBtnList.end())
		{
			if ((*iter) == pBtn)
			{
				iter++;
				continue;
			}

			(*iter)->setBtnStatus(NORMAL_BTN);
			iter++;
		}

		});

	btnIndex++;
	return true;
}

void AnkerBtnList::RemoveTab(const size_t& index)
{
	if (index == m_currentIndex)
	{
		m_currentIndex = 0;
	}

	int tempIndex = 0;
	AnkerTabBtn* pBtn = nullptr;

	for (auto iter = m_tabBtnList.begin(); iter != m_tabBtnList.end(); iter++)
	{
		if (index == tempIndex)
		{
			pBtn = *iter;
			m_tabBtnList.erase(iter);
		}

		tempIndex++;
	}
	if (pBtn)
	{
		m_pScrolledwinSizer->Remove(index);
		delete pBtn;
		pBtn = nullptr;
	}
}

void AnkerBtnList::initUi()
{
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);
	m_pScrolledwinSizer = new wxBoxSizer(wxVERTICAL);	
	
	m_pScrolledWindow = new wxScrolledWindow(this,
											 wxID_ANY);

	m_pScrolledWindow->SetSizer(m_pScrolledwinSizer);	
	m_pScrolledWindow->SetScrollbars(0, 20, 275, 700);
	pMainVSizer->Add(m_pScrolledWindow, wxEXPAND | wxALIGN_CENTER, wxEXPAND | wxALIGN_CENTER, 0);

	SetSizer(pMainVSizer);
}


BEGIN_EVENT_TABLE(AnkerTabBtn, wxControl)
EVT_SIZE(AnkerTabBtn::OnSize)
EVT_LEFT_DOWN(AnkerTabBtn::OnClick)
EVT_PAINT(AnkerTabBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerTabBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerTabBtn::OnLeave)
EVT_LEFT_DOWN(AnkerTabBtn::OnPressed)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerTabBtn, wxControl)

#define ankerLabel_ID  1873

AnkerTabBtn::AnkerTabBtn(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxString& btnName /*= wxString("")*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/) :
	wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_status(ANKER_BTN_STATUS::NORMAL_BTN)
{

	SetBackgroundColour(wxColour(BTN_BACKGROUND_COLOR));

	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("M5_device_n.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(22, 22);
	wxBitmap scaledBitmap(image);
	m_icon = scaledBitmap;

	m_tabName = btnName;
}

AnkerTabBtn::AnkerTabBtn()
{

}


void AnkerTabBtn::OnClick(wxMouseEvent& event)
{
	wxCommandEvent evt(wxEVT_BUTTON, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}

void AnkerTabBtn::OnEnter(wxMouseEvent& event)
{
	if (m_status == SELECT_BTN)
		return;

	m_status = HOVER_BTN;
	Refresh();	
}

void AnkerTabBtn::OnLeave(wxMouseEvent& event)
{
	if (!IsEnabled())
	{
		return;
	}

	wxPoint pos = event.GetPosition();

	if (m_status != SELECT_BTN)
		m_status = NORMAL_BTN;
	else
		return;
	Refresh();	
}

void AnkerTabBtn::OnPressed(wxMouseEvent& event)
{
	m_status = SELECT_BTN;
	Refresh();	
}

bool AnkerTabBtn::Enable(bool enable /*= true*/)
{
	if (enable)
	{
		m_status = NORMAL_BTN;
	}
	else
	{
		m_status = DISABLE_BTN;
	}

	Refresh(false);	
	return wxControl::Enable(enable);
}


void AnkerTabBtn::OnSize(wxSizeEvent& event)
{
	Refresh();
}

void AnkerTabBtn::setTabBtnName(const std::string& btnName)
{	
	m_tabName = btnName;
	Update();
}

void AnkerTabBtn::setBtnStatus(ANKER_BTN_STATUS status)
{
	m_status = status;	
	Refresh();
}

void AnkerTabBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rect = GetClientRect();

	switch (m_status)
	{
	case NORMAL_BTN:
	{		
		dc.Clear();
		wxBrush brush(BTN_BACKGROUND_COLOR);
		wxPen pen(BTN_BACKGROUND_COLOR);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);
	}
	break;
	case HOVER_BTN:
	{
		
		wxBrush brush(BTN_HOVER_COLOR);
		wxPen pen(BTN_HOVER_COLOR);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);
	}
	break;
	case SELECT_BTN:
	{		
		wxBrush brush(BTN_HOVER_COLOR);
		wxPen pen(BTN_HOVER_COLOR);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);

	}
	break;
	case DISABLE_BTN:
	{		
		wxBrush brush(BTN_HOVER_COLOR);
		wxPen pen(BTN_HOVER_COLOR);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);

	}
	break;
	case UNKNOWN_BTN:
	{		
		dc.Clear();
		wxBrush brush(wxTRANSPARENT);
		wxPen pen(wxTRANSPARENT);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);
	}
	break;

	default:
	{		
		dc.Clear();
		wxBrush brush(wxTRANSPARENT);
		wxPen pen(wxTRANSPARENT);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRectangle(rect);
	}
	break;
	}
	
	dc.DrawBitmap(m_icon, wxPoint(9, 4));

	{		
		wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
		dc.SetFont(font);
		wxBrush brush(wxColour(BTN_BACKGROUND_COLOR));
		wxPen pen(wxColour(255, 255, 255));	
		dc.SetTextForeground(wxColour(169, 170, 171));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawText(m_tabName, wxPoint(42,7));
	}
	
}

void AnkerTabBtn::OnLabelClicked(AnkerCustomEvent& event)
{
	wxCommandEvent evt(wxEVT_BUTTON, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}

AnkerNavBar::AnkerNavBar(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	InitUi();
}


AnkerNavBar::~AnkerNavBar()
{

}


void AnkerNavBar::updateRefresh(bool isRefreh)
{
	m_isRefresh = isRefreh;
}

bool AnkerNavBar::checkTabExist(const std::string& sn)
{
	return m_navBarList->checkTabExist(sn);
}

void AnkerNavBar::clearExpiredTab(const std::string& sn)
{
	m_navBarList->clearExpiredTab(sn);
}

int AnkerNavBar::getCount() const
{
	return m_navBarList->getCount();
}

void AnkerNavBar::addItem(const std::string& ItemName, const std::string& snId)
{
	m_navBarList->InsertTab(m_navBarList->getCount(), ItemName, snId);
}


void AnkerNavBar::clearItem()
{
	m_navBarList->clearList();
}

void AnkerNavBar::showEmptyPanel(bool isShow)
{
	if (isShow)
	{
		m_navBarList->Hide();
		m_emptyPanel->Show(true);
	}
	else
	{
		m_navBarList->Show(true);
		m_emptyPanel->Hide();
	}
	Layout();	
	Refresh();
}

void AnkerNavBar::InitUi()
{

	m_reloadTimer = new wxTimer(this,wxID_ANY);	
	m_reloadTimer->Bind(wxEVT_TIMER, &AnkerNavBar::OnTimer, this);

	SetBackgroundColour(wxColour("#292A2D"));
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	{
		wxPanel* pTitlePanel = new wxPanel(this);
		pTitlePanel->SetMinSize(wxSize(240, 45));
		pTitlePanel->SetBackgroundColour(wxColour("#292A2D"));

		wxStaticText* pTitleText = new wxStaticText(pTitlePanel, wxID_ANY, L"Printers");
		pTitleText->SetForegroundColour(wxColour("#FFFFFF"));

		wxFont titleFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, wxFONTWEIGHT_NORMAL);
		titleFont.SetUnderlined(false);		
		pTitleText->SetFont(titleFont);

		wxClientDC dc(this);
		dc.SetFont(pTitleText->GetFont());
		wxSize titleSize = dc.GetTextExtent(L"Printers");
		pTitleText->SetMinSize(wxSize(120,30));		
		pTitleText->SetSize(wxSize(120, 30));
		
		m_reloadBtn = new wxButton(pTitlePanel, wxID_ANY ,"", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_reloadBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			
			if (m_isRefresh)
				return;

			m_reloadTimer->SetOwner(this, wxID_ANY);
			m_reloadTimer->StartOnce(3000);

			Connect(m_reloadTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler(AnkerNavBar::OnTimer), NULL, this);

			m_reloadBtn->Refresh();
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_BTN_CLICKED);
			ProcessEvent(evt);

			m_isRefresh = true;
			});
		wxImage btnImage = wxImage(wxString::FromUTF8(Slic3r::var("update_ccw_icon.png")), wxBITMAP_TYPE_PNG);
		btnImage.Rescale(20, 20);
		m_reloadBtn->SetBitmap(btnImage);
		m_reloadBtn->SetMinSize(wxSize(25, 25));
		m_reloadBtn->SetMaxSize(wxSize(25, 25));

#ifdef _WIN32
		m_reloadBtn->SetWindowStyle(wxBORDER_NONE);				
#endif
		m_reloadBtn->SetBackgroundColour(pTitlePanel->GetBackgroundColour());
		wxBoxSizer* pTitlePanelHSizer = new wxBoxSizer(wxHORIZONTAL);

		pTitlePanelHSizer->Add(pTitleText, wxEXPAND | wxALL, wxEXPAND|wxALL, 15);		
		pTitlePanelHSizer->AddStretchSpacer(1);
		pTitlePanelHSizer->Add(m_reloadBtn, 0, wxALL, 10);

		pTitlePanel->SetSizer(pTitlePanelHSizer);
		pMainVSizer->Add(pTitlePanel);

		wxPanel* pLine = new wxPanel(this, wxID_ANY);
		pLine->SetBackgroundColour(wxColour("#38393C"));
		pLine->SetMaxSize(wxSize(500, 1));
		pLine->SetMinSize(wxSize(500, 1));
		pMainVSizer->Add(pLine);
	}

	{
		m_navBarList = new AnkerBtnList(this, wxID_ANY, wxPoint(0, 0), wxSize(214, 705));
		m_navBarList->Bind(wxCUSTOMEVT_SWITCH_DEVICE, [this](wxCommandEvent& event) {

			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SWITCH_DEVICE);
			evt.SetClientObject(event.GetClientObject());
			wxPostEvent(this->GetParent(), evt);
			});
		pMainVSizer->Add(m_navBarList, wxEXPAND, wxALIGN_CENTER, 0);
		
	}

	{
		m_emptyPanel = new wxPanel(this);
		m_emptyPanel->SetBackgroundColour(wxColour("#292A2D"));
		wxBoxSizer* pEmptyPanelHSizer = new wxBoxSizer(wxVERTICAL);

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("file_icon_50x50.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(64, 64);
		wxBitmap bitmap(image);
		wxStaticBitmap* staticBitmap = new wxStaticBitmap(m_emptyPanel, wxID_ANY, bitmap);

		wxStaticText* pEmptyText = new wxStaticText(m_emptyPanel,
			wxID_ANY,
			L"No Printer, Please Use The AnkerMake APP,to Add A Device.");
		pEmptyText->SetForegroundColour(wxColour("#999999"));		
		pEmptyText->Wrap(142);
		wxClientDC dc(this);
		dc.SetFont(pEmptyText->GetFont());
		wxSize size = dc.GetTextExtent(L"No Printer, Please Use The AnkerMake APP,to Add A Device.");
		int textHeight = (size.GetWidth()/142 + 2)* size.GetHeight();

		pEmptyText->SetMinSize(wxSize(142, textHeight));
		

		pEmptyPanelHSizer->AddSpacer(36);
		pEmptyPanelHSizer->Add(staticBitmap, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pEmptyPanelHSizer->AddSpacer(8);
		pEmptyPanelHSizer->Add(pEmptyText, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

		m_emptyPanel->SetSizer(pEmptyPanelHSizer);
		m_emptyPanel->Hide();
		pMainVSizer->Add(m_emptyPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	}

	SetSizer(pMainVSizer);
}

void AnkerNavBar::OnTimer(wxTimerEvent& event)
{
	updateRefresh(false);
	m_reloadTimer->Stop();
}


#include "AnkerNavWidget.hpp"
#include "wx/univ/theme.h"
#include "wx/artprov.h"
#include "libslic3r/Utils.hpp"
#include "wxExtensions.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"

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
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	for (auto iter = m_tabBtnList.begin(); iter != m_tabBtnList.end(); )
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		std::string widgetId = std::string();
		if (pIterSnId)
		{
			widgetId = pIterSnId->GetData().ToStdString();
			if (!ankerNet->getDeviceObjectFromSn(widgetId))
			{
				delete* iter;
				iter = m_tabBtnList.erase(iter);
			}
			else
			{
				(*iter)->setBtnStatus(NORMAL_BTN);
				auto deviceObj = CurDevObject(widgetId);
				if(deviceObj)
					(*iter)->setTabBtnName(deviceObj->GetStationName());
				++iter;
			}
		}
	}
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

void AnkerBtnList::switchTabFromSn(const std::string& sn)
{
	ANKER_LOG_INFO << "switch nav for id: " << sn.c_str();
	for (auto iter = m_tabBtnList.begin(); iter != m_tabBtnList.end(); )
	{
		wxStringClientData* pIterSnId = static_cast<wxStringClientData*>((*iter)->GetClientObject());
		std::string widgetId = std::string();
		if (pIterSnId)
		{
			widgetId = pIterSnId->GetData().ToStdString();
			if (widgetId == sn) {
				SetCurrentTab((*iter)->GetId());
				(*iter)->setBtnStatus(SELECT_BTN);
			}
			else {
				(*iter)->setBtnStatus(NORMAL_BTN);
			}
		}
		iter++;
	}
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
	wxString  wxTabName(tabName.c_str(), wxConvUTF8);
	static int btnIndex = 0;
	AnkerTabBtn* pBtn = new AnkerTabBtn(m_pScrolledWindow, snID, wxID_ANY, wxTabName);

	DeviceObjectBasePtr deviceObj = CurDevObject(snID);

	if (!deviceObj)
	{
		ANKER_LOG_ERROR << "insert tab fail for sn:" << snID;
		return false;
	}
	wxVariant myData(snID);
	if (deviceObj->GetDeviceType() == DEVICE_V8111_TYPE)//default show
	{
		pBtn->setIcon("V8111_device_n.png");
	}
	else if (deviceObj->GetDeviceType() == DEVICE_V8110_TYPE)
	{
		pBtn->setIcon("V8110_device_n.png");
	}

	if (!deviceObj->IsOnlined())
		pBtn->setOnlineStatus(false);

	pBtn->SetClientObject(new wxStringClientData(myData));
	pBtn->SetMinSize(AnkerSize(214, 40));
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


void AnkerBtnList::setTabOnlineStatus(bool isOnline, const wxString& snId)
{
	auto iter = m_tabBtnList.begin();
	while (iter != m_tabBtnList.end())
	{
		if ((*iter)->getSnId() == snId)
			(*iter)->setOnlineStatus(isOnline);
		++iter;
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
	const wxString& snId,
	wxWindowID winid /*= wxID_ANY*/,
	const wxString& btnName /*= wxString("")*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/) :
	wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_status(ANKER_BTN_STATUS::NORMAL_BTN),
	m_snid(snId)
{

	SetBackgroundColour(wxColour(BTN_BACKGROUND_COLOR));

	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("V8111_device_n.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(22, 22, wxIMAGE_QUALITY_HIGH);
	wxBitmap scaledBitmap(image);
	m_icon = scaledBitmap;


	wxImage offlineImage = wxImage(wxString::FromUTF8(Slic3r::var("device_offline.png")), wxBITMAP_TYPE_PNG);
	offlineImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
	wxBitmap offlineScaledBitmap(offlineImage);
	

	m_offlineIcon = offlineScaledBitmap;
	m_tabName = btnName;
}

AnkerTabBtn::AnkerTabBtn()
{

}


wxString AnkerTabBtn::getSnId()
{
	return m_snid;
}

void AnkerTabBtn::setIcon(const wxString& strIcon)
{
	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var(strIcon.ToStdString())), wxBITMAP_TYPE_PNG);
	image.Rescale(22, 22, wxIMAGE_QUALITY_HIGH);
	wxBitmap scaledBitmap(image);
	m_icon = scaledBitmap;
	Refresh();
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
	wxString  wxTabName(btnName.c_str(), wxConvUTF8);
	m_tabName = wxTabName;
	Update();
}

void AnkerTabBtn::setBtnStatus(ANKER_BTN_STATUS status)
{
	m_status = status;	
	Refresh();
}


void AnkerTabBtn::setOnlineStatus(bool isOnline)
{
	m_isOnline = isOnline;
	Update();
}

void AnkerTabBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rect = GetClientRect();

	wxPoint iconPoint;
	wxPoint textPoint;
	wxPoint logoPoint;
	iconPoint.x = 12;
	int yy = GetRect().y;
	int hh = GetRect().GetHeight();
	int ww = GetRect().GetWidth();

	iconPoint.y = (GetRect().GetHeight() - m_icon.GetHeight())/2;

	dc.SetFont(ANKER_FONT_NO_1);
	wxSize textSize = dc.GetTextExtent(m_tabName);
	int textHeight = textSize.GetHeight();

#ifdef __APPLE__
	wxFontMetrics metrics = dc.GetFontMetrics();
	textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__

	textPoint.x = 18 + m_icon.GetWidth();
	textPoint.y = (GetRect().GetHeight() - textHeight) / 2;

	logoPoint.x = GetRect().GetWidth() - 12 - m_offlineIcon.GetWidth();
	logoPoint.y = (GetRect().GetHeight() - m_offlineIcon.GetHeight()) / 2;

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

	dc.DrawBitmap(m_icon, iconPoint);

	{
		dc.SetFont(ANKER_FONT_NO_2);
		wxBrush brush(wxColour(BTN_BACKGROUND_COLOR));
		wxPen pen(wxColour(255, 255, 255));
		dc.SetTextForeground(wxColour(169, 170, 171));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_FONT_NO_1);
		dc.DrawText(m_tabName, textPoint);
	}

	if (!m_isOnline)
	{	
		int xwith = GetRect().GetWidth() - 87;
		dc.DrawBitmap(m_offlineIcon, logoPoint);
		
	}
	//TODO: by Samuel, draw printer status picture
	
}

void AnkerTabBtn::OnLabelClicked(AnkerCustomEvent& event)
{
	wxCommandEvent evt(wxEVT_BUTTON, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}

void AnkerTabBtn::GetPrinterStatus()
{

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


void AnkerNavBar::stopLoading()
{
	m_reloadBtn->stopLoading();
}


bool AnkerNavBar::checkTabExist(const std::string& sn)
{
	return m_navBarList->checkTabExist(sn);
}

void AnkerNavBar::switchTabFromSn(const std::string& sn)
{
	m_navBarList->switchTabFromSn(sn);
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


void AnkerNavBar::setTabOnlineStatus(bool isOnline, const wxString& snId)
{
	m_navBarList->setTabOnlineStatus(isOnline, snId);
}

void AnkerNavBar::InitUi()
{
	SetBackgroundColour(wxColour("#292A2D"));
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	{
		wxPanel* pTitlePanel = new wxPanel(this);
		pTitlePanel->SetMaxSize(AnkerSize(240, 35));
		pTitlePanel->SetSize(AnkerSize(240, 35));
		pTitlePanel->SetBackgroundColour(wxColour("#292A2D"));

		wxStaticText* pTitleText = new wxStaticText(pTitlePanel, wxID_ANY, _L("common_print_printerlist_title"));
		pTitleText->SetForegroundColour(wxColour("#FFFFFF"));
			
		pTitleText->SetFont(ANKER_BOLD_FONT_NO_1);
		wxClientDC dc(this);
		dc.SetFont(pTitleText->GetFont());
		wxSize titleSize = dc.GetTextExtent(_L("common_print_printerlist_title"));
		int textHeight = titleSize.GetHeight();

#ifdef __APPLE__
		wxFontMetrics metrics = dc.GetFontMetrics();
		textHeight = metrics.ascent + metrics.descent;
#endif // !__APPLE__


		pTitleText->SetMinSize(AnkerSize(120, textHeight));
		pTitleText->SetSize(AnkerSize(120, textHeight));
		
		wxImage btnImage = wxImage(wxString::FromUTF8(Slic3r::var("update_ccw_icon.png")), wxBITMAP_TYPE_PNG);
		btnImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		m_reloadBtn = new AnkerLoadingLabel(pTitlePanel, btnImage, "#292A2D"); 
		wxCursor handCursor(wxCURSOR_HAND);		
		m_reloadBtn->SetCursor(handCursor);
		m_reloadBtn->SetWindowStyleFlag(wxBORDER_NONE);
		m_reloadBtn->SetMinSize(AnkerSize(16, 16));
		m_reloadBtn->SetMaxSize(AnkerSize(16, 16));

		m_reloadBtn->Bind(wxCUSTOMEVT_ANKER_LOADING_LABEL_CLICKED, [this](wxCommandEvent& event) {
			ANKER_LOG_INFO << "reload devs list";
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_BTN_CLICKED);			
			wxPostEvent(this, evt);
		});

#ifdef _WIN32
		m_reloadBtn->SetWindowStyle(wxBORDER_NONE);				
#endif
		m_reloadBtn->SetBackgroundColour(pTitlePanel->GetBackgroundColour());
		wxBoxSizer* pTitlePanelHSizer = new wxBoxSizer(wxHORIZONTAL);

		pTitlePanelHSizer->AddSpacer(5);
		pTitlePanelHSizer->Add(pTitleText, wxEXPAND|wxALL, wxEXPAND|wxALL, 7);		
		pTitlePanelHSizer->AddStretchSpacer(1);
		pTitlePanelHSizer->Add(m_reloadBtn, wxEXPAND | wxALL, wxEXPAND | wxALL, 7);
		pTitlePanelHSizer->AddSpacer(5);

		pTitlePanel->SetSizer(pTitlePanelHSizer);
		pMainVSizer->AddSpacer(8);
		pMainVSizer->Add(pTitlePanel);
		pMainVSizer->AddSpacer(8);

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
		image.Rescale(64, 64, wxIMAGE_QUALITY_HIGH);
		wxBitmap bitmap(image);
		wxStaticBitmap* staticBitmap = new wxStaticBitmap(m_emptyPanel, wxID_ANY, bitmap);

		wxStaticText* pEmptyText = new wxStaticText(m_emptyPanel,
			wxID_ANY,
			_L("common_print_statusnotice_noprinter"));
		pEmptyText->SetForegroundColour(wxColour("#999999"));	
		pEmptyText->SetFont(ANKER_FONT_NO_1);
		wxString emptyText = Slic3r::GUI::WrapEveryCharacter(_L("common_print_statusnotice_noprinter"), ANKER_FONT_NO_1,140);
		pEmptyText->Wrap(142);
		pEmptyText->SetLabelText(emptyText);
		wxClientDC dc(this);
		dc.SetFont(pEmptyText->GetFont());
		wxSize size = dc.GetTextExtent(_L("common_print_statusnotice_noprinter"));
		int textHeight = (size.GetWidth()/150 + 3)* size.GetHeight();

		pEmptyText->SetMinSize(AnkerSize(150, textHeight));
		

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

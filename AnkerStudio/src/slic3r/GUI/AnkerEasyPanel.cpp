#include "AnkerEasyPanel.hpp"
#include <wx/event.h>
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "common/AnkerGUIConfig.hpp"


BEGIN_EVENT_TABLE(AnkerEasyItem, wxControl)
EVT_PAINT(AnkerEasyItem::OnPaint)
EVT_ENTER_WINDOW(AnkerEasyItem::OnEnter)
EVT_LEAVE_WINDOW(AnkerEasyItem::OnLeave)
EVT_LEFT_DOWN(AnkerEasyItem::OnPressed)
EVT_LEFT_DCLICK(AnkerEasyItem::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerEasyItem, wxControl)

#ifndef __APPLE__
#define WIDGET_WIDTH (420)
#define WIDGET_WRAP_WITH AnkerLength(WIDGET_WIDTH - 20)
#else
#define WIDGET_WIDTH (400)
#define WIDGET_WRAP_WITH AnkerLength(WIDGET_WIDTH - 30)
#endif // !__APPLE__



wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_EASYITEM_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_EASY_ITEM_CLICKED, wxCommandEvent);
AnkerEasyItem::AnkerEasyItem()
{

}

AnkerEasyItem::AnkerEasyItem(wxWindow* parent,
	wxString title,
	wxImage  logo,
	wxImage  selectLogo,
	wxImage  checkLogo,
	wxWindowID winid /*= wxID_ANY*/, 
	const wxPoint& pos /*= wxDefaultPosition*/, 
	const wxSize& size /*= wxDefaultSize*/)
	: m_title(title)
	, m_logo(logo)
	, m_selectLogo(selectLogo)
	, m_checkLogo(checkLogo)

{
	Create(parent, winid, pos, size, wxBORDER_NONE, wxDefaultValidator);
	SetBackgroundColour(wxColour("#292A2D"));
}

AnkerEasyItem::~AnkerEasyItem()
{

}

wxString AnkerEasyItem::getTitle()
{
	return m_title;
}

void AnkerEasyItem::setSelectStatus(const ANKER_EASY_ITEM& isSelect)
{
	m_Status = isSelect;
	Refresh();
}

void AnkerEasyItem::OnPressed(wxMouseEvent& event)
{
	if (m_Status != ITEM_SELECT)
	{
		m_Status = ITEM_SELECT;
		Refresh();
	}

 	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_EASYITEM_CLICKED);
 	evt.SetEventObject(this);
 	ProcessEvent(evt);
}

void AnkerEasyItem::OnDClick(wxMouseEvent& event)
{
	if (m_Status != ITEM_SELECT)
	{
		m_Status = ITEM_SELECT;
		Refresh();
	}

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_EASYITEM_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);
}

void AnkerEasyItem::OnEnter(wxMouseEvent& event)
{
	if (m_Status != ITEM_SELECT)
	{
		m_Status = ITEM_ENTER;
		Refresh();
	}
}

void AnkerEasyItem::OnLeave(wxMouseEvent& event)
{
	if (m_Status != ITEM_SELECT)
	{
		m_Status = ITEM_NOR;
		Refresh();
	}
}

void AnkerEasyItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rectArea = GetClientRect();

	int titleHeight = 0;
#ifndef __APPLE__
	titleHeight = 12;
#else
	titleHeight = 16;
#endif // !__APPLE__

	wxPoint logoPoint;
	wxPoint textPoint;
	wxPoint checkPoint;


	dc.SetFont(ANKER_BOLD_FONT_NO_1);
	wxSize textSize = dc.GetTextExtent(m_title);
//#ifdef __APPLE__
//	int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
//	if (wxLanguage::wxLANGUAGE_JAPANESE_JAPAN == type || wxLanguage::wxLANGUAGE_JAPANESE == type) {
//		// text real size is small than the value calculate by dc.GetTextExtent()
//		textSize.SetHeight(textSize.GetHeight() - 6);
//	}
//#endif // !__APPLE__

	logoPoint.x = 10;
	logoPoint.y = (GetRect().GetHeight() - m_selectLogo.GetHeight()) / 2;

	textPoint.x = logoPoint.x + m_selectLogo .GetWidth() + 6;
	textPoint.y = (GetRect().GetHeight() - textSize.GetHeight()) / 2;

	checkPoint.x = GetRect().GetWidth() - m_checkLogo.GetWidth() - 12;
	checkPoint.y = (GetRect().GetHeight() - m_checkLogo.GetHeight()) / 2;


	switch (m_Status)
	{
	case ITEM_NOR:
	{
		dc.Clear();
	
		dc.SetBrush(wxBrush(wxColour(61, 62, 65)));
		dc.SetPen(wxColour(61, 62, 65));
		dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 4);

		dc.SetBrush(wxBrush(wxColour(41, 42, 45)));
		dc.SetPen(wxColour(41, 42, 45));
		dc.DrawRoundedRectangle(1, 1, GetSize().x - 2, GetSize().y - 2, 4);

		dc.DrawBitmap(m_logo, logoPoint, true);
		wxSize norTitleSize;
	
		dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
		dc.SetPen(wxColour(255, 255, 255));
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		dc.SetTextForeground(wxColour(255, 255, 255));
		norTitleSize = dc.GetTextExtent(m_title);
		dc.DrawText(m_title, textPoint);
	
	}
		break;
	case ITEM_ENTER:	
	{
		dc.Clear();
		dc.SetBrush(wxBrush(wxColour(50, 68, 53)));
		dc.SetPen(wxColour(50, 68, 53));
		dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 4);

		dc.DrawBitmap(m_logo, logoPoint, true);

		wxSize enterTitleSize;
		dc.SetBrush(wxBrush(wxColour(98, 211, 97)));
		dc.SetPen(wxColour(98, 211, 97));
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		dc.SetTextForeground(wxColour(98, 211, 97));
		enterTitleSize = dc.GetTextExtent(m_title);
		dc.DrawText(m_title, textPoint);
	}

		break;
	case ITEM_SELECT:
	{
		dc.Clear();

		dc.SetBrush(wxBrush(wxColour(50, 68, 53)));
		dc.SetPen(wxColour(50, 68, 53));
		dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 4);

		dc.DrawBitmap(m_selectLogo, logoPoint, true);

		wxSize selectTitleSize;

		dc.SetBrush(wxBrush(wxColour(98, 211, 97)));
		dc.SetPen(wxColour(98, 211, 97));
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		dc.SetTextForeground(wxColour(98, 211, 97));
		selectTitleSize = dc.GetTextExtent(m_title);
		dc.DrawText(m_title, textPoint);

		dc.DrawBitmap(m_checkLogo, checkPoint, true);
	}
		break;
	default:
		break;
	}

}

AnkerEasyPanel::AnkerEasyPanel(wxWindow* parent,
								wxWindowID winid /*= wxID_ANY*/,
								const wxPoint& pos /*= wxDefaultPosition*/,
								const wxSize& size /*= wxDefaultSize*/)
								: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	initUi();
}

AnkerEasyPanel::~AnkerEasyPanel()
{

}

void AnkerEasyPanel::initUi()
{
	SetBackgroundColour(wxColour("#292A2D"));
	//SetBackgroundColour(wxColour("#0000FF"));

	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	m_pItemVSizer = new wxBoxSizer(wxVERTICAL);	
	wxBoxSizer* m_pItemHBoderSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pItemHBoderSizer->Add(m_pItemVSizer, wxHORIZONTAL | wxEXPAND, wxALL | wxEXPAND, 0);
	
	m_pTipsLabel = new wxStaticText(this, wxID_ANY, "");
	//m_pTipsLabel->SetMinSize(AnkerSize(WIDGET_WIDTH, 60));
	m_pTipsLabel->SetForegroundColour(wxColour("#FFFFFF"));

	m_fastStrTips = _L("common_slicepannel_easy3_description");	
	m_normalStrTips = _L("common_slicepannel_easy2_description");	
	m_PrecisionStrTips = _L("common_slicepannel_easy1_description");	

	m_pTipsLabel->SetFont(ANKER_FONT_NO_1);
	m_pTipsLabel->SetLabelText(m_normalStrTips);
	m_pTipsLabel->SetForegroundColour(wxColour("#A9AAAB"));	
	//m_pTipsLabel->SetBackgroundColour(wxColour(255,0,0));
	wxSize textSize = AnkerSize(WIDGET_WIDTH, 60);
	wxString tipsLabel = Slic3r::GUI::WrapEveryCharacter(_L("common_print_statusnotice_noprinter"), ANKER_FONT_NO_1, WIDGET_WRAP_WITH);
	m_pTipsLabel->Wrap(WIDGET_WRAP_WITH);
	m_pTipsLabel->SetLabelText(tipsLabel);

	wxBoxSizer* pHSizer = new wxBoxSizer(wxHORIZONTAL);
	pHSizer->Add(m_pTipsLabel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

	pMainVSizer->AddSpacer(10);
	pMainVSizer->Add(m_pItemHBoderSizer, 0, wxALL | wxEXPAND, 0);
	pMainVSizer->AddSpacer(8);
	pMainVSizer->Add(pHSizer, 0, wxALL | wxEXPAND, 0);
	pMainVSizer->AddStretchSpacer(1);

	SetSizer(pMainVSizer);
}


void AnkerEasyPanel::showWidget(const std::vector<std::string>& widgetList, const wxString& selectWidgetName)
{
	if (widgetList.size() <= 0)
		return;

	auto presetIter = widgetList.begin();
	std::vector<wxString> vaildList;

	//get vaild data
	while (presetIter != widgetList.end())
	{
		wxString presetName = (*presetIter);
		if (presetName.Contains("Normal"))
			vaildList.push_back("Normal");

		else if(presetName.Contains("Fast"))
			vaildList.push_back("Fast");

		else if(presetName.Contains("Precision"))
			vaildList.push_back("Precision");
		++presetIter;
	}

	//hide all item
	auto hideIter = m_itemVector.begin();
	while (hideIter != m_itemVector.end())
	{
		(*hideIter)->setSelectStatus(ITEM_NOR);
		(*hideIter)->Hide();	
		++hideIter;
	}

	//show vaild item
	auto vaildIter = vaildList.begin();
	bool isSelect = false;
	while (vaildIter != vaildList.end())
	{
		auto iter = m_ItemMap.find((*vaildIter));
		if (iter != m_ItemMap.end())
		{
			if (selectWidgetName.Contains((*vaildIter)))
			{
				iter->second->setSelectStatus(ITEM_SELECT);
				wxString currentLabel = "";
				if (iter->second->getTitle() == _L("common_slicepannel_easy3_fast"))
				{				
					currentLabel = m_fastStrTips;
				}
				else if (iter->second->getTitle() == _L("common_slicepannel_easy2_normal"))
				{				
					currentLabel = m_normalStrTips;
				}
				else
				{					
					currentLabel = m_PrecisionStrTips;
				}
				isSelect = true;
				wxSize textSize = AnkerSize(WIDGET_WIDTH, 60);
				m_pTipsLabel->SetSize(AnkerSize(WIDGET_WIDTH, 60));

				wxString realLabel = Slic3r::GUI::WrapEveryCharacter(currentLabel, ANKER_FONT_NO_1, WIDGET_WRAP_WITH);				
				m_pTipsLabel->SetLabelText(realLabel);
				m_pTipsLabel->Wrap(WIDGET_WRAP_WITH);
			}
			else
				iter->second->setSelectStatus(ITEM_NOR);
			iter->second->Show();
		}
		++vaildIter;
	}

	SetSize(AnkerSize(WIDGET_WIDTH, 60));
	Update();
	Layout();
}

void AnkerEasyPanel::setCurrentWidget(AnkerEasyItem* pWidget)
{
	this->Freeze();
	auto iter = m_itemVector.begin();
	auto strTitle = pWidget->getTitle();

	wxString currentLabel = "";
	if (strTitle == _L("common_slicepannel_easy3_fast"))
	{	
		currentLabel = m_fastStrTips;
	}
	else if (strTitle == _L("common_slicepannel_easy2_normal"))
	{	
		currentLabel = m_normalStrTips;
	}
	else
	{		
		currentLabel = m_PrecisionStrTips;
	}
	wxString realLabel = Slic3r::GUI::WrapEveryCharacter(currentLabel, ANKER_FONT_NO_1, WIDGET_WRAP_WITH);
	m_pTipsLabel->SetLabelText(realLabel);
	m_pTipsLabel->Wrap(WIDGET_WRAP_WITH);
	while (iter != m_itemVector.end())
	{
		if (pWidget == (*iter))
			(*iter)->setSelectStatus(ITEM_SELECT);
		else
			(*iter)->setSelectStatus(ITEM_NOR);

		++iter;
	}

	Refresh();
	Layout();
	this->Thaw();
}


void AnkerEasyPanel::setCurrentWidget(const int& index)
{
	int vectorIndex = 0;

	auto iter = m_itemVector.begin();

	while (iter != m_itemVector.end())
	{
		if (vectorIndex == index)
			(*iter)->setSelectStatus(ITEM_SELECT);
		else
			(*iter)->setSelectStatus(ITEM_NOR);

		++vectorIndex;
		++iter;
	}

	Update();
	Layout();
}

void AnkerEasyPanel::createrItem(wxString title,
								wxImage logo, 
								wxImage selectlogo,
								wxImage checkLogo)
{
	AnkerEasyItem* pWidget = new AnkerEasyItem(this,
												title, 
												logo,
												selectlogo,												
												checkLogo,
												wxID_ANY);	
	pWidget->SetMinSize(AnkerSize(WIDGET_WIDTH, 40));
	pWidget->SetSize(AnkerSize(WIDGET_WIDTH, 40));
	wxCursor handCursor(wxCURSOR_HAND);

	pWidget->SetCursor(handCursor);
	pWidget->Bind(wxCUSTOMEVT_ANKER_EASYITEM_CLICKED, [this](wxCommandEvent& event) {
		AnkerEasyItem* pWidget = dynamic_cast<AnkerEasyItem*>(event.GetEventObject());
		if (pWidget)
		{
			setCurrentWidget(pWidget);
		}
		wxString title = pWidget->getTitle();
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_EASY_ITEM_CLICKED);
		evt.SetClientObject(new wxStringClientData(title));
		evt.SetEventObject(this);
		ProcessEvent(evt);
		});

	m_pItemVSizer->Add(pWidget, 0, wxALL | wxEXPAND, 0);
	
	m_pItemVSizer->AddSpacer(8);

	m_itemVector.push_back(pWidget);
	
	if(title == _L("common_slicepannel_easy2_normal"))
		m_ItemMap.insert(std::make_pair("Normal", pWidget));
	else if (title == _L("common_slicepannel_easy1_smooth"))
		m_ItemMap.insert(std::make_pair("Precision", pWidget));
	else if (title == _L("common_slicepannel_easy3_fast"))
		m_ItemMap.insert(std::make_pair("Fast", pWidget));

	Update();
	Layout();
}

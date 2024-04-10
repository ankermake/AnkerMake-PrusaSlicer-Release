#include "AnkerPopupWidget.hpp"
#include "AnkerGUIConfig.hpp"

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_ITEM_CLICKED, wxCommandEvent);

AnkerPopupWidget::AnkerPopupWidget(wxWindow* parent)
								  :wxPopupWindow(parent, wxBORDER_NONE)
{
	initUi();	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

AnkerPopupWidget::~AnkerPopupWidget()
{

}

void AnkerPopupWidget::AddItem(wxString tab, wxString optionKey, wxString optionLabel, wxString optionGroup)
{
	if (m_pScrolledVSizer)
	{
		AnkerSearchItem* pItem = new AnkerSearchItem(m_scrolledWindow, tab, optionKey, optionLabel, optionGroup, ANKER_FONT_NO_1, wxID_ANY);
		pItem->Bind(wxCUSTOMEVT_ANKER_ITEM_CLICKED, [this, tab, optionKey, optionLabel](wxCommandEvent&event) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_ITEM_CLICKED);
			wxVariant eventData;
			eventData.ClearList();
			eventData.Append(wxVariant(tab));
			eventData.Append(wxVariant(optionKey));
			eventData.Append(wxVariant(optionLabel));
			evt.SetClientData(new wxVariant(eventData));

			ProcessEvent(evt);
			});
		m_itemMap.insert(std::make_pair(optionKey, pItem));
		pItem->SetMaxSize(wxSize(-1, 30));		
		pItem->SetMinSize(wxSize(-1, 30));		
		m_pScrolledVSizer->Add(pItem, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
		Layout();
	}
}


//itemData: tab--{optionkey, [optionLabel, group]}
void AnkerPopupWidget::AddItem(const std::map<wxString, std::map<wxString, std::vector<wxString> >>& itemData)
{
	if (m_pScrolledVSizer)
	{
		auto iter = itemData.begin();
		while (iter != itemData.end())
		{
			auto iterEx = iter->second.begin();	

			while (iterEx != iter->second.end())
			{
				if ((*iterEx).second.size() != 2)
					continue;

				wxString tab = iter->first;                  // tab
				wxString optionKey = (*iterEx).first;        // option key
				wxString optionLabel = (*iterEx).second[0];  // option label
				wxString optionGroup = (*iterEx).second[1];  // option group

				AnkerSearchItem* pItem = new AnkerSearchItem(m_scrolledWindow, tab, optionKey, optionLabel, optionGroup, ANKER_FONT_NO_1, wxID_ANY);
				pItem->Bind(wxCUSTOMEVT_ANKER_ITEM_CLICKED, [this, tab, optionKey, optionLabel](wxCommandEvent& event) {
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_ITEM_CLICKED);
					wxVariant eventData;
					eventData.ClearList();
					eventData.Append(wxVariant(tab));
					eventData.Append(wxVariant(optionKey));
					eventData.Append(wxVariant(optionLabel));
					evt.SetClientData(new wxVariant(eventData));

					ProcessEvent(evt);
					});
				m_itemMap.insert(std::make_pair(optionKey, pItem));
				pItem->SetMaxSize(wxSize(-1, 30));
				pItem->SetMinSize(wxSize(-1, 30));
				m_pScrolledVSizer->Add(pItem, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);
				++iterEx;
			}
			
			++iter;
		}

		Layout();
	}
}


//searchMap tab--{optionkey, optionLabel}
void AnkerPopupWidget::showResMap(const std::map<wxString, std::map<wxString, wxString>>& searchMap)
{
	if (!m_pScrolledVSizer)	
		return;

	auto hideItem = m_itemMap.begin();
	while (hideItem != m_itemMap.end())
	{
		hideItem->second->Hide();
		++hideItem;
	}

	Update();
	Layout();

	if (searchMap.size() <= 0)
	{
		return;
	}

	//tab-labelList	
	auto searchIter = searchMap.begin();
	while (searchIter != searchMap.end())
	{
		wxString tab = searchIter->first;

		auto optionListIter = searchIter->second.begin();
		//labelList
		while (optionListIter != searchIter->second.end())
		{
			wxString optionKey = (*optionListIter).first;
			wxString labelValue = (*optionListIter).second;

			//find item from label-Item map				

			auto item = m_itemMap.find(optionKey);
			if (item != m_itemMap.end())
			{					
				item->second->Show();					
			}
			else
			{
				item->second->Hide();
			}

			++optionListIter;
		}

		++searchIter;
	}
	
	Refresh();
	Layout();
}


void AnkerPopupWidget::showAllItem()
{
	auto showItem = m_itemMap.begin();
	while (showItem != m_itemMap.end())
	{
		showItem->second->Show();
		++showItem;
	}
	Update();
	Layout();

}

void AnkerPopupWidget::initUi()
{
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);
	m_scrolledWindow = new wxScrolledWindow(this,wxID_ANY);
	m_pScrolledVSizer = new wxBoxSizer(wxVERTICAL);

	m_scrolledWindow->SetSizer(m_pScrolledVSizer);		
	m_scrolledWindow->SetBackgroundColour(wxColour("#292d2a"));
	m_scrolledWindow->SetScrollbars(0, 20, 276, 368, 0, 0);	
	
	pMainVSizer->Add(m_scrolledWindow, wxEXPAND | wxALL, wxEXPAND | wxALL, 0);

	SetSizer(pMainVSizer);
}


AnkerSearchItem::AnkerSearchItem()
{

}

BEGIN_EVENT_TABLE(AnkerSearchItem, wxControl)
EVT_PAINT(AnkerSearchItem::OnPaint)
EVT_ENTER_WINDOW(AnkerSearchItem::OnEnter)
EVT_LEAVE_WINDOW(AnkerSearchItem::OnLeave)
EVT_LEFT_DOWN(AnkerSearchItem::OnPressed)
EVT_LEFT_DCLICK(AnkerSearchItem::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerSearchItem, wxControl)

AnkerSearchItem::AnkerSearchItem(wxWindow* parent,
								 wxString  tab,
								 wxString  key,
								 wxString text,
								 wxString group,
								 wxFont font,
								 wxWindowID winid /*= wxID_ANY*/, 
								 const wxPoint& pos /*= wxDefaultPosition*/,
								 const wxSize& size /*= wxDefaultSize*/)
								 : m_tab(tab)
								 , m_key(key)
								 , m_text(text)
								 , m_group(group)
								 , m_font(font)
								 , m_textColor(wxColour("#FFFFFF"))
								 , m_bgColor(wxColour("#3A3B3F"))
								 , m_hoverColor(wxColour("#4D4E52"))
								 , m_status(SEARCH_ITEM_STATUS::SEARCH_ITEM_NOR)
{
	wxControl::Create(parent, winid, pos, size, wxBORDER_NONE, wxDefaultValidator);
	
}

AnkerSearchItem::~AnkerSearchItem()
{

}

void AnkerSearchItem::OnPressed(wxMouseEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_ITEM_CLICKED);
	ProcessEvent(evt);
	event.Skip();
}

void AnkerSearchItem::OnDClick(wxMouseEvent& event)
{
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_ITEM_CLICKED);
	ProcessEvent(evt);
	event.Skip();
}

void AnkerSearchItem::OnEnter(wxMouseEvent& event)
{
	m_status = SEARCH_ITEM_HOVER;
	Update();
	event.Skip();
}

void AnkerSearchItem::OnLeave(wxMouseEvent& event)
{
	m_status = SEARCH_ITEM_NOR;
	Update();
	event.Skip();
}

void AnkerSearchItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxBrush brush(wxTRANSPARENT);
	wxPen pen(wxTRANSPARENT);
	wxRect rect = GetClientRect();

	if(m_status == SEARCH_ITEM_NOR)
	{
	dc.SetBrush(m_bgColor);
	dc.SetPen(m_bgColor);
	}
	else if (m_status == SEARCH_ITEM_HOVER)
	{
		dc.SetBrush(m_hoverColor);
		dc.SetPen(m_hoverColor);
	}

	dc.DrawRectangle(rect);

	dc.SetFont(m_font);
	dc.SetTextForeground(m_textColor);
	if (m_group.empty())
		dc.DrawText(m_text,wxPoint(12,8));
	else
		dc.DrawText(m_group + " : " + m_text, wxPoint(12, 8));

}

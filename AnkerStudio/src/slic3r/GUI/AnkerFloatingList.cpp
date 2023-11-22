#include "AnkerFloatingList.hpp"

#include "Common/AnkerGUIConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI_App.hpp"
#include "AnkerObjectBar.hpp"


AnkerFloatingList::AnkerFloatingList(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW)
    , m_currentIndex(-1)
    , m_pListVSizer(nullptr)
	, m_pClickCallback(nullptr)
{
    initUI();

	Bind(wxEVT_KILL_FOCUS, &AnkerFloatingList::OnKillFocus, this);
	Bind(wxEVT_SHOW, &AnkerFloatingList::OnShow, this);
}

AnkerFloatingList::~AnkerFloatingList()
{
}

void AnkerFloatingList::setContentList(std::vector<std::pair<wxColour, wxString>> content)
{
	if (content.size() > 0)
	{
		clear();
		for (int i = 0; i < content.size(); i++)
		{
			AnkerFloatingListItem* item = new AnkerFloatingListItem(i + 1, this);
			item->setIndexColor(content[i].first);
			item->setText(content[i].second);
			item->Bind(wxEVT_LEFT_UP, &AnkerFloatingList::OnItemClicked, this);

			m_pListVSizer->Add(item, 1, wxEXPAND | wxCENTER, 0);
			m_pItemList.push_back(item);
		}

		m_contentList = content;

		Layout();
		Refresh();
	}
}

std::pair<wxColour, wxString> AnkerFloatingList::getItemContent(int index)
{
	if (index < 0 || index >= m_contentList.size())
		return { wxColour(), "" };
	else
		return m_contentList[index];
}

void AnkerFloatingList::clear()
{
	if (m_pListVSizer)
	{
		m_pListVSizer->Clear();
		m_pListVSizer->AddSpacer(4);
		DestroyChildren();

		m_currentIndex = -1;
		m_pItemList.clear();
		m_contentList.clear();

		Layout();
		Refresh();
	}
}

void AnkerFloatingList::setCurrentSelection(int index)
{
	if (m_currentIndex > -1 && m_currentIndex < m_pItemList.size())
		m_pItemList[m_currentIndex]->setSelected(false);

	m_currentIndex = index;


	if (m_currentIndex > -1 && m_currentIndex < m_pItemList.size())
		m_pItemList[m_currentIndex]->setSelected(true);
}

void AnkerFloatingList::initUI()
{
    SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
	SetMinSize(wxSize(202, 260));
	SetMaxSize(wxSize(202, 260));
    SetSize(wxSize(202, 260));

    m_pListVSizer = new wxBoxSizer(wxVERTICAL);

	m_pListVSizer->AddSpacer(4);

    SetSizer(m_pListVSizer);
	Show(false);
	Layout();
}

bool AnkerFloatingList::Show(bool show)
{
	m_visible = show;

	return wxFrame::Show(show);
}

bool AnkerFloatingList::Hide()
{
	m_visible = false;
	return wxFrame::Hide();
}

void AnkerFloatingList::OnShow(wxShowEvent& event)
{
	// event.Skip();
}

void AnkerFloatingList::OnItemClicked(wxMouseEvent& event)
{
	AnkerFloatingListItem* item = dynamic_cast<AnkerFloatingListItem*>(event.GetEventObject());

	if (item == nullptr)
		return;

	setCurrentSelection(item->getIndex() - 1);

	if (m_pClickCallback)
	{
		m_pClickCallback(m_currentIndex);
	}
}

void AnkerFloatingList::OnKillFocus(wxFocusEvent& event)
{
	wxWindow* newFocusWindow = event.GetWindow();
	if (newFocusWindow && newFocusWindow->GetParent() == this)
		return;

	Hide();

	//if (m_pClickCallback)
	//{
	//	m_pClickCallback(m_currentIndex);
	//}
}


// AnkerFloatingListItem
AnkerFloatingListItem::AnkerFloatingListItem(int index, wxWindow* parent)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_selected(false)
    , m_itemIndex(index)
    , m_itemText("")
	, m_itemShowText("")
    , m_normalColour(PANEL_BACK_RGB_INT)
    , m_hoverColour(64, 82, 73)
    , m_selectedColour(/*64, 82, 73*/PANEL_BACK_RGB_INT)
{
    
	m_selectedBitmap = wxImage(wxString::FromUTF8(Slic3r::var("appbar_sure_icon.png")), wxBITMAP_TYPE_PNG);
	m_selectedBitmap.Rescale(16, 16);

    initUI();

	Bind(wxEVT_PAINT, &AnkerFloatingListItem::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &AnkerFloatingListItem::OnMouseEnterWindow, this);
	Bind(wxEVT_LEAVE_WINDOW, &AnkerFloatingListItem::OnMouseLeaveWindow, this);
}

AnkerFloatingListItem::~AnkerFloatingListItem()
{
}

void AnkerFloatingListItem::setIndexColor(wxColour color)
{
	m_indexColor = color;
}

void AnkerFloatingListItem::setText(wxString text)
{
	m_itemText = text;
	m_itemShowText = text;

	if (m_itemShowText.size() > 11)
		m_itemShowText = m_itemShowText.substr(0, 6) + "..." + m_itemShowText.substr(m_itemShowText.size() - 1 - 4, 5);
}

void AnkerFloatingListItem::setSelected(bool selected)
{
	m_selected = selected;

	SetBackgroundColour(m_selected ? m_selectedColour : m_normalColour);
	Refresh();
}

void AnkerFloatingListItem::initUI()
{
    SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    SetMinSize(wxSize(202, 42));
    SetMaxSize(wxSize(202, 42));
	SetSize(wxSize(202, 42));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerFloatingListItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxColour bgColor = GetBackgroundColour();

	wxRect rect = GetClientRect();
	wxBrush brush(bgColor);
	wxPen pen(wxColour(41, 42, 45));
	dc.SetBrush(brush);
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

	// draw Index Image
	{
		wxRect rect(12, 8, 26, 26);
		wxBrush brush(m_indexColor);
		wxPen pen(wxColour(41, 42, 45));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.DrawRoundedRectangle(rect.GetPosition(), rect.GetSize(), 3);
	}

	// draw index
	{
		wxColour foreColor = wxColour(255, 255, 255);
		if (m_indexColor.GetLuminance() > 0.6)
			foreColor = wxColour(0, 0, 0);

		wxBrush brush(bgColor);
		wxPen pen(foreColor);
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_BOLD_FONT_NO_2);
		dc.SetTextForeground(foreColor);
#ifdef __APPLE__
        wxPoint textPoint = wxPoint(20, 14);
#else
        wxPoint textPoint = wxPoint(22, 13);
#endif
		dc.DrawText(std::to_string(m_itemIndex), textPoint);
	}

	// draw text
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(184, 184, 186));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_FONT_NO_1);
		dc.SetTextForeground(wxColour(255, 255, 255));
		wxPoint textPoint = wxPoint(48, 13);
		dc.DrawText(m_itemShowText, textPoint);
	}

	// draw selected image
	if (m_selected)
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(184, 184, 186));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextForeground(wxColour(184, 184, 186));
		wxPoint textPoint = wxPoint(174, 13);
		dc.DrawBitmap(m_selectedBitmap, textPoint);
	}
}

void AnkerFloatingListItem::OnMouseEnterWindow(wxMouseEvent& event)
{
	SetBackgroundColour(m_hoverColour);
	Refresh();
}

void AnkerFloatingListItem::OnMouseLeaveWindow(wxMouseEvent& event)
{
	SetBackgroundColour(m_selected ? m_selectedColour : m_normalColour);
	Refresh();
}

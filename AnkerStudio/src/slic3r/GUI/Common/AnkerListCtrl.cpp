#include "AnkerListCtrl.hpp"
#include <wx/dcclient.h>
#include <wx/ctrlsub.h>
#include <wx/event.h>
#include <wx/uri.h>

AnkerListCtrl::AnkerListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
    wxListCtrl(parent, id, pos, size, style)
{
   
}

AnkerListCtrl::~AnkerListCtrl()
{
}

AnkerCopyrightListCtrl::AnkerCopyrightListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style) : AnkerListCtrl(parent, id, pos, size, style)
{
    Bind(wxEVT_PAINT, &AnkerCopyrightListCtrl::OnPaint, this);
    //Bind(wxEVT_MOTION, &AnkerCopyrightListCtrl::OnMouseMove, this);
    Bind(wxEVT_MOUSEWHEEL, &AnkerCopyrightListCtrl::OnMouseWheel, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerCopyrightListCtrl::OnMouseClick, this);
    Bind(wxEVT_SCROLLWIN_THUMBTRACK, &AnkerCopyrightListCtrl::OnScrollThumbTrack, this);
    //Bind(wxEVT_LIST_ITEM_SELECTED, &AnkerCopyrightListCtrl::OnItemClick, this);
}

void AnkerCopyrightListCtrl::initLinks(int totalRow, int totalColumn)
{
    m_links = std::vector<std::vector<std::pair<std::string, std::string>>>(totalRow, std::vector<std::pair<std::string, std::string>>(totalColumn));
}

void AnkerCopyrightListCtrl::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    PrepareDC(dc);

    // Clear the background
    dc.SetTextForeground(wxColour("#FFFFFF"));
    dc.SetBackground(GetParent()->GetBackgroundColour());
    dc.Clear();

    dc.SetFont(GetFont());
    int lineHeight = dc.GetCharHeight() + 10;
    m_lineHeight = lineHeight;
    int topRow = GetTopItem();
    int visibleRowCount = GetCountPerPage();
    int leftCol, rightCol;
    wxRect rect = GetViewRect();
    leftCol = rect.GetLeft();
    rightCol = rect.GetRight();
    int visibleColCount = rightCol - leftCol + 1;

    int rowCount = GetItemCount();
    int colCount = GetColumnCount();
    if (rowCount <= 0 || colCount <= 0) {
        return;
    }
    if (topRow > 10) {
        topRow += 3;
    }
    int columnWidth = GetClientSize().GetWidth() / colCount;

    for (int row = topRow; row <= topRow + visibleRowCount && row < rowCount; row++)
    {
        for (int col = leftCol; col < leftCol + visibleColCount && col < colCount; col++)
        {
            int x = (col - leftCol) * columnWidth;
            int y = (row - topRow) * lineHeight;
            wxString label = GetItemText(row, col);
            wxFont font = ANKER_FONT_NO_1;
            if (row > 0 && (col == 0 || col == 1)) {
                font = ANKER_BOLD_FONT_NO_1;
                dc.SetTextForeground(wxColour("#61D37D"));
            }
            else {
                font = ANKER_FONT_NO_1;
                dc.SetTextForeground(wxColour("#FFFFFF"));
            }
            dc.SetFont(m_font);
            dc.DrawText(label, x + 5, y + 5);

        }
    }
   // event.Skip();
}

void AnkerCopyrightListCtrl::OnMouseMove(wxMouseEvent& event)
{
    
}

void AnkerCopyrightListCtrl::OnMouseWheel(wxMouseEvent& event)
{
    Update();
    Refresh();
    event.Skip();
}

void AnkerCopyrightListCtrl::OnMouseClick(wxMouseEvent& event)
{
    wxPoint point = event.GetPosition();
    int row = -1;
    int column = -1;
    getRow(point, row);
    getColumn(point, column);

    if ((row > 0 && row < GetItemCount()) && (column == 0 || column == 1)) {
        wxString url = wxString(m_links[row][column].second);
        if (!url.empty()) {
            wxURI uri(url);
            url = uri.BuildURI();
            wxLaunchDefaultBrowser(url);
        }
    }
    Update();
    Refresh();
    event.Skip();
}

void AnkerCopyrightListCtrl::OnItemClick(wxListEvent& event)
{
    int index = event.GetIndex();
    int selection = event.GetSelection();
    wxPoint point = event.GetPoint();
    event.Skip();
}

void AnkerCopyrightListCtrl::OnColumnClick(wxListEvent& event)
{
    int index = event.GetIndex();
    wxPoint point = event.GetPoint();
    event.Skip();
}

void AnkerCopyrightListCtrl::getColumn(const wxPoint& point, int& column)
{
    wxSize size = GetSize();
    if ((point.x > 0) && (point.x < size.x / 3)) {
        column = 0;
    }
    else if ((point.x > size.x / 3) && (point.x < size.x / 3 * 2)) {
        column = 1;
    }
    else if ((point.x > size.x / 3 * 2) && (point.x < size.x)) {
        column = 2;
    }
    else {
        column = -1;
    }
}

void AnkerCopyrightListCtrl::getRow(const wxPoint& point, int& itemIndex)
{
#ifdef __APPLE__
    int flags;
    itemIndex = HitTest(point, flags);
#elif _WIN32
    int lineHeight = m_lineHeight;
    int rowCount = GetItemCount();
    int topRow = GetTopItem();

    int visibleRowCount = GetCountPerPage();
    if (topRow > 10) {
        topRow += 3;
    }
    for (int row = topRow; row <= topRow + visibleRowCount && row < rowCount; row++)
    {
        int y = (row - topRow) * lineHeight;
        if ((point.y > y) && (point.y < y + lineHeight)) {
            itemIndex = row;
            break;
        }
    }
#endif // __APPLE__
}

void AnkerCopyrightListCtrl::setLink(int row, int column, const std::string& label, const std::string& link)
{
    if (row >= m_links.size() || row < 0) {
        return;
    }
    if (column >= m_links[0].size() || column < 0) {
        return;
    }

    m_links[row][column] = std::pair<std::string, std::string>(label, link);
}


void AnkerCopyrightListCtrl::OnScrollThumbTrack(wxScrollWinEvent& event)
{
    Update();
    Refresh();
    event.Skip();
}

void AnkerCopyrightListCtrl::drawItems(wxDC& dc)
{
    wxColour bkColour = GetParent()->GetBackgroundColour();
    wxRect rect = GetClientRect();
    int itemHeight = GetCharHeight() + 4;  // Adjust the item height according to your needs

    int itemCount = GetItemCount();
    int columnCount = GetColumnCount();

    int y = rect.y;

    for (int i = 0; i < itemCount; i++)
    {
        int x = rect.x;

        for (int j = 0; j < columnCount; j++)
        {
            wxListItem item;
            item.SetId(i);
            item.SetColumn(j);
            item.SetMask(wxLIST_MASK_TEXT);
            GetItem(item);

            wxRect cellRect(x, y, GetColumnWidth(j), itemHeight);

            // Set the background color of the cell
            dc.SetBrush(wxBrush(bkColour));  // Set your desired cell background color here
            dc.DrawRectangle(cellRect);

            // Set the text color of the cell
            dc.SetTextForeground(wxColour("#FFFFFF"));  // Set your desired text color here
            dc.DrawText(item.GetText(), cellRect.x + 2, cellRect.y + 2);

            x += GetColumnWidth(j);
        }

        y += itemHeight;
    }
}

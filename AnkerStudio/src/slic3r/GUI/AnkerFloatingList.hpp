#ifndef _ANKER_FLOATING_LIST_H_
#define _ANKER_FLOATING_LIST_H_

#include "wx/wx.h"

#include <functional>


class AnkerFloatingListItem : public wxControl
{
public:
    AnkerFloatingListItem(int index, wxWindow* parent);
    ~AnkerFloatingListItem();

    void setIndexColor(wxColour color);
    void setText(wxString text);

    int getIndex() { return m_itemIndex; }

    void setSelected(bool selected);

private:
    void initUI();

    void OnPaint(wxPaintEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeaveWindow(wxMouseEvent& event);


private:
    bool m_selected;

    wxColour m_normalColour;
    wxColour m_hoverColour;
    wxColour m_selectedColour;

    int m_itemIndex;
    wxString m_itemText;
    wxString m_itemShowText;
    wxColour m_indexColor;
    wxImage m_selectedBitmap;
};

class AnkerFloatingList : public wxFrame
{
public:
    AnkerFloatingList(wxWindow* parent);
    ~AnkerFloatingList();

    void setContentList(std::vector<std::pair<wxColour, wxString>> content);
    std::pair<wxColour, wxString> getItemContent(int index);
    void clear();

    void setItemClickCallback(std::function<void(int)> callback) { m_pClickCallback = callback; }

    void setCurrentSelection(int index);
    int getCurrentSelectionIndex() { return m_currentIndex; }
    int length() { return m_contentList.size(); }

    bool Show(bool show = true);
    bool 	Hide();

private:
    void initUI();

    void OnShow(wxShowEvent& event);
    void OnItemClicked(wxMouseEvent& event);
    void OnKillFocus(wxFocusEvent& event);

private:
    bool m_visible;
    int m_currentIndex;
    wxBoxSizer* m_pListVSizer;

    std::vector<std::pair<wxColour, wxString>> m_contentList;
    std::vector<AnkerFloatingListItem*> m_pItemList;

    std::function<void(int)> m_pClickCallback;
};

#endif // _ANKER_FLOATING_LIST_H_


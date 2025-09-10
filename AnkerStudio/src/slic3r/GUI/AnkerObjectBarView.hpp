#ifndef _ANKER_OBJECT_BAR_VIEW_H_
#define _ANKER_OBJECT_BAR_VIEW_H_

#include "wx/wx.h"


namespace Slic3r
{
    namespace GUI
    {
        class Plater;
        class ObjectList;
    }
    class ObjectBase;
}
class ScalableButton;
class AnkerObjectItem;

wxDECLARE_EVENT(wxANKEREVT_AOI_COLLAPSE_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOI_SETTINGS_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOI_PRINTABLE_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOI_FILAMENT_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOI_ITEM_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOBV_SETTINGS_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOBV_PRINTABLE_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOBV_FILAMENT_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOBV_ITEM_CLICK, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_AOBV_KEY_EVENT, wxKeyEvent);

class AnkerObjectViewItem : public wxControl
{
public:
    AnkerObjectViewItem(wxWindow* parent);
    ~AnkerObjectViewItem();

    void setObject(AnkerObjectItem* object) { m_object = object; }
    AnkerObjectItem* getObject() { return m_object; }

    //void setName(wxString name, bool refresh = true);
    //wxString getName() { return m_name; }

    //// if item has any childern, it's a group, else if isGroup is true, item is a empty group
    //void setIsGroup(bool group);
    //
    //// if iconPath is empty, item has no icon
    //void setIcon(wxString iconPath, wxBitmapType type = wxBitmapType::wxBITMAP_TYPE_PNG);

    //void set
    //void setPrintable(bool printable, bool refresh = true) { m_printable = printable; if (refresh) Refresh(); }
    //bool getPrintable() { return m_printable; }

    //void setFilament(int filamentIndex, wxColour filamentColour, bool refresh = true) { m_filamentIndex = filamentIndex; m_filamentColour = filamentColour; if (refresh) Refresh(); }
    //int getFilamentIndex() { return m_filamentIndex; }
    //wxColour getFilamentColour() { return m_filamentColour; }
    void setFilamentVisible(bool visible, bool refresh = true);

    void setParentItem(AnkerObjectViewItem* parentItem);
    AnkerObjectViewItem* getParentItem() { return m_pParentItem; }
    bool hasParent() { return m_pParentItem != nullptr; };

    void addChildItem(AnkerObjectViewItem* item, bool refresh = true);
    AnkerObjectViewItem* removeChildItem(AnkerObjectItem* object);
    void clearAll(bool deleted = true, AnkerObjectViewItem* newParentItem = nullptr);
    int getChildItemCount() { return m_childItems.size(); }
    int getFamilyItemCount();
    AnkerObjectViewItem* getChildItemById(AnkerObjectItem* object) { auto itr = m_childItems.find(object); return itr != m_childItems.end() ? itr->second : nullptr; }
    AnkerObjectViewItem* getChildItemByIndex(int index) { return index < m_childIds.size() ? m_childItems[m_childIds[index]] : nullptr; }

    void collapse(bool on);
    bool isCollapsed() { return m_isCollapsed; }

    void select(bool selected);
    bool isSelected() { return m_isSelected; }

    void setBackgroundColour(wxColour bgColor, bool refresh = true) { m_bgColour = bgColor; if (refresh) Refresh(); }
    void setForegroundColour(wxColour fgColor, bool refresh = true) { m_fgColour = fgColor; if (refresh) Refresh(); }
    void setHoverColour(wxColour hoverColor, bool refresh = true) { m_hoverColour = hoverColor; if (refresh) Refresh(); }
    void setSelectedColour(wxColour selectedColor, bool refresh = true) { m_selectedColour = selectedColor; if (refresh) Refresh(); }
    void setFilamenColourBtnRect(wxRect rect, bool refresh = true) { m_filamenColourBtnRect = rect; if (refresh) Refresh(); }
    void setCollapseBtnRect(wxRect rect, bool refresh = true) { m_collapseBtnRect = rect; if (refresh) Refresh(); }


private:
    void initUI();

    void OnPaint(wxPaintEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);

private:
    AnkerObjectViewItem* m_pParentItem;

    bool m_isCollapsed;
    bool m_isSelected;
    bool m_isHover;
    int m_topMargin;
    int m_borderMargin;
    int m_collapseMargin;
    int m_toolMargin;
    int m_childXOffset;
    AnkerObjectItem* m_object;

    //bool m_isGroup;
    //bool m_hasIcon;
    //wxImage m_iconImage;

    int m_drawingNameLenMax;
    //wxString m_drawingName;

    //bool m_hasPrintInfo;
    //bool m_printable;

    bool m_hasFilament;
    //int m_filamentIndex;
    //wxColour m_filamentColour;
    //wxString m_name;

    //bool m_hasSetting;

    wxColour m_bgColour;
    wxColour m_fgColour;
    wxColour m_normalColour;
    wxColour m_hoverColour;
    wxColour m_selectedColour;

    wxSize m_collapseBtnSize;
    wxSize m_iconSize;
    wxSize m_toolBtnSize;
    wxRect m_collapseBtnRect;
    wxRect m_iconRect;
    wxRect m_settingBtnRect;
    wxRect m_printableBtnRect;
    wxRect m_filamenColourBtnRect;
    wxImage m_collapseBtnImage;
    wxImage m_expandBtnImage;
    wxImage m_printableBtnImage;
    wxImage m_unprintableBtnImage;
    wxImage m_settingBtnImage;

    std::vector<AnkerObjectItem*> m_childIds;
    std::map<AnkerObjectItem*, AnkerObjectViewItem*> m_childItems;
};

class AnkerObjectBarView : public wxFrame
{
public:
    AnkerObjectBarView(Slic3r::GUI::Plater* parent);
    ~AnkerObjectBarView();

    //Slic3r::GUI::ObjectList* getObjectList() { return m_pObjectList; }

    void change_top_border_for_mode_sizer(bool increase_border);
    void msw_rescale();
    void sys_color_changed();
    void update_objects_list_extruder_column(size_t extruders_count);
    void update_ui_from_settings();
    void object_list_changed();

    void setListMaxHeight(int maxHeight);
    void updateSize();

    void addObject(AnkerObjectItem* object, AnkerObjectItem* parentObject = nullptr, bool refresh = true);
    void removeObject(AnkerObjectItem*, bool removeChildItem = true, bool refresh = true);
    void clearAll(bool sizeUpdateFlag = true);
    //void updateObjectName(AnkerObjectItem* object);
    //void updateObjectPrintable(AnkerObjectItem* object);
    //void updateObjectFilament(AnkerObjectItem* object);
    void updateObject(AnkerObjectItem* object);
    void updateObjectParent(AnkerObjectItem* object, AnkerObjectItem* parentObject = nullptr);   // parentObject = nullptr means no parent

    void setSelectedObjectSingle(AnkerObjectItem* object);
    void setSelectedObjectMulti(AnkerObjectItem* object, bool selected = true);
    void setSelectedObjectMultiContinuous(AnkerObjectItem* object);
    int getSelectedCount() { return m_selectedItems.size(); }
    AnkerObjectItem* getSelectedObject(int index = 0) { return index < 0 || index >= m_selectedItems.size() ? nullptr : m_selectedItems[index]->getObject(); }
    std::vector<AnkerObjectItem*> getSelectedObjects() 
    {
        std::vector<AnkerObjectItem*> itemVector;
        for (int i = 0; i < m_selectedItems.size(); i++)
        {
            itemVector.push_back(m_selectedItems[i]->getObject());
        }
        return itemVector;
    }
    AnkerObjectItem* getParentObject(AnkerObjectItem* object);

    void selectAll(bool selected = true);

    void collapseObject(AnkerObjectItem* object, bool collapsed = true);

private:
    void initUI();

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnShow(wxShowEvent& event);
    void OnKey(wxKeyEvent& event);
    void OnAddButton(wxCommandEvent& event);
    void OnPullDownButton(wxCommandEvent& event);
    void OnItemClicked(wxCommandEvent& event);
    void OnItemSettingsClicked(wxCommandEvent& event);
    void OnItemPrintableClicked(wxCommandEvent& event);
    void OnItemFilamentClicked(wxCommandEvent& event);

private:
    bool m_emptyFlag;
    int m_listTopMargin;
    int m_listBottomMargin;
    int m_listItemHeight;
    int m_listMaxHeight;

    Slic3r::GUI::Plater* m_pPlater;

    ScalableButton* m_pAddButton;
    ScalableButton* m_pPullDownButton;
    wxControl* m_pSplitControl;
    wxBoxSizer* m_pListWinVSizer;
    wxScrolledWindow* m_pListWindow;

    AnkerObjectViewItem* m_currentSelectedItem;
    std::vector<AnkerObjectViewItem*> m_selectedItems;
    std::vector<AnkerObjectItem*> m_itemList;
    std::map<AnkerObjectItem*, AnkerObjectViewItem*> m_itemMap;
};

#endif //_ANKER_OBJECT_BAR_VIEW_H_

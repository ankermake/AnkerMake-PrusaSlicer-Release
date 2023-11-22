#include "AnkerObjectBarView.hpp"
#include "Plater.hpp"
#include "GUI_App.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "libslic3r/Model.hpp"
#include "wxExtensions.hpp"
#include "AnkerObjectBar.hpp"

wxDEFINE_EVENT(wxANKEREVT_AOI_COLLAPSE_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOI_SETTINGS_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOI_PRINTABLE_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOI_FILAMENT_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOI_ITEM_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOBV_SETTINGS_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOBV_PRINTABLE_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOBV_FILAMENT_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOBV_ITEM_CLICK, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_AOBV_KEY_EVENT, wxKeyEvent);

#ifndef __APPLE__
wxFont OBJECT_LIST_TITLE_FONT(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"));
#else
wxFont OBJECT_LIST_TITLE_FONT(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Square"));
#endif // !__APPLE__

AnkerObjectBarView::AnkerObjectBarView(Slic3r::GUI::Plater* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW)
    , m_emptyFlag(true)
    , m_listItemHeight(30)
    , m_listMaxHeight(600)
    , m_listTopMargin(12)
    , m_pPlater(parent)
    , m_pAddButton(nullptr)
    , m_pPullDownButton(nullptr)
    , m_pListWinVSizer(nullptr)
    , m_pListWindow(nullptr)
    , m_currentSelectedItem(nullptr)
{
    initUI();

    Bind(wxEVT_SIZE, &AnkerObjectBarView::OnSize, this); 
    Bind(wxEVT_PAINT, &AnkerObjectBarView::OnPaint, this); 
    Bind(wxEVT_SHOW, &AnkerObjectBarView::OnShow, this);
    Bind(wxEVT_CHAR, &AnkerObjectBarView::OnKey, this); 
}

AnkerObjectBarView::~AnkerObjectBarView()
{
}

void AnkerObjectBarView::initUI()
{
    SetWindowStyleFlag(GetWindowStyleFlag() | wxFRAME_SHAPED);
    SetExtraStyle(GetExtraStyle() | wxFRAME_EX_METAL);
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    SetMinSize(AnkerSize(290, -1));
    SetMaxSize(AnkerSize(290, 680));
    SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    //wxGraphicsPath path = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
    //// appends a rounded rectangle
    //path.AddRoundedRectangle(0, 0, GetSize().GetWidth(), GetSize().GetHeight(), 8);
    //SetShape(path);

    wxBoxSizer* listVSizer = new wxBoxSizer(wxVERTICAL);
    //listVSizer->SetSizeHints(this);

    wxPanel* titleBar = new wxPanel(this, wxID_ANY);
    titleBar->SetMinSize(AnkerSize(290, 35));
    titleBar->SetMaxSize(AnkerSize(290, 35));
    titleBar->SetSize(AnkerSize(290, 35));
    titleBar->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    listVSizer->Add(titleBar, 0, wxEXPAND | wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

    wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
    titleBar->SetSizer(titleHSizer);

    wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, /*L"Object List"*/_("common_slice_objectlist_title"));
    //titleText->SetMinSize(wxSize(70, 30));
    titleText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    titleText->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
    titleText->SetFont(ANKER_BOLD_FONT_NO_1);
    titleText->Fit();
    titleText->Bind(wxEVT_KEY_DOWN, &AnkerObjectBarView::OnKey, this);
    titleText->SetPreviousHandler(this);
    titleHSizer->Add(titleText, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT, 12);

    titleHSizer->AddStretchSpacer(1);

    m_pAddButton = new ScalableButton(titleBar, wxID_ANY, "add_model", "");
    m_pAddButton->Bind(wxEVT_BUTTON, &AnkerObjectBarView::OnAddButton, this);
    m_pAddButton->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
    m_pAddButton->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });
    m_pAddButton->Fit();
    titleHSizer->Add(m_pAddButton, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT, 8);

    m_pPullDownButton = new ScalableButton(titleBar, wxID_ANY, "pulldown_48x48", "");
    m_pPullDownButton->Bind(wxEVT_BUTTON, &AnkerObjectBarView::OnPullDownButton, this);
    m_pPullDownButton->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_HAND)); });
    m_pPullDownButton->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {SetCursor(wxCursor(wxCURSOR_NONE)); });
    m_pPullDownButton->Fit();
    m_pPullDownButton->Show(false);
    titleHSizer->Add(m_pPullDownButton, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT, 12);

    // split line
    m_pSplitControl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    m_pSplitControl->SetBackgroundColour(wxColour(62, 63, 66));
    m_pSplitControl->SetMaxSize(AnkerSize(290, 1));
    m_pSplitControl->SetMinSize(AnkerSize(290, 1));
    m_pSplitControl->Show(false);
    listVSizer->Add(m_pSplitControl, 0, wxEXPAND | wxALIGN_CENTER | wxBOTTOM, 8);

    m_pListWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_pListWindow->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    m_pListWindow->Show(false);

    m_pListWinVSizer = new wxBoxSizer(wxVERTICAL);
    m_pListWindow->SetSizer(m_pListWinVSizer);

    listVSizer->Add(m_pListWindow, 1, wxEXPAND | wxBOTTOM, 4);

    SetSizer(listVSizer);

    Fit();
    Layout();
}

void AnkerObjectBarView::updateSize()
{
    if (m_itemMap.size() > 0)
    {
        if (m_emptyFlag)
        {
            m_emptyFlag = false;

            m_pPullDownButton->Show();
            m_pSplitControl->Show();
            m_pListWindow->Show();
        }

        int lineCount = 0;
        std::vector<AnkerObjectViewItem*> upperList;
        for (int i = 0; i < m_itemList.size(); i++)
        {
            upperList.push_back(m_itemMap[m_itemList[i]]);

            do
            {
                AnkerObjectViewItem* parentItem = upperList.back();
                upperList.pop_back();
                lineCount++;

                if (!parentItem->isCollapsed())
                {
                    for (int i = parentItem->getChildItemCount() - 1; i >= 0; i--)
                    {
                        upperList.push_back(parentItem->getChildItemByIndex(i));
                    }
                }

            } while (upperList.size() > 0);
        }

        int listWindowHeight = lineCount * m_listItemHeight;
        if (listWindowHeight >= m_listMaxHeight)
        {
            listWindowHeight = m_listMaxHeight;
            m_pListWindow->SetScrollRate(-1, m_listItemHeight);
        }
        else
        {
            m_pListWindow->SetScrollRate(-1, -1);
        }
        m_pListWindow->SetMinSize(AnkerSize(290, listWindowHeight));
        m_pListWindow->SetMaxSize(AnkerSize(290, listWindowHeight));

        Fit();
        Layout();
        Update();
    }
    else if (!m_emptyFlag)
    {
        m_emptyFlag = true;

        m_pPullDownButton->Hide();
        m_pSplitControl->Hide();
        m_pListWindow->Hide();

        Fit();
        Layout();
        Update();
    }
}

void AnkerObjectBarView::change_top_border_for_mode_sizer(bool increase_border)
{
    m_pPullDownButton->Show(true);
    Layout();
}

void AnkerObjectBarView::msw_rescale()
{
    //m_pObjectList->msw_rescale();

    Fit();
    Layout();
}

void AnkerObjectBarView::sys_color_changed()
{
    //m_pObjectList->sys_color_changed();
    Layout();
    Refresh();
}

void AnkerObjectBarView::update_objects_list_extruder_column(size_t extruders_count)
{
    //m_pObjectList->update_objects_list_extruder_column(extruders_count);

    bool filamentVisible = false;
    if (extruders_count > 1)
        filamentVisible = true;

    Freeze();

    const std::vector<Slic3r::GUI::SFilamentInfo>& filamentInfos = Slic3r::GUI::wxGetApp().plater()->sidebarnew().getEditFilamentList();
    for (auto itr = m_itemMap.begin(); itr != m_itemMap.end(); itr++)
    {
        itr->second->setFilamentVisible(filamentVisible);
        //itr->second->setFilament(itr->second->getFilamentIndex(), wxColour(filamentInfos[itr->second->getFilamentIndex() - 1].wxStrColor));
        //itr->second->Refresh();
    }

    Thaw();
}

void AnkerObjectBarView::update_ui_from_settings()
{
    //m_pObjectList->apply_volumes_order();
}

void AnkerObjectBarView::object_list_changed()
{
    //updateSize();
}

void AnkerObjectBarView::setListMaxHeight(int maxHeight)
{
    m_listMaxHeight = maxHeight;
    SetMaxSize(AnkerSize(290, m_listMaxHeight + 80));

    updateSize();
}

void AnkerObjectBarView::addObject(AnkerObjectItem* object, AnkerObjectItem* parentObject, bool refresh)
{
    if (m_itemMap.find(object) == m_itemMap.end())
    {
        int filamentSize = Slic3r::GUI::wxGetApp().preset_bundle->filament_presets.size();

        AnkerObjectViewItem* newItem = new AnkerObjectViewItem(m_pListWindow);
        newItem->setObject(object);
        //newItem->setName(object->getText(), false);
        //newItem->;
        //newItem->setPrintable(object->getPrintable(), false);
        //newItem->setFilament(filamentIndex, filamentColor, false);
        newItem->setFilamentVisible(filamentSize > 1, false);
        newItem->Bind(wxANKEREVT_AOI_COLLAPSE_CLICK, [this](wxCommandEvent& event) {this->updateSize(); });
        newItem->Bind(wxANKEREVT_AOI_ITEM_CLICK, &AnkerObjectBarView::OnItemClicked, this);
        newItem->Bind(wxANKEREVT_AOI_SETTINGS_CLICK, &AnkerObjectBarView::OnItemSettingsClicked, this);
        newItem->Bind(wxANKEREVT_AOI_PRINTABLE_CLICK, &AnkerObjectBarView::OnItemPrintableClicked, this);
        newItem->Bind(wxANKEREVT_AOI_FILAMENT_CLICK, &AnkerObjectBarView::OnItemFilamentClicked, this);
        newItem->Bind(wxEVT_KEY_DOWN, &AnkerObjectBarView::OnKey, this);
        newItem->SetPreviousHandler(this);

        auto parentItr = m_itemMap.find(parentObject);
        if (parentItr != m_itemMap.end())
        {
            int insertIndex = 0;
            std::vector<AnkerObjectViewItem*> upperList;
            for (int i = 0; i < m_itemList.size(); i++)
            {
                upperList.push_back(m_itemMap[m_itemList[i]]);

                do
                {
                    AnkerObjectViewItem* parentItem = upperList.back();
                    if (parentItem->getObject() == parentObject)
                    {
                        insertIndex += parentItem->getFamilyItemCount();
                        break;
                    }

                    upperList.pop_back();
                    insertIndex++;

                    for (int i = parentItem->getChildItemCount() - 1; i >= 0; i--)
                    {
                        upperList.push_back(parentItem->getChildItemByIndex(i));
                    }

                } while (upperList.size() > 0);

                if (upperList.size() > 0)
                    break;
            }

            m_pListWinVSizer->Insert(insertIndex, newItem, 1, wxEXPAND | wxCENTER, 0);

            parentItr->second->addChildItem(newItem, false);
            m_itemMap.insert({ object, newItem });

            newItem->Show(!parentItr->second->isCollapsed());
        }
        else
        {
            m_pListWinVSizer->Add(newItem, 1, wxEXPAND | wxCENTER, 0);

            m_itemMap.insert({ object, newItem });
            m_itemList.push_back(object);
        
            //setSelectedObjectSingle(object);
            newItem->Show();
        }

        if (refresh)
        {
            updateSize();
        }
    }
}

void AnkerObjectBarView::removeObject(AnkerObjectItem* object, bool removeChildItem, bool refresh)
{
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        AnkerObjectViewItem* targetItem = itr->second;
        std::vector<AnkerObjectViewItem*> deletingItemList = { targetItem };
        if (removeChildItem)
        {
            for (int i = 0; i < deletingItemList.size(); i++)
            {
                AnkerObjectViewItem* item = deletingItemList[i];
                for (int j = 0; j < item->getChildItemCount(); j++)
                {
                    AnkerObjectViewItem* childItem = item->getChildItemByIndex(j);
                    deletingItemList.push_back(childItem);
                }
			}
        }
        else
            targetItem->clearAll(removeChildItem);

        if (targetItem->hasParent())
            targetItem->getParentItem()->removeChildItem(object);

        for (int i = 0; i < deletingItemList.size(); i++)
        {
            AnkerObjectViewItem* item = deletingItemList[i];

            for (auto itr = m_selectedItems.begin(); itr != m_selectedItems.end(); itr++)
            {
                if (*itr == item)
                {
                    m_selectedItems.erase(itr);
                    break;
                }
            }

            m_itemMap.erase(item->getObject());

            for (auto itr = m_itemList.begin(); itr != m_itemList.end(); itr++)
            {
                if (*itr == object)
                {
                    m_itemList.erase(itr);
                    break;
                }
            }

            m_pListWinVSizer->Detach(item);
            delete item;
            item = nullptr;
        }

        if (m_currentSelectedItem && m_itemMap.find(m_currentSelectedItem->getObject()) == m_itemMap.end())
            m_currentSelectedItem = nullptr;

        if (refresh)
            updateSize();
    }
}

void AnkerObjectBarView::clearAll(bool sizeUpdateFlag)
{
    m_pListWinVSizer->Clear();
    m_pListWindow->DestroyChildren();
    //for (auto itr = m_itemMap.begin(); itr != m_itemMap.end(); itr++)
    //{
    //    itr->second->SetParent(nullptr);
    //    delete itr->second;
    //}
    m_itemMap.clear();
    m_itemList.clear();
    m_currentSelectedItem = nullptr;
    m_selectedItems.clear();

    if (sizeUpdateFlag)
        updateSize();
}

//void AnkerObjectBarView::updateObjectName(AnkerObjectItem* object, wxString name)
//{
//    auto itr = m_itemMap.find(object);
//    if (itr != m_itemMap.end())
//    {
//        AnkerObjectViewItem* targetItem = itr->second;
//        targetItem->setName(name);
//    }
//}
//
//void AnkerObjectBarView::updateObjectPrintable(AnkerObjectItem* object, bool printable)
//{
//    auto itr = m_itemMap.find(object);
//    if (itr != m_itemMap.end())
//    {
//        AnkerObjectViewItem* targetItem = itr->second;
//        targetItem->setPrintable(printable);
//    }
//}
//
//void AnkerObjectBarView::updateObjectFilament(AnkerObjectItem* object, int filamentIndex, wxColour filamentColor)
//{
//    auto itr = m_itemMap.find(object);
//    if (itr != m_itemMap.end())
//    {
//        AnkerObjectViewItem* targetItem = itr->second;
//        targetItem->setFilament(filamentIndex, filamentColor);
//    }
//}

void AnkerObjectBarView::updateObject(AnkerObjectItem* object)
{
	auto itr = m_itemMap.find(object);
	if (itr != m_itemMap.end())
	{
		AnkerObjectViewItem* targetItem = itr->second;
		targetItem->Refresh();

        std::vector<AnkerObjectViewItem*> updatedItems = { targetItem };
        do
        {
            AnkerObjectViewItem* item = updatedItems.back();
            item->Refresh();
            updatedItems.pop_back();

            for (int i = 0; i < item->getChildItemCount(); i++)
            {
                updatedItems.push_back(item->getChildItemByIndex(i));
            }
        } while (updatedItems.size() > 0);
	}
}

void AnkerObjectBarView::updateObjectParent(AnkerObjectItem* object, AnkerObjectItem* parentObject)
{
    auto itr = m_itemMap.find(object);
    auto parentItr = m_itemMap.find(parentObject);
    if (itr != m_itemMap.end() && parentItr != m_itemMap.end())
    {
        AnkerObjectViewItem* targetItem = itr->second;

        // remove from old parent
        if (targetItem->hasParent())
        {
            AnkerObjectViewItem* oldParentItem = targetItem->getParentItem();
            oldParentItem->removeChildItem(object);
        }

        m_pListWinVSizer->Detach(targetItem);

        // add to new parent
        parentItr->second->addChildItem(targetItem);

        int insertIndex = 0;
        for (int i = 0; i < m_itemList.size(); i++)
        {
            insertIndex += m_itemMap[m_itemList[i]]->getChildItemCount() + 1;

            if (m_itemList[i] == parentObject)
                break;
        }
        m_pListWinVSizer->Insert(insertIndex, targetItem, 1, wxEXPAND | wxCENTER, 0);

        Layout();
        Refresh();
    }
}

AnkerObjectItem* AnkerObjectBarView::getParentObject(AnkerObjectItem* object)
{
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        AnkerObjectViewItem* targetItem = itr->second;

        if (targetItem->hasParent())
        {
            AnkerObjectViewItem* parentItem= targetItem->getParentItem();
            for (auto itr = m_itemMap.begin(); itr != m_itemMap.end(); itr++)
            {
                if (itr->second == parentItem)
                    return itr->first;
            }
        }
        return nullptr;
    }
    return nullptr;
}

void AnkerObjectBarView::setSelectedObjectSingle(AnkerObjectItem* object)
{
    for (int i = 0; i < m_selectedItems.size(); i++)
    {
        m_selectedItems[i]->select(false);
    }
    m_selectedItems.clear();

    m_currentSelectedItem = nullptr;

    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        m_currentSelectedItem = itr->second;
        m_currentSelectedItem->select(true);
        m_selectedItems.push_back(m_currentSelectedItem);
    }
}

void AnkerObjectBarView::setSelectedObjectMulti(AnkerObjectItem* object, bool selected)
{
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        m_currentSelectedItem = itr->second;

        auto objItr = m_selectedItems.begin();
        for ( ; objItr != m_selectedItems.end(); objItr++)
        {
            if (*objItr == m_currentSelectedItem)
                break;
        }

        if (selected)
        {
            if (objItr == m_selectedItems.end())
                m_selectedItems.push_back(m_currentSelectedItem);
        }
        else if (objItr != m_selectedItems.end())
        {
            m_selectedItems.erase(objItr);
        }

        m_currentSelectedItem->select(selected);
    }
}

void AnkerObjectBarView::setSelectedObjectMultiContinuous(AnkerObjectItem* object)
{
    // TODO

    //auto itr = m_itemMap.find(object);
    //if (itr != m_itemMap.end())
    //{
    //    m_currentSelectedItem = itr->second;
    //    m_selectedItem->select(true);
    //}
}

void AnkerObjectBarView::selectAll(bool selected)
{
    for(auto &selectItem : m_selectedItems) {
        if (selectItem) {
            selectItem->select(false);
        }
    }
    
    m_selectedItems.clear();
    for (int i = 0; i < m_itemList.size(); i++)
    {
        m_itemMap[m_itemList[i]]->select(selected);

        if (selected)
        {
            m_selectedItems.push_back(m_itemMap[m_itemList[i]]);
        }
    }
}

void AnkerObjectBarView::collapseObject(AnkerObjectItem* object, bool collapsed)
{
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        itr->second->collapse(collapsed);
    }
}

void AnkerObjectBarView::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxSize size = GetSize();
    dc.SetPen(GetBackgroundColour());
    dc.SetBrush(GetBackgroundColour());
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
}

void AnkerObjectBarView::OnSize(wxSizeEvent& event)
{
    //// Handle the size event here
    //wxSize newSize = event.GetSize();
    //wxGraphicsPath path = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
    //// appends a rounded rectangle
    //path.AddRoundedRectangle(0, 0, newSize.GetWidth(), newSize.GetHeight(), 8);
    //SetShape(path);
}

void AnkerObjectBarView::OnShow(wxShowEvent& event)
{
    // event.Skip();
}

void AnkerObjectBarView::OnKey(wxKeyEvent& event)
{
    std::cout << "===========AnkerObjectBarView::OnKey" << std::endl;

	wxKeyEvent evt = wxKeyEvent(wxANKEREVT_AOBV_KEY_EVENT);
    evt.m_keyCode = event.GetKeyCode();
	ProcessEvent(evt);
}

void AnkerObjectBarView::OnAddButton(wxCommandEvent& event)
{
    m_pPlater->add_model();
}

void AnkerObjectBarView::OnPullDownButton(wxCommandEvent& event)
{
    if (m_pListWindow)
    {
        m_pSplitControl->Show(!m_pSplitControl->IsShown());
        m_pListWindow->Show(!m_pListWindow->IsShown());

        if (m_pListWindow->IsShown())
        {
            m_pPullDownButton->SetBitmap_("pulldown_48x48");
        }
        else
        {
            m_pPullDownButton->SetBitmap_("pullup_48x48");
        }

        Fit();
        Layout();
    }
}

void AnkerObjectBarView::OnItemClicked(wxCommandEvent& event)
{
   // std::cout << "void AnkerObjectBarView::OnItemClicked(wxCommandEvent& event)" << std::endl;

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* object = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOBV_ITEM_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
}

void AnkerObjectBarView::OnItemSettingsClicked(wxCommandEvent& event)
{
    std::cout << "void AnkerObjectBarView::OnItemSettingsClicked(wxCommandEvent& event)" << std::endl;

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* object = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOBV_SETTINGS_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
}


void AnkerObjectBarView::OnItemPrintableClicked(wxCommandEvent& event)
{
    std::cout << "void AnkerObjectBarView::OnItemPrintableClicked(wxCommandEvent& event)" << std::endl;

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* object = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOBV_PRINTABLE_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
}

void AnkerObjectBarView::OnItemFilamentClicked(wxCommandEvent& event)
{
    std::cout << "void AnkerObjectBarView::OnItemFilamentClicked(wxCommandEvent& event)" << std::endl;

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* object = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    auto itr = m_itemMap.find(object);
    if (itr != m_itemMap.end())
    {
        AnkerObjectViewItem* viewItem = itr->second;
        AnkerObjectItem* objectItem = viewItem->getObject();

        int objId = event.GetInt();

        int currentIndex = objectItem->getFilamentIndex() - 1;
        std::vector<std::pair<wxColour, wxString>> contentList;
        const std::vector<Slic3r::GUI::SFilamentInfo>& filamentInfos = Slic3r::GUI::wxGetApp().plater()->sidebarnew().getEditFilamentList();
        for (int i = 0; i < filamentInfos.size(); i++)
        {
            contentList.push_back({ wxColour(filamentInfos[i].wxStrColor), filamentInfos[i].wxStrLabelType });
        }
        auto size = viewItem->GetSize();
        Slic3r::GUI::wxGetApp().floatinglist()->Move(viewItem->GetScreenPosition() + wxPoint(size.x, 0));
        Slic3r::GUI::wxGetApp().floatinglist()->setContentList(contentList);
        Slic3r::GUI::wxGetApp().floatinglist()->setCurrentSelection(currentIndex);
        Slic3r::GUI::wxGetApp().floatinglist()->Show();
        Slic3r::GUI::wxGetApp().floatinglist()->Raise();
        Slic3r::GUI::wxGetApp().floatinglist()->SetFocus();

        Slic3r::GUI::wxGetApp().floatinglist()->setItemClickCallback([this, object](int index) {
            Slic3r::GUI::wxGetApp().floatinglist()->Hide();

            int currentIndex = Slic3r::GUI::wxGetApp().floatinglist()->getCurrentSelectionIndex();
            //std::pair<wxColour, wxString> currentSelection = Slic3r::GUI::wxGetApp().floatinglist()->getItemContent(currentIndex);

            wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOBV_FILAMENT_CLICK);
            wxVariant eventData;
            eventData.ClearList();
            eventData.Append(wxVariant(object));
            eventData.Append(wxVariant(currentIndex));
            evt.SetClientData(new wxVariant(eventData));
            ProcessEvent(evt);

            Refresh();
         });
    }
}

AnkerObjectViewItem::AnkerObjectViewItem(wxWindow* parent)
    : wxControl(parent, wxID_ANY, wxPoint(300, 300), wxDefaultSize, wxNO_BORDER) // init the control out of the parent to prevent flicker
    , m_pParentItem(nullptr)
    , m_isCollapsed(false)
    , m_isSelected(false)
    , m_isHover(false)
    , m_topMargin(7)
    , m_borderMargin(12)
    , m_collapseMargin(4)
    , m_toolMargin(8)
    , m_childXOffset(16)
    , m_object(nullptr)
    , m_hasFilament(false)
    , m_drawingNameLenMax(18)
    //, m_drawingName("NAME")
    //, m_printable(true)
    //, m_name("NAME")
    //, m_filamentIndex(0)
    //, m_filamentColour(0, 0, 0)
    , m_bgColour(PANEL_BACK_RGB_INT)
    , m_fgColour(255, 255, 255)
    , m_normalColour(PANEL_BACK_RGB_INT)
    , m_hoverColour(62, 63, 66)
    , m_selectedColour(53, 65, 56)
    //, m_collapseBtnRect(AnkerRect(5, 11, 8, 30))
    //, m_printableBtnRect(AnkerRect(223, 8, 30, 30))
    //, m_printableBtnDrawingRect(AnkerRect(252, 8, 20, 20))
    //, m_filamenColourBtnRect(AnkerRect(252, 6, 20, 20))
    , m_collapseBtnSize(8, 8)
    , m_iconSize(16, 16)
    , m_toolBtnSize(23, 23)
{
    m_collapseBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("group_collapsed.png")), wxBITMAP_TYPE_PNG);
    m_collapseBtnImage.Rescale(8, 8);
    m_expandBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("group_expand.png")), wxBITMAP_TYPE_PNG);
    m_expandBtnImage.Rescale(8, 8); 
    m_printableBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("fdm_eye_icon_s_48x48.png")), wxBITMAP_TYPE_PNG);
    m_printableBtnImage.Rescale(16, 16, wxImageResizeQuality::wxIMAGE_QUALITY_BILINEAR);
    m_unprintableBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("fdm_hidden_icon_s_48x48.png")), wxBITMAP_TYPE_PNG);
    m_unprintableBtnImage.Rescale(16, 16);
    m_settingBtnImage = wxImage(wxString::FromUTF8(Slic3r::var("fdm_setting_icon_s_48x48.png")), wxBITMAP_TYPE_PNG);
    m_settingBtnImage.Rescale(16, 16);

    initUI();

    Bind(wxEVT_PAINT, &AnkerObjectViewItem::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerObjectViewItem::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &AnkerObjectViewItem::OnLeftUp, this);
    Bind(wxEVT_MOTION, &AnkerObjectViewItem::OnMouseMove, this);

    Bind(wxEVT_ENTER_WINDOW, &AnkerObjectViewItem::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) { m_isHover = false; m_bgColour = (m_isSelected ? m_selectedColour : m_normalColour); SetCursor(wxCursor(wxCURSOR_NONE)); Refresh(); });
}

AnkerObjectViewItem::~AnkerObjectViewItem()
{
    setParentItem(nullptr);

    m_childItems.clear();
}

//void AnkerObjectViewItem::setName(wxString name, bool refresh)
//{
//    m_name = name;
//    m_drawingName = name;
//    if (m_drawingName.Length() > m_drawingNameLenMax)
//    {
//        int subLen = m_drawingNameLenMax / 2;
//        m_drawingName = m_drawingName.substr(0, subLen) + "..." + m_drawingName.substr(m_drawingName.Len() - subLen, subLen);
//    }
//
//    if (refresh)
//        Refresh();
//}
//
void AnkerObjectViewItem::setFilamentVisible(bool visible, bool refresh)
{
    m_hasFilament = visible; 
    //m_printableBtnDrawingRect.x = m_hasFilament ? m_printableBtnRect.x : m_filamenColourBtnRect.x;

    if (refresh)
        Refresh();
}

void AnkerObjectViewItem::setParentItem(AnkerObjectViewItem* parentItem)
{
    m_pParentItem = parentItem;
}

void AnkerObjectViewItem::addChildItem(AnkerObjectViewItem* item, bool refresh)
{
    AnkerObjectItem* object = nullptr;
    if (item && item->getObject() != nullptr)
    {
        object = item->getObject();
        item->setParentItem(this);
        m_childItems.insert({ object, item });
        m_childIds.push_back(object);

        if (refresh)
            Refresh();
    }
}

AnkerObjectViewItem* AnkerObjectViewItem::removeChildItem(AnkerObjectItem* object)
{
    auto itr = m_childItems.find(object);
    if (itr != m_childItems.end())
    {
        AnkerObjectViewItem* targetItem = itr->second;
        m_childItems.erase(itr);

        for (auto itr = m_childIds.begin(); itr != m_childIds.end(); itr++)
        {
            if (*itr == object)
            {
                m_childIds.erase(itr);
                break;
            }
        }

        return targetItem;
    }

    return nullptr;
}

void AnkerObjectViewItem::clearAll(bool deleted, AnkerObjectViewItem* newParentItem)
{
    for (auto itr = m_childItems.begin(); itr != m_childItems.end(); itr++)
    {
        if (deleted)
        {
            delete itr->second;
            itr->second = nullptr;
        }
        else
        {
            newParentItem->addChildItem(itr->second);
        }
    }
    m_childItems.clear();
}

int AnkerObjectViewItem::getFamilyItemCount()
{
    int count = 1;
    for (int i = 0; i < m_childIds.size(); i++)
    {
        count += m_childItems[m_childIds[i]]->getFamilyItemCount();
    }
    return count;
}

void AnkerObjectViewItem::collapse(bool on)
{
    if (on != m_isCollapsed)
    {
        m_isCollapsed = on;
        for (auto itr = m_childItems.begin(); itr != m_childItems.end(); itr++)
        {
            if (itr->second)
            {
                itr->second->Show(!m_isCollapsed);
                itr->second->collapse(m_isCollapsed);
            }
        }
        Refresh();
    }
}

void AnkerObjectViewItem::select(bool selected)
{
    m_isSelected = selected;

    if (!m_isHover)
    {
        m_bgColour = m_isSelected ? m_selectedColour : m_normalColour;

        Refresh();
    }
}

void AnkerObjectViewItem::initUI()
{
    SetMinSize(AnkerSize(290, 30));
    SetMaxSize(AnkerSize(290, 30));
    SetSize(AnkerSize(290, 30));

    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerObjectViewItem::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.Clear();

    if (m_object == nullptr)
        return;

    wxColour bgColor = m_bgColour;
    wxColour fgColor = m_fgColour;

    wxRect rect = GetClientRect();
    rect.width += 1;
    rect.height += 1;
    wxBrush brush(bgColor);
    wxPen pen(bgColor);
    dc.SetBrush(brush);
    dc.SetPen(pen);
    dc.DrawRectangle(rect);

    int x = m_borderMargin;
    int y = m_topMargin;

    AnkerObjectViewItem* parentTemp = m_pParentItem;
    while (parentTemp != nullptr)
    {
        x += m_childXOffset;
        parentTemp = parentTemp->getParentItem();
    }

    // draw collapse button
    if (getChildItemCount() > 0)
    {
        wxBrush brush(bgColor);
        wxPen pen(wxColour(255, 255, 255));
        dc.SetBrush(brush);
        dc.SetPen(pen);
        wxPoint imagePos(x, (rect.GetHeight() - m_collapseBtnSize.GetHeight()) / 2.0);
        if (m_isCollapsed)
            dc.DrawBitmap(m_collapseBtnImage, imagePos);
        else
            dc.DrawBitmap(m_expandBtnImage, imagePos);

        m_collapseBtnRect = wxRect(imagePos.x, imagePos.y, m_collapseBtnSize.GetWidth(), rect.GetHeight());
        x += m_collapseBtnSize.GetWidth();
    }
    else
        m_collapseBtnRect = wxRect(0, 0, 0, 0);

    // draw object name
    {       
        x += m_collapseMargin;

        wxString drawingName = m_object->getText();
		if (drawingName.Length() > m_drawingNameLenMax)
		{
			int subLen = m_drawingNameLenMax / 2;
			drawingName = drawingName.substr(0, subLen) + "..." + drawingName.substr(drawingName.Len() - subLen);
		}

        wxBrush brush(bgColor);
        wxPen pen(fgColor);
        dc.SetBrush(brush);
        dc.SetPen(pen);
        dc.SetFont(ANKER_FONT_NO_1);
        dc.SetTextForeground(fgColor);
//#ifdef __APPLE__
//        wxPoint textPoint = wxPoint(hasParent() ? 29 : 17, 10);
//#else
//        wxPoint textPoint = wxPoint(hasParent() ? 29 : 17, 5);
//#endif
        wxSize textSize = dc.GetTextExtent(/*drawingName*/"A");
        wxPoint textPoint = wxPoint(x, (rect.GetHeight() - textSize.GetHeight()) / 2.0);
        dc.DrawText(drawingName, textPoint);
    }

    // draw reverse
    x = GetSize().GetWidth() - m_borderMargin - 5;

    // draw filament colour button
    if (m_object->hasFilamentInfo() && m_hasFilament)
    {
        x -= m_toolBtnSize.GetWidth();

        // draw filemant colour
        bgColor = m_object->getFilamentColour();
        wxBrush brush(bgColor);
        wxPen pen(wxColour(41, 42, 45));
        dc.SetBrush(brush);
        dc.SetPen(pen);
        wxPoint drawPoint = wxPoint(x, (rect.GetHeight() - m_toolBtnSize.GetHeight()) / 2.0);
        dc.DrawRoundedRectangle(drawPoint, m_toolBtnSize, 3);

        // draw filament index
        wxColour foreColor = wxColour(255, 255, 255);
        if (bgColor.GetLuminance() > 0.6)
            foreColor = wxColour(0, 0, 0);

        {
            wxBrush brush(bgColor);
            wxPen pen(foreColor);
            dc.SetBrush(brush);
            dc.SetPen(pen);
            dc.SetFont(ANKER_BOLD_FONT_NO_2);
            dc.SetTextForeground(foreColor);
#ifdef __APPLE__
            wxPoint textPoint = wxPoint(m_filamenColourBtnRect.x + m_filamenColourBtnRect.width / 2 - 5, m_filamenColourBtnRect.y + m_filamenColourBtnRect.height / 2 - 5);
#else
            wxPoint textPoint = wxPoint(drawPoint.x + m_toolBtnSize.GetWidth() / 2 - 4, drawPoint.y + m_toolBtnSize.GetHeight() / 2 - 8);
#endif
            dc.DrawText(std::to_string(m_object->getFilamentIndex()), textPoint);
        }

        // draw bottom right triangle
        {
            wxBrush brush(foreColor);
            wxPen pen(foreColor);
            dc.SetBrush(brush);
            dc.SetPen(pen);
            wxPoint triPoints[3];
            triPoints[0] = wxPoint(drawPoint.x + m_toolBtnSize.GetWidth() - 8, drawPoint.y + m_toolBtnSize.GetHeight() - 3);
            triPoints[1] = wxPoint(drawPoint.x + m_toolBtnSize.GetWidth() - 3, drawPoint.y + m_toolBtnSize.GetHeight() - 3);
            triPoints[2] = wxPoint(drawPoint.x + m_toolBtnSize.GetWidth() - 3, drawPoint.y + m_toolBtnSize.GetHeight() - 8);
            dc.DrawPolygon(3, triPoints);
        }

        m_filamenColourBtnRect = wxRect(drawPoint.x, drawPoint.y, m_toolBtnSize.GetWidth(), m_toolBtnSize.GetHeight());
    }
    else
        m_filamenColourBtnRect = wxRect(0, 0, 0, 0);

    // draw printable button
    if (m_object->hasPrintableInfo())
    {
        x -= m_toolMargin + m_toolBtnSize.GetWidth();

        wxColour btnFgColour(fgColor.Red(), fgColor.Blue(), fgColor.Green());
        wxBrush brush(bgColor);
        wxPen pen(btnFgColour);
        dc.SetBrush(brush);
        dc.SetPen(wxColour(255, 0, 0));
        wxPoint textPoint(x, (rect.GetHeight() - m_iconSize.GetHeight()) / 2.0);
        if (m_object->getPrintable())
            dc.DrawBitmap(m_printableBtnImage, textPoint);
        else
            dc.DrawBitmap(m_unprintableBtnImage, textPoint);

        m_printableBtnRect = wxRect(textPoint.x, textPoint.y, m_iconSize.GetWidth(), m_iconSize.GetHeight());
    }
    else
        m_printableBtnRect = wxRect(0, 0, 0, 0);

    // draw settings button
    if (m_object->hasSetting())
    {
        x -= m_toolMargin + m_toolBtnSize.GetWidth();

        wxColour btnFgColour(fgColor.Red(), fgColor.Blue(), fgColor.Green());
        wxBrush brush(bgColor);
        wxPen pen(btnFgColour);
        dc.SetBrush(brush);
        dc.SetPen(wxColour(255, 0, 0));
        wxPoint textPoint(x, (rect.GetHeight() - m_iconSize.GetHeight()) / 2.0);
        dc.DrawBitmap(m_settingBtnImage, textPoint);

        m_settingBtnRect = wxRect(textPoint.x, textPoint.y, m_iconSize.GetWidth(), m_iconSize.GetHeight());
    }
    else
        m_settingBtnRect = wxRect(0, 0, 0, 0);
}

void AnkerObjectViewItem::OnMouseEnter(wxMouseEvent& event)
{
    m_isHover = true; 
    m_bgColour = m_hoverColour; 
    Refresh();
}

void AnkerObjectViewItem::OnMouseMove(wxMouseEvent& event)
{
    if (!m_isHover)
        return;

    int mouseX = 0, mouseY = 0;
    event.GetPosition(&mouseX, &mouseY);

    wxPoint mousePos(mouseX, mouseY);
    if (m_collapseBtnRect.Contains(mousePos))
    {
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    else if (m_printableBtnRect.Contains(mousePos))
    {
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    else if (m_filamenColourBtnRect.Contains(mousePos))
    {
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    else if (m_settingBtnRect.Contains(mousePos))
    {
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    else
    {
        SetCursor(wxCursor(wxCURSOR_NONE));
    }
}

void AnkerObjectViewItem::OnLeftDown(wxMouseEvent& event)
{
}

void AnkerObjectViewItem::OnLeftUp(wxMouseEvent& event)
{
    int mouseX = 0, mouseY = 0;
    event.GetPosition(&mouseX, &mouseY);

    wxPoint mousePos(mouseX, mouseY);
    if (m_collapseBtnRect.Contains(mousePos))
    {
        collapse(!m_isCollapsed);

        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOI_COLLAPSE_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(m_object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
    else if (m_printableBtnRect.Contains(mousePos))
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOI_PRINTABLE_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(m_object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
    else if (m_filamenColourBtnRect.Contains(mousePos))
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOI_FILAMENT_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(m_object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
    else if (m_settingBtnRect.Contains(mousePos))
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOI_SETTINGS_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(m_object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
    else
    {
        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOI_ITEM_CLICK);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(m_object));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);

        SetFocus();
    }
}

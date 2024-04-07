#include "AnkerObjectLayers.hpp"
//#include "GUI_ObjectList.hpp"

#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"

#include "OptionsGroup.hpp"
#include "GUI_App.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Model.hpp"
#include "GLCanvas3D.hpp"
#include "Plater.hpp"

#include <boost/algorithm/string.hpp>

#include "I18N.hpp"

#include <wx/wupdlock.h>

namespace Slic3r
{
namespace GUI
{

AnkerObjectLayers::AnkerObjectLayers(wxWindow* parent)// :  OG_Settings(parent, true)
{
/*
    m_grid_sizer = new wxFlexGridSizer(3, 5, wxGetApp().em_unit()); // "Min Z", "Max Z", "Layer height" & buttons sizer
    m_grid_sizer->SetFlexibleDirection(wxHORIZONTAL);

 
    // Legend for object layers
    for (const std::string col : { L("Start at height"), L("Stop at height"), L("Layer height") }) {
        auto temp = new wxStaticText(m_parent, wxID_ANY, _(col), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
        temp->SetBackgroundStyle(wxBG_STYLE_PAINT);
        temp->SetFont(wxGetApp().bold_font());

        m_grid_sizer->Add(temp);
    }
  
    m_og->activate();
    m_og->sizer->Clear(true);
    m_og->sizer->Add(m_grid_sizer, 0, wxEXPAND | wxALL, wxOSX ? 0 : 5);
*/

    m_parent = parent;
    m_bmp_delete    = ScalableBitmap(parent, "remove_layer"/*"cross"*/);
    m_bmp_add       = ScalableBitmap(parent, "add_copies");

}

void AnkerObjectLayers::select_editor(AnkerLayerRangeEditor* editor, const bool is_last_edited_range)
{
    if (is_last_edited_range && m_selection_type == editor->type()) {
    /* Workaround! Under OSX we should use CallAfter() for SetFocus() after LayerEditors "reorganizations", 
     * because of selected control's strange behavior: 
     * cursor is set to the control, but blue border - doesn't.
     * And as a result we couldn't edit this control.
     * */
#ifdef __WXOSX__
        wxTheApp->CallAfter([editor]() {
#endif
        editor->SetFocus();
        editor->SetInsertionPointEnd();
#ifdef __WXOSX__
        });
#endif
    }    
}

wxSizer* AnkerObjectLayers::create_layer(const t_layer_height_range& range, PlusMinusButton *delete_btn, PlusMinusButton *add_btn) 
{
    ++m_rangeCnt;

    m_ranges.push_back(range);
    t_layer_height_range* pRangeItem = nullptr;
    if (m_ranges.size())
        pRangeItem = &m_ranges[m_ranges.size() - 1];

    bool isSelectedObjectItem = range.first == m_selectable_range.first && range.second == m_selectable_range.second ? true : false;
 
    if (m_top_layer_range.first < range.first) {
        m_top_layer_range = range;
    }

    if (m_top_layer_range.first == range.first && m_top_layer_range.second < range.second) {
        m_top_layer_range = range;
    }

    const bool is_last_edited_range = range == m_selectable_range;

    auto set_focus_data = [range, this](const AnkerEditorType type)
    {
        m_selectable_range = range;
        m_selection_type = type;
    };

    auto update_focus_data = [range, this](const t_layer_height_range& new_range, AnkerEditorType type, bool enter_pressed)
    {
        // change selectable range for new one, if enter was pressed or if same range was selected
        if (enter_pressed || m_selectable_range == range) {
            m_selectable_range = new_range;
        }
        if (enter_pressed)
            m_selection_type = type;
    };

    wxBoxSizer* lineSizer = new wxBoxSizer(wxHORIZONTAL);
    wxWindow* theParent = m_contentPanel;

    if (m_rangeCnt == 1)
        m_contentPanelSizer->AddSpacer(6);

    // hightlight the selected Range line
    if (isSelectedObjectItem) {
        wxBoxSizer* innerSizer = new wxBoxSizer(wxHORIZONTAL);
        auto hightLightPanel = new wxPanel(m_contentPanel);
        hightLightPanel->SetBackgroundColour(wxColour(53, 65, 56));
        hightLightPanel->SetMinSize(wxSize(m_contentPanel->GetClientSize().x, -1));
        hightLightPanel->SetSize(wxSize(m_contentPanel->GetClientSize().x, -1));
        hightLightPanel->SetSizer(innerSizer);
        lineSizer->Add(hightLightPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);
        //m_contentPanelSizer->Add(lineSizer, 0, wxEXPAND | wxALL, 10);
        m_contentPanelSizer->Add(lineSizer);

        lineSizer = innerSizer;
        theParent = hightLightPanel;
    }
    else {
        m_contentPanelSizer->Add(lineSizer, 0, wxEXPAND | wxALL, 0);
    }


    // add range line: index,minRange,-,maxRange,mm,deleteBtn 
    {
        PlusMinusButton* delete_button = new PlusMinusButton(theParent, m_bmp_delete, range);
        delete_button->SetBackgroundColour(theParent->GetBackgroundColour());
        //delete_button->SetMinSize(wxSize(25, 25));
        //delete_button->SetMaxSize(wxSize(25,25));
        //del_btn->SetToolTip(_L("Remove layer range"));
        delete_button->Bind(wxEVT_BUTTON, [delete_button](wxEvent&) {
            delete_button->SetFocus();
            ANKER_LOG_INFO << "delete layer range :" << delete_button->range.first << "-" << delete_button->range.second;
            wxGetApp().objectbar()->del_layer_range(delete_button->range);
            });
        PlusMinusButton* add_button = nullptr;//new PlusMinusButton(theParent, m_bmp_add, range);
        //add_button->SetToolTip(_L("add layer range"));

        // add index number
        lineSizer->AddSpacer(15);
        auto indexLabel = new wxStaticText(theParent, wxID_ANY, "99");
        indexLabel->SetBackgroundStyle(wxBG_STYLE_PAINT);
        indexLabel->SetOwnForegroundColour(wxColour("#a9aaab"));
        indexLabel->SetFont(ANKER_FONT_NO_1);
        int width = indexLabel->GetTextExtent("99").x;
        indexLabel->SetMinSize(wxSize(width, -1));
        indexLabel->SetSize(wxSize(width, -1));
        indexLabel->SetLabel(wxString::Format("%d", m_rangeCnt));
        lineSizer->Add(indexLabel, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);

        lineSizer->AddSpacer(10);

        // Add editor for the "Min Z"
        auto editor = new AnkerLayerRangeEditor(this, theParent, pRangeItem, double_to_string(range.first), Ak_etMinZ, set_focus_data,
            [/*range,*/ update_focus_data, this, delete_button, add_button](t_layer_height_range* range, coordf_t min_z, bool enter_pressed, bool dont_update_ui)
            {
                if (!range) {
                    ANKER_LOG_ERROR << "range ptr is null.";
                    return false;
                }

                if (fabs(min_z - (*range).first) < EPSILON) {
                    m_selection_type = Ak_etUndef;
                    ANKER_LOG_ERROR << "edit layer range faile ,fabs(min_z - range.first) < EPSILON ";
                    return false;
                }

                // data for next focusing
                coordf_t max_z = min_z < (*range).second ? (*range).second : min_z + 0.5;
                const t_layer_height_range new_range = { min_z, max_z };
                if (delete_button)
                    delete_button->range = new_range;
                if (add_button)
                    add_button->range = new_range;
                update_focus_data(new_range, Ak_etMinZ, enter_pressed);

                if (wxGetApp().objectbar()->edit_layer_range((*range), new_range, dont_update_ui))
                {
                    if (m_top_layer_range.second < new_range.second)
                        m_top_layer_range = new_range;

                    (*range) = new_range;
                    return true;
                }
                else
                    return false;
            });
        //editor->SetBackgroundStyle(wxBG_STYLE_PAINT);
        editor->SetBackgroundColour(theParent->GetBackgroundColour());
        editor->SetForegroundColour(wxColour(255, 255, 255));
        select_editor(editor, is_last_edited_range);
        lineSizer->Add(editor, 0, wxTOP | wxBOTTOM, 6);
        lineSizer->AddSpacer(10);

        // add "-"
        auto lable = new wxStaticText(theParent, wxID_ANY, ("-"));
        lable->SetOwnForegroundColour(wxColour("#a9aaab"));
        lable->SetBackgroundStyle(wxBG_STYLE_PAINT);
        lable->SetFont(ANKER_FONT_NO_1);
        lineSizer->Add(lable, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);
        lineSizer->AddSpacer(10);

        // Add editor for the "Max Z"
        editor = new AnkerLayerRangeEditor(this, theParent, pRangeItem, double_to_string(range.second), Ak_etMaxZ, set_focus_data,
            [/*range,*/ update_focus_data, this, delete_button, add_button](t_layer_height_range* range, coordf_t max_z, bool enter_pressed, bool dont_update_ui)
            {
                if (!range) {
                    ANKER_LOG_ERROR << "range ptr is null.";
                    return false;
                }

                if (fabs(max_z - (*range).second) < EPSILON || (*range).first > max_z) {
                    m_selection_type = Ak_etUndef;
                    ANKER_LOG_ERROR << "edit layer range faile ,fabs(min_z - range.first) < EPSILON ";
                    return false;       // LayersList would not be updated/recreated
                }

                // data for next focusing
                const t_layer_height_range& new_range = { (*range).first, max_z };
                if (delete_button)
                    delete_button->range = new_range;
                if (add_button)
                    add_button->range = new_range;
                update_focus_data(new_range, Ak_etMaxZ, enter_pressed);

                if (wxGetApp().objectbar()->edit_layer_range((*range), new_range, dont_update_ui))
                {
                    if (m_top_layer_range.second < new_range.second)
                        m_top_layer_range = new_range;

                    (*range) = new_range;
                    return true;
                }
                else
                    return false;
            });

        //editor->SetBackgroundStyle(wxBG_STYLE_PAINT);
        //editor->SetBackgroundColour(wxColour(41, 42, 45));
        editor->SetBackgroundColour(theParent->GetBackgroundColour());
        editor->SetForegroundColour(wxColour(255, 255, 255));
        select_editor(editor, is_last_edited_range);
        lineSizer->Add(editor, 0, wxTOP | wxBOTTOM, 6);
        lineSizer->AddSpacer(10);

        /*
            // Add control for the "Layer height"
            editor = new AnkerLayerRangeEditor(this, double_to_string(m_object->layer_config_ranges[range].option("layer_height")->getFloat()), Ak_etLayerHeight, set_focus_data,
                [range](coordf_t layer_height, bool, bool)
            {
                return wxGetApp().obj_list()->edit_layer_range(range, layer_height);
            });

            select_editor(editor, is_last_edited_range);

            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(editor);
        */

        // add "mm"
        auto mm = new wxStaticText(theParent, wxID_ANY, _L("mm"));
        mm->SetOwnForegroundColour(wxColour("#a9aaab"));
        mm->SetBackgroundStyle(wxBG_STYLE_PAINT);
        mm->SetFont(ANKER_FONT_NO_1); 
        lineSizer->Add(mm, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);

        // add delete btn
        lineSizer->AddStretchSpacer(1);
        if (isSelectedObjectItem) {
            //lineSizer->AddStretchSpacer(30);
            //lineSizer->Add(delete_button,0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            lineSizer->Add(delete_button, 1, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT,10);
        }
        else {
            //lineSizer->Add(delete_button, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            lineSizer->Add(delete_button, 1, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 10);
        }

        lineSizer->AddSpacer(12);
    }

    return lineSizer;
}

void AnkerObjectLayers::create_layers_list()
{
    auto layerRanges = m_object->layer_config_ranges;
    for (const auto &layer : layerRanges) {
        const t_layer_height_range& range = layer.first;
        auto lineSizer = create_layer(range, nullptr, nullptr);
    }
}

void AnkerObjectLayers::update_layers_list()
{
    int selectCnt = (wxGetApp().objectbar()->getObjectBarView()->getSelectedCount());
    if (selectCnt > 1)
        return;

    AnkerObjectItem* objviewItem = wxGetApp().objectbar()->getObjectBarView()->getSelectedObject();
    if (!objviewItem)
        return;

    m_object = static_cast<ModelObject*>(objviewItem->getObject());
    if (!m_object || m_object->layer_config_ranges.empty())
    {
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
        return;
    }

    bool display_all_rangs = true;
    AnkerObjectItem::ItemType type = objviewItem->getType();
    if (type == AnkerObjectItem::ITYPE_LAYER) 
    {
        m_selectable_range = objviewItem->getLayerHeightRange();
        m_selection_type = Ak_etUndef;
        update_scene_from_editor_selection();
    }
    else
    {
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
    }

    if (type == AnkerObjectItem::ITYPE_LAYER_GROUP || display_all_rangs)
        create_layers_list();
    else {
        create_layer(objviewItem->getLayerHeightRange(), nullptr, nullptr);
    }

    m_parent->Layout();
}

void AnkerObjectLayers::update_scene_from_editor_selection() const
{
    // needed to show the visual hints in 3D scene
    wxGetApp().plater()->canvas3D()->handle_layers_data_focus_event(m_selectable_range, m_selection_type);
}

void AnkerObjectLayers::UpdateAndShow(const bool show)
{
    std::string panelFlag = "Layer";

    if (!show) 
    {
        if (wxGetApp().plater()->sidebarnew().getSizerFlags() == panelFlag)
        {
            wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer();
            Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
        }
        return;
    }

    wxWindow* rightSidebar = &(wxGetApp().plater()->sidebarnew());
    if (!rightSidebar) return;

    if (!m_parent) m_parent = rightSidebar;
    if (!m_Sizer)  m_Sizer = new wxBoxSizer(wxVERTICAL);

    if (!m_panel) {
        AnkerTitledPanel* container = new AnkerTitledPanel(rightSidebar, 46, 12);
        container->setTitle(_L("common_slice_toolpannel_heightranges"));    // "Height Range"
        container->setTitleAlign(AnkerTitledPanel::TitleAlign::LEFT);
        int returnBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("return.png")), true);
        int addLayerRangeBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("add_layer_range.png")), false);
        m_Sizer->Add(container, 1, wxEXPAND, 0);

       // wxScrolledWindow* scrollWin = new wxScrolledWindow(container);
        scrollWin = new wxScrolledWindow(container);
        scrollWin->SetScrollRate(0, 20);
       // scrollWin->SetVirtualSize(AnkerSize(300, 500));
        scrollWin->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
        scrollWin->SetSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, -1));
        scrollWin->SetScrollbars(0, 30, 300 / 50, 500 / 50);
        m_contentPanel = scrollWin;

        m_contentPanel->SetBackgroundColour(container->GetBackgroundColour());
        m_contentPanelSizer = new wxBoxSizer(wxVERTICAL);
        m_contentPanel->SetSizer(m_contentPanelSizer);

        container->setContentPanel(m_contentPanel);
        container->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, returnBtnID, addLayerRangeBtnID](wxCommandEvent& event) {
            int btnID = event.GetInt();
            if (btnID == returnBtnID)
            {
                wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer();
                Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
            }
            else if (btnID == addLayerRangeBtnID)
            {
                AddLayerRange();
            }
            });

        m_panel = container;
    }

    m_rangeCnt = 0;
    if (m_contentPanelSizer)
        m_contentPanelSizer->Clear(true);


    m_panel->Freeze();
    update_layers_list();
    m_panel->Thaw();


    wxGetApp().plater()->sidebarnew().setMainSizer();
    wxGetApp().plater()->sidebarnew().replaceUniverseSubSizer(m_Sizer, panelFlag);
}


void AnkerObjectLayers::AddLayerRange()
{
    ANKER_LOG_INFO << "add layer range after range:" << m_top_layer_range.first << "-" << m_top_layer_range.second;
    wxGetApp().objectbar()->add_layer_range_after_current(m_top_layer_range);
}


void AnkerObjectLayers::msw_rescale()
{
    //m_bmp_delete.msw_rescale();
    //m_bmp_add.msw_rescale();

    //m_grid_sizer->SetHGap(wxGetApp().em_unit());

    //// rescale edit-boxes
    //const int cells_cnt = m_grid_sizer->GetCols() * m_grid_sizer->GetEffectiveRowsCount();
    //for (int i = 0; i < cells_cnt; ++i) {
    //    const wxSizerItem* item = m_grid_sizer->GetItem(i);
    //    if (item->IsWindow()) {
    //        AnkerLayerRangeEditor* editor = dynamic_cast<AnkerLayerRangeEditor*>(item->GetWindow());
    //        if (editor != nullptr)
    //            editor->msw_rescale();
    //    }
    //    else if (item->IsSizer()) // case when we have editor with buttons
    //    {
    //        wxSizerItem* e_item = item->GetSizer()->GetItem(size_t(0)); // editor
    //        if (e_item->IsWindow()) {
    //            AnkerLayerRangeEditor* editor = dynamic_cast<AnkerLayerRangeEditor*>(e_item->GetWindow());
    //            if (editor != nullptr)
    //                editor->msw_rescale();
    //        }

    //        if (item->GetSizer()->GetItemCount() > 2) // if there are Add/Del buttons
    //            for (size_t btn : {2, 3}) { // del_btn, add_btn
    //                wxSizerItem* b_item = item->GetSizer()->GetItem(btn);
    //                if (b_item->IsWindow()) {
    //                    auto button = dynamic_cast<PlusMinusButton*>(b_item->GetWindow());
    //                    if (button != nullptr)
    //                        button->msw_rescale();
    //                }
    //            }
    //    }
    //}
    if (m_Sizer)
        m_Sizer->Layout();
}

void AnkerObjectLayers::sys_color_changed()
{
    m_bmp_delete.sys_color_changed();
    m_bmp_add.sys_color_changed();

 /*
    // rescale edit-boxes
    const int cells_cnt = m_grid_sizer->GetCols() * m_grid_sizer->GetEffectiveRowsCount();
    for (int i = 0; i < cells_cnt; ++i) {
        const wxSizerItem* item = m_grid_sizer->GetItem(i);
        if (item->IsSizer()) {// case when we have editor with buttons
            for (size_t btn : {2, 3}) { // del_btn, add_btn
                wxSizerItem* b_item = item->GetSizer()->GetItem(btn);
                if (b_item->IsWindow()) {
                    auto button = dynamic_cast<PlusMinusButton*>(b_item->GetWindow());
                    if (button != nullptr)
                        button->sys_color_changed();
                }
            }
        }
    }

#ifdef _WIN32
    m_og->sys_color_changed();
    for (int i = 0; i < cells_cnt; ++i) {
        const wxSizerItem* item = m_grid_sizer->GetItem(i);
        if (item->IsWindow()) {
            if (AnkerLayerRangeEditor* editor = dynamic_cast<AnkerLayerRangeEditor*>(item->GetWindow()))
                wxGetApp().UpdateDarkUI(editor);
        }
        else if (item->IsSizer()) {// case when we have editor with buttons
            if (wxSizerItem* e_item = item->GetSizer()->GetItem(size_t(0)); e_item->IsWindow()) {
                if (AnkerLayerRangeEditor* editor = dynamic_cast<AnkerLayerRangeEditor*>(e_item->GetWindow()))
                    wxGetApp().UpdateDarkUI(editor);
            }
        }
    }
#endif
*/
}

void AnkerObjectLayers::reset_selection()
{

    m_selectable_range = { 0.0, 0.0 };
    m_top_layer_range = { 0.0, 0.0 };
    m_selection_type = Ak_etLayerHeight;
    m_ranges.clear();
}

AnkerLayerRangeEditor::AnkerLayerRangeEditor( AnkerObjectLayers* obj_layer,
                                    wxWindow* parent,
                                    t_layer_height_range* range,
                                    const wxString& value,
                                    AnkerEditorType type,
                                    std::function<void(AnkerEditorType)> set_focus_data_fn,
                                    std::function<bool(t_layer_height_range* ,coordf_t, bool, bool)>   edit_fn
                                    ) :
    m_valid_value(value),
    m_type(type),
    m_range(range),
    m_set_focus_data(set_focus_data_fn),
    wxTextCtrl(parent, wxID_ANY, value, wxDefaultPosition,
               wxSize(8 * em_unit(parent), wxDefaultCoord), wxTE_PROCESS_ENTER
#ifdef _WIN32
        | wxBORDER_SIMPLE
#else
        | wxBORDER_NONE
#endif
    )
{
    if (parent) {
        wxColour clr =  parent->GetBackgroundColour();
        this->SetBackgroundColour(clr);
    }
    this->SetForegroundColour(wxColour(255, 255, 255));
    this->SetFont(ANKER_FONT_NO_1);
    //wxGetApp().UpdateDarkUI(this);

    // Reset m_enter_pressed flag to _false_, when value is editing
    this->Bind(wxEVT_TEXT, [this](wxEvent&) { m_enter_pressed = false; }, this->GetId());
    
    this->Bind(wxEVT_TEXT_ENTER, [this, edit_fn](wxEvent&)
    {
        m_enter_pressed     = true;
        // Workaround! Under Linux we have to use CallAfter() to avoid crash after pressing ENTER key
        // see #7531, #8055, #8408
#ifdef __linux__
        wxTheApp->CallAfter([this, edit_fn]() {
#endif
            // If LayersList wasn't updated/recreated, we can call wxEVT_KILL_FOCUS.Skip()
            if (m_type & Ak_etLayerHeight) {
                if (!edit_fn(m_range, get_value(), true, false))
                    SetValue(m_valid_value);
                else
                    m_valid_value = double_to_string(get_value());
                m_call_kill_focus = true;
            }
            else if (!edit_fn(m_range, get_value(), true, false)) {
                SetValue(m_valid_value);
                m_call_kill_focus = true;
            }
#ifdef __linux__
        });
#endif 
    }, this->GetId());

    this->Bind(wxEVT_KILL_FOCUS, [this, edit_fn](wxFocusEvent& e)
    {
        if (!m_enter_pressed) {
#ifndef __WXGTK__
            /* Update data for next editor selection.
             * But under GTK it looks like there is no information about selected control at e.GetWindow(),
             * so we'll take it from wxEVT_LEFT_DOWN event
             * */
            AnkerLayerRangeEditor* new_editor = dynamic_cast<AnkerLayerRangeEditor*>(e.GetWindow());
            if (new_editor)
                new_editor->set_focus_data();
#endif // not __WXGTK__
            // If LayersList wasn't updated/recreated, we should call e.Skip()
            if (m_type & Ak_etLayerHeight) {
                if (!edit_fn(m_range, get_value(), false, dynamic_cast<AnkerObjectLayers::PlusMinusButton*>(e.GetWindow()) != nullptr))
                    SetValue(m_valid_value);
                else
                    m_valid_value = double_to_string(get_value());
                e.Skip();
            }
            else if (!edit_fn(m_range, get_value(), false, dynamic_cast<AnkerObjectLayers::PlusMinusButton*>(e.GetWindow()) != nullptr)) {
                SetValue(m_valid_value);
                e.Skip();
            } 
        }
        else if (m_call_kill_focus) {
            m_call_kill_focus = false;
            e.Skip();
        }
    }, this->GetId());

    this->Bind(wxEVT_SET_FOCUS, [this, obj_layer](wxFocusEvent& e)
    {
        set_focus_data();
        obj_layer->update_scene_from_editor_selection();
        e.Skip();
    }, this->GetId());

#ifdef __WXGTK__ // Workaround! To take information about selectable range
    this->Bind(wxEVT_LEFT_DOWN, [this](wxEvent& e)
    {
        set_focus_data();
        e.Skip();
    }, this->GetId());
#endif //__WXGTK__

    this->Bind(wxEVT_CHAR, ([this](wxKeyEvent& event)
    {
        // select all text using Ctrl+A
        if (wxGetKeyState(wxKeyCode('A')) && wxGetKeyState(WXK_CONTROL))
            this->SetSelection(-1, -1); //select all
        event.Skip();
    }));
}

coordf_t AnkerLayerRangeEditor::get_value()
{
    wxString str = GetValue();

    coordf_t layer_height;
    const char dec_sep = is_decimal_separator_point() ? '.' : ',';
    const char dec_sep_alt = dec_sep == '.' ? ',' : '.';
    // Replace the first incorrect separator in decimal number.
    str.Replace(dec_sep_alt, dec_sep, false);

    if (str == ".")
        layer_height = 0.0;
    else if (!str.ToDouble(&layer_height) || layer_height < 0.0f) {
        show_error(m_parent, _L("Invalid numeric input."));
        assert(m_valid_value.ToDouble(&layer_height));
    }

    return layer_height;
}

void AnkerLayerRangeEditor::msw_rescale()
{
    SetMinSize(wxSize(8 * wxGetApp().em_unit(), wxDefaultCoord));
}

} //namespace GUI
} //namespace Slic3r 

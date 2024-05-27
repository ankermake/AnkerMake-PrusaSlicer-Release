#include "GUI_ObjectLayers.hpp"
#include "GUI_ObjectList.hpp"

#include "OptionsGroup.hpp"
#include "GUI_App.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Model.hpp"
#include "GLCanvas3D.hpp"
#include "Plater.hpp"

#include <boost/algorithm/string.hpp>
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"

#include "I18N.hpp"

#include <wx/wupdlock.h>

namespace Slic3r
{
namespace GUI
{

ObjectLayers::ObjectLayers(wxWindow* parent) :
    OG_Settings(parent, true)
{
    m_grid_sizer = new wxFlexGridSizer(3, 5, wxGetApp().em_unit()); // "Min Z", "Max Z", "Layer height" & buttons sizer
    m_grid_sizer->SetFlexibleDirection(wxHORIZONTAL);

    //// Legend for object layers
    //for (const std::string col : { L("Start at height"), L("Stop at height"), L("Layer height") }) {
    //    auto temp = new wxStaticText(m_parent, wxID_ANY, _(col), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
    //    temp->SetBackgroundStyle(wxBG_STYLE_PAINT);
    //    temp->SetFont(wxGetApp().bold_font());

    //    m_grid_sizer->Add(temp);
    //}

    m_og->activate();
    m_og->sizer->Clear(true);
    m_og->sizer->Add(m_grid_sizer, 0, wxEXPAND | wxALL, wxOSX ? 0 : 5);

    m_bmp_delete    = ScalableBitmap(parent, "remove_copies"/*"cross"*/);
    m_bmp_add       = ScalableBitmap(parent, "add_copies");
}

void ObjectLayers::select_editor(LayerRangeEditor* editor, const bool is_last_edited_range)
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

#if 1
wxSizer* ObjectLayers::create_layer(const t_layer_height_range& range, PlusMinusButton* delete_button, PlusMinusButton* add_button)
{
    const bool is_last_edited_range = range == m_selectable_range;

    auto set_focus_data = [range, this](const EditorType type)
        {
            m_selectable_range = range;
            m_selection_type = type;
        };

    auto update_focus_data = [range, this](const t_layer_height_range& new_range, EditorType type, bool enter_pressed)
        {
            // change selectable range for new one, if enter was pressed or if same range was selected
            if (enter_pressed || m_selectable_range == range)
                m_selectable_range = new_range;
            if (enter_pressed)
                m_selection_type = type;
        };

    // Add text
    auto head_text = new wxStaticText(m_parent, wxID_ANY, _L("Height Range"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    head_text->SetMaxSize({ 12 * wxGetApp().em_unit(), -1 });
    //head_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    head_text->SetForegroundColour(wxColour("#FFFFFF"));
    head_text->SetBackgroundColour(wxColour("#292A2D"));
    head_text->SetFont(wxGetApp().normal_font());
    m_grid_sizer->Add(head_text, 0, wxALIGN_CENTER_VERTICAL, wxGetApp().em_unit());

    // Add control for the "Min Z"

    auto editor = new LayerRangeEditor(this, double_to_string(range.first), etMinZ, set_focus_data,
        [range, update_focus_data, this, delete_button, add_button](coordf_t min_z, bool enter_pressed, bool dont_update_ui)
        {
            if (fabs(min_z - range.first) < EPSILON) {
                m_selection_type = etUndef;
                return false;
            }

            // data for next focusing
            coordf_t max_z = min_z < range.second ? range.second : min_z + 0.5;
            const t_layer_height_range new_range = { min_z, max_z };
            if (delete_button)
                delete_button->range = new_range;
            if (add_button)
                add_button->range = new_range;
            update_focus_data(new_range, etMinZ, enter_pressed);

            return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
        });

    select_editor(editor, is_last_edited_range);

    auto sizer1 = new wxBoxSizer(wxHORIZONTAL);
    sizer1->Add(editor, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());
    auto middle_text = new wxStaticText(m_parent, wxID_ANY, _L("to"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    middle_text->SetMaxSize({ 4 * wxGetApp().em_unit(), -1 });
    //middle_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    middle_text->SetForegroundColour(wxColour("#FFFFFF"));
    middle_text->SetBackgroundColour(wxColour("#292A2D"));
    middle_text->SetFont(wxGetApp().normal_font());
    sizer1->Add(middle_text, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());

    m_grid_sizer->Add(sizer1);

    // Add control for the "Max Z"

    editor = new LayerRangeEditor(this, double_to_string(range.second), etMaxZ, set_focus_data,
        [range, update_focus_data, this, delete_button, add_button](coordf_t max_z, bool enter_pressed, bool dont_update_ui)
        {
            if (fabs(max_z - range.second) < EPSILON || range.first > max_z) {
                m_selection_type = etUndef;
                return false;       // LayersList would not be updated/recreated
            }

            // data for next focusing
            const t_layer_height_range& new_range = { range.first, max_z };
            if (delete_button)
                delete_button->range = new_range;
            if (add_button)
                add_button->range = new_range;
            update_focus_data(new_range, etMaxZ, enter_pressed);

            return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
        });

    //select_editor(editor, is_last_edited_range);

    auto sizer2 = new wxBoxSizer(wxHORIZONTAL);
    sizer2->Add(editor);
    auto unit_text = new wxStaticText(m_parent, wxID_ANY, _L("mm"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    //unit_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    unit_text->SetForegroundColour(wxColour("#FFFFFF"));
    unit_text->SetBackgroundColour(wxColour("#292A2D"));
    unit_text->SetFont(wxGetApp().normal_font());
    sizer2->Add(unit_text, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());

    m_grid_sizer->Add(sizer2);

    // Add control for the "Layer height"

    //editor = new LayerRangeEditor(this, double_to_string(m_object->layer_config_ranges[range].option("layer_height")->getFloat()), etLayerHeight, set_focus_data,
    //    [range](coordf_t layer_height, bool, bool)
    //{
    //    return wxGetApp().obj_list()->edit_layer_range(range, layer_height);
    //});

    //select_editor(editor, is_last_edited_range);

    //auto sizer = new wxBoxSizer(wxHORIZONTAL);
    //sizer->Add(editor);

    //auto temp = new wxStaticText(m_parent, wxID_ANY, _L("mm"));
    //temp->SetBackgroundStyle(wxBG_STYLE_PAINT);
    //temp->SetFont(wxGetApp().normal_font());
    //sizer->Add(temp, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, wxGetApp().em_unit());

    //m_grid_sizer->Add(sizer);

    return sizer2;
}

#else
wxSizer* ObjectLayers::create_layer(const t_layer_height_range& range, PlusMinusButton *delete_button, PlusMinusButton *add_button) 
{
    const bool is_last_edited_range = range == m_selectable_range;

    auto set_focus_data = [range, this](const EditorType type)
    {
        m_selectable_range = range;
        m_selection_type = type;
    };

    auto update_focus_data = [range, this](const t_layer_height_range& new_range, EditorType type, bool enter_pressed)
    {
        // change selectable range for new one, if enter was pressed or if same range was selected
        if (enter_pressed || m_selectable_range == range)
            m_selectable_range = new_range;
        if (enter_pressed)
            m_selection_type = type;
    };

    // Add control for the "Min Z"

    auto editor = new LayerRangeEditor(this, double_to_string(range.first), etMinZ, set_focus_data, 
        [range, update_focus_data, this, delete_button, add_button](coordf_t min_z, bool enter_pressed, bool dont_update_ui) 
    {
        if (fabs(min_z - range.first) < EPSILON) {
            m_selection_type = etUndef;
            return false;
        }

        // data for next focusing
        coordf_t max_z = min_z < range.second ? range.second : min_z + 0.5;
        const t_layer_height_range new_range = { min_z, max_z };
        if (delete_button)
            delete_button->range = new_range;
        if (add_button)
            add_button->range = new_range;
        update_focus_data(new_range, etMinZ, enter_pressed);

        return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
    });

    select_editor(editor, is_last_edited_range);
    m_grid_sizer->Add(editor);

    // Add control for the "Max Z"

    editor = new LayerRangeEditor(this, double_to_string(range.second), etMaxZ, set_focus_data, 
        [range, update_focus_data, this, delete_button, add_button](coordf_t max_z, bool enter_pressed, bool dont_update_ui)
    {
        if (fabs(max_z - range.second) < EPSILON || range.first > max_z) {
            m_selection_type = etUndef;
            return false;       // LayersList would not be updated/recreated
        }

        // data for next focusing
        const t_layer_height_range& new_range = { range.first, max_z };
        if (delete_button)
            delete_button->range = new_range;
        if (add_button)
            add_button->range = new_range;
        update_focus_data(new_range, etMaxZ, enter_pressed);

        return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
    });

    select_editor(editor, is_last_edited_range);
    m_grid_sizer->Add(editor);

    // Add control for the "Layer height"

    editor = new LayerRangeEditor(this, double_to_string(m_object->layer_config_ranges[range].option("layer_height")->getFloat()), etLayerHeight, set_focus_data,
        [range](coordf_t layer_height, bool, bool)
    {
        return wxGetApp().obj_list()->edit_layer_range(range, layer_height);
    });

    select_editor(editor, is_last_edited_range);

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(editor);

    auto temp = new wxStaticText(m_parent, wxID_ANY, _L("mm"));
    temp->SetBackgroundStyle(wxBG_STYLE_PAINT);
    temp->SetFont(wxGetApp().normal_font());
    sizer->Add(temp, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, wxGetApp().em_unit());

    m_grid_sizer->Add(sizer);

    return sizer;
}
#endif    

void ObjectLayers::create_layers_list()
{
    for (const auto &layer : m_object->layer_config_ranges) {
        const t_layer_height_range& range = layer.first;
        auto del_btn = new PlusMinusButton(m_parent, m_bmp_delete, range);
        del_btn->SetToolTip(_L("Remove layer range"));

        auto add_btn = new PlusMinusButton(m_parent, m_bmp_add, range);
        wxString tooltip = wxGetApp().obj_list()->can_add_new_range_after_current(range);
        add_btn->SetToolTip(tooltip.IsEmpty() ? _L("Add layer range") : tooltip);
        add_btn->Enable(tooltip.IsEmpty());

        auto sizer = create_layer(range, del_btn, add_btn);
        sizer->Add(del_btn, 0, wxRIGHT | wxLEFT, em_unit(m_parent));
        sizer->Add(add_btn);

        del_btn->Bind(wxEVT_BUTTON, [del_btn](wxEvent &) {
            wxGetApp().obj_list()->del_layer_range(del_btn->range);
        });

        add_btn->Bind(wxEVT_BUTTON, [add_btn](wxEvent &) {
            wxGetApp().obj_list()->add_layer_range_after_current(add_btn->range);
        });
    }
}


#if 1
void ObjectLayers::update_layers_list()
{
    ObjectList* objects_ctrl = wxGetApp().obj_list();
    if (objects_ctrl->multiple_selection()) return;

    const auto item = objects_ctrl->GetSelection();
    if (!item) return;

    const int obj_idx = objects_ctrl->get_selected_obj_idx();
    if (obj_idx < 0) return;

    const ItemType type = objects_ctrl->GetModel()->GetItemType(item);
    if (!(type & (itLayerRoot | itLayer))) return;

    m_object = objects_ctrl->object(obj_idx);
    if (!m_object || m_object->layer_config_ranges.empty()) return;

    auto range = objects_ctrl->GetModel()->GetLayerRangeByItem(item);

    // only call sizer->Clear(true) via CallAfter, otherwise crash happens in Linux when press enter in Height Range
    // because an element cannot be destroyed while there are pending events for this element.(https://github.com/wxWidgets/Phoenix/issues/1854)
    wxGetApp().CallAfter([this, type, objects_ctrl, range]() {
        // Delete all controls from options group
        m_grid_sizer->Clear(true);

        // Add new control according to the selected item  

        if (type & itLayerRoot)
            create_layers_list();
        else
            create_layer(range, nullptr, nullptr);

        m_parent->Layout();
        });
}
#else
void ObjectLayers::update_layers_list()
{
    ObjectList* objects_ctrl   = wxGetApp().obj_list();
    if (objects_ctrl->multiple_selection()) return;

    const auto item = objects_ctrl->GetSelection();
    if (!item) return;

    const int obj_idx = objects_ctrl->get_selected_obj_idx();
    if (obj_idx < 0) return;

    const ItemType type = objects_ctrl->GetModel()->GetItemType(item);
    if (!(type & (itLayerRoot | itLayer))) return;

    m_object = objects_ctrl->object(obj_idx);
    if (!m_object || m_object->layer_config_ranges.empty()) return;

    // Delete all controls from options group except of the legends

    const int cols = m_grid_sizer->GetEffectiveColsCount();
    const int rows = m_grid_sizer->GetEffectiveRowsCount();
    for (int idx = cols*rows-1; idx >= cols; idx--) {
        wxSizerItem* t = m_grid_sizer->GetItem(idx);
        if (t->IsSizer())
            t->GetSizer()->Clear(true);
        else
            t->DeleteWindows();
        m_grid_sizer->Remove(idx);
    }

    // Add new control according to the selected item  

    if (type & itLayerRoot)
        create_layers_list();
    else
        create_layer(objects_ctrl->GetModel()->GetLayerRangeByItem(item), nullptr, nullptr);

    m_parent->Layout();
}
#endif

void ObjectLayers::update_scene_from_editor_selection() const
{
    // needed to show the visual hints in 3D scene
    wxGetApp().plater()->canvas3D()->handle_layers_data_focus_event(m_selectable_range, m_selection_type);
}

void ObjectLayers::UpdateAndShow(const bool show)
{
    if (show)
        update_layers_list();

    //OG_Settings::UpdateAndShow(show);
}

void ObjectLayers::msw_rescale()
{
    m_bmp_delete.msw_rescale();
    m_bmp_add.msw_rescale();

    m_grid_sizer->SetHGap(wxGetApp().em_unit());

    // rescale edit-boxes
    const int cells_cnt = m_grid_sizer->GetCols() * m_grid_sizer->GetEffectiveRowsCount();
    for (int i = 0; i < cells_cnt; ++i) {
        const wxSizerItem* item = m_grid_sizer->GetItem(i);
        if (item->IsWindow()) {
            LayerRangeEditor* editor = dynamic_cast<LayerRangeEditor*>(item->GetWindow());
            if (editor != nullptr)
                editor->msw_rescale();
        }
        else if (item->IsSizer()) // case when we have editor with buttons
        {
            wxSizerItem* e_item = item->GetSizer()->GetItem(size_t(0)); // editor
            if (e_item->IsWindow()) {
                LayerRangeEditor* editor = dynamic_cast<LayerRangeEditor*>(e_item->GetWindow());
                if (editor != nullptr)
                    editor->msw_rescale();
            }

            if (item->GetSizer()->GetItemCount() > 2) // if there are Add/Del buttons
                for (size_t btn : {2, 3}) { // del_btn, add_btn
                    wxSizerItem* b_item = item->GetSizer()->GetItem(btn);
                    if (b_item->IsWindow()) {
                        auto button = dynamic_cast<PlusMinusButton*>(b_item->GetWindow());
                        if (button != nullptr)
                            button->msw_rescale();
                    }
                }
        }
    }
    m_grid_sizer->Layout();
}

void ObjectLayers::sys_color_changed()
{
    m_bmp_delete.sys_color_changed();
    m_bmp_add.sys_color_changed();

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
            if (LayerRangeEditor* editor = dynamic_cast<LayerRangeEditor*>(item->GetWindow()))
                wxGetApp().UpdateDarkUI(editor);
        }
        else if (item->IsSizer()) {// case when we have editor with buttons
            if (wxSizerItem* e_item = item->GetSizer()->GetItem(size_t(0)); e_item->IsWindow()) {
                if (LayerRangeEditor* editor = dynamic_cast<LayerRangeEditor*>(e_item->GetWindow()))
                    wxGetApp().UpdateDarkUI(editor);
            }
        }
    }
#endif

}

void ObjectLayers::reset_selection()
{
    m_selectable_range = { 0.0, 0.0 };
    m_selection_type = etLayerHeight;
}

LayerRangeEditor::LayerRangeEditor( ObjectLayers* parent,
                                    const wxString& value,
                                    EditorType type,
                                    std::function<void(EditorType)> set_focus_data_fn,
                                    std::function<bool(coordf_t, bool, bool)>   edit_fn
                                    ) :
    m_valid_value(value),
    m_type(type),
    m_set_focus_data(set_focus_data_fn),
    wxTextCtrl(parent->m_parent, wxID_ANY, value, wxDefaultPosition, 
               wxSize(8 * em_unit(parent->m_parent), wxDefaultCoord), wxTE_PROCESS_ENTER
#ifdef _WIN32
        | wxBORDER_SIMPLE
#endif
    )
{
    this->SetFont(wxGetApp().normal_font());
    wxGetApp().UpdateDarkUI(this);

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
            if (m_type & etLayerHeight) {
                if (!edit_fn(get_value(), true, false))
                    SetValue(m_valid_value);
                else
                    m_valid_value = double_to_string(get_value());
                m_call_kill_focus = true;
            }
            else if (!edit_fn(get_value(), true, false)) {
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
            LayerRangeEditor* new_editor = dynamic_cast<LayerRangeEditor*>(e.GetWindow());
            if (new_editor)
                new_editor->set_focus_data();
#endif // not __WXGTK__
            // If LayersList wasn't updated/recreated, we should call e.Skip()
            if (m_type & etLayerHeight) {
                if (!edit_fn(get_value(), false, dynamic_cast<ObjectLayers::PlusMinusButton*>(e.GetWindow()) != nullptr))
                    SetValue(m_valid_value);
                else
                    m_valid_value = double_to_string(get_value());
                e.Skip();
            }
            else if (!edit_fn(get_value(), false, dynamic_cast<ObjectLayers::PlusMinusButton*>(e.GetWindow()) != nullptr)) {
                SetValue(m_valid_value);
                e.Skip();
            } 
        }
        else if (m_call_kill_focus) {
            m_call_kill_focus = false;
            e.Skip();
        }
    }, this->GetId());

    this->Bind(wxEVT_SET_FOCUS, [this, parent](wxFocusEvent& e)
    {
        set_focus_data();
        parent->update_scene_from_editor_selection();
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

coordf_t LayerRangeEditor::get_value()
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

void LayerRangeEditor::msw_rescale()
{
    SetMinSize(wxSize(8 * wxGetApp().em_unit(), wxDefaultCoord));
}



/// <summary>
/// AnkerObjectLayerEditor
/// </summary>
/// <param name="parent"></param>

AnkerObjectLayerRangeEditor::AnkerObjectLayerRangeEditor(AnkerObjectLayerEditor* obj_layer,
    wxWindow* parent,
    const wxString& value,
    EditorType type,
    std::function<void(EditorType)> set_focus_data_fn,
    std::function<bool(coordf_t, bool, bool)>   edit_fn
) :
    m_valid_value(value),
    m_type(type),
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
        wxColour clr = parent->GetBackgroundColour();
        this->SetBackgroundColour(clr);
    }
    this->SetForegroundColour(wxColour(255, 255, 255));
    this->SetFont(ANKER_FONT_NO_1);
    //wxGetApp().UpdateDarkUI(this);

    // Reset m_enter_pressed flag to _false_, when value is editing
    this->Bind(wxEVT_TEXT, [this](wxEvent&) { m_enter_pressed = false; }, this->GetId());

    this->Bind(wxEVT_TEXT_ENTER, [this, edit_fn](wxEvent&)
        {
            m_enter_pressed = true;
            // If LayersList wasn't updated/recreated, we can call wxEVT_KILL_FOCUS.Skip()
            if (m_type & etLayerHeight) {
                if (!edit_fn(get_value(), true, false))
                    SetValue(m_valid_value);
                else
                    m_valid_value = double_to_string(get_value());
                m_call_kill_focus = true;
            }
            else if (!edit_fn(get_value(), true, false)) {
                SetValue(m_valid_value);
                m_call_kill_focus = true;
            }
        }, this->GetId());

    this->Bind(wxEVT_KILL_FOCUS, [this, edit_fn](wxFocusEvent& e)
        {
            if (!m_enter_pressed) {
#ifndef __WXGTK__
                /* Update data for next editor selection.
                 * But under GTK it looks like there is no information about selected control at e.GetWindow(),
                 * so we'll take it from wxEVT_LEFT_DOWN event
                 * */
                AnkerObjectLayerRangeEditor* new_editor = dynamic_cast<AnkerObjectLayerRangeEditor*>(e.GetWindow());
                if (new_editor)
                    new_editor->set_focus_data();
#endif // not __WXGTK__
                // If LayersList wasn't updated/recreated, we should call e.Skip()
                if (m_type & etLayerHeight) {
                    if (!edit_fn(get_value(), false, dynamic_cast<AnkerObjectLayerEditor::PlusMinusButton*>(e.GetWindow()) != nullptr))
                        SetValue(m_valid_value);
                    else
                        m_valid_value = double_to_string(get_value());
                    e.Skip();
                }
                else if (!edit_fn(get_value(), false, dynamic_cast<AnkerObjectLayerEditor::PlusMinusButton*>(e.GetWindow()) != nullptr)) {
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


coordf_t AnkerObjectLayerRangeEditor::get_value()
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

void AnkerObjectLayerRangeEditor::msw_rescale()
{
    SetMinSize(wxSize(8 * wxGetApp().em_unit(), wxDefaultCoord));
}

AnkerObjectLayerEditor::AnkerObjectLayerEditor(wxWindow* parent)
{
    m_bmp_delete = ScalableBitmap(parent, "remove_copies"/*"cross"*/);
    m_bmp_add = ScalableBitmap(parent, "add_copies");
}

void AnkerObjectLayerEditor::select_editor(AnkerObjectLayerRangeEditor* editor, const bool is_last_edited_range)
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
            //editor->SetFocus();
            //editor->SetInsertionPointEnd();
#ifdef __WXOSX__
            });
#endif
    }
}

wxSizer* AnkerObjectLayerEditor::create_layer(const t_layer_height_range& range, PlusMinusButton* delete_button, PlusMinusButton* add_button)
{
    const bool is_last_edited_range = range == m_selectable_range;

    auto set_focus_data = [range, this](const EditorType type)
        {
            m_selectable_range = range;
            m_selection_type = type;
        };

    auto update_focus_data = [range, this](const t_layer_height_range& new_range, EditorType type, bool enter_pressed)
        {
            // change selectable range for new one, if enter was pressed or if same range was selected
            if (enter_pressed || m_selectable_range == range)
                m_selectable_range = new_range;
            if (enter_pressed)
                m_selection_type = type;
        };

    wxWindow* parent_Panel = m_contentPanel;
    wxBoxSizer* hor_Sizer = new wxBoxSizer(wxHORIZONTAL);
    m_contentPanelSizer->Add(hor_Sizer, 0, wxEXPAND | wxALL, 0);
    hor_Sizer->AddSpacer(15);

    // Add text
    auto head_text = new wxStaticText(parent_Panel, wxID_ANY, _L("common_slice_toolpannel_heightranges"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    head_text->SetForegroundColour(wxColour("#FFFFFF"));
   // head_text->SetBackgroundColour(wxColour("#202124"));
    head_text->SetMaxSize({ 12 * wxGetApp().em_unit(), -1 });
    head_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    head_text->SetFont(wxGetApp().normal_font());
    hor_Sizer->Add(head_text, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);
    
    // Add control for the "Min Z"
    auto editor = new AnkerObjectLayerRangeEditor(this, parent_Panel, double_to_string(range.first), etMinZ, set_focus_data,
        [range, update_focus_data, this, delete_button, add_button](coordf_t min_z, bool enter_pressed, bool dont_update_ui)
        {
            if (fabs(min_z - range.first) < EPSILON) {
                m_selection_type = etUndef;
                return false;
            }

            // data for next focusing
            coordf_t max_z = min_z < range.second ? range.second : min_z + 0.5;
            const t_layer_height_range new_range = { min_z, max_z };
            if (delete_button)
                delete_button->range = new_range;
            if (add_button)
                add_button->range = new_range;
            update_focus_data(new_range, etMinZ, enter_pressed);

            return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
        });

    select_editor(editor, is_last_edited_range);

    auto sizer1 = new wxBoxSizer(wxHORIZONTAL);
    sizer1->Add(editor, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());
    auto middle_text = new wxStaticText(parent_Panel, wxID_ANY, _L("to"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    middle_text->SetMaxSize({ 4 * wxGetApp().em_unit(), -1 });
    //middle_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    middle_text->SetForegroundColour(wxColour("#FFFFFF"));
    middle_text->SetBackgroundColour(wxColour("#292A2D"));
    middle_text->SetFont(wxGetApp().normal_font());
    sizer1->AddSpacer(5);
    sizer1->Add(middle_text, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());

    hor_Sizer->AddSpacer(10);
    hor_Sizer->Add(sizer1, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);
    hor_Sizer->AddSpacer(10);
    // Add control for the "Max Z"

    editor = new AnkerObjectLayerRangeEditor(this, parent_Panel, double_to_string(range.second), etMaxZ, set_focus_data,
        [range, update_focus_data, this, delete_button, add_button](coordf_t max_z, bool enter_pressed, bool dont_update_ui)
        {
            if (fabs(max_z - range.second) < EPSILON || range.first > max_z) {
                m_selection_type = etUndef;
                return false;       // LayersList would not be updated/recreated
            }

            // data for next focusing
            const t_layer_height_range& new_range = { range.first, max_z };
            if (delete_button)
                delete_button->range = new_range;
            if (add_button)
                add_button->range = new_range;
            update_focus_data(new_range, etMaxZ, enter_pressed);

            return wxGetApp().obj_list()->edit_layer_range(range, new_range, dont_update_ui);
        });

    //select_editor(editor, is_last_edited_range);

    auto sizer2 = new wxBoxSizer(wxHORIZONTAL);
    sizer2->Add(editor);
    auto unit_text = new wxStaticText(parent_Panel, wxID_ANY, _L("mm"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    //unit_text->SetBackgroundStyle(wxBG_STYLE_PAINT);
    unit_text->SetForegroundColour(wxColour("#FFFFFF"));
    unit_text->SetBackgroundColour(wxColour("#292A2D"));
    unit_text->SetFont(wxGetApp().normal_font());
    sizer2->Add(unit_text, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0.5 * wxGetApp().em_unit());

    hor_Sizer->Add(sizer2, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);
    return sizer2;
}

void AnkerObjectLayerEditor::create_layers_list()
{
    wxWindow* theParent = m_contentPanel;
    for (const auto& layer : m_object->layer_config_ranges) {
        const t_layer_height_range& range = layer.first;
        auto del_btn = new PlusMinusButton(theParent, m_bmp_delete, range);
        del_btn->SetToolTip(_L("Remove layer range"));

        auto add_btn = new PlusMinusButton(theParent, m_bmp_add, range);
        wxString tooltip = wxGetApp().obj_list()->can_add_new_range_after_current(range);
        add_btn->SetToolTip(tooltip.IsEmpty() ? _L("Add layer range") : tooltip);
        add_btn->Enable(tooltip.IsEmpty());

        auto sizer = create_layer(range, del_btn, add_btn);
        sizer->Add(del_btn, 0, wxRIGHT | wxLEFT, em_unit(theParent));
        sizer->Add(add_btn);
        sizer->Layout();
        del_btn->Bind(wxEVT_BUTTON, [del_btn](wxEvent&) {
            wxGetApp().obj_list()->del_layer_range(del_btn->range);
            });

        add_btn->Bind(wxEVT_BUTTON, [add_btn](wxEvent&) {
            wxGetApp().obj_list()->add_layer_range_after_current(add_btn->range);
            });
    }
}

void AnkerObjectLayerEditor::update_layers_list()
{
    ObjectList* objects_ctrl = wxGetApp().obj_list();
    if (objects_ctrl->multiple_selection()) return;

    const auto item = objects_ctrl->GetSelection();
    if (!item) return;

    const int obj_idx = objects_ctrl->get_selected_obj_idx();
    if (obj_idx < 0) return;

    const ItemType type = objects_ctrl->GetModel()->GetItemType(item);
    if (!(type & (itLayerRoot | itLayer))) return;

    m_object = objects_ctrl->object(obj_idx);
    if (!m_object || m_object->layer_config_ranges.empty()) return;

    auto range = objects_ctrl->GetModel()->GetLayerRangeByItem(item);

    // only call sizer->Clear(true) via CallAfter, otherwise crash happens in Linux when press enter in Height Range
    // because an element cannot be destroyed while there are pending events for this element.(https://github.com/wxWidgets/Phoenix/issues/1854)
    wxGetApp().CallAfter([this, type, objects_ctrl, range]() {
        // Delete all controls from options group
        //m_grid_sizer->Clear(true);
        // Add new control according to the selected item  

        if (type & itLayerRoot)
            create_layers_list();
        else
            create_layer(range, nullptr, nullptr);

        m_parent->Layout();
    });
}

void AnkerObjectLayerEditor::update_scene_from_editor_selection() const
{
    // needed to show the visual hints in 3D scene
    wxGetApp().plater()->canvas3D()->handle_layers_data_focus_event(m_selectable_range, m_selection_type);
}

void AnkerObjectLayerEditor::UpdateAndShow(const bool show)
{
    std::string panelFlag = "Layer";

    if (!show) {
        wxGetApp().plater()->sidebarnew().detach_layer_height_sizer();
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
        return;
    }

    AnkerSidebarNew* rightSidebar = &(wxGetApp().plater()->sidebarnew());
    if (!rightSidebar) return;
    wxWindow* ItemParameterPanel = rightSidebar->GetItemParameterPanel();
    if (!ItemParameterPanel) return;

    if (!m_parent) m_parent = ItemParameterPanel;
    if (!m_Sizer)  m_Sizer = new wxBoxSizer(wxVERTICAL);
    m_parent->Freeze();
    
    if(!m_panel) {
        auto container = new wxPanel(ItemParameterPanel);
        container->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        auto verSize = new wxBoxSizer(wxVERTICAL);
        container->SetSizer(verSize);
        m_Sizer->Add(container, 1, wxEXPAND | wxALL, 0);

        scrollWin = new wxScrolledWindow(container);
        //scrollWin->SetScrollRate(0, 30);
        scrollWin->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 40));
        scrollWin->SetScrollbars(0, 40, 300 / 50, 500 / 50);
        m_contentPanel = scrollWin;

        scrollWin->SetBackgroundColour(container->GetBackgroundColour());
        m_contentPanelSizer = new wxBoxSizer(wxVERTICAL);
        scrollWin->SetSizer(m_contentPanelSizer);

        auto split_line_top = new wxPanel(container, wxID_ANY);
        split_line_top->SetBackgroundColour(wxColour("#38393C"));
        split_line_top->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 1));
        
        auto split_line_bottom = new wxPanel(container, wxID_ANY);
        split_line_bottom->SetBackgroundColour(wxColour("#38393C"));
        split_line_bottom->SetMinSize(AnkerSize(PARAMETER_PANEL_WITH, 1));

        verSize->Add(split_line_top, 0, wxEXPAND, 0);
        verSize->Add(scrollWin, 1, wxEXPAND | wxALL, 0);
        verSize->Add(split_line_bottom, 0, wxEXPAND, 0);
        m_panel = container;
    }

    if (m_contentPanelSizer)
        m_contentPanelSizer->Clear(true);


    update_layers_list();
    show_layer_height();
    m_parent->Layout();
    m_parent->Thaw();
}

void AnkerObjectLayerEditor::show_layer_height()
{
    ObjectList* objects_ctrl = wxGetApp().obj_list();
    if (objects_ctrl->multiple_selection()) return;

    const auto item = objects_ctrl->GetSelection();
    if (!item) return;

    const int obj_idx = objects_ctrl->get_selected_obj_idx();
    if (obj_idx < 0) return;

    const ItemType type = objects_ctrl->GetModel()->GetItemType(item);
    if (!(type & (itLayerRoot | itLayer))) return;
    
    if (type & itLayerRoot) {
        wxGetApp().plater()->sidebarnew().set_layer_height_sizer(m_Sizer, true);
    }
    else if (type & itLayer) {
        wxGetApp().plater()->sidebarnew().set_layer_height_sizer(m_Sizer, false);
    }
}

void AnkerObjectLayerEditor::msw_rescale()
{
    if (m_Sizer)
        m_Sizer->Layout();
}
void AnkerObjectLayerEditor::sys_color_changed()
{
    m_bmp_delete.sys_color_changed();
    m_bmp_add.sys_color_changed();
}

void AnkerObjectLayerEditor::reset_selection()
{
    m_selectable_range = { 0.0, 0.0 };
    m_selection_type = etLayerHeight;
}


} //namespace GUI
} //namespace Slic3r 

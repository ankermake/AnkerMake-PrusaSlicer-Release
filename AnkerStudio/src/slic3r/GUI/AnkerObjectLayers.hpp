#ifndef slic3r_Anker_ObjectLayers_hpp_
#define slic3r_Anker_ObjectLayers_hpp_

#include "GUI_ObjectSettings.hpp"
#include "wxExtensions.hpp"

#ifdef __WXOSX__
#include "../libslic3r/PrintConfig.hpp"
#endif

class wxBoxSizer;

namespace Slic3r {
class ModelObject;

namespace GUI {
class ConfigOptionsGroup;

typedef double                          coordf_t;
typedef std::pair<coordf_t, coordf_t>   t_layer_height_range;

class AnkerObjectLayers;

enum AnkerEditorType
{
    Ak_etUndef         = 0,
    Ak_etMinZ          = 1,
    Ak_etMaxZ          = 2,
    Ak_etLayerHeight   = 4,
};

class AnkerLayerRangeEditor : public wxTextCtrl
{
    bool                m_enter_pressed     { false };
    bool                m_call_kill_focus   { false };
    wxString            m_valid_value;
    AnkerEditorType          m_type;

    std::function<void(AnkerEditorType)> m_set_focus_data;

public:
    AnkerLayerRangeEditor(   AnkerObjectLayers* objLayer,
                        wxWindow* parent,
                        const wxString& value = wxEmptyString,
                        AnkerEditorType type = Ak_etUndef,
                        std::function<void(AnkerEditorType)>     set_focus_data_fn   = [](AnkerEditorType)      {;},
                        // callback parameters: new value, from enter, dont't update panel UI (when called from edit field's kill focus handler for the PlusMinusButton)
                        std::function<bool(coordf_t, bool, bool)> edit_fn       = [](coordf_t, bool, bool) {return false; }
                        );
    ~AnkerLayerRangeEditor() {}

    AnkerEditorType          type() const {return m_type;}
    void                set_focus_data() const { m_set_focus_data(m_type);}
    void                msw_rescale();

private:
    coordf_t            get_value();
};

class AnkerObjectLayers // : public OG_Settings
{
    ScalableBitmap  m_bmp_delete;
    ScalableBitmap  m_bmp_add;
    ModelObject*    m_object {nullptr};

    //wxFlexGridSizer*       m_grid_sizer;
    t_layer_height_range   m_selectable_range;
    AnkerEditorType             m_selection_type {Ak_etUndef};

    t_layer_height_range m_top_layer_range;
    
    int m_rangeCnt;

    wxWindow* m_parent{ nullptr };
    wxBoxSizer* m_Sizer{ nullptr };
    wxPanel* m_panel{ nullptr };
    wxPanel* m_contentPanel{ nullptr };
    wxBoxSizer* m_contentPanelSizer{nullptr};

    wxScrolledWindow* scrollWin{ nullptr };
public:
    AnkerObjectLayers(wxWindow* parent);
    ~AnkerObjectLayers() {}

    
    // Button remembers the layer height range, for which it has been created.
    // The layer height range for this button is updated when the low or high boundary of the layer height range is updated
    // by the respective text edit field, so that this button emits an action for an up to date layer height range value.
    class PlusMinusButton : public ScalableButton
    {
    public:
        PlusMinusButton(wxWindow *parent, const ScalableBitmap &bitmap, std::pair<coordf_t, coordf_t> range) : ScalableButton(parent, wxID_ANY, bitmap), range(range) {}
        // updated when the text edit field loses focus for any PlusMinusButton.
        std::pair<coordf_t, coordf_t> range;
    };

    void        select_editor(AnkerLayerRangeEditor* editor, const bool is_last_edited_range);
    // Create sizer with layer height range and layer height text edit fields, without buttons.
    // If the delete and add buttons are provided, the respective text edit fields will modify the layer height ranges of thes buttons
    // on value change, so that these buttons work with up to date values.
    wxSizer*    create_layer(const t_layer_height_range& range, PlusMinusButton *delete_button, PlusMinusButton *add_button);
    void        create_layers_list();
    void        update_layers_list();

    void        update_scene_from_editor_selection() const;

    void        UpdateAndShow(const bool show);
    void        msw_rescale();
    void        sys_color_changed();
    void        reset_selection();
    void        set_selectable_range(const t_layer_height_range& range) { m_selectable_range = range; }

    void AddLayerRange();

    friend class AnkerLayerRangeEditor;
};

}}

#endif // slic3r_Anker_ObjectLayers_hpp_

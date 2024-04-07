#ifndef _ANKER_OBJECT_BAR_H_
#define _ANKER_OBJECT_BAR_H_

#include "wx/wx.h"
#include "AnkerObjectBarView.hpp"
#include "libslic3r/Model.hpp"


namespace Slic3r
{
    namespace GUI
    {
        class Plater;
        class ObjectList;
    }

    class ModelConfig;
    class ModelObject;
    class ModelVolume;
}

typedef double  coordf_t;
typedef std::pair<coordf_t, coordf_t> t_layer_height_range;
typedef std::map<t_layer_height_range, Slic3r::ModelConfig> t_layer_config_ranges;

class AnkerObjectItem
{
public:
    enum ItemType {
        ITYPE_OBJECT = 0,
        ITYPE_VOLUME,
        ITYPE_INSTANCE,
        ITYPE_INSTANCE_GROUP,
        ITYPE_LAYER,
        ITYPE_LAYER_GROUP,
        ITYPE_SEAM,
        ITYPE_SUPPORT,
        ITYPE_COUNT,
    };

public:
    AnkerObjectItem();
    ~AnkerObjectItem();

    // data interface
    void setType(ItemType type);
    ItemType getType() { return m_itemType; }
    void inValidate() { m_valid = false; }
    bool isValid() { return m_valid; }
    void setObject(Slic3r::ModelObject* object);
    void setVolume(Slic3r::ModelVolume* volume);
    void setInstance(Slic3r::ModelInstance* instance);
    void setInstanceIndex(int index);
    void setLayerIndex(int index);
    void setLayerHeightRange(t_layer_height_range range);
    void enableCustomConfig(bool enable);
    Slic3r::ModelObject* getObject() { return m_pObjectData; }
    Slic3r::ModelVolume* getVolume() { return m_pVolumeData; }
    Slic3r::ModelInstance* getInstance() { return m_pInstanceData; }
    int getInstanceIndex() { return m_instanceIndex; }
    int getLayerIndex() { return m_layerIndex; }
    t_layer_height_range getLayerHeightRange() { return m_layerRange; }
    bool hasCustomConfig() { return m_customConfigFlag; }

    // view interface
    bool isGroup();
    bool hasIcon();
    wxImage& getIcon();
    wxString getText();
    bool hasPrintableInfo() { return m_itemType == ITYPE_OBJECT || m_itemType == ITYPE_INSTANCE; }
    bool getPrintable();
    bool hasSetting();
    bool hasFilamentInfo() { return m_itemType < ITYPE_VOLUME || (m_itemType == ITYPE_VOLUME && m_pVolumeData &&
        (m_pVolumeData->type() == Slic3r::ModelVolumeType::MODEL_PART || m_pVolumeData->type() == Slic3r::ModelVolumeType::PARAMETER_MODIFIER)); }
    int getFilamentIndex();
    wxColour getFilamentColour();

private:
    bool m_valid;
    ItemType m_itemType;

    // data
    bool m_customConfigFlag;
    int m_instanceIndex;
    int m_layerIndex;
    t_layer_height_range m_layerRange;
    Slic3r::ModelObject* m_pObjectData;
    Slic3r::ModelVolume* m_pVolumeData;
    Slic3r::ModelInstance* m_pInstanceData;
};

class AnkerObjectBar 
{
public:
    struct ItemForDelete
    {
        AnkerObjectItem::ItemType    type;
        int         obj_idx;
        int         sub_obj_idx;

        ItemForDelete(AnkerObjectItem::ItemType type, int obj_idx, int sub_obj_idx)
            : type(type), obj_idx(obj_idx), sub_obj_idx(sub_obj_idx)
        {}

        bool operator==(const ItemForDelete& r) const
        {
            return (type == r.type && obj_idx == r.obj_idx && sub_obj_idx == r.sub_obj_idx);
        }

        bool operator<(const ItemForDelete& r) const
        {
            if (obj_idx != r.obj_idx)
                return (obj_idx < r.obj_idx);
            return (sub_obj_idx < r.sub_obj_idx);
        }
    };
public:
    AnkerObjectBar(Slic3r::GUI::Plater* parent);
    ~AnkerObjectBar();

    AnkerObjectBarView* getViewer() { return m_pObjectBarView; }

    //Slic3r::GUI::ObjectList* getObjectList() { return m_pObjectBarView->getObjectList(); }
    AnkerObjectBarView* getObjectBarView() { return m_pObjectBarView; }

    void setListMaxHeight(int maxHeight);

    void change_top_border_for_mode_sizer(bool increase_border);
    void msw_rescale();
    void sys_color_changed();
    void update_objects_list_extruder_column(size_t extruders_count);
    void update_ui_from_settings();
    void object_list_changed();

    void init_objects();
    Slic3r::ModelObject* object(const int obj_idx) const;
    std::vector<Slic3r::ModelObject*>* objects() const;
    void copy();
    void paste();
    bool copy_to_clipboard();
    bool paste_from_clipboard();
    void undo();
    void redo();
    void increase_instances();
    void decrease_instances();

    // Add object's volumes to the list
    std::vector<AnkerObjectItem*> add_volumes_to_object_in_list(size_t obj_idx, std::function<bool(const Slic3r::ModelVolume*)> add_to_selection = nullptr);
    // Add object to the list
    void add_object_to_list(size_t obj_idx, bool call_selection_changed = true, bool singleSelection = false);

    AnkerObjectItem* reorder_volumes_and_get_selection(size_t obj_idx, std::function<bool(const Slic3r::ModelVolume*)> add_to_selection = nullptr);


    // Remove objects/sub-object from the list
    void bar_remove();
    //// Delete object from the list
    //void delete_object_from_list();
    void delete_object_from_list(const size_t obj_idx);
    void delete_volume_from_list(const size_t obj_idx, const size_t vol_idx);
    void delete_instance_from_list(const size_t obj_idx, const size_t inst_idx);
    bool delete_from_model_and_list(const AnkerObjectItem::ItemType type, const int obj_idx, const int sub_obj_idx);
    bool delete_from_model_and_list(const std::vector<ItemForDelete>& items_for_delete);
    void delete_from_model_and_list(const size_t obj_idx, bool reSelectFlag = true);
    void delete_from_model_and_list(Slic3r::ModelObject* obj);
    // Delete all objects from the list
    void delete_all_objects_from_list();
    // Delete item from model
    bool del_object(const int obj_idx);
    bool del_subobject_item(AnkerObjectItem* item, bool reSelectFlag = true);
	void del_settings_from_config();
	void del_instances_from_object(const int obj_idx);
	void del_layer_from_object(const int obj_idx, const t_layer_height_range& layer_range);
	void del_layers_from_object(const int obj_idx);
    bool del_from_cut_object(bool is_connector, bool is_model_part = false, bool is_negative_volume = false);
    bool del_subobject_from_object(const int objectID, const int volumeID, AnkerObjectItem::ItemType type, bool reSelectFlag = true);
    //void                del_info_item(const int obj_idx, InfoItemType type);

    void update_name_in_list(int obj_idx, int vol_idx) const;
    void update_volumes(int obj_idx);


    // Increase instances count
    void increase_object_instances(const size_t obj_idx, const size_t num, bool selectFlag = true);
    // Decrease instances count
    void decrease_object_instances(const size_t obj_idx, const size_t num);
    // 
    void split_instances();
    void instances_to_separated_object(const int obj_idx, const std::set<int>& inst_idxs);
    void instances_to_separated_objects(const int obj_idx);

    bool is_instance_or_object_selected();
    bool is_selected_object_cut();
    bool has_selected_cut_object() const;
    bool is_selected_object_avalible_settings();

    bool is_editing() const { return /*m_is_editing_started*/false; }


    void rename_item();
    void fix_through_netfabb();

    //update printable state for item from objects model
    void update_printable_state(int obj_idx, int instance_idx);
    bool toggle_printable_state(AnkerObjectItem* object);

    void load_subobject(Slic3r::ModelVolumeType type, bool from_galery = false);
    // ! ysFIXME - delete commented code after testing and rename "load_modifier" to something common
    //void                load_part(ModelObject& model_object, std::vector<ModelVolume*>& added_volumes, ModelVolumeType type, bool from_galery = false);
    void load_modifier(const wxArrayString& input_files, Slic3r::ModelObject& model_object, std::vector<Slic3r::ModelVolume*>& added_volumes, Slic3r::ModelVolumeType type, bool from_galery = false);
    void load_generic_subobject(const std::string& type_name, const Slic3r::ModelVolumeType type);
    void load_shape_object(const std::string& type_name);
    void load_shape_object_from_gallery();
    void load_shape_object_from_gallery(const wxArrayString& input_files);
    void load_mesh_object(const Slic3r::TriangleMesh& mesh, const std::string& name, bool center = true,
        const Slic3r::TextConfiguration* text_config = nullptr, const Slic3r::Transform3d* transformation = nullptr);

    void change_part_type();

    //void copy_layers_to_clipboard();
    //void paste_layers_into_list();
    //void copy_settings_to_clipboard();
    //void paste_settings_into_list();
    //bool clipboard_is_empty() const { return m_clipboard.empty(); }
    void paste_volumes_into_list(int obj_idx, const std::vector<Slic3r::ModelVolume*>& volumes); 
    void paste_objects_into_list(const std::vector<size_t>& object_idxs);

    void split();
    void merge(bool to_multipart_object);

    bool get_volume_by_item(int obj_idx);
    bool is_splittable(bool to_objects);
    //bool                selected_instances_of_same_object();
    bool can_split_instances();
    bool can_merge_to_multipart_object();
    bool can_merge_to_single_object();

    int get_selected_obj_idx();
    void get_selection_indexes(std::vector<int>& obj_idxs, std::vector<int>& vol_idxs);

    void unselect_objects();
    void select_item_all_children();
    void scene_selection_changed();
    void changed_object(const int obj_idx = -1);
    void part_selection_changed();

    void update_selections_on_canvas();

    void update_extruder_in_config(AnkerObjectItem* item, int filamentIndex);

    void update_after_undo_redo();

    void update_and_show_object_settings_item();

    // init layers root item then select it
    void layers_editing(bool selectFlag = true, AnkerObjectItem* item = nullptr);
    AnkerObjectItem* add_layer_item(const t_layer_height_range& range, AnkerObjectItem* layers_item, int layer_idx = -1);
    bool edit_layer_range(const t_layer_height_range& range, coordf_t layer_height);
    // This function may be called when a text field loses focus for a "add layer" or "remove layer" button.
    // In that case we don't want to destroy the panel with that "add layer" or "remove layer" buttons, as some messages
    // are already planned for them and destroying these widgets leads to crashes at least on OSX.
    // In that case the "add layer" or "remove layer" button handlers are responsible for always rebuilding the panel
    // even if the "add layer" or "remove layer" buttons did not update the layer spans or layer heights.
    bool edit_layer_range(const t_layer_height_range& range,
        const t_layer_height_range& new_range,
        // Don't destroy the panel with the "add layer" or "remove layer" buttons.
        bool suppress_ui_update = false);
    wxString can_add_new_range_after_current(t_layer_height_range current_range);
    void del_layer_range(const t_layer_height_range& range);
    void add_layer_range_after_current(const t_layer_height_range current_range);
    Slic3r::DynamicPrintConfig  get_default_layer_config(const int obj_idx);
    // Settings
    Slic3r::ModelConfig& get_item_config(AnkerObjectItem* item);
    void add_settings_to_item(bool panelFlag = true, AnkerObjectItem* item = nullptr);

    void enableSettingsPanel(bool enable, AnkerObjectItem* item = nullptr);
    void enableLayerPanel(bool enable, AnkerObjectItem* item = nullptr);

    void update_info_items(size_t obj_idx, AnkerObjectItem::ItemType type);
    void update_info_items(size_t obj_idx, wxDataViewItemArray* selections = nullptr, bool added_object = false);

    void clearAll(bool sizeUpdateFlag = true);
   
private:
    void OnItemClicked(wxCommandEvent& event);
    void OnItemSettingsClicked(wxCommandEvent& event);
    void OnItemPrintableClicked(wxCommandEvent& event);
    void OnItemFilamentClicked(wxCommandEvent& event);
    void OnKeyEvent(wxKeyEvent& event);

    void bar_selection_changed();

    int getObjectIndex(Slic3r::ModelObject* target, std::vector<Slic3r::ModelObject*>& targetVector);
    int getVolumeIndex(Slic3r::ModelVolume* target, std::vector<Slic3r::ModelVolume*>& targetVector);
    bool getItemID(AnkerObjectItem* item, int& objectIndex, int& volumeIndex, int& instanceIndex);

    void delViewItem(AnkerObjectItem* item, bool refresh);

private:
    Slic3r::GUI::Plater* m_pPlater;

    AnkerObjectBarView* m_pObjectBarView;

    bool m_toolFlag;
    Slic3r::ModelConfig* m_config{ nullptr };
    std::vector<Slic3r::ModelObject*>* m_objects{ nullptr };

    bool m_bindFlag { false };
    bool m_settingsEditing;
    bool m_layerEditing;
    AnkerObjectItem* m_editingObjectItem;

    // data item
    std::map<Slic3r::ModelObject*, AnkerObjectItem*> m_objectItemsMap;
    std::map<Slic3r::ModelObject*, std::map<Slic3r::ModelVolume*, AnkerObjectItem*>> m_volumeItemsMap;
    std::map<Slic3r::ModelObject*, std::vector<AnkerObjectItem*>> m_instanceItemsMap;
    std::map<Slic3r::ModelObject*, std::map<t_layer_height_range, AnkerObjectItem*>> m_layerItemsMap;
    std::map<Slic3r::ModelObject*, std::map<AnkerObjectItem::ItemType, AnkerObjectItem*>> m_groupItemsMap;
};

#endif // _ANKER_OBJECT_BAR_H_


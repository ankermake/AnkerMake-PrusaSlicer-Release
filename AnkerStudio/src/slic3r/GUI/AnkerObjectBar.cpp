#include "AnkerObjectBar.hpp"
#include "Plater.hpp"
#include "GUI_ObjectList.hpp"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "GLCanvas3D.hpp"
#include "AnkerObjectManipulator.hpp"
#include "AnkerFloatingList.hpp"
#include "AnkerObjectLayers.hpp"
#include "format.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "slic3r/Utils/UndoRedo.hpp"
#include "Gizmos/GLGizmoScale.hpp"
#include "Gizmos/GLGizmoSeam.hpp"
#include "Gizmos/GLGizmoFdmSupports.hpp"
#include "wxExtensions.hpp"
#include "libSlic3r/TriangleMesh.hpp"
#include "GalleryDialog.hpp"
#include "libslic3r/Geometry.hpp"

#define CONFIG_EXTRUDER_KEY "extruder"
#define CONFIG_LAYER_HEIGHT_KEY "layer_height"


static const Slic3r::GUI::Selection& scene_selection()
{
    return Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_selection();
}

static void take_snapshot(const wxString& snapshot_name)
{
    Slic3r::GUI::Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater)
        plater->take_snapshot(snapshot_name);
}

static Slic3r::DynamicPrintConfig& printer_config()
{
    return Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config;
}

static Slic3r::PrinterTechnology printer_technology()
{
    return Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology();
}

static double get_min_layer_height(const int extruder_idx)
{
    const Slic3r::DynamicPrintConfig& config = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config;
    return config.opt_float("min_layer_height", std::max(0, extruder_idx - 1));
}

static double get_max_layer_height(const int extruder_idx)
{
    const Slic3r::DynamicPrintConfig& config = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_edited_preset().config;
    int extruder_idx_zero_based = std::max(0, extruder_idx - 1);
    double max_layer_height = config.opt_float("max_layer_height", extruder_idx_zero_based);

    // In case max_layer_height is set to zero, it should default to 75 % of nozzle diameter:
    if (max_layer_height < EPSILON)
        max_layer_height = 0.75 * config.opt_float("nozzle_diameter", extruder_idx_zero_based);

    return max_layer_height;
}

static bool can_add_volumes_to_object(const Slic3r::ModelObject* object)
{
    bool can = object ? object->volumes.size() > 1 : false;

    if (can && object->is_cut()) {
        int no_connectors_cnt = 0;
        for (const Slic3r::ModelVolume* v : object->volumes)
            if (v && !v->is_cut_connector()) {
                if (!v->is_model_part())
                    return true;
                no_connectors_cnt++;
            }
        can = no_connectors_cnt > 1;
    }

    return can;
}

static wxString get_item_name(const std::string& name, const bool is_text_volume)
{
    return (is_text_volume ? _L("Text") + " - " : "") + Slic3r::GUI::from_u8(name);
}

static Slic3r::TriangleMesh create_mesh(const std::string& type_name, const Slic3r::BoundingBoxf3& bb)
{
    const double side = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_size_proportional_to_max_bed_size(0.1);

    indexed_triangle_set mesh;
    if (type_name == "Box")
        // Sitting on the print bed, left front front corner at (0, 0).
        mesh = Slic3r::its_make_cube(side, side, side);
    else if (type_name == "Cylinder")
        // Centered around 0, sitting on the print bed.
        // The cylinder has the same volume as the box above.
        mesh = Slic3r::its_make_cylinder(0.564 * side, side);
    else if (type_name == "Sphere")
        // Centered around 0, half the sphere below the print bed, half above.
        // The sphere has the same volume as the box above.
        mesh = Slic3r::its_make_sphere(0.62 * side, PI / 18);
    else if (type_name == "Slab")
        // Sitting on the print bed, left front front corner at (0, 0).
        mesh = Slic3r::its_make_cube(bb.size().x() * 1.5, bb.size().y() * 1.5, bb.size().z() * 0.5);
    return Slic3r::TriangleMesh(mesh);
}

static std::string getVolumeTypeText(Slic3r::ModelVolumeType type)
{
    static std::map<Slic3r::ModelVolumeType, wxString> modelVolumeTypeStr = {
        {Slic3r::ModelVolumeType::INVALID, _L("common_objecelist_modelType_invalid")},
        {Slic3r::ModelVolumeType::MODEL_PART, _L("common_objecelist_modelType_part")},
        {Slic3r::ModelVolumeType::NEGATIVE_VOLUME, _L("common_objecelist_modelType_negetive")},
        {Slic3r::ModelVolumeType::PARAMETER_MODIFIER, _L("common_objecelist_modelType_modifier")},
        {Slic3r::ModelVolumeType::SUPPORT_BLOCKER, _L("common_objecelist_modelType_blocker")},
        {Slic3r::ModelVolumeType::SUPPORT_ENFORCER, _L("common_objecelist_modelType_enforcer")} };
    return modelVolumeTypeStr[type].ToStdString();
}

AnkerObjectBar::AnkerObjectBar(Slic3r::GUI::Plater* parent)
    : m_pPlater(parent)
    , m_pObjectBarView(nullptr)
    , m_toolFlag(false)
    , m_settingsEditing(false)
    , m_layerEditing(false)
    , m_editingObjectItem(nullptr)
{
    m_pObjectBarView = new AnkerObjectBarView(parent);
    m_pObjectBarView->Bind(wxANKEREVT_AOBV_ITEM_CLICK, &AnkerObjectBar::OnItemClicked, this);
    m_pObjectBarView->Bind(wxANKEREVT_AOBV_SETTINGS_CLICK, &AnkerObjectBar::OnItemSettingsClicked, this);
    m_pObjectBarView->Bind(wxANKEREVT_AOBV_PRINTABLE_CLICK, &AnkerObjectBar::OnItemPrintableClicked, this);
    m_pObjectBarView->Bind(wxANKEREVT_AOBV_FILAMENT_CLICK, &AnkerObjectBar::OnItemFilamentClicked, this);
    m_pObjectBarView->Bind(wxANKEREVT_AOBV_KEY_EVENT, &AnkerObjectBar::OnKeyEvent, this);
}

AnkerObjectBar::~AnkerObjectBar()
{
    clearAll();
}

void AnkerObjectBar::change_top_border_for_mode_sizer(bool increase_border)
{
    if (m_pObjectBarView)
        m_pObjectBarView->change_top_border_for_mode_sizer(increase_border);
}

void AnkerObjectBar::msw_rescale()
{
    if (m_pObjectBarView)
        m_pObjectBarView->msw_rescale();
}

void AnkerObjectBar::sys_color_changed()
{
    if (m_pObjectBarView)
        m_pObjectBarView->sys_color_changed();
}

void AnkerObjectBar::update_objects_list_extruder_column(size_t extruders_count)
{
    if (m_pObjectBarView)
        m_pObjectBarView->update_objects_list_extruder_column(extruders_count);

    // update scene
    Slic3r::GUI::wxGetApp().plater()->update();
}

void AnkerObjectBar::update_ui_from_settings()
{
    //m_pObjectBarView->apply_volumes_order();
}

void AnkerObjectBar::object_list_changed()
{
    if (m_pObjectBarView)
        m_pObjectBarView->updateSize();
}

void AnkerObjectBar::init_objects()
{
    m_objects = &(Slic3r::GUI::wxGetApp().model().objects);
}

Slic3r::ModelObject* AnkerObjectBar::object(const int obj_idx) const
{
    if (m_objects == nullptr || obj_idx < 0)
    {
        ANKER_LOG_ERROR << "m_objects is NAN: " << obj_idx;
        return nullptr;
    }

    return (*m_objects)[obj_idx];
}

void AnkerObjectBar::copy()
{
    wxPostEvent((wxEvtHandler*)Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_wxglcanvas(), Slic3r::GUI::SimpleEvent(Slic3r::GUI::EVT_GLTOOLBAR_COPY));
}

void AnkerObjectBar::paste()
{
    wxPostEvent((wxEvtHandler*)Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_wxglcanvas(), Slic3r::GUI::SimpleEvent(Slic3r::GUI::EVT_GLTOOLBAR_PASTE));
}

bool AnkerObjectBar::copy_to_clipboard()
{
    return false;
}

bool AnkerObjectBar::paste_from_clipboard()
{
    return false;
}

void AnkerObjectBar::undo()
{
    Slic3r::GUI::wxGetApp().plater()->undo();
}

void AnkerObjectBar::redo()
{
    Slic3r::GUI::wxGetApp().plater()->redo();
}

void AnkerObjectBar::increase_instances()
{
    Slic3r::GUI::wxGetApp().plater()->increase_instances(1);
}

void AnkerObjectBar::decrease_instances()
{
    Slic3r::GUI::wxGetApp().plater()->decrease_instances(1);
}

std::vector<AnkerObjectItem*> AnkerObjectBar::add_volumes_to_object_in_list(size_t obj_idx, std::function<bool(const Slic3r::ModelVolume*)> add_to_selection)
{
    ANKER_LOG_INFO << "add volumes " << obj_idx << ", " << m_objects->size();

    std::vector<AnkerObjectItem*> resultItem;
    Slic3r::ModelObject* object = (*m_objects)[obj_idx];
    // add volumes to the object
    if (can_add_volumes_to_object(object)) {
        int volume_idx{ -1 };
        for (int i = 0; i < object->volumes.size(); i++) 
        {
            Slic3r::ModelVolume* volume = object->volumes[i];

            ++volume_idx;
            if ((object->is_cut() && volume  && volume->is_cut_connector()) ||
                (printer_technology() == Slic3r::PrinterTechnology::ptSLA && volume->type() == Slic3r::ModelVolumeType::PARAMETER_MODIFIER) ||
                m_volumeItemsMap[object].find(volume) != m_volumeItemsMap[object].end())
                continue;

            AnkerObjectItem* volumeItem = new AnkerObjectItem;
            volumeItem->setType(AnkerObjectItem::ItemType::ITYPE_VOLUME);
            volumeItem->setVolume(volume);
            volumeItem->setObject(object);
            m_volumeItemsMap[object][volume] = volumeItem;
            m_pObjectBarView->addObject(volumeItem, m_objectItemsMap[object]);

            resultItem.push_back(volumeItem);
        }
    }

    return resultItem;
}

void AnkerObjectBar::add_object_to_list(size_t obj_idx, bool call_selection_changed, bool singleSelection)
{
    ANKER_LOG_INFO << "add object to list " << obj_idx << ", " << m_objects->size();

    auto model_object = (*m_objects)[obj_idx];
    if (model_object == nullptr)
    {
        ANKER_LOG_ERROR << "model_object is NAN ";
        return;
    }
    const wxString& item_name = get_item_name(model_object->name, model_object->is_text());

    AnkerObjectItem* objectItem = new AnkerObjectItem;
    objectItem->setType(AnkerObjectItem::ItemType::ITYPE_OBJECT);
    objectItem->setObject(model_object);
    m_objectItemsMap[model_object] = objectItem;
    m_pObjectBarView->addObject(objectItem);
    if (call_selection_changed)
    {
        if (singleSelection)
            m_pObjectBarView->setSelectedObjectSingle(objectItem);
        else
            m_pObjectBarView->setSelectedObjectMulti(objectItem);
    }

    // volumes
    add_volumes_to_object_in_list(obj_idx);

    // add instances to the object, if it has those
    if (model_object->instances.size() > 1)
    {
        //std::vector<bool> print_idicator(model_object->instances.size());
        //for (size_t i = 0; i < model_object->instances.size(); ++i)
        //    print_idicator[i] = model_object->instances[i]->printable;

        //const wxDataViewItem object_item = m_objects_model->GetItemById(obj_idx);
        //m_objects_model->AddInstanceChild(object_item, print_idicator);
        //Expand(m_objects_model->GetInstanceRootItem(object_item));

        increase_object_instances(obj_idx, model_object->instances.size(), false);
    }
    //else
    //    m_objects_model->SetPrintableState(model_object->instances[0]->printable ? piPrintable : piUnprintable, obj_idx);

    // add settings to the object, if it has those
    //add_settings_item(item, &model_object->config.get());
    const Slic3r::DynamicPrintConfig& from_config = printer_technology() == Slic3r::ptFFF ?
        Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config :
        Slic3r::GUI::wxGetApp().preset_bundle->sla_prints.get_edited_preset().config;
    
    if (model_object->config.keys().size() == from_config.size())
        objectItem->enableCustomConfig(true);


    // Add layers if it has
    if (model_object->layer_config_ranges.size() > 0)
        layers_editing(false, objectItem);

    // info
    bool hasSeam = false;
    bool hasSupport = false;
    for (Slic3r::ModelVolume* mv : model_object->volumes) 
    {
        if (mv == nullptr)
            continue;

        if (!mv->seam_facets.empty())
            hasSeam = true;
        if (!mv->supported_facets.empty())
            hasSupport = true;

        if (hasSeam && hasSupport)
            break;
    }
    if (hasSeam)
        update_info_items(obj_idx, AnkerObjectItem::ITYPE_SEAM);

    if (hasSupport)
        update_info_items(obj_idx, AnkerObjectItem::ITYPE_SUPPORT);

//#ifndef __WXOSX__ 
//    if (call_selection_changed)
//        selection_changed();
//#endif //__WXMSW__

    // TODO
    m_pObjectBarView->object_list_changed();
}

void AnkerObjectBar::bar_remove()
{
    ANKER_LOG_INFO << "remove from bar " << m_pObjectBarView->getSelectedCount();

    std::vector<AnkerObjectItem*> selectedItems = m_pObjectBarView->getSelectedObjects();
    for (int i = 0; i < selectedItems.size(); i++)
    {
        bool reSelectFlag = i == selectedItems.size() - 1;
        AnkerObjectItem* item = selectedItems[i];
        if (item == nullptr)
        {
            ANKER_LOG_ERROR << "selected item is NAN";
            continue;
        }

        int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
        getItemID(item, objectIndex, volumeIndex, instanceIndex);

        AnkerObjectItem::ItemType type = item->getType();
        if (type == AnkerObjectItem::ITYPE_VOLUME)
        {
            del_subobject_from_object(objectIndex, volumeIndex, item->getType(), reSelectFlag);
        }
        else if (type == AnkerObjectItem::ITYPE_INSTANCE)
        {
            del_subobject_from_object(objectIndex, instanceIndex, item->getType(), reSelectFlag);
        }
        else if (type == AnkerObjectItem::ITYPE_OBJECT)
        {
            delete_from_model_and_list(objectIndex, reSelectFlag);
        }
        else
        {
            del_subobject_item(item, reSelectFlag);
        }
    }

    update_selections_on_canvas();
}

void AnkerObjectBar::delete_object_from_list(const size_t obj_idx)
{
    if (m_objects == nullptr || obj_idx < 0 || obj_idx >= m_objects->size())
    {
        ANKER_LOG_ERROR << "m_objects error";
        return;
    }

    ANKER_LOG_INFO << "delete_object_from_list";

    Slic3r::ModelObject* targetObj = m_objects->at(obj_idx);

    delViewItem(m_objectItemsMap[targetObj], true);

    AnkerObjectItem* newSelectedItem = m_objectItemsMap.size() > 0 ? m_objectItemsMap.begin()->second : nullptr;
    m_pObjectBarView->setSelectedObjectSingle(newSelectedItem);

    part_selection_changed();
}

void AnkerObjectBar::delete_volume_from_list(const size_t obj_idx, const size_t vol_idx)
{
    ANKER_LOG_INFO << "delete_volume_from_list";

    Slic3r::ModelObject* targetModel = (*m_objects)[obj_idx];
    if (targetModel == nullptr)
    {
        ANKER_LOG_ERROR << "targetModel is NAN";
        return;
    }

    Slic3r::ModelVolume* targetVolume = targetModel->volumes.size() > vol_idx && vol_idx > -1 ? targetModel->volumes[vol_idx] : nullptr;
    auto targetItr = m_volumeItemsMap[targetModel].find(targetVolume);
    auto objectItr = m_objectItemsMap.find(targetModel);
    if (targetItr != m_volumeItemsMap[targetModel].end() && objectItr != m_objectItemsMap.end())
    {
        delViewItem(targetItr->second, true);

        m_pObjectBarView->selectAll(false);
        m_pObjectBarView->setSelectedObjectSingle(objectItr->second);

        part_selection_changed();
    }
}

bool AnkerObjectBar::delete_from_model_and_list(const AnkerObjectItem::ItemType type, const int obj_idx, const int sub_obj_idx)
{
    ANKER_LOG_INFO << "delete from model and list itemType: " << (int)type;

    if (type == AnkerObjectItem::ItemType::ITYPE_OBJECT || type == AnkerObjectItem::ItemType::ITYPE_VOLUME || type == AnkerObjectItem::ItemType::ITYPE_INSTANCE) {
        if (type == AnkerObjectItem::ItemType::ITYPE_OBJECT) 
        {
            bool was_cut = object(obj_idx)->is_cut();

            delete_object_from_list(obj_idx);
            del_object(obj_idx);

	        //if (was_cut)
			//    update_lock_icons_for_model();
			return true;
		}
        else
        {
            type == AnkerObjectItem::ItemType::ITYPE_VOLUME ? delete_volume_from_list(obj_idx, sub_obj_idx) :
                delete_instance_from_list(obj_idx, sub_obj_idx);

            del_subobject_from_object(obj_idx, sub_obj_idx, type);

            return true;
        }
    }
    return false;
}

bool AnkerObjectBar::delete_from_model_and_list(const std::vector<AnkerObjectBar::ItemForDelete>& items_for_delete)
{
    ANKER_LOG_INFO << "delete from model and list itemForDelete: " << items_for_delete.size();

    if (items_for_delete.empty())
        return false;

    //m_prevent_list_events = true;
    //ScopeGuard sg_prevent_list_events = ScopeGuard([this]() { m_prevent_list_events = false; });

    std::set<size_t> modified_objects_ids;
    for (std::vector<ItemForDelete>::const_reverse_iterator item = items_for_delete.rbegin(); item != items_for_delete.rend(); ++item) {
        if (!(item->type == AnkerObjectItem::ItemType::ITYPE_OBJECT ||
            item->type == AnkerObjectItem::ItemType::ITYPE_VOLUME ||
            item->type == AnkerObjectItem::ItemType::ITYPE_INSTANCE))
            continue;

        Slic3r::ModelObject* obj = object(item->obj_idx);

        if (item->type == AnkerObjectItem::ItemType::ITYPE_OBJECT) 
        {
            bool was_cut = object(item->obj_idx)->is_cut();
            if (!del_object(item->obj_idx))
                return false;// continue;

            delViewItem(m_objectItemsMap[obj], false);
            //if (was_cut)
            //    update_lock_icons_for_model();
        }
        else if (!del_subobject_from_object(item->obj_idx, item->sub_obj_idx, item->type))
			continue;

        modified_objects_ids.insert(static_cast<size_t>(item->obj_idx));
    }

    for (size_t id : modified_objects_ids) {
        update_info_items(id);
    }

    m_pObjectBarView->updateSize();

    //m_prevent_list_events = false;
    if (modified_objects_ids.empty())
        return false;
    part_selection_changed();

    return true;
}


void AnkerObjectBar::delete_from_model_and_list(const size_t obj_idx, bool reSelectFlag)
{
    ANKER_LOG_INFO << "delete from model and list objIdx: " << obj_idx;

    if (obj_idx > -1 && obj_idx < m_objects->size())
        return;

    Slic3r::ModelObject* targetObj = m_objects->at(obj_idx);

    delViewItem(m_objectItemsMap[targetObj], true);

    Slic3r::GUI::wxGetApp().plater()->delete_object_from_model(obj_idx);

    if (reSelectFlag)
    {
        AnkerObjectItem* newSelectedItem = m_objectItemsMap.size() > 0 ? m_objectItemsMap.begin()->second : nullptr;
        m_pObjectBarView->setSelectedObjectSingle(newSelectedItem);

        part_selection_changed();
    }
}

void AnkerObjectBar::delete_from_model_and_list(Slic3r::ModelObject* obj)
{
    ANKER_LOG_INFO << "delete from model and list obj: " << (obj == nullptr);

    if (obj == nullptr)
        return;

    int obj_idx = getObjectIndex(obj, *m_objects);
    if (obj_idx >= 0)
    {
        delete_from_model_and_list(obj_idx);
    }
}

void AnkerObjectBar::delete_instance_from_list(const size_t obj_idx, const size_t inst_idx)
{
}

void AnkerObjectBar::delete_all_objects_from_list()
{
    clearAll();

    part_selection_changed();
}

bool AnkerObjectBar::del_object(const int obj_idx)
{
    return Slic3r::GUI::wxGetApp().plater()->delete_object_from_model(obj_idx);
}

bool AnkerObjectBar::del_subobject_item(AnkerObjectItem* item, bool reSelectFlag)
{
    if (!item) return false;

    ANKER_LOG_INFO << "del_subobject_item: " << (int)item->getType();

    int obj_idx, idx, ins_idx;
    getItemID(item, obj_idx, idx, ins_idx);
    AnkerObjectItem::ItemType type = item->getType();

    if (type == AnkerObjectItem::ITYPE_COUNT || obj_idx == -1)
        return false;

    Slic3r::ModelObject* obj = item->getObject();
	AnkerObjectItem* newSelectedItem = m_objectItemsMap[obj];

    if (type == AnkerObjectItem::ITYPE_INSTANCE_GROUP)
    {
        del_instances_from_object(obj_idx);
    }
	else if (type == AnkerObjectItem::ITYPE_LAYER_GROUP)
	{
		del_layers_from_object(obj_idx);
    }
    else if (type == AnkerObjectItem::ITYPE_LAYER)
    {
        del_layer_from_object(obj_idx, item->getLayerHeightRange());

        if (obj->layer_config_ranges.size() > 0)
            newSelectedItem = m_layerItemsMap[obj].begin()->second;
    }
    else if (type == AnkerObjectItem::ITYPE_SEAM)
    {
        //Slic3r::GUI::GLGizmoBase* gizmo = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::Seam);
        //Slic3r::GUI::GLGizmoSeam* seamGizmo = dynamic_cast<Slic3r::GUI::GLGizmoSeam*>(gizmo);
        //if (seamGizmo)
        //    seamGizmo->clearSeam();

        Slic3r::GUI::Plater::TakeSnapshot(Slic3r::GUI::wxGetApp().plater_, _L("Remove paint-on seam"));
        for (Slic3r::ModelVolume* mv : (*m_objects)[obj_idx]->volumes)
            mv->seam_facets.reset();
    }
    else if (type == AnkerObjectItem::ITYPE_SUPPORT)
    {
        if (auto gizmo_base = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::FdmSupports)) {
            if (auto support_gizmo = dynamic_cast<Slic3r::GUI::GLGizmoFdmSupports*>(gizmo_base)) {
                support_gizmo->reset();
            }
        }

        Slic3r::GUI::Plater::TakeSnapshot(Slic3r::GUI::wxGetApp().plater_, _L("Remove paint-on supports"));
        for (Slic3r::ModelVolume* mv : (*m_objects)[obj_idx]->volumes)
            mv->supported_facets.reset();

    }
    //else if (idx == -1 || !del_subobject_from_object(obj_idx, idx, type))
    //    return false;

    //// If last volume item with warning was deleted, unmark object item
    //if (type == AnkerObjectItem::ITYPE_VOLUME) {
    //    const std::string& icon_name = get_warning_icon_name(object(obj_idx)->get_object_stl_stats());
    //    m_objects_model->UpdateWarningIcon(parent, icon_name);
    //}

    //if (!(type & itInfo) || item_info_type != InfoItemType::CutConnectors) {
    //    // Connectors Item is already updated/deleted inside the del_info_item()
    //    m_objects_model->Delete(item);
    //    update_info_items(obj_idx);
    //}

    delViewItem(item, true);
    if (reSelectFlag)
    {
        m_pObjectBarView->setSelectedObjectSingle(newSelectedItem);
        bar_selection_changed();
    }

    return true;
}

void AnkerObjectBar::del_layer_from_object(const int obj_idx, const t_layer_height_range& layer_range)
{
    Slic3r::ModelObject* obj = object(obj_idx);
    const auto del_range = obj->layer_config_ranges.find(layer_range);
    if (del_range == obj->layer_config_ranges.end())
        return;

    take_snapshot(_(L("Delete Height Range")));

    obj->layer_config_ranges.erase(del_range);

    changed_object(obj_idx);
}

void AnkerObjectBar::del_layers_from_object(const int obj_idx)
{
    Slic3r::ModelObject* obj = object(obj_idx);
    obj->layer_config_ranges.clear();

    changed_object(obj_idx);
}

bool AnkerObjectBar::del_from_cut_object(bool is_connector, bool is_model_part, bool is_negative_volume)
{
    //const long buttons_style = is_cut_connector ? (wxYES | wxNO | wxCANCEL) : (wxYES | wxCANCEL);

    //const wxString title = is_cut_connector ? _L("Delete connector from object which is a part of cut") :
    //    is_model_part ? _L("Delete solid part from object which is a part of cut") :
    //    is_negative_volume ? _L("Delete negative volume from object which is a part of cut") : "";

    //const wxString msg_end = is_cut_connector ? ("\n" + _L("To save cut information you can delete all connectors from all related objects.")) : "";

    //InfoDialog dialog(wxGetApp().plater(), title,
    //    _L("This action will break a cut information.\n"
    //        "After that AnkerMake Studio can't guarantee model consistency.") + "\n\n" +
    //    _L("To manipulate with solid parts or negative volumes you have to invalidate cut infornation first." + msg_end),
    //    false, buttons_style | wxCANCEL_DEFAULT | wxICON_WARNING);

    //dialog.SetButtonLabel(wxID_YES, _L("Invalidate cut info"));
    //if (is_cut_connector)
    //    dialog.SetButtonLabel(wxID_NO, _L("Delete all connectors"));

    //const int answer = dialog.ShowModal();
    //if (answer == wxID_CANCEL)
    //    return false;

    //if (answer == wxID_YES)
    //    invalidate_cut_info_for_selection();
    //else if (answer == wxID_NO)
    //    delete_all_connectors_for_selection();
    return true;
}

bool AnkerObjectBar::del_subobject_from_object(const int objectID, const int volumeID, AnkerObjectItem::ItemType type, bool reSelectFlag)
{
    if (m_objects == nullptr || objectID < 0 || objectID >= m_objects->size() || volumeID < 0)
    {
        ANKER_LOG_ERROR << "Failed to delete subobject:  " << type << ", " << objectID << ", " << volumeID << ", " << (m_objects ? m_objects->size() : -1);
        return false;
    }

    ANKER_LOG_INFO << "TYPE = " << type << ", " << objectID << ", " << volumeID << ", " << m_objects->size();

    Slic3r::ModelObject* object = (*m_objects)[objectID];
    if (object == nullptr)
    {
        ANKER_LOG_ERROR << "object is nullptr";
        return false;
    }

    auto itr = m_objectItemsMap.find(object);
    AnkerObjectItem* newSelectedItem = nullptr;
    if (itr != m_objectItemsMap.end())
        newSelectedItem = itr->second;
	if (type == AnkerObjectItem::ItemType::ITYPE_VOLUME)
    {
        if (volumeID >= object->volumes.size())
        {
            ANKER_LOG_ERROR << "volumeID out of range: "  << volumeID << ", " << object->volumes.size();
            return false;
        }

        ANKER_LOG_INFO << "volume size = " << object->volumes.size();

        const auto volume = object->volumes[volumeID];

        if (volume == nullptr)
        {
            ANKER_LOG_ERROR << "volume is nullptr";
            return false;
        }

        // if user is deleting the last solid part, throw error
        int solid_cnt = 0;
        for (auto vol : object->volumes)
        {
            if (vol == nullptr)
            {
                ANKER_LOG_ERROR << "object volumes is nullptr";
                return false;
            }

            if (vol->is_model_part())
                ++solid_cnt;
        }
        if (volume->is_model_part() && solid_cnt == 1) 
        {
            ANKER_LOG_ERROR << "From Object List You can't delete the last solid part from object.";
            Slic3r::GUI::show_error(nullptr, _L("From Object List You can't delete the last solid part from object."));
            return false;
        }
        if (object->is_cut() && (volume->is_model_part() || volume->is_negative_volume())) 
        {
            del_from_cut_object(volume->is_cut_connector(), volume->is_model_part(), volume->is_negative_volume());
            ANKER_LOG_ERROR << "no delete cut";
            // in any case return false to break the deletion
            return false;
        }

        take_snapshot(_L("Delete Subobject"));

        delViewItem(m_volumeItemsMap[object][volume], true);
        object->delete_volume(volumeID);

        if (object->volumes.size() > 0)
        {
            auto volItr = m_volumeItemsMap[object].find(object->volumes[0]);
            if (volItr != m_volumeItemsMap[object].end())
            {
                newSelectedItem = volItr->second;
            }
        }

        if (object->volumes.size() == 1 && object->volumes[0]->config.has(CONFIG_EXTRUDER_KEY))
        {
            const Slic3r::ConfigOption* option = object->volumes[0]->config.option(CONFIG_EXTRUDER_KEY);
			object->config.set_key_value(CONFIG_EXTRUDER_KEY, option->clone());
            object->volumes[0]->config.erase(CONFIG_EXTRUDER_KEY);
        }
    }
    else if (type == AnkerObjectItem::ItemType::ITYPE_INSTANCE) 
    {
        if (object->instances.size() == 1) 
        {
            ANKER_LOG_ERROR << "Last instance of an object cannot be deleted.";
            Slic3r::GUI::show_error(nullptr, _L("Last instance of an object cannot be deleted."));
            return false;
        }
        if (object->is_cut()) 
        {
            ANKER_LOG_ERROR << "Instance cannot be deleted from cut object.";
            Slic3r::GUI::show_error(nullptr, _L("Instance cannot be deleted from cut object."));
            return false;
        }

        take_snapshot(_L("Delete Instance"));
        delViewItem(m_instanceItemsMap[object][volumeID], true);
        object->delete_instance(volumeID);

        if (object->instances.size() > 0 && m_instanceItemsMap[object].size() > 0)
        {
            newSelectedItem = m_instanceItemsMap[object][0];
        }
    }
    else
    {
        ANKER_LOG_ERROR << "not subobject";
        return false;
    }

    changed_object(objectID);

    if (reSelectFlag)
	{
		m_pObjectBarView->setSelectedObjectSingle(newSelectedItem);
		bar_selection_changed();
	}

    return true;
}

void AnkerObjectBar::update_name_in_list(int obj_idx, int vol_idx) const
{
    if (obj_idx < 0) return;

    Slic3r::ModelObject* obj = object(obj_idx);
    ////const bool is_text_volume = type == itVolume ? obj->volumes[vol_idx]->is_text() : obj->is_text();
    //const wxString new_name = get_item_name(object(obj_idx)->volumes[vol_idx]->name, /*is_text_volume*/false);

    //if (new_name.IsEmpty())
    //    return;

    //m_pObjectBarView->updateObjectName(obj, new_name);

    auto itr = m_objectItemsMap.find(obj);
    if (itr != m_objectItemsMap.end())
    {
        m_pObjectBarView->updateObject(itr->second);
    }

    // if object has just one volume, rename object too
    if (obj->volumes.size() == 1)
        obj->name = obj->volumes.front()->name;
}

void AnkerObjectBar::update_volumes(int obj_idx)
{
    if (obj_idx < 0 || obj_idx >= m_objects->size())
        return;

    Slic3r::ModelObject* obj = object(obj_idx);
    auto objItr = m_objectItemsMap.find(obj);
    if (objItr == m_objectItemsMap.end())
        return;

    auto volsItr = m_volumeItemsMap.find(obj);
    if (volsItr != m_volumeItemsMap.end())
    {
        std::vector<AnkerObjectItem*> oldVolumes;
        for (auto itr = volsItr->second.begin(); itr != volsItr->second.end(); itr++)
        {
            itr->second->inValidate();
            oldVolumes.push_back(itr->second);
        }
        for (int i = 0; i < oldVolumes.size(); i++)
        {
            delViewItem(oldVolumes[i], false);
        }
    }

    add_volumes_to_object_in_list(obj_idx);

    update_info_items(obj_idx, AnkerObjectItem::ITYPE_SEAM);
    update_info_items(obj_idx, AnkerObjectItem::ITYPE_SUPPORT);
}

void AnkerObjectBar::increase_object_instances(const size_t obj_idx, const size_t num, bool selectFlag)
{
    if (obj_idx < 0 || obj_idx >= m_objects->size())
        return;

    ANKER_LOG_INFO << "increase object instances num: " << num;

    Slic3r::ModelObject* obj = object(obj_idx);
    auto objItr = m_objectItemsMap.find(obj);
    if (objItr == m_objectItemsMap.end())
        return;

    AnkerObjectItem* objItem = m_objectItemsMap[obj];
    AnkerObjectItem* groupItem = m_groupItemsMap[obj][AnkerObjectItem::ITYPE_INSTANCE_GROUP];
    if (groupItem == nullptr)
    {
        groupItem = new AnkerObjectItem;
        groupItem->setType(AnkerObjectItem::ItemType::ITYPE_INSTANCE_GROUP);
        groupItem->setObject(obj);
        m_groupItemsMap[obj][AnkerObjectItem::ITYPE_INSTANCE_GROUP] = groupItem;
        m_pObjectBarView->addObject(groupItem, objItem);
    }

    AnkerObjectItem* item = nullptr;
    AnkerObjectItem* selectedItem = groupItem;
    int addedCount = num;
    int indexBase = obj->instances.size() - addedCount;
    if (indexBase < 0)
        addedCount = obj->instances.size();
    for (int i = 0; i < addedCount; i++)
    {
        item = new AnkerObjectItem;
        item->setType(AnkerObjectItem::ItemType::ITYPE_INSTANCE);
        item->setObject(obj);
        item->setInstance(obj->instances[indexBase + i]);
        item->setInstanceIndex(m_instanceItemsMap[obj].size());
        m_instanceItemsMap[obj].push_back(item);
        m_pObjectBarView->addObject(item, groupItem);

        selectedItem = item;
    }

    m_pObjectBarView->updateSize();

    if (selectFlag)
    {
        //m_pObjectBarView->selectAll(false);
        m_pObjectBarView->setSelectedObjectSingle(selectedItem);
        bar_selection_changed();
    }
}

void AnkerObjectBar::decrease_object_instances(const size_t obj_idx, const size_t num)
{
    //select_item([this, obj_idx, num]() { return m_objects_model->DeleteLastInstance(m_objects_model->GetItemById(obj_idx), num); });

    if (obj_idx < 0 || obj_idx >= m_objects->size())
        return;

    ANKER_LOG_INFO << "decrease object instances num: " << num;

    Slic3r::ModelObject* obj = object(obj_idx);
    auto objItr = m_objectItemsMap.find(obj);
    if (objItr == m_objectItemsMap.end())
        return;

    AnkerObjectItem* objItem = m_objectItemsMap[obj];
    AnkerObjectItem* groupItem = nullptr;
    auto groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_INSTANCE_GROUP);
    if (groupItr == m_groupItemsMap[obj].end())
        return;

    groupItem = groupItr->second;
    if (m_instanceItemsMap[obj].size() <= num)
    {
        delViewItem(groupItem, true);
    }
    else
    {
        int leftNum = std::max(size_t(0), m_instanceItemsMap[obj].size() - num);
        std::vector<AnkerObjectItem*> removedItem;
        for (int i = m_instanceItemsMap[obj].size() - 1; i >= leftNum; i--)
        {
            removedItem.push_back(m_instanceItemsMap[obj][i]);
        }

        for (int i = 0; i < removedItem.size(); i++)
        {
            delViewItem(removedItem[i], false);
        }

        m_pObjectBarView->updateSize();
    }


    //m_pObjectBarView->selectAll(false);
    m_pObjectBarView->setSelectedObjectSingle(groupItem);
    bar_selection_changed();
}

AnkerObjectItem* AnkerObjectBar::reorder_volumes_and_get_selection(size_t obj_idx, std::function<bool(const Slic3r::ModelVolume*)> add_to_selection)
{
    (*m_objects)[obj_idx]->sort_volumes(Slic3r::GUI::wxGetApp().app_config->get_bool("order_volumes"));

    std::vector<AnkerObjectItem*> items = add_volumes_to_object_in_list(obj_idx, std::move(add_to_selection));

    changed_object(int(obj_idx));

    return items.size() > 0 ? items[0] : nullptr;
}

void AnkerObjectBar::split_instances()
{
    ANKER_LOG_INFO << "split instances";

    const Slic3r::GUI::Selection& selection = scene_selection();
    const int obj_idx = selection.get_object_idx();
    if (obj_idx == -1)
        return;

    Slic3r::GUI::Plater::TakeSnapshot snapshot(Slic3r::GUI::wxGetApp().plater(), _(L("Instances to Separated Objects")));

    if (selection.is_single_full_object())
    {
        instances_to_separated_objects(obj_idx);
        return;
    }

    const int inst_idx = selection.get_instance_idx();
    const std::set<int> inst_idxs = inst_idx < 0 ?
        selection.get_instance_idxs() :
        std::set<int>{ inst_idx };

    instances_to_separated_object(obj_idx, inst_idxs);
}

void AnkerObjectBar::instances_to_separated_object(const int obj_idx, const std::set<int>& inst_idxs)
{
    //if ((*m_objects)[obj_idx]->instances.size() == inst_idxs.size())
    //{
    //    instances_to_separated_objects(obj_idx);
    //    return;
    //}

    //// create new object from selected instance  
    //Slic3r::ModelObject* model_object = (*m_objects)[obj_idx]->get_model()->add_object(*(*m_objects)[obj_idx]);
    //for (int inst_idx = int(model_object->instances.size()) - 1; inst_idx >= 0; inst_idx--)
    //{
    //    if (find(inst_idxs.begin(), inst_idxs.end(), inst_idx) != inst_idxs.end())
    //        continue;
    //    model_object->delete_instance(inst_idx);
    //}

    //// Add new object to the object_list
    //const size_t new_obj_indx = static_cast<size_t>(m_objects->size() - 1);
    //add_object_to_list(new_obj_indx);

    //for (std::set<int>::const_reverse_iterator it = inst_idxs.rbegin(); it != inst_idxs.rend(); ++it)
    //{
    //    // delete selected instance from the object
    //    del_subobject_from_object(obj_idx, *it, itInstance);
    //    delete_instance_from_list(obj_idx, *it);
    //}

    //// update printable state for new volumes on canvas3D
    //Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_object(new_obj_indx);
   
    //// 
    //update_info_items(new_obj_indx);
}

void AnkerObjectBar::instances_to_separated_objects(const int obj_idx)
{
    //const int inst_cnt = (*m_objects)[obj_idx]->instances.size();

    //std::vector<size_t> object_idxs;

    //for (int i = inst_cnt - 1; i > 0; i--)
    //{
    //    // create new object from initial
    //    Slic3r::ModelObject* object = (*m_objects)[obj_idx]->get_model()->add_object(*(*m_objects)[obj_idx]);
    //    for (int inst_idx = object->instances.size() - 1; inst_idx >= 0; inst_idx--)
    //    {
    //        if (inst_idx == i)
    //            continue;
    //        // delete unnecessary instances
    //        object->delete_instance(inst_idx);
    //    }

    //    // Add new object to the object_list
    //    const size_t new_obj_indx = static_cast<size_t>(m_objects->size() - 1);
    //    add_object_to_list(new_obj_indx);
    //    object_idxs.push_back(new_obj_indx);

    //    // delete current instance from the initial object
    //    del_subobject_from_object(obj_idx, i, itInstance);
    //    delete_instance_from_list(obj_idx, i);
    //}

    //// update printable state for new volumes on canvas3D
    //Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_objects(object_idxs);
    //for (size_t object : object_idxs)
    //    update_info_items(object);
}

bool AnkerObjectBar::is_instance_or_object_selected()
{
    const Slic3r::GUI::Selection& selection = scene_selection();
    return selection.is_single_full_instance() || selection.is_single_full_object();
}

bool AnkerObjectBar::is_selected_object_cut()
{
    const Slic3r::GUI::Selection& selection = scene_selection();
    int obj_idx = selection.get_object_idx();
    if (obj_idx < 0)
        return false;
    return object(obj_idx)->is_cut();
}

bool AnkerObjectBar::has_selected_cut_object() const
{
    for (int i = 0; i < m_pObjectBarView->getSelectedCount(); i++)
    {
        AnkerObjectItem* item = m_pObjectBarView->getSelectedObject(i);
        Slic3r::ModelObject* modelObject = item->getType() == AnkerObjectItem::ItemType::ITYPE_OBJECT ? item->getObject() : nullptr;

        if (modelObject && modelObject->is_cut())
            return true;
    }

    return false;
}

bool AnkerObjectBar::is_selected_object_avalible_settings()
{
    AnkerObjectItem* selectedItem = m_pObjectBarView->getSelectedCount() > 0 ? m_pObjectBarView->getSelectedObject() : nullptr;
    if (selectedItem)
    {
        AnkerObjectItem::ItemType type = selectedItem->getType();
        return type == AnkerObjectItem::ITYPE_INSTANCE || type == AnkerObjectItem::ITYPE_OBJECT || 
            (type == AnkerObjectItem::ITYPE_VOLUME && 
                (selectedItem->getVolume()->type() == Slic3r::ModelVolumeType::MODEL_PART || 
                    selectedItem->getVolume()->type() == Slic3r::ModelVolumeType::PARAMETER_MODIFIER));
    }
    return false;
}

void AnkerObjectBar::rename_item()
{
    //const wxDataViewItem item = GetSelection();
    //auto type = m_objects_model->GetItemType(item);
    //if (!item || !(type & (itVolume | itObject)))
    //    return;

    //wxString input_name = m_objects_model->GetName(item);
    //if (ModelObject* obj = object(m_objects_model->GetObjectIdByItem(item))) {
    //    // if there is text item to editing, than edit just a name without Text marker
    //    if (type == itObject && obj->is_text())
    //        input_name = from_u8(obj->name);
    //}

    //const wxString new_name = wxGetTextFromUser(_L("Enter new name") + ":", _L("Renaming"), input_name, this);

    //if (new_name.IsEmpty())
    //    return;

    //if (Plater::has_illegal_filename_characters(new_name)) {
    //    Plater::show_illegal_characters_warning(this);
    //    return;
    //}

    //if (m_objects_model->SetName(new_name, item))
    //    update_name_in_model(item);
}

void AnkerObjectBar::fix_through_netfabb()
{
//    // Do not fix anything when a gizmo is open. There might be issues with updates
//// and what is worse, the snapshot time would refer to the internal stack.
//    if (!wxGetApp().plater()->canvas3D()->get_gizmos_manager().check_gizmos_closed_except(GLGizmosManager::Undefined))
//        return;
//
//    //          model_name
//    std::vector<std::string>                           succes_models;
//    //                   model_name     failing reason
//    std::vector<std::pair<std::string, std::string>>   failed_models;
//
//    std::vector<int> obj_idxs, vol_idxs;
//    get_selection_indexes(obj_idxs, vol_idxs);
//
//    std::vector<std::string> model_names;
//
//    // clear selections from the non-broken models if any exists
//    // and than fill names of models to repairing 
//    if (vol_idxs.empty()) {
//#if !FIX_THROUGH_NETFABB_ALWAYS
//        for (int i = int(obj_idxs.size()) - 1; i >= 0; --i)
//            if (object(obj_idxs[i])->get_repaired_errors_count() == 0)
//                obj_idxs.erase(obj_idxs.begin() + i);
//#endif // FIX_THROUGH_NETFABB_ALWAYS
//        for (int obj_idx : obj_idxs)
//            model_names.push_back(object(obj_idx)->name);
//    }
//    else {
//        ModelObject* obj = object(obj_idxs.front());
//#if !FIX_THROUGH_NETFABB_ALWAYS
//        for (int i = int(vol_idxs.size()) - 1; i >= 0; --i)
//            if (obj->get_repaired_errors_count(vol_idxs[i]) == 0)
//                vol_idxs.erase(vol_idxs.begin() + i);
//#endif // FIX_THROUGH_NETFABB_ALWAYS
//        for (int vol_idx : vol_idxs)
//            model_names.push_back(obj->volumes[vol_idx]->name);
//    }
//
//    auto plater = wxGetApp().plater();
//
//    auto fix_and_update_progress = [this, plater, model_names](const int obj_idx, const int vol_idx,
//        int model_idx,
//        wxProgressDialog& progress_dlg,
//        std::vector<std::string>& succes_models,
//        std::vector<std::pair<std::string, std::string>>& failed_models)
//    {
//        const std::string& model_name = model_names[model_idx];
//        wxString msg = _L("Repairing model");
//        if (model_names.size() == 1)
//            msg += ": " + from_u8(model_name) + "\n";
//        else {
//            msg += ":\n";
//            for (int i = 0; i < int(model_names.size()); ++i)
//                msg += (i == model_idx ? " > " : "   ") + from_u8(model_names[i]) + "\n";
//            msg += "\n";
//        }
//
//        plater->clear_before_change_mesh(obj_idx, _u8L("Custom supports, seams and multimaterial painting were "
//            "removed after repairing the mesh."));
//        std::string res;
//        if (!fix_model_by_win10_sdk_gui(*(object(obj_idx)), vol_idx, progress_dlg, msg, res))
//            return false;
//        wxGetApp().plater()->changed_mesh(obj_idx);
//
//        plater->changed_mesh(obj_idx);
//
//        if (res.empty())
//            succes_models.push_back(model_name);
//        else
//            failed_models.push_back({ model_name, res });
//
//        update_item_error_icon(obj_idx, vol_idx);
//        update_info_items(obj_idx);
//
//        return true;
//    };
//
//    Plater::TakeSnapshot snapshot(plater, _L("Fix through NetFabb"));
//
//    // Open a progress dialog.
//    wxProgressDialog progress_dlg(_L("Fixing through NetFabb"), "", 100, find_toplevel_parent(plater),
//        wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
//    int model_idx{ 0 };
//    if (vol_idxs.empty()) {
//        int vol_idx{ -1 };
//        for (int obj_idx : obj_idxs) {
//#if !FIX_THROUGH_NETFABB_ALWAYS
//            if (object(obj_idx)->get_repaired_errors_count(vol_idx) == 0)
//                continue;
//#endif // FIX_THROUGH_NETFABB_ALWAYS
//            if (!fix_and_update_progress(obj_idx, vol_idx, model_idx, progress_dlg, succes_models, failed_models))
//                break;
//            model_idx++;
//        }
//    }
//    else {
//        int obj_idx{ obj_idxs.front() };
//        for (int vol_idx : vol_idxs) {
//            if (!fix_and_update_progress(obj_idx, vol_idx, model_idx, progress_dlg, succes_models, failed_models))
//                break;
//            model_idx++;
//        }
//    }
//    // Close the progress dialog
//    progress_dlg.Update(100, "");
//
//    // Show info notification
//    wxString msg;
//    wxString bullet_suf = "\n   - ";
//    if (!succes_models.empty()) {
//        msg = _L_PLURAL("The following model was repaired successfully", "The following models were repaired successfully", succes_models.size()) + ":";
//        for (auto& model : succes_models)
//            msg += bullet_suf + from_u8(model);
//        msg += "\n\n";
//    }
//    if (!failed_models.empty()) {
//        msg += _L_PLURAL("Folowing model repair failed", "Folowing models repair failed", failed_models.size()) + ":\n";
//        for (auto& model : failed_models)
//            msg += bullet_suf + from_u8(model.first) + ": " + _(model.second);
//    }
//    if (msg.IsEmpty())
//        msg = _L("Repairing was canceled");
//    plater->get_notification_manager()->push_notification(NotificationType::NetfabbFinished, NotificationManager::NotificationLevel::PrintInfoShortNotificationLevel, boost::nowide::narrow(msg));
}

void AnkerObjectBar::update_printable_state(int obj_idx, int instance_idx)
{
    //ModelObject* object = (*m_objects)[obj_idx];

    //const PrintIndicator printable = object->instances[instance_idx]->printable ? piPrintable : piUnprintable;
    //if (object->instances.size() == 1)
    //    instance_idx = -1;

    //m_objects_model->SetPrintableState(printable, obj_idx, instance_idx);

}

bool AnkerObjectBar::toggle_printable_state(AnkerObjectItem* object)
{
    ANKER_LOG_INFO << "toggle printable state: " << (object == nullptr);

    if (object == nullptr)
        return false;

    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(object, objectIndex, volumeIndex, instanceIndex);

    bool printable = false;
    Slic3r::ModelObject* targetModel = object->getObject();
    if (object->getType() == AnkerObjectItem::ItemType::ITYPE_OBJECT)
    {
        printable = !(targetModel->instances.size() > 0 ? targetModel->instances[0]->printable : targetModel->printable);

        const wxString snapshot_text = printable ? _L("Set Printable Instance") : _L("Set Unprintable Instance");
        take_snapshot(snapshot_text);

        //targetModel->printable = printable;

        for (int i = 0; i < targetModel->instances.size(); i++)
        {
            targetModel->instances[i]->printable = printable;
        }
    }
    else if (object->getType() == AnkerObjectItem::ItemType::ITYPE_INSTANCE && object->getInstance())
    {
        printable = !object->getInstance()->printable;
		object->getInstance()->printable = printable;

        bool objectPrintable = object->getPrintable();
        for (int i = 0; i < targetModel->instances.size(); i++)
        {
            objectPrintable = objectPrintable || targetModel->instances[i]->printable;
        }
        //targetModel->printable = objectPrintable;
    }

    m_pObjectBarView->updateObject(m_objectItemsMap[targetModel]);

    std::vector<size_t> obj_idxs;
    obj_idxs.emplace_back(static_cast<size_t>(objectIndex));
    sort(obj_idxs.begin(), obj_idxs.end());
    obj_idxs.erase(unique(obj_idxs.begin(), obj_idxs.end()), obj_idxs.end());

    // update printable state on canvas
    Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_objects(obj_idxs);

    // update scene
    Slic3r::GUI::wxGetApp().plater()->update();

    return printable;
}

void AnkerObjectBar::load_subobject(Slic3r::ModelVolumeType type, bool from_galery)
{
    ANKER_LOG_INFO << "load_subobject: " << getVolumeTypeText(type);

    if (type == Slic3r::ModelVolumeType::INVALID && from_galery) {
        load_shape_object_from_gallery();
        return;
    }

    //wxDataViewItem item = Slic3r::GUI::GetSelection();
    AnkerObjectItem* item = m_pObjectBarView->getSelectedObject();
    // we can add volumes for Object or Instance
    if (!item || !(item->getType() == AnkerObjectItem::ITYPE_OBJECT || item->getType() == AnkerObjectItem::ITYPE_INSTANCE))
        return;
    int obj_idx, vol_idx, ins_idx;
    getItemID(item, obj_idx, vol_idx, ins_idx);

    if (obj_idx < 0) return;

    //// Get object item, if Instance is selected
    //if (m_objects_model->GetItemType(item) & itInstance)
    //    item = m_objects_model->GetItemById(obj_idx);

    wxArrayString input_files;
    if (from_galery) {
        if (Slic3r::GUI::wxGetApp().gallery_dialog()->show() != wxID_CLOSE)
            Slic3r::GUI::wxGetApp().gallery_dialog()->get_input_files(input_files);
    }
    else
        Slic3r::GUI::wxGetApp().import_model(Slic3r::GUI::wxGetApp().tab_panel()->GetPage(0), input_files);

    if (input_files.IsEmpty())
        return;

    take_snapshot((type == Slic3r::ModelVolumeType::MODEL_PART) ? _L("Load Part") : _L("Load Modifier"));

    std::vector<Slic3r::ModelVolume*> volumes;
    // ! ysFIXME - delete commented code after testing and rename "load_modifier" to something common
    /*
    if (type == ModelVolumeType::MODEL_PART)
        load_part(*(*m_objects)[obj_idx], volumes, type, from_galery);
    else*/
    load_modifier(input_files, *(*m_objects)[obj_idx], volumes, type, from_galery);

    if (volumes.empty())
        return;

    AnkerObjectItem* newItem = reorder_volumes_and_get_selection(obj_idx, [volumes](const Slic3r::ModelVolume* volume) {
        return std::find(volumes.begin(), volumes.end(), volume) != volumes.end(); });

    if (type == Slic3r::ModelVolumeType::MODEL_PART)
        // update printable state on canvas
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_object((size_t)obj_idx);

    m_pObjectBarView->setSelectedObjectSingle(newItem);

    //selection_changed();
    part_selection_changed();
}

void AnkerObjectBar::load_modifier(const wxArrayString& input_files, Slic3r::ModelObject& model_object, std::vector<Slic3r::ModelVolume*>& added_volumes, Slic3r::ModelVolumeType type, bool from_galery)
{
    ANKER_LOG_INFO << "load_modifier: " << getVolumeTypeText(type);

    // ! ysFIXME - delete commented code after testing and rename "load_modifier" to something common
//if (type == ModelVolumeType::MODEL_PART)
//    return;

    wxWindow* parent = Slic3r::GUI::wxGetApp().tab_panel()->GetPage(0);

    wxProgressDialog dlg(_L("Loading") + Slic3r::GUI::dots, "", 100, Slic3r::GUI::wxGetApp().mainframe, wxPD_AUTO_HIDE);
    wxBusyCursor busy;

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const Slic3r::GUI::Selection& selection = scene_selection();
    assert(obj_idx == selection.get_object_idx());

    /** Any changes of the Object's composition is duplicated for all Object's Instances
      * So, It's enough to take a bounding box of a first selected Instance and calculate Part(generic_subobject) position
      */
    int instance_idx = *selection.get_instance_idxs().begin();
    assert(instance_idx != -1);
    if (instance_idx == -1)
        return;

    // Bounding box of the selected instance in world coordinate system including the translation, without modifiers.
    const Slic3r::BoundingBoxf3 instance_bb = model_object.instance_bounding_box(instance_idx);

    // First (any) GLVolume of the selected instance. They all share the same instance matrix.
    const Slic3r::GLVolume* v = selection.get_first_volume();
    const Slic3r::Geometry::Transformation inst_transform = v->get_instance_transformation();
    const Slic3r::Transform3d inv_inst_transform = inst_transform.get_matrix_no_offset().inverse();
    const Slic3r::Vec3d instance_offset = v->get_instance_offset();

    for (size_t i = 0; i < input_files.size(); ++i) {
        const std::string input_file = input_files.Item(i).ToUTF8().data();

        dlg.Update(static_cast<int>(100.0f * static_cast<float>(i) / static_cast<float>(input_files.size())),
            _L("Loading file") + ": " + Slic3r::GUI::from_path(boost::filesystem::path(input_file).filename()));
        dlg.Fit();

        Slic3r::Model model;
        try {
            model = Slic3r::Model::read_from_file(input_file);
        }
        catch (std::exception& e) {
            auto msg = _L("Error!") + " " + input_file + " : " + e.what() + ".";
            Slic3r::GUI::show_error(parent, msg);
            exit(1);
        }

        if (from_galery)
            model.center_instances_around_point(Slic3r::Vec2d::Zero());
        else {
            for (auto object : model.objects) {
                if (model_object.origin_translation != Slic3r::Vec3d::Zero()) {
                    object->center_around_origin();
                    const Slic3r::Vec3d delta = model_object.origin_translation - object->origin_translation;
                    for (auto volume : object->volumes) {
                        volume->translate(delta);
                    }
                }
            }
        }

        Slic3r::TriangleMesh mesh = model.mesh();
        // Mesh will be centered when loading.
        Slic3r::ModelVolume* new_volume = model_object.add_volume(std::move(mesh), type);
        new_volume->name = boost::filesystem::path(input_file).filename().string();
        // set a default extruder value, since user can't add it manually
        new_volume->config.set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(0));
        // update source data
        new_volume->source.input_file = input_file;
        new_volume->source.object_idx = obj_idx;
        new_volume->source.volume_idx = int(model_object.volumes.size()) - 1;
        if (model.objects.size() == 1 && model.objects.front()->volumes.size() == 1)
            new_volume->source.mesh_offset = model.objects.front()->volumes.front()->source.mesh_offset;

        if (from_galery) {
            // Transform the new modifier to be aligned with the print bed.
            new_volume->set_transformation(v->get_instance_transformation().get_matrix_no_offset().inverse());
            const Slic3r::BoundingBoxf3 mesh_bb = new_volume->mesh().bounding_box();
            // Set the modifier position.
            // Translate the new modifier to be pickable: move to the left front corner of the instance's bounding box, lift to print bed.
            const Slic3r::Vec3d offset = Slic3r::Vec3d(instance_bb.max.x(), instance_bb.min.y(), instance_bb.min.z()) + 0.5 * mesh_bb.size() - instance_offset;
            new_volume->set_offset(inv_inst_transform * offset);
        }

        added_volumes.push_back(new_volume);
    }
}

void AnkerObjectBar::load_generic_subobject(const std::string& type_name, const Slic3r::ModelVolumeType type)
{
    ANKER_LOG_INFO << "load_generic_subobject: " << getVolumeTypeText(type);

    if (type == Slic3r::ModelVolumeType::INVALID) {
        load_shape_object(type_name);
        return;
    }

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const Slic3r::GUI::Selection& selection = scene_selection();
    assert(obj_idx == selection.get_object_idx());

    /** Any changes of the Object's composition is duplicated for all Object's Instances
      * So, It's enough to take a bounding box of a first selected Instance and calculate Part(generic_subobject) position
      */
    int instance_idx = *selection.get_instance_idxs().begin();
    assert(instance_idx != -1);
    if (instance_idx == -1)
        return;

    take_snapshot(_L("Add Generic Subobject"));

    // Selected object
    Slic3r::ModelObject& model_object = *(*m_objects)[obj_idx];
    // Bounding box of the selected instance in world coordinate system including the translation, without modifiers.
    Slic3r::BoundingBoxf3 instance_bb = model_object.instance_bounding_box(instance_idx);

    Slic3r::TriangleMesh mesh = create_mesh(type_name, instance_bb);

    // Mesh will be centered when loading.
    Slic3r::ModelVolume* new_volume = model_object.add_volume(std::move(mesh), type);

    // First (any) GLVolume of the selected instance. They all share the same instance matrix.
    const Slic3r::GLVolume* v = selection.get_first_volume();
    // Transform the new modifier to be aligned with the print bed.
    new_volume->set_transformation(v->get_instance_transformation().get_matrix_no_offset().inverse());
    const Slic3r::BoundingBoxf3 mesh_bb = new_volume->mesh().bounding_box();
    // Set the modifier position.
    Slic3r::Vec3d offset;
    if (type_name == "Slab") {
        Slic3r::Vec3d inst_center = instance_bb.center() - v->get_instance_offset();
        // Slab: Lift to print bed and and push to the center of instance
        offset = Slic3r::Vec3d(inst_center.x(), inst_center.y(), 0.5 * mesh_bb.size().z() + instance_bb.min.z() - v->get_instance_offset().z());
    }
    else {
        // Translate the new modifier to be pickable: move to the left front corner of the instance's bounding box, lift to print bed.
        offset = Slic3r::Vec3d(instance_bb.max.x(), instance_bb.min.y(), instance_bb.min.z()) + 0.5 * mesh_bb.size() - v->get_instance_offset();
    }
    new_volume->set_offset(v->get_instance_transformation().get_matrix_no_offset().inverse() * offset);

    std::string volTypeName = getVolumeTypeText(type);
    const wxString name = _L("Generic") + "-" + _(type_name) + "-" + volTypeName;
    new_volume->name = Slic3r::GUI::into_u8(name);
    // set a default extruder value, since user can't add it manually
    new_volume->config.set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(0));
    new_volume->source.is_from_builtin_objects = true;

    AnkerObjectItem* item = reorder_volumes_and_get_selection(obj_idx, [new_volume](const Slic3r::ModelVolume* volume) { return volume == new_volume; });
    //AnkerObjectItem* volumeItem = new AnkerObjectItem;
    //volumeItem->setType(AnkerObjectItem::ItemType::ITYPE_VOLUME);
    //volumeItem->setVolume(new_volume);
    //volumeItem->setObject(&model_object);
    //m_volumeItemsMap[new_volume] = volumeItem;
    //m_pObjectBarView->addObject(volumeItem);
    m_pObjectBarView->setSelectedObjectSingle(item);

    if (type == Slic3r::ModelVolumeType::MODEL_PART)
        // update printable state on canvas
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_object((size_t)obj_idx);

    if (model_object.is_cut())
        update_info_items(obj_idx);

    //selection_changed();
}

void AnkerObjectBar::change_part_type()
{
    //Slic3r::ModelVolume* volume = get_selected_model_volume();
    //if (!volume)
    //    return;

    //const int obj_idx = get_selected_obj_idx();
    //if (obj_idx < 0) return;

    //const Slic3r::ModelVolumeType type = volume->type();
    //const Slic3r::ModelObject* obj = object(obj_idx);
    //if (type == Slic3r::ModelVolumeType::MODEL_PART)
    //{
    //    int model_part_cnt = 0;
    //    for (auto vol : obj->volumes) {
    //        if (vol->type() == Slic3r::ModelVolumeType::MODEL_PART)
    //            ++model_part_cnt;
    //    }

    //    if (model_part_cnt == 1) {
    //        Slic3r::GUI::show_error(nullptr, _(L("You can't change a type of the last solid part of the object.")));
    //        return;
    //    }
    //}

    //const bool is_cut_object = obj->is_cut();
    //wxArrayString                   names;
    //std::vector<Slic3r::ModelVolumeType>    types;
    //types.reserve(5);
    //if (!is_cut_object) {
    //    for (const wxString& name : { _L("Part"),                     _L("Negative Volume") })
    //        names.Add(name);
    //    for (const Slic3r::ModelVolumeType type_id : { Slic3r::ModelVolumeType::MODEL_PART, Slic3r::ModelVolumeType::NEGATIVE_VOLUME })
    //        types.emplace_back(type_id);
    //}

    //if (printer_technology() != Slic3r::ptSLA) {
    //    names.Add(_L("Modifier"));
    //    types.emplace_back(Slic3r::ModelVolumeType::PARAMETER_MODIFIER);
    //}

    //if (!volume->text_configuration.has_value()) {
    //    for (const wxString& name : { _L("Support Blocker"),          _L("Support Enforcer") })
    //        names.Add(name);
    //    for (const Slic3r::ModelVolumeType type_id : { Slic3r::ModelVolumeType::SUPPORT_BLOCKER, Slic3r::ModelVolumeType::SUPPORT_ENFORCER })
    //        types.emplace_back(type_id);
    //}

    //int selection = 0;
    //if (auto it = std::find(types.begin(), types.end(), type); it != types.end())
    //    selection = it - types.begin();

    //auto choice = Slic3r::GUI::wxGetApp().GetSingleChoiceIndex(_L("Type:"), _L("Select type of part"), names, selection);
    //const auto new_type = choice >= 0 ? types[choice] : Slic3r::ModelVolumeType::INVALID;

    //if (new_type == type || new_type == Slic3r::ModelVolumeType::INVALID)
    //    return;

    //take_snapshot(_L("Change Part Type"));

    //volume->set_type(new_type);
    //wxDataViewItemArray sel = reorder_volumes_and_get_selection(obj_idx, [volume](const Slic3r::ModelVolume* vol) { return vol == volume; });
    //if (!sel.IsEmpty())
    //    select_item(sel.front());
}

void AnkerObjectBar::setListMaxHeight(int maxHeight)
{
    if (m_pObjectBarView)
        m_pObjectBarView->setListMaxHeight(maxHeight);
}

void AnkerObjectBar::paste_volumes_into_list(int obj_idx, const std::vector<Slic3r::ModelVolume*>& volumes)
{
    if ((obj_idx < 0) || ((int)m_objects->size() <= obj_idx))
        return;

    if (volumes.empty())
        return;

    //wxDataViewItemArray items = reorder_volumes_and_get_selection(obj_idx, [volumes](const ModelVolume* volume) {
    //    return std::find(volumes.begin(), volumes.end(), volume) != volumes.end(); });
    //if (items.size() > 1) {
    //    m_selection_mode = smVolume;
    //    m_last_selected_item = wxDataViewItem(nullptr);
    //}

    add_volumes_to_object_in_list(obj_idx);

    changed_object(int(obj_idx));

    //select_items(items);
    bar_selection_changed();
}

void AnkerObjectBar::paste_objects_into_list(const std::vector<size_t>& object_idxs)
{
    if (object_idxs.empty())
        return;

    for (const size_t object : object_idxs)
    {
        add_object_to_list(object);
    }

    Slic3r::GUI::wxGetApp().plater()->changed_objects(object_idxs);

    //select_items(items);
    bar_selection_changed();
}

void AnkerObjectBar::split()
{
    ANKER_LOG_INFO << "object split";

    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(m_pObjectBarView->getSelectedObject(), objectIndex, volumeIndex, instanceIndex);
    if (objectIndex < 0)
        return;

    //if (!get_volume_by_item(objectIndex)) return;
    if (volumeIndex < 0)
        volumeIndex = 0;
    Slic3r::ModelVolume* volume = (*m_objects)[objectIndex]->volumes[volumeIndex];
    Slic3r::DynamicPrintConfig& config = printer_config();
    const Slic3r::ConfigOption* nozzle_dmtrs_opt = config.option("nozzle_diameter", false);
    const auto nozzle_dmrs_cnt = (nozzle_dmtrs_opt == nullptr) ? size_t(1) : dynamic_cast<const Slic3r::ConfigOptionFloats*>(nozzle_dmtrs_opt)->values.size();
    if (!volume->is_splittable()) {
        wxMessageBox(_(L("The selected object couldn't be split because it contains only one part.")));
        return;
    }

    take_snapshot(_(L("Split to Parts")));

    // Before splitting volume we have to remove all custom supports, seams, and multimaterial painting.
    Slic3r::GUI::wxGetApp().plater()->clear_before_change_mesh(objectIndex, _u8L("Custom supports, seams and multimaterial painting were "
        "removed after splitting the object."));

    volume->split(nozzle_dmrs_cnt);

    (*m_objects)[objectIndex]->input_file.clear();

    wxBusyCursor wait;

    add_volumes_to_object_in_list(objectIndex);

    changed_object(objectIndex);
    // update printable state for new volumes on canvas3D
    Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_object(objectIndex);

    // After removing custom supports, seams, and multimaterial painting, we have to update info about the object to remove information about
    // custom supports, seams, and multimaterial painting in the right panel.
    //Slic3r::GUI::wxGetApp().obj_list()->update_info_items(objectIndex);
}

bool AnkerObjectBar::get_volume_by_item(int obj_idx)
{
    if (obj_idx < 0)
        return false;

    return (*m_objects)[obj_idx]->volumes.size() == 1;
}

void AnkerObjectBar::merge(bool to_multipart_object)
{
    ANKER_LOG_INFO << "object merge " << to_multipart_object;

    // merge selected objects to the multipart object
    if (to_multipart_object) {
        std::set<Slic3r::ModelObject*> objs;
        int selectedCount = m_pObjectBarView->getSelectedCount();
        for (int i = 0; i < selectedCount; i++)
        {
            AnkerObjectItem* selectedItem = m_pObjectBarView->getSelectedObject(i);
            objs.insert(selectedItem->getObject());
        }
        if (objs.size() == 0)
            return;

        std::vector<int> obj_idxs;
        for (Slic3r::ModelObject* obj : objs)
        {
            obj_idxs.push_back(getObjectIndex(obj, *m_objects));
        }

        Slic3r::GUI::Plater::TakeSnapshot snapshot(Slic3r::GUI::wxGetApp().plater(), _L("Merge"));

        // resulted objects merge to the one
        Slic3r::Model* model = (*m_objects)[0]->get_model();
        Slic3r::ModelObject* new_object = model->add_object();
        new_object->name = _u8L("Merged");
        Slic3r::ModelConfig& config = new_object->config;

        for (int obj_idx : obj_idxs) {
            Slic3r::ModelObject* object = (*m_objects)[obj_idx];

            const Slic3r::Geometry::Transformation& transformation = object->instances[0]->get_transformation();
            const Slic3r::Vec3d scale = transformation.get_scaling_factor();
            const Slic3r::Vec3d mirror = transformation.get_mirror();
            const Slic3r::Vec3d rotation = transformation.get_rotation();

            if (object->id() == (*m_objects)[obj_idxs.front()]->id()) {
                new_object->add_instance();
                new_object->instances[0]->printable = false;
            }
            new_object->instances[0]->printable |= object->instances[0]->printable;

            const Slic3r::Transform3d& volume_offset_correction = transformation.get_matrix();

            // merge volumes
            for (const Slic3r::ModelVolume* volume : object->volumes) {
                Slic3r::ModelVolume* new_volume = new_object->add_volume(*volume);

                //set rotation
                const Slic3r::Vec3d vol_rot = new_volume->get_rotation() + rotation;
                new_volume->set_rotation(vol_rot);

                // set scale
                const Slic3r::Vec3d vol_sc_fact = new_volume->get_scaling_factor().cwiseProduct(scale);
                new_volume->set_scaling_factor(vol_sc_fact);

                // set mirror
                const Slic3r::Vec3d vol_mirror = new_volume->get_mirror().cwiseProduct(mirror);
                new_volume->set_mirror(vol_mirror);

                // set offset
                const Slic3r::Vec3d vol_offset = volume_offset_correction * new_volume->get_offset();
                new_volume->set_offset(vol_offset);
            }
            new_object->sort_volumes(Slic3r::GUI::wxGetApp().app_config->get_bool("order_volumes"));

            // merge settings
            auto new_opt_keys = config.keys();
            const Slic3r::ModelConfig& from_config = object->config;
            auto opt_keys = from_config.keys();

            for (auto& opt_key : opt_keys) {
                if (find(new_opt_keys.begin(), new_opt_keys.end(), opt_key) == new_opt_keys.end()) {
                    const Slic3r::ConfigOption* option = from_config.option(opt_key);
                    if (!option) {
                        // if current option doesn't exist in prints.get_edited_preset(),
                        // get it from default config values
                        option = Slic3r::DynamicPrintConfig::new_from_defaults_keys({ opt_key })->option(opt_key);
                    }
                    config.set_key_value(opt_key, option->clone());
                }
            }
            // save extruder value if it was set
            if (object->volumes.size() == 1 && find(opt_keys.begin(), opt_keys.end(), CONFIG_EXTRUDER_KEY) != opt_keys.end()) {
                Slic3r::ModelVolume* volume = new_object->volumes.back();
                const Slic3r::ConfigOption* option = from_config.option(CONFIG_EXTRUDER_KEY);
                if (option)
                    volume->config.set_key_value(CONFIG_EXTRUDER_KEY, option->clone());
            }

            // merge layers
            for (const auto& range : object->layer_config_ranges)
                new_object->layer_config_ranges.emplace(range);
        }

        new_object->center_around_origin();
        new_object->translate_instances(-new_object->origin_translation);
        new_object->origin_translation = Slic3r::Vec3d::Zero();

        //init asssmble transformation
        Slic3r::Geometry::Transformation t = new_object->instances[0]->get_transformation();
        new_object->instances[0]->set_assemble_transformation(t);

        // remove selected objects
        for (Slic3r::ModelObject* obj : objs)
        {
            delete_from_model_and_list(obj);
        }

        // Add new object(merged) to the object_list
        add_object_to_list(m_objects->size() - 1);
        update_selections_on_canvas();

        // update printable state for new volumes on canvas3D
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_object(int(model->objects.size()) - 1);
    }
    //// merge all parts to the one single object
    //// all part's settings will be lost
    //else {
    //    wxDataViewItem item = GetSelection();
    //    if (!item)
    //        return;
    //    const int obj_idx = m_objects_model->GetIdByItem(item);
    //    if (obj_idx == -1)
    //        return;

    //    Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Merge all parts to the one single object"));

    //    ModelObject* model_object = (*m_objects)[obj_idx];
    //    model_object->merge();

    //    m_objects_model->DeleteVolumeChildren(item);

    //    changed_object(obj_idx);
    //}
}

bool AnkerObjectBar::can_split_instances()
{
    const Slic3r::GUI::Selection& selection = scene_selection();
    return selection.is_multiple_full_instance() || selection.is_single_full_instance();
}

bool AnkerObjectBar::can_merge_to_multipart_object()
{
    //if (has_selected_cut_object())
    //    return false;

    //wxDataViewItemArray sels;
    //GetSelections(sels);
    //if (sels.IsEmpty())
    //    return false;

    //// should be selected just objects
    //for (wxDataViewItem item : sels)
    //    if (!(m_objects_model->GetItemType(item) & (itObject | itInstance)))
    //        return false;

    return true;
}

bool AnkerObjectBar::can_merge_to_single_object()
{
    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(m_pObjectBarView->getSelectedObject(), objectIndex, volumeIndex, instanceIndex);
    if (objectIndex < 0)
        return false;

    // selected object should be multipart
    return (*m_objects)[objectIndex]->volumes.size() > 1;
}

int AnkerObjectBar::get_selected_obj_idx()
{
    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(m_pObjectBarView->getSelectedObject(), objectIndex, volumeIndex, instanceIndex);
    return objectIndex;
}

void AnkerObjectBar::get_selection_indexes(std::vector<int>& obj_idxs, std::vector<int>& vol_idxs)
{
}

void AnkerObjectBar::del_settings_from_config()
{
    if (m_pObjectBarView->getSelectedCount() > 1)
        return;

    AnkerObjectItem* selectedItem = m_pObjectBarView->getSelectedObject();
    const bool is_layer_settings = selectedItem->getType() == AnkerObjectItem::ITYPE_LAYER;

    take_snapshot(_(L("Delete Settings")));

    m_config = &get_item_config(selectedItem);

    // keep the extruder index config
    double extruderIndex = -1;
    if (m_config->has(CONFIG_EXTRUDER_KEY))
        extruderIndex = m_config->get().opt_int(CONFIG_EXTRUDER_KEY);

    m_config->reset();

    if (extruderIndex > -1)
        m_config->set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(extruderIndex));
    
    if (!is_layer_settings)
    {
        selectedItem->enableCustomConfig(false);
        m_pObjectBarView->updateObject(selectedItem);
    }
    else
    {
        int obj_idx = getObjectIndex(selectedItem->getObject(), *m_objects);
        m_config->assign_config(get_default_layer_config(obj_idx));
    }

    m_config = nullptr;

    changed_object();
}

void AnkerObjectBar::del_instances_from_object(const int obj_idx)
{
    auto& instances = (*m_objects)[obj_idx]->instances;
    if (instances.size() <= 1)
        return;

    take_snapshot(_(L("Delete All Instances from Object")));

    while (instances.size() > 1)
        instances.pop_back();

    (*m_objects)[obj_idx]->invalidate_bounding_box(); // ? #ys_FIXME

    changed_object(obj_idx);
}

void AnkerObjectBar::bar_selection_changed()
{
    ANKER_LOG_INFO << "bar_selection_changed";

    //if (m_prevent_list_events) return;

    //fix_multiselection_conflicts();

    //fix_cut_selection();

    //// update object selection on Plater
    //if (!m_prevent_canvas_selection_update)
        update_selections_on_canvas();

    //if (const wxDataViewItem item = GetSelection())
    //{
    //    const ItemType type = m_objects_model->GetItemType(item);
    //    // to correct visual hints for layers editing on the Scene
    //    if (type & (itLayer | itLayerRoot)) {
    //        wxGetApp().obj_layers()->reset_selection();

    //        if (type & itLayerRoot)
    //            wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
    //        else {
    //            wxGetApp().obj_layers()->set_selectable_range(m_objects_model->GetLayerRangeByItem(item));
    //            wxGetApp().obj_layers()->update_scene_from_editor_selection();
    //        }
    //    }
    //}

    part_selection_changed();
}

int AnkerObjectBar::getObjectIndex(Slic3r::ModelObject* target, std::vector<Slic3r::ModelObject*>& targetVector)
{
    for (int i = 0; i < targetVector.size(); i++)
    {
        if (targetVector[i] == target)
            return i;
    }
    return -1;
}

int AnkerObjectBar::getVolumeIndex(Slic3r::ModelVolume* target, std::vector<Slic3r::ModelVolume*>& targetVector)
{
    for (int i = 0; i < targetVector.size(); i++)
    {
        if (targetVector[i] == target)
            return i;
    }
    return -1;
}

bool AnkerObjectBar::getItemID(AnkerObjectItem* item, int& objectIndex, int& volumeIndex, int& instanceIndex)
{
    if (item == nullptr) {
        return false;
    }

    Slic3r::ModelObject* modelObject = item->getObject();
    Slic3r::ModelVolume* modelVolume = item->getVolume();
    Slic3r::ModelInstance* modelInstance = item->getInstance();

    if (item->getType() == AnkerObjectItem::ITYPE_VOLUME)
    {
        objectIndex = getObjectIndex(modelVolume->get_object(), *m_objects);
        volumeIndex = getVolumeIndex(modelVolume, modelVolume->get_object()->volumes);
        instanceIndex = -1;
    }
    else if (item->getType() == AnkerObjectItem::ITYPE_INSTANCE)
    {
        objectIndex = getObjectIndex(modelObject, *m_objects);
        volumeIndex = -1;
        instanceIndex = item->getInstanceIndex();
    }
    else/* if (item->getType() == AnkerObjectItem::ITYPE_OBJECT)*/
    {
        objectIndex = getObjectIndex(modelObject, *m_objects);
        volumeIndex = -1;
        instanceIndex = -1;

        return false;
    }

    return true;
}

void AnkerObjectBar::unselect_objects()
{
}

void AnkerObjectBar::select_item_all_children()
{
    m_pObjectBarView->selectAll();

    bar_selection_changed();
}

void AnkerObjectBar::scene_selection_changed()
{
    ANKER_LOG_INFO << "scene_selection_changed";

    Slic3r::GUI::Selection& selection = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_selection();
    //std::set<unsigned int> select_idxs = selection.get_object_idxs();
    //int single_obj_idx = selection.get_object_idx();
    //const Slic3r::GUI::Selection::InstanceIdxsList select_ins_ids = single_obj_idx > -1 ? selection.get_instance_idxs() : Slic3r::GUI::Selection::InstanceIdxsList();
    const Slic3r::GUI::Selection::IndicesList& select_vol_ids = selection.get_volume_idxs();

    //const bool is_part = selection.is_single_volume_or_modifier() && !selection.is_any_connector();
    //if (is_part)
    //    return;
    
    m_pObjectBarView->setSelectedObjectSingle(nullptr);

    if (selection.is_any_volume() || selection.is_any_modifier())
    {
        //Slic3r::ModelObject* singleObject = single_obj_idx < 0 || single_obj_idx >= m_objects->size() ? nullptr : m_objects->at(single_obj_idx);
        //if (singleObject)
        {
            for (auto itr = select_vol_ids.begin(); itr != select_vol_ids.end(); itr++)
            {
                int glVolIndex = *itr;
                if (glVolIndex < 0 /*|| volIndex >= singleObject->volumes.size()*/)
                    continue;

                int objIndex = selection.get_volume(glVolIndex)->object_idx();
                int volIndex = selection.get_volume(glVolIndex)->volume_idx();

                if (objIndex < 0 || volIndex < 0 || volIndex >= m_objects->at(objIndex)->volumes.size())
                    continue;

                Slic3r::ModelObject* obj = m_objects->at(objIndex);
                Slic3r::ModelVolume* volume = obj->volumes[volIndex];
				auto volItr = m_volumeItemsMap[obj].find(volume);
                AnkerObjectItem* volItem = volItr->second;
                if (volItem)
                    m_pObjectBarView->setSelectedObjectMulti(volItem);
            }
        }
    }
    else
    {
        for (auto itr = select_vol_ids.begin(); itr != select_vol_ids.end(); itr++)
        {
            Slic3r::GLVolume* vol = selection.get_volume(*itr);
            if (vol)
            {
                int objIdx = vol->object_idx();
                int insIdx = vol->instance_idx();

                Slic3r::ModelObject* obj = objIdx < 0 || objIdx >= m_objects->size() ? nullptr : m_objects->at(objIdx);
                if (obj)
                {
                    if (obj->instances.size() <= 1)
                    {
                        AnkerObjectItem* insItem = m_objectItemsMap[obj];
                        if (insItem)
                            m_pObjectBarView->setSelectedObjectMulti(insItem);
                    }
                    else if (insIdx >= 0 && insIdx < m_instanceItemsMap[obj].size())
                    {
                        AnkerObjectItem* insItem = m_instanceItemsMap[obj][insIdx];
                        if (insItem)
                            m_pObjectBarView->setSelectedObjectMulti(insItem);
                    }
                }
            }
        }
    }

    part_selection_changed();
}

void AnkerObjectBar::changed_object(const int obj_idx)
{
    Slic3r::GUI::wxGetApp().plater()->changed_object(obj_idx < 0 ? get_selected_obj_idx() : obj_idx);
}

void AnkerObjectBar::part_selection_changed()
{
    ANKER_LOG_INFO << "part_selection_changed";

    {
        //if (m_extruder_editor) m_extruder_editor->Hide();
        //int obj_idx = -1;
        //int volume_id = -1;
        //m_config = nullptr;
        //wxString og_name = wxEmptyString;

        //bool update_and_show_manipulations = false;
        //bool update_and_show_settings = false;
        //bool update_and_show_layers = false;

        //bool enable_manipulation{ true };
        //bool disable_ss_manipulation{ false };
        //bool disable_ununiform_scale{ false };

        //Slic3r::GUI::ECoordinatesType coordinates_type = Slic3r::GUI::wxGetApp().aobj_manipul()->get_coordinates_type();

        //const auto item = GetSelection();

        //Slic3r::GUI::GLGizmosManager& gizmos_mgr = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager();

        //if (item && m_objects_model->GetItemType(item) == itInfo && m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors) {
        //    og_name = _L("Cut Connectors information");

        //    update_and_show_manipulations = true;
        //    enable_manipulation = false;
        //    disable_ununiform_scale = true;
        //}
        //else if (multiple_selection() || (item && m_objects_model->GetItemType(item) == itInstanceRoot)) {
        //    const Selection& selection = scene_selection();

        //    if (selection.is_single_full_object()) {
        //        og_name = _L("Object manipulation");
        //        coordinates_type = Slic3r::GUI::ECoordinatesType::World;
        //        update_and_show_manipulations = true;

        //        obj_idx = selection.get_object_idx();
        //        Slic3r::ModelObject* object = (*m_objects)[obj_idx];
        //        m_config = &object->config;
        //        disable_ss_manipulation = object->is_cut();
        //    }
        //    else {
        //        og_name = _L("Group manipulation");
        //        coordinates_type = Slic3r::GUI::ECoordinatesType::World;

        //        // don't show manipulation panel for case of all Object's parts selection 
        //        update_and_show_manipulations = !selection.is_single_full_instance();

        //        if (int obj_idx = selection.get_object_idx(); obj_idx >= 0) {
        //            if (selection.is_any_volume() || selection.is_any_modifier())
        //                enable_manipulation = !(*m_objects)[obj_idx]->is_cut();
        //            else// if (item && m_objects_model->GetItemType(item) == itInstanceRoot)
        //                disable_ss_manipulation = (*m_objects)[obj_idx]->is_cut();
        //        }
        //        else {
        //            wxDataViewItemArray sels;
        //            GetSelections(sels);
        //            if (selection.is_single_full_object() || selection.is_multiple_full_instance()) {
        //                int obj_idx = m_objects_model->GetObjectIdByItem(sels.front());
        //                disable_ss_manipulation = (*m_objects)[obj_idx]->is_cut();
        //            }
        //            else if (selection.is_mixed() || selection.is_multiple_full_object()) {
        //                std::map<CutObjectBase, std::set<int>> cut_objects;

        //                // find cut objects
        //                for (auto item : sels) {
        //                    int obj_idx = m_objects_model->GetObjectIdByItem(item);
        //                    const Slic3r::ModelObject* obj = object(obj_idx);
        //                    if (obj->is_cut()) {
        //                        if (cut_objects.find(obj->cut_id) == cut_objects.end())
        //                            cut_objects[obj->cut_id] = std::set<int>{ obj_idx };
        //                        else
        //                            cut_objects.at(obj->cut_id).insert(obj_idx);
        //                    }
        //                }

        //                // check if selected cut objects are "full selected"
        //                for (auto cut_object : cut_objects)
        //                    if (cut_object.first.check_sum() != cut_object.second.size()) {
        //                        disable_ss_manipulation = true;
        //                        break;
        //                    }
        //                disable_ununiform_scale = !cut_objects.empty();
        //            }
        //        }
        //    }
        //}
        //else {
        //    if (item) {
        //        const ItemType type = m_objects_model->GetItemType(item);
        //        const wxDataViewItem parent = m_objects_model->GetParent(item);
        //        const ItemType parent_type = m_objects_model->GetItemType(parent);
        //        obj_idx = m_objects_model->GetObjectIdByItem(item);
        //        Slic3r::ModelObject* object = (*m_objects)[obj_idx];

        //        if (parent == wxDataViewItem(nullptr)
        //            || type == itInfo) {
        //            og_name = _L("Object manipulation");
        //            m_config = &object->config;
        //            if (!scene_selection().is_single_full_instance() || coordinates_type > ECoordinatesType::Instance)
        //                coordinates_type = Slic3r::GUI::ECoordinatesType::World;
        //            update_and_show_manipulations = true;

        //            if (type == itInfo) {
        //                InfoItemType info_type = m_objects_model->GetInfoItemType(item);
        //                switch (info_type)
        //                {
        //                case InfoItemType::VariableLayerHeight:
        //                {
        //                    Slic3r::GUI::wxGetApp().plater()->toggle_layers_editing(true);
        //                    break;
        //                }
        //                case InfoItemType::CustomSupports:
        //                case InfoItemType::CustomSeam:
        //                case InfoItemType::MmuSegmentation:
        //                {
        //                    Slic3r::GUI::GLGizmosManager::EType gizmo_type = info_type == InfoItemType::CustomSupports ? Slic3r::GUI::GLGizmosManager::EType::FdmSupports :
        //                        info_type == InfoItemType::CustomSeam ? Slic3r::GUI::GLGizmosManager::EType::Seam :
        //                        Slic3r::GUI::GLGizmosManager::EType::MmuSegmentation;
        //                    if (gizmos_mgr.get_current_type() != gizmo_type)
        //                        gizmos_mgr.open_gizmo(gizmo_type);
        //                    break;
        //                }
        //                case InfoItemType::Sinking:
        //                default: { break; }
        //                }
        //            }
        //            else
        //                disable_ss_manipulation = object->is_cut();
        //        }
        //        else {
        //            if (type & itSettings) {
        //                if (parent_type & itObject) {
        //                    og_name = _L("Object Settings to modify");
        //                    m_config = &object->config;
        //                }
        //                else if (parent_type & itVolume) {
        //                    og_name = _L("Part Settings to modify");
        //                    volume_id = m_objects_model->GetVolumeIdByItem(parent);
        //                    m_config = &object->volumes[volume_id]->config;
        //                }
        //                else if (parent_type & itLayer) {
        //                    og_name = _L("Layer range Settings to modify");
        //                    m_config = &get_item_config(parent);
        //                }
        //                update_and_show_settings = true;
        //            }
        //            else if (type & itVolume) {
        //                og_name = _L("Part manipulation");
        //                volume_id = m_objects_model->GetVolumeIdByItem(item);
        //                m_config = &object->volumes[volume_id]->config;
        //                update_and_show_manipulations = true;
        //                const Slic3r::ModelVolume* volume = object->volumes[volume_id];
        //                enable_manipulation = !(object->is_cut() && (volume->is_cut_connector() || volume->is_model_part()));
        //            }
        //            else if (type & itInstance) {
        //                og_name = _L("Instance manipulation");
        //                update_and_show_manipulations = true;

        //                // fill m_config by object's values
        //                m_config = &object->config;
        //                disable_ss_manipulation = object->is_cut();
        //            }
        //            else if (type & (itLayerRoot | itLayer)) {
        //                og_name = type & itLayerRoot ? _L("Height ranges") : _L("Settings for height range");
        //                update_and_show_layers = true;

        //                if (type & itLayer)
        //                    m_config = &get_item_config(item);
        //            }
        //        }
        //    }
        //}

        //m_selected_object_id = obj_idx;

        //if (update_and_show_manipulations) {
        //    Slic3r::GUI::wxGetApp().obj_manipul()->get_og()->set_name(" " + og_name + " ");
        //    if (Slic3r::GUI::wxGetApp().obj_manipul()->get_coordinates_type() != coordinates_type)
        //        Slic3r::GUI::wxGetApp().obj_manipul()->set_coordinates_type(coordinates_type);

        //    if (item) {
        //        Slic3r::GUI::wxGetApp().obj_manipul()->update_item_name(m_objects_model->GetName(item));
        //        Slic3r::GUI::wxGetApp().obj_manipul()->update_warning_icon_state(get_mesh_errors_info(obj_idx, volume_id));
        //    }

        //    if (disable_ss_manipulation)
        //        Slic3r::GUI::wxGetApp().obj_manipul()->DisableScale();
        //    else {
        //        Slic3r::GUI::wxGetApp().obj_manipul()->Enable(enable_manipulation);
        //        if (disable_ununiform_scale)
        //            Slic3r::GUI::wxGetApp().obj_manipul()->DisableUnuniformScale();
        //    }

        //    if (Slic3r::GUI::GLGizmoScale3D* scale = dynamic_cast<Slic3r::GUI::GLGizmoScale3D*>(gizmos_mgr.get_gizmo(Slic3r::GUI::GLGizmosManager::Scale)))
        //        scale->enable_ununiversal_scale(!disable_ununiform_scale);
        //}

        //if (update_and_show_settings)
        //    Slic3r::GUI::wxGetApp().obj_settings()->get_og()->set_name(" " + og_name + " ");

        //if (printer_technology() == Slic3r::PrinterTechnology::ptSLA)
        //    update_and_show_layers = false;
        //else if (update_and_show_layers)
        //    Slic3r::GUI::wxGetApp().obj_layers()->get_og()->set_name(" " + og_name + " ");

        //update_min_height();

        //Sidebar& panel = wxGetApp().sidebar();
        //panel.Freeze();

        //std::string opt_key;
        //if (m_selected_object_id >= 0) {
        //    const ManipulationEditor* const editor = Slic3r::GUI::wxGetApp().obj_manipul()->get_focused_editor();
        //    if (editor != nullptr)
        //        opt_key = editor->get_full_opt_name();
        //}
        //Slic3r::GUI::wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event(opt_key, !opt_key.empty());
        //Slic3r::GUI::wxGetApp().plater()->canvas3D()->enable_moving(enable_manipulation); // ysFIXME
        //wxGetApp().obj_manipul() ->UpdateAndShow(update_and_show_manipulations);
        //wxGetApp().obj_settings()->UpdateAndShow(update_and_show_settings);
        //wxGetApp().obj_layers()  ->UpdateAndShow(update_and_show_layers);
        //wxGetApp().sidebar().show_info_sizer();
        //Slic3r::GUI::wxGetApp().plater()->CalcModelObjectSize();

        //panel.Layout();
        //panel.Thaw();
    }

    Slic3r::GUI::wxGetApp().plater()->sidebarnew().Freeze();

    if (m_pObjectBarView->getSelectedCount() == 1)
    {
        AnkerObjectItem* selectedItem = m_pObjectBarView->getSelectedObject();

        // settings item
        if (m_settingsEditing && selectedItem != m_editingObjectItem)
            enableSettingsPanel(false);          

        // layer item
		AnkerObjectItem::ItemType type = selectedItem->getType();
		if (type == AnkerObjectItem::ITYPE_LAYER || type == AnkerObjectItem::ITYPE_LAYER_GROUP)
		{
            if (m_settingsEditing)
                enableSettingsPanel(false);

			enableLayerPanel(true, selectedItem);
		}
		else if (m_layerEditing)
		{
			enableLayerPanel(false);
		}

        // gizmo item
        auto currentToolType = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_current_type();
        if (selectedItem->getType() == AnkerObjectItem::ITYPE_SEAM)
        {
            m_toolFlag = true;
            //Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().open_gizmo(Slic3r::GUI::GLGizmosManager::EType::Seam);
            if (currentToolType != Slic3r::GUI::GLGizmosManager::EType::Seam)
            {
                if (currentToolType != Slic3r::GUI::GLGizmosManager::EType::Undefined)
                {
                    std::string currentItemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(currentToolType)->get_name(false, false);
                    int currentItemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(currentItemName);
                    Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(currentItemId);
                }

                std::string itemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::Seam)->get_name(false, false);
                int itemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(itemName);
                Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(itemId);
            }
        }
        else if (selectedItem->getType() == AnkerObjectItem::ITYPE_SUPPORT)
        {
            m_toolFlag = true;

            if (currentToolType != Slic3r::GUI::GLGizmosManager::EType::FdmSupports)
            {
                if (currentToolType != Slic3r::GUI::GLGizmosManager::EType::Undefined)
                {
                    std::string currentItemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(currentToolType)->get_name(false, false);
                    int currentItemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(currentItemName);
                    Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(currentItemId);
                }

                std::string itemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::FdmSupports)->get_name(false, false);
                int itemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(itemName);
                Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(itemId);
            }
        }
        else
        {
            if (m_toolFlag)
            {
                m_toolFlag = false;

                if (currentToolType == Slic3r::GUI::GLGizmosManager::EType::Seam)
                {
                    std::string itemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::Seam)->get_name(false, false);
                    int itemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(itemName);
                    Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(itemId);
                }
                else if (currentToolType == Slic3r::GUI::GLGizmosManager::EType::FdmSupports)
                {
                    std::string itemName = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_gizmo(Slic3r::GUI::GLGizmosManager::EType::FdmSupports)->get_name(false, false);
                    int itemId = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_main_toolbar_item_id(itemName);
                    Slic3r::GUI::wxGetApp().plater()->canvas3D()->force_main_toolbar_left_action(itemId);
                }
            }
        }
    }
    else
    {
        if (m_layerEditing)
            enableLayerPanel(false);

        if (m_settingsEditing)
            enableSettingsPanel(false);
    }

    Slic3r::GUI::wxGetApp().plater()->sidebarnew().Thaw();

    Slic3r::GUI::wxGetApp().plater()->CalcModelObjectSize();
}

void AnkerObjectBar::update_selections_on_canvas()
{
    ANKER_LOG_INFO << "update_selections_on_canvas";

    Slic3r::GUI::Selection& selection = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_selection();

    std::vector<unsigned int> volume_idxs;
    std::vector<unsigned int> instance_idxs;
	Slic3r::GUI::Selection::EMode mode = Slic3r::GUI::Selection::Volume;
    bool single_selection = /*sel_cnt == 1*/true;
    auto add_to_selection = [this, &volume_idxs, &instance_idxs, &single_selection](const Slic3r::GUI::Selection& selection,
        int objectIndex, int volumeIndex, int instance_idx, AnkerObjectItem* item)
    {
        {
            //const ItemType& type = m_objects_model->GetItemType(item);
            //const int obj_idx = m_objects_model->GetObjectIdByItem(item);

            //if (type == itVolume) {
            //    const int vol_idx = m_objects_model->GetVolumeIdByItem(item);
            //    std::vector<unsigned int> idxs = selection.get_volume_idxs_from_volume(obj_idx, std::max(instance_idx, 0), vol_idx);
            //    volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            //}
            //else if (type == itInstance) {
            //    const int inst_idx = m_objects_model->GetInstanceIdByItem(item);
            //    mode = Selection::Instance;
            //    std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(obj_idx, inst_idx);
            //    volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            //}
            //else if (type == itInfo) {
            //    if (m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors) {
            //        mode = Selection::Volume;

            //        // When selecting CutConnectors info item, select all object volumes, which are marked as a connector
            //        const ModelObject* obj = object(obj_idx);
            //        for (unsigned int vol_idx = 0; vol_idx < obj->volumes.size(); vol_idx++)
            //            if (obj->volumes[vol_idx]->is_cut_connector()) {
            //                std::vector<unsigned int> idxs = selection.get_volume_idxs_from_volume(obj_idx, std::max(instance_idx, 0), vol_idx);
            //                volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            //            }
            //    }
            //    else {
            //        // When selecting an info item, select one instance of the
            //        // respective object - a gizmo may want to be opened.
            //        int inst_idx = selection.get_instance_idx();
            //        int scene_obj_idx = selection.get_object_idx();
            //        mode = Selection::Instance;
            //        // select first instance, unless an instance of the object is already selected
            //        if (scene_obj_idx == -1 || inst_idx == -1 || scene_obj_idx != obj_idx)
            //            inst_idx = 0;
            //        std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(obj_idx, inst_idx);
            //        volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            //    }
            //}
            //else
            //{
            //    mode = Slic3r::GUI::Selection::Instance;
            //    single_selection &= (obj_idx != selection.get_object_idx());
            //    std::vector<unsigned int> idxs = selection.get_volume_idxs_from_object(obj_idx);
            //    volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            //}
        }

        if (item == nullptr) {
            return;
        }

        ANKER_LOG_INFO << "add_to_selection-->objectIndex: " << objectIndex << " volumeIndex: " << volumeIndex << " instance_idx: " << instance_idx;
        AnkerObjectItem::ItemType type = item->getType();

        //single_selection &= (objectIndex != selection.get_object_idx());
        if (type == AnkerObjectItem::ITYPE_INSTANCE)
        {
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(objectIndex, instance_idx);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        }
        else if (type == AnkerObjectItem::ITYPE_VOLUME)
        {
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_object(objectIndex);
            for (int i = 0; i < idxs.size(); i++)
            {
                if (selection.get_volume(idxs[i])->volume_idx() == volumeIndex)
                {
                    volume_idxs.insert(volume_idxs.end(), idxs[i]);
                    break;
                }
            }         
        }
        else if (type == AnkerObjectItem::ITYPE_SEAM || type == AnkerObjectItem::ITYPE_SUPPORT)
        {
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(objectIndex, 0);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        }
        else/* if (type == AnkerObjectItem::ITYPE_OBJECT)*/
        {
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_object(objectIndex);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        }
    };

    // stores current instance idx before to clear the selection
    int instance_idx = selection.get_instance_idx();
    ANKER_LOG_INFO << "getSelectedCount: " << m_pObjectBarView->getSelectedCount();
    if (m_pObjectBarView->getSelectedCount() <= 0)
    {
        selection.remove_all();
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_gizmos_on_off_state();
        //Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_main_toolbar_state();
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->reset_main_toolbar_toggled_state();
        Slic3r::GUI::wxGetApp().plater()->canvas3D()->render();
        Slic3r::GUI::wxGetApp().plater()->aobject_manipulator()->set_dirty();
        Slic3r::GUI::wxGetApp().plater()->aobject_manipulator()->update_if_dirty();
        return;
	}

    for (int i = 0; i < m_pObjectBarView->getSelectedCount(); i++)
    {
        AnkerObjectItem* selectedItem = m_pObjectBarView->getSelectedObject(i);
        int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
        getItemID(selectedItem, objectIndex, volumeIndex, instanceIndex);
        if (objectIndex >= 0) 
        {
            add_to_selection(selection, objectIndex, volumeIndex, instanceIndex, selectedItem);
        }
    }

    ANKER_LOG_INFO << "volume_idxs size: " << volume_idxs.size();
    if (selection.contains_all_volumes(volume_idxs))
    {
        // remove
        volume_idxs = selection.get_missing_volume_idxs_from(volume_idxs);
        ANKER_LOG_INFO << "missing_volume_idxs size: " << volume_idxs.size();
        if (volume_idxs.size() > 0)
        {
            Slic3r::GUI::Plater::TakeSnapshot snapshot(Slic3r::GUI::wxGetApp().plater(), _(L("Selection-Remove from list")), Slic3r::UndoRedo::SnapshotType::Selection);
            selection.remove_volumes(mode, volume_idxs);
        }
    }
    else
    {
        // add
        // to avoid lost of some volumes in selection
        // check non-selected volumes only if selection mode wasn't changed
        // OR there is no single selection
        //if (selection.get_mode() == mode || !single_selection)
            //volume_idxs = selection.get_unselected_volume_idxs_from(volume_idxs);
        Slic3r::GUI::Plater::TakeSnapshot snapshot(Slic3r::GUI::wxGetApp().plater(), _(L("Selection-Add from list")), Slic3r::UndoRedo::SnapshotType::Selection);
        selection.add_volumes(mode, volume_idxs, single_selection);
    }

    Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_gizmos_on_off_state();
    Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_main_toolbar_state();
    // do not reset the toggled state of the opened gizmo
    //Slic3r::GUI::wxGetApp().plater()->canvas3D()->reset_main_toolbar_toggled_state();
    Slic3r::GUI::wxGetApp().plater()->canvas3D()->render();
    Slic3r::GUI::wxGetApp().plater()->aobject_manipulator()->set_dirty();
    Slic3r::GUI::wxGetApp().plater()->aobject_manipulator()->update_if_dirty();
    Slic3r::GUI::wxGetApp().plater()->SetFocus();
    ANKER_LOG_INFO << "--- update_selections_on_canvas end ---";
}

void AnkerObjectBar::load_shape_object(const std::string& type_name)
{
    ANKER_LOG_INFO << "load_shape_object: " << type_name;

    const Slic3r::GUI::Selection& selection = Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_selection();
    assert(selection.get_object_idx() == -1); // Add nothing is something is selected on 3DScene
    if (selection.get_object_idx() != -1)
        return;

    take_snapshot(_L("Add Shape"));

    // Create mesh
    Slic3r::BoundingBoxf3 bb;
    Slic3r::TriangleMesh mesh = create_mesh(type_name, bb);

    load_mesh_object(mesh, _u8L("Shape") + "-" + _u8L(type_name));
    if (!m_objects->empty())
        m_objects->back()->volumes.front()->source.is_from_builtin_objects = true;
    Slic3r::GUI::wxGetApp().mainframe->update_title();
}

void AnkerObjectBar::load_shape_object_from_gallery()
{
    if (Slic3r::GUI::wxGetApp().plater()->canvas3D()->get_selection().get_object_idx() != -1)
        return;// Add nothing if something is selected on 3DScene

    wxArrayString input_files;
    Slic3r::GUI::GalleryDialog* gallery_dlg = Slic3r::GUI::wxGetApp().gallery_dialog();
    if (gallery_dlg->show() == wxID_CLOSE)
        return;
    gallery_dlg->get_input_files(input_files);
    if (input_files.IsEmpty())
        return;
    load_shape_object_from_gallery(input_files);
}

void AnkerObjectBar::load_shape_object_from_gallery(const wxArrayString& input_files)
{
    std::vector<boost::filesystem::path> paths;
    for (const auto& file : input_files)
        paths.push_back(Slic3r::GUI::into_path(file));

    assert(!paths.empty());
    wxString snapshot_label = (paths.size() == 1 ? _L("Add Shape from Gallery") : _L("Add Shapes from Gallery")) + ": " +
        wxString::FromUTF8(paths.front().filename().string().c_str());
    for (size_t i = 1; i < paths.size(); ++i)
        snapshot_label += ", " + wxString::FromUTF8(paths[i].filename().string().c_str());

    take_snapshot(snapshot_label);
    if (!Slic3r::GUI::wxGetApp().plater()->load_files(paths, true, false).empty())
        Slic3r::GUI::wxGetApp().mainframe->update_title();
}

void AnkerObjectBar::load_mesh_object(const Slic3r::TriangleMesh& mesh,
    const std::string& name, bool center, const Slic3r::TextConfiguration* text_config,
    const Slic3r::Transform3d* transformation)
{
    Slic3r::GUI::PlaterAfterLoadAutoArrange plater_after_load_auto_arrange;
    // Add mesh to model as a new object
    Slic3r::Model& model = Slic3r::GUI::wxGetApp().plater()->model();

#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */

    std::vector<size_t> object_idxs;
    Slic3r::ModelObject* new_object = model.add_object();
    new_object->name = name;
    new_object->add_instance(); // each object should have at list one instance

    Slic3r::ModelVolume* new_volume = new_object->add_volume(mesh);
    new_object->sort_volumes(Slic3r::GUI::wxGetApp().app_config->get_bool("order_volumes"));
    new_volume->name = name;
    if (text_config)
        new_volume->text_configuration = *text_config;
    // set a default extruder value, since user can't add it manually
    new_volume->config.set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(0));
    new_object->invalidate_bounding_box();
    if (transformation) {
        assert(!center);
        Slic3r::Geometry::Transformation tr(*transformation);
        new_object->instances[0]->set_transformation(tr);
    }
    else {
        auto bb = mesh.bounding_box();
        new_object->translate(-bb.center());
        new_object->instances[0]->set_offset(
            center ? Slic3r::to_3d(Slic3r::GUI::wxGetApp().plater()->build_volume().bounding_volume2d().center(), -new_object->origin_translation.z()) :
            bb.center());
    }

    new_object->ensure_on_bed();

    //init assmeble transformation
    Slic3r::Geometry::Transformation t = new_object->instances[0]->get_transformation();
    new_object->instances[0]->set_assemble_transformation(t);

    object_idxs.push_back(model.objects.size() - 1);
#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */

    paste_objects_into_list(object_idxs);

#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */
}

void AnkerObjectBar::update_extruder_in_config(AnkerObjectItem* item, int filamentIndex)
{
    //if (m_prevent_update_extruder_in_config)
    //    return;

    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(item, objectIndex, volumeIndex, instanceIndex);

    if (volumeIndex < 0)
    {
        m_config = &(*m_objects)[objectIndex]->config;
    }
    else
    {
        m_config = &(*m_objects)[objectIndex]->volumes[volumeIndex]->config;
    }

    if (!m_config)
        return;

    take_snapshot(_(L("Change Extruder")));

    m_config->set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(filamentIndex));

    // update scene
    Slic3r::GUI::wxGetApp().plater()->update();
}

void AnkerObjectBar::update_after_undo_redo()
{
    ANKER_LOG_INFO << "update_after_undo_redo";

    //m_prevent_canvas_selection_update = true;

    Slic3r::GUI::Plater::SuppressSnapshots suppress(Slic3r::GUI::wxGetApp().plater());

    // Unselect all objects before deleting them, so that no change of selection is emitted during deletion.

    /* To avoid execution of selection_changed()
     * from wxEVT_DATAVIEW_SELECTION_CHANGED emitted from DeleteAll(),
     * wrap this two functions into m_prevent_list_events *
     * */
    //m_prevent_list_events = true;
    //this->UnselectAll();
    //m_objects_model->DeleteAll();
    //m_prevent_list_events = false;


    clearAll(false);

    int obj_idx = 0;
    std::vector<size_t> obj_idxs;
    obj_idxs.reserve(m_objects->size());
    while (obj_idx < m_objects->size()) {
        add_object_to_list(obj_idx, false);
        obj_idxs.push_back(obj_idx);
        ++obj_idx;
    }

    m_pObjectBarView->Refresh();
    m_pObjectBarView->updateSize();

    scene_selection_changed();

    // update printable states on canvas
    Slic3r::GUI::wxGetApp().plater()->canvas3D()->update_instance_printable_state_for_objects(obj_idxs);
    // update scene
    Slic3r::GUI::wxGetApp().plater()->update();
}

void AnkerObjectBar::update_and_show_object_settings_item()
{

}

void AnkerObjectBar::layers_editing(bool selectFlag, AnkerObjectItem* item)
{
    ANKER_LOG_INFO << "layer init " << (item == nullptr);

    AnkerObjectItem* currentSelectedItem = item ? item : m_pObjectBarView->getSelectedObject();
    Slic3r::ModelObject* targetObject = currentSelectedItem->getObject();
    if (targetObject == nullptr)
        return;

    int obj_idx = getObjectIndex(targetObject, *m_objects);
    AnkerObjectItem* targetObjectItem = m_objectItemsMap[targetObject];
    AnkerObjectItem* groupItem = nullptr;
    auto itr = m_groupItemsMap.find(targetObject);
    if (itr != m_groupItemsMap.end())
        groupItem = itr->second[AnkerObjectItem::ITYPE_LAYER_GROUP];
 
    if (groupItem == nullptr)
    {
        groupItem = new AnkerObjectItem;    
        groupItem->setType(AnkerObjectItem::ItemType::ITYPE_LAYER_GROUP);
        groupItem->setObject(currentSelectedItem->getObject());
        m_groupItemsMap[targetObject][AnkerObjectItem::ITYPE_LAYER_GROUP] = groupItem;
        m_pObjectBarView->addObject(groupItem, targetObjectItem);
    }

    t_layer_config_ranges& ranges = targetObject->layer_config_ranges;

    // set some default value
    if (ranges.empty()) 
    {
        t_layer_height_range defaultHeightRange = { 0.0f, 2.0f };

        take_snapshot(_(L("Add Layers")));
        ranges[defaultHeightRange].assign_config(get_default_layer_config(obj_idx));
    }
    
    for (auto itr = ranges.begin(); itr != ranges.end(); itr++)
    {
        if (m_layerItemsMap[targetObject].find(itr->first) != m_layerItemsMap[targetObject].end())
            continue;

        AnkerObjectItem* newLayerItem = new AnkerObjectItem;
        newLayerItem->setType(AnkerObjectItem::ItemType::ITYPE_LAYER);
        newLayerItem->setObject(currentSelectedItem->getObject());
        newLayerItem->setLayerHeightRange(itr->first);
        newLayerItem->setLayerIndex(0);
        newLayerItem->enableCustomConfig(true);
        m_layerItemsMap[targetObject][itr->first] = newLayerItem;
        m_pObjectBarView->addObject(newLayerItem, groupItem);
    }


    if (selectFlag)
    {
        m_pObjectBarView->selectAll(false);
        m_pObjectBarView->setSelectedObjectSingle(groupItem);
        part_selection_changed();
    }

    m_pObjectBarView->collapseObject(groupItem, false);
    m_pObjectBarView->updateSize();

    //enableLayerPanel(true, groupItem);
}

bool AnkerObjectBar::is_splittable(bool to_objects)
{
    if (m_pObjectBarView->getSelectedCount() != 1)
        return false;

    int objectIndex = -1, volumeIndex = -1, instanceIndex = -1;
    getItemID(m_pObjectBarView->getSelectedObject(), objectIndex, volumeIndex, instanceIndex);
    if (objectIndex < 0) return false;

    if (to_objects)
    {
        if (volumeIndex > -1)
            return false;

        if (objectIndex < 0)
            return false;
        const Slic3r::ModelObject* object = (*m_objects)[objectIndex];
        if (object->is_cut())
            return false;
        if (object->volumes.size() > 1)
            return true;
        return object->volumes[0]->is_splittable();
    }

    if ((*m_objects)[objectIndex]->volumes.size() <= 0)
        return false;

    if (volumeIndex <= -1)
    {
        if ((*m_objects)[objectIndex]->volumes.size() > 1)
            return false;
        else
            return (*m_objects)[objectIndex]->volumes[0]->is_splittable();
    }

    Slic3r::ModelVolume* volume = (*m_objects)[objectIndex]->volumes[volumeIndex];
    return volume->is_splittable();

    //if (objectID < 0 || volumeID < 0 || volumeID >= (*m_objects)[objectID]->volumes.size())
    //    return false;

    //Slic3r::ModelVolume* volume = (*m_objects)[objectID]->volumes[0];

    //return volume->is_splittable();
}

AnkerObjectItem* AnkerObjectBar::add_layer_item(const t_layer_height_range& range, AnkerObjectItem* layers_item, int layer_idx)
{
    ANKER_LOG_INFO << "add layer item " << (layers_item == nullptr);

    if (layers_item == nullptr) return nullptr;

    Slic3r::ModelObject* obj = layers_item->getObject();
    if (obj == nullptr) return nullptr;

    const Slic3r::DynamicPrintConfig& config = obj->layer_config_ranges[range].get();
    if (!config.has(CONFIG_EXTRUDER_KEY))
        return nullptr;

    AnkerObjectItem* newItem = new AnkerObjectItem;
    newItem->setType(AnkerObjectItem::ITYPE_LAYER);
    newItem->setObject(obj);
    newItem->setLayerHeightRange(range);
    newItem->enableCustomConfig(true);
    newItem->setLayerIndex(layer_idx < 0 ? m_layerItemsMap[obj].size() : layer_idx);
    
    m_pObjectBarView->addObject(newItem, m_groupItemsMap[obj][AnkerObjectItem::ITYPE_LAYER_GROUP]);
    m_layerItemsMap[obj][range] = newItem;

    //add_settings_item(layer_item, &config);

    return newItem;
}

bool AnkerObjectBar::edit_layer_range(const t_layer_height_range& range, coordf_t layer_height)
{
    // Use m_selected_object_id instead of get_selected_obj_idx()
    // because of get_selected_obj_idx() return obj_idx for currently selected item.
    // But edit_layer_range(...) function can be called, when Selection in ObjectList could be changed
    AnkerObjectItem* currentItem = m_pObjectBarView->getSelectedObject();
    if (currentItem == nullptr) return false;
    Slic3r::ModelObject* obj = currentItem->getObject();
    if (obj == nullptr) return false;
    auto itr = m_layerItemsMap[obj].find(range);
    if (itr == m_layerItemsMap[obj].end()) return false;
    AnkerObjectItem* editLayerItem = itr->second;
    int obj_idx = getObjectIndex(obj, *m_objects);

    Slic3r::ModelConfig* config = &(obj->layer_config_ranges)[range];
    if (fabs(layer_height - config->opt_float(CONFIG_LAYER_HEIGHT_KEY)) < EPSILON)
        return false;

    const int extruder_idx = config->opt_int(CONFIG_EXTRUDER_KEY);

    if (layer_height >= get_min_layer_height(extruder_idx) &&
        layer_height <= get_max_layer_height(extruder_idx))
    {
        config->set_key_value(CONFIG_LAYER_HEIGHT_KEY, new Slic3r::ConfigOptionFloat(layer_height));
        changed_object(obj_idx);
        return true;
    }

    return false;
}

bool AnkerObjectBar::edit_layer_range(const t_layer_height_range& range, const t_layer_height_range& new_range, bool suppress_ui_update)
{
    // Use m_selected_object_id instead of get_selected_obj_idx()
    // because of get_selected_obj_idx() return obj_idx for currently selected item.
    // But edit_layer_range(...) function can be called, when Selection in ObjectList could be changed
    AnkerObjectItem* currentItem = m_pObjectBarView->getSelectedObject();
    if (currentItem == nullptr) return false;
    Slic3r::ModelObject* obj = currentItem->getObject();
    if (obj == nullptr) return false;
    auto itr = m_layerItemsMap[obj].find(range);
    if (itr == m_layerItemsMap[obj].end()) return false;
    AnkerObjectItem* editLayerItem = itr->second;
    int obj_idx = getObjectIndex(obj, *m_objects);

    take_snapshot(_L("Edit Height Range"));

    auto& ranges = obj->layer_config_ranges;

    {
        Slic3r::ModelConfig config = std::move(ranges[range]);
        ranges.erase(range);
        ranges[new_range] = std::move(config);
    }

    editLayerItem->setLayerHeightRange(new_range);
    m_layerItemsMap[obj][new_range] = editLayerItem;
    m_layerItemsMap[obj].erase(range);

    changed_object(obj_idx);

    //auto itr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_LAYER_GROUP);
    //if (itr == m_groupItemsMap[obj].end()) return;
    //AnkerObjectItem* layerGroup = itr->second;
    //// To avoid update selection after deleting of a selected item (under GTK)
    //// set m_prevent_list_events to true
    //m_prevent_list_events = true;
    //m_objects_model->DeleteChildren(root_item);

    //if (root_item.IsOk()) {
    //    // create Layer item(s) according to the layer_config_ranges
    //    for (const auto& r : ranges)
    //        add_layer_item(r.first, root_item);
    //}

    //// if this function was invoked from wxEVT_CHANGE_SELECTION selected item could be other than itLayer or itLayerRoot      
    //if (!dont_update_ui && (sel_type & (itLayer | itLayerRoot)))
    //    select_item(sel_type & itLayer ? m_objects_model->GetItemByLayerRange(obj_idx, new_range) : root_item);

    //Expand(root_item);

    //m_prevent_list_events = false;
    return true;
}

wxString AnkerObjectBar::can_add_new_range_after_current(std::pair<double, double> current_range)
{
    const int obj_idx = get_selected_obj_idx();
    assert(obj_idx >= 0);
    if (obj_idx < 0)
        // This should not happen.
        return "AnkerObjectBar assert";

    auto& ranges = object(obj_idx)->layer_config_ranges;
    auto it_range = ranges.find(current_range);
    assert(it_range != ranges.end());
    if (it_range == ranges.end())
        // This shoudl not happen.
        return "AnkerObjectBar assert";

    auto it_next_range = it_range;
    if (++it_next_range == ranges.end())
        // Adding a layer after the last layer is always possible.
        return "";

    if (const std::pair<coordf_t, coordf_t>& next_range = it_next_range->first; current_range.second <= next_range.first)
    {
        if (current_range.second == next_range.first) {
            if (next_range.second - next_range.first < get_min_layer_height(it_next_range->second.opt_int(CONFIG_EXTRUDER_KEY)) + get_min_layer_height(0) - EPSILON)
                return _(L("Cannot insert a new layer range after the current layer range.\n"
                    "The next layer range is too thin to be split to two\n"
                    "without violating the minimum layer height."));
        }
        else if (next_range.first - current_range.second < get_min_layer_height(0) - EPSILON) {
            return _(L("Cannot insert a new layer range between the current and the next layer range.\n"
                "The gap between the current layer range and the next layer range\n"
                "is thinner than the minimum layer height allowed."));
        }
    }
    else
        return _(L("Cannot insert a new layer range after the current layer range.\n"
            "Current layer range overlaps with the next layer range."));

    // All right, new layer height range could be inserted.
    return "";
}

void AnkerObjectBar::del_layer_range(const t_layer_height_range& range)
{
    ANKER_LOG_INFO << "del layer range";

    AnkerObjectItem* item = m_pObjectBarView->getSelectedObject();
    if (item == nullptr) return;

    AnkerObjectItem* parentItem = nullptr;
    if (item->getType() == AnkerObjectItem::ITYPE_LAYER)
        parentItem = m_pObjectBarView->getParentObject(item);

    Slic3r::ModelObject* targetObject = item->getObject();
    if (targetObject == nullptr) return;

    auto itr = m_layerItemsMap[targetObject].find(range);
    if (itr == m_layerItemsMap[targetObject].end())
        return;

    AnkerObjectItem* delItem = itr->second;

    AnkerObjectItem* selectable_item = m_objectItemsMap[targetObject];

    del_subobject_item(delItem);

    // select layer group
    if (item->getType() == AnkerObjectItem::ITYPE_LAYER_GROUP)
        m_pObjectBarView->setSelectedObjectSingle(item);
    else{
        //AnkerObjectItem* parentItem = m_pObjectBarView->getParentObject(item);
        if (parentItem)
            m_pObjectBarView->setSelectedObjectSingle(parentItem);
        else
            m_pObjectBarView->setSelectedObjectSingle(selectable_item);
    }
    part_selection_changed();
}

void AnkerObjectBar::add_layer_range_after_current(t_layer_height_range current_range)
{
    ANKER_LOG_INFO << "add layer range after current: " << current_range.first << ", " << current_range.second;

    AnkerObjectItem* currentItem = m_pObjectBarView->getSelectedObject();
    if (currentItem == nullptr) return;
    Slic3r::ModelObject* targetObject = currentItem->getObject();
    if (targetObject == nullptr) return;
    int obj_idx = getObjectIndex(targetObject, *m_objects);
    auto itr = m_layerItemsMap[targetObject].find(current_range);
    if (itr == m_layerItemsMap[targetObject].end()) return;
    AnkerObjectItem* currentLayerItem = itr->second;
    AnkerObjectItem* selectedItem = nullptr;

    auto& ranges = targetObject->layer_config_ranges;
    auto it_range = ranges.find(current_range);
    assert(it_range != ranges.end());
    if (it_range == ranges.end())
        // This shoudl not happen.
        return;

    auto it_next_range = it_range;
    bool changed = false;
    if (++ it_next_range == ranges.end())
    {
        // Adding a new layer height range after the last one.
        take_snapshot(_(L("Add Height Range")));
        changed = true;

        const t_layer_height_range new_range = { current_range.second, current_range.second + 2. };
        ranges[new_range].assign_config(get_default_layer_config(obj_idx));
        selectedItem = add_layer_item(new_range, currentLayerItem);
    }
    else if (const std::pair<coordf_t, coordf_t> &next_range = it_next_range->first; current_range.second <= next_range.first)
    {
        const int layer_idx = currentLayerItem->getLayerIndex();
        assert(layer_idx >= 0);
        if (layer_idx >= 0) 
        {
            if (current_range.second == next_range.first)
            {
                // Splitting the next layer height range to two.
                const auto old_config = ranges.at(next_range);
                const coordf_t delta = next_range.second - next_range.first;
                // Layer height of the current layer.
                const coordf_t old_min_layer_height = get_min_layer_height(old_config.opt_int(CONFIG_EXTRUDER_KEY));
                // Layer height of the layer to be inserted.
                const coordf_t new_min_layer_height = get_min_layer_height(0);
                if (delta >= old_min_layer_height + new_min_layer_height - EPSILON) {
                    const coordf_t middle_layer_z = (new_min_layer_height > 0.5 * delta) ?
	                    next_range.second - new_min_layer_height :
                    	next_range.first + std::max(old_min_layer_height, 0.5 * delta);
                    t_layer_height_range new_range = { middle_layer_z, next_range.second };

                    Slic3r::GUI::Plater::TakeSnapshot snapshot(Slic3r::GUI::wxGetApp().plater(), _(L("Add Height Range")));
                    changed = true;

                    // create new 2 layers instead of deleted one
                    // delete old layer

                    AnkerObjectItem* item = m_layerItemsMap[targetObject][next_range];
                    del_subobject_item(item);

                    ranges[new_range] = old_config;
                    add_layer_item(new_range, currentLayerItem, layer_idx);

                    new_range = { current_range.second, middle_layer_z };
                    ranges[new_range].assign_config(get_default_layer_config(obj_idx));
                    selectedItem = add_layer_item(new_range, currentLayerItem, layer_idx);
                }
            }
            else if (next_range.first - current_range.second >= get_min_layer_height(0) - EPSILON)
            {
                // Filling in a gap between the current and a new layer height range with a new one.
                take_snapshot(_(L("Add Height Range")));
                changed = true;

                const t_layer_height_range new_range = { current_range.second, next_range.first };
                ranges[new_range].assign_config(get_default_layer_config(obj_idx));
                selectedItem = add_layer_item(new_range, currentLayerItem, layer_idx);
            }
        }
    }
    else if (const std::pair<coordf_t, coordf_t>& next_range = it_next_range->first; current_range.second > next_range.first)
    {
        ANKER_LOG_ERROR<<"current_range.second > next_range.first  ("<< current_range.second<<" >"<< next_range.first<<")";
    }

    if (changed)
        changed_object(obj_idx);

    // The layer range panel is updated even if this function does not change the layer ranges, as the panel update
    // may have been postponed from the "kill focus" event of a text field, if the focus was lost for the "add layer" button.
    // select item to update layers sizer
    if (selectedItem) {
        m_pObjectBarView->setSelectedObjectSingle(selectedItem);
        part_selection_changed();
    }
}

Slic3r::DynamicPrintConfig AnkerObjectBar::get_default_layer_config(const int obj_idx)
{
    const Slic3r::DynamicPrintConfig& from_config = printer_technology() == Slic3r::ptFFF ?
        Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config :
        Slic3r::GUI::wxGetApp().preset_bundle->sla_prints.get_edited_preset().config;

    Slic3r::DynamicPrintConfig config = from_config;
    coordf_t layer_height = object(obj_idx)->config.has(CONFIG_LAYER_HEIGHT_KEY) ?
        object(obj_idx)->config.opt_float(CONFIG_LAYER_HEIGHT_KEY) :
        Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config.opt_float(CONFIG_LAYER_HEIGHT_KEY);
    config.set_key_value(CONFIG_LAYER_HEIGHT_KEY, new Slic3r::ConfigOptionFloat(layer_height));
    config.set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(0));

    return config;
}

Slic3r::ModelConfig& AnkerObjectBar::get_item_config(AnkerObjectItem* item)
{
    const AnkerObjectItem::ItemType type = item->getType();

    int obj_idx, vol_idx, ins_idx;
    getItemID(item, obj_idx, vol_idx, ins_idx);

    assert(obj_idx >= 0 || ((type == AnkerObjectItem::ITYPE_VOLUME) && vol_idx >= 0));
    return type == AnkerObjectItem::ITYPE_VOLUME ? (*m_objects)[obj_idx]->volumes[vol_idx]->config :
        type == AnkerObjectItem::ITYPE_LAYER ? (*m_objects)[obj_idx]->layer_config_ranges[item->getLayerHeightRange()] :
        (*m_objects)[obj_idx]->config;
}

void AnkerObjectBar::add_settings_to_item(bool panelFlag, AnkerObjectItem* item)
{
    ANKER_LOG_INFO << "item add setttings";

    AnkerObjectItem* selectedItem = item ? item : m_pObjectBarView->getSelectedObject();
    if (selectedItem == nullptr) return;
    AnkerObjectItem::ItemType type = selectedItem->getType();
    if (type != AnkerObjectItem::ITYPE_OBJECT && type != AnkerObjectItem::ITYPE_VOLUME && type != AnkerObjectItem::ITYPE_LAYER 
        && type != AnkerObjectItem::ITYPE_INSTANCE)
        return;

    if (type == AnkerObjectItem::ITYPE_INSTANCE)
        selectedItem = m_objectItemsMap[selectedItem->getObject()];

    m_config = &get_item_config(selectedItem);

    assert(m_config);

    if (!selectedItem->hasCustomConfig())
    {
        const wxString snapshot_text = type == AnkerObjectItem::ITYPE_LAYER ? _L("Add Settings for Layers") :
            type == AnkerObjectItem::ITYPE_VOLUME ? _L("Add Settings for Sub-object") :
            _L("Add Settings for Object");
        take_snapshot(snapshot_text);

        const Slic3r::DynamicPrintConfig& from_config = printer_technology() == Slic3r::ptFFF ?
            Slic3r::GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config :
            Slic3r::GUI::wxGetApp().preset_bundle->sla_prints.get_edited_preset().config;

        // keep the extruder index config
        double extruderIndex = -1;
        if (m_config->has(CONFIG_EXTRUDER_KEY))
            extruderIndex = m_config->get().opt_int(CONFIG_EXTRUDER_KEY);

        m_config->assign_config(from_config);

        if (extruderIndex > -1)
            m_config->set_key_value(CONFIG_EXTRUDER_KEY, new Slic3r::ConfigOptionInt(extruderIndex));

        selectedItem->enableCustomConfig(true);
        m_pObjectBarView->updateObject(selectedItem);

        //m_pObjectBarView->Refresh();
    }

    if (panelFlag)
        enableSettingsPanel(true, selectedItem);
}

void AnkerObjectBar::enableSettingsPanel(bool enable, AnkerObjectItem* item)
{
    ANKER_LOG_INFO << "enable setting panel";

    m_settingsEditing = enable;

    if (enable && item)
    {
        m_editingObjectItem = item;

        wxString objName = item->getText();
        Slic3r::ModelConfig* itemCfg = &(get_item_config(item)/*.getPrintCfg()*/);
        //Slic3r::GUI::wxGetApp().plater()->sidebarnew().showRightMenuParameterPanel(objName, itemCfg);

        if (!m_bindFlag)
        {
            m_bindFlag = true;
            Slic3r::GUI::wxGetApp().plater()->sidebarnew().Bind(wxCUSTOMEVT_ANKER_DELETE_CFG_EDIT, [this](wxCommandEvent& event) {
                del_settings_from_config();
            });

            Slic3r::GUI::wxGetApp().plater()->sidebarnew().Bind(wxCUSTOMEVT_ANKER_EXIT_RIGHT_MENU_PANEL, [this, item, itemCfg](wxCommandEvent& event) {
                });
        }
    }
    else
    {
        m_editingObjectItem = nullptr;

        //Slic3r::GUI::wxGetApp().plater()->sidebarnew().exitRightMenuParameterPanel();
    }
}

void AnkerObjectBar::enableLayerPanel(bool enable, AnkerObjectItem* item)
{
    ANKER_LOG_INFO << "enable layer panel";

    m_layerEditing = enable;

    if (enable)
        m_editingObjectItem = m_objectItemsMap[item->getObject()];
    else
        m_editingObjectItem = nullptr;

	//Slic3r::GUI::wxGetApp().obj_layers()->reset_selection();
	//Slic3r::GUI::wxGetApp().obj_layers()->UpdateAndShow(enable);
}

void AnkerObjectBar::update_info_items(size_t obj_idx, AnkerObjectItem::ItemType type)
{
    ANKER_LOG_INFO << "update items info";

    if (obj_idx < 0 || obj_idx >= m_objects->size())
        return;
    Slic3r::ModelObject* obj = m_objects->at(obj_idx);
    auto objItr = m_objectItemsMap.find(obj);
    if (objItr == m_objectItemsMap.end())
        return;

    bool hasTypeInfo = false;
    for (Slic3r::ModelVolume* mv : obj->volumes)
    {
        if (type == AnkerObjectItem::ItemType::ITYPE_SEAM && !mv->seam_facets.empty())
            hasTypeInfo = true;
        else if (type == AnkerObjectItem::ItemType::ITYPE_SUPPORT && !mv->supported_facets.empty())
            hasTypeInfo = true;

        if (hasTypeInfo)
            break;
    }

    AnkerObjectItem* objItem = objItr->second;
    auto infoItr = m_groupItemsMap[obj].find(type);
    if (infoItr == m_groupItemsMap[obj].end())
    {
        if (hasTypeInfo)
        {
            AnkerObjectItem* infoItem = new AnkerObjectItem;
            infoItem->setType(type);
            infoItem->setObject(obj);
            m_groupItemsMap[obj][type] = infoItem;
            m_pObjectBarView->addObject(infoItem, objItem);

            m_pObjectBarView->updateSize();
        }
    }
    else if (!hasTypeInfo)
    {
        delViewItem(infoItr->second, true);
    }
}

void AnkerObjectBar::update_info_items(size_t obj_idx, wxDataViewItemArray* selections, bool added_object)
{

}

void AnkerObjectBar::clearAll(bool sizeUpdateFlag)
{
    ANKER_LOG_INFO << "clear all items: " << sizeUpdateFlag;

    m_toolFlag = false;
    m_config = nullptr;

    if (m_settingsEditing)
        enableSettingsPanel(false);
    if (m_layerEditing)
        enableLayerPanel(false);

    m_pObjectBarView->clearAll(sizeUpdateFlag);

    for (auto itr = m_objectItemsMap.begin(); itr != m_objectItemsMap.end(); itr++)
    {
        if (itr->second)
            delete itr->second;
    }
    m_objectItemsMap.clear();

    for (auto itr = m_volumeItemsMap.begin(); itr != m_volumeItemsMap.end(); itr++)
    {
        for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
            delete itr2->second;
    }
    m_volumeItemsMap.clear();

    for (auto itr = m_instanceItemsMap.begin(); itr != m_instanceItemsMap.end(); itr++)
    {
        for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
            delete* itr2;
    }
    m_instanceItemsMap.clear();

    for (auto itr = m_layerItemsMap.begin(); itr != m_layerItemsMap.end(); itr++)
    {
        for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
            delete itr2->second;
    }
    m_layerItemsMap.clear();

    for (auto itr = m_groupItemsMap.begin(); itr != m_groupItemsMap.end(); itr++)
    {
        for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
            delete itr2->second;
    }
    m_groupItemsMap.clear();
}

void AnkerObjectBar::delViewItem(AnkerObjectItem* item, bool refresh)
{
    ANKER_LOG_INFO << "delViewItem";

    if (item == nullptr)
        return;

    AnkerObjectItem::ItemType type = item->getType();
    Slic3r::ModelObject* obj = item->getObject();
    Slic3r::ModelVolume* vol = item->getVolume();

    m_pObjectBarView->removeObject(item, true, refresh);

    switch (type)
    {
    case AnkerObjectItem::ITYPE_OBJECT:
    {
        auto volItr = m_volumeItemsMap.find(obj);
        if (volItr != m_volumeItemsMap.end())
        {
            for (auto itr = volItr->second.begin(); itr != volItr->second.end(); itr++)
            {
                delete itr->second;
            }
            m_volumeItemsMap.erase(volItr);
        }

        auto insItr = m_instanceItemsMap.find(obj);
        if (insItr != m_instanceItemsMap.end())
        {
            for (auto itr = insItr->second.begin(); itr != insItr->second.end(); itr++)
            {
                delete* itr;
            }
            m_instanceItemsMap.erase(insItr);
        }

        auto layerItr = m_layerItemsMap.find(obj);
        if (layerItr != m_layerItemsMap.end())
        {
            for (auto itr = layerItr->second.begin(); itr != layerItr->second.end(); itr++)
            {
                delete itr->second;
            }
            m_layerItemsMap.erase(layerItr);
        }

        auto groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_INSTANCE_GROUP);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_INSTANCE_GROUP);

        groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_LAYER_GROUP);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_LAYER_GROUP);

        groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_SEAM);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_SEAM);

        groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_SUPPORT);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_SUPPORT);

        m_objectItemsMap.erase(obj);
        delete item;
        item = nullptr;
    }
    break;
    case AnkerObjectItem::ITYPE_VOLUME:
    {
        m_volumeItemsMap[obj].erase(vol);
        delete item;
        item = nullptr;

        if (m_volumeItemsMap[obj].size() == 1)
        {
            m_pObjectBarView->removeObject(m_volumeItemsMap[obj].begin()->second, true, refresh);
            delete m_volumeItemsMap[obj][0];
            m_volumeItemsMap.erase(obj);
        }
    }
    break;
    case AnkerObjectItem::ITYPE_INSTANCE:
    {
        for (int i = item->getInstanceIndex() + 1; i < m_instanceItemsMap[obj].size(); i++)
        {
            m_instanceItemsMap[obj][i]->setInstanceIndex(i - 1);
        }

        m_instanceItemsMap[obj].erase(m_instanceItemsMap[obj].begin() + item->getInstanceIndex());
        delete item;
        item = nullptr;

        auto groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_INSTANCE_GROUP);
        if (m_instanceItemsMap[obj].size() == 1)
        {
            if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            {
                m_pObjectBarView->removeObject(groupItr->second, true, refresh);
                delete groupItr->second;
            }
            m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_INSTANCE_GROUP);
            
            delete m_instanceItemsMap[obj][0];
            m_instanceItemsMap.erase(obj);
        }
        else
        {
            m_pObjectBarView->updateObject(groupItr->second);
        }
    }
    break;
    case AnkerObjectItem::ITYPE_INSTANCE_GROUP:
    {
        auto insItr = m_instanceItemsMap.find(obj);
        if (insItr != m_instanceItemsMap.end())
        {
            for (auto itr = insItr->second.begin(); itr != insItr->second.end(); itr++)
            {
                delete* itr;
            }
            m_instanceItemsMap.erase(insItr);
        }

        auto groupItr = m_groupItemsMap[obj].find(type);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(type);
    }
    break;
    case AnkerObjectItem::ITYPE_LAYER:
    {
        m_layerItemsMap[obj].erase(item->getLayerHeightRange());
        delete item;
        item = nullptr;

        if (m_layerItemsMap[obj].size() == 0)
        {
            auto groupItr = m_groupItemsMap[obj].find(AnkerObjectItem::ITYPE_LAYER_GROUP);
            if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            {
                m_pObjectBarView->removeObject(groupItr->second, true, refresh);
                delete groupItr->second;
            }
            m_groupItemsMap[obj].erase(AnkerObjectItem::ITYPE_LAYER_GROUP);
        }
    }
    break;
    case AnkerObjectItem::ITYPE_LAYER_GROUP:
    {
        auto insItr = m_layerItemsMap.find(obj);
        if (insItr != m_layerItemsMap.end())
        {
            for (auto itr = insItr->second.begin(); itr != insItr->second.end(); itr++)
            {
                delete itr->second;
            }
            m_layerItemsMap.erase(insItr);
        }

        auto groupItr = m_groupItemsMap[obj].find(type);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(type);
    }
    break;
    case AnkerObjectItem::ITYPE_SEAM:
    case AnkerObjectItem::ITYPE_SUPPORT:
    {
        auto groupItr = m_groupItemsMap[obj].find(type);
        if (groupItr != m_groupItemsMap[obj].end() && groupItr->second)
            delete groupItr->second;
        m_groupItemsMap[obj].erase(type);
    }
    break;
    default:
        break;
    }
}

void AnkerObjectBar::OnItemClicked(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "item clicked";

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* item = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());

    // TODO: ctrl + click / shift + click
    m_pObjectBarView->setSelectedObjectSingle(item);
    bar_selection_changed();
}

void AnkerObjectBar::OnItemSettingsClicked(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "item settings clicked";

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* item = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    if (item)
    {
        m_pObjectBarView->setSelectedObjectSingle(item);

        if (m_layerEditing)
            enableLayerPanel(false);

        enableSettingsPanel(true, item);
    }
}

void AnkerObjectBar::OnItemPrintableClicked(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "item printable clicked";

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* item = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    toggle_printable_state(item);
}

void AnkerObjectBar::OnItemFilamentClicked(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "item filament clicked";

    wxVariant* pData = (wxVariant*)(event.GetClientData());
    AnkerObjectItem* item = (AnkerObjectItem*)(pData->GetList()[0]->GetVoidPtr());
    Slic3r::ModelObject* objectData = (Slic3r::ModelObject*)(pData->GetList()[1]->GetVoidPtr());
    Slic3r::ModelVolume* volumeData = (Slic3r::ModelVolume*)(pData->GetList()[2]->GetVoidPtr());
    int newFilamentIndex = pData->GetList()[3]->GetInteger();

    auto itemItr = m_objectItemsMap.find(objectData);
    auto volumeMapItr = m_volumeItemsMap.find(objectData);
    if (volumeData == nullptr)
    {
        if (itemItr == m_objectItemsMap.end() || item != itemItr->second)
            return;
    }
    else
    {
        if (volumeMapItr == m_volumeItemsMap.end())
            return;

        auto volumeItr = volumeMapItr->second.find(volumeData);
        if (volumeItr == volumeMapItr->second.end() || item != volumeItr->second)
            return;
    }

    update_extruder_in_config(item, newFilamentIndex + 1);
}

void AnkerObjectBar::OnKeyEvent(wxKeyEvent& event)
{
    ANKER_LOG_INFO << "item key: " << event.GetKeyCode();
	if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK)
        bar_remove();
	else if (wxGetKeyState(wxKeyCode('A')) && wxGetKeyState(WXK_CONTROL/*WXK_SHIFT*/))
		select_item_all_children();
	else if (wxGetKeyState(wxKeyCode('C')) && wxGetKeyState(WXK_CONTROL))
		copy();
	else if (wxGetKeyState(wxKeyCode('V')) && wxGetKeyState(WXK_CONTROL))
		paste();
	else if (wxGetKeyState(wxKeyCode('Y')) && wxGetKeyState(WXK_CONTROL))
		redo();
	else if (wxGetKeyState(wxKeyCode('Z')) && wxGetKeyState(WXK_CONTROL))
		undo();
	else if (wxGetKeyState(wxKeyCode('+')))
		increase_instances();
	else if (wxGetKeyState(wxKeyCode('-')))
		decrease_instances();
	else if (wxGetKeyState(wxKeyCode('p')))
		toggle_printable_state(m_pObjectBarView->getSelectedObject());
	else
		event.Skip();
}

AnkerObjectItem::AnkerObjectItem()
    : m_valid(false)
    , m_itemType(ITYPE_COUNT)
    , m_pObjectData(nullptr)
    , m_pVolumeData(nullptr)
    , m_pInstanceData(nullptr)
    , m_customConfigFlag(false)
    , m_instanceIndex(-1)
    , m_layerIndex(-1)
{
}

AnkerObjectItem::~AnkerObjectItem()
{
}

void AnkerObjectItem::setType(ItemType type)
{
    m_itemType = type;
    if (m_itemType != ITYPE_COUNT)
        m_valid = true;
}

void AnkerObjectItem::setObject(Slic3r::ModelObject* object)
{
    m_pObjectData = object;
}

void AnkerObjectItem::setVolume(Slic3r::ModelVolume* volume)
{
    m_pVolumeData = volume;
}

void AnkerObjectItem::setInstance(Slic3r::ModelInstance* instance)
{
    m_pInstanceData = instance;
}

void AnkerObjectItem::setInstanceIndex(int index)
{
    m_instanceIndex = index;
}

void AnkerObjectItem::setLayerIndex(int index)
{
    m_layerIndex = index;
}

void AnkerObjectItem::setLayerHeightRange(t_layer_height_range range)
{
    m_layerRange = range;
}

void AnkerObjectItem::enableCustomConfig(bool enable)
{
    m_customConfigFlag = enable;
}

bool AnkerObjectItem::isGroup()
{
    switch (m_itemType)
    {
    case AnkerObjectItem::ITYPE_OBJECT:
        return m_pObjectData ? m_pObjectData->volumes.size() > 1 : false;
    case AnkerObjectItem::ITYPE_VOLUME:
        return false;
    case AnkerObjectItem::ITYPE_INSTANCE:
        return false;
    case AnkerObjectItem::ITYPE_INSTANCE_GROUP:
        return true;
    case AnkerObjectItem::ITYPE_LAYER:
        return false;
    case AnkerObjectItem::ITYPE_LAYER_GROUP:
        return true;
    case AnkerObjectItem::ITYPE_SEAM:
        return false;
    case AnkerObjectItem::ITYPE_SUPPORT:
        return false;
    case AnkerObjectItem::ITYPE_COUNT:
        return false;
    default:
        return false;
    }
}

bool AnkerObjectItem::hasIcon()
{
    return false;
}

// TODO
wxImage& AnkerObjectItem::getIcon()
{
    switch (m_itemType)
    {
    case AnkerObjectItem::ITYPE_OBJECT:
        break;
    case AnkerObjectItem::ITYPE_VOLUME:
        break;
    case AnkerObjectItem::ITYPE_INSTANCE:
        break;
    case AnkerObjectItem::ITYPE_INSTANCE_GROUP:
        break;
    case AnkerObjectItem::ITYPE_LAYER:
        break;
    case AnkerObjectItem::ITYPE_LAYER_GROUP:
        break;
    case AnkerObjectItem::ITYPE_SEAM:
        break;
    case AnkerObjectItem::ITYPE_SUPPORT:
        break;
    case AnkerObjectItem::ITYPE_COUNT:
        break;
    default:
        break;
    }

    wxImage temp;
    return temp;
}

wxString AnkerObjectItem::getText()
{
    wxString text = "";

    // TODO
    switch (m_itemType)
    {
    case AnkerObjectItem::ITYPE_OBJECT:
        text = m_pObjectData ? get_item_name(m_pObjectData->name, m_pObjectData->is_text()) : "NO_DATA";
        break;
    case AnkerObjectItem::ITYPE_VOLUME:
        text = m_pVolumeData ? get_item_name(m_pVolumeData->name, m_pVolumeData->is_text()) : "NO_DATA";
        break;
    case AnkerObjectItem::ITYPE_INSTANCE:
        text = wxString::Format(_L("Instance %d"), m_instanceIndex + 1);
        break;
    case AnkerObjectItem::ITYPE_INSTANCE_GROUP:
        text = _L("Instances");
        break;
    case AnkerObjectItem::ITYPE_LAYER:
        text = _L(wxString::Format(wxT("%.2f"), m_layerRange.first) + "-" + wxString::Format(wxT("%.2f"), m_layerRange.second) + " mm");
        break;
    case AnkerObjectItem::ITYPE_LAYER_GROUP:
        text = _L("common_slice_toolpannel_heightranges");
        break;
    case AnkerObjectItem::ITYPE_SEAM:
        text = _L("common_slice_toolpannel_seampaint");
        break;
    case AnkerObjectItem::ITYPE_SUPPORT:
        text = _L("common_slice_toolpannel_paintsupport");
        break;
    case AnkerObjectItem::ITYPE_COUNT:
        text = _L("N/A");
        break;
    default:
        text = _L("N/A");
        break;
    }

    return text;
}

bool AnkerObjectItem::getPrintable()
{
    bool printable = false;

    // TODO: instance
    if (m_itemType == ITYPE_OBJECT && m_pObjectData)
    {
        if (m_pObjectData->instances.size() > 0)
        {
            for (int i = 0; i < m_pObjectData->instances.size(); i++)
            {
                printable = printable || m_pObjectData->instances[i]->printable;
            }
        }
        else
            printable = m_pObjectData->printable;
    }
    else if (m_itemType == ITYPE_INSTANCE && m_pInstanceData)
        printable = m_pInstanceData->printable;

    return printable;
}

bool AnkerObjectItem::hasSetting()
{
    return m_customConfigFlag && (m_itemType == ITYPE_OBJECT || m_itemType == ITYPE_LAYER || m_itemType == ITYPE_VOLUME);
}

int AnkerObjectItem::getFilamentIndex()
{
    int filamentIndex = -1;
    if (m_itemType == ITYPE_OBJECT && m_pObjectData)
        filamentIndex = m_pObjectData->config.has(CONFIG_EXTRUDER_KEY) ? m_pObjectData->config.extruder() : 1;
    else if (m_itemType == ITYPE_VOLUME && m_pVolumeData)
        filamentIndex = m_pVolumeData->config.has(CONFIG_EXTRUDER_KEY) && m_pVolumeData->config.extruder() > 0 ? m_pVolumeData->config.extruder() : (m_pObjectData->config.has(CONFIG_EXTRUDER_KEY) ? m_pObjectData->config.extruder() : 1);

    const std::vector<Slic3r::GUI::SFilamentInfo>& filamentInfos = Slic3r::GUI::wxGetApp().plater()->sidebarnew().getEditFilamentList();
    filamentIndex = std::min((int)(filamentInfos.size()), std::max(filamentIndex, 1));

    return filamentIndex;
}

wxColour AnkerObjectItem::getFilamentColour()
{
    const std::vector<Slic3r::GUI::SFilamentInfo>& filamentInfos = Slic3r::GUI::wxGetApp().plater()->sidebarnew().getEditFilamentList();
    int filamentIndex = getFilamentIndex();

    // modify by Samule, check index to avoid crash
    if (filamentIndex < 1 || filamentInfos.size() ==0)
    {
        return wxColour("#ffffff");
    }
    else
    {
        return wxColour(filamentInfos[filamentIndex - 1].wxStrColor);
    }
}

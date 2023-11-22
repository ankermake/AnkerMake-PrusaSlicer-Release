#ifndef _ANKER_OBJECT_MANIPULATOR_H_
#define _ANKER_OBJECT_MANIPULATOR_H_

#include "wx/wx.h"
#include "libslic3r/Point.hpp"
#include "GUI_Geometry.hpp"
#include "Selection.hpp"

#include <functional>


wxDECLARE_EVENT(wxANKEREVT_AOM_RETURN, wxCommandEvent);

class AnkerObjectManipulator : public wxPanel
{
    enum EditorType
    {
        EDITOR_TRANSLATE = 0,
        EDITOR_ROTATE,
        EDITOR_SCALE_FACTOR,
        EDITOR_SCALE_SIZE,
        EDITOR_TYPE_COUNT
    };
    enum EditorAxisType
    {
        EDITOR_X = 0,
        EDITOR_Y,
        EDITOR_Z,
        EDITOR_AXIS_COUNT
    };
public:
    AnkerObjectManipulator(wxWindow* parent);
    ~AnkerObjectManipulator();

    void set_uniform_scaling(const bool use_uniform_scale);
    bool get_uniform_scaling() const { return m_uniform_scale; }

    void set_coordinates_type(Slic3r::GUI::ECoordinatesType type);
    Slic3r::GUI::ECoordinatesType get_coordinates_type() const;
    bool is_world_coordinates() const { return m_coordinates_type == Slic3r::GUI::ECoordinatesType::World; }
    bool is_instance_coordinates() const { return m_coordinates_type == Slic3r::GUI::ECoordinatesType::Instance; }
    bool is_local_coordinates() const { return m_coordinates_type == Slic3r::GUI::ECoordinatesType::Local; }

    void set_dirty() { m_dirty = true; }
    // Called from the App to update the UI if dirty.
    void update_if_dirty(bool force = false);

    void update_ui_from_settings();

    void reset_cache() { m_cache.reset(); }

    void setReturnFunc(std::function<void(void)> func) { m_pReturnFunc = func; };

private:
    void initUI();

    void reset();
    void OnTextEdit(EditorType editor, EditorAxisType axis, double newValue);

    void reset_settings_value();
    void update_settings_value(const Slic3r::GUI::Selection& selection);

    // change values 
    void change_position_value(int axis, double value);
    void change_rotation_value(int axis, double value);
    void change_scale_value(int axis, double value);
    void change_size_value(int axis, double value);
    void do_scale(int axis, const Slic3r::Vec3d& scale) const;
    void do_size(int axis, const Slic3r::Vec3d& scale) const;

public:
    static const double in_to_mm;
    static const double mm_to_in;

private:
    struct Cache
    {
        Slic3r::Vec3d position;
        Slic3r::Vec3d position_rounded;
        Slic3r::Vec3d rotation;
        Slic3r::Vec3d rotation_rounded;
        Slic3r::Vec3d scale;
        Slic3r::Vec3d scale_rounded;
        Slic3r::Vec3d size;
        Slic3r::Vec3d size_inches;
        Slic3r::Vec3d size_rounded;

        wxString move_label_string;
        wxString rotate_label_string;
        wxString scale_label_string;

        Cache() { reset(); }
        void reset()
        {
            position = position_rounded = Slic3r::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
            rotation = rotation_rounded = Slic3r::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
            scale = scale_rounded = Slic3r::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
            size = size_rounded = Slic3r::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
            move_label_string = wxString();
            rotate_label_string = wxString();
            scale_label_string = wxString();
        }
        bool is_valid() const { return position != Slic3r::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX); }
    };

    Cache m_cache;

    bool m_dirty;
    bool m_textUpdating;
    bool m_uniform_scale;
    bool m_imperial_units;
    Slic3r::Vec3d           m_new_position;
    Slic3r::Vec3d           m_new_rotation;
    Slic3r::Vec3d           m_new_scale;
    Slic3r::Vec3d           m_new_size;
    bool            m_new_enabled{ true };
    Slic3r::GUI::ECoordinatesType m_coordinates_type;
    std::function<void(void)> m_pReturnFunc;

    wxTextCtrl* m_editor[EDITOR_TYPE_COUNT][EDITOR_AXIS_COUNT];
};

#endif // _ANKER_OBJECT_MANIPULATOR_H_


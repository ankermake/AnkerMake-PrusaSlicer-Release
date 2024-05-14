#ifndef slic3r_GUI_CalibrationTempDialog_hpp_
#define slic3r_GUI_CalibrationTempDialog_hpp_

#include "CalibrationAbstractDialog.hpp"
#include "libslic3r/calib.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/TextInput.hpp"

namespace Slic3r { 
namespace GUI {

class CalibrationTempDialog : public wxDialog
{
public:
    CalibrationTempDialog(wxWindow* parent, wxWindowID id, Plater* plater);
    ~CalibrationTempDialog();
    //void on_dpi_changed(const wxRect& suggested_rect) override;

protected:

    virtual void on_start(wxCommandEvent& event);
    virtual void on_filament_type_changed(wxCommandEvent& event);
    Calib_Params m_params;

    wxRadioBox* m_rbFilamentType;
    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    Button* m_btnStart;
    Plater* m_plater;

};

} // namespace GUI
} // namespace Slic3r

#endif

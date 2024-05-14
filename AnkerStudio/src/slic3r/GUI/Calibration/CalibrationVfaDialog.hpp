#ifndef slic3r_GUI_CalibrationVfaDialog_hpp_
#define slic3r_GUI_CalibrationVfaDialog_hpp_

#include "CalibrationAbstractDialog.hpp"
#include "libslic3r/calib.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/TextInput.hpp"
namespace Slic3r { 
namespace GUI {

class CalibrationVfaDialog : public wxDialog
{

public:
    CalibrationVfaDialog(wxWindow* parent, wxWindowID id, Plater* plater);
    ~CalibrationVfaDialog();
    //void on_dpi_changed(const wxRect& suggested_rect) override;

protected:
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    Button* m_btnStart;
    Plater* m_plater;

};

} // namespace GUI
} // namespace Slic3r

#endif

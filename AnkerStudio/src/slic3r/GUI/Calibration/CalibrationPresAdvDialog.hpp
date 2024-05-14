#ifndef slic3r_GUI_CalibrationPresAdvDialog_hpp_
#define slic3r_GUI_CalibrationPresAdvDialog_hpp_

#include "CalibrationAbstractDialog.hpp"
#include "libslic3r/calib.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/TextInput.hpp"
#include "slic3r/GUI/AnkerCheckBox.hpp"


namespace Slic3r {
namespace GUI {

class CalibrationPresAdvDialog : public wxDialog
{

public:
    CalibrationPresAdvDialog(wxWindow* parent, wxWindowID id, Plater* plater);
	~CalibrationPresAdvDialog();
	//void on_dpi_changed(const wxRect& suggested_rect) override;
	void on_show(wxShowEvent& event);
protected:
	void reset_params();
	virtual void on_start(wxCommandEvent& event);
	virtual void on_extruder_type_changed(wxCommandEvent& event);
	virtual void on_method_changed(wxCommandEvent& event);

protected:
	bool m_bDDE;
	Calib_Params m_params;


	wxRadioBox* m_rbExtruderType;
	wxRadioBox* m_rbMethod;
	TextInput* m_tiStartPA;
	TextInput* m_tiEndPA;
	TextInput* m_tiPAStep;
	AnkerCheckBox* m_cbPrintNum;
	Button* m_btnStart;

	Plater* m_plater;

};

} // namespace GUI
} // namespace Slic3r

#endif

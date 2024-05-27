#ifndef slic3r_GUI_CalibrationAbstractDialog_hpp_
#define slic3r_GUI_CalibrationAbstractDialog_hpp_

#include <wx/wx.h>
#include <map>
#include <vector>

#include "slic3r/GUI/Jobs/ProgressIndicator.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include <wx/html/htmlwin.h>

namespace Slic3r { 
namespace GUI {

//class CalibrationAbstractDialog : public DPIDialog
//{
//
//public:
//    CalibrationAbstractDialog(GUI_App* app, MainFrame* mainframe, std::string name);
//    virtual ~CalibrationAbstractDialog(){ if(gui_app!=nullptr) gui_app->change_calibration_dialog(this, nullptr);}
//    
//private:
//    wxPanel* create_header(wxWindow* parent, const wxFont& bold_font);
//protected:
//    void create(boost::filesystem::path html_path, std::string html_name, wxSize dialogsize = wxSize(850, 550));
//    virtual void create_buttons(wxStdDialogButtonSizer*) = 0;
//    void on_dpi_changed(const wxRect& suggested_rect) override;
//    void close_me(wxCommandEvent& event_args);
//    ModelObject* add_part(ModelObject* model_object, std::string input_file, Vec3d move, Vec3d scale = Vec3d{ 1,1,1 });
//
//    wxHtmlWindow* html_viewer;
//    MainFrame* main_frame;
//    GUI_App* gui_app;
//
//};

} // namespace GUI
} // namespace Slic3r

#endif

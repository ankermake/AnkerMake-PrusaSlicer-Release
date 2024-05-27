#include "CalibrationVfaDialog.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/CustomGCode.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/MsgDialog.hpp"
#include "slic3r/GUI/Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include "slic3r/GUI/wxExtensions.hpp"

#if ENABLE_SCROLLABLE
static wxSize get_screen_size(wxWindow* window)
{
    const auto idx = wxDisplay::GetFromWindow(window);
    wxDisplay display(idx != wxNOT_FOUND ? idx : 0u);
    return display.GetClientArea().GetSize();
}
#endif // ENABLE_SCROLLABLE

namespace Slic3r {
namespace GUI {

    CalibrationVfaDialog::CalibrationVfaDialog(wxWindow* parent, wxWindowID id, Plater* plater)
        : wxDialog(parent, id, _L("common_calib_VFA_test"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE/* | wxRESIZE_BORDER*/)
        , m_plater(plater)
    {
        CenterOnParent();
        wxColour BackgroundColor(61, 62, 66);
        this->SetBackgroundColour(BackgroundColor);
        wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(v_sizer);

        // Settings
        //
        wxString start_str = _L("common_calib_start_speed");
        wxString end_vol_str = _L("common_calib_end_speed");
        wxString vol_step_str = _L("common_calib_step");
        auto text_size = wxWindow::GetTextExtent(start_str);
        text_size.IncTo(wxWindow::GetTextExtent(end_vol_str));
        text_size.IncTo(wxWindow::GetTextExtent(vol_step_str));
        //text_size.x = text_size.x * 1.5;
        wxStaticBoxSizer* settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _L("common_calib_settings"));
        settings_sizer->GetStaticBox()->SetForegroundColour(wxColour("#FFFFFF"));

        auto st_size = FromDIP(wxSize(text_size.x, -1));
        auto ti_size = FromDIP(wxSize(90, -1));
        // start vol
        auto start_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
        auto start_vol_text = new wxStaticText(this, wxID_ANY, start_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
        m_tiStart = new TextInput(this, std::to_string(40), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
        start_vol_text->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiStart->SetBackgroundColor(BackgroundColor);
        m_tiStart->SetBorderColor(wxColour(200, 200, 200));
        m_tiStart->SetLabelColor(wxColour(200, 200, 200));
        m_tiStart->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
        m_tiStart->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
        start_vol_sizer->Add(start_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        start_vol_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        settings_sizer->Add(start_vol_sizer);

        // end vol
        auto end_vol_sizer = new wxBoxSizer(wxHORIZONTAL);
        auto end_vol_text = new wxStaticText(this, wxID_ANY, end_vol_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
        m_tiEnd = new TextInput(this, std::to_string(200), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
        end_vol_text->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiEnd->SetBackgroundColor(BackgroundColor);
        m_tiEnd->SetBorderColor(wxColour(200, 200, 200));
        m_tiEnd->SetLabelColor(wxColour(200, 200, 200));
        m_tiEnd->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
        m_tiEnd->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiEnd->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
        end_vol_sizer->Add(end_vol_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        end_vol_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        settings_sizer->Add(end_vol_sizer);

        // vol step
        auto vol_step_sizer = new wxBoxSizer(wxHORIZONTAL);
        auto vol_step_text = new wxStaticText(this, wxID_ANY, vol_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
        m_tiStep = new TextInput(this, wxString::FromDouble(10), _L("mm/s"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
        vol_step_text->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiStep->SetBackgroundColor(BackgroundColor);
        m_tiStep->SetBorderColor(wxColour(200, 200, 200));
        m_tiStep->SetLabelColor(wxColour(200, 200, 200));
        m_tiStep->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
        m_tiStep->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
        m_tiStep->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
        vol_step_sizer->Add(vol_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        vol_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
        settings_sizer->Add(vol_step_sizer);

        v_sizer->Add(settings_sizer);
        v_sizer->Add(0, FromDIP(10), 0, wxEXPAND, 5);
        m_btnStart = new Button(this, _L("OK"));
        StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(98, 211, 97), StateColor::Pressed),
            std::pair<wxColour, int>(wxColour(98, 211, 97), StateColor::Hovered),
            std::pair<wxColour, int>(wxColour(98, 211, 97), StateColor::Normal));

        m_btnStart->SetBackgroundColor(btn_bg_green);
        m_btnStart->SetBorderColor(wxColour(0, 150, 136));
        m_btnStart->SetTextColor(wxColour("#FFFFFF"));
        m_btnStart->SetSize(wxSize(FromDIP(48), FromDIP(24)));
        m_btnStart->SetMinSize(wxSize(FromDIP(48), FromDIP(24)));
        m_btnStart->SetCornerRadius(FromDIP(3));
        m_btnStart->Bind(wxEVT_BUTTON, &CalibrationVfaDialog::on_start, this);
        v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_RIGHT, FromDIP(5));

        m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CalibrationVfaDialog::on_start), NULL, this);

        // wxGetApp().UpdateDlgDarkUI(this);

        Layout();
        Fit();
    }

    CalibrationVfaDialog::~CalibrationVfaDialog()
    {
        // Disconnect Events
        m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CalibrationVfaDialog::on_start), NULL, this);
    }

    void CalibrationVfaDialog::on_start(wxCommandEvent& event)
    {
        bool read_double = false;
        read_double = m_tiStart->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
        read_double = read_double && m_tiEnd->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
        read_double = read_double && m_tiStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);

        if (!read_double || m_params.start <= 10 || m_params.step <= 0 || m_params.end < (m_params.start + m_params.step)) {
            MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nstart > 10 \nstep >= 0\nend > start + step)"), wxEmptyString, wxICON_WARNING | wxOK);
            msg_dlg.ShowModal();
            return;
        }

        m_params.mode = CalibMode::Calib_VFA_Tower;
        m_plater->calib_VFA(m_params);
        EndModal(wxID_OK);
    }

    //void CalibrationVfaDialog::on_dpi_changed(const wxRect& suggested_rect)
    //{
    //    this->Refresh();
    //    Fit();
    //}

} // namespace GUI
} // namespace Slic3r

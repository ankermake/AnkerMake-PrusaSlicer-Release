#include "CalibrationTempDialog.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/CustomGCode.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/MsgDialog.hpp"

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
enum FILAMENT_TYPE : int
{
    tPLA = 0,
    tABS_ASA,
    tPETG,
    tTPU,
    tPA_CF,
    tPET_CF,
    tCustom
};

CalibrationTempDialog::CalibrationTempDialog(wxWindow* parent, wxWindowID id, Plater* plater)
    : wxDialog(parent, id, _L("common_calib_temperature_calibration"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE /*| wxRESIZE_BORDER*/), m_plater(plater)
{
    CenterOnParent();
    wxColour BackgroundColor(61, 62, 66);
    this->SetBackgroundColour(BackgroundColor);
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);
    wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxString m_rbFilamentTypeChoices[] = { _L("PLA"), _L("ABS/ASA"), _L("PETG"), _L("TPU"), _L("PA-CF"), _L("PET-CF"), _L("common_calib_custom") };
    int m_rbFilamentTypeNChoices = sizeof(m_rbFilamentTypeChoices) / sizeof(wxString);
    m_rbFilamentType = new wxRadioBox(this, wxID_ANY, _L("common_calib_filament_type"), wxDefaultPosition, wxDefaultSize, m_rbFilamentTypeNChoices, m_rbFilamentTypeChoices, 2, wxRA_SPECIFY_COLS);
    m_rbFilamentType->SetForegroundColour(wxColour("#FFFFFF"));
    m_rbFilamentType->SetSelection(0);
    m_rbFilamentType->Select(0);
    choice_sizer->Add(m_rbFilamentType, 0, wxALL, 5);
    choice_sizer->Add(FromDIP(5), 0, 0, wxEXPAND, 5);
    v_sizer->Add(choice_sizer);

    // Settings
    //
    wxString start_temp_str = _L("common_calib_start_temp");
    wxString end_temp_str = _L("common_calib_end_temp");
    wxString temp_step_str = _L("common_calib_temp_step");
    auto text_size = wxWindow::GetTextExtent(start_temp_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_temp_str));
    text_size.IncTo(wxWindow::GetTextExtent(temp_step_str));
    text_size.x = text_size.x * 1.5;
    wxStaticBoxSizer* settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _L("common_calib_settings"));
    settings_sizer->GetStaticBox()->SetForegroundColour(wxColour("#FFFFFF"));
    auto st_size = FromDIP(wxSize(text_size.x, -1));
    auto ti_size = FromDIP(wxSize(90, -1));
    // start temp
    auto start_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_temp_text = new wxStaticText(this, wxID_ANY, start_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiStart = new TextInput(this, std::to_string(230), _L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStart->SetBackgroundColor(BackgroundColor);
    m_tiStart->SetBorderColor(wxColour(200, 200, 200));
    m_tiStart->SetLabelColor(wxColour(200, 200, 200));
    m_tiStart->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    m_tiStart->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStart->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    start_temp_text->SetForegroundColour(wxColour("#FFFFFF"));
    start_temp_sizer->Add(start_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    start_temp_sizer->Add(m_tiStart, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(start_temp_sizer);

    // end temp
    auto end_temp_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_temp_text = new wxStaticText(this, wxID_ANY, end_temp_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiEnd = new TextInput(this, std::to_string(190), _L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiEnd->SetBackgroundColor(BackgroundColor);
    m_tiEnd->SetBorderColor(wxColour(200, 200, 200));
    m_tiEnd->SetLabelColor(wxColour(200, 200, 200));
    m_tiEnd->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    m_tiEnd->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiEnd->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_temp_text->SetForegroundColour(wxColour("#FFFFFF"));
    end_temp_sizer->Add(end_temp_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    end_temp_sizer->Add(m_tiEnd, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(end_temp_sizer);

    // temp step
    auto temp_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto temp_step_text = new wxStaticText(this, wxID_ANY, temp_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiStep = new TextInput(this, wxString::FromDouble(5), _L("\u2103"), "", wxDefaultPosition, ti_size, wxTE_CENTRE);
    m_tiStep->SetBackgroundColor(BackgroundColor);
    m_tiStep->SetBorderColor(wxColour(200, 200, 200));
    m_tiStep->SetLabelColor(wxColour(200, 200, 200));
    m_tiStep->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    temp_step_text->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStep->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStep->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_tiStep->Enable(false);
    temp_step_sizer->Add(temp_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    temp_step_sizer->Add(m_tiStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(temp_step_sizer);

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
    m_btnStart->Bind(wxEVT_BUTTON, &CalibrationTempDialog::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_RIGHT, FromDIP(5));

    m_rbFilamentType->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationTempDialog::on_filament_type_changed), NULL, this);
    m_btnStart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CalibrationTempDialog::on_start), NULL, this);

    //wxGetApp().UpdateDlgDarkUI(this);
    
    Layout();
    Fit();

    auto validate_text = [this](TextInput* ti) {
        unsigned long t = 0;
        if (!ti->GetTextCtrl()->GetValue().ToULong(&t))
            return;
        bool warningMsgFlag = false;
        if (t > 300 || t < 170) {
            warningMsgFlag = true;
            if (t > 300)
                t = 300;
            else
                t = 170;
        }
        t = (t / 5) * 5;
        ti->GetTextCtrl()->SetValue(std::to_string(t));

        // msg dialog show modal will take the focus, which will send the kill focus event to the textEdit twice on mac, so ShowModal() after set value to the textEdit -- xavier
        if (warningMsgFlag)
        {
            MessageDialog msg_dlg(nullptr, wxString::Format(L"Supported range: 170%s - 300%s", _L("\u2103"), _L("\u2103")), wxEmptyString, wxICON_WARNING | wxOK);
            msg_dlg.ShowModal();
        }
    };

    m_tiStart->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [&](wxFocusEvent& e) {
        validate_text(this->m_tiStart);
        e.Skip();
        });

    m_tiEnd->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [&](wxFocusEvent& e) {
        validate_text(this->m_tiEnd);
        e.Skip();
        });


}

CalibrationTempDialog::~CalibrationTempDialog() {
    // Disconnect Events
    m_rbFilamentType->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationTempDialog::on_filament_type_changed), NULL, this);
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CalibrationTempDialog::on_start), NULL, this);
}

void CalibrationTempDialog::on_start(wxCommandEvent& event) {
    bool read_long = false;
    unsigned long start = 0, end = 0;
    read_long = m_tiStart->GetTextCtrl()->GetValue().ToULong(&start);
    read_long = read_long && m_tiEnd->GetTextCtrl()->GetValue().ToULong(&end);

    if (!read_long || start > 300 || end < 170 || end >(start - 5)) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nStart temp: <= 300\nEnd temp: >= 170\nStart temp > End temp + 5)"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }
    m_params.start = start;
    m_params.end = end;
    m_params.mode = CalibMode::Calib_Temp_Tower;
    m_plater->calib_temp(m_params);
    EndModal(wxID_OK);

}

void CalibrationTempDialog::on_filament_type_changed(wxCommandEvent& event) {
    int selection = event.GetSelection();
    unsigned long start, end;
    switch (selection)
    {
    case tABS_ASA:
        start = 270;
        end = 230;
        break;
    case tPETG:
        start = 250;
        end = 230;
        break;
    case tTPU:
        start = 240;
        end = 210;
        break;
    case tPA_CF:
        start = 300;
        end = 280;
        break;
    case tPET_CF:
        start = 300;
        end = 280;
        break;
    case tPLA:
    case tCustom:
        start = 230;
        end = 190;
        break;
    }

    m_tiEnd->GetTextCtrl()->SetValue(std::to_string(end));
    m_tiStart->GetTextCtrl()->SetValue(std::to_string(start));
    event.Skip();
}

//void CalibrationTempDialog::on_dpi_changed(const wxRect& suggested_rect) {
//    this->Refresh();
//    Fit();
//}


} // namespace GUI
} // namespace Slic3r

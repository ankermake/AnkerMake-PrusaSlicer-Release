#include "CalibrationPresAdvDialog.hpp"
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
#include "slic3r/GUI/MsgDialog.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include "slic3r/GUI/wxExtensions.hpp"

#define  ankerTextColor wxColour("#ADAEAF")

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

wxBoxSizer* create_item_checkbox(wxString title, wxWindow* parent, bool* value, AnkerCheckBox*& checkbox)
{
    wxBoxSizer* m_sizer_checkbox = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 5);

    wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
    uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap uncheckScaledBitmap(uncheckImage);

    wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check.png")), wxBITMAP_TYPE_PNG);
    checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap checkScaledBitmap(checkImage);

    wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
    disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap disUncheckScaledBitmap(disuncheckImage);

    wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
    discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap disCheckScaledBitmap(discheckImage);
    checkbox = new AnkerCheckBox(parent,
        uncheckScaledBitmap.ConvertToImage(),
        checkScaledBitmap.ConvertToImage(),
        disUncheckScaledBitmap.ConvertToImage(),
        disCheckScaledBitmap.ConvertToImage(),
        title,
        ANKER_FONT_NO_1,
        ankerTextColor,
        wxID_ANY, wxDefaultPosition, wxSize(16, 16));

    m_sizer_checkbox->Add(checkbox, 0, wxALIGN_CENTER, 0);
    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 8);

    auto checkbox_title = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxSize(-1, -1), 0);
    checkbox_title->SetForegroundColour(wxColour(190, 190, 190));
    wxFont title_font = GUI::wxGetApp().bold_font();
    checkbox_title->SetFont(title_font);
    checkbox_title->Wrap(-1);
    m_sizer_checkbox->Add(checkbox_title, 0, wxALIGN_CENTER | wxALL, 3);

    checkbox->SetValue(true);

    checkbox->Bind(wxEVT_BUTTON, [parent, checkbox, value](wxCommandEvent& e) {
        (*value) = (*value) ? false : true;
        e.Skip();
        });

    return m_sizer_checkbox;
}

CalibrationPresAdvDialog::CalibrationPresAdvDialog(wxWindow* parent, wxWindowID id, Plater* plater)
    : wxDialog(parent, id, _L("common_calib_PA_calibration"), wxDefaultPosition, parent->FromDIP(wxSize(-1, 280)), wxDEFAULT_DIALOG_STYLE/* | wxRESIZE_BORDER*/), m_plater(plater)
{
    CenterOnParent();
    wxColour BackgroundColor(61, 62, 66);
    this->SetBackgroundColour(BackgroundColor);
    wxBoxSizer* v_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(v_sizer);
    wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxString m_rbExtruderTypeChoices[] = { _L("common_calib_DDE"), _L("common_calib_bowden") };
    int m_rbExtruderTypeNChoices = sizeof(m_rbExtruderTypeChoices) / sizeof(wxString);
    m_rbExtruderType = new wxRadioBox(this, wxID_ANY, _L("common_calib_extruder_type"), wxDefaultPosition, wxDefaultSize, m_rbExtruderTypeNChoices, m_rbExtruderTypeChoices, 2, wxRA_SPECIFY_COLS);
    m_rbExtruderType->SetForegroundColour(wxColour("#FFFFFF"));
    m_rbExtruderType->SetSelection(0);
    choice_sizer->Add(m_rbExtruderType, 0, wxALL, 5);
    choice_sizer->Add(FromDIP(5), 0, 0, wxEXPAND, 5);
    wxString m_rbMethodChoices[] = { _L("common_calib_PA_tower"), _L("common_calib_PA_line"), _L("common_calib_PA_pattern") };
    int m_rbMethodNChoices = sizeof(m_rbMethodChoices) / sizeof(wxString);
    m_rbMethod = new wxRadioBox(this, wxID_ANY, _L("common_calib_method"), wxDefaultPosition, wxDefaultSize, m_rbMethodNChoices, m_rbMethodChoices, 2, wxRA_SPECIFY_COLS);
    m_rbMethod->SetForegroundColour(wxColour("#FFFFFF"));
    m_rbMethod->SetSelection(0);
    choice_sizer->Add(m_rbMethod, 0, wxALL, 5);

    v_sizer->Add(choice_sizer);

    // Settings
    //
    wxString start_pa_str = _L("common_calib_start_PA");
    wxString end_pa_str = _L("common_calib_end_PA");
    wxString PA_step_str = _L("common_calib_PA_step");
    auto text_size = wxWindow::GetTextExtent(start_pa_str);
    text_size.IncTo(wxWindow::GetTextExtent(end_pa_str));
    text_size.IncTo(wxWindow::GetTextExtent(PA_step_str));
    text_size.x = text_size.x * 1.5;
    wxStaticBoxSizer* settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _L("common_calib_settings"));
    settings_sizer->GetStaticBox()->SetForegroundColour(wxColour("#FFFFFF"));
    auto st_size = FromDIP(wxSize(text_size.x, -1));
    auto ti_size = FromDIP(wxSize(90, -1));
    // start PA
    auto start_PA_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto start_pa_text = new wxStaticText(this, wxID_ANY, start_pa_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiStartPA = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    start_pa_text->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStartPA->SetBackgroundColor(BackgroundColor);
    m_tiStartPA->SetBorderColor(wxColour(200, 200, 200));
    m_tiStartPA->SetLabelColor(wxColour(200, 200, 200));
    m_tiStartPA->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    m_tiStartPA->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

    start_PA_sizer->Add(start_pa_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    start_PA_sizer->Add(m_tiStartPA, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(start_PA_sizer);

    // end PA
    auto end_PA_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto end_pa_text = new wxStaticText(this, wxID_ANY, end_pa_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiEndPA = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    end_pa_text->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiEndPA->SetBackgroundColor(BackgroundColor);
    m_tiEndPA->SetBorderColor(wxColour(200, 200, 200));
    m_tiEndPA->SetLabelColor(wxColour(200, 200, 200));
    m_tiEndPA->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    m_tiEndPA->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    end_PA_sizer->Add(end_pa_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    end_PA_sizer->Add(m_tiEndPA, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(end_PA_sizer);

    // PA step
    auto PA_step_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto PA_step_text = new wxStaticText(this, wxID_ANY, PA_step_str, wxDefaultPosition, st_size, wxALIGN_LEFT);
    m_tiPAStep = new TextInput(this, "", "", "", wxDefaultPosition, ti_size, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    PA_step_text->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiPAStep->SetBackgroundColor(BackgroundColor);
    m_tiPAStep->SetBorderColor(wxColour(200, 200, 200));
    m_tiPAStep->SetLabelColor(wxColour(200, 200, 200));
    m_tiPAStep->GetTextCtrl()->SetBackgroundColour(BackgroundColor);
    m_tiPAStep->GetTextCtrl()->SetForegroundColour(wxColour("#FFFFFF"));
    m_tiStartPA->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    PA_step_sizer->Add(PA_step_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    PA_step_sizer->Add(m_tiPAStep, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    settings_sizer->Add(PA_step_sizer);

    settings_sizer->Add(create_item_checkbox(_L("common_calib_print_numbers"), this, &m_params.print_numbers, m_cbPrintNum));
    m_cbPrintNum->SetValue(false);

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
    m_btnStart->Bind(wxEVT_BUTTON, &CalibrationPresAdvDialog::on_start, this);
    v_sizer->Add(m_btnStart, 0, wxALL | wxALIGN_RIGHT, FromDIP(5));

    CalibrationPresAdvDialog::reset_params();

    // Connect Events
    m_rbExtruderType->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationPresAdvDialog::on_extruder_type_changed), NULL, this);
    m_rbMethod->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationPresAdvDialog::on_method_changed), NULL, this);
    this->Connect(wxEVT_SHOW, wxShowEventHandler(CalibrationPresAdvDialog::on_show));
    //wxGetApp().UpdateDlgDarkUI(this);

    Layout();
    Fit();
}

CalibrationPresAdvDialog::~CalibrationPresAdvDialog() {
    // Disconnect Events
    m_rbExtruderType->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationPresAdvDialog::on_extruder_type_changed), NULL, this);
    m_rbMethod->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CalibrationPresAdvDialog::on_method_changed), NULL, this);
    m_btnStart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CalibrationPresAdvDialog::on_start), NULL, this);
}

void CalibrationPresAdvDialog::reset_params() {
    bool isDDE = m_rbExtruderType->GetSelection() == 0 ? true : false;
    int method = m_rbMethod->GetSelection();

    m_tiStartPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.0));

    switch (method) {
    case 1:
        m_params.mode = CalibMode::Calib_PA_Line;
        m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.1));
        m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.002));
        m_cbPrintNum->SetValue(true);
        m_cbPrintNum->Enable(true);
        break;
    case 2:
        m_params.mode = CalibMode::Calib_PA_Pattern;
        m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.08));
        m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.005));
        m_cbPrintNum->SetValue(true);
        m_cbPrintNum->Enable(false);
        break;
    default:
        m_params.mode = CalibMode::Calib_PA_Tower;
        m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(0.1));
        m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.002));
        m_cbPrintNum->SetValue(false);
        m_cbPrintNum->Enable(false);
        break;
    }

    if (!isDDE) {
        m_tiEndPA->GetTextCtrl()->SetValue(wxString::FromDouble(1.0));

        if (m_params.mode == CalibMode::Calib_PA_Pattern) {
            m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.05));
        }
        else {
            m_tiPAStep->GetTextCtrl()->SetValue(wxString::FromDouble(0.02));
        }
    }
}

void CalibrationPresAdvDialog::on_start(wxCommandEvent& event) {
    bool read_double = false;
    read_double = m_tiStartPA->GetTextCtrl()->GetValue().ToDouble(&m_params.start);
    read_double = read_double && m_tiEndPA->GetTextCtrl()->GetValue().ToDouble(&m_params.end);
    read_double = read_double && m_tiPAStep->GetTextCtrl()->GetValue().ToDouble(&m_params.step);
    if (!read_double || m_params.start < 0 || m_params.step < EPSILON || m_params.end < m_params.start + m_params.step) {
        MessageDialog msg_dlg(nullptr, _L("Please input valid values:\nStart PA: >= 0.0\nEnd PA: > Start PA\nPA step: >= 0.001)"), wxEmptyString, wxICON_WARNING | wxOK);
        msg_dlg.ShowModal();
        return;
    }

    switch (m_rbMethod->GetSelection()) {
    case 1:
        m_params.mode = CalibMode::Calib_PA_Line;
        break;
    case 2:
        m_params.mode = CalibMode::Calib_PA_Pattern;
        break;
    default:
        m_params.mode = CalibMode::Calib_PA_Tower;
    }

    m_params.print_numbers = m_cbPrintNum->GetValue();

    m_plater->calib_pa(m_params);
    EndModal(wxID_OK);

}
void CalibrationPresAdvDialog::on_extruder_type_changed(wxCommandEvent& event) {
    CalibrationPresAdvDialog::reset_params();
    event.Skip();
}
void CalibrationPresAdvDialog::on_method_changed(wxCommandEvent& event) {
    CalibrationPresAdvDialog::reset_params();
    event.Skip();
}

//void CalibrationPresAdvDialog::on_dpi_changed(const wxRect& suggested_rect) {
//    this->Refresh();
//    Fit();
//}

void CalibrationPresAdvDialog::on_show(wxShowEvent& event) {
    CalibrationPresAdvDialog::reset_params();
}


} // namespace GUI
} // namespace Slic3r

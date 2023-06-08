#include "ExtruderSequenceDialog.hpp"

#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/bmpcbox.h>
#include <wx/checkbox.h>

#include <vector>
#include <set>
#include <functional>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "OptionsGroup.hpp"
#include "MainFrame.hpp"
#include "BitmapComboBox.hpp"


namespace Slic3r {
namespace GUI {

ExtruderSequenceDialog::ExtruderSequenceDialog(const DoubleSlider::ExtrudersSequence& sequence)
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _(L("Set extruder sequence")),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_sequence(sequence)
{
#ifdef _WIN32
    wxGetApp().UpdateDarkUI(this);
#else
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif
    SetDoubleBuffered(true);
    SetFont(wxGetApp().normal_font());

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    const int em = wxGetApp().em_unit();

    m_bmp_del = ScalableBitmap(this, "remove_copies");
    m_bmp_add = ScalableBitmap(this, "add_copies");

    auto option_sizer = new wxBoxSizer(wxVERTICAL);

    auto intervals_box = new wxStaticBox(this, wxID_ANY, _(L("Set extruder change for every"))+ ": ");
    wxGetApp().UpdateDarkUI(intervals_box);
    auto intervals_box_sizer = new wxStaticBoxSizer(intervals_box, wxVERTICAL);

    m_intervals_grid_sizer = new wxFlexGridSizer(3, 5, em);

    auto editor_sz = wxSize(4*em, wxDefaultCoord);

    auto ID_RADIO_BUTTON = wxID_ANY;// wxWindow::NewControlId(1);

    wxRadioButton* rb_by_layers = new wxRadioButton(this, ID_RADIO_BUTTON, "", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rb_by_layers->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent& event) { m_sequence.is_mm_intervals = false; });
    rb_by_layers->SetValue(!m_sequence.is_mm_intervals);

    wxStaticText* st_by_layers = new wxStaticText(this, wxID_ANY, _(L("layers")));
    m_interval_by_layers = new wxTextCtrl(this, wxID_ANY, 
                                          wxString::Format("%d", m_sequence.interval_by_layers), 
                                          wxDefaultPosition, editor_sz
#ifdef _WIN32
        , wxBORDER_SIMPLE
#endif
    );
    wxGetApp().UpdateDarkUI(m_interval_by_layers);
    m_interval_by_layers->Bind(wxEVT_TEXT, [this, rb_by_layers](wxEvent&)
    {
        wxString str = m_interval_by_layers->GetValue();
        if (str.IsEmpty()) {
            m_interval_by_layers->SetValue(wxString::Format("%d", m_sequence.interval_by_layers));
            return;
        }

        int val = wxAtoi(str);
        if (val < 1) {
            m_interval_by_layers->SetValue("1");
            val = 1;
        }
        
        if (m_sequence.interval_by_layers == val)
            return;

        m_sequence.interval_by_layers = val;

        m_sequence.is_mm_intervals = false;
        rb_by_layers->SetValue(true);
    });

    m_intervals_grid_sizer->Add(rb_by_layers, 0, wxALIGN_CENTER_VERTICAL);
    m_intervals_grid_sizer->Add(m_interval_by_layers,0, wxALIGN_CENTER_VERTICAL);
    m_intervals_grid_sizer->Add(st_by_layers,0, wxALIGN_CENTER_VERTICAL);

    wxRadioButton* rb_by_mm = new wxRadioButton(this, ID_RADIO_BUTTON, "");
    rb_by_mm->Bind(wxEVT_RADIOBUTTON, [this](wxEvent&) { m_sequence.is_mm_intervals = true; });
    rb_by_mm->SetValue(m_sequence.is_mm_intervals);

    wxStaticText* st_by_mm = new wxStaticText(this, wxID_ANY, _(L("mm")));
    m_interval_by_mm = new wxTextCtrl(this, wxID_ANY, 
                                      double_to_string(sequence.interval_by_mm), 
                                      wxDefaultPosition, editor_sz, wxTE_PROCESS_ENTER
#ifdef _WIN32
        | wxBORDER_SIMPLE
#endif
    );
    wxGetApp().UpdateDarkUI(m_interval_by_mm);

    double min_layer_height = wxGetApp().preset_bundle->prints.get_edited_preset().config.opt_float("layer_height");
    auto change_value = [this, min_layer_height]()
    {
        wxString str = m_interval_by_mm->GetValue();
        if (str.IsEmpty()) {
            m_interval_by_mm->SetValue(wxString::Format("%d", m_sequence.interval_by_mm));
            return;
        }

        char dec_sep = '.';
        if (! is_decimal_separator_point()) {
            str.Replace(".", ",", false);
            dec_sep = ',';
        }

        double val;
        if (str == dec_sep || !str.ToDouble(&val) || val <= 0.0)
            val = 3.0; // default value

        if (fabs(m_sequence.interval_by_layers - val) < 0.001)
            return;

        if (val < min_layer_height) {
            val = min_layer_height;
            m_interval_by_mm->SetValue(double_to_string(val, 2));
        }

        m_sequence.interval_by_mm = val;
    };

    m_interval_by_mm->Bind(wxEVT_TEXT, [this, rb_by_mm](wxEvent&)
    {
        m_sequence.is_mm_intervals = true;
        rb_by_mm->SetValue(true);
    });

    m_interval_by_mm->Bind(wxEVT_KILL_FOCUS, [change_value](wxFocusEvent& event)
    {
        change_value();
        event.Skip();
    });

    m_interval_by_mm->Bind(wxEVT_TEXT_ENTER, [change_value](wxEvent&)
    {
        change_value();
    });

    m_intervals_grid_sizer->Add(rb_by_mm, 0, wxALIGN_CENTER_VERTICAL);
    m_intervals_grid_sizer->Add(m_interval_by_mm, 0, wxALIGN_CENTER_VERTICAL);
    m_intervals_grid_sizer->Add(st_by_mm,0, wxALIGN_CENTER_VERTICAL);

    intervals_box_sizer->Add(m_intervals_grid_sizer, 0, wxLEFT, em);
    option_sizer->Add(intervals_box_sizer, 0, wxEXPAND);

    m_random_sequence = new wxCheckBox(this, wxID_ANY, _L("Random sequence"));
    m_random_sequence->SetValue(m_sequence.random_sequence);
    m_random_sequence->SetToolTip(_L("If enabled, random sequence of the selected extruders will be used."));
    m_random_sequence->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        m_sequence.random_sequence = e.IsChecked();
        m_color_repetition->Enable(m_sequence.random_sequence);
    });

    m_color_repetition = new wxCheckBox(this, wxID_ANY, _L("Allow next color repetition"));
    m_color_repetition->SetValue(m_sequence.color_repetition);
    m_color_repetition->SetToolTip(_L("If enabled, a repetition of the next random color will be allowed."));
    m_color_repetition->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {m_sequence.color_repetition = e.IsChecked();  });
    
    auto extruders_box = new wxStaticBox(this, wxID_ANY, _(L("Set extruder(tool) sequence"))+ ": ");
    wxGetApp().UpdateDarkUI(extruders_box);

    auto extruders_box_sizer = new wxStaticBoxSizer(extruders_box, wxVERTICAL);

    m_extruders_grid_sizer = new wxFlexGridSizer(3, 5, em);

    apply_extruder_sequence();

    extruders_box_sizer->Add(m_extruders_grid_sizer, 0, wxALL, em);
    extruders_box_sizer->Add(m_random_sequence, 0, wxLEFT | wxBOTTOM, em);
    extruders_box_sizer->Add(m_color_repetition, 0, wxLEFT | wxBOTTOM, em);
    option_sizer->Add(extruders_box_sizer, 0, wxEXPAND | wxTOP, em);

    main_sizer->Add(option_sizer, 0, wxEXPAND | wxALL, em);

    wxStdDialogButtonSizer* buttons = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    wxGetApp().UpdateDarkUI(static_cast<wxButton*>(this->FindWindowById(wxID_OK, this)));
    wxGetApp().UpdateDarkUI(static_cast<wxButton*>(this->FindWindowById(wxID_CANCEL, this)));
    main_sizer->Add(buttons, 0, wxEXPAND | wxRIGHT | wxBOTTOM, em);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    /* For this moment min sizes for dialog and its sizer are calculated.
     * If we left them, it can cause a problem with layouts during deleting of extruders
     */
    if (m_sequence.extruders.size()>1)
    {
        wxSize sz = wxSize(-1, 10 * em);
        SetMinSize(sz);
        GetSizer()->SetMinSize(sz);
    }
}

void ExtruderSequenceDialog::apply_extruder_sequence()
{
    m_extruders_grid_sizer->Clear(true);

    for (size_t extruder=0; extruder < m_sequence.extruders.size(); ++extruder)
    {
        BitmapComboBox* extruder_selector = nullptr;
        apply_extruder_selector(&extruder_selector, this, "", wxDefaultPosition, wxSize(15*wxGetApp().em_unit(), -1));
        extruder_selector->SetSelection(m_sequence.extruders[extruder]);

        extruder_selector->Bind(wxEVT_COMBOBOX, [this, extruder_selector, extruder](wxCommandEvent& evt)
        {
            m_sequence.extruders[extruder] = extruder_selector->GetSelection();
            evt.StopPropagation();
        });

        auto del_btn = new ScalableButton(this, wxID_ANY, m_bmp_del);
        del_btn->SetToolTip(_(L("Remove extruder from sequence")));
        if (m_sequence.extruders.size()==1)
            del_btn->Disable();

        del_btn->Bind(wxEVT_BUTTON, [this, extruder](wxEvent&) {
            m_sequence.delete_extruder(extruder);
            apply_extruder_sequence();
        });

        auto add_btn = new ScalableButton(this, wxID_ANY, m_bmp_add);
        add_btn->SetToolTip(_(L("Add extruder to sequence")));

        add_btn->Bind(wxEVT_BUTTON, [this, extruder, extruder_selector](wxEvent&) {
            size_t extr_cnt = (size_t)extruder_selector->GetCount();
            size_t seq_extr_cnt = m_sequence.extruders.size();
            size_t extr_id = seq_extr_cnt - size_t(seq_extr_cnt / extr_cnt) * extr_cnt;
            m_sequence.add_extruder(extruder, std::min(extr_id, extr_cnt-1));
            apply_extruder_sequence();
        });

        m_extruders_grid_sizer->Add(extruder_selector, 0, wxALIGN_CENTER_VERTICAL);
        m_extruders_grid_sizer->Add(del_btn, 0, wxALIGN_CENTER_VERTICAL);
        m_extruders_grid_sizer->Add(add_btn, 0, wxALIGN_CENTER_VERTICAL);
    }
    m_extruders_grid_sizer->ShowItems(true); // show items hidden in apply_extruder_selector()

    bool show_checkboxes = m_sequence.extruders.size() > 1;
    m_random_sequence->Enable(show_checkboxes);
    m_color_repetition->Enable(show_checkboxes && m_sequence.random_sequence);

    Fit();
    Refresh();
}

void ExtruderSequenceDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    SetFont(wxGetApp().normal_font());

    const int em = em_unit();

    m_intervals_grid_sizer->SetHGap(em);
    m_intervals_grid_sizer->SetVGap(em);
    m_extruders_grid_sizer->SetHGap(em);
    m_extruders_grid_sizer->SetVGap(em);

    msw_buttons_rescale(this, em, { wxID_OK, wxID_CANCEL });

    // wxSize size = get_size();
    // SetMinSize(size);

    Fit();
    Refresh();
}

}
}



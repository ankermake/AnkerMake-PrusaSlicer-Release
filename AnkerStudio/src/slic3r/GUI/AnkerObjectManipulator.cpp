#include "AnkerObjectManipulator.hpp"

#include "GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerSplitCtrl.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "libslic3r/AppConfig.hpp"

#include <wx/valnum.h>

#define BTN_PRESSED "pressed"
#define BTN_NORMAL "normal"

using namespace Slic3r;
using namespace Slic3r::GUI;


wxDEFINE_EVENT(wxANKEREVT_AOM_RETURN, wxCommandEvent);

const double AnkerObjectManipulator::in_to_mm = 25.4;
const double AnkerObjectManipulator::mm_to_in = 1 / AnkerObjectManipulator::in_to_mm;

AnkerObjectManipulator::AnkerObjectManipulator(wxWindow* parent)
	: wxPanel(parent)
    , m_dirty(false)
    , m_textUpdating(false)
    , m_uniform_scale(true)
    , m_imperial_units(false)
    , m_coordinates_type(ECoordinatesType::World)
    , m_pReturnFunc(nullptr)
{
	initUI();
}

AnkerObjectManipulator::~AnkerObjectManipulator()
{
}

void AnkerObjectManipulator::set_uniform_scaling(const bool use_uniform_scale)
{
    if (!use_uniform_scale)
    {
        this->set_dirty();
        this->update_if_dirty();
    }

    m_uniform_scale = use_uniform_scale;

    set_dirty();
}

void AnkerObjectManipulator::update_if_dirty(bool force)
{
    if (!m_dirty)
        return;

    m_textUpdating = true;

    const Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
    this->update_settings_value(selection);

    //auto update_label = [](wxString& label_cache, const std::string& new_label, wxStaticText* widget) {
    //    wxString new_label_localized = _(new_label) + ":";
    //    if (label_cache != new_label_localized) {
    //        label_cache = new_label_localized;
    //        widget->SetLabel(new_label_localized);
    //        if (wxOSX) set_font_and_background_style(widget, wxGetApp().normal_font());
    //    }
    //};
    //update_label(m_cache.move_label_string, m_new_move_label_string, m_move_Label);
    //update_label(m_cache.rotate_label_string, m_new_rotate_label_string, m_rotate_Label);
    //update_label(m_cache.scale_label_string, m_new_scale_label_string, m_scale_Label);

    for (int i = EDITOR_X; i < EDITOR_AXIS_COUNT; ++i) {
        auto update = [this, i, force](Vec3d& cached, Vec3d& cached_rounded, EditorType key_id, const Vec3d& new_value) {
            cached(i) = new_value(i);
            //if (!force && m_editor[key_id][i]->HasFocus())
            //    return;

            wxString new_text = /*double_to_string(new_value(i), m_imperial_units && key_id == EDITOR_SCALE_SIZE ? 4 : 2)*/wxString::Format(wxT("%.2f"), new_value(i));
            double new_rounded;
            new_text.ToDouble(&new_rounded);
            if (std::abs(cached_rounded(i) - new_rounded) > EPSILON) {
                cached_rounded(i) = new_rounded;
                const int id = key_id * 3 + i;
                if (m_imperial_units) {
                    double inch_value = new_value(i) * mm_to_in;
                    if (key_id == EDITOR_TRANSLATE)
                        new_text = /*double_to_string(inch_value, 2)*/wxString::Format(wxT("%.2f"), inch_value);
                    ;
                    if (key_id == EDITOR_SCALE_SIZE) {
                        if (std::abs(m_cache.size_inches(i) - inch_value) > EPSILON)
                            m_cache.size_inches(i) = inch_value;
                        new_text = /*double_to_string(inch_value, 2)*/wxString::Format(wxT("%.2f"), inch_value);
                    }
                }
                m_editor[key_id][i]->SetValue(new_text);
            }
            //cached(i) = new_value(i);
        };
        update(m_cache.position, m_cache.position_rounded, EDITOR_TRANSLATE, m_new_position);
        update(m_cache.scale, m_cache.scale_rounded, EDITOR_SCALE_FACTOR, m_new_scale);
        update(m_cache.size, m_cache.size_rounded, EDITOR_SCALE_SIZE, m_new_size);
        update(m_cache.rotation, m_cache.rotation_rounded, EDITOR_ROTATE, m_new_rotation);
    }

    //m_lock_bnt->SetLock(m_uniform_scale);
    //m_lock_bnt->SetToolTip(wxEmptyString);
    //m_lock_bnt->enable();

    //if (m_new_enabled)
    //    m_og->enable();
    //else
    //    m_og->disable();

    //if (!wxGetApp().plater()->canvas3D()->is_dragging()) {
    //    update_reset_buttons_visibility();
    //    update_mirror_buttons_visibility();
    //}

    m_textUpdating = false;

    m_dirty = false;
}

void AnkerObjectManipulator::update_ui_from_settings()
{
    if (m_imperial_units != wxGetApp().app_config->get_bool("use_inches")) {
        m_imperial_units = wxGetApp().app_config->get_bool("use_inches");

        //auto update_unit_text = [](const wxString& new_unit_text, wxStaticText* widget) {
        //    widget->SetLabel(new_unit_text);
        //    if (wxOSX) set_font_and_background_style(widget, wxGetApp().normal_font());
        //};
        //update_unit_text(m_imperial_units ? _L("in") : _L("mm"), m_position_unit);
        //update_unit_text(m_imperial_units ? _L("in") : _L("mm"), m_size_unit);

        for (int i = 0; i < 3; ++i) {
            auto update = [this, i](EditorType key_id, const Vec3d& new_value) {
                double value = new_value(i);
                if (m_imperial_units)
                    value *= mm_to_in;
                //wxString new_text = double_to_string(value, m_imperial_units && key_id == 3/*meSize*/ ? 4 : 2);
                wxString new_text = double_to_string(value, 2);
                m_editor[key_id][i]->SetLabelText(new_text);
            };
            update(EDITOR_TRANSLATE, m_new_position);
            update(EDITOR_SCALE_SIZE, m_new_size);
        }
    }
}

void AnkerObjectManipulator::initUI()
{
    SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));

    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(contentSizer);

    AnkerTitledPanel* container = new AnkerTitledPanel(this, 46, 12);
    container->setTitle(/*L"Object Modify"*/_("common_slice_toolpannel_modify"));
    container->setTitleAlign(AnkerTitledPanel::TitleAlign::LEFT);
    int returnBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("return.png")), true);
    int resetBtnID = container->addTitleButton(wxString::FromUTF8(Slic3r::var("reset.png")), false);
    contentSizer->Add(container, 1, wxEXPAND, 0);

    wxPanel* objectModifyPanel = new wxPanel(container);
    wxBoxSizer* objectModifySizer = new wxBoxSizer(wxVERTICAL);
    objectModifyPanel->SetSizer(objectModifySizer);
    container->setContentPanel(objectModifyPanel);

    objectModifySizer->AddSpacer(20);

    auto applyFunc = [this](wxTextCtrl* textCtrl, EditorType editType, EditorAxisType axisType) {
        if (m_textUpdating)
            return;

        double validMin = -1, validMax = 1;
        switch (editType)
        {
        case AnkerObjectManipulator::EDITOR_TRANSLATE:
            validMin = -1000;
            validMax = 1000;
            break;
        case AnkerObjectManipulator::EDITOR_ROTATE:
            validMin = -360.0;
            validMax = 360.0;
            break;
        case AnkerObjectManipulator::EDITOR_SCALE_FACTOR:
            validMin = 0.01;
            validMax = 1000000;
            break;
        case AnkerObjectManipulator::EDITOR_SCALE_SIZE:
            validMin = 0.000001;
            validMax = 1000;
            break;
        default:
            break;
        }

        double newValue = 10.0;
        wxString newValueStr = textCtrl->GetLineText(0);
        bool success = newValueStr.ToDouble(&newValue);
        if (success)
        {
            if (newValue >= validMin && newValue <= validMax)
            {
                this->OnTextEdit(editType, axisType, newValue);
                return;
            }
        }
        
        {
            // TODO: warning msg dialog
            //std::string levelReminder = "Invalid numeric input! "/*_("common_print_popup_printstop").ToStdString()*/;
            //std::string title = "Warning"/*_("common_print_controlbutton_stop").ToStdString()*/;
            //AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, true);
            //if (result != AnkerMsgDialog::MSG_OK)
            //    return;

            double originValue = -1.0;
            if (editType == EDITOR_SCALE_SIZE)
                originValue = m_cache.size((int)axisType);
            else if (editType == EDITOR_SCALE_FACTOR)
                originValue = m_cache.scale((int)axisType);
            else if (editType == EDITOR_TRANSLATE)
                originValue = m_cache.position((int)axisType);
            else if (editType == EDITOR_ROTATE)
                originValue = m_cache.rotation((int)axisType);
            textCtrl->SetValue(wxString::Format(wxT("%.2f"), originValue));
        }
    };
    // A name used to call handle_sidebar_focus_event()
    std::string opts[4] = {"position", "rotation", "scale", "scale"};
    std::string axes[3] = {"x", "y", "z"};
    auto focusFunc = [this, opts, axes](wxTextCtrl* textCtrl, EditorType editType, EditorAxisType axisType)
    {
        if (textCtrl == nullptr)
            return;

        // needed to show the visual hints in 3D scene
        std::string opt_name = opts[editType] + "_" + axes[axisType];
        wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event(opt_name, textCtrl->HasFocus());
    };
    wxFloatingPointValidator<double> translateValidator;
    translateValidator.SetMin(-1000);
    translateValidator.SetMax(1000);
    translateValidator.SetPrecision(2);
    wxFloatingPointValidator<double> rotateValidator;
    rotateValidator.SetMin(-360);
    rotateValidator.SetMax(360);
    rotateValidator.SetPrecision(2);
    wxFloatingPointValidator<double> factorValidator;
    factorValidator.SetMin(-100000);
    factorValidator.SetMax(100000);
    factorValidator.SetPrecision(2);
    wxFloatingPointValidator<double> sizeValidator;
    sizeValidator.SetMin(0.0);
    sizeValidator.SetMax(1000);
    sizeValidator.SetPrecision(2);

    // move / rotate
    wxStaticText* moveNRotateText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Move/Rotate"*/_("common_slice_toolpannelmodify_title1"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    moveNRotateText->SetFont(ANKER_FONT_NO_1);
    moveNRotateText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    moveNRotateText->SetForegroundColour(wxColour(105, 106, 107));
    objectModifySizer->Add(moveNRotateText, 0, wxALIGN_LEFT | wxLEFT, 12);

    // param table
    wxBoxSizer* mrTableSizer = new wxBoxSizer(wxVERTICAL);
    objectModifySizer->Add(mrTableSizer, 0, wxEXPAND | wxALIGN_TOP | wxTOP, 8);

    {
        wxBoxSizer* tableSizer = mrTableSizer;
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(titleSizer, 0, wxEXPAND | wxALIGN_RIGHT | wxRIGHT, 58);

        titleSizer->AddStretchSpacer(1);

        wxStaticText* xText = new wxStaticText(objectModifyPanel, wxID_ANY, L"X", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        xText->SetFont(ANKER_FONT_NO_1);
        xText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xText->SetForegroundColour(wxColour(255, 0, 0));
#ifdef __APPLE__
        titleSizer->Add(xText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(120));
#else
        titleSizer->Add(xText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(108));
#endif 

        wxStaticText* yText = new wxStaticText(objectModifyPanel, wxID_ANY, L"Y", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        yText->SetFont(ANKER_FONT_NO_1);
        yText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        yText->SetForegroundColour(wxColour(0, 255, 0));
        titleSizer->Add(yText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(50));

        wxStaticText* zText = new wxStaticText(objectModifyPanel, wxID_ANY, L"Z", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        zText->SetFont(ANKER_FONT_NO_1);
        zText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zText->SetForegroundColour(wxColour(0, 0, 255));
#ifdef __APPLE__
        titleSizer->Add(zText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(35));
#else
        titleSizer->Add(zText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(45));
#endif 


        wxBoxSizer* locationSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(locationSizer, 0, wxEXPAND | wxALIGN_LEFT | wxTOP, 8);

        wxStaticText* locationText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Location"*/_("common_slice_toolpannelmodify_location"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        locationText->SetFont(ANKER_FONT_NO_1);
        locationText->SetMinSize(AnkerSize(100, 24));
        locationText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        locationText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        locationText->Fit();
        locationSizer->Add(locationText, 0, wxALIGN_CENTER | wxLEFT, 12);

        locationSizer->AddStretchSpacer(1);

        char text[10];
        sprintf(text, "%d", 90);
        wxTextCtrl* xLocationTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        xLocationTextCtrl->SetFont(ANKER_FONT_NO_1);
        xLocationTextCtrl->SetMinSize(AnkerSize(54, 24));
        xLocationTextCtrl->SetMaxSize(AnkerSize(54, 24));
        xLocationTextCtrl->SetSize(AnkerSize(54, 24));
        xLocationTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xLocationTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        xLocationTextCtrl->SetValidator(translateValidator);
        //xLocationTextCtrl->Bind(wxEVT_TEXT, [this, xLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_X); });
        xLocationTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, xLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_X); });
        xLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xLocationTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(xLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_X); event.Skip(); });
        xLocationTextCtrl->Bind(wxEVT_SET_FOCUS, [this, xLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_X); event.Skip(); });
        xLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_X); event.Skip(); });
        locationSizer->Add(xLocationTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_TRANSLATE][EDITOR_X] = xLocationTextCtrl;

        sprintf(text, "%d", 90);
        wxTextCtrl* yLocationTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        yLocationTextCtrl->SetFont(ANKER_FONT_NO_1);
        yLocationTextCtrl->SetMinSize(AnkerSize(54, 24));
        yLocationTextCtrl->SetMaxSize(AnkerSize(54, 24));
        yLocationTextCtrl->SetSize(AnkerSize(54, 24));
        yLocationTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        yLocationTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        yLocationTextCtrl->SetValidator(translateValidator);
        //yLocationTextCtrl->Bind(wxEVT_TEXT, [this, yLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Y); });
        yLocationTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, yLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Y); });
        yLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yLocationTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(yLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Y); event.Skip(); });
        yLocationTextCtrl->Bind(wxEVT_SET_FOCUS, [this, yLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Y); event.Skip(); });
        yLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Y); event.Skip(); });
        locationSizer->Add(yLocationTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_TRANSLATE][EDITOR_Y] = yLocationTextCtrl;

        sprintf(text, "%d", 90);
        wxTextCtrl* zLocationTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        zLocationTextCtrl->SetFont(ANKER_FONT_NO_1);
        zLocationTextCtrl->SetMinSize(AnkerSize(54, 24));
        zLocationTextCtrl->SetMaxSize(AnkerSize(54, 24));
        zLocationTextCtrl->SetSize(AnkerSize(54, 24));
        zLocationTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zLocationTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        zLocationTextCtrl->SetValidator(translateValidator);
        //zLocationTextCtrl->Bind(wxEVT_TEXT, [this, zLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Z); });
        zLocationTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, zLocationTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Z); });
        zLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zLocationTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(zLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Z); event.Skip(); });
        zLocationTextCtrl->Bind(wxEVT_SET_FOCUS, [this, zLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Z); event.Skip(); });
        zLocationTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zLocationTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zLocationTextCtrl, EDITOR_TRANSLATE, EDITOR_Z); event.Skip(); });
        locationSizer->Add(zLocationTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_TRANSLATE][EDITOR_Z] = zLocationTextCtrl;

        wxStaticText* localUnitText = new wxStaticText(objectModifyPanel, wxID_ANY, L"mm", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        localUnitText->SetFont(ANKER_FONT_NO_2);
        //localUnitText->SetSize(AnkerSize(18, 22));
        //localUnitText->SetSizeHints(AnkerSize(18, 22), AnkerSize(18, 22));
        localUnitText->Fit();
        localUnitText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        localUnitText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        locationSizer->Add(localUnitText, 1, wxALIGN_CENTER | wxRIGHT, 12);


        wxBoxSizer* rotateSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(rotateSizer, 0, wxEXPAND | wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

        wxStaticText* rotateText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Rotation"*/_("common_slice_toolpannelmodify_rotation"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        rotateText->SetFont(ANKER_FONT_NO_1);
        rotateText->SetMinSize(AnkerSize(100, 24));
        rotateText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        rotateText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        rotateSizer->Add(rotateText, 0, wxALIGN_CENTER | wxLEFT, 12);

        rotateSizer->AddStretchSpacer(1);

        sprintf(text, "%d", 0);
        wxTextCtrl* xRotateTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        xRotateTextCtrl->SetFont(ANKER_FONT_NO_1);
        xRotateTextCtrl->SetMinSize(AnkerSize(54, 24));
        xRotateTextCtrl->SetMaxSize(AnkerSize(54, 24));
        xRotateTextCtrl->SetSize(AnkerSize(54, 24));
        xRotateTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xRotateTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        xRotateTextCtrl->SetValidator(rotateValidator);
        //xRotateTextCtrl->Bind(wxEVT_TEXT, [this, xRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xRotateTextCtrl, EDITOR_ROTATE, EDITOR_X); });
        xRotateTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, xRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xRotateTextCtrl, EDITOR_ROTATE, EDITOR_X); });
        xRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xRotateTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(xRotateTextCtrl, EDITOR_ROTATE, EDITOR_X); event.Skip(); });
        xRotateTextCtrl->Bind(wxEVT_SET_FOCUS, [this, xRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xRotateTextCtrl, EDITOR_ROTATE, EDITOR_X); event.Skip(); });
        xRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xRotateTextCtrl, EDITOR_ROTATE, EDITOR_X); event.Skip(); });
        rotateSizer->Add(xRotateTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_ROTATE][EDITOR_X] = xRotateTextCtrl;

        sprintf(text, "%d", 0);
        wxTextCtrl* yRotateTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        yRotateTextCtrl->SetFont(ANKER_FONT_NO_1);
        yRotateTextCtrl->SetMinSize(AnkerSize(54, 24));
        yRotateTextCtrl->SetMaxSize(AnkerSize(54, 24));
        yRotateTextCtrl->SetSize(AnkerSize(54, 24));
        yRotateTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        yRotateTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        yRotateTextCtrl->SetValidator(rotateValidator);
        //yRotateTextCtrl->Bind(wxEVT_TEXT, [this, yRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yRotateTextCtrl, EDITOR_ROTATE, EDITOR_Y); });
        yRotateTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, yRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yRotateTextCtrl, EDITOR_ROTATE, EDITOR_Y); });
        yRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yRotateTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(yRotateTextCtrl, EDITOR_ROTATE, EDITOR_Y); event.Skip(); });
        yRotateTextCtrl->Bind(wxEVT_SET_FOCUS, [this, yRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yRotateTextCtrl, EDITOR_ROTATE, EDITOR_Y); event.Skip(); });
        yRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yRotateTextCtrl, EDITOR_ROTATE, EDITOR_Y); event.Skip(); });
        rotateSizer->Add(yRotateTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_ROTATE][EDITOR_Y] = yRotateTextCtrl;

        sprintf(text, "%d", 0);
        wxTextCtrl* zRotateTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        zRotateTextCtrl->SetFont(ANKER_FONT_NO_1);
        zRotateTextCtrl->SetMinSize(AnkerSize(54, 24));
        zRotateTextCtrl->SetMaxSize(AnkerSize(54, 24));
        zRotateTextCtrl->SetSize(AnkerSize(54, 22));
        zRotateTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zRotateTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        zRotateTextCtrl->SetValidator(rotateValidator);
        //zRotateTextCtrl->Bind(wxEVT_TEXT, [this, zRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zRotateTextCtrl, EDITOR_ROTATE, EDITOR_Z); });
        zRotateTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, zRotateTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zRotateTextCtrl, EDITOR_ROTATE, EDITOR_Z); });
        zRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zRotateTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(zRotateTextCtrl, EDITOR_ROTATE, EDITOR_Z); event.Skip(); });
        zRotateTextCtrl->Bind(wxEVT_SET_FOCUS, [this, zRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zRotateTextCtrl, EDITOR_ROTATE, EDITOR_Z); event.Skip(); });
        zRotateTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zRotateTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zRotateTextCtrl, EDITOR_ROTATE, EDITOR_Z); event.Skip(); });
        rotateSizer->Add(zRotateTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_ROTATE][EDITOR_Z] = zRotateTextCtrl;

        wxStaticText* scaleUnitText = new wxStaticText(objectModifyPanel, wxID_ANY, _L("°"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        scaleUnitText->SetFont(ANKER_FONT_NO_2);
        //scaleUnitText->SetSize(AnkerSize(18, 22));
        //scaleUnitText->SetSizeHints(AnkerSize(18, 22), AnkerSize(18, 22));
        scaleUnitText->Fit();
        scaleUnitText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        scaleUnitText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        rotateSizer->Add(scaleUnitText, 1, wxALIGN_CENTER | wxRIGHT, 12);
    }


    AnkerSplitCtrl* splitCtrl = new AnkerSplitCtrl(objectModifyPanel);
    objectModifySizer->Add(splitCtrl, 0, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 12);


    // scale
    wxBoxSizer* scaleSizer = new wxBoxSizer(wxHORIZONTAL);
    objectModifySizer->Add(scaleSizer, 0, wxEXPAND | wxALIGN_TOP | wxTOP | wxBOTTOM, 8);

    wxStaticText* scaleText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Scale"*/_("common_slice_toolpannelmodify_title2"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    scaleText->SetFont(ANKER_FONT_NO_1);
    scaleText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    scaleText->SetForegroundColour(wxColour(105, 106, 107));
    scaleSizer->Add(scaleText, 0, wxALIGN_LEFT | wxLEFT, 12);

    scaleSizer->AddStretchSpacer(1);

    wxImage scaleUnLockImage = wxImage(wxString::FromUTF8(Slic3r::var("unlock.png")), wxBITMAP_TYPE_PNG);
    scaleUnLockImage.Rescale(16, 16);
    wxImage scaleLockedImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("locked.png")), wxBITMAP_TYPE_PNG);
    scaleLockedImagePressed.Rescale(16, 16);
    wxButton* scaleLockButton = new wxButton(objectModifyPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    scaleLockButton->SetBitmap(scaleLockedImagePressed);
    scaleLockButton->SetMinSize(scaleUnLockImage.GetSize());
    scaleLockButton->SetMaxSize(scaleUnLockImage.GetSize());
    scaleLockButton->SetSize(scaleUnLockImage.GetSize());
    scaleLockButton->SetName(BTN_PRESSED);
    scaleLockButton->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
    scaleSizer->Add(scaleLockButton, 0, wxALIGN_RIGHT | wxRIGHT, 14);

    // param table
    wxBoxSizer* sTableSizer = new wxBoxSizer(wxVERTICAL);
    objectModifySizer->Add(sTableSizer, 0, wxEXPAND | wxALIGN_TOP | wxTOP, 8);

    {
        wxBoxSizer* tableSizer = sTableSizer;
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(titleSizer, 0, wxEXPAND | wxALIGN_RIGHT | wxRIGHT, 58);

        titleSizer->AddStretchSpacer(1);

        wxStaticText* xText = new wxStaticText(objectModifyPanel, wxID_ANY, L"X", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        xText->SetFont(ANKER_FONT_NO_1);
        xText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xText->SetForegroundColour(wxColour(255, 0, 0));
#ifdef __APPLE__
        titleSizer->Add(xText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(120));
#else
        titleSizer->Add(xText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(108));
#endif 


        wxStaticText* yText = new wxStaticText(objectModifyPanel, wxID_ANY, L"Y", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        yText->SetFont(ANKER_FONT_NO_1);
        yText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        yText->SetForegroundColour(wxColour(0, 255, 0));
        titleSizer->Add(yText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(50));

        wxStaticText* zText = new wxStaticText(objectModifyPanel, wxID_ANY, L"Z", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        zText->SetFont(ANKER_FONT_NO_1);
        zText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zText->SetForegroundColour(wxColour(0, 0, 255));
#ifdef __APPLE__
        titleSizer->Add(zText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(35));
#else
        titleSizer->Add(zText, 1, wxALIGN_RIGHT | wxLEFT, AnkerLength(45));
#endif 


        wxBoxSizer* factorSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(factorSizer, 0, wxEXPAND | wxALIGN_LEFT | wxTOP, 8);

        wxStaticText* factorText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Factor"*/_("common_slice_toolpannelmodify_factors"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        factorText->SetFont(ANKER_FONT_NO_1);
        factorText->SetMinSize(AnkerSize(100, 24));
        //factorText->Fit();
        factorText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        factorText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        factorSizer->Add(factorText, 1, wxALIGN_CENTER | wxLEFT, 12);

        factorSizer->AddStretchSpacer(1);

        char text[10];
        sprintf(text, "%d", 90);
        wxTextCtrl* xFactorTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        xFactorTextCtrl->SetFont(ANKER_FONT_NO_1);
        xFactorTextCtrl->SetMinSize(AnkerSize(54, 24));
        xFactorTextCtrl->SetMaxSize(AnkerSize(54, 24));
        xFactorTextCtrl->SetSize(AnkerSize(54, 24));
        xFactorTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xFactorTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));      
        xFactorTextCtrl->SetValidator(factorValidator);
        //xFactorTextCtrl->Bind(wxEVT_TEXT, [this, xFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_X); });
        xFactorTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, xFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_X); });
        xFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xFactorTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(xFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_X); event.Skip(); });
        xFactorTextCtrl->Bind(wxEVT_SET_FOCUS, [this, xFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_X); event.Skip(); });
        xFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_X); event.Skip(); });
        factorSizer->Add(xFactorTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_FACTOR][EDITOR_X] = xFactorTextCtrl;

        sprintf(text, "%d", 90);
        wxTextCtrl* yFactorTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        yFactorTextCtrl->SetFont(ANKER_FONT_NO_1);
        yFactorTextCtrl->SetMinSize(AnkerSize(54, 24));
        yFactorTextCtrl->SetMaxSize(AnkerSize(54, 24));
        yFactorTextCtrl->SetSize(AnkerSize(54, 24));
        yFactorTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        yFactorTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        yFactorTextCtrl->SetValidator(factorValidator);
        //yFactorTextCtrl->Bind(wxEVT_TEXT, [this, yFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Y); });
        yFactorTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, yFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(yFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Y); });
        yFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yFactorTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(yFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Y); event.Skip(); });
        yFactorTextCtrl->Bind(wxEVT_SET_FOCUS, [this, yFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Y); event.Skip(); });
        yFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, yFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(yFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Y); event.Skip(); });
        factorSizer->Add(yFactorTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_FACTOR][EDITOR_Y] = yFactorTextCtrl;

        sprintf(text, "%d", 90);
        wxTextCtrl* zFactorTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        zFactorTextCtrl->SetFont(ANKER_FONT_NO_1);
        zFactorTextCtrl->SetMinSize(AnkerSize(54, 24));
        zFactorTextCtrl->SetMaxSize(AnkerSize(54, 24));
        zFactorTextCtrl->SetSize(AnkerSize(54, 24));
        zFactorTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zFactorTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        zFactorTextCtrl->SetValidator(factorValidator);
        //zFactorTextCtrl->Bind(wxEVT_TEXT, [this, zFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Z); });
        zFactorTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, zFactorTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Z); });
        zFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zFactorTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(zFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Z); event.Skip(); });
        zFactorTextCtrl->Bind(wxEVT_SET_FOCUS, [this, zFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Z); event.Skip(); });
        zFactorTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zFactorTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zFactorTextCtrl, EDITOR_SCALE_FACTOR, EDITOR_Z); event.Skip(); });
        factorSizer->Add(zFactorTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_FACTOR][EDITOR_Z] = zFactorTextCtrl;

        wxStaticText* factorUnitText = new wxStaticText(objectModifyPanel, wxID_ANY, L"%", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        factorUnitText->SetFont(ANKER_FONT_NO_2);
        factorUnitText->Fit();
        factorUnitText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        factorUnitText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        factorSizer->Add(factorUnitText, 1, wxALIGN_CENTER | wxRIGHT, 12);

        wxBoxSizer* sizeSizer = new wxBoxSizer(wxHORIZONTAL);
        tableSizer->Add(sizeSizer, 0, wxEXPAND | wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

        wxStaticText* sizeText = new wxStaticText(objectModifyPanel, wxID_ANY, /*L"Size"*/_("common_slice_toolpannelmodify_size"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        sizeText->SetFont(ANKER_FONT_NO_1);
        sizeText->SetMinSize(AnkerSize(100, 24));
       // sizeText->Fit();
        sizeText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        sizeText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        sizeSizer->Add(sizeText, 1, wxALIGN_CENTER | wxLEFT, 12);

        sizeSizer->AddStretchSpacer(1);

        sprintf(text, "%d", 0);
        wxTextCtrl* xSizeTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        xSizeTextCtrl->SetFont(ANKER_FONT_NO_1);
        xSizeTextCtrl->SetMinSize(AnkerSize(54, 24));
        xSizeTextCtrl->SetMaxSize(AnkerSize(54, 24));
        xSizeTextCtrl->SetSize(AnkerSize(54, 24));
        xSizeTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        xSizeTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        xSizeTextCtrl->SetValidator(sizeValidator);
        //xSizeTextCtrl->Bind(wxEVT_TEXT, [this, xSizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_X); });
        xSizeTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, xSizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(xSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_X); });
        xSizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xSizeTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(xSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_X); event.Skip(); });
        xSizeTextCtrl->Bind(wxEVT_SET_FOCUS, [this, xSizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_X); event.Skip(); });
        xSizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, xSizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(xSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_X); event.Skip(); });
        sizeSizer->Add(xSizeTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_SIZE][EDITOR_X] = xSizeTextCtrl;

        sprintf(text, "%d", 0);
        wxTextCtrl* ySizeTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        ySizeTextCtrl->SetFont(ANKER_FONT_NO_1);
        ySizeTextCtrl->SetMinSize(AnkerSize(54, 24));
        ySizeTextCtrl->SetMaxSize(AnkerSize(54, 24));
        ySizeTextCtrl->SetSize(AnkerSize(54, 24));
        ySizeTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        ySizeTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        ySizeTextCtrl->SetValidator(sizeValidator);
        //ySizeTextCtrl->Bind(wxEVT_TEXT, [this, ySizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(ySizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Y); });
        ySizeTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, ySizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(ySizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Y); });
        ySizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, ySizeTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(ySizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Y); event.Skip(); });
        ySizeTextCtrl->Bind(wxEVT_SET_FOCUS, [this, ySizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(ySizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Y); event.Skip(); });
        ySizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, ySizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(ySizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Y); event.Skip(); });
        sizeSizer->Add(ySizeTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_SIZE][EDITOR_Y] = ySizeTextCtrl;

        sprintf(text, "%d", 0);
        wxTextCtrl* zSizeTextCtrl = new wxTextCtrl(objectModifyPanel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTE_CENTER | wxTE_PROCESS_ENTER);
        zSizeTextCtrl->SetFont(ANKER_FONT_NO_1);
        zSizeTextCtrl->SetMinSize(AnkerSize(54, 24));
        zSizeTextCtrl->SetMaxSize(AnkerSize(54, 24));
        zSizeTextCtrl->SetSize(AnkerSize(54, 24));
        zSizeTextCtrl->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        zSizeTextCtrl->SetForegroundColour(wxColour(TEXT_LIGHT_RGB_INT));
        zSizeTextCtrl->SetValidator(sizeValidator);
        //zSizeTextCtrl->Bind(wxEVT_TEXT, [this, zSizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Z); });
        zSizeTextCtrl->Bind(wxEVT_TEXT_ENTER, [this, zSizeTextCtrl, applyFunc](wxCommandEvent& event) { applyFunc(zSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Z); });
        zSizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zSizeTextCtrl, applyFunc](wxFocusEvent& event) { applyFunc(zSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Z); event.Skip(); });
        zSizeTextCtrl->Bind(wxEVT_SET_FOCUS, [this, zSizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Z); event.Skip(); });
        zSizeTextCtrl->Bind(wxEVT_KILL_FOCUS, [this, zSizeTextCtrl, focusFunc](wxFocusEvent& event) { focusFunc(zSizeTextCtrl, EDITOR_SCALE_SIZE, EDITOR_Z); event.Skip(); });
        sizeSizer->Add(zSizeTextCtrl, 0, wxALIGN_RIGHT | wxRIGHT, 4);
        m_editor[EDITOR_SCALE_SIZE][EDITOR_Z] = zSizeTextCtrl;

        wxStaticText* sizeUnitText = new wxStaticText(objectModifyPanel, wxID_ANY, L"mm", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        sizeUnitText->SetFont(ANKER_FONT_NO_2);
        sizeUnitText->Fit();
        sizeUnitText->SetBackgroundColour(wxColour(PANEL_BACK_RGB_INT));
        sizeUnitText->SetForegroundColour(wxColour(TEXT_DARK_RGB_INT));
        sizeSizer->Add(sizeUnitText, 1, wxALIGN_CENTER | wxRIGHT, 12);
    }


    objectModifySizer->AddStretchSpacer(1);


    container->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, returnBtnID, resetBtnID, container](wxCommandEvent& event) {
        int btnID = event.GetInt();
        if (btnID == returnBtnID)
        {
            wxCommandEvent evt = wxCommandEvent(wxANKEREVT_AOM_RETURN);
            ProcessEvent(evt);

            if (m_pReturnFunc)
                m_pReturnFunc();
        }
        else if (btnID == resetBtnID)
        {
            this->reset();
        }
        });

    scaleLockButton->Bind(wxEVT_BUTTON, [this, scaleLockButton, container](wxCommandEvent& event) {
        this->set_uniform_scaling(!this->get_uniform_scaling());

        wxImage scaleUnLockImage = wxImage(wxString::FromUTF8(Slic3r::var("unlock.png")), wxBITMAP_TYPE_PNG);
        scaleUnLockImage.Rescale(16, 16);
        wxImage scaleLockedImagePressed = wxImage(wxString::FromUTF8(Slic3r::var("locked.png")), wxBITMAP_TYPE_PNG);
        scaleLockedImagePressed.Rescale(16, 16);
        scaleLockButton->SetBitmap(scaleLockButton->GetName() == BTN_PRESSED ? scaleUnLockImage : scaleLockedImagePressed);
        scaleLockButton->SetName(scaleLockButton->GetName() == BTN_PRESSED ? BTN_NORMAL : BTN_PRESSED);

        container->Layout();
        container->Refresh();
        });
}

void AnkerObjectManipulator::set_coordinates_type(ECoordinatesType type)
{
    if (wxGetApp().get_mode() == comSimple)
        type = ECoordinatesType::World;

    if (m_coordinates_type == type)
        return;

    m_coordinates_type = type;
    this->set_dirty();
    this->update_if_dirty();

    GLCanvas3D* canvas = wxGetApp().plater()->canvas3D();
    canvas->get_gizmos_manager().update_data();
    canvas->set_as_dirty();
    canvas->request_extra_frame();
}

ECoordinatesType AnkerObjectManipulator::get_coordinates_type() const
{
    return m_coordinates_type;
}

void AnkerObjectManipulator::reset()
{
    GLCanvas3D* canvas = wxGetApp().plater()->canvas3D();
    Selection& selection = canvas->get_selection();
    selection.setup_cache();

    // reset rotate
    if (selection.is_single_volume_or_modifier()) {
        GLVolume* vol = const_cast<GLVolume*>(selection.get_first_volume());
        Geometry::Transformation trafo = vol->get_volume_transformation();
        trafo.reset_rotation();
        vol->set_volume_transformation(trafo);
    }
    else if (selection.is_single_full_instance()) {
        Geometry::Transformation trafo = selection.get_first_volume()->get_instance_transformation();
        trafo.reset_rotation();
        for (unsigned int idx : selection.get_volume_idxs()) {
            const_cast<GLVolume*>(selection.get_volume(idx))->set_instance_transformation(trafo);
        }
    }
    //else
    //    return;
    else
    {
        Slic3r::GUI::Selection::IndicesList volumeIndices = selection.get_volume_idxs();
        for (auto itr = volumeIndices.begin(); itr != volumeIndices.end(); itr++)
        {
            GLVolume* vol = const_cast<GLVolume*>(selection.get_volume(*itr));
            Geometry::Transformation trafo = vol->get_instance_transformation();
            trafo.reset_rotation();
            vol->set_instance_transformation(trafo);
        }
    }

    // Synchronize instances/volumes.

    selection.synchronize_unselected_instances(Selection::SyncRotationType::RESET);
    selection.synchronize_unselected_volumes();

    // Copy rotation values from GLVolumes into Model (ModelInstance / ModelVolume), trigger background processing.
    canvas->do_rotate(L("Reset Rotation"));


    // reset scale factor
    if (selection.is_single_volume_or_modifier()) {
        GLVolume* vol = const_cast<GLVolume*>(selection.get_first_volume());
        Geometry::Transformation trafo = vol->get_volume_transformation();
        trafo.reset_scaling_factor();
        vol->set_volume_transformation(trafo);
    }
    else if (selection.is_single_full_instance()) {
        Geometry::Transformation trafo = selection.get_first_volume()->get_instance_transformation();
        trafo.reset_scaling_factor();
        for (unsigned int idx : selection.get_volume_idxs()) {
            const_cast<GLVolume*>(selection.get_volume(idx))->set_instance_transformation(trafo);
        }
    }
    //else
    //    return;
    else
    {
        Slic3r::GUI::Selection::IndicesList volumeIndices = selection.get_volume_idxs();
        for (auto itr = volumeIndices.begin(); itr != volumeIndices.end(); itr++)
        {
            GLVolume* vol = const_cast<GLVolume*>(selection.get_volume(*itr));
            Geometry::Transformation trafo = vol->get_instance_transformation();
            trafo.reset_scaling_factor();
            vol->set_instance_transformation(trafo);
        }
    }

    // Synchronize instances/volumes. 
    selection.synchronize_unselected_instances(Selection::SyncRotationType::GENERAL);
    selection.synchronize_unselected_volumes();

    canvas->do_scale(L("Reset scale"));


    //// reset scale factor
    //if (selection.is_single_volume_or_modifier()) {
    //    GLVolume* vol = const_cast<GLVolume*>(selection.get_first_volume());
    //    Geometry::Transformation trafo = vol->get_volume_transformation();
    //    trafo.reset_offset();
    //    vol->set_volume_transformation(trafo);
    //}
    //else if (selection.is_single_full_instance()) {
    //    Geometry::Transformation trafo = selection.get_first_volume()->get_instance_transformation();
    //    trafo.reset_offset();
    //    for (unsigned int idx : selection.get_volume_idxs()) {
    //        const_cast<GLVolume*>(selection.get_volume(idx))->set_instance_transformation(trafo);
    //    }
    //}
    //else
    //    return;

    //// Synchronize instances/volumes. 
    //selection.synchronize_unselected_instances(Selection::SyncRotationType::GENERAL);
    //selection.synchronize_unselected_volumes();

    //canvas->do_move(L("Reset move"));

    this->set_dirty();
    this->update_if_dirty(true);
}

void AnkerObjectManipulator::OnTextEdit(EditorType editor, EditorAxisType axis, double newValue)
{
    if (!m_cache.is_valid())
        return;

    if (editor == EditorType::EDITOR_TRANSLATE) {
        if (m_imperial_units)
            newValue *= in_to_mm;
        change_position_value(axis, newValue);
    }
    else if (editor == EditorType::EDITOR_ROTATE)
        change_rotation_value(axis, newValue);
    else if (editor == EditorType::EDITOR_SCALE_FACTOR) {
        if (newValue > 0.0)
            change_scale_value(axis, newValue);
        else {
            newValue = m_cache.scale(axis);
            m_cache.scale(axis) = 0.0;
            m_cache.scale_rounded(axis) = DBL_MAX;
            change_scale_value(axis, newValue);
        }
    }
    else if (editor == EditorType::EDITOR_SCALE_SIZE) {
        if (newValue > 0.0)
            change_size_value(axis, newValue);
        else {
            Vec3d& size = m_imperial_units ? m_cache.size_inches : m_cache.size;
            newValue = size(axis);
            size(axis) = 0.0;
            m_cache.size_rounded(axis) = DBL_MAX;
            change_size_value(axis, newValue);
        }
    }
}

void AnkerObjectManipulator::reset_settings_value()
{
    m_new_position = Vec3d::Zero();
    m_new_rotation = Vec3d::Zero();
    m_new_scale = Vec3d::Ones() * 100.;
    m_new_size = Vec3d::Zero();
    m_new_enabled = false;
    // no need to set the dirty flag here as this method is called from update_settings_value(),
    // which is called from update_if_dirty(), which resets the dirty flag anyways.
//    m_dirty = true;
}

void AnkerObjectManipulator::update_settings_value(const Selection& selection)
{
    if (selection.is_empty()) {
        // No selection, reset the cache.
        reset_settings_value();
        return;
    }

    //m_new_move_label_string = L("Position");
    //m_new_rotate_label_string = L("Rotation");
    //m_new_scale_label_string = L("Scale factors");

    //ObjectList* obj_list = wxGetApp().obj_list();
    if (selection.is_single_full_instance()) {
        // all volumes in the selection belongs to the same instance, any of them contains the needed instance data, so we take the first one
        const GLVolume* volume = selection.get_first_volume();

        if (is_world_coordinates()) {
            m_new_position = volume->get_instance_offset();
            m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
            m_new_scale = m_new_size.cwiseQuotient(selection.get_unscaled_instance_bounding_box().size()) * 100.0;
            //m_new_rotate_label_string = L("Rotate (relative)");
            m_new_rotation = Vec3d::Zero();
        }
        else {
            //m_new_move_label_string = L("Translate (relative) [World]");
            //m_new_rotate_label_string = L("Rotate (relative)");
            m_new_position = Vec3d::Zero();
            m_new_rotation = Vec3d::Zero();
            m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
            m_new_scale = m_new_size.cwiseQuotient(selection.get_full_unscaled_instance_local_bounding_box().size()) * 100.0;
        }

        m_new_enabled = true;
    }
    else if (selection.is_single_full_object()/* && obj_list->is_selected(itObject)*/) {
        const BoundingBoxf3& box = selection.get_bounding_box();
        m_new_position = box.center();
        m_new_rotation = Vec3d::Zero();
        m_new_scale = Vec3d(100.0, 100.0, 100.0);
        m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
        //m_new_rotate_label_string = L("Rotate");
        //m_new_scale_label_string = L("Scale");
        m_new_enabled = true;
    }
    else if (selection.is_single_volume_or_modifier()) {
        // the selection contains a single volume
        const GLVolume* volume = selection.get_first_volume();
        if (is_world_coordinates()) {
            const Geometry::Transformation trafo(volume->world_matrix());

            const Vec3d& offset = trafo.get_offset();

            m_new_position = offset;
            //m_new_rotate_label_string = L("Rotate (relative)");
            //m_new_scale_label_string = L("Scale");
            m_new_scale = Vec3d(100.0, 100.0, 100.0);
            m_new_rotation = Vec3d::Zero();
            m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
        }
        else if (is_local_coordinates()) {
            //m_new_move_label_string = L("Translate (relative) [World]");
            //m_new_rotate_label_string = L("Rotate (relative)");
            m_new_position = Vec3d::Zero();
            m_new_rotation = Vec3d::Zero();
            m_new_scale = volume->get_volume_scaling_factor() * 100.0;
            m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
        }
        else {
            m_new_position = volume->get_volume_offset();
            //m_new_rotate_label_string = L("Rotate (relative)");
            m_new_rotation = Vec3d::Zero();
            //m_new_scale_label_string = L("Scale");
            m_new_scale = Vec3d(100.0, 100.0, 100.0);
            m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
        }
        m_new_enabled = true;
    }
    else/* if (obj_list->is_connectors_item_selected() || obj_list->multiple_selection() || obj_list->is_selected(itInstanceRoot))*/ {
        reset_settings_value();
        //m_new_move_label_string = L("Translate");
        //m_new_rotate_label_string = L("Rotate");
        //m_new_scale_label_string = L("Scale");
        m_new_size = selection.get_bounding_box_in_current_reference_system().first.size();
        m_new_enabled = true;
    }

    if (m_new_size(EDITOR_X) > 0 && m_new_size(EDITOR_Y) > 0 && m_new_size(EDITOR_Z) > 0) {
        wxString str = wxString::Format("%.2f X %.2f X %.2f mm", m_new_size(EDITOR_X), m_new_size(EDITOR_Y), m_new_size(EDITOR_Z));
        wxGetApp().plater()->setScaledModelObjectSizeText(str);
    }
}

void AnkerObjectManipulator::change_position_value(int axis, double value)
{
    if (std::abs(m_cache.position_rounded(axis) - value) < EPSILON)
        return;

    Vec3d position = m_cache.position;
    position(axis) = value;

    auto canvas = wxGetApp().plater()->canvas3D();
    Selection& selection = canvas->get_selection();
    selection.setup_cache();
    TransformationType trafo_type;
    trafo_type.set_relative();
    switch (get_coordinates_type())
    {
    case ECoordinatesType::Instance: { trafo_type.set_instance(); break; }
    case ECoordinatesType::Local: { trafo_type.set_local(); break; }
    default: { break; }
    }
    selection.translate(position - m_cache.position, trafo_type);
    canvas->do_move(L("Set Position"));

    m_cache.position = position;
    m_cache.position_rounded(axis) = DBL_MAX;

    this->set_dirty();
    this->update_if_dirty();
}

void AnkerObjectManipulator::change_rotation_value(int axis, double value)
{
    if (std::abs(m_cache.rotation_rounded(axis) - value) < EPSILON)
        return;

    Vec3d rotation = m_cache.rotation;
    rotation(axis) = value;

    GLCanvas3D* canvas = wxGetApp().plater()->canvas3D();
    Selection& selection = canvas->get_selection();

    TransformationType transformation_type;
    transformation_type.set_relative();
    if (selection.is_single_full_instance())
        transformation_type.set_independent();

    if (is_local_coordinates())
        transformation_type.set_local();

    if (is_instance_coordinates())
        transformation_type.set_instance();

    selection.setup_cache();
    selection.rotate(
        (M_PI / 180.0) * (transformation_type.absolute() ? rotation : rotation - m_cache.rotation),
        transformation_type);
    canvas->do_rotate(L("Set Orientation"));

    m_cache.rotation = rotation;
    m_cache.rotation_rounded(axis) = DBL_MAX;

    this->set_dirty();
    this->update_if_dirty();
}

void AnkerObjectManipulator::change_scale_value(int axis, double value)
{
    if (value <= 0.0)
        return;

    if (std::abs(m_cache.scale_rounded(axis) - value) < EPSILON)
        return;

    Vec3d scale = m_cache.scale;
    scale(axis) = value;

    const Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
    Vec3d ref_scale = m_cache.scale;
    if (selection.is_single_volume_or_modifier()) {
        scale = scale.cwiseQuotient(ref_scale);
        ref_scale = Vec3d::Ones();
    }
    else if (selection.is_single_full_instance())
        ref_scale = 100.0 * Vec3d::Ones();

    this->do_scale(axis, scale.cwiseQuotient(ref_scale));

    m_cache.scale = scale;
    m_cache.scale_rounded(axis) = DBL_MAX;

    this->set_dirty();
    this->update_if_dirty();
}

void AnkerObjectManipulator::change_size_value(int axis, double value)
{
    if (value <= 0.0)
        return;

    if (m_imperial_units) {
        if (std::abs(m_cache.size_inches(axis) - value) < EPSILON)
            return;
        m_cache.size_inches(axis) = value;
        value *= in_to_mm;
    }

    if (std::abs(m_cache.size_rounded(axis) - value) < EPSILON)
        return;

    Vec3d size = m_cache.size;
    size(axis) = value;

    const Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();

    Vec3d ref_size = m_cache.size;
    if (selection.is_single_volume_or_modifier()) {
        size = size.cwiseQuotient(ref_size);
        ref_size = Vec3d::Ones();
    }
    else if (selection.is_single_full_instance()) {
        if (is_world_coordinates())
            ref_size = selection.get_full_unscaled_instance_bounding_box().size();
        else
            ref_size = selection.get_full_unscaled_instance_local_bounding_box().size();
    }

    this->do_size(axis, size.cwiseQuotient(ref_size));

    m_cache.size = size;
    m_cache.size_rounded(axis) = DBL_MAX;

    this->set_dirty();
    this->update_if_dirty();
}

void AnkerObjectManipulator::do_scale(int axis, const Slic3r::Vec3d& scale) const
{
    TransformationType transformation_type;
    if (is_local_coordinates())
        transformation_type.set_local();
    else if (is_instance_coordinates())
        transformation_type.set_instance();

    Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
    if (selection.is_single_volume_or_modifier() && !is_local_coordinates())
        transformation_type.set_relative();

    const Vec3d scaling_factor = m_uniform_scale ? scale(axis) * Vec3d::Ones() : scale;
    selection.setup_cache();
    selection.scale(scaling_factor, transformation_type);
    wxGetApp().plater()->canvas3D()->do_scale(L("Set Scale"));
}

void AnkerObjectManipulator::do_size(int axis, const Slic3r::Vec3d& scale) const
{
    Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();

    TransformationType transformation_type;
    if (is_local_coordinates())
        transformation_type.set_local();
    else if (is_instance_coordinates())
        transformation_type.set_instance();

    const Vec3d scaling_factor = m_uniform_scale ? scale(axis) * Vec3d::Ones() : scale;
    selection.setup_cache();
    selection.scale(scaling_factor, transformation_type);
    wxGetApp().plater()->canvas3D()->do_scale(L("Set Size"));
}

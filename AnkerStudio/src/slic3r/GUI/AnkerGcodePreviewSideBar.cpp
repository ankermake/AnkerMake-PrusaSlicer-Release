#include "AnkerGcodePreviewSideBar.hpp"

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/stattext.h>

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/Overlay.h>
#include "wx/dcbuffer.h"

#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Point.hpp"
#include "GLCanvas3D.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "common/AnkerGUIConfig.hpp"

#include "AnkerSideBarNew.hpp"

void AnkerGcodeViewPanel::InitGUI()
{
    wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vBox);
    // title
    {
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        titleSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        titleSizer->AddSpacer(5);
        wxStaticText* titleBanner = new wxStaticText(this, wxID_ANY, _L("common_preview_view_title")); // "View"
        titleBanner->SetBackgroundColour(wxColour("#292A2D"));
        titleBanner->SetForegroundColour(wxColour("#FFFFFF"));
        /*wxFont Font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"));*/
        titleBanner->SetFont(/*Font*/ANKER_BOLD_FONT_NO_1);
        titleBanner->SetCanFocus(false);

        titleSizer->Add(titleBanner, 0, wxALIGN_CENTER_VERTICAL , 0);
        titleSizer->AddStretchSpacer(1);
 
        m_viewGcodeLegendBtn = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        m_viewGcodeLegendBtn->SetBackgroundColour(wxColour("#292A2D"));
        {
            wxBitmap BtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("viewGcodeLegend_Show.png")), wxBITMAP_TYPE_PNG);
            wxImage BtnImage = BtnBitmap.ConvertToImage();
            BtnImage.Rescale(AnkerLength(15), AnkerLength(15));
            wxBitmap scaledBitmap(BtnImage);
            m_viewGcodeLegendBtn->SetBitmap(scaledBitmap);
            m_viewGcodeLegendBtn->SetMinSize(scaledBitmap.GetSize());
            m_viewGcodeLegendBtn->SetMaxSize(scaledBitmap.GetSize());
        }
        UpdateViewGcodeLegendBtn();
        m_viewGcodeLegendBtn->Bind(wxEVT_BUTTON, &AnkerGcodeViewPanel::onViewGcodeLegendBtnClick, this);

        titleSizer->Add(m_viewGcodeLegendBtn, 0, wxALIGN_CENTER_VERTICAL |wxALIGN_RIGHT, 0);
        titleSizer->AddSpacer(10);

        vBox->Add(titleSizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
    }

    {
        // Line
        wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
        splitLineCtrl->SetMaxSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        //splitLineCtrl->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        vBox->Add(splitLineCtrl, 0, wxEXPAND, 0);
    }

    // combox
    wxArrayString itemList;
    itemList.Add(_L("common_preview_sheettitle_featuretype"));  // "Feature Type"
    itemList.Add(_L("common_preview_viewtype_height"));                                 // "Height"
    itemList.Add(_L("common_preview_viewtype_width"));                                  // "Width"
    itemList.Add(_L("common_preview_viewtype_speed"));                                  // "Speed"
    itemList.Add(_L("common_preview_viewtype_fanspeed"));                              // "Fan Speed"
    itemList.Add(_L("common_preview_viewtype_temperature"));                            // "Temperature"
    itemList.Add(_L("common_preview_viewtype_volumetricflow"));                   // "Volumetric Flow Rate"
    itemList.Add(_L("common_preview_viewtype_linearlayertime"));                    // "Layer Time (linear)"
    itemList.Add(_L("common_preview_viewtype_logarithmiclayertime"));               // "Layer Time (logarithmic)"
    itemList.Add(_L("common_preview_viewtype_tool"));                                   // "Tool"
    itemList.Add(_L("common_preview_viewtype_colortprint"));                            // "Color Print"

    wxImage dropBtnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
    dropBtnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
    wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));
    wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));
    wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(dropBtnImage));

    m_viewTypeChoice = new AnkerSimpleCombox();
    m_viewTypeChoice->Create(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, itemList, wxNO_BORDER | wxCB_READONLY);
    m_viewTypeChoice->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
    m_viewTypeChoice->SetBackgroundColour(wxColour("#292A2D"));
    m_viewTypeChoice->setColor(wxColour("#434447"), wxColour("#3A3B3F"));
    m_viewTypeChoice->SetMinSize(wxSize(290, ANKER_COMBOBOX_HEIGHT));
    m_viewTypeChoice->SetSize(wxSize(290, ANKER_COMBOBOX_HEIGHT));
    m_viewTypeChoice->SetSelection(0);
    m_viewTypeChoice->Bind(wxEVT_COMBOBOX, &AnkerGcodeViewPanel::OnChoiceSelected, this);
    vBox->Add(m_viewTypeChoice, 0, wxEXPAND | wxALL, 10);
}


void AnkerGcodeViewPanel::onViewGcodeLegendBtnClick(wxCommandEvent& event)
{
    ANKER_LOG_INFO << "view gcode legent btn clicked";
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        ANKER_LOG_ERROR << "plater is null";
        return;
    }

    bool show = plater->is_legend_shown();
    plater->show_legend(!show);
    UpdateViewGcodeLegendBtn();
}

void AnkerGcodeViewPanel::OnChoiceSelected(wxCommandEvent& event)
{
    wxOwnerDrawnComboBox* pComboboxCtrl = dynamic_cast<wxOwnerDrawnComboBox*>(event.GetEventObject());
    int index = 0;
    wxString selectedChoice = wxString();

    if (pComboboxCtrl)
    {
        index = pComboboxCtrl->GetSelection();
        selectedChoice = pComboboxCtrl->GetString(index);
    }
    else {
        ANKER_LOG_ERROR << "pComboboxCtrl is null"  ;
    }
    ANKER_LOG_INFO << "  ---------------OnChoiceSelected:"<< index << "   text:"<< selectedChoice ;

    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        ANKER_LOG_ERROR << "plater is null";
        return;
    }

    GCodeViewer::EViewType viewType = GetViewTypeFromName(selectedChoice);
    GLCanvas3D* canvas = plater->canvas_preview();
    if (canvas) {
        canvas->set_gcode_view_preview_type(viewType);
        canvas->render();
    }
}


void AnkerGcodeViewPanel::UpdateCurrentChoice(GCodeViewer::EViewType viewType)
{
    if (!m_viewTypeChoice)
        return;

    int itemCount = m_viewTypeChoice->GetCount();
    for (int i = 0; i < itemCount; ++i) {
        wxString itemText = m_viewTypeChoice->GetString(i);
        if (viewType == GetViewTypeFromName(itemText))
        {
            m_viewTypeChoice->SetSelection(i);
            Refresh();
            break;
        }
    }
}

void AnkerGcodeViewPanel::UpdateCurrentChoice()
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        return;
    }

    GLCanvas3D* canvas = plater->canvas_preview();
    if (canvas) {
        GCodeViewer::EViewType type = canvas->get_gcode_view_preview_type();
        UpdateCurrentChoice(type);
    }
}


void AnkerGcodeViewPanel::UpdateViewGcodeLegendBtn()
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        return;
    }

    bool show = plater->is_legend_shown();
    std::string iconFile = show ? "viewGcodeLegend_Show.png" : "viewGcodeLegend_Hide.png";
    wxBitmap BtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var(iconFile)), wxBITMAP_TYPE_PNG);
    wxImage BtnImage = BtnBitmap.ConvertToImage();
    BtnImage.Rescale(AnkerLength(15), AnkerLength(15));
    wxBitmap scaledBitmap(BtnImage);
    m_viewGcodeLegendBtn->SetBitmap(scaledBitmap);

    Refresh();
}


//GCodeViewer::EViewType 
GCodeViewer::EViewType AnkerGcodeViewPanel::GetViewTypeFromName(wxString viewTypeName)
{
    GCodeViewer::EViewType retType = GCodeViewer::EViewType::Count;
        if (viewTypeName == _L("common_preview_viewtype_feature")) // "Feature Type
            retType = GCodeViewer::EViewType::FeatureType;
        else if (viewTypeName == _L("common_preview_viewtype_height"))
            retType = GCodeViewer::EViewType::Height;
        else if (viewTypeName == _L("common_preview_viewtype_width"))
            retType = GCodeViewer::EViewType::Width;
        else if (viewTypeName == _L("common_preview_viewtype_speed"))
            retType = GCodeViewer::EViewType::Feedrate;
        else if (viewTypeName == _L("common_preview_viewtype_fanspeed"))
            retType = GCodeViewer::EViewType::FanSpeed;
        else if (viewTypeName == _L("common_preview_viewtype_temperature"))
            retType = GCodeViewer::EViewType::Temperature;
        else if (viewTypeName == _L("common_preview_viewtype_volumetricflow"))
            retType = GCodeViewer::EViewType::VolumetricRate;
        else if (viewTypeName == _L("common_preview_viewtype_linearlayertime"))
            retType = GCodeViewer::EViewType::LayerTimeLinear;
        else if (viewTypeName == _L("common_preview_viewtype_logarithmiclayertime"))
            retType = GCodeViewer::EViewType::LayerTimeLogarithmic;
        else if (viewTypeName == _L("common_preview_viewtype_tool"))
            retType = GCodeViewer::EViewType::Tool;
        else if (viewTypeName == _L("common_preview_viewtype_colortprint"))
            retType = GCodeViewer::EViewType::ColorPrint;

        return retType;
}


void AnkerGCodeExtrusionRoleSelectPanel::InitGUI()
{
    wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vBox);
    // title
    {
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);

        //titleSizer->AddSpacer(3);
        wxStaticText* titleBanner = new wxStaticText(this, wxID_ANY, _L("common_preview_showoptions_title"));   // "Show Options"
        titleBanner->SetBackgroundColour(wxColour("#292A2D"));
        titleBanner->SetForegroundColour(wxColour("#FFFFFF"));
        //wxFont Font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"));
        titleBanner->SetFont(/*Font*/ANKER_BOLD_FONT_NO_1);
        titleBanner->SetCanFocus(false);
        titleSizer->Add(titleBanner, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 0);

        // All lable
        titleSizer->AddStretchSpacer(1);
        wxStaticText* AllLabel = new wxStaticText(this, wxID_ANY, _L("common_preview_showoptions_all"));   // "All"
        AllLabel->SetBackgroundColour(wxColour("#292A2D"));
        AllLabel->SetForegroundColour(wxColour("#FFFFFF"));
        AllLabel->SetFont(/*Font*/ANKER_BOLD_FONT_NO_1);
        AllLabel->SetCanFocus(false);
        titleSizer->Add(AllLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);

        titleSizer->AddSpacer(3);

        // All checkbox
        wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
        uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap uncheckScaledBitmap(uncheckImage);

        wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check_style_gray.png")), wxBITMAP_TYPE_PNG);
        checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap checkScaledBitmap(checkImage);

		wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
		disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disUncheckScaledBitmap(disuncheckImage);

		wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
		discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disCheckScaledBitmap(discheckImage);


        m_allcheckBox = new AnkerCheckBox(this, 
                                          uncheckScaledBitmap.ConvertToImage(),
                                          checkScaledBitmap.ConvertToImage(),
                                          disUncheckScaledBitmap.ConvertToImage(),
                                          disCheckScaledBitmap.ConvertToImage(),
                                          wxString(),
                                          ANKER_FONT_NO_1,
                                          wxColour("#FFFFFF"),
                                          wxID_ANY);

        m_allcheckBox->SetWindowStyleFlag(wxBORDER_NONE);
        m_allcheckBox->SetBackgroundColour(wxColour("#292A2D"));
        m_allcheckBox->SetMinSize(wxSize(16, 16));
        m_allcheckBox->SetMaxSize(wxSize(16, 16));
        m_allcheckBox->SetSize(wxSize(16, 16));
        m_allcheckBox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, &AnkerGCodeExtrusionRoleSelectPanel::OnAllClicked, this);
        titleSizer->Add(m_allcheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);

        titleSizer->AddSpacer(10);

        vBox->Add(titleSizer, 0, wxEXPAND | wxLEFT|wxTOP|wxBOTTOM, 10);
    }
    // Line
    {
        wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
        splitLineCtrl->SetMaxSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        splitLineCtrl->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        vBox->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);
    }

    // option list in slider
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);

    wxBoxSizer* optionListSizer = new wxBoxSizer(wxVERTICAL);

    auto addOneExtrusionRole = [&](const wxString& itemName) {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->AddSpacer(5);

        // lablue
        wxStaticText* roleName = new wxStaticText(scrolledWindow, wxID_ANY, itemName);
        roleName->SetBackgroundColour(wxColour("#292A2D"));
        roleName->SetForegroundColour(wxColour("#FFFFFF"));
        roleName->SetFont(ANKER_FONT_NO_1);

        rowSizer->Add(roleName, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 10);

        rowSizer->AddStretchSpacer(5);

        //check box
        wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
        uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap uncheckScaledBitmap(uncheckImage);

        wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check_style_gray.png")), wxBITMAP_TYPE_PNG);
        checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap checkScaledBitmap(checkImage);

		wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
		disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disUncheckScaledBitmap(disuncheckImage);

		wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
		discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disCheckScaledBitmap(discheckImage);

        wxWindowID id = wxNewId();
        AnkerCheckBox* roleCheckBox = new AnkerCheckBox(scrolledWindow, 
                                                        uncheckScaledBitmap.ConvertToImage(), 
                                                        checkScaledBitmap.ConvertToImage(),
                                                        disUncheckScaledBitmap.ConvertToImage(),
                                                        disCheckScaledBitmap.ConvertToImage(),
                                                        wxString(),
                                                        ANKER_FONT_NO_1,
                                                        wxColour("#FFFFFF"),
                                                        id);
        roleCheckBox->SetWindowStyleFlag(wxBORDER_NONE);
        roleCheckBox->SetBackgroundColour(wxColour("#292A2D"));
        roleCheckBox->SetMinSize(wxSize(16, 16));
        roleCheckBox->SetMaxSize(wxSize(16, 16));
        roleCheckBox->SetSize(wxSize(16, 16));
        roleCheckBox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, &AnkerGCodeExtrusionRoleSelectPanel::OnGCodeExtrusionRoleClicked, this);
        roleCheckBox->SetName(itemName);
        m_ExtrusionRoleStringForCheckBoxID[id] = roleCheckBox;
        rowSizer->Add(roleCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        rowSizer->AddSpacer(5);
        optionListSizer->Add(rowSizer, 0, wxEXPAND | wxALL, 5);
    };

    addOneExtrusionRole(_L("common_preview_showoptions_perimeter"));     // "Perimeter"
    addOneExtrusionRole(_L("common_preview_showoptions_externalperimeter"));                       // "External Perimeter"
    addOneExtrusionRole(_L("common_preview_showoptions_overhang"));                       // "Overhang Perimeter"
    addOneExtrusionRole(_L("common_preview_showoptions_internalinfill"));                          // "Internal Infill"
    addOneExtrusionRole(_L("common_preview_showoptions_solidinfill"));                             // "Solid Infill"
    addOneExtrusionRole(_L("common_preview_showoptions_topsolidinfill"));                         // "Top Solid Infill"
    addOneExtrusionRole(_L("common_preview_showoptions_ironing"));                                  // "Ironing"
    addOneExtrusionRole(_L("common_preview_showoptions_bridgeinfill"));                            // "Bridge Infill"
    addOneExtrusionRole(_L("common_preview_showoptions_gapfill"));                                 // "Gap Fill"
    addOneExtrusionRole(_L("common_preview_showoptions_skirtbrim"));                               // "Skirt/Brim"
    addOneExtrusionRole(_L("common_preview_showoptions_supportmaterial"));                         // "Support Material"
    addOneExtrusionRole(_L("common_preview_showoptions_supportmaterialinterface"));               // "Support Material Interface"
    addOneExtrusionRole(_L("common_preview_showoptions_wipetower"));                               // "Wipe Tower"
    addOneExtrusionRole(_L("common_preview_showoptions_custom"));                                   // "Custom"
    //addOneExtrusionRole(_L("common_preview_showoptions_unknown"));                                   // "Unkown"

    scrolledWindow->SetScrollRate(0, 20);
    scrolledWindow->SetMinSize(wxSize(320,-1 ));
    scrolledWindow->SetSize(wxSize(320, -1));
    scrolledWindow->SetSizer(optionListSizer);

    vBox->Add(scrolledWindow, 1, wxEXPAND| wxALL, 0);

    UpdateAllGCodeExtrusionRoleUI();
}


void AnkerGCodeExtrusionRoleSelectPanel::OnAllClicked(wxCommandEvent& event)
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        ANKER_LOG_ERROR << "plater is null";
        return;
    }

    GLCanvas3D* canvas = plater->canvas_preview();
    if (!canvas) {
        ANKER_LOG_ERROR << "preview canvas is null";
        return;
    }

    bool check = m_allcheckBox->getCheckStatus();
    ANKER_LOG_INFO << "  click All checkBox, check:" << check; 
   // if (check) {
        for (const auto& pair : m_ExtrusionRoleStringForCheckBoxID) {
            wxWindowID CheckBoxWinID = pair.first;
            wxString RoleName = static_cast<AnkerCheckBox*>(pair.second)->GetName();
            SetGCodeExtrusionRoleVisible(RoleName, check);
        }
   // }

    UpdateAllGCodeExtrusionRoleUI();
}


void AnkerGCodeExtrusionRoleSelectPanel::OnGCodeExtrusionRoleClicked(wxCommandEvent& event)
{
    //int checkBoxWindowID = event.GetId();
    //AnkerCheckBox* checkBox = wxDynamicCast(FindWindowById(checkBoxWindowID), AnkerCheckBox);
    AnkerCheckBox* checkBox = dynamic_cast<AnkerCheckBox*>(event.GetEventObject());
    if (!checkBox) {
        ANKER_LOG_ERROR << "OnGCodeExtrusionRoleClicked:checkbox pointer null" ;
        return;
    }

    ANKER_LOG_INFO << "  role:" << checkBox->GetName() << "  checkBox Val:" << checkBox->getCheckStatus();
    SetGCodeExtrusionRoleVisible(checkBox->GetName(), checkBox->getCheckStatus());
    UpdateAllGCodeExtrusionRoleUI();
}

void AnkerGCodeExtrusionRoleSelectPanel::UpdateAllGCodeExtrusionRoleUI()
{
    bool visibleAll = true;
    for (const auto& pair : m_ExtrusionRoleStringForCheckBoxID) {
        wxWindowID CheckBoxWinID = pair.first;
        //wxString RoleName = pair.second;
        wxString RoleName = static_cast<AnkerCheckBox*>(pair.second)->GetName();
        bool visible = IsGCodeExtrusionRoleVisible(RoleName);
        AnkerCheckBox* checkBox = wxDynamicCast(FindWindowById(CheckBoxWinID), AnkerCheckBox);
        if (checkBox) {
            checkBox->setCheckStatus(visible);
        }

        if (visible == false)
            visibleAll = false;
    }

    m_allcheckBox->setCheckStatus(visibleAll);

    Refresh();
}


bool AnkerGCodeExtrusionRoleSelectPanel::SetGCodeExtrusionRoleVisible(wxString roleName, bool visible)
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    GLCanvas3D* canvas = plater ? plater->canvas_preview() : nullptr;

    for (const auto& pair : m_ExtrusionRoleStringForCheckBoxID) {
        wxWindowID CheckBoxWinID = pair.first;
        wxString RoleName = static_cast<AnkerCheckBox*>(pair.second)->GetName();
        if (RoleName == roleName)
        {
            GCodeExtrusionRole role = roleNameToGcodeExtrusionRole(RoleName.ToStdString());
            if (role < GCodeExtrusionRole::Count && canvas)
                canvas->set_gCodeExtrusionRole_visible(role, visible);
            return true;
        }
    }
    return false;
}


bool AnkerGCodeExtrusionRoleSelectPanel::IsGCodeExtrusionRoleVisible(wxString roleName)
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    GLCanvas3D* canvas = plater ? plater->canvas_preview() : nullptr;

    for (const auto& pair : m_ExtrusionRoleStringForCheckBoxID) {
        wxWindowID CheckBoxWinID = pair.first;
        //wxString RoleName = pair.second;
        wxString RoleName = static_cast<AnkerCheckBox*>(pair.second)->GetName();
        if (RoleName == roleName)
        {
            GCodeExtrusionRole role = roleNameToGcodeExtrusionRole(RoleName.ToStdString());
            if (role < GCodeExtrusionRole::Count && canvas) {
                bool visible = canvas->is_gCodeExtrusionRole_visible(role);
                return visible;
            }
        }
    }
    return false;
}

GCodeExtrusionRole AnkerGCodeExtrusionRoleSelectPanel::roleNameToGcodeExtrusionRole(std::string roleStr)
{
    GCodeExtrusionRole ret = GCodeExtrusionRole::Count;
    if (roleStr == _L("common_preview_showoptions_perimeter")) {
        ret = GCodeExtrusionRole::Perimeter;
    }
    else if (roleStr == _L("common_preview_showoptions_externalperimeter")) {
        ret = GCodeExtrusionRole::ExternalPerimeter;
    }
    else if (roleStr == _L("common_preview_showoptions_overhang")) {
        ret = GCodeExtrusionRole::OverhangPerimeter;
    }
    else if (roleStr == _L("common_preview_showoptions_internalinfill")) {
        ret = GCodeExtrusionRole::InternalInfill;
    }
    else if (roleStr == _L("common_preview_showoptions_solidinfill")) {
        ret = GCodeExtrusionRole::SolidInfill;
    }
    else if (roleStr == _L("common_preview_showoptions_topsolidinfill")) {
        ret = GCodeExtrusionRole::TopSolidInfill;
    }
    else if (roleStr == _L("common_preview_showoptions_ironing")) {
        ret = GCodeExtrusionRole::Ironing;
    }
    else if (roleStr == _L("common_preview_showoptions_bridgeinfill")) {
        ret = GCodeExtrusionRole::BridgeInfill;
    }
    else if (roleStr == _L("common_preview_showoptions_gapfill")) {
        ret = GCodeExtrusionRole::GapFill;
    }
    else if (roleStr == _L("common_preview_showoptions_skirtbrim")) {
        ret = GCodeExtrusionRole::Skirt;
    }
    else if (roleStr == _L("common_preview_showoptions_supportmaterial")) {
        ret = GCodeExtrusionRole::SupportMaterial;
    }
    else if (roleStr == _L("common_preview_showoptions_supportmaterialinterface")) {
        ret = GCodeExtrusionRole::SupportMaterialInterface;
    }
    else if (roleStr == _L("common_preview_showoptions_wipetower")) {
        ret = GCodeExtrusionRole::WipeTower;
    }
    else if (roleStr == _L("common_preview_showoptions_custom")) {
        ret = GCodeExtrusionRole::Custom;
    }
    else if (roleStr == _L("common_preview_showoptions_unknown")) {// "Unknown"
        ret = GCodeExtrusionRole::None;
    }

    return ret;
}

wxString AnkerGCodeExtrusionRoleSelectPanel::GcodeExtrusionRoleToroleName(GCodeExtrusionRole role)
{
    switch (role) {
    case GCodeExtrusionRole::None: return _L("Unknown");
    case GCodeExtrusionRole::Perimeter: return _L("common_preview_showoptions_perimeter");
    case GCodeExtrusionRole::ExternalPerimeter: return _L("common_preview_showoptions_externalperimeter");
    case GCodeExtrusionRole::OverhangPerimeter: return _L("common_preview_showoptions_overhang");
    case GCodeExtrusionRole::InternalInfill: return _L("common_preview_showoptions_internalinfill");
    case GCodeExtrusionRole::SolidInfill: return _L("common_preview_showoptions_solidinfill");
    case GCodeExtrusionRole::TopSolidInfill: return _L("common_preview_showoptions_topsolidinfill");
    case GCodeExtrusionRole::Ironing: return _L("common_preview_showoptions_ironing");
    case GCodeExtrusionRole::BridgeInfill: return _L("common_preview_showoptions_bridgeinfill");
    case GCodeExtrusionRole::GapFill: return _L("common_preview_showoptions_gapfill");
    case GCodeExtrusionRole::Skirt: return _L("common_preview_showoptions_skirtbrim");
    case GCodeExtrusionRole::SupportMaterial: return _L("common_preview_showoptions_supportmaterial");
    case GCodeExtrusionRole::SupportMaterialInterface: return _L("common_preview_showoptions_supportmaterialinterface");
    case GCodeExtrusionRole::WipeTower: return _L("common_preview_showoptions_wipetower");
    case GCodeExtrusionRole::Custom: return _L("common_preview_showoptions_custom");
    default: assert(false);
    }
    return "";
}



GcodeExportProgressDialog::GcodeExportProgressDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE |/* wxSTAY_ON_TOP | */ wxFRAME_NO_TASKBAR/*wxDEFAULT_DIALOG_STYLE & ~wxCAPTION*/)
{
    InitUI();

    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater)
    {
        plater->set_export_progress_change_callback(std::bind(&GcodeExportProgressDialog::onProgressChange, this, std::placeholders::_1));
    }
}


void GcodeExportProgressDialog::InitUI()
{
    SetBackgroundColour(wxColour(44, 44, 45));
    wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vBox);


    // row 1
    wxBoxSizer* hBox1 = new wxBoxSizer(wxHORIZONTAL);

    m_exportingLabel = new wxStaticText(this, wxID_ANY, _L("common_preview_exporting_title"));    // "Exporting..."
    m_exportingLabel->SetBackgroundColour(wxColour("#292A2D"));
    m_exportingLabel->SetForegroundColour(wxColour(255, 255, 255));
    m_exportingLabel->SetFont(ANKER_BOLD_FONT_NO_1);
    hBox1->Add(m_exportingLabel, 1, wxEXPAND | wxALL, 15);

    m_stopExportBtn = new wxBitmapButton(this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_stopExportBtn->SetBackgroundColour(wxColour(44, 44, 45));
    wxImage image(wxString::FromUTF8(Slic3r::var("stopExportingGcode.png")), wxBITMAP_TYPE_PNG);
    if (image.IsOk())
    {
        wxBitmap bitmap(image);
        m_stopExportBtn->SetBitmap(bitmap);
    }
    m_stopExportBtn->SetSize(wxSize(25, 25));
    m_stopExportBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        // std::cout << "-->stop exporting" ;
        Plater* plater = Slic3r::GUI::wxGetApp().plater();
        if (plater)
        {
            plater->stop_exporting_Gcode();
        }
        this->Hide();
        }
    );
    hBox1->Add(m_stopExportBtn, 0, wxALIGN_RIGHT |/* wxSHAPED |*/ wxALL, 15);

    vBox->Add(hBox1, 1, wxEXPAND);

    // row 2
    wxBoxSizer* hBox2 = new wxBoxSizer(wxHORIZONTAL);

    m_exportProgressBar = new wxGauge(this, wxID_ANY, 100);
    m_exportProgressBar->SetMinSize(wxSize(-1, 5));
    m_exportProgressBar->SetBackgroundColour(wxColour(60, 60, 60));
    hBox2->Add(m_exportProgressBar, 1, wxALIGN_CENTER_VERTICAL | wxALL, 15);

    m_exportProgressText = new wxStaticText(this, wxID_ANY, ("100%"));
    m_exportProgressText->SetBackgroundColour(wxColour("#292A2D"));
    m_exportProgressText->SetForegroundColour(wxColour(255, 255, 255));
    m_exportProgressText->SetFont(ANKER_BOLD_FONT_NO_1);
    hBox2->Add(m_exportProgressText, 0, wxALIGN_CENTER_VERTICAL/* | wxALIGN_RIGHT*/ | wxALL, 15);

    vBox->Add(hBox2, 1, wxEXPAND);

    this->SetSizerAndFit(vBox);
}

void GcodeExportProgressDialog::onProgressChange(float percentage)
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater)
    {
        // set this dialog at postion at right top of canvas
        int marginToTop = 12;
        int marginToRiht = 6;
        Size canvasSize = plater->canvas_preview()->get_canvas_size();
        wxPoint screenPos = plater->ClientToScreen(wxPoint(canvasSize.get_width()- marginToRiht - this->GetSize().GetWidth(), marginToTop));
        this->SetPosition(screenPos);
    }

    this->Show();
    if (m_exportProgressText) {
        wxString str = wxString::Format("%d", (int)(percentage * 100)) + "%";
        m_exportProgressText->SetLabelText(str);
        m_exportProgressBar->SetRange(100);

        m_exportProgressBar->SetValue((int)(percentage * 100));
        if (percentage >= 1.0f) {
            this->Hide();
            ANKER_LOG_INFO << "gcode export progress: 100%";
        }

        Layout();
        Refresh();
        Update();
    }
}



void AnkerGcodeInfo::InitGUI()
{
    wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vBox);
    // title
    {
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);

        //titleSizer->AddSpacer(3);
        wxStaticText* titleBanner = new wxStaticText(this, wxID_ANY, _L("common_preview_gcodeinfo_title")); // "Gcode Informations"
        titleBanner->SetBackgroundColour(wxColour("#292A2D"));
        titleBanner->SetForegroundColour(wxColour(255, 255, 255));
        titleBanner->SetCanFocus(false);
        //wxFont Font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"));
        titleBanner->SetFont(/*Font*/ANKER_BOLD_FONT_NO_1);

        titleSizer->Add(titleBanner, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 0);
        vBox->Add(titleSizer, 0, wxEXPAND | wxALL, 10);
    }
    {
        // Line
        wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
        splitLineCtrl->SetMaxSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        splitLineCtrl->SetMinSize(AnkerSize(SIDEBARNEW_WIDGET_WIDTH, 1));
        vBox->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);
    }

    //vBox->AddSpacer(10);

    // Size
    {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        rowSizer->AddSpacer(10);

        // Icon
        BlinkingBitmap* Bitmap = nullptr;
        std::string IconName = "gcode_size";
        if (try_get_bmp_bundle(IconName)) {
            Bitmap = new BlinkingBitmap(this, IconName);
        }
        else {
            Bitmap = new BlinkingBitmap(this);
        }
        Bitmap->SetMinSize(AnkerSize(20, 20));
        Bitmap->SetMaxSize(AnkerSize(20, 20));
        Bitmap->activate();
        rowSizer->Add(Bitmap, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddSpacer(7);

        // Parameter Name
        wxStaticText* paramNameText = new wxStaticText(this, wxID_ANY, _L("common_preview_gcodeinfo_size")); // "Size"
        paramNameText->SetForegroundColour("#FFFFFF");
        paramNameText->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(paramNameText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddStretchSpacer(1);
        //rowSizer->AddSpacer(7);

        m_sizeValueLabel = new wxStaticText(this, wxID_ANY, "--"/*, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT*/);
        m_sizeValueLabel->SetForegroundColour("#AAAAAA");
        m_sizeValueLabel->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(m_sizeValueLabel, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 10);
        rowSizer->AddSpacer(10);

        vBox->Add(rowSizer, 0, wxEXPAND | wxALL, 0);
    }

    // Filament
    {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        rowSizer->AddSpacer(10);

        BlinkingBitmap* Bitmap = nullptr;
        std::string IconName = "gcode_filament";
        if (try_get_bmp_bundle(IconName)) {
            Bitmap = new BlinkingBitmap(this, IconName);
        }
        else {
            Bitmap = new BlinkingBitmap(this);
        }
        Bitmap->SetMinSize(AnkerSize(20, 20));
        Bitmap->SetMaxSize(AnkerSize(20, 20));
        Bitmap->activate();
        rowSizer->Add(Bitmap, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddSpacer(7);

        wxStaticText* paramNameText = new wxStaticText(this, wxID_ANY, _L("common_preview_gcodeinfo_filament")); // "Filament"
        paramNameText->SetForegroundColour("#FFFFFF");
        paramNameText->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(paramNameText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddStretchSpacer(1);
        //rowSizer->AddSpacer(7);

        m_filametValueLabel = new wxStaticText(this, wxID_ANY, "--"/*, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT*/);
        m_filametValueLabel->SetForegroundColour("#AAAAAA");
        m_filametValueLabel->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(m_filametValueLabel, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 10);
        rowSizer->AddSpacer(10);

        vBox->Add(rowSizer, 0, wxEXPAND | wxALL, 0);
    }

    // Print Time
    {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        rowSizer->AddSpacer(10);

        BlinkingBitmap* Bitmap = nullptr;
        std::string IconName = "gcode_time";
        if (try_get_bmp_bundle(IconName)) {
            Bitmap = new BlinkingBitmap(this, IconName);
        }
        else {
            Bitmap = new BlinkingBitmap(this);
        }
        Bitmap->SetMinSize(AnkerSize(20, 20));
        Bitmap->SetMaxSize(AnkerSize(20, 20));
        Bitmap->activate();
        rowSizer->Add(Bitmap, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        rowSizer->AddSpacer(7);

        wxStaticText* paramNameText = new wxStaticText(this, wxID_ANY, _L("common_preview_gcodeinfo_time"));   // "Print Time"
        paramNameText->SetForegroundColour("#FFFFFF");
        paramNameText->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(paramNameText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddStretchSpacer(1);
        //rowSizer->AddSpacer(7);

        m_printTimeValLabel = new wxStaticText(this, wxID_ANY, "--"/*, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT*/);
        m_printTimeValLabel->SetForegroundColour("#AAAAAA");
        m_printTimeValLabel->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(m_printTimeValLabel, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 10);
        rowSizer->AddSpacer(10);

        vBox->Add(rowSizer, 0, wxEXPAND | wxALL, 0);
    }

    wxPoint textTPos;
    wxSize textSize;
    // Create AI File
    {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        rowSizer->AddSpacer(10);

        // Icon
        BlinkingBitmap* Bitmap = nullptr;
        std::string IconName = "create_AI_file";
        if (try_get_bmp_bundle(IconName)) {
            Bitmap = new BlinkingBitmap(this, IconName);
        }
        else {
            Bitmap = new BlinkingBitmap(this);
        }
        Bitmap->activate();
        rowSizer->Add(Bitmap, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

        rowSizer->AddSpacer(7);

        // Parameter Name
        wxStaticText* paramNameText = new wxStaticText(this, wxID_ANY, _L("common_preview_gcodeinfo_ai"));   // "Create AI File"
        paramNameText->SetForegroundColour("#FFFFFF");
        paramNameText->SetFont(ANKER_BOLD_FONT_NO_1);
        rowSizer->Add(paramNameText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
        rowSizer->AddStretchSpacer(1);
        //rowSizer->AddSpacer(7);

        wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
        uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap uncheckScaledBitmap(uncheckImage);

        wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check_style_gray.png")), wxBITMAP_TYPE_PNG);
        checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
        wxBitmap checkScaledBitmap(checkImage);

		wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
		disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disUncheckScaledBitmap(disuncheckImage);

		wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
		discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
		wxBitmap disCheckScaledBitmap(discheckImage);

        m_createAIFilecheckbox = new AnkerCheckBox(this,
                                                    uncheckScaledBitmap.ConvertToImage(),
                                                    checkScaledBitmap.ConvertToImage(),
			                                        disUncheckScaledBitmap.ConvertToImage(),
			                                        disCheckScaledBitmap.ConvertToImage(),
			                                        wxString(""),
                                                    ANKER_BOLD_FONT_NO_1,
			                                        wxColour("#FFFFFF"),
                                                    wxID_ANY);
        m_createAIFilecheckbox->SetWindowStyleFlag(wxBORDER_NONE);
        m_createAIFilecheckbox->SetBackgroundColour(wxColour("#292A2D"));
        m_createAIFilecheckbox->SetMinSize(wxSize(16, 16));
        m_createAIFilecheckbox->SetMaxSize(wxSize(16, 16));
        m_createAIFilecheckbox->SetSize(wxSize(16, 16));
        m_createAIFilecheckbox->Enable(true);
       // m_createAIFilecheckbox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, &AnkerGCodeExtrusionRoleSelectPanel::OnCreateAIFile, this);
        m_createAIFilecheckbox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, [this](wxCommandEvent& event) {
            if (this->m_createAIFilecheckbox) {
                bool ischeck = this->m_createAIFilecheckbox->getCheckStatus();
                Plater * plater = Slic3r::GUI::wxGetApp().plater();
                if (plater)
                {
                    plater->set_create_AI_file_val(ischeck);
                }
            }
            }
        );

        Plater* plater = Slic3r::GUI::wxGetApp().plater();
        if (plater)
        {
            bool val = plater->get_create_AI_file_val();
            m_createAIFilecheckbox->setCheckStatus(val);
        }

        rowSizer->Add(m_createAIFilecheckbox, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 10);
        rowSizer->AddSpacer(10);

        vBox->Add(rowSizer, 0, wxEXPAND | wxALL, 0);
        AISizer = rowSizer;

        textTPos = paramNameText->GetPosition();
        textSize = paramNameText->GetTextExtent(_L("common_preview_gcodeinfo_ai")); // "Create AI File"
    }

    // AI file not suport 
    {
        // The current Gcode file does not support generating AI
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 34));
        rowSizer->AddSpacer(30);

        m_AIFileNotSuportLabel = new wxStaticText(this, wxID_ANY, _L("The current Gcode file does not support generating AI"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxTE_MULTILINE | wxTE_WORDWRAP);
        m_AIFileNotSuportLabel->SetMinSize(wxSize(260, 50));
        m_AIFileNotSuportLabel->SetForegroundColour(wxColour("#777777"));
        m_AIFileNotSuportLabel->SetFont(ANKER_BOLD_FONT_NO_2);
        m_AIFileNotSuportLabel->Wrap(260);
       // m_AIFileNotSuportLabel->SetPosition(wxPoint(textTPos.x, textTPos.y+ textSize.GetHeight()+5));
        m_AIFileNotSuportLabel->Hide();
        rowSizer->Add(m_AIFileNotSuportLabel, 0, wxALIGN_LEFT, 0);

        rowSizer->AddStretchSpacer(1);

        vBox->Add(rowSizer, 0, wxEXPAND | wxALL, 0);
    }

    vBox->AddStretchSpacer(1);

    wxBoxSizer* btnsSizer = new wxBoxSizer(wxHORIZONTAL);
    {
        btnsSizer->AddSpacer(10);
        // slice btn
        {
            m_sliceBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            //m_sliceBtn->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 24));
            m_sliceBtn->SetMinSize(wxSize(-1, 30));
            m_sliceBtn->SetMaxSize(wxSize(-1, 30));
            m_sliceBtn->SetSize(wxSize(-1, 30));
            m_sliceBtn->SetText(_L("common_slicepannel_button_slice"));   // "Slice Now"
            m_sliceBtn->SetDisableTextColor(wxColour(105, 105, 108));
            m_sliceBtn->SetBackgroundColour("#71d35a");
            m_sliceBtn->SetTextColor("#FFFFFF");
            m_sliceBtn->SetRadius(5);
            m_sliceBtn->SetFont(ANKER_BOLD_FONT_NO_1);
            m_sliceBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
                this->m_sliceBtn->Enable(false);
                //this->Refresh();
                //this->Update();
                wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SLICE_BTN_CLICKED);
                evt.SetEventObject(this);
                ProcessEvent(evt);
                });
            m_sliceBtn->Hide();
            btnsSizer->Add(m_sliceBtn, 1, wxEXPAND | wxRIGHT, 10);
        }

        // Export btn
        {
            m_exportBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            //m_exportBtn->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 24));
            m_exportBtn->SetMinSize(wxSize(-1, 30));
            m_exportBtn->SetMaxSize(wxSize(-1, 30));
            m_exportBtn->SetSize(wxSize(-1, 30));
            m_exportBtn->SetText(_L("common_preview_button_export")); // "Export"
            m_exportBtn->SetDisableTextColor(wxColour(105, 105, 108));
            m_exportBtn->SetBackgroundColour("#3a3b3f");
            m_exportBtn->SetTextColor("#FFFFFF");
            m_exportBtn->SetRadius(5);
            m_exportBtn->SetFont(ANKER_BOLD_FONT_NO_1);
            m_exportBtn->Bind(wxEVT_BUTTON, &AnkerGcodeInfo::OnExportBtnClick, this);
            btnsSizer->Add(m_exportBtn, 1, wxEXPAND | wxRIGHT, 10);
        }

        // Go Print btn
        {
            m_goPrintBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            //m_goPrintBtn->SetMinSize(wxSize(SIDEBARNEW_WIDGET_WIDTH, 24));
            m_goPrintBtn->SetMinSize(wxSize(-1, 30));
            m_goPrintBtn->SetMaxSize(wxSize(-1, 30));
            m_goPrintBtn->SetSize(wxSize(-1, 30));
            m_goPrintBtn->SetText(_L("common_preview_button_1print")); // "Go Print"
            m_goPrintBtn->SetDisableTextColor(wxColour(105, 105, 108));
            m_goPrintBtn->SetBackgroundColour("#62d361");
            m_goPrintBtn->SetTextColor("#FFFFFF");
            m_goPrintBtn->SetRadius(5);
            m_goPrintBtn->SetFont(ANKER_BOLD_FONT_NO_1);
            m_goPrintBtn->Bind(wxEVT_BUTTON, &AnkerGcodeInfo::OnGoPrintBtnClick, this);
            btnsSizer->Add(m_goPrintBtn, 1, wxEXPAND  | wxRIGHT, 10);
        }
    }
    vBox->Add(btnsSizer, 1, wxEXPAND);

    vBox->AddSpacer(10);
}


void AnkerGcodeInfo::UpdateSlicedInfo(bool GcodeValid)
{
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (!plater)
    {
        ANKER_LOG_ERROR << "plater is null";
        return;
    }

#if 0
    // check printer type to determing whether to show AI checkbox (have camera)
    if (AISizer) {
        const Preset& current_preset = wxGetApp().preset_bundle->printers.get_edited_preset();
        std::string printModel = current_preset.config.opt_string("printer_model");

        if (printModel == "M5" || printModel == "m5") {
            AISizer->Show(true);
        }
        else {
            AISizer->Show(false);
        }
    }
#else
    AISizer->Show(false);
#endif

    bool previewLoad = wxGetApp().plater()->is_preview_loaded();
    bool is_Gcode_Added_PauseCmd = false;
    if (!GcodeValid && previewLoad) {
        is_Gcode_Added_PauseCmd = true;
    }

    std::string tmpGcodePath = plater->get_temp_gcode_output_path();
    if (plater->isImportGCode()) {
        // the gcode is drag into the soft
        tmpGcodePath = plater->getAKeyPrintSlicerTempGcodePath();
    }

    if (tmpGcodePath.empty() || GcodeValid == false)
    {
        ANKER_LOG_INFO << "tmpGcodePath empty, reset gcode info";
        m_sizeValueLabel->SetLabelText("--");
        m_filametValueLabel->SetLabelText("--");
        m_printTimeValLabel->SetLabelText("--");
        m_goPrintBtn->SetBackgroundColour("#3a3b3f");
        m_goPrintBtn->SetTextColor("#777777");
        m_goPrintBtn->Enable(false);
        m_sliceBtn->SetBackgroundColour("#3a3b3f");
        m_sliceBtn->SetTextColor("#777777");
        m_sliceBtn->Enable(false);
        m_exportBtn->SetTextColor("#777777");
        m_exportBtn->Enable(false);

        if (is_Gcode_Added_PauseCmd)
        {
            ANKER_LOG_INFO << "update by add pause command";
            m_goPrintBtn->Hide();
            m_sliceBtn->SetBackgroundColour("#71d35a");
            m_sliceBtn->SetTextColor("#FFFFFF");
            m_sliceBtn->Enable(true);
            m_sliceBtn->Show();
        }
    }
    else
    {
        // fix:621 GCodeProcessor can parse .gcode file only
        ANKER_LOG_INFO << "tmp GcodePath:"<< tmpGcodePath;
        std::string tmpGcodeExtensionFile;
        {
            auto isAcodeFile = [](const std::string& filename) {
                std::string extension = boost::filesystem::extension(filename);
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                return (extension == ".acode");
            };
            
            if (!wxGetApp().plater()->isImportGCode() && isAcodeFile(tmpGcodePath)) {
                boost::filesystem::path srcFile(tmpGcodePath);

                tmpGcodeExtensionFile = tmpGcodePath + ".gcode";
                boost::filesystem::path targetFile(tmpGcodeExtensionFile);

                boost::filesystem::copy_file(srcFile, targetFile);

                tmpGcodePath = tmpGcodeExtensionFile;

                ANKER_LOG_INFO << " tmp GcodePath have .acode extetion, rename it:" << tmpGcodePath;
            }
        }

        wxString tmp_GcodePath;
        tmp_GcodePath = tmpGcodePath.c_str();

        if (boost::filesystem::exists(boost::filesystem::path(tmp_GcodePath.ToUTF8().data()))) {
            Slic3r::GCodeProcessor processor;
            Slic3r::GCodeProcessorResultExt out;
            // we still open the file which filepath may contains special characters
            processor.process_file_ext(tmp_GcodePath.ToUTF8().data(), out);
            //m_sizeValueLabel->SetLabelText(plater->getModelObjectSizeText());
            m_sizeValueLabel->SetLabelText(plater->getScaledModelObjectSizeText());
            std::string filament = out.filament_cost;
            m_filametValueLabel->SetLabelText(filament.empty() ? "--" : filament);
            m_printTimeValLabel->SetLabelText(getReadableTime(out.print_time));
            m_goPrintBtn->Show();
            m_sliceBtn->Hide();
            m_goPrintBtn->SetBackgroundColour("#71d35a");
            m_goPrintBtn->SetTextColor("#FFFFFF");
            m_goPrintBtn->Enable(true);
            m_exportBtn->SetTextColor("#FFFFFF");
            m_exportBtn->Enable(true);

            if (!tmpGcodeExtensionFile.empty()) {
                boost::filesystem::remove(tmpGcodeExtensionFile);
            }
        }
        else
        {
            ANKER_LOG_ERROR << " tmp_GcodePath not exist:" << tmp_GcodePath;
        }
    }

    Layout();
    Refresh();
}

std::string AnkerGcodeInfo::getReadableTime(int seconds)
{
    int hours = seconds / 60 / 60;
    int minutes = seconds / 60 % 60;
    std::string newTime = "";
    if (hours > 0)
        newTime += std::to_string(hours) + "h ";
    if (minutes > 0)
        newTime += std::to_string(minutes) + "min";
    if (newTime.empty())
        newTime = "--";

    return newTime;
}


void AnkerGcodeInfo::OnGoPrintBtnClick(wxCommandEvent& event)
{ 
    if (m_exportingGcode)
        return;

    ANKER_LOG_INFO << "print btn click===in";
    m_exportingGcode = true;
    Slic3r::GUI::Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater)
    {
        ANKER_LOG_INFO << "print btn click" ;
        plater->a_key_print_clicked();
    }
    m_exportingGcode = false;
    ANKER_LOG_INFO << "print btn click===out";
}

void AnkerGcodeInfo::OnExportBtnClick(wxCommandEvent& event)
{
    if (m_exportingGcode)
        return;
    ANKER_LOG_INFO << "export btn click===in";
    m_exportingGcode = true;
    if (!m_exportProgressDlg) {
        m_exportProgressDlg = new GcodeExportProgressDialog(this);
        m_exportProgressDlg->SetSize(wxSize(450, 110));
    }

    Slic3r::GUI::Plater* plater = Slic3r::GUI::wxGetApp().plater();
    if (plater)
    {
        plater->export_gcode(false);
    }
    m_exportingGcode = false;
    ANKER_LOG_INFO << "export btn click===out";
}


void AnkerGcodePreviewSideBar::InitGUI()
{
    wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vBox);

    // add top margin sizer
    vBox->AddSpacer(12);

    m_GcodeViewPanel = new AnkerGcodeViewPanel(this);
    m_GcodeViewPanel->SetBackgroundColour(wxColour("#292A2D"));
    vBox->Add(m_GcodeViewPanel, 0, wxEXPAND | wxLEFT, 5);

    vBox->AddSpacer(8);

    m_gCodeExtrusionRoleSelectPanel = new AnkerGCodeExtrusionRoleSelectPanel(this);
    m_gCodeExtrusionRoleSelectPanel->SetBackgroundColour(wxColour("#292A2D"));
   // m_gCodeExtrusionRoleSelectPanel->SetMinSize(wxSize(-1, 300));
   // m_gCodeExtrusionRoleSelectPanel->SetMaxSize(wxSize(310, 500));
    vBox->Add(m_gCodeExtrusionRoleSelectPanel, 1, wxEXPAND | wxLEFT, 5);

    vBox->AddSpacer(8);

    m_GcodeInfoPanel = new AnkerGcodeInfo(this);
    m_GcodeInfoPanel->SetBackgroundColour(wxColour("#292A2D"));

    /*m_GcodeInfoPanel->SetMinSize(wxSize(300, -1));
    m_GcodeInfoPanel->SetSize(wxSize(300, -1));*/

    vBox->Add(m_GcodeInfoPanel, 0, wxEXPAND | wxLEFT, 5);

    vBox->AddSpacer(2);
}

void AnkerGcodePreviewSideBar::UpdateCurrentViewType(GCodeViewer::EViewType type)
{
    if(m_GcodeViewPanel)
        m_GcodeViewPanel->UpdateCurrentChoice(type);
}

void AnkerGcodePreviewSideBar::UpdateGcodePreviewSideBar(bool GcodeValid)
{
    if (m_GcodeViewPanel) {
        m_GcodeViewPanel->UpdateViewGcodeLegendBtn();
        m_GcodeViewPanel->UpdateCurrentChoice();
    }

    if (m_gCodeExtrusionRoleSelectPanel) {
        m_gCodeExtrusionRoleSelectPanel->UpdateAllGCodeExtrusionRoleUI();
    }

    if (m_GcodeInfoPanel)
    {
        m_GcodeInfoPanel->UpdateSlicedInfo(GcodeValid);
    }
    Layout();
    Refresh();
}


#include "AboutDialog.hpp"
#include "I18N.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Color.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "format.hpp"

#include <wx/clipbrd.h>

namespace Slic3r { 
namespace GUI {

AboutDialogLogo::AboutDialogLogo(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    this->SetBackgroundColour(*wxWHITE);
    this->logo = wxBitmap(from_u8(Slic3r::var("AnkerStudio_192px.png")), wxBITMAP_TYPE_PNG);
    this->SetMinSize(this->logo.GetSize());
    
    this->Bind(wxEVT_PAINT, &AboutDialogLogo::onRepaint, this);
}

void AboutDialogLogo::onRepaint(wxEvent &event)
{
    wxPaintDC dc(this);
    dc.SetBackgroundMode(wxTRANSPARENT);

    wxSize size = this->GetSize();
    int logo_w = this->logo.GetWidth();
    int logo_h = this->logo.GetHeight();
    dc.DrawBitmap(this->logo, (size.GetWidth() - logo_w)/2, (size.GetHeight() - logo_h)/2, true);

    event.Skip();
}


// -----------------------------------------
// CopyrightsDialog
// -----------------------------------------
CopyrightsDialog::CopyrightsDialog()
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY, format_wxstr("%1% - %2%"
        , wxGetApp().is_editor() ? SLIC3R_APP_NAME : GCODEVIEWER_APP_NAME
        , _L("Portions copyright")),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    this->SetFont(wxGetApp().normal_font());
#ifdef _WIN32
    wxGetApp().UpdateDarkUI(this);
#else
	this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif

	auto sizer = new wxBoxSizer(wxVERTICAL);
    
    fill_entries();

    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, 
                              wxSize(40 * em_unit(), 20 * em_unit()), wxHW_SCROLLBAR_AUTO);

    wxFont font = get_default_font(this);
    const int fs = font.GetPointSize();
    const int fs2 = static_cast<int>(1.2f*fs);
    int size[] = { fs, fs, fs, fs, fs2, fs2, fs2 };

    m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
    m_html->SetBorders(2);        
    m_html->SetPage(get_html_text());

    sizer->Add(m_html, 1, wxEXPAND | wxALL, 15);
    m_html->Bind(wxEVT_HTML_LINK_CLICKED, &CopyrightsDialog::onLinkClicked, this);

    wxStdDialogButtonSizer* buttons = this->CreateStdDialogButtonSizer(wxCLOSE);
    wxGetApp().UpdateDlgDarkUI(this, true);
    this->SetEscapeId(wxID_CLOSE);
    this->Bind(wxEVT_BUTTON, &CopyrightsDialog::onCloseDialog, this, wxID_CLOSE);
    sizer->Add(buttons, 0, wxEXPAND | wxRIGHT | wxBOTTOM, 3);

    SetSizer(sizer);
    sizer->SetSizeHints(this);
    
}

void CopyrightsDialog::fill_entries()
{
    m_entries = {
        { "wxWidgets"       , "2019 wxWidgets"                              , "https://www.wxwidgets.org/" },
        { "OpenGL"          , "1997-2019 The Khronos™ Group Inc"            , "https://www.opengl.org/" },
        { "GNU gettext"     , "1998, 2019 Free Software Foundation, Inc."   , "https://www.gnu.org/software/gettext/" },
        { "PoEdit"          , "2019 Václav Slavík"                          , "https://poedit.net/" },
        { "ImGUI"           , "2014-2019 Omar Cornut"                       , "https://github.com/ocornut/imgui" },
        { "Eigen"           , ""                                            , "http://eigen.tuxfamily.org" },
        { "ADMesh"          , "1995, 1996  Anthony D. Martin; "
                              "2015, ADMesh contributors"                   , "https://admesh.readthedocs.io/en/latest/" },
        { "Anti-Grain Geometry"
                            , "2002-2005 Maxim Shemanarev (McSeem)"         , "http://antigrain.com" },
        { "Boost"           , "1998-2005 Beman Dawes, David Abrahams; "
                              "2004 - 2007 Rene Rivera"                     , "https://www.boost.org/" },
        { "Clipper"         , "2010-2015 Angus Johnson "                    , "http://www.angusj.com " },
        { "GLEW (The OpenGL Extension Wrangler Library)", 
                              "2002 - 2007, Milan Ikits; "
                              "2002 - 2007, Marcelo E.Magallon; "
                              "2002, Lev Povalahev"                         , "http://glew.sourceforge.net/" },
        { "Libigl"          , "2013 Alec Jacobson and others"               , "https://libigl.github.io/" },
        { "Qhull"           , "1993-2015 C.B.Barber Arlington and "
                              "University of Minnesota"                     , "http://qhull.org/" },
        { "SemVer"          , "2015-2017 Tomas Aparicio"                    , "https://semver.org/" },
        { "Nanosvg"         , "2013-14 Mikko Mononen"                       , "https://github.com/memononen/nanosvg" },
        { "Miniz"           , "2013-2014 RAD Game Tools and Valve Software; "
                              "2010-2014 Rich Geldreich and Tenacious Software LLC"
                                                                            , "https://github.com/richgel999/miniz" },
        { "Expat"           , "1998-2000 Thai Open Source Software Center Ltd and Clark Cooper"
                              "2001-2016 Expat maintainers"                 , "http://www.libexpat.org/" },
        { "AVRDUDE"         , "2018  Free Software Foundation, Inc."        , "http://savannah.nongnu.org/projects/avrdude" },
        { "Real-Time DXT1/DXT5 C compression library"   
                                    , "Based on original by fabian \"ryg\" giesen v1.04. "
                              "Custom version, modified by Yann Collet"     , "https://github.com/Cyan4973/RygsDXTc" },
        { "Icons for STL and GCODE files."
                            , "Akira Yasuda"                                , "http://3dp0.com/icons-for-stl-and-gcode/" },
        { "AppImage packaging for Linux using AppImageKit"
                            , "2004-2019 Simon Peter and contributors"      , "https://appimage.org/" },
        { "lib_fts"
                            , "Forrest Smith"                               , "https://www.forrestthewoods.com/" },
        { "fast_float"
                            , "Daniel Lemire, João Paulo Magalhaes and contributors", "https://github.com/fastfloat/fast_float" },
        { "CuraEngine (Arachne, etc.)"
                            , "Ultimaker", "https://github.com/Ultimaker/CuraEngine" },
        { "Open CASCADE Technology"
                            , "Open Cascade SAS", "https://github.com/Open-Cascade-SAS/OCCT" }
    };
}

wxString CopyrightsDialog::get_html_text()
{
    wxColour bgr_clr = wxGetApp().get_window_default_clr();//wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

    const auto text_clr = wxGetApp().get_label_clr_default();
    const auto text_clr_str = encode_color(ColorRGB(text_clr.Red(), text_clr.Green(), text_clr.Blue()));
    const auto bgr_clr_str = encode_color(ColorRGB(bgr_clr.Red(), bgr_clr.Green(), bgr_clr.Blue()));

    const wxString copyright_str = _L("Copyright") + "&copy; ";
    const wxString header_str = _L("License agreements of all following programs (libraries) are part of application license agreement");

    wxString text = wxString::Format(
        "<html>"
            "<body bgcolor= %s link= %s>"
            "<font color=%s>"
                "<font size=\"5\">%s.</font>"
                "<br /><br />"
                "<font size=\"3\">"
        , bgr_clr_str, text_clr_str
        , text_clr_str
        , header_str);

    for (const auto& entry : m_entries) {
        text += wxString::Format(
                    "<a href=\"%s\">%s</a><br/>"
                    , entry.link, entry.lib_name);

        if (!entry.copyright.empty())
            text += format_wxstr(
                    "%1% %2%<br/><br/>"
                    , copyright_str, entry.copyright);
    }

    text += wxString(
                "</font>"
            "</font>"
            "</body>"
        "</html>");

    return text;
}

void CopyrightsDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    const wxFont& font = GetFont();
    const int fs = font.GetPointSize();
    const int fs2 = static_cast<int>(1.2f*fs);
    int font_size[] = { fs, fs, fs, fs, fs2, fs2, fs2 };

    m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), font_size);

    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_CLOSE });

    const wxSize& size = wxSize(40 * em, 20 * em);

    m_html->SetMinSize(size);
    m_html->Refresh();

    SetMinSize(size);
    Fit();

    Refresh();
}

void CopyrightsDialog::onLinkClicked(wxHtmlLinkEvent &event)
{
    wxGetApp().open_browser_with_warning_dialog(event.GetLinkInfo().GetHref());
    event.Skip(false);
}

void CopyrightsDialog::onCloseDialog(wxEvent &)
{
     this->EndModal(wxID_CLOSE);
}

AboutDialog::AboutDialog()
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY, format_wxstr(_L("About %s"), wxGetApp().is_editor() ? SLIC3R_APP_NAME : GCODEVIEWER_APP_NAME), wxDefaultPosition,
        wxDefaultSize, /*wxCAPTION*/wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetFont(wxGetApp().normal_font());

    wxColour bgr_clr = wxGetApp().get_window_default_clr();//wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	SetBackgroundColour(bgr_clr);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

	auto main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(hsizer, 0, wxEXPAND | wxALL, 20);

    // logo
    m_logo = new wxStaticBitmap(this, wxID_ANY, *get_bmp_bundle(wxGetApp().logo_name(), 192));
	hsizer->Add(m_logo, 1, wxALIGN_CENTER_VERTICAL);
    
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL); 	
    hsizer->Add(vsizer, 2, wxEXPAND|wxLEFT, 20);

    // title
    {
        wxStaticText* title = new wxStaticText(this, wxID_ANY, wxGetApp().is_editor() ? SLIC3R_APP_NAME : GCODEVIEWER_APP_NAME, wxDefaultPosition, wxDefaultSize);
        wxFont title_font = GUI::wxGetApp().bold_font();
        title_font.SetFamily(wxFONTFAMILY_ROMAN);
        title_font.SetPointSize(24);
        title->SetFont(title_font);
        vsizer->Add(title, 0, wxALIGN_LEFT | wxTOP, 10);
    }
    
    // version
    {
        auto version_string = _L("Version") + " " + std::string(SLIC3R_VERSION);
        wxStaticText* version = new wxStaticText(this, wxID_ANY, version_string.c_str(), wxDefaultPosition, wxDefaultSize);
        wxFont version_font = GetFont();
        #ifdef __WXMSW__
        version_font.SetPointSize(version_font.GetPointSize()-1);
        #else
            version_font.SetPointSize(11);
        #endif
        version->SetFont(version_font);
        vsizer->Add(version, 0, wxALIGN_LEFT | wxBOTTOM, 10);
    }
    
    // text
    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO/*NEVER*/);
    {
        m_html->SetMinSize(wxSize(-1, 16 * wxGetApp().em_unit()));
        wxFont font = get_default_font(this);
        const auto text_clr = wxGetApp().get_label_clr_default();
        const auto text_clr_str = encode_color(ColorRGB(text_clr.Red(), text_clr.Green(), text_clr.Blue()));
        const auto bgr_clr_str = encode_color(ColorRGB(bgr_clr.Red(), bgr_clr.Green(), bgr_clr.Blue()));

		const int fs = font.GetPointSize()-1;
        int size[] = {fs,fs,fs,fs,fs,fs,fs};
        m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
        m_html->SetBorders(2);
        const wxString copyright_str    = _L("Copyright");
        // TRN AboutDialog: "Slic3r %1% GNU Affero General Public License"
        const wxString is_lecensed_str  = _L("is licensed under the");
        const wxString license_str      = _L("GNU Affero General Public License, version 3");
        const wxString based_on_str     = _L("eufyMake Studio is Based on PrusaSlicer by Alessandro Ranellucci and the RepRap community.");
        const wxString contributors_str = _L("Contributions by Henrik Brix Andersen, Nicolas Dandrimont, Mark Hindess, Petr Ledvina, Joseph Lenox, Y. Sapir, Mike Sheldrake, Vojtech Bubnik and numerous others.");
        const auto text = format_wxstr(
            "<html>"
            "<body bgcolor= %1% link= %2%>"
            "<font color=%3%>"
            "%4% &copy; 2016-2023 Anker Research. <br />"
            "%5% &copy; 2011-2018 Alessandro Ranellucci. <br />"
            "<a href=\"http://slic3r.org/\">Slic3r</a> %6% "
            "<a href=\"http://www.gnu.org/licenses/agpl-3.0.html\">%7%</a>."
            "<br /><br />"
            "%8%"
            "<br /><br />"
            "%9%"
            "</font>"
            "</body>"
            "</html>", bgr_clr_str, text_clr_str, text_clr_str
            , copyright_str, copyright_str
            , is_lecensed_str
            , license_str
            , based_on_str
            , contributors_str);
        m_html->SetPage(text);
        vsizer->Add(m_html, 1, wxEXPAND | wxBOTTOM, 10);
        m_html->Bind(wxEVT_HTML_LINK_CLICKED, &AboutDialog::onLinkClicked, this);
    }


    wxStdDialogButtonSizer* buttons = this->CreateStdDialogButtonSizer(wxCLOSE);

    m_copy_rights_btn_id = NewControlId();
    auto copy_rights_btn = new wxButton(this, m_copy_rights_btn_id, _L("Portions copyright")+dots);
    buttons->Insert(0, copy_rights_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    copy_rights_btn->Bind(wxEVT_BUTTON, &AboutDialog::onCopyrightBtn, this);

    m_copy_version_btn_id = NewControlId();
    auto copy_version_btn = new wxButton(this, m_copy_version_btn_id, _L("Copy Version Info"));
    buttons->Insert(1, copy_version_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    copy_version_btn->Bind(wxEVT_BUTTON, &AboutDialog::onCopyToClipboard, this);

    wxGetApp().UpdateDlgDarkUI(this, true);
    
    this->SetEscapeId(wxID_CLOSE);
    this->Bind(wxEVT_BUTTON, &AboutDialog::onCloseDialog, this, wxID_CLOSE);
    vsizer->Add(buttons, 0, wxEXPAND | wxRIGHT | wxBOTTOM, 3);

	SetSizer(main_sizer);
	main_sizer->SetSizeHints(this);
}

void AboutDialog::on_dpi_changed(const wxRect &suggested_rect)
{
//    m_logo_bitmap.msw_rescale();
//    m_logo->SetBitmap(m_logo_bitmap.bmp());

    const wxFont& font = GetFont();
    const int fs = font.GetPointSize() - 1;
    int font_size[] = { fs, fs, fs, fs, fs, fs, fs };
    m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), font_size);

    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_CLOSE, m_copy_rights_btn_id });

    m_html->SetMinSize(wxSize(-1, 16 * em));
    m_html->Refresh();

    const wxSize& size = wxSize(65 * em, 30 * em);

    SetMinSize(size);
    Fit();

    Refresh();
}

void AboutDialog::onLinkClicked(wxHtmlLinkEvent &event)
{
    wxGetApp().open_browser_with_warning_dialog(event.GetLinkInfo().GetHref());
    event.Skip(false);
}

void AboutDialog::onCloseDialog(wxEvent &)
{
    this->EndModal(wxID_CLOSE);
}

void AboutDialog::onCopyrightBtn(wxEvent &)
{
    CopyrightsDialog dlg;
    dlg.ShowModal();
}

void AboutDialog::onCopyToClipboard(wxEvent&)
{
    wxTheClipboard->Open();
    wxTheClipboard->SetData(new wxTextDataObject(_L("Version") + " " + std::string(SLIC3R_VERSION)));
    wxTheClipboard->Close();
}

} // namespace GUI
} // namespace Slic3r

#include "AnkerCopyrightDialog.hpp"

AnkerCopyrightDiaogPanel::AnkerCopyrightDiaogPanel(wxWindow* parent, const wxString& title, 
    const wxSize& size, int totalRow, int totalColumn) :
    AnkerDialogPanel(parent, title, size)
{
	wxColour bkcolour = wxColour("#333438");
	if (parent) {
		bkcolour = parent->GetBackgroundColour();
	}

	//m_mainSizer->AddSpacer(24);
	int listPosY = m_line->GetPosition().y + m_line->m_liseSize.y + 24;

	int textLeftIntal = 88;
	int textRightIntal = 87;
	
    int listLeftIntal = AnkerSize(12, 12).GetWidth();
	wxPoint listPos(listLeftIntal, listPosY);
	wxSize listSize = wxSize(size.x, size.y - listPosY - 17 - 21);
	m_list = new AnkerCopyrightListCtrl(this, wxID_ANY, listPos, listSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_NO_SORT_HEADER | wxLC_SINGLE_SEL | wxNO_BORDER);
    m_list->initLinks(totalRow, totalColumn);
    m_list->SetBackgroundColour(bkcolour);
    wxFont listFont = ANKER_FONT_NO_1;
    m_list->SetFont(listFont);

    wxBoxSizer* hListSizer = new wxBoxSizer(wxHORIZONTAL);
    hListSizer->AddSpacer(listLeftIntal);
	wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->AddSpacer(listLeftIntal);
	listSizer->Add(m_list, 0, wxEXPAND);
	m_list->SetWindowStyleFlag(m_list->GetWindowStyleFlag() & ~(wxLC_HRULES | wxLC_VRULES));
    hListSizer->Add(listSizer);

    m_mainSizer->Add(hListSizer, 0);

	wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL); 
    wxString copyrightText = _AnkerL("Copyright (C) 2024 Anker Innovations All Rights Reserved");
    int textIntal = 10;
    wxPoint textPos(textLeftIntal, m_list->GetPosition().y + m_list->GetSize().GetHeight() + textIntal);
    wxSize textSize = AnkerSize(400, 21);
	wxStaticText* text = new wxStaticText(this, wxID_ANY, copyrightText, textPos, textSize);
    wxFont tmpFont = ANKER_FONT_NO_2;
	text->SetFont(tmpFont);
	text->SetForegroundColour(wxColour(158, 159, 160));
	text->SetBackgroundColour(bkcolour);
    wxSize copyrightTextSize = getTextSize(text, copyrightText);
    wxPoint copyrightTextPos((size.GetWidth() - copyrightTextSize.GetWidth()) / 2, textPos.y);
    text->SetPosition(copyrightTextPos);
    text->SetSize(copyrightTextSize);
    textSizer->AddSpacer(copyrightTextPos.x + (size.GetWidth() - m_list->GetSize().GetWidth()) / 2);
    textSizer->Add(text);
	//textSizer->AddSpacer(size.GetWidth() - ((size.GetWidth() + copyrightTextSize.GetWidth()) / 2));

    m_mainSizer->AddSpacer(textIntal);
	m_mainSizer->Add(textSizer, 0);
	//m_mainSizer->AddSpacer(17);
}

void AnkerCopyrightDiaogPanel::insertColumnHeader(long column, const wxString& str)
{
	m_list->InsertColumn(column, str, wxLIST_FORMAT_LEFT);
}

void AnkerCopyrightDiaogPanel::setAutoColumnWidth(long column)
{
	m_list->SetColumnWidth(column, wxLIST_AUTOSIZE);
}

void AnkerCopyrightDiaogPanel::setColumnWidth(long column, int width)
{
	m_list->SetColumnWidth(column, width);
}

long AnkerCopyrightDiaogPanel::insertRowItemHeader(long row, const wxString& str)
{
	return m_list->InsertItem(row, str);
}


void AnkerCopyrightDiaogPanel::insertItem(long row, long column, const wxString& str)
{
    m_list->SetItem(row, column, str);     
}


void AnkerCopyrightDiaogPanel::setColumnItem(long column, AnkerCopyrightListCtrl& item)
{						
	
	m_list->SetColumn(column, *(dynamic_cast<wxListItem*>(&item)) );
}

void AnkerCopyrightDiaogPanel::setItem(AnkerCopyrightListCtrl& item)
{
	m_list->SetItem(*(dynamic_cast<wxListItem*>(&item)));
}

void AnkerCopyrightDiaogPanel::setHeaderAttr(const wxItemAttr& attr)
{
	m_list->SetHeaderAttr(attr);
}

void AnkerCopyrightDiaogPanel::setItemTextColour(long item, const wxColour& colour)
{
	m_list->SetItemTextColour(item, colour);
}

void AnkerCopyrightDiaogPanel::setLink(int row, int column, const std::string& label, const std::string& link)
{
    m_list->setLink(row, column, label, link);
}

AnkerCopyrightDialog::AnkerCopyrightDialog(wxWindow* parent, wxWindowID id, const wxString& title,
	const wxString& context, const wxPoint& pos, 
	const wxSize& size, long style, const wxString& name) :
	AnkerDialog(parent, id, title, context, pos, size, style, name)
{

}

AnkerCopyrightDialog::~AnkerCopyrightDialog()
{
}

void AnkerCopyrightDialog::InitDialogPanel(int dialogType)
{
	setBackgroundColour();
    AnkerCopyrightDiaogPanel *copyrightPanel = new AnkerCopyrightDiaogPanel(this, m_title, m_size);
    copyrightPanel->insertColumnHeader(0, _AnkerL("common_popup_copyright_name"));
    copyrightPanel->insertColumnHeader(1, _AnkerL("common_popup_copyright_license"));
    copyrightPanel->insertColumnHeader(2, _AnkerL("common_popup_copyright_modified"));
    copyrightPanel->setColumnWidth(0, 100);
    copyrightPanel->setColumnWidth(1, 100);
    copyrightPanel->setColumnWidth(2, 130);

    copyrightPanel->insertRowItemHeader(0, _AnkerL("common_popup_copyright_name"));
    copyrightPanel->insertItem(0, 1, _AnkerL("common_popup_copyright_license"));
    copyrightPanel->insertItem(0, 2, _AnkerL("common_popup_copyright_modified"));

    copyrightPanel->insertRowItemHeader(1, "PrusaSlicer");
    copyrightPanel->setLink(1, 0, "PrusaSlicer", "https://github.com/prusa3d/PrusaSlicer");
    copyrightPanel->insertItem(1, 1, "GPLV3");
    copyrightPanel->setLink(1, 1, "GPLV3", "https://github.com/prusa3d/PrusaSlicer/blob/master/LICENSE");
    copyrightPanel->insertItem(1, 2, _L("common_popup_copyright_modifiedyes"));
    
    copyrightPanel->insertRowItemHeader(2, "Blosc");
    copyrightPanel->setLink(2, 0, "Blosc", "https://github.com/Blosc/c-blosc");
    copyrightPanel->insertItem(2, 1, "BSD");
    copyrightPanel->setLink(2, 1, "BSD", "https://github.com/Blosc/c-blosc/tree/main/LICENSES");
    copyrightPanel->insertItem(2, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(3, "Boost");
    copyrightPanel->setLink(3, 0, "Boost", "https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.zip");
    copyrightPanel->insertItem(3, 1, "BSL");
    copyrightPanel->setLink(3, 1, "BSL", "https://www.boost.org/LICENSE_1_0.txt");
    copyrightPanel->insertItem(3, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(4, "CGAL");
    copyrightPanel->setLink(4, 0, "CGAL", "https://github.com/CGAL/cgal/");
    copyrightPanel->insertItem(4, 1, "LGPL");
    copyrightPanel->setLink(4, 1, "LGPL", "https://github.com/CGAL/cgal/blob/master/LICENSE.md");
    copyrightPanel->insertItem(4, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(5, "Curl");
    copyrightPanel->setLink(5, 0, "Curl", "https://github.com/curl/curl");
    copyrightPanel->insertItem(5, 1, "BSD");
    copyrightPanel->setLink(5, 1, "BSD", "https://github.com/curl/curl/tree/master/LICENSES");
    copyrightPanel->insertItem(5, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(6, "Cereal");
    copyrightPanel->setLink(6, 0, "Cereal", "https://github.com/USCiLab/cereal");
    copyrightPanel->insertItem(6, 1, "BSD");
    copyrightPanel->setLink(6, 1, "BSD", "https://github.com/USCiLab/cereal/blob/master/LICENSE");
    copyrightPanel->insertItem(6, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(7, "Expat");
    copyrightPanel->setLink(7, 0, "Expat", "https://github.com/libexpat/libexpat");
    copyrightPanel->insertItem(7, 1, "MIT");
    copyrightPanel->setLink(7, 1, "MIT", "https://www.debian.org/legal/licenses/mit");
    copyrightPanel->insertItem(7, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(8, "Glew");
    copyrightPanel->setLink(8, 0, "Glew", "https://sourceforge.net/projects/glew/");
    copyrightPanel->insertItem(8, 1, "BSD");
    copyrightPanel->setLink(8, 1, "BSD", "https://github.com/nigels-com/glew/blob/master/LICENSE.txt");
    copyrightPanel->insertItem(8, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(9, "GMP");
    copyrightPanel->setLink(9, 0, "GMP", "https://gmplib.org/download/gmp/gmp-6.2.1.tar.bz2");
    copyrightPanel->insertItem(9, 1, "GPLV3");
    copyrightPanel->setLink(9, 1, "GPLV3", "https://github.com/sethtroisi/libgmp/blob/master/README");
    copyrightPanel->insertItem(9, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(10, "JPEG");
    copyrightPanel->setLink(10, 0, "JPEG", "https://github.com/libjpeg-turbo/libjpeg-turbo/");
    copyrightPanel->insertItem(10, 1, "BSD");
    copyrightPanel->setLink(10, 1, "BSD", "https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/LICENSE.md");
    copyrightPanel->insertItem(10, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(11, "Mpfr");
    copyrightPanel->setLink(11, 0, "Mpfr", "http://ftp.vim.org/ftp/gnu/mpfr/mpfr-3.1.6.tar.bz2");
    copyrightPanel->insertItem(11, 1, "LGPL");
    copyrightPanel->setLink(11, 1, "LGPL", "https://www.gnu.org/licenses/lgpl-3.0.html");
    copyrightPanel->insertItem(11, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(12, "Nlopt");
    copyrightPanel->setLink(12, 0, "Nlopt", "https://github.com/stevengj/nlopt");
    copyrightPanel->insertItem(12, 1, "MIT");
    copyrightPanel->setLink(12, 1, "MIT", "https://nlopt.readthedocs.io/en/latest/NLopt_License_and_Copyright/");
    copyrightPanel->insertItem(12, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(13, "NanoSVG");
    copyrightPanel->setLink(13, 0, "NanoSVG", "https://github.com/fltk/nanosvg/");
    copyrightPanel->insertItem(13, 1, "MIT");
    copyrightPanel->setLink(13, 1, "MIT", "https://github.com/fltk/nanosvg/blob/fltk/LICENSE.txt");
    copyrightPanel->insertItem(13, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(14, "OCCT");
    copyrightPanel->setLink(14, 0, "OCCT", "https://github.com/Open-Cascade-SAS/OCCT");
    copyrightPanel->insertItem(14, 1, "LGPLV2.1");
    copyrightPanel->setLink(14, 1, "LGPLV2.1", "https://github.com/Open-Cascade-SAS/OCCT/blob/master/LICENSE_LGPL_21.txt");
    copyrightPanel->insertItem(14, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(15, "OpenCSG");
    copyrightPanel->setLink(15, 0, "OpenCSG", "https://github.com/floriankirsch/OpenCSG");
    copyrightPanel->insertItem(15, 1, "LGPL");
    copyrightPanel->setLink(15, 1, "LGPL", "https://github.com/floriankirsch/OpenCSG/blob/master/copying.txt");
    copyrightPanel->insertItem(15, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(16, "OpenEXR");
    copyrightPanel->setLink(16, 0, "OpenEXR", "https://github.com/AcademySoftwareFoundation/openexr");
    copyrightPanel->insertItem(16, 1, "Modified BSD");
    copyrightPanel->setLink(16, 1, "Modified BSD", "https://github.com/AcademySoftwareFoundation/openexr/blob/main/LICENSE.md");
    copyrightPanel->insertItem(16, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(17, "OpenSSL");
    copyrightPanel->setLink(17, 0, "OpenSSL", "https://github.com/openssl/openssl/");
    copyrightPanel->insertItem(17, 1, "AL2.0");
    copyrightPanel->setLink(17, 1, "AL2.0", "https://github.com/openssl/openssl/blob/master/LICENSE.txt");
    copyrightPanel->insertItem(17, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(18, "OpenVDB");
    copyrightPanel->setLink(18, 0, "OpenVDB", "https://github.com/tamasmeszaros/openvdb");
    copyrightPanel->insertItem(18, 1, "MPL-2.0");
    copyrightPanel->setLink(18, 1, "MPL-2.0", "https://github.com/tamasmeszaros/openvdb/blob/master/LICENSE");
    copyrightPanel->insertItem(18, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(19, "PNG");
    copyrightPanel->setLink(19, 0, "PNG", "https://github.com/glennrp/libpng");
    copyrightPanel->insertItem(19, 1, "PNGRLLV2");
    copyrightPanel->setLink(19, 1, "PNGRLLV2", "https://github.com/glennrp/libpng/blob/libpng16/LICENSE");
    copyrightPanel->insertItem(19, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(20, "Qhull");
    copyrightPanel->setLink(20, 0, "Qhull", "https://github.com/qhull/qhull");
    copyrightPanel->insertItem(20, 1, "Qhull License");
    copyrightPanel->setLink(20, 1, "Qhull License", "https://github.com/qhull/qhull/blob/master/COPYING.txt");
    copyrightPanel->insertItem(20, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(21, "TBB");
    copyrightPanel->setLink(21, 0, "TBB", "https://github.com/oneapi-src/oneTBB/");
    copyrightPanel->insertItem(21, 1, "AL2.0");
    copyrightPanel->setLink(21, 1, "Apache License 2.0", "https://github.com/oneapi-src/oneTBB/blob/master/LICENSE.txt");
    copyrightPanel->insertItem(21, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(22, "TIFF");
    copyrightPanel->setLink(22, 0, "TIFF", "https://gitlab.com/libtiff/libtiff/");
    copyrightPanel->insertItem(22, 1, "LibTIFF License");
    copyrightPanel->setLink(22, 1, "LibTIFF License", "https://gitlab.com/libtiff/libtiff/-/blob/master/LICENSE.md");
    copyrightPanel->insertItem(22, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(23, "Zlib");
    copyrightPanel->setLink(23, 0, "Zlib", "https://github.com/madler/zlib");
    copyrightPanel->insertItem(23, 1, "ZLIB License");
    copyrightPanel->setLink(23, 1, "ZLIB License", "https://github.com/madler/zlib/blob/master/LICENSE");
    copyrightPanel->insertItem(23, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(24, "wxWidgets");
    copyrightPanel->setLink(24, 0, "wxWidgets", "https://github.com/prusa3d/wxWidgets/");
    copyrightPanel->insertItem(24, 1, "LGPL");
    copyrightPanel->setLink(24, 1, "LGPL", "https://github.com/wxWidgets/wxWidgets/blob/master/docs/licence.txt");
    copyrightPanel->insertItem(24, 2, _L("common_popup_copyright_modifiedno"));

    copyrightPanel->insertRowItemHeader(25, "paho.mqtt.c");
    copyrightPanel->setLink(25, 0, "paho.mqtt.c", "https://github.com/eclipse/paho.mqtt.c");
    copyrightPanel->insertItem(25, 1, "EPLV2.0");
    copyrightPanel->setLink(25, 1, "EPLV2.0", "https://github.com/eclipse/paho.mqtt.c/blob/master/LICENSE");
    copyrightPanel->insertItem(25, 2, _L("common_popup_copyright_modifiedno"));

    m_panel = copyrightPanel;

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}


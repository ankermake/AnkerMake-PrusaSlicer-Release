#include "AnkerHint.h"
#include <slic3r/GUI/AnkerBtn.hpp>
#include <slic3r/GUI/GUI_App.hpp>
#include <slic3r/GUI/I18N.hpp>
#include <slic3r/GUI/Common/AnkerGUIConfig.hpp>
#include <slic3r/Utils/WxFontUtils.hpp>

#ifdef __APPLE__
AnkerHint::AnkerHint(const wxString& title, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxNO_BORDER | wxSTAY_ON_TOP | wxFRAME_TOOL_WINDOW)
#else
AnkerHint::AnkerHint(const wxString& title, wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxNO_BORDER)
#endif
    , m_message(title) 
{    
}

AnkerHint::~AnkerHint() {

}

void AnkerHint::InitUI(int width, int height)
{
    SetSize(AnkerSize(width, height));
    auto size = GetSize();
    SetBackgroundColour(wxColour("#333438"));

    // message
    m_msgText = new wxStaticText(this, wxID_ANY, "");   
    m_msgText->SetForegroundColour(wxColour("#FFFFFF"));
    //m_msgText->SetBackgroundColour(wxColour(255, 0, 0));
    m_msgText->SetFont(ANKER_FONT_NO_2);

    int langType = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
    auto ww = m_msgText->GetSize().GetWidth();
    Slic3r::GUI::WxFontUtils::setText_wrap(m_msgText, GetSize().GetWidth(), m_message, langType);
    m_msgText->Fit();
   
    // checkbox
    wxStaticText* checkBoxLabel = new wxStaticText(this, wxID_ANY, _L("common_button_donotshowagain"));
    checkBoxLabel->SetFont(ANKER_FONT_NO_2);
    checkBoxLabel->SetForegroundColour(wxColour("#FFFFFF"));
    wxCheckBox* checkBox = new wxCheckBox(this, wxID_ANY, "");
    checkBox->SetSize(size.GetWidth(), AnkerLength(21));

    wxBoxSizer* checkBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    checkBoxSizer->Add(checkBox, 0, wxALIGN_LEFT | wxALL);
    checkBoxSizer->AddSpacer(5);
    checkBoxSizer->Add(checkBoxLabel, 0, wxALL);

    // ok button
    wxSize okSize(size.GetWidth(), AnkerLength(30));
    AnkerBtn* okButton = new AnkerBtn(this, wxID_OK, wxDefaultPosition, okSize, wxBORDER_NONE);
    okButton->SetText(_L("common_button_ok"));
    okButton->SetFont(ANKER_FONT_NO_1);
    okButton->SetBackgroundColour("#62D361");
    okButton->SetTextColor("#FFFFFF");
    okButton->SetRadius(4);
    okButton->Bind(wxEVT_BUTTON, [this, checkBox](wxCommandEvent& event) {
        Close();
        Destroy();
        if (m_okBtnCallback) {
			m_okBtnCallback(checkBox->IsChecked());
		}
    });
    wxBoxSizer* okBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    okBoxSizer->Add(okButton, 1, wxALIGN_CENTER_VERTICAL | wxALL);

    wxBoxSizer* contentVSizer = new wxBoxSizer(wxVERTICAL);
    contentVSizer->AddStretchSpacer(1);
    contentVSizer->Add(m_msgText, 0, wxLEFT | wxRIGHT | wxTOP, AnkerLength(16));
    contentVSizer->AddSpacer(AnkerLength(10));
    contentVSizer->Add(checkBoxSizer, 0 | wxALIGN_CENTER, wxLEFT | wxRIGHT, AnkerLength(16));
    contentVSizer->AddSpacer(AnkerLength(24));
    contentVSizer->Add(okBoxSizer, 0, wxLEFT | wxRIGHT, AnkerLength(16));
    contentVSizer->AddSpacer(AnkerLength(16));
    contentVSizer->AddStretchSpacer(1);
    SetSizerAndFit(contentVSizer);

    checkBox->Bind(wxEVT_CHECKBOX, [this, checkBox](wxCommandEvent& event) {
	});
}

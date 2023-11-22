#include "AnkerRoundDialog.hpp"
#include "../GUI_App.hpp"
#include "AnkerGUIConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "../AnkerBtn.hpp"

namespace Slic3r {
	namespace GUI {

		AnkerRoundDialog::AnkerRoundDialog(wxWindow* parent, wxString strTitle, wxString strContentText)
			:AnkerRoundBaseDialog(parent),m_TitleText(strTitle),m_ContentText(strContentText)
		{
			InitUi();
			resetWindow(parent);
			InitEvent();
		}

		AnkerRoundDialog::~AnkerRoundDialog()
		{

		}

		void AnkerRoundDialog::InitUi()
		{
			m_strCloseImgFileName = "fdm_nav_del_icon.png";
			SetMinSize(AnkerSize(400, 185));
			m_pSizer = new wxBoxSizer(wxVERTICAL);
			SetSizer(m_pSizer);
			m_pHeadPanel = new AnkerBasePanel(this);
			m_pSizer->Add(m_pHeadPanel,0,wxALL,0);
			m_pHeadPanel->SetMinSize(AnkerSize(400, 40));

			wxBoxSizer* titleHSizer = new wxBoxSizer(wxHORIZONTAL);
			titleHSizer->AddStretchSpacer(186);
			m_pHeadPanel->SetSizer(titleHSizer);
			wxStaticText* pTitleText = new wxStaticText(m_pHeadPanel, wxID_ANY, m_TitleText, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
			pTitleText->SetMinSize(AnkerSize(267, 21));
			pTitleText->SetForegroundColour(wxColour("#FFFFFF"));
			pTitleText->SetFont(ANKER_BOLD_FONT_SIZE(12));
			titleHSizer->Add(pTitleText, 107, wxEXPAND | wxALIGN_CENTER | wxTOP| wxBOTTOM, 12);

			titleHSizer->AddStretchSpacer(185);

			wxImage exitImage = wxImage(wxString::FromUTF8(Slic3r::var(m_strCloseImgFileName.c_str())), wxBITMAP_TYPE_PNG);
			exitImage.Rescale(20, 20);
			AnkerBaseButton* pExitBtn = new AnkerBaseButton(m_pHeadPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 40), wxNO_BORDER);
			pExitBtn->SetBitmap(exitImage);
			pExitBtn->SetSizeHints(exitImage.GetSize(),exitImage.GetSize());
			pExitBtn->SetBackgroundColour(DEFAULT_BG_COLOR);
			pExitBtn->Bind(wxEVT_BUTTON, &AnkerRoundDialog::OnExitButtonClicked, this);
			titleHSizer->Add(pExitBtn, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM |wxRIGHT, 10);

			//add split line
			wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxNO_BORDER);
			splitLineCtrl->SetBackgroundColour(wxColour(54, 58, 63));
			m_pSizer->Add(splitLineCtrl, 0, wxEXPAND | wxALL, 0);
	
			m_pContentPanel = new AnkerBasePanel(this);
			wxStaticText* pMessageText = new wxStaticText(m_pContentPanel, wxID_ANY, m_ContentText);
			pMessageText->SetMinSize(AnkerSize(352, 21));
			pMessageText->SetMaxSize(AnkerSize(352, 90));
			pMessageText->SetBackgroundColour(DEFAULT_BG_COLOR);
			pMessageText->SetForegroundColour(wxColour("#FFFFFF"));
			pMessageText->SetFont(ANKER_FONT_NO_1);
			wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
			contentSizer->Add(pMessageText, 1, wxEXPAND | wxALIGN_CENTER, 0);
			m_pContentPanel->SetSizer(contentSizer);
			m_pSizer->Add(m_pContentPanel, 1, wxEXPAND | wxLeft|wxRIGHT, 24);

			m_pBottomPanel = new AnkerBasePanel(this);
			wxBoxSizer* btnHSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pBottomPanel->SetSizer(btnHSizer);
			btnHSizer->AddSpacer(24);

			AnkerBtn* pCancelBtn = new AnkerBtn(m_pBottomPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			pCancelBtn->SetText(m_cancelText);
			pCancelBtn->SetMinSize(AnkerSize(170, 32));
			pCancelBtn->SetBackgroundColour(wxColor(97, 98, 101));
			pCancelBtn->SetForegroundColour(wxColour("#FFFFFF"));
			pCancelBtn->SetRadius(3);
			pCancelBtn->SetTextColor(wxColor("#FFFFFF"));
			pCancelBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
			pCancelBtn->Bind(wxEVT_BUTTON, &AnkerRoundDialog::OnCancelButtonClicked, this);
			btnHSizer->Add(pCancelBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

			m_pBtnSpaceItem = btnHSizer->AddSpacer(12);

			AnkerBtn* pOKBtn = new AnkerBtn(m_pBottomPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			pOKBtn->SetText(m_okText);
			pOKBtn->SetMinSize(AnkerSize(170, 32));
			pOKBtn->SetBackgroundColour("#62D361");
			pOKBtn->SetForegroundColour(wxColour("#FFFFFF"));
			pOKBtn->SetRadius(3);
			pOKBtn->SetTextColor(wxColor("#FFFFFF"));
			pOKBtn->SetFont(ANKER_BOLD_FONT_SIZE(12));
			pOKBtn->Bind(wxEVT_BUTTON, &AnkerRoundDialog::OnOKButtonClicked, this);
			btnHSizer->Add(pOKBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);
			btnHSizer->AddSpacer(24);
			m_pSizer->Add(m_pBottomPanel, 0, wxEXPAND | wxBOTTOM, 16);
		}

		void AnkerRoundDialog::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC dc(this);
			wxSize size = GetSize();
			dc.SetPen(DEFAULT_BG_COLOR);
			dc.SetBrush(DEFAULT_BG_COLOR);
			dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
		}

		void AnkerRoundDialog::resetWindow(wxWindow* parent)
		{
			if (parent == nullptr)
			{
				int screenH = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, nullptr);
				int screenW = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, nullptr);
				SetPosition(wxPoint((screenW - 400) / 2, (screenH - 180) / 2));
			}
			else
			{
				SetPosition(wxPoint((parent->GetSize().x - 400) / 2, (parent->GetSize().y - 180) / 2));
			}
		}

		void AnkerRoundDialog::InitEvent()
		{
			Bind(wxEVT_PAINT, &AnkerRoundDialog::OnPaint, this);
		}

		void AnkerRoundDialog::OnExitButtonClicked(wxCommandEvent& event)
		{
			EndModal(wxID_OK);
			Hide();
		}

		void AnkerRoundDialog::OnOKButtonClicked(wxCommandEvent& event)
		{
			EndModal(wxID_OK);
			Hide();
		}

		void AnkerRoundDialog::OnCancelButtonClicked(wxCommandEvent& event)
		{
			EndModal(wxID_OK);
			Hide();
		}
	}
}
#include "AnkerPrintFinishDialog.hpp"
#include "AnkerBtn.hpp"
#include "GUI_App.hpp"

#include "libslic3r/Utils.hpp"
 #include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/Thumbnails.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"
#include "slic3r/Utils/WxFontUtils.hpp"
#include "DeviceObjectBase.h"
#ifdef _WIN32
#include <dbt.h>
#include <shlobj.h>
#endif // _WIN32

#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetModule/BuryDefines.h"

class AnkerStaticBox : public wxPanel {
public:
	AnkerStaticBox(wxWindow* parent, wxColour bgClr = wxColour(51, 52, 56), wxColour borderClr = wxColour(71, 72, 76))
		: wxPanel(parent) 
		, m_bgClr(bgClr)
		, m_boderClr(borderClr)
	{
		SetBackgroundColour(m_bgClr);

		Bind(wxEVT_PAINT, &AnkerStaticBox::OnPaint, this);
	}

private:
	void OnPaint(wxPaintEvent& event) {
		wxPaintDC dc(this);

		//dc.SetPen(wxPen(m_boderClr, 2));
		//dc.DrawRectangle(GetSize());


		wxRect rect = GetClientRect();
		dc.SetPen(wxPen(m_boderClr, 1));
		dc.SetBrush(wxBrush(m_bgClr));
		dc.DrawRoundedRectangle(rect, 4);
	}

private:
	wxColour m_boderClr;
	wxColour m_bgClr;
};






AnkerPrintFinishDialog::AnkerPrintFinishDialog(std::string currentDeviceSn, wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE)
	, m_currentDeviceSn(currentDeviceSn)
{
	m_dialogColor = wxColour(51, 52, 56);
	m_textLightColor = wxColour(TEXT_LIGHT_RGB_INT);
#ifdef __APPLE__
	m_textDarkColor = wxColour(193, 193, 193);
#else
	m_textDarkColor = wxColour(TEXT_DARK_RGB_INT);
#endif // __APPLE__
	m_LineColor = wxColour(71, 72, 76);

	initUI();

	//Bind(wxEVT_SHOW, &AnkerPrintFinishDialog::OnShow, this);
	//Bind(wxEVT_MOVE, &AnkerPrintFinishDialog::OnMove, this);
}

AnkerPrintFinishDialog::~AnkerPrintFinishDialog()
{

}


void AnkerPrintFinishDialog::setPreviewImage(int width, int height, unsigned char* data, unsigned char* alpha, bool freeFlag)
{
#if 0
	if (data == nullptr)
		return;

	wxImage image(width, height, data, alpha, freeFlag);
	image.Rescale(160, 160);
	image.Replace(0, 0, 0, m_dialogColor.Red(), m_dialogColor.Green(), m_dialogColor.Blue());

	wxBitmap scaledBitmap(image);
	m_pPreviewImage->SetBitmap(scaledBitmap);
	m_pPreviewImage->SetMinSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetMaxSize(scaledBitmap.GetSize());

	m_importResult.m_previewImage = image;
#endif
}

void AnkerPrintFinishDialog::Reset()
{
	m_qualityScore = -1;
	m_printStopReason = STOP_REASON_NONE;

}

void AnkerPrintFinishDialog::ShowPrintFinishDialog(bool finishSuss, AnkerPrintFinishDialog::PrintFinishInfo& result)
{
	Reset();
	UpdateUI(finishSuss, result);

	if (finishSuss) {
		m_panelType = PRINT_SUCCESS_PANEL;
		m_pPrintSuccessPanel->Show(true);
		m_pPrintFailPanel->Show(false);
	}
	else {
		m_panelType = PRINT_FAIL_PANEL;

		m_failStatusSizer->Show(true);
		m_pPreviewImg_fail->Show(true);
		m_FeedBackHyperLink->Show(true);
		m_PrintingFailReasonSizer->Show(false);

		m_pPrintSuccessPanel->Show(false);
		m_pPrintFailPanel->Show(true);
	}

	Layout();
	Refresh();
}



void AnkerPrintFinishDialog::UpdateUI(bool finishSuss, AnkerPrintFinishDialog::PrintFinishInfo& result)
{
	DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
	if (currentDev)
		m_pTitledPanel->setTitle(wxString::FromUTF8(currentDev->GetStationName()));


	wxImage image = result.m_previewImage;
	if (result.m_previewImage.IsOk())
		image = result.m_previewImage;
	else
	{
		image = wxImage(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
	}
	image.Rescale(100, 100);
	//image.Replace(0, 0, 0, m_imageBackColor.Red(), m_imageBackColor.Green(), m_imageBackColor.Blue());
	wxBitmap scaledBitmap(image);

	wxDateTime currentDateTime = wxDateTime::Now();
	wxString currentDateTimeFormated = wxString::Format("%d.%d.%d %d:%02d %s",
		currentDateTime.GetYear(),
		currentDateTime.GetMonth() + 1,
		currentDateTime.GetDay(),
		currentDateTime.GetHour(),
		currentDateTime.GetMinute(),
		currentDateTime.GetHour() >= 0 && currentDateTime.GetHour() < 12 ? "AM" : "PM"
	);

	int seconds = result.m_timeSecond;
	int hours = seconds / 60 / 60;
	int minutes = seconds / 60 % 60;
	seconds = seconds % 60;
	std::string newTime =
		(hours > 0 ? std::to_string(hours) + "h" : "") +
		(minutes > 0 || seconds <= 0 ? std::to_string(minutes) + "min" : "") +
		(hours <= 0 && minutes <= 0 && seconds > 0 ? std::to_string(seconds) + "s" : "");

	if (finishSuss) {
		m_pPreviewImg->SetBitmap(scaledBitmap);
		m_pGcodeFileNameText->SetLabelText(wxString::FromUTF8(result.m_fileName));
		m_pPercentageText->SetLabelText("100%");
		m_pFinishTimeStampText->SetLabelText(currentDateTimeFormated);
		m_pPrintTimeValText->SetLabelText(newTime);
		m_pFilamentValText->SetLabelText(result.m_filamentStr);

		UpdateAllQuilityScoreBtns();
	}
	else {
		m_pPreviewImg_fail->SetBitmap(scaledBitmap);
		m_pGcodeFileNameText_fail->SetLabelText(wxString::FromUTF8(result.m_fileName));
		if (result.m_totleLayer > 0)
			m_pPercentageText_fail->SetLabelText(wxString::Format("%d%%(%d/%d)", (int)(100 * result.m_currentLayer / result.m_totleLayer), result.m_currentLayer, result.m_totleLayer));
		else
			m_pPercentageText_fail->SetLabelText("--");
		m_pFinishTimeStampText_fail->SetLabelText(currentDateTimeFormated);
		m_pPrintTimeValText_fail->SetLabelText(newTime);
		m_pFilamentValText_fail->SetLabelText(result.m_filamentStr);

		UpdatePrintStopedReasonBtn();
	}

	Refresh();
}




void AnkerPrintFinishDialog::initUI()
{
	SetBackgroundColour(m_dialogColor);
	SetSizeHints(AnkerSize(324, 530), AnkerSize(324, 530));
	SetSize(AnkerSize(324, 530));

	wxBoxSizer* dialogVSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogVSizer);

	m_pTitledPanel = new AnkerTitledPanel(this, 44, 8);
	m_pTitledPanel->SetBackgroundColour(m_dialogColor);
	m_pTitledPanel->setTitle(L"Select File");
	m_pTitledPanel->setTitleAlign(AnkerTitledPanel::TitleAlign::CENTER);
	int closeBtnID = m_pTitledPanel->addTitleButton(wxString::FromUTF8(Slic3r::var("fdm_nav_del_icon.png")), false);
	m_pTitledPanel->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, closeBtnID](wxCommandEvent& event) {
		int btnID = event.GetInt();
		if (btnID == closeBtnID)
		{

			/*
			m_gcodeInfoReq = false;
			m_fileListUpdateReq = false;
			m_gcodePreviewWaiting = false;
			*/
			EndModal(wxCANCEL);
			Hide();
		}
		});
	dialogVSizer->Add(m_pTitledPanel, 1, wxEXPAND, 0);

	wxPanel* contentPanel = new wxPanel(m_pTitledPanel);
	wxBoxSizer* mainVSizer = new wxBoxSizer(wxVERTICAL);
	contentPanel->SetSizer(mainVSizer);

	m_pTitledPanel->setContentPanel(contentPanel);



	initPrintSuccessSizer(contentPanel);
	initPrintFailSizer(contentPanel);

	// dhf test data
	{
		AnkerPrintFinishDialog::PrintFinishInfo res;
		res.m_fileName = "xxxx.gcode";
		res.m_filamentStr = "12345_testing";
		res.m_timeSecond = 1000;
		ShowPrintFinishDialog(false, res);
	}
}






bool AnkerPrintFinishDialog::initPrintSuccessSizer(wxWindow* parent)
{
	int margin = 25;

	m_pPrintSuccessPanel = new wxPanel(parent);
	wxBoxSizer* pPrintSucessSizer = new wxBoxSizer(wxVERTICAL);

	parent->GetSizer()->Add(m_pPrintSuccessPanel, 1 , wxEXPAND |wxALL, 0);

	pPrintSucessSizer->AddSpacer(AnkerLength(20));


	{
		wxBoxSizer* SuccessStatusSizer = new wxBoxSizer(wxHORIZONTAL);


		SuccessStatusSizer->AddStretchSpacer(1);

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("print_finish_sucess.png")), wxBITMAP_TYPE_PNG);
		//image.Rescale(20, 20);
		wxStaticBitmap* pFinishedSuccessIcon = new wxStaticBitmap(m_pPrintSuccessPanel, wxID_ANY, image);
		pFinishedSuccessIcon->SetMinSize(image.GetSize());
		pFinishedSuccessIcon->SetMaxSize(image.GetSize());
		pFinishedSuccessIcon->SetBackgroundColour(m_dialogColor);
		SuccessStatusSizer->Add(pFinishedSuccessIcon,0, wxALIGN_CENTRE_VERTICAL);

		SuccessStatusSizer->AddSpacer(6);

		// Success
		wxStaticText* pFinishedSuccessText = new wxStaticText(m_pPrintSuccessPanel, wxID_ANY, _("common_print_popupfinished_noticecompleted"));
		pFinishedSuccessText->SetBackgroundColour(m_dialogColor);
		pFinishedSuccessText->SetForegroundColour(wxColour(98,211,97));
		pFinishedSuccessText->SetFont(ANKER_FONT_NO_1);
		SuccessStatusSizer->Add(pFinishedSuccessText,0, wxALIGN_CENTRE_VERTICAL);

		SuccessStatusSizer->AddStretchSpacer(1);

        pPrintSucessSizer->Add(SuccessStatusSizer,0, wxALIGN_CENTER,0);
	}

	pPrintSucessSizer->AddSpacer(10);

	// gcode preview; filename ..
	{
		AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintSuccessPanel);

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(100, 100);
		m_pPreviewImg = new wxStaticBitmap(staticBox, wxID_ANY, image);
		m_pPreviewImg->SetMinSize(image.GetSize());
		m_pPreviewImg->SetMaxSize(image.GetSize());
		m_pPreviewImg->SetBackgroundColour(m_dialogColor);

		m_pGcodeFileNameText = new wxStaticText(staticBox, wxID_ANY, _("xxxxxx.gcode"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
		m_pGcodeFileNameText->SetBackgroundColour(m_dialogColor);
		m_pGcodeFileNameText->SetForegroundColour(m_textDarkColor);
		//m_pGcodeFileNameText->SetMinSize(wxSize(270, 50));
		m_pGcodeFileNameText->SetMaxSize(wxSize(270, 50));
		m_pGcodeFileNameText->SetFont(ANKER_FONT_NO_1);

		m_pPercentageText = new wxStaticText(staticBox, wxID_ANY, _("100%"));
		m_pPercentageText->SetBackgroundColour(m_dialogColor);
		m_pPercentageText->SetForegroundColour(m_textLightColor);
		m_pPercentageText->SetFont(ANKER_BOLD_FONT_NO_1);

		m_pFinishTimeStampText = new wxStaticText(staticBox, wxID_ANY, _("2023.3.30 12:45 PM"));
		m_pFinishTimeStampText->SetBackgroundColour(m_dialogColor);
		m_pFinishTimeStampText->SetForegroundColour(m_textDarkColor);
		m_pFinishTimeStampText->SetFont(ANKER_FONT_NO_1);

		wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
		vSizer->AddSpacer(15);
		vSizer->Add(m_pPreviewImg, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->AddSpacer(5);
		vSizer->Add(m_pGcodeFileNameText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->Add(m_pPercentageText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->Add(m_pFinishTimeStampText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->AddSpacer(15);

		staticBox->SetSizer(vSizer);
		pPrintSucessSizer->Add(staticBox,1,wxEXPAND | wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT, margin);
		//pPrintSucessSizer->Add(staticBox, 1, wxEXPAND  | wxLEFT | wxRIGHT, margin);
	}
	pPrintSucessSizer->AddSpacer(10);

	// Print time & Filament
	{
		wxBoxSizer* Hsizer = new wxBoxSizer(wxHORIZONTAL);

		// Print time
		{
			AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintSuccessPanel);

			wxStaticText* pPrintTimeLabelText = new wxStaticText(staticBox, wxID_ANY, _("common_print_popupfinished_timecost"));
			pPrintTimeLabelText->SetBackgroundColour(m_dialogColor);
			pPrintTimeLabelText->SetForegroundColour(m_textDarkColor);
			pPrintTimeLabelText->SetFont(ANKER_FONT_NO_1);

			m_pPrintTimeValText = new wxStaticText(staticBox, wxID_ANY, _("20h 40m"));
			m_pPrintTimeValText->SetBackgroundColour(m_dialogColor);
			m_pPrintTimeValText->SetForegroundColour(m_textLightColor);
			m_pPrintTimeValText->SetFont(ANKER_BOLD_FONT_NO_1);


			wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
			vSizer->AddSpacer(5);
			vSizer->Add(pPrintTimeLabelText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->Add(m_pPrintTimeValText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->AddSpacer(5);

			staticBox->SetSizer(vSizer);
			Hsizer->Add(staticBox, 1);
		}
		
		Hsizer->AddSpacer(10);

		// Filament
		{
			AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintSuccessPanel);

			wxStaticText* pFilamentLabelText = new wxStaticText(staticBox, wxID_ANY, _("common_print_popupfinished_filamentcost"));
			pFilamentLabelText->SetBackgroundColour(m_dialogColor);
			pFilamentLabelText->SetForegroundColour(m_textDarkColor);
			pFilamentLabelText->SetFont(ANKER_FONT_NO_1);

			m_pFilamentValText = new wxStaticText(staticBox, wxID_ANY, _("200g"));
			m_pFilamentValText->SetBackgroundColour(m_dialogColor);
			m_pFilamentValText->SetForegroundColour(m_textLightColor);
			m_pFilamentValText->SetFont(ANKER_BOLD_FONT_NO_1);


			wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
			vSizer->AddSpacer(5);
			vSizer->Add(pFilamentLabelText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->Add(m_pFilamentValText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->AddSpacer(5);

			staticBox->SetSizer(vSizer);
			Hsizer->Add(staticBox, 1);
		}

		pPrintSucessSizer->Add(Hsizer, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);
	}

	pPrintSucessSizer->AddSpacer(10);

	// split line
	wxPanel* line = new wxPanel(m_pPrintSuccessPanel);
	line->SetBackgroundColour(m_LineColor);
	line->SetMinSize(wxSize(-1,1));
	line->SetMaxSize(wxSize(-1, 1));
	pPrintSucessSizer->Add(line, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);

	pPrintSucessSizer->AddSpacer(10);

	// rate the quality hint text
	wxStaticText* pRateQuatlityHintText = new wxStaticText(m_pPrintSuccessPanel, wxID_ANY, _("common_print_popupfinished_tate"));
	pRateQuatlityHintText->SetBackgroundColour(m_dialogColor);
	pRateQuatlityHintText->SetForegroundColour(m_textDarkColor);
	pRateQuatlityHintText->SetFont(ANKER_FONT_NO_1);
	pPrintSucessSizer->Add(pRateQuatlityHintText, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);

	// quality score Icons
	{
		wxBoxSizer* Hsizer = new wxBoxSizer(wxHORIZONTAL);
		auto CreateQualityScoreButton = [&](int qualityScore) ->wxBitmapButton* {
#if 1
			wxBitmap bitmap(wxString::FromUTF8(Slic3r::var("print_quality_unmark.png")), wxBITMAP_TYPE_PNG); 
			wxBitmap::Rescale(bitmap, wxSize(20, 20));
			wxBitmapButton* pQualityScoreBtn = new wxBitmapButton(m_pPrintSuccessPanel, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			pQualityScoreBtn->SetBackgroundColour(m_dialogColor);
			pQualityScoreBtn->SetName(wxString::Format("quality_score_%d", qualityScore));
			pQualityScoreBtn->SetMinSize(wxSize(25,25));
			pQualityScoreBtn->SetMaxSize(wxSize(25, 25));
			pQualityScoreBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnClickQualityScoreBtn, this);
			pQualityScoreBtn->Bind(wxEVT_ENTER_WINDOW, &AnkerPrintFinishDialog::OnEnterQualityScoreBtn, this);
			pQualityScoreBtn->Bind(wxEVT_LEAVE_WINDOW, &AnkerPrintFinishDialog::OnLeaveQualityScoreBtn, this);

			Hsizer->Add(pQualityScoreBtn, 0, wxALL, 6);

			return pQualityScoreBtn;
#else
			m_bitmapUnmark = new wxBitmap(wxString::FromUTF8(Slic3r::var("print_quality_unmark.png")), wxBITMAP_TYPE_PNG);
			m_bitmapMark = new wxBitmap(wxString::FromUTF8(Slic3r::var("print_quality_mark.png")), wxBITMAP_TYPE_PNG);
			wxBitmap::Rescale(*m_bitmapUnmark, wxSize(20, 20));
			wxBitmap::Rescale(*m_bitmapMark, wxSize(20, 20));

			AnkerBtn* pQualityScoreBtn = new AnkerBtn(m_pPrintSuccessPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			pQualityScoreBtn->SetMinSize(wxSize(30, 30));
			pQualityScoreBtn->SetMaxSize(wxSize(30, 30));
			pQualityScoreBtn->SetSize(wxSize(30, 30));
			pQualityScoreBtn->SetBackgroundColour(m_dialogColor);
			pQualityScoreBtn->SetNorImg(m_bitmapUnmark);
			pQualityScoreBtn->SetEnterImg(m_bitmapMark);
			pQualityScoreBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnClickQualityScoreBtn, this);

			Hsizer->Add(pQualityScoreBtn, 0, wxALL, 0);
#endif
		};

		for (int idx = 0; idx < 5; ++idx) {
			int qualityScore = idx + 1;
			m_QualityScoreBtns.push_back(CreateQualityScoreButton(qualityScore));
		}
		pPrintSucessSizer->Add(Hsizer, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);
	}

	pPrintSucessSizer->AddStretchSpacer(1);

	// button sizer
	{
		wxBoxSizer* buttonHSizer = new wxBoxSizer(wxHORIZONTAL);
		
		buttonHSizer->AddStretchSpacer(1);

		m_pRePrintBtn = new AnkerBtn(m_pPrintSuccessPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_pRePrintBtn->SetMinSize(AnkerSize(120, 32));
		m_pRePrintBtn->SetMaxSize(AnkerSize(120, 32));
		m_pRePrintBtn->SetText(/*L"Reprint"*/_("common_print_popupfinished_buttonreprint"));
		m_pRePrintBtn->SetBackgroundColour(wxColor("#5c5d60"));
		m_pRePrintBtn->SetRadius(4);
		m_pRePrintBtn->SetTextColor(wxColor("#FFFFFF"));
		m_pRePrintBtn->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pRePrintBtn->SetBackRectColour(m_dialogColor);
		m_pRePrintBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintBtn, this);
		buttonHSizer->Add(m_pRePrintBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFinishBtn = new AnkerBtn(m_pPrintSuccessPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_pFinishBtn->SetMinSize(AnkerSize(120, 32));
		m_pFinishBtn->SetMaxSize(AnkerSize(120, 32));
		m_pFinishBtn->SetText(/*L"Finish"*/_("common_print_popupfinished_buttonfinish"));
		m_pFinishBtn->SetBackgroundColour(wxColor("#62D361"));
		m_pFinishBtn->SetRadius(4);
		m_pFinishBtn->SetTextColor(wxColor("#FFFFFF"));
		m_pFinishBtn->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pFinishBtn->SetBackRectColour(m_dialogColor);
		m_pFinishBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnFinishBtn, this);
		buttonHSizer->Add(m_pFinishBtn, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT, 30);

		buttonHSizer->AddStretchSpacer(1);

		pPrintSucessSizer->Add(buttonHSizer, 1, wxEXPAND|wxBOTTOM, 15);
	}

	m_pPrintSuccessPanel->SetSizer(pPrintSucessSizer);
	return true;
}


bool AnkerPrintFinishDialog::initPrintFailSizer(wxWindow* parent)
{
	int margin = 25;

	m_pPrintFailPanel = new wxPanel(parent);
	wxBoxSizer* pPrintFailSizer = new wxBoxSizer(wxVERTICAL);
	m_pPrintFailPanel->SetSizer(pPrintFailSizer);
	parent->GetSizer()->Add(m_pPrintFailPanel, 1, wxEXPAND, 0);

	//pPrintFailSizer->AddSpacer(AnkerLength(10));

	{
		m_failStatusSizer = new wxBoxSizer(wxHORIZONTAL);

		m_failStatusSizer->AddStretchSpacer(1);

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("print_finish_sucess.png")), wxBITMAP_TYPE_PNG);   // print_finish_fail.png
		//image.Rescale(20, 20);
		wxStaticBitmap* pFinishedFailIcon = new wxStaticBitmap(m_pPrintFailPanel, wxID_ANY, image);
		pFinishedFailIcon->SetMinSize(image.GetSize());
		pFinishedFailIcon->SetMaxSize(image.GetSize());
		pFinishedFailIcon->SetBackgroundColour(m_dialogColor);
		m_failStatusSizer->Add(pFinishedFailIcon, 0, wxALIGN_CENTRE_VERTICAL);

		m_failStatusSizer->AddSpacer(6);

		// fail
		wxStaticText* pFinishedFaileText = new wxStaticText(m_pPrintFailPanel, wxID_ANY, _("common_print_popupfinished_noticefailed"));
		pFinishedFaileText->SetBackgroundColour(m_dialogColor);
		pFinishedFaileText->SetForegroundColour(wxColour(255, 0, 0));
		pFinishedFaileText->SetFont(ANKER_FONT_NO_1);
		m_failStatusSizer->Add(pFinishedFaileText, 0, wxALIGN_CENTRE_VERTICAL);

		m_failStatusSizer->AddStretchSpacer(1);

		pPrintFailSizer->Add(m_failStatusSizer,0,wxALIGN_CENTER,0);

	}

	pPrintFailSizer->AddSpacer(10);

	// gcode preview; filename ..
	{
		AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintFailPanel);

		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(100, 100);
		m_pPreviewImg_fail = new wxStaticBitmap(staticBox, wxID_ANY, image);
		m_pPreviewImg_fail->SetMinSize(image.GetSize());
		m_pPreviewImg_fail->SetMaxSize(image.GetSize());
		m_pPreviewImg_fail->SetBackgroundColour(m_dialogColor);

		m_pGcodeFileNameText_fail = new wxStaticText(staticBox, wxID_ANY, _("xxxxxx.gcode"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
		m_pGcodeFileNameText_fail->SetBackgroundColour(m_dialogColor);
		m_pGcodeFileNameText_fail->SetForegroundColour(m_textDarkColor);
		//m_pGcodeFileNameText_fail->SetMinSize(wxSize(270, 50));
		m_pGcodeFileNameText_fail->SetMaxSize(wxSize(265, 50));
		m_pGcodeFileNameText_fail->SetFont(ANKER_FONT_NO_1);

		m_pPercentageText_fail = new wxStaticText(staticBox, wxID_ANY, _("67%(670/1000)"));
		m_pPercentageText_fail->SetBackgroundColour(m_dialogColor);
		m_pPercentageText_fail->SetForegroundColour(m_textLightColor);
		m_pPercentageText_fail->SetFont(ANKER_BOLD_FONT_NO_1);

		m_pFinishTimeStampText_fail = new wxStaticText(staticBox, wxID_ANY, _("2023.3.30 12:45 PM"));
		m_pFinishTimeStampText_fail->SetBackgroundColour(m_dialogColor);
		m_pFinishTimeStampText_fail->SetForegroundColour(m_textDarkColor);
		m_pFinishTimeStampText_fail->SetFont(ANKER_FONT_NO_1);

		wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
		vSizer->AddSpacer(15);
		vSizer->Add(m_pPreviewImg_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->AddSpacer(5);
		vSizer->Add(m_pGcodeFileNameText_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->Add(m_pPercentageText_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->Add(m_pFinishTimeStampText_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);
		vSizer->AddSpacer(15);

		staticBox->SetSizer(vSizer);
		pPrintFailSizer->Add(staticBox, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);
	}
	pPrintFailSizer->AddSpacer(10);

	// Print time & Filament
	{
		wxBoxSizer* Hsizer = new wxBoxSizer(wxHORIZONTAL);

		// Print time
		{
			AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintFailPanel);

			wxStaticText* pPrintTimeLabelText = new wxStaticText(staticBox, wxID_ANY, _("common_print_popupfinished_timecost"));
			pPrintTimeLabelText->SetBackgroundColour(m_dialogColor);
			pPrintTimeLabelText->SetForegroundColour(m_textDarkColor);
			pPrintTimeLabelText->SetFont(ANKER_FONT_NO_1);

			m_pPrintTimeValText_fail = new wxStaticText(staticBox, wxID_ANY, _("20h 40m"));
			m_pPrintTimeValText_fail->SetBackgroundColour(m_dialogColor);
			m_pPrintTimeValText_fail->SetForegroundColour(m_textLightColor);
			m_pPrintTimeValText_fail->SetFont(ANKER_BOLD_FONT_NO_1);


			wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
			vSizer->AddSpacer(5);
			vSizer->Add(pPrintTimeLabelText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->Add(m_pPrintTimeValText_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->AddSpacer(5);

			staticBox->SetSizer(vSizer);
			Hsizer->Add(staticBox, 1);
		}

		Hsizer->AddSpacer(10);

		// Filament
		{
			AnkerStaticBox* staticBox = new AnkerStaticBox(m_pPrintFailPanel);

			wxStaticText* pFilamentLabelText = new wxStaticText(staticBox, wxID_ANY, _("common_print_popupfinished_filamentcost"));
			pFilamentLabelText->SetBackgroundColour(m_dialogColor);
			pFilamentLabelText->SetForegroundColour(m_textDarkColor);
			pFilamentLabelText->SetFont(ANKER_FONT_NO_1);

			m_pFilamentValText_fail = new wxStaticText(staticBox, wxID_ANY, _("200g"));
			m_pFilamentValText_fail->SetBackgroundColour(m_dialogColor);
			m_pFilamentValText_fail->SetForegroundColour(m_textLightColor);
			m_pFilamentValText_fail->SetFont(ANKER_BOLD_FONT_NO_1);

			wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
			vSizer->AddSpacer(5);
			vSizer->Add(pFilamentLabelText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->Add(m_pFilamentValText_fail, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
			vSizer->AddSpacer(5);

			staticBox->SetSizer(vSizer);
			Hsizer->Add(staticBox, 1);
		}

		pPrintFailSizer->Add(Hsizer, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);
	}

	pPrintFailSizer->AddSpacer(10);

	// split line
	wxPanel* line = new wxPanel(m_pPrintFailPanel);
	line->SetBackgroundColour(m_LineColor);
	line->SetMinSize(wxSize(-1, 1));
	line->SetMaxSize(wxSize(-1, 1));
	pPrintFailSizer->Add(line, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);

	pPrintFailSizer->AddSpacer(10);

	// FeedBackHyperLink
	{
		wxString lableText = _L("common_print_popupfinished_failedfeedback");
		m_FeedBackHyperLink = new AnkerHyperlink(m_pPrintFailPanel, wxID_ANY, lableText, "", wxColour("#000000"));
		m_FeedBackHyperLink->SetBackgroundColour(m_dialogColor);
		//wxSize textSize = m_FeedBackHyperLink->GetTextExtent(lableText);
		m_FeedBackHyperLink->SetMinSize(AnkerSize(this->GetSize().GetWidth() - margin * 2,  50));
		m_FeedBackHyperLink->SetSize(AnkerSize(this->GetSize().GetWidth() - margin * 2, 50));
		m_FeedBackHyperLink->SetWrapWidth(this->GetSize().GetWidth() - margin*2);
		m_FeedBackHyperLink->SetCustumAction(std::bind(&AnkerPrintFinishDialog::OnClickFeedbackHyperlink, this));
		pPrintFailSizer->Add(m_FeedBackHyperLink, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);
	}

	// printing stop reasion sizer
	{
		// lable: "Why did you stop the printing?"
		wxStaticText* label = new wxStaticText(m_pPrintFailPanel, wxID_ANY, _("common_print_popupfinished_feedbacktitle"));
		label->SetBackgroundColour(m_dialogColor);
		label->SetForegroundColour(m_textLightColor);
		label->SetFont(ANKER_BOLD_FONT_NO_1);

		// btn: Bottom Layer Adhesion Failure
			auto BottomLyerAdhesionBtn = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
			BottomLyerAdhesionBtn->SetMinSize(wxSize(220, 32));
			BottomLyerAdhesionBtn->SetMaxSize(wxSize(220, 32));
			BottomLyerAdhesionBtn->SetSize(wxSize(220, 32));
			BottomLyerAdhesionBtn->SetText(_L("common_print_popupfinished_feedbackoption1"));
			BottomLyerAdhesionBtn->SetName("Bottom Layer Adhesion Failure");
			BottomLyerAdhesionBtn->SetBackgroundColour(m_dialogColor);
			BottomLyerAdhesionBtn->SetBgHoverColor(m_dialogColor);
			BottomLyerAdhesionBtn->SetborderNorColor(m_LineColor);
			BottomLyerAdhesionBtn->SetborderHoverBGColor("#A0A0A0");
			BottomLyerAdhesionBtn->SetTextColor(m_textDarkColor);
			BottomLyerAdhesionBtn->SetRadius(0);
			BottomLyerAdhesionBtn->SetFont(ANKER_FONT_NO_1);
			BottomLyerAdhesionBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintStopedReasonBtnClick, this);
	
		// btn: Spaghetti Mess
			auto SpaghettiMessBtn = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
			SpaghettiMessBtn->SetMinSize(wxSize(220, 32));
			SpaghettiMessBtn->SetMaxSize(wxSize(220, 32));
			SpaghettiMessBtn->SetSize(wxSize(220, 32));
			SpaghettiMessBtn->SetText(_L("common_print_popupfinished_feedbackoption2"));
			SpaghettiMessBtn->SetName("Spaghetti Mess");
			SpaghettiMessBtn->SetBackgroundColour(m_dialogColor);
			SpaghettiMessBtn->SetBgHoverColor(m_dialogColor);
			SpaghettiMessBtn->SetborderNorColor(m_LineColor);
			SpaghettiMessBtn->SetborderHoverBGColor("#A0A0A0");
			SpaghettiMessBtn->SetTextColor(m_textDarkColor);
			SpaghettiMessBtn->SetRadius(2);
			SpaghettiMessBtn->SetFont(ANKER_FONT_NO_1);
			SpaghettiMessBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintStopedReasonBtnClick, this);

		// btn: Extruder Jam
			auto ExtruderJamBtn = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
			ExtruderJamBtn->SetMinSize(wxSize(220, 32));
			ExtruderJamBtn->SetMaxSize(wxSize(220, 32));
			ExtruderJamBtn->SetSize(wxSize(220, 32));
			ExtruderJamBtn->SetText(_L("common_print_popupfinished_feedbackoption3"));
			ExtruderJamBtn->SetName("Extruder Jam");
			ExtruderJamBtn->SetBackgroundColour(m_dialogColor);
			ExtruderJamBtn->SetBgHoverColor(m_dialogColor);
			ExtruderJamBtn->SetborderNorColor(m_LineColor);
			ExtruderJamBtn->SetborderHoverBGColor("#A0A0A0");
			ExtruderJamBtn->SetTextColor(m_textDarkColor);
			ExtruderJamBtn->SetRadius(2);
			ExtruderJamBtn->SetFont(ANKER_FONT_NO_1);
			ExtruderJamBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintStopedReasonBtnClick, this);

		// btn: Give up to print
			auto GiveUpPrintBtn = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
			GiveUpPrintBtn->SetMinSize(wxSize(220, 32));
			GiveUpPrintBtn->SetMaxSize(wxSize(220, 32));
			GiveUpPrintBtn->SetSize(wxSize(220, 32));
			GiveUpPrintBtn->SetText(_L("common_print_popupfinished_feedbackoption4"));
			GiveUpPrintBtn->SetName("Give up to print");
			GiveUpPrintBtn->SetBackgroundColour(m_dialogColor);
			GiveUpPrintBtn->SetBgHoverColor(m_dialogColor);
			GiveUpPrintBtn->SetborderNorColor(m_LineColor);
			GiveUpPrintBtn->SetborderHoverBGColor("#A0A0A0");
			GiveUpPrintBtn->SetTextColor(m_textDarkColor);
			GiveUpPrintBtn->SetRadius(2);
			GiveUpPrintBtn->SetFont(ANKER_FONT_NO_1);
			GiveUpPrintBtn->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintStopedReasonBtnClick, this);

			m_PrintingFailReasonSizer = new wxBoxSizer(wxVERTICAL);
			m_PrintingFailReasonSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);
			m_PrintingFailReasonSizer->Add(BottomLyerAdhesionBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 3);
			m_PrintingFailReasonSizer->Add(SpaghettiMessBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 3);
			m_PrintingFailReasonSizer->Add(ExtruderJamBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 3);
			m_PrintingFailReasonSizer->Add(GiveUpPrintBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 3);

			m_printStopReasonBtns.push_back(BottomLyerAdhesionBtn);
			m_printStopReasonBtns.push_back(SpaghettiMessBtn);
			m_printStopReasonBtns.push_back(ExtruderJamBtn);
			m_printStopReasonBtns.push_back(GiveUpPrintBtn);

		pPrintFailSizer->Add(m_PrintingFailReasonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, margin);

		m_PrintingFailReasonSizer->Show(false);
	}

	pPrintFailSizer->AddStretchSpacer(10);

	// button sizer
	{
		wxBoxSizer* buttonHSizer = new wxBoxSizer(wxHORIZONTAL);

		buttonHSizer->AddStretchSpacer(1);

		m_pRePrintBtn_fail = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_pRePrintBtn_fail->SetMinSize(AnkerSize(120, 32));
		m_pRePrintBtn_fail->SetMaxSize(AnkerSize(120, 32));
		m_pRePrintBtn_fail->SetText(/*L"Reprint"*/_("common_print_popupfinished_buttonreprint"));
		m_pRePrintBtn_fail->SetBackgroundColour(wxColor("#5c5d60"));
		m_pRePrintBtn_fail->SetRadius(4);
		m_pRePrintBtn_fail->SetTextColor(wxColor("#FFFFFF"));
		m_pRePrintBtn_fail->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pRePrintBtn_fail->SetBackRectColour(m_dialogColor);
		m_pRePrintBtn_fail->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnPrintBtn, this);
		buttonHSizer->Add(m_pRePrintBtn_fail, 1, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFinishBtn_fail = new AnkerBtn(m_pPrintFailPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_pFinishBtn_fail->SetMinSize(AnkerSize(120, 32));
		m_pFinishBtn_fail->SetMaxSize(AnkerSize(120, 32));
		m_pFinishBtn_fail->SetText(/*L"Finish"*/_("common_print_popupfinished_buttonfinish"));
		m_pFinishBtn_fail->SetBackgroundColour(wxColor("#62D361"));
		m_pFinishBtn_fail->SetRadius(4);
		m_pFinishBtn_fail->SetTextColor(wxColor("#FFFFFF"));
		m_pFinishBtn_fail->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pFinishBtn_fail->SetBackRectColour(m_dialogColor);
		m_pFinishBtn_fail->Bind(wxEVT_BUTTON, &AnkerPrintFinishDialog::OnFinishBtn, this);
		buttonHSizer->Add(m_pFinishBtn_fail, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT, 30);
		buttonHSizer->AddStretchSpacer(1);

		pPrintFailSizer->Add(buttonHSizer, 1, wxEXPAND | wxBOTTOM, 15);
	}

	//pPrintFailSizer->AddSpacer(30);

	return true;
}













void AnkerPrintFinishDialog::OnClickQualityScoreBtn(wxCommandEvent& event)
{
	wxBitmapButton* button = dynamic_cast<wxBitmapButton*>(event.GetEventObject());
	if (!button) {
		return;
	}

	m_qualityScore = GetBtnQuilityScore(button);
	ANKER_LOG_INFO << "set print quality score:"<< m_qualityScore;
	UpdateAllQuilityScoreBtns();
}




void AnkerPrintFinishDialog::OnEnterQualityScoreBtn(wxMouseEvent& event)
{
	wxBitmapButton* button = dynamic_cast<wxBitmapButton*>(event.GetEventObject());
	if (!button) {
		return;
	}

	button->SetBackgroundColour(m_dialogColor);
	int btnScore = GetBtnQuilityScore(button);

	wxBitmap bitmapUnmark(wxString::FromUTF8(Slic3r::var("print_quality_unmark.png")), wxBITMAP_TYPE_PNG);
	wxBitmap bitmapMark(wxString::FromUTF8(Slic3r::var("print_quality_mark.png")), wxBITMAP_TYPE_PNG);
	wxBitmap::Rescale(bitmapUnmark, wxSize(22, 22));
	wxBitmap::Rescale(bitmapMark, wxSize(22, 22));
	button->SetBitmapLabel(btnScore <= m_qualityScore ? bitmapMark : bitmapUnmark);

	Refresh();
}



void AnkerPrintFinishDialog::OnLeaveQualityScoreBtn(wxMouseEvent& event)
{
	wxBitmapButton* button = dynamic_cast<wxBitmapButton*>(event.GetEventObject());
	if (!button) {
		return;
	}
	UpdateAllQuilityScoreBtns();
}

void AnkerPrintFinishDialog::UpdateAllQuilityScoreBtns()
{
	wxBitmap bitmapUnmark(wxString::FromUTF8(Slic3r::var("print_quality_unmark.png")), wxBITMAP_TYPE_PNG);
	wxBitmap bitmapMark(wxString::FromUTF8(Slic3r::var("print_quality_mark.png")), wxBITMAP_TYPE_PNG);
	bool mark = true;
	for (int i = 0; i < m_QualityScoreBtns.size(); ++i) {
		auto btn = m_QualityScoreBtns[i];
		if (btn) {
			btn->SetBitmapLabel(GetBtnQuilityScore(btn) <= m_qualityScore ? bitmapMark : bitmapUnmark);
		}
	}
	Refresh();
}


void AnkerPrintFinishDialog::OnClickFeedbackHyperlink()
{
	m_failStatusSizer->Show(false);
	m_pPreviewImg_fail->Show(false);
	m_FeedBackHyperLink->Show(false);
	m_PrintingFailReasonSizer->Show(true);
	Layout();
	Refresh();
}


int AnkerPrintFinishDialog::GetBtnQuilityScore(wxBitmapButton* button)
{
	if (!button) {
		return 0;
	}

	wxString btnName = button->GetName();
	btnName.Replace("quality_score_", "");
	int btnScore = 0;
	if (btnName.ToInt(&btnScore))
		return btnScore;

	return 0;
}



void AnkerPrintFinishDialog::OnPrintStopedReasonBtnClick(wxCommandEvent& event)
{
	AnkerBtn* button = dynamic_cast<AnkerBtn*>(event.GetEventObject());
	if (!button) {
		return;
	}
	m_printStopReason = GetPrintStopReasonFromBtn(button);
	ANKER_LOG_INFO << "set print stop reason :" << m_printStopReason;
	UpdatePrintStopedReasonBtn();
}


AnkerPrintFinishDialog::PrintStopReason AnkerPrintFinishDialog::GetPrintStopReasonFromBtn(AnkerBtn* button)
{
	PrintStopReason reason = STOP_REASON_NONE;
	if (button) {
		if (button->GetName() == "Bottom Layer Adhesion Failure")
			reason = STOP_REASON_BOTTOM_LAYER_ADHESION;
		else if (button->GetName() == "Spaghetti Mess")
			reason = STOP_REASON_SPAGHETTI_MESS;
		else if (button->GetName() == "Extruder Jam")
			reason = STOP_REASON_EXTRUDER_JAM;
		else if (button->GetName() == "Give up to print")
			reason = STOP_REASON_GIVE_UP;
	}
	return reason;
}


void AnkerPrintFinishDialog::UpdatePrintStopedReasonBtn()
{
	for (int i = 0; i < m_printStopReasonBtns.size(); ++i) {
		AnkerBtn* btn = m_printStopReasonBtns[i];
		if (btn) {
			PrintStopReason reason = GetPrintStopReasonFromBtn(btn);
			// ANKER_LOG_INFO << "==xxx=== m_printStopReason:" << m_printStopReason<<"  reason:"<< reason<<"  btn:"<< btn->GetName();
			if (m_printStopReason == reason) {
				btn->SetBgNorColor(wxColour("#62d361"));
				btn->SetTextColor(wxColour("#FFFFFF"));
			}
			else {
				btn->SetBgNorColor(m_dialogColor);
				btn->SetTextColor(m_textDarkColor);
			}
		}
	}
	Refresh();
}

void AnkerPrintFinishDialog::ReportPrintResult()
{
	if (m_panelType = PRINT_SUCCESS_PANEL) {
		if (m_qualityScore > 0)
		{
			std::map<std::string, std::string> map;
			map.insert(std::make_pair(c_ratings, std::to_string(m_qualityScore)));
			BuryAddEvent(e_print_ratings, map);
			ANKER_LOG_INFO << "report print quality score :" << m_qualityScore;
		}
	}
	else if (m_panelType = PRINT_FAIL_PANEL) {
		if (m_printStopReason >= 0)
		{
			std::map<std::string, std::string> map;
			map.insert(std::make_pair(c_reason, std::to_string(m_printStopReason)));
			BuryAddEvent(e_print_stop_reason, map);
			ANKER_LOG_INFO << "report print stop reason :" << m_printStopReason;
		}
	}
}

void AnkerPrintFinishDialog::OnPrintBtn(wxCommandEvent& event)
{
	ReportPrintResult();
	EndModal(wxOK);
	Hide();
}

void AnkerPrintFinishDialog::OnFinishBtn(wxCommandEvent& event)
{
	ReportPrintResult();
	EndModal(wxCANCEL);
	Hide();
}

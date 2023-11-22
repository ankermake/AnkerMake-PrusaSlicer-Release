#include "AnkerGCodeImportDialog.hpp"
#include "AnkerBtn.hpp"
#include "GUI_App.hpp"

#include "libslic3r/Utils.hpp"
 #include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/Thumbnails.hpp"
#include "slic3r/GUI/Common/AnkerGUIConfig.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"
#include "slic3r/GUI/Common/AnkerLoadingMask.hpp"
#include "slic3r/GUI/Common/AnkerTitledPanel.hpp"
#include "slic3r/Utils/WxFontUtils.hpp"

#ifdef _WIN32
#include <dbt.h>
#include <shlobj.h>
#endif // _WIN32

#include <time.h>
#include <chrono>


wxDEFINE_EVENT(wxCUSTOMEVT_FILEITEM_CLICKED, wxCommandEvent);

AnkerGCodeImportDialog::AnkerGCodeImportDialog(std::string currentDeviceSn, wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE)
	, m_currentDeviceSn(currentDeviceSn)
	, m_localImportDefaultDir(".")
	, m_pFileSelectPanel(nullptr)
	, m_pFileInfoPanel(nullptr)
	, m_pFinishedPanel (nullptr)
	, m_pFSComputerWidget(nullptr)
	, m_pFSEmptyWidget(nullptr)
	, m_pFSStorageListWidget(nullptr)
	, m_pFSUSBListWidget(nullptr)
	, m_pComputerBtn(nullptr)
	, m_pStorageBtn(nullptr)
	, m_pUSBBtn(nullptr)
	, m_pPreviewImage(nullptr)
	, m_pSpeedText(nullptr)
	, m_pFilamentText(nullptr)
	, m_pPrintTimeText(nullptr)
	, m_pSearchTextCtrl(nullptr)
	, m_pLoadingMask(nullptr)
	, m_currentMode(FSM_NONE)
	, m_fileListUpdateReq(false)
	, m_gcodeInfoReq(false)
	, m_gcodePreviewWaiting(false)
{
	m_dialogColor = wxColour(PANEL_BACK_LIGHT_RGB_INT);
	m_textLightColor = wxColour(TEXT_LIGHT_RGB_INT);
#ifdef __APPLE__
	m_textDarkColor = wxColour(193, 193, 193);
#else
	m_textDarkColor = wxColour(TEXT_DARK_RGB_INT);
#endif // __APPLE__
	m_btnFocusColor = wxColour(69, 102, 74);
	m_btnFocusTextColor = wxColour(ANKER_RGB_INT);
	m_splitLineColor = wxColour(255, 255, 255);
	m_splitLineColor = m_splitLineColor.ChangeLightness(30);

	initUI();

	Bind(wxEVT_SHOW, &AnkerGCodeImportDialog::OnShow, this);
	Bind(wxEVT_MOVE, &AnkerGCodeImportDialog::OnMove, this);
}

AnkerGCodeImportDialog::~AnkerGCodeImportDialog()
{

}

size_t onDownLoadFinishedCallBack(char* dest, size_t size, size_t nmemb, void* userp)
{
	return size * nmemb;
}


void onDownLoadProgress(double dltotal, double dlnow, double ultotal, double ulnow)
{

}

void AnkerGCodeImportDialog::requestCallback(int type)
{

}

void AnkerGCodeImportDialog::setFileInfoSpeed(std::string str)
{
	std::string speedStr = "--";
	if (!str.empty())
	{
		speedStr = str;
		int unitIndex = speedStr.find("mm/s");
		std::string speedValueStr = speedStr.substr(0, unitIndex);
		if (unitIndex == -1)
		{
			speedStr += "mm/s";
		}

		int speedValue = std::atoi(speedValueStr.c_str());
		int speedRate = speedValue / 50;
		if (speedRate > 0)
		{
			speedStr = "X" + std::to_string(speedRate) + ".0(" + speedStr + ")";
		}
	}

	//m_pSpeedText->SetLabelText(speedStr);

	Layout();

	m_importResult.m_speedStr = speedStr;
}

void AnkerGCodeImportDialog::setFileInfoFilament(std::string str)
{
	m_pFilamentText->SetLabelText(str);

	Layout();

	m_importResult.m_filamentStr = str;
}

void AnkerGCodeImportDialog::setFileInfoTime(int seconds)
{
	int hours = seconds / 60 / 60;
	int minutes = seconds / 60 % 60;
	std::string newTime = "";
	if (hours > 0)
		newTime += std::to_string(hours) + "h";
	if (minutes > 0)
		newTime += std::to_string(minutes) + "min";
	if (newTime.empty())
		newTime = "--";

	m_pPrintTimeText->SetLabelText(newTime);

	Layout();

	m_importResult.m_timeSecond = seconds;
}

void AnkerGCodeImportDialog::setPreviewImage(int width, int height, unsigned char* data, unsigned char* alpha, bool freeFlag)
{
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
}

void AnkerGCodeImportDialog::switch2FileSelect(FileSelectMode mode)
{
	m_pFileSelectPanel->Show(true);
	m_pFileInfoPanel ->Show(false);
	m_pFinishedPanel->Show(false);

	//SetTitle(L"Select File");
	m_pTitledPanel->setTitle(/*L"Select File"*/_("common_print_filepath_title"));
	//SetSizer(m_pFileSelectVSizer, false);

	switch2FSMode(mode);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::switch2FileInfo(std::string filepath, std::string strTitleName)
{
	m_pFileSelectPanel->Show(false);
	m_pFileInfoPanel->Show(true);
	m_pFinishedPanel->Show(false);

	int lastSlashIndex = filepath.find_last_of("\\");
	lastSlashIndex = lastSlashIndex < 0 ? filepath.find_last_of("/") : lastSlashIndex;
	int subStrStart = lastSlashIndex >= 0 ? lastSlashIndex + 1 : 0;
	int subStrLen = lastSlashIndex >= 0 ? filepath.size() - lastSlashIndex : filepath.size();
	std::string filename = subStrStart >= filepath.size() ? "" : filepath.substr(subStrStart, subStrLen);

	m_importResult.m_fileName = filename;
	m_importResult.m_filePath = filepath;

	if (!strTitleName.empty()) {
        int lastSlashIndex1 = strTitleName.find_last_of("\\");
        lastSlashIndex1 = lastSlashIndex1 < 0 ? strTitleName.find_last_of("/") : lastSlashIndex1;
        int subStrStart1 = lastSlashIndex1 >= 0 ? lastSlashIndex1 + 1 : 0;
        int subStrLen1 = lastSlashIndex1 >= 0 ? filepath.size() - lastSlashIndex1 : filepath.size();
		filename = subStrStart1 >= strTitleName.size() ? "" : strTitleName.substr(subStrStart1, subStrLen1);
	}
   
	wxString wxFileName = wxString(filename);
	//SetTitle(wxFileName);
	m_pTitledPanel->setTitle(wxFileName);
	//SetSizer(m_pFileInfoVSizer, false);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::switch2PrintFinished(bool success, GCodeImportResult& result)
{
	m_pFileSelectPanel->Show(false);
	m_pFileInfoPanel->Show(false);
	m_pFinishedPanel->Show(true);

	m_importResult = result;

	//SetTitle(wxString::FromUTF8(result.m_fileName));
	m_pTitledPanel->setTitle(wxString::FromUTF8(result.m_fileName));
	//SetSizer(m_pFinishedVSizer, false);

	if (success)
	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(120, 120);
		m_pFinishedStatusImage->SetBitmap(image);
		m_pFinishedStatusImage->SetMinSize(image.GetSize());
		m_pFinishedStatusImage->SetMaxSize(image.GetSize());

		m_pFinishedStatusText->SetLabelText(/*L"Success"*/_("common_print_taskpannelfinished_completed"));
	}
	else
	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_failed_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(120, 120);
		m_pFinishedStatusImage->SetBitmap(image);
		m_pFinishedStatusImage->SetMinSize(image.GetSize());
		m_pFinishedStatusImage->SetMaxSize(image.GetSize());

		m_pFinishedStatusText->SetLabelText(/*L"Print Failed"*/_("common_print_taskpannelfinished_failed"));
	}

	m_pFinishedFilamentText->SetLabelText(result.m_filamentStr);

	int seconds = result.m_timeSecond;
	int hours = seconds / 60 / 60;
	int minutes = seconds / 60 % 60;
	seconds = seconds % 60;
	std::string newTime = 
		(hours > 0 ? std::to_string(hours) + "h" : "") +
		(minutes > 0 || seconds <= 0 ? std::to_string(minutes) + "min" : "") +
		(hours <= 0 && minutes <= 0 && seconds > 0 ? std::to_string(seconds) + "s" : "");
	m_pFinishedPrintTimeText->SetLabelText(newTime);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::initUI()
{
	SetBackgroundColour(m_dialogColor);
	SetSizeHints(AnkerSize(324, 531), AnkerSize(324, 531));
	SetSize(AnkerSize(324, 531));

	wxBoxSizer* dialogVSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogVSizer);

	m_pTitledPanel = new AnkerTitledPanel(this, 44, 8);
	m_pTitledPanel->SetBackgroundColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	m_pTitledPanel->setTitle(L"Select File");
	m_pTitledPanel->setTitleAlign(AnkerTitledPanel::TitleAlign::CENTER);
	int closeBtnID = m_pTitledPanel->addTitleButton(wxString::FromUTF8(Slic3r::var("fdm_nav_del_icon.png")), false);
	m_pTitledPanel->Bind(wxANKEREVT_ATP_BUTTON_CLICKED, [this, closeBtnID](wxCommandEvent& event) {
		int btnID = event.GetInt();
		if (btnID == closeBtnID)
		{
			m_gcodeInfoReq = false;
			m_fileListUpdateReq = false;
			m_gcodePreviewWaiting = false;

			EndModal(wxCANCEL);
			Hide();
		}
		});
	dialogVSizer->Add(m_pTitledPanel, 1, wxEXPAND, 0);

	wxPanel* contentPanel = new wxPanel(m_pTitledPanel);
	wxBoxSizer* mainVSizer = new wxBoxSizer(wxVERTICAL);
	contentPanel->SetSizer(mainVSizer);

	m_pTitledPanel->setContentPanel(contentPanel);

	// File Select Sizer
	{
		m_pFileSelectPanel = new wxPanel(contentPanel);
		wxBoxSizer* pFileSelectVSizer = new wxBoxSizer(wxVERTICAL);
		m_pFileSelectPanel->SetSizer(pFileSelectVSizer);
		mainVSizer->Add(m_pFileSelectPanel, 1, wxEXPAND, 0);

		pFileSelectVSizer->AddSpacer(9);

		{
			wxBoxSizer* buttonTabSizer = new wxBoxSizer(wxHORIZONTAL);
			pFileSelectVSizer->Add(buttonTabSizer, 0, wxALIGN_CENTER, 0);

            m_pComputerBtn = new AnkerBtn(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_CENTER);
            m_pComputerBtn->SetMinSize(AnkerSize(90, 24));
            m_pComputerBtn->SetMaxSize(AnkerSize(90, 24));
            m_pComputerBtn->SetText(/*L"Computer"*/_("common_print_filepath_title1"));
            m_pComputerBtn->SetBackgroundColour(m_dialogColor);
            m_pComputerBtn->SetRadius(4);
            m_pComputerBtn->SetTextColor(m_textDarkColor);
            m_pComputerBtn->SetFont(ANKER_FONT_NO_1);
            m_pComputerBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnComputerBtn, this);
			m_pComputerBtn->SetBackRectColour(m_dialogColor);
            buttonTabSizer->Add(m_pComputerBtn, 0, wxEXPAND, 0);

            m_pStorageBtn = new AnkerBtn(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_CENTER);
            m_pStorageBtn->SetMinSize(AnkerSize(90, 24));
            m_pStorageBtn->SetMaxSize(AnkerSize(90, 24));
            m_pStorageBtn->SetText(/*L"Storage"*/_("common_print_filepath_title2"));
            m_pStorageBtn->SetBackgroundColour(m_dialogColor);
            m_pStorageBtn->SetRadius(4);
            m_pStorageBtn->SetTextColor(m_textDarkColor);
            m_pStorageBtn->SetFont(ANKER_FONT_NO_1);
            m_pStorageBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnStorageBtn, this);
			m_pStorageBtn->SetBackRectColour(m_dialogColor);
            buttonTabSizer->Add(m_pStorageBtn, 0, wxEXPAND, 0);
            
            m_pUSBBtn = new AnkerBtn(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_CENTER);
            m_pUSBBtn->SetMinSize(AnkerSize(90, 24));
            m_pUSBBtn->SetMaxSize(AnkerSize(90, 24));
            m_pUSBBtn->SetText(/*L"USB"*/_("common_print_filepath_title3"));
            m_pUSBBtn->SetBackgroundColour(m_dialogColor);
            m_pUSBBtn->SetRadius(4);
            m_pUSBBtn->SetTextColor(m_textDarkColor);
            m_pUSBBtn->SetFont(ANKER_FONT_NO_1);
            m_pUSBBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnUSBBtn, this);
			m_pUSBBtn->SetBackRectColour(m_dialogColor);
            buttonTabSizer->Add(m_pUSBBtn, 0, wxEXPAND, 0);
		}

		{
			pFileSelectVSizer->AddSpacer(8);

			wxControl* frameBox = new wxControl(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
			frameBox->SetBackgroundColour(m_dialogColor);
			frameBox->SetForegroundColour(m_textDarkColor);
			frameBox->SetMinSize(AnkerSize(276, 24));
			frameBox->SetMaxSize(AnkerSize(276, 24));
			pFileSelectVSizer->Add(frameBox, 0, wxALIGN_CENTER, 0);

			wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);
			frameBox->SetSizer(searchSizer);

			wxButton* searchBtn = new wxButton(frameBox, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			searchBtn->SetBackgroundColour(m_dialogColor);
			wxBitmap searchBtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("search_white.png")), wxBITMAP_TYPE_PNG);
			wxImage image = searchBtnBitmap.ConvertToImage();
			image.Rescale(20, 20);
			wxBitmap scaledBitmap(image);
			searchBtn->SetBitmap(scaledBitmap);
			searchBtn->SetMaxSize(scaledBitmap.GetSize());
			searchBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnSearchBtn, this);
			searchSizer->Add(searchBtn, 0, wxALIGN_LEFT | wxALIGN_BOTTOM, 0);

			searchSizer->AddSpacer(5);

			m_pSearchTextCtrl = new wxTextCtrl(frameBox, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			m_pSearchTextCtrl->SetMinSize(AnkerSize(270, 24));
#ifdef __APPLE__
			m_pSearchTextCtrl->SetBackgroundColour(wxColour(31, 32, 35));
#else
			m_pSearchTextCtrl->SetBackgroundColour(m_dialogColor);
#endif // __APPLE__
			m_pSearchTextCtrl->SetForegroundColour(m_textLightColor);
			m_pSearchTextCtrl->Bind(wxEVT_TEXT, &AnkerGCodeImportDialog::OnSearchTextChanged, this);
			m_pSearchTextCtrl->Bind(wxEVT_TEXT_ENTER, &AnkerGCodeImportDialog::OnSearchTextEnter, this);
			m_pSearchTextCtrl->SetEditable(true);
			searchSizer->Add(m_pSearchTextCtrl, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_BOTTOM | wxTOP | wxBOTTOM, 2);
		}

		pFileSelectVSizer->AddSpacer(5);

		m_pFSComputerWidget = new wxScrolledWindow(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSComputerWidget->SetMinSize(AnkerSize(324, 422));
		m_pFSComputerWidget->SetScrollRate(-1, 60);
		pFileSelectVSizer->Add(m_pFSComputerWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSEmptyWidget = new wxScrolledWindow(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSEmptyWidget->SetMinSize(AnkerSize(324, 422));
		m_pFSEmptyWidget->SetScrollRate(-1, 60);
		pFileSelectVSizer->Add(m_pFSEmptyWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSStorageListWidget = new wxScrolledWindow(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSStorageListWidget->SetMinSize(AnkerSize(324, 422));
		m_pFSStorageListWidget->SetScrollRate(-1, 60);
		pFileSelectVSizer->Add(m_pFSStorageListWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSUSBListWidget = new wxScrolledWindow(m_pFileSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSUSBListWidget->SetMinSize(AnkerSize(324, 422));
		m_pFSUSBListWidget->SetScrollRate(-1, 60);
		pFileSelectVSizer->Add(m_pFSUSBListWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		initFSComputerSizer(m_pFSComputerWidget);
		initFSEmptySizer(m_pFSEmptyWidget);
		initFSListSizer(m_pFSStorageListWidget);
		initFSListSizer(m_pFSUSBListWidget);

		m_pFSEmptyWidget->Hide();
		m_pFSStorageListWidget->Hide();
		m_pFSUSBListWidget->Hide();
	}

	// File Info Sizer
	initFileInfoSizer(contentPanel);

	initFinishedSizer(contentPanel);

	switch2FileSelect(FSM_COMPUTER);
}

void AnkerGCodeImportDialog::SimulateData()
{
	printFilamentInfo info;
	info.iIndex = 0;
	info.bCanReplace = true;
	filamentInfo innerInfo;
	innerInfo.strfilamentColor = "#fffff";//white
	innerInfo.strFilamentName = "PLA";
	info.infoDetail = innerInfo;
	m_PrinterFilamentVec.push_back(info);

	info.iIndex = 1;
	innerInfo.strfilamentColor = "#FF0000";
	innerInfo.strFilamentName = "PLA+";
	m_PrinterFilamentVec.push_back(info);

	info.iIndex = 2;
	innerInfo.strfilamentColor = "#00ff00";
	innerInfo.strFilamentName = "PLA+";


	info.iIndex = 3;
	innerInfo.strfilamentColor = "#0000FF";
	innerInfo.strFilamentName = "PLA+";
	m_PrinterFilamentVec.push_back(info);

	info.iIndex = 4;
	innerInfo.strfilamentColor = "#12ff00";
	innerInfo.strFilamentName = "TPT+";
	info.bCanReplace = false;


	info.iIndex = 5;
	innerInfo.strfilamentColor = "#00ffF3";
	innerInfo.strFilamentName = "?";
	info.bCanReplace = false;


	filamentInfo gcodeFilementInfo;
	gcodeFilementInfo.strfilamentColor = "#fffff";
	gcodeFilementInfo.strFilamentName = "PLA";

	filamentMap.insert(std::make_pair(gcodeFilementInfo,m_PrinterFilamentVec[0]));

	gcodeFilementInfo.strfilamentColor = "#123456";
	gcodeFilementInfo.strFilamentName = "PLA+";
	filamentMap.insert(std::make_pair(gcodeFilementInfo, m_PrinterFilamentVec[1]));
}

bool AnkerGCodeImportDialog::initFSComputerSizer(wxWindow* parent)
{
	int langType = Slic3r::GUI::wxGetApp().getCurrentLanguageType();

	wxBoxSizer* computerSizer = new wxBoxSizer(wxVERTICAL);

	computerSizer->AddSpacer(115);

	// drag drop icon
	wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("DragDrop_File.png")), wxBITMAP_TYPE_PNG);
	wxImage image = bitmapEx.ConvertToImage();
	image.Rescale(60, 60);
	wxBitmap scaledBitmap(image);
	wxStaticBitmap* iconImage = new wxStaticBitmap(parent, wxID_ANY, scaledBitmap);
	iconImage->SetMinSize(bitmapEx.GetSize());
	iconImage->SetMaxSize(bitmapEx.GetSize());
	computerSizer->Add(iconImage, 0, wxALIGN_CENTER, 0);

	computerSizer->AddSpacer(16);

	// drag drop text
	wxStaticText* tipText = new wxStaticText(parent, wxID_ANY, "");
	tipText->SetBackgroundColour(m_dialogColor);
	tipText->SetForegroundColour(m_textDarkColor);
	tipText->SetFont(ANKER_FONT_NO_1);
	tipText->SetSize(AnkerSize(224, 32));
	Slic3r::GUI::WxFontUtils::setText_wrap(tipText, 224,  /*"Click to open the file."*/_("common_print_filepath_computernotice"), langType);
	computerSizer->Add(tipText, 0, wxALIGN_CENTER, 0);

	computerSizer->AddSpacer(32);

	// open button
	AnkerBtn* openButton = new AnkerBtn(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_CENTER);
	openButton->SetMinSize(AnkerSize(184, 24));
	openButton->SetMaxSize(AnkerSize(184, 24));
	openButton->SetText(/*L"Open"*/_("common_button_open"));
	openButton->SetBackgroundColour(wxColor("#62D361"));
	openButton->SetRadius(4);
	openButton->SetTextColor(wxColor("#FFFFFF"));
	openButton->SetFont(ANKER_BOLD_FONT_NO_1);
	openButton->SetBackRectColour(m_dialogColor);
	openButton->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnComputerImportBtn, this);
	computerSizer->Add(openButton, 0, wxALIGN_CENTER, 0);

	computerSizer->AddStretchSpacer(1);

	//computerSizer->Show(false);
	parent->SetSizer(computerSizer);

	return true;
}

bool AnkerGCodeImportDialog::initFSEmptySizer(wxWindow* parent)
{
	wxBoxSizer* emptySizer = new wxBoxSizer(wxVERTICAL);

	emptySizer->AddSpacer(120);

	// drag drop icon
	wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("file_icon_50x50.png")), wxBITMAP_TYPE_PNG);
	wxImage image = bitmapEx.ConvertToImage();
	image.Rescale(50, 50);
	wxBitmap scaledBitmap(image);
	wxStaticBitmap* iconImage = new wxStaticBitmap(parent, wxID_ANY, scaledBitmap);
	iconImage->SetMinSize(bitmapEx.GetSize());
	iconImage->SetMaxSize(bitmapEx.GetSize());
	emptySizer->Add(iconImage, 0, wxALIGN_CENTER, 0);

	emptySizer->AddSpacer(21);

	// drag drop text
	wxStaticText* tipText = new wxStaticText(parent, wxID_ANY, /*"No Files"*/_("common_print_filepathnotice_nofile"));
	tipText->SetBackgroundColour(m_dialogColor);
	tipText->SetForegroundColour(m_textDarkColor);
	tipText->SetFont(ANKER_FONT_NO_1);
	emptySizer->Add(tipText, 0, wxALIGN_CENTER, 0);

	//m_pFSEmptyWidget->Show(false);
	parent->SetSizer(emptySizer);

	return true;
}

bool AnkerGCodeImportDialog::initFSListSizer(wxWindow* parent)
{
	wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);

	parent->SetSizer(listSizer);

	return true;
}

bool AnkerGCodeImportDialog::initFileInfoSizer(wxWindow* parent)
{
	m_pFileInfoPanel = new wxPanel(parent);
	wxBoxSizer* pFileInfoVSizer = new wxBoxSizer(wxVERTICAL);
	m_pFileInfoPanel->SetSizer(pFileInfoVSizer);
	parent->GetSizer()->Add(m_pFileInfoPanel, 1, wxEXPAND, 0);

	pFileInfoVSizer->AddSpacer(AnkerLength(40));

	// preview image
	wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
	wxImage image = bitmapEx.ConvertToImage();

	//
	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);
	if(currentDev) {
		if (currentDev->devicePartsType == DEVICE_PARTS_NO)
		{
			image.Rescale(160, 160);
			image.Replace(0, 0, 0, m_dialogColor.Red(), m_dialogColor.Green(), m_dialogColor.Blue());
		}
		else
		{
			image.Rescale(115, 115);
			image.Replace(0, 0, 0, m_dialogColor.Red(), m_dialogColor.Green(), m_dialogColor.Blue());
		}
	}	

	wxBitmap scaledBitmap(image);
	m_pPreviewImage = new wxStaticBitmap(m_pFileInfoPanel, wxID_ANY, scaledBitmap);
	m_pPreviewImage->SetMinSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetMaxSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetBackgroundColour(m_dialogColor);

	if(currentDev)
	{
		if (currentDev->devicePartsType == DEVICE_PARTS_NO)
		{
			pFileInfoVSizer->Add(m_pPreviewImage, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 32);
		}
		else
		{
			pFileInfoVSizer->Add(m_pPreviewImage, 1, wxEXPAND | wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);
			//by Samuel,show filament mapping info for multi-color printer
			// split line
			wxControl* splitLineCtrl = new wxControl(m_pFileInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			splitLineCtrl->SetBackgroundColour(m_splitLineColor);
			splitLineCtrl->SetSizeHints(wxSize(264, 1), wxSize(264, 1));
			pFileInfoVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);

			//TODO: by Samuel,To get the mapping info from printer
		}
	}


	// split line
	wxControl* splitLineCtrl = new wxControl(m_pFileInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(m_splitLineColor);
	splitLineCtrl->SetMaxSize(wxSize(264, 1));
	splitLineCtrl->SetMinSize(wxSize(264, 1));
	pFileInfoVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 8);

	//m_pFileInfoVSizer->AddSpacer(16);

	// speed
	{
		//by Samuel, no need to display speed info
		/*wxBoxSizer* speedSizer = new wxBoxSizer(wxHORIZONTAL);
		speedSizer->SetMinSize(AnkerSize(264, 32));
		pFileInfoVSizer->Add(speedSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 30);

		wxImage speedBitmap = wxImage(wxString::FromUTF8(Slic3r::var("printer_speed.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* speedIcon = new wxStaticBitmap(m_pFileInfoPanel, wxID_ANY, speedBitmap);
		speedIcon->SetMinSize(speedBitmap.GetSize());
		speedSizer->Add(speedIcon, 0, wxALIGN_LEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

		wxStaticText* speedText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, "Speed"_("common_print_previewpopup_speed"));
		speedText->SetBackgroundColour(m_dialogColor);
		speedText->SetForegroundColour(m_textDarkColor);
		speedText->SetFont(ANKER_FONT_NO_1);
		speedText->Fit();
		speedSizer->Add(speedText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

		speedSizer->AddStretchSpacer(1);

		m_pSpeedText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, "X5.0(250mm/s)");
		m_pSpeedText->SetBackgroundColour(m_dialogColor);
		m_pSpeedText->SetForegroundColour(m_textLightColor);
		m_pSpeedText->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pSpeedText->Fit();
		speedSizer->Add(m_pSpeedText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 9);*/
	}


	// filament
	{
		wxBoxSizer* filamentSizer = new wxBoxSizer(wxHORIZONTAL);
		filamentSizer->SetMinSize(AnkerSize(264, 32));
		pFileInfoVSizer->Add(filamentSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 30);

		wxImage usedFBitmap = wxImage(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFileInfoPanel, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		filamentSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

		wxStaticText* usedFText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, /*"Filament"*/_("common_print_previewpopup_filament"));
		usedFText->SetBackgroundColour(m_dialogColor);
		usedFText->SetForegroundColour(m_textDarkColor);
		usedFText->SetFont(ANKER_FONT_NO_1);
		usedFText->Fit();
		filamentSizer->Add(usedFText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

		filamentSizer->AddStretchSpacer(1);

		m_pFilamentText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, "220g");
		m_pFilamentText->SetBackgroundColour(m_dialogColor);
		m_pFilamentText->SetForegroundColour(m_textLightColor);
		m_pFilamentText->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pFilamentText->Fit();
		filamentSizer->Add(m_pFilamentText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
	}

	//pFileInfoVSizer->AddSpacer(16);

	// print time
	{
		wxBoxSizer* printTimeSizer = new wxBoxSizer(wxHORIZONTAL);
		printTimeSizer->SetMinSize(AnkerSize(264, 32));
		pFileInfoVSizer->Add(printTimeSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 30);

		wxImage usedFBitmap = wxImage(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFileInfoPanel, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		printTimeSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

		wxStaticText* printTimeText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, /*"Print Time"*/_("common_print_previewpopup_printtime"));
		printTimeText->SetBackgroundColour(m_dialogColor);
		printTimeText->SetForegroundColour(m_textDarkColor);
		printTimeText->SetFont(ANKER_FONT_NO_1);
		printTimeText->Fit();
		printTimeSizer->Add(printTimeText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

		printTimeSizer->AddStretchSpacer(1);

		m_pPrintTimeText = new wxStaticText(m_pFileInfoPanel, wxID_ANY, "1h32mins");
		m_pPrintTimeText->SetBackgroundColour(m_dialogColor);
		m_pPrintTimeText->SetForegroundColour(m_textLightColor);
		m_pPrintTimeText->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pPrintTimeText->Fit();
		printTimeSizer->Add(m_pPrintTimeText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
	}

	pFileInfoVSizer->AddStretchSpacer(1);

	// button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	pFileInfoVSizer->Add(buttonSizer, 0, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 30);

	buttonSizer->AddStretchSpacer(1);

	m_pPrintBtn = new AnkerBtn(m_pFileInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pPrintBtn->SetMinSize(AnkerSize(264, 24));
	m_pPrintBtn->SetMaxSize(AnkerSize(264, 24));
	m_pPrintBtn->SetText(/*L"Start Printing"*/_("common_print_taskbar_buttonstart"));
	m_pPrintBtn->SetDisableTextColor(wxColor(105,105,108));
	m_pPrintBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pPrintBtn->SetRadius(4);
	m_pPrintBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pPrintBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pPrintBtn->SetBackRectColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	m_pPrintBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnPrintBtn, this);
	buttonSizer->Add(m_pPrintBtn, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT , 24);

	return true;
}

bool AnkerGCodeImportDialog::initFinishedSizer(wxWindow* parent)
{
	m_pFinishedPanel = new wxPanel(parent);
	m_pFinishedPanel->SetMinSize(AnkerSize(324, 486));
	wxBoxSizer* pFinishedVSizer = new wxBoxSizer(wxVERTICAL);
	m_pFinishedPanel->SetSizer(pFinishedVSizer);
	parent->GetSizer()->Add(m_pFinishedPanel, 1, wxEXPAND, 0);

	pFinishedVSizer->AddSpacer(AnkerLength(20));

	// preview image
	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(120, 120);
	m_pFinishedStatusImage = new wxStaticBitmap(m_pFinishedPanel, wxID_ANY, image);
	m_pFinishedStatusImage->SetMinSize(image.GetSize());
	m_pFinishedStatusImage->SetMaxSize(image.GetSize());
	m_pFinishedStatusImage->SetBackgroundColour(m_dialogColor);
	pFinishedVSizer->Add(m_pFinishedStatusImage, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	m_pFinishedStatusText = new wxStaticText(m_pFinishedPanel, wxID_ANY, /*"Success"*/_("common_print_taskpannelfinished_completed"),
												wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
	m_pFinishedStatusText->SetWindowStyle(wxALIGN_CENTER_HORIZONTAL);
	m_pFinishedStatusText->SetMinSize(AnkerSize(120, 30));
	m_pFinishedStatusText->SetSize(AnkerSize(120, 30));
	m_pFinishedStatusText->SetBackgroundColour(m_dialogColor);
	m_pFinishedStatusText->SetForegroundColour(m_textLightColor);

	m_pFinishedStatusText->SetFont(ANKER_FONT_NO_1);
	pFinishedVSizer->Add(m_pFinishedStatusText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 20);

	//m_pFinishedVSizer->AddSpacer(36);

	// split line
	wxControl* splitLineCtrl = new wxControl(m_pFinishedPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(m_splitLineColor);
	splitLineCtrl->SetMaxSize(wxSize(264, 1));
	splitLineCtrl->SetMinSize(wxSize(264, 1));
	pFinishedVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 16);

	//m_pFinishedVSizer->AddSpacer(16);

	// print time
	{
		wxBoxSizer* printTimeSizer = new wxBoxSizer(wxHORIZONTAL);
		printTimeSizer->SetMinSize(AnkerSize(264, 32));
		pFinishedVSizer->Add(printTimeSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 30);

		//wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
		//wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFinishedPanel, wxID_ANY, usedFBitmap);
		//usedFIcon->SetMinSize(usedFBitmap.GetSize());
		//printTimeSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		//printTimeSizer->AddSpacer(7);

		wxStaticText* printTimeText = new wxStaticText(m_pFinishedPanel, wxID_ANY, /*"Print Time"*/_("common_print_previewpopup_printtime"));
		printTimeText->SetBackgroundColour(m_dialogColor);
		printTimeText->SetForegroundColour(m_textDarkColor);
		printTimeText->SetFont(ANKER_FONT_NO_1);
		printTimeText->Fit();
		printTimeSizer->Add(printTimeText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

		printTimeSizer->AddStretchSpacer(1);

		m_pFinishedPrintTimeText = new wxStaticText(m_pFinishedPanel, wxID_ANY, "1h32min");
		m_pFinishedPrintTimeText->SetBackgroundColour(m_dialogColor);
		m_pFinishedPrintTimeText->SetForegroundColour(m_textLightColor);
		m_pFinishedPrintTimeText->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pFinishedPrintTimeText->Fit();
		printTimeSizer->Add(m_pFinishedPrintTimeText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
	}

	//pFinishedVSizer->AddSpacer(16);

	// filament
	{
		wxBoxSizer* filamentSizer = new wxBoxSizer(wxHORIZONTAL);
		filamentSizer->SetMinSize(AnkerSize(264, 34));
		pFinishedVSizer->Add(filamentSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 30);

		//wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
		//wxStaticBitmap* usedFIcon = new wxStaticBitmap(m_pFinishedPanel, wxID_ANY, usedFBitmap);
		//usedFIcon->SetMinSize(usedFBitmap.GetSize());
		//filamentSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		//filamentSizer->AddSpacer(7);

		wxStaticText* usedFText = new wxStaticText(m_pFinishedPanel, wxID_ANY, /*"Filament"*/_("common_print_previewpopup_filament"));
		usedFText->SetBackgroundColour(m_dialogColor);
		usedFText->SetForegroundColour(m_textDarkColor);
		usedFText->SetFont(ANKER_FONT_NO_1);
		usedFText->Fit();
		filamentSizer->Add(usedFText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

		filamentSizer->AddStretchSpacer(1);

		m_pFinishedFilamentText = new wxStaticText(m_pFinishedPanel, wxID_ANY, "220g");
		m_pFinishedFilamentText->SetBackgroundColour(m_dialogColor);
		m_pFinishedFilamentText->SetForegroundColour(m_textLightColor);
		m_pFinishedFilamentText->SetFont(ANKER_BOLD_FONT_NO_1);
		m_pFinishedFilamentText->Fit();
		filamentSizer->Add(m_pFinishedFilamentText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
	}

	pFinishedVSizer->AddStretchSpacer(1);

	// button sizer
	wxBoxSizer* buttonHSizer = new wxBoxSizer(wxHORIZONTAL);
	pFinishedVSizer->Add(buttonHSizer, 0, wxALIGN_BOTTOM | wxBOTTOM, 30);

	buttonHSizer->AddStretchSpacer(1);

	m_pRePrintBtn = new AnkerBtn(m_pFinishedPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pRePrintBtn->SetMinSize(AnkerSize(104, 24));
	m_pRePrintBtn->SetMaxSize(AnkerSize(104, 24));
	m_pRePrintBtn->SetText(/*L"Reprint"*/_("common_print_popupfinished_buttonreprint"));
	m_pRePrintBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pRePrintBtn->SetRadius(4);
	m_pRePrintBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pRePrintBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pRePrintBtn->SetBackRectColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	m_pRePrintBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnPrintBtn, this);
	buttonHSizer->Add(m_pRePrintBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

	m_pFinishBtn = new AnkerBtn(m_pFinishedPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pFinishBtn->SetMinSize(AnkerSize(104, 24));
	m_pFinishBtn->SetMaxSize(AnkerSize(104, 24));
	m_pFinishBtn->SetText(/*L"Finish"*/_("common_print_popupfinished_buttonfinish"));
	m_pFinishBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pFinishBtn->SetRadius(4);
	m_pFinishBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pFinishBtn->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pFinishBtn->SetBackRectColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	m_pFinishBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnFinishBtn, this);
	buttonHSizer->Add(m_pFinishBtn, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT, 30);

	buttonHSizer->AddStretchSpacer(1);

	return true;
}

void AnkerGCodeImportDialog::switch2FSMode(FileSelectMode mode)
{
	if (m_currentMode != mode)
	{
		m_pComputerBtn->SetBackgroundColour(m_dialogColor);
		m_pComputerBtn->SetTextColor(m_textDarkColor);
		m_pComputerBtn->SetFont(ANKER_FONT_NO_1);
		m_pStorageBtn->SetBackgroundColour(m_dialogColor);
		m_pStorageBtn->SetTextColor(m_textDarkColor);
		m_pStorageBtn->SetFont(ANKER_FONT_NO_1);
		m_pUSBBtn->SetBackgroundColour(m_dialogColor);
		m_pUSBBtn->SetTextColor(m_textDarkColor);
		m_pUSBBtn->SetFont(ANKER_FONT_NO_1);

		m_pFSComputerWidget->Show(false);
		m_pFSEmptyWidget->Show(false);
		m_pFSStorageListWidget->Show(false);
		m_pFSUSBListWidget->Show(false);

		m_pSearchTextCtrl->Clear();

		switch (mode)
		{
		case AnkerGCodeImportDialog::FSM_COMPUTER:
			m_pComputerBtn->SetBackgroundColour(m_btnFocusColor);
			m_pComputerBtn->SetTextColor(m_btnFocusTextColor);
			m_pComputerBtn->SetFont(ANKER_BOLD_FONT_NO_1);
			m_pFSComputerWidget->Show(true);
			break;
		case AnkerGCodeImportDialog::FSM_STORAGE:
			m_pStorageBtn->SetBackgroundColour(m_btnFocusColor);
			m_pStorageBtn->SetTextColor(m_btnFocusTextColor);
			m_pStorageBtn->SetFont(ANKER_BOLD_FONT_NO_1);
			m_pFSEmptyWidget->Show(true);
			break;
		case AnkerGCodeImportDialog::FSM_USB:
			m_pUSBBtn->SetBackgroundColour(m_btnFocusColor);
			m_pUSBBtn->SetTextColor(m_btnFocusTextColor);
			m_pUSBBtn->SetFont(ANKER_BOLD_FONT_NO_1);
			m_pFSEmptyWidget->Show(true);
			break;
		default:
			break;
		}	

		m_currentMode = mode;

		Layout();
		Refresh();
	}
}

void AnkerGCodeImportDialog::setLoadingVisible(bool visible)
{
	m_pComputerBtn->Enable(!visible);
	m_pStorageBtn->Enable(!visible);
	m_pUSBBtn->Enable(!visible);
	m_pSearchTextCtrl->Enable(!visible);

	// loading frame
	if (m_pLoadingMask == nullptr)
	{
		m_pLoadingMask = new AnkerLoadingMask(this, 15000, false);
		m_pLoadingMask->setText("");
		m_pLoadingMask->Bind(wxANKEREVT_LOADING_TIMEOUT, &AnkerGCodeImportDialog::OnLoadingTimeout, this);
	}

	int x, y;
	GetScreenPosition(&x, &y);
	y += 51;
	x += 4;

	wxSize clientSize = GetClientSize();
	clientSize.x -= 5;
	clientSize.y -= 50;

	m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
	m_pLoadingMask->Show(visible);
	if (visible)
		m_pLoadingMask->start();
	else 
		m_pLoadingMask->stop();
}

void AnkerGCodeImportDialog::OnComputerBtn(wxCommandEvent& event)
{
	switch2FSMode(FSM_COMPUTER);
}

void AnkerGCodeImportDialog::OnStorageBtn(wxCommandEvent& event)
{
	switch2FSMode(FSM_STORAGE);

	// update file list
	m_pFSStorageListWidget->GetSizer()->Clear();

	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);
	
	if (currentDev == nullptr)
		return;

	MqttType::FileList fileList;
	currentDev->getDeviceLocalFileLists(fileList);

	m_pFSStorageListWidget->Show(false);
	m_pFSUSBListWidget->Show(false);
	m_pFSEmptyWidget->Show(false);
	Layout();
	Refresh();

	setLoadingVisible(true);
	m_fileListUpdateReq = true;
}

void AnkerGCodeImportDialog::OnUSBBtn(wxCommandEvent& event)
{
	switch2FSMode(FSM_USB);

	// update file list
	m_pFSUSBListWidget->GetSizer()->Clear();

	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

	if (currentDev == nullptr)
		return;

	MqttType::FileList fileList;
	currentDev->getDeviceUsbFileLists(fileList);

	m_pFSStorageListWidget->Show(false);
	m_pFSUSBListWidget->Show(false);
	m_pFSEmptyWidget->Show(false);
	Layout();
	Refresh();

	setLoadingVisible(true);
	m_fileListUpdateReq = true;
}

void AnkerGCodeImportDialog::OnSearchBtn(wxCommandEvent& event)
{
}

void AnkerGCodeImportDialog::OnSearchTextChanged(wxCommandEvent& event)
{
	if (m_gfItemList.size() > 0)
	{
		wxString targetText = m_pSearchTextCtrl->GetLineText(0);

		for (int i = 0; i < m_gfItemList.size(); i++)
		{
			wxString fileName = m_gfItemList[i]->getFileName();

			bool visible = false;
			int searchLen = fileName.size() - targetText.size();
			for (int j = 0; j <= searchLen; j++)
			{
				int k = 0;
				int l = j;
				for (; k < targetText.size(); k++, l++)
				{
					if (fileName.at(l) != targetText.at(k))
						break;
				}

				if (k == targetText.size())
				{
					visible = true;
					break;
				}
			}

			m_gfItemList[i]->Show(visible);
		}

		Layout();
		Refresh();
	}
}

void AnkerGCodeImportDialog::OnSearchTextEnter(wxCommandEvent& event)
{
}

void AnkerGCodeImportDialog::OnFileItemClicked(wxMouseEvent& event)
{
	if (m_gcodeInfoReq)
		return;

	m_gcodePreviewWaiting = false;

	AnkerGCodeFileItem* item = dynamic_cast<AnkerGCodeFileItem*>(event.GetEventObject());

	if (item == nullptr)
		return;

	m_importResult.m_srcType = m_currentMode;

	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

	if (currentDev == nullptr)
		return;

	wxString filePath = item->getFilePath().ToStdString();

	m_gcodeInfoFilePath = filePath.ToStdString();
	std::string filePathUTF8 = filePath.ToUTF8().data();
	currentDev->setRequestGCodeInfo(filePathUTF8);

	m_gcodeInfoReq = true;
	setLoadingVisible(true);
}

void AnkerGCodeImportDialog::OnShow(wxShowEvent& event)
{
	m_gcodePreviewWaiting = false;

	if (!event.IsShown())
		setLoadingVisible(false);
}

void AnkerGCodeImportDialog::OnLoadingTimeout(wxCommandEvent& event)
{
	// TODO
	if (m_fileListUpdateReq)
	{

	}
	else if (m_gcodeInfoReq)
	{

	}

	setLoadingVisible(false);
	m_fileListUpdateReq = false;
	m_gcodeInfoReq = false;

	ANKER_LOG_INFO << "AnkerGCodeImportDialog: OnLoadingTimeout";

	std::string levelReminder = /*"Failed to connect to the machine"*/_("common_print_filepathnotice_loadingfail").ToStdString();
	std::string title = /*"Error"*/_("common_print_taskpannelfinished_failed").ToStdString();
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, false);
}

void AnkerGCodeImportDialog::OnMove(wxMoveEvent& event)
{
	if (m_pLoadingMask && m_pLoadingMask->IsShown())
	{
		int x, y;
		GetScreenPosition(&x, &y);
		y += 71;
		x += 8;

		wxSize clientSize = GetClientSize();
		clientSize.y -= 40;

		m_pLoadingMask->updateMaskRect(wxPoint(x, y), clientSize);
	}
}

void AnkerGCodeImportDialog::OnComputerImportBtn(wxCommandEvent& event)
{
	wxFileDialog openFileDialog(this, (/*"Open GCode File"*/_("common_print_filepath_computernotice")), m_localImportDefaultDir, "",
		"GCode files (*.gcode;*.acode)|*.gcode;*.acode", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	m_importResult.m_srcType = m_currentMode;
	wxString resultOriFilePath = openFileDialog.GetPath();
	std::string resultFilePath = resultOriFilePath.ToUTF8().data();
	// do not set the UTF data to wxString which will let wxString be empty
	//wxString resultFilePath = wxString(fileNameT);
	PrintLog("AnkerGCodeImportDialog ===== import gcode file path: " + resultOriFilePath.ToStdString());
	// filepath may contains special characters and couldn't be converted to unicode
    if (!resultOriFilePath.empty() && resultFilePath.empty()) {
        std::string title = /*"AnkerSlicer Warning"*/_("common_popup_titlenotice").ToStdString();
        std::string content = /*"The file path may contain special characters like '+#%-' that cannot be converted to unicode,so it will be ignored"*/_("common_print_filepath_errorfile").ToStdString();
        AnkerMessageBox(nullptr, content, title, false);
    }

    switch2FileInfo(resultFilePath, std::string(resultOriFilePath.mb_str()));

	m_localImportDefaultDir = resultOriFilePath.substr(0, resultOriFilePath.find_last_of("\\")).ToStdString();

	Slic3r::GCodeProcessor processor;
	Slic3r::GCodeProcessorResultExt out;
	// we still open the file which filepath may contains special characters
	processor.process_file_ext(resultOriFilePath.ToUTF8().data(), out);

	// get the print info from gcode
	// set the info to dialog

	//setFileInfoSpeed(out.speed);
	setFileInfoFilament(out.filament_cost.empty() ? "--" : out.filament_cost);
	setFileInfoTime(out.print_time);

	wxImage image;
	if (!out.base64_str.empty())
	{
		image = Slic3r::GCodeThumbnails::base64ToImage<wxImage, wxMemoryInputStream>(out.base64_str);
	}

	if (!image.IsOk())
	{
		wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
		image = bitmapEx.ConvertToImage();
	}

	setPreviewImage(image.GetWidth(), image.GetHeight(), image.GetData());
}

void AnkerGCodeImportDialog::OnPrintBtn(wxCommandEvent& event)
{
	EndModal(wxOK);
	Hide();
}

void AnkerGCodeImportDialog::OnFinishBtn(wxCommandEvent& event)
{
	EndModal(wxCANCEL);
	Hide();
}

AnkerGCodeFileItem::AnkerGCodeFileItem(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
	, m_enterChildFlag(false)
	, m_fileName("")
	, m_filePath("")
	, m_fileTimeInfo("")
	, m_pFileNameText(nullptr)
	, m_pFilePathText(nullptr)
	, m_pFileTimeText(nullptr)
{
	initUI();

	Bind(wxEVT_ENTER_WINDOW, &AnkerGCodeFileItem::OnMouseEnterWindow, this);
	Bind(wxEVT_LEAVE_WINDOW, &AnkerGCodeFileItem::OnMouseLeaveWindow, this);
	Bind(wxEVT_PAINT, &AnkerGCodeFileItem::OnPaint, this);

	m_fileImage = wxImage(wxString::FromUTF8(Slic3r::var("file_icon_50x50.png")), wxBITMAP_TYPE_PNG);
	m_fileImage.Rescale(20, 20);
}

AnkerGCodeFileItem::~AnkerGCodeFileItem()
{
}

void AnkerGCodeFileItem::setFileName(wxString filename)
{
	m_fileName = filename;

	if (m_pFileNameText)
		m_pFileNameText->SetLabelText(filename);
}

void AnkerGCodeFileItem::setFilePath(wxString filepath)
{
	m_filePath = filepath;

	if (filepath.size() > 40)
		filepath = filepath.substr(0, 40) + "...";

	m_filePathRenderStr = filepath;

	if (m_pFilePathText)
		m_pFilePathText->SetLabelText(filepath);
}

void AnkerGCodeFileItem::setFileTimeStr(wxString time)
{
	m_fileTimeInfo = time;

	if (m_pFileTimeText)
		m_pFileTimeText->SetLabelText(time);
}

wxString AnkerGCodeFileItem::getFileName()
{
	return m_fileName;
}

wxString AnkerGCodeFileItem::getFilePath()
{
	return m_filePath;
}

wxString AnkerGCodeFileItem::getFileTimeStr()
{
	return m_fileTimeInfo;
}

void AnkerGCodeFileItem::initUI()
{
	SetBackgroundColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	SetMinSize(AnkerSize(324, 88));
	SetMaxSize(AnkerSize(324, 88));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerGCodeFileItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxColour bgColor = GetBackgroundColour();

	wxRect rect = GetClientRect();
	wxBrush brush(bgColor);
	wxPen pen(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	dc.SetBrush(brush);
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

	// draw line
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(255, 255, 255).ChangeLightness(30));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextForeground(wxColour(255, 255, 255));
		wxPoint startPoint = wxPoint(2, 1);
		wxPoint endPoint = wxPoint(321, 1);
		dc.DrawLine(startPoint, endPoint);
	}

	// draw image
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(255, 255, 255));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetTextForeground(wxColour(255, 255, 255));
		wxPoint imagePos(16, 16);
		dc.DrawBitmap(m_fileImage, imagePos);
	}

	// draw file name
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(255, 255, 255));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_BOLD_FONT_NO_1);
		dc.SetTextForeground(wxColour(255, 255, 255));
		wxPoint textPoint = wxPoint(42, 13);
		dc.DrawText(m_fileName, textPoint);
	}

	// draw file path
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(184, 184, 186));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_FONT_NO_2);
		dc.SetTextForeground(wxColour(184, 184, 186));
		wxPoint textPoint = wxPoint(42, 39);
		dc.DrawText(m_filePathRenderStr, textPoint);
	}

	// draw file time
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(184, 184, 186));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(ANKER_FONT_NO_2);
		dc.SetTextForeground(wxColour(184, 184, 186));
		wxPoint textPoint = wxPoint(42, 61);
		dc.DrawText(m_fileTimeInfo, textPoint);
	}
}

void AnkerGCodeFileItem::OnMouseEnterWindow(wxMouseEvent& event)
{
	SetBackgroundColour(wxColour(77, 78, 82));
	Refresh();
}

void AnkerGCodeFileItem::OnMouseLeaveWindow(wxMouseEvent& event)
{
	SetBackgroundColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
	Refresh();
}

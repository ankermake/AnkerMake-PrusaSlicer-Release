#include "AnkerGCodeImportDialog.hpp"
#include "AnkerBtn.hpp"
#include "AnkerMsgDialog.hpp"
#include "AnkerLoadingMask.hpp"
#include "libslic3r/Utils.hpp"
 #include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/Thumbnails.hpp"
#ifdef _WIN32
#include <dbt.h>
#include <shlobj.h>
#endif // _WIN32

#include <time.h>
#include <chrono>


wxDEFINE_EVENT(wxCUSTOMEVT_FILEITEM_CLICKED, wxCommandEvent);

AnkerGCodeImportDialog::AnkerGCodeImportDialog(std::string currentDeviceSn, wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize)
	, m_currentDeviceSn(currentDeviceSn)
	, m_localImportDefaultDir(".")
	, m_pFileSelectVSizer(nullptr)
	, m_pFileInfoVSizer(nullptr)
	, m_pFinishedVSizer(nullptr)
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
	m_dialogColor = wxColour(41, 42, 45);
	m_textLightColor = wxColour(255, 255, 255);
	m_textDarkColor = wxColour(183, 183, 183);
	m_btnFocusColor = wxColour(69, 102, 74);
	m_btnFocusTextColor = wxColour("#62D361");

	initUI();

	Bind(wxEVT_SHOW, &AnkerGCodeImportDialog::OnShow, this);

	m_pRequestTimer = new wxTimer();
	m_pRequestTimer->Bind(wxEVT_TIMER, &AnkerGCodeImportDialog::OnTimer, this);

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

void AnkerGCodeImportDialog::requestCallback()
{
	PrintLog("AnkerGCodeImportDialog::OnRequestReply...");

	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

	if (!currentDev)
		return;

	if (currentDev->currentCmdType == mqtt_command_type_e::MQTT_CMD_FILE_LIST_REQUEST)
	{
		m_fileListUpdateReq = false;

		MqttType::FileList& fileList = currentDev->deviceFileList;
		wxScrolledWindow* fileListWidget = m_currentMode == FSM_STORAGE ? m_pFSStorageListWidget : m_pFSUSBListWidget;
		wxScrolledWindow* anotherListWidget = m_currentMode == FSM_STORAGE ? m_pFSUSBListWidget : m_pFSStorageListWidget;
		if (fileList.files.size() > 0)
		{
			fileListWidget->GetSizer()->Clear();
			fileListWidget->DestroyChildren();
			m_gfItemList.clear();

			for (auto itr = fileList.files.begin(); itr != fileList.files.end(); itr++)
			{
				AnkerGCodeFileItem* item = new AnkerGCodeFileItem(fileListWidget);
				item->setFileName(wxString::FromUTF8(itr->name).ToStdString());
				item->setFilePath(wxString::FromUTF8(itr->path).ToStdString());
				time_t time = itr->timestamp;
				struct tm* timeP = gmtime(&time);
				time_t PTime = time + (8 * 60 * 60);
				timeP = gmtime(&time);
				std::string timeStr = std::to_string(timeP->tm_year + 1900) + "."
					+ std::to_string(timeP->tm_mon + 1) + "."
					+ std::to_string(timeP->tm_mday) + " "
					+ std::to_string(timeP->tm_hour) + ":"
					+ std::to_string(timeP->tm_min);
				item->setFileTimeStr(timeStr);
				item->Bind(wxEVT_LEFT_UP, &AnkerGCodeImportDialog::OnFileItemClicked, this);
				fileListWidget->GetSizer()->Add(item, 0, wxEXPAND | wxALIGN_LEFT, 0);

				m_gfItemList.push_back(item);
			}

			fileListWidget->Show(true);
			anotherListWidget->Show(false);
			m_pFSEmptyWidget->Show(false);
		}
		else
		{
			fileListWidget->Show(false);
			anotherListWidget->Show(false);
			m_pFSEmptyWidget->Show(true);
		}

		Layout();

		setLoadingVisible(false);
	}
	else if (currentDev->currentCmdType == mqtt_command_type_e::MQTT_CMD_GCODE_FILE_REQUEST)
	{
		m_gcodeInfoReq = false;

		auto targetMapItr = dm.getMqttDataMap()->find(m_currentDeviceSn);
		if (targetMapItr != dm.getMqttDataMap()->end())
		{
			auto targetItr = targetMapItr->second.find(MQTT_CMD_GCODE_FILE_REQUEST);
			if (targetItr != targetMapItr->second.end())
			{
				CmdType* targetData = targetItr->second.get();
				GCodeInfo* gcodeInfo = static_cast<GCodeInfo*>(targetData);

				setFileInfoSpeed(std::to_string(gcodeInfo->speed) + "mm/s");
				setFileInfoFilament(std::to_string(gcodeInfo->filamentUsed) + gcodeInfo->filamentUnit);
				int leftTime = gcodeInfo->leftTime;
				setFileInfoTime(leftTime);

				// request image from server: gcodeInfo->completeUrl
				wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
				setPreviewImage(image.GetWidth(), image.GetHeight(), image.GetData());

				switch2FileInfo(m_gcodeInfoFilePath);
			}
		}

		m_gcodeInfoFilePath = "";

		setLoadingVisible(false);

		Layout();
		Refresh();

		m_gcodePreviewWaiting = true;
	}
	else if (m_gcodePreviewWaiting && currentDev->currentCmdType == mqtt_command_type_e::MQTT_CMD_THUMBNAIL_UPLOAD_NOTICE && !currentDev->thumbnail.empty())
	{
		GCodeInfoPtr gcodeInfo = currentDev->getGcodeInfo();
		std::string url = currentDev->thumbnail;
		wxString filePath = std::string();


		wxStandardPaths standarPaths = wxStandardPaths::Get();
		filePath = standarPaths.GetUserDataDir();
		// Todo: check the file status when the cache is existed
		filePath = filePath + "/cache/" + gcodeInfo->fileName + ".png";

#ifndef __APPLE__
		filePath.Replace("\\", "/");
#endif       

		//if file not exists
		std::string filePathStr = wxString::FromUTF8(filePath.ToStdString()).ToStdString();
		if (!wxFileExists(filePathStr) && url.size() > 0) {
			std::vector<std::string> headerList;
			Datamanger::GetInstance().setHeaderList(headerList);
			Datamanger::GetInstance().getAnkerNetBase()->AsyDownLoad(
				url,
				filePathStr,
				headerList,
				this,
				onDownLoadFinishedCallBack,
				onDownLoadProgress, true);
		}

		wxImage image = wxImage(wxString::FromUTF8(/*Slic3r::var("gcode_image_sample.png")*/filePath.ToStdString()), wxBITMAP_TYPE_PNG);
		setPreviewImage(image.GetWidth(), image.GetHeight(), image.GetData());

		setLoadingVisible(false);

		Layout();
		Refresh();

		m_gcodePreviewWaiting = false;
	}
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

	m_pSpeedText->SetLabelText(speedStr);

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

	wxBitmap scaledBitmap(image);
	m_pPreviewImage->SetBitmap(scaledBitmap);
	m_pPreviewImage->SetMinSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetMaxSize(scaledBitmap.GetSize());

	m_importResult.m_previewImage = image;
}

void AnkerGCodeImportDialog::switch2FileSelect(FileSelectMode mode)
{
	m_pFileSelectVSizer->Show(true);
	m_pFileInfoVSizer->Show(false);
	m_pFinishedVSizer->Show(false);

	SetTitle("Select File");
	SetSizer(m_pFileSelectVSizer, false);

	switch2FSMode(mode);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::switch2FileInfo(std::string filepath, std::string strTitleName)
{
	m_pFileSelectVSizer->Show(false);
	m_pFileInfoVSizer->Show(true);
	m_pFinishedVSizer->Show(false);

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
	SetTitle(wxFileName);
	SetSizer(m_pFileInfoVSizer, false);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::switch2PrintFinished(bool success, GCodeImportResult& result)
{
	m_pFileSelectVSizer->Show(false);
	m_pFileInfoVSizer->Show(false);
	m_pFinishedVSizer->Show(true);

	m_importResult = result;

	SetTitle(result.m_fileName);
	SetSizer(m_pFinishedVSizer, false);

	if (success)
	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(120, 120);
		m_pFinishedStatusImage->SetBitmap(image);
		m_pFinishedStatusImage->SetMinSize(image.GetSize());
		m_pFinishedStatusImage->SetMaxSize(image.GetSize());

		m_pFinishedStatusText->SetLabelText(L"Print Success");
	}
	else
	{
		wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_failed_icon.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(120, 120);
		m_pFinishedStatusImage->SetBitmap(image);
		m_pFinishedStatusImage->SetMinSize(image.GetSize());
		m_pFinishedStatusImage->SetMaxSize(image.GetSize());

		m_pFinishedStatusText->SetLabelText(L"Print Failed");
	}

	m_pFinishedFilamentText->SetLabelText(result.m_filamentStr);

	int seconds = result.m_timeSecond;
	int hours = seconds / 60 / 60;
	int minutes = seconds / 60 % 60;
	std::string newTime = std::to_string(hours) + "hs" + std::to_string(minutes) + "mins";
	m_pFinishedPrintTimeText->SetLabelText(newTime);

	Layout();
	Refresh();
}

void AnkerGCodeImportDialog::initUI()
{
	SetBackgroundColour(m_dialogColor);
	SetSize(wxSize(324, 531));

	// File Select Sizer
	{
		m_pFileSelectVSizer = new wxBoxSizer(wxVERTICAL);

		m_pFileSelectVSizer->AddSpacer(9);

		{
			wxBoxSizer* buttonTabSizer = new wxBoxSizer(wxHORIZONTAL);
			m_pFileSelectVSizer->Add(buttonTabSizer, 0, wxALIGN_CENTER, 0);

			m_pComputerBtn = new wxButton(this, wxID_ANY, "Computer", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			m_pComputerBtn->SetBackgroundColour(m_dialogColor);
			m_pComputerBtn->SetForegroundColour(m_textDarkColor);
			m_pComputerBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnComputerBtn, this);
			buttonTabSizer->Add(m_pComputerBtn, 0, wxEXPAND, 0);

			m_pStorageBtn = new wxButton(this, wxID_ANY, "Storage", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			m_pStorageBtn->SetBackgroundColour(m_dialogColor);
			m_pStorageBtn->SetForegroundColour(m_textDarkColor);
			m_pStorageBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnStorageBtn, this);
			buttonTabSizer->Add(m_pStorageBtn, 0, wxEXPAND, 0);

			m_pUSBBtn = new wxButton(this, wxID_ANY, "USB", wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			m_pUSBBtn->SetBackgroundColour(m_dialogColor);
			m_pUSBBtn->SetForegroundColour(m_textDarkColor);
			m_pUSBBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnUSBBtn, this);
			buttonTabSizer->Add(m_pUSBBtn, 0, wxEXPAND, 0);
		}

		{
			m_pFileSelectVSizer->AddSpacer(8);

			wxControl* frameBox = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
			frameBox->SetBackgroundColour(m_dialogColor);
			frameBox->SetForegroundColour(m_textDarkColor);
			frameBox->SetMinSize(wxSize(276, 24));
			frameBox->SetMaxSize(wxSize(276, 24));
			m_pFileSelectVSizer->Add(frameBox, 0, wxALIGN_CENTER, 0);

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

			m_pSearchTextCtrl = new wxTextCtrl(frameBox, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
			m_pSearchTextCtrl->SetMinSize(wxSize(270, 24));
			m_pSearchTextCtrl->SetBackgroundColour(m_dialogColor);
			m_pSearchTextCtrl->SetForegroundColour(m_textLightColor);
			m_pSearchTextCtrl->Bind(wxEVT_TEXT, &AnkerGCodeImportDialog::OnSearchTextChanged, this);
			m_pSearchTextCtrl->Bind(wxEVT_TEXT_ENTER, &AnkerGCodeImportDialog::OnSearchTextEnter, this);
			m_pSearchTextCtrl->SetEditable(true);
			searchSizer->Add(m_pSearchTextCtrl, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_BOTTOM, 0);
		}

		m_pFileSelectVSizer->AddSpacer(5);

		m_pFSComputerWidget = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSComputerWidget->SetMinSize(wxSize(324, 422));
		m_pFSComputerWidget->SetScrollRate(324, 60);
		m_pFileSelectVSizer->Add(m_pFSComputerWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSEmptyWidget = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSEmptyWidget->SetMinSize(wxSize(324, 422));
		m_pFSEmptyWidget->SetScrollRate(324, 60);
		m_pFileSelectVSizer->Add(m_pFSEmptyWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSStorageListWidget = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSStorageListWidget->SetMinSize(wxSize(324, 422));
		m_pFSStorageListWidget->SetScrollRate(324, 60);
		m_pFileSelectVSizer->Add(m_pFSStorageListWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		m_pFSUSBListWidget = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxVSCROLL);
		m_pFSUSBListWidget->SetMinSize(wxSize(324, 422));
		m_pFSUSBListWidget->SetScrollRate(324, 60);
		m_pFileSelectVSizer->Add(m_pFSUSBListWidget, 0, wxEXPAND | wxALIGN_CENTER, 0);

		initFSComputerSizer(m_pFSComputerWidget);
		initFSEmptySizer(m_pFSEmptyWidget);
		initFSListSizer(m_pFSStorageListWidget);
		initFSListSizer(m_pFSUSBListWidget);

		m_pFSEmptyWidget->Hide();
		m_pFSStorageListWidget->Hide();
		m_pFSUSBListWidget->Hide();
	}

	// File Info Sizer
	initFileInfoSizer(this);

	initFinishedSizer(this);

	switch2FileSelect(FSM_COMPUTER);
}

bool AnkerGCodeImportDialog::initFSComputerSizer(wxWindow* parent)
{
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
	wxStaticText* tipText = new wxStaticText(parent, wxID_ANY, "Click to open the file.");
	tipText->SetBackgroundColour(m_dialogColor);
	tipText->SetForegroundColour(m_textDarkColor);
	wxFont font = tipText->GetFont();
	font.SetPointSize(10);
	tipText->SetFont(font);
	computerSizer->Add(tipText, 0, wxALIGN_CENTER, 0);

	computerSizer->AddSpacer(32);

	// open button
	AnkerBtn* openButton = new AnkerBtn(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	openButton->SetMinSize(wxSize(184, 24));
	openButton->SetMaxSize(wxSize(184, 24));
	openButton->SetText(L"Open");
	openButton->SetBackgroundColour(wxColor("#62D361"));
	openButton->SetRadius(4);
	openButton->SetTextColor(wxColor("#FFFFFF"));
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
	wxStaticText* tipText = new wxStaticText(parent, wxID_ANY, "No Files");
	tipText->SetBackgroundColour(m_dialogColor);
	tipText->SetForegroundColour(m_textDarkColor);
	wxFont font = tipText->GetFont();
	font.SetPointSize(10);
	tipText->SetFont(font);
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
	m_pFileInfoVSizer = new wxBoxSizer(wxVERTICAL);

	m_pFileInfoVSizer->AddSpacer(45);

	// preview image
	wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
	wxImage image = bitmapEx.ConvertToImage();
	image.Rescale(160, 160);

	wxBitmap scaledBitmap(image);
	m_pPreviewImage = new wxStaticBitmap(parent, wxID_ANY, scaledBitmap);
	m_pPreviewImage->SetMinSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetMaxSize(scaledBitmap.GetSize());
	m_pPreviewImage->SetBackgroundColour(wxColour(41, 42, 45));
	m_pFileInfoVSizer->Add(m_pPreviewImage, 0, wxALIGN_CENTER, 0);

	m_pFileInfoVSizer->AddSpacer(44);

	// split line
	wxControl* splitLineCtrl = new wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(64, 65, 70));
	splitLineCtrl->SetMaxSize(wxSize(264, 1));
	splitLineCtrl->SetMinSize(wxSize(264, 1));
	m_pFileInfoVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);

	m_pFileInfoVSizer->AddSpacer(16);

	// speed
	{
		wxBoxSizer* speedSizer = new wxBoxSizer(wxHORIZONTAL);
		speedSizer->SetMinSize(wxSize(264, 34));
		m_pFileInfoVSizer->Add(speedSizer, 0, wxALIGN_CENTER, 0);

		wxBitmap speedBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("printer_speed.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* speedIcon = new wxStaticBitmap(parent, wxID_ANY, speedBitmap);
		speedIcon->SetMinSize(speedBitmap.GetSize());
		speedSizer->Add(speedIcon, 0, wxALIGN_LEFT, 0);

		speedSizer->AddSpacer(7);

		wxStaticText* speedText = new wxStaticText(parent, wxID_ANY, "Speed");
		speedText->SetBackgroundColour(m_dialogColor);
		speedText->SetForegroundColour(m_textDarkColor);
		wxFont font = speedText->GetFont();
		font.SetPointSize(10);
		speedText->SetFont(font);
		speedSizer->Add(speedText, 0, wxALIGN_LEFT, 0);

		speedSizer->AddStretchSpacer(1);

		m_pSpeedText = new wxStaticText(parent, wxID_ANY, "X5.0(250mm/s)");
		m_pSpeedText->SetBackgroundColour(m_dialogColor);
		m_pSpeedText->SetForegroundColour(m_textDarkColor);
		font = m_pSpeedText->GetFont();
		font.SetPointSize(10);
		m_pSpeedText->SetFont(font);
		speedSizer->Add(m_pSpeedText, 0, wxALIGN_RIGHT, 0);
	}

	m_pFileInfoVSizer->AddSpacer(16);

	// filament
	{
		wxBoxSizer* filamentSizer = new wxBoxSizer(wxHORIZONTAL);
		filamentSizer->SetMinSize(wxSize(264, 34));
		m_pFileInfoVSizer->Add(filamentSizer, 0, wxALIGN_CENTER, 0);

		wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(parent, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		filamentSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		filamentSizer->AddSpacer(7);

		wxStaticText* usedFText = new wxStaticText(parent, wxID_ANY, "Filament");
		usedFText->SetBackgroundColour(m_dialogColor);
		usedFText->SetForegroundColour(m_textDarkColor);
		wxFont font = usedFText->GetFont();
		font.SetPointSize(10);
		usedFText->SetFont(font);
		filamentSizer->Add(usedFText, 0, wxALIGN_LEFT, 0);

		filamentSizer->AddStretchSpacer(1);

		m_pFilamentText = new wxStaticText(parent, wxID_ANY, "220g");
		m_pFilamentText->SetBackgroundColour(m_dialogColor);
		m_pFilamentText->SetForegroundColour(m_textDarkColor);
		font = m_pFilamentText->GetFont();
		font.SetPointSize(10);
		m_pFilamentText->SetFont(font);
		filamentSizer->Add(m_pFilamentText, 0, wxALIGN_RIGHT, 0);
	}

	m_pFileInfoVSizer->AddSpacer(16);

	// print time
	{
		wxBoxSizer* printTimeSizer = new wxBoxSizer(wxHORIZONTAL);
		printTimeSizer->SetMinSize(wxSize(264, 34));
		m_pFileInfoVSizer->Add(printTimeSizer, 0, wxALIGN_CENTER, 0);

		wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(parent, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		printTimeSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		printTimeSizer->AddSpacer(7);

		wxStaticText* printTimeText = new wxStaticText(parent, wxID_ANY, "Print Time");
		printTimeText->SetBackgroundColour(m_dialogColor);
		printTimeText->SetForegroundColour(m_textDarkColor);
		wxFont font = printTimeText->GetFont();
		font.SetPointSize(10);
		printTimeText->SetFont(font);
		printTimeSizer->Add(printTimeText, 0, wxALIGN_LEFT, 0);

		printTimeSizer->AddStretchSpacer(1);

		m_pPrintTimeText = new wxStaticText(parent, wxID_ANY, "1h32mins");
		m_pPrintTimeText->SetBackgroundColour(m_dialogColor);
		m_pPrintTimeText->SetForegroundColour(m_textDarkColor);
		font = m_pPrintTimeText->GetFont();
		font.SetPointSize(10);
		m_pPrintTimeText->SetFont(font);
		printTimeSizer->Add(m_pPrintTimeText, 0, wxALIGN_RIGHT, 0);
	}

	m_pFileInfoVSizer->AddStretchSpacer(1);

	// button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pFileInfoVSizer->Add(buttonSizer, 1, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 30);

	buttonSizer->AddStretchSpacer(1);

	m_pPrintBtn = new AnkerBtn(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pPrintBtn->SetMinSize(wxSize(104, 24));
	m_pPrintBtn->SetMaxSize(wxSize(264, 24));
	m_pPrintBtn->SetText(L"Start Printing");
	m_pPrintBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pPrintBtn->SetRadius(4);
	m_pPrintBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pPrintBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnPrintBtn, this);
	buttonSizer->Add(m_pPrintBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

	buttonSizer->AddStretchSpacer(1);

	return true;
}

bool AnkerGCodeImportDialog::initFinishedSizer(wxWindow* parent)
{
	m_pFinishedVSizer = new wxBoxSizer(wxVERTICAL);

	m_pFinishedVSizer->AddSpacer(40);

	// preview image
	wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
	image.Rescale(120, 120);
	m_pFinishedStatusImage = new wxStaticBitmap(parent, wxID_ANY, image);
	m_pFinishedStatusImage->SetMinSize(image.GetSize());
	m_pFinishedStatusImage->SetMaxSize(image.GetSize());
	m_pFinishedStatusImage->SetBackgroundColour(wxColour(41, 42, 45));
	m_pFinishedVSizer->Add(m_pFinishedStatusImage, 0, wxALIGN_CENTER, 0);

	m_pFinishedStatusText = new wxStaticText(parent, wxID_ANY, "Print Success");
	m_pFinishedStatusText->SetMinSize(wxSize(102, 16));
	m_pFinishedStatusText->SetBackgroundColour(m_dialogColor);
	m_pFinishedStatusText->SetForegroundColour(m_textDarkColor);
	wxFont font = m_pFinishedStatusText->GetFont();
	font.SetPointSize(10);
	m_pFinishedStatusText->SetFont(font);
	m_pFinishedVSizer->Add(m_pFinishedStatusText, 0, wxALIGN_CENTER, 0);

	m_pFinishedVSizer->AddSpacer(36);

	// split line
	wxControl* splitLineCtrl = new wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	splitLineCtrl->SetBackgroundColour(wxColour(64, 65, 70));
	splitLineCtrl->SetMaxSize(wxSize(264, 1));
	splitLineCtrl->SetMinSize(wxSize(264, 1));
	m_pFinishedVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);

	m_pFinishedVSizer->AddSpacer(16);

	// filament
	{
		wxBoxSizer* filamentSizer = new wxBoxSizer(wxHORIZONTAL);
		filamentSizer->SetMinSize(wxSize(264, 34));
		m_pFinishedVSizer->Add(filamentSizer, 0, wxALIGN_CENTER, 0);

		wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(parent, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		filamentSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		filamentSizer->AddSpacer(7);

		wxStaticText* usedFText = new wxStaticText(parent, wxID_ANY, "Filament");
		usedFText->SetBackgroundColour(m_dialogColor);
		usedFText->SetForegroundColour(m_textDarkColor);
		wxFont font = usedFText->GetFont();
		font.SetPointSize(10);
		usedFText->SetFont(font);
		filamentSizer->Add(usedFText, 0, wxALIGN_LEFT, 0);

		filamentSizer->AddStretchSpacer(1);

		m_pFinishedFilamentText = new wxStaticText(parent, wxID_ANY, "220g");
		m_pFinishedFilamentText->SetBackgroundColour(m_dialogColor);
		m_pFinishedFilamentText->SetForegroundColour(m_textDarkColor);
		font = m_pFinishedFilamentText->GetFont();
		font.SetPointSize(10);
		m_pFinishedFilamentText->SetFont(font);
		filamentSizer->Add(m_pFinishedFilamentText, 0, wxALIGN_RIGHT, 0);
	}

	m_pFinishedVSizer->AddSpacer(16);

	// print time
	{
		wxBoxSizer* printTimeSizer = new wxBoxSizer(wxHORIZONTAL);
		printTimeSizer->SetMinSize(wxSize(264, 34));
		m_pFinishedVSizer->Add(printTimeSizer, 0, wxALIGN_CENTER, 0);

		wxBitmap usedFBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
		wxStaticBitmap* usedFIcon = new wxStaticBitmap(parent, wxID_ANY, usedFBitmap);
		usedFIcon->SetMinSize(usedFBitmap.GetSize());
		printTimeSizer->Add(usedFIcon, 0, wxALIGN_LEFT, 0);

		printTimeSizer->AddSpacer(7);

		wxStaticText* printTimeText = new wxStaticText(parent, wxID_ANY, "Print Time");
		printTimeText->SetBackgroundColour(m_dialogColor);
		printTimeText->SetForegroundColour(m_textDarkColor);
		wxFont font = printTimeText->GetFont();
		font.SetPointSize(10);
		printTimeText->SetFont(font);
		printTimeSizer->Add(printTimeText, 0, wxALIGN_LEFT, 0);

		printTimeSizer->AddStretchSpacer(1);

		m_pFinishedPrintTimeText = new wxStaticText(parent, wxID_ANY, "1h32mins");
		m_pFinishedPrintTimeText->SetBackgroundColour(m_dialogColor);
		m_pFinishedPrintTimeText->SetForegroundColour(m_textDarkColor);
		font = m_pFinishedPrintTimeText->GetFont();
		font.SetPointSize(10);
		m_pFinishedPrintTimeText->SetFont(font);
		printTimeSizer->Add(m_pFinishedPrintTimeText, 0, wxALIGN_RIGHT, 0);
	}

	m_pFinishedVSizer->AddStretchSpacer(1);

	// button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pFinishedVSizer->Add(buttonSizer, 1, wxEXPAND | wxALIGN_BOTTOM | wxBOTTOM, 30);

	buttonSizer->AddStretchSpacer(1);

	m_pRePrintBtn = new AnkerBtn(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pRePrintBtn->SetMinSize(wxSize(104, 24));
	m_pRePrintBtn->SetMaxSize(wxSize(264, 24));
	m_pRePrintBtn->SetText(L"Reprint");
	m_pRePrintBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pRePrintBtn->SetRadius(4);
	m_pRePrintBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pRePrintBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnPrintBtn, this);
	buttonSizer->Add(m_pRePrintBtn, 1, wxEXPAND | wxALIGN_CENTER, 0);

	m_pFinishBtn = new AnkerBtn(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pFinishBtn->SetMinSize(wxSize(104, 24));
	m_pFinishBtn->SetMaxSize(wxSize(264, 24));
	m_pFinishBtn->SetText(L"Finish");
	m_pFinishBtn->SetBackgroundColour(wxColor("#62D361"));
	m_pFinishBtn->SetRadius(4);
	m_pFinishBtn->SetTextColor(wxColor("#FFFFFF"));
	m_pFinishBtn->Bind(wxEVT_BUTTON, &AnkerGCodeImportDialog::OnFinishBtn, this);
	buttonSizer->Add(m_pFinishBtn, 1, wxALIGN_CENTER | wxLEFT, 30);

	buttonSizer->AddStretchSpacer(1);

	return true;
}

void AnkerGCodeImportDialog::switch2FSMode(FileSelectMode mode)
{
	if (m_currentMode != mode)
	{
		m_pComputerBtn->SetBackgroundColour(m_dialogColor);
		m_pComputerBtn->SetForegroundColour(m_textDarkColor);
		m_pStorageBtn->SetBackgroundColour(m_dialogColor);
		m_pStorageBtn->SetForegroundColour(m_textDarkColor);
		m_pUSBBtn->SetBackgroundColour(m_dialogColor);
		m_pUSBBtn->SetForegroundColour(m_textDarkColor);

		m_pFSComputerWidget->Show(false);
		m_pFSEmptyWidget->Show(false);
		m_pFSStorageListWidget->Show(false);
		m_pFSUSBListWidget->Show(false);

		m_pSearchTextCtrl->Clear();

		switch (mode)
		{
		case AnkerGCodeImportDialog::FSM_COMPUTER:
			m_pComputerBtn->SetBackgroundColour(m_btnFocusColor);
			m_pComputerBtn->SetForegroundColour(m_btnFocusTextColor);
			m_pFSComputerWidget->Show(true);
			break;
		case AnkerGCodeImportDialog::FSM_STORAGE:
			m_pStorageBtn->SetBackgroundColour(m_btnFocusColor);
			m_pStorageBtn->SetForegroundColour(m_btnFocusTextColor);
			m_pFSEmptyWidget->Show(true);
			break;
		case AnkerGCodeImportDialog::FSM_USB:
			m_pUSBBtn->SetBackgroundColour(m_btnFocusColor);
			m_pUSBBtn->SetForegroundColour(m_btnFocusTextColor);
			m_pFSEmptyWidget->Show(true);
			break;
		default:
			break;
		}	

		m_currentMode = mode;

		Layout();
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
		m_pLoadingMask = new AnkerLoadingMask(this);
		//m_pLoadingMask->SetSize(GetClientSize());
		//m_pLoadingMask->SetPosition(GetScreenPosition() + GetClientAreaOrigin());
	}

	int x, y;
	GetScreenPosition(&x, &y);
	y += 71;
	x += 8;

	wxSize clientSize = GetClientSize();

	m_pLoadingMask->SetSize(GetSize());
	m_pLoadingMask->SetPosition(wxPoint(0, 0)/* + GetClientAreaOrigin()*/);
	m_pLoadingMask->Show(visible);
}

void AnkerGCodeImportDialog::startRequestTimer()
{
	m_pRequestTimer->Start(30000, true);
}

void AnkerGCodeImportDialog::OnComputerBtn(wxCommandEvent& event)
{
	switch2FSMode(FSM_COMPUTER);

	Layout();
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
	m_pFSEmptyWidget->Show(true);
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
	m_pFSEmptyWidget->Show(true);
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
		std::string targetText = m_pSearchTextCtrl->GetLineText(0).ToStdString();

		for (int i = 0; i < m_gfItemList.size(); i++)
		{
			std::string fileName = m_gfItemList[i]->getFileName();

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
	m_gcodePreviewWaiting = false;

	AnkerGCodeFileItem* item = dynamic_cast<AnkerGCodeFileItem*>(event.GetEventObject());

	if (item == nullptr)
		return;

	m_importResult.m_srcType = m_currentMode;

	Datamanger& dm = Datamanger::GetInstance();
	DeviceObjectPtr currentDev = dm.getDeviceObjectFromSn(m_currentDeviceSn);

	if (currentDev == nullptr)
		return;

	std::string fileName = item->getFileName();
	std::string filePath = item->getFilePath();
	std::string fileTime = item->getFileTimeStr();

	m_gcodeInfoFilePath = filePath;
	filePath = wxString(filePath).ToUTF8();
	currentDev->setRequestGCodeInfo(filePath);

	m_gcodeInfoReq = true;
	setLoadingVisible(true);
}

void AnkerGCodeImportDialog::OnShow(wxShowEvent& event)
{
	m_gcodePreviewWaiting = false;

	if (!event.IsShown())
		setLoadingVisible(false);
}

void AnkerGCodeImportDialog::OnTimer(wxTimerEvent& event)
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

	std::string levelReminder = "Failed to connect to the machine";
	std::string title = "Error";
	AnkerMsgDialog::MsgResult result = AnkerMessageBox(nullptr, levelReminder, title, false);
}

void AnkerGCodeImportDialog::OnComputerImportBtn(wxCommandEvent& event)
{
	wxFileDialog openFileDialog(this, ("Open GCode File"), m_localImportDefaultDir, "",
		"GCode files (*.gcode;*.acode)|*.gcode;*.acode", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	m_importResult.m_srcType = m_currentMode;
	wxString resultOriFilePath = openFileDialog.GetPath();
	wxString resultFilePath = resultOriFilePath.ToUTF8();
	PrintLog("AnkerGCodeImportDialog ===== import gcode file path: " + resultOriFilePath.ToStdString() + 
		", unicode fila path:" + resultFilePath.ToStdString());
	// filepath may contains special characters and couldn't be converted to unicode
    if (!resultOriFilePath.empty() && resultFilePath.empty()) {
        std::string title = "AnkerSlicer Warning";
        std::string content = "The file path may contain special characters like '+#%-' that cannot be converted to unicode,so it will be ignored";
        AnkerMessageBox(nullptr, content, title, false);
    }

    switch2FileInfo(resultFilePath.ToStdString(), std::string(resultOriFilePath.mb_str()));

	m_localImportDefaultDir = resultFilePath.substr(0, resultFilePath.find_last_of("\\")).ToStdString();

	Slic3r::GCodeProcessor processor;
	Slic3r::GCodeProcessorResultExt out;
	// we still open the file which filepath may contains special characters
	processor.process_file_ext(resultOriFilePath.ToUTF8().data(), out);
	wxImage image = Slic3r::GCodeThumbnails::base64ToImage<wxImage, wxMemoryInputStream>(out.base64_str);

	// get the print info from gcode
	// set the info to dialog

	setFileInfoSpeed(out.speed);
	setFileInfoFilament(out.filament_cost.empty() ? "--" : out.filament_cost);
	setFileInfoTime(out.print_time);

	if (out.base64_str.empty())
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

void AnkerGCodeFileItem::setFileName(std::string filename)
{
	m_fileName = filename;

	if (m_pFileNameText)
		m_pFileNameText->SetLabelText(filename);
}

void AnkerGCodeFileItem::setFilePath(std::string filepath)
{
	m_filePath = filepath;

	if (filepath.size() > GetSize().y)
		filepath = filepath.substr(0, GetSize().y) + "...";

	if (m_pFilePathText)
		m_pFilePathText->SetLabelText(filepath);
}

void AnkerGCodeFileItem::setFileTimeStr(std::string time)
{
	m_fileTimeInfo = time;

	if (m_pFileTimeText)
		m_pFileTimeText->SetLabelText(time);
}

std::string AnkerGCodeFileItem::getFileName()
{
	return m_fileName;
}

std::string AnkerGCodeFileItem::getFilePath()
{
	return m_filePath;
}

std::string AnkerGCodeFileItem::getFileTimeStr()
{
	return m_fileTimeInfo;
}

void AnkerGCodeFileItem::initUI()
{
	//wxBoxSizer* itemSizer = new wxBoxSizer(wxVERTICAL);
	//SetSizer(itemSizer);
	SetBackgroundColour(wxColour(41, 42, 45));
	SetMinSize(wxSize(324, 88));
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	//// split line
	//wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
	//splitLineCtrl->SetBackgroundColour(wxColour(64, 65, 70));
	//splitLineCtrl->SetMaxSize(wxSize(300, 1));
	//splitLineCtrl->SetMinSize(wxSize(300, 1));
	//itemSizer->Add(splitLineCtrl, 0, wxALIGN_LEFT | wxLEFT, 12);

	//itemSizer->AddSpacer(8);

	//// icon and name
	//wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
	//titleSizer->SetMinSize(wxSize(324, 20));
	//itemSizer->Add(titleSizer, 0, wxALIGN_LEFT | wxLEFT, 12);

	//wxImage image = wxImage(wxString::FromUTF8(Slic3r::var("file_icon_50x50.png")), wxBITMAP_TYPE_PNG);
	//image.Rescale(20, 20);
	//wxStaticBitmap* iconImage = new wxStaticBitmap(this, wxID_ANY, image);
	//iconImage->SetMaxSize(wxSize(20, 20));
	//titleSizer->Add(iconImage, wxEXPAND | wxALL, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);

	//titleSizer->AddSpacer(8);

	//m_pFileNameText = new wxStaticText(this, wxID_ANY, "File Name");
	//m_pFileNameText->SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFileNameText->SetForegroundColour(wxColour(255, 255, 255));
	//m_pFileNameText->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
	//m_pFileNameText->SetMinSize(wxSize(270, 20));
	//wxFont font = m_pFileNameText->GetFont();
	//font.SetPointSize(10);
	//m_pFileNameText->SetFont(font);
	//titleSizer->Add(m_pFileNameText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 0);

	//itemSizer->AddSpacer(4);

	//// path
	//m_pFilePathText = new wxStaticText(this, wxID_ANY, "File Path");
	//m_pFilePathText->SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFilePathText->SetForegroundColour(wxColour(183, 183, 183));
	//m_pFilePathText->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
	//m_pFilePathText->SetMinSize(wxSize(290, 15));
	//font = m_pFilePathText->GetFont();
	//font.SetPointSize(10);
	//m_pFilePathText->SetFont(font);
	//itemSizer->Add(m_pFilePathText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 34);

	//itemSizer->AddSpacer(4);

	//// time
	//m_pFileTimeText = new wxStaticText(this, wxID_ANY, "File Time");
	//m_pFileTimeText->SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFileTimeText->SetForegroundColour(wxColour(183, 183, 183));
	//m_pFileTimeText->SetMinSize(wxSize(290, 15));
	//font = m_pFileTimeText->GetFont();
	//font.SetPointSize(10);
	//m_pFileTimeText->SetFont(font);
	//itemSizer->Add(m_pFileTimeText, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 34);

	//itemSizer->AddSpacer(18);
}

void AnkerGCodeFileItem::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.Clear();

	wxColour bgColor = GetBackgroundColour();

	wxRect rect = GetClientRect();
	wxBrush brush(bgColor);
	wxPen pen(wxColour(41, 42, 45));
	dc.SetBrush(brush);
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

	// draw line
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(64, 65, 70));
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
		wxFont font = dc.GetFont();
		font.SetPointSize(10);
		dc.SetFont(font);
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
		wxFont font = dc.GetFont();
		font.SetPointSize(8);
		dc.SetFont(font);
		dc.SetTextForeground(wxColour(184, 184, 186));
		wxPoint textPoint = wxPoint(42, 39);
		dc.DrawText(m_filePath, textPoint);
	}

	// draw file time
	{
		wxBrush brush(bgColor);
		wxPen pen(wxColour(184, 184, 186));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		wxFont font = dc.GetFont();
		font.SetPointSize(8);
		dc.SetFont(font);
		dc.SetTextForeground(wxColour(184, 184, 186));
		wxPoint textPoint = wxPoint(42, 61);
		dc.DrawText(m_fileTimeInfo, textPoint);
	}
}

void AnkerGCodeFileItem::OnMouseEnterWindow(wxMouseEvent& event)
{
	SetBackgroundColour(wxColour(77, 78, 82));
	//m_pFileNameText->SetBackgroundColour(wxColour(77, 78, 82));
	//m_pFilePathText->SetBackgroundColour(wxColour(77, 78, 82));
	//m_pFileTimeText->SetBackgroundColour(wxColour(77, 78, 82));
	Refresh();
}

void AnkerGCodeFileItem::OnMouseLeaveWindow(wxMouseEvent& event)
{
	SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFileNameText->SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFilePathText->SetBackgroundColour(wxColour(41, 42, 45));
	//m_pFileTimeText->SetBackgroundColour(wxColour(41, 42, 45));
	Refresh();
}

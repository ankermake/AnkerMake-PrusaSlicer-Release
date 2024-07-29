#include "AnkerMsgCentreDialog.hpp"
#include "../libslic3r/Utils.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "../GUI/GUI_App.hpp"
#include "wx/statbmp.h"
#include <wx/graphics.h>
#include <wx/dcclient.h>
#include "../Utils/DataMangerUi.hpp"
#include <algorithm>
#include "Common/AnkerMsgDialog.hpp"

#define BG_COLOR wxColor("#333438")

AnkerOfficialMsg::AnkerOfficialMsg()
{

}
AnkerOfficialMsg::AnkerOfficialMsg(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	initUi();
}

AnkerOfficialMsg::~AnkerOfficialMsg()
{

}
void AnkerOfficialMsg::initMsg(const wxString& msgTitle,
	const wxString& time,
	const wxString& content,
	bool isRead /*= false*/)
{
	auto font = m_pContentTextCtrl->GetFont();	
	wxClientDC dc(this);
	dc.SetFont(font);
	wxSize textSize = dc.GetTextExtent(content);
	int length = 0;
	if (content.size() % 100 == 0)
		length = content.size() / 100;
	else 
		length = content.size() / 100 + 1;
	wxString labelStr = Slic3r::GUI::WrapEveryCharacter(content, font, 100);
	m_pContentTextCtrl->SetMaxSize(AnkerSize(620, length*25));
	m_pContentTextCtrl->SetMinSize(AnkerSize(620, length*25));
	m_pContentTextCtrl->SetSize(AnkerSize(620, length*25));

	SetMaxSize(AnkerSize(700, length * 25 + 25));
	SetMinSize(AnkerSize(700, length * 25 + 25));
	SetSize(AnkerSize(700, length * 25 + 25));

	m_pTitleLabel->SetLabelText(msgTitle);
	m_pTimeLabel->SetLabelText(time);
	m_pContentTextCtrl->SetLabelText(labelStr);
	//m_pContentTextCtrl->SetValue(content);

	if (!isRead)
		m_pRedLogoLabel->Show();
	else
		m_pRedLogoLabel->Hide();

	Refresh();
	Layout();

}

void AnkerOfficialMsg::setReadStatus(bool isRead)
{
	if (!isRead)
		m_pRedLogoLabel->Show();
	else
		m_pRedLogoLabel->Hide();

	Refresh();
	Layout();
}

void AnkerOfficialMsg::initUi()
{
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel* pBGWidget = new wxPanel(this, wxID_ANY);
	pBGWidget->SetMaxSize(AnkerSize(700, -1));
	pBGWidget->SetMinSize(AnkerSize(700, -1));
	pBGWidget->SetSize(AnkerSize(700, -1));
	//pBGWidget->SetBackgroundColour(wxColor("#ADFF2F"));//for test
	pBGWidget->SetBackgroundColour(BG_COLOR);
	wxBoxSizer* pBGVSizer = new wxBoxSizer(wxVERTICAL);
	pBGWidget->SetSizer(pBGVSizer);

	wxPanel* pTitleWidget = new wxPanel(this, wxID_ANY);
	pTitleWidget->SetMaxSize(AnkerSize(700, 20));
	pTitleWidget->SetMinSize(AnkerSize(700, 20));
	pTitleWidget->SetSize(AnkerSize(700, 20));
	pTitleWidget->SetBackgroundColour(BG_COLOR);
	//pTitleWidget->SetBackgroundColour(wxColour("#BC8F8F"));//for test

	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	pTitleWidget->SetSizer(pTitleHSizer);
	wxBoxSizer* pContentHSizer = new wxBoxSizer(wxHORIZONTAL);

	m_pRedLogoLabel = new AnkerMsgCircleLabel(pTitleWidget, wxID_ANY);
	m_pRedLogoLabel->SetMaxSize(AnkerSize(10, 10));
	m_pRedLogoLabel->SetMinSize(AnkerSize(10, 10));
	m_pRedLogoLabel->SetSize(AnkerSize(10, 10));

	m_pTitleLabel = new wxStaticText(pTitleWidget, wxID_ANY, ("Anker Message"));
	m_pTitleLabel->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pTitleLabel->SetForegroundColour(wxColor("#FFFFFF"));
	m_pTimeLabel = new wxStaticText(pTitleWidget, wxID_ANY, ("2023/1/1 00:00:00"));
	m_pTimeLabel->SetMaxSize(AnkerSize(74, 25));
	m_pTimeLabel->SetMinSize(AnkerSize(74, 25));
	m_pTimeLabel->SetSize(AnkerSize(74, 25));
	m_pTimeLabel->SetFont(ANKER_FONT_NO_1);
	m_pTimeLabel->SetForegroundColour(wxColor("#707174"));

	pTitleHSizer->AddSpacer(26);
	pTitleHSizer->Add(m_pRedLogoLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddSpacer(5);
	pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddStretchSpacer(1);
	pTitleHSizer->Add(m_pTimeLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_RIGHT, 0);
	pTitleHSizer->AddSpacer(40);

	m_pContentTextCtrl = new wxStaticText(this, wxID_ANY, "");
	m_pContentTextCtrl->Wrap(100);
	
	m_pContentTextCtrl->SetBackgroundColour(BG_COLOR);
	m_pContentTextCtrl->SetForegroundColour(wxColour("#adaeaf"));
	
	m_pContentTextCtrl->SetFont(ANKER_FONT_NO_1);

	pContentHSizer->AddSpacer(40);
	pContentHSizer->Add(m_pContentTextCtrl, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pContentHSizer->AddSpacer(20);

	pBGVSizer->Add(pTitleWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pBGVSizer->AddSpacer(5);
	pBGVSizer->Add(pContentHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

	pMainVSizer->Add(pBGWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	SetSizer(pMainVSizer);
}


BEGIN_EVENT_TABLE(AnkerPrinterMsg, wxPanel)
EVT_LEFT_DOWN(AnkerPrinterMsg::OnPressed)
EVT_ENTER_WINDOW(AnkerPrinterMsg::OnEnter)
EVT_LEAVE_WINDOW(AnkerPrinterMsg::OnLeave)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerPrinterMsg, wxPanel)
AnkerPrinterMsg::AnkerPrinterMsg()
{

}
AnkerPrinterMsg::AnkerPrinterMsg(
	wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	long style /*= wxTAB_TRAVERSAL | wxNO_BORDER*/,
	const wxString& name /*= wxASCII_STR(wxPanelNameStr)*/) : wxPanel(parent, winid, pos, size, style, name)
{
	
	SetBackgroundColour(BG_COLOR);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	initUi();
		
}

AnkerPrinterMsg::~AnkerPrinterMsg()
{

}

void AnkerPrinterMsg::OnEnter(wxMouseEvent& event)
{	
	SetBackgroundColour(wxColor("#3D3E42"));
	event.Skip();
}

void AnkerPrinterMsg::OnLeave(wxMouseEvent& event)
{
	SetBackgroundColour(BG_COLOR);
	event.Skip();
}

void AnkerPrinterMsg::OnPressed(wxMouseEvent& event)
{
	setReadStatus(true);

	if (m_supportUrl.empty())
	{
		event.Skip();
		return;
	}

	std::string realUrl = Slic3r::UrlDecode(m_supportUrl.ToStdString());
	wxLaunchDefaultBrowser(realUrl.c_str());	
 	
	event.Skip();
}

void AnkerPrinterMsg::initMsg(const wxString& msgTitle,
								const wxString& time,
								const wxString& content,
								const wxString& url,
								bool isRead /*= false*/)								
{	
	auto font = m_pContentLabel->GetFont();
	wxClientDC dc(this);
	dc.SetFont(font);
	wxSize textSize = dc.GetTextExtent(content);
	int length = 0;
	if (content.size() % 100 == 0)
		length = content.size() / 100;
	else
		length = content.size() / 100 + 1;
	wxString labelStr = Slic3r::GUI::WrapEveryCharacter(content, font, 100);
	m_pContentLabel->SetMaxSize(AnkerSize(620, length * 25));
	m_pContentLabel->SetMinSize(AnkerSize(620, length * 25));
	m_pContentLabel->SetSize(AnkerSize(620, length * 25));

	SetMaxSize(AnkerSize(700, length * 25 + 25));
	SetMinSize(AnkerSize(700, length * 25 + 25));
	SetSize(AnkerSize(700, length * 25 + 25));

	m_pTitleLabel->SetLabelText(msgTitle);
	m_pTimeLabel->SetLabelText(time);
	m_pContentLabel->SetLabelText(content);
	//m_pContentTextCtrl->SetValue(content);

	if (!isRead)
		m_pRedLogoLabel->Show();
	else
		m_pRedLogoLabel->Hide();

	if (!url.empty())
		m_supportUrl = url;

	Refresh();
	Layout();
}

void AnkerPrinterMsg::setReadStatus(bool isRead)
{
	if (!isRead)
		m_pRedLogoLabel->Show();
	else
		m_pRedLogoLabel->Hide();

	Refresh();
	Layout();
}


void AnkerPrinterMsg::initUi()
{
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel* pBGWidget = new wxPanel(this, wxID_ANY);
	pBGWidget->SetMaxSize(AnkerSize(700, -1));
	pBGWidget->SetMinSize(AnkerSize(700, -1));
	pBGWidget->SetSize(AnkerSize(700, -1));
	pBGWidget->SetBackgroundColour(BG_COLOR);

	wxBoxSizer* pBGVSizer = new wxBoxSizer(wxVERTICAL);
	pBGWidget->SetSizer(pBGVSizer);

	wxPanel* pTitleWidget = new wxPanel(this, wxID_ANY);
	pTitleWidget->SetMaxSize(AnkerSize(700, 20));
	pTitleWidget->SetMinSize(AnkerSize(700, 20));
	pTitleWidget->SetSize(AnkerSize(700, 20));
	pTitleWidget->SetBackgroundColour(BG_COLOR);

	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	pTitleWidget->SetSizer(pTitleHSizer);
	wxBoxSizer* pContentHSizer = new wxBoxSizer(wxHORIZONTAL);

	m_pRedLogoLabel = new AnkerMsgCircleLabel(pTitleWidget, wxID_ANY);
	m_pRedLogoLabel->SetMaxSize(AnkerSize(10, 10));
	m_pRedLogoLabel->SetMinSize(AnkerSize(10, 10));
	m_pRedLogoLabel->SetSize(AnkerSize(10, 10));

	m_pTitleLabel = new wxStaticText(pTitleWidget, wxID_ANY, (""));
	m_pTitleLabel->SetFont(ANKER_BOLD_FONT_NO_1);
	m_pTitleLabel->SetForegroundColour(wxColor("#FFFFFF"));
	m_pTimeLabel = new wxStaticText(pTitleWidget, wxID_ANY, (""));
	m_pTimeLabel->SetMaxSize(AnkerSize(80, 25));
	m_pTimeLabel->SetMinSize(AnkerSize(80, 25));
	m_pTimeLabel->SetSize(AnkerSize(80, 25));
	m_pTimeLabel->SetFont(ANKER_FONT_NO_1);
	m_pTimeLabel->SetForegroundColour(wxColor("#707174"));

	pTitleHSizer->AddSpacer(26);
	pTitleHSizer->Add(m_pRedLogoLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddSpacer(5);
	pTitleHSizer->Add(m_pTitleLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pTitleHSizer->AddStretchSpacer(1);
	pTitleHSizer->Add(m_pTimeLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_RIGHT, 0);
	pTitleHSizer->AddSpacer(45);
	
	m_pContentLabel = new wxStaticText(this, wxID_ANY, "");
	m_pContentLabel->Wrap(100);
	//m_pContentTextCtrl->SetBackgroundColour(wxColour("#ff00ff"));
	m_pContentLabel->SetBackgroundColour(BG_COLOR);
	m_pContentLabel->SetForegroundColour(wxColour("#adaeaf"));
	m_pContentLabel->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {});
	//m_pContentTextCtrl->SetEditable(false);
	wxRichTextAttr attr;
	attr.SetTextColour(wxColor("#adaeaf"));
	//m_pContentTextCtrl->SetBasicStyle(attr);
	m_pContentLabel->SetFont(ANKER_FONT_NO_1);

	wxImage officicalRightNorImg = wxImage(wxString::FromUTF8(Slic3r::var("right_nor.png")), wxBITMAP_TYPE_PNG);
	m_pArrowLogoLabel = new AnkerMsgPageBtn(this,wxID_ANY, officicalRightNorImg, officicalRightNorImg);
	m_pArrowLogoLabel->SetMaxSize(AnkerSize(16,16));
	m_pArrowLogoLabel->SetMinSize(AnkerSize(16,16));
	m_pArrowLogoLabel->SetSize(AnkerSize(16,16));

	pContentHSizer->AddSpacer(40);
	pContentHSizer->Add(m_pContentLabel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pContentHSizer->AddSpacer(3);
	pContentHSizer->Add(m_pArrowLogoLabel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pContentHSizer->AddSpacer(20);

	pBGVSizer->Add(pTitleWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pBGVSizer->AddSpacer(5);
	pBGVSizer->Add(pContentHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

	pMainVSizer->Add(pBGWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	SetSizer(pMainVSizer);

	pBGWidget->Bind(wxEVT_ENTER_WINDOW, [this, pBGWidget, pTitleWidget](wxMouseEvent& event) {
		pBGWidget->SetBackgroundColour(wxColor("#3D3E42"));
		m_pContentLabel->SetBackgroundColour(wxColor("#3D3E42"));
		pTitleWidget->SetBackgroundColour(wxColor("#3D3E42"));
		m_pTimeLabel->SetBackgroundColour(wxColor("#3D3E42"));
		m_pTitleLabel->SetBackgroundColour(wxColor("#3D3E42"));
		m_pRedLogoLabel->setBgColor(wxColor("#3D3E42"));
		m_pArrowLogoLabel->setBgColor(wxColor("#3D3E42"));
		SetBackgroundColour(wxColor("#3D3E42"));
		Refresh();
		event.Skip();
		});
	pBGWidget->Bind(wxEVT_LEAVE_WINDOW, [this, pBGWidget, pTitleWidget](wxMouseEvent& event) {		
		pBGWidget->SetBackgroundColour(BG_COLOR);
		m_pContentLabel->SetBackgroundColour(BG_COLOR);
		pTitleWidget->SetBackgroundColour(BG_COLOR);
		m_pTimeLabel->SetBackgroundColour(BG_COLOR);
		m_pTitleLabel->SetBackgroundColour(BG_COLOR);
		m_pRedLogoLabel->setBgColor(BG_COLOR);
		m_pArrowLogoLabel->setBgColor(BG_COLOR);
		SetBackgroundColour(BG_COLOR);
		Refresh();
		event.Skip();
		});
	pBGWidget->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
		setReadStatus(true);

		if (m_supportUrl.empty())
		{
			event.Skip();
			return;
		}

		std::string realUrl = Slic3r::UrlDecode(m_supportUrl.ToStdString());
		wxLaunchDefaultBrowser(realUrl.c_str());
		
		event.Skip();
		});

}

AnkerMsgCentreDialog::AnkerMsgCentreDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, _L("msg_center_title"))
{
	SetBackgroundColour(BG_COLOR);
	initUi();
	initMulitLanguage();
	Bind(wxEVT_MOVE, &AnkerMsgCentreDialog::onMove, this);	
	showMsgCenterWidget();
	m_pPrinterBtn->setStatus(MsgNormalBtnStatus);
	if(!m_MsgRecords)
		m_MsgRecords = new std::vector<MsgCenterItem>();
}

AnkerMsgCentreDialog::~AnkerMsgCentreDialog()
{
	if (m_MsgRecords)
	{
		delete m_MsgRecords;
		m_MsgRecords = nullptr;
	}
}
void AnkerMsgCentreDialog::updateMsg(std::vector<AnkerNet::MsgCenterItem>* dataList)
{	
	if (!dataList|| dataList->size() <= 0)
	{		
		if (!m_MsgRecords|| m_MsgRecords->size() <= 0)		
		{
			showLoading(false);
			onNoRecords();
			ANKER_LOG_INFO << "no data on msg center";
			return;
		}
		bool hasData = false;
		for (auto item : m_msgList)
		{
			if (!item->getSelectStatus())
			{
				hasData = true;
				break;
			}
		}

		if(!hasData)
			onNoRecords();

		ANKER_LOG_INFO << "no data to load";
		showNataTips(true);
		showLoading(false);		
		return;
	}
	else
	{
		showNataTips(false);
	}

	if (!m_MsgRecords)
		m_MsgRecords = new std::vector<MsgCenterItem>();

	//m_MsgRecords = dataList;
	int officecialUnReadNews = 0;
	int printerUnReadNews = 0;
	
	
	std::vector<AnkerNet::MsgCenterItem>* tmpVector = new std::vector<AnkerNet::MsgCenterItem>();
	for ( auto it = dataList->begin(); it != dataList->end(); ++it) {
	
		bool isRepeated = false;

		if(m_MsgRecords->size() <= 0)
			tmpVector->push_back((*it));
		else
		{
			for (auto findItem = m_MsgRecords->begin(); findItem != m_MsgRecords->end(); ++findItem)
			{
				if ((*findItem).msgCreateTime == (*it).msgCreateTime)
				{
					isRepeated = true;
					break;
				}
			}
			tmpVector->push_back((*it));
		}

		if (isRepeated)
		{			
			continue;
		}

		std::string strTitleData = (*it).msgTitle;
		wxString titleData = wxString::FromUTF8(strTitleData);

		bool isRead = !(*it).msgIsNew;
		AnkerCustomMsg* pItemWidgetEx = new AnkerCustomMsg(m_printerMsgWidget, wxID_ANY);
		pItemWidgetEx->SetMaxSize(wxSize(700, 106));
		pItemWidgetEx->SetMinSize(wxSize(700, 106));
		pItemWidgetEx->SetSize(wxSize(700, 106));
		wxString contentStr = "";
		if ((*it).alarm_type != 0)
		{
			auto type = (*it).alarm_type;			
			if(type != 1001)
				contentStr = m_mulitLanguageMap[type];
			else
			{
				auto aiType = (*it).ai_alarm_type;
				contentStr = m_mulitLanguageMap[type];
			}
		}
		else
			contentStr = (*it).msgContent;
				
		//wxString uft8ContentStr = wxString::FromUTF8(contentStr.c_str());
		wxString uft8ContentStr = contentStr;
		if ((*it).msgLevel == LEVEL_S|| (*it).msgLevel == LEVEL_P0)
		{
			uft8ContentStr = "[" + (*it).msgErrorCode + "] "+ uft8ContentStr;
		}
		m_printerMsgVec.push_back((*it).msgUrl);
		pItemWidgetEx->initMsg(titleData, (*it).msgCreateTime, uft8ContentStr, (*it).msgUrl, (*it).msgID,isRead);
		if ((*it).msgIsNew)
			printerUnReadNews++;
		//m_pPrinterScrolledVSizer->Insert(0, pItemWidgetEx, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pItemWidgetEx->setEditMode(m_isEditmode);
		pItemWidgetEx->setSelect(m_selectAllBtnStatus);
		m_msgList.push_back(pItemWidgetEx);
		m_pPrinterScrolledVSizer->Add(pItemWidgetEx, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	}

	m_MsgRecords->insert(m_MsgRecords->end(), tmpVector->begin(), tmpVector->end());

	if(dataList)
	{
		delete dataList;
		dataList = nullptr;
	}	
	
	if(tmpVector)
	{
		delete tmpVector;
		tmpVector = nullptr;
	}

	showMsgCenterWidget();
	Layout();
	showLoading(false);
}

void AnkerMsgCentreDialog::clearMsg()
{
	if (m_MsgRecords)
	{
		delete m_MsgRecords;
		m_MsgRecords = nullptr;
	}

	m_isEditmode = false;
	m_selectAllBtnStatus = false;

	m_msgList.clear();

	m_currentPage = 0;

	resetSelectAllBtn();

	showNataTips(false);

	isShowEditMode(false);

	m_printerMsgVec.clear();
	m_officialMsgVec.clear();
	wxSizerItemList& childrenList = m_pPrinterScrolledVSizer->GetChildren();
	std::vector<wxSizerItem*> itemsToDelete;

	for (wxSizerItemList::iterator it = childrenList.begin(); it != childrenList.end(); ++it) {
		wxSizerItem* item = *it;
		
		if (item->IsWindow()) {			
			if (item->IsWindow()) {
				itemsToDelete.push_back(item);
			}
		}
	}
	for (wxSizerItem* item : itemsToDelete) {
		
		wxWindow* pWindow = item->GetWindow();
		
		if (!pWindow)
			continue;

		AnkerCustomMsg* pCustomMsgWindow = static_cast<AnkerCustomMsg*>(item->GetWindow());
		if(pCustomMsgWindow)
		{
			m_pPrinterScrolledVSizer->Detach(pCustomMsgWindow);
			pCustomMsgWindow->Destroy();
		}		
	}
	
	m_printerMsgWidget->Layout();
	m_printerMsgWidget->Refresh();
	Refresh();
	Layout();
}

void AnkerMsgCentreDialog::resetSelectAllBtn()
{
	//clear select all btn
	m_selectAllBtnStatus = false;
	wxBitmap selectBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("msg_uncheck.png")), wxBITMAP_TYPE_PNG);
	wxImage selectBtnImage = selectBitmap.ConvertToImage();
	selectBtnImage.Rescale(AnkerLength(16), AnkerLength(16));
	wxBitmap scaledSelectBitmap(selectBtnImage);
	m_selectAllBtn->SetBitmap(scaledSelectBitmap);

}

void AnkerMsgCentreDialog::initMulitLanguage()
{
	m_mulitLanguageMap.insert(std::make_pair(1012, _L("fdm_advancedpause_content")));
	m_mulitLanguageMap.insert(std::make_pair(1, _L("fdm_aialarm_notify_bederr")));//1001 and  diff 1
	m_mulitLanguageMap.insert(std::make_pair(2, _L("fdm_aialarm_notify_othererr")));//1001 and diff 2

	m_mulitLanguageMap.insert(std::make_pair(1000, _L("fdm_alarm_broken_content")));
	m_mulitLanguageMap.insert(std::make_pair(1007, _L("fdm_alarm_heatbederr_content")));
	m_mulitLanguageMap.insert(std::make_pair(1006, _L("fdm_alarm_nozzleerr_content")));

	m_mulitLanguageMap.insert(std::make_pair(1005, _L("fdm_alarm_printcompleted_content")));
	m_mulitLanguageMap.insert(std::make_pair(1002, _L("fdm_alarm_printerr_content")));
	m_mulitLanguageMap.insert(std::make_pair(2000, _L("fdm_alarm_share_content")));
	
	m_mulitLanguageMap.insert(std::make_pair(2002, _L("fdm_alarm_sharepermissionchange_content")));
	m_mulitLanguageMap.insert(std::make_pair(2001, _L("fdm_alarm_sharepermissionremove_content")));
	
	m_mulitLanguageMap.insert(std::make_pair(1009, _L("fdm_hyperthermia_error_tip")));
	m_mulitLanguageMap.insert(std::make_pair(1010, _L("fdm_hyperthermia_error_tip")));
	m_mulitLanguageMap.insert(std::make_pair(1011, _L("fdm_lowtemp_tip_content")));
	m_mulitLanguageMap.insert(std::make_pair(1008, _L("fdm_mos_error_tip")));

}
void AnkerMsgCentreDialog::initUi()
{
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel* pWidget = new wxPanel(this);
	pWidget->SetBackgroundColour(BG_COLOR);	
	wxBoxSizer* pWidgetVSizer = new wxBoxSizer(wxVERTICAL);
	pWidget->SetSizer(pWidgetVSizer);

	wxBoxSizer* pTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* pContentWindowHSizer = new wxBoxSizer(wxHORIZONTAL);
	pMainVSizer->AddSpacer(AnkerLength(16));

	m_printerPanel = new wxPanel(pWidget);
	wxBoxSizer* pPrinterVSizer = new wxBoxSizer(wxVERTICAL);
	m_printerPanel->SetSizer(pPrinterVSizer);
	//title
	{
		wxCursor handCursor(wxCURSOR_HAND);
		wxBoxSizer* pEditTitleHSizer = new wxBoxSizer(wxHORIZONTAL);
		pEditTitleHSizer->AddSpacer(AnkerLength(16));

		m_selectPanel = new  wxPanel(m_printerPanel);
		wxBoxSizer* pSelectHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_selectPanel->SetSizer(pSelectHSizer);
		//m_selectAllBtn = new AnkerMsgCheckBox(this,wxID_ANY,_L("msg_center_label_select_all"));

		wxBitmap selectBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("msg_uncheck.png")), wxBITMAP_TYPE_PNG);
		wxImage selectBtnImage = selectBitmap.ConvertToImage();
		selectBtnImage.Rescale(AnkerLength(16), AnkerLength(16));
		wxBitmap scaledSelectBitmap(selectBtnImage);
		
		m_selectAllBtn = new wxButton(m_selectPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_selectAllBtn->SetBackgroundColour("#333438");
		m_selectAllBtn->SetMaxSize(AnkerSize(16, 16));
		m_selectAllBtn->SetMinSize(AnkerSize(16, 16));
		m_selectAllBtn->SetSize(AnkerSize(16, 16));
		m_selectAllBtn->SetBitmap(scaledSelectBitmap);
		
		m_selectAllLabel = new wxStaticText(m_selectPanel, wxID_ANY, _L("msg_center_label_select_all"));
		m_selectAllLabel->SetFont(ANKER_FONT_NO_1);
		m_selectAllLabel->SetForegroundColour("#FFFFFF");
		m_selectAllLabel->SetMaxSize(AnkerSize(90, 24));
		m_selectAllLabel->SetMinSize(AnkerSize(90, 24));
		m_selectAllLabel->SetSize(AnkerSize(90, 24));
		
		pSelectHSizer->Add(m_selectAllBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pSelectHSizer->AddSpacer(AnkerLength(8));
		pSelectHSizer->Add(m_selectAllLabel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

		m_printerTitle = new wxStaticText(m_printerPanel,wxID_ANY,_L("msg_center_label_printer"));
		m_printerTitle->SetForegroundColour("#FFFFFF");
		m_printerTitle->SetFont(ANKER_FONT_NO_1);

		m_printerTitleSpcaer = new wxStaticText(m_printerPanel, wxID_ANY, _L(""));
		m_printerTitleSpcaer->SetMaxSize(AnkerSize(22, 22));
		m_printerTitleSpcaer->SetMinSize(AnkerSize(22, 22));
		m_printerTitleSpcaer->SetSize(AnkerSize(22, 22));

		wxClientDC dc(this);
		dc.SetFont(ANKER_FONT_NO_1);
		wxSize size = dc.GetTextExtent(_L("msg_center_btn_cancel"));
		int textWidth = size.GetWidth();
		size.SetWidth(textWidth + AnkerLength(20));
		size.SetHeight(AnkerLength(24));

		m_cancelBtn = new AnkerBtn(m_printerPanel, wxID_ANY);
		m_cancelBtn->SetText(_L("msg_center_btn_cancel"));
		m_cancelBtn->SetTextColor("#FFFFFF");
		m_cancelBtn->SetBackgroundColour("#5C5D60");
		m_cancelBtn->SetFont(ANKER_FONT_NO_1);
		m_cancelBtn->SetMaxSize(size);
		m_cancelBtn->SetMinSize(size);
		m_cancelBtn->SetSize(size);
		m_cancelBtn->SetRadius(4);

		m_deleteBtn = new AnkerBtn(m_printerPanel,wxID_ANY);
		m_deleteBtn->SetText(_L("msg_center_btn_delete"));
		m_deleteBtn->SetTextColor("#FFFFFF");		
		m_deleteBtn->SetBackgroundColour("#FF3636");
		m_deleteBtn->SetFont(ANKER_FONT_NO_1);
		m_deleteBtn->SetMaxSize(AnkerSize(65, 24));
		m_deleteBtn->SetMinSize(AnkerSize(65, 24));
		m_deleteBtn->SetSize(AnkerSize(65, 24));
		m_deleteBtn->SetRadius(4);

		wxBitmap editBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("msg_center_edit.png")), wxBITMAP_TYPE_PNG);
		wxImage editBtnImage = editBitmap.ConvertToImage();
		editBtnImage.Rescale(AnkerLength(16), AnkerLength(16));
		wxBitmap scaledEditBitmap(editBtnImage);
		
		m_editBtn = new wxButton(m_printerPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		m_editBtn->SetMaxSize(AnkerSize(16,16));
		m_editBtn->SetMinSize(AnkerSize(16,16));
		m_editBtn->SetSize(AnkerSize(16,16));
		m_editBtn->SetBitmap(scaledEditBitmap);
		m_editBtn->SetBackgroundColour("#333438");
		m_editBtn->SetToolTip(_L("msg_center_edit_btn"));

		m_selectAllBtn->SetCursor(handCursor);
		m_cancelBtn->SetCursor(handCursor);
		m_deleteBtn->SetCursor(handCursor);
		m_editBtn->SetCursor(handCursor);

		m_selectAllBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {						
			m_selectAllBtnStatus = !m_selectAllBtnStatus;
			if(m_selectAllBtnStatus)
			{
				wxBitmap selectBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("msg_check.png")), wxBITMAP_TYPE_PNG);
				wxImage selectBtnImage = selectBitmap.ConvertToImage();
				selectBtnImage.Rescale(AnkerLength(16), AnkerLength(16));
				wxBitmap scaledSelectBitmap(selectBtnImage);
				m_selectAllBtn->SetBitmap(scaledSelectBitmap);
			}
			else
			{
				wxBitmap selectBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("msg_uncheck.png")), wxBITMAP_TYPE_PNG);
				wxImage selectBtnImage = selectBitmap.ConvertToImage();
				selectBtnImage.Rescale(AnkerLength(16), AnkerLength(16));
				wxBitmap scaledSelectBitmap(selectBtnImage);
				m_selectAllBtn->SetBitmap(scaledSelectBitmap);
			}

			for (auto item : m_msgList)
			{
				item->setSelect(m_selectAllBtnStatus);
			}

			Refresh();
			Layout();
			});
		m_cancelBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			isShowEditMode(false);
			resetSelectAllBtn();
			for (auto item : m_msgList)
			{
				item->setEditMode(m_isEditmode);
				item->setSelect(false);
			}
			});
		m_deleteBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
			isShowEditMode(false);
			std::vector<int> msgIdList;
			for (auto item : m_msgList)
			{
				item->setEditMode(m_isEditmode);
				if (item->getSelectStatus())
				{
					item->Hide();
					msgIdList.push_back(item->getMsgID());
				}
			}
			if (msgIdList.size() > 0)
			{
				auto ankerNet = AnkerNetInst();
				if (!ankerNet || !ankerNet->IsLogined()) {
					return;
				}
				if(m_msgList.size() == msgIdList.size())
					ankerNet->removeMsgByIds(msgIdList,true);//syn
				else
					ankerNet->removeMsgByIds(msgIdList,false);//asy
			}
			else
			{
				ANKER_LOG_INFO << "no data to delete. ";
				return;
			}
			resetSelectAllBtn();
			if (m_msgList.size() == msgIdList.size())
			{
				m_currentPage = 0;
				getMsgCenterRecords();
			}
			Refresh();
			Layout();
			});
		m_editBtn->Bind(wxEVT_BUTTON,[this](wxCommandEvent & event) {
			isShowEditMode(true);

			for (auto item : m_msgList)
			{
				item->setEditMode(m_isEditmode);
			}
			
		});

		pEditTitleHSizer->Add(m_selectPanel,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pEditTitleHSizer->Add(m_printerTitleSpcaer,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pEditTitleHSizer->Add(m_printerTitle,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

		pEditTitleHSizer->AddStretchSpacer();

		pEditTitleHSizer->Add(m_cancelBtn,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pEditTitleHSizer->AddSpacer(AnkerLength(7));
		pEditTitleHSizer->Add(m_deleteBtn,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pEditTitleHSizer->Add(m_editBtn,wxALL | wxEXPAND, wxALL | wxEXPAND, 0);		
		pEditTitleHSizer->AddSpacer(AnkerLength(35));

		pPrinterVSizer->Add(pEditTitleHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	}

	wxStaticText* pLine = new wxStaticText(m_printerPanel, wxID_ANY, "");
	pLine->SetBackgroundColour("#484A51");
	pLine->SetMaxSize(AnkerSize(800, 1));
	pLine->SetMinSize(AnkerSize(800, 1));
	pLine->SetSize(AnkerSize(800, 1));
	pPrinterVSizer->AddSpacer(AnkerLength(16));
	pPrinterVSizer->Add(pLine, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	//pPrinterVSizer->AddSpacer(AnkerLength(25));

	//Temporarily unused
	{
		wxColor normalTextColor = wxColor("#adaeaf");
		wxColor SelectTextColor = wxColor("#80d16f");
		wxCursor handCursor(wxCURSOR_HAND);
		m_pOfficialBtn = new AnkerMsgTitleBtn(pWidget, wxID_ANY, normalTextColor, SelectTextColor);
		m_pOfficialBtn->initData(_L("msg_center_officical_tab"), MsgPressedBtnStatus);		
		m_pOfficialBtn->SetMaxSize(AnkerSize(100, 25));
		m_pOfficialBtn->SetMinSize(AnkerSize(100, 25));
		m_pOfficialBtn->SetSize(AnkerSize(100, 25));
		m_pOfficialBtn->Bind(wxCUSTOMEVT_ANKER_MSG_BTN_CLICKED, &AnkerMsgCentreDialog::onOfficialBtn, this);
		m_pOfficialBtn->SetCursor(handCursor);		
		m_pOfficialBtn->Hide();

		m_pPrinterBtn = new AnkerMsgTitleBtn(pWidget, wxID_ANY, normalTextColor, SelectTextColor);
		m_pPrinterBtn->initData(_L("msg_center_printer_tab"), MsgNormalBtnStatus);		
		m_pPrinterBtn->SetMaxSize(AnkerSize(100, 25));
		m_pPrinterBtn->SetMinSize(AnkerSize(100, 25));
		m_pPrinterBtn->SetSize(AnkerSize(100, 25));
		m_pPrinterBtn->Bind(wxCUSTOMEVT_ANKER_MSG_BTN_CLICKED, &AnkerMsgCentreDialog::onPrinterBtn, this);
		m_pPrinterBtn->SetCursor(handCursor);
		m_pPrinterBtn->Hide();

		//pTitleHSizer->AddStretchSpacer();
		pTitleHSizer->Add(m_pOfficialBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);		
		pTitleHSizer->Add(m_pPrinterBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		//pTitleHSizer->AddStretchSpacer();

		//pWidgetVSizer->AddSpacer(AnkerLength(20));
		pWidgetVSizer->Add(pTitleHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		//pWidgetVSizer->AddSpacer(AnkerLength(32));	
	}

	{
		m_OfficicalMsgWidget = new wxScrolledWindow(pWidget, wxID_ANY);		
		m_pOfficScrolledVSizer = new wxBoxSizer(wxVERTICAL);

		m_OfficicalMsgWidget->SetMinSize(AnkerSize(700, 600));
		m_OfficicalMsgWidget->SetMaxSize(AnkerSize(700, 600));
		m_OfficicalMsgWidget->SetSize(AnkerSize(700, 600));
		m_OfficicalMsgWidget->SetSizer(m_pOfficScrolledVSizer);		
		m_OfficicalMsgWidget->SetBackgroundColour(BG_COLOR);

		m_OfficicalMsgWidget->SetVirtualSize(AnkerSize(700, 600));
		m_OfficicalMsgWidget->SetScrollRate(0, 30);
		m_OfficicalMsgWidget->SetScrollbars(0, 30, 1, 700 / 20);
		m_OfficicalMsgWidget->Hide();

		wxBoxSizer* pOfficHSizer = new wxBoxSizer(wxHORIZONTAL);
		wxCursor handCursor(wxCURSOR_HAND);
		//printer scrolled widget
		m_printerMsgWidget = new wxScrolledWindow(m_printerPanel, wxID_ANY);
		
		m_printerMsgWidget->Bind(wxEVT_SCROLLWIN_THUMBRELEASE, [this](wxScrollWinEvent& event) {			
			event.Skip();
			isLoadingData(true);
			});

		m_printerMsgWidget->Bind(wxEVT_SCROLLWIN_LINEDOWN, [this](wxScrollWinEvent& event) {
			event.Skip();
			isLoadingData();			
			});

		m_printerMsgWidget->Bind(wxEVT_SCROLLWIN_LINEUP, [this](wxScrollWinEvent& event) {
			event.Skip();
			showNataTips(false);
			});

		m_limitTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
			m_limitRequest = false;
		});

		m_pPrinterScrolledVSizer = new wxBoxSizer(wxVERTICAL);
		m_printerMsgWidget->SetMinSize(AnkerSize(720, 600));
		m_printerMsgWidget->SetMaxSize(AnkerSize(720, 600));
		m_printerMsgWidget->SetSize(AnkerSize(720, 600));
		m_printerMsgWidget->SetSizer(m_pPrinterScrolledVSizer);
		m_printerMsgWidget->SetBackgroundColour(BG_COLOR);		
		m_printerMsgWidget->SetVirtualSize(AnkerSize(720, 600));
		m_printerMsgWidget->SetScrollRate(0, 30);
		m_printerMsgWidget->SetScrollbars(0, 30, 1, 720 / 20);

		m_pNodatasPanel = new wxPanel(m_printerMsgWidget, wxID_ANY);
		m_pNodatasPanel->SetMaxSize(AnkerSize(700, 30));
		m_pNodatasPanel->SetMinSize(AnkerSize(700, 30));
		m_pNodatasPanel->SetSize(AnkerSize(700, 30));
		m_pNodatasPanel->SetBackgroundColour(BG_COLOR);		
		wxBoxSizer* pNodataHSizer = new wxBoxSizer(wxHORIZONTAL);
		m_pNodatasPanel->SetSizer(pNodataHSizer);

		wxStaticText* pNodataLabel = new wxStaticText(m_pNodatasPanel, wxID_ANY, _L("msg_center_no_any_data"),
			wxDefaultPosition, wxDefaultSize,
			wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
		pNodataLabel->SetFont(ANKER_FONT_NO_1);
		pNodataLabel->SetBackgroundColour(BG_COLOR);
		pNodataLabel->SetForegroundColour(wxColor("#707174"));
		pNodataLabel->SetMaxSize(AnkerSize(100, 20));
		pNodataLabel->SetMinSize(AnkerSize(100, 20));
		pNodataLabel->SetSize(AnkerSize(100, 20));
		pNodataHSizer->AddStretchSpacer();
		pNodataHSizer->Add(pNodataLabel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		pNodataHSizer->AddStretchSpacer();

		m_pNodatasPanel->Hide();

		m_EmptyPanel = new wxPanel(this, wxID_ANY);		
		m_EmptyPanel->SetMaxSize(AnkerSize(700,630));
		m_EmptyPanel->SetMinSize(AnkerSize(700,630));
		m_EmptyPanel->SetSize(AnkerSize(700,630));
		m_EmptyPanel->SetBackgroundColour(BG_COLOR);

		wxBoxSizer* pEmptyVSizer = new wxBoxSizer(wxVERTICAL);
		m_EmptyPanel->SetSizer(pEmptyVSizer);		
		
		wxImage emptyMsgImage = wxImage(wxString::FromUTF8(Slic3r::var("emptyMsg.png")), wxBITMAP_TYPE_PNG);		
		emptyMsgImage.Rescale(AnkerLength(48), AnkerLength(48), wxIMAGE_QUALITY_HIGH);
		wxBitmap bitmap(emptyMsgImage);
		wxStaticBitmap* pEmptyImg = new wxStaticBitmap(m_EmptyPanel, wxID_ANY, bitmap);
		pEmptyImg->SetMaxSize(AnkerSize(48, 48));
		pEmptyImg->SetMinSize(AnkerSize(48, 48));
		pEmptyImg->SetSize(AnkerSize(48, 48));
		
		wxStaticText* pEmptyLabel = new wxStaticText(m_EmptyPanel, wxID_ANY, _L("msg_center_empty_tips"),
													wxDefaultPosition, wxDefaultSize,
													wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
		pEmptyLabel->SetFont(ANKER_FONT_NO_1);
		pEmptyLabel->SetBackgroundColour(BG_COLOR);
		pEmptyLabel->SetForegroundColour(wxColor("#707174"));
		pEmptyLabel->SetMaxSize(AnkerSize(250, 40));
		pEmptyLabel->SetMinSize(AnkerSize(250, 40));
		pEmptyLabel->SetSize(AnkerSize(250, 40));
		
		pEmptyVSizer->AddStretchSpacer();
		pEmptyVSizer->Add(pEmptyImg, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
		pEmptyVSizer->AddSpacer(16);
		pEmptyVSizer->Add(pEmptyLabel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);		
		pEmptyVSizer->AddStretchSpacer();

	}
	pPrinterVSizer->Add(m_printerMsgWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);

	pContentWindowHSizer->Add(m_OfficicalMsgWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pContentWindowHSizer->Add(m_printerPanel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pContentWindowHSizer->Add(m_EmptyPanel, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

	pWidgetVSizer->Add(pContentWindowHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	pWidgetVSizer->AddSpacer(12);
	
	m_printerPanel->Hide();
	m_EmptyPanel->Hide();

	pMainVSizer->Add(pWidget, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
	SetSizer(pMainVSizer);
	isShowEditMode(false);
}
void AnkerMsgCentreDialog::showLoading(bool visible)
{
	// loading frame
	if (m_loadingMask == nullptr)
	{
		m_loadingMask = new AnkerLoadingMask(this, 30000, false);
		m_loadingMask->setText("");
		m_loadingMask->Bind(wxANKEREVT_LOADING_TIMEOUT, [this](wxCommandEvent& event) {			
			updateCurrentPage();
			showLoading(false);
			});		
	}

	int x, y;
	GetScreenPosition(&x, &y);
	wxSize totalSize = GetSize();
	wxSize clientSize = GetClientSize();
	int borderWidth = (totalSize.x - clientSize.x) / 2;
	int borderHeight = (totalSize.y - clientSize.y) / 4;
	int titleHeight = totalSize.GetHeight() - clientSize.GetHeight();
	m_loadingMask->updateMaskRect(wxPoint(x + borderWidth, y + titleHeight - borderHeight), clientSize);
	m_loadingMask->Show(visible);
	if (visible)
		m_loadingMask->start();
	else
		m_loadingMask->stop();
}

void AnkerMsgCentreDialog::isShowEditMode(bool isEdit)
{
	if (isEdit)
	{				
		m_isEditmode = true;		
		m_printerTitle->Hide();
		m_editBtn->Hide();
		m_printerTitleSpcaer->Hide();

		m_selectPanel->Show();
		m_cancelBtn->Show();
		m_deleteBtn->Show();
	}
	else
	{		
		m_isEditmode = false;	
		m_selectAllBtnStatus = false;
		m_printerTitle->Show();
		m_editBtn->Show();		
		m_printerTitleSpcaer->Show();

		m_selectPanel->Hide();
		m_cancelBtn->Hide();
		m_deleteBtn->Hide();
	}

	Refresh();
	Layout();
}

void AnkerMsgCentreDialog::isLoadingData(bool iskeyEvent)
{
	if (m_limitRequest)
		return;

	int x, y;
	m_printerMsgWidget->GetViewStart(&x, &y);

	int unitsX, unitsY;
	m_printerMsgWidget->GetScrollPixelsPerUnit(&unitsX, &unitsY);

	int clientX, clientY;
	m_printerMsgWidget->GetClientSize(&clientX, &clientY);

	int virtualHeight;
	m_printerMsgWidget->GetVirtualSize(NULL, &virtualHeight);

	if ((y * unitsY + clientY) >= virtualHeight)
	{
		getMsgCenterRecords();
		m_limitTimer.Start(2000);
		m_limitRequest = true;
	}
	else
	{
		if (iskeyEvent)
			showNataTips(false);
	}
}

void AnkerMsgCentreDialog::onMove(wxMoveEvent& event)
{
	if (m_loadingMask && m_loadingMask->IsShown())
	{
		int x, y;
		GetScreenPosition(&x, &y);
		wxSize totalSize = GetSize();
		wxSize clientSize = GetClientSize();
		int borderWidth = (totalSize.x - clientSize.x) / 2;
		int borderHeight = (totalSize.y - clientSize.y) / 4;
		int titleHeight = totalSize.GetHeight() - clientSize.GetHeight();
		m_loadingMask->updateMaskRect(wxPoint(x + borderWidth, y + titleHeight - borderHeight), clientSize);		
	}
}
void AnkerMsgCentreDialog::onNoRecords()
{
	m_EmptyPanel->Show();
	m_OfficicalMsgWidget->Hide();
	m_printerPanel->Hide();

	Refresh();
	Layout();
}

void AnkerMsgCentreDialog::onOfficialBtn(wxCommandEvent& event)
{
	this->Thaw();
	onShowOfficealWidget(true);
	m_pPrinterBtn->setStatus(MsgNormalBtnStatus);
	
	Refresh();
	Layout();
	this->Freeze();
}

void AnkerMsgCentreDialog::onPrinterBtn(wxCommandEvent& event)
{
	this->Thaw();
	onShowOfficealWidget(false);
	m_pOfficialBtn->setStatus(MsgNormalBtnStatus);
	Refresh();
	Layout();
	this->Freeze();
}

void AnkerMsgCentreDialog::updateCurrentPage()
{
	std::unique_lock lock(m_currentPageMutex);
	int counts = 0;
	if (m_MsgRecords)
	{ 
		counts = m_MsgRecords->size();
	}
			
	if (counts % 10 == 0 && counts != 0)
		m_currentPage = counts / 10 + 1;
	else if (counts == 0)
		m_currentPage = 0;
	else
		m_currentPage = counts / 10 ;
}

void AnkerMsgCentreDialog::getMsgCenterRecords(bool isSyn)
{
	auto ankerNet = AnkerNetInst();
	if (!ankerNet || !ankerNet->IsLogined()) {
		return;
	}
	showLoading(true);
	std::unique_lock lock(m_currentPageMutex);

	if (isSyn)
	{
		m_currentPage = 0;
	}

	//1 offical 2 printer ,pages records ,request pages
	ankerNet->GetMsgCenterRecords(2, m_pagesRecords, m_currentPage, isSyn);
	m_currentPage += 1;
}

void AnkerMsgCentreDialog::setMsgAllRead()
{
	wxSizerItemList& childrenList = m_pPrinterScrolledVSizer->GetChildren();

	for (wxSizerItemList::iterator it = childrenList.begin(); it != childrenList.end(); ++it) {
		wxSizerItem* item = *it;

		if (item->IsWindow()) {
			AnkerCustomMsg* pWindow = static_cast<AnkerCustomMsg*>(item->GetWindow());

			if (pWindow)
			{						
				pWindow->setReadStatus(true);
			}
		}
	}
}

void AnkerMsgCentreDialog::showNataTips(bool isShow)
{
	if (isShow)
	{
		m_pPrinterScrolledVSizer->Add(m_pNodatasPanel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
		m_pNodatasPanel->Show();
	}
	else
	{
		bool res = m_pPrinterScrolledVSizer->Detach(m_pNodatasPanel);		
		m_pNodatasPanel->Hide();
	}
	Layout();
}
void AnkerMsgCentreDialog::onShowOfficealWidget(bool isShow)
{
	if (isShow)
	{
		if (m_officialMsgVec.size() > 0)
		{
			m_OfficicalMsgWidget->Show();
			m_printerPanel->Hide();
			m_EmptyPanel->Hide();

			m_pOfficialBtn->setStatus(MsgPressedBtnStatus);
			m_pPrinterBtn->setStatus(MsgNormalBtnStatus);
		}
		else
		{
			onNoRecords();
		}
	}
	else
	{
		if (m_printerMsgVec.size() > 0)
		{
			m_OfficicalMsgWidget->Hide();
			m_EmptyPanel->Hide();
			m_printerPanel->Show();
			m_pOfficialBtn->setStatus(MsgNormalBtnStatus);
			m_pPrinterBtn->setStatus(MsgPressedBtnStatus);
		}
		else
		{
			onNoRecords();
		}
	}

	Refresh();
	Layout();
}
void AnkerMsgCentreDialog::showPrinterWidget(bool isShow)
{
	if (isShow)
	{
		m_OfficicalMsgWidget->Hide();
		m_EmptyPanel->Hide();
		m_printerPanel->Show();
		m_pOfficialBtn->setStatus(MsgNormalBtnStatus);
		m_pPrinterBtn->setStatus(MsgPressedBtnStatus);
	}
	else
	{
		m_OfficicalMsgWidget->Show();
		m_printerPanel->Hide();
		m_EmptyPanel->Hide();

		m_pOfficialBtn->setStatus(MsgPressedBtnStatus);
		m_pPrinterBtn->setStatus(MsgNormalBtnStatus);
	}
}

void AnkerMsgCentreDialog::showMsgCenterWidget()
{	
	//only show printer widget
	//if (m_officialMsgVec.size() > 0)
	//{
	//	m_OfficicalMsgWidget->Show();
	//	m_printerPanel->Hide();
	//	m_EmptyPanel->Hide();

	//	m_pOfficialBtn->setStatus(MsgPressedBtnStatus);
	//	m_pPrinterBtn->setStatus(MsgNormalBtnStatus);
	//}
	//else
	if (m_printerMsgVec.size() > 0)
	{
		m_OfficicalMsgWidget->Hide();
		m_EmptyPanel->Hide();
		m_printerPanel->Show();
		m_pOfficialBtn->setStatus(MsgNormalBtnStatus);
		m_pPrinterBtn->setStatus(MsgPressedBtnStatus);
	}
	else
	{
		onNoRecords();
	}

	Refresh();
	Layout();
}

BEGIN_EVENT_TABLE(AnkerMsgTitleBtn, wxControl)
EVT_PAINT(AnkerMsgTitleBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerMsgTitleBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerMsgTitleBtn::OnLeave)
EVT_LEFT_DOWN(AnkerMsgTitleBtn::OnPressed)

EVT_SIZE(AnkerMsgTitleBtn::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerMsgTitleBtn, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_MSG_BTN_CLICKED, wxCommandEvent);

AnkerMsgTitleBtn::AnkerMsgTitleBtn()
{
}

AnkerMsgTitleBtn::AnkerMsgTitleBtn(wxWindow* parent, wxWindowID id,
	wxColor textNorColor,
	wxColor textSelectColor,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

AnkerMsgTitleBtn::~AnkerMsgTitleBtn()
{

}

void AnkerMsgTitleBtn::initUi()
{

}

void AnkerMsgTitleBtn::OnEnter(wxMouseEvent& event)
{
	if (m_status != MsgPressedBtnStatus)
	{	
		m_status = MsgEnterBtnStatus;
		Refresh();
	}
}
void AnkerMsgTitleBtn::OnLeave(wxMouseEvent& event)
{
	if (m_status != MsgPressedBtnStatus)
	{
		m_status = MsgNormalBtnStatus;
		Refresh();
	}
}

void AnkerMsgTitleBtn::initData(const wxString& btnName, MsgBtnStatus status)
{
	m_text = btnName;
	m_status = status;
	Refresh();
}

void AnkerMsgTitleBtn::setStatus(MsgBtnStatus status)
{
	m_status = status;
	Refresh();
}

void AnkerMsgTitleBtn::OnPressed(wxMouseEvent& event)
{
	m_status = MsgPressedBtnStatus;
	Refresh();	

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_MSG_BTN_CLICKED);
	wxVariant eventData;
	eventData.ClearList();
	evt.SetClientData(new wxVariant(eventData));
	evt.SetEventObject(this);
	ProcessEvent(evt);
}

void AnkerMsgTitleBtn::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rectArea = GetClientRect();		
	wxColor selectBgColor = wxColor("#3e4c3f");
	wxColor normalBgColor = BG_COLOR;

	wxColor normalTextColor = wxColor("#adaeaf");
	wxColor SelectTextColor = wxColor("#80d16f");

	rectArea.SetX(rectArea.x - 2);
	rectArea.SetY(rectArea.y - 2);

	rectArea.SetHeight(rectArea.height + 3);
	rectArea.SetWidth(rectArea.width + 3);

	switch (m_status)
	{
	case MsgNormalBtnStatus:
	case MsgLeaveBtnStatus:
	{
		dc.Clear();

		dc.SetBrush(wxBrush(wxColor("#333438")));
		dc.SetTextForeground(normalTextColor);
		dc.SetFont(ANKER_FONT_NO_1);
		dc.DrawRectangle(rectArea);		
		
		wxSize textSize = dc.GetTextExtent(m_text);
		int textX = (rectArea.GetWidth() - textSize.GetWidth()) / 2;
		int textY = (rectArea.GetHeight() - textSize.GetHeight()) / 2;

		wxPoint realPoint(textX, textY);
		dc.DrawText(m_text, realPoint);
	}
	break;
	case MsgEnterBtnStatus:
	case MsgPressedBtnStatus:
	{
		dc.Clear();		
		dc.SetBrush(wxBrush(wxColor("#3e4c3f")));		
		dc.SetTextForeground(SelectTextColor);
		dc.SetFont(ANKER_FONT_NO_1);//ANKER_BOLD_FONT_NO_1
		dc.DrawRectangle(rectArea);				

		wxSize textSize = dc.GetTextExtent(m_text);
		int textX = (rectArea.GetWidth() - textSize.GetWidth()) / 2;
		int textY = (rectArea.GetHeight() - textSize.GetHeight()) / 2;

		wxPoint realPoint(textX, textY);
		dc.DrawText(m_text, realPoint);
	}	
	break;	
	defualt:
		
		dc.Clear();

		dc.SetBrush(wxBrush(wxColor("#333438")));
		dc.SetTextForeground(normalTextColor);
		dc.SetFont(ANKER_FONT_NO_1);
		dc.DrawRectangle(rectArea);				

		wxSize textSize = dc.GetTextExtent(m_text);
		int textX = (rectArea.GetWidth() - textSize.GetWidth()) / 2;
		int textY = (rectArea.GetHeight() - textSize.GetHeight()) / 2;

		wxPoint realPoint(textX, textY);
		dc.DrawText(m_text, realPoint);		
	}
}

void AnkerMsgTitleBtn::OnSize(wxSizeEvent& event)
{
	Refresh();
}
BEGIN_EVENT_TABLE(AnkerMsgCircleLabel, wxControl)
EVT_PAINT(AnkerMsgCircleLabel::OnPaint)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(AnkerMsgCircleLabel, wxControl)
AnkerMsgCircleLabel::AnkerMsgCircleLabel()
{

}

AnkerMsgCircleLabel::AnkerMsgCircleLabel(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_Color = BG_COLOR;
}

AnkerMsgCircleLabel::~AnkerMsgCircleLabel()
{

}
void AnkerMsgCircleLabel::setBgColor(wxColor color)
{
	m_Color = color;
	Refresh();
}

void AnkerMsgCircleLabel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

	wxRect rectArea = GetClientRect();
	int rCenter = GetClientRect().width / 2;

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_Color);
	rectArea.SetSize(AnkerSize(rectArea.width + 20, rectArea.height + 20));
	dc.DrawRectangle(rectArea);

	gc->SetBrush(wxColor("#FF4F4F"));
	gc->SetPen(wxPen(wxColor("#FF4F4F")));
	wxGraphicsPath contentPath = gc->CreatePath();
	contentPath.AddCircle(5, 5, 3);
	gc->DrawPath(contentPath);

}

BEGIN_EVENT_TABLE(AnkerMsgPageBtn, wxControl)
EVT_PAINT(AnkerMsgPageBtn::OnPaint)
EVT_LEFT_DOWN(AnkerMsgPageBtn::OnPressed)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(AnkerMsgPageBtn, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_PAGE_BTN_CLICKED, wxCommandEvent);
AnkerMsgPageBtn::AnkerMsgPageBtn()
{

}
AnkerMsgPageBtn::AnkerMsgPageBtn(wxWindow* parent, wxWindowID id,
	wxImage norImg,
	wxImage disImg,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator):
	  m_norImg(norImg)
	, m_disImg(disImg)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	init();
}
AnkerMsgPageBtn::~AnkerMsgPageBtn()
{

}

void AnkerMsgPageBtn::setSatus(PageBtnStatus btnStatus)
{
	m_status = btnStatus;
	Refresh();
	Layout();
}

void AnkerMsgPageBtn::setBgColor(wxColor color)
{
	m_bgColor = color;
	Refresh();
	Layout();
}

void AnkerMsgPageBtn::init()
{
	m_bgColor = wxColor("#333438");	
	m_status = PageNormalBtnStatus;
	Refresh();
	Layout();
}

void AnkerMsgPageBtn::OnPressed(wxMouseEvent& event)
{
	if (m_status == PageDisableBtnStatus)
		return;
	
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_PAGE_BTN_CLICKED);
	wxVariant eventData;
	eventData.ClearList();
	evt.SetClientData(new wxVariant(eventData));
	evt.SetEventObject(this);
	ProcessEvent(evt);
}

void AnkerMsgPageBtn::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	PrepareDC(dc);

	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

	dc.SetBrush(wxBrush(m_bgColor));
	dc.SetPen(wxPen(m_bgColor));

	auto widgetRect = GetClientRect();
	widgetRect.SetSize(AnkerSize(widgetRect.width + 20, widgetRect.height + 20));	
	dc.DrawRectangle(widgetRect);

	int w, h;
	GetClientSize(&w, &h);
	int squareSize = std::min(w, h);

	wxImage currentImg = wxImage();
	if (m_status == PageNormalBtnStatus)
	{
		currentImg = m_norImg;

	}
	else//disable
	{
		currentImg = m_disImg;
	}	

	wxDouble imgWidth = currentImg.GetWidth();
	wxDouble imgHeight = currentImg.GetHeight();
	wxDouble scaleX = squareSize / imgWidth;
	wxDouble scaleY = squareSize / imgHeight;
	wxDouble scale = std::min(scaleX, scaleY);

	wxDouble scaledWidth = imgWidth;
	wxDouble scaledHeight = imgHeight;
	wxDouble posX = (w - scaledWidth) / 2;
	wxDouble posY = (h - scaledHeight) / 2;


	wxGraphicsBitmap bitmap = gc->CreateBitmapFromImage(currentImg);
	gc->DrawBitmap(bitmap, posX, posY, scaledWidth, scaledHeight);

	delete gc;
}

BEGIN_EVENT_TABLE(AnkerCustomMsg, wxControl)
EVT_PAINT(AnkerCustomMsg::OnPaint)
EVT_LEFT_DOWN(AnkerCustomMsg::OnPressed)
EVT_ENTER_WINDOW(AnkerCustomMsg::OnEnter)
EVT_LEAVE_WINDOW(AnkerCustomMsg::OnLeave)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(AnkerCustomMsg, wxControl)

AnkerCustomMsg::AnkerCustomMsg(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void AnkerCustomMsg::setReadStatus(bool isRead)
{
	m_isRead = isRead;
	Refresh();
}

void AnkerCustomMsg::initMsg(const wxString& msgTitle,
	const wxString& time,
	const wxString& content,
	const wxString& url,
	int msgID,
	bool isRead /*= false*/)
{	
	m_msgTitle = msgTitle;
	m_time = time;
	m_content = content;
	m_url = url;
	m_msgID = msgID;
	m_isRead = isRead;
	
	SetFont(ANKER_FONT_NO_1);
	int contentHeight = GetTextExtent(m_content).GetHeight();	
	int contentLength = GetTextExtent(m_content).GetWidth();
	int heightFactor = 0;

	if (contentLength % 600 == 0)
		heightFactor = contentLength / 600;
	else
		heightFactor = contentLength / 600 + 1;
	
	wxSize winSize = wxSize(AnkerLength(720), contentHeight * heightFactor + AnkerLength(58));
	wxSize winSizeEx = AnkerSize(720, contentHeight * heightFactor + 42);
	SetMaxSize(winSize);
	SetMinSize(winSize);
	SetSize(winSize);

	Refresh();	
}

void AnkerCustomMsg::setEditMode(bool isEdit)
{
	m_isEditMode = isEdit;
	Refresh();
	Layout();
}

void AnkerCustomMsg::setSelect(bool isSelect)
{
	m_isSelect = isSelect;
	Refresh();
	Layout();
}

void AnkerCustomMsg::OnPressed(wxMouseEvent& event)
{
	if (m_isEditMode)
	{
		m_isSelect = !m_isSelect;
	}
	else
	{
		if (m_url.empty())
		{
			ANKER_LOG_INFO << "url is null:" << m_content << "msgID:" << m_msgID;
			event.Skip();
			return;
		}

		setReadStatus(true);
		std::string realUrl = m_url.ToStdString();
		wxLaunchDefaultBrowser(realUrl.c_str());
	}
	Refresh();
	Layout();
	event.Skip();
}

void AnkerCustomMsg::OnEnter(wxMouseEvent& event)
{
	m_currentBgColor = m_hoverColor;
	Refresh();
	event.Skip();	
}

void AnkerCustomMsg::OnLeave(wxMouseEvent& event)
{
	m_currentBgColor = m_bgColor;
	Refresh();
	event.Skip();
	
}
std::string AnkerCustomMsg::warpString(const wxString& str, wxFont font, const int& lineLength)
{
	std::string strRes = "";
	std::string tmpStr = str.ToStdString();
	int strWidths = 0;
	
	wxSize size = GetTextExtent(str);
	for (size_t i = 0; i < tmpStr.length(); i++) {
		wxSize singleCharacterSize = GetTextExtent(str[i]);
		strWidths += singleCharacterSize.GetWidth();

		if (strWidths < lineLength)
			strRes += tmpStr[i];
		else
		{
			strRes += '\n';
			strWidths = 0;
		}
	}
	return strRes;
}
void AnkerCustomMsg::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
	wxColor titleColor("#FFFFFF");
	wxColor timeColor("#707174");
	wxColor contentColor("#adaeaf");
	wxColor redPointColor("#ec5d56");

	wxRect rectArea = GetClientRect();
	int rCenter = GetClientRect().width / 2;

	//draw background color
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_currentBgColor);	
	rectArea.SetSize(wxSize(rectArea.width + 20, rectArea.height + 20));
	dc.DrawRectangle(rectArea);

	//draw red point

	if (m_isEditMode)
	{
		wxImage image = wxImage();
		if (m_isSelect)
		{
			image = wxImage(wxString::FromUTF8(Slic3r::var("msg_check.png")), wxBITMAP_TYPE_PNG);
		}
		else
		{
			image = wxImage(wxString::FromUTF8(Slic3r::var("msg_uncheck.png")), wxBITMAP_TYPE_PNG);
		}

		image.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
		wxGraphicsBitmap bitmapEx = gc->CreateBitmapFromImage(image);
		gc->DrawBitmap(bitmapEx, AnkerLength(16), AnkerLength(16), AnkerLength(16), AnkerLength(16));
	}
	else
	{
		if (!m_isRead)
		{
			gc->SetBrush(redPointColor);
			gc->SetPen(wxPen(redPointColor));
			wxGraphicsPath contentPath = gc->CreatePath();
			contentPath.AddCircle(29, 26, 3);
			gc->DrawPath(contentPath);
		}
	}	

	//draw title 
	wxBrush titleBrush(titleColor);
	gc->SetBrush(titleBrush);
	gc->SetFont(ANKER_FONT_NO_1, titleColor);
	gc->SetPen(wxPen(titleColor));

	double titleStrWidth = 0;
	double titleStrHeight = 0;
	gc->GetTextExtent(m_msgTitle, &titleStrWidth, &titleStrHeight);

	if (!m_msgTitle.empty())
		gc->DrawText(m_msgTitle, AnkerLength(40), AnkerLength(16));

	//draw time	
	wxBrush timeBrush(timeColor);
	gc->SetBrush(timeBrush);
	gc->SetFont(ANKER_FONT_NO_1, timeColor);
	gc->SetPen(wxPen(timeColor));

	double timeStrWidth = 0;
	double timeStrHeight = 0;
	gc->GetTextExtent(m_time, &timeStrWidth, &timeStrHeight);
	int timeStartPos = GetClientRect().width - timeStrWidth - AnkerLength(50);

	if (!m_time.empty())
		gc->DrawText(m_time, timeStartPos, AnkerLength(16));

	//draw content
	wxBrush contentBrush(contentColor);
	gc->SetBrush(contentBrush);
	gc->SetFont(ANKER_FONT_NO_1, contentColor);
	gc->SetPen(wxPen(contentColor));

	double contentStrWidth = 0;
	double contentStrHeight = 0;
	gc->GetTextExtent(m_content, &contentStrWidth, &contentStrHeight);
	int contentStartPos = GetClientRect().width - contentStrWidth;	

	int contentPosX = GetClientRect().x + AnkerLength(40);
	int contentPosY = (titleStrHeight + AnkerLength(26));

	wxString contentStr = Slic3r::GUI::WrapEveryCharacter(m_content, ANKER_FONT_NO_1, AnkerLength(630));
	//wxString contentStr = Slic3r::GUI::WrapStr(m_content, ANKER_FONT_NO_1, AnkerLength(630));
	//std::string contentStr = warpString(m_content, ANKER_FONT_NO_1, AnkerLength(600));
	//std::string contentStr = m_content.ToStdString();

	int contentwidth = 0;
	int contentHeight = 0;

	dc.GetTextExtent(m_content, &contentwidth, &contentHeight);
	if (!contentStr.empty())	
	{
		dc.SetBrush(contentBrush);
		dc.SetFont(ANKER_FONT_NO_1);
		dc.SetPen(wxPen(contentColor));
		dc.SetTextForeground(contentColor);		
		dc.DrawText(contentStr, contentPosX, contentPosY);
	}

	//draw arrowLog	
	if(!m_url.empty())
	{		
		wxImage image = wxImage();
		if(m_isEditMode)
			image = wxImage(wxString::FromUTF8(Slic3r::var("disable_right_nor.png")), wxBITMAP_TYPE_PNG);		
		else
			image = wxImage(wxString::FromUTF8(Slic3r::var("right_nor.png")), wxBITMAP_TYPE_PNG);
		image.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
		wxGraphicsBitmap bitmapEx = gc->CreateBitmapFromImage(image);
		int imgPosX = GetClientRect().width - AnkerLength(64);
		int imgPosY = timeStrHeight + AnkerLength(26) + (contentHeight - AnkerLength(16)) / 2;

		gc->DrawBitmap(bitmapEx, imgPosX, imgPosY, AnkerLength(16), AnkerLength(16));
	}

	if (gc)
	{
		delete gc;
		gc = nullptr;
	}
}

BEGIN_EVENT_TABLE(AnkerMsgCheckBox, wxControl)
EVT_PAINT(AnkerMsgCheckBox::OnPaint)
EVT_LEFT_DOWN(AnkerMsgCheckBox::OnPressed)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(AnkerMsgCheckBox, wxControl)

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CHECK_BOX_CLICKED, wxCommandEvent);

AnkerMsgCheckBox::AnkerMsgCheckBox()
{

}
AnkerMsgCheckBox::AnkerMsgCheckBox(wxWindow* parent, wxWindowID id,
	wxString label,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator) :m_label(label)
{
	Create(parent, id, pos, size, style, validator);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	init();
}
AnkerMsgCheckBox::~AnkerMsgCheckBox()
{

}

void AnkerMsgCheckBox::init()
{

}

void AnkerMsgCheckBox::resetStatus()
{
	m_isCheck = false;
	Refresh();
	Layout();
}

void AnkerMsgCheckBox::setText(wxString label)
{
	m_label = label;
	Refresh();
	Layout();
}

void AnkerMsgCheckBox::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);	
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	wxRect rectArea = GetClientRect();
	
	dc.SetBackground(wxColor("#333438"));
	dc.SetPen(wxPen(wxColor("#333438")));	
	dc.SetBrush(wxBrush(wxColor("#333438")));	
	
	rectArea.SetPosition(wxPoint(rectArea.x - 20, rectArea.y - 20));
	dc.DrawRectangle(rectArea);
	 
	wxImage image = wxImage();
	if(m_isCheck)
	{
		image = wxImage(wxString::FromUTF8(Slic3r::var("msg_check.png")), wxBITMAP_TYPE_PNG);		
	}
	else
	{
		image = wxImage(wxString::FromUTF8(Slic3r::var("msg_uncheck.png")), wxBITMAP_TYPE_PNG);
	}

	image.Rescale(AnkerLength(16), AnkerLength(16), wxIMAGE_QUALITY_HIGH);
	wxGraphicsBitmap bitmapEx = gc->CreateBitmapFromImage(image);
	//,gc->DrawBitmap(bitmapEx, 0, 0, AnkerLength(16), AnkerLength(16));

	//draw text
	dc.SetBrush(wxBrush(wxColor("#333438")));
	dc.SetTextForeground("#FFFFFF");
	dc.SetFont(ANKER_FONT_NO_1);	

	wxSize textSize = dc.GetTextExtent(m_label);
	int textX = (rectArea.GetWidth() - textSize.GetWidth()) / 2 + AnkerLength(22);
	int textY = (rectArea.GetHeight() - textSize.GetHeight()) / 2;

	gc->DrawBitmap(bitmapEx, 0, textY, AnkerLength(16), AnkerLength(16));
	wxPoint realPoint(textX, textY);
	dc.DrawText(m_label, realPoint);

	if (gc)
	{
		delete gc;
		gc = nullptr;
	}
	
}

void AnkerMsgCheckBox::OnPressed(wxMouseEvent& event)
{
	m_isCheck = !m_isCheck;
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CHECK_BOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);
	Update();
		
}
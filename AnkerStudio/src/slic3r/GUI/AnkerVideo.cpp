#include "libslic3r/Utils.hpp"
#include "GUI_App.hpp"
#include "AnkerVideo.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include <slic3r/GUI/GcodeVerify/PrintCheckHint.hpp>
#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetBase.h"
#include "DeviceObjectBase.h"
#include "../AnkerComFunction.hpp"


extern AnkerPlugin* pAnkerPlugin;
static std::string num2Str(long long num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

std::string GetCurrentTimestampAsString()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&currentTime), "%Y%m%d_%H%M%S") << "." << milliseconds.count();

	return ss.str();
}

long long getThreadIdAsLongLong() {
	std::thread::id threadId = std::this_thread::get_id();

	std::hash<std::thread::id> hasher;
	long long threadIdValue = static_cast<long long>(hasher(threadId));

	return threadIdValue;
}

wxDEFINE_EVENT(wxCUSTOMEVT_FRAME_DATA_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_UI_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(imageDisplayer, wxPanel)
EVT_PAINT(imageDisplayer::OnPaint)
EVT_MOTION(imageDisplayer::OnMouseOver)
EVT_LEFT_DOWN(imageDisplayer::OnLeftButtonDown)
wxEND_EVENT_TABLE()


wxBEGIN_EVENT_TABLE(AnkerVideo, wxPanel)
EVT_SIZE(AnkerVideo::OnSize)
EVT_CLOSE(AnkerVideo::OnCloseWindow)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(CustomComboBox, wxWindow)
EVT_PAINT(CustomComboBox::OnPaint)
EVT_LEFT_DOWN(CustomComboBox::OnMouseLeftDown)
EVT_MOUSE_EVENTS(CustomComboBox::OnMouse)
wxEND_EVENT_TABLE()


CustomPopupMenu::CustomPopupMenu(wxWindow* parent) : wxPopupTransientWindow(parent)
{
	m_menuTotalHeight = 0;
	m_menuItemWidth = 120;
	m_menuItemHeight = 30;
	m_marginLeft = 10;
	m_marginRight = 10;
	m_fondSize = 16;

	m_hoveredMenuItemColor = wxColour(113, 211, 90);
	m_selectedMenuItemColor = wxColour(113, 211, 90);

	// SetBackgroundColour(wxColour(41, 42, 45));

	Bind(wxEVT_PAINT, &CustomPopupMenu::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &CustomPopupMenu::OnMouseLeftDown, this);
	Bind(wxEVT_MOTION, &CustomPopupMenu::OnMouseMotion, this);
}

void CustomPopupMenu::AddMenuItem(const wxString& label)
{
	m_menuItems.push_back(label);
	UpdateMenuHeight();
	Layout();
}

void CustomPopupMenu::SetSelectedMenuItem(wxString text)
{
	m_selectedMenuItem = text;
}
/*
wxString CustomPopupMenu::GetSelectedMenuItem() 
{
	return m_selectedMenuItem;
}
*/
void CustomPopupMenu::setMenuItemWidth(int w)
{
	m_menuItemWidth = w;
}

int CustomPopupMenu::GetMenuItemWidth()
{
	return m_menuItemWidth;
}

void CustomPopupMenu::setMenuItemHeight(int h)
{
	m_menuItemHeight = h;
}
void CustomPopupMenu::setFontSize(int size)
{
	m_fondSize = size;
}
void CustomPopupMenu::setLeftMargin(int value)
{
	m_marginLeft = value;
}
void CustomPopupMenu::setRightMargin(int value)
{
	m_marginRight = value;
}

bool CustomPopupMenu::setHightLightMenuColour(wxColour colour)
{
	if (colour.IsOk())
	{
		m_hoveredMenuItemColor = colour;
		return true;
	}
	return false;
}

void CustomPopupMenu::SetMenuItemSelectedCallback(std::function<void(const wxString&)> callback)
{
	m_menuItemSelectedCallback = std::move(callback);
}

void CustomPopupMenu::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	int y = 0;
	int index = 0;
	for (const auto& menuItem : m_menuItems)
	{
		// hightlight the item in mouse position
		if (menuItem == m_hoveredMenuItem) {
			dc.SetBrush(wxBrush(m_hoveredMenuItemColor));
			dc.SetPen(wxPen(m_hoveredMenuItemColor));
			dc.DrawRectangle(wxRect(0, y, GetSize().GetWidth(), m_menuItemHeight));
		}

		// draw menu item text
		dc.SetTextForeground(GetForegroundColour());
		dc.SetFont(ANKER_FONT_NO_1);
		wxSize textSize = dc.GetTextExtent(menuItem);
		//int textX = rectX + (rectWidth - textSize.GetWidth()) / 2;
		int textY = y + (m_menuItemHeight - textSize.GetHeight()) / 2;
		dc.DrawText(menuItem, m_marginLeft, textY);

		// mark the selected menu item
		if (menuItem == m_selectedMenuItem)
		{
			dc.SetPen(wxPen(m_selectedMenuItemColor));
			dc.SetBrush(wxBrush(m_selectedMenuItemColor));
			int radius = 5;
			int cx = m_menuItemWidth - (m_marginRight + radius);
			int cy = y + m_menuItemHeight / 2;
			dc.DrawCircle(cx, cy, radius);
		}

		y += m_menuItemHeight;
		++index;
	}
}

void CustomPopupMenu::OnMouseLeftDown(wxMouseEvent& event)
{
	int menuItemIndex = event.GetY() / m_menuItemHeight;
	if (menuItemIndex >= 0 && menuItemIndex < m_menuItems.size())
	{
		m_selectedMenuItemIndex = menuItemIndex;
		m_selectedMenuItem = m_menuItems[menuItemIndex];

		if (m_menuItemSelectedCallback)
		{
			m_menuItemSelectedCallback(m_selectedMenuItem);
		}
		//wxLogMessage("Selected item: %s", m_selectedMenuItem);
	}

	Dismiss();
}

void CustomPopupMenu::OnMouseMotion(wxMouseEvent& event)
{
	int menuItemIndex = event.GetY() / m_menuItemHeight;
	if (menuItemIndex >= 0 && menuItemIndex < m_menuItems.size())
	{
		m_hoveredMenuItemIndx = menuItemIndex;
		m_hoveredMenuItem = m_menuItems[menuItemIndex];
	}
	else
	{
		m_hoveredMenuItemIndx = -1;
		m_hoveredMenuItem.clear();
	}

	Refresh();
}

void CustomPopupMenu::UpdateMenuHeight()
{
	int itemCount = m_menuItems.size();
	m_menuTotalHeight = itemCount * m_menuItemHeight;
	SetSize(wxSize(m_menuItemWidth, m_menuTotalHeight));
	Refresh();
}





CustomComboBox::CustomComboBox(wxWindow* parent)
	: wxWindow(parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	m_backgroudColor = wxColour(100, 22, 200);
	m_borderColor = wxColour(100, 100, 100);
	m_textColorEnable = GetForegroundColour();
	m_textColorDisable = wxColour(80, 80, 80);
	m_hightLightMenuItemColor = wxColour(113, 211, 90);
	//m_isEnable = true;

}

void CustomComboBox::AddOption(const wxString& option)
{
	m_options.push_back(option);
	if (m_currentOption.empty() && m_options.size() > 0)
	{
		m_currentOption = m_options[0];
	}
}

void CustomComboBox::SetCurrentOption(const wxString& option)
{
	m_currentOption = option;
}

bool CustomComboBox::SetBackgroundColour(const wxColour& colour)
{
	if (colour.IsOk())
	{
		m_backgroudColor = colour;
		return true;
	}
	return false;
}

bool CustomComboBox::SetBorderColour(const wxColour& colour)
{
	if (colour.IsOk())
	{
		m_borderColor = colour;
		return true;
	}
	return false;
}

bool CustomComboBox::setPopupMenuBackgroudColour(wxColour colour)
{
	if (colour.IsOk())
	{
		m_PopupMenuBackgroudColour = colour;
		return true;
	}
	return false;
}

bool CustomComboBox::setHightlightMenuItemColour(wxColour colour)
{
	if (colour.IsOk())
	{
		m_hightLightMenuItemColor = colour;
		return true;
	}
	return false;
}

void CustomComboBox::SetOptionChangeCallback(std::function<void(const wxString&)> callback)
{
	m_optionChangeCallback = std::move(callback);
}

void CustomComboBox::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	dc.SetBrush(/*GetParent()->GetBackgroundColour() */ m_backgroudColor);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(GetClientRect());

	dc.SetBrush(m_backgroudColor);
	dc.SetPen(wxPen(m_borderColor));
	dc.DrawRoundedRectangle(GetClientRect(), 8);

	m_textColorEnable = GetForegroundColour();
	dc.SetTextForeground(IsEnabled() ? m_textColorEnable : m_textColorDisable);
	dc.SetFont(ANKER_FONT_NO_1);
	wxSize textSize = dc.GetTextExtent(m_currentOption);
	int textX = 0 + (GetClientRect().GetWidth() - textSize.GetWidth()) / 2;
	int textY = 0 + (GetClientRect().GetHeight() - textSize.GetHeight()) / 2;
	dc.DrawText(m_currentOption, textX, textY);
}

void CustomComboBox::OnMouse(wxMouseEvent& event)
{
}

void CustomComboBox::OnMouseLeftDown(wxMouseEvent& event)
{
	int mouseX = event.GetX();
	int mouseY = event.GetY();

	CustomPopupMenu* m_popupMenu = new CustomPopupMenu(this);
	m_popupMenu->SetFont(ANKER_FONT_NO_1);
	m_popupMenu->SetBackgroundColour(m_PopupMenuBackgroudColour);
	m_popupMenu->SetForegroundColour(GetForegroundColour());
	m_popupMenu->setHightLightMenuColour(m_hightLightMenuItemColor);
	for (int i = 0; i < m_options.size(); ++i) {
		m_popupMenu->AddMenuItem(m_options[i]);
	}
	m_popupMenu->SetSelectedMenuItem(m_currentOption);

	wxPoint screenPos = ClientToScreen(wxPoint(mouseX - m_popupMenu->GetMenuItemWidth(), mouseY + 20));
	m_popupMenu->SetPosition(screenPos);
	m_popupMenu->SetMenuItemSelectedCallback([this, m_popupMenu](const wxString& menuItem) {
		//wxLogMessage("you Selected item: %s", menuItem);
		if (menuItem != m_currentOption) {
			m_currentOption = menuItem;
			if (m_optionChangeCallback) {
				m_optionChangeCallback(menuItem);
			}
		}
		Refresh();

		if (m_popupMenu) {
			// delete later
			CallAfter([m_popupMenu]() {
				m_popupMenu->Close();
				delete m_popupMenu;
				});
		}
		});
	m_popupMenu->Popup();

	event.Skip();
}




imageDisplayer::imageDisplayer(wxWindow* parent, std::string sn, wxColour bgColor)
    : wxPanel(parent),
	m_sn(sn),
	m_bgColor(bgColor)
{
	wxWindow::SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxCUSTOMEVT_FRAME_DATA_UPDATE, [this](wxCommandEvent& event) {
		MyCustomEvent* myCustomEvent = dynamic_cast<MyCustomEvent*>(&event);
		if (myCustomEvent && !m_bStopRender) {
			DataFrame &frameData =const_cast<DataFrame&> (myCustomEvent->GetCustomData());

			wxImage image(frameData.width, frameData.height, const_cast<unsigned char*>(frameData.frame), true);
			wxBitmap bitmap(image);
			SetBitmap(bitmap);
			Update(); 
		}
		});
}

void imageDisplayer::showOSDItem(VideoOSDItem item, bool show)
{
	if (item == Video_Status_Icon)
		m_showVideoStatusIcon = show;
	else if (item == Video_Status_Msg)
		m_showVideoStatusMsg = show;
	else if (item == Retry_Btn)
		m_showRetryBtn = show;
}

void imageDisplayer::setOSDItem(VideoOSDItem item, std::string value)
{
	if (item == Video_Status_Icon)
		m_videoStatusIcon = value;
	else if (item == Video_Status_Msg)
		m_videoStatusMsg = value;
	else if (item == Retry_Btn)
		m_RetryBtnText = value;
}

void imageDisplayer::setRetryBtnClickCB(std::function<void(void)> fn)
{
	if (!fn)
		return;

	m_retryBtnClickCB = fn;
}

void imageDisplayer::OnLeftButtonDown(wxMouseEvent& event)
{
	wxPoint mousePos = event.GetPosition();
	if (IsInRetryButtonArea(mousePos))
	{
		if (m_retryBtnClickCB)
			m_retryBtnClickCB();
	}
	event.Skip();
}

void imageDisplayer::OnMouseOver(wxMouseEvent& event)
{
	AnkerVideo* video = dynamic_cast<AnkerVideo*>(this->GetParent());
	if (video && video->IsOffLine())
		return;

	wxPoint mousePos = event.GetPosition();
	if (IsInRetryButtonArea(mousePos))
	{
		m_isMouseOnRetryBtn = true;
	}
	else {
		m_isMouseOnRetryBtn = false;
	}
	Update();
	Refresh();

	event.Skip();
}

bool imageDisplayer::IsInRetryButtonArea(const wxPoint& pos)
{
	return pos.x >= m_retryButtonX && pos.x < m_retryButtonX + m_retryButtonWidth
		&& pos.y >= m_retryButtonY && pos.y < m_retryButtonY + m_retryButtonHeight;
}

void imageDisplayer::OnPaint(wxPaintEvent& event)
{
    //wxPaintDC dc(this);
   wxBufferedPaintDC dc(this);
   render(dc);
}

int imageDisplayer::getOffsetY(VideoOSDItem item)
{
	int pixBetweenItem = 50;
	int numItemShow = (int)m_showVideoStatusIcon + (int)m_showVideoStatusMsg + (int)m_showRetryBtn;

	if (numItemShow == 1) {
		return 0;
	}

	if (numItemShow == 3) {
		return ((int)item - (int)Video_Status_Msg) * pixBetweenItem;
	}

	if (numItemShow == 2) {
		if (m_showVideoStatusIcon && m_showVideoStatusMsg && !m_showRetryBtn) {
			if (item == Video_Status_Icon)
				return -1 * (pixBetweenItem /2);
			if (item == Video_Status_Msg)
				return 1 * (pixBetweenItem / 2);
		}

		if (m_showVideoStatusIcon && !m_showVideoStatusMsg && m_showRetryBtn) {
			if (item == Video_Status_Icon)
				return -1 * (pixBetweenItem / 2);
			if (item == Retry_Btn)
				return 1 * (pixBetweenItem / 2);
		}

		if (!m_showVideoStatusIcon && m_showVideoStatusMsg && m_showRetryBtn) {
			if (item == Video_Status_Msg)
				return -1 * (pixBetweenItem / 2);
			if (item == Retry_Btn)
				return 1 * (pixBetweenItem / 2);
		}
	}

	return 0;
}

void imageDisplayer::render(wxDC& dc)
{
	//ANKER_LOG_INFO << ("==============render()");
    int panelWidth, panelHeight;
    GetSize(&panelWidth, &panelHeight);

	wxBrush brush(m_bgColor);
	dc.SetBrush(brush);
	dc.DrawRectangle(0, 0, panelWidth, panelHeight);

    int imageWidth = m_bitmap.GetWidth();
    int imageHeight = m_bitmap.GetHeight();

	// draw video frame
    if (imageWidth > 0 && imageHeight > 0)
    {
        double scale = std::min(static_cast<double>(panelWidth) / imageWidth, static_cast<double>(panelHeight) / imageHeight);

        int drawWidth = static_cast<int>(imageWidth * scale);
        int drawHeight = static_cast<int>(imageHeight * scale);

        int drawX = (panelWidth - drawWidth) / 2;
        int drawY = (panelHeight - drawHeight) / 2;

        wxImage scaledImage = m_bitmap.ConvertToImage().Rescale(drawWidth, drawHeight);
        wxBitmap scaledBitmap(scaledImage);
        dc.DrawBitmap(scaledBitmap, drawX, drawY);
    }

	// draw OSD(On screen Display) info
	if (m_showVideoStatusIcon ) {
		if (!m_videoStatusIcon.empty()) {
			wxImage image(wxString::FromUTF8(Slic3r::var(m_videoStatusIcon)), wxBITMAP_TYPE_PNG);
			image.Rescale(30, 30);
			wxBitmap bitmap(image);
			dc.DrawBitmap(bitmap, (panelWidth - 30) / 2, (panelHeight - 20) / 2 + getOffsetY(Video_Status_Icon));
		}
	}

	if (m_showVideoStatusMsg) {
		dc.SetFont(ANKER_FONT_NO_1);
		dc.SetTextForeground(wxColour(255, 255, 255));
		dc.SetTextBackground(wxColour(0, 0, 0, 0));
		wxSize textSize = dc.GetTextExtent(m_videoStatusMsg);
		int textX = (panelWidth - textSize.GetWidth()) / 2;
		int textY = (panelHeight - textSize.GetHeight()) / 2 + getOffsetY(Video_Status_Msg);
		dc.DrawText(m_videoStatusMsg, textX, textY);
	}

	if (m_showRetryBtn)
	{
		m_retryButtonWidth = 80;
		m_retryButtonHeight = 30;
		m_retryButtonX = (panelWidth - m_retryButtonWidth) / 2;
		m_retryButtonY = (panelHeight - m_retryButtonHeight) / 2 + getOffsetY(Retry_Btn);

		if (m_isMouseOnRetryBtn) {
			dc.SetBrush(wxBrush(wxColour(98, 211, 97, 0)));
		}
		else
		{
			dc.SetBrush(wxBrush(wxColour(0, 0, 0, 0), wxBRUSHSTYLE_TRANSPARENT)); 
		}
		dc.SetPen(wxPen(wxColour(255, 255,255))); 
		dc.DrawRoundedRectangle(m_retryButtonX, m_retryButtonY, m_retryButtonWidth, m_retryButtonHeight, 5); 
		
		dc.SetFont(ANKER_FONT_NO_1);
		dc.SetTextForeground(wxColour(255, 255, 255)); 
		dc.SetTextBackground(wxColour(0, 0, 0, 0)); 
		wxSize textSize = dc.GetTextExtent(m_RetryBtnText);
		int textX = m_retryButtonX + (m_retryButtonWidth - textSize.GetWidth()) / 2;
		int textY = m_retryButtonY + (m_retryButtonHeight - textSize.GetHeight()) / 2;
		dc.DrawText(m_RetryBtnText, textX, textY);
	}
}

void imageDisplayer::SetBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
    Refresh();
}


AnkerVideo::AnkerVideo(wxWindow* parent, std::string sn)
	: m_sn(sn),
	wxPanel(parent)
{
	ANKER_LOG_INFO << ("AnkerVideo constructor, sn="+sn);
	Bind(wxEVT_SIZE, &AnkerVideo::OnSize, this);
	Bind(wxCUSTOMEVT_UI_UPDATE, [this](wxCommandEvent& event) {
		// ANKER_LOG_INFO << ("--------on recv wxCUSTOMEVT_UI_UPDATE, thread id=" + num2Str((getThreadIdAsLongLong())));
		UpdateUI();
	});

    InitGUI();
}

AnkerVideo::~AnkerVideo()
{
}

void AnkerVideo::InitGUI()
{
	wxBoxSizer* vBox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vBox);
	// title
	{
		wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
		m_titlePanel = new wxPanel(this);
		vBox->Add(m_titlePanel, 0, wxALL , 0 );
		m_titlePanel->SetSizer(titleSizer);

		m_titlePanel->SetBackgroundColour(wxColour("#292A2D"));
		m_titlePanel->SetMinSize(AnkerSize(-1, m_titileBarHeight));
		m_titlePanel->SetMaxSize(AnkerSize(-1, m_titileBarHeight));
		m_titlePanel->SetSize(AnkerSize(-1, m_titileBarHeight));

		titleSizer->AddSpacer(12);
		wxStaticText* title = new wxStaticText(m_titlePanel, wxID_ANY, _L("common_print_monitor_title"));
		title->SetFont(ANKER_BOLD_FONT_NO_1);
		title->SetBackgroundColour(wxColour("#292A2D"));
		title->SetForegroundColour(wxColour(255, 255, 255));
		title->SetCanFocus(false);

		titleSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 0);
	}


	{
		// Line
		wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
		splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
		splitLineCtrl->SetMaxSize(AnkerSize(2000, m_spliteLineHeight));
		splitLineCtrl->SetMinSize(AnkerSize(2000, m_spliteLineHeight));
		vBox->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);
	}

	{
		// video image display area
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		wxColour backgroudColor("#000000");
		videoImgDisplayer = new imageDisplayer(this, m_sn, backgroudColor);
		videoImgDisplayer->SetMinSize(wxSize(560 , 420));
		videoImgDisplayer->SetBackgroundColour(backgroudColor);
		sizer->Add(videoImgDisplayer, 1, wxEXPAND | wxALIGN_TOP | wxALL, 0);
		vBox->Add(sizer, 0, wxALIGN_CENTER, 0);
		//vBox->Add(sizer);
	}
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->AddStretchSpacer(1);
		// online 
		{
			m_videoIconButton = new wxButton(videoImgDisplayer, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			m_videoIconButton->SetBackgroundColour(wxColour("#000000"));
			{
				wxBitmap BtnBitmap = wxBitmap(wxString::FromUTF8(Slic3r::var("videoIcon.png")), wxBITMAP_TYPE_PNG);
				wxImage BtnImage = BtnBitmap.ConvertToImage();
				BtnImage.Rescale(40, 40);
				wxBitmap scaledBitmap(BtnImage);
				m_videoIconButton->SetBitmap(scaledBitmap);
				m_videoIconButton->SetMinSize(scaledBitmap.GetSize());
				m_videoIconButton->SetMaxSize(scaledBitmap.GetSize());
			}
			m_videoIconButton->Bind(wxEVT_BUTTON, &AnkerVideo::OnPlayBtnClicked, this);
			m_showVideoIcon = true;
			sizer->Add(m_videoIconButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 5);
		}

		// offline
		{
			int px_size = 60;
			m_offlineIconButton = new ScalableButton(videoImgDisplayer, wxID_ANY, "device_wifi_icon_off", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_size);
			m_offlineIconButton->SetBackgroundColour(wxColour("#000000"));
			sizer->Add(m_offlineIconButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 0);
			m_offlineIconButton->Disable();

			m_offlineText = new wxStaticText(videoImgDisplayer, wxID_ANY, _L("common_print_statusnotice_disconnected")); // "Device Is Disconnected,"
			m_offlineText->SetFont(ANKER_FONT_NO_1);
			m_offlineText->SetBackgroundColour(wxColour(0,0,0,255));
			m_offlineText->SetForegroundColour(wxColour("#999999"));
			m_offlineText->SetCanFocus(false);
			sizer->Add(m_offlineText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 0);

			wxString helpText = _L("common_print_statusnotice_disconnected2"); // "please click to see more help >>"
			wxSize textSize = videoImgDisplayer->GetTextExtent(helpText);
			m_offlineHelpLink = new AnkerHyperlink(videoImgDisplayer, wxID_ANY, helpText, "https://support.ankermake.com/s/article/How-to-Fix-WiFi-Connection-Issue", wxColour("#000000"), wxDefaultPosition,wxDefaultSize ,ALIGN_CENTER);
			m_offlineHelpLink->SetMinSize(AnkerSize(textSize.x+20, textSize.y + 10));
			m_offlineHelpLink->SetSize(AnkerSize(textSize.x + 20, textSize.y + 10));
			sizer->Add(m_offlineHelpLink, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 0);

			wxBoxSizer* offLineMsgSizer = new wxBoxSizer(wxHORIZONTAL);
			sizer->Add(offLineMsgSizer);

			m_offlineIconButton->Hide();
			m_offlineText->Hide();
			m_offlineHelpLink->Hide();
		}

		sizer->AddStretchSpacer(1);
		videoImgDisplayer->SetSizer(sizer);
	}

	{
		// Line
		wxControl* splitLineCtrl = new wxControl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
		splitLineCtrl->SetBackgroundColour(wxColour(62, 63, 66));
		splitLineCtrl->SetMaxSize(AnkerSize(2000, m_spliteLineHeight));
		splitLineCtrl->SetMinSize(AnkerSize(2000, m_spliteLineHeight));
		vBox->Add(splitLineCtrl, 0, wxALIGN_CENTER, 0);
	}

	{
		// buttons
		wxBoxSizer* controlBarSizer = new wxBoxSizer(wxHORIZONTAL);
		m_controlBar = new wxPanel(this);
		m_controlBar->SetBackgroundColour(wxColour("#292A2D"));
		m_controlBar->SetMinSize(AnkerSize(-1, m_controlBarHeight));
		m_controlBar->SetSize(AnkerSize(-1, m_controlBarHeight));

		m_controlBar->SetSizer(controlBarSizer);
		controlBarSizer->AddSpacer(10);

		{
			int px_size = 20;
			m_playBtn = new ScalableButton(m_controlBar, wxID_ANY, "PlayVideo", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_size);
			m_playBtn->SetBackgroundColour(wxColour("#292A2D"));
			m_playBtn->Bind(wxEVT_BUTTON, &AnkerVideo::OnPlayBtnClicked, this);
		}
		{
			int px_size = 20;
			m_stopBtn = new ScalableButton(m_controlBar, wxID_ANY, "StopVideo", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_size);
			m_stopBtn->SetBackgroundColour(wxColour("#292A2D"));
			m_stopBtn->Hide();
			m_stopBtn->Bind(wxEVT_BUTTON, &AnkerVideo::OnStopBtnClicked, this);
		}
		{
			m_playStopStateText = new wxStaticText(m_controlBar, wxID_ANY, "");
			m_playStopStateText->SetBackgroundColour(wxColour("#292A2D"));
			m_playStopStateText->SetForegroundColour("#777777");
			m_playStopStateText->SetFont(ANKER_FONT_NO_1);
		}
		{
			int px_size = 20;
			m_turnOnCameraLightBtn = new ScalableButton(m_controlBar, wxID_ANY, "TurnonLight", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_size);
			m_turnOnCameraLightBtn->SetBackgroundColour(wxColour("#292A2D"));
			m_turnOnCameraLightBtn->Bind(wxEVT_BUTTON, &AnkerVideo::OnTurnOnLightBtnClicked, this);
		}
		{
			int px_size = 20;
			m_turnOffCameraLightBtn = new ScalableButton(m_controlBar, wxID_ANY, "TurnoffLight", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_size);
			m_turnOffCameraLightBtn->SetBackgroundColour(wxColour("#292A2D"));
			m_turnOffCameraLightBtn->Hide();
			m_turnOffCameraLightBtn->Bind(wxEVT_BUTTON, &AnkerVideo::OnTurnOffLightBtnClicked, this);
		}

		m_videoModeSelector = new CustomComboBox(m_controlBar);
		m_videoModeSelector->SetBackgroundColour(wxColour("#292A2D"));
		m_videoModeSelector->SetForegroundColour(wxColour("#EAEAEA"));
		m_videoModeSelector->setPopupMenuBackgroudColour(wxColour("#202020"));
		m_videoModeSelector->setHightlightMenuItemColour(wxColour("#555555"));
		m_videoModeSelector->SetFont(ANKER_FONT_NO_1);
		m_videoModeSelector->SetMinSize(wxSize(80, 20));
		m_videoModeSelector->AddOption(_L("common_print_monitor_switchhd"));		// "HD"
		m_videoModeSelector->AddOption(_L("common_print_monitor_switchsd"));	// "Smooth"
		m_videoModeSelector->SetCurrentOption(_L("common_print_monitor_switchhd"));	// "HD"
		m_videoModeSelector->SetOptionChangeCallback([this](const wxString& menuItem) {
			//wxLogMessage("you have Selected item: %s", menuItem);
			OnSelectVideoMode(menuItem);
			});

		controlBarSizer->Add(m_playBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->Add(m_stopBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->AddSpacer(5);
		controlBarSizer->Add(m_playStopStateText, 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->AddStretchSpacer(1);
		controlBarSizer->Add(m_videoModeSelector, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->AddSpacer(10);
		controlBarSizer->Add(m_turnOnCameraLightBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->Add(m_turnOffCameraLightBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
		controlBarSizer->AddSpacer(10);

		vBox->Add(m_controlBar, 0, wxEXPAND | wxALL, 0);
	}
	UpdateUI();

	m_videoStreamConnetingTimer.Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
		ANKER_LOG_INFO << ("ConectingVideo time out");
		CloseVideoStream(VIDEO_CLOSE_BY_UI_TIMEOUT);

		this->m_videoStreamConnetingTimer.Stop();
		this->m_VideoConnetingClosingIconUpdtTimer.Stop();
		this->currVideoState = Video_State_ConectingVideo_Fail;
		this->UpdateUI();
		});

	m_VideoConnetingClosingIconUpdtTimer.Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
		static int currIdx = 0;
		++currIdx;
		std::string pngFile = "preparingVideo" + num2Str(1 + currIdx % 8) + ".png";
		//ANKER_LOG_INFO << ("==================update icon pngFile");
		videoImgDisplayer->setOSDItem(Video_Status_Icon, pngFile);
		videoImgDisplayer->Update();
		videoImgDisplayer->Refresh();
		this->Refresh();
		});

	videoImgDisplayer->setRetryBtnClickCB(std::bind(&AnkerVideo::onRetryBtnClickCB, this));
}

void AnkerVideo::SetOnlineOfflineState(bool state)
{
	ANKER_LOG_INFO << ("SetOnlineOfflineState:"+num2Str(state));
	m_onlineOfflineState = state;
	currVideoState = Video_State_None;
	UpdateUI();

	AnkerNetBase* ankerNet = AnkerNetInst();
	if (ankerNet) {
		ankerNet->closeVideoStream(VIDEO_CLOSE_BY_OFF_LINE);
	}
}

bool AnkerVideo::IsOffLine()
{
	return m_onlineOfflineState;
}

void  AnkerVideo::SetSN(std::string sn)
{
	m_sn = sn;
	UpdateUI();
}

void AnkerVideo::OnSize(wxSizeEvent& event)
{
	m_resizing = true;

	videoImgDisplayer->SetBackgroundColour(wxColour("#000000"));

	int titileHeight = AnkerSize(-1, m_titileBarHeight).GetHeight();
	int controlBarHeight = AnkerSize(-1, m_controlBarHeight).GetHeight();
	int spliteLineHeight = AnkerSize(-1, m_spliteLineHeight).GetHeight();
    videoImgDisplayer->SetMinSize(wxSize(GetSize().GetWidth(), GetSize().GetHeight()-(titileHeight + controlBarHeight + spliteLineHeight * 2)));
	Update();
	Refresh();
	Layout();
	event.Skip();
	m_resizing = false;
}

void AnkerVideo::OnCloseWindow(wxCloseEvent& event)
{
	event.Skip();
}


void AnkerVideo::onRetryBtnClickCB()
{
	ANKER_LOG_INFO << (" clickRetryBtn");
	wxCommandEvent event;
	OnPlayBtnClicked(event);
}

void AnkerVideo::OnPlayBtnClicked(wxCommandEvent& event)
{
	ANKER_LOG_INFO << ("===>OnPlayBtnClicked,sn = "+ m_sn);
    DeviceObjectBasePtr pDevObj = CurDevObject(m_sn);
	//report: start play the video
	std::string deviceSn = m_sn;
	std::string errorCode = std::string("0");
	std::string errorMsg = std::string("start play video");

	std::map<std::string, std::string> buryMap;
	buryMap.insert(std::make_pair(c_pv_device_sn, deviceSn));
	buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
	buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));

    if (!pDevObj)    {
		ANKER_LOG_ERROR << ("could not connect video:device obj is null");
		errorCode = "-1";
		errorMsg = std::string("could not connect video:device obj is null");
		buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));
		ANKER_LOG_INFO << "Report bury event is " << e_play_video;
		reportBuryEvent(e_play_video, buryMap);
		
        return;
    }

    if (!pDevObj->GetOnline()) {
		ANKER_LOG_ERROR << ("could not connect video:device is offline");
		errorCode = "-1";
		errorMsg = std::string("could not connect video:device is offline");
		buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));
		ANKER_LOG_INFO << "Report bury event is " << e_play_video;
		reportBuryEvent(e_play_video, buryMap);
		
        return;
    }

	if (pDevObj->GetDeviceType() == DEVICE_V8110_TYPE)
	{
		m_isVideoSuport = false;
	}

	if (PrintCheckHint::StopForV6UnInited(m_sn, this)) {
		ANKER_LOG_ERROR << ("could not connect video:v6 uninited");
		errorCode = "-1";
		errorMsg = std::string("could not connect video:v6 uninited");
		buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));
		ANKER_LOG_INFO << "Report bury event is " << e_play_video;
		reportBuryEvent(e_play_video, buryMap);
		
		return;
	}
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		errorCode = "-1";
		errorMsg = std::string("no net module to play video");
		buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));
		ANKER_LOG_INFO << "Report bury event is " << e_play_video;
		reportBuryEvent(e_play_video, buryMap);
		
		return;
	}

    if (!pDevObj->GetCameraLimit()) {
		ANKER_LOG_ERROR << ("could not connect video:no camera permission");
		errorCode = "-1";
		errorMsg = std::string("could not connect video:no camera permission");
		buryMap.insert(std::make_pair(c_pv_error_code, errorCode));
		buryMap.insert(std::make_pair(c_pv_error_msg, errorMsg));
		ANKER_LOG_INFO << "Report bury event is " << e_play_video;
		reportBuryEvent(e_play_video, buryMap);
		
        currVideoState = Video_State_ConectingVideo_Fail;
        UpdateUI();
        return;
    }

	m_videoModeSelector->SetCurrentOption(_L("common_print_monitor_switchhd"));  // "HD"
	if (currVideoState == Video_State_None || currVideoState == Video_State_ConectingVideo_Fail || currVideoState == Video_State_p2p_was_preempted)
	{
		m_videoIconButton->Show(false);
		this->Refresh();

		m_VideoConnetingClosingIconUpdtTimer.Start(100);
		m_videoStreamConnetingTimer.Start(30 * 1000);
		currVideoState = Video_State_ConectingVideo;
		UpdateUI();

		ankerNet->StartP2pOperator(P2P_TRANSFER_VIDEO_STREAM, m_sn, "");
	}
	else {
		ANKER_LOG_ERROR << ("===> ignore, currstate:"+ getVideoStateStr(currVideoState));
	}
	ANKER_LOG_INFO << "Report bury event is " << e_play_video;
	reportBuryEvent(e_play_video, buryMap);
	
}

void AnkerVideo::OnStopBtnClicked(wxCommandEvent& event)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	DeviceObjectBasePtr pDevObj = CurDevObject(m_sn);
	if (!pDevObj) {
		ANKER_LOG_ERROR << ("device obj is null");
		return;
	}

	if (currVideoState == Video_State_Recving_Frame) {
		// AkUtil::TDebug("autoCloseVideoStreamLater time out, to close video ... ");
		currVideoState = Video_State_Closing_Video;
		m_VideoConnetingClosingIconUpdtTimer.Start(100);
		UpdateUI();

		AnkerNetBase* ankerNet = AnkerNetInst();
		if (ankerNet) {
			ankerNet->closeVideoStream(VIDEO_CLOSE_BY_USER);
		}
	}
	else {
		ANKER_LOG_ERROR << ("error state!! curr state is not playing video,should not stop, reset state ...");
		currVideoState = Video_State_None;
		UpdateUI();

		auto ankerNet = AnkerNetInst();
		if (ankerNet) {
			ankerNet->closeVideoStream(VIDEO_CLOSE_NONE);
		}
	}
}


void AnkerVideo::OnTurnOnLightBtnClicked(wxCommandEvent& event)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		ANKER_LOG_ERROR << ("ankerNet obj is null");
		return;
	}

    m_turnOnCameraLightBtn->Enable(false);
    m_turnOffCameraLightBtn->Enable(false);

	m_turnOnCameraLightBtn->SetBitmap_("TurnonLight_disable");

	m_timer.Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
		if (currVideoState == Video_State_Recving_Frame) {
			m_camerLightOnoff = true;
			UpdateUI();
		}
	});
	m_timer.StartOnce(3 * 1000);

	ankerNet->setCameraLightState(true);
}

void AnkerVideo::OnTurnOffLightBtnClicked(wxCommandEvent& event)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	m_turnOnCameraLightBtn->Enable(false);
	m_turnOffCameraLightBtn->Enable(false);

	m_timer.Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
		if (currVideoState == Video_State_Recving_Frame) {
			m_camerLightOnoff = false;
			UpdateUI();
		}
		});
	m_timer.StartOnce(3 * 1000);

	ankerNet->setCameraLightState(false);
}

void AnkerVideo::OnSelectVideoMode(wxString mode)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	AnkerNetBase* ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}

	if (mode == "HD" || mode == _L("common_print_monitor_switchhd")) {
		ankerNet->setVideoMode(P2P_VIDEO_MODE_HD);
	}else if (mode == "Smooth" || mode == _L("common_print_monitor_switchsd") ) {
		ankerNet->setVideoMode(P2P_VIDEO_MODE_SMOOTH);
	}
}

void AnkerVideo::displayVideoFrame(const unsigned char* imgData, int width, int height)
{
	if (Slic3r::GUI::wxGetApp().is_recreating_gui()) {
		videoImgDisplayer->m_bStopRender = true;
		return;
	}

	if (currVideoState == Video_State_p2pInit_OK) {
		m_showVideoIcon = false;
		currVideoState = Video_State_Recving_Frame;
		//m_showVideo = true;
		videoImgDisplayer->m_bStopRender = false;
		m_VideoConnetingClosingIconUpdtTimer.Stop();
		m_videoStreamConnetingTimer.Stop();
		//UpdateUI();
		PostUpdateUIEvent();
	}

	if (m_resizing) {
		return;
	}
	
	if (m_videoStreamConnetingTimer.IsRunning())
	{
		m_videoStreamConnetingTimer.Stop();
	}

	//if (!m_showVideo) {
	//	return;
	//}

	if (currVideoState == Video_State_Recving_Frame)
	{
		if (imgData)
		{
			//ANKER_LOG_INFO << (" draw a frame ...");
			//wxImage image( width, height, const_cast<unsigned char*>(imgData), true);
			DataFrame data;
			data.frame = const_cast<unsigned char *> (imgData);
			data.width = width;
			data.height = height;
			wxAny any(data);
			wxVariant va(any);
			MyCustomEvent evt = MyCustomEvent(wxCUSTOMEVT_FRAME_DATA_UPDATE);
			evt.SetCustomData(data);
			wxPostEvent(videoImgDisplayer, evt); 
			// ANKER_LOG_INFO << ("===post frame data, sn="+m_sn);
		}
	}
}

void AnkerVideo::HideVideoStatusInfo()
{
	videoImgDisplayer->showOSDItem(Video_Status_Icon, false);
	videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
	videoImgDisplayer->showOSDItem(Retry_Btn, false);

	m_videoIconButton->Show(false);
	m_offlineIconButton->Show(false);
	m_offlineText->Show(false);
	m_offlineHelpLink->Show(false);
}

void AnkerVideo::OffLineUI()
{
	videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
	videoImgDisplayer->showOSDItem(Video_Status_Msg, true);
	videoImgDisplayer->showOSDItem(Retry_Btn, false);

	m_videoIconButton->Show(false);
	m_offlineIconButton->Show(true);
	m_offlineText->Show(true);
	m_offlineHelpLink->Show(true);
}

void AnkerVideo::videNotSuportUI()
{
	videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
	videoImgDisplayer->showOSDItem(Video_Status_Msg, true);
	videoImgDisplayer->showOSDItem(Retry_Btn, false);

	std::string pngFile = "cameraNotExist.png";
	videoImgDisplayer->setOSDItem(Video_Status_Icon, pngFile);
	videoImgDisplayer->setOSDItem(Video_Status_Msg, _L("common_print_monitor_nomonitor").ToStdString());    // "The current device does not support real-time video"
}

void AnkerVideo::videoShowRetryUI()
{
	if (currVideoState != Video_State_Recving_Frame)
	{
		videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
		videoImgDisplayer->showOSDItem(Video_Status_Msg, true);
		videoImgDisplayer->showOSDItem(Retry_Btn, true);

		videoImgDisplayer->setOSDItem(Video_Status_Icon, "UnablePlayVideo.png");
		videoImgDisplayer->setOSDItem(Video_Status_Msg, _L("common_print_monitor_disconnect").ToStdString());	// "Unable to play real-time video"
		videoImgDisplayer->setOSDItem(Retry_Btn, _L("common_print_monitor_buttonretry").ToStdString());	// "Retry"
	}
}

void AnkerVideo::PostUpdateUIEvent()
{
	// ANKER_LOG_INFO << ("--------in PostUpdateUIEvent, thread id=" + num2Str((getThreadIdAsLongLong())));
	MyCustomEvent evt = MyCustomEvent(wxCUSTOMEVT_UI_UPDATE);
	// evt.SetCustomData(data);
	wxPostEvent(this, evt);
}

void AnkerVideo::UpdateUI()
{
	ANKER_LOG_INFO << ("===> sn:"+ m_sn+" currstate:" + getVideoStateStr(currVideoState) + " online ? : " + num2Str(m_onlineOfflineState))<<"    isShown: "<<this->IsShown();

	bool isOffline = false;
	if (m_onlineOfflineState == false || m_sn.empty()) {
		isOffline = true;
	}
	else {
		DeviceObjectBasePtr pDevObj = CurDevObject(m_sn);
		if (!pDevObj) {
			return;
		}
		if (pDevObj->GetDeviceType() == DEVICE_V8110_TYPE)
		{
			m_isVideoSuport = false;
		}
		else {
			m_isVideoSuport = true;
		}
	}

	HideVideoStatusInfo();

	if (isOffline) {
		// offline UI
		m_playBtn->Show(true);
		m_playBtn->Enable(false);

		m_stopBtn->Show(false);
		m_stopBtn->Enable(false);

		m_playStopStateText->Show(false);

		m_videoModeSelector->Show(true);
		m_videoModeSelector->Enable(false);

		m_turnOnCameraLightBtn->Show(true);
		m_turnOnCameraLightBtn->Enable(false);

		m_turnOffCameraLightBtn->Show(false);
		m_turnOffCameraLightBtn->Enable(false);

		m_videoIconButton->Show(false);
		m_offlineIconButton->Show(false);
		m_offlineText->Show(false);
		m_offlineHelpLink->Show(false);

		videoImgDisplayer->showOSDItem(Video_Status_Icon, false);
		videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
		videoImgDisplayer->showOSDItem(Retry_Btn, false);

		OffLineUI();
	}
	else
	{
		if (m_isVideoSuport == false)
		{
			m_playBtn->Show(true);
			m_playBtn->Enable(false);

			m_stopBtn->Show(false);
			m_stopBtn->Enable(false);

			m_playStopStateText->Show(false);

			m_videoModeSelector->Show(true);
			m_videoModeSelector->Enable(false);

			m_turnOnCameraLightBtn->Show(true);
			m_turnOnCameraLightBtn->Enable(false);

			m_turnOffCameraLightBtn->Show(false);
			m_turnOffCameraLightBtn->Enable(false);

			m_videoIconButton->Show(false);
			m_offlineIconButton->Show(false);
			m_offlineText->Show(false);
			m_offlineHelpLink->Show(false);

			videoImgDisplayer->showOSDItem(Video_Status_Icon, false);
			videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
			videoImgDisplayer->showOSDItem(Retry_Btn, false);

			videNotSuportUI();
		}
		else {
			// online
			m_offlineIconButton->Show(false);
			m_offlineText->Show(false);
			m_offlineHelpLink->Show(false);

			switch (currVideoState)
			{
			case Video_State_None:
				m_playBtn->Show(true);
				m_playBtn->Enable(true);

				m_stopBtn->Show(false);
				m_stopBtn->Enable(true);

				if (m_showVideoIcon) {
					m_playStopStateText->Show(false);
				}
				else {
					m_playStopStateText->Show(true);
					m_playStopStateText->SetLabelText(_L("common_print_monitor_paused"));    // "Paused"
				}

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(false);

				m_videoIconButton->Show(m_showVideoIcon);
				videoImgDisplayer->showOSDItem(Video_Status_Icon, false);
				videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
				videoImgDisplayer->showOSDItem(Retry_Btn, false);
				break;
			case Video_State_ConectingVideo:
				m_playBtn->Show(true);
				m_playBtn->Enable(false);

				m_stopBtn->Show(false);
				m_stopBtn->Enable(true);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText(_L("common_print_monitor_paused"));    // "Paused"

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(false);

				m_videoIconButton->Show(false);

				videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
				videoImgDisplayer->showOSDItem(Video_Status_Msg, true);
				videoImgDisplayer->showOSDItem(Retry_Btn, false);
				//videoImgDisplayer->setOSDItem(Video_Status_Msg, "Establishing secure video channel...");	// ""Establishing secure video channel...""
				videoImgDisplayer->setOSDItem(Video_Status_Msg, "");	// ""Establishing secure video channel...""
				break;

			case Video_State_ConectingVideo_Fail:
				m_playBtn->Show(true);
				m_playBtn->Enable(false);

				m_stopBtn->Show(false);
				m_stopBtn->Enable(true);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText(_L("common_print_monitor_paused"));    // "Paused"

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(false);

				m_videoIconButton->Show(false);

				videoShowRetryUI();
				break;

			case Video_State_p2pInit_OK:
				m_playBtn->Show(true);
				m_playBtn->Enable(false);

				m_stopBtn->Show(false);
				m_stopBtn->Enable(true);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText(_L("common_print_monitor_paused"));    // "Paused"

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(true);

				m_videoIconButton->Show(false);

				videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
				videoImgDisplayer->showOSDItem(Video_Status_Msg, true);
				videoImgDisplayer->showOSDItem(Retry_Btn, false);

				break;

			case Video_State_Recving_Frame:
				m_playBtn->Show(false);
				m_playBtn->Enable(true);

				m_stopBtn->Show(true);
				m_stopBtn->Enable(true);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText(_L("common_print_monitor_playing"));    // "Playing"

				m_videoModeSelector->Enable(true);

				if (m_camerLightOnoff)
				{
					m_turnOnCameraLightBtn->Show(false);
					m_turnOnCameraLightBtn->Enable(true);

					m_turnOffCameraLightBtn->Show(true);
					m_turnOffCameraLightBtn->Enable(true);
				}
				else
				{
					m_turnOnCameraLightBtn->Show(true);
					m_turnOnCameraLightBtn->Enable(true);

					m_turnOffCameraLightBtn->Show(false);
					m_turnOffCameraLightBtn->Enable(true);
				}

				m_videoIconButton->Show(false);

				videoImgDisplayer->showOSDItem(Video_Status_Icon, false);
				videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
				videoImgDisplayer->showOSDItem(Retry_Btn, false);

				break;

			case Video_State_Closing_Video:
				m_playBtn->Show(false);
				m_playBtn->Enable(true);

				m_stopBtn->Show(true);
				m_stopBtn->Enable(false);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText(_L("common_print_monitor_paused"));    // "Paused"

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(false);

				m_videoIconButton->Show(false);

				videoImgDisplayer->showOSDItem(Video_Status_Icon, true);
				videoImgDisplayer->showOSDItem(Video_Status_Msg, false);
				videoImgDisplayer->showOSDItem(Retry_Btn, false);
				videoImgDisplayer->setOSDItem(Video_Status_Msg, "");

				break;

			case Video_State_p2p_was_preempted:
				m_playBtn->Show(true);
				m_playBtn->Enable(true);

				m_stopBtn->Show(false);
				m_stopBtn->Enable(true);

				m_playStopStateText->Show(true);
				m_playStopStateText->SetLabelText("common_print_monitor_paused");    // "Paused"

				m_videoModeSelector->Enable(false);

				m_turnOnCameraLightBtn->Show(true);
				m_turnOnCameraLightBtn->Enable(false);

				m_turnOffCameraLightBtn->Show(false);
				m_turnOffCameraLightBtn->Enable(false);

				videoShowRetryUI();
				break;

			default: break;
			}
		}
	}

	if (m_playBtn->IsThisEnabled())
	{
		m_playBtn->SetBitmap_("PlayVideo");
	}
	else
	{
		m_playBtn->SetBitmap_("PlayVideo_disable");
	}

	if (m_stopBtn->IsEnabled())
	{
		m_stopBtn->SetBitmap_("StopVideo");
	}
	else
	{
		m_stopBtn->SetBitmap_("StopVideo_disable");
	}

	if (m_turnOnCameraLightBtn->IsEnabled())
	{
		m_turnOnCameraLightBtn->SetBitmap_("TurnonLight");
	}
	else
	{
		m_turnOnCameraLightBtn->SetBitmap_("TurnonLight_disable");
	}
	//ANKER_LOG_INFO << ("---in updateUI, to update and start update timer ");
	Layout();
	videoImgDisplayer->Update();
	videoImgDisplayer->Refresh();
	Refresh();
}

void AnkerVideo::onRecvCameraLightStateNotify(bool onOff)
{
	m_camerLightOnoff = onOff;
	//UpdateUI();
	PostUpdateUIEvent();
}

void AnkerVideo::onRecVideoModeNotify(int mode)
{
}

void AnkerVideo::onVideoP2pInited()
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	if (currVideoState == Video_State_ConectingVideo) {
		currVideoState = Video_State_p2pInit_OK;
		//UpdateUI();
		PostUpdateUIEvent();
	}
}

void AnkerVideo::onP2pvideoStreamSessionInit()
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));
	if (currVideoState == Video_State_ConectingVideo) {
		currVideoState = Video_State_p2pInit_OK;
		//UpdateUI();
		PostUpdateUIEvent();
	}
	m_isInP2pSession = true;
	//UpdateUI();
	PostUpdateUIEvent();
}


void AnkerVideo::onRecvVideoStreamClosing(int closeReason)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState) + "  reason:"+num2Str(closeReason));
	videoImgDisplayer->m_bStopRender = true;
	if (closeReason > 0) // video is not close by click "stop" button
	{
		m_VideoConnetingClosingIconUpdtTimer.Start(100);
		this->currVideoState = Video_State_Closing_Video;
		//UpdateUI();
		PostUpdateUIEvent();
	}
}

void AnkerVideo::onP2pvideoStreamSessionClosed(int closeReason)
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState) + "  reason:" + num2Str(closeReason));
	auto ankerNet = AnkerNetInst();
	if (!ankerNet) {
		return;
	}
	
	m_isInP2pSession = false;
	m_camerLightOnoff = false;
	m_VideoConnetingClosingIconUpdtTimer.Stop();
	// m_videoModeSelector->SetCurrentOption(_L("common_print_monitor_switchhd")); // "HD"
	if (currVideoState == Video_State_ConectingVideo_Fail)
	{
		// do nothing
	}
	else
	{
		if (closeReason == 2) { // video stop for p2p transfer File
			currVideoState = Video_State_p2p_was_preempted;
		}
		else {
			currVideoState = Video_State_None;
		}
	}
	//UpdateUI();
	PostUpdateUIEvent();
	// ANKER_LOG_INFO << ("--------in onClosed, thread id=" + num2Str((getThreadIdAsLongLong())));
}

void AnkerVideo::onRcvP2pVideoStreamCtrlBusyFeedback()
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState) );

	m_videoStreamConnetingTimer.Stop();
	m_VideoConnetingClosingIconUpdtTimer.Stop();
	currVideoState = Video_State_ConectingVideo_Fail;
	//UpdateUI();
	PostUpdateUIEvent();
}

void AnkerVideo::onRcvP2pVideoStreamCtrlAbnomal()
{
	ANKER_LOG_INFO << ("===> currstate:" + getVideoStateStr(currVideoState));

	m_videoStreamConnetingTimer.Stop();
	m_VideoConnetingClosingIconUpdtTimer.Stop();
	currVideoState = Video_State_ConectingVideo_Fail;
	//UpdateUI();
	PostUpdateUIEvent();
}

void AnkerVideo::PrintVideoState()
{
	DeviceObjectBasePtr pDevObj = CurDevObject(m_sn);
	if (pDevObj) {
	// ANKER_LOG_INFO << ("VideoState:" + getVideoStateStr(currVideoState)  + " sn:" + pDevObj->m_sn + "  devName:" + pDevObj->station_name);
	}
}


std::string AnkerVideo::getVideoStateStr(videoState state)
{
	std::string str = "";
	switch (state)
	{
	case Video_State_None:
		str = "Video_State_None";
		break;
	case Video_State_ConectingVideo:
		str = "Video_State_ConectingVideo";
		break;
	case Video_State_ConectingVideo_Fail:
		str = "Video_State_ConectingVideo_Fail";
		break;
	case Video_State_p2pInit_OK:
		str = "Video_State_p2pInit_OK";
		break;
	case Video_State_Recving_Frame:
		str = "Video_State_Recving_Frame";
		break;
	case Video_State_Closing_Video:
		str = "Video_State_Closing_Video";
		break;
	case Video_State_p2p_was_preempted:
		str = "Video_State_p2p_was_preempted";
		break;
	default:
		str = "Unknown, something is wrong!";
		break;
	}

	// AkUtil::TDebug(str);
	return str + "(state:" + num2Str(state) + ")";
}

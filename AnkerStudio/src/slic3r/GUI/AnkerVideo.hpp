#ifndef ANKER_NETWORK_VIDEO_PLAYER_H
#define ANKER_NETWORK_VIDEO_PLAYER_H

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

#include "wx/popupwin.h"
#include "wxExtensions.hpp"
#include "GUI_App.hpp"
#include "AnkerHyperlink.hpp"

enum videoState
{
	Video_State_None = 0,
	Video_State_ConectingVideo,
	Video_State_ConectingVideo_Fail,
	Video_State_p2pInit_OK,
	Video_State_Recving_Frame,
	Video_State_Closing_Video,
	Video_State_p2p_was_preempted,   // video is close by p2p tranfer file
};

struct DataFrame
{
    unsigned char* frame;
    int width;
    int height;
};

class MyCustomEvent : public wxCommandEvent
{
public:
	MyCustomEvent(wxEventType eventType = wxEVT_NULL, int id = 0)
		: wxCommandEvent(eventType, id) {}

	
	void SetCustomData(const DataFrame& data)
	{
		m_data.frame = data.frame;
		m_data.width = data.width;
		m_data.height = data.height;
	}

	const DataFrame& GetCustomData() const
	{
		return m_data;
	}

	virtual wxEvent* Clone() const wxOVERRIDE
	{
		return new MyCustomEvent(*this);
	}

private:
	DataFrame m_data;
};


enum VideoOSDItem				// OSD(On screen Display)
{
	Video_Status_Icon = 0,
	Video_Status_Msg,
	Retry_Btn
};


class CustomPopupMenu : public wxPopupTransientWindow
{
public:
    CustomPopupMenu(wxWindow* parent);
    void SetSelectedMenuItem(wxString text);
    void setMenuItemWidth(int w);
    int GetMenuItemWidth();
    void setMenuItemHeight(int h);
    void setFontSize(int size);
    void setLeftMargin(int value);
    void setRightMargin(int value);
    bool setHightLightMenuColour(wxColour colour);
    void SetMenuItemSelectedCallback(std::function<void(const wxString&)> callback);
	void AddMenuItem(const wxString& label);
    // wxString GetSelectedMenuItem();
private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void UpdateMenuHeight();
private:
    std::vector<wxString> m_menuItems;

    wxString m_selectedMenuItem;
    int m_selectedMenuItemIndex = -1;
    wxColour m_selectedMenuItemColor;

    wxString m_hoveredMenuItem;
    int m_hoveredMenuItemIndx = -1;
    wxColour m_hoveredMenuItemColor;

    int m_menuItemHeight;
    int m_menuItemWidth;

    int m_menuTotalHeight;

    int m_fondSize;
    int m_marginLeft;
    int m_marginRight;

    std::function<void(const wxString&)> m_menuItemSelectedCallback;
};


class CustomComboBox : public wxWindow
{
public:
    CustomComboBox(wxWindow* parent);
    bool SetBackgroundColour(const wxColour& colour);
    bool SetBorderColour(const wxColour& colour);
    // bool SetForegroundColourForDisableState(const wxColour& colour);
    bool setHightlightMenuItemColour(wxColour colour);
    bool setPopupMenuBackgroudColour(wxColour colour);

    void AddOption(const wxString& option);
    void SetCurrentOption(const wxString& option);

    void SetOptionChangeCallback(std::function<void(const wxString&)> callback);

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouse(wxMouseEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);

private:
    wxString m_currentOption;
    std::vector<wxString> m_options;

    wxString m_selectedMenuItem;

    wxColour m_backgroudColor;
    wxColour m_borderColor;
    wxColour m_textColorEnable;
    wxColour m_textColorDisable;

    wxColour m_PopupMenuBackgroudColour;
    wxColour m_hightLightMenuItemColor;

    //bool m_isEnable;
    std::function<void(const wxString&)> m_optionChangeCallback;
    wxDECLARE_EVENT_TABLE();
};


class imageDisplayer : public wxPanel
{
public:
    imageDisplayer(wxWindow* parent, std::string sn, wxColour bgColor);
    void SetBitmap(const wxBitmap& bitmap);
	void render(wxDC& dc);
	void setRetryBtnClickCB(std::function<void(void)> fn);

	void showOSDItem(VideoOSDItem item, bool show);
	void setOSDItem(VideoOSDItem item, std::string value);
protected:
    void OnPaint(wxPaintEvent& event);

	void OnLeftButtonDown(wxMouseEvent& event);
	void OnMouseOver(wxMouseEvent& event);

	bool IsInRetryButtonArea(const wxPoint& pos);
	int getOffsetY(VideoOSDItem item);
public:
	std::atomic<bool> m_bStopRender{ false };
private:
    wxBitmap m_bitmap;
	wxColour m_bgColor;
	std::string m_sn;

	// OSD(On screen Display) Item
	std::string m_videoStatusIcon;
	std::string m_videoStatusMsg;
	std::string m_RetryBtnText;

	bool m_showVideoStatusIcon = true;
	bool m_showVideoStatusMsg = true;
	bool m_showRetryBtn = true;

	int m_retryButtonX;
	int m_retryButtonY ;
	int m_retryButtonWidth ;
	int m_retryButtonHeight ;
	std::function<void(void)> m_retryBtnClickCB = nullptr;

	bool m_isMouseOnRetryBtn = false;

	wxDECLARE_EVENT_TABLE();
};

class AnkerVideo : public wxPanel
{
public:
    AnkerVideo(wxWindow* parent, std::string sn="");
	~AnkerVideo();
    void InitGUI();
    void SetOnlineOfflineState(bool state);
    bool IsOffLine();
    void SetSN(std::string sn);

    void OnPlayBtnClicked(wxCommandEvent& event);
    void OnStopBtnClicked(wxCommandEvent& event);
    void OnTurnOnLightBtnClicked(wxCommandEvent& event);
    void OnTurnOffLightBtnClicked(wxCommandEvent& event);
    //void OnSelectVideoMode(wxCommandEvent& event);
    void OnSelectVideoMode(wxString mode);

    void displayVideoFrame(const unsigned char* imgData, int width, int height);
    void onRecvCameraLightStateNotify(bool onOff);
    void onRecVideoModeNotify(int mode);

	void onP2pvideoStreamSessionInit();
	void onRecvVideoStreamClosing(int closeReason = 0);
	void onP2pvideoStreamSessionClosed(int closeReason = 0);
	void onRcvP2pVideoStreamCtrlBusyFeedback();
	void onRcvP2pVideoStreamCtrlAbnomal();
    void onVideoP2pInited();

protected:
    void OnSize(wxSizeEvent& event);
	void OnCloseWindow(wxCloseEvent& event);
	void onRetryBtnClickCB();
private:
    void videNotSuportUI();
    void videoShowRetryUI();
    void HideVideoStatusInfo();
    void OffLineUI();
    void PrintVideoState();
    std::string getVideoStateStr(videoState state);
private:
    void UpdateUI();
    void PostUpdateUIEvent();
private:
    std::string m_sn;

    wxPanel* m_titlePanel;
    imageDisplayer* videoImgDisplayer;

	// control bar
    wxPanel* m_controlBar;
    ScalableButton* m_playBtn;
    ScalableButton* m_stopBtn;
	wxStaticText* m_playStopStateText;
    CustomComboBox* m_videoModeSelector;
    ScalableButton* m_turnOnCameraLightBtn;
    ScalableButton* m_turnOffCameraLightBtn;

    //ScalableButton* m_videoIconButton;
    wxButton* m_videoIconButton;

    // offline info
    wxButton* m_offlineIconButton;
    wxStaticText* m_offlineText;
    wxButton* m_offlineHelpButton;
    AnkerHyperlink* m_offlineHelpLink;

    int m_titileBarHeight = 30;
    int m_controlBarHeight = 30;
    int m_spliteLineHeight = 1;

	//wxStaticBitmap* m_videoStatusIcon;
	//wxStaticText* m_videoStatusMsg;
	//wxButton* m_retryBtn;

    bool m_isVideoSuport = true;
    bool m_onlineOfflineState = true;  // true: online ; false:offline

    videoState currVideoState = Video_State_None;
    bool m_camerLightOnoff = false;
    bool m_isInP2pSession = false;
    bool m_showVideoIcon = true;

	wxTimer m_timer;
	wxTimer m_VideoConnetingClosingIconUpdtTimer;
	wxTimer m_videoStreamConnetingTimer;

	//bool m_showVideo = false;
	bool m_resizing = false;
private:
	wxDECLARE_EVENT_TABLE();
};

#endif // !ANKER_NETWORK_VIDEO_PLAYER_H

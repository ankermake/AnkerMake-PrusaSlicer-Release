#ifndef _ANKER_LOADING_MASK_H_
#define _ANKER_LOADING_MASK_H_

#include "wx/wx.h"


#define MUM_IMAGE_COUNT 8

wxDECLARE_EVENT(wxANKEREVT_LOADING_TIMEOUT, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_LOADMASK_RECTUPDATE, wxCommandEvent);

class AnkerLoadingMask : public wxFrame
{
public:
	AnkerLoadingMask(wxWindow* parent, int timeoutMS = -1, bool rectUpdateFlag = true);
	~AnkerLoadingMask();

	void setText(wxString text);
	void updateMaskRect(wxPoint newPos, wxSize newSize);

	void start();
	void stop();
	bool isLoading();

private:
	void initUI();
	void OnTimeEvent(wxTimerEvent& event);
	void OnTimeOutEvent(wxTimerEvent& event);
	void OnRectUpdateEvent(wxTimerEvent& event);
	void OnShow(wxShowEvent& event);

private:
	bool m_rectUpdateFlag;
	int m_rectUpdateFreq;
	int m_loadingCounter;
	int m_deltaCount;
	int m_timeoutMS;
	wxString m_loadingText;

	wxImage m_mumImage[MUM_IMAGE_COUNT];
	wxStaticBitmap* m_pMumLabel;
	wxStaticText* m_pLoadingLabel;
	wxTimer* m_pAnimationTimer;
	wxTimer* m_pTimeOutTimer;
	wxTimer* m_pRectUpdatetTimer;
};

#endif // _ANKER_LOADING_MASK_H_


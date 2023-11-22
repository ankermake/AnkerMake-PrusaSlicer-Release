#ifndef _ANKER_LOADING_LABEL_HPP_
#define _ANKER_LOADING_LABEL_HPP_


#include "wx/wx.h"
#include "wx/timer.h"

enum LoadingStatus
{
	Load_Nor = 1,
	Load_Ing = 2,
};

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_LOADING_LABEL_CLICKED, wxCommandEvent);

class AnkerLoadingLabel : public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerLoadingLabel)
	DECLARE_EVENT_TABLE()
public:
	AnkerLoadingLabel();
	AnkerLoadingLabel(wxWindow* parent, const wxBitmap& bitmap,const wxString& bgColor);
	~AnkerLoadingLabel();
	void startLoading();
	void stopLoading();

	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);                                                            
	void OnTimer(wxTimerEvent& event);
	void init();

	void readlStopLoading();
private:
	wxBitmap m_btnImg;
	wxString m_strbgColor;
	wxTimer* m_loadingTimer;
	wxTimer* m_resetTimer;
	LoadingStatus m_labelStatus;
	int m_imgIndex = 1;
	int timeCount = 0;;
};

#endif // !

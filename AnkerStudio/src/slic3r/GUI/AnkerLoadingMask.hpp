#ifndef _ANKER_LOADING_MASK_H_
#define _ANKER_LOADING_MASK_H_

#include "wx/wx.h"


class AnkerLoadingMask : public wxControl
{
public:
	AnkerLoadingMask(wxWindow* parent);
	~AnkerLoadingMask();

private:
	void initUI();
	void OnTimeEvent(wxTimerEvent& event);
	void OnShow(wxShowEvent& event);

private:
	int m_loadingCounter;
	int m_deltaCount;

	wxImage m_mumImage;
	wxStaticBitmap* m_pMumLabel;
	wxTimer* m_pTimer;
};

static AnkerLoadingMask* AnkerLoading(wxWindow* parent, wxSize size, wxPoint position)
{
	AnkerLoadingMask* loadingMask = new AnkerLoadingMask(parent);
	loadingMask->SetSize(size);
	loadingMask->SetPosition(position);
	loadingMask->Show();

	return loadingMask;
}


#endif // _ANKER_LOADING_MASK_H_


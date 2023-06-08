#ifndef _ANKER_SHOW_WIDGET_HPP_
#define _ANKER_SHOW_WIDGET_HPP_

#include "wx/wx.h"
#include <wx/event.h>
#include "AnkerHyperlink.hpp"
#include "AnkerBtn.hpp"

wxDECLARE_EVENT(wxCUSTOMEVT_LOGIN_CLCIKED, wxCommandEvent);

class AnkerOtherWidget :public wxControl
{
public:
	AnkerOtherWidget(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerOtherWidget();
protected:
	void initUi();
private:
	wxStaticText* m_title;
};

class AnkerEmptyDevice :public wxControl
{
public:
	AnkerEmptyDevice(wxWindow* parent);
	~AnkerEmptyDevice();

protected:
	void initUi();

private:

};

class AnkerOfflineDevice :public wxControl
{
public:
	AnkerOfflineDevice(wxWindow* parent);
	~AnkerOfflineDevice();

protected:
	void initUi();

private:
};

class AnkerUnLoginPanel : public wxPanel
{
public:
	AnkerUnLoginPanel(wxWindow* parent);;
	~AnkerUnLoginPanel();
protected:
	void initUi();
private:
	wxStaticBitmap* m_logoBitmap;
	wxStaticText* m_loginText;
	AnkerBtn* m_loginBtn;
	wxStaticText* m_tipsText;
};

#endif // !_ANKER_SHOW_WIDGET_HPP_



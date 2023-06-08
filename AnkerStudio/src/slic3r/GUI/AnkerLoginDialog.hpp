#ifndef _ANKER_LOGIN_DIALOG_H_
#define _ANKER_LOGIN_DIALOG_H_

#include "wx/wx.h"


class AnkerLoginDialog :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerLoginDialog)
	DECLARE_EVENT_TABLE()
public:
	AnkerLoginDialog();
	AnkerLoginDialog(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxValidator& validator = wxDefaultValidator);

	~AnkerLoginDialog() {}
	bool getLoginStatus();
	void setLoginStatus(bool loginStatus);
	void reset();
	void setAvatar(const wxString& strAvatar);
	void setName(const wxString& strName);
protected:
	bool Create(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSUNKEN_BORDER,
		const wxValidator& validator = wxDefaultValidator);


	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	void OnPaint(wxPaintEvent& event);
private:
	wxString m_Name;
	wxString m_Avatar;
	
	bool	 m_isLogin;
};

class AnkerLoginPanle :public wxPanel
{
	DECLARE_DYNAMIC_CLASS(AnkerLoginPanle)
	DECLARE_EVENT_TABLE()
public:
	AnkerLoginPanle();
	AnkerLoginPanle(wxWindow* parent, wxWindowID id = wxID_ANY);
	~AnkerLoginPanle() {}
protected:
	void initUi();

	virtual void OnClickEvent(wxMouseEvent& event);
private:
	wxBitmapButton* m_avatarBtn;
	wxButton*		m_name;
};

#endif


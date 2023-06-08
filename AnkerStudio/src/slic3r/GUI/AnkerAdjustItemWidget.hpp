#ifndef _ANKER_ADJUSTITEM_WIDGET_H_
#define  _ANKER_ADJUSTITEM_WIDGET_H_

#include "wx/wx.h"

class AnkerAdjustItemWidget :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerAdjustWidget)
	DECLARE_EVENT_TABLE()
public:
	AnkerAdjustItemWidget();
	AnkerAdjustItemWidget(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerAdjustItemWidget();

	void setLogo(const std::string& logo);
	void setTitle(const std::string& title);
	void setContent(const std::string& content);

	void setStatus(bool isDisabel);

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnSize(wxSizeEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnClick(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);
private:
	wxString m_logo;
	wxString m_title;
	wxString m_content;

	bool	 m_status = true;
};

#endif


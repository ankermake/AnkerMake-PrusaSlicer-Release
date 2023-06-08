#ifndef _ANKER_TEXT_LABEL_H_
#define _ANKER_TEXT_LABEL_H_


#include "wx/wx.h"

class AnkerTextLabel :public wxStaticText
{
	DECLARE_DYNAMIC_CLASS(AnkerTextLabel)
	DECLARE_EVENT_TABLE()
public:
	AnkerTextLabel();
	AnkerTextLabel(wxWindow* parent,
		wxWindowID id,
		const wxString& label,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	~AnkerTextLabel() {};

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnClick(wxMouseEvent& event);
protected:
private:
};
#endif


#ifndef _ANKER_MATERIAL_BOX_BTN_HPP_
#define _ANKER_MATERIAL_BOX_BTN_HPP_

#include "wx/control.h"
#include "wx/image.h"
#include "wx/textctrl.h"
#include "wx/sizer.h"
#include "wx/dcclient.h"
#include "wx/graphics.h"
#include "wx/button.h"

enum MaterialBoxStatus
{
	BOX_NOR = 0,
	BOX_HOVER,
	BOX_SELECT,
	BOX_UNKNOWN,	
	BOX_DISWORK,	
	BOX_WORKING,	
	BOX_OFFLINE,
};

wxDECLARE_EVENT(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, wxCommandEvent);

class AnkerMerterialBoxBtn : public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerMerterialBoxBtn)
	DECLARE_EVENT_TABLE()
public:
	AnkerMerterialBoxBtn();
	AnkerMerterialBoxBtn(wxWindow* parent,
		wxColour bgColor,
		wxColour borderColor = wxColour(169, 170, 171),		
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerMerterialBoxBtn();
	

	void setMaterialName(const wxString& name);
	MaterialBoxStatus getBtnStatus();
	void setBtnStatus(const MaterialBoxStatus& status);//status changed use
	void reSetBtnStatus(const MaterialBoxStatus& status);//status changed use	
	void setMaterialColor(const wxColour& color);
	
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);

	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
protected:
	void initUi(const wxColor& bgColor, const wxColor& borderColor);

	void OnPaint(wxPaintEvent& event);
private:

	wxFont	 m_nameFont;
	wxString m_materialName;
	wxString m_alotMaterialName;
	wxColour m_materialColor;
	wxColour m_borderColor;

	MaterialBoxStatus m_currentStatus;		
};
#endif

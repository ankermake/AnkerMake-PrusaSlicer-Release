#ifndef _ANKER_EASY_PANEL_HPP_
#define _ANKER_EASY_PANEL_HPP_

#include "wx/wx.h"
#include "wx/dc.h"
#include <wx/control.h>
#include <wx/event.h>

class AnkerEasyItem;
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_EASY_ITEM_CLICKED, wxCommandEvent);
class AnkerEasyPanel:public wxControl
{

public:
	AnkerEasyPanel(wxWindow* parent, 
					wxWindowID winid = wxID_ANY,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize);
	~AnkerEasyPanel();
	void initUi();
	void showWidget(const std::vector<std::string>& widgetList,const wxString& selectWidgetName);
	void setCurrentWidget(AnkerEasyItem * pWidget);
	void setCurrentWidget(const int& index);
	void createrItem(wxString title, wxImage logo, wxImage selectlogo, wxImage checkLogo);
protected:

private:
	std::vector<AnkerEasyItem*> m_itemVector;
	std::map<wxString, AnkerEasyItem*> m_ItemMap;
	wxBoxSizer* m_pItemVSizer;
	wxStaticText* m_pTipsLabel;

	wxString m_fastStrTips;
	wxString m_normalStrTips;
	wxString m_PrecisionStrTips;
};


enum ANKER_EASY_ITEM
{
	ITEM_NOR = 1,
	ITEM_ENTER,
	ITEM_SELECT
};

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_EASYITEM_CLICKED, wxCommandEvent);
class AnkerEasyItem :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerEasyItem)
	DECLARE_EVENT_TABLE()
public:
	AnkerEasyItem();
	AnkerEasyItem(wxWindow* parent,
		wxString title,
		wxImage  logo,
		wxImage  selectlogo,
		wxImage  checkLogo,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerEasyItem();

	wxString getTitle();
	void setSelectStatus(const ANKER_EASY_ITEM& isSelect);
	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);
private:

	ANKER_EASY_ITEM m_Status = ITEM_NOR;

	wxImage  m_logo;
	wxImage  m_selectLogo;
	wxImage  m_checkLogo;
	wxString m_title;	

};
#endif


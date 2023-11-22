#ifndef _ANKER_POPUP_WIDGET_HPP_
#define _ANKER_POPUP_WIDGET_HPP_

#include "wx/popupwin.h"
#include "wx/scrolwin.h"
#include "wx/sizer.h"
#include "wx/dcclient.h"

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_ITEM_CLICKED, wxCommandEvent);


enum SEARCH_ITEM_STATUS
{
	SEARCH_ITEM_NOR = 0,
	SEARCH_ITEM_HOVER,
	SEARCH_ITEM_SELECT
};

class AnkerSearchItem :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerControlWidget)
	DECLARE_EVENT_TABLE()
public:
	AnkerSearchItem();
	AnkerSearchItem(wxWindow* parent,
		wxString  key,
		wxString text,
		wxFont font,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerSearchItem();

	virtual void OnPressed(wxMouseEvent& event);
	virtual void OnDClick(wxMouseEvent& event);
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
protected:
	void OnPaint(wxPaintEvent& event);
private:
	wxString  m_key;
	wxString  m_text;
	wxFont	  m_font;
	wxColour  m_textColor;
	wxColour  m_bgColor;
	wxColour  m_hoverColor;
	SEARCH_ITEM_STATUS m_status;
};


class AnkerPopupWidget : public wxPopupWindow
{
public:
	AnkerPopupWidget(wxWindow* parent);
	~AnkerPopupWidget();

	void AddItem(wxString key,wxString text);
	void AddItem(const std::map<wxString, std::vector<wxString>>& searchMap);
	void showResMap(const std::map<wxString, std::vector<wxString>>& searchMap);
	void showAllItem();
protected:
	void initUi();
	
private:
	wxScrolledWindow* m_scrolledWindow{nullptr};
	wxBoxSizer* m_pScrolledVSizer{ nullptr };
	std::multimap<wxString, AnkerSearchItem*> m_itemMap;//Item -- label
};
#endif


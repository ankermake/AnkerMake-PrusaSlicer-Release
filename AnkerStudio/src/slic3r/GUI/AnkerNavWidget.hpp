#ifndef _Anker_NAV_WIDGET_HPP_
#define _Anker_NAV_WIDGET_HPP_

#include "wx/wx.h"
#include "AnkerTextLabel.hpp"
#include "AnkerCustomEvent.hpp"
#include <wx/scrolwin.h>

enum ANKER_BTN_STATUS
{
	NORMAL_BTN = 0,
	HOVER_BTN,
	SELECT_BTN,
	DISABLE_BTN,
	UNKNOWN_BTN
};

wxDECLARE_EVENT(wxCUSTOMEVT_BTN_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_SWITCH_DEVICE, wxCommandEvent);

class AnkerTabBtn :public wxControl
{
	DECLARE_DYNAMIC_CLASS(AnkerTabBtn)
	DECLARE_EVENT_TABLE()
public:
	AnkerTabBtn();
	AnkerTabBtn(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxString& btnName = wxString(""),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerTabBtn() {}

	virtual void OnClick(wxMouseEvent& event);
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual bool Enable(bool enable = true);
	virtual void OnSize(wxSizeEvent& event);
	void setTabBtnName(const std::string& btnName);

	void setBtnStatus(ANKER_BTN_STATUS status);
protected:
	void OnPaint(wxPaintEvent& event);	
	void OnLabelClicked(AnkerCustomEvent& event);

private:

	wxString  m_tabName;
	wxBitmap  m_icon;

	ANKER_BTN_STATUS m_status;

};

class AnkerBtnList :public wxControl
{
public:
	AnkerBtnList(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerBtnList() {}

	void clearExpiredTab(const std::string& sn);
	bool checkTabExist(const std::string& sn);
	void clearList();
	int  getCount();
	void SetCurrentTab(const int& index);
	bool InsertTab(const size_t& index,
		const std::string& tabName,
		const std::string& snID,
		bool isSelect = false);
	void RemoveTab(const size_t& index);
protected:
	void initUi();
private:
	std::vector<AnkerTabBtn*>  m_tabBtnList;
	wxBoxSizer				 * m_pScrolledwinSizer;
	wxScrolledWindow		 * m_pScrolledWindow;
	int						   m_currentIndex;
};

class AnkerNavBar :public wxControl
{
public:	
	AnkerNavBar(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerNavBar();

	void updateRefresh(bool isRefreh);
	bool checkTabExist(const std::string& sn);
	void clearExpiredTab(const std::string& sn);
	int getCount()const;
	void addItem(const std::string& ItemName, const std::string& snId);
	void clearItem();
	void showEmptyPanel(bool isShow);
protected:
	void InitUi();
	void OnTimer(wxTimerEvent& event);
private:
	wxButton* m_reloadBtn;
	bool	  m_isRefresh = false;
	AnkerBtnList* m_navBarList;
	wxPanel* m_emptyPanel;
	wxTimer* m_reloadTimer;
};


#endif // !_Anker_NAV_WIDGET_HPP_



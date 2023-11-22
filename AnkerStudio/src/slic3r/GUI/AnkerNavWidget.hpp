#ifndef _Anker_NAV_WIDGET_HPP_
#define _Anker_NAV_WIDGET_HPP_

#include "wx/wx.h"
#include "AnkerTextLabel.hpp"
#include "AnkerCustomEvent.hpp"
#include <wx/scrolwin.h>
#include "Common/AnkerLoadingLabel.hpp"

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
		const wxString& snId,
		wxWindowID winid = wxID_ANY,		
		const wxString& btnName = wxString(""),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~AnkerTabBtn() {}
	wxString getSnId();
	void setIcon(const wxString& strIcon);
	virtual void OnClick(wxMouseEvent& event);
	virtual void OnEnter(wxMouseEvent& event);
	virtual void OnLeave(wxMouseEvent& event);
	virtual void OnPressed(wxMouseEvent& event);
	virtual bool Enable(bool enable = true);
	virtual void OnSize(wxSizeEvent& event);
	void setTabBtnName(const std::string& btnName);

	void setBtnStatus(ANKER_BTN_STATUS status);
	void setOnlineStatus(bool isOnline);
protected:
	void OnPaint(wxPaintEvent& event);	
	void OnLabelClicked(AnkerCustomEvent& event);
	//TODO: by Samuel,to refresh the printers status
	void GetPrinterStatus();

private:

	wxString  m_tabName;
	wxBitmap  m_icon;
	wxBitmap  m_offlineIcon;
	wxBitmap  m_statusIcon;//the icon to represent printer status

	bool	  m_isOnline = true;
	ANKER_BTN_STATUS m_status;
	wxString  m_snid;

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
	void switchTabFromSn(const std::string& sn);
	void clearList();
	int  getCount();
	void SetCurrentTab(const int& index);
	bool InsertTab(const size_t& index,
		const std::string& tabName,
		const std::string& snID,
		bool isSelect = false);
	void RemoveTab(const size_t& index);
	void setTabOnlineStatus(bool isOnline, const wxString& snId);
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

	void stopLoading();
	
	bool checkTabExist(const std::string& sn);
	void switchTabFromSn(const std::string& sn);
	void clearExpiredTab(const std::string& sn);
	int getCount()const;
	void addItem(const std::string& ItemName, const std::string& snId);
	void clearItem();
	void showEmptyPanel(bool isShow);
	void setTabOnlineStatus(bool isOnline, const wxString& snId);
protected:
	void InitUi();	
private:
	AnkerLoadingLabel* m_reloadBtn;
	//bool	  m_isRefresh = false;
	AnkerBtnList* m_navBarList;
	wxPanel* m_emptyPanel;
};


#endif // !_Anker_NAV_WIDGET_HPP_



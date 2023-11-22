#ifndef ANKER_LIST_ITEM_HPP
#define ANKER_LIST_ITEM_HPP

#include "wx/listbox.h"
#include "wx/listctrl.h"
#include "AnkerBase.hpp"
#include "AnkerGUIConfig.hpp"
class AnkerListCtrl : public wxListCtrl, public AnkerBase
{
public:
    AnkerListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
	~AnkerListCtrl();
    
};  


class AnkerCopyrightListCtrl : public AnkerListCtrl
{
public:
    AnkerCopyrightListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
    void initLinks(int totalRow, int totalColumn);
    void setLink(int row, int column, const std::string& label, const std::string& link);

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMouseClick(wxMouseEvent& event);
    void OnScrollThumbTrack(wxScrollWinEvent& event);
    void drawItems(wxDC& dc);
    void OnItemClick(wxListEvent& event);
    void OnColumnClick(wxListEvent& event);

    void getColumn(const wxPoint& point, int& column);
    void getRow(const wxPoint& point, int& itemIndex);

private:
    std::vector<std::vector<std::pair<std::string, std::string>>> m_links;
    int m_lineHeight = 10;
};


//class AnkerListBoxCtrl : public wxListBox
//{
//public:
//	AnkerListBoxCtrl(wxWindow* parent, wxWindowID id,
//		const wxPoint& pos = wxDefaultPosition,
//		const wxSize& size = wxDefaultSize,
//		int n = 0, const wxString choices[] = NULL,
//		long style = 0,
//		const wxValidator& validator = wxDefaultValidator,
//		const wxString& name = wxASCII_STR(wxListBoxNameStr));
//	~AnkerListBoxCtrl();
//};
//
//
//class AnkerChooseDeviceListBox : public AnkerListBoxCtrl
//{
//public:
//	struct ListBoxItemData {
//		wxString label;
//		wxControl* control;
//	};
//	AnkerChooseDeviceListBox(wxWindow* parent, wxWindowID id,
//		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
//
//	virtual void AddChooseDeviceItem(void* pData);
//	virtual void selectedChooseDeviceItem(int index = 0);
//	virtual void updateSelectedStatus(int index);
//
//	std::string getSelcetedSn() const;
//
//protected:
//	void AddControlItem(wxControl* control, const wxString& label);
//	wxControl* GetControlItem(int index) const;
//	void DeleteItemData(size_t index);
//
//	std::string m_selectedSn;
//};
//
//
//
//class  AnkerMulticolorSysncFilamentListBox : public AnkerChooseDeviceListBox
//{
//public:
//	AnkerMulticolorSysncFilamentListBox(
//		wxWindow* parent, wxWindowID id,
//		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
//	virtual void  AddChooseDeviceItem(void* pData);
//	virtual void  selectedChooseDeviceItem(int index = 0);
//	virtual void  updateSelectedStatus(int index);
//};



#endif // !ANKER_LIST_ITEM_HPP

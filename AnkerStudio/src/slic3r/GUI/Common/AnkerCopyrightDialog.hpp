#ifndef ANKER_COPYRIGHT_DIALOG_PANEL_HPP
#define  ANKER_COPYRIGHT_DIALOG_PANEL_HPP

#include "AnkerDialog.hpp"
#include "AnkerListCtrl.hpp"
#include <vector>

class AnkerCopyrightDiaogPanel : public AnkerDialogPanel
{
public:
    AnkerCopyrightDiaogPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, int totalRow = 26, int totalColumn = 2);
	void insertColumnHeader(long column, const wxString& str = "");
	void setAutoColumnWidth(long column);
	void setColumnWidth(long column, int width);
	long insertRowItemHeader(long row, const wxString& str = "");
	void insertItem(long row, long column, const wxString& str = "");
	void setColumnItem(long column,  AnkerCopyrightListCtrl& item);
	void setItem(AnkerCopyrightListCtrl& item);

	void setHeaderAttr(const wxItemAttr& attr);
	void setItemTextColour(long item, const wxColour& colour);
	void setLink(int row, int column, const std::string& label, const std::string& link);

private:
	AnkerCopyrightListCtrl* m_list;
	
};

class AnkerCopyrightDialog : public  AnkerDialog
{
public:
	AnkerCopyrightDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context = "",
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerCopyrightDialog();

	virtual void InitDialogPanel(int dialogType = 0);
	//virtual void setBackgroundColour(const wxColour& color = "#333438");
	
private:

};


#endif // !ANKER_COPYRIGHT_DIALOG_PANEL_HPP

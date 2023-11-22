#ifndef ANKER_FRAME_MODAL_HPP
#define ANKER_FRAME_MODAL_HPP

#include "AnkerDialog.hpp"
#include <wx/timer.h>

enum PartialModal {
	PartialModal_OK,
	PartialModal_CANCEL_OK,
};

class PartialModalCancalOkPanel : public AnkerDialogDisplayTextCancelOkPanel
{
public:
	PartialModalCancalOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, const wxString& context = "");

protected:
	virtual void cancelButtonClicked(wxCommandEvent& event);
	virtual void  okButtonClicked(wxCommandEvent& event);

};

class PartialModalOkPanel : public AnkerDialogDisplayTextOkPanel
{
public:
	PartialModalOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, const wxString& context = "");

protected:
	virtual void okButtonClicked(wxCommandEvent& event);
};


class HalfModalDialog : public AnkerDialog
{
public:
	HalfModalDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~HalfModalDialog();

	virtual void InitDialogPanel(int dialogType = 0);
	virtual void setBackgroundColour(const wxColour& color = "#333438");
	virtual int ShowAnkerModal(int dialogType = 0);
	
	void ShowAnker(int dialogType = 0);

	void HideWindow(bool hide = true);
	void CheckWindowShow();
	
private:
};

class HalfModalPanel : public wxPanel
{
public:
	HalfModalPanel(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));

	void InitDialogPanel(int dialogType = 0);
	void ShowAnkerModal(int dialogType = 0);

	void OnMouseEvents(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

private:
	HalfModalDialog* m_dialog;
};


#endif // !ANKER_FRAME_MODAL_HPP

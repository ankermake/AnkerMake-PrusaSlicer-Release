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

	void ShowNoTitle(AnkerDialogIconTextOkPanel::EventCallBack_T callback);

	void SetOkBtnCallBack(AnkerDialogBtnCallBack_T callback = nullptr);

	void HideWindow(bool hide = true);
	void CheckWindowShow();
	
private:	
	AnkerDialogBtnCallBack_T m_okBtnCallBack = nullptr;
};

#endif // !ANKER_FRAME_MODAL_HPP

#ifndef ANKER_FRAME_MODAL_HPP
#define ANKER_FRAME_MODAL_HPP

#include "AnkerDialog.hpp"
#include <wx/timer.h>

enum PartialModal {
	PartialModal_OK,
	PartialModal_IMAGE_OK,
	PartialModal_CANCEL_OK,
};

class PartialModalCancalOkPanel : public AnkerDialogDisplayTextCancelOkPanel
{
public:
	PartialModalCancalOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, const wxString& context = "");

protected:
	virtual void cancelButtonClicked(wxCommandEvent& event);
	virtual void okButtonClicked(wxCommandEvent& event);
};

class HalfModalDialog : public AnkerDialog
{
public:
	HalfModalDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context,
		const wxString& imageName = "",
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~HalfModalDialog();

	virtual void InitDialogPanel(int dialogType = 0);
	virtual void setBackgroundColour(const wxColour& color = "#333438");
	
	void ShowAnker(int dialogType = 0);

	void SetOkBtnCallBack(AnkerDialogBtnCallBack_T callback = nullptr);

	void CheckWindowShow();

private:	
	AnkerDialogBtnCallBack_T m_okBtnCallBack = nullptr;
	wxString m_imageName;
};

#endif // !ANKER_FRAME_MODAL_HPP

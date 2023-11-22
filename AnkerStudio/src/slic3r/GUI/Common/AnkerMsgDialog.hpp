#ifndef _ANKER_MSG_DIALOG_H_
#define _ANKER_MSG_DIALOG_H_

#include "wx/wx.h"
#include "slic3r/GUI/I18N.hpp"
#include "AnkerDialog.hpp"


class AnkerBtn;
class AnkerMsgDialog : public AnkerDialogBase
{
public:
	enum MsgResult
	{
		MSG_OK,
		MSG_CANCEL,
		MSG_CLOSE
	};

public:
	AnkerMsgDialog(wxWindow* parent, std::string message = "Hello Anker", std::string title = "MessageBox");
	~AnkerMsgDialog();

	void setTitle(std::string title);
	void setMessage(wxString message);
	void setIconPath(std::string path);
	void setOKText(std::string text);
	void setCancelText(std::string text);
	void setOKVisible(bool visible);
	void setCancelVisible(bool visible);
	MsgResult getMsgResult();

private:
	void initUI();
	void initEvent();

	void OnExitButtonClicked(wxCommandEvent& event);
	void OnOKButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

private:
	int m_titleFontSize;
	int m_messageFontSize;
	bool m_dragging;
	wxPoint m_dragStartPos;

	std::string m_title;
	wxString m_message;
	std::string m_iconPath;
	std::string m_okText;
	std::string m_cancelText;
	MsgResult m_result;

	wxStaticText* m_pTitleText;
	wxStaticText* m_pMessageText;
	wxButton* m_pExitBtn;
	AnkerBtn* m_pOKBtn;
	AnkerBtn* m_pCancelBtn;
	wxSizerItem* m_pBtnSpaceItem{nullptr};
	bool		m_bIsOkShow{ true };
	bool		m_bIsCancelShow{ true };
};

static AnkerMsgDialog::MsgResult AnkerMessageBox(wxWindow* parent, std::string message, std::string title, bool cancelVisible = true, std::string okText = "", std::string cancelText = "")
{
	static bool isShowModal = false;
	if (isShowModal) {
		return AnkerMsgDialog::MSG_CANCEL;
	}
	isShowModal = true;

	if (okText.empty())
		okText = _("common_button_ok").ToStdString();

	if (cancelText.empty())
		cancelText = _("common_button_cancel").ToStdString();

	AnkerMsgDialog* msgBox = new AnkerMsgDialog(parent);
	msgBox->setMessage(message);
	msgBox->setTitle(title);
	msgBox->setOKText(okText);
	msgBox->setCancelVisible(cancelVisible);
	msgBox->setCancelText(cancelText);

	msgBox->ShowModal();

	AnkerMsgDialog::MsgResult result = msgBox->getMsgResult();
	msgBox->Reparent(nullptr);
	delete msgBox;
	msgBox = nullptr;
	isShowModal = false;
	return result;
}

#endif // _ANKER_MSG_DIALOG_H_


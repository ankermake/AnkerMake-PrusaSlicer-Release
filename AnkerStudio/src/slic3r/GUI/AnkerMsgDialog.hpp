#ifndef _ANKER_MSG_DIALOG_H_
#define _ANKER_MSG_DIALOG_H_

#include "wx/wx.h"


class AnkerBtn;
class AnkerMsgDialog : public wxDialog
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
	void setMessage(std::string message);
	void setIconPath(std::string path);
	void setOKText(std::string text);
	void setCancelText(std::string text);
	void setOKVisible(bool visible);
	void setCancelVisible(bool visible);
	MsgResult getMsgResult();

private:
	void initUI();

	void OnExitButtonClicked(wxCommandEvent& event);
	void OnOKButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

private:
	int m_titleFontSize;
	int m_messageFontSize;

	std::string m_title;
	std::string m_message;
	std::string m_iconPath;
	std::string m_okText;
	std::string m_cancelText;
	MsgResult m_result;

	wxStaticText* m_pTitleText;
	wxStaticText* m_pMessageText;
	wxButton* m_pExitBtn;
	AnkerBtn* m_pOKBtn;
	AnkerBtn* m_pCancelBtn;
};

static AnkerMsgDialog::MsgResult AnkerMessageBox(wxWindow* parent, std::string message, std::string title, bool cancelVisible = true, std::string okText = "OK", std::string cancelText = "Cancel")
{
	static bool isShowModal = false;
	if (isShowModal) {
		return AnkerMsgDialog::MSG_CANCEL;
	}
	isShowModal = true;
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


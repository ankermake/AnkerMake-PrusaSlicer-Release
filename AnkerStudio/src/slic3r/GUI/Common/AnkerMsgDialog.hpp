#ifndef _ANKER_MSG_DIALOG_H_
#define _ANKER_MSG_DIALOG_H_

#include "wx/wx.h"
#include "slic3r/GUI/I18N.hpp"
#include "AnkerDialog.hpp"
#include <wx/richtext/richtextctrl.h>

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


// Notice: messge and title parameter should be utf8 format
static AnkerMsgDialog::MsgResult AnkerMessageBox(wxWindow* parent, std::string message, std::string title, 
	bool cancelVisible = true, std::string okText = "", std::string cancelText = "")
{
	static bool isShowModal = false;
	if (isShowModal) {
		return AnkerMsgDialog::MSG_CANCEL;
	}
	isShowModal = true;

	if (okText.empty())
		okText = _("common_button_ok").ToStdString(wxConvUTF8);

	if (cancelText.empty())
		cancelText = _("common_button_cancel").ToStdString(wxConvUTF8);

	AnkerMsgDialog* msgBox = new AnkerMsgDialog(parent);
	msgBox->setMessage(wxString::FromUTF8(message));
	msgBox->setTitle(title);
	msgBox->setOKText(okText);
	msgBox->setCancelVisible(cancelVisible);
	msgBox->setCancelText(cancelText);
	msgBox->Center(wxBOTH);
	msgBox->Raise();
	msgBox->ShowModal();

	AnkerMsgDialog::MsgResult result = msgBox->getMsgResult();
	delete msgBox;
	msgBox = nullptr;
	isShowModal = false;
	return result;
}

// fix: AnkerMessageBox would crash at msgBox->GetParent()
// Notice: messge and title parameter should be utf8 format
static AnkerMsgDialog::MsgResult AnkerMessageBoxV2(wxWindow* parent, std::string message, std::string title,
	bool cancelVisible = true, std::string okText = "", std::string cancelText = "")
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

	AnkerMsgDialog msgBox (parent);
	msgBox.setMessage(wxString::FromUTF8(message));
	msgBox.setTitle(title);
	msgBox.setOKText(okText);
	msgBox.setCancelVisible(cancelVisible);
	msgBox.setCancelText(cancelText);
	msgBox.Center(wxBOTH);
	msgBox.ShowModal();

	AnkerMsgDialog::MsgResult result = msgBox.getMsgResult();

	isShowModal = false;
	return result;
}

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_LEARN_MORE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_OK, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_OTHER, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_CANCEL, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CUSTOM_CLOSE, wxCommandEvent);


#define	LEVEL_S "S"
#define	LEVEL_P0 "P0"
#define	LEVEL_P1 "P1"
#define	LEVEL_P2 "P2"	


class AnkerCustomDialog : public wxDialog
{
public:
	AnkerCustomDialog(wxWindow* parent = nullptr, wxString content = "");
	~AnkerCustomDialog();

	void clearData();
	void setValue(const wxString& strContent);
	void setValue(const wxString& strContent,
					const std::string& url, 
					const std::string& errorCode,
					const std::string& msgLevel,					
					const std::string &sn,
					const wxString& title);
	std::string getDialogSn()const { return m_sn; }
	std::string getDialogErrCode()const { return m_errorCode; }
protected:
	void initUi();
	bool checkErrorLevel(const std::string& errorLevel);
private:
	void OnButton(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	wxBoxSizer* m_pMainVSizer{ nullptr };
	wxRichTextCtrl* m_pContentTextCtrl{ nullptr };
	AnkerBtn* m_pLearnMoreBtn{ nullptr };
	AnkerBtn* m_pOkBtn{ nullptr };
	std::string m_sn = "";
	std::string m_url = "";
	std::string m_errorCode = "";
	std::string m_msgLevel = "";
};

#endif // _ANKER_MSG_DIALOG_H_


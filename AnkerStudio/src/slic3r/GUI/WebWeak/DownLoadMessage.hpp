#ifndef DOWNLOAD_MESSAGE_MANAGER_H_
#define DOWNLOAD_MESSAGE_MANAGER_H_

#include "wx/wx.h"
#include "slic3r/GUI/AnkerBtn.hpp"
#include "slic3r/GUI/Common/AnkerMsgDialog.hpp"

namespace Slic3r {
namespace GUI {
enum class CancelType { Close_Type, Cancel_Type };
using CancelCb = std::function<void(CancelType type)>;
wxDECLARE_EVENT(EVT_DOWNLOAD_CANCEL, wxCommandEvent);
class DownLoadMsgDialog : public AnkerDialogBase
{
public:
	DownLoadMsgDialog(wxWindow* parent, std::string message = "Hello Anker", std::string title = "MessageBox");
	~DownLoadMsgDialog();

	void setExitVisible(bool visible);
	void setCancelText(std::string text);
	void updateMsg(const wxString& message);
	void setCanShow(bool bShow) { m_bCanShow = bShow; }
	bool IsCanShow() { return m_bCanShow; }
private:
	void initUI();
	void OnExitButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

private:
	bool m_bCanShow{ true };
	wxString m_message;
	std::string m_title;
	std::string m_cancelText;
	AnkerBtn* m_pCancelBtn { nullptr };
	wxStaticText* m_pTitleText { nullptr };
	wxStaticText* m_pMessageText { nullptr };
	wxSizerItem* m_pBtnSpaceItem { nullptr };
};

class DownLoadMessageManager
{
public:
	DownLoadMessageManager() = default;
	~DownLoadMessageManager();

	void Init();
	void onHideProcess();
	void onDownLoadComplete();
	void SetPercent(const wxString& msg);
	void ShowError(const std::string& title, const wxString& msg);

	void SetCancelCb(CancelCb cb) {
		m_cb = std::move(cb);
	}

private:
	CancelCb m_cb{ nullptr };
	DownLoadMsgDialog* m_pdlg{ nullptr };
};
}
}

#endif // !DOWNLOAD_MESSAGE_MANAGER_H_

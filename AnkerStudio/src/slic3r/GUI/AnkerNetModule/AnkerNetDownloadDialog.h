#pragma once

#include "slic3r/GUI/Common/AnkerDialog.hpp"

class AnkerProgressCtrl;

class AnkerNetBtnPanel : public AnkerDialogPanel
{
public:
	using OkFunc_T = std::function<void()>;
	using CancelFunc_T = std::function<void()>;

	AnkerNetBtnPanel(wxWindow* parent, 
		const wxString& title = "", 
		const wxSize& size = wxDefaultSize,
		bool hideCancelBtn = false);

	virtual void BindBtnEvent(OkFunc_T okFunc = nullptr, CancelFunc_T cancelFunc = nullptr);

protected:
	virtual void setOkBtnText(const wxString& text = _AnkerL("common_button_ok"));
	virtual void setCancelBtnText(const wxString& text = _AnkerL("common_button_cancel"));
	virtual void cancelButtonClicked(wxCommandEvent& event);
	virtual void okButtonClicked(wxCommandEvent& event);

protected:
	AnkerBox* m_centerPanel = nullptr;

protected:
	AnkerBtn* m_okBtn = nullptr;
	AnkerBtn* m_cancelBtn = nullptr;
};

class AnkerNetInstallPanel : public AnkerNetBtnPanel
{
public:
	AnkerNetInstallPanel(wxWindow* parent = nullptr,
		const wxString& title = "",
		const wxString& context = "",		
		const wxSize & size = wxDefaultSize);

private:
	AnkerStaticText* m_contextText;
};

class AnkerNetProgressPanel : public AnkerNetBtnPanel
{
public:
	AnkerNetProgressPanel(wxWindow* parent = nullptr,
		const wxString& title = "",
		const wxSize& size = wxDefaultSize);
	void UpdateProgress(double progress);

private:
	AnkerProgressCtrl* m_progressBar = nullptr;
	AnkerStaticText* m_progressText = nullptr;
};

class AnkerNetStatusPanel : public AnkerNetBtnPanel
{
public:
	AnkerNetStatusPanel(bool success = true,
		bool hideOkBtn = false,
		wxWindow* parent = nullptr,
		const wxString& title = "",
		const wxString& context = "",
		const wxSize& size = wxDefaultSize);

private:
	bool m_success;
	wxStaticBitmap* m_statusImage;
	AnkerStaticText* m_progressText = nullptr;
};

class AnkerNetSuccessPanel : public AnkerNetStatusPanel
{
public:
	AnkerNetSuccessPanel(wxWindow* parent = nullptr,
		const wxString& title = "",
		const wxSize& size = wxDefaultSize);
};

class AnkerNetFailedPanel : public AnkerNetStatusPanel
{
public:
	AnkerNetFailedPanel(wxWindow* parent = nullptr,
		const wxString& title = "",
		const wxSize& size = wxDefaultSize);
};

class AnkerNetDownloadDialog : public wxDialog
{
public:
	enum class Status
	{
		InstallHint,
		UpdateHint,
		DownLoading,
		DownLoadSucc,
		DownLoadFailed
	};
	enum class Result
	{
		Success,
		Cancel,	
		Failed
	};

	AnkerNetDownloadDialog(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerNetDownloadDialog();

	void Change(Status status, 
		AnkerNetBtnPanel::OkFunc_T okFunc = nullptr, 
		AnkerNetBtnPanel::CancelFunc_T cancelFunc = nullptr);
	int ShowHint(Status status, 
		AnkerNetBtnPanel::OkFunc_T okFunc = nullptr, 
		AnkerNetBtnPanel::CancelFunc_T cancelFunc = nullptr);
	void UpdateProgress(int progress);

private:
	void ChangeInternal(Status status,
		AnkerNetBtnPanel::OkFunc_T okFunc = nullptr,
		AnkerNetBtnPanel::CancelFunc_T cancelFunc = nullptr);

public:
	static const int defualtWidth = 400;
	static const int defualtHeight = 200;	

protected:
	AnkerNetBtnPanel* m_panel = nullptr;
	wxString m_title;
	wxSize m_size;

private:
	static const int resultHeight = 288;
	wxPoint defaultPos;
};
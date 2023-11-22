#ifndef ANKER_DILAOG_HPP
#define ANKER_DILAOG_HPP


#include "AnkerBox.hpp"
#include "AnkerLine.hpp"
#include "AnkerButton.hpp"
#include "AnkerStaticText.hpp"
#include "../GUI_App.hpp"
#include "../AnkerBtn.hpp"

enum AnkerDialogType {
	AnkerDialogType_Dialog,
	AnkerDialogType_OkDialog,
	AnkerDialogType_CancelOkDialog,
	AnkerDialogType_DisplayTextOkDialog,
	AnkerDialogType_DisplayTextCancelOkDialog,
	AnkerDialogType_DisplayTextNoYesDialog,
	AnkerDialogType_DisplayTextCheckBoxNoYesDialog,
	AnkerDialogType_CustomContent,
};

#define EMPTY_EVENTCB (std::function<void(wxCommandEvent&, wxControl*)>())

class AnkerDialogPanel : public AnkerBox
{
public:
	AnkerDialogPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize);

	void closeButtonClicked(wxCommandEvent& event);
	virtual void closeWindow();
	
	void disableCloseButton(bool disable);
	void hideCloseButton(bool hide);

public:
	wxSizer* m_mainSizer;
protected:
	AnkerLine2* m_line;

private:
	AnkerStaticText* m_title;
	AnkerButton* closeBtn;
};

// TODO: this dialog should use factory mode
class AnkerDialog : public wxDialog
{
public:
	using EventCallBack_T = std::function<void(wxCommandEvent&, wxControl*)>;
	AnkerDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& context,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerDialog();

	virtual void InitDialogPanel(int dialogType = 0);
	virtual void InitDialogPanel2(int dialogType = 0, const wxString& otherText = "", EventCallBack_T eventCallBack = EMPTY_EVENTCB);
	virtual void setBackgroundColour(const wxColour& color = "#333438");
	virtual int ShowAnkerModal(int dialogType = 0);
	virtual int ShowAnkerModal2(int dialogType = 0, const wxString& otherText = "", EventCallBack_T eventCallBack = EMPTY_EVENTCB);
	bool ShowAnker(int dialogType = 0);
	bool ShowAnker2(int dialogType = 0, EventCallBack_T eventCallBack = EMPTY_EVENTCB);

	void SetCustomContent(wxWindow* customContent);
protected:
	AnkerDialogPanel* m_panel;
	wxString m_title;
	wxSize m_size;
	wxString m_context;	
	wxWindow* m_customContent = nullptr;
};


class AnkerDialogCustomSizer : public AnkerDialogPanel
{
public:
	AnkerDialogCustomSizer(wxWindow* parent, wxBoxSizer* contentSizer, const wxString& title = "",  const wxSize& size = wxDefaultSize);
	~AnkerDialogCustomSizer();
};

class AnkerDialogOkPanel : public AnkerDialogPanel
{
public:
	AnkerDialogOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize);
	~AnkerDialogOkPanel();
	void setOkBtnText(const wxString& text);

protected:
	AnkerBox* m_centerPanel;
	virtual void okButtonClicked(wxCommandEvent& event);

protected:
	AnkerBtn* m_okBtn;
};

class AnkerDialogDisplayTextOkPanel : public AnkerDialogOkPanel
{
public:
	AnkerDialogDisplayTextOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, const wxString& context = "");
	~AnkerDialogDisplayTextOkPanel();

private:
	AnkerStaticText* m_contextText;
};


class AnkerDialogCancelOkPanel : public AnkerDialogPanel
{
public:
	AnkerDialogCancelOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize);
	~AnkerDialogCancelOkPanel();

	virtual void bindBtnEvent();
	void setOkBtnText(const wxString& text = _AnkerL("common_button_ok"));
	void setCancelBtnText(const wxString &text = _AnkerL("common_button_cancel"));
protected:
	AnkerBox* m_centerPanel;

	virtual void cancelButtonClicked(wxCommandEvent& event);
	virtual void  okButtonClicked(wxCommandEvent& event);

protected:
	AnkerBtn* m_okBtn;
	AnkerBtn* m_cancelBtn;
};

class AnkerDialogDisplayTextCancelOkPanel : public AnkerDialogCancelOkPanel
{
public:
	AnkerDialogDisplayTextCancelOkPanel(wxWindow* parent, const wxString& title = "", const wxSize& size = wxDefaultSize, const wxString& context = "");
	~AnkerDialogDisplayTextCancelOkPanel();

private:
	AnkerStaticText* m_contextText;
};


class AnkerDialogDisplayTextCheckBoxCancelOkPanel : public AnkerDialogCancelOkPanel
{
public:
	AnkerDialogDisplayTextCheckBoxCancelOkPanel(const wxString& title = "", 
		const wxSize& size = wxDefaultSize, const wxString& context = "", 
		const wxString& checkBoxStr = "", AnkerDialog::EventCallBack_T callback = EMPTY_EVENTCB, wxWindow * parent = nullptr);
	~AnkerDialogDisplayTextCheckBoxCancelOkPanel();

private:
	AnkerStaticText* m_contextText;
};

class AnkerDialogBase : public wxDialog
{
public:
	AnkerDialogBase(wxWindow* parent, wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerDialogBase();

protected:
	void SetSizer(wxSizer* sizer);
	void SetSize(wxSize size);
	void SetTitle(wxString title);
	void SetBackgroundColour(wxColour colour);
	void OnDragMouseDown(wxMouseEvent& event);
	void OnDragMouseMove(wxMouseEvent& event);
	void OnDragMouseUp(wxMouseEvent& event);
	void OnDragMouseLost(wxMouseCaptureLostEvent& event);
	void OnExitButtonClicked(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
protected:
	wxSizer* m_mainSizer;
	wxSizer* m_titleSizer;
	wxSizer* m_contentSizer;
	wxPanel* m_titlePanel;
	wxStaticText* m_titleText;
	wxButton* m_pExitBtn;
	wxString m_title;
	wxPoint m_startPos;
	wxRect  m_titlePanelRect;
};



#endif // !ANKER_DILAOG_HPP

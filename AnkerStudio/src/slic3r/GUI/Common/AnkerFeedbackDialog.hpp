#pragma once

#include "wx/wx.h"
#include <wx/richtext/richtextctrl.h>
#include "AnkerLineEditUnit.hpp"
#include "slic3r/GUI/Network/basetype.hpp"
#include "AnkerDialog.hpp"

class AnkerBtn;
class AnkerFeedbackDialog : public AnkerDialogBase
{
public:

public:
	AnkerFeedbackDialog(wxWindow* parent, std::string title, wxPoint position, wxSize size);
	~AnkerFeedbackDialog();

	void setTitle(std::string title);
	void setIconPath(std::string path);
	void setOKText(std::string text);
	void setCancelText(std::string text);
	void setOKVisible(bool visible);
	void setCancelVisible(bool visible);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	HttpsType::FeedBackInfo GetFeedBack() { return m_feedback; }

private:
	void initUI();
	void initEvent();

	void OnExitButtonClicked(wxCommandEvent& event);
	void OnOKButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

	bool isValidEmail(wxString email);
	void checkOkBtn();
private:
	int m_titleFontSize;
	int m_messageFontSize;
	HttpsType::FeedBackInfo m_feedback;
	bool m_dragging;
	wxPoint m_dragStartPos;

	std::string m_title;
	std::string m_iconPath;
	std::string m_okText;
	std::string m_cancelText;

	wxRichTextCtrl* m_pTextEdit;
	wxStaticText* m_pTitleText;
	AnkerLineEditUnit* m_pEmailLineEdit;
	wxButton* m_pExitBtn;
	AnkerBtn* m_pOKBtn;
	AnkerBtn* m_pCancelBtn;
	wxSizerItem* m_pBtnSpaceItem{ nullptr };
	bool		m_bIsOkShow{ true };
	bool		m_bIsCancelShow{ true };
	bool		m_bIsShareLog{ true };
};


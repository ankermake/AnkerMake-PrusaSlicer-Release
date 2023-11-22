#pragma once

#include "wx/wx.h"
#include "AnkerLineEditUnit.hpp"
#include "AnkerDialog.hpp"

class AnkerBtn;
class AnkerNumberEnterDialog : public AnkerDialogBase
{
public:

public:
	AnkerNumberEnterDialog(wxWindow* parent, std::string title = "Number Enter Dislog", wxString prompt = "", double max = 1, double min = 0, double value = 0);
	~AnkerNumberEnterDialog();

	void setTitle(std::string title);
	void setPrompt(wxString prompt);
	void setIconPath(std::string path);
	void setOKText(std::string text);
	void setCancelText(std::string text);
	void setOKVisible(bool visible);
	void setCancelVisible(bool visible);
	void setEditLineText(wxString value);
	long GetValue() { return m_textValue; }
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	void initUI();
	void initEvent();

	void OnExitButtonClicked(wxCommandEvent& event);
	void OnOKButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

private:
	int m_titleFontSize;
	int m_messageFontSize;
	double m_max;
	double m_min;
	long m_textValue;
	bool m_dragging;
	wxPoint m_dragStartPos;

	std::string m_title;
	wxString m_prompt;
	std::string m_iconPath;
	std::string m_okText;
	std::string m_cancelText;

	AnkerLineEditUnit* m_pLineEdit;
	wxStaticText* m_pTitleText;
	wxStaticText* m_pPromptText;
	wxButton* m_pExitBtn;
	AnkerBtn* m_pOKBtn;
	AnkerBtn* m_pCancelBtn;
	wxSizerItem* m_pBtnSpaceItem{ nullptr };
	bool		m_bIsOkShow{ true };
	bool		m_bIsCancelShow{ true };
};

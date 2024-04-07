#ifndef _ANKER_TITLED_PANEL_H_
#define _ANKER_TITLED_PANEL_H_

#include "wx/wx.h"


wxDECLARE_EVENT(wxANKEREVT_ATP_BUTTON_CLICKED, wxCommandEvent);

class AnkerTitledPanel : public wxPanel
{
public:
	enum TitleAlign
	{
		LEFT,
		CENTER,
		RIGHT
	};
public:
	AnkerTitledPanel(wxWindow* parent, int titleHeight = 50, int titleBorder = 5);
	~AnkerTitledPanel();

	void setTitle(wxString title);
	void setTitleAlign(TitleAlign align);
	int addTitleButton(wxString iconPath, bool beforeTitle = false);
	bool removeTitleButton(int id);
	void setTitleButtonVisible(int id, bool visible);
	void setTitleMoveObject(wxWindow* target);

	wxPanel* setContentPanel(wxPanel* content);
	wxPanel* getContentPanel() { return m_pContentPanel; }

private:
	void initUI();

	void OnButton(wxCommandEvent& event);

private:
	int m_titleHeight;
	int m_titleBorder;

	int m_titleBeforeBtnCount;
	int m_titleAfterBtnCount;
	wxStaticText* m_pTitleText;
	wxSizerItem* m_pTitleBeforeStretch;
	wxSizerItem* m_pTitleAfterStretch;
	std::vector<std::pair<wxButton*, bool>> m_pTitleBtnList;

	wxBoxSizer* m_pTitleSizer;
	wxBoxSizer* m_pMainSizer;

	wxPanel* m_pTitledPanel;
	wxPanel* m_pContentPanel;

	wxWindow* m_pMoveObject;
};

#endif // _ANKER_TITLED_PANEL_H_


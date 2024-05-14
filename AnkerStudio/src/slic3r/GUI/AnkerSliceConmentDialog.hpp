#ifndef ANKER_SLICE_CONMENT_H
#define ANKER_SLICE_CONMENT_H

#include "wx/wx.h"
#include "slic3r/GUI/I18N.hpp"
#include "GUI_App.hpp"
#include "Common/AnkerGUIConfig.hpp"
#include "AnkerBtn.hpp"
#include <wx/richtext/richtextctrl.h>
class AnkerBtn;

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_NOT_ASK, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_SUBMIT, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_CLOSE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_STAR_COUNTS_CHANGED, wxCommandEvent);
class AnkerConmentStar :public wxControl
{
public:
	AnkerConmentStar(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerConmentStar();
	int getStarCounts();
	void exitClearBtnImg();
    void resetStar();
protected:
	void initUi();
	void setBtnImg(AnkerBtn* pBtn, wxBitmap* Img);

private:
	wxPanel* m_pStarPanel{ nullptr };
	AnkerBtn* m_pStarFirstBtn{ nullptr };
	AnkerBtn* m_pStarSecondBtn{ nullptr };
	AnkerBtn* m_pStarThirdBtn{ nullptr };
	AnkerBtn* m_pStarFourthBtn{ nullptr };
	AnkerBtn* m_pStarFifthBtn{ nullptr };

	void onBtnClick(wxCommandEvent& event);

	wxBitmap* m_badStarBitmap { nullptr };
	wxBitmap* m_goodStarBitmap{ nullptr };
	int m_starCounts = 0;

};

class AnkerSliceConmentDialog : public wxDialog
{
public:
	AnkerSliceConmentDialog(wxWindow* parent = nullptr, wxString content = "");
	~AnkerSliceConmentDialog();

protected:
	void initUi();		
	void initEvent();
	void checkSubmitBtn();
private:	
	wxBoxSizer* m_pMainVSizer{ nullptr };	
	
	wxPanel* m_sumitPanel{ nullptr };

	wxString m_strSuggestion = "";
	wxRichTextCtrl* m_pConmentTextCtrl{ nullptr };
	wxStaticBitmap* m_start{ nullptr };
	AnkerBtn* m_pDonotAskBtn{ nullptr };
	AnkerBtn* m_pSubmitBtn{ nullptr };

	AnkerConmentStar* m_pStarPanel{ nullptr };

	wxPanel* m_finishedPanel{ nullptr };
	wxStaticBitmap* m_finishTipsImg{ nullptr };	
	
	AnkerBtn* m_pOkBtn{ nullptr };
	bool m_isUserClose = true;
};

#endif

#ifndef _ANKER_PRINT_FINISH_DIALOG_H_
#define _ANKER_PRINT_FINISH_DIALOG_H_

#include "wx/wx.h"
#include <boost/bind/bind.hpp>
#include <boost/signals2/connection.hpp>
#include "Common/AnkerBaseCtls.h"
#include "AnkerHyperlink.hpp"
#include "AnkerGCodeImportDialog.hpp"


class AnkerBtn;
class AnkerLoadingMask;
class AnkerTitledPanel;


class AnkerPrintFinishDialog : public wxDialog
{
public:
	enum DlgPanelType
	{
		PANEL_TYPE_NONE,
		PRINT_SUCCESS_PANEL,
		PRINT_FAIL_PANEL,
	};

	struct PrintFinishInfo
	{
		bool finishSuss = true;
		int m_currentLayer = 0;
		int m_totleLayer = 0;
		int m_timeSecond = 0;             // time uesed
		std::string m_fileName;
		std::string m_filePath;
		std::string m_filamentStr = "--";  // filament used, value + unit

		wxImage m_previewImage;
	};



public:
	AnkerPrintFinishDialog(std::string currentDeviceSn, wxWindow* parent);
	~AnkerPrintFinishDialog();


	void ShowPrintFinishDialog(bool finishSuss, AnkerPrintFinishDialog::PrintFinishInfo& result);
	void UpdateUI(bool finishSuss, AnkerPrintFinishDialog::PrintFinishInfo& result);

	// set current device sn
	void setCurrentDeviceSn(std::string sn) { m_currentDeviceSn = sn; }

	// use these after init
	//void setFileInfoSpeed(std::string str);
	//void setFileInfoFilament(std::string str);
	//void setFileInfoTime(int seconds);
	void setPreviewImage(int width, int height, unsigned char* data, unsigned char* alpha = nullptr, bool freeFlag = true);

private:
	void initUI();
	bool initPrintSuccessSizer(wxWindow* parent);
	bool initPrintFailSizer(wxWindow* parent);

	void Reset();

	void UpdateAllQuilityScoreBtns( );
	int GetBtnQuilityScore(wxBitmapButton* btn);

	PrintStopReasonInfo GetPrintStopReasonFromBtn(AnkerBtn* button);
	void UpdatePrintStopedReasonBtn();
	void ReportPrintResult();
	void UpdateFeedBackInfo();

	void OnClickQualityScoreBtn(wxCommandEvent& event);
	void OnEnterQualityScoreBtn(wxMouseEvent& event);
	void OnLeaveQualityScoreBtn(wxMouseEvent& event);

	void OnClickFeedbackHyperlink();
	void OnClickFeedbackLearnMore();


	void OnPrintStopedReasonBtnClick(wxCommandEvent& event);

	//  -------------------

	void OnPrintBtn(wxCommandEvent& event);
	void OnFinishBtn(wxCommandEvent& event);


private:
	std::string m_currentDeviceSn;
	wxColour m_dialogColor;
	wxColour m_textLightColor;
	wxColour m_textDarkColor;
	wxColour m_LineColor;


	DlgPanelType m_panelType{ PANEL_TYPE_NONE };
	AnkerTitledPanel* m_pTitledPanel{ nullptr };

	// success dialog 
	wxPanel* m_pPrintSuccessPanel { nullptr };

	wxStaticBitmap* m_pPreviewImg { nullptr };
	wxStaticText* m_pGcodeFileNameText { nullptr };
	wxStaticText* m_pPercentageText { nullptr };
	wxStaticText* m_pFinishTimeStampText { nullptr };

	wxStaticText* m_pPrintTimeValText { nullptr };
	wxStaticText* m_pFilamentValText { nullptr };

	AnkerBtn* m_pRePrintBtn { nullptr };
	AnkerBtn* m_pFinishBtn { nullptr };


	wxBitmap* m_bitmapUnmark { nullptr };
	wxBitmap* m_bitmapMark { nullptr };
	std::vector<wxBitmapButton*> m_QualityScoreBtns;
	int m_qualityScore{ 0 };




	// print fail dialog
	wxPanel* m_pPrintFailPanel { nullptr };

	wxBoxSizer* m_failStatusSizer { nullptr };
	wxStaticBitmap* m_pPreviewImg_fail{ nullptr };
	wxStaticText* m_pGcodeFileNameText_fail{ nullptr };
	wxStaticText* m_pPercentageText_fail { nullptr };
	wxStaticText* m_pFinishTimeStampText_fail { nullptr };

	wxStaticText* m_pPrintTimeValText_fail { nullptr };
	wxStaticText* m_pFilamentValText_fail { nullptr };

	AnkerHyperlink* m_FeedBackHyperLink { nullptr };
	wxBoxSizer* m_PrintingFailReasonSizer { nullptr };

	wxStaticText* m_lable_desc { nullptr };
	AnkerHyperlink* m_feedbackReadMore{ nullptr };
	wxBoxSizer* m_PrintingFailFeedbackSizer { nullptr };
	std::vector<std::pair<AnkerBtn*, PrintStopReasonInfo>>m_printStopReasonBtns;

	AnkerBtn* m_pRePrintBtn_fail;
	AnkerBtn* m_pFinishBtn_fail;

	PrintStopReasonInfo m_printStopReason;

};

#endif // _ANKER_PRINT_FINISH_DIALOG_H_


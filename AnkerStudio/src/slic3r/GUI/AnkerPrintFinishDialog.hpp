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

	enum PrintStopReason
	{
		STOP_REASON_NONE = -1,
		STOP_REASON_BOTTOM_LAYER_ADHESION = 0,
		STOP_REASON_SPAGHETTI_MESS,
		STOP_REASON_EXTRUDER_JAM,
		STOP_REASON_GIVE_UP,
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

	PrintStopReason GetPrintStopReasonFromBtn(AnkerBtn* button);
	void UpdatePrintStopedReasonBtn();
	void ReportPrintResult();



	void OnClickQualityScoreBtn(wxCommandEvent& event);
	void OnEnterQualityScoreBtn(wxMouseEvent& event);
	void OnLeaveQualityScoreBtn(wxMouseEvent& event);

	void OnClickFeedbackHyperlink();



	void OnPrintStopedReasonBtnClick(wxCommandEvent& event);

	//  -------------------

	void OnPrintBtn(wxCommandEvent& event);
	void OnFinishBtn(wxCommandEvent& event);


private:
	std::string m_currentDeviceSn;

	AnkerTitledPanel* m_pTitledPanel;
	wxColour m_dialogColor;
	wxColour m_textLightColor;
	wxColour m_textDarkColor;
	wxColour m_LineColor;

	DlgPanelType m_panelType{ PANEL_TYPE_NONE };

	// success dialog 
	wxPanel* m_pPrintSuccessPanel;

	wxStaticBitmap* m_pPreviewImg;
	wxStaticText* m_pGcodeFileNameText;
	wxStaticText* m_pPercentageText;
	wxStaticText* m_pFinishTimeStampText;

	wxStaticText* m_pPrintTimeValText;
	wxStaticText* m_pFilamentValText;

	AnkerBtn* m_pRePrintBtn;
	AnkerBtn* m_pFinishBtn;


	wxBitmap* m_bitmapUnmark;
	wxBitmap* m_bitmapMark;
	std::vector<wxBitmapButton*> m_QualityScoreBtns;
	int m_qualityScore{ 0 };




	// print fail dialog
	wxPanel* m_pPrintFailPanel;

	wxBoxSizer* m_failStatusSizer;
	wxStaticBitmap* m_pPreviewImg_fail;
	wxStaticText* m_pGcodeFileNameText_fail;
	wxStaticText* m_pPercentageText_fail;
	wxStaticText* m_pFinishTimeStampText_fail;

	wxStaticText* m_pPrintTimeValText_fail;
	wxStaticText* m_pFilamentValText_fail;

	AnkerHyperlink* m_FeedBackHyperLink;
	wxBoxSizer* m_PrintingFailReasonSizer;
	std::vector<AnkerBtn*> m_printStopReasonBtns;

	AnkerBtn* m_pRePrintBtn_fail;
	AnkerBtn* m_pFinishBtn_fail;

	PrintStopReason m_printStopReason = STOP_REASON_NONE;

};

#endif // _ANKER_PRINT_FINISH_DIALOG_H_


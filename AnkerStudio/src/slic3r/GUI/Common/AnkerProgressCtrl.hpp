#ifndef _ANKER_PROGRESS_CTRL_H_
#define _ANKER_PROGRESS_CTRL_H_

#include "wx/wx.h"


class AnkerProgressCtrl : public wxControl
{
public:
	AnkerProgressCtrl(wxWindow* parent, bool labelBack = false);
	~AnkerProgressCtrl();

	void setProgressColor(wxColour color) { m_progressColor = color; }
	void setUnProgressColor(wxColour color) { m_unProgressColor = color; }
	void SetBackgroundColor(wxColour color) { m_backgroudColor = color; }

	bool setProgressRange(double max, double min = 0.0);
	void updateProgress(double progress);
	double getProgressRange() { return m_rangeMax - m_rangeMin; }
	double getCurrentProgress() { return m_currentProgress; }

	// line length rate of the window
	void setLineLenRate(double rate);
	void setLineWidth(int width) { m_lineWidth = width; }
	void setMargin(int margin);

	void setLabelFont(wxFont font) { m_labelFont = font; }
	void setLabelVisible(bool visible) { m_labelVisible = visible; }

private:
	void initUI();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	int m_lineWidth;
	int m_margin;
	double m_lineLenRate;
	double m_progressedLineLenRate;

	int m_rangeMin;
	int m_rangeMax;
	int m_currentProgress;

	wxColour m_backgroudColor;
	wxColour m_progressColor;
	wxColour m_unProgressColor;

	bool m_labelVisible;
	bool m_labelBack;
	std::string m_labelStr;
	wxFont m_labelFont;
};

#endif // _ANKER_PROGRESS_CTRL_H_


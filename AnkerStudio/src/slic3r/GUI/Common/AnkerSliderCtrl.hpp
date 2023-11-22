#ifndef _ANKER_SLIDER_CTRL_H_
#define _ANKER_SLIDER_CTRL_H_

#include "wx/wx.h"


wxDECLARE_EVENT(wxANKEREVT_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxANKEREVT_SLIDERCTRL_VALUE_CHANGED, wxCommandEvent);

class AnkerSlider : public wxControl
{
public:
	AnkerSlider(wxWindow* parent);
	~AnkerSlider();

	bool setRange(double min, double max, double step);
	bool setRangeMin(double min) { return setRange(min, m_valueRangeMax, m_stepValue); }
	bool setRangeMax(double max) { return setRange(m_valueRangeMin, max, m_stepValue); }
	double getRangeMin() { return m_valueRangeMin; }
	double getRangeMax() { return m_valueRangeMax; }

	bool setStepValue(double step) { return setRange(m_valueRangeMin, m_valueRangeMax, step); }
	double getStepValue() { return m_stepValue; }

	bool setCurrentValue(double value);
	double getCurrentValue() { return m_stepValue * m_currentStepCount + m_valueRangeMin; }
	int getCurrentStepCount() { return m_currentStepCount; }

	// line length rate of the window
	void setLineWidth(int width) { m_lineWidth = width; }
	void setHandleRadius(int radius);
	void setHandleHighlightOffset(int offset);
	void setMargin(int margin);

	void setLineHighlightLeft(bool left) { m_lineHighlightLeft = left; }
	void setHandleHighlight(bool enable) { m_handleHighlight = enable; }
	void setLineBgColor(wxColour color) { m_lineBgColor = color; }
	void setLineHighlightColor(wxColour color) { m_lineHighlightColor = color; }
	void setHandleFgColor(wxColour color) { m_handleFgColor = color; }
	void setHandleHighlightColor(wxColour color) { m_handleHighlightColor = color; }

	void setTooltipCFormatStr(wxString str) { m_tooltipCFormatStr = str; }
	void setTooltipFont(wxFont font) { m_tooltipFont = font; }
	void setTooltipVisible(bool visible) { m_tooltipVisible = visible; }
	void setTooltipBackgroundColor(wxColour color) { m_tooltipBgColor = color; }
	void setTooltipForegroundColor(wxColour color) { m_tooltipFgColor = color; }
	void setTooltipBorderColor(wxColour color) { m_tooltipBorderColor = color; }

private:
	void initUI();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	int getRealLineLen();
	void updateCurrentHandleX(int newX);

private:
	bool m_mouseLeftDown;
	double m_valueRangeMin;
	double m_valueRangeMax;
	double m_stepValue;
	int m_stepRangeMin;
	int m_stepRangeMax;
	int  m_currentStepCount;
	int m_currentHandleX;

	int m_lineWidth;
	int m_margin;
	int m_handleRadius;
	int m_handleHighlightOffset;
	int m_lineCornerRadius;

	bool m_lineHighlightLeft;
	bool m_handleHighlight;
	wxColour m_lineBgColor;
	wxColour m_lineHighlightColor;
	wxColour m_handleFgColor;
	wxColour m_handleHighlightColor;
	wxColour m_hoverColor;

	bool m_tooltipVisible;
	bool m_mouseHovering;
	wxString m_tooltipCFormatStr;
	wxFont m_tooltipFont;
	wxColour m_tooltipBgColor;
	wxColour m_tooltipFgColor;
	wxColour m_tooltipBorderColor;
};

#endif // _ANKER_SLIDER_CTRL_H_


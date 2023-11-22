#include "AnkerSliderCtrl.hpp"
#include "AnkerSliderCtrl.hpp"
#include "AnkerGUIConfig.hpp"


wxDEFINE_EVENT(wxANKEREVT_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxANKEREVT_SLIDERCTRL_VALUE_CHANGED, wxCommandEvent);

AnkerSlider::AnkerSlider(wxWindow* parent)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
    , m_mouseLeftDown(false)
    , m_valueRangeMin(0.0)
    , m_valueRangeMax(1.0)
    , m_stepValue(0.1)
    , m_stepRangeMin(0)
    , m_stepRangeMax(10)
    , m_currentStepCount(0)
    , m_currentHandleX(0)
    , m_lineWidth(5)
    , m_margin(10)
    , m_handleRadius(6)
    , m_handleHighlightOffset(3)
    , m_lineCornerRadius(2)
    , m_lineHighlightLeft(true)
    , m_handleHighlight(true)
    , m_lineBgColor(wxColour(76, 77, 79))
    , m_lineHighlightColor(wxColour(ANKER_RGB_INT))
    , m_handleFgColor(wxColour("#C0FFD0"))
    , m_handleHighlightColor(wxColour(ANKER_RGB_INT))
    , m_mouseHovering(false)
    , m_tooltipVisible(true)
    , m_tooltipCFormatStr("%.2f")
    , m_tooltipFont(ANKER_BOLD_FONT_SIZE(8))
    , m_tooltipBgColor(wxColour(30, 30, 30))
    , m_tooltipFgColor(wxColour(180, 180, 180))
    , m_tooltipBorderColor(wxColour(180, 180, 180))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    initUI();

    Bind(wxEVT_PAINT, &AnkerSlider::OnPaint, this);
    Bind(wxEVT_SIZE, &AnkerSlider::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerSlider::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &AnkerSlider::OnLeftUp, this);
    Bind(wxEVT_MOTION, &AnkerSlider::OnMouseMove, this);

    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) { m_mouseHovering = true; SetCursor(wxCursor(wxCURSOR_HAND)); Refresh(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) { m_mouseHovering = false; m_mouseLeftDown = false; SetCursor(wxCursor(wxCURSOR_NONE)); Refresh(); });
}

AnkerSlider::~AnkerSlider()
{
}

void AnkerSlider::initUI()
{
}

bool AnkerSlider::setRange(double min, double max, double step)
{
    if (max <= min || step >= max - min)
        return false;

    m_valueRangeMin = min;
    m_valueRangeMax = max;
    m_stepValue = step;

    m_stepRangeMin = 0;
    m_stepRangeMax = std::ceil((m_valueRangeMax - m_valueRangeMin) / m_stepValue);

    double currentValue = m_currentStepCount * m_stepValue;
    currentValue = std::max(std::min(currentValue, m_valueRangeMax), m_valueRangeMin);
    m_currentStepCount = std::ceil((currentValue - m_valueRangeMin) / m_stepValue);

    m_currentHandleX = m_margin + m_currentStepCount * 1.0 / m_stepRangeMax * getRealLineLen();

    return true;
}

bool AnkerSlider::setCurrentValue(double value)
{
    if (value < m_valueRangeMin || value > m_valueRangeMax)
        return false;

    m_currentStepCount = std::ceil((value - m_valueRangeMin) / m_stepValue);

    m_currentHandleX = m_margin + m_currentStepCount * 1.0 / m_stepRangeMax * getRealLineLen();

    return true;
}

void AnkerSlider::setHandleRadius(int radius)
{
    if (radius > 0)
    {
        m_handleRadius = radius;
        if (m_margin < m_handleRadius + m_handleHighlightOffset)
            m_margin = m_handleRadius + m_handleHighlightOffset;
    }
}

void AnkerSlider::setHandleHighlightOffset(int offset)
{
    if (offset > 0)
    {
        m_handleHighlightOffset = offset;
        if (m_margin < m_handleRadius + m_handleHighlightOffset)
            m_margin = m_handleRadius + m_handleHighlightOffset;
    }
}

void AnkerSlider::setMargin(int margin)
{
    if (margin <= 0 || margin == m_margin)
        return;

    m_margin = std::min(margin, m_handleRadius + m_handleHighlightOffset);
}

void AnkerSlider::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.Clear();

    wxRect rect = GetClientRect();
    rect.width += 1;
    rect.height += 1;

	wxBrush brush(wxColour(PANEL_BACK_RGB_INT));
	wxPen pen(wxColour(PANEL_BACK_RGB_INT));
	dc.SetBrush(brush);
	dc.SetPen(pen);
	dc.DrawRectangle(rect);

    int lineLen = getRealLineLen();

    if (lineLen > 0)
    {
        int centerY = GetSize().y / 2;
        int radius = m_lineCornerRadius;

        // draw bg line
        dc.SetBrush(wxBrush(m_lineBgColor));
        dc.SetPen(wxPen(m_lineBgColor));
        dc.DrawRoundedRectangle(m_margin, centerY, lineLen, m_lineWidth, radius);

        //  draw highlight line
        dc.SetBrush(wxBrush(m_lineHighlightColor));
        dc.SetPen(wxPen(m_lineHighlightColor));
        if (m_lineHighlightLeft)
            dc.DrawRoundedRectangle(m_margin, centerY, m_currentHandleX - m_margin, m_lineWidth, radius);
        else
            dc.DrawRoundedRectangle(m_currentHandleX, centerY, lineLen - m_currentHandleX + m_margin, m_lineWidth, radius);
    }

    // draw slider handler
    {
        int centerY = GetSize().y / 2;
        int radius = m_handleRadius;
        int highlightRadius = m_handleRadius + m_handleHighlightOffset;

        // draw highlight handle
        if (m_handleHighlight)
        {
            dc.SetBrush(wxBrush(m_handleHighlightColor));
            dc.SetPen(wxPen(m_handleHighlightColor));
            dc.DrawCircle(m_currentHandleX, centerY, highlightRadius);
        }

        //  draw foreground handle
        dc.SetBrush(wxBrush(m_handleFgColor));
        dc.SetPen(wxPen(m_handleFgColor));
        dc.DrawCircle(m_currentHandleX, centerY, radius);
    }

    // draw tooltip
    if (m_tooltipVisible && m_mouseHovering)
    {
        double currentValue = getCurrentValue();
        wxString tooltipStr = "";
        tooltipStr.Printf(m_tooltipCFormatStr, currentValue);

        dc.SetFont(m_tooltipFont);
        wxSize textSize = dc.GetTextExtent(tooltipStr);
        wxRect tooltipRect(m_currentHandleX + m_handleRadius + 2, 1, textSize.GetWidth(), textSize.GetHeight());

        //// draw box
        //dc.SetBrush(wxBrush(m_tooltipBgColor));
        //dc.SetPen(wxPen(m_tooltipBorderColor));
        //dc.DrawRectangle(tooltipRect);

        // draw text
        dc.SetBrush(wxBrush(m_handleFgColor));
        dc.SetPen(wxPen(m_handleFgColor));
		dc.SetTextForeground(m_handleFgColor);
		dc.DrawText(tooltipStr, tooltipRect.GetPosition());
    }
}

void AnkerSlider::OnSize(wxSizeEvent& event)
{
    m_currentHandleX = m_margin + m_currentStepCount * 1.0 / m_stepRangeMax * getRealLineLen();
}

void AnkerSlider::OnLeftDown(wxMouseEvent& event)
{
    m_mouseLeftDown = true;

    int mouseX = 0, mouseY = 0;
    event.GetPosition(&mouseX, &mouseY);

    updateCurrentHandleX(mouseX);

    Refresh();

    wxCommandEvent evt = wxCommandEvent(wxANKEREVT_SLIDER_VALUE_CHANGED);
    wxVariant eventData;
    eventData.ClearList();
    eventData.Append(wxVariant(getCurrentValue()));
    evt.SetClientData(new wxVariant(eventData));
    ProcessEvent(evt);
}

void AnkerSlider::OnLeftUp(wxMouseEvent& event)
{
    m_mouseLeftDown = false;
}

void AnkerSlider::updateCurrentHandleX(int newX)
{
    int lineLen = getRealLineLen();
    double stepLen = lineLen * 1.0 / m_stepRangeMax;
    m_currentStepCount = std::min(m_stepRangeMax, std::max(m_stepRangeMin, int(std::round((newX - m_margin) / stepLen))));
    m_currentHandleX = m_currentStepCount * stepLen + m_margin;
}

int AnkerSlider::getRealLineLen()
{
    int realLineLen =  GetSize().GetWidth() - 2 * m_margin;
    return realLineLen;
}

void AnkerSlider::OnMouseMove(wxMouseEvent& event)
{
    if (m_mouseLeftDown)
    {
        int mouseX = 0, mouseY = 0;
        event.GetPosition(&mouseX, &mouseY);

        updateCurrentHandleX(mouseX);

        Refresh();

        wxCommandEvent evt = wxCommandEvent(wxANKEREVT_SLIDER_VALUE_CHANGED);
        wxVariant eventData;
        eventData.ClearList();
        eventData.Append(wxVariant(getCurrentValue()));
        evt.SetClientData(new wxVariant(eventData));
        ProcessEvent(evt);
    }
}


#include "AnkerProgressCtrl.hpp"
#include "AnkerGUIConfig.hpp"


AnkerProgressCtrl::AnkerProgressCtrl(wxWindow* parent)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_lineLenRate(0.98)
    , m_lineWidth(3)
    , m_margin(1)
    , m_progressedLineLenRate(0)
    , m_rangeMin(0)
    , m_rangeMax(700)
    , m_currentProgress(0)
    , m_progressColor(ANKER_RGB_INT)
    , m_unProgressColor(m_progressColor.ChangeLightness(51))
    , m_labelVisible(true)
    , m_labelStr("0%")
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &AnkerProgressCtrl::OnPaint, this);
    Bind(wxEVT_SIZE, &AnkerProgressCtrl::OnSize, this);
}

AnkerProgressCtrl::~AnkerProgressCtrl()
{
}

bool AnkerProgressCtrl::setProgressRange(double max, double min)
{
    if (max <= min)
        return false;

    m_rangeMax = max;
    m_rangeMin = min;

    m_currentProgress = std::max(std::min(m_currentProgress, m_rangeMax), m_rangeMin);
    updateProgress(m_currentProgress);

    return true;
}

void AnkerProgressCtrl::updateProgress(double progress)
{
    if (progress != m_currentProgress && progress >= m_rangeMin && progress <= m_rangeMax)
    {
        m_currentProgress = progress;

        m_progressedLineLenRate = m_currentProgress * 1.0 / (m_rangeMax - m_rangeMin);
        m_labelStr = std::to_string((int)(m_progressedLineLenRate * 100)) + "%";
    }
}

void AnkerProgressCtrl::setLineLenRate(double rate)
{
    if (rate <= 0 || rate == m_lineLenRate)
        return;

    m_lineLenRate = rate;
}

void AnkerProgressCtrl::setMargin(int margin)
{
    if (margin < 0 || margin == m_margin)
        return;

    m_margin = margin;
}

void AnkerProgressCtrl::initUI()
{
}

void AnkerProgressCtrl::OnPaint(wxPaintEvent& event)
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

    int winLen = GetSize().x;
    int lineLen = winLen * m_lineLenRate;
    int progressedLineLen = winLen * m_lineLenRate * m_progressedLineLenRate;

    if (m_labelVisible)
    {
        wxBrush brush(m_progressColor);
        wxPen pen(m_progressColor);
        dc.SetBrush(brush);
        dc.SetPen(pen);
        dc.SetFont(m_labelFont);
        dc.SetTextForeground(m_progressColor);
        wxPoint textPoint = wxPoint(lineLen - m_labelFont.GetPointSize() * 3, 0);
        dc.DrawText(m_labelStr, textPoint);
    }

    if (lineLen > 0 && m_margin >= 0)
    {
        int centerY = GetSize().y / 2;
        int radius = 2;

        // draw unProgress line
        dc.SetBrush(wxBrush(m_unProgressColor));
        dc.SetPen(wxPen(m_unProgressColor));
        //dc.SetTextForeground(m_unProgressColor);
        dc.DrawRoundedRectangle(m_margin, GetSize().y - m_lineWidth, lineLen, m_lineWidth, radius);

        //  draw progress line
        dc.SetBrush(wxBrush(m_progressColor));
        dc.SetPen(wxPen(m_progressColor));
        //dc.SetTextForeground(m_progressColor);
        dc.DrawRoundedRectangle(m_margin, GetSize().y - m_lineWidth, progressedLineLen, m_lineWidth, radius);
    }
}

void AnkerProgressCtrl::OnSize(wxSizeEvent& event)
{
    //Layout();
    Update();
}

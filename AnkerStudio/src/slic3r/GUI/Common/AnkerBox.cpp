#include "AnkerBox.hpp"
#include "../wxExtensions.hpp"
#include "libslic3r/utils.hpp"


AnkerBox::AnkerBox()
{
}

AnkerBox::AnkerBox(wxWindow* parent, wxWindowID winid, const wxPoint& pos,
    const wxSize& size, long style, const wxString& name)  : 
    wxPanel(parent, winid, pos, size, style, name)
{
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(wxColour("#292A2D"));
    }
    //borderHighEnable(); 
}

AnkerBox::~AnkerBox()
{
}

wxSize AnkerBox::getTextSize(wxWindow* window, const wxString& text)
{
    wxClientDC dc(window);
    return dc.GetTextExtent(text);
}

void AnkerBox::setStrWrap(wxStaticText* context, int width, const wxString& text, int language)
{
    if (language == wxLanguage::wxLANGUAGE_CHINESE_CHINA ||
        language == wxLanguage::wxLANGUAGE_JAPANESE_JAPAN || 
        language == wxLanguage::wxLANGUAGE_JAPANESE) {
        wxString wrapStr = "";
        wxClientDC dc(context);
        int tmpWidth = 0;
        for (size_t i = 0; i < text.length(); i++) {
            wxSize tmpSize = dc.GetTextExtent(text[i]);
            tmpWidth += tmpSize.x;     
            if (tmpWidth < width) {
                wrapStr += text[i];
            }
            else {
                wrapStr += '\n';
                wrapStr += text[i];
                tmpWidth = 1;
            }
        }
        context->SetLabel(wrapStr);
    }
    else {
        context->SetLabel(text);
        context->Wrap(width);
    }
}

void AnkerBox::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();       
    wxColour bkColour(GetBackgroundColour());
    dc.Clear();
    dc.SetBackground(wxBrush(bkColour));

    wxColour colour;
    if (mouseEnter) {
        colour = m_mouseEnterColour;
    }
    else {
        colour = m_mouseLeaveColour;
    }
    wxPen borderPen(colour, 2, wxPENSTYLE_SOLID);

    dc.SetPen(borderPen);
    dc.SetBrush(bkColour);
    dc.DrawRectangle(rect);
    event.Skip();
}

void AnkerBox::OnMouseEnter(wxMouseEvent& event)
{
    setMouseEnterStatus(true);
    event.Skip();
}

void AnkerBox::OnMouseLeave(wxMouseEvent& event)
{
    setMouseEnterStatus(false);
    event.Skip();
}

void AnkerBox::setBorderColour(const wxColour& enterColour, const wxColour& leaveColour)
{
    m_mouseEnterColour = enterColour;
    m_mouseLeaveColour = leaveColour;
}

void AnkerBox::borderHighEnable(bool enable)
{
    if (enable) {
        Bind(wxEVT_ENTER_WINDOW, &AnkerBox::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &AnkerBox::OnMouseLeave, this);
        Bind(wxEVT_PAINT, &AnkerBox::OnPaint, this);
    }
    else {
        Unbind(wxEVT_ENTER_WINDOW, &AnkerBox::OnMouseEnter, this);
        Unbind(wxEVT_LEAVE_WINDOW, &AnkerBox::OnMouseLeave, this);
        Unbind(wxEVT_PAINT, &AnkerBox::OnPaint, this);
    }
    setBorderColour();
    setTextCtrlFont();
    setTextForegroundColour();
}

void AnkerBox::setTextCtrlFont(const wxFont& font)
{
    SetFont(font);
}

void AnkerBox::setTextForegroundColour(const wxColour& colour)
{
    SetForegroundColour(colour);
}

void AnkerBox::setBackgroundColour(const wxColour& colour)
{
    SetBackgroundColour(colour);
}

void AnkerBox::setMouseEnterStatus(bool enter)
{
    mouseEnter = enter;
    Update();
    Refresh();
}

// mod by allen for filament hover
AnkerHighlightPanel::AnkerHighlightPanel()
{
}

AnkerHighlightPanel::AnkerHighlightPanel(wxWindow* parent, wxWindowID winid, const wxPoint& pos,
    const wxSize& size, long style, const wxString& name) :
    wxPanel(parent, winid, pos, size, style, name)
{
  /*  Bind(wxEVT_ENTER_WINDOW, &AnkerHighlightPanel::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &AnkerHighlightPanel::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerHighlightPanel::OnLeftDown, this);*/
}

AnkerHighlightPanel::~AnkerHighlightPanel()
{
}

void AnkerHighlightPanel::setBackgroundColour(const wxColour& colour)
{
    m_mouseLeaveColour = colour;
    SetBackgroundColour(colour);
}


void AnkerHighlightPanel::OnMouseEnter(wxMouseEvent& event)
{
    //ANKER_LOG_INFO << "AnkerHighlightPanel OnMouseEnter";
    setMouseEnterStatus(true);
    event.Skip();
}

void AnkerHighlightPanel::OnMouseLeave(wxMouseEvent& event)
{
    //ANKER_LOG_INFO << "AnkerHighlightPanel OnMouseLeave";
    setMouseEnterStatus(false);
    event.Skip();
}

void AnkerHighlightPanel::OnLeftDown(wxMouseEvent& event)
{
    m_bSelectedHighlight = !m_bSelectedHighlight;
    m_bSelectedHighlight ? EnableHighlight() : DisableHighlight();
}

void AnkerHighlightPanel::setMouseEnterStatus(bool enter)
{
    // if one of siblings is clicked, return and ignore it
    wxWindowList& siblings = GetParent()->GetChildren();
    for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
    {
        AnkerHighlightPanel* siblingPanel = dynamic_cast<AnkerHighlightPanel*>(*it);
        if (siblingPanel && siblingPanel != this) {
            if (siblingPanel->m_bSelectedHighlight) {
                return;
            }
        }
    }

    (enter || m_bSelectedHighlight) ? EnableHighlight() : DisableHighlight();
    Refresh();
}

void AnkerHighlightPanel::setMouseClickStatus() {
    // update self click status
    m_bSelectedHighlight = !m_bSelectedHighlight;
    m_bSelectedHighlight ? EnableHighlight() : DisableHighlight();
    Refresh();
}

bool AnkerHighlightPanel::getMouseClickStatus() {
    return m_bSelectedHighlight;
}
void AnkerHighlightPanel::EnableHighlight() {
    setBackgroundColour(wxColour("#506853"));
    wxWindowList children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        if (ScalableButton* btn = dynamic_cast<ScalableButton*>(child)) {
            btn->SetBackgroundColour(wxColour("#506853"));
        }
    }

    wxWindowList& siblings = GetParent()->GetChildren();
    for (wxWindowList::iterator it = siblings.begin(); it != siblings.end(); ++it)
    {
        AnkerHighlightPanel* siblingPanel = dynamic_cast<AnkerHighlightPanel*>(*it);
        if (siblingPanel && siblingPanel != this)
        {
            if (siblingPanel->m_bSelectedHighlight) {
                siblingPanel->m_bSelectedHighlight = !siblingPanel->m_bSelectedHighlight;
            }
            siblingPanel->setBackgroundColour(wxColour("#292A2D")); 
            wxWindowList children = siblingPanel->GetChildren();
            for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
                wxWindow* child = *it;
                if (ScalableButton* btn = dynamic_cast<ScalableButton*>(child)) {
                    btn->SetBackgroundColour(wxColour("#292A2D"));
                }
            }
            siblingPanel->Refresh();
        }
    }
}

void AnkerHighlightPanel::DisableHighlight() {
    setBackgroundColour(wxColour("#292A2D"));
    wxWindowList children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        if (ScalableButton* btn = dynamic_cast<ScalableButton*>(child)) {
            btn->SetBackgroundColour(wxColour("#292A2D"));
        }
    }
}
#ifndef ANKER_BOX_HPP
#define ANKER_BOX_HPP
#include <wx/wx.h>
#include <wx/panel.h>
#include "AnkerBase.hpp"


class AnkerBox : public wxPanel, public AnkerBase
{
public:
    AnkerBox();
	AnkerBox(wxWindow* parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL | wxNO_BORDER,
        const wxString& name = wxASCII_STR(wxPanelNameStr));
	~AnkerBox();

    static wxSize getTextSize(wxWindow* window, const wxString& text);
    static void setStrWrap(wxStaticText* context, int width, const wxString& text, int language);

    virtual void borderHighEnable(bool enable = true);
    virtual void setTextCtrlFont(const wxFont& font = ANKER_FONT_NO_1);
    virtual void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    virtual void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    virtual void setBorderColour(const wxColour& enterColour = wxColour(0, 255, 0), const wxColour& leaveColour = wxColour("#FFFFFF"));
    
protected:
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnMouseEnter(wxMouseEvent& event);
    virtual void OnMouseLeave(wxMouseEvent& event);
    void setMouseEnterStatus(bool enter = true);
    
    bool mouseEnter = false;
    wxColour m_mouseEnterColour;
    wxColour m_mouseLeaveColour;

};

// mod by allen for filament hover
class AnkerHighlightPanel : public wxPanel
{
public:
    AnkerHighlightPanel();
    AnkerHighlightPanel(wxWindow* parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL | wxNO_BORDER,
        const wxString& name = wxASCII_STR(wxPanelNameStr));
    ~AnkerHighlightPanel();

    virtual void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    void setMouseEnterStatus(bool enter = true);
    void setMouseClickStatus();
    bool getMouseClickStatus();
    /* static wxSize getTextSize(wxWindow* window, const wxString& text);
     virtual void borderHighEnable(bool enable = true);
     virtual void setTextCtrlFont(const wxFont& font = _AnkerFont(14));
     virtual void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
     virtual void setBorderColour(const wxColour& enterColour = wxColour(0, 255, 0), const wxColour& leaveColour = wxColour("#FFFFFF"));*/

protected:
    //virtual void OnPaint(wxPaintEvent& event);
    virtual void OnMouseEnter(wxMouseEvent& event);
    virtual void OnMouseLeave(wxMouseEvent& event);
    virtual void OnLeftDown(wxMouseEvent& event);
private:
    void EnableHighlight();
    void DisableHighlight();

    std::atomic<bool> m_bSelectedHighlight { false };
    wxColour m_mouseEnterColour;
    wxColour m_mouseLeaveColour;

};

#endif // !ANKER_BOX_HPP

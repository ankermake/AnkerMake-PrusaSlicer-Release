#ifndef ANKER_STATIC_TEXT_HPP_
#define ANKER_STATIC_TEXT_HPP_

#include <wx/richtext/richtextctrl.h>
#include <wx/wx.h> 
class AnkerStaticText : public wxStaticText
{
public:
    AnkerStaticText();
	AnkerStaticText(wxWindow* parent,
        wxWindowID id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxASCII_STR(wxStaticTextNameStr));
	~AnkerStaticText();

private:

};

class MultipleLinesStaticText : public wxRichTextCtrl
{
public:
    MultipleLinesStaticText(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTE_MULTILINE | wxBORDER_NONE);
    ~MultipleLinesStaticText();

public:
    void SetText(const wxString& text, wxTextAttrAlignment align = wxTEXT_ALIGNMENT_DEFAULT);
    void SetTextFont(const wxFont& font);

    void ChangeTextToLink(const wxString& text, const wxString& textToUrl, const wxColour& linkColor = *wxBLUE, bool isShowUnderLine = false, bool isHighLight = false);
    void AppendLinkText(const wxString& url, const wxString& linkText, const wxColour& linkColor = *wxBLUE, bool isShowUnderLine = false);
public:
    void SetControlColour(const wxColour& foregroundColour, const wxColour& backgroundColour);
    void SetControlSize(const wxSize& size);

public:
    wxString GetText() { return wxRichTextCtrl::GetValue(); }

    //usage:
    // MultipleLinesStaticText* richTextCtrl = new MultipleLinesStaticText(this);
    // auto attr = richTextCtrl->GetControlAttr();
    // attr.textColour, ...
    auto GetControlAttr();

private:
    void OnPaint(wxPaintEvent& event);
    void OnLinkClicked(wxCommandEvent& event);

private:
    wxFont mFont;
    wxSize mSize;

    wxColour mTextColour;
    wxColour mBackgroundColour;
    wxColour mForegroundColour;

    std::string mUrl;
};





#endif // !ANKER_STATIC_TEXT_HPP_

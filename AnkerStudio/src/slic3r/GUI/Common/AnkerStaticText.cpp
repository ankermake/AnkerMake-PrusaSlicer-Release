#include "AnkerStaticText.hpp"

AnkerStaticText::AnkerStaticText()
{
}

AnkerStaticText::AnkerStaticText(
    wxWindow* parent, wxWindowID id, const wxString& label,
    const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
    wxStaticText(parent, id, label, pos, size, style, name)
{
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
    }
    else {
        SetBackgroundColour(wxColour("#292A2D"));
    }
}

AnkerStaticText::~AnkerStaticText()
{
}


MultipleLinesStaticText::MultipleLinesStaticText(wxWindow* parent, wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style)
    : wxRichTextCtrl(parent, id, wxEmptyString, pos, size, style)
{
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    Bind(wxEVT_SET_FOCUS, [](wxFocusEvent& event) {
        // remove cursor focus in word's head
        });

    Unbind(wxEVT_PAINT, &wxRichTextCtrl::OnPaint, this);
    Bind(wxEVT_PAINT, &MultipleLinesStaticText::OnPaint, this);
    Bind(wxEVT_TEXT_URL, &MultipleLinesStaticText::OnLinkClicked, this);
}

MultipleLinesStaticText::~MultipleLinesStaticText()
{ 
    Unbind(wxEVT_PAINT, &MultipleLinesStaticText::OnPaint, this); 
}

void MultipleLinesStaticText::SetText(const wxString& text, wxTextAttrAlignment align)
{
    BeginAlignment(align);
    SetValue(text);
    EndAlignment();
}

void MultipleLinesStaticText::SetTextFont(const wxFont& font)
{
    wxTextAttr textAttr;
    mFont = font;
    textAttr.SetFont(mFont);
    SetDefaultStyle(textAttr);
}

void MultipleLinesStaticText::ChangeTextToLink(const wxString& text, const wxString& textToUrl, const wxColour& linkColor, bool isShowUnderLine, bool isHighLight)
{
    long start = GetValue().Find(text);
    if (start != wxNOT_FOUND){
        long end = start + wxString(text).Length();
        if (isHighLight) {
            SetSelection(start, end);
        }
        mUrl = textToUrl.ToStdString();
        wxRichTextAttr hyperlinkAttr;
        hyperlinkAttr.SetURL(textToUrl);
        hyperlinkAttr.SetTextColour(linkColor);
        hyperlinkAttr.SetFontUnderlined(isShowUnderLine);

        SetStyle(start, end, hyperlinkAttr);
    }
}

void MultipleLinesStaticText::AppendLinkText(const wxString& url, const wxString& linkText, const wxColour& linkColor, bool isShowUnderLine)
{
    wxRichTextAttr linkAttr;
    linkAttr.SetTextColour(linkColor);
    linkAttr.SetFontUnderlined(isShowUnderLine);
    BeginStyle(linkAttr);
    mUrl = url.ToStdString();
    BeginURL(url);
    WriteText(linkText);
    EndURL();
    EndStyle();
}

void MultipleLinesStaticText::SetControlColour(const wxColour& foregroundColour, const wxColour& backgroundColour)
{
    mTextColour = foregroundColour;
    mForegroundColour = foregroundColour;
    mBackgroundColour = backgroundColour;
}

void MultipleLinesStaticText::SetControlSize(const wxSize& size)
{
    SetMinSize(size);
    SetSize(size);
    SetMaxSize(size);
}

auto MultipleLinesStaticText::GetControlAttr()
{
    struct ControlAttr {
        wxFont font;
        wxSize size;
        wxColour textColour;
        wxColour backgroundColour;
        wxColour foregroundColour;
    };

    return ControlAttr{ mFont, mSize, mTextColour, mBackgroundColour, mForegroundColour };
}

void MultipleLinesStaticText::OnPaint(wxPaintEvent& event)
{
    wxRichTextAttr attr;
    attr.SetTextColour(mTextColour);
    SetBasicStyle(attr);

    wxRichTextCtrl::SetForegroundColour(mForegroundColour);
    wxRichTextCtrl::SetBackgroundColour(mBackgroundColour);

    wxRichTextCtrl::OnPaint(event);
}

void MultipleLinesStaticText::OnLinkClicked(wxCommandEvent& event)
{
    wxTextUrlEvent& urlEvent = static_cast<wxTextUrlEvent&>(event);
    wxRichTextCtrl* richTextCtrl = dynamic_cast<wxRichTextCtrl*>(urlEvent.GetEventObject());
    if (richTextCtrl) {
        if (!mUrl.empty()) {
            wxLaunchDefaultBrowser(mUrl);
        }
    }
}
#include "AnkerHyperlink.hpp"
#include "GUI_App.hpp"

BEGIN_EVENT_TABLE(AnkerHyperlink, wxControl)
EVT_LEFT_DOWN(AnkerHyperlink::OnClick)
EVT_PAINT(AnkerHyperlink::OnPaint)
EVT_LEFT_DOWN(AnkerHyperlink::OnPressed)
EVT_ENTER_WINDOW(AnkerHyperlink::OnEnter)
EVT_LEAVE_WINDOW(AnkerHyperlink::OnLeave)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerHyperlink, wxControl)

AnkerHyperlink::AnkerHyperlink(wxWindow* parent,
	wxWindowID winid /*= wxID_ANY*/,
	const wxString& name /*= wxString("")*/,
	const wxString& urlLink /*= wxString("")*/,
	const wxColour& backgroudColor /*= wxColour("#1F2022")*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	const TextAlignType& align /*= ALIGN_LEFT*/
)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	m_text = name;
	m_link = urlLink;
	m_alignType = align;
	SetBackgroundColour(backgroudColor);
}

AnkerHyperlink::AnkerHyperlink()
{

}

void AnkerHyperlink::SetCustumAction(CustomActionFun f)
{
	CustomAction = f;
}

void AnkerHyperlink::SetCustomFont(const wxFont& font)
{
	m_font = font;
}

void AnkerHyperlink::SetWrapWidth(int w)
{
	m_wrapWidth = w;
}

void AnkerHyperlink::OnEnter(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_HAND));
}

void AnkerHyperlink::OnLeave(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_NONE));
}

void AnkerHyperlink::OnPressed(wxMouseEvent& event)
{
	if (!m_link.IsEmpty())
		wxLaunchDefaultBrowser(m_link);
	else if (CustomAction)
		CustomAction();
}

void AnkerHyperlink::OnClick(wxMouseEvent& event)
{
	if (!m_link.IsEmpty())
		wxLaunchDefaultBrowser(m_link);
	else if (CustomAction)
		CustomAction();
}

wxString GetNextLineNonEnglishStr(wxString& text, wxDC& dc, int maxWidth) {
	wxString line;
	for (wxString::const_iterator it = text.begin(); it != text.end(); ++it) {
		line += *it;
		wxCoord width, height;
		dc.GetTextExtent(line, &width, &height);
		if (width > maxWidth) {
			line.RemoveLast();
			text = text.Mid(line.length()).Trim();
			return line;
		}
	}
	text.clear();
	return line;
}

wxString GetNextLineEnglishStr(wxString& text, wxDC& dc, int maxWidth) {
	wxString line;
	wxString space = " ";
	wxCoord spaceWidth, height;
	dc.GetTextExtent(space, &spaceWidth, &height);

	wxCoord width = 0;
	size_t lastSpace = wxString::npos;

	for (size_t i = 0; i < text.length(); ++i) {
		width += dc.GetTextExtent(text[i]).GetWidth();

		if (text[i] == ' ') {
			lastSpace = i;
		}

		if (width > maxWidth) {
			if (lastSpace != wxString::npos) {
				line = text.Mid(0, lastSpace);
				text = text.Mid(lastSpace + 1).Trim();
			}
			else {
				line = text.Mid(0, i);
				text = text.Mid(i).Trim();
			}
			return line;
		}
	}

	line = text;
	text.clear();
	return line;
}

void AnkerHyperlink::drawWrapText(wxPaintDC &dc, wxString& text, int wrapWidth)
{
	wxFont font = dc.GetFont();
	wxCoord lineHeight = dc.GetMultiLineTextExtent("Test").GetHeight();

	wxString remainingText = text;
	int y = 0;
	while (!remainingText.empty()) {
		wxString line;

		int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
		if ( wxLanguage::wxLANGUAGE_ENGLISH == type || wxLanguage::wxLANGUAGE_ENGLISH_US == type) {
			line = GetNextLineEnglishStr(remainingText, dc, wrapWidth);
		}
		else{
			line = GetNextLineNonEnglishStr(remainingText, dc, wrapWidth);
		}

		dc.DrawText(line, 0, y);
		y += lineHeight;
	}
}

void AnkerHyperlink::drawPrintFailWrapText(wxPaintDC &dc, wxString& text, int wrapWidth)
{
    wxFont font = dc.GetFont();
    wxCoord lineHeight = dc.GetMultiLineTextExtent("Test").GetHeight();
    wxPoint textPoint = wxPoint(0, 0);
    if(m_alignType == ALIGN_CENTER){
        wxSize textSize = dc.GetTextExtent(m_text);
        textPoint.x = (GetSize().GetWidth() - textSize.GetWidth()) / 2;
    }


    wxString remainingText = text;
    int y = 0;
    while (!remainingText.empty()) {
        wxString line;

        int type = Slic3r::GUI::wxGetApp().getCurrentLanguageType();
        if ( wxLanguage::wxLANGUAGE_ENGLISH == type || wxLanguage::wxLANGUAGE_ENGLISH_US == type) {
            line = GetNextLineEnglishStr(remainingText, dc, wrapWidth);
        }
        else{
            line = GetNextLineNonEnglishStr(remainingText, dc, wrapWidth);
        }

        dc.DrawText(line, textPoint.x, y);
        y += lineHeight;
    }
}

void AnkerHyperlink::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	if (!m_text.IsEmpty())
	{
		dc.Clear();
		wxBrush brush(wxColour("#62D361"));
		wxPen pen(wxColour("#62D361"));
		dc.SetBrush(brush);
		dc.SetPen(pen);
		dc.SetFont(m_font);
		dc.SetTextForeground(wxColour("#62D361"));

		if (m_wrapWidth > 0) {
            if(m_printflag){
                drawPrintFailWrapText(dc, m_text, m_wrapWidth);
            }else{
                drawWrapText(dc, m_text, m_wrapWidth);
            }
		}
		else {
			wxPoint textPoint = wxPoint(0, 0);
			if (m_alignType == ALIGN_LEFT) 
			{
			}
			else if (m_alignType == ALIGN_CENTER) {
				wxSize textSize = dc.GetTextExtent(m_text);
				textPoint.x = (GetSize().GetWidth() - textSize.GetWidth()) / 2;
			}
			else if (m_alignType == ALIGN_RIGHT) {

			}
			dc.DrawText(m_text, textPoint);
		}
	}
}

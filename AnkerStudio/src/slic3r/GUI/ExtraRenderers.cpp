#include "ExtraRenderers.hpp"
#include "wxExtensions.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
#include "BitmapComboBox.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "AnkerSideBarNew.hpp"

#include <wx/dc.h>
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
#include "wx/generic/private/markuptext.h"
#include "wx/generic/private/rowheightcache.h"
#include "wx/generic/private/widthcalc.h"
#endif
/*
#ifdef __WXGTK__
#include "wx/gtk/private.h"
#include "wx/gtk/private/value.h"
#endif
*/
#if wxUSE_ACCESSIBILITY
#include "wx/private/markupparser.h"
#endif // wxUSE_ACCESSIBILITY

using Slic3r::GUI::from_u8;
using Slic3r::GUI::into_u8;


//-----------------------------------------------------------------------------
// DataViewBitmapText
//-----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(DataViewBitmapText, wxObject)

IMPLEMENT_VARIANT_OBJECT(DataViewBitmapText)

static wxSize get_size(const wxBitmap& icon)
{
#ifdef __WIN32__
    return icon.GetSize();
#else
    return icon.GetScaledSize();
#endif
}

// ---------------------------------------------------------
// BitmapTextRenderer
// ---------------------------------------------------------

#if ENABLE_NONCUSTOM_DATA_VIEW_RENDERING
BitmapTextRenderer::BitmapTextRenderer(wxDataViewCellMode mode /*= wxDATAVIEW_CELL_EDITABLE*/, 
                                                 int align /*= wxDVR_DEFAULT_ALIGNMENT*/): 
wxDataViewRenderer(wxT("AnkerDataViewBitmapText"), mode, align)
{
    SetMode(mode);
    SetAlignment(align);
}
#endif // ENABLE_NONCUSTOM_DATA_VIEW_RENDERING

BitmapTextRenderer::~BitmapTextRenderer()
{
#ifdef SUPPORTS_MARKUP
    #ifdef wxHAS_GENERIC_DATAVIEWCTRL
    delete m_markupText;
    #endif //wxHAS_GENERIC_DATAVIEWCTRL
#endif // SUPPORTS_MARKUP
}

void BitmapTextRenderer::EnableMarkup(bool enable)
{
#ifdef SUPPORTS_MARKUP
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    if (enable) {
        if (!m_markupText)
            m_markupText = new wxItemMarkupText(wxString());
    }
    else {
        if (m_markupText) {
            delete m_markupText;
            m_markupText = nullptr;
        }
    }
#else
    is_markupText = enable;
#endif //wxHAS_GENERIC_DATAVIEWCTRL
#endif // SUPPORTS_MARKUP
}

bool BitmapTextRenderer::SetValue(const wxVariant &value)
{
    m_value << value;

#ifdef SUPPORTS_MARKUP
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    if (m_markupText)
        m_markupText->SetMarkup(m_value.GetText());
    /* 
#else 
#if defined(__WXGTK__)
   GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, G_TYPE_STRING);
    g_value_set_string(&gvalue, wxGTK_CONV_FONT(str.GetText(), GetOwner()->GetOwner()->GetFont()));
    g_object_set_property(G_OBJECT(m_renderer/ *.GetText()* /), is_markupText ? "markup" : "text", &gvalue);
    g_value_unset(&gvalue);
#endif // __WXGTK__
    */
#endif // wxHAS_GENERIC_DATAVIEWCTRL
#endif // SUPPORTS_MARKUP

    return true;
}

bool BitmapTextRenderer::GetValue(wxVariant& WXUNUSED(value)) const
{
    return false;
}

#if ENABLE_NONCUSTOM_DATA_VIEW_RENDERING && wxUSE_ACCESSIBILITY
wxString BitmapTextRenderer::GetAccessibleDescription() const
{
#ifdef SUPPORTS_MARKUP
    if (m_markupText)
        return wxMarkupParser::Strip(m_text);
#endif // SUPPORTS_MARKUP

    return m_value.GetText();
}
#endif // wxUSE_ACCESSIBILITY && ENABLE_NONCUSTOM_DATA_VIEW_RENDERING

bool BitmapTextRenderer::Render(wxRect rect, wxDC *dc, int state)
{
    int xoffset = 0;

    const wxBitmap& icon = m_value.GetBitmap();
    if (icon.IsOk())
    {
        wxSize icon_sz = get_size(icon);
        dc->DrawBitmap(icon, rect.x, rect.y + (rect.height - icon_sz.y) / 2);
        xoffset = icon_sz.x + 4;
    }

#if defined(SUPPORTS_MARKUP) && defined(wxHAS_GENERIC_DATAVIEWCTRL)
    if (m_markupText)
    {
        rect.x += xoffset;
        m_markupText->Render(GetView(), *dc, rect, 0, GetEllipsizeMode());
    }
    else
#endif // SUPPORTS_MARKUP && wxHAS_GENERIC_DATAVIEWCTRL
#ifdef _WIN32 
        // workaround for Windows DarkMode : Don't respect to the state & wxDATAVIEW_CELL_SELECTED to avoid update of the text color
        RenderText(m_value.GetText(), xoffset, rect, dc, state & wxDATAVIEW_CELL_SELECTED ? 0 :state);
#else
        RenderText(m_value.GetText(), xoffset, rect, dc, state);
#endif

    return true;
}

wxSize BitmapTextRenderer::GetSize() const
{
    if (!m_value.GetText().empty())
    {
        wxSize size;
#if defined(SUPPORTS_MARKUP) && defined(wxHAS_GENERIC_DATAVIEWCTRL)
        if (m_markupText)
        {
            wxDataViewCtrl* const view = GetView();
            wxClientDC dc(view);
            if (GetAttr().HasFont())
                dc.SetFont(GetAttr().GetEffectiveFont(view->GetFont()));

            size = m_markupText->Measure(dc);

            int lines = m_value.GetText().Freq('\n') + 1;
            size.SetHeight(size.GetHeight() * lines);
        }
        else
#endif // SUPPORTS_MARKUP && wxHAS_GENERIC_DATAVIEWCTRL
            size = GetTextExtent(m_value.GetText());

        if (m_value.GetBitmap().IsOk())
            size.x += m_value.GetBitmap().GetWidth() + 4;
        return size;
    }
    return wxSize(80, 20);
}


wxWindow* BitmapTextRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
    if (can_create_editor_ctrl && !can_create_editor_ctrl())
        return nullptr;

    DataViewBitmapText data;
    data << value;

    m_was_unusable_symbol = false;

    wxPoint position = labelRect.GetPosition();
    if (data.GetBitmap().IsOk()) {
        const int bmp_width = data.GetBitmap().GetWidth();
        position.x += bmp_width;
        labelRect.SetWidth(labelRect.GetWidth() - bmp_width);
    }

#ifdef __WXMSW__
    // Case when from some reason we try to create next EditorCtrl till old one was not deleted
    if (auto children = parent->GetChildren(); children.GetCount() > 0)
        for (auto child : children)
            if (dynamic_cast<wxTextCtrl*>(child)) {
                parent->RemoveChild(child);
                child->Destroy();
                break;
            }
#endif // __WXMSW__

    wxTextCtrl* text_editor = new wxTextCtrl(parent, wxID_ANY, data.GetText(),
                                             position, labelRect.GetSize(), wxTE_PROCESS_ENTER);
    text_editor->SetInsertionPointEnd();
    text_editor->SelectAll();

    return text_editor;
}

bool BitmapTextRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
    wxTextCtrl* text_editor = wxDynamicCast(ctrl, wxTextCtrl);
    if (!text_editor || text_editor->GetValue().IsEmpty())
        return false;

    m_was_unusable_symbol = Slic3r::GUI::Plater::has_illegal_filename_characters(text_editor->GetValue());
    if (m_was_unusable_symbol)
        return false;

    // The icon can't be edited so get its old value and reuse it.
    wxVariant valueOld;
    GetView()->GetModel()->GetValue(valueOld, m_item, /*colName*/0); 
    
    DataViewBitmapText bmpText;
    bmpText << valueOld;

    // But replace the text with the value entered by user.
    bmpText.SetText(text_editor->GetValue());

    value << bmpText;
    return true;
}

// ----------------------------------------------------------------------------
// BitmapChoiceRenderer
// ----------------------------------------------------------------------------

bool BitmapChoiceRenderer::SetValue(const wxVariant& value)
{
    m_value << value;

    return true;
}

bool BitmapChoiceRenderer::GetValue(wxVariant& value) const 
{
    value << m_value;

    return true;
}

bool BitmapChoiceRenderer::Render(wxRect rect, wxDC* dc, int state)
{
    int xoffset = 0;

    wxColour bgColor = m_value.GetColor();
    wxString text = m_value.GetText();

    if (text.empty())
        return true;

    wxRect bgRect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2);
    wxBrush brush(bgColor);
    wxPen pen(wxColour(41, 42, 45));
    dc->SetBrush(brush);
    dc->SetPen(pen);
    dc->DrawRectangle(bgRect);

    wxColour foreColor = wxColour(255, 255, 255);
    if (bgColor.GetRed() + bgColor.GetGreen() + bgColor.GetBlue() > 550)
        foreColor = wxColour(0, 0, 0);

    // draw index text
    {
        wxBrush brush(bgColor);
        wxPen pen(foreColor);
        dc->SetBrush(brush);
        dc->SetPen(pen);
        wxFont font = dc->GetFont();
#ifdef __APPLE__
        font.SetPointSize(14);
#else
        font.SetPointSize(10);
#endif
        dc->SetFont(font);
        dc->SetTextForeground(foreColor);
#ifdef __APPLE__
        wxPoint textPoint = wxPoint(rect.x + rect.width / 2 - 5, rect.y + rect.height / 2 - 9);
#else
        wxPoint textPoint = wxPoint(rect.x + rect.width / 2 - 5, rect.y + rect.height / 2 - 11);
#endif
        dc->DrawText(text, textPoint);
    }

    // draw bottom right triangle
    {
        wxBrush brush(foreColor);
        wxPen pen(foreColor);
        dc->SetBrush(brush);
        dc->SetPen(pen);
        wxPoint triPoints[3];
        triPoints[0] = wxPoint(rect.x + rect.width - 8, rect.y + rect.height - 3);
        triPoints[1] = wxPoint(rect.x + rect.width - 3, rect.y + rect.height - 3);
        triPoints[2] = wxPoint(rect.x + rect.width - 3, rect.y + rect.height - 8);
        dc->DrawPolygon(3, triPoints);
    }

    //const wxBitmap& icon = m_value.GetBitmap();
    //if (icon.IsOk())
    //{
    //    wxSize icon_sz = get_size(icon);

    //    //dc->DrawBitmap(icon, rect.x, rect.y + (rect.height - icon_sz.GetHeight()) / 2);
    //    //xoffset = icon_sz.GetWidth() + 4;

    //    //if (rect.height==0)
    //    //  rect.height= icon_sz.GetHeight();
    //}

#ifdef _WIN32
    // workaround for Windows DarkMode : Don't respect to the state & wxDATAVIEW_CELL_SELECTED to avoid update of the text color
    //RenderText(m_value.GetText(), /*xoffset*/0, rect, dc, state & wxDATAVIEW_CELL_SELECTED ? 0 : state);
#else
    //RenderText(m_value.GetText(), /*xoffset*/0, rect, dc, state);
#endif

    return true;
}

wxSize BitmapChoiceRenderer::GetSize() const
{
    //wxSize sz = GetTextExtent(m_value.GetText());

    //if (m_value.GetBitmap().IsOk())
    //    sz.x += m_value.GetBitmap().GetWidth() + 4;

    wxSize sz = wxSize(28, 28);

    return sz;
}


wxWindow* BitmapChoiceRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
    if (can_create_editor_ctrl && !can_create_editor_ctrl())
        return nullptr;

    //std::vector<wxBitmapBundle*> icons = get_extruder_color_icons();
    //std::vector<std::string> extruder_colors = Slic3r::GUI::wxGetApp().plater()->get_filament_colors_from_plater_config();


    //if (icons.empty())
    //    return nullptr;

    DataViewBitmapText data;
    data << value;
//
//#ifdef _WIN32
//    Slic3r::GUI::BitmapComboBox* c_editor = new Slic3r::GUI::BitmapComboBox(parent, wxID_ANY, wxEmptyString,
//#else
//    auto c_editor = new wxBitmapComboBox(parent, wxID_ANY, wxEmptyString,
//#endif
//        labelRect.GetTopLeft(), wxSize(labelRect.GetWidth(), -1), 
//        0, nullptr , wxCB_READONLY);
//
//    int def_id = get_default_extruder_idx ? get_default_extruder_idx() : 0;
//    c_editor->Append(_L("default"), def_id < 0 ? wxNullBitmap : *icons[def_id]);
//    for (size_t i = 0; i < icons.size(); i++)
//        c_editor->Append(wxString::Format("%d", i+1), *icons[i]);
//
//    c_editor->SetSelection(atoi(data.GetText().c_str()));
//
//    
//#ifdef __linux__
//    c_editor->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& evt) {
//        // to avoid event propagation to other sidebar items
//        evt.StopPropagation();
//        // FinishEditing grabs new selection and triggers config update. We better call
//        // it explicitly, automatic update on KILL_FOCUS didn't work on Linux.
//        this->FinishEditing();
//    });
//#else
//    // to avoid event propagation to other sidebar items
//    c_editor->Bind(wxEVT_COMBOBOX, [](wxCommandEvent& evt) { evt.StopPropagation(); });
//#endif
//
//    return c_editor;

    int currentIndex = atoi(data.GetText().c_str()) - 1;
    std::vector<std::pair<wxColour, wxString>> contentList;
    const std::vector<Slic3r::GUI::SFilamentInfo>& filamentInfos = Slic3r::GUI::wxGetApp().plater()->sidebarnew().getEditFilamentList();
    for (int i = 0; i < filamentInfos.size(); i++)
    {
        contentList.push_back({ wxColour(filamentInfos[i].wxStrColor), filamentInfos[i].wxStrLabelType });
    }

    wxColour foreColor = wxColour(255, 255, 255);
    if (contentList[currentIndex].first.GetRed() + contentList[currentIndex].first.GetGreen() + contentList[currentIndex].first.GetBlue() > 550)
        foreColor = wxColour(0, 0, 0);

    wxButton* bitmapTextBtn = new wxButton(parent, wxID_ANY, data.GetText(), labelRect.GetTopLeft(), wxSize(labelRect.GetWidth(), -1), wxNO_BORDER);
    bitmapTextBtn->SetSizeHints(wxSize(16, 16), wxSize(16, 16));
    bitmapTextBtn->SetForegroundColour(foreColor);
    bitmapTextBtn->SetBackgroundColour(contentList[currentIndex].first);

    Slic3r::GUI::wxGetApp().floatinglist()->SetParent(bitmapTextBtn);
    Slic3r::GUI::wxGetApp().floatinglist()->setContentList(contentList);
    Slic3r::GUI::wxGetApp().floatinglist()->setCurrentSelection(currentIndex);
    Slic3r::GUI::wxGetApp().floatinglist()->Show();
    
	Slic3r::GUI::wxGetApp().floatinglist()->setItemClickCallback([this](int index){
		Slic3r::GUI::wxGetApp().objectbar()->getObjectBarView()->SetFocus();
        });
    return bitmapTextBtn;
}

bool BitmapChoiceRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
    //wxBitmapComboBox* c = static_cast<wxBitmapComboBox*>(ctrl);
    //int selection = c->GetSelection();
    //if (selection < 0)
    //    return false;

    //DataViewBitmapText bmpText;

    //bmpText.SetText(c->GetString(selection));
    //bmpText.SetBitmap(c->GetItemBitmap(selection));

    wxButton* c = static_cast<wxButton*>(ctrl);
    if (c == nullptr)
        return false;
   
    Slic3r::GUI::wxGetApp().floatinglist()->Hide();
    Slic3r::GUI::wxGetApp().floatinglist()->SetParent(Slic3r::GUI::wxGetApp().plater());

    int currentIndex = Slic3r::GUI::wxGetApp().floatinglist()->getCurrentSelectionIndex();
    std::pair<wxColour, wxString> currentSelection = Slic3r::GUI::wxGetApp().floatinglist()->getItemContent(currentIndex);
    c->SetLabelText(std::to_string(currentIndex + 1));

    DataViewBitmapText bmpText;

    bmpText.SetText(std::to_string(currentIndex + 1));
    bmpText.SetColor(currentSelection.first);

    value << bmpText;
    return true;
}


// ----------------------------------------------------------------------------
// TextRenderer
// ----------------------------------------------------------------------------

bool TextRenderer::SetValue(const wxVariant& value)
{
    m_value = value.GetString();
    return true;
}

bool TextRenderer::GetValue(wxVariant& value) const
{
    return false;
}

bool TextRenderer::Render(wxRect rect, wxDC* dc, int state)
{
#ifdef _WIN32
    // workaround for Windows DarkMode : Don't respect to the state & wxDATAVIEW_CELL_SELECTED to avoid update of the text color
    RenderText(m_value, 0, rect, dc, state & wxDATAVIEW_CELL_SELECTED ? 0 : state);
#else
    RenderText(m_value, 0, rect, dc, state);
#endif

    return true;
}

wxSize TextRenderer::GetSize() const
{
    return GetTextExtent(m_value);
}



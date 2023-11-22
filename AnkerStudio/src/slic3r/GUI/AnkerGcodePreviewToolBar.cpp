#include "libslic3r/libslic3r.h"

#include "libslic3r/GCode.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "I18N.hpp"
#include "ExtruderSequenceDialog.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/AppConfig.hpp"
#include "GUI_Utils.hpp"
#include "MsgDialog.hpp"
#include "Tab.hpp"
#include "GUI_ObjectList.hpp"

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/menu.h>
#include <wx/bmpcbox.h>
#include <wx/statline.h>
#include <wx/dcclient.h>
#include <wx/colordlg.h>

#include <cmath>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include "Field.hpp"
#include "format.hpp"
#include "NotificationManager.hpp"

#include "libslic3r/CustomGCode.hpp"
#include "wxExtensions.hpp"
#include "libslic3r/Utils.hpp"


#include "GLCanvas3D.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"

#include "AnkerGcodePreviewToolBar.hpp"

#include "../Utils/DataManger.hpp"
#include "Common/AnkerGUIConfig.hpp"

using namespace Slic3r::GUI;
//using Slic3r::SLAPrint;
using namespace Slic3r;

static std::string num2Str(long long num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}


static std::string gcode(Slic3r::CustomGCode::Type type)
{
    const Slic3r::PrintConfig& config = Slic3r::GUI::wxGetApp().plater()->fff_print().config();
    switch (type) {
    case Slic3r::CustomGCode::ColorChange: return config.color_change_gcode;
    case Slic3r::CustomGCode::PausePrint:  return config.pause_print_gcode;
    case Slic3r::CustomGCode::Template:    return config.template_custom_gcode;
    default:          return "";
    }
}


wxDEFINE_EVENT(wxCUSTOMEVT_PAUSE_PRINT_LIST_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GCODE_LAYER_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_GCODE_MOVES_SLIDER_VALUE_CHANGED, wxCommandEvent);

CustomSlider::CustomSlider(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style,
    const wxString& name)
    : wxWindow(parent, id, pos, size, style, name),
    m_value(0),
    m_dragging(false)
{
    //SetLineSize(4);
    m_leftMargin = 20;
    m_rigthMargin = 20;

    m_slideBarHeidth = 4;

    m_dragBarWidth = 6;
    m_dragBarHeightExpand = 6;
    m_dragBarHeight = m_slideBarHeidth + m_dragBarHeightExpand * 2;

    m_minValue = 0;
    m_maxValue = 100;

    m_colourDisableState = wxColour(80, 80, 80);

    m_value = 0;
    valueChangeNotify();
    Bind(wxEVT_PAINT, &CustomSlider::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &CustomSlider::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &CustomSlider::OnMouseUp, this);
    Bind(wxEVT_MOTION, &CustomSlider::OnMouseMove, this);
    Bind(wxEVT_ENTER_WINDOW, &CustomSlider::OnEnterWin, this);
    Bind(wxEVT_LEAVE_WINDOW, &CustomSlider::OnLeaveWin, this);
    // Bind(wxEVT_KEY_DOWN, &CustomSlider::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &CustomSlider::OnKeyUp, this);
    Bind(wxEVT_MOUSEWHEEL, &CustomSlider::OnWheel, this);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

bool CustomSlider::SetSlideBarHeigth(int h)
{
    if (h > 0) {
        m_slideBarHeidth = h;
        return true;
    }
    return false;
}

bool CustomSlider::SetValue(int val)
{
    m_value = val;
    if (m_maxValue >= val && val >= m_minValue)
    {
        m_value = val;
        Refresh();
        Update();
        valueChangeNotify();
        return true;
    }
    return false;

    Refresh();
}

bool CustomSlider::SetRange(int minValue, int maxValue)
{
    if (maxValue >= minValue)
    {
        m_minValue = minValue;
        m_maxValue = maxValue;
        if (std::abs(m_maxValue - 0.0f) < 0.000001)
        {
            m_maxValue = static_cast<float>(0.000001);
        }
        valueChangeNotify();
        return true;
    }
    return false;
}

void CustomSlider::SetSliderValues(const std::vector<double>& values)
{
    m_values = values;
}

void CustomSlider::SetSliderAlternateValues(const std::vector<double>& values) 
{ 
    m_alternate_values = values; 
}




double CustomSlider::GetValueD()
{
    if (m_values.empty() || m_minValue < 0)
        return 0.0;
    if (m_values.size() <= size_t(m_minValue)) {
        correct_value();
        return m_values.back();
    }
    return m_values[(int)m_value];
}

void CustomSlider::correct_value()
{
    if (m_value > m_maxValue)
        m_value = m_maxValue;
    else if (m_value < m_minValue)
        m_value = m_minValue;
}

double CustomSlider::get_scroll_step()
{
    if (m_maxValue == m_minValue)
    {
        ANKER_LOG_ERROR << "m_maxValue:" << m_maxValue << ",m_minValue:" << m_minValue;
    }

    return m_maxValue != m_minValue ? double(m_slideBarWidth) / (m_maxValue - m_minValue) : 1;
}

void CustomSlider::setValueChangeCallBack(valueChangeCallbackFunction cb)
{
    onValueChaneCallBack = cb;
}

void CustomSlider::valueChangeNotify()
{
    if (onValueChaneCallBack)
    {
        onValueChaneCallBack(m_minValue, m_value);
    }
}

void CustomSlider::OnPaint(wxPaintEvent& event)
{
    //wxPaintDC dc(this);
    wxBufferedPaintDC dc(this);
    wxBrush brush(wxColour(41, 42, 45));
    dc.SetBackground(brush);
    dc.Clear();

    int width = GetClientSize().GetWidth();
    int height = GetClientSize().GetHeight();

    int centerY = height / 2 + 1;
    m_slideBarWidth = width - (m_leftMargin + m_rigthMargin);

    /*
    if (m_addPausePrintIconRadius * 2 * 2 + m_lineSize  <= height)
    {
        return;
    }
    */

    bool isEnable = IsThisEnabled();

    // slider bar
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(isEnable ? wxColour(55, 76, 54) : m_colourDisableState);
    wxDouble radius = 3.0;
    dc.DrawRectangle(0 + m_leftMargin, centerY - m_slideBarHeidth / 2, m_slideBarWidth, m_slideBarHeidth);

    // slider bar between low and hight value
    //dc.SetBrush(GetForegroundColour());
    //dc.SetPen(wxPen(GetForegroundColour(), 2));
    dc.SetBrush(isEnable ? wxColor(113, 211, 90) : m_colourDisableState);
    dc.SetPen(*wxTRANSPARENT_PEN);

    int valueAtSlider_X = m_maxValue == 0 ? 1 : (m_value * m_slideBarWidth) / m_maxValue;
    dc.DrawRectangle(0 + m_leftMargin + 0 + m_dragBarWidth / 2, centerY - m_slideBarHeidth / 2, valueAtSlider_X - 0 - m_dragBarWidth, m_slideBarHeidth);

    // drag bar
    {
        std::string dragBarIconFile = isEnable ? "sliderBarHighterDrager.png" : "sliderBarHighterDrager_disable.png";
        wxImage image(wxString::FromUTF8(Slic3r::var(dragBarIconFile)), wxBITMAP_TYPE_PNG);
        image.Rescale(8, 14);
        wxBitmap bitmap(image);
        int iconWidth = bitmap.GetWidth();
        int iconHeight = bitmap.GetHeight();
        dc.DrawBitmap(bitmap, 0 + m_leftMargin + valueAtSlider_X, centerY - iconHeight / 2);

        // draw draw value tick
        dc.SetPen(wxPen(isEnable ?  wxColour(113, 211, 90) : m_colourDisableState , 0));
        wxPoint start(0 + m_leftMargin + valueAtSlider_X, centerY - 12);
        wxPoint end(0 + m_leftMargin + valueAtSlider_X, centerY + 12);
        dc.DrawLine(start, end);
        // ANKER_LOG_DEBUG << " draw hight tick at:" << 0 + m_leftMargin + highSliderX   << "    highter:" << m_highLayer ;
    }
}

void CustomSlider::OnMouseDown(wxMouseEvent& event)
{
    if (HasCapture())
        return;
    this->CaptureMouse();

    int x = event.GetX();
    int y = event.GetY();
    int width = GetClientSize().GetWidth();
    int height = GetClientSize().GetHeight();
    int centerY = height / 2 + 1;

    m_slideBarWidth = width - (m_leftMargin + m_rigthMargin);
    int lowSliderX = m_maxValue == 0 ? 1 : (m_value * m_slideBarWidth) / m_maxValue;

    if (std::abs(x - m_leftMargin - lowSliderX) <= m_dragBarWidth && std::abs(y - centerY) <= m_dragBarHeight / 2)
    {
        m_dragging = true;
    }
    else {
        // move select thumb to mouse position
        int slideBarLeft = m_leftMargin;
        int slideBarRight = m_leftMargin + m_slideBarWidth;

        if (x > slideBarLeft && x < slideBarRight) {
            int value = (x - slideBarLeft) / get_scroll_step() + m_minValue;
            if (value > m_minValue && value < m_maxValue){
                    SetValue(value);
            }
        }
    }
    Refresh();
}


void CustomSlider::OnMouseUp(wxMouseEvent& event)
{
    if (!HasCapture())
        return;
    this->ReleaseMouse();

    m_dragging = false;
}

void CustomSlider::OnMouseMove(wxMouseEvent& event)
{
    int x = event.GetX();
    int y = event.GetY();
    if (m_dragging)
    {
        int width = GetClientSize().GetWidth();
        m_slideBarWidth = std::max(width - (m_leftMargin + m_rigthMargin),1);
        if (m_dragging)
        {
            m_value = ((x - m_leftMargin) * m_maxValue) / m_slideBarWidth;
            if (m_value < m_minValue)
                m_value = m_minValue;
            if (m_value > m_maxValue)
                m_value = m_maxValue;
        }

        valueChangeNotify();
        Refresh();
    }
}


void CustomSlider::OnEnterWin(wxMouseEvent& event)
{
    m_is_focused = true;
}

void CustomSlider::OnLeaveWin(wxMouseEvent& event)
{
    m_is_focused = false;
    // m_dragging = false;
}


void CustomSlider::OnKeyDown(wxKeyEvent& event)
{
    //ANKER_LOG_DEBUG<<"====CustomSlider::OnKeyDown" ;
    const int key = event.GetKeyCode();

    if (/*m_is_focused || */ true) {
        int DeltaValue = 1;
        if (wxGetKeyState(WXK_SHIFT))
            DeltaValue = 10;
        else if (wxGetKeyState(WXK_CONTROL))
            DeltaValue = 5;

        if (key == WXK_LEFT || key == WXK_RIGHT) {
            int moveDiretioin = 0;
            if (key == WXK_LEFT)
                moveDiretioin = -1;
            else if (key == WXK_RIGHT)
                moveDiretioin = 1;

            int moveValue = DeltaValue * moveDiretioin;
            SetValue(m_value + moveValue);
        }
    }

    event.Skip();
}

void CustomSlider::OnKeyUp(wxKeyEvent& event)
{
    const int key = event.GetKeyCode();

    if (key == WXK_SHIFT) {
        m_ShiftKeyDown = false;
    }
}

void CustomSlider::OnWheel(wxMouseEvent& event)
{
    int upDown = event.GetWheelRotation();

    if (true) {
        int DeltaValue = 1;
        if (wxGetKeyState(WXK_SHIFT))
            DeltaValue = 10;
        else if (wxGetKeyState(WXK_CONTROL))
            DeltaValue = 5;

        int moveDiretioin = 0;
        if (upDown < 0)
            moveDiretioin = -1;
        else if (upDown > 0)
            moveDiretioin = 1;

        int moveValue = DeltaValue * moveDiretioin;
        SetValue(m_value + moveValue);
    }
}

GcodeLayerSlider::GcodeLayerSlider(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style,
    const wxString& name)
    : wxWindow(parent, id, pos, size, style, name),
    m_lowLayer(0),
    m_highLayer(100),
    m_draggingLow(false),
    m_draggingHigh(false)
{
    //SetLineSize(4);
    m_leftMargin = 20;
    m_rigthMargin = 20;

    m_slideBarHeidth = 4;

    m_dragBarWidth = 6;
    m_dragBarHeightExpand = 6;
    m_dragBarHeight = m_slideBarHeidth + m_dragBarHeightExpand * 2;


    m_drawAddPausePrintIcon = true;
    m_addPausePrintIconRadius = 7;

    m_minValue = 0;
    m_maxValue = 100;

    m_lowLayer = 0;
    m_highLayer = 100;

    m_selectedPausePrintIconIndex = -1;
    m_currentLayerOnly = false;

    m_selection = ssHigher;

    Bind(wxEVT_PAINT, &GcodeLayerSlider::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &GcodeLayerSlider::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &GcodeLayerSlider::OnMouseUp, this);
    Bind(wxEVT_MOTION, &GcodeLayerSlider::OnMouseMove, this);
    Bind(wxEVT_ENTER_WINDOW, &GcodeLayerSlider::OnEnterWin, this);
    Bind(wxEVT_LEAVE_WINDOW, &GcodeLayerSlider::OnLeaveWin, this);
    // Bind(wxEVT_KEY_DOWN, &GcodeLayerSlider::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &GcodeLayerSlider::OnKeyUp, this);
    Bind(wxEVT_MOUSEWHEEL, &GcodeLayerSlider::OnWheel, this);

    valueChangeNotify();

    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

bool GcodeLayerSlider::SetSlideBarHeigth(int h)
{
    if (h > 0) {
        m_slideBarHeidth = h;
        return true;
    }
    return false;
}

void GcodeLayerSlider::SetLowLayer(int lowLayer)
{
    //m_selection = ssLower;
    m_lowLayer = lowLayer;
    correct_lower_value();

    Refresh();
    Update();
    valueChangeNotify();

}

void GcodeLayerSlider::SetHighLayer(int highLayer)
{
    //m_selection = ssHigher;
    m_highLayer = highLayer;
    correct_higher_value();
    Refresh();
    Update();
    valueChangeNotify();
}

bool GcodeLayerSlider::SetRange(int minValue, int maxValue)
{
    if (maxValue >= minValue)
    {
        m_minValue = minValue;
        m_maxValue = maxValue;
        return true;
    }

    return false;
}


void GcodeLayerSlider::SetSelectionSpan(const int lower_val, const int higher_val)
{
    m_lowLayer = std::max(lower_val, m_minValue);
    m_highLayer = std::max(std::min(higher_val, m_maxValue), m_lowLayer);
    if (m_lowLayer < m_highLayer)
        m_currentLayerOnly = false;

    Refresh();
    Update();
    valueChangeNotify();
}


void GcodeLayerSlider::SetSliderValues(const std::vector<double>& values)
{
    m_values = values;
}


double GcodeLayerSlider::get_double_value(const SelectedSlider& selection)
{
    if (m_values.empty() || m_lowLayer < 0)
        return 0.0;
    if (m_values.size() <= size_t(m_highLayer)) {
        correct_higher_value();
        return m_values.back();
    }
    return m_values[selection == ssLower ? m_lowLayer : m_highLayer];
}

void GcodeLayerSlider::correct_higher_value()
{
    if (m_highLayer > m_maxValue)
        m_highLayer = m_maxValue;
    else if (m_highLayer < m_minValue)
        m_highLayer = m_minValue;

    if ((m_highLayer <= m_lowLayer && m_highLayer >= m_minValue) || m_currentLayerOnly)
        m_lowLayer = m_highLayer;

   // ANKER_LOG_DEBUG << "         correct_higher_value, m_highLayer:" << m_highLayer ;
}


void GcodeLayerSlider::correct_lower_value()
{
    if (m_lowLayer < m_minValue)
        m_lowLayer = m_minValue;
    else if (m_lowLayer > m_maxValue)
        m_lowLayer = m_maxValue;

    if ((m_lowLayer >= m_highLayer && m_lowLayer <= m_maxValue) || m_currentLayerOnly)
        m_highLayer = m_lowLayer;
   // ANKER_LOG_DEBUG << "         correct_lower_value, m_highLayer:" << m_highLayer ;
}



void GcodeLayerSlider::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this);
    wxBrush brush(wxColour(41, 42, 45));
    dc.SetBackground(brush);
    dc.Clear();
  //  wxBufferedDC bufferedDC(&dc);

    int width = GetClientSize().GetWidth();
    int height = GetClientSize().GetHeight();

    int centerY = height / 2 + 1;   // slider bar center in Y
    m_slideBarWidth = width - (m_leftMargin + m_rigthMargin);
    /*
    if (m_addPausePrintIconRadius * 2 * 2 + m_lineSize  <= height)
    {
        return;
    }
    */

    // slider bar
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxColour(55, 76, 54));
    wxDouble radius = 2.0;
    dc.DrawRoundedRectangle(0 + m_leftMargin, centerY - m_slideBarHeidth / 2, m_slideBarWidth, m_slideBarHeidth, radius);

    // drag 2 bars
    //dc.SetPen(*wxTRANSPARENT_PEN);
    //dc.SetBrush(wxColour(200, 200, 200));
    //dc.SetPen(wxPen(GetForegroundColour(), 2));
      int lowSliderX = int(m_lowLayer * get_scroll_step() + 0.5);
      int highSliderX = int(m_highLayer * get_scroll_step() + 0.5);
    //dc.DrawRectangle(0 + m_leftMargin + lowSliderX - m_dragBarWidth / 2, centerY - m_dragBarHeight / 2, m_dragBarWidth, m_dragBarHeight);
    //dc.DrawRectangle(0 + m_leftMargin + highSliderX - m_dragBarWidth / 2, centerY - m_dragBarHeight / 2, m_dragBarWidth, m_dragBarHeight);

    float dpiScale = 1.0f;
    /*
    wxWindow* window = GetParent();
    if (window)
    {
        wxWindowDC windowDC(window);
        dpiScale = windowDC.GetContentScaleFactor();
    }
    */

    dc.SetPen(wxPen(wxColour(113, 211, 90), 0));
    // drawer lower drager
    {
        wxImage image(wxString::FromUTF8(Slic3r::var("sliderBarLowerDrager.png")), wxBITMAP_TYPE_PNG);
        image.Rescale(8, 14);
        wxBitmap bitmap(image);
        int iconWidth = bitmap.GetWidth();
        int iconHeight = bitmap.GetHeight();
        dc.DrawBitmap(bitmap, 0 + m_leftMargin + lowSliderX - iconWidth, centerY - iconHeight / 2);

        int screenX = static_cast<int>((0 + m_leftMargin + lowSliderX - iconWidth) * dpiScale);
        int screenY = static_cast<int>((centerY - iconHeight / 2) * dpiScale);
        int screenWidth = static_cast<int>(iconWidth * dpiScale);
        int screenHeight = static_cast<int>(iconHeight * dpiScale);
        m_lowThumbRect = wxRect(screenX, screenY, screenWidth, screenHeight);
        //ANKER_LOG_DEBUG << "    m_lowThumbRect:" << screenX << " " << screenY << "    " << screenWidth << "*" << screenHeight << "         mouse:" << m_mouseX << "," << m_mouseY ;

        // draw draw lower tick
        wxPoint start(0 + m_leftMargin + lowSliderX, centerY-12);
        wxPoint end(0 + m_leftMargin + lowSliderX, centerY+12);
        dc.DrawLine(start, end);
    }
    // drawer hight drager
    {
        wxImage image(wxString::FromUTF8(Slic3r::var("sliderBarHighterDrager.png")), wxBITMAP_TYPE_PNG);
        image.Rescale(8, 14);
        wxBitmap bitmap(image);
        int iconWidth = bitmap.GetWidth();
        int iconHeight = bitmap.GetHeight();
        dc.DrawBitmap(bitmap, 0 + m_leftMargin + highSliderX, centerY - iconHeight / 2);

        int screenX = static_cast<int>((0 + m_leftMargin + highSliderX) * dpiScale);
        int screenY = static_cast<int>((centerY - iconHeight / 2) * dpiScale);
        int screenWidth = static_cast<int>(iconWidth * dpiScale);
        int screenHeight = static_cast<int>(iconHeight * dpiScale);
        m_hightThumbRect = wxRect(screenX, screenY, screenWidth, screenHeight);
        //ANKER_LOG_DEBUG << "    m_hightThumbRect:" << screenX << " " << screenY << "    " << screenWidth << "*" << screenHeight <<  "         mouse:"<<m_mouseX<<","<< m_mouseY ;

        // draw draw lower tick
        wxPoint start(0 + m_leftMargin + highSliderX, centerY - 12);
        wxPoint end(0 + m_leftMargin + highSliderX, centerY + 12);
        dc.DrawLine(start, end);
    }


    // slider bar between low and hight value
    dc.SetBrush(wxColor(113, 211, 90));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0 + m_leftMargin + lowSliderX + m_dragBarWidth / 2, centerY - m_slideBarHeidth / 2, highSliderX - lowSliderX - m_dragBarWidth, m_slideBarHeidth);


    int selectVal = ((m_selection == ssHigher) ? m_highLayer : ((m_selection == ssLower) ? m_lowLayer : -1));
    bool valHaveAdded = false;
    if (m_selection == ssHigher) {
        m_addPauseCmdIcon_x = highSliderX + m_leftMargin;
        m_addPauseCmdValue = m_highLayer;
    }
    else if (m_selection == ssLower) {
        m_addPauseCmdIcon_x = lowSliderX + m_leftMargin;
        m_addPauseCmdValue = m_lowLayer;
    }
    m_addPauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;

    // draw pause print Icon
    int drawDeleteTextForIndex = -1;
    for (int i = 0; i < m_pauseCmdLayers.size(); ++i)
    {
        int pausePositionAtSlider = int(m_pauseCmdLayers[i] * get_scroll_step() + 0.5);
        int PauseCmdIcon_x = pausePositionAtSlider + m_leftMargin;
        int PauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;

        bool isHover = false;
        if (std::abs(m_mouseX - PauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(m_mouseY - PauseCmdIcon_y) <= m_addPausePrintIconRadius)
        {
            isHover = true;
        }

        if (m_pauseCmdLayers[i] == selectVal)
            valHaveAdded = true;

        if (m_selectedPausePrintIconIndex == i)
        {
            /*
            // draw 'x' in circle
            wxPoint Vstart1(PauseCmdIcon_x - 3, PauseCmdIcon_y - 3);
            wxPoint Vend1(PauseCmdIcon_x + 4, PauseCmdIcon_y + 4);
            dc.DrawLine(Vstart1, Vend1);
            wxPoint Vstart2(PauseCmdIcon_x - 3, PauseCmdIcon_y + 3);
            wxPoint Vend2(PauseCmdIcon_x + 4, PauseCmdIcon_y - 4);
            dc.DrawLine(Vstart2, Vend2);
            m_selectedPausePrintIconState = STATE_DELETE;
            */
            drawDeleteTextForIndex = i;
        }
        else
        {
            // draw circle 
            dc.SetBrush(isHover ? wxColour(10, 200, 10) : GetBackgroundColour());
            dc.SetPen(wxPen(wxColor(113, 211, 90), 1));
            wxPoint center(PauseCmdIcon_x, PauseCmdIcon_y);
            int radius = m_addPausePrintIconRadius;
            dc.DrawCircle(center, radius);

            // draw '||' in circle
            wxPoint Vstart1(PauseCmdIcon_x - 2, PauseCmdIcon_y - 2);
            wxPoint Vend1(PauseCmdIcon_x - 2, PauseCmdIcon_y + 3);
            dc.DrawLine(Vstart1, Vend1);
            wxPoint Vstart2(PauseCmdIcon_x + 2, PauseCmdIcon_y - 2);
            wxPoint Vend2(PauseCmdIcon_x + 2, PauseCmdIcon_y + 3);
            dc.DrawLine(Vstart2, Vend2);
        }

        // drag tick for pause icon
        dc.SetPen(wxPen(wxColour(113, 211, 90), 1));
        int lowSliderX = int(m_pauseCmdLayers[i] * get_scroll_step() + 0.5);
        wxPoint start(PauseCmdIcon_x, centerY + m_slideBarHeidth / 2);
        wxPoint end(PauseCmdIcon_x, centerY - m_dragBarHeight / 2);
        dc.DrawLine(start, end);
    }

    // draw "add pause command text" beside the add pause cmd icon
    if (m_drawAddPausePrintIcon && !valHaveAdded && ((m_hoverOnHightDrager && m_selection == ssHigher) || (m_hoverOnLowDrager && m_selection == ssLower) || (m_hoverOnAddPausePrintIcon && m_selectedPausePrintIconIndex < 0)))
    {
        if (true) {
            wxString pausePrintGcode = gcode(Slic3r::PausePrint);
            //wxString text = _L("Add Pause Command") + wxString::Format(" (%s)", pausePrintGcode);
            wxString text = _L("common_preview_operation_tipsadd"); // "Add Pause Command"
            wxSize textSize = dc.GetTextExtent(text);

            int pausePositionAtSlider = int((m_hoverOnHightDrager ? m_highLayer : m_lowLayer) * get_scroll_step() + 0.5);
            int PauseCmdIcon_x = m_addPauseCmdIcon_x;
            int PauseCmdIcon_y = m_addPauseCmdIcon_y;

            int textX = PauseCmdIcon_x + m_addPausePrintIconRadius + 3;
            textX = (textX + textSize.GetWidth() > width) ? width - textSize.GetWidth() : textX;
            if ((m_selection == ssHigher ? m_highLayer : m_lowLayer) > (m_minValue + m_maxValue) / 2)
            {
                textX = PauseCmdIcon_x - m_addPausePrintIconRadius - 3 - textSize.GetWidth();
                textX = (textX < 0) ? 0 : textX;
            }

            int textY = PauseCmdIcon_y - m_addPausePrintIconRadius;

            dc.SetTextForeground(wxColour(255, 255, 255));
            wxRect textrect(textX - 2, textY, textSize.GetWidth() + 4, textSize.GetHeight());
            dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
            dc.SetPen(*wxTRANSPARENT_PEN);

            dc.DrawRectangle(textrect);
            dc.DrawText(text, textX, textY);
        }
    }

    //draw add pause print icon  
    if (m_drawAddPausePrintIcon  && !valHaveAdded /* && (m_hoverOnHightDrager || m_hoverOnLowDrager || m_hoverOnAddPausePrintIcon)*/)
    {
        bool hoverOnAddPausePrintIcon = false;
        if (std::abs(m_mouseX - m_addPauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(m_mouseY - m_addPauseCmdIcon_y) <= m_addPausePrintIconRadius)
        {
            hoverOnAddPausePrintIcon = true;
        }
        // draw circle
        //ANKER_LOG_DEBUG << "  hoverOnAddPausePrintIcon:"<< hoverOnAddPausePrintIcon ;
        dc.SetBrush(hoverOnAddPausePrintIcon ? wxColour(10, 200, 10) : GetBackgroundColour());
        dc.SetPen(wxPen(GetForegroundColour(), 1));
        wxPoint center(m_addPauseCmdIcon_x, m_addPauseCmdIcon_y);
        int radius = m_addPausePrintIconRadius;
        dc.DrawCircle(center, radius);
        // draw '+' in circle
        wxPoint Hstart(m_addPauseCmdIcon_x - 3, m_addPauseCmdIcon_y);
        wxPoint Hend(m_addPauseCmdIcon_x + 4, m_addPauseCmdIcon_y);
        dc.DrawLine(Hstart, Hend);
        wxPoint Vstart(m_addPauseCmdIcon_x, m_addPauseCmdIcon_y - 3);
        wxPoint Vend(m_addPauseCmdIcon_x, m_addPauseCmdIcon_y + 4);
        dc.DrawLine(Vstart, Vend);

    }

    // draw text "Delete Pause Command" for the item to to be delete
    if (drawDeleteTextForIndex >= 0 && drawDeleteTextForIndex < m_pauseCmdLayers.size())
    {
        wxString pausePrintGcode = gcode(Slic3r::PausePrint);
        //wxString text = _L("Delete Pause Command") + wxString::Format(" (%s)", pausePrintGcode);
        wxString text = _L("common_preview_operation_tipsdelete");
        wxSize textSize = dc.GetTextExtent(text);

        int pausePositionAtSlider = int(m_pauseCmdLayers[drawDeleteTextForIndex] * get_scroll_step() + 0.5);
        int PauseCmdIcon_x = pausePositionAtSlider + m_leftMargin;
        int PauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;
        int textX = PauseCmdIcon_x + m_addPausePrintIconRadius + 3;
        textX = (textX + textSize.GetWidth() > width) ? width - textSize.GetWidth() : textX;
        if (m_pauseCmdLayers[drawDeleteTextForIndex] > (m_minValue + m_maxValue) / 2)
        {
            textX = PauseCmdIcon_x - m_addPausePrintIconRadius - 3 - textSize.GetWidth();
            textX = (textX < 0) ? 0 : textX;
        }

        int textY = PauseCmdIcon_y - m_addPausePrintIconRadius;

        dc.SetTextForeground(wxColour(200, 200, 200));
        wxRect textrect(textX - 2, textY, textSize.GetWidth() + 4, textSize.GetHeight());
        dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(textrect);
        dc.DrawText(text, textX, textY);
    }


    // draw delete icon in circle for the item to to be delete
    for (int i = 0; i < m_pauseCmdLayers.size(); ++i)
    {
        int pausePositionAtSlider = int(m_pauseCmdLayers[i] * get_scroll_step() + 0.5);
        int PauseCmdIcon_x = pausePositionAtSlider + m_leftMargin;
        int PauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;

        bool isHover = false;
        if (std::abs(m_mouseX - PauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(m_mouseY - PauseCmdIcon_y) <= m_addPausePrintIconRadius)
        {
            isHover = true;
        }

        if (m_selectedPausePrintIconIndex == i)
        {
            // draw circle 
            dc.SetBrush(isHover ? wxColour(10, 200, 10) :  GetBackgroundColour());
            dc.SetPen(wxPen(wxColor(113, 211, 90), 1));
            wxPoint center(PauseCmdIcon_x, PauseCmdIcon_y);
            int radius = m_addPausePrintIconRadius;
            dc.DrawCircle(center, radius);

            // draw 'x' in circle
            wxPoint Vstart1(PauseCmdIcon_x - 3, PauseCmdIcon_y - 3);
            wxPoint Vend1(PauseCmdIcon_x + 4, PauseCmdIcon_y + 4);
            dc.DrawLine(Vstart1, Vend1);
            wxPoint Vstart2(PauseCmdIcon_x - 3, PauseCmdIcon_y + 3);
            wxPoint Vend2(PauseCmdIcon_x + 4, PauseCmdIcon_y - 4);
            dc.DrawLine(Vstart2, Vend2);
            m_selectedPausePrintIconState = STATE_DELETE;

            drawDeleteTextForIndex = i;
        }

        // drag tick for pause icon
        dc.SetPen(wxPen(wxColour(113, 211, 90), 1));
        int lowSliderX = int(m_pauseCmdLayers[i] * get_scroll_step() + 0.5);
        wxPoint start(PauseCmdIcon_x, centerY + m_slideBarHeidth / 2);
        wxPoint end(PauseCmdIcon_x, centerY - m_dragBarHeight / 2);
        dc.DrawLine(start, end);
    }

    //dc.DrawText(num2Str(m_lowLayer) + "  " + num2Str(m_highLayer) + "    move:" + num2Str(m_mouseX) + " " + num2Str(m_mouseX) + "  index:" + num2Str(m_selectedPausePrintIconIndex), 20, 50);
}

void GcodeLayerSlider::OnMouseDown(wxMouseEvent& event)
{
    if (HasCapture())
        return;
    this->CaptureMouse();

    int x = event.GetX();
    int y = event.GetY();
    int width = GetClientSize().GetWidth();
    int height = GetClientSize().GetHeight();
    int centerY = height / 2 + 1;
   // ANKER_LOG_DEBUG << "         in OnMouseDown, m_highLayer:" << m_highLayer ;
    m_slideBarWidth = width - (m_leftMargin + m_rigthMargin);
    for (int i = 0; i < m_pauseCmdLayers.size(); ++i)
    {
        int pausePositionAtSlider = int(m_pauseCmdLayers[i] * get_scroll_step() + 0.5);
        int PauseCmdIcon_x = pausePositionAtSlider + m_leftMargin;
        int PauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;

        bool isHover = false;
        if (std::abs(x - PauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(y - PauseCmdIcon_y) <= m_addPausePrintIconRadius)
        {

            if (m_selectedPausePrintIconIndex == -1 || m_selectedPausePrintIconIndex != i)
            {
                m_selectedPausePrintIconIndex = i;
                m_highLayer = m_pauseCmdLayers[i];
                m_lowLayer = m_lowLayer > m_highLayer ? m_highLayer : m_lowLayer;
                ANKER_LOG_INFO << "     select pause :" << m_pauseCmdLayers[i] <<  std::endl;
                if (m_currentLayerOnly) {
                    m_lowLayer = m_highLayer;
                }
            }
            else if (m_selectedPausePrintIconState == STATE_DELETE)
            {
                DeletePausePrintCmdAtIndex(m_selectedPausePrintIconIndex);
                m_selectedPausePrintIconIndex = -1;
            }

            Refresh();
            return;
        }
    }


    //std::cerr << "           onMouseDown: pause icon:" << m_addPauseCmdIcon_x<< ","<< m_addPauseCmdIcon_y  <<  "      x,y:"<<x<<"," <<y;
    
    if (x > m_hightThumbRect.x && (x - m_hightThumbRect.x) <= m_hightThumbRect.width && std::abs(y - centerY) <= m_hightThumbRect.height / 2 )
    {
        // click lowThumb
        m_draggingHigh = true;
        //m_dragingBarType = HIGHT_BAR;
        m_selection = ssHigher;
    }
    else if (x > m_lowThumbRect.x && (x - m_lowThumbRect.x) <= m_lowThumbRect.width && std::abs(y - centerY) <= m_lowThumbRect.height / 2 )
    {
        // click lowThumb
        m_draggingLow = true;
        //m_dragingBarType = LOWER_BAR;
        m_selection = ssLower;
    }
    else if (std::abs(x - m_addPauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(y - m_addPauseCmdIcon_y) <= m_addPausePrintIconRadius)
    {
        // click add pause icon
        AddPausePrintCmdAtLayer(m_addPauseCmdValue);
    }
    else
    {
        // move select thumb to mouse position
        int slideBarLeft = m_leftMargin;
        int slideBarRight = m_leftMargin + m_slideBarWidth;

        if (x > slideBarLeft && x < slideBarRight) {
            int value = (x - slideBarLeft) / get_scroll_step() + m_minValue;
            if (value > m_highLayer && value <= m_maxValue) {
                m_selection = ssHigher;
                SetHighLayer(value);
            }
            else if (value < m_lowLayer && value >= m_minValue) {
                m_selection = ssLower;
                SetLowLayer(value);
            }
            else {
                if (m_selection == ssHigher) {
                    SetHighLayer(value);
                } else {
                    SetLowLayer(value);
                }
            }
        }
    }

    m_selectedPausePrintIconIndex = -1;
    Refresh();
}


void GcodeLayerSlider::OnMouseUp(wxMouseEvent& event)
{
    if (!HasCapture())
        return;
    this->ReleaseMouse();

    m_draggingLow = false;
    m_draggingHigh = false;
}

void GcodeLayerSlider::OnMouseMove(wxMouseEvent& event)
{
    int x = event.GetX();
    int y = event.GetY();
    m_mouseX = x;
    m_mouseY = y;
    int height = GetClientSize().GetHeight();
    int centerY = height / 2 + 1;

    static bool need_extra_refresh = true;  // to recover the state from "hover" state 
    if (m_draggingLow || m_draggingHigh)
    {
        int width = GetClientSize().GetWidth();
        m_slideBarWidth = std::max(width - (m_leftMargin + m_rigthMargin), 1);

        if (m_draggingLow)
        {
            m_lowLayer = ((x - m_leftMargin) * m_maxValue) / m_slideBarWidth;
            if (m_lowLayer < m_minValue) {
                m_lowLayer = m_minValue;
            }
            if (m_lowLayer > m_maxValue) {
                m_lowLayer = m_maxValue;
                m_highLayer = m_maxValue;
            }
            if (m_lowLayer > m_highLayer) {
                m_highLayer = m_lowLayer;
            }
            if (m_currentLayerOnly) {
                m_highLayer = m_lowLayer;
            }
            //ANKER_LOG_DEBUG << "         draggingLow, m_highLayer:" << m_highLayer ;
        }
        else if (m_draggingHigh)
        {
            m_highLayer = ((x - m_leftMargin) * m_maxValue) / m_slideBarWidth;
            if (m_highLayer > m_maxValue) {
                m_highLayer = m_maxValue;
            }
            if (m_highLayer < m_minValue) {
                m_lowLayer = m_minValue;
                m_highLayer = m_minValue;
            }
            if (m_lowLayer > m_highLayer) {
                m_lowLayer = m_highLayer;
            }
            if (m_currentLayerOnly) {
                m_lowLayer = m_highLayer;
            }
        }
        valueChangeNotify();

        Refresh();
        Update();
    }
    else {
        // hover

        static bool HoverOnIcon = false;

        m_hoverOnPausePrintIcon = false;
        m_hoverOnHightDrager = false;
        m_hoverOnLowDrager = false;
        m_hoverOnAddPausePrintIcon = false;

        if (std::abs(x - m_addPauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(y - m_addPauseCmdIcon_y) <= m_addPausePrintIconRadius)
        {
            m_hoverOnAddPausePrintIcon = true;
           // ANKER_LOG_DEBUG << "=== hover on  add pause :"  ;
        }
        else if (x > m_hightThumbRect.x && (x - m_hightThumbRect.x) <= m_hightThumbRect.width && std::abs(y - centerY) <= m_hightThumbRect.height / 2 + m_addPausePrintIconRadius)
        {
            m_hoverOnHightDrager = true;
        }
        else if (x > m_lowThumbRect.x && (x - m_lowThumbRect.x) <= m_lowThumbRect.width && std::abs(y - centerY) <= m_lowThumbRect.height / 2 + m_addPausePrintIconRadius)
        {
            m_hoverOnLowDrager = true;
        }
        else
        {
            for (int i = 0; i < m_pauseCmdLayers.size(); ++i)
            {
                int pausePositionAtSlider = int(m_pauseCmdLayers[i] * get_scroll_step() +0.5);
                int PauseCmdIcon_x = pausePositionAtSlider + m_leftMargin;
                int PauseCmdIcon_y = centerY - m_dragBarHeight / 2 - m_addPausePrintIconRadius - 2;

                if (std::abs(x - PauseCmdIcon_x) <= m_addPausePrintIconRadius && std::abs(y - PauseCmdIcon_y) <= m_addPausePrintIconRadius)
                {
                    m_hoverOnPausePrintIcon = true;
                    break;
                }
            }
        }

#if 0
        if (m_hoverOnAddPausePrintIcon || m_hoverOnPausePrintIcon || m_hoverOnHightDrager || m_hoverOnLowDrager) {
            if (!HoverOnIcon)
            {
                std::string str = m_hoverOnHightDrager ? "hight" : (m_hoverOnLowDrager ? "low" : "other");
                ANKER_LOG_DEBUG << "===on_move in :"<< str ;
                HoverOnIcon = true;
                Refresh();
            }
        }
        else
        {
            if (HoverOnIcon)
            {
                std::string str = m_hoverOnHightDrager ? "hight" : (m_hoverOnLowDrager ? "low" : "other");
                ANKER_LOG_DEBUG << "===on_move out:" << str ;
                HoverOnIcon = false;
                Refresh();
            }
        }
#endif
        Refresh();
       // Update();
    }
}



void GcodeLayerSlider::OnEnterWin(wxMouseEvent& event)
{
    m_is_focused = true;
}

void GcodeLayerSlider::OnLeaveWin(wxMouseEvent& event)
{
    m_is_focused = false;
    // m_draggingLow = false;
    // m_draggingHigh = false;
}

void GcodeLayerSlider::OnKeyDown(wxKeyEvent& event)
{
    const int key = event.GetKeyCode();

    if (/*m_is_focused || */ true) {
        int DeltaValue = 1;
        if (wxGetKeyState(WXK_SHIFT))
            DeltaValue = 10;
        else if (wxGetKeyState(WXK_CONTROL))
            DeltaValue = 5;

        if (key == WXK_UP || key == WXK_DOWN) {
            int moveDiretioin = 0;
            if (key == WXK_DOWN)
                moveDiretioin = -1;
            else if (key == WXK_UP)
                moveDiretioin = 1;

            int moveValue = DeltaValue * moveDiretioin;
            if (m_selection == ssHigher) {
                SetHighLayer(m_highLayer + moveValue);
            }
            else {
                SetLowLayer(m_lowLayer + moveValue);
            }
        }
        else if (key == WXK_RIGHT) {
            if (m_is_focused) {
                m_selection = ssHigher;
                Refresh();
                Update();
            }
        }
        else if (key == WXK_LEFT) {
            if (m_is_focused) {
                m_selection = ssLower;
                Refresh();
                Update();
            }
        }
    }

    event.Skip();
}

void GcodeLayerSlider::OnKeyUp(wxKeyEvent& event)
{
    const int key = event.GetKeyCode();

    if (key == WXK_SHIFT) {
        m_ShiftKeyDown = false;
    }
}

void GcodeLayerSlider::OnWheel(wxMouseEvent& event)
{
    int upDown = event.GetWheelRotation();

    if (true) {
        int DeltaValue = 1;
        if (wxGetKeyState(WXK_SHIFT))
            DeltaValue = 10;
        else if (wxGetKeyState(WXK_CONTROL))
            DeltaValue = 5;

       int moveDiretioin = 0;
       if (upDown<0)
           moveDiretioin = -1;
       else if (upDown>0)
           moveDiretioin = 1;

       int moveValue = DeltaValue * moveDiretioin;
       if (m_selection == ssHigher) {
           SetHighLayer(m_highLayer + moveValue);
       }
       else {
           SetLowLayer(m_lowLayer + moveValue);
       }
    }
}


void GcodeLayerSlider::valueChangeNotify()
{
    if (onValueChaneCallBack)
    {
        onValueChaneCallBack(m_values.empty()? m_lowLayer : m_lowLayer+1, m_values.empty()? m_highLayer : m_highLayer+1);
    }
}

void GcodeLayerSlider::printPauseListChangeNotify()
{
    if (onPausePrintListChangCallBack)
    {
        onPausePrintListChangCallBack();
    }
}

bool GcodeLayerSlider::AddPausePrintCmdAtLayer(int layer)
{
    ANKER_LOG_INFO << "          AddPausePrintCmdAtLayer:"<< layer ;
    if (gcode(Slic3r::PausePrint).empty()) {
        ANKER_LOG_ERROR << "          gcode(Slic3r::PausePrint) is not config" ;
        return false;
    }

    for (int i = 0; i < m_pauseCmdLayers.size(); ++i)
    {
        if (std::abs(m_pauseCmdLayers[i] - layer) < 0.00001)
        {
            return false;
        }
    }
    m_pauseCmdLayers.push_back(layer);
    printPauseListChangeNotify();
    return true;
}

bool GcodeLayerSlider::DeletePausePrintCmdAtIndex(int index)
{
    if (index < m_pauseCmdLayers.size()) {
        m_pauseCmdLayers.erase(m_pauseCmdLayers.begin() + index);
        printPauseListChangeNotify();
        return true;
    }
    return false;
}

/*
void GcodeLayerSlider::postPausePrintCmdListChangedEvent()
{
    //    m_force_mode_apply = type != ToolChange; // It looks like this condition is no needed now. Leave it for the testing

    wxPostEvent(this->GetParent(), wxCommandEvent(wxCUSTOMEVT_PAUSE_PRINT_LIST_CHANGE));
}
*/

void GcodeLayerSlider::setCurentLayerOnly(bool value)
{
    m_currentLayerOnly = value;

    if (!m_currentLayerOnly) {
        m_lowLayer = m_minValue;
        m_highLayer = m_maxValue;
    }

    m_selection == ssLower ? correct_lower_value() : correct_higher_value();
    if (!m_selection) {
        m_selection = ssHigher;
    }

    Refresh();
    Update();
    valueChangeNotify();
}

void GcodeLayerSlider::setValueChangeCallBack(valueChangeCallbackFunction cb)
{
    onValueChaneCallBack = cb;
}

void GcodeLayerSlider::setPausePrintListChangeCallBack(pausePrintListChangeCallbackFunction cb)
{
    onPausePrintListChangCallBack = cb;
}

float GcodeLayerSlider::GetLowerValue()
{
    return m_lowLayer;
}

float GcodeLayerSlider::GetHighterValue()
{
    return m_highLayer;
}


void GcodeLayerSlider::SetModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder)
{
    m_mode = !is_one_extruder_printed_model ? Slic3r::CustomGCode::MultiExtruder :
        only_extruder < 0 ? Slic3r::CustomGCode::SingleExtruder :
        Slic3r::CustomGCode::MultiAsSingle;
    // if (!m_ticks.mode || (m_ticks.empty() && m_ticks.mode != m_mode))
    //    m_ticks.mode = m_mode;
    m_only_extruder = only_extruder;
    // UseDefaultColors(m_mode == SingleExtruder);
    m_is_wipe_tower = m_mode != Slic3r::CustomGCode::SingleExtruder;
}


int GcodeLayerSlider::get_Layer_from_value(double value, bool force_lower_bound)
{
    std::vector<double>::iterator it;
    if (m_is_wipe_tower && !force_lower_bound)
        it = std::find_if(m_values.begin(), m_values.end(),
            [this, value](const double& val) { return fabs(value - val) <= epsilon(); });
    else
        it = std::lower_bound(m_values.begin(), m_values.end(), value - epsilon());

    if (it == m_values.end())
        return -1;
    return int(it - m_values.begin());
}



double GcodeLayerSlider::get_scroll_step()
{
   if (m_maxValue == m_minValue)
    {
        ANKER_LOG_ERROR << "GcodeLayerSlider::get_scroll_step()" <<"m_maxValue :" << m_maxValue << ",m_minValue:" << m_minValue;
    }

    return m_maxValue != m_minValue ? double(m_slideBarWidth) / (m_maxValue - m_minValue) : 1 ;
}

void GcodeLayerSlider::SetGcodePauseCmdValues(const Slic3r::CustomGCode::Info& custom_gcode_per_print_z)
{
    if (m_values.empty()) {
        //m_ticks.mode = m_mode;
        return;
    }

    const bool was_empty = m_pauseCmdLayers.empty();

    m_pauseCmdLayers.clear();
    const std::vector<Slic3r::CustomGCode::Item>& heights = custom_gcode_per_print_z.gcodes;
    for (auto h : heights) {
        int layer = get_Layer_from_value(h.print_z);
        if (layer >= 0 && h.type == Slic3r::CustomGCode::PausePrint) {
            // m_ticks.ticks.emplace(TickCode{ tick, h.type, h.extruder, h.color, h.extra });
            m_pauseCmdLayers.emplace_back(layer);
        }
    }

    if (!was_empty && m_pauseCmdLayers.empty()) {
        // Switch to the "Feature type"/"Tool" from the very beginning of a new object slicing after deleting of the old one
       // post_ticks_changed_event();
        printPauseListChangeNotify();
    }
    /*
            // init extruder sequence in respect to the extruders count
            if (m_pauseCmdLayers.empty())
                m_extruders_sequence.init(m_extruder_colors.size());
    */
    if (custom_gcode_per_print_z.mode && !custom_gcode_per_print_z.gcodes.empty())
        m_mode = custom_gcode_per_print_z.mode;

    Refresh();
    Update();
}


Slic3r::CustomGCode::Info GcodeLayerSlider::GetGcodePauseCmdValues() const
{

    //const int extruders_cnt = Slic3r::GUI::wxGetApp().extruders_edited_cnt();
    int selected_extruder = -1;

    Slic3r::CustomGCode::Info custom_gcode_per_print_z;
    std::vector<Slic3r::CustomGCode::Item>& values = custom_gcode_per_print_z.gcodes;

    const int val_size = m_values.size();
    if (!m_values.empty())
        for (const float& layerValueIdx : m_pauseCmdLayers) {
            if (layerValueIdx > val_size)
                break;

            const int extruder = selected_extruder > 0 ? selected_extruder : std::max<int>(1, m_only_extruder);
            values.emplace_back(Slic3r::CustomGCode::Item{ m_values[(int)layerValueIdx], Slic3r::CustomGCode::PausePrint, extruder, "", "Place bearings in slots and resume printing" });
        }

    if (m_force_mode_apply) {
        custom_gcode_per_print_z.mode = m_mode;
    }

    return custom_gcode_per_print_z;
}



AnkerGcodePreviewLayerToolbar::AnkerGcodePreviewLayerToolbar(wxWindow* parent) //: wxPanel(parent, wxID_ANY)
    :wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW)
{
    initGui();
    Bind(wxEVT_SIZE, &AnkerGcodePreviewLayerToolbar::OnSize, this);
    Bind(wxEVT_KEY_DOWN, &AnkerGcodePreviewLayerToolbar::OnKeyDown, this);
    Bind(wxEVT_ENTER_WINDOW, &AnkerGcodePreviewLayerToolbar::OnEnterWin, this);
    Bind(wxEVT_LEFT_DOWN, &AnkerGcodePreviewLayerToolbar::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &AnkerGcodePreviewLayerToolbar::OnMouseUp, this);
    Bind(wxEVT_MOTION, &AnkerGcodePreviewLayerToolbar::OnMouseMove, this);
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        //event.Skip();
        });

}

void AnkerGcodePreviewLayerToolbar::initGui()
{
    // row 1
    m_layerLabel = new wxStaticText(this, wxID_ANY, "LayerLabel");
    m_layerLabel->SetForegroundColour(wxColour(240, 240, 240));
    m_layerLabel->SetFont(ANKER_FONT_NO_1);
    m_layerLabel->SetLabelText(_L("common_preview_operationtitle_layer")); // "Layer"

    m_layerSlider = new GcodeLayerSlider(this);
    m_layerSlider->SetBackgroundColour(wxColour(41, 42, 45));
    m_layerSlider->SetForegroundColour(wxColour(100, 100, 100));
    m_layerSlider->setValueChangeCallBack(std::bind(&AnkerGcodePreviewLayerToolbar::onLayerChange, this, std::placeholders::_1, std::placeholders::_2));
    m_layerSlider->setPausePrintListChangeCallBack(std::bind(&AnkerGcodePreviewLayerToolbar::onPausePrintListChange, this));
    m_layerSlider->SetRange(0, 100);

    m_layerVal = new wxStaticText(this, wxID_ANY, "LayerVal");
    m_layerVal->SetForegroundColour(wxColour(140, 140, 140));
    m_layerVal->SetFont(ANKER_FONT_NO_1);
    m_layerVal->SetLabelText("0/0");

    wxImage uncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_uncheck.png")), wxBITMAP_TYPE_PNG);
    uncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap uncheckScaledBitmap(uncheckImage);

    wxImage checkImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_check_style_gray.png")), wxBITMAP_TYPE_PNG);
    checkImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    wxBitmap checkScaledBitmap(checkImage);

	wxImage disuncheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_disuncheck.png")), wxBITMAP_TYPE_PNG);
	disuncheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
	wxBitmap disUncheckScaledBitmap(disuncheckImage);

	wxImage discheckImage = wxImage(wxString::FromUTF8(Slic3r::var("checkbox_discheck.png")), wxBITMAP_TYPE_PNG);
	discheckImage.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
	wxBitmap disCheckScaledBitmap(discheckImage);

    m_currentLayerOnlyCheckbox = new AnkerCheckBox(this, 
                                                    uncheckScaledBitmap.ConvertToImage(),
                                                    checkScaledBitmap.ConvertToImage(),
                                                    disUncheckScaledBitmap.ConvertToImage(),
                                                    disCheckScaledBitmap.ConvertToImage(),
                                                    wxString(""),
                                                    ANKER_FONT_NO_1,
                                                    wxColour("#FFFFFF"),
                                                    wxID_ANY);

    m_currentLayerOnlyCheckbox->SetWindowStyleFlag(wxBORDER_NONE);
    m_currentLayerOnlyCheckbox->SetBackgroundColour(wxColour("#292A2D"));
    m_currentLayerOnlyCheckbox->SetMinSize(wxSize(16, 16));
    m_currentLayerOnlyCheckbox->SetMaxSize(wxSize(16, 16));
    m_currentLayerOnlyCheckbox->SetSize(wxSize(16, 16));
    m_currentLayerOnlyCheckbox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, &AnkerGcodePreviewLayerToolbar::OnCurrentLayerOnlyCheckboxClicked, this);

    m_currentLayerOnlyLable = new wxStaticText(this, wxID_ANY, _L("common_preview_operation_switch1"));  // "Current Layer Only"
    m_currentLayerOnlyLable->SetForegroundColour(wxColour(240, 240, 240));
    m_currentLayerOnlyLable->SetFont(ANKER_FONT_NO_1);

    // row 2
    m_seteplabel = new wxStaticText(this, wxID_ANY, "seteplabel");
    m_seteplabel->SetForegroundColour(wxColour(240, 240, 240));
    m_seteplabel->SetFont(ANKER_FONT_NO_1);
    m_seteplabel->SetLabelText(_L("common_preview_operationtitle_step")); // "Step"

    m_movesSlider = new CustomSlider(this);
    m_movesSlider->SetBackgroundColour(wxColour(41, 42, 45));
    m_movesSlider->SetForegroundColour(wxColour(100, 100, 100));
    m_movesSlider->setValueChangeCallBack(std::bind(&AnkerGcodePreviewLayerToolbar::onStepChange, this, std::placeholders::_1, std::placeholders::_2));

    m_stepVal = new wxStaticText(this, wxID_ANY, "stepVal");
    m_stepVal->SetForegroundColour(wxColour(140, 140, 140));
    m_stepVal->SetFont(ANKER_FONT_NO_1);
    m_stepVal->SetLabelText("0");

    m_GcodeReaderCheckbox = new AnkerCheckBox(this, 
                                                uncheckScaledBitmap.ConvertToImage(),
                                                checkScaledBitmap.ConvertToImage(),
		                                        disUncheckScaledBitmap.ConvertToImage(),
		                                        disCheckScaledBitmap.ConvertToImage(),
		                                        wxString(""),
		                                        wxFont(),
		                                        wxColour("#FFFFFF"),
                                                wxID_ANY);
    m_GcodeReaderCheckbox->SetWindowStyleFlag(wxBORDER_NONE);
    m_GcodeReaderCheckbox->SetBackgroundColour(wxColour("#292A2D"));
    m_GcodeReaderCheckbox->SetMinSize(wxSize(16, 16));
    m_GcodeReaderCheckbox->SetMaxSize(wxSize(16, 16));
    m_GcodeReaderCheckbox->SetSize(wxSize(16, 16));
    m_GcodeReaderCheckbox->Bind(wxCUSTOMEVT_ANKER_CHECKBOX_CLICKED, &AnkerGcodePreviewLayerToolbar::OnGcodeReaderCheckboxClicked, this);

    m_GcodeReaderLable = new wxStaticText(this, wxID_ANY, _L("common_preview_operation_switch2")); //"Gcode Reader"
    m_GcodeReaderLable->SetForegroundColour(wxColour(240, 240, 240));
    m_GcodeReaderLable->SetFont(ANKER_FONT_NO_1);
}

void AnkerGcodePreviewLayerToolbar::SetHigherValue(const int higher_val) 
{
    m_layerSlider->SetHighLayer(higher_val);
}

void AnkerGcodePreviewLayerToolbar::SetLowerValue(const int lower_val)
{
    m_layerSlider->SetLowLayer(lower_val);
}

void AnkerGcodePreviewLayerToolbar::SetLayerSliderModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder)
{
    m_layerSlider->SetModeAndOnlyExtruder(is_one_extruder_printed_model, only_extruder);
}

void AnkerGcodePreviewLayerToolbar::SetLayerSliderValues(const std::vector<double>& values)
{
    m_layerSlider->SetSliderValues(values);
}


void AnkerGcodePreviewLayerToolbar::onLayerChange(int lowerLayer, int hightLayer)
{
    m_layerVal->SetLabelText("" + num2Str((long long)(lowerLayer ? lowerLayer : 1)) + "/" + num2Str((long long)(hightLayer ? hightLayer : 1)));
    //Refresh();
    updateUI();

    SendLayerScrollChangeEvt();
}

void AnkerGcodePreviewLayerToolbar::onPausePrintListChange()
{
#if false
    if (m_pausePrintListChangeCB)
        m_pausePrintListChangeCB();
#else
    SendPausePrintListChangeEvt();
#endif
}


void AnkerGcodePreviewLayerToolbar::onStepChange(int lowerLayer, int hightLayer)
{
    //static_cast<unsigned long>(m_alternate_values[value]));
    int minVal = static_cast<unsigned long>(m_movesSlider->m_minValue);
    int val = static_cast<unsigned long>(m_movesSlider->m_value);
    std::vector<double>& alternateVals = m_movesSlider->m_alternate_values;

    int first = static_cast<unsigned long> (minVal >= 0 && minVal < alternateVals.size() ? alternateVals[minVal] : 0);
    int curr = static_cast<unsigned long>(val>=0 && val < alternateVals.size() ? alternateVals[val]:0);

    m_stepVal->SetLabelText("" + /*num2Str((long long)first) + "/" + */ num2Str((long long)curr));
    //Refresh();
    updateUI();

#if false
    if(m_movesSliderValueChangeCB)
        m_movesSliderValueChangeCB();
#else
    SendMovesScrollChangeEvt();
#endif
}

void AnkerGcodePreviewLayerToolbar::OnCurrentLayerOnlyCheckboxClicked(wxCommandEvent& event)
{
    bool isChecked = m_currentLayerOnlyCheckbox->getCheckStatus();
    m_layerSlider->setCurentLayerOnly(isChecked);
}

void AnkerGcodePreviewLayerToolbar::OnGcodeReaderCheckboxClicked(wxCommandEvent& event)
{
    bool isChecked = m_GcodeReaderCheckbox->getCheckStatus();
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    GLCanvas3D* canvas = plater ? plater->canvas_preview() : nullptr;

    if(canvas)
        canvas->set_gcode_window_visibility(isChecked);
}

void AnkerGcodePreviewLayerToolbar::OnSize(wxSizeEvent& event)
{
    //calculateLayoutAndResize();
    Refresh();
    //updateUI();
}

void AnkerGcodePreviewLayerToolbar::OnKeyDown(wxKeyEvent& event)
{
    m_layerSlider->OnKeyDown(event);
    m_movesSlider->OnKeyDown(event);

    event.Skip();
}


void AnkerGcodePreviewLayerToolbar::OnEnterWin(wxMouseEvent& event)
{
    this->SetFocus();
}


void AnkerGcodePreviewLayerToolbar::OnMouseDown(wxMouseEvent& event)
{
    m_isDragging = true;
    if (HasCapture())
        return;
    this->CaptureMouse();

    int x = event.GetX();
    int y = event.GetY();

    SetCursor(wxCursor(wxCURSOR_SIZING));

    m_clickPos.x = x;
    m_clickPos.y = y;
}

void AnkerGcodePreviewLayerToolbar::OnMouseUp(wxMouseEvent& event)
{
    m_isDragging = false;
    if (!HasCapture())
        return;
    this->ReleaseMouse();
    SetCursor(wxCursor(wxCURSOR_NONE));
}

void AnkerGcodePreviewLayerToolbar::OnMouseMove(wxMouseEvent& event)
{
    int x = event.GetX();
    int y = event.GetY();

    int maxMove = 100;

    if (m_isDragging) {
        ANKER_LOG_DEBUG << "    tool bar move x,y: " << x - m_clickPos.x << " " << y - m_clickPos.y ;
        if (std::abs(x - m_clickPos.x) > maxMove || std::abs(y - m_clickPos.y) > maxMove)
            return;

        this->SetPosition(wxPoint(this->GetPosition().x + x - m_clickPos.x, this->GetPosition().y + y - m_clickPos.y));
    }
}

void AnkerGcodePreviewLayerToolbar::SendLayerScrollChangeEvt()
{
   // wxCommandEvent e(wxEVT_SCROLL_CHANGED);
    wxCommandEvent e(EVT_GCODE_LAYER_SLIDER_VALUE_CHANGED);
    e.SetEventObject(this);
    ProcessWindowEvent(e);
}

void AnkerGcodePreviewLayerToolbar::SendMovesScrollChangeEvt()
{
    // wxCommandEvent e(wxEVT_SCROLL_CHANGED);
    wxCommandEvent e(EVT_GCODE_MOVES_SLIDER_VALUE_CHANGED);
    e.SetEventObject(this);
    ProcessWindowEvent(e);
}

void AnkerGcodePreviewLayerToolbar::SendPausePrintListChangeEvt()
{
    wxCommandEvent e(wxCUSTOMEVT_PAUSE_PRINT_LIST_CHANGE);
    e.SetEventObject(this);
    ProcessWindowEvent(e);
}


void AnkerGcodePreviewLayerToolbar::updateUI()
{
    //ANKER_LOG_DEBUG << "====AnkerGcodePreviewLayerToolbar::updateUI" ;
    Plater* plater = Slic3r::GUI::wxGetApp().plater();
    GLCanvas3D* canvas = plater ? plater->canvas_preview() : nullptr;

    if (canvas) {
        bool CheckBoxCurrentStatus = m_GcodeReaderCheckbox->getCheckStatus();
        bool gcodeWindowVisible = canvas->get_gcode_window_visibility();
        if (CheckBoxCurrentStatus != gcodeWindowVisible) {
            m_GcodeReaderCheckbox->setCheckStatus(gcodeWindowVisible);
            Refresh();
        }
    }
}


void AnkerGcodePreviewLayerToolbar::calculateLayoutAndResize()
{
    int row1Height = 50;
    int row2Height = 30;

    int checkBox_text_Space = 5;
    int space = 10;
    int wideSpace = 20;
    int margingUp = 5;
    int margingDown = 10;

    wxSize layerLabeltextSize = m_layerLabel->GetTextExtent(m_layerLabel->GetLabel());
    wxSize setepLabeltextSize = m_seteplabel->GetTextExtent(m_seteplabel->GetLabel());

    wxSize layerValtextSize = m_layerVal->GetTextExtent(m_layerVal->GetLabel());
    wxSize setepValtextSize = m_stepVal->GetTextExtent(m_stepVal->GetLabel());
    int bigerWidth = std::max(layerValtextSize.GetWidth(), setepValtextSize.GetWidth());
    static int ValueMaxWidth = m_seteplabel->GetTextExtent("9999999").GetWidth();  // asume the value text length
    ValueMaxWidth = bigerWidth > ValueMaxWidth ? bigerWidth + 10 : ValueMaxWidth;   // expand width

    wxSize currentLayerOnlyLableTextSize = m_currentLayerOnlyLable->GetTextExtent(m_currentLayerOnlyLable->GetLabel());
    wxSize GcodeReaderLableTextSize = m_GcodeReaderLable->GetTextExtent(m_GcodeReaderLable->GetLabel());

    int col1width = std::max(layerLabeltextSize.GetWidth(), setepLabeltextSize.GetWidth());
    int col3width = ValueMaxWidth;
    int col4width = std::max(m_currentLayerOnlyCheckbox->GetBestSize().GetWidth(), 0);
    int col5width = std::max(currentLayerOnlyLableTextSize.GetWidth(), GcodeReaderLableTextSize.GetWidth());
    int col2width = GetSize().GetWidth() - col1width - col3width - col4width - col5width - space * 5 - wideSpace;
    int textHeight = layerLabeltextSize.GetHeight();
    int checkBoxHeight = m_currentLayerOnlyCheckbox->GetClientRect().GetHeight();

    // row 1, 5 column
    m_layerLabel->SetPosition(wxPoint(space * 1, row1Height / 2 - textHeight / 2 + margingUp));
    m_layerLabel->SetSize(wxSize(layerLabeltextSize.GetWidth(), setepLabeltextSize.GetWidth()));

    m_layerSlider->SetPosition(wxPoint(col1width + space * 2, margingUp));
    m_layerSlider->SetSize(wxSize(col2width, row1Height));

    m_layerVal->SetPosition(wxPoint(col1width + col2width + space * 3, row1Height / 2 - textHeight / 2 + margingUp));

    m_currentLayerOnlyCheckbox->SetPosition(wxPoint(col1width + col2width + col3width + space * 3 + wideSpace  , row1Height / 2 - checkBoxHeight / 2 + margingUp));

    m_currentLayerOnlyLable->SetPosition(wxPoint(col1width + col2width + col3width + space * 3 + wideSpace + col4width + checkBox_text_Space, row1Height / 2 - textHeight / 2 + margingUp));
    m_currentLayerOnlyLable->SetSize(wxSize(currentLayerOnlyLableTextSize.GetWidth(), currentLayerOnlyLableTextSize.GetWidth()));

    // row 2, 5 column
    m_seteplabel->SetPosition(wxPoint(space * 1, margingUp + ( row1Height + row2Height / 2 - textHeight / 2)));
    m_seteplabel->SetSize(wxSize(layerLabeltextSize.GetWidth(), setepLabeltextSize.GetWidth()));

    m_movesSlider->SetPosition(wxPoint(col1width + space * 2, row1Height + margingUp));
    m_movesSlider->SetSize(wxSize(col2width, row2Height));

    m_stepVal->SetPosition(wxPoint(col1width + col2width + space * 3, margingUp + (row1Height + row2Height / 2 - textHeight / 2)));

    m_GcodeReaderCheckbox->SetPosition(wxPoint(col1width + col2width + col3width  + space * 3 + wideSpace, margingUp + (row1Height + row2Height / 2 - checkBoxHeight / 2)));

    m_GcodeReaderLable->SetPosition(wxPoint(col1width + col2width + col3width + space * 3 + wideSpace + col4width + checkBox_text_Space, margingUp + (row1Height + row2Height / 2 - textHeight / 2)));
    m_GcodeReaderLable->SetSize(wxSize(currentLayerOnlyLableTextSize.GetWidth(), currentLayerOnlyLableTextSize.GetWidth()));

    this->SetSize(wxSize(GetSize().GetWidth() ,row1Height + row2Height + margingDown));
   // ANKER_LOG_DEBUG << "====AnkerGcodePreviewLayerToolbar::calculateLayoutAndResize ,size:"<< GetSize().GetWidth() <<"  "<< row1Height + row2Height + margingDown ;
}


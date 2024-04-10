#ifndef ANKER_GCODE_PREVIEW_TOOL_BAR_H
#define ANKER_GCODE_PREVIEW_TOOL_BAR_H

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/stattext.h>

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/Overlay.h>

//#include "wx/popupwin.h"
#include "libslic3r/CustomGCode.hpp"

#include "AnkerCheckBox.hpp"

wxDECLARE_EVENT(wxCUSTOMEVT_PAUSE_PRINT_LIST_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_GCODE_LAYER_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(EVT_GCODE_MOVES_SLIDER_VALUE_CHANGED, wxCommandEvent);

class CustomSlider : public wxWindow
{
public:
    using valueChangeCallbackFunction = std::function<void(float, float)>;

    CustomSlider(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE,
        const wxString& name = wxPanelNameStr);

    bool SetSlideBarHeigth(int h);
    void SetMaxValue(const int value) { m_maxValue = value; }
    void SetMinValue(const int value) { m_minValue = value; }
    bool SetValue(int val);
    bool SetRange(int minValue, int maxValue);
    void setValueChangeCallBack(valueChangeCallbackFunction cb);

    void SetSliderValues(const std::vector<double>& values);
    void SetSliderAlternateValues(const std::vector<double>& values);

    int GetMaxValue() { return m_maxValue; }
    int  GetMinValue() const { return m_minValue; }

    //double  GetMinValueD();// { return get_double_value(ssLower); }
    double  GetValueD();// { return get_double_value(ssHigher); }
    double  GetMinValueD() { return m_values.empty() || (int)m_minValue >= m_values.size() ? 0. : m_values[(int)m_minValue]; }
    double  GetMaxValueD() { return m_values.empty() || (int)m_maxValue >= m_values.size() ? 0. : m_values[(int)m_maxValue]; }

private:
    void valueChangeNotify();

    void correct_value();
    double get_scroll_step();

    void OnPaint(wxPaintEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnEnterWin(wxMouseEvent& event);
    void OnLeaveWin(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnWheel(wxMouseEvent& event);
private:
    int m_leftMargin;
    int m_rigthMargin;

    int m_slideBarWidth;
    int m_slideBarHeidth;


    int m_dragBarWidth;
    int m_dragBarHeight;
    int m_dragBarHeightExpand;

    bool m_dragging;

    int m_minValue;
    int m_maxValue;
    int m_value;

    wxColour m_colourDisableState;

    std::vector<double> m_values;
    std::vector<double> m_alternate_values;

    bool m_ShiftKeyDown = false;
    bool m_is_focused = false;

    valueChangeCallbackFunction onValueChaneCallBack = nullptr;

    friend class AnkerGcodePreviewLayerToolbar;
};






class GcodeLayerSlider : public wxWindow
{
    using valueChangeCallbackFunction = std::function<void(float, float)>;
    using pausePrintListChangeCallbackFunction = std::function<void(void)>;

    enum selectedPausePrintIconDisplayState
    {
        STATE_PAUSE,
        STATE_DELETE
    };

    enum dragingBarType
    {
        LOWER_BAR,
        HIGHT_BAR
    };

    enum SelectedSlider {
        ssUndef,
        ssLower,
        ssHigher
    };

public:
    GcodeLayerSlider(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE,
        const wxString& name = wxPanelNameStr);

    bool SetSlideBarHeigth(int h);

    void SetLowLayer(int lowLayer);
    void SetHighLayer(int highLayer);
    bool SetRange(int minValue, int maxValue);
    void SetSelectionSpan(int lower_val, int higher_val);

    void SetSliderValues(const std::vector<double>& values);

    void SetMaxValue(const int max_value) { m_maxValue = max_value;}
    int GetMaxValue() {return m_maxValue;}
    int  GetMinValue() const { return m_minValue; }

    double  GetLowerValueD() { return get_double_value(ssLower); }
    double  GetHigherValueD() { return get_double_value(ssHigher); }
    double  GetMinValueD() { return m_values.empty() || (int)m_minValue >= m_values.size() ? 0. : m_values[(int)m_minValue];}
    double  GetMaxValueD() { return m_values.empty() || (int)m_maxValue >= m_values.size() ? 0. : m_values[(int)m_maxValue]; }

    double get_double_value(const SelectedSlider& selection);
    void correct_higher_value();
    void correct_lower_value();


    void setCurentLayerOnly(bool value);
    void setValueChangeCallBack(valueChangeCallbackFunction cb);
    void setPausePrintListChangeCallBack(pausePrintListChangeCallbackFunction cb);

    float GetLowerValue();
    float GetHighterValue();

    void SetModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder);
    void SetGcodePauseCmdValues(const Slic3r::CustomGCode::Info& custom_gcode_per_print_z);
    Slic3r::CustomGCode::Info GetGcodePauseCmdValues() const;
private:
    void valueChangeNotify();
    void printPauseListChangeNotify();

    bool AddPausePrintCmdAtLayer(int layer);
    bool DeletePausePrintCmdAtIndex(int index);
    void postPausePrintCmdListChangedEvent();

    double epsilon() { return 0.0011; }
    int get_Layer_from_value(double value, bool force_lower_bound = false);

    double get_scroll_step();

    void OnPaint(wxPaintEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnEnterWin(wxMouseEvent& event);
    void OnLeaveWin(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnWheel(wxMouseEvent& event);
private:
    // UI data
    int m_leftMargin;
    int m_rigthMargin;

    int m_slideBarWidth;
    int m_slideBarHeidth;

    int m_dragBarWidth;
    int m_dragBarHeight;
    int m_dragBarHeightExpand;

    int m_addPauseCmdValue;
    bool m_drawAddPausePrintIcon;
    int m_addPausePrintIconRadius;
    int m_addPauseCmdIcon_x;
    int m_addPauseCmdIcon_y;

    bool m_hoverOnHightDrager = false;
    bool m_hoverOnLowDrager = false;
    bool m_hoverOnAddPausePrintIcon = false;
    bool m_hoverOnPausePrintIcon = false;

    bool m_draggingLow;
    bool m_draggingHigh;
    //dragingBarType m_dragingBarType;
    SelectedSlider m_selection;

    std::vector<int> m_pauseCmdLayers;
    int m_selectedPausePrintIconIndex;
    selectedPausePrintIconDisplayState m_selectedPausePrintIconState;

    bool m_currentLayerOnly;

    int m_mouseX;
    int m_mouseY;

    wxRect m_hightThumbRect;
    wxRect m_lowThumbRect;

    // logic data
    int m_minValue;
    int m_maxValue;
    int m_lowLayer;
    int m_highLayer;

    std::vector<double> m_values;

    bool m_ShiftKeyDown = false;
    bool m_is_focused = false;

    Slic3r::CustomGCode::Mode m_mode = Slic3r::CustomGCode::SingleExtruder;
    int m_only_extruder = -1;
    bool m_force_mode_apply = true;
    bool m_is_wipe_tower = false; //This flag indicates that there is multiple extruder print with wipe tower

    valueChangeCallbackFunction onValueChaneCallBack = nullptr;
    pausePrintListChangeCallbackFunction onPausePrintListChangCallBack = nullptr;

    friend class AnkerGcodePreviewLayerToolbar;
};



class AnkerGcodePreviewLayerToolbar : public wxFrame //wxPanel
{
    using ValueChangeCallbackFunction = std::function<void()>;
    using PausePrintListChangeCallbackFunction = std::function<void()>;
public:
    AnkerGcodePreviewLayerToolbar(wxWindow* parent);
    void SetHigherValue(const int higher_val);
    void SetLowerValue(const int lower_val);
    float GetLayerSliderHigherValue() { return m_layerSlider->m_highLayer; };
    float GetLayerSliderLowerValue() { return m_layerSlider->m_lowLayer;};

    void SetLayerSliderMaxValue(const int max_value) { 
        m_layerSlider->SetMaxValue(max_value);
    }
    int GetLayerSliderMaxValue() {return m_layerSlider->GetMaxValue();}
    void SetLayerSliderValues(const std::vector<double>& values);

    void SetLayerSliderSelectionSpan(const int lower_val, const int higher_val) { m_layerSlider->SetSelectionSpan(lower_val, higher_val); }

    void SetLayerSliderModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder);

    int GetLayerSliderMinValue() const { return m_layerSlider->GetMinValue(); }

    bool LayerSliderIsLowerAtMin() const { return m_layerSlider->m_lowLayer == m_layerSlider->m_minValue; }
    bool LayerSliderIsHigherAtMax() const { return m_layerSlider->m_highLayer == m_layerSlider->m_maxValue; }

    double  GetLayerSliderLowerValueD() { return m_layerSlider->GetLowerValueD(); }
    double  GetLayerSliderHigherValueD() { return m_layerSlider->GetHigherValueD(); }
    double  GetLayerSliderMinValueD() { return m_layerSlider->GetMinValueD(); }
    double  GetLayerSliderMaxValueD() { return m_layerSlider->GetMaxValueD(); }

    void SetLayerSliderGcodePauseCmdValues(const Slic3r::CustomGCode::Info& custom_gcode_per_print_z) { m_layerSlider->SetGcodePauseCmdValues(custom_gcode_per_print_z); }
    Slic3r::CustomGCode::Info GettLayerSliderGcodePauseCmdValues() { return m_layerSlider->GetGcodePauseCmdValues(); }
    void setPausePrintListChangeCallBack(PausePrintListChangeCallbackFunction cb) { m_pausePrintListChangeCB = cb; };

    // Move Slider
    void setMovesSliderEnable(bool enable) {
        m_movesSlider->Enable(enable); 
        m_movesSlider->Refresh();
        m_movesSlider->Update();
    }
    void setMovesSliderValueChangeCallBack(ValueChangeCallbackFunction cb) { m_movesSliderValueChangeCB = cb;};
    void SetMovesSliderValues(const std::vector<double>& values) { 
        m_movesSlider->SetSliderValues(values); 
        calculateLayoutAndResize();
    };
    void SetMovesSliderAlternateValues(const std::vector<double>& values) { m_movesSlider->SetSliderAlternateValues(values); }
    void SetMovesSliderMaxValue(const int value) { m_movesSlider->SetMaxValue(value); }
    void SetMovesSliderMinValue(const int value) { m_movesSlider->SetMinValue(value); }
    void SetMovesSliderValue(const int value) { m_movesSlider->SetValue(value); m_movesSlider->Refresh();}

    double  GetMovesSliderValueD() { return m_movesSlider->GetValueD(); }
    double  GetMovesSliderMinValueD() { return m_movesSlider->GetMinValueD(); }
    double  GetMovesSliderMaxValueD() { return m_movesSlider->GetMaxValueD(); }

    void updateUI();
    void calculateLayoutAndResize();
    void msw_rescale(){ }
private:
    void InitGui();
    void onLayerChange(int lowerLayer, int hightLayer);
    void onStepChange(int m_minValue, int Value);
    void SendLayerScrollChangeEvt();
    void SendMovesScrollChangeEvt();

    void onPausePrintListChange();
    void SendPausePrintListChangeEvt();

private:

    void OnCurrentLayerOnlyCheckboxClicked(wxCommandEvent& event);
    void OnGcodeReaderCheckboxClicked(wxCommandEvent& event);

    void OnSize(wxSizeEvent& event);

public:
    void OnKeyDown(wxKeyEvent& event);
    void OnEnterWin(wxMouseEvent& event);

    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);

private:
    wxStaticText* m_layerLabel;
    GcodeLayerSlider* m_layerSlider;
    wxStaticText* m_layerVal;
    AnkerCheckBox* m_currentLayerOnlyCheckbox;
    wxStaticText* m_currentLayerOnlyLable;

    wxStaticText* m_seteplabel;
    CustomSlider* m_movesSlider;
    wxStaticText* m_stepVal;
    AnkerCheckBox* m_GcodeReaderCheckbox;
    wxStaticText* m_GcodeReaderLable;

    PausePrintListChangeCallbackFunction m_pausePrintListChangeCB = nullptr;
    ValueChangeCallbackFunction m_movesSliderValueChangeCB = nullptr;

    bool m_isDragging = false;
    wxPoint m_clickPos;
};



#endif // !ANKER_GCODE_PREVIEW_TOOL_BAR_H

#ifndef ANKER_SPINBOX_HPP_
#define ANKER_SPINBOX_HPP_

#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include "AnkerBox.hpp"
#include "AnkerStaticText.hpp"
#include "AnkerButton.hpp"
#include <boost/regex.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/signals2/connection.hpp>

enum AnkerSpinBoxCtrlDoubleType {
    AnkerSpinBoxCtrlDoubleType_Unit,
    AnkerSpinBoxCtrlDoubleType_2Btn,
};


class AnkerTextCtrl : public wxTextCtrl, public AnkerBase
{
public:
    AnkerTextCtrl(wxWindow* parent, wxWindowID id,
        const wxString& value = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxTextCtrlNameStr));
};

class AnkerSpinBoxCtrlDoubleWith2Btn : public AnkerBox
{
public:
    AnkerSpinBoxCtrlDoubleWith2Btn(wxWindow* parent, 
        const wxPoint& pos = wxDefaultPosition, 
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL);

    wxString getValue();
    virtual void borderHighEnable(bool enable = true);
    virtual void setTextCtrlFont(const wxFont& font = ANKER_FONT_NO_1);
    virtual void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    virtual void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));

    void setInterval(float interval = 1.0);
    void setValue(float value = 0.0);
    void setValueRange(float minValue, float maxValue);
    
    boost::signals2::connection FocusLeaveConnect(const boost::signals2::slot<void()>& slot);
    boost::signals2::connection EnterKeyConnect(const boost::signals2::slot<void()>& slot);

private:
    void OnUpBtnClicked(wxCommandEvent& event);
    void OnDownBtnClicked(wxCommandEvent& event);
    void OnEditTextFinished(wxCommandEvent& event);
    void OnTextCtrlTextChanged(wxCommandEvent& event);
    void OnKeyHook(wxKeyEvent& event);
    void OnTextCtrlLostFocus(wxFocusEvent& event);
    void OnTextCtrlFocus(wxFocusEvent& event);
    bool inputValueIsVaild(const wxString& text);
private:
    AnkerButton* m_upBtn;
    AnkerButton* m_downBtn;
    AnkerTextCtrl* m_spinCtrl;
    float m_interval = 1.0;
    float m_maxValue = 100000.0;
    float m_minValue = -100000.0;
    boost::signals2::signal<void()> m_focusLeaveSignal;
    boost::signals2::signal<void()> m_enterKeySignal;
};

class AnkerSpinBoxCtrlDoubleWithUnit : public AnkerBox
{
public:
    AnkerSpinBoxCtrlDoubleWithUnit(wxWindow* parent, const wxString& unit,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL);


    wxString getValue();
    virtual void borderHighEnable(bool enable = true);
    virtual void setTextCtrlFont(const wxFont& font = ANKER_FONT_NO_1);
    virtual void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    virtual void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    void setTextCtrlRegex(const std::string& reg = ("[-+]?[0-9]+(\\.[0-9]+)?"));
    
    void setUnitTextCtrlFont(const wxFont& font);
    void setInputTextCtrlFont(const wxFont& font);
    void setInterval(float interval = 1.0);
    void setValue(float value = 0.0);
    void setValueRange(float minValue, float maxValue);
    boost::signals2::connection FocusLeaveConnect(const boost::signals2::slot<void()>& slot);
    boost::signals2::connection EnterKeyConnect(const boost::signals2::slot<void()>& slot);
    
private:
    void OnEditTextFinished(wxCommandEvent& event);
    void OnTextCtrlTextChanged(wxCommandEvent& event);
    void OnKeyHook(wxKeyEvent& event);
    void OnTextCtrlLostFocus(wxFocusEvent& event);
    void OnTextCtrlFocus(wxFocusEvent& event);
    bool inputValueIsVaild(const wxString& text);
   
private:
    AnkerTextCtrl* m_spinCtrl;
    AnkerStaticText* m_unitText;
    boost::regex m_regex;
    float m_interval = 1.0;
    float m_maxValue = 100000.0;
    float m_minValue = -100000.0;
    boost::signals2::signal<void()> m_focusLeaveSignal;
    boost::signals2::signal<void()> m_enterKeySignal;
};

class AnkerCustomSpinBox : public AnkerBox
{
public:
    AnkerCustomSpinBox(wxWindow* parent, const wxString& unit,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL, 
        AnkerSpinBoxCtrlDoubleType type = AnkerSpinBoxCtrlDoubleType_Unit);

    wxString getSpinBoxValue();
    wxString getSpinBox2BtnValue();
    void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    void setBorderColour(const wxColour& enterColour = wxColour(0, 255, 0), const wxColour& leaveColour = wxColour("#FFFFFF"));

    void setUnitTextCtrlFont(const wxFont& font);
    void setInputTextCtrlFont(const wxFont& font);
    void setInterval(float interval = 1.0);
    void setValue(float value = 0.0);
    void setValueRange(float minValue, float maxValue);

public:
    boost::signals2::connection FocusLeaveConnect(const boost::signals2::slot<void()>& slot);
    boost::signals2::connection EnterKeyConnect(const boost::signals2::slot<void()>& slot);

private:
    AnkerSpinBoxCtrlDoubleWithUnit* m_doubleSpinbox;
    AnkerSpinBoxCtrlDoubleWith2Btn* m_doubleSpinbox2Btn;
    AnkerSpinBoxCtrlDoubleType m_type;
};






#endif // !ANKER_SPINBOX_HPP_

#include "AnkerSpinBox.hpp"

AnkerSpinBoxCtrlDoubleWithUnit::AnkerSpinBoxCtrlDoubleWithUnit(wxWindow* parent, const wxString& unit, const wxPoint& pos, const wxSize& size, long style)
    : AnkerBox(parent, wxID_ANY, pos, size, style)

{
    wxBoxSizer* hrSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(hrSizer);

    wxColour bkColour = wxColour("#292A2D");
    if (parent) {
        bkColour = parent->GetBackgroundColour();
    }

    int interval = 1;
    int height = size.y - interval * 2;
    int unitWidth = height;
    wxPoint spinPos(interval, interval);
    int spinWidth = (float)size.x - 2 * interval - unitWidth;
    wxPoint unitPos(interval + spinWidth, interval);

    SetBackgroundColour(bkColour);
#ifdef _WIN32
    m_spinCtrl = new AnkerTextCtrl(this, wxID_ANY, wxEmptyString, spinPos, wxSize(spinWidth, height - interval), wxBORDER_NONE);

#elif __APPLE__
    m_spinCtrl = new AnkerTextCtrl(this, wxID_ANY, wxEmptyString, spinPos, wxSize(spinWidth, height - interval), wxSP_HORIZONTAL | wxALIGN_CENTRE_HORIZONTAL | wxBORDER_NONE);

#endif // _WIN32

    m_spinCtrl->SetLabelText("0.0");
    m_spinCtrl->SetBackgroundColour(bkColour);
    wxFont font = ANKER_FONT_NO_2;
    wxFont unitFont = ANKER_FONT_NO_2;
    m_spinCtrl->SetFont(font);

#ifdef _WIN32
    m_unitText = new AnkerStaticText(this, wxID_ANY, unit, unitPos, wxSize(unitWidth, height - 1), wxBORDER_NONE);
#elif __APPLE__
    m_unitText = new AnkerStaticText(this, wxID_ANY, unit, unitPos, wxSize(unitWidth, height - 1), wxSP_HORIZONTAL | wxALIGN_CENTRE_HORIZONTAL | wxBORDER_NONE);
#endif // _WIN32
     m_unitText->SetBackgroundColour(bkColour);
    m_unitText->SetFont(font);
    m_unitText->SetForegroundColour(wxColour("#999999"));
    wxSize unitSize = getTextSize(m_unitText, unit);
    m_unitText->SetSize(unitSize);
    m_unitText->SetPosition(wxPoint(size.x - unitSize.x - interval, interval));
    
    hrSizer->Add(m_spinCtrl, 0);
    hrSizer->Add(m_unitText, 0);

#ifdef _WIN32
    borderHighEnable();
#endif // _WIN32
}


wxString AnkerSpinBoxCtrlDoubleWithUnit::getValue()
{
    return m_spinCtrl->GetValue();
}

void AnkerSpinBoxCtrlDoubleWithUnit::setTextCtrlFont(const wxFont& font)
{
    m_spinCtrl->SetFont(font);    
    AnkerBox::setTextCtrlFont(font);
}

void AnkerSpinBoxCtrlDoubleWithUnit::setTextCtrlRegex(const std::string& reg)
{
    m_regex = reg;
}

void AnkerSpinBoxCtrlDoubleWithUnit::setUnitTextCtrlFont(const wxFont& font)
{
    m_spinCtrl->SetFont(font);
}

void AnkerSpinBoxCtrlDoubleWithUnit::setInputTextCtrlFont(const wxFont& font)
{
    m_unitText->SetFont(font);
}

void AnkerSpinBoxCtrlDoubleWithUnit::setInterval(float interval)
{
    m_interval = interval;
}

void AnkerSpinBoxCtrlDoubleWithUnit::setValue(float value)
{
    if (value >= m_minValue && value <= m_maxValue) {
        m_spinCtrl->SetLabel(wxString::Format("%.2f", value));
    }
}

void AnkerSpinBoxCtrlDoubleWithUnit::setValueRange(float minValue, float maxValue)
{
    m_maxValue = maxValue;
    m_minValue = minValue;
}

//void AnkerCustomSpinBox::BindFoucusLeaveConnect(VoidFunctionPtr functionPtr, wxControl* pCtrl)
//{
//    FocusLeaveConnect(boost::bind(functionPtr, pCtrl));
//}
//
//void AnkerCustomSpinBox::BindEnterKeyConnect(VoidFunctionPtr functionPtr, wxControl* pCtrl)
//{
//    EnterKeyConnect(boost::bind(functionPtr, pCtrl));
//}

boost::signals2::connection AnkerSpinBoxCtrlDoubleWithUnit::FocusLeaveConnect(const boost::signals2::slot<void()>& slot)
{
    return m_focusLeaveSignal.connect(slot);
}

boost::signals2::connection AnkerSpinBoxCtrlDoubleWithUnit::EnterKeyConnect(const boost::signals2::slot<void()>& slot)
{
    return m_enterKeySignal.connect(slot);
}

void AnkerSpinBoxCtrlDoubleWithUnit::setTextForegroundColour(const wxColour& colour)
{
    m_spinCtrl->SetForegroundColour(colour);
    m_unitText ->SetForegroundColour(colour);
    AnkerBox::SetForegroundColour(colour);
}

void AnkerSpinBoxCtrlDoubleWithUnit::setBackgroundColour(const wxColour& colour)
{
    m_spinCtrl->SetBackgroundColour(colour);
    m_unitText->SetBackgroundColour(colour);
    AnkerBox::SetBackgroundColour(colour);
}

void AnkerSpinBoxCtrlDoubleWithUnit::OnEditTextFinished(wxCommandEvent& event)
{
    wxString text = m_spinCtrl->GetValue();
    if (boost::regex_match(std::string(text.mb_str()), m_regex)) {
        setMouseEnterStatus(true);
    }
    else {
        m_spinCtrl->Clear();
    }
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlTextChanged(wxCommandEvent& event)
{
    if (m_spinCtrl) {
        wxString text = m_spinCtrl->GetValue();
        double value;
        if (!text.IsEmpty() && inputValueIsVaild(text)) {
            
        }
        else if (text.IsEmpty()) {

        }
        else {
            text.RemoveLast();
            m_spinCtrl->SetValue(text);
            m_spinCtrl->SetInsertionPointEnd();
        }

        // Focus leave.
        if (!m_spinCtrl->FindFocus()) {
            m_focusLeaveSignal();
        }
    }
    setMouseEnterStatus(true);
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWithUnit::OnKeyHook(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    if (keyCode == WXK_RETURN) {
        m_enterKeySignal();
    }
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlLostFocus(wxFocusEvent& event)
{
    m_focusLeaveSignal();
    event.Skip();
    Update();
    Refresh();
}

void AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlFocus(wxFocusEvent& event)
{
    setMouseEnterStatus(true);
    event.Skip();
}


void AnkerSpinBoxCtrlDoubleWithUnit::borderHighEnable(bool enable)
{
    if (enable) {
        m_spinCtrl->Bind(wxEVT_TEXT, &AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlTextChanged, this);
        m_spinCtrl->Bind(wxEVT_CHAR_HOOK, &AnkerSpinBoxCtrlDoubleWithUnit::OnKeyHook, this);
        m_spinCtrl->Bind(wxEVT_KILL_FOCUS, &AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlLostFocus, this);
        m_spinCtrl->Bind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseEnter, this);
        m_spinCtrl->Bind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseLeave, this);
        m_unitText->Bind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseEnter, this);
        m_unitText->Bind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseLeave, this);
        Bind(wxEVT_PAINT, &AnkerSpinBoxCtrlDoubleWithUnit::OnPaint, this);
    }
    else {
        m_spinCtrl->Unbind(wxEVT_TEXT, &AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlTextChanged, this);
        m_spinCtrl->Unbind(wxEVT_CHAR_HOOK, &AnkerSpinBoxCtrlDoubleWithUnit::OnKeyHook, this);
        m_spinCtrl->Unbind(wxEVT_KILL_FOCUS, &AnkerSpinBoxCtrlDoubleWithUnit::OnTextCtrlLostFocus, this);
        m_spinCtrl->Unbind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseEnter, this);
        m_spinCtrl->Unbind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseLeave, this);
        m_unitText->Unbind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseEnter, this);
        m_unitText->Unbind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWithUnit::OnMouseLeave, this);
        Unbind(wxEVT_PAINT, &AnkerSpinBoxCtrlDoubleWithUnit::OnPaint, this);
    }
    setBorderColour();
    setTextCtrlFont();
    setTextForegroundColour();
}

bool AnkerSpinBoxCtrlDoubleWithUnit::inputValueIsVaild(const wxString& text)
{
    double value;
    return text.ToDouble(&value) && (value >= m_minValue && value <= m_maxValue);
}

AnkerTextCtrl::AnkerTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value, 
    const wxPoint& pos, const wxSize& size, long style, 
    const wxValidator& validator, const wxString& name) :
    wxTextCtrl(parent, id, value, pos, size, style, validator, name)
{
#ifdef _WIN32
    SetWindowStyleFlag(wxNO_BORDER);
#endif // _WIN32

   
}

AnkerCustomSpinBox::AnkerCustomSpinBox(wxWindow* parent, const wxString& unit, const wxPoint& pos, const wxSize& size, long style, AnkerSpinBoxCtrlDoubleType type) :
    AnkerBox(parent), m_doubleSpinbox(nullptr), m_doubleSpinbox2Btn(nullptr), m_type(type)
{
    wxColour bkColour = wxColour("#292A2D");
    if (parent) {
        bkColour = parent->GetBackgroundColour();
     }
    SetBackgroundColour(bkColour);

    switch (type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        m_doubleSpinbox = new  AnkerSpinBoxCtrlDoubleWithUnit(this, unit, pos, size, style);
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        m_doubleSpinbox2Btn = new AnkerSpinBoxCtrlDoubleWith2Btn(this, pos, size, style);
        break;
    default:
        break;
    }
    
}


wxString AnkerCustomSpinBox::getSpinBoxValue()
{
   return m_doubleSpinbox->getValue();
}


wxString AnkerCustomSpinBox::getSpinBox2BtnValue()
{
    return m_doubleSpinbox->getValue();
}

void AnkerCustomSpinBox::setTextForegroundColour(const wxColour& colour)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setTextForegroundColour(colour);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setTextForegroundColour(colour);
        }
        break;
    default:
        break;
    }
    
}

void AnkerCustomSpinBox::setBackgroundColour(const wxColour& colour)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setBackgroundColour(colour);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setBackgroundColour(colour);
        }
        break;
    default:
        break;
    }
}

void AnkerCustomSpinBox::setBorderColour(const wxColour& enterColour, const wxColour& leaveColour)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setBorderColour(enterColour, leaveColour);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setBorderColour(enterColour, leaveColour);
        }
        break;
    default:
        break;
    }
}

boost::signals2::connection AnkerCustomSpinBox::FocusLeaveConnect(const boost::signals2::slot<void()>& slot)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            return m_doubleSpinbox->FocusLeaveConnect(slot);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            return m_doubleSpinbox2Btn->FocusLeaveConnect(slot);
        }
        break;
    default:
        break;
    }

    return boost::signals2::connection();
}

boost::signals2::connection AnkerCustomSpinBox::EnterKeyConnect(const boost::signals2::slot<void()>& slot)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            return  m_doubleSpinbox->EnterKeyConnect(slot);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            return m_doubleSpinbox2Btn->EnterKeyConnect(slot);
        }
        break;
    default:
        break;
    }
    return boost::signals2::connection();
}

void AnkerCustomSpinBox::setUnitTextCtrlFont(const wxFont& font)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setUnitTextCtrlFont(font);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
       
        break;
    default:
        break;
    }
}

void AnkerCustomSpinBox::setInputTextCtrlFont(const wxFont& font)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setInputTextCtrlFont(font);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setTextCtrlFont(font);
         }
        break;
    default:
        break;
    }
}

void AnkerCustomSpinBox::setInterval(float interval)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setInterval(interval);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setInterval(interval);
        }
        break;
    default:
        break;
    }
}

void AnkerCustomSpinBox::setValue(float value)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setValue(value);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setValue(value);
        }
        break;
    default:
        break;
    }
}

void AnkerCustomSpinBox::setValueRange(float minValue, float maxValue)
{
    switch (m_type)
    {
    case AnkerSpinBoxCtrlDoubleType_Unit:
        if (m_doubleSpinbox) {
            m_doubleSpinbox->setValueRange(minValue, maxValue);
        }
        break;
    case AnkerSpinBoxCtrlDoubleType_2Btn:
        if (m_doubleSpinbox2Btn) {
            m_doubleSpinbox2Btn->setValueRange(minValue, maxValue);
        }
        break;
    default:
        break;
    }
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnEditTextFinished(wxCommandEvent& event)
{

}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlTextChanged(wxCommandEvent& event)
{
    if (m_spinCtrl) {
        wxString text = m_spinCtrl->GetValue();
        double value;
        if (!text.IsEmpty() && inputValueIsVaild(text)) {

        }
        else if (text.IsEmpty()) {

        }
        else {
            text.RemoveLast();
            m_spinCtrl->SetValue(text);
            m_spinCtrl->SetInsertionPointEnd();
        }

        // Focus leave.
        if (!m_spinCtrl->FindFocus()) {
            m_focusLeaveSignal();
        }
    }
    setMouseEnterStatus(true);
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnKeyHook(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    if (keyCode == WXK_RETURN) {
        m_enterKeySignal();
    }
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlLostFocus(wxFocusEvent& event)
{
    m_focusLeaveSignal();
    event.Skip();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlFocus(wxFocusEvent& event)
{
}

bool AnkerSpinBoxCtrlDoubleWith2Btn::inputValueIsVaild(const wxString& text)
{
    double value;
    return text.ToDouble(&value) && (value >= m_minValue && value <= m_maxValue);
}

AnkerSpinBoxCtrlDoubleWith2Btn::AnkerSpinBoxCtrlDoubleWith2Btn(wxWindow* parent, const wxPoint& pos, const wxSize& size, long style) : 
    AnkerBox(parent, wxID_ANY, pos, size, style)
{
    wxBoxSizer* hrSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(hrSizer);

    wxColour bkColour = wxColour("#292A2D");
    if (parent) {
        bkColour = parent->GetBackgroundColour();
    }
    SetBackgroundColour(bkColour);

    int interval = 1;
    int btnImageWidth = 8;
    wxPoint spinPos(interval, interval);
    int height = size.y - interval * 2;
    int spinWidth = (float)size.x - 2 * interval - height;
    wxSize btnSize(height / 2 - interval, height / 2 - interval);

    wxBoxSizer* vBtnSizer = new wxBoxSizer(wxVERTICAL);
    wxPoint upBtnPos(interval + spinWidth, interval);  
    wxPoint downBtnPos(interval + spinWidth, btnSize.y + 2 * interval);
    wxImage upImage(AnkerBase::AnkerResourceIconPath + "spinbox_up.png", wxBITMAP_TYPE_PNG);
    upImage.Rescale(btnImageWidth, btnImageWidth);
    wxImage downImage(AnkerBase::AnkerResourceIconPath + "spinbox_down.png", wxBITMAP_TYPE_PNG);
    downImage.Rescale(btnImageWidth, btnImageWidth);

    m_upBtn = new AnkerButton(this, wxID_ANY, "", upBtnPos, btnSize, wxBORDER_NONE);
    m_upBtn->SetBitmap(upImage);
    m_upBtn->SetBackgroundColour(bkColour);
    m_upBtn->Bind(wxEVT_BUTTON, &AnkerSpinBoxCtrlDoubleWith2Btn::OnUpBtnClicked, this);
    vBtnSizer->Add(m_upBtn, 0);
    m_downBtn = new AnkerButton(this, wxID_ANY, "", downBtnPos, btnSize, wxBORDER_NONE);
    m_downBtn->SetBitmap(downImage);
    m_downBtn->SetBackgroundColour(bkColour);
    m_downBtn->Bind(wxEVT_BUTTON, &AnkerSpinBoxCtrlDoubleWith2Btn::OnDownBtnClicked, this);
    vBtnSizer->Add(m_downBtn, 0);

#ifdef _WIN32

    m_spinCtrl = new AnkerTextCtrl(this, wxID_ANY, wxEmptyString, spinPos, wxSize(spinWidth, height - 1), wxBORDER_NONE);

#elif __APPLE__

    m_spinCtrl = new AnkerTextCtrl(this, wxID_ANY, wxEmptyString, spinPos, wxSize(spinWidth, height - 1), wxSP_HORIZONTAL | wxALIGN_CENTRE_HORIZONTAL | wxBORDER_NONE);

#endif // _WIN32
    m_spinCtrl->SetLabelText("0.0");
    m_spinCtrl->SetBackgroundColour(bkColour);
    //wxFont font = _AnkerFont(14);
    wxFont font = ANKER_FONT_NO_1;
    m_spinCtrl->SetFont(font);

    hrSizer->Add(m_spinCtrl, 0);
    hrSizer->Add(vBtnSizer, 0);
#ifdef _WIN32
    borderHighEnable();
#endif // _WIN32
}


wxString AnkerSpinBoxCtrlDoubleWith2Btn::getValue()
{
    return m_spinCtrl->GetValue();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::borderHighEnable(bool enable)
{
    if (enable) {
        m_spinCtrl->Bind(wxEVT_TEXT, &AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlTextChanged, this);  
        m_spinCtrl->Bind(wxEVT_CHAR_HOOK, &AnkerSpinBoxCtrlDoubleWith2Btn::OnKeyHook, this);  
        m_spinCtrl->Bind(wxEVT_KILL_FOCUS, &AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlLostFocus, this);
        m_spinCtrl->Bind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_spinCtrl->Bind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        m_upBtn->Bind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_upBtn->Bind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        m_downBtn->Bind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_downBtn->Bind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        Bind(wxEVT_PAINT, &AnkerSpinBoxCtrlDoubleWith2Btn::OnPaint, this);
    }
    else {
        m_spinCtrl->Unbind(wxEVT_TEXT, &AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlTextChanged, this);
        m_spinCtrl->Unbind(wxEVT_CHAR_HOOK, &AnkerSpinBoxCtrlDoubleWith2Btn::OnKeyHook, this);
        m_spinCtrl->Unbind(wxEVT_KILL_FOCUS, &AnkerSpinBoxCtrlDoubleWith2Btn::OnTextCtrlLostFocus, this);
        m_spinCtrl->Unbind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_spinCtrl->Unbind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        m_upBtn->Unbind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_upBtn->Unbind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        m_downBtn->Unbind(wxEVT_ENTER_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseEnter, this);
        m_downBtn->Unbind(wxEVT_LEAVE_WINDOW, &AnkerSpinBoxCtrlDoubleWith2Btn::OnMouseLeave, this);
        Unbind(wxEVT_PAINT, &AnkerSpinBoxCtrlDoubleWith2Btn::OnPaint, this);
    }
    setBorderColour();
    setTextCtrlFont();
    setTextForegroundColour();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setTextCtrlFont(const wxFont& font)
{
    m_spinCtrl->SetFont(font);
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setTextForegroundColour(const wxColour& colour)
{
    m_spinCtrl->SetForegroundColour(colour);
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setBackgroundColour(const wxColour& colour)
{
    m_spinCtrl->SetBackgroundColour(colour);
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setInterval(float interval)
{
    m_interval = interval;
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setValue(float value)
{
    if (value >= m_minValue && value <= m_maxValue) {
        m_spinCtrl->SetLabel(wxString::Format("%.2f", value));
    }
}

void AnkerSpinBoxCtrlDoubleWith2Btn::setValueRange(float minValue, float maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
}

boost::signals2::connection AnkerSpinBoxCtrlDoubleWith2Btn::FocusLeaveConnect(const boost::signals2::slot<void()>& slot)
{
    return m_focusLeaveSignal.connect(slot);
}

boost::signals2::connection AnkerSpinBoxCtrlDoubleWith2Btn::EnterKeyConnect(const boost::signals2::slot<void()>& slot)
{
    return m_enterKeySignal.connect(slot);
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnUpBtnClicked(wxCommandEvent& event)
{
    double value = 0.0;
    if (m_spinCtrl->GetValue().ToDouble(&value)) {
        value += m_interval;
        wxString text = wxString::Format("%.2f", value);
        m_spinCtrl->SetLabel(text);
   }
   Update();
   Refresh();
}

void AnkerSpinBoxCtrlDoubleWith2Btn::OnDownBtnClicked(wxCommandEvent& event)
{
    double value = 0.0;
    if (m_spinCtrl->GetValue().ToDouble(&value)) {
        value -= m_interval;
        wxString text = wxString::Format("%.2f", value);
        m_spinCtrl->SetLabel(text);
    }
    Update();
    Refresh();
}

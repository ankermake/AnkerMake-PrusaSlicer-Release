#include "AnkerLineEditUnit.hpp"
#include <wx/artprov.h>
#include <wx/univ/theme.h>
#include "libslic3r/Utils.hpp"
#include "../GUI_App.hpp"
#include "AnkerGUIConfig.hpp"

AnkerLineEditUnit::AnkerLineEditUnit(wxWindow* parent, 
									wxString unit,
									wxFont unitFont,
									wxColour bgColor,
									wxColour borderColor,
									int radio,
									wxWindowID winid /*= wxID_ANY*/,
									const wxPoint& pos /*= wxDefaultPosition*/,
									const wxSize& size /*= wxDefaultSize*/)
									: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
									, m_unit(unit)
									, m_unitFont(unitFont)
{
	SetBackgroundColour(bgColor);
	initUi(bgColor, borderColor, radio);
	Bind(wxEVT_SIZE, &AnkerLineEditUnit::OnSize, this);
}

AnkerLineEditUnit::AnkerLineEditUnit()
{

}

AnkerLineEditUnit::~AnkerLineEditUnit()
{

}

wxString AnkerLineEditUnit::getValue()
{
	return m_pLineEdit->GetValue();
}

wxString AnkerLineEditUnit::GetValue()
{
	return getValue();
}

void AnkerLineEditUnit::setValue(const wxString& value)
{
	m_pLineEdit->SetValue(value);
}


void AnkerLineEditUnit::SetValue(const wxString& value)
{
	wxCriticalSectionLocker locker(m_pThreadCS);
	m_pLineEdit->SetValue(value);
}

void AnkerLineEditUnit::setEditLength(const int& length)
{
	m_pLineEdit->SetMaxLength(length);
}

void AnkerLineEditUnit::setLineEditFont(const wxFont& font)
{
	m_pLineEdit->SetFont(font);
}

void AnkerLineEditUnit::setLineEditTextColor(const wxColour& color)
{
	m_pLineEdit->SetForegroundColour(color);
	Refresh();
}

void AnkerLineEditUnit::setLineUnitTextColor(const wxColour& color)
{
	m_pUnitLabel->SetForegroundColour(color);
	Refresh();
}

void AnkerLineEditUnit::setLineEditBgColor(const wxColour& color)
{
	m_pLineEdit->SetBackgroundColour(color);
	Refresh();
}

void AnkerLineEditUnit::setLineUnitBgColor(const wxColour& color)
{
	m_pUnitLabel->SetBackgroundColour(color);
	Refresh();
}

wxRichTextCtrl* AnkerLineEditUnit::getTextEdit()
{
	return m_pLineEdit;
}

wxRichTextCtrl* AnkerLineEditUnit::getUnitEdit()
{
	return m_pUnitLabel;
}

void AnkerLineEditUnit::OnSize(wxSizeEvent& event)
{
#ifdef __WXOSX__
	int shrink = 10;
#else
	int shrink = 4;
#endif
	int height = GetSize().y;
	int editHeight = height - shrink;

	int unitEditWidth = 0;
	if (!m_unit.empty()) {
		int unitTextWidth = m_pUnitLabel->GetTextExtent(m_unit).x;
#ifdef __APPLE__
		int textMargin = unitTextWidth / AnkerLength(4);
#else
		int textMargin = unitTextWidth / AnkerLength(3);
#endif
		if (textMargin < 5) textMargin = 5;
		unitEditWidth = AnkerLength(unitTextWidth) + textMargin;
	}

	m_pUnitLabel->SetMaxSize(wxSize(unitEditWidth, editHeight));
	m_pUnitLabel->SetMinSize(wxSize(unitEditWidth, editHeight));
	m_pUnitLabel->SetSize(wxSize(unitEditWidth, editHeight));

	m_pLineEdit->SetMinSize(wxSize(-1, editHeight));
	m_pLineEdit->SetMaxSize(wxSize(-1, editHeight));
	m_pLineEdit->SetSize(wxSize(-1, editHeight));

	Layout();
	Refresh();
}


bool AnkerLineEditUnit::Enable(bool enable)
{
	if (enable) {
		m_pLineEdit->SetForegroundColour(wxColour(255, 255, 255));
	}
	else {
		m_pLineEdit->SetForegroundColour(wxColour(80, 80, 80));
	}
	bool ret = wxControl::Enable(enable);
	Refresh();
	return ret;
}

bool AnkerLineEditUnit::Disable()
{
	bool ret = Enable(false);
	Refresh();
	return ret;
}


void AnkerLineEditUnit::initUi(wxColour bgColor, wxColour borderColor, int radio)
{
	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxVERTICAL);
	m_pBgWidget = new AnkerBgPanel(this, bgColor, borderColor, radio, wxID_ANY);
	wxBoxSizer* pBgWidgetHSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pBgWidget->SetSizer(pBgWidgetHSizer);

	wxClientDC dc(this);
	dc.SetFont(m_unitFont);
	wxSize textSize = dc.GetTextExtent(m_unit);
	int textWidth = textSize.GetWidth();

	m_pLineEdit = new AnkerLineEdit(m_pBgWidget, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
//#ifndef _WIN32	
//	m_pLineEdit->SetWindowStyle(wxBORDER_THEME);
//#endif // _WIN32	
	m_pLineEdit->SetBackgroundColour(bgColor);
	m_pLineEdit->SetForegroundColour(wxColour("#FFFFFF"));
	m_pLineEdit->SetFont(ANKER_FONT_NO_1);

	m_pLineEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this](wxCommandEvent& event) {
		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_FINISHED);
		ProcessEvent(evt);
		});


	// use AnkerLineEdit for unit to fix bad display when AnkerLineEditUnit is disable
	m_pUnitLabel = new AnkerLineEdit(m_pBgWidget, wxID_ANY, m_unit, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxNO_BORDER);
	m_pUnitLabel->Enable(false);
	m_pUnitLabel->SetFont(ANKER_FONT_NO_2);
	m_pUnitLabel->SetBackgroundColour(bgColor);
	m_pUnitLabel->SetForegroundColour(wxColour("#999999"));

	pBgWidgetHSizer->AddSpacer(5);
	pBgWidgetHSizer->Add(m_pLineEdit, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);

    if(!m_unit.empty()){
        //pBgWidgetHSizer->AddSpacer(3);

#ifndef  __APPLE__
        pBgWidgetHSizer->Add(m_pUnitLabel, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
#else
        pBgWidgetHSizer->Add(m_pUnitLabel, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
#endif // ! __APPLE__
	}
	else
	{
		m_pUnitLabel->Hide();
	}
	
	pBgWidgetHSizer->AddSpacer(3);

	pMainHSizer->Add(m_pBgWidget, 1, wxEXPAND | wxALL, 0);
	
	SetSizer(pMainHSizer);
}


AnkerBgPanel::AnkerBgPanel(wxWindow* parent, 
						wxColour bgColor,
						wxColour borderColor,
						int radio,
						wxWindowID id /*= wxID_ANY*/, 
						const wxPoint& pos /*= wxDefaultPosition*/,
						const wxSize& size /*= wxDefaultSize*/)
						: wxPanel(parent, id, pos, size)
						, m_bgColor(bgColor)
						, m_borderColor(borderColor)
						, m_radio(radio)
{
	SetBackgroundColour(bgColor); 
	Bind(wxEVT_PAINT, &AnkerBgPanel::OnPaint, this);
}


void AnkerBgPanel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (gc)
	{
		gc->SetPen(wxPen(m_borderColor, 2));
		gc->SetBrush(wxBrush(m_bgColor)); 

		wxRect rect = GetClientRect();
		wxGraphicsPath path = gc->CreatePath();
		path.AddRoundedRectangle(0, 0, rect.width, rect.height, m_radio);

		gc->DrawPath(path);
		delete gc;
	}
}

AnkerSpinEdit::AnkerSpinEdit()
{

}
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED, wxCommandEvent);
AnkerSpinEdit::AnkerSpinEdit(wxWindow* parent,
							wxColour bgColor,
							wxColour borderColor, 
							int radio, 
							wxString unit /*= ""*/,
							wxWindowID winid /*= wxID_ANY*/,
							const wxPoint& pos /*= wxDefaultPosition*/,
							const wxSize& size /*= wxDefaultSize*/)
							: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
							, m_unit(unit)
{
	SetBackgroundColour(bgColor);
	initUi(bgColor, borderColor, radio);
	Bind(wxEVT_SIZE, &AnkerSpinEdit::OnSize, this);
}

AnkerSpinEdit::~AnkerSpinEdit()
{

}

wxString AnkerSpinEdit::getValue()
{
	return m_pLineEdit->GetValue();
}

wxString AnkerSpinEdit::GetValue()
{
	return m_pLineEdit->GetValue();
}

/*
int AnkerSpinEdit::GetValue()
{
	return m_val;
}
*/

void AnkerSpinEdit::setValue(const wxString& value)
{
	m_val = wxVariant(value).GetInteger();
	m_pLineEdit->SetValue(value);
}


void AnkerSpinEdit::SetValue(const int value)
{
	m_val = value;
	m_pLineEdit->SetValue(wxVariant(value).GetString());
}

void AnkerSpinEdit::SetValue(const wxString& value)
{
	m_val = wxVariant(value).GetInteger();
	setValue(value);
}


int AnkerSpinEdit::GetMinValue()
{
	return m_minVal;
}

void AnkerSpinEdit::SetMinValue(const int value)
{
	m_minVal = value;
}

int AnkerSpinEdit::GetMaxValue()
{
	return m_maxVal;
}

void AnkerSpinEdit::SetMaxValue(const int value)
{
	m_maxVal = value;
}

void AnkerSpinEdit::setEditLength(const int& length)
{
	m_pLineEdit->SetMaxLength(length);
}

void AnkerSpinEdit::setLineEditFont(const wxFont& font)
{
	m_pLineEdit->SetFont(font);
}

void AnkerSpinEdit::setLineEditTextColor(const wxColour& color)
{
	m_pLineEdit->SetForegroundColour(color);
}

void AnkerSpinEdit::setLineEditBgColor(const wxColour& color)
{
	m_pLineEdit->SetBackgroundColour(color);
}

AnkerLineEdit* AnkerSpinEdit::getTextEdit()
{
	return m_pLineEdit;
}

AnkerLineEdit* AnkerSpinEdit::getUnitEdit()
{
	return m_pUnitLabel;
}

void AnkerSpinEdit::OnSize(wxSizeEvent& event)
{
#ifdef __WXOSX__
	int shrink = 10;
#else
	int shrink = 4;
#endif
	int height = GetSize().y;
	int editHeight = height - shrink;
	//getUnitEdit()->SetMinSize(wxSize(-1, height - shrink));
	//getUnitEdit()->SetMaxSize(wxSize(-1, height - shrink));
	//getUnitEdit()->SetSize(wxSize(-1, height - shrink));

	int unitEditWidth = 0;
	if (!m_unit.empty()) {
		int unitTextWidth = m_pUnitLabel->GetTextExtent(m_unit).x;
		int textMargin = 6;
		unitEditWidth = unitTextWidth + textMargin;
	}

	m_pUnitLabel->SetMaxSize(wxSize(unitEditWidth, editHeight));
	m_pUnitLabel->SetMinSize(wxSize(unitEditWidth, editHeight));
	m_pUnitLabel->SetSize(wxSize(unitEditWidth, editHeight));

	m_pLineEdit->SetMinSize(wxSize(-1, editHeight));
	m_pLineEdit->SetMaxSize(wxSize(-1, editHeight));
	m_pLineEdit->SetSize(wxSize(-1, editHeight));

	Layout();
	Refresh();
}

bool AnkerSpinEdit::Enable(bool enable)
{
	if (enable) {
		m_pLineEdit->SetForegroundColour(wxColour(255, 255, 255));
	}
	else {
		m_pLineEdit->SetForegroundColour(wxColour(80, 80, 80));
	}
	bool ret = wxControl::Enable(enable);
	Refresh();
	return ret;
}

bool AnkerSpinEdit::Disable()
{
	bool ret = Enable(false);
	Refresh();
	return ret;
}


void AnkerSpinEdit::initUi(wxColour bgColor, wxColour borderColor, int radio)
{
	wxBoxSizer* pMainHSizer = new wxBoxSizer(wxVERTICAL);
	m_pBgWidget = new AnkerBgPanel(this, bgColor, borderColor, radio, wxID_ANY);
	//m_pBgWidget->SetMaxSize(wxSize(-1, 28));
	//m_pBgWidget->SetMinSize(wxSize(-1, 28));
	//m_pBgWidget->SetSize(wxSize(-1, 28));
	wxBoxSizer* pBgWidgetHSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pBgWidget->SetSizer(pBgWidgetHSizer);

	m_pLineEdit = new AnkerLineEdit(m_pBgWidget, wxID_ANY,"", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
	m_pLineEdit->SetBackgroundColour(bgColor);
	m_pLineEdit->SetForegroundColour(wxColour("#FFFFFF"));
	//m_pLineEdit->SetMaxSize(wxSize(110 - 21, -1));
	//m_pLineEdit->SetMinSize(wxSize(110 - 21, -1));
	//m_pLineEdit->SetSize(wxSize(110 - 21, -1));
	
	// use AnkerLineEdit for unit to fix bad display when AnkerLineEditUnit is disable
	m_pUnitLabel = new AnkerLineEdit(m_pBgWidget, wxID_ANY, m_unit, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_pUnitLabel->Enable(false);
	m_pUnitLabel->SetForegroundColour(wxColour("#999999"));
	m_pUnitLabel->SetBackgroundColour(bgColor);
	m_pUnitLabel->SetFont(ANKER_FONT_NO_2);

	m_pLineEdit->Bind(wxCUSTOMEVT_EDIT_FINISHED, [this](wxCommandEvent& event) {
		wxCommandEvent evtEdit = wxCommandEvent(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED);
		evtEdit.SetEventObject(this);
		ProcessEvent(evtEdit);

		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_EDIT_FINISHED);
		ProcessEvent(evt);
		});

	m_pLineEdit->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
		bool valueChanged = false;
		int keyCode = event.GetKeyCode();
		if (keyCode == WXK_UP) {
			int tmpVal = std::stoi(GetValue().ToStdString());
			++tmpVal;
			valueChanged = true;
			SetValue(tmpVal);
		}
		else if (keyCode == WXK_DOWN) {
			int tmpVal = std::stoi(GetValue().ToStdString());
			--tmpVal;
			valueChanged = true;
			SetValue(tmpVal);
		}
		else {
			event.Skip();
		}

		if (valueChanged) {
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED);
			evt.SetEventObject(this);
			ProcessEvent(evt);
		}
	});


	wxImage upBtnImage(wxString::FromUTF8(Slic3r::var("spinbox_up.png")), wxBITMAP_TYPE_PNG);
	upBtnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
	wxBitmap scaledUpBitmap(upBtnImage);
	wxImage downBtnImage(wxString::FromUTF8(Slic3r::var("spinbox_down.png")), wxBITMAP_TYPE_PNG);
	downBtnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
	wxBitmap scaledDownBitmap(downBtnImage);

#ifndef __APPLE__

	m_upBtn = new wxButton(m_pBgWidget, wxID_ANY, "");
	m_upBtn->SetWindowStyleFlag(wxBORDER_NONE);
	m_upBtn->SetBackgroundColour(bgColor);

	m_downBtn = new wxButton(m_pBgWidget, wxID_ANY, "");
	m_downBtn->SetWindowStyleFlag(wxBORDER_NONE);
	m_downBtn->SetBackgroundColour(bgColor);

	m_upBtn->SetBitmap(scaledUpBitmap);
	m_downBtn->SetBitmap(scaledDownBitmap);
#else
	m_upBtn = new ScalableButton(this, wxID_ANY, "spinbox_up", "", wxSize(8, 8));
	m_downBtn = new ScalableButton(this, wxID_ANY, "spinbox_down", "", wxSize(8, 8));
#endif
	
	
	m_upBtn->SetMaxSize(wxSize(8,8));
	m_upBtn->SetMinSize(wxSize(8,8));
	m_upBtn->SetSize(wxSize(8,8));

	m_downBtn->SetMaxSize(wxSize(8, 8));
	m_downBtn->SetMinSize(wxSize(8, 8));
	m_downBtn->SetSize(wxSize(8, 8));

	m_upBtn->Bind(wxEVT_BUTTON, &AnkerSpinEdit::onUpBtnClicked, this);
	m_downBtn->Bind(wxEVT_BUTTON, &AnkerSpinEdit::onDownBtnClicked, this);

	wxBoxSizer* pBtnVSizer = new wxBoxSizer(wxVERTICAL);
	pBtnVSizer->AddStretchSpacer(1);
	pBtnVSizer->Add(m_upBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBtnVSizer->AddSpacer(2);
	pBtnVSizer->Add(m_downBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	pBtnVSizer->AddStretchSpacer(1);


	pBgWidgetHSizer->AddSpacer(3);
	pBgWidgetHSizer->Add(m_pLineEdit, 1 , wxEXPAND|wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL , 0);
	if (!m_unit.empty()) {
		pBgWidgetHSizer->Add(m_pUnitLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
	}
	else {
		m_pUnitLabel->Hide();
	}
	pBgWidgetHSizer->AddSpacer(3);
	pBgWidgetHSizer->Add(pBtnVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL , 0);
	pBgWidgetHSizer->AddSpacer(3);

	pMainHSizer->Add(m_pBgWidget, wxEXPAND | wxALL, wxEXPAND | wxTOP, 0);

	SetSizer(pMainHSizer);
}

void AnkerSpinEdit::onUpBtnClicked(wxCommandEvent& event)
{
	wxString data = "";	
	data = m_pLineEdit->GetValue().ToStdWstring();

	int value = 0;
	data.ToInt(&value);
	value = value + 1;
	
	m_pLineEdit->SetValue(wxString::Format("%d", value));

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED);
	evt.SetEventObject(this);
	ProcessEvent(evt);
}

void AnkerSpinEdit::onDownBtnClicked(wxCommandEvent& event)
{
	wxString data = "";
	data = m_pLineEdit->GetValue().ToStdWstring();
	int value = 0;

	data.ToInt(&value);
	value = value - 1;

	if (value < 0)
	{
		//if (m_title != _L("Ooze prevention"))
		value = 0;
	}

	m_pLineEdit->SetValue(wxString::Format("%d", value));

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED);
	evt.SetEventObject(this);
	ProcessEvent(evt);
}


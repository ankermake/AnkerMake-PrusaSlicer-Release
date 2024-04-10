#ifndef _ANKER_LINE_EDIT_UINIT_HPP_
#define _ANKER_LINE_EDIT_UINIT_HPP_

#include "wx/wx.h"
#include "wx/control.h"
#include "wx/sizer.h"
#include <wx/dc.h>
#include <wx/graphics.h>
#include "../wxExtensions.hpp"
#include "../AnkerLineEdit.hpp"


class AnkerBgPanel;

class AnkerLineEditUnit : public wxControl
{

public:
	AnkerLineEditUnit();
	AnkerLineEditUnit(wxWindow* parent,
		wxString unit,
		wxFont unitFont,
		wxColour bgColor,
		wxColour borderColor,		
		int radio,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerLineEditUnit();
	
	void SetToolTip(const wxString& toolTipsStr);
	wxString getValue();
	wxString GetValue();
	void setValue(const wxString& value);
	void SetValue(const wxString& value);

	void setEditLength(const int& length);
	void setLineEditFont(const wxFont& font);

	void setLineEditTextColor(const wxColour& color);
	void setLineUnitTextColor(const wxColour& color);
	void setLineEditBgColor(const wxColour& color);
	void setLineUnitBgColor(const wxColour& color);

	void AddValidatorInt(uint32_t min, uint32_t max);
	void AddValidatorFloat(float min, float max, int precision = 6);

	wxRichTextCtrl* getTextEdit();
	wxStaticText* getUnitEdit();

	virtual void OnSize(wxSizeEvent& event);

	bool Enable(bool enable = true);
	bool Disable();
	wxString getUnit() {return m_unit;}
protected:

	void initUi(wxColour bgColor,
				wxColour borderColor,
				int radio);
private:
	AnkerBgPanel*  m_pBgWidget{nullptr};
	AnkerLineEdit*	   m_pLineEdit{nullptr};
	
	wxCriticalSection m_pThreadCS;
	wxFont		   m_unitFont;
	wxString	   m_unit{""};
	wxStaticText*  m_pUnitLabel{ nullptr };
};

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_SPIN_EDIT_TEXT_CHANGED, wxCommandEvent);
class AnkerSpinEdit : public wxControl
{

public:
	AnkerSpinEdit();
	AnkerSpinEdit(wxWindow* parent,				
		wxColour bgColor,
		wxColour borderColor,
		int radio,
		wxString unit = "",
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~AnkerSpinEdit();

	wxString getValue();
	wxString GetValue();
	//int  GetValue();
	void setValue(const wxString& value);
	void SetValue(const wxString& value);
	void SetValue(const int value);

	int  GetMinValue();
	void SetMinValue(const int value);

	int  GetMaxValue();
	void SetMaxValue(const int value);

	void AddValidatorInt(int min, int max);

	void setEditLength(const int& length);
	void setLineEditFont(const wxFont& font);
	void setLineEditTextColor(const wxColour& color);	
	void setLineEditBgColor(const wxColour& color);
	
	AnkerLineEdit* getTextEdit();
	wxStaticText* getUnitEdit();
	virtual void OnSize(wxSizeEvent& event);

	bool Enable(bool enable = true);
	bool Disable();

protected:

	void initUi(wxColour bgColor,
				wxColour borderColor,
				int radio);

	void onUpBtnClicked(wxCommandEvent &event);
	void onDownBtnClicked(wxCommandEvent& event);
private:
	AnkerBgPanel* m_pBgWidget{ nullptr };
	AnkerLineEdit*   m_pLineEdit{ nullptr };
	wxStaticText* m_pUnitLabel{ nullptr };

	wxString		m_unit;

	int m_val;
	int m_minVal;
	int m_maxVal;
	bool m_minEnable = false;
	bool m_maxEnable = false;

#ifndef __APPLE__
	wxButton* m_upBtn{ nullptr };
	wxButton* m_downBtn{ nullptr };
#else
	ScalableButton* m_upBtn{ nullptr };
	ScalableButton* m_downBtn{ nullptr };	
#endif // !__APPLE__

	
};

class AnkerBgPanel : public wxPanel
{
public:
	AnkerBgPanel(wxWindow* parent,
				wxColour bgColor,
				wxColour borderColor,
				int radio,
				wxWindowID id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize);
	
protected:

private:
	virtual void OnPaint(wxPaintEvent& event);

	wxColour m_bgColor;
	wxColour m_borderColor;
	int m_radio = 0;
	

};

#endif // !_ANKER_LINE_EDIT_UINIT_HPP_





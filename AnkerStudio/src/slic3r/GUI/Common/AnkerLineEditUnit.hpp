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

	wxRichTextCtrl* getTextEdit();
	wxRichTextCtrl* getUnitEdit();

	virtual void OnSize(wxSizeEvent& event);

	bool Enable(bool enable = true);
	bool Disable();
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
	AnkerLineEdit*  m_pUnitLabel{ nullptr };
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

	void setEditLength(const int& length);
	void setLineEditFont(const wxFont& font);
	void setLineEditTextColor(const wxColour& color);	
	void setLineEditBgColor(const wxColour& color);
	
	AnkerLineEdit* getTextEdit();
	AnkerLineEdit* getUnitEdit();
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
	AnkerLineEdit* m_pUnitLabel{ nullptr };

	wxString		m_unit;

	int m_val;
	int m_minVal;
	int m_maxVal;

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





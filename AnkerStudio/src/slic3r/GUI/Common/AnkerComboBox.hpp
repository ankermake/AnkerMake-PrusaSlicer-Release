#ifndef ANKER_COMBOX_HPP_
#define ANKER_COMBOX_HPP_

#include <wx/wx.h>
#include <wx/odcombo.h>
#include "AnkerBox.hpp"
#include "AnkerButton.hpp"
#include "../wxExtensions.hpp"

class AnkerComboBoxCtrl	: public wxComboBox, public AnkerBase
{
public:
    AnkerComboBoxCtrl();
	AnkerComboBoxCtrl(wxWindow* parent, wxWindowID id,
        const wxString& value = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,        
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxComboBoxNameStr));
	~AnkerComboBoxCtrl();

    void OnPaint(wxPaintEvent& event);

protected:
    void DoMoveWindow(int x, int y, int width, int height) override
    {
        wxComboBox::DoMoveWindow(x, y, width, height);
        SetScrollbar(wxHORIZONTAL, 0, 0, 0);
    }

private:
};

class AnkerComboBox : public AnkerBox
{
public:
    AnkerComboBox();
    AnkerComboBox(wxWindow* parent, wxWindowID id,
        const wxString& value = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxComboBoxNameStr));

    virtual void borderHighEnable(bool enable = true);
    virtual void setTextCtrlFont(const wxFont& font = ANKER_FONT_NO_1);
    virtual void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    virtual void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    void setRightBtnIcon(const wxIcon& icon);

    int getCurrentIndex();
    wxString getCurrentStr();
    void setComboxTextFont(const wxFont& textFont);
    void removeComboxItem(const int& index);
    void removeComboxItem(const wxString& itemName);
    void SetSelection(const int& index);
    void readOnly(bool isReadOnly);
    void AppendItem(const wxString& itemNmae);
    void AppendItem(const wxStringList& itemList);
    void ClearAllItem();
private:
    void OnButtonClicked(wxCommandEvent& event);

private:
    AnkerComboBoxCtrl* m_comboboxCtrl;
    AnkerButton * m_rightButton;

};

class AnkerPenStyleCombox : public  wxOwnerDrawnComboBox, public AnkerBase
{
public:

    void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));

    int getCurrentIndex();
    wxString getCurrentStr() const;
    void setComboxTextFont(const wxFont& textFont);
    void removeComboxItem(const int& index);
    void removeComboxItem(const wxString& itemName);
    void setSelection(const int& index);
    void readOnly(bool isReadOnly);
    void AppendItem(const wxString& itemNmae);
    void AppendItem(const wxStringList& itemList);
    void ClearAllItem();

protected:
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const;
    virtual void OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const;
    virtual void DrawButton(wxDC& dc, const wxRect& rect, int flags = Button_BitmapOnly);

private:
    wxColour m_textForegroundColor;
    wxColour m_backgroundColor;
};

class AnkerCustomComboBox : public AnkerBox
{
public:
    AnkerCustomComboBox(wxWindow* parent, wxWindowID id,
        const wxString& value = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,        
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxComboBoxNameStr));
    
    void setTextForegroundColour(const wxColour& colour = wxColour("#FFFFFF"));
    void setBackgroundColour(const wxColour& colour = wxColour("#292A2D"));
    //void setBorderColour(const wxColour& enterColour = wxColour(0, 255, 0), const wxColour& leaveColour = wxColour("#FFFFFF"));

    wxString getStrFromIndex(const int& index);
    int getCurrentIndex();
    wxString getCurrentStr();
    void SetSelection(const int& index);
    void setComboxTextFont(const wxFont& font);
	void removeComboxItem(const int& index);
	void removeComboxItem(const wxString& itemName);	
	void readOnly(bool isReadOnly);
	void AppendItem(const wxString& itemName);
	void AppendItem(const wxStringList& itemList);
	void ClearAllItem();

private:
    AnkerPenStyleCombox* m_comboBox;
};


#endif // !ANKER_COMBOX_HPP_

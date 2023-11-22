#ifndef _ANKER_HINT_H_
#define _ANKER_HINT_H_

#include "wx/wx.h"

#ifdef __APPLE__
class AnkerHint : public wxFrame
#else
class AnkerHint : public wxDialog
#endif
{
public:
    using OkBtnEventCallBack_T = std::function<void(bool isChecked)>;
    AnkerHint(const wxString& title, wxWindow* parent = nullptr);
    ~AnkerHint();
    void InitUI(int width, int height);
    void SetCallBack(OkBtnEventCallBack_T okCallback) { m_okBtnCallback = okCallback; }

private:
    wxString m_message;
    wxStaticText* m_msgText{ nullptr };
    OkBtnEventCallBack_T m_okBtnCallback;
};


#endif

#ifndef _ANKER_MSG_CEMTRE_DIALOG_hpp_
#define _ANKER_MSG_CEMTRE_DIALOG_hpp_

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/richtext/richtextctrl.h>
#include "wx/wx.h"
#include <iostream>
#include <vector>
#include "AnkerNetBase.h"
#include "Common/AnkerLoadingMask.hpp"
#include "AnkerBtn.hpp"

class AnkerBtn;
class AnkerCustomMsg;
enum PageBtnStatus
{
    PageNormalBtnStatus,
    PagePressedBtnStatus,    
    PageDisableBtnStatus
};
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_CHECK_BOX_CLICKED, wxCommandEvent);
class AnkerMsgCheckBox :public wxControl
{
    DECLARE_DYNAMIC_CLASS(AnkerMsgCheckBox)
    DECLARE_EVENT_TABLE()
public:
    AnkerMsgCheckBox();
    virtual ~AnkerMsgCheckBox();
    AnkerMsgCheckBox(wxWindow* parent, wxWindowID id,
        wxString label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);

    void resetStatus();
    void setText(wxString label);

    bool getStatus() const {return m_isCheck;}
protected:
    void init();
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnPressed(wxMouseEvent& event);
private:
    wxString m_label = "";
    bool m_isCheck = false;
};

wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_PAGE_BTN_CLICKED, wxCommandEvent);
class AnkerMsgPageBtn :public wxControl
{
    DECLARE_DYNAMIC_CLASS(AnkerMsgPageBtn)
    DECLARE_EVENT_TABLE()
public:
    AnkerMsgPageBtn();
    virtual ~AnkerMsgPageBtn();
    AnkerMsgPageBtn(wxWindow* parent, wxWindowID id,
        wxImage norImg,
        wxImage disImg,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);

    void setSatus(PageBtnStatus btnStatus);
    void setBgColor(wxColor color);
    virtual void OnPressed(wxMouseEvent& event);
protected:

    void init();
    void OnPaint(wxPaintEvent& event);
private:
    wxImage   m_norImg;
    wxImage   m_disImg;
    wxColor   m_bgColor;
    PageBtnStatus m_status;

};

class AnkerMsgCircleLabel :public wxControl
{
    DECLARE_DYNAMIC_CLASS(AnkerMsgCircleLabel)
    DECLARE_EVENT_TABLE()
public:
    AnkerMsgCircleLabel();
    virtual ~AnkerMsgCircleLabel();
    AnkerMsgCircleLabel(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);

    void setBgColor(wxColor color);
protected:
    
    void OnPaint(wxPaintEvent& event);
private:
    wxColor m_Color;
};

enum MsgBtnStatus
{
    MsgNormalBtnStatus,
    MsgEnterBtnStatus,
    MsgPressedBtnStatus,
    MsgUpBtnStatus,
    MsgLeaveBtnStatus,
    MsgDClickBtnStatus,
    MsgDisableBtnStatus
};
wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_MSG_BTN_CLICKED, wxCommandEvent);
class AnkerMsgTitleBtn :public wxControl
{
    DECLARE_DYNAMIC_CLASS(AnkerMsgTitleBtn)
    DECLARE_EVENT_TABLE()
public:
    AnkerMsgTitleBtn();
    virtual ~AnkerMsgTitleBtn();
    AnkerMsgTitleBtn(wxWindow* parent, wxWindowID id,
        wxColor textNorColor,
        wxColor textSelectColor,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);

    void initData(const wxString& btnName, MsgBtnStatus status);

    void setStatus(MsgBtnStatus status);

    virtual void OnEnter(wxMouseEvent& event);
    virtual void OnLeave(wxMouseEvent& event);
    virtual void OnPressed(wxMouseEvent& event);    
    virtual void OnSize(wxSizeEvent& event);

protected:
    void initUi();
    void OnPaint(wxPaintEvent& event);
private:
    MsgBtnStatus m_status{ MsgNormalBtnStatus };
    wxString     m_text{wxString("")};

};

class AnkerOfficialMsg :public wxControl
{
public:
    AnkerOfficialMsg();
    AnkerOfficialMsg(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);
    virtual~AnkerOfficialMsg();

    void initMsg(const wxString &msgTitle,
                const wxString &time,
                const wxString &content,
                bool isRead = false);

    void setReadStatus(bool isRead);
protected:
    void initUi();
private:
    bool m_isRead{ false };
    wxStaticText* m_pTitleLabel{ nullptr };
    wxStaticText* m_pTimeLabel{ nullptr };
    AnkerMsgCircleLabel* m_pRedLogoLabel{ nullptr };
    wxStaticText* m_pContentTextCtrl{ nullptr };
    //wxRichTextCtrl* m_pContentTextCtrl{ nullptr };
};

class AnkerPrinterMsg : public wxPanel
{
    DECLARE_DYNAMIC_CLASS(AnkerPrinterMsg)
    DECLARE_EVENT_TABLE()
public:
    AnkerPrinterMsg();
    AnkerPrinterMsg(wxWindow* parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL | wxNO_BORDER,
        const wxString& name = wxASCII_STR(wxPanelNameStr));
    virtual~AnkerPrinterMsg();
    void initMsg(const wxString& msgTitle,
        const wxString& time,
        const wxString& content,
        const wxString& url,
        bool isRead = false);

    void setReadStatus(bool isRead);    
    virtual void OnPressed(wxMouseEvent& event);    
    virtual void OnEnter(wxMouseEvent& event);
    virtual void OnLeave(wxMouseEvent& event);    

protected:
    void initUi();
private:
    bool m_isRead{ false };
    wxStaticText* m_pTitleLabel{ nullptr };
    wxStaticText* m_pTimeLabel{ nullptr };
    wxStaticText* m_pContentLabel{ nullptr };
    AnkerMsgCircleLabel* m_pRedLogoLabel{ nullptr };

    //wxRichTextCtrl* m_pContentTextCtrl{ nullptr };
    AnkerMsgPageBtn* m_pArrowLogoLabel{ nullptr };
    wxString m_supportUrl{ wxString() };
};

class AnkerMsgCentreDialog : public wxDialog
{
public:
    AnkerMsgCentreDialog(wxWindow* parent = nullptr);
    ~AnkerMsgCentreDialog();

    void updateMsg(std::vector<AnkerNet::MsgCenterItem>* dataList);
    void clearMsg();
    void showMsgCenterWidget();
    void showPrinterWidget(bool isShow);
    void setMsgAllRead();
    void getMsgCenterRecords(bool isSyn = false);
    void updateCurrentPage();
    void showNataTips(bool isShow);
protected:
    void resetSelectAllBtn();
    void initMulitLanguage();
    void initUi();
    void onShowOfficealWidget(bool isShow);
    void onNoRecords();
    
    void onMove(wxMoveEvent& event);
    void onOfficialBtn(wxCommandEvent& event);
    void onPrinterBtn(wxCommandEvent& event);
    void showLoading(bool visible);
    void isLoadingData(bool iskeyEvent = false);

    void isShowEditMode(bool isEdit);
private:
    AnkerMsgTitleBtn* m_pOfficialBtn{ nullptr };
    AnkerMsgTitleBtn* m_pPrinterBtn{ nullptr };

    wxScrolledWindow* m_OfficicalMsgWidget{ nullptr };
    wxScrolledWindow* m_printerMsgWidget{ nullptr };
    wxBoxSizer* m_pOfficScrolledVSizer{ nullptr };
    wxBoxSizer* m_pPrinterScrolledVSizer{ nullptr };
    bool m_limitRequest = false;
    wxTimer m_limitTimer;
    wxPanel* m_EmptyPanel{ nullptr };    
    wxPanel* m_printerPanel{ nullptr };

    std::vector<std::string> m_printerMsgVec;
    std::vector<std::string> m_officialMsgVec;
    
    wxStaticText* m_printerTitleSpcaer{ nullptr };
    wxStaticText* m_printerTitle{ nullptr };
    //AnkerMsgCheckBox* m_selectAllBtn{ nullptr };

    wxPanel* m_selectPanel{ nullptr };
    wxButton* m_selectAllBtn{ nullptr };
    wxStaticText* m_selectAllLabel{ nullptr };

    AnkerBtn* m_cancelBtn{ nullptr };
    AnkerBtn* m_deleteBtn{ nullptr };
    wxButton* m_editBtn{ nullptr };

    bool m_isEditmode = false;    

    bool m_selectAllBtnStatus = false;

    std::vector<AnkerNet::MsgCenterItem>* m_MsgRecords{ nullptr };
    int m_currentPage = 0;
    int m_pagesRecords = 10;
    std::mutex m_currentPageMutex;
    AnkerLoadingMask* m_loadingMask{ nullptr };
    wxPanel* m_pNodatasPanel{nullptr};
    
    std::map<int, wxString> m_mulitLanguageMap;
    std::vector<AnkerCustomMsg*> m_msgList;
};

class AnkerCustomMsg : public wxControl
{
    DECLARE_DYNAMIC_CLASS(AnkerPrinterMsg)
    DECLARE_EVENT_TABLE()
public:
    AnkerCustomMsg() {}
    AnkerCustomMsg(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxBORDER_NONE,
        const wxValidator& validator = wxDefaultValidator);
    virtual~AnkerCustomMsg() {}

    void setEditMode(bool isEdit);
    void setSelect(bool isSelect);
    bool getSelectStatus()const {return m_isSelect;}
    int getMsgID()const {return m_msgID;}
    virtual void OnPressed(wxMouseEvent& event);
    virtual void OnEnter(wxMouseEvent& event);
    virtual void OnLeave(wxMouseEvent& event);

    void initMsg(const wxString& msgTitle,
        const wxString& time,
        const wxString& content,
        const wxString& url,
        int msgID,
        bool isRead = false);

    void setReadStatus(bool isRead);
protected:
    std::string warpString(const wxString& str, wxFont font, const int& lineLength);
    virtual void OnPaint(wxPaintEvent& event);
private:

    bool m_isRead{ true };
    wxString m_msgTitle{ wxString() };
    wxString m_time{ wxString() };
    wxImage m_arrowLogo{ wxImage() };
    wxString m_content{ wxString() };
    wxString m_url{ wxString() };

    wxColor m_currentBgColor{ wxColor("#333438") };
    wxColor m_bgColor{ wxColor("#333438") };
    wxColor m_hoverColor{ wxColor("#3d3e41") };
    int m_msgID = 0;
    bool m_isEditMode = false;
    bool m_isSelect = false;
};

#endif
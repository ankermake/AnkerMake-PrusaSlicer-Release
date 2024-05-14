#include "AnkerSliceConmentDialog.hpp"

wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_NOT_ASK, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_SUBMIT, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_CONMENT_CLOSE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_STAR_COUNTS_CHANGED, wxCommandEvent);

AnkerConmentStar::AnkerConmentStar( wxWindow* parent,
    wxWindowID winid /*= wxID_ANY*/,
    const wxPoint& pos /*= wxDefaultPosition*/,
    const wxSize& size /*= wxDefaultSize*/)    
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
    initUi();
}

AnkerConmentStar::~AnkerConmentStar()
{
    exitClearBtnImg();
    if (m_badStarBitmap)
    {
        delete m_badStarBitmap;
        m_badStarBitmap = nullptr;
    }
    if (m_goodStarBitmap)
    {
        delete m_goodStarBitmap;
        m_goodStarBitmap = nullptr;
    }    
}

void AnkerConmentStar::initUi()
{    
    SetBackgroundColour(wxColour("#333438"));
    wxBoxSizer* pMainHsizer = new wxBoxSizer(wxHORIZONTAL);

    m_pStarFirstBtn = new AnkerBtn(this, wxID_ANY);
    m_pStarFirstBtn->SetBackgroundColour(wxColor("#333438"));
    m_pStarFirstBtn->SetMinSize(AnkerSize(20, 20));
    m_pStarFirstBtn->SetMaxSize(AnkerSize(20, 20));
    m_pStarFirstBtn->SetSize(AnkerSize(20, 20));
    
    m_pStarSecondBtn = new AnkerBtn(this, wxID_ANY);
    m_pStarSecondBtn->SetBackgroundColour(wxColor("#333438"));
    m_pStarSecondBtn->SetMinSize(AnkerSize(20, 20));
    m_pStarSecondBtn->SetMaxSize(AnkerSize(20, 20));
    m_pStarSecondBtn->SetSize(AnkerSize(20, 20));

    m_pStarThirdBtn = new AnkerBtn(this, wxID_ANY);
    m_pStarThirdBtn->SetBackgroundColour(wxColor("#333438"));
    m_pStarThirdBtn->SetMinSize(AnkerSize(20, 20));
    m_pStarThirdBtn->SetMaxSize(AnkerSize(20, 20));
    m_pStarThirdBtn->SetSize(AnkerSize(20, 20));

    m_pStarFourthBtn = new AnkerBtn(this, wxID_ANY);
    m_pStarFourthBtn->SetBackgroundColour(wxColor("#333438"));
    m_pStarFourthBtn->SetMinSize(AnkerSize(20, 20));
    m_pStarFourthBtn->SetMaxSize(AnkerSize(20, 20));
    m_pStarFourthBtn->SetSize(AnkerSize(20, 20));

    m_pStarFifthBtn = new AnkerBtn(this, wxID_ANY);
    m_pStarFifthBtn->SetBackgroundColour(wxColor("#333438"));
    m_pStarFifthBtn->SetMinSize(AnkerSize(20, 20));
    m_pStarFifthBtn->SetMaxSize(AnkerSize(20, 20));
    m_pStarFifthBtn->SetSize(AnkerSize(20, 20));
    wxImage badStarImg = wxImage(wxString::FromUTF8(Slic3r::var("badStar.png")), wxBITMAP_TYPE_PNG);
    wxImage goodStartImg = wxImage(wxString::FromUTF8(Slic3r::var("goodStar.png")), wxBITMAP_TYPE_PNG);

    m_badStarBitmap = new wxBitmap(badStarImg.Scale(AnkerLength(20), AnkerLength(20)));
    m_goodStarBitmap = new wxBitmap(goodStartImg.Scale(AnkerLength(20), AnkerLength(20)));

    setBtnImg(m_pStarFirstBtn, m_badStarBitmap);
    setBtnImg(m_pStarSecondBtn, m_badStarBitmap);
    setBtnImg(m_pStarThirdBtn, m_badStarBitmap);
    setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
    setBtnImg(m_pStarFifthBtn, m_badStarBitmap);

    m_pStarFirstBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerConmentStar::onBtnClick, this);
    m_pStarSecondBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerConmentStar::onBtnClick, this);
    m_pStarThirdBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerConmentStar::onBtnClick, this);
    m_pStarFourthBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerConmentStar::onBtnClick, this);
    m_pStarFifthBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AnkerConmentStar::onBtnClick, this);

    pMainHsizer->AddStretchSpacer();
    pMainHsizer->Add(m_pStarFirstBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pMainHsizer->AddSpacer(AnkerLength(12));
    pMainHsizer->Add(m_pStarSecondBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pMainHsizer->AddSpacer(AnkerLength(12));
    pMainHsizer->Add(m_pStarThirdBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pMainHsizer->AddSpacer(AnkerLength(12));
    pMainHsizer->Add(m_pStarFourthBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pMainHsizer->AddSpacer(AnkerLength(12));
    pMainHsizer->Add(m_pStarFifthBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pMainHsizer->AddStretchSpacer();

    SetSizer(pMainHsizer);
}

void AnkerConmentStar::setBtnImg(AnkerBtn* pBtn, wxBitmap* Img)
{
    if (!pBtn || !Img)
        return;

    pBtn->SetNorImg(Img);
    pBtn->SetPressedImg(Img);
    pBtn->SetEnterImg(Img);
    pBtn->SetDisableImg(Img);
}

void AnkerConmentStar::resetStar()
{
    setBtnImg(m_pStarFirstBtn, m_badStarBitmap);
    setBtnImg(m_pStarSecondBtn, m_badStarBitmap);
    setBtnImg(m_pStarThirdBtn, m_badStarBitmap);
    setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
    setBtnImg(m_pStarFifthBtn, m_badStarBitmap);

    m_starCounts = 0;
}

void AnkerConmentStar::exitClearBtnImg()
{
    m_pStarFirstBtn->clearImg();
    m_pStarSecondBtn->clearImg();
    m_pStarThirdBtn->clearImg();
    m_pStarFourthBtn->clearImg();
    m_pStarFifthBtn->clearImg();
}
int AnkerConmentStar::getStarCounts()
{
    return m_starCounts;
}

void AnkerConmentStar::onBtnClick(wxCommandEvent& event)
{
    AnkerBtn* senderObjt = dynamic_cast<AnkerBtn*>(event.GetEventObject());
    if (!senderObjt)
    {
        return;
    }

    if (m_pStarFirstBtn == senderObjt)
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_badStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_badStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_badStarBitmap);

        m_starCounts = 1;
    }
    else if (m_pStarSecondBtn == senderObjt)
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_goodStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_badStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_badStarBitmap);
        m_starCounts = 2;
    }
    else if (m_pStarThirdBtn == senderObjt)
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_goodStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_goodStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_badStarBitmap);
        m_starCounts = 3;
    }
    else if (m_pStarFourthBtn == senderObjt)
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_goodStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_goodStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_goodStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_badStarBitmap);
        m_starCounts = 4;
    }
    else if (m_pStarFifthBtn == senderObjt)
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_goodStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_goodStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_goodStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_goodStarBitmap);
        m_starCounts = 5;
    }
    else
    {
        setBtnImg(m_pStarFirstBtn, m_goodStarBitmap);
        setBtnImg(m_pStarSecondBtn, m_badStarBitmap);
        setBtnImg(m_pStarThirdBtn, m_badStarBitmap);
        setBtnImg(m_pStarFourthBtn, m_badStarBitmap);
        setBtnImg(m_pStarFifthBtn, m_badStarBitmap);
        m_starCounts = 1;
    }
    wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_STAR_COUNTS_CHANGED);
    ProcessEvent(evt);
    
    Update();
    Refresh();
}

AnkerSliceConmentDialog::AnkerSliceConmentDialog(wxWindow* parent, wxString content)
    : wxDialog(parent, wxID_ANY, _L("common_star_conment_title"))//_L("Rate Your Experience"))
{
    SetBackgroundColour(wxColor(41, 42, 45));    
    initUi();
    initEvent();
}

AnkerSliceConmentDialog::~AnkerSliceConmentDialog()
{

}

void AnkerSliceConmentDialog::checkSubmitBtn()
{
    int starCount = m_pStarPanel->getStarCounts();
    if (starCount > 0) {
        m_pSubmitBtn->SetBackgroundColour(wxColor("#62D361"));
        m_pSubmitBtn->Enable(true);
    }
    else {
        m_pSubmitBtn->SetBackgroundColour(wxColor("#3F4044"));
        m_pSubmitBtn->Enable(false);
    }
}

void AnkerSliceConmentDialog::initEvent()
{        
    //clear hint text
    m_pConmentTextCtrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {
        auto hint = m_pConmentTextCtrl->GetHint();
        auto text = m_pConmentTextCtrl->GetValue();
        if (hint == text) {
            m_pConmentTextCtrl->SetValue("");
        }
        Update();
        Refresh();
        Layout();
        event.Skip();
        });
    m_pConmentTextCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent& event) {
        auto text = m_pConmentTextCtrl->GetValue();
        if (text.Length() > 5000) {
            text = text.Left(5000);
            m_pConmentTextCtrl->SetValue(text);
            m_pConmentTextCtrl->SetInsertionPointEnd();
        }
        //checkOkBtn();
        event.Skip();
        });    
}

void AnkerSliceConmentDialog::initUi()
{
    SetBackgroundColour(wxColour("#333438"));
    wxStaticText* pLine = new wxStaticText(this, wxID_ANY, "");
    pLine->SetBackgroundColour(wxColor("#545859"));
    pLine->SetMaxSize(AnkerSize(500,1));
    pLine->SetMinSize(AnkerSize(500,1));
    pLine->SetSize(AnkerSize(500,1));

    m_pMainVSizer = new wxBoxSizer(wxVERTICAL);
    m_sumitPanel = new wxPanel(this, wxID_ANY);
    m_sumitPanel->SetMaxSize(AnkerSize(400,430));
    m_sumitPanel->SetMinSize(AnkerSize(400, 430));
    m_sumitPanel->SetSize(AnkerSize(400, 430));

    wxBoxSizer *pSumitPanelHSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pSumitPanelVSizer = new wxBoxSizer(wxVERTICAL);
    
    m_pMainVSizer->Add(pLine);

    //m_strSuggestion = "We'd love your feedback! How would you rate your experience with AnkerMake Studio V3.1.20?";
    wxString versionInfo = "V";
    versionInfo +=SLIC3R_VERSION;

    m_strSuggestion = wxString::Format(_L("common_star_conment_content"), versionInfo);
    m_strSuggestion = Slic3r::GUI::WrapEveryCharacter(m_strSuggestion, ANKER_FONT_NO_1, AnkerLength(350));
    wxStaticText* pConmentTitle = new wxStaticText(m_sumitPanel, 
                                                    wxID_ANY,
                                                    "",
                                                    wxDefaultPosition, 
                                                    wxDefaultSize,
                                                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

    pConmentTitle->SetMaxSize(AnkerSize(350,70));
    pConmentTitle->SetMinSize(AnkerSize(350,70));
    pConmentTitle->SetSize(AnkerSize(350,70));
    pConmentTitle->SetBackgroundColour(wxColour("#333438"));
    pConmentTitle->SetForegroundColour(wxColour("#ADAEAF"));
    pConmentTitle->SetFont(ANKER_FONT_NO_1);   
    pConmentTitle->SetLabelText(m_strSuggestion);

    //star
    m_pStarPanel = new AnkerConmentStar(m_sumitPanel, wxID_ANY);
    m_pStarPanel->SetMaxSize(AnkerSize(150,20));
    m_pStarPanel->SetMinSize(AnkerSize(150, 20));
    m_pStarPanel->SetSize(AnkerSize(150, 20));
    m_pStarPanel->Bind(wxCUSTOMEVT_STAR_COUNTS_CHANGED, [this](wxCommandEvent &event) {
        checkSubmitBtn();
    });
    wxBoxSizer* pStarHSizer = new wxBoxSizer(wxHORIZONTAL);
    pStarHSizer->AddStretchSpacer();
    pStarHSizer->Add(m_pStarPanel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pStarHSizer->AddStretchSpacer();

    m_pConmentTextCtrl = new wxRichTextCtrl(m_sumitPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_CHARWRAP | wxBORDER_NONE);
    m_pConmentTextCtrl->SetBackgroundColour(wxColour("#292A2D"));    
    m_pConmentTextCtrl->SetMaxLength(3000);
    wxRichTextAttr attr;
    attr.SetTextColour(*wxWHITE);
    m_pConmentTextCtrl->SetEditable(true);
    m_pConmentTextCtrl->SetBasicStyle(attr);
    //m_pConmentTextCtrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {});
    m_pConmentTextCtrl->SetFont(ANKER_FONT_NO_1);
    m_pConmentTextCtrl->SetMaxSize(AnkerSize(350, 160));
    m_pConmentTextCtrl->SetMinSize(AnkerSize(350, 160));
    m_pConmentTextCtrl->SetSize(AnkerSize(350, 160));
    //m_pConmentTextCtrl->SetHint(_L("Tell us more about your feeling, we will contact you directly if you have the complain about the slicer. (Optional)"));   
    m_pConmentTextCtrl->SetHint(_L("common_star_conment_placed_text"));   

    wxCursor handCursor(wxCURSOR_HAND);
    m_pDonotAskBtn = new AnkerBtn(m_sumitPanel, wxID_ANY);
    m_pDonotAskBtn->SetText(_L("common_star_conment_refused_btn"));
    m_pDonotAskBtn->SetFont(ANKER_FONT_NO_1);
    m_pDonotAskBtn->SetBackgroundColour(wxColor(97, 98, 101));
    m_pDonotAskBtn->SetMinSize(AnkerSize(170, 32));
    m_pDonotAskBtn->SetMaxSize(AnkerSize(170, 32));
    m_pDonotAskBtn->SetSize(AnkerSize(170, 32));
    m_pDonotAskBtn->SetRadius(5);
    m_pDonotAskBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pDonotAskBtn->SetCursor(handCursor);
    m_pDonotAskBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this] (wxCommandEvent & event) {
        m_isUserClose = false;
        wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CONMENT_NOT_ASK);
        ProcessEvent(evt);  
        m_finishedPanel->Hide();
        m_sumitPanel->Show();
        m_pConmentTextCtrl->SetHint(_L("common_star_conment_placed_text"));
        m_pStarPanel->resetStar();        
        this->EndModal(wxID_CLOSE);
    });

    m_pSubmitBtn = new AnkerBtn(m_sumitPanel, wxID_ANY);
    m_pSubmitBtn->SetText(_L("common_star_conment_submit_btn"));
    m_pSubmitBtn->SetFont(ANKER_FONT_NO_1);

    m_pSubmitBtn->SetMinSize(AnkerSize(170, 32));
    m_pSubmitBtn->SetMaxSize(AnkerSize(170, 32));
    m_pSubmitBtn->SetSize(AnkerSize(170, 32));
    m_pSubmitBtn->SetRadius(5);
    m_pSubmitBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pSubmitBtn->SetCursor(handCursor);
    m_pSubmitBtn->SetBackgroundColour(wxColor("#3F4044"));
    m_pSubmitBtn->Enable(false);
    m_pSubmitBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
        m_finishedPanel->Show();    
        m_sumitPanel->Hide();
        Update();
        Refresh();
        Layout();
        m_isUserClose = false;
        wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CONMENT_SUBMIT);
        wxVariant eventData;
        eventData.ClearList();

        wxString data = m_pConmentTextCtrl->GetValue();
        auto hint = m_pConmentTextCtrl->GetHint();
        auto text = m_pConmentTextCtrl->GetValue();
        if (hint == text) {
            data = "";
        }
        eventData.Append(wxVariant(m_pStarPanel->getStarCounts()));
        eventData.Append(wxVariant(data));

        evt.SetClientData(new wxVariant(eventData));
        evt.SetEventObject(this);
        ProcessEvent(evt);        
        });

    wxBoxSizer* pBtnHSizer = new wxBoxSizer(wxHORIZONTAL);

    pBtnHSizer->AddStretchSpacer();
    pBtnHSizer->Add(m_pDonotAskBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pBtnHSizer->AddSpacer(AnkerLength(12));
    pBtnHSizer->Add(m_pSubmitBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pBtnHSizer->AddStretchSpacer();

    pSumitPanelVSizer->AddSpacer(AnkerLength(24));
    pSumitPanelVSizer->Add(pConmentTitle, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pSumitPanelVSizer->AddSpacer(AnkerLength(16));
    pSumitPanelVSizer->Add(pStarHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pSumitPanelVSizer->AddSpacer(AnkerLength(12));
    pSumitPanelVSizer->Add(m_pConmentTextCtrl, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pSumitPanelVSizer->AddSpacer(AnkerLength(24));
    pSumitPanelVSizer->Add(pBtnHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pSumitPanelVSizer->AddSpacer(AnkerLength(16));

    pSumitPanelHSizer->AddSpacer(AnkerLength(24));
    pSumitPanelHSizer->Add(pSumitPanelVSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pSumitPanelHSizer->AddSpacer(AnkerLength(24));
    m_sumitPanel->SetSizer(pSumitPanelHSizer);

    //finished panel
    m_finishedPanel = new wxPanel(this, wxID_ANY);
    m_finishedPanel->SetMaxSize(AnkerSize(400, 430));
    m_finishedPanel->SetMinSize(AnkerSize(400, 430));
    m_finishedPanel->SetSize(AnkerSize(400, 430));

    wxBoxSizer* pFinishedPanelHSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* pFinishedPanelVSizer = new wxBoxSizer(wxVERTICAL);    

    wxImage resImg = wxImage(wxString::FromUTF8(Slic3r::var("result_success_icon.png")), wxBITMAP_TYPE_PNG);
    resImg.Rescale(AnkerLength(120), AnkerLength(120), wxIMAGE_QUALITY_HIGH);
    m_finishTipsImg = new wxStaticBitmap(m_finishedPanel, wxID_ANY, resImg);    
    m_finishTipsImg->SetMaxSize(AnkerSize(120,120));
    m_finishTipsImg->SetMinSize(AnkerSize(120,120));
    m_finishTipsImg->SetSize(AnkerSize(120,120));
    wxBoxSizer* pImgHSizer = new wxBoxSizer(wxHORIZONTAL);
    pImgHSizer->AddStretchSpacer();
    pImgHSizer->Add(m_finishTipsImg);
    pImgHSizer->AddStretchSpacer();
    
    wxStaticText* pFinishedTitle = new wxStaticText(m_finishedPanel, 
                                                    wxID_ANY,
                                                    _L("common_star_conment_thanks_tips"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize,
                                                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    pFinishedTitle->SetForegroundColour(wxColour("#FFFFFF"));
    pFinishedTitle->SetBackgroundColour(wxColour("#333438"));
    pFinishedTitle->SetFont(ANKER_FONT_NO_1);
    pFinishedTitle->SetMaxSize(AnkerSize(350,30));
    pFinishedTitle->SetMinSize(AnkerSize(350, 30));
    pFinishedTitle->SetSize(AnkerSize(350, 30));

    wxString finishedStr = wxString::Format(_L("common_star_conment_finish_tips"), versionInfo);
    finishedStr = Slic3r::GUI::WrapEveryCharacter(finishedStr, ANKER_FONT_NO_1, AnkerLength(350));
    wxStaticText* pFinishedContent = new wxStaticText(m_finishedPanel, 
                                                        wxID_ANY, 
                                                        finishedStr,
                                                        wxDefaultPosition,
                                                        wxDefaultSize,
                                                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

    pFinishedContent->SetForegroundColour(wxColour("#ABACAD"));
    pFinishedContent->SetBackgroundColour(wxColour("#333438"));
    pFinishedContent->SetFont(ANKER_FONT_NO_1);
    pFinishedContent->SetMaxSize(AnkerSize(350, 50));
    pFinishedContent->SetMinSize(AnkerSize(350, 50));
    pFinishedContent->SetSize(AnkerSize(350, 50));

    m_pOkBtn = new AnkerBtn(m_finishedPanel, wxID_ANY);
    m_pOkBtn->SetText(_L("common_star_conment_ok_btn"));
    m_pOkBtn->SetFont(ANKER_FONT_NO_1);
    m_pOkBtn->SetBackgroundColour(wxColor("#62D361"));
    m_pOkBtn->SetMinSize(AnkerSize(352, 32));
    m_pOkBtn->SetMaxSize(AnkerSize(352, 32));
    m_pOkBtn->SetSize(AnkerSize(352, 32));
    m_pOkBtn->SetRadius(5);
    m_pOkBtn->SetTextColor(wxColor("#FFFFFF"));
    m_pOkBtn->SetCursor(handCursor);

    m_pOkBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](wxCommandEvent& event) {
        this->Hide();
        m_finishedPanel->Hide();
        m_sumitPanel->Show();
        m_pConmentTextCtrl->SetHint(_L("common_star_conment_placed_text"));
        m_pStarPanel->resetStar();
        m_isUserClose = false;
        Update();
        Refresh();
        Layout();
        this->EndModal(wxID_CLOSE);
        });

    pFinishedPanelVSizer->AddStretchSpacer();
    pFinishedPanelVSizer->Add(pImgHSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pFinishedPanelVSizer->AddSpacer(AnkerLength(24));
    pFinishedPanelVSizer->Add(pFinishedTitle, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pFinishedPanelVSizer->AddSpacer(AnkerLength(6));
    pFinishedPanelVSizer->Add(pFinishedContent, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pFinishedPanelVSizer->AddSpacer(AnkerLength(86));
    pFinishedPanelVSizer->Add(m_pOkBtn, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pFinishedPanelVSizer->AddStretchSpacer();

    pFinishedPanelHSizer->AddSpacer(AnkerLength(24));
    pFinishedPanelHSizer->Add(pFinishedPanelVSizer, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    pFinishedPanelHSizer->AddSpacer(AnkerLength(24));

    m_finishedPanel->SetSizer(pFinishedPanelHSizer);

    m_finishedPanel->Hide();

    m_pMainVSizer->Add(m_sumitPanel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    m_pMainVSizer->Add(m_finishedPanel, wxALL | wxEXPAND, wxALL | wxEXPAND, 0);
    
    SetSizer(m_pMainVSizer);    

    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {        

        if (m_isUserClose)
        {        
            m_finishedPanel->Hide();
            m_sumitPanel->Show();
            m_pConmentTextCtrl->SetHint(_L("common_star_conment_placed_text"));
            m_pStarPanel->resetStar();
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_CONMENT_CLOSE);
            ProcessEvent(evt);
        }
        event.Skip();
        });
}

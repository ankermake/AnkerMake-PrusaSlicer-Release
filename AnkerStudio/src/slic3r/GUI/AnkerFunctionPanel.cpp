#include <wx/wx.h>
#include <wx/button.h>
#include <iterator>
#include "AnkerFunctionPanel.h"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "Notebook.hpp"
#include "AnkerMsgCentreDialog.hpp"

wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_FEEDBACK, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_MSG_CENTRE, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_RELEASE_NOTE, wxCommandEvent);

wxDEFINE_EVENT(wxCUSTOMEVT_FEEDBACK, wxCommandEvent);
wxDEFINE_EVENT(wxCUSTOMEVT_SHOW_DOC, wxCommandEvent);

namespace Slic3r {
	namespace GUI {

		BEGIN_EVENT_TABLE(AnkerTabBarBtn, wxControl)
			EVT_PAINT(AnkerTabBarBtn::OnPaint)
			EVT_ENTER_WINDOW(AnkerTabBarBtn::OnEnter)
			EVT_LEAVE_WINDOW(AnkerTabBarBtn::OnLeave)
			EVT_LEFT_DOWN(AnkerTabBarBtn::OnPressed)
			END_EVENT_TABLE()

			IMPLEMENT_DYNAMIC_CLASS(AnkerTabBarBtn, wxControl)
			wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_TABBAR_BTN_CLICKED, wxCommandEvent);
		AnkerTabBarBtn::AnkerTabBarBtn()
		{

		}

		AnkerTabBarBtn::~AnkerTabBarBtn()
		{


		}
		AnkerTabBarBtn::AnkerTabBarBtn(wxWindow* parent, wxWindowID id,
			wxImage btnImg,
			const wxPoint& pos,
			const wxSize& size,
			long style,
			const wxValidator& validator) :m_norImg(btnImg)
		{
			Create(parent, id, pos, size, style, validator);
			SetBackgroundStyle(wxBG_STYLE_PAINT);
			m_bgColor = wxColor("#202124");
		}
		void AnkerTabBarBtn::setImg(wxImage img)
		{
			m_norImg = img;
			Refresh();
		}
		void AnkerTabBarBtn::OnPressed(wxMouseEvent& event)
		{
			wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_TABBAR_BTN_CLICKED);
			wxVariant eventData;
			eventData.ClearList();
			evt.SetClientData(new wxVariant(eventData));
			evt.SetEventObject(this);
			ProcessEvent(evt);
		}
		void AnkerTabBarBtn::OnEnter(wxMouseEvent& event)
		{
			m_bgColor = wxColor("#333438");
			Refresh();
		}
		void AnkerTabBarBtn::OnLeave(wxMouseEvent& event)
		{
			m_bgColor = wxColor("#202124");
			Refresh();
		}
		void AnkerTabBarBtn::OnPaint(wxPaintEvent& event)
		{
			wxBufferedPaintDC dc(this);
			PrepareDC(dc);

			wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
			gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

			dc.SetBrush(wxBrush(m_bgColor));
			dc.SetPen(wxPen(m_bgColor));

			auto widgetRect = GetClientRect();
			widgetRect.SetSize(AnkerSize(widgetRect.width + 20, widgetRect.height + 20));
			dc.DrawRectangle(widgetRect);

			int w, h;
			GetClientSize(&w, &h);
			int squareSize = std::min(w, h);

			wxDouble imgWidth = m_norImg.GetWidth();
			wxDouble imgHeight = m_norImg.GetHeight();
			wxDouble scaleX = squareSize / imgWidth;
			wxDouble scaleY = squareSize / imgHeight;
			wxDouble scale = std::min(scaleX, scaleY);

			wxDouble scaledWidth = 24;
			wxDouble scaledHeight = 24;
			wxDouble posX = (w - scaledWidth) / 2;
			wxDouble posY = (h - scaledHeight) / 2;


			wxGraphicsBitmap bitmap = gc->CreateBitmapFromImage(m_norImg);
			gc->DrawBitmap(bitmap, posX, posY, scaledWidth, scaledHeight);

			delete gc;
		}

		AnkerFunctionPanel::AnkerFunctionPanel(
			wxWindow* parent,
			wxWindowID winid /*= wxID_ANY*/,
			const wxPoint& pos /*= wxDefaultPosition*/,
			const wxSize& size /*= wxDefaultSize*/,
			long style /*= wxTAB_TRAVERSAL | wxNO_BORDER*/,
			const wxString& name /*= wxASCII_STR(wxPanelNameStr)*/) :wxPanel(parent, winid, pos, size, style, name)
		{
			initUI();
			initEvent();
		}
		AnkerFunctionPanel::~AnkerFunctionPanel()
		{
			if (m_pFeedbackHelpMenu)
			{
				delete m_pFeedbackHelpMenu;
				m_pFeedbackHelpMenu = nullptr;
			}
		}
		void AnkerFunctionPanel::initUI()
		{
			// backgroud colour
			m_pSizer = new wxBoxSizer(wxHORIZONTAL);
			wxImage::AddHandler(new wxPNGHandler);
			//load images
			wxImage image(wxString::FromUTF8(Slic3r::var("image.png")), wxBITMAP_TYPE_PNG);
			wxImage image_active(wxString::FromUTF8(Slic3r::var("image_active.png")), wxBITMAP_TYPE_PNG);
			wxBitmap bitmap(image);
			wxBitmap bitmap_actice(image_active);


			//BlinkingBitmap* svgbitMap = nullptr;
			//std::string OGIconName = "image";     //
			//if (try_get_bmp_bundle(OGIconName)) {
			//	svgbitMap = new BlinkingBitmap(m_parent, OGIconName);
			//}
			//else {

			//	svgbitMap = new BlinkingBitmap(m_parent);
			//}
			//svgbitMap->activate();



			ScalableBitmap svgbitMap;
			std::string OGIconName = "image"; 
			if (try_get_bmp_bundle(OGIconName)) {
				svgbitMap =  ScalableBitmap(m_parent, OGIconName);
			}
			else {

				svgbitMap =  ScalableBitmap(m_parent);
			}
			
			wxImage devcieImage(wxString::FromUTF8(Slic3r::var("device.png")), wxBITMAP_TYPE_PNG);
			wxImage devcieImage_active(wxString::FromUTF8(Slic3r::var("device_active.png")), wxBITMAP_TYPE_PNG);
			wxBitmap devcieBitmap(devcieImage);
			wxBitmap devcieBitmap_actice(devcieImage_active);

			m_pSliceButton = new AnkerCombinButton(this, bitmap, _L("Slice")); 
			m_pSliceButton->SetMinSize(AnkerSize(160, 35));
			m_pSliceButton->SetActieBitMap(bitmap_actice);
			m_pSizer->Add(m_pSliceButton, 0, wxALL, 1);			

			m_pPrintButton = new AnkerCombinButton(this, devcieBitmap, _L("Device"));
			m_pPrintButton->SetMinSize(AnkerSize(160, 35));
			m_pPrintButton->SetActieBitMap(devcieBitmap_actice);
			m_pSizer->Add(m_pPrintButton, 0, wxALL, 1);
			m_pSizer->AddStretchSpacer();

			m_pTabBtnVec.push_back(m_pSliceButton);
			m_pTabBtnVec.push_back(m_pPrintButton);

			//set Slice button default selected
			m_pSliceButton->SetSelected(true);

			wxImage noMsgImg = wxImage(wxString::FromUTF8(Slic3r::var("noNewsMsg.png")), wxBITMAP_TYPE_PNG);
			wxImage feedBackImg = wxImage(wxString::FromUTF8(Slic3r::var("feedback.png")), wxBITMAP_TYPE_PNG);
			m_msgCentreBtn = new AnkerTabBarBtn(this, wxID_ANY, noMsgImg);
			m_msgCentreBtn->SetToolTip(_L("msg_center_entrance_tips"));
			m_msgCentreBtn->Bind(wxCUSTOMEVT_ANKER_TABBAR_BTN_CLICKED, [this](wxCommandEvent& event) {
				bool isShowOfficical = true;

				if (m_officicalNews > 0)				
					isShowOfficical = true;
				else
				{
					if(m_printerNews > 0)
						isShowOfficical = false;
				}

				wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SHOW_MSG_CENTRE);
				wxVariant eventData;
				eventData.ClearList();	
				eventData.Append(wxVariant(isShowOfficical));
				evt.SetClientData(new wxVariant(eventData));
				wxPostEvent(this, evt);
				});

			wxCursor handCursor(wxCURSOR_HAND);
			m_msgCentreBtn->SetBackgroundColour(wxColor("#62D361"));
			m_msgCentreBtn->SetMinSize(AnkerSize(40, 35));
			m_msgCentreBtn->SetMaxSize(AnkerSize(40, 35));
			m_msgCentreBtn->SetSize(AnkerSize(40, 35));
			m_msgCentreBtn->SetCursor(handCursor);

			m_pSizer->Add(m_msgCentreBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
			m_pSizer->AddSpacer((10));

			//wxImage feedBackImg = wxImage(wxString::FromUTF8(Slic3r::var("feedback.png")), wxBITMAP_TYPE_PNG);
			m_FeedBackHelpBtn = new AnkerTabBarBtn(this, wxID_ANY, feedBackImg);
			m_FeedBackHelpBtn->SetToolTip(_L("common_tab_feed_back_hlep_tips"));

			m_pFeedbackHelpMenu = new wxMenu();

			append_menu_item(m_pFeedbackHelpMenu,
				wxID_ANY,
				_L("common_tab_feed_back_entrance"),
				_L("common_tab_feed_back_entrance"),
				[=](wxCommandEvent&) {
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_FEEDBACK);
					evt.SetEventObject(this);
					ProcessEvent(evt);					
				});
			// add by alves wait url
			//append_menu_item(m_pFeedbackHelpMenu,
			//	wxID_ANY,
			//	_L("common_tab_documentation_entrance"),
			//	_L("common_tab_documentation_entrance"),
			//	[=](wxCommandEvent&) {
			//		wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_SHOW_DOC);
			//		evt.SetEventObject(this);
			//		ProcessEvent(evt);
			//	});

			append_menu_item(m_pFeedbackHelpMenu,
				wxID_ANY,
				_L("common_tab_release_note_entrance"),
				_L("common_tab_release_note_entrance"),
				[=](wxCommandEvent&) {
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_RELEASE_NOTE);
					evt.SetEventObject(this);
					ProcessEvent(evt);

				});

			m_FeedBackHelpBtn->Bind(wxCUSTOMEVT_ANKER_TABBAR_BTN_CLICKED, [this](wxCommandEvent& event) {				
				PopupMenu(m_pFeedbackHelpMenu);
				});			
						
			m_FeedBackHelpBtn->SetBackgroundColour(wxColor("#62D361"));
			m_FeedBackHelpBtn->SetMinSize(AnkerSize(40, 35));
			m_FeedBackHelpBtn->SetMaxSize(AnkerSize(40, 35));
			m_FeedBackHelpBtn->SetSize(AnkerSize(40, 35));
			m_FeedBackHelpBtn->SetCursor(handCursor);

			m_pSizer->Add(m_FeedBackHelpBtn, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
			m_pSizer->AddSpacer((10));

			SetSizer(m_pSizer);
			m_pSizer->Layout();
			m_pSizer->Fit(this);
		}

		void AnkerFunctionPanel::initEvent()
		{
			for (AnkerCombinButton* pCombinBtn : m_pTabBtnVec)
			{
				pCombinBtn->Bind(wxEVT_BUTTON, [this, pCombinBtn](wxCommandEvent& event){
					
					if (!pCombinBtn->GetSelected())
					{
						pCombinBtn->SetSelected(true);
						pCombinBtn->Refresh();
						for (AnkerCombinButton* pCombinBtnTmp : m_pTabBtnVec)
						{
							if (pCombinBtnTmp != pCombinBtn && pCombinBtnTmp->GetSelected())
							{
								pCombinBtnTmp->SetSelected(false);
								pCombinBtnTmp->Refresh();
							}
						}

						auto combinBtnIter = std::find(m_pTabBtnVec.begin(), m_pTabBtnVec.end(), pCombinBtn);
						int iPageInx = std::distance(m_pTabBtnVec.begin(), combinBtnIter);
						wxCommandEvent evt = wxCommandEvent(wxEVT_NOTEBOOK_PAGE_CHANGED);
						evt.SetId(iPageInx);
						wxPostEvent(m_pPrintTab, evt);
					}
					//disable calibration if "Device" selected
					if (m_calib_menu) {
						bool enable = pCombinBtn->GetText() == _L("Device") ? !pCombinBtn->GetSelected() : true;
						for (auto item : m_calib_menu->GetMenuItems()) {
							item->Enable(enable);
						}
					}
				});
			}	

			Bind(wxCUSTOMEVT_ON_TAB_CHANGE, [this](wxCommandEvent& event) {
				int iPageInx = event.GetId();
				for (int i = 0; i< m_pTabBtnVec.size();i++)
				{
					if (iPageInx == i)
					{
						m_pTabBtnVec[i]->SetSelected(true);
						m_pTabBtnVec[i]->Refresh();
					}
					else
					{
						m_pTabBtnVec[i]->SetSelected(false);
						m_pTabBtnVec[i]->Refresh();
					}
				}
			});
		}

		void AnkerFunctionPanel::setMsgEntrenceRedPointStatus(bool hasUnread)
		{
			if (hasUnread)
			{
				wxImage noMsgImg = wxImage(wxString::FromUTF8(Slic3r::var("newMsg.png")), wxBITMAP_TYPE_PNG);
				m_msgCentreBtn->setImg(noMsgImg);
			}
			else
			{
				wxImage noMsgImg = wxImage(wxString::FromUTF8(Slic3r::var("noNewsMsg.png")), wxBITMAP_TYPE_PNG);
				m_msgCentreBtn->setImg(noMsgImg);
			}
		}

		void AnkerFunctionPanel::setMsgItemRedPointStatus(int officicalNews, int printerNews)
		{
			bool res = false;
			if (officicalNews + printerNews)
				res = true;
			m_officicalNews = officicalNews;
			m_printerNews = printerNews;
			if(res)
			{
				wxImage noMsgImg = wxImage(wxString::FromUTF8(Slic3r::var("newMsg.png")), wxBITMAP_TYPE_PNG);
				m_msgCentreBtn->setImg(noMsgImg);
			}
			else
			{
				wxImage noMsgImg = wxImage(wxString::FromUTF8(Slic3r::var("noNewsMsg.png")), wxBITMAP_TYPE_PNG);
				m_msgCentreBtn->setImg(noMsgImg);
			}
		}
		void AnkerFunctionPanel::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC dc(this);
			dc.SetBrush(wxBrush(wxColour(DEFAULT_BG_COLOUR)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			wxSize size = GetSize();
			dc.DrawRectangle(0, 0, size.x, size.y);

			for (int i = 0; i < m_pTabBtnVec.size(); i++)
			{
				m_pTabBtnVec[i]->Refresh();
			}


		}
		BEGIN_EVENT_TABLE(AnkerFunctionPanel, wxPanel)
		EVT_PAINT(AnkerFunctionPanel::OnPaint)
		END_EVENT_TABLE()
	}
}
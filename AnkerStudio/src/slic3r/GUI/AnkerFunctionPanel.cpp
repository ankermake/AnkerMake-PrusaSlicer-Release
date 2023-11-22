#include <wx/wx.h>
#include <wx/button.h>
#include <iterator>
#include "AnkerFunctionPanel.h"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "Notebook.hpp"



namespace Slic3r {
	namespace GUI {
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

			m_pTabBtnVec.push_back(m_pSliceButton);

			//set Slice button default selected
			m_pSliceButton->SetSelected(true);

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

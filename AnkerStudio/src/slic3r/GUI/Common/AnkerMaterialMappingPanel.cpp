#include <wx/graphics.h>
#include "AnkerGUIConfig.hpp"
#include "../../GUI/GUI_App.hpp"
#include "../../GUI/AnkerBtn.hpp"
#include "AnkerButton.hpp"
#include "../Common/AnkerRoundPanel.hpp"
#include "AnkerMaterialMappingPanel.h"
#include "libslic3r/Utils.hpp"
#include "../../GUI/GcodeVerify/PrintCheckHint.hpp"
#include <slic3r/Utils/DataMangerUi.hpp>
#include "AnkerNetDefines.h"
#include "DeviceObjectBase.h"

namespace Slic3r {
	namespace GUI {
		wxDEFINE_EVENT(wxCUSTOMEVT_COMBO_SELECT_CHANGE, wxCommandEvent);
		wxDEFINE_EVENT(wxCUSTOMEVT_CHECK_SELECTION, wxCommandEvent);

//TODO: need replace with  defined key 
#define EMPTY_MAP_DEVICE_INDEX			_L("Please specify filament before printing")
#define NOT_SUPPORT_MATERIAL_SELECT		_L("Only supports selecting materials of the same type as you sliced.")
#define WARNING_COLOR					wxColour(253, 147, 70)
#define ERROR_COLOR						wxColour(255, 0, 0)
#define WARP_LENGTH						AnkerLength(260)
#define UNKNOWN_MATERIAL				"?"
#define DISABLED_MATERIAL				"N/A"




		wxColour MixColorsWithAlpha(wxColour fgColor, wxColour bgColor) {
			int alpha1 = fgColor.Alpha();
			int alpha2 = bgColor.Alpha();

			int resultAlpha = alpha1 + (255 - alpha1) * alpha2 / 255;
			int resultRed = (fgColor.Red() * alpha1 + bgColor.Red() * alpha2 * (255 - alpha1) / 255) / resultAlpha;
			int resultGreen = (fgColor.Green() * alpha1 + bgColor.Green() * alpha2 * (255 - alpha1) / 255) / resultAlpha;
			int resultBlue = (fgColor.Blue() * alpha1 + bgColor.Blue() * alpha2 * (255 - alpha1) / 255) / resultAlpha;

			wxColour resultColor(resultRed, resultGreen, resultBlue, resultAlpha);

			return resultColor;
		}

		AnkerMaterialMappingPanel::AnkerMaterialMappingPanel(wxWindow* parent, std::string strDeviceSn, AnkerMaterialMappingViewModel* pViewModel, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name /*= wxScrollBarNameStr*/)
			: AnkerBasePanel(parent, id, pos, size, style, name), m_currentDeviceSn(strDeviceSn), m_ViewModel(pViewModel)
		{
			InitData();
			InitUI();
			initEvent();
		}

		void AnkerMaterialMappingPanel::InitData()
		{
			m_dialogColor = wxColour(PANEL_BACK_LIGHT_RGB_INT);
			m_textLightColor = wxColour(TEXT_LIGHT_RGB_INT);
			m_splitLineColor = wxColour(255, 255, 255);
			m_noticeTextClolor = ERROR_COLOR;
			m_splitLineColor = m_splitLineColor.ChangeLightness(30);

#ifdef __APPLE__
			m_textDarkColor = wxColour(193, 193, 193);
#else
			m_textDarkColor = wxColour(TEXT_DARK_RGB_INT);
#endif // __APPLE__

			//check viewModle
			if (m_ViewModel == nullptr)
			{
				ANKER_LOG_ERROR << "AnkerMaterialMappingViewModel is nullptr,please check";
				return;
			}

			m_filamentMap = m_ViewModel->m_curFilamentMap;
#if LOCAL_SIMULATE_V6
			SimulateData();
#endif // LOCAL_SIMULATE_V6		
		}

		void AnkerMaterialMappingPanel::SimulateData()
		{
			//GetColorMaterialIdMap
			GcodeFilementInfo gCodeInfoTmp;
			DeviceFilementInfo devcieFilementInfoTmp;
			std::vector<DeviceFilementInfo> devcieFilementInfoTmpVec;
			int64_t materialIdArr[6] = { 10000001,10000002 ,10000003 ,10000004 ,10000005 ,10000006 };
			int64_t colourlIdArr[6] = { 10000001,10000002 ,10000003 ,10000004 ,10000005 ,10000006 };
			wxString strMaterialNameArr[] = { "PLA","PLA+","AnkerMake PLA+Basic","PLA","PLA+","AnkerMake PLA+Basic" };
			wxString colourNameArr[] = { "#000000","#FFFFFF","#AAAAA8","#E60012","#00358E" ,"#EB6100" };

			for (int i = 0; i < 6; i++)
			{
				gCodeInfoTmp.iCoLorId = i;
				gCodeInfoTmp.strMaterialName = strMaterialNameArr[i];
				gCodeInfoTmp.filamentColor = wxColor(colourNameArr[i]);
				devcieFilementInfoTmp.strMaterialName = strMaterialNameArr[i];
				devcieFilementInfoTmp.filamentColor = wxColor(colourNameArr[i]);
				devcieFilementInfoTmpVec.push_back(devcieFilementInfoTmp);
				m_filamentMap.insert(std::make_pair(gCodeInfoTmp, devcieFilementInfoTmpVec));
				devcieFilementInfoTmpVec.clear();
				m_allDeviceFialmentInfo.push_back(devcieFilementInfoTmp);
			}
		}

		void AnkerMaterialMappingPanel::InitUI()
		{
			wxWindowUpdateLocker noUpdates(this);
			//check viewModle
			if (m_ViewModel == nullptr)
			{
				ANKER_LOG_ERROR << "AnkerMaterialMappingViewModel is nullptr,please check";
				return;
			}
			wxBoxSizer* pInnnerVSizer = new wxBoxSizer(wxVERTICAL);
			SetSizer(pInnnerVSizer);
			m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
			m_scrolledWindow->SetMinSize(AnkerSize(324, 432));
			pInnnerVSizer->Add(m_scrolledWindow, 0, wxEXPAND);
			wxBoxSizer* pFileInfoVSizer = new wxBoxSizer(wxVERTICAL);
			m_scrolledWindow->SetSizer(pFileInfoVSizer);
			m_scrolledWindow->EnableScrolling(false, true);
			m_scrolledWindow->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
			m_scrolledWindow->SetScrollRate(-1, 10);
			pFileInfoVSizer->AddSpacer(AnkerLength(16));
			// preview image
			//wxBitmap bitmapEx = wxBitmap(wxString::FromUTF8(Slic3r::var("gcode_image_sample.png")), wxBITMAP_TYPE_PNG);
			//wxImage image = bitmapEx.ConvertToImage();
			wxImage image = m_ViewModel->m_previewImage;
			DeviceObjectBasePtr currentDev = CurDevObject(m_currentDeviceSn);
			if (currentDev)
			{
				if (!currentDev->IsMultiColorDevice())
				{
					image.Rescale(AnkerLength(160), AnkerLength(160));
					image.Replace(0, 0, 0, m_dialogColor.Red(), m_dialogColor.Green(), m_dialogColor.Blue());
				}
				else
				{
					image.Rescale(AnkerLength(115), AnkerLength(115));
					image.Replace(0, 0, 0, m_dialogColor.Red(), m_dialogColor.Green(), m_dialogColor.Blue());
				}
			}

			wxBitmap scaledBitmap(image);
			wxStaticBitmap* pPreviewImage = new wxStaticBitmap(m_scrolledWindow, wxID_ANY, scaledBitmap);
			pPreviewImage->SetMinSize(scaledBitmap.GetSize());
			pPreviewImage->SetMaxSize(scaledBitmap.GetSize());
			pPreviewImage->SetBackgroundColour(m_dialogColor);

			if (currentDev)
			{
				if (!currentDev->IsMultiColorDevice())
				{
					pFileInfoVSizer->Add(pPreviewImage, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 32);
				}
				else
				{
					pFileInfoVSizer->Add(pPreviewImage, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);
					//by Samuel,show filament mapping info for multi-color printer
					// split line
					wxControl* splitLineCtrl = new wxControl(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
					splitLineCtrl->SetBackgroundColour(m_splitLineColor);
					splitLineCtrl->SetSizeHints(AnkerSize(264, 1), AnkerSize(264, 1));
					pFileInfoVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);

					AnkerBasePanel* mapPanel = new AnkerBasePanel(m_scrolledWindow);
					wxBoxSizer* pMapInfoVSizer = new wxBoxSizer(wxVERTICAL);
					mapPanel->SetSizer(pMapInfoVSizer);
					for (const auto& pair : m_filamentMap)
					{
						AnkerMaterialMapItemViewModel* pItemViewModel = new AnkerMaterialMapItemViewModel();
						pItemViewModel->m_deviceFilamentInfoVM = pair.second;
						pItemViewModel->m_filamentNameVM = pair.first.strMaterialName;
						pItemViewModel->m_gcodeFilamentColorVM = pair.first.filamentColor;
						m_pMaterialItemViewModelVec.push_back(pItemViewModel);
						AnkerMaterialMapItem* filamentMapItem = new AnkerMaterialMapItem(mapPanel, pItemViewModel);
						m_pMaterialItemViewVec.push_back(filamentMapItem);
						pMapInfoVSizer->Add(filamentMapItem, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
					}
					pFileInfoVSizer->Add(mapPanel, 0, wxALIGN_CENTER_HORIZONTAL);
				}
			}

			// split line
			wxControl* splitLineCtrl = new wxControl(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			splitLineCtrl->SetBackgroundColour(m_splitLineColor);
			splitLineCtrl->SetMaxSize(AnkerSize(264, 1));
			splitLineCtrl->SetMinSize(AnkerSize(264, 1));
			pFileInfoVSizer->Add(splitLineCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 8);

			// filament
			{
				AnkerBasePanel* pFilamentPanel = new AnkerBasePanel(m_scrolledWindow);
				wxBoxSizer* filamentSizer = new wxBoxSizer(wxHORIZONTAL);
				filamentSizer->SetMinSize(AnkerSize(264, 32));
				pFilamentPanel->SetSizer(filamentSizer);


				wxImage usedFBitmap = wxImage(wxString::FromUTF8(Slic3r::var("used_filament.png")), wxBITMAP_TYPE_PNG);
				wxStaticBitmap* usedFIcon = new wxStaticBitmap(pFilamentPanel, wxID_ANY, usedFBitmap);
				usedFIcon->SetMinSize(usedFBitmap.GetSize());
				filamentSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

				wxStaticText* usedFText = new wxStaticText(pFilamentPanel, wxID_ANY, /*"Filament"*/_("common_print_previewpopup_filament"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_VERTICAL);
				usedFText->SetBackgroundColour(m_dialogColor);
				usedFText->SetForegroundColour(m_textDarkColor);
				usedFText->SetFont(ANKER_FONT_NO_1);
				usedFText->Fit();
				filamentSizer->Add(usedFText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

				filamentSizer->AddStretchSpacer(1);
				wxStaticText* pFilamentText = new wxStaticText(pFilamentPanel, wxID_ANY, m_ViewModel->m_filamentCost);
				pFilamentText->SetBackgroundColour(m_dialogColor);
				pFilamentText->SetForegroundColour(m_textLightColor);
				pFilamentText->SetFont(ANKER_BOLD_FONT_NO_1);
				pFilamentText->Fit();
				filamentSizer->Add(pFilamentText, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
				filamentSizer->Layout();
				pFileInfoVSizer->Add(pFilamentPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 30);
			}


			//pFileInfoVSizer->AddSpacer(16);

			// print time
			{
				AnkerBasePanel* pPrintTimePanel = new AnkerBasePanel(m_scrolledWindow);
				wxBoxSizer* printTimeSizer = new wxBoxSizer(wxHORIZONTAL);
				printTimeSizer->SetMinSize(AnkerSize(264, 32));
				pPrintTimePanel->SetSizer(printTimeSizer);
				pFileInfoVSizer->Add(pPrintTimePanel, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 30);

				wxImage usedFBitmap = wxImage(wxString::FromUTF8(Slic3r::var("finish_time.png")), wxBITMAP_TYPE_PNG);
				wxStaticBitmap* usedFIcon = new wxStaticBitmap(pPrintTimePanel, wxID_ANY, usedFBitmap);
				usedFIcon->SetMinSize(usedFBitmap.GetSize());
				printTimeSizer->Add(usedFIcon, 0, wxALIGN_LEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

				wxStaticText* printTimeText = new wxStaticText(pPrintTimePanel, wxID_ANY, /*"Print Time"*/_("common_print_previewpopup_printtime"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_VERTICAL);
				printTimeText->SetBackgroundColour(m_dialogColor);
				printTimeText->SetForegroundColour(m_textDarkColor);
				printTimeText->SetFont(ANKER_FONT_NO_1);
				printTimeText->Fit();
				printTimeSizer->Add(printTimeText, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 8);

				printTimeSizer->AddStretchSpacer(1);

				wxStaticText* pPrintTimeText = new wxStaticText(pPrintTimePanel, wxID_ANY, m_ViewModel->m_PrintTime);
				pPrintTimeText->SetBackgroundColour(m_dialogColor);
				pPrintTimeText->SetForegroundColour(m_textLightColor);
				pPrintTimeText->SetFont(ANKER_BOLD_FONT_NO_1);
				pPrintTimeText->Fit();
				printTimeSizer->Add(pPrintTimeText, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_RIGHT | wxTOP | wxBOTTOM, 8);
				printTimeSizer->Layout();
			}

			// split line
			m_NoticePanel = new AnkerBasePanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			wxControl* splitLineCtrl_Next = new wxControl(m_NoticePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
			splitLineCtrl_Next->SetBackgroundColour(m_splitLineColor);
			splitLineCtrl_Next->SetMaxSize(AnkerSize(264, 1));
			splitLineCtrl_Next->SetMinSize(AnkerSize(264, 1));
			wxBoxSizer* pNoticeSizer = new wxBoxSizer(wxVERTICAL);
			m_NoticePanel->SetSizer(pNoticeSizer);
			pNoticeSizer->Add(splitLineCtrl_Next, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 8);

			m_noticeText = new wxStaticText(m_NoticePanel, wxID_ANY, _L(EMPTY_MAP_DEVICE_INDEX));
			m_noticeText->SetBackgroundColour(m_dialogColor);
			m_noticeText->SetForegroundColour(m_noticeTextClolor);
			m_noticeText->SetFont(ANKER_FONT_NO_1);
			m_noticeText->SetMinSize(AnkerSize(264, 38));
			m_noticeText->Fit();
			pNoticeSizer->Add(m_noticeText, 1, wxEXPAND | wxLEFT, 30);
            m_noticeText->Wrap(WARP_LENGTH);
			pFileInfoVSizer->Add(m_NoticePanel, 1, wxEXPAND);
			pFileInfoVSizer->AddSpacer(AnkerLength(20));

			//default set hide 
			//m_NoticePanel->Hide();
			// button sizer
			wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
			pInnnerVSizer->Add(buttonSizer, 0, wxALIGN_BOTTOM | wxBOTTOM, 30);
			buttonSizer->AddStretchSpacer(1);
			m_pPrintBtn = new AnkerBtn(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
			m_pPrintBtn->SetMinSize(AnkerSize(264, 24));
			m_pPrintBtn->SetMaxSize(AnkerSize(264, 24));
			m_pPrintBtn->SetText(/*L"Start Printing"*/_L("common_print_taskbar_buttonstart"));
			m_pPrintBtn->SetDisableTextColor(wxColor(105, 105, 108));
			m_pPrintBtn->SetBackgroundColour(wxColor("#62D361"));
			m_pPrintBtn->SetRadius(4);
			m_pPrintBtn->SetTextColor(wxColor("#FFFFFF"));
			m_pPrintBtn->SetFont(ANKER_BOLD_FONT_NO_1);
			m_pPrintBtn->SetBackRectColour(wxColour(PANEL_BACK_LIGHT_RGB_INT));
			m_pPrintBtn->Bind(wxEVT_BUTTON, &AnkerMaterialMappingPanel::OnPrintBtn, this);
			buttonSizer->Add(m_pPrintBtn, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT, 24);
			buttonSizer->Layout();
			if (!IsDeviceFilamentMapped().bAllMapped)
			{
				m_pPrintBtn->SetStatus(DisableBtnStatus);
			}
			Refresh();
		}
		void AnkerMaterialMappingPanel::initEvent()
		{
			Bind(wxCUSTOMEVT_CHECK_SELECTION, [this](wxCommandEvent& event)
				{
					updateDevcieItemViewModel();
					if (!IsDeviceFilamentMapped().bAllMapped)
					{
						m_noticeText->SetLabelText(EMPTY_MAP_DEVICE_INDEX);
						m_noticeText->SetForegroundColour(ERROR_COLOR);
                        m_noticeText->Wrap(WARP_LENGTH);
						m_pPrintBtn->SetStatus(DisableBtnStatus);
					}
					else
					{
						m_noticeText->SetLabelText("");
						m_pPrintBtn->SetStatus(NormalBtnStatus);
					}

					if (IsDeviceFilamentMapped().bSelectUnkonwnFilament)
					{
						m_noticeText->SetLabelText(NOT_SUPPORT_MATERIAL_SELECT);
                        m_noticeText->Wrap(WARP_LENGTH);
						m_noticeText->SetForegroundColour(WARNING_COLOR);
						m_NoticePanel->Show();
						Layout();
					}

					Refresh();
				});
		}
		void AnkerMaterialMappingPanel::OnPrintBtn(wxCommandEvent& event)
		{
			//TODO: check is should send material map info 
			auto currentDev = CurDevObject(m_currentDeviceSn);
			//TODO: send new maping to devcie
			if (currentDev != nullptr)
			{
				VrCardInfoMap vrCardInfoMap;
				CardInfo gcodeCardInfo, devcieCardInfo;
				std::map<GcodeFilementInfo, std::vector<DeviceFilementInfo>>::iterator it = m_ViewModel->m_curFilamentMap.begin();
				updateDevcieItemViewModel();
				for (; it != m_ViewModel->m_curFilamentMap.end(); it++)
				{
					gcodeCardInfo.colorId = it->first.iCoLorId;
					gcodeCardInfo.materialId = it->first.iFilamentId;
					gcodeCardInfo.index = it->first.iNozzelInx;
					int position = std::distance(m_ViewModel->m_curFilamentMap.begin(), it);
					int iSelectedInx = m_pMaterialItemViewModelVec[position]->m_MapedInx;
					if (position >= 0 && iSelectedInx >= 0 && m_pMaterialItemViewModelVec[position]->m_deviceFilamentInfoVM.size() > iSelectedInx)
					{
						devcieCardInfo.colorId = m_pMaterialItemViewModelVec[position]->m_deviceFilamentInfoVM[iSelectedInx].iCoLorId;
						devcieCardInfo.index = m_pMaterialItemViewModelVec[position]->m_deviceFilamentInfoVM[iSelectedInx].iNozzelInx;
						devcieCardInfo.materialId = m_pMaterialItemViewModelVec[position]->m_deviceFilamentInfoVM[iSelectedInx].iFilamentId;
					}
					vrCardInfoMap.push_back(std::make_pair(gcodeCardInfo, devcieCardInfo));
				}

				m_ViewModel->m_vrCardInfoMap = vrCardInfoMap;

			
				PrintCheckHint printCheckHint;
				printCheckHint.SetFuncs([&printCheckHint, this](const std::string& sn2) {
					if (printCheckHint.Type() == PrintCheckHint::HintType::NEED_LEVEL) {
						// should remove m_gcodeImportDialog, use FindWindowById
						// auto dailog = dynamic_cast<wxDialog*>(wxWindow::FindWindowById(ID_GCODE_IMPORT_DIALOG));
						if (m_gcodeImportDialog) {
							m_gcodeImportDialog->EndModal(wxCANCEL);
						}
					}
				});

				//TODO:  should check is send success, if send fialed,should propmt a error mesasge and write log
				if (m_ViewModel->m_FileSelectMode != FSM_STORAGE && m_ViewModel->m_FileSelectMode != FSM_USB)
				{
					ANKER_LOG_INFO << "start to check need remind for local print";
					if (printCheckHint.CheckNeedRemind(currentDev, m_ViewModel->m_gcodeFilePath))
					{
						wxWindow* mainWindow = wxTheApp->GetTopWindow();
						if (mainWindow != nullptr)
						{
							if (!printCheckHint.Hint(currentDev, mainWindow))
							{
								return;
							}
						}
					}
					currentDev->SetLocalPrintData(vrCardInfoMap, m_ViewModel->m_gcodeFilePath);
				}
				else
				{
					ANKER_LOG_INFO << "start to check need remind for remote print";
					if (printCheckHint.CheckNeedRemind(currentDev))
					{
						wxWindow* mainWindow = wxTheApp->GetTopWindow();
						if (mainWindow != nullptr)
						{
							if (!printCheckHint.Hint(currentDev, mainWindow,
								wxDefaultSize,
								wxDefaultPosition,
								true))
							{
								return;
							}
						}
					}
					currentDev->SetRemotePrintData(vrCardInfoMap, m_ViewModel->m_gcodeFilePath);
				}
			}

			wxPanel* pParent = dynamic_cast<wxPanel*>(GetParent());
			if (pParent != nullptr)
			{
				//TODO: need refactor code 
				wxDialog* pDialogParent = dynamic_cast<wxDialog*>(pParent->GetParent()->GetParent());
				if (pDialogParent != nullptr)
				{
					pDialogParent->EndModal(wxOK);
					pDialogParent->Hide();
				}
			}
		}

		void AnkerMaterialMappingPanel::updateDevcieItemViewModel()
		{
			for (int i = 0; i < m_pMaterialItemViewVec.size(); i++)
			{
				if (m_pMaterialItemViewVec[i] != nullptr)
				{
					m_pMaterialItemViewVec[i]->updateViewModel();
				}
			}
		}

		MapCheckResult AnkerMaterialMappingPanel::IsDeviceFilamentMapped()
		{
			MapCheckResult result;
			bool bMapped = true;
			bool bSelectUnknowFilament = false;
			for (int i = 0; i < m_pMaterialItemViewVec.size(); i++)
			{
				AnkerMaterialMapItemViewModel* pItemViewModel = m_pMaterialItemViewVec[i]->getMapItemVm();
				if (m_pMaterialItemViewVec[i] == nullptr || pItemViewModel == nullptr || pItemViewModel->m_MapedInx == -1)
				{
					bMapped = false;
				}

				if ( pItemViewModel != nullptr && pItemViewModel->m_mappedFilamentName == "?")
				{
					bSelectUnknowFilament = true;
				}
			}

			result.bAllMapped = bMapped;
			result.bSelectUnkonwnFilament = bSelectUnknowFilament;
			return result;
		}

		void AnkerDeviceMaterialCombo::setColor(const wxColor& borderColor, const wxColor& bgColor, const wxColor& textColor)
		{
			m_borderColor = borderColor;
			m_bgColor = bgColor;
			m_textColor = textColor;
		}

		void AnkerDeviceMaterialCombo::setComBoxItemHeight(const int& height)
		{
			m_itemHeight = height;
		}
		void AnkerDeviceMaterialCombo::OnDrawItem(wxDC& dc,
			const wxRect& rect,
			int item,
			int flags) const
		{
			if (item == wxNOT_FOUND)
				return;

			wxRect r(rect);

			bool drawMaterialText = true;
			// If the current item is selected, then draw a custom background color.
			if (flags & wxODCB_PAINTING_CONTROL) {
				// content show in combobox
				wxColour colour("#232426");
				dc.SetBrush(wxBrush(colour));
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.DrawRectangle(rect);

				drawMaterialText = false;
			}
			else if (flags & wxODCB_PAINTING_SELECTED) {
				// item select
				wxColour colour("#506853");
				dc.SetBrush(wxBrush(colour));
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.DrawRectangle(rect);
			}
			else {
				// unselect popup item 
				dc.SetBrush(wxBrush(m_bgColor));
				dc.SetPen(m_bgColor);
				wxRect newRec(rect);
				newRec.SetWidth(rect.width + 1);
				newRec.SetHeight(rect.height + 1);
				dc.DrawRectangle(newRec);
			}

			AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(item);
			if (!comboBoxItem)
			{
				return;
			}

			DrawItem(dc, r, comboBoxItem, drawMaterialText);
		}

		void AnkerDeviceMaterialCombo::DrawItem(wxDC& dc, wxRect& r, AnkerBitmapComboxItem* pItem, bool drawMaterialText) const
		{
			if (!pItem)
				return;

			wxPoint textPoint(r.x + (m_LeftSpan), (r.y + 0) + ((r.height / 2) - dc.GetCharHeight() / 2));
			// Get text colour as pen colour
			dc.SetPen(wxPen(m_textColor));
			dc.SetFont(ANKER_FONT_NO_1);
			dc.SetTextForeground(pItem->enable ? m_textColor : m_textColorDisable);
			dc.DrawText(pItem->label.ToStdString(), textPoint);

			// draw material text
			if (drawMaterialText) {
				wxSize labelTextSize = dc.GetTextExtent(pItem->label.ToStdString());
				wxPoint materialTextPoint(r.x + (m_LeftSpan + labelTextSize.GetWidth() + m_LabelMaterialSpan), (r.y + 0) + ((r.height / 2) - dc.GetCharHeight() / 2));
				dc.DrawText(pItem->material.ToStdString(), materialTextPoint);
			}

			// draw color Icon
			wxPoint colorIconPos(r.width - m_diameter - m_LeftSpan, r.y + (r.height - m_diameter) / 2);
			if (pItem->color.IsOk())
			{
				wxPen colorRecPen(wxColour(pItem->color));
				wxBrush colorRecBrush(wxColour(pItem->color));
				//wxRect square(r.x + 10, r.y + (r.height - 30) / 2, 30, 30);

				//wxAutoBufferedPaintDC oldDc(dc);
				//oldDc.Clear(); // Clear the background
				//wxGraphicsContext* gc = wxGraphicsContext::Create(&dc);
				//if (gc)
				//{
				//	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT); // Enable anti-aliasing
				//	wxSize size = GetSize();
				//	gc->SetBrush(colorRecBrush);
				//	gc->DrawEllipse(m_leftGapWidth, m_topGapWidth, m_diameter, m_diameter);
				//	delete gc;
				//}

				wxColour BrushColor = wxColour(pItem->color);
				if (pItem->enable == false) {
					wxColour halfAlphaItemColor(BrushColor.Red(), BrushColor.Green(), BrushColor.Blue(), 80);
					BrushColor = MixColorsWithAlpha(halfAlphaItemColor, m_bgColor);
				}
				dc.SetBrush(BrushColor);
				dc.SetPen(BrushColor);

				dc.DrawEllipse(colorIconPos.x, colorIconPos.y, m_diameter, m_diameter);
			}
			else {
				// unkonw color item
				dc.SetBrush(wxBrush(wxColour("#393a3c")));
				dc.SetPen(wxPen(wxColour("#606060")));
				dc.DrawEllipse(colorIconPos.x, colorIconPos.y, m_diameter, m_diameter);

				wxString str = "?";
				dc.SetTextForeground(m_textColorDisable);
				wxSize textSize = dc.GetTextExtent(str);
				dc.DrawText("?", colorIconPos.x + (m_diameter / 2 - textSize.GetWidth() / 2), colorIconPos.y + (m_diameter / 2 - textSize.GetHeight() / 2));
			}

			if (pItem->nozzleStatus != Normal) {
				// material Empty item Icon
				dc.SetBrush(wxBrush(wxColour("#393a3c")));
				dc.SetPen(wxPen(wxColour("#606060")));
				dc.DrawEllipse(colorIconPos.x, colorIconPos.y, m_diameter, m_diameter);

				// draw X in the circle
				int iconCenter_x = colorIconPos.x + m_diameter / 2;
				int iconCenter_y = colorIconPos.y + m_diameter / 2;
				int squareSize = m_diameter / sqrt(2);
				int squareLeft = iconCenter_x - squareSize / 2;
				int squareRigth = iconCenter_x + squareSize / 2;
				int squareUp = iconCenter_y - squareSize / 2;
				int squareBottom = iconCenter_y + squareSize / 2;
				dc.DrawLine(squareLeft + 1, squareUp + 1, squareRigth, squareBottom);
				dc.DrawLine(squareLeft + 1, squareBottom - 1, squareRigth, squareUp);
			}
		}

		void AnkerDeviceMaterialCombo::OnDrawBackground(wxDC& dc,
			const wxRect& rect,
			int item,
			int flags)const
		{
			wxRect thisRect = GetClientRect();
			dc.SetBrush(m_bgColor);
			dc.SetPen(m_bgColor);
			dc.DrawRoundedRectangle(thisRect, 0);

			if (this->GetSelection() == wxNOT_FOUND)
			{
				// draw red color border for combobox
				dc.SetBrush(wxBrush(m_bgColor));
				dc.SetPen(wxPen(wxColor("#FF0000")));
				dc.DrawRoundedRectangle(thisRect, 0);

				//dc.SetBrush(m_bgColor);
				//dc.SetPen(m_bgColor);
				//dc.DrawRoundedRectangle(thisRect.Deflate(1), 5);
			}
		}

		void AnkerDeviceMaterialCombo::DrawButton(wxDC& dc,
			const wxRect&
			rect,
			int flags)
		{
			wxRect thisRect = GetClientRect();
			dc.SetBrush(m_bgColor);
			dc.SetPen(m_bgColor);
			dc.DrawRoundedRectangle(thisRect, 0);

			m_btnRect = rect;
			flags = Button_PaintBackground | Button_BitmapOnly;

			wxOwnerDrawnComboBox::DrawButton(dc, rect, flags);
		}

		const wxRect AnkerDeviceMaterialCombo::GetButtonRect()
		{
			return m_btnRect;
		}

		//AnkerDeviceMaterialCombo::AnkerDeviceMaterialCombo(wxWindow* parent,
		//	wxWindowID 	id ,
		//	const wxString& value,
		//	const wxPoint& pos,
		//	const wxSize& size,
		//	const wxArrayString& choices,
		//	long 	style,
		//	const wxValidator& validator,
		//	const wxString& name
		//): wxOwnerDrawnComboBox(parent, id, value, pos, size, choices, style, validator, name)
		//{
		//	initData();
		//}

		void AnkerDeviceMaterialCombo::initData()
		{
			m_leftGapWidth = 42;
			m_topGapWidth = 6;
			m_diameter = 18;
		}

		wxCoord AnkerDeviceMaterialCombo::OnMeasureItem(size_t item) const
		{
			// Simply demonstrate the ability to have variable-height items
			//	return FromDIP(item & 1 ? 36 : 24);
			return m_itemHeight;
		}

		wxCoord AnkerDeviceMaterialCombo::OnMeasureItemWidth(size_t WXUNUSED(item)) const
		{
			//return -1; // default - will be measured from text width
			//wxDC dc;

			int maxLabelTextWidth = 0;
			int maxmaterialTextWidth = 0;

			int cnt = GetCount();
			for (int i = 0; i < cnt; ++i)
			{
				AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(i);
				if (comboBoxItem)
				{
					maxLabelTextWidth = std::max(maxLabelTextWidth, this->GetTextExtent(comboBoxItem->label).GetWidth());
					maxmaterialTextWidth = std::max(maxmaterialTextWidth, this->GetTextExtent(comboBoxItem->material).GetWidth());
				}
			}
			return m_LeftSpan + maxLabelTextWidth + m_LabelMaterialSpan + maxmaterialTextWidth + m_MaterialColorSpan + m_diameter + m_RightSpan;;
		}

		void AnkerDeviceMaterialCombo::OnItemSelected(wxCommandEvent& event) {
			int selectedIndex = GetSelection();
			AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(selectedIndex);
			if (!comboBoxItem)
			{
				wxWindow* pHanlePanel = GetParent();
				if (pHanlePanel != nullptr)
				{
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_COMBO_SELECT_CHANGE);
					evt.SetClientData((void*)comboBoxItem);
					wxPostEvent(pHanlePanel, evt);
				}
				return;
			}
			// ANKER_LOG_INFO << "===xxx===OnItemSelected, idx:" << selectedIndex<< "   lable:"<< comboBoxItem->label<<"  enable:"<< comboBoxItem->enable;
			if (comboBoxItem->enable) {
				wxComboCtrl::HidePopup();
				wxWindow* pHanlePanel = GetParent();
				if (pHanlePanel != nullptr)
				{
					wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_COMBO_SELECT_CHANGE);
					evt.SetClientData((void*)comboBoxItem);
					wxPostEvent(pHanlePanel, evt);
				}
			}
			else
			{
				// item is disable, not hide popup
			}
		}


		void AnkerDeviceMaterialCombo::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC dc(this);
			wxRect thisRect = GetClientRect();

			dc.SetBrush(m_bgColor);
			dc.SetPen(m_bgColor);
			dc.DrawRoundedRectangle(thisRect, 0);

			wxRect btnRect = thisRect;
			btnRect.width = AnkerLength(16);
			btnRect.x = thisRect.GetWidth() - btnRect.width;
			DrawButton(dc, btnRect, 0);

			wxRect itemRect = thisRect;
			itemRect.width = thisRect.GetWidth() - btnRect.width;

			if (this->GetSelection() == wxNOT_FOUND)
			{
				// draw red color border for combobox
				dc.SetBrush(wxBrush(m_bgColor, wxBRUSHSTYLE_TRANSPARENT));
				dc.SetPen(wxPen(wxColor("#FF0000")));
				dc.DrawRoundedRectangle(thisRect, 0);
			}
			else
			{
				int selectedIndex = GetSelection();
				AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(selectedIndex);
				if (!comboBoxItem)
					return;

				DrawItem(dc, itemRect, comboBoxItem, false);
			}

			event.Skip(false);
		}

		/*
		void AnkerDeviceMaterialCombo::Dismiss()
		{
		}
		*/

		void AnkerDeviceMaterialCombo::HidePopup(bool  generateEvent)
		{
			// change the default behavior of HidePopup , ( if click the AnkerBitmapComboxItem.enable == true item ,not hide popup)
			AnkerBitmapComboxItem* comboBoxItem = (AnkerBitmapComboxItem*)GetClientData(GetSelection());
			if (comboBoxItem) {
				if (comboBoxItem->enable == false)
					SetSelection(wxNOT_FOUND);
			}
			return;
		}

		AnkerMaterialMapItem::AnkerMaterialMapItem(wxWindow* parent, AnkerMaterialMapItemViewModel* pItemViewModel, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
			AnkerBasePanel(parent, id, pos, size, style, name), m_pItemViewModel(pItemViewModel)
		{
			if (pItemViewModel == nullptr)
			{
				ANKER_LOG_ERROR << "AnkerMaterialMapItemViewModel is nullptr,please check";
				return;
			}
			m_filamentName = pItemViewModel->m_filamentNameVM;
			m_gCodeFilamentColour = pItemViewModel->m_gcodeFilamentColorVM;
			m_deviceFilamentInfo = pItemViewModel->m_deviceFilamentInfoVM;
			initData();
			initUI();
			initEvent();
		}
		void AnkerMaterialMapItem::initUI()
		{
			wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
			SetSizer(pSizer);
			wxStaticText* pFilamentText = new wxStaticText(this, wxID_ANY, _L(m_filamentName), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
			pFilamentText->SetBackgroundColour(m_bgColor);
			pFilamentText->SetForegroundColour(m_fgColor);
			pFilamentText->SetFont(ANKER_FONT_NO_1);
			pFilamentText->SetMinSize(AnkerSize(70, -1));
			wxClientDC dc(pFilamentText);
			dc.SetFont(ANKER_FONT_NO_1);
			wxSize textSize = dc.GetTextExtent(pFilamentText->GetLabel());
			int textHeight = textSize.GetHeight();
			int iSpan = (AnkerLength(30) - textHeight) / 2;
			pSizer->AddSpacer(AnkerLength(30));
			pSizer->Add(pFilamentText, 1, wxTOP | wxBOTTOM, iSpan);
			AnkerGcodeMaterialPanel* pGocdeFilamentColorPanel = new AnkerGcodeMaterialPanel(this);
			pGocdeFilamentColorPanel->SetMinSize(AnkerSize(41, 30));
			pGocdeFilamentColorPanel->SetMaxSize(AnkerSize(41, 30));
			pGocdeFilamentColorPanel->setFilamentColor(m_gCodeFilamentColour);
			pSizer->Add(pGocdeFilamentColorPanel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, AnkerLength(10));

			wxImage mapBitmap = wxImage(wxString::FromUTF8(Slic3r::var("maping.png")), wxBITMAP_TYPE_PNG);
			wxStaticBitmap* usedFIcon = new wxStaticBitmap(this, wxID_ANY, mapBitmap);
			usedFIcon->SetMinSize(mapBitmap.GetSize());
			pSizer->Add(usedFIcon, 0, wxALIGN_CENTER_VERTICAL |wxRIGHT, AnkerLength(10));

			wxArrayString   m_colorArrayItems{};
			m_printerMaterialCombo = new AnkerDeviceMaterialCombo();
			m_printerMaterialCombo->initData();
			m_printerMaterialCombo->Create(this,
				wxID_ANY,
				wxEmptyString,
				wxDefaultPosition,
				AnkerSize(250, AnkerLength(ANKER_COMBOBOX_HEIGHT)),
				m_colorArrayItems,
				wxNO_BORDER | wxCB_READONLY);

			m_printerMaterialCombo->setComBoxItemHeight(AnkerLength(ANKER_COMBOBOX_HEIGHT));
			m_printerMaterialCombo->SetMinSize(AnkerSize(82, 30));
			for (int i = 0; i < m_deviceFilamentInfo.size(); i++)
			{
				bool itemEnable = true;
				itemEnable = checkItemEnable(m_deviceFilamentInfo[i]);
				// testing data					
#if LOCAL_SIMULATE_V6   
				if (i == 3)
					itemEnable = false;
#endif
				wxString strFiamentName = m_deviceFilamentInfo[i].strMaterialName.IsEmpty()? UNKNOWN_MATERIAL : m_deviceFilamentInfo[i].strMaterialName;
				if (!itemEnable)
				{
					strFiamentName = DISABLED_MATERIAL;
				}

				AnkerBitmapComboxItem* comboItem = new AnkerBitmapComboxItem(std::to_string(m_deviceFilamentInfo[i].iNozzelInx + 1) + "#", strFiamentName, m_deviceFilamentInfo[i].filamentColor, m_deviceFilamentInfo[i].nozzleStatus, itemEnable);

				m_printerMaterialCombo->SetClientData(
					m_printerMaterialCombo->Append(m_deviceFilamentInfo[i].strMaterialName),
					comboItem);
		}
			m_printerMaterialCombo->SetFont(ANKER_FONT_NO_1);
			m_printerMaterialCombo->SetBackgroundColour(wxColour("#232426"));
			m_printerMaterialCombo->setColor(wxColour("#232426"), wxColour("#232426"), wxColour("#ffffff"));
			wxImage btnImage(wxString::FromUTF8(Slic3r::var("drop_down.png")), wxBITMAP_TYPE_PNG);
			btnImage.Rescale(8, 8, wxIMAGE_QUALITY_HIGH);
			wxBitmapBundle dropBtnBmpNormal = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
			wxBitmapBundle dropBtnBmpPressed = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
			wxBitmapBundle dropBtnBmpHover = wxBitmapBundle::FromBitmap(wxBitmap(btnImage));
			m_printerMaterialCombo->SetButtonBitmaps(dropBtnBmpNormal, true, dropBtnBmpPressed, dropBtnBmpHover);
			pSizer->Add(m_printerMaterialCombo, 0, wxALIGN_CENTER_VERTICAL);
			pSizer->AddSpacer(AnkerLength(30));
			pSizer->Layout();

	}
		void AnkerMaterialMapItem::setAutoMatching()
		{
			//TODO: set automatching by matching rulers

		}
		bool AnkerMaterialMapItem::checkItemEnable(DeviceFilementInfo filamentInfo)
		{
			bool bRet = true; 
			if (filamentInfo.nozzleStatus != 0)
			{
				bRet = false;
			}
			return bRet;
		}
		void AnkerMaterialMapItem::initEvent()
		{
			Bind(wxCUSTOMEVT_COMBO_SELECT_CHANGE, [this](wxCommandEvent& event) {
				AnkerBitmapComboxItem* pData = (AnkerBitmapComboxItem*)(event.GetClientData());

					if (this->m_pItemViewModel != nullptr)
					{
						if (pData != nullptr)
						{
							this->m_pItemViewModel->m_mappedFilamentName = pData->material;
							int iMapInx;
							pData->label.ToInt(&iMapInx);
							this->m_pItemViewModel->m_MapedInx = iMapInx;
						}
						else
						{
							//set to -1 describe not setting a mapping value
							this->m_pItemViewModel->m_MapedInx = -1;
						}
						wxWindow* pHanleWindow = GetParent();
						if (pHanleWindow != nullptr)
						{
							wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_CHECK_SELECTION);
							wxPostEvent(pHanleWindow, evt);
						}
						else
						{
							ANKER_LOG_ERROR << "wxCUSTOMEVT_CHECK_SELECTION event send fail, handle window is null,please check ";
						}
					}
				});
		}

		void AnkerMaterialMapItem::initData()
		{
#if LOCAL_SIMULATE_V6
			simulateData();
#endif // LOCAL_SIMULATE_V6
		}
		void AnkerMaterialMapItem::simulateData()
		{
			GcodeFilementInfo gCodeInfoTmp;
			DeviceFilementInfo devcieFilementInfoTmp;

			int64_t materialIdArr[6] = { 10000001,10000002 ,10000003 ,10000004 ,10000005 ,10000006 };
			int64_t colourlIdArr[6] = { 10000001,10000002 ,10000003 ,10000004 ,10000005 ,10000006 };
			wxString strMaterialNameArr[] = { "PLA","PLA+","PLA+Basic","PLA","PLA+","PLA+Basic" };
			wxString colourNameArr[] = { "#FF0000","#FF0000","#00FF00","#00FF00","#0000FF" ,"#0000FF" };

			for (int i = 0; i < 6; i++)
			{
				gCodeInfoTmp.iCoLorId = i;
				gCodeInfoTmp.strMaterialName = strMaterialNameArr[i];
				gCodeInfoTmp.filamentColor = wxColor(colourNameArr[i]);
				devcieFilementInfoTmp.iNozzelInx = i;
				devcieFilementInfoTmp.strMaterialName = strMaterialNameArr[i];
				devcieFilementInfoTmp.filamentColor = wxColor(colourNameArr[i]);
				devcieFilementInfoTmp.nozzleStatus = (i != 4 ? Normal : CutOff);
				m_deviceFilamentInfo.push_back(devcieFilementInfoTmp);
			}
		}
		AnkerGcodeMaterialPanel::AnkerGcodeMaterialPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
			:AnkerBasePanel(parent, id, pos, size, style, name)
		{
			initData();

		}
		void AnkerGcodeMaterialPanel::initData()
		{
#if LOCAL_SIMULATE_V6
			simulateData();
#endif // LOCAL_SIMULATE_V6
			setbgColor(wxColour("#232426"));
			m_iHorizonSpan = 11;
			m_iVerticalSpan = 6;
		}
		void AnkerGcodeMaterialPanel::initUI()
		{

		}
		void AnkerGcodeMaterialPanel::initEvent()
		{
		}
		void AnkerGcodeMaterialPanel::simulateData()
		{
			m_filamentColour = wxColor("#3096FF");
		}
		void AnkerGcodeMaterialPanel::OnPaint(wxPaintEvent& event)
		{
			wxAutoBufferedPaintDC dc(this);
			dc.SetBackground(m_bgColor);
			dc.Clear(); // Clear the background
			wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
			if (gc)
			{
				gc->SetAntialiasMode(wxANTIALIAS_DEFAULT); // Enable anti-aliasing
				wxSize size = GetSize();
				gc->SetBrush(m_filamentColour);
				//draw spacer area  background
				int iDiameter = std::min(size.GetWidth() - 2 * m_iHorizonSpan, size.GetHeight() - 2 * m_iVerticalSpan);
				int iLeftSpan, iTopSpan;
				if (m_iHorizonSpan > m_iVerticalSpan)
				{
					iLeftSpan = size.GetWidth() - 2 * m_iHorizonSpan;
					iTopSpan = m_iVerticalSpan;
				}
				else
				{
					iTopSpan = size.GetWidth() - 2 * m_iVerticalSpan;
					iTopSpan = m_iHorizonSpan;
				}

				gc->DrawEllipse(m_iHorizonSpan, m_iVerticalSpan, iDiameter, iDiameter);
				delete gc;
			}
		}

		BEGIN_EVENT_TABLE(AnkerGcodeMaterialPanel, wxPanel)
			EVT_PAINT(AnkerGcodeMaterialPanel::OnPaint)
			END_EVENT_TABLE()
		}
		}

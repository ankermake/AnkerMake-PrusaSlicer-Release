#pragma once
#include <map>
#include "wx/odcombo.h"
#include <wx/wx.h>
#include "AnkerBaseCtls.h"
#include "slic3r/GUI/ViewModel/AnkerMaterialMappingViewModel.h"

namespace Slic3r {
	namespace GUI {


wxDECLARE_EVENT(wxCUSTOMEVT_COMBO_SELECT_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(wxCUSTOMEVT_CHECK_SELECTION, wxCommandEvent);


		struct AnkerBitmapComboxItem
		{
			wxString label;
			wxString material;
			wxColor color;
			bool enable;
			NozzleStatus nozzleStatus;

			AnkerBitmapComboxItem(wxString Label, wxString Material, wxColor Colour, NozzleStatus NozzleStatus ,bool Enable = true) :
				label(Label),
				material(Material),
				color(Colour),
				enable(Enable),
				nozzleStatus(NozzleStatus)
			{

			}
		};

		class AnkerDeviceMaterialCombo: public  wxOwnerDrawnComboBox
		{
		public:
			/*AnkerDeviceMaterialCombo(wxWindow* parent,
				wxWindowID 	id = wxID_ANY,
				const wxString& value = wxEmptyString,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxSize(250, AnkerLength(ANKER_COMBOBOX_HEIGHT)),
				const wxArrayString& choices = wxArrayString(),
				long 	style = 0,
				const wxValidator& validator = wxDefaultValidator,
				const wxString& name = "comboBox"
			);*/
			AnkerDeviceMaterialCombo()
			{
			
				Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, &AnkerDeviceMaterialCombo::OnItemSelected, this);
				Bind(wxEVT_PAINT, &AnkerDeviceMaterialCombo::OnPaint, this);
			}
			void initData();
			virtual void setColor(const wxColor& borderColor,
				const wxColor& bgColor,
				const wxColor& textColor = wxColour("#FFFFFF"));
			virtual void setComBoxItemHeight(const int& height);

		protected:
			
			virtual void OnDrawItem(wxDC& dc,
				const wxRect& rect,
				int item,
				int flags) const;

			virtual void OnDrawBackground(wxDC& dc,
				const wxRect& rect,
				int item,
				int flags) const;

			virtual void DrawButton(wxDC& dc,
				const wxRect&
				rect,
				int flags);
			virtual const wxRect GetButtonRect();

			virtual wxCoord OnMeasureItem(size_t item) const wxOVERRIDE;

			virtual wxCoord OnMeasureItemWidth(size_t WXUNUSED(item)) const wxOVERRIDE;

			//virtual void Dismiss() override;
			virtual void HidePopup(bool  generateEvent = false) override;
			void OnItemSelected(wxCommandEvent& event);
			void OnPaint(wxPaintEvent& event);

			void DrawItem(wxDC& dc, wxRect& rect, AnkerBitmapComboxItem* pItem, bool drawMaterialText) const;
		private:
			wxColor		m_borderColor = wxColor("#434447");
			wxColor		m_bgColor = wxColor("#292A2D");
			wxColor		m_textColor = wxColour("#FFFFFF");
			wxColor		m_textColorDisable = wxColour("#808080");
			int			m_itemHeight = 24;
			wxRect		m_btnRect;
			int			m_leftGapWidth;
			int			m_topGapWidth;
			int			m_diameter;

			int m_LeftSpan = 6;
			int m_LabelMaterialSpan = 30;		// span between Label and Material
			int m_MaterialColorSpan = 6;		// span between Label and Material
			int m_RightSpan = 6;
		};


		class AnkerGcodeMaterialPanel: public AnkerBasePanel
		{
		public:

			AnkerGcodeMaterialPanel(wxWindow* parent,
				wxWindowID 	id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long 	style = wxTAB_TRAVERSAL,
				const wxString& name = wxPanelNameStr);
			void initData();
			void initUI();
			void initEvent();
			void simulateData();
			void setFilamentColor(wxColor newColor) { m_filamentColour = newColor; }
			void setbgColor(wxColor newColor) { m_bgColor = newColor; }
			void OnPaint(wxPaintEvent& event);
		private:
			wxColor m_bgColor;
			wxColor m_filamentColour;
			int m_iHorizonSpan;
			int m_iVerticalSpan;

			DECLARE_EVENT_TABLE()
		};

		class AnkerMaterialMapItem :public AnkerBasePanel
		{
		public:
			 AnkerMaterialMapItem(wxWindow* parent,
				AnkerMaterialMapItemViewModel* pItemViewModel,
				wxWindowID 	id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long 	style = wxTAB_TRAVERSAL,
				const wxString& name = wxPanelNameStr);
			void initUI();
			void setAutoMatching();
			bool checkItemEnable(DeviceFilementInfo filamentInfo);
			void initEvent();
			void initData();
			void simulateData();
			void updateViewModel()
			{
				if (m_pItemViewModel != nullptr)
				{
					m_pItemViewModel->m_MapedInx = getDeviceComboSelectItem();
					if (m_pItemViewModel->m_MapedInx == -1)
					{
						m_pItemViewModel->clear();
					}
				}
			}

			int getDeviceComboSelectItem()
			{
				int iSelectedInx = wxNOT_FOUND;
				if (m_printerMaterialCombo != nullptr)
				{
					iSelectedInx = m_printerMaterialCombo->GetSelection();
				}
				return  iSelectedInx;
			}

			AnkerMaterialMapItemViewModel* getMapItemVm() {return m_pItemViewModel;}

		private:
			wxString m_filamentName;
			wxColor m_gCodeFilamentColour;
			wxColor m_mappedNozzleColur;
			std::vector<DeviceFilementInfo> m_deviceFilamentInfo;
			AnkerDeviceMaterialCombo* m_printerMaterialCombo;
			AnkerMaterialMapItemViewModel* m_pItemViewModel;
		};


		typedef struct tagMapCheckResult {
		public:
			bool bAllMapped;
			bool bSelectUnkonwnFilament;
		
			tagMapCheckResult()
			{
				bSelectUnkonwnFilament = false;
				bAllMapped = false;
			}

		}MapCheckResult;


		class AnkerMaterialMappingPanel : public AnkerBasePanel
		{
		public:
			AnkerMaterialMappingPanel(	wxWindow* parent,
							std::string strDeviceSn,
							AnkerMaterialMappingViewModel* pViewModel,
							wxWindowID id = wxID_ANY,
							const wxPoint& pos = wxDefaultPosition,
							const wxSize& size = wxDefaultSize,
							long style = wxSB_HORIZONTAL,
							const wxString& name = wxScrollBarNameStr);


			~AnkerMaterialMappingPanel()
			{
				for (int i = 0; i< m_pMaterialItemViewModelVec.size();i++)
				{
					delete m_pMaterialItemViewModelVec[i];
				}
			}
			
			void InitData();
			void InitUI();
			void initEvent();
			void SimulateData();
			void OnPrintBtn(wxCommandEvent& event);
			void updateDevcieItemViewModel();
			MapCheckResult IsDeviceFilamentMapped();
			void SetGcodeImportDialog(wxDialog* dialog) { m_gcodeImportDialog = dialog; };

		private:
			std::map<GcodeFilementInfo, std::vector<DeviceFilementInfo>> m_filamentMap;
			std::vector<DeviceFilementInfo> m_allDeviceFialmentInfo;
			wxColour m_dialogColor = wxColour(PANEL_BACK_LIGHT_RGB_INT);
			wxColour m_textLightColor = wxColour(TEXT_LIGHT_RGB_INT);
			wxColour m_splitLineColor;
			wxColour m_textDarkColor;
			wxColour m_noticeTextClolor;
			std::string m_currentDeviceSn;
			AnkerMaterialMappingViewModel* m_ViewModel;
			std::vector<AnkerMaterialMapItemViewModel*> m_pMaterialItemViewModelVec;
			std::vector<AnkerMaterialMapItem*> m_pMaterialItemViewVec;
			wxScrolledWindow* m_scrolledWindow;
			AnkerBasePanel* m_NoticePanel;
			AnkerBtn* m_pPrintBtn;
			wxStaticText* m_noticeText;
			wxDialog* m_gcodeImportDialog{ nullptr };
		};
	}
}

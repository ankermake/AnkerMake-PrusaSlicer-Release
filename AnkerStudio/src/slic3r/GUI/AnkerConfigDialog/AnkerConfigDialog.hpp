#ifndef _ANKER_CONFIG_DIALOG_HPP_
#define _ANKER_CONFIG_DIALOG_HPP_

#include <wx/dialog.h>
#include "../AnkerCfgTab.hpp"
#include "../PresetComboBoxes.hpp"

wxDECLARE_EVENT(wxCUSTOMEVT_UPDATE_PARAMETERS_PANEL, wxCommandEvent);

namespace Slic3r {
    namespace GUI {
        class AnkerConfigDlg : public AnkerDPIDialog
        {
        public:
            //AnkerConfigDlg(wxWindow* parent);
            AnkerConfigDlg(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxString& title = wxEmptyString,
                const wxString& url = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
#ifdef __APPLE__
                long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER,
#else
                long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER,
#endif
                const wxString& name = wxDialogNameStr);
            ~AnkerConfigDlg();
            void AddCreateTab(AnkerTab* panel, const std::string& bmp_name = "");
            AnkerTabPresetComboBox* GetAnkerTabPresetCombo(const Preset::Type type);
            void switchAnkerTab(const Preset::Type type);
            void updateAnkerTabComBoxHighlight(const Preset::Type type);
            void resetPresetComBoxHighlight();
            void CloseDlg();
            void msw_rescale();

            wxSize GetRightPanelSize();
            void ChangeAnkerTabComboSel(AnkerTabPresetComboBox* presetChoice, const int selection);

        protected:
            virtual void on_dpi_changed(const wxRect& suggested_rect) override;
        private:
            void initUI();
            wxIcon mainFrameIcon();

            void initEventBind();

            void OnSize(wxSizeEvent& event);
            void OnClose(wxCloseEvent& event);
            void OnLeftDown(wxMouseEvent& event);
            void OnLeftUp(wxMouseEvent& event);
            void OnMouseMove(wxMouseEvent& event);
            void OnMouseLost(wxMouseCaptureLostEvent& event);
            
            void OnExitButtonClicked(wxCommandEvent& event);

            
            void onComboBoxCloseUp(wxCommandEvent& event);
            void onPresetRightClick(wxMouseEvent& event);
            void onPresetOpPopupClick(wxCommandEvent& event);
            void onComboBoxClick(AnkerTabPresetComboBox* presetComboBox);
            
            bool handleDirtyPreset();

        public:
            wxBookCtrlBase* m_rightPanel{ nullptr };

            AnkerTabPresetComboBox* m_printerPresetsChoice{nullptr};
            AnkerTabPresetComboBox* m_filamentPresetsChoice{ nullptr };
            AnkerTabPresetComboBox* m_printPresetsChoice{ nullptr };
            AnkerTabPresetComboBox* m_lastSelectPreset{nullptr};

        private:
            wxWindow* m_parent{ nullptr };
            // sizer
            wxBoxSizer* m_mainSizer{ nullptr };
            wxBoxSizer* m_contentSizer{ nullptr };
            wxBoxSizer* m_leftPanelSizer{ nullptr };
            wxBoxSizer* m_rightPanelSizer{ nullptr };

            // panel
            wxPanel* m_titlePanel{ nullptr };
            wxStaticText* m_titleText{nullptr};
            ScalableButton* m_exitBtn{ nullptr };
            wxPanel* m_leftPanel{ nullptr };
            wxPanel* m_leftPresetPanel{ nullptr };
            wxSize m_rightPanelSize;

            // move dialog 
            wxPoint m_startPos;
            wxRect  m_titlePanelRect;
        };
    }
}
#endif

#ifndef slic3r_GUI_ObjectSettings_hpp_
#define slic3r_GUI_ObjectSettings_hpp_

#include <memory>
#include <vector>
#include <wx/panel.h>
#include "wxExtensions.hpp"
#include "ObjectDataViewModel.hpp"
class wxBoxSizer;

namespace Slic3r {
class DynamicPrintConfig;
class ModelConfig;
namespace GUI {
class ConfigOptionsGroup;

class OG_Settings
{
protected:
    std::shared_ptr<ConfigOptionsGroup> m_og;
    wxWindow* m_parent;
public:
    OG_Settings(wxWindow* parent, const bool staticbox);
    virtual ~OG_Settings() {}

    virtual bool        IsShown();
    virtual void        Show(const bool show);
    virtual void        Hide();
    virtual void        UpdateAndShow(const bool show);

    virtual wxSizer*    get_sizer();
    ConfigOptionsGroup* get_og() { return m_og.get(); }
    const ConfigOptionsGroup* get_og() const { return m_og.get(); }
    wxWindow*           parent() const {return m_parent; }
};


class ObjectSettings : public OG_Settings
{
    // sizer for extra Object/Part's settings
    wxBoxSizer* m_settings_list_sizer{ nullptr };  
    // option groups for settings
    std::vector <std::shared_ptr<ConfigOptionsGroup>> m_og_settings;

    ScalableBitmap m_bmp_delete;
    ScalableBitmap m_bmp_delete_focus;

public:
    ObjectSettings(wxWindow* parent);
    ~ObjectSettings() {}

    bool        update_settings_list();
    /* Additional check for override options: Add options, if its needed.
     * Example: if Infill is set to 100%, and Fill Pattern is missed in config_to,
     * we should add fill_pattern to avoid endless loop in update
     */
    bool        add_missed_options(ModelConfig *config_to, const DynamicPrintConfig &config_from);
    void        update_config_values(ModelConfig *config);
    void        UpdateAndShow(const bool show) override;
    void        sys_color_changed();
};

class AnkerObjectSettings
{
public:
    AnkerObjectSettings() = default;
    ~AnkerObjectSettings() = default;

    void UpdateAndShow(bool show);
    bool update_settings_list(bool enable);

    std::map<t_config_option_key, ConfigOption*> GetCalibrationValue (ModelConfig* config, int &extruder);
    bool UpdateModelConfig(const std::map<t_config_option_key, ConfigOption*>& calibrations, ModelConfig* config, int extruder);
    ModelConfig* GetSelsItemModelConfig() const { return m_model_config_sels; }
    ObjectDataViewModelNode* GetNode() const { return m_node;  }

private:
    void SetItemModelConfig(ModelConfig*& config);

private:
    bool m_bindFlag{ false };
    void* m_ID{ nullptr };

    ObjectDataViewModelNode* m_node { nullptr };
    ModelConfig* m_model_config_sels { nullptr};
};


}}

#endif // slic3r_GUI_ObjectSettings_hpp_

#ifndef slic3r_AppConfig_hpp_
#define slic3r_AppConfig_hpp_

#include <set>
#include <map>
#include <string>

#include <boost/algorithm/string/trim_all.hpp>

#include "libslic3r/Config.hpp"
#include "libslic3r/Semver.hpp"

namespace Slic3r {

struct SliceConfig {
    std::string slice_key;  // user_id + version_id
    std::string user_id;
    std::string version_id;
    int slice_times;
    SliceConfig():slice_key(""),user_id(""),version_id("0"),slice_times(0) {}
    //SliceConfig(const SliceConfig& sl) { slice_key = sl.slice_key; user_id = sl.user_id; version_id = sl.version_id; slice_times = sl.slice_times; }
};

class AppConfig
{
public:
	enum class EAppMode : unsigned char
	{
		Editor,
		GCodeViewer
	};

	explicit AppConfig(EAppMode mode) :
		m_mode(mode)
	{
		this->reset();
	}

	// Clear and reset to defaults.
	void 			   	reset();
	// Override missing or keys with their defaults.
	void 			   	set_defaults();

	// Set Default Printer List info to config
	void initDefaultPrinterList();
	
	// Load the slic3r.ini from a user profile directory (or a datadir, if configured).
	// return error string or empty strinf
	std::string         load();
	// Load from an explicit path.
	std::string         load(const std::string &path);
	// Store the slic3r.ini into a user profile directory (or a datadir, if configured).
	void 			   	save();

	// Does this config need to be saved?
	bool 				dirty() const { return m_dirty; }

	// Const accessor, it will return false if a section or a key does not exist.
	bool get(const std::string &section, const std::string &key, std::string &value) const
	{
		value.clear();
		auto it = m_storage.find(section);
		if (it == m_storage.end())
			return false;
		auto it2 = it->second.find(key);
		if (it2 == it->second.end()) 
			return false;
		value = it2->second;
		return true;
	}
	std::string 		get(const std::string &section, const std::string &key) const
		{ std::string value; this->get(section, key, value); return value; }
	bool  				get_bool(const std::string &section, const std::string &key) const
		{ return this->get(section, key) == "1"; }
	std::string 		get(const std::string &key) const
		{ std::string value; this->get("", key, value); return value; }
	bool  				get_bool(const std::string &key) const
		{ return this->get(key) == "1"; }
    
    bool get_slice_times(const const std::string& section, const std::string& sliceKey, int& sliceTimes) {
        auto it = m_slice_config.find(section);
        if (it == m_slice_config.end())
            return false;

        auto it2 = it->second.find(sliceKey);
        if (it2 == it->second.end())
            return false;
        if (it2->second != nullptr) {
            sliceTimes = it2->second->slice_times;
            return true;
        }
        return false;
    }
    bool set_slice_times(const const std::string& section, const std::string& sliceKey, const std::string& userId, const std::string& versionId, int sliceTimes) {
#ifndef NDEBUG
        {
            std::string key_trimmed = sliceKey;
            boost::trim_all(key_trimmed);
            assert(key_trimmed == sliceKey);
            assert(!key_trimmed.empty());
        }
#endif // NDEBUG
        if (auto it = m_slice_config.find(section); it != m_slice_config.end()) {

            auto it2 = it->second.find(sliceKey);
            if (it2 == it->second.end()) {
                SliceConfig sliceConfig;
                sliceConfig.slice_key = sliceKey;
                sliceConfig.user_id = userId;
                sliceConfig.version_id = versionId;
                sliceConfig.slice_times = sliceTimes;
                m_slice_config[section].emplace(sliceKey, std::make_shared<SliceConfig>(sliceConfig));
                m_dirty = true;
                return true;
            } else {
                std::shared_ptr<SliceConfig>& old = m_slice_config[section][sliceKey]; // rightvalue refer
                if (old == nullptr) {
                    return false;
                }
                m_dirty = true;
                old->slice_times = sliceTimes;
                old->user_id = userId;
                old->version_id = versionId;
                return true;
            }
        }else {
            SliceConfig sliceConfig;
            sliceConfig.slice_key = sliceKey;
            sliceConfig.user_id = userId;
            sliceConfig.version_id = versionId;
            sliceConfig.slice_times = sliceTimes;
            m_slice_config[section].emplace(sliceKey, std::make_shared<SliceConfig>(sliceConfig));
            m_dirty = true;
            return true;
        }
    }
    
	bool			    set(const std::string &section, const std::string &key, const std::string &value)
	{
#ifndef NDEBUG
		{
			std::string key_trimmed = key;
			boost::trim_all(key_trimmed);
			assert(key_trimmed == key);
			assert(! key_trimmed.empty());
		}
#endif // NDEBUG
		std::string &old = m_storage[section][key];
		if (old != value) {
			old = value;
			m_dirty = true;
			return true;
		}
		return false;
	}
	bool			    set(const std::string &key, const std::string &value)
		{ return this->set("", key, value);  }
	bool				has(const std::string &section, const std::string &key) const
	{
		auto it = m_storage.find(section);
		if (it == m_storage.end())
			return false;
		auto it2 = it->second.find(key);
		return it2 != it->second.end() && ! it2->second.empty();
	}
	bool				has(const std::string &key) const
		{ return this->has("", key); }

	bool				erase(const std::string &section, const std::string &key);

	bool                has_section(const std::string &section) const
		{ return m_storage.find(section) != m_storage.end(); }
	const std::map<std::string, std::string>& get_section(const std::string &section) const
		{ auto it = m_storage.find(section); assert(it != m_storage.end()); return it->second; }
	bool 				set_section(const std::string &section, std::map<std::string, std::string> data);
	bool 				clear_section(const std::string &section);
	bool 				delete_section(const std::string& section);

	typedef std::map<std::string, std::map<std::string, std::set<std::string>>> VendorMap;
	bool                get_variant(const std::string &vendor, const std::string &model, const std::string &variant) const;
	bool                set_variant(const std::string &vendor, const std::string &model, const std::string &variant, bool enable);
	bool                set_vendors(const AppConfig &from) { return this->set_vendors(from.vendors()); }
	bool 				set_vendors(const VendorMap &vendors);
	bool 				set_vendors(VendorMap &&vendors);
	const VendorMap&    vendors() const { return m_vendors; }

	// return recent/skein_directory or recent/config_directory or empty string.
	std::string 		get_last_dir() const;
	bool 				update_config_dir(const std::string &dir);
	bool 				update_skein_dir(const std::string &dir);

	//std::string 		get_last_output_dir(const std::string &alt) const;
	//void                update_last_output_dir(const std::string &dir);
	std::string 		get_last_output_dir(const std::string& alt, const bool removable = false) const;
	bool                update_last_output_dir(const std::string &dir, const bool removable = false);

	// reset the current print / filament / printer selections, so that 
	// the  PresetBundle::load_selections(const AppConfig &config) call will select
	// the first non-default preset when called.
    void                reset_selections();

	// Get the default config path from Slic3r::data_dir().
	std::string			config_path() const;

	// Returns true if the user's data directory comes from before Slic3r 1.40.0 (no updating)
	bool 				legacy_datadir() const { return m_legacy_datadir; }
	void 				set_legacy_datadir(bool value) { m_legacy_datadir = value; }

	// Get the Slic3r version check url.
	// This returns a hardcoded string unless it is overriden by "version_check_url" in the ini file.
	std::string 		version_check_url() const;
	// Get the Slic3r url to vendor index archive zip.
	std::string  		index_archive_url() const;
	// Get the Slic3r url to folder with vendor profile files.
	std::string 		profile_folder_url() const;


	// Returns the original Slic3r version found in the ini file before it was overwritten
	// by the current version
	Semver 				orig_version() const { return m_orig_version; }

	// Does the config file exist?
	bool 				exists() const;

    std::vector<std::string> get_recent_projects() const;
    bool set_recent_projects(const std::vector<std::string>& recent_projects);

	bool set_mouse_device(const std::string& name, double translation_speed, double translation_deadzone, float rotation_speed, float rotation_deadzone, double zoom_speed, bool swap_yz);
	std::vector<std::string> get_mouse_device_names() const;
	bool get_mouse_device_translation_speed(const std::string& name, double& speed) const
		{ return get_3dmouse_device_numeric_value(name, "translation_speed", speed); }
    bool get_mouse_device_translation_deadzone(const std::string& name, double& deadzone) const
		{ return get_3dmouse_device_numeric_value(name, "translation_deadzone", deadzone); }
    bool get_mouse_device_rotation_speed(const std::string& name, float& speed) const
		{ return get_3dmouse_device_numeric_value(name, "rotation_speed", speed); }
    bool get_mouse_device_rotation_deadzone(const std::string& name, float& deadzone) const
		{ return get_3dmouse_device_numeric_value(name, "rotation_deadzone", deadzone); }
	bool get_mouse_device_zoom_speed(const std::string& name, double& speed) const
		{ return get_3dmouse_device_numeric_value(name, "zoom_speed", speed); }
	bool get_mouse_device_swap_yz(const std::string& name, bool& swap) const
		{ return get_3dmouse_device_numeric_value(name, "swap_yz", swap); }

	static const std::string SECTION_FILAMENTS;
    static const std::string SECTION_MATERIALS;
    static const std::string SECTION_EMBOSS_STYLE;

private:
	template<typename T>
	bool get_3dmouse_device_numeric_value(const std::string &device_name, const char *parameter_name, T &out) const 
	{
	    std::string key = std::string("mouse_device:") + device_name;
	    auto it = m_storage.find(key);
	    if (it == m_storage.end())
	        return false;
	    auto it_val = it->second.find(parameter_name);
	    if (it_val == it->second.end())
	        return false;
        out = T(string_to_double_decimal_point(it_val->second));
	    return true;
	}

	// Type of application: Editor or GCodeViewer
	EAppMode													m_mode { EAppMode::Editor };
	// Map of section, name -> value
	std::map<std::string, std::map<std::string, std::string>> 	m_storage;
	// Map of enabled vendors / models / variants
	VendorMap                                                   m_vendors;
	// Has any value been modified since the config.ini has been last saved or loaded?
	bool														m_dirty;
	// Original version found in the ini file before it was overwritten
	Semver                                                      m_orig_version;
	// Whether the existing version is before system profiles & configuration updating
	bool                                                        m_legacy_datadir;
    // Map of section, key -> value
    std::map<std::string, std::map<std::string, std::shared_ptr<SliceConfig>>>   m_slice_config;
};
// "a=AAA,b=BBB,c=CCC,.." => {("a","AAA"),("b","BBB"),("c", "CCC"),...}
inline  std::pair<std::string, std::string>  extract_keyval(const std::string& str, const char& sep) {
    auto trim_space = [=](std::string& str)->std::string {
        const char* spaces = " \n\r\t";
        str.erase(str.find_last_not_of(spaces) + 1);
        str.erase(0, str.find_first_not_of(spaces));
        return str;
    };

    auto n = str.find(sep);
    std::string k, v;
    if (n == std::string::npos) {
        v = str;
    }
    else {
        k = str.substr(0, n);
        v = str.substr(n + 1);
    }
    return std::make_pair(trim_space(k), trim_space(v));
}

inline std::map<std::string, std::shared_ptr<SliceConfig>> load_slice_config(const std::string& str){
    std::map<std::string, std::shared_ptr<SliceConfig>> sliceInfo;
    std::string sliceKey{};

    std::string token;  std::istringstream token_stream(str);
    std::unordered_map<std::string, std::string> rv{};
    while (std::getline(token_stream, token, ',')) {
        if (token.empty()) {
            continue;
        }
        auto keyVal = extract_keyval(token, '=');

        if (keyVal.first == "slice_key") {
            sliceKey = keyVal.second;
            sliceInfo.emplace(sliceKey, std::make_shared<SliceConfig>());
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_key = sliceKey;
            }
        }
        if (keyVal.first == "user_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->user_id = keyVal.second;
            }
        }
        if (keyVal.first == "version_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->version_id = keyVal.second;
            }
            
        }

        if (keyVal.first == "slice_times") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_times = stoi(keyVal.second);
            }
        }
    }
    return sliceInfo;
}

inline std::map<std::string, std::shared_ptr<SliceConfig>> load_slice_config(const std::string& str, const std::string & strKey) {
    std::map<std::string, std::shared_ptr<SliceConfig>> sliceInfo;
    std::string sliceKey{};

    std::string token;  std::istringstream token_stream(str);
    std::unordered_map<std::string, std::string> rv{};
    while (std::getline(token_stream, token, ',')) {
        if (token.empty()) {
            continue;
        }
        auto keyVal = extract_keyval(token, '=');

        if (keyVal.first == strKey) {
            sliceKey = keyVal.second;
            sliceInfo.emplace(sliceKey, std::make_shared<SliceConfig>());
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_key = sliceKey;
            }
        }
        if (keyVal.first == "user_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->user_id = keyVal.second;
            }
        }
        if (keyVal.first == "version_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->version_id = keyVal.second;
            }

        }

        if (keyVal.first == "slice_times") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_times = stoi(keyVal.second);
            }
        }
    }
    return sliceInfo;
}

inline std::shared_ptr<SliceConfig> load_slice_config_final(const std::string& str, const std::string& strKey, std::string &sliceKey) {
    std::map<std::string, std::shared_ptr<SliceConfig>> sliceInfo;


    std::string token;  std::istringstream token_stream(str);
    std::unordered_map<std::string, std::string> rv{};
    while (std::getline(token_stream, token, ',')) {
        if (token.empty()) {
            continue;
        }
        auto keyVal = extract_keyval(token, '=');

        if (keyVal.first == strKey) {
            sliceKey = keyVal.second;
            sliceInfo.emplace(sliceKey, std::make_shared<SliceConfig>());
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_key = sliceKey;
            }
        }
        if (keyVal.first == "user_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->user_id = keyVal.second;
            }
        }
        if (keyVal.first == "version_id") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->version_id = keyVal.second;
            }

        }

        if (keyVal.first == "slice_times") {
            if (sliceInfo[sliceKey] != nullptr) {
                sliceInfo[sliceKey]->slice_times = stoi(keyVal.second);
            }
        }
    }
    return sliceInfo[sliceKey];
}
} // namespace Slic3r

#endif /* slic3r_AppConfig_hpp_ */

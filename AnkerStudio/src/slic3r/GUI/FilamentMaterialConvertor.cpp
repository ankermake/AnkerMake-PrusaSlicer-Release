#include "FilamentMaterialConvertor.hpp"

#include "FilamentMaterialManager.hpp"
#include "GUI_App.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"

#define ANKER_COLOR_ID_KEY "anker_colour_id"
#define ANKER_COLOR_VALUE_KEY "filament_colour"

#define ANKER_FILAMENT_ID_KEY "anker_filament_id"
#define ANKER_FILAMENT_TYPE_KEY "filament_type"

#define ANKER_ID_PREFIX_0x "0x"
#define ANKER_FILAMENT_NAME_PREFIX "AnkerMake"

//first is color id ,second is color value
std::map<wxString, wxString> ColorCache;

//first is filament id,second is filament name
std::map<wxString, wxString> FilamentCache;

std::map<wxString,wxString> FilamentCategory={
	{"1001","PLA"},
	{"1002","TPU"},
	{"1003","ABS"},
	{"1004","PETG"},
	{"1005","PLA-CF"},
	{"1006","PET-CF"},
	{"1007","PA-CF"},
	{"1008","PVA"},
};

namespace Slic3r {
	namespace GUI {


		bool CheckString0(const wxString& str)
		{
			int value = 0;
			str.ToInt(&value, 0);
			return value == 0;
		}

		wxString RemovePrefix0x(const wxString& str)
		{
			int len = wxString(ANKER_ID_PREFIX_0x).size();
			if (str.size() <= len)
				return str;
			if (str.substr(0, len).Lower() == wxString(ANKER_ID_PREFIX_0x).Lower())
				return str.substr(len);
			return str;
		}

		wxString RemovePrefixAnkerMake(const wxString& str)
		{
			int len = wxString(ANKER_FILAMENT_NAME_PREFIX).size();
			if (str.size() <= len)
				return str;
			if (str.substr(0, len).Lower() == wxString(ANKER_FILAMENT_NAME_PREFIX).Lower())
				return str.substr(len + 1);
			return str;
		}

		wxString ToHexadecimalString(int num)
		{
			char buffer[32] = { 0 };
			sprintf(buffer, "%x", num);
			return wxString(buffer);
		}

		wxString ColorValueToString(long colorValue)
		{
			return wxColour(colorValue).GetAsString(wxC2S_HTML_SYNTAX);
		}

		bool FilamentMaterialConvertor::TryConvertColorId(wxString colorId, wxString& colorValue, bool autoRemove0x)
		{
			if (autoRemove0x)
				colorId = RemovePrefix0x(colorId);

			if(CheckString0(colorId))
			{
				return false;
			}

			if (ColorCache.count(colorId) > 0)
			{
				colorValue = ColorCache[colorId];
				return !colorValue.empty();
			}

			std::string id_key = ANKER_COLOR_ID_KEY;
			std::string value_key = ANKER_COLOR_VALUE_KEY;

			auto bunble = wxGetApp().preset_bundle;
			auto presets = bunble->filaments.get_presets();
			for (int i = 0; i < presets.size(); ++i)
			{
				auto preset = presets[i];
				auto config = preset.config;
				auto keys = config.keys();
				auto colorIdOpt = config.optptr(id_key);
				auto colorValueOpt = config.optptr(value_key);
				if (colorIdOpt == nullptr || colorValueOpt == nullptr || colorIdOpt->type() != coStrings || colorValueOpt->type() != coStrings)
					continue;
				wxString id = config.opt_string(id_key, static_cast<unsigned int>(0));
				if (autoRemove0x)
					id = RemovePrefix0x(id);
				if (colorId == id)
				{
					colorValue = config.opt_string(value_key, static_cast<unsigned int>(0));
					ColorCache[colorId] = colorValue;
					return true;
				}
			}

			auto colorInfo = wxGetApp().filamentMaterialManager()->GetFilamentColorInfo();
			for (auto color : colorInfo)
			{
				if (colorId == RemovePrefix0x(color.uid))
				{
					colorValue = ColorValueToString(color.value);
					ColorCache[colorId] = colorValue;
					return true;
				}
			}
			ANKER_LOG_ERROR << "Can not find target color . Color ID = " << colorId;
			ColorCache[colorId] = wxString("");
			return false;
		}

		bool FilamentMaterialConvertor::TryConvertFilamentId(wxString filamentId, wxString& filamentName, bool autoRemove0x, bool autoRemoveAnkerMake)
		{
			if (autoRemove0x)
				filamentId = RemovePrefix0x(filamentId);

			if (CheckString0(filamentId))
			{
				return false;
			}

			if (FilamentCache.count(filamentId) > 0)
			{
				filamentName = FilamentCache[filamentId];
				return !filamentName.empty();
			}

			std::string id_key = ANKER_FILAMENT_ID_KEY;
			std::string name_key = ANKER_FILAMENT_TYPE_KEY;

			auto bunble = wxGetApp().preset_bundle;
			auto presets = bunble->filaments.get_presets();
			for (int i = 0; i < presets.size(); ++i)
			{
				auto preset = presets[i];
				auto config = preset.config;
				auto keys = config.keys();
				auto filamentIdOpt = config.optptr(id_key);
				auto nameOpt = config.optptr(name_key);
				if (filamentIdOpt == nullptr || nameOpt == nullptr || filamentIdOpt->type() != coStrings || nameOpt->type() != coStrings)
					continue;
				wxString id = config.opt_string(id_key, static_cast<unsigned int>(0));
				if (autoRemove0x)
					id = RemovePrefix0x(id);
				if (filamentId == id)
				{
					filamentName = config.opt_string(name_key, static_cast<unsigned int>(0));
					if (autoRemoveAnkerMake)
						filamentName = RemovePrefixAnkerMake(filamentName);
					FilamentCache[filamentId] = filamentName;
					return true;
				}
			}

			auto materials = wxGetApp().filamentMaterialManager()->GetFilamentMaterialInfo();
			for (auto material : materials)
			{
				if (filamentId == RemovePrefix0x(material.uid))
				{
					filamentName = material.Name();
					if (autoRemoveAnkerMake)
						filamentName = RemovePrefixAnkerMake(filamentName);
					FilamentCache[filamentId] = filamentName;
					return true;
				}
			}

			ANKER_LOG_ERROR << "Can not find target filament . Filament ID = " << filamentId;
			FilamentCache[filamentId] = wxString("");
			return false;
		}

		wxString FilamentMaterialConvertor::ConvertColorId(wxString colorId)
		{
			wxString colorValue;
			if (TryConvertColorId(colorId, colorValue))
				return colorValue;
			return "";
		}

		wxString FilamentMaterialConvertor::ConvertFilamentId(wxString filamentId)
		{
			wxString filamentName;
			if (TryConvertFilamentId(filamentId, filamentName))
				return filamentName;
			return "";
		}

		wxString FilamentMaterialConvertor::ConvertFilamentIdToCategory(wxString filamentId)
		{
			if (CheckString0(filamentId))
				return "";
			assert(filamentId.size() == 8);
			if (filamentId.size() != 8)
			{
				ANKER_LOG_ERROR << "Format of filamentId is illegal,length of hex must be 8. FilamentID = " << filamentId << ",hexString = " << filamentId;
				return "";
			}
			auto head = filamentId.substr(0, 4);

			assert(FilamentCategory.count(head) > 0);
			if (FilamentCategory.count(head) > 0)	
			{
				return FilamentCategory[head];
			}
			else
			{
				ANKER_LOG_ERROR << "Can't find Category by filamentId . FilamentID = " << filamentId << ",hexString = " << filamentId;
			}
			return "";
		}

		wxString FilamentMaterialConvertor::ConvertColorId(int colorId)
		{
			wxString colorIdStr = ToHexadecimalString(colorId);
			return ConvertColorId(colorIdStr);
		}

		wxString FilamentMaterialConvertor::ConvertFilamentId(int filamentId)
		{
			wxString filamentIdStr = ToHexadecimalString(filamentId);
			return ConvertFilamentId(filamentIdStr);
		}

		wxString  FilamentMaterialConvertor::ConvertFilamentIdToCategory(int filamentId)
		{
			auto filamentIdStr = ToHexadecimalString(filamentId);
			return ConvertFilamentIdToCategory(filamentIdStr);
		}

		void FilamentMaterialConvertor::ResetCache()
		{
			ColorCache.clear();
			FilamentCache.clear();
		}
	}
}

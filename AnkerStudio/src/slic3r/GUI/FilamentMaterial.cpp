#include "FilamentMaterial.hpp"
#include "3DBed.hpp"
#include "GUI_App.hpp"

namespace Slic3r {
	namespace GUI {

		wxString FilamentColor::Name()
		{
			auto lang = wxGetApp().current_language_code();
			if (names.count(lang) > 0)
			{
				return names[lang];
			}
			else if (names.count("en") > 0)
			{
				return names["en"];
			}
			else
			{
				return color;
			}
			//ANKER_LOG_ERROR << "Can't find name of Filament Color .uid = " << uid << ", Color = " << color << ". Lang = " << lang;
			return color;
		}

		wxString FilamentMaterial::Name()
		{
			auto lang = wxGetApp().current_language_code();
			if (names.count(lang) > 0)
			{
				return names[lang];
			}
			else if (names.count("en") > 0)
			{
				return names["en"];
			}
			else
			{
				return name;
			}
			//ANKER_LOG_INFO << "Can't find name of Filament Material .uid = " << uid << ", name = " << name << ". Lang = " << lang;
			return name;
		}
	}
}


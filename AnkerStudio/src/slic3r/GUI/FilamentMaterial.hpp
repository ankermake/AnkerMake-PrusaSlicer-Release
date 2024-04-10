#ifndef slic3r_GUI_FilamentMaterial_hpp_
#define slic3r_GUI_FilamentMaterial_hpp_

namespace Slic3r {
	namespace GUI {

		struct FilamentColor
		{
			wxString uid;
			wxString color;
			long value;
			std::map<wxString, wxString> names;//int GUI_App::AnkerLanguageType
			//Name for multi-language.
			wxString Name();
		};

		struct FilamentMaterial
		{
			wxString uid;
			wxString name;
			int plate_temp;
			int nozzle_temp;
			int nozzle_min_temp;
			int nozzle_max_temp;
			int weight;//unit ,g
			float width;//unit,mm
			wxString brand;//Manufacturer information.value may be in [anker,other]
			std::vector<FilamentColor> color_ids;//List of supported colors
			std::map<wxString, wxString> names;//int GUI_App::AnkerLanguageType
			//Name for multi-language.
			wxString Name();
		};

	}
}

#endif

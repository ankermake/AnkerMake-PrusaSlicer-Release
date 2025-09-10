#ifndef slic3r_GUI_FilamentMaterialHelper_hpp_
#define slic3r_GUI_FilamentMaterialHelper_hpp_
#include "FilamentMaterial.hpp"

namespace Slic3r {
	namespace GUI {

		class FilamentMaterialHelper
		{
		public:
			//try parse data from json
			static bool Parse(wxString json, std::vector<FilamentMaterial>& materials, std::vector<FilamentColor>& colors, long long& time, int& code, wxString& msg);

			//try unParse data to json
			//void UnParse(const std::vector<FilamentMaterial>& materials, const std::vector<FilamentColor>& colors, const int& time, wxString& json );

		};
	}
}

#endif

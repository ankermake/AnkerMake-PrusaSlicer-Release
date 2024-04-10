#ifndef slic3r_GUI_FilamentMaterialConvertor_hpp_
#define slic3r_GUI_FilamentMaterialConvertor_hpp_
#include "FilamentMaterial.hpp"

namespace Slic3r {
	namespace GUI {

		class FilamentMaterialConvertor 
		{
		public:
			//try convert color id to color value
			//if prefix of color id is 0x,it will be removed.
			//return false when color id can not be found
			static bool TryConvertColorId(wxString colorId, wxString& colorValue,bool autoRemove0x = true);

			//try convert color id [16 Hexadecimal as 10001003 or 0x10001003]  to color value in format wxC2S_HTML_SYNTAX
			//if prefix of color id is 0x,it will be removed.
			//return empty string when color id can not be found
			static wxString ConvertColorId(wxString colorId);


			//try convert filament id [16 Hexadecimal as 10001003 or 0x10001003]  to filament name
			//if prefix of filament id  is 0x,it will be removed.
			//return empty string  when filament id can not be found
			//return category of filament like PLA or TPU
			static wxString ConvertFilamentIdToCategory(wxString filamentId);

			//try convert color id [10 Decimal as 268500995]  to color value in format wxC2S_HTML_SYNTAX
			//if prefix of color id is 0x,it will be removed.
			//return empty string when color id can not be found
			static wxString ConvertColorId(int colorId);

			//try convert filament id [10 Decimal as 268500995 ]  to filament name
			//if prefix of filament id  is 0x,it will be removed.
			//return empty string  when filament id can not be found
			//return category of filament like PLA or TPU
			static wxString ConvertFilamentIdToCategory(int filamentId);


			//reset cache of queried content
			//in order to improve query performance, we have established an internal cache of queried content.
			static void ResetCache();

		private:
			//tab.wang:In our current needs,because there is only a usage scenario of converting ID to Category name,we move following interface to private.

			//try convert filament id [16 Hexadecimal as 10001003 or 0x10001003]  to filament name
			//if prefix of filament id  is 0x,it will be removed.
			//return empty string  when filament id can not be found
			//return total name of filament like PLA+ Basic or PLA+ Matte
			static wxString ConvertFilamentId(wxString filamentId);

			//try convert filament id [10 Decimal as 268500995 ]  to filament name
			//if prefix of filament id  is 0x,it will be removed.
			//return empty string  when filament id can not be found
			//return total name of filament like PLA+ Basic or PLA+ Matte
			static wxString ConvertFilamentId(int filamentId);

			//try convert filament id to filament name
			//if prefix of filament id  is 0x,it will be removed.
			//return false when color id can not be found
			//return total name of filament like PLA+ Basic or PLA+ Matte
			static bool TryConvertFilamentId(wxString filamentId, wxString& filamentName, bool autoRemove0x = true, bool autoRemoveAnkerMake = true);
		};

	}
}

#endif

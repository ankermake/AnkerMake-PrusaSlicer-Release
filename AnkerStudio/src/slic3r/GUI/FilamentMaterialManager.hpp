#ifndef slic3r_GUI_FilamentMaterialManager_hpp_
#define slic3r_GUI_FilamentMaterialManager_hpp_
#include "FilamentMaterial.hpp"

namespace Slic3r {
	namespace GUI {

		wxDECLARE_EVENT(wxCUSTOMEVT_ANKER_FILAMENT_MATERIAL_INFO_UPDATE, wxCommandEvent);

		class FilamentMaterialManager:wxEvtHandler
		{
		public:
			//Load Filament Material Info from local file
 			bool Load();

			//Get Filament Material Info.
			inline  std::vector<FilamentMaterial> GetFilamentMaterialInfo() { return m_materials; }

			//Get Filament Material Info.
			inline std::vector<FilamentColor> GetFilamentColorInfo() { return m_colors; }

			void ResetData(wxString json);
		private:
			std::vector<FilamentMaterial> m_materials;
			std::vector<FilamentColor> m_colors;
			long long m_time = 0;
			bool init = false;
		};

	}
}

#endif

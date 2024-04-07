#include "FilamentMaterialManager.hpp"
#include "FilamentMaterialHelper.hpp"
#include "libslic3r/Utils.hpp"

#define FILAMENT_MATERIAL_FILE "FilamentMaterial.json"
#define FILAMENT_MATERIAL_FILE_BACKUP "FilamentMaterial.json.bak"

namespace Slic3r {
	namespace GUI {
		

		bool FilamentMaterialManager::Load()
		{
			auto std_path = (boost::filesystem::path(Slic3r::data_dir()) / FILAMENT_MATERIAL_FILE).make_preferred().string();
			wxString path = wxString::FromUTF8(std_path);
			if (wxFile::Exists(path) == false)
			{
				ANKER_LOG_ERROR << "Can't find " << FILAMENT_MATERIAL_FILE;
				return false;
			}

			wxFile file;
			if (file.Open(path) == false)
			{
				ANKER_LOG_ERROR << "Can't open file for " << FILAMENT_MATERIAL_FILE;
				return false;
			}

			wxString buffer;
			bool readRet =  file.ReadAll(&buffer);
			file.Close();
			if (readRet == false)
				return false;

			int code;
			wxString msg;
			bool ret = FilamentMaterialHelper::Parse(buffer, m_materials, m_colors, m_time, code, msg);
			if (ret == false)
				return false;

			return true;
		}


		void FilamentMaterialManager::ResetData(wxString json)
		{
			std::vector<FilamentMaterial> materials;
			std::vector<FilamentColor> colors;
			long long time = 0;
			int code;
			wxString msg;
			bool ret = FilamentMaterialHelper::Parse(json, materials, colors, time, code, msg);
			if (ret == false)
				return;
			if (time == m_time)
				return;
			if (init)
				return;
			init = true;

			std::string path = (boost::filesystem::path(Slic3r::data_dir()) / FILAMENT_MATERIAL_FILE).make_preferred().string();
			std::string pathBackup = (boost::filesystem::path(Slic3r::data_dir()) / FILAMENT_MATERIAL_FILE_BACKUP).make_preferred().string();
			if (wxFile::Exists(wxString::FromUTF8(path)))
			{
				std::string err;
				auto copyRet = copy_file(path, pathBackup,err);
				if(copyRet != SUCCESS)
				{
					ANKER_LOG_ERROR << "Can't make back up for " << FILAMENT_MATERIAL_FILE << ".Error Code : "<<copyRet;
					return;
				}
			}
			else
			{
				ANKER_LOG_ERROR << "Can't find " << FILAMENT_MATERIAL_FILE << " for backup";
			}

			m_materials = materials;
			m_colors = colors;
			m_time = time;
			
			wxFile fileDst(wxString::FromUTF8(path), wxFile::write);
			if (fileDst.IsOpened() == false)
			{
				ANKER_LOG_ERROR << "Can't open file for " << FILAMENT_MATERIAL_FILE ;
				return;
			}
			if( fileDst.Write(json) == false)
			{
				ANKER_LOG_ERROR << "Can't write file for " << FILAMENT_MATERIAL_FILE;
			}
			fileDst.Close();
		}
	}
}
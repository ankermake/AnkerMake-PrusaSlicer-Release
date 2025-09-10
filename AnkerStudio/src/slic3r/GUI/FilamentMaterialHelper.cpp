#include "FilamentMaterialHelper.hpp"
#include "jansson.h"
#include "libslic3r/Utils.hpp"

namespace Slic3r {
	namespace GUI {

		static std::vector<wxString> MultiLanguageKey = { "de","fr","ja","en" };

		bool FilamentMaterialHelper::Parse(wxString json, std::vector<FilamentMaterial>& materials,
			std::vector<FilamentColor>& colors, long long & time,int & code,wxString& msg)
		{
			json_error_t error;
			json_t* root = json_loads(json.ToUTF8(), 0, nullptr);
			if (root == nullptr ) {
				ANKER_LOG_ERROR<< std::string("Error parsing JSON: ") << error.text;
				return false;
			}

			//top level
			auto jsonCode = json_object_get(root, "code");
			auto jsonMsg = json_object_get(root, "msg");
			auto jsonData = json_object_get(root, "data");
			if(jsonCode == nullptr || jsonMsg == nullptr || jsonData == nullptr)
				return false;
			code = json_integer_value(jsonCode);
			msg = json_string_value(jsonMsg);

			//data level
			auto jsonTime = json_object_get(jsonData, "last_modify_sec");
			auto jsonColors = json_object_get(jsonData, "colors");
			auto jsonMaterials = json_object_get(jsonData, "materials");
			auto jsonMultiLanguages = json_object_get(jsonData, "multi-language");
			if (jsonTime == nullptr)
				return false;
			time = json_integer_value(jsonTime);

			//parse color list
			colors.clear();
			for (int i = 0; jsonColors != nullptr && i < static_cast<int>(json_array_size(jsonColors)); ++i)
			{
				auto jsonColor = json_array_get(jsonColors, i);
				if (jsonColor == nullptr)
					break;
				auto jsonColorUid = json_object_get(jsonColor, "uid");
				auto jsonColorColor = json_object_get(jsonColor, "color");
				auto jsonColorValue = json_object_get(jsonColor, "value");
				if(jsonColorUid == nullptr || jsonColorColor == nullptr || jsonColorValue == nullptr)
					continue;
				auto color = FilamentColor();
				color.uid = json_string_value(jsonColorUid);
				color.color = json_string_value(jsonColorColor);
				color.value = static_cast<int> (json_integer_value(jsonColorValue));
				colors.push_back(color);
			}

			//parse material list
			materials.clear();
			for (int i = 0; jsonMaterials != nullptr && i < static_cast<int>(json_array_size(jsonMaterials)); ++i)
			{
				auto jsonMaterial = json_array_get(jsonMaterials, i);
				if (jsonMaterial == nullptr)
					break;
				auto jsonMaterialUid = json_object_get(jsonMaterial, "uid");
				auto jsonMaterialName = json_object_get(jsonMaterial, "name");
				auto jsonMaterialPlateTemp = json_object_get(jsonMaterial, "plate_temp");
				auto jsonMaterialNozzleTemp = json_object_get(jsonMaterial, "nozzle_temp");
				auto jsonMaterialNozzleMinTemp = json_object_get(jsonMaterial, "nozzle_min_temp");
				auto jsonMaterialNozzleMaxTemp = json_object_get(jsonMaterial, "nozzle_max_temp");
				auto jsonMaterialWeight = json_object_get(jsonMaterial, "weight");
				auto jsonMaterialWidth = json_object_get(jsonMaterial, "width");
				auto jsonMaterialBrand = json_object_get(jsonMaterial, "brand");
				if (jsonMaterialUid == nullptr || 
					jsonMaterialName == nullptr || 
					jsonMaterialPlateTemp == nullptr ||
					jsonMaterialNozzleTemp == nullptr ||
					jsonMaterialNozzleMinTemp == nullptr ||
					jsonMaterialNozzleMaxTemp == nullptr ||
					jsonMaterialWeight == nullptr ||
					jsonMaterialWidth == nullptr ||
					jsonMaterialBrand == nullptr  )
					continue;

				auto material = FilamentMaterial();
				material.uid = json_string_value(jsonMaterialUid);
				material.name = json_string_value(jsonMaterialName);
				material.plate_temp = static_cast<int> (json_integer_value(jsonMaterialPlateTemp));
				material.nozzle_temp = static_cast<int> (json_integer_value(jsonMaterialNozzleTemp));
				material.nozzle_min_temp = static_cast<int> (json_integer_value(jsonMaterialNozzleMinTemp));
				material.nozzle_max_temp = static_cast<int> (json_integer_value(jsonMaterialNozzleMaxTemp));
				material.weight = static_cast<int> (json_integer_value(jsonMaterialWeight));
				material.width = json_real_value(jsonMaterialWidth);
				material.brand = json_string_value(jsonMaterialBrand);

				auto jsonMaterialColorIds = json_object_get(jsonMaterial, "color_ids");
				for (int j = 0; jsonMaterialColorIds != nullptr && j < static_cast<int>(json_array_size(jsonMaterialColorIds)); ++j)
				{
					auto jsonMaterialColorId = json_array_get(jsonMaterialColorIds, j);
					if (jsonMaterialColorId == nullptr)
						break;
					auto color = FilamentColor();
					color.uid = json_string_value(jsonMaterialColorId);
					material.color_ids.push_back(color);
				}
				materials.push_back(material);
			}

			//parse MultiLanguage list of color
			auto jsonLangColors = json_object_get(jsonMultiLanguages, "colors");
			for (auto& color : colors)
			{
				wxString uid = color.uid;
				for (auto lang : MultiLanguageKey)
				{
					wxString key = uid + "-color-" + lang;

					auto jsonLang = json_object_get(jsonLangColors, key.c_str());
					if (jsonLang == nullptr)
						continue;

					wxString langColor = json_string_value(jsonLang);
					color.names.insert(std::pair(lang,langColor));
				}
			}

			//parse MultiLanguage list of material
			auto jsonLangMaterials = json_object_get(jsonMultiLanguages, "materials");
			for (auto& material : materials)
			{
				wxString uid = material.uid;
				for (auto lang : MultiLanguageKey)
				{
					wxString key = uid + "-name-" + lang;

					auto jsonLang = json_object_get(jsonLangMaterials, key.c_str());
					if (jsonLang == nullptr)
						continue;

					wxString langMaterial = json_string_value(jsonLang);
					material.names.insert(std::pair(lang, langMaterial));
				}
			}

			//update material list
			for (auto& material : materials)
			{
				for (auto& mc : material.color_ids)
				{
					for (auto c : colors)
					{
						if (mc.uid == c.uid)
						{
							mc = c;
							break;
						}
					}
				}
			}

			return true;
		}

		//void FilamentMaterialHelper::UnParse(const std::vector<FilamentMaterial>& materials,
		//	const std::vector<FilamentColor>& colors, const int& time, wxString& jsonStr)
		//{
		//	json_t* jsonLangMaterials = json_object();
		//	for (auto material : materials)
		//	{
		//		wxString uid = material.uid;
		//		for (auto lang : material.names)
		//		{
		//			wxString key = uid + "-name-" + lang.first;
		//			json_object_set_new(jsonLangMaterials, key.c_str(), json_string(lang.second.c_str()));
		//		}
		//	}

		//	json_t* jsonLangColors = json_object();
		//	for (auto color : colors)
		//	{
		//		wxString uid = color.uid;
		//		for (auto lang : color.names)
		//		{
		//			wxString key = uid + "-color-" + lang.first;
		//			json_object_set_new(jsonLangColors, key.c_str(), json_string(lang.second.c_str()));
		//		}
		//	}

		//	json_t* jsonMaterials = json_object();
		//	for (auto material : materials)
		//	{
		//		auto jsonMaterialColorIds = json_object();
		//		for (auto color : material.color_ids)
		//		{
		//			json_array_append(jsonMaterialColorIds, json_string(color.uid.c_str()));
		//		}

		//		auto jsonMaterial = json_object();
		//		json_object_set_new(jsonMaterial, "uid",json_string(material.uid.c_str()) );
		//		json_object_set_new(jsonMaterial, "name", json_string(material.name.c_str()));
		//		json_object_set_new(jsonMaterial, "plate_temp", json_integer(material.plate_temp));
		//		json_object_set_new(jsonMaterial, "nozzle_temp", json_integer(material.nozzle_temp));
		//		json_object_set_new(jsonMaterial, "nozzle_min_temp", json_integer(material.nozzle_min_temp));
		//		json_object_set_new(jsonMaterial, "nozzle_max_temp", json_integer(material.nozzle_max_temp));
		//		json_object_set_new(jsonMaterial, "weight", json_integer(material.weight));
		//		json_object_set_new(jsonMaterial, "width", json_real(material.width));
		//		json_object_set_new(jsonMaterial, "brand", json_string(material.brand.c_str()));
		//		json_object_set_new(jsonMaterial, "color_ids", jsonMaterialColorIds);

		//		json_array_append(jsonMaterials, jsonMaterial);
		//	}

		//	json_t* jsonColors = json_object();
		//	for (auto color : colors)
		//	{
		//		auto jsonColor = json_object();
		//		json_object_set_new(jsonColor, "uid", json_string(color.uid.c_str()));
		//		json_object_set_new(jsonColor, "color", json_string(color.color.c_str()));
		//		json_object_set_new(jsonColor, "value", json_integer(color.value));

		//		json_array_append(jsonColors, jsonColor);
		//	}

		//	//data level
		//	auto jsonData = json_object();
		//	json_object_set_new(jsonData, "last_modify_sec",json_integer(time));
		//	json_object_set_new(jsonData, "colors",jsonColors);
		//	json_object_set_new(jsonData, "materials",jsonMaterials);
		//	json_object_set_new(jsonData, "multi-language",jsonLangMaterials);

		//	json_t* json = json_object();
		//	json_object_set_new(json, "code", json_integer(0));
		//	json_object_set_new(json, "msg", json_string(""));
		//	json_object_set_new(json, "data", jsonData);

		//	jsonStr = json_dumps(json, 0);
		//}
	}
}



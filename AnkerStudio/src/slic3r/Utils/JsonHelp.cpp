#include "JsonHelp.hpp"
#include "libslic3r/Utils.hpp"

namespace Slic3r {
	bool JsonHelp::GetDataB(json_t* jsonObj, const char* key, bool defaultValue)
	{
		json_t* object = json_object_get(jsonObj, key);
		if (object) {
			return json_boolean_value(object);
		}
		ANKER_LOG_DEBUG << "get " << key << " failed, use default value: " << defaultValue;
		return defaultValue;
	}

	int JsonHelp::GetDataI(json_t* jsonObj, const char* key, int defaultValue)
	{
		json_t* object = json_object_get(jsonObj, key);
		if (object) {
			return (int)json_integer_value(object);
		}
		ANKER_LOG_DEBUG << "get " << key << " failed, use default value: " << defaultValue;
		return defaultValue;
	}

	const char* JsonHelp::GetDataS(json_t* jsonObj, const char* key, std::string defaultValue)
	{
		json_t* object = json_object_get(jsonObj, key);
		if (object) {
			return json_string_value(object);
		}
		ANKER_LOG_DEBUG << "get " << key << " failed, use default value: " << defaultValue;
		return "";
	}
}
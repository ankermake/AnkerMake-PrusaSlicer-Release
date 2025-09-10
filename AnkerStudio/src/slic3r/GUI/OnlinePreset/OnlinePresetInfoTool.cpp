#include "OnlinePresetInfoTool.hpp"
#include "slic3r/Utils/wxFileTool.hpp"
#include "libslic3r/Utils.hpp"
#include <jansson.h>

namespace Slic3r {
namespace GUI {


	bool OnlinePresetInfoTool::Read(wxString filename,OnlinePresetInfo& info)
	{
		wxString content;
		bool ret = Utils::wxFileTool::ReadFileContent(filename, content);
		if(ret == false)
		{
			return false;
		}
		ret = Parse(content, info);
		return ret;
	}

	bool OnlinePresetInfoTool::Write(wxString filename, const OnlinePresetInfo& info)
	{
		wxString content = Compose(info);
		bool ret = Utils::wxFileTool::WriteFileContent(filename, content);
		return ret;
	}

	bool OnlinePresetInfoTool::Parse(wxString json, OnlinePresetInfo& info)
	{
		json_error_t error;
		json_t* root = json_loads(json.ToUTF8(), 0, nullptr);
		if (root == nullptr) {
			ANKER_LOG_ERROR << std::string("Error parsing JSON: ") << error.text;
			return false;
		}

		auto curVersion = json_object_get(root, "curVersion");
		auto newVersion = json_object_get(root, "newVersion");
		auto newMD5 = json_object_get(root, "newMD5");
		auto newTips = json_object_get(root, "newTips");
		if (curVersion == nullptr || newVersion == nullptr || newMD5 == nullptr || newTips == nullptr)
			return false;

		info.curVersion = json_string_value(curVersion);
		info.newVersion = json_string_value(newVersion);
		info.newMD5 = json_string_value(newMD5);
		info.newTips = json_string_value(newTips);
		return true;
	}

	wxString OnlinePresetInfoTool::Compose(const OnlinePresetInfo& info)
	{
		json_t* root = json_object();
		json_object_set_new(root, "curVersion", json_string(info.curVersion.c_str()));
		json_object_set_new(root, "newVersion", json_string(info.newVersion.c_str()));
		json_object_set_new(root, "newMD5", json_string(info.newMD5.c_str()));
		json_object_set_new(root, "newTips", json_string(info.newTips.c_str()));
		wxString json = wxString::FromUTF8(json_dumps(root, 0)) ;
		return json;
	}
} // GUI
} // Slic3r

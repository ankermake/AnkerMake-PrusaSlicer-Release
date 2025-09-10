#ifndef slic3r_OnlinePresetInfo_hpp_
#define slic3r_OnlinePresetInfo_hpp_
#include <string>
namespace Slic3r {
namespace GUI {
	struct OnlinePresetInfo
	{
		std::string curVersion = "0";
		//std::string curMD5;
		//std::string curTips;
		std::string newVersion = "0";
		std::string newMD5 = "";
		std::string newTips = "";
	};

} // GUI
} // Slic3r

#endif /* slic3r_2DBed_hpp_ */

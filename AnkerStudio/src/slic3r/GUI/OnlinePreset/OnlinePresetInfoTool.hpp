#ifndef slic3r_OnlinePresetInfoTool_hpp_
#define slic3r_OnlinePresetInfoTool_hpp_
#include "OnlinePresetInfo.hpp"


namespace Slic3r {
namespace GUI {
	class OnlinePresetInfoTool
	{
	public:
		static bool Read(wxString filename, OnlinePresetInfo& info);
		static bool Write(wxString filename, const OnlinePresetInfo& info);

		static bool Parse(wxString json, OnlinePresetInfo& info);
		static wxString Compose(const OnlinePresetInfo& info);
	};

} // GUI
} // Slic3r

#endif /* slic3r_2DBed_hpp_ */

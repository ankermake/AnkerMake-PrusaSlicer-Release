#ifndef slic3r_VersionTool_hpp_
#define slic3r_VersionTool_hpp_
#include "Version.hpp"

namespace Slic3r {
namespace GUI {
	class VersionTool
	{
	public:
		//return 1 if a is over b
		//return 0 if a is same as b
		//return -1 if a is below  b
		static int Compare(VersionT a, VersionT b);

		//return 1 if a is over b
		//return 0 if a is same as b
		//return -1 if a is below  b
		static int Compare(wxString a, wxString b);

		//version string is like 1.5.0
		static VersionT TransformF(wxString version);
	};

} // GUI
} // Slic3r

#endif /* slic3r_2DBed_hpp_ */

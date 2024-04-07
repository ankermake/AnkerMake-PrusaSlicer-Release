#include "VersionTool.hpp"

namespace Slic3r {
namespace GUI {
	int VersionTool::Compare(VersionT a, VersionT b)
	{
		if (a.Major != b.Major)
			return a.Major > b.Major ? 1 : -1;
		if (a.Minor != b.Minor)
			return a.Minor > b.Minor ? 1 : -1;
		if (a.Revision != b.Revision)
			return a.Revision > b.Revision ? 1 : -1;
		return 0;
	}

	int VersionTool::Compare(wxString a, wxString b)
	{
		return Compare(TransformF(a), TransformF(b));
	}

	VersionT VersionTool::TransformF(wxString version)
	{
		VersionT v;

		int pos = version.Find('.');
		if (pos <= 0)
			return v;
		wxString major = version.substr(0, pos);
		bool ret = major.ToInt(&v.Major);
		if (ret == false)
			return v;
		if(pos >= (version.size()-1))
			return v;

		version = version.substr(pos + 1);
		pos = version.Find('.');
		if (pos <= 0)
			return v;
		wxString minor = version.substr(0, pos);
		ret = minor.ToInt(&v.Minor);
		if (ret == false)
			return v;
		if (pos >= (version.size() - 1))
			return v;

		wxString revision = version.substr(pos + 1);
		ret = revision.ToInt(&v.Revision);
		if (ret == false)
			return v;

		return v;
	}
} // GUI
} // Slic3r

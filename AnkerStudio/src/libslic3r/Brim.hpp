#ifndef slic3r_Brim_hpp_
#define slic3r_Brim_hpp_

namespace Slic3r {

class Print;

// Produce brim lines around those objects, that have the brim enabled.
// Collect islands_area to be merged into the final 1st layer convex hull.
ExtrusionEntityCollection make_brim(const Print &print, PrintTryCancel try_cancel, Polygons &islands_area);
void make_brim(const Print& print, PrintTryCancel try_cancel, Polygons& islands_area,
	std::map<ObjectID, ExtrusionEntityCollection>& brimMap,
	std::map<ObjectID, ExtrusionEntityCollection>& supportBrimMap,
	std::vector<std::pair<ObjectID, unsigned int> >& objPrintVec,
	std::vector<unsigned int>& printExtruders);

} // Slic3r

#endif // slic3r_Brim_hpp_

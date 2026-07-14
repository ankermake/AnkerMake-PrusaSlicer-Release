#include "../ClipperUtils.hpp"
#include "../ShortestPath.hpp"
#include "../Arachne/WallToolPaths.hpp"

#include "FillEnsuring.hpp"

#include <boost/log/trivial.hpp>

namespace Slic3r {

template<typename T> inline void reorder_by_shortest_traverse(std::vector<T>& polylines_out)
{
    Points start_point;
    start_point.reserve(polylines_out.size());
    for (const T contour : polylines_out) start_point.push_back(contour.points.front());

    std::vector<Points::size_type> order = chain_points(start_point);

    std::vector<T> Temp = polylines_out;
    polylines_out.erase(polylines_out.begin(), polylines_out.end());

    for (size_t i : order) polylines_out.emplace_back(std::move(Temp[i]));
}

ThickPolylines FillEnsuring::fill_surface_arachne(const Surface *surface, const FillParams &params)
{
    assert(params.use_arachne);
    assert(this->print_config != nullptr && this->print_object_config != nullptr && this->print_region_config != nullptr);

    const coord_t               scaled_spacing         = params.flow.scaled_spacing();

    // Perform offset.
    Slic3r::ExPolygons expp = this->overlap != 0. ? offset_ex(surface->expolygon, scaled<float>(this->overlap)) : ExPolygons{surface->expolygon};
    // Create the infills for each of the regions.
    ThickPolylines thick_polylines_out;
    for (ExPolygon &ex_poly : this->no_overlap_expolygons) {
        Point    bbox_size   = ex_poly.contour.bounding_box().size();
        coord_t  loops_count = std::max(bbox_size.x(), bbox_size.y()) / scaled_spacing + 1;
        Polygons polygons    = to_polygons(ex_poly);

        double min_nozzle_diameter = *std::min_element(print_config->nozzle_diameter.values.begin(), print_config->nozzle_diameter.values.end());
        PrintObjectConfig print_object_config           = *this->print_object_config;
        print_object_config.min_bead_width.percent      = false;
        print_object_config.min_bead_width.value        = 0.85 * min_nozzle_diameter;
        print_object_config.min_feature_size.percent    = false;
        print_object_config.min_feature_size.value      = 0.25 * min_nozzle_diameter;
        print_object_config.wall_transition_length.percent = false;
        print_object_config.wall_transition_length.value    = 0.4;
        print_object_config.wall_transition_angle.value     = 10;
        print_object_config.wall_transition_filter_deviation.percent = false;
        print_object_config.wall_transition_filter_deviation.value = 0.25 * min_nozzle_diameter;
        print_object_config.wall_distribution_count.value = 1;

        Arachne::WallToolPaths wall_tool_paths(polygons, scaled_spacing, scaled_spacing, loops_count, 0, params.layer_height, print_object_config, *this->print_config);
        if (std::vector<Arachne::VariableWidthLines> loops = wall_tool_paths.getToolPaths(); !loops.empty()) {
            std::vector<const Arachne::ExtrusionLine *> all_extrusions;
            for (Arachne::VariableWidthLines &loop : loops) {
                if (loop.empty())
                    continue;
                for (const Arachne::ExtrusionLine &wall : loop)
                    all_extrusions.emplace_back(&wall);
            }

            // Split paths using a nearest neighbor search.
            size_t firts_poly_idx = thick_polylines_out.size();
            Point  last_pos(0, 0);
            for (const Arachne::ExtrusionLine *extrusion : all_extrusions) {
                if (extrusion->empty())
                    continue;

                ThickPolyline thick_polyline = Arachne::to_thick_polyline(*extrusion);
                if (thick_polyline.length() == 0.)
                    //FIXME this should not happen.
                    continue;
                assert(thick_polyline.size() > 1);
                assert(thick_polyline.length() > 0.);
                //assert(thick_polyline.points.size() == thick_polyline.width.size());
                if (extrusion->is_closed)
                    thick_polyline.start_at_index(nearest_point_index(thick_polyline.points, last_pos));

                assert(thick_polyline.size() > 1);
                //assert(thick_polyline.points.size() == thick_polyline.width.size());
                thick_polylines_out.emplace_back(std::move(thick_polyline));
                //last_pos = thick_polylines_out.back().last_point();
            }

            // clip the paths to prevent the extruder from getting exactly on the first point of the loop
            // Keep valid paths only.
            size_t j = firts_poly_idx;
            for (size_t i = firts_poly_idx; i < thick_polylines_out.size(); ++i) {
                assert(thick_polylines_out[i].size() > 1);
                assert(thick_polylines_out[i].length() > 0.);
                //assert(thick_polylines_out[i].points.size() == thick_polylines_out[i].width.size());
                thick_polylines_out[i].clip_end(this->loop_clipping);
                //update by alves, cover right parameters data to the config if fff_print then process
                //assert(thick_polylines_out[i].size() > 1);
                if (thick_polylines_out[i].is_valid()) {
                    if (j < i)
                        thick_polylines_out[j] = std::move(thick_polylines_out[i]);
                    ++j;
                }
            }
            if (j < thick_polylines_out.size())
                thick_polylines_out.erase(thick_polylines_out.begin() + int(j), thick_polylines_out.end());

            reorder_by_shortest_traverse(thick_polylines_out);
        }
    }

    return thick_polylines_out;
}

} // namespace Slic3r

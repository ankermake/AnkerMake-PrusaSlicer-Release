#ifndef slic3r_ExtrusionProcessor_hpp_
#define slic3r_ExtrusionProcessor_hpp_

#include "../AABBTreeLines.hpp"
#include "../SupportSpotsGenerator.hpp"
#include "../libslic3r.h"
#include "../ExtrusionEntity.hpp"
#include "../Layer.hpp"
#include "../Point.hpp"
#include "../SVG.hpp"
#include "../BoundingBox.hpp"
#include "../Polygon.hpp"
#include "../ClipperUtils.hpp"
#include "../Flow.hpp"
#include "../Config.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Slic3r {

class SlidingWindowCurvatureAccumulator
{
    float        window_size;
    float        total_distance  = 0; // accumulated distance
    float        total_curvature = 0; // accumulated signed ccw angles
    deque<float> distances;
    deque<float> angles;

public:
    SlidingWindowCurvatureAccumulator(float window_size) : window_size(window_size) {}

    void add_point(float distance, float angle)
    {
        total_distance += distance;
        total_curvature += angle;
        distances.push_back(distance);
        angles.push_back(angle);

        while (distances.size() > 1 && total_distance > window_size) {
            total_distance -= distances.front();
            total_curvature -= angles.front();
            distances.pop_front();
            angles.pop_front();
        }
    }

    float get_curvature() const
    {
        return total_curvature / window_size;
    }

    void reset()
    {
        total_curvature = 0;
        total_distance  = 0;
        distances.clear();
        angles.clear();
    }
};

class CurvatureEstimator
{
    static const size_t               sliders_count          = 3;
    SlidingWindowCurvatureAccumulator sliders[sliders_count] = {{1.0},{4.0}, {10.0}};

public:
    void add_point(float distance, float angle)
    {
        if (distance < EPSILON)
            return;
        for (SlidingWindowCurvatureAccumulator &slider : sliders) {
            slider.add_point(distance, angle);
        }
    }
    float get_curvature()
    {
        float max_curvature = 0.0f;
        for (const SlidingWindowCurvatureAccumulator &slider : sliders) {
            if (abs(slider.get_curvature()) > abs(max_curvature)) {
                max_curvature = slider.get_curvature();
            }
        }
        return max_curvature;
    }
    void reset()
    {
        for (SlidingWindowCurvatureAccumulator &slider : sliders) {
            slider.reset();
        }
    }
};

struct ExtendedPoint
{
    ExtendedPoint(Vec2d position, float distance = 0.0, size_t nearest_prev_layer_line = size_t(-1), float curvature = 0.0)
        : position(position), distance(distance), nearest_prev_layer_line(nearest_prev_layer_line), curvature(curvature)
    {}

    Vec2d  position;
    float  distance;
    size_t nearest_prev_layer_line;
    float  curvature;
};

template<bool SCALED_INPUT, bool ADD_INTERSECTIONS, bool PREV_LAYER_BOUNDARY_OFFSET, bool SIGNED_DISTANCE, typename P, typename L>
std::vector<ExtendedPoint> estimate_points_properties(const std::vector<P>                   &input_points,
                                                      const AABBTreeLines::LinesDistancer<L> &unscaled_prev_layer,
                                                      float                                   flow_width,
                                                      float                                   max_line_length = -1.0f)
{
    using AABBScalar = typename AABBTreeLines::LinesDistancer<L>::Scalar;
    if (input_points.empty())
        return {};
    float              boundary_offset = PREV_LAYER_BOUNDARY_OFFSET ? 0.5 * flow_width : 0.0f;
    CurvatureEstimator cestim;
    auto maybe_unscale = [](const P &p) { return SCALED_INPUT ? unscaled(p) : p.template cast<double>(); };

    std::vector<ExtendedPoint> points;
    points.reserve(input_points.size() * (ADD_INTERSECTIONS ? 1.5 : 1));

    {
        ExtendedPoint start_point{maybe_unscale(input_points.front())};
        auto [distance, nearest_line, x]    = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(start_point.position.cast<AABBScalar>());
        start_point.distance                = distance + boundary_offset;
        start_point.nearest_prev_layer_line = nearest_line;
        points.push_back(start_point);
    }
    for (size_t i = 1; i < input_points.size(); i++) {
        ExtendedPoint next_point{maybe_unscale(input_points[i])};
        auto [distance, nearest_line, x]   = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(next_point.position.cast<AABBScalar>());
        next_point.distance                = distance + boundary_offset;
        next_point.nearest_prev_layer_line = nearest_line;

        if (ADD_INTERSECTIONS &&
            ((points.back().distance > boundary_offset + EPSILON) != (next_point.distance > boundary_offset + EPSILON))) {
            const ExtendedPoint &prev_point = points.back();
            auto intersections = unscaled_prev_layer.template intersections_with_line<true>(L{prev_point.position.cast<AABBScalar>(), next_point.position.cast<AABBScalar>()});
            const auto& prev_position = prev_point.position;
            const auto& next_position = next_point.position;
            for (const auto &intersection : intersections) {
                //Galen.Xiao, remove small segment 2023/10/17 begin
                auto curr_position = intersection.first.template cast<double>();
                if ((curr_position - prev_position).norm() < 1 || (curr_position - next_position).norm() < 1) {
                    continue;
                }
                //Galen.Xiao, remove small segment 2023/10/17 end

                points.emplace_back(intersection.first.template cast<double>(), boundary_offset, intersection.second);
            }
        }
        points.push_back(next_point);
    }

    if (PREV_LAYER_BOUNDARY_OFFSET && ADD_INTERSECTIONS) {
        std::vector<ExtendedPoint> new_points;
        new_points.reserve(points.size()*2);
        new_points.push_back(points.front());
        for (int point_idx = 0; point_idx < int(points.size()) - 1; ++point_idx) {
            const ExtendedPoint &curr = points[point_idx];
            const ExtendedPoint &next = points[point_idx + 1];

            if ((curr.distance > EPSILON && curr.distance < boundary_offset + 2.0f) ||
                (next.distance > EPSILON && next.distance < boundary_offset + 2.0f)) {
                double line_len = (next.position - curr.position).norm();
                if (line_len > 4.0f) {
                    double a0 = std::clamp((curr.distance + 2 * boundary_offset) / line_len, 0.0, 1.0);
                    double a1 = std::clamp(1.0f - (next.distance + 2 * boundary_offset) / line_len, 0.0, 1.0);
                    double t0 = std::min(a0, a1);
                    double t1 = std::max(a0, a1);

                    if (t0 < 1.0) {
                        auto p0                         = curr.position + t0 * (next.position - curr.position);
                        auto [p0_dist, p0_near_l, p0_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(p0.cast<AABBScalar>());
                        new_points.push_back(ExtendedPoint{p0, float(p0_dist + boundary_offset), p0_near_l});
                    }
                    if (t1 > 0.0) {
                        auto p1                         = curr.position + t1 * (next.position - curr.position);
                        auto [p1_dist, p1_near_l, p1_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(p1.cast<AABBScalar>());
                        new_points.push_back(ExtendedPoint{p1, float(p1_dist + boundary_offset), p1_near_l});
                    }
                }
            }
            new_points.push_back(next);
        }
        points = new_points;
    }

    if (max_line_length > 0) {
        std::vector<ExtendedPoint> new_points;
        new_points.reserve(points.size()*2);
        {
            for (size_t i = 0; i + 1 < points.size(); i++) {
                const ExtendedPoint &curr = points[i];
                const ExtendedPoint &next = points[i + 1];
                new_points.push_back(curr);
                double len             = (next.position - curr.position).squaredNorm();
                double t               = sqrt((max_line_length * max_line_length) / len);
                size_t new_point_count = 1.0 / t;
                for (size_t j = 1; j < new_point_count + 1; j++) {
                    Vec2d pos  = curr.position * (1.0 - j * t) + next.position * (j * t);
                    auto [p_dist, p_near_l,
                          p_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(pos.cast<AABBScalar>());
                    new_points.push_back(ExtendedPoint{pos, float(p_dist + boundary_offset), p_near_l});
                }
            }
            new_points.push_back(points.back());
        }
        points = new_points;
    }

    for (int point_idx = 0; point_idx < int(points.size()); ++point_idx) {
        ExtendedPoint &a    = points[point_idx];
        ExtendedPoint &prev = points[point_idx > 0 ? point_idx - 1 : point_idx];

        int prev_point_idx = point_idx;
        while (prev_point_idx > 0) {
            prev_point_idx--;
            if ((a.position - points[prev_point_idx].position).squaredNorm() > EPSILON) { break; }
        }

        int next_point_index = point_idx;
        while (next_point_index < int(points.size()) - 1) {
            next_point_index++;
            if ((a.position - points[next_point_index].position).squaredNorm() > EPSILON) { break; }
        }

        if (prev_point_idx != point_idx && next_point_index != point_idx) {
            float distance = (prev.position - a.position).norm();
            float alfa     = angle(a.position - points[prev_point_idx].position, points[next_point_index].position - a.position);
            cestim.add_point(distance, alfa);
        }

        a.curvature = cestim.get_curvature();
    }

    return points;
}

struct ProcessedPoint
{
    Point p;
    float speed = 1.0f;
    int fan_speed = 0;
};

class ExtrusionQualityEstimator
{
    std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<Linef>> prev_layer_boundaries;
    std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<Linef>> next_layer_boundaries;
    std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<CurledLine>> prev_curled_extrusions;
    std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<CurledLine>> next_curled_extrusions;
    const PrintObject                                                            *current_object;

public:
    void set_current_object(const PrintObject *object) { current_object = object; }

    Point move_inside_to_pre_layer_boundary(Point point, double line_width)
    { 
        auto closestPoint = [](Point pt, Linef line) {
            double ax = line.a.x();
            double ay = line.a.y();
            double bx = line.b.x();
            double by = line.b.y();
            double a, b, c, t;
            a = ax - bx;
            b = ay - by;
            c = -a * ax - b * ay;
            t = -a * unscaled(pt.x()) - b * unscaled(pt.y()) - c;
            
            return Point(scaled(ax + a * t), scaled(ay + b * t));
        };

        const auto& prev_layer_boundary = prev_layer_boundaries[current_object];

        auto get_distance_and_line_idx = [&](Point pt) {
            Points path;
            path.push_back(pt);
            std::vector<ExtendedPoint> extended_points =
                estimate_points_properties<true, true, true, true>(path, prev_layer_boundary, line_width);
            return extended_points.back();
        };

        ExtendedPoint distance_data = get_distance_and_line_idx(point);
        if (distance_data.distance < EPSILON || distance_data.distance > line_width)
        {
            return point;
        }
        Linef nearest_line = prev_layer_boundary.get_line(distance_data.nearest_prev_layer_line);
        Point projection = closestPoint(point, nearest_line);
        Vec2d  p1 = point.cast<double>();
        Vec2d  p2 = projection.cast<double>();
        Vec2d  v = p2 - p1;
        double l2 = v.norm();
        Vec2d  inward_point = p1 + v * ((l2 + scaled(line_width)) / l2);
        Point inward_point_coord_t = inward_point.cast<coord_t>();
        ExtendedPoint distance_data_new = get_distance_and_line_idx(inward_point_coord_t);
        double move_dist = (inward_point - p1).norm();
        if (move_dist > 2 * scaled(line_width), distance_data_new.distance > EPSILON)
        {
            return point;
        }   
        return inward_point_coord_t;
    }

    bool check_point_is_overhang(Point point, double line_width)
    {
        const auto& prev_layer_boundary = prev_layer_boundaries[current_object];
        if (prev_layer_boundary.get_lines().empty())
        {
            return false;
        }

        auto get_distance_and_line_idx = [&](Point pt) {
            Points path;
            path.push_back(pt);
            std::vector<ExtendedPoint> extended_points =
                estimate_points_properties<true, true, true, true>(path, prev_layer_boundary, line_width);
            return extended_points.back();
        };

        ExtendedPoint distance_data = get_distance_and_line_idx(point);
        if (distance_data.distance > 0.25 * line_width)
        {
            return true;
        }
        return false;
    }

    void prepare_for_new_layer(const PrintObject* obj, const Layer* layer)
    {
        if (layer == nullptr) return;
        const PrintObject *object     = obj;
        prev_layer_boundaries[object] = next_layer_boundaries[object];
        next_layer_boundaries[object] = AABBTreeLines::LinesDistancer<Linef>{to_unscaled_linesf(layer->lslices)};
        CurledLines curled_lines;
        for (CurledLine cLine : layer->curled_lines) {
            CurledLine cLine_new;
            cLine_new.a = Point(cLine.a.x() / 1000, cLine.a.y() / 1000);
            cLine_new.b = Point(cLine.b.x() / 1000, cLine.b.y() / 1000);
            cLine_new.curled_height = cLine.curled_height;
            curled_lines.push_back(cLine_new);
        }
        prev_curled_extrusions[object] = next_curled_extrusions[object];
        next_curled_extrusions[object] = AABBTreeLines::LinesDistancer<CurledLine>{ curled_lines };
    }

    std::vector<ProcessedPoint> estimate_extrusion_quality(const ExtrusionPath                                          &path,
                                                           const std::vector<std::pair<int, ConfigOptionFloatOrPercent>> overhangs_w_speeds,
                                                           const std::vector<std::pair<int, ConfigOptionInts>> overhangs_w_fan_speeds,
                                                           size_t                                              extruder_id,
                                                           float                                               ext_perimeter_speed,
                                                           float                                               original_speed,
                                                           bool                                                slowdown_for_curled_edges)
    {
        float                  speed_base = ext_perimeter_speed > 0 ? ext_perimeter_speed : original_speed;
        std::map<float, float> speed_sections;
        for (size_t i = 0; i < overhangs_w_speeds.size(); i++) {
            float distance           = path.width * (1.0 - (overhangs_w_speeds[i].first / 100.0));
            float speed              = overhangs_w_speeds[i].second.percent ? (speed_base * overhangs_w_speeds[i].second.value / 100.0) :
                                                                              overhangs_w_speeds[i].second.value;
            speed_sections[distance] = speed;
        }

        std::map<float, float> fan_speed_sections;
        for (size_t i = 0; i < overhangs_w_fan_speeds.size(); i++) {
            float distance           = path.width * (1.0 - (overhangs_w_fan_speeds[i].first / 100.0));
            float fan_speed            = overhangs_w_fan_speeds[i].second.get_at(extruder_id);
            fan_speed_sections[distance] = fan_speed;
        }

        std::vector<ExtendedPoint> extended_points =
            estimate_points_properties<true, true, true, true>(path.polyline.points, prev_layer_boundaries[current_object], path.width);
        const auto width_inv = 1.0f / path.width;
        //Galen.Xiao, Overhang length is less than min short threshold that no variable speed of endpoint 2023/10/15 begin
        auto update_extended_point = [&](ExtendedPoint& cur_point, const ExtendedPoint& next_point) {
            const auto& prev_layer_boundary = prev_layer_boundaries[current_object];

            if (cur_point.nearest_prev_layer_line < 0
                || cur_point.nearest_prev_layer_line >= prev_layer_boundary.get_lines().size())
                return;
            auto& line = prev_layer_boundary.get_line(cur_point.nearest_prev_layer_line);

            auto is_need_min_segment_optimize = [](const Linef& line, Vec2d position) {
                return (line.a - line.b).norm() < 1.5 && std::min((position - line.a).norm(), (position - line.b).norm()) < 1;
            };
            if (is_need_min_segment_optimize(line, cur_point.position)) {
                auto position = (cur_point.position + next_point.position) / 2.0f;
                using AABBScalar = typename AABBTreeLines::LinesDistancer<Linef>::Scalar;
                auto [distance, nearest_line, x] = prev_layer_boundary.template distance_from_lines_extra<true>(position.cast<AABBScalar>());
                float other_distance = (cur_point.distance + next_point.distance) / 2;
                cur_point.distance = std::min((float)distance, other_distance);
            }
        };

        if (extended_points.size() > 1) {
            update_extended_point(extended_points.front(), *(++extended_points.begin()));
            update_extended_point(extended_points.back(), *(++extended_points.rbegin()));
        }
        //Galen.Xiao, Overhang length is less than min short threshold that no variable speed in the endpoint 2023/10/15 end

        std::vector<ProcessedPoint> processed_points;
        processed_points.reserve(extended_points.size());
        for (size_t i = 0; i < extended_points.size(); i++) {
            const ExtendedPoint &curr = extended_points[i];
            const ExtendedPoint &next = extended_points[i + 1 < extended_points.size() ? i + 1 : i];

            float artificial_distance_to_curled_lines = 0.0;
            if (slowdown_for_curled_edges ) {
                // The following code artifically increases the distance to provide slowdown for extrusions that are over curled lines
                const double dist_limit = 10.0 * path.width;
                {
                    Vec2d middle = 0.5 * (curr.position + next.position);
                    auto line_indices = prev_curled_extrusions[current_object].all_lines_in_radius(Point(middle.x() * 1000, middle.y() * 1000), dist_limit * 1000);
                    if (!line_indices.empty()) {
                        double len = (next.position - curr.position).norm();
                        // For long lines, there is a problem with the additional slowdown. If by accident, there is small curled line near the middle of this long line
                        //  The whole segment gets slower unnecesarily. For these long lines, we do additional check whether it is worth slowing down.
                        // NOTE that this is still quite rough approximation, e.g. we are still checking lines only near the middle point
                        // TODO maybe split the lines into smaller segments before running this alg? but can be demanding, and GCode will be huge
                        if (len > 8) {
                            Vec2d dir = Vec2d(next.position - curr.position) / len;
                            Vec2d right = Vec2d(-dir.y(), dir.x());

                            Polygon box_of_influence = {
                                scaled(Vec2d(curr.position + right * dist_limit)),
                                scaled(Vec2d(next.position + right * dist_limit)),
                                scaled(Vec2d(next.position - right * dist_limit)),
                                scaled(Vec2d(curr.position - right * dist_limit)),
                            };

                            double projected_lengths_sum = 0;
                            for (size_t idx : line_indices) {
                                const CurledLine& line = prev_curled_extrusions[current_object].get_line(idx);
                                CurledLine cLine_new;
                                cLine_new.a = Point(line.a.x() * 1000, line.a.y() * 1000);
                                cLine_new.b = Point(line.b.x() * 1000, line.b.y() * 1000);
                                cLine_new.curled_height = line.curled_height;
                                Lines             inside = intersection_ln({ {cLine_new.a, cLine_new.b} }, { box_of_influence });
                                if (inside.empty())
                                    continue;
                                double projected_length = abs(dir.dot(unscaled(Vec2d((inside.back().b - inside.back().a).cast<double>()))));
                                projected_lengths_sum += projected_length;
                            }
                            if (projected_lengths_sum < 0.4 * len) {
                                line_indices.clear();
                            }
                        }

                        for (size_t idx : line_indices) {
                            const CurledLine& line = prev_curled_extrusions[current_object].get_line(idx);
                            CurledLine cLine_new;
                            cLine_new.a = Point(line.a.x() * 1000, line.a.y() * 1000);
                            cLine_new.b = Point(line.b.x() * 1000, line.b.y() * 1000);
                            cLine_new.curled_height = line.curled_height;
                            float             distance_from_curled = unscaled(line_alg::distance_to(cLine_new, Point::new_scale(middle)));
                            float             dist = path.width * (1.0 - (distance_from_curled / dist_limit)) *
                                (1.0 - (distance_from_curled / dist_limit)) *
                                (cLine_new.curled_height / (path.height * 10.0f)); // max_curled_height_factor from SupportSpotGenerator
                            artificial_distance_to_curled_lines = std::max(artificial_distance_to_curled_lines, dist);
                        }
                    }
                }
            }

            auto interpolate_speed = [](const std::map<float, float> &values, float distance) {
                auto upper_dist = values.lower_bound(distance);
                if (upper_dist == values.end()) {
                    return values.rbegin()->second;
                }
                if (upper_dist == values.begin()) {
                    return upper_dist->second;
                }

                auto  lower_dist = std::prev(upper_dist);
                float t          = (distance - lower_dist->first) / (upper_dist->first - lower_dist->first);
                return (1.0f - t) * lower_dist->second + t * upper_dist->second;
            };

            float extrusion_speed = std::min(interpolate_speed(speed_sections, curr.distance),
                                             interpolate_speed(speed_sections, next.distance));
            float fan_speed       = std::min(interpolate_speed(fan_speed_sections, curr.distance),
                                             interpolate_speed(fan_speed_sections, next.distance));

            if (slowdown_for_curled_edges && !path.role().is_external_perimeter()) {
                float curled_speed = interpolate_speed(speed_sections, artificial_distance_to_curled_lines);
                extrusion_speed = std::min(curled_speed, extrusion_speed); // adjust extrusion speed based on what is smallest - the calculated overhang speed or the artificial curled speed
            }

            processed_points.push_back({scaled(curr.position), extrusion_speed, int(fan_speed)});
        }
        return processed_points;
    }
};

} // namespace Slic3r

#endif // slic3r_ExtrusionProcessor_hpp_

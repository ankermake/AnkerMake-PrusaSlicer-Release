#include "clipper/clipper_z.hpp"

#include "ClipperUtils.hpp"
#include "EdgeGrid.hpp"
#include "Layer.hpp"
#include "Print.hpp"
#include "ShortestPath.hpp"
#include "libslic3r.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <mutex>

#include <tbb/parallel_for.h>
#include <boost/thread/lock_guard.hpp>

#ifndef NDEBUG
// #define BRIM_DEBUG_TO_SVG
#endif

#if defined(BRIM_DEBUG_TO_SVG)
#include "SVG.hpp"
#include "Brim.hpp"
#endif

namespace Slic3r {

    // AnkerMake: generate brim area by objs
    static void append_and_translate(ExPolygons& dst, const ExPolygons& src,
        const PrintInstance& instance, const Print& print, std::map<ObjectID, ExPolygons>& brimAreaMap) {
        ExPolygons srcShifted = src;
        Point instance_shift = instance.shift_without_plate_offset();
        for (size_t src_idx = 0; src_idx < srcShifted.size(); ++src_idx)
            srcShifted[src_idx].translate(instance_shift);
        srcShifted = diff_ex(srcShifted, dst);
        //expolygons_append(dst, temp2);
        expolygons_append(brimAreaMap[instance.print_object->id()], std::move(srcShifted));
    }

    static void append_and_translate(ExPolygons& dst, const ExPolygons& src, const PrintInstance& instance) {
        size_t dst_idx = dst.size();
        expolygons_append(dst, src);
        for (; dst_idx < dst.size(); ++dst_idx)
            dst[dst_idx].translate(instance.shift.x(), instance.shift.y());
    }

    static void append_and_translate(Polygons& dst, const Polygons& src, const PrintInstance& instance) {
        size_t dst_idx = dst.size();
        polygons_append(dst, src);
        for (; dst_idx < dst.size(); ++dst_idx)
            dst[dst_idx].translate(instance.shift.x(), instance.shift.y());
    }

    static float max_brim_width(const SpanOfConstPtrs<PrintObject>& objects)
    {
        assert(!objects.empty());
        return float(std::accumulate(objects.begin(), objects.end(), 0.,
            [](double partial_result, const PrintObject* object) {
                return std::max(partial_result, object->config().brim_type == btNoBrim ? 0. : object->config().brim_width.value);
            }));
    }

    // Returns ExPolygons of the bottom layer of the print object after elephant foot compensation.
    static ExPolygons get_print_object_bottom_layer_expolygons(const PrintObject& print_object)
    {
        ExPolygons ex_polygons;
        for (LayerRegion* region : print_object.layers().front()->regions())
            Slic3r::append(ex_polygons, closing_ex(region->slices().surfaces, float(SCALED_EPSILON)));
        return ex_polygons;
    }

    // Returns ExPolygons of bottom layer for every print object in Print after elephant foot compensation.
    static std::vector<ExPolygons> get_print_bottom_layers_expolygons(const Print& print)
    {
        std::vector<ExPolygons> bottom_layers_expolygons;
        bottom_layers_expolygons.reserve(print.objects().size());
        for (const PrintObject* object : print.objects())
            bottom_layers_expolygons.push_back(get_print_object_bottom_layer_expolygons(*object));

        return bottom_layers_expolygons;
    }

    // Generate ears brim
    static ExPolygons make_brim_ears(ExPolygons& obj_expoly,
        coord_t size_ear,
        coord_t ear_detection_length,
        coordf_t brim_ears_max_angle,
        bool is_outer_brim) {
        ExPolygons mouse_ears_ex;
        if (size_ear <= 0) {
            return mouse_ears_ex;
        }

        // find suitable places to put ears
        const coordf_t angle_threshold = (180 - brim_ears_max_angle) * PI / 180.0;
        Points pt_ears;
        for (ExPolygon& poly : obj_expoly) {
            Polygon dectimated_polygon = poly.contour;
            if (ear_detection_length > 0) {
                // decimate polygon
                Points points = poly.contour.points;
                points.push_back(points.front());
                points = MultiPoint::_douglas_peucker(points, ear_detection_length);
                // don't decimate if it's going to be below 4 points,as it's surely enough to fill everything anyway
                if (points.size() > 4) {
                    points.erase(points.end() - 1);
                    dectimated_polygon.points = points;
                }
            }

            append(pt_ears, is_outer_brim ? dectimated_polygon.convex_points(angle_threshold)
                : dectimated_polygon.concave_points(angle_threshold));
        }

        // add ears and create ear pattern
        Polygon point_round;
        for (size_t i = 0; i < POLY_SIDES; i++) {
            double angle = (2.0 * PI * i) / POLY_SIDES;
            point_round.points.emplace_back(size_ear * cos(angle), size_ear * sin(angle));
        }

        for (Point& pt : pt_ears) {
            mouse_ears_ex.emplace_back();
            mouse_ears_ex.back().contour = point_round;
            mouse_ears_ex.back().contour.translate(pt);
        }
        return mouse_ears_ex;
    }

    // adhension coefficients from print object class
    double getadhesionCoeff(const PrintObject* printObject)
    {
        auto& insts = printObject->instances();
        auto objectVolumes = insts[0].model_instance->get_object()->volumes;
        auto print = printObject->print();
        std::vector<size_t> extrudersFirstLayer;
        auto firstLayerRegions = printObject->layers().front()->regions();
        if (!firstLayerRegions.empty()) {
            for (const LayerRegion* regionPtr : firstLayerRegions) {
                if (regionPtr->has_extrusions())
                    extrudersFirstLayer.push_back(regionPtr->region().extruder(frExternalPerimeter));
            }
        }
        double adhensionCoeff = 1;
        for (const ModelVolume* modelVolume : objectVolumes) {
            for (auto iter = extrudersFirstLayer.begin(); iter != extrudersFirstLayer.end(); iter++)
                if (modelVolume->extruder_id() == *iter) {
                    if (Model::extruderParamsMap.find(modelVolume->extruder_id()) != Model::extruderParamsMap.end())
                        if (Model::extruderParamsMap.at(modelVolume->extruder_id()).materialName == "PETG") {
                            adhensionCoeff = 2;
                        }
                        else if (Model::extruderParamsMap.at(modelVolume->extruder_id()).materialName == "TPU") {
                            adhensionCoeff = 0.5;
                        }
                }
        }
        return  adhensionCoeff;
    }

    // Virgil : properties of an expolygon
    struct ExPolyProp
    {
        double aera = 0;
        Vec2d  centroid;
        Vec2d  secondMomentOfAreaRespectToCentroid;

    };

    // Virgil : second moment of area of a polygon
    bool compSecondMoment(Polygon poly, Vec2d& sm)
    {
        if (poly.is_clockwise())
            poly.make_counter_clockwise();

        sm = Vec2d(0., 0.);
        if (poly.points.size() >= 3) {
            Vec2d p1 = poly.points.back().cast<double>();
            for (const Point& p : poly.points) {
                Vec2d p2 = p.cast<double>();
                double a = cross2(p1, p2);
                sm += Vec2d((p1.y() * p1.y() + p1.y() * p2.y() + p2.y() * p2.y()), (p1.x() * p1.x() + p1.x() * p2.x() + p2.x() * p2.x())) * a / 12;
                p1 = p2;
            }
            return true;
        }
        return false;
    }

    // Virgil : second moment of area of single expolygon
    bool compSecondMoment(const ExPolygon& expoly, ExPolyProp& expolyProp)
    {
        double aera = expoly.contour.area();
        Vec2d cent = expoly.contour.centroid().cast<double>() * aera;
        Vec2d sm;
        if (!compSecondMoment(expoly.contour, sm))
            return false;

        for (auto& hole : expoly.holes) {
            double a = hole.area();
            aera += hole.area();
            cent += hole.centroid().cast<double>() * a;
            Vec2d smh;
            if (compSecondMoment(hole, smh))
                sm += -smh;
        }

        cent = cent / aera;
        sm = sm - Vec2d(cent.y() * cent.y(), cent.x() * cent.x()) * aera;
        expolyProp.aera = aera;
        expolyProp.centroid = cent;
        expolyProp.secondMomentOfAreaRespectToCentroid = sm;
        return true;
    }

    // Virgil : calculate the second moment of area of expolygons
    bool compSecondMoment(const ExPolygons& expolys, double& smExpolysX, double& smExpolysY)
    {
        if (expolys.empty()) return false;
        std::vector<ExPolyProp> props;
        for (const ExPolygon& expoly : expolys) {
            ExPolyProp prop;
            if (compSecondMoment(expoly, prop))
                props.push_back(prop);
        }
        if (props.empty())
            return false;
        double totalArea = 0.;
        Vec2d staticMoment(0., 0.);
        for (const ExPolyProp& prop : props) {
            totalArea += prop.aera;
            staticMoment += prop.centroid * prop.aera;
        }
        double totalCentroidX = staticMoment.x() / totalArea;
        double totalCentroidY = staticMoment.y() / totalArea;

        smExpolysX = 0;
        smExpolysY = 0;
        for (const ExPolyProp& prop : props) {
            double deltaX = prop.centroid.x() - totalCentroidX;
            double deltaY = prop.centroid.y() - totalCentroidY;
            smExpolysX += prop.secondMomentOfAreaRespectToCentroid.x() + prop.aera * deltaY * deltaY;
            smExpolysY += prop.secondMomentOfAreaRespectToCentroid.y() + prop.aera * deltaX * deltaX;
        }

        return true;
    }

    //BBS: config brim_width by group of volumes
    double configBrimWidthByVolumeGroups(double adhension, double maxSpeed, const std::vector<ModelVolume*> modelVolumePtrs, const ExPolygons& expolys, double& groupHeight)
    {
        // height of a group of volumes
        double height = 0;
        BoundingBoxf3 mergedBbx;
        for (const auto& modelVolumePtr : modelVolumePtrs) {
            if (modelVolumePtr->is_model_part()) {
                Slic3r::Transform3d t;
                if (modelVolumePtr->get_object()->instances.size() > 0)
                    t = modelVolumePtr->get_object()->instances.front()->get_matrix() * modelVolumePtr->get_matrix();
                else
                    t = modelVolumePtr->get_matrix();
                auto bbox = modelVolumePtr->mesh().transformed_bounding_box(t);
                mergedBbx.merge(bbox);
            }
        }
        auto bbox_size = mergedBbx.size();
        height = bbox_size(2);
        groupHeight = height;
        // second moment of the expolygons of the first layer of the volume group
        double Ixx = -1.e30, Iyy = -1.e30;
        if (!expolys.empty()) {
            if (!compSecondMoment(expolys, Ixx, Iyy))
                Ixx = Iyy = -1.e30;
        }
        Ixx = Ixx * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR;
        Iyy = Iyy * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR;

        // bounding box of the expolygons of the first layer of the volume
        BoundingBox bbox2;
        for (const auto& expoly : expolys)
            bbox2.merge(get_extents(expoly.contour));
        const double& bboxX = bbox2.size()(0) * SCALING_FACTOR;
        const double& bboxY = bbox2.size()(1) * SCALING_FACTOR;
        double thermalLength = sqrt(bboxX * bboxX + bboxY * bboxY);
        double thermalLengthRef = Model::getThermalLength(modelVolumePtrs);
        double height_to_area = std::max(height / Ixx * bboxY, height / Iyy * bboxX);
        double minThermalLength = 1.5 * thermalLength;
        double brim_width = adhension * std::min(std::min(std::max(height_to_area * maxSpeed / 24, thermalLength * 8. / thermalLengthRef * std::min(height, 30.) / 30.), 18.), minThermalLength);
        // small brims are omitted
        brim_width = (brim_width < 5 && brim_width < minThermalLength) ? 0 : brim_width > 18 ? 18 : brim_width;
        return brim_width;
    }

    // update the maxSpeed of an object if it is different from the global configuration
    double Model::findMaxSpeed(const ModelObject* object) {
        auto objectKeys = object->config.keys();
        double objMaxSpeed = -1.;
        if (objectKeys.empty())
            objMaxSpeed = 250.;
        //return Model::printSpeedMap.maxSpeed;
        return objMaxSpeed;
        double perimeterSpeedObj = Model::printSpeedMap.perimeterSpeed;
        double externalPerimeterSpeedObj = Model::printSpeedMap.externalPerimeterSpeed;
        double infillSpeedObj = Model::printSpeedMap.infillSpeed;
        double solidInfillSpeedObj = Model::printSpeedMap.solidInfillSpeed;
        double topSolidInfillSpeedObj = Model::printSpeedMap.topSolidInfillSpeed;
        double supportSpeedObj = Model::printSpeedMap.supportSpeed;
        double smallPerimeterSpeedObj = Model::printSpeedMap.smallPerimeterSpeed;
        for (std::string objectKey : objectKeys) {
            if (objectKey == "perimeter_speed")
                perimeterSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "infill_speed")
                infillSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "solid_infill_speed")
                solidInfillSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "top_solid_infill_speed")
                topSolidInfillSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "support_material_speed")
                supportSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "external_perimeter_speed")
                externalPerimeterSpeedObj = object->config.opt_float(objectKey);
            if (objectKey == "small_perimeter_speed")
                smallPerimeterSpeedObj = object->config.opt_float(objectKey);
        }
        objMaxSpeed = std::max(perimeterSpeedObj, std::max(externalPerimeterSpeedObj, std::max(infillSpeedObj, std::max(solidInfillSpeedObj, std::max(topSolidInfillSpeedObj, std::max(supportSpeedObj, std::max(smallPerimeterSpeedObj, objMaxSpeed)))))));
        if (objMaxSpeed <= 0) objMaxSpeed = 250.;
        return objMaxSpeed;
    }

    // BBS: thermal length is calculated according to the material of a volume
    double Model::getThermalLength(const ModelVolume* modelVolumePtr) {
        double thermalLength = 1000.;
        auto aa = modelVolumePtr->extruder_id();
        if (Model::extruderParamsMap.find(aa) != Model::extruderParamsMap.end()) {
            if (Model::extruderParamsMap.at(aa).materialName == "ABS" ||
                Model::extruderParamsMap.at(aa).materialName == "PA-CF" ||
                Model::extruderParamsMap.at(aa).materialName == "PET-CF") {
                thermalLength = 100;
            }
            if (Model::extruderParamsMap.at(aa).materialName == "PC") {
                thermalLength = 40;
            }
            if (Model::extruderParamsMap.at(aa).materialName == "TPU") {
                thermalLength = 1000;
            }

        }
        return thermalLength;
    }

    // BBS: thermal length calculation for a group of volumes
    double Model::getThermalLength(const std::vector<ModelVolume*> modelVolumePtrs)
    {
        double thermalLength = 1250.;

        for (const auto& modelVolumePtr : modelVolumePtrs) {
            if (modelVolumePtr != nullptr) {
                // the thermal length of a group is decided by the volume with shortest thermal length
                thermalLength = std::min(thermalLength, getThermalLength(modelVolumePtr));
            }
        }
        return thermalLength;
    }

    static ConstPrintObjectPtrs get_top_level_objects_with_brim(const Print& print, const std::vector<ExPolygons>& bottom_layers_expolygons)
    {
        assert(print.objects().size() == bottom_layers_expolygons.size());
        Polygons             islands;
        ConstPrintObjectPtrs island_to_object;
        for (size_t print_object_idx = 0; print_object_idx < print.objects().size(); ++print_object_idx) {
            const PrintObject* object = print.objects()[print_object_idx];
            Polygons islands_object;
            islands_object.reserve(bottom_layers_expolygons[print_object_idx].size());
            for (const ExPolygon& ex_poly : bottom_layers_expolygons[print_object_idx])
                islands_object.emplace_back(ex_poly.contour);

            islands.reserve(islands.size() + object->instances().size() * islands_object.size());
            for (const PrintInstance& instance : object->instances())
                for (Polygon& poly : islands_object) {
                    islands.emplace_back(poly);
                    islands.back().translate(instance.shift);
                    island_to_object.emplace_back(object);
                }
        }
        assert(islands.size() == island_to_object.size());

        ClipperLib_Z::Paths islands_clip;
        islands_clip.reserve(islands.size());
        for (const Polygon& poly : islands) {
            islands_clip.emplace_back();
            ClipperLib_Z::Path& island_clip = islands_clip.back();
            island_clip.reserve(poly.points.size());
            int island_idx = int(&poly - &islands.front());
            // The Z coordinate carries index of the island used to get the pointer to the object.
            for (const Point& pt : poly.points)
                island_clip.emplace_back(pt.x(), pt.y(), island_idx + 1);
        }

        // Init Clipper
        ClipperLib_Z::Clipper clipper;
        // Assign the maximum Z from four points. This values is valid index of the island
        clipper.ZFillFunction([](const ClipperLib_Z::IntPoint& e1bot, const ClipperLib_Z::IntPoint& e1top, const ClipperLib_Z::IntPoint& e2bot,
            const ClipperLib_Z::IntPoint& e2top, ClipperLib_Z::IntPoint& pt) {
                pt.z() = std::max(std::max(e1bot.z(), e1top.z()), std::max(e2bot.z(), e2top.z()));
            });
        // Add islands
        clipper.AddPaths(islands_clip, ClipperLib_Z::ptSubject, true);
        // Execute union operation to construct poly tree
        ClipperLib_Z::PolyTree islands_polytree;
        //FIXME likely pftNonZero or ptfPositive would be better. Why are we using ptfEvenOdd for Unions?
        clipper.Execute(ClipperLib_Z::ctUnion, islands_polytree, ClipperLib_Z::pftEvenOdd, ClipperLib_Z::pftEvenOdd);

        std::unordered_set<size_t> processed_objects_idx;
        ConstPrintObjectPtrs       top_level_objects_with_brim;
        for (int i = 0; i < islands_polytree.ChildCount(); ++i) {
            for (const ClipperLib_Z::IntPoint& point : islands_polytree.Childs[i]->Contour) {
                if (point.z() != 0 && processed_objects_idx.find(island_to_object[point.z() - 1]->id().id) == processed_objects_idx.end()) {
                    top_level_objects_with_brim.emplace_back(island_to_object[point.z() - 1]);
                    processed_objects_idx.insert(island_to_object[point.z() - 1]->id().id);
                }
            }
        }
        return top_level_objects_with_brim;
    }

    static Polygons top_level_outer_brim_islands(const ConstPrintObjectPtrs& top_level_objects_with_brim, const double scaled_resolution)
    {
        Polygons islands;
        for (const PrintObject* object : top_level_objects_with_brim) {
            if (!object->has_brim())
                continue;

            //FIXME how about the brim type?
            auto     brim_separation = float(scale_(object->config().brim_separation.value));
            Polygons islands_object;
            for (const ExPolygon& ex_poly : get_print_object_bottom_layer_expolygons(*object)) {
                Polygons contour_offset = offset(ex_poly.contour, brim_separation, ClipperLib::jtSquare);
                for (Polygon& poly : contour_offset)
                    poly.douglas_peucker(scaled_resolution);

                polygons_append(islands_object, std::move(contour_offset));
            }

            if (!object->support_layers().empty()) {
                for (const Polygon& support_contour : object->support_layers().front()->support_fills.polygons_covered_by_spacing()) {
                    Polygons contour_offset = offset(support_contour, brim_separation, ClipperLib::jtSquare);
                    for (Polygon& poly : contour_offset)
                        poly.douglas_peucker(scaled_resolution);

                    polygons_append(islands_object, std::move(contour_offset));
                }
            }

            for (const PrintInstance& instance : object->instances())
                append_and_translate(islands, islands_object, instance);
        }
        return islands;
    }

    static ExPolygons top_level_outer_brim_area(const Print& print,
        const ConstPrintObjectPtrs& top_level_objects_with_brim,
        const std::vector<ExPolygons>& bottom_layers_expolygons,
        // anker_make
        std::map<ObjectID, double>& brim_width_map)
    {
        assert(print.objects().size() == bottom_layers_expolygons.size());
        std::unordered_set<size_t> top_level_objects_idx;
        top_level_objects_idx.reserve(top_level_objects_with_brim.size());
        for (const PrintObject* object : top_level_objects_with_brim)
            top_level_objects_idx.insert(object->id().id);
        const auto         scaled_resolution    = scaled<double>(print.config().resolution.value);
        const Flow         flow                 = print.brim_flow();
        const float        scaled_flow_width    = flow.scaled_spacing();
        const double       flow_width           = flow.scaled_spacing() * SCALING_FACTOR;
        ExPolygons         brim_area;
        ExPolygons         no_brim_area;
        Polygons           holes;
        Polygon            bed_shape(get_bed_shape(print.config()));
        holes.emplace_back(get_bed_shape(print.config()));
        for (size_t print_object_idx = 0; print_object_idx < print.objects().size(); ++print_object_idx) {
            const PrintObject* object                       = print.objects()[print_object_idx];
            const BrimType     brim_type                    = object->config().brim_type.value;
            const float        scaled_brim_separation       = scale_(object->config().brim_separation.value);
            // calculate auto_brim_width of single object
            const float        scaled_additional_brim_width = scale_(floor(5 / flow_width / 2) * flow_width * 2);
            const float        scaled_additional_brim_width_support = scale_(floor(3.0 / flow_width / 2) * flow_width * 2);
            const float        scaled_half_min_adh_length   = scale_(9.0);
            const float        scaled_support_half_min_adh_length = scale_(50.0);
            const bool         has_brim_auto                = object->config().brim_type == btAutoBrim;
            const bool         use_brim_ears                = object->config().brim_type == btEar;
            const bool         has_inner_brim               = brim_type == btInnerOnly || brim_type == btOuterAndInner;
            const bool         has_outer_brim               = brim_type == btOuterOnly || brim_type == btOuterAndInner || brim_type == btAutoBrim || use_brim_ears;
            double             brim_width                   = scale_(floor(object->config().brim_width.value / flow_width / 2) * flow_width * 2);
            const coord_t      ear_detection_length         = scale_(object->config().brim_ears_detection_length.value);
            const coordf_t     brim_ears_max_angle          = object->config().brim_ears_max_angle.value;
            ExPolygons         brim_area_object;
            ExPolygons         no_brim_area_object;
            Polygons           holes_object;
            const double       adhension                    = getadhesionCoeff(object);
            const double       maxSpeed                     = Model::findMaxSpeed(object->model_object());  // TODO
            double             max_brim_width               = brim_width;

            for (const auto& volumeGroup : object->firstLayerObjGroups()) {
                // find volumePtrs included in this group
                std::vector<ModelVolume*> groupVolumePtrs;
                for (auto& volumeID : volumeGroup.volume_ids) {
                    ModelVolume* currentModelVolumePtr = nullptr;
                    //BBS: support shared object logic
                    const PrintObject* shared_object = object->get_shared_object();
                    if (!shared_object)
                        shared_object = object;
                    for (auto volumePtr : shared_object->model_object()->volumes) {
                        if (volumePtr->id() == volumeID) {
                            currentModelVolumePtr = volumePtr;
                            break;
                        }
                    }
                    if (currentModelVolumePtr != nullptr) groupVolumePtrs.push_back(currentModelVolumePtr);
                }
                if (groupVolumePtrs.empty()) continue;
                double groupHeight = 0.;
                if (has_brim_auto) {
                    double brimWidthRaw = configBrimWidthByVolumeGroups(adhension, maxSpeed, groupVolumePtrs, volumeGroup.slices, groupHeight);
                    brim_width = scale_(floor(brimWidthRaw / flow_width / 2) * flow_width * 2);
                }
                for (const ExPolygon& ex_poly : bottom_layers_expolygons[print_object_idx]) {
                    double brim_width_mod = brim_width;
                    if (has_brim_auto) {
                        if (brim_width < scale_(5.) && groupHeight > 3.) {
                            brim_width_mod = ex_poly.area() / ex_poly.contour.length() < scaled_half_min_adh_length
                                 && brim_width < scaled_flow_width ? brim_width + scaled_additional_brim_width : brim_width;
                        }
                        BoundingBox bbox2 = ex_poly.contour.bounding_box();
                        brim_width_mod = std::min(brim_width_mod, double(std::max(bbox2.size()(0), bbox2.size()(1))));
                    }
                    brim_width_mod = floor(brim_width_mod / scaled_flow_width / 2) * scaled_flow_width * 2;
                    Polygons ex_poly_holes_reversed = ex_poly.holes;
                    polygons_reverse(ex_poly_holes_reversed);
                    if (has_outer_brim) {
                        // BBS: inner and outer boundary are offset from the same polygon incase of round off error.
                        auto innerExpoly = offset_ex(ex_poly.contour, scaled_brim_separation, jtRound, SCALED_RESOLUTION);
                        auto& clipExpoly = innerExpoly;

                        if (use_brim_ears) {
                            coord_t size_ear = (brim_width_mod - scaled_brim_separation - scaled_flow_width);
                            append(brim_area_object, diff_ex(make_brim_ears(innerExpoly, size_ear, ear_detection_length, brim_ears_max_angle, true), clipExpoly));
                        }
                        else {
                            // Normal brims
                            append(brim_area_object, diff_ex(offset_ex(innerExpoly, brim_width_mod, jtRound, SCALED_RESOLUTION), clipExpoly));
                        }
                    }
                    if (!has_inner_brim) {
                        // BBS: brim should be apart from holes
                        append(no_brim_area_object, diff_ex(ex_poly_holes_reversed, offset_ex(ex_poly_holes_reversed, -scale_(5.))));
                    }
                    if (!has_outer_brim)
                        append(no_brim_area_object, diff_ex(offset(ex_poly.contour, scaled_flow_width), ex_poly_holes_reversed));
                    if (!has_inner_brim && !has_outer_brim)
                        append(no_brim_area_object, offset_ex(ex_poly_holes_reversed, -scaled_flow_width));
                    append(holes_object, ex_poly_holes_reversed);
                    no_brim_area_object.emplace_back(ex_poly.contour);
                    max_brim_width = std::max(max_brim_width, brim_width_mod);
                }
            }
            brim_width_map.insert(std::make_pair(object->id(), max_brim_width));
            const bool         is_top_outer_brim = top_level_objects_idx.find(object->id().id) != top_level_objects_idx.end();
            if (!object->support_layers().empty() && has_outer_brim) {
                double brim_width_support = scale_(EPSILON);
                for (const Polygon& support_contour : to_polygons(object->support_layers().front()->support_islands)) {
                    if (brim_type == btAutoBrim && object->support_layers().size() > 3. && support_contour.area() < scale_(scaled_support_half_min_adh_length))
                        brim_width_support = scaled_additional_brim_width_support;
                    if (brim_type != btAutoBrim) brim_width_support = brim_width;
                    append(brim_area_object, offset_ex(support_contour, brim_width_support, ClipperLib::jtRound, scaled_resolution));
                    append(no_brim_area_object, offset_ex(support_contour, 0.0));
                    no_brim_area_object.emplace_back(support_contour);
                }
            }
            for (const PrintInstance& instance : object->instances()) {
                append_and_translate(brim_area, brim_area_object, instance);
                append_and_translate(no_brim_area, no_brim_area_object, instance);
                append_and_translate(holes, holes_object, instance);
            }
        }
        brim_area = intersection_ex(to_polygons(brim_area), holes);
        return diff_ex(std::move(brim_area), no_brim_area);
    }

    // AnkerMake: the brims of different objs will not overlapped with each other, and are stored by objs and by extruders
    static ExPolygons top_level_outer_brim_area(const Print& print, const ConstPrintObjectPtrs& top_level_objects_with_brim,
        const float no_brim_offset, double& brim_width_max, std::map<ObjectID, double>& brim_width_map,
        std::map<ObjectID, ExPolygons>& brimAreaMap,
        std::map<ObjectID, ExPolygons>& supportBrimAreaMap, std::vector<std::pair<ObjectID, unsigned int>>& objPrintVec)
    {
        std::unordered_set<size_t> top_level_objects_idx;
        top_level_objects_idx.reserve(top_level_objects_with_brim.size());
        for (const PrintObject* object : top_level_objects_with_brim)
            top_level_objects_idx.insert(object->id().id);

        unsigned int support_material_extruder = 1;
        if (print.has_support_material()) {
            assert(top_level_objects_with_brim.front()->config().support_material >= 0);
            if (top_level_objects_with_brim.front()->config().support_material > 0)
                support_material_extruder = top_level_objects_with_brim.front()->config().support_material;
        }

        ExPolygons brim_area;
        ExPolygons no_brim_area;
        brim_width_max = 0;
        struct brimWritten {
            bool obj;
            bool sup;
        };
        std::map<ObjectID, brimWritten> brimToWrite;
        for (const auto& objectWithExtruder : objPrintVec)
            brimToWrite.insert({ objectWithExtruder.first, {true,true} });

        for (unsigned int extruderNo : print.extruders()) {
            ++extruderNo;
            for (const auto& objectWithExtruder : objPrintVec) {
                const PrintObject* object = print.get_object(objectWithExtruder.first);
                const BrimType brim_type = object->config().brim_type.value;
                const float    brim_offset = scale_(object->config().brim_separation.value);
                // recording the autoAssigned brimWidth and corresponding objs
                double brimWidthAuto = object->config().brim_width.value;
                double flowWidth = print.brim_flow().scaled_spacing() * SCALING_FACTOR;
                brimWidthAuto = floor(brimWidthAuto / flowWidth / 2) * flowWidth * 2;
                brim_width_map.insert(std::make_pair(object->id(), brimWidthAuto));
                brim_width_max = std::max(brim_width_max, brimWidthAuto);
                const float    brim_width = scale_(brimWidthAuto);
                const bool     is_top_outer_brim = top_level_objects_idx.find(object->id().id) != top_level_objects_idx.end();

                ExPolygons nullBrim;
                brimAreaMap.insert(std::make_pair(object->id(), nullBrim));
                ExPolygons brim_area_object;
                ExPolygons brim_area_support;
                ExPolygons no_brim_area_object;
                ExPolygons no_brim_area_support;
                if (objectWithExtruder.second == extruderNo && brimToWrite.at(object->id()).obj) {
                    for (const ExPolygon& ex_poly : object->layers().front()->lslices) {
                        if ((brim_type == BrimType::btOuterOnly || brim_type == BrimType::btOuterAndInner || brim_type == BrimType::btAutoBrim) && is_top_outer_brim) {
                            append(brim_area_object, diff_ex(offset_ex(ex_poly.contour, brim_width + brim_offset, jtRound, SCALED_RESOLUTION),
                                offset_ex(ex_poly.contour, brim_offset)));
                        }
                        if (brim_type == BrimType::btOuterOnly || brim_type == BrimType::btNoBrim)
                            append(no_brim_area_object, offset_ex(ex_poly.holes, -no_brim_offset));

                        if (brim_type == BrimType::btInnerOnly || brim_type == BrimType::btNoBrim)
                            append(no_brim_area_object, diff_ex(offset(ex_poly.contour, no_brim_offset), ex_poly.holes));

                        if (brim_type != BrimType::btNoBrim)
                            append(no_brim_area_object, offset_ex(ExPolygon(ex_poly.contour), brim_offset));

                        no_brim_area_object.emplace_back(ex_poly.contour);
                    }
                    brimToWrite.at(object->id()).obj = false;
                    for (const PrintInstance& instance : object->instances()) {
                        if (!brim_area_object.empty())
                            append_and_translate(brim_area, brim_area_object, instance, print, brimAreaMap);
                        append_and_translate(no_brim_area, no_brim_area_object, instance);
                    }
                    if (brimAreaMap.find(object->id()) != brimAreaMap.end())
                        expolygons_append(brim_area, brimAreaMap[object->id()]);
                }
                if (support_material_extruder == extruderNo && brimToWrite.at(object->id()).sup) {
                    if (!object->support_layers().empty()) {
                        for (const Polygon& support_contour : object->support_layers().front()->support_fills.polygons_covered_by_spacing()) {
                            //BBS: no brim offset for supports
                            if ((brim_type == BrimType::btOuterOnly || brim_type == BrimType::btOuterAndInner || brim_type == BrimType::btAutoBrim) && is_top_outer_brim)
                                append(brim_area_support, diff_ex(offset(support_contour, brim_width, jtRound, SCALED_RESOLUTION), offset(support_contour, 0)));

                            if (brim_type != BrimType::btNoBrim)
                                append(no_brim_area_support, offset_ex(support_contour, 0));

                            no_brim_area_support.emplace_back(support_contour);
                        }
                    }

                    brimToWrite.at(object->id()).sup = false;
                    for (const PrintInstance& instance : object->instances()) {
                        if (!brim_area_support.empty())
                            append_and_translate(brim_area, brim_area_support, instance, print, supportBrimAreaMap);
                        append_and_translate(no_brim_area, no_brim_area_support, instance);
                    }
                    if (supportBrimAreaMap.find(object->id()) != supportBrimAreaMap.end())
                        expolygons_append(brim_area, supportBrimAreaMap[object->id()]);
                }
            }
        }
        for (const PrintObject* object : print.objects()) {
            if (brimAreaMap.find(object->id()) != brimAreaMap.end())
                brimAreaMap[object->id()] = diff_ex(brimAreaMap[object->id()], no_brim_area);
            if (supportBrimAreaMap.find(object->id()) != supportBrimAreaMap.end())
                supportBrimAreaMap[object->id()] = diff_ex(supportBrimAreaMap[object->id()], no_brim_area);
        }
        return diff_ex(std::move(brim_area), no_brim_area);
    }

    // Return vector of booleans indicated if polygons from bottom_layers_expolygons contain another polygon or not.
    // Every ExPolygon is counted as several Polygons (contour and holes). Contour polygon is always processed before holes.
    static std::vector<bool> has_polygons_nothing_inside(const Print& print, const std::vector<ExPolygons>& bottom_layers_expolygons)
    {
        assert(print.objects().size() == bottom_layers_expolygons.size());
        Polygons islands;
        for (size_t print_object_idx = 0; print_object_idx < print.objects().size(); ++print_object_idx) {
            const PrintObject* object = print.objects()[print_object_idx];
            const Polygons     islands_object = to_polygons(bottom_layers_expolygons[print_object_idx]);

            islands.reserve(islands.size() + object->instances().size() * islands_object.size());
            for (const PrintInstance& instance : object->instances())
                append_and_translate(islands, islands_object, instance);
        }

        ClipperLib_Z::Paths islands_clip;
        islands_clip.reserve(islands.size());
        for (const Polygon& poly : islands) {
            size_t             island_idx = &poly - &islands.front();
            ClipperLib_Z::Path island_clip;
            for (const Point& pt : poly.points)
                island_clip.emplace_back(pt.x(), pt.y(), island_idx + 1);
            islands_clip.emplace_back(island_clip);
        }

        ClipperLib_Z::Clipper clipper;
        // Always assign zero to detect cases when two polygons are overlapping.
        clipper.ZFillFunction([](const ClipperLib_Z::IntPoint& e1bot, const ClipperLib_Z::IntPoint& e1top, const ClipperLib_Z::IntPoint& e2bot, const ClipperLib_Z::IntPoint& e2top, ClipperLib_Z::IntPoint& pt) {
            pt.z() = 0;
            });

        clipper.AddPaths(islands_clip, ClipperLib_Z::ptSubject, true);
        ClipperLib_Z::PolyTree islands_polytree;
        clipper.Execute(ClipperLib_Z::ctUnion, islands_polytree, ClipperLib_Z::pftEvenOdd, ClipperLib_Z::pftEvenOdd);

        std::vector<bool> has_nothing_inside(islands.size());
        std::function<void(const ClipperLib_Z::PolyNode&)> check_contours = [&check_contours, &has_nothing_inside](const ClipperLib_Z::PolyNode& parent_node)->void {
            if (!parent_node.Childs.empty())
                for (const ClipperLib_Z::PolyNode* child_node : parent_node.Childs)
                    check_contours(*child_node);

            if (parent_node.Childs.empty() && !parent_node.Contour.empty() && parent_node.Contour.front().z() != 0) {
                int polygon_idx = parent_node.Contour.front().z();
                assert(polygon_idx > 0 && polygon_idx <= int(has_nothing_inside.size()));

                // The whole contour must have the same ID. In other cases, some counters overlap.
                for (const ClipperLib_Z::IntPoint& point : parent_node.Contour)
                    if (polygon_idx != point.z())
                        return;

                has_nothing_inside[polygon_idx - 1] = true;
            }
        };

        check_contours(islands_polytree);
        return has_nothing_inside;
    }

    // INNERMOST means that ExPolygon doesn't contain any other ExPolygons.
    // NORMAL is for other cases.
    enum class InnerBrimType { NORMAL, INNERMOST };

    struct InnerBrimExPolygons
    {
        ExPolygons    brim_area;
        InnerBrimType type = InnerBrimType::NORMAL;
        double        brim_width = 0.;
    };

    static std::vector<InnerBrimExPolygons> inner_brim_area(const Print& print,
        const ConstPrintObjectPtrs& top_level_objects_with_brim,
        const std::vector<ExPolygons>& bottom_layers_expolygons,
        const ExPolygons& outer_brim_exs,
        const float                    no_brim_offset)
    {
        assert(print.objects().size() == bottom_layers_expolygons.size());
        std::vector<bool>          has_nothing_inside = has_polygons_nothing_inside(print, bottom_layers_expolygons);
        std::unordered_set<size_t> top_level_objects_idx;
        top_level_objects_idx.reserve(top_level_objects_with_brim.size());
        for (const PrintObject* object : top_level_objects_with_brim)
            top_level_objects_idx.insert(object->id().id);

        std::vector<ExPolygons> brim_area_innermost(print.objects().size());
        ExPolygons              brim_area;
        ExPolygons              no_brim_area;
        Polygons                holes_reversed;
        Polygon                 bed_shape(get_bed_shape(print.config()));
        holes_reversed.emplace_back(get_bed_shape(print.config()));
        // polygon_idx must correspond to idx generated inside has_polygons_nothing_inside()
        size_t polygon_idx = 0;
        for (size_t print_object_idx = 0; print_object_idx < print.objects().size(); ++print_object_idx) {
            const PrintObject* object = print.objects()[print_object_idx];
            const BrimType     brim_type = object->config().brim_type.value;
            const float        brim_separation = scale_(object->config().brim_separation.value);
            const float        brim_width = scale_(object->config().brim_width.value);
            const bool         top_outer_brim = top_level_objects_idx.find(object->id().id) != top_level_objects_idx.end();

            ExPolygons brim_area_innermost_object;
            ExPolygons brim_area_object;
            ExPolygons no_brim_area_object;
            Polygons   holes_reversed_object;
            for (const ExPolygon& ex_poly : bottom_layers_expolygons[print_object_idx]) {
                if (brim_type == BrimType::btOuterOnly || brim_type == BrimType::btOuterAndInner || brim_type == BrimType::btAutoBrim || brim_type == BrimType::btEar) {
                    if (top_outer_brim)
                        no_brim_area_object.emplace_back(ex_poly);
                    else
                        append(brim_area_object, diff_ex(offset(ex_poly.contour, brim_width + brim_separation, ClipperLib::jtSquare), offset(ex_poly.contour, brim_separation, ClipperLib::jtSquare)));
                }

                // After 7ff76d07684858fd937ef2f5d863f105a10f798e offset and shrink don't work with CW polygons (holes), so let's make it CCW.
                Polygons ex_poly_holes_reversed = ex_poly.holes;
                polygons_reverse(ex_poly_holes_reversed);
                for ([[maybe_unused]] const PrintInstance& instance : object->instances()) {
                    ++polygon_idx; // Increase idx because of the contour of the ExPolygon.

                    if (brim_type == BrimType::btInnerOnly || brim_type == BrimType::btOuterAndInner)
                        for (const Polygon& hole : ex_poly_holes_reversed) {
                            size_t hole_idx = &hole - &ex_poly_holes_reversed.front();
                            if (has_nothing_inside[polygon_idx + hole_idx])
                                append(brim_area_innermost_object, shrink_ex({ hole }, brim_separation, ClipperLib::jtSquare));
                            else
                                append(brim_area_object, diff_ex(shrink_ex({ hole }, brim_separation, ClipperLib::jtSquare), shrink_ex({ hole }, brim_width + brim_separation, ClipperLib::jtSquare)));
                        }

                    polygon_idx += ex_poly.holes.size(); // Increase idx for every hole of the ExPolygon.
                }

                if (brim_type == BrimType::btInnerOnly || brim_type == BrimType::btNoBrim)
                    append(no_brim_area_object, diff_ex(offset(ex_poly.contour, no_brim_offset, ClipperLib::jtSquare), ex_poly_holes_reversed));

                if (brim_type == BrimType::btOuterOnly || brim_type == BrimType::btNoBrim)
                    append(no_brim_area_object, diff_ex(ex_poly.contour, shrink_ex(ex_poly_holes_reversed, no_brim_offset, ClipperLib::jtSquare)));

                append(holes_reversed_object, ex_poly_holes_reversed);
            }
            append(no_brim_area_object, offset_ex(bottom_layers_expolygons[print_object_idx], brim_separation, ClipperLib::jtSquare));

            for (const PrintInstance& instance : object->instances()) {
                append_and_translate(brim_area_innermost[print_object_idx], brim_area_innermost_object, instance);
                append_and_translate(brim_area, brim_area_object, instance);
                append_and_translate(no_brim_area, no_brim_area_object, instance);
                append_and_translate(holes_reversed, holes_reversed_object, instance);
            }
        }
        assert(polygon_idx == has_nothing_inside.size());
        ExPolygons brim_area_innermost_merged;
        // Append all innermost brim areas.
        std::vector<InnerBrimExPolygons> brim_area_out;
        for (size_t print_object_idx = 0; print_object_idx < print.objects().size(); ++print_object_idx)
            if (const double brim_width = print.objects()[print_object_idx]->config().brim_width.value; !brim_area_innermost[print_object_idx].empty()) {
                append(brim_area_innermost_merged, brim_area_innermost[print_object_idx]);
                brim_area_out.push_back({ std::move(brim_area_innermost[print_object_idx]), InnerBrimType::INNERMOST, brim_width });
            }

        // Append all normal brim areas.
        brim_area_out.push_back({ diff_ex(intersection_ex(to_polygons(std::move(brim_area)), holes_reversed), no_brim_area), InnerBrimType::NORMAL });

        // Cut out a huge brim areas that overflows into the INNERMOST holes.
        //brim_area_out.back().brim_area = diff_ex(brim_area_out.back().brim_area, brim_area_innermost_merged);
        brim_area_out.back().brim_area = diff_ex(diff_ex(brim_area_out.back().brim_area, brim_area_innermost_merged), outer_brim_exs);

        return brim_area_out;
    }

    // get maximum temperature difference from print object class
    //double getTemperatureFromExtruder(const PrintObject* printObject) {
    //    auto print = printObject->print();
    //    std::vector<size_t> extrudersFirstLayer;
    //    auto firstLayerRegions = printObject->layers().front()->regions();
    //    if (!firstLayerRegions.empty()) {
    //        for (const LayerRegion* regionPtr : firstLayerRegions) {
    //            if (regionPtr->has_extrusions())
    //                extrudersFirstLayer.push_back(regionPtr->region().extruder(frExternalPerimeter));
    //        }
    //    }
    //
    //    const PrintConfig& config = print->config();
    //    int curr_bed_type = config.option("curr_bed_type")->getInt();
    //    const ConfigOptionInt* bed_temp_1st_layer_opt = config.option<ConfigOptionInt>(get_bed_temp_1st_layer_key((BedType)curr_bed_type));
    //
    //    double maxDeltaTemp = 0;
    //    for (auto extruderId : extrudersFirstLayer) {
    //        int bedTemp = bed_temp_1st_layer_opt->get_at(extruderId - 1);
    //        if (bedTemp > maxDeltaTemp)
    //            maxDeltaTemp = bedTemp;
    //    }
    //    return maxDeltaTemp;
    //}

    // Flip orientation of open polylines to minimize travel distance.
    static void optimize_polylines_by_reversing(Polylines* polylines)
    {
        for (size_t poly_idx = 1; poly_idx < polylines->size(); ++poly_idx) {
            const Polyline& prev = (*polylines)[poly_idx - 1];
            Polyline& next = (*polylines)[poly_idx];

            if (!next.is_closed()) {
                double dist_to_start = (next.first_point() - prev.last_point()).cast<double>().norm();
                double dist_to_end = (next.last_point() - prev.last_point()).cast<double>().norm();

                if (dist_to_end < dist_to_start)
                    next.reverse();
            }
        }
    }

    static Polylines connect_brim_lines(Polylines&& polylines, const Polygons& brim_area, float max_connection_length)
    {
        if (polylines.empty())
            return {};

        BoundingBox bbox = get_extents(polylines);
        bbox.merge(get_extents(brim_area));

        EdgeGrid::Grid grid(bbox.inflated(SCALED_EPSILON));
        grid.create(brim_area, polylines, coord_t(scale_(10.)));

        struct Visitor
        {
            explicit Visitor(const EdgeGrid::Grid& grid) : grid(grid) {}

            bool operator()(coord_t iy, coord_t ix)
            {
                // Called with a row and column of the grid cell, which is intersected by a line.
                auto cell_data_range = grid.cell_data_range(iy, ix);
                this->intersect = false;
                for (auto it_contour_and_segment = cell_data_range.first; it_contour_and_segment != cell_data_range.second; ++it_contour_and_segment) {
                    // End points of the line segment and their vector.
                    auto segment = grid.segment(*it_contour_and_segment);
                    if (Geometry::segments_intersect(segment.first, segment.second, brim_line.a, brim_line.b)) {
                        this->intersect = true;
                        return false;
                    }
                }
                // Continue traversing the grid along the edge.
                return true;
            }

            const EdgeGrid::Grid& grid;
            Line                  brim_line;
            bool                  intersect = false;

        } visitor(grid);

        // Connect successive polylines if they are open, their ends are closer than max_connection_length.
        // Remove empty polylines.
        {
            // Skip initial empty lines.
            size_t poly_idx = 0;
            for (; poly_idx < polylines.size() && polylines[poly_idx].empty(); ++poly_idx);
            size_t end = ++poly_idx;
            double max_connection_length2 = Slic3r::sqr(max_connection_length);
            for (; poly_idx < polylines.size(); ++poly_idx) {
                Polyline& next = polylines[poly_idx];
                if (!next.empty()) {
                    Polyline& prev = polylines[end - 1];
                    bool   connect = false;
                    if (!prev.is_closed() && !next.is_closed()) {
                        double dist2 = (prev.last_point() - next.first_point()).cast<double>().squaredNorm();
                        if (dist2 <= max_connection_length2) {
                            visitor.brim_line.a = prev.last_point();
                            visitor.brim_line.b = next.first_point();
                            // Shrink the connection line to avoid collisions with the brim centerlines.
                            visitor.brim_line.extend(-SCALED_EPSILON);
                            grid.visit_cells_intersecting_line(visitor.brim_line.a, visitor.brim_line.b, visitor);
                            connect = !visitor.intersect;
                        }
                    }
                    if (connect) {
                        append(prev.points, std::move(next.points));
                    }
                    else {
                        if (end < poly_idx)
                            polylines[end] = std::move(next);
                        ++end;
                    }
                }
            }
            if (end < polylines.size())
                polylines.erase(polylines.begin() + int(end), polylines.end());
        }

        return std::move(polylines);
    }

    void make_inner_brim_smart_ordering(const Print& print, Polygons& loops) {
        if (loops.size() < 2) return;
        std::set<size_t> needReverseIdx;
        for (const PrintObject* obj : print.objects()) {
            if (!obj->config().brim_smart_ordering) continue;
            for (const ExPolygon& ex : obj->layers().front()->lslices) {
                for (const Polygon& hole : ex.holes) {
                    size_t idx = 0;
                    double holeLen = hole.length();
                    double disDiff = holeLen;
                    for (size_t i = 0; i < loops.size(); ++i) {
                        if (holeLen - loops[i].length() > 0 && holeLen - loops[i].length() < disDiff) {
                            idx = i;
                            disDiff = holeLen - loops[i].length();
                        }
                    }
                    if (idx != 0) needReverseIdx.insert(idx);
                }
            }
        }
        for (size_t index : needReverseIdx) {
            if (index > 0 && index < loops.size())
                std::swap(loops[index], loops[index - 1]);
        }
    }

    void make_outer_brim_smart_ordering(const Print& print, Polylines& loops) {
        if (loops.size() < 2) return;
        std::set<size_t> needReverseIdx;
        for (const PrintObject* obj : print.objects()) {
            if (!obj->config().brim_smart_ordering) continue;
            for (const ExPolygon& ex : obj->layers().front()->lslices) {
                const Polygon& cont = ex.contour;
                size_t idx = 0;
                double contLen = cont.length();
                double disDiff = loops.front().length();
                for (size_t i = 0; i < loops.size(); ++i) {
                    if (loops[i].length() - contLen > 0 && loops[i].length() - contLen < disDiff) {
                        idx = i;
                        disDiff = loops[i].length() - contLen;
                    }
                }
                if (idx != 0) needReverseIdx.insert(idx);
            }
        }
        for (size_t index : needReverseIdx) {
            if (index > 0 && index < loops.size())
                std::swap(loops[index], loops[index - 1]);
        }
    }

    static void make_inner_brim(const Print& print,
        const ConstPrintObjectPtrs& top_level_objects_with_brim,
        const std::vector<ExPolygons>& bottom_layers_expolygons,
        const ExPolygons& outer_brim_exs,
        ExtrusionEntityCollection& brim)
    {
        assert(print.objects().size() == bottom_layers_expolygons.size());
        const auto                       scaled_resolution = scaled<double>(print.config().gcode_resolution.value);
        const Flow                             flow = print.brim_flow();
        std::vector<InnerBrimExPolygons> inner_brims_ex = inner_brim_area(print, top_level_objects_with_brim, bottom_layers_expolygons, outer_brim_exs, float(flow.scaled_spacing()));
        Polygons                         loops;
        std::mutex                       loops_mutex;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, inner_brims_ex.size()), [&inner_brims_ex, &flow, &scaled_resolution, &loops, &loops_mutex](const tbb::blocked_range<size_t>& range) {
            for (size_t brim_idx = range.begin(); brim_idx < range.end(); ++brim_idx) {
                const InnerBrimExPolygons& inner_brim_ex = inner_brims_ex[brim_idx];
                auto                       num_loops = size_t(floor(inner_brim_ex.brim_width / flow.spacing()));
                ExPolygons                 islands_ex = offset_ex(inner_brim_ex.brim_area, -0.5f * float(flow.scaled_spacing()), ClipperLib::jtSquare);
                for (size_t i = 0; (inner_brim_ex.type == InnerBrimType::INNERMOST ? i < num_loops : !islands_ex.empty()); ++i) {
                    for (ExPolygon& poly_ex : islands_ex)
                        poly_ex.douglas_peucker(scaled_resolution);
                    {
                        boost::lock_guard<std::mutex> lock(loops_mutex);
                        polygons_append(loops, to_polygons(islands_ex));
                    }
                    islands_ex = offset_ex(islands_ex, -float(flow.scaled_spacing()), ClipperLib::jtSquare);
                }
            }
            }); // end of parallel_for
        loops = union_pt_chained_outside_in(loops);
        std::reverse(loops.begin(), loops.end());
        // virgil add for smart_brim_ordering  
        make_inner_brim_smart_ordering(print, loops);
        extrusion_entities_append_loops(brim.entities, std::move(loops), ExtrusionRole::Skirt, float(flow.mm3_per_mm()),
            float(flow.width()), float(print.skirt_first_layer_height()));
    }

    // generate out brim by offsetting ExPolygons "islands_area_ex"
    Polygons tryExPolygonOffset(const ExPolygons islandAreaEx, const Print& print) {
        const auto scaled_resolution = scaled<double>(print.config().resolution.value);
        Polygons   loops;
        ExPolygons islands_ex;
        Flow       flow = print.brim_flow();
        double resolution = 0.0125 / SCALING_FACTOR;
        islands_ex = islandAreaEx;
        for (ExPolygon& poly_ex : islands_ex)
            poly_ex.douglas_peucker(resolution);
        islands_ex = offset_ex(std::move(islands_ex), -0.5f * float(flow.scaled_spacing()), jtRound, resolution);
        for (size_t i = 0; !islands_ex.empty(); ++i) {
            for (ExPolygon& poly_ex : islands_ex)
                poly_ex.douglas_peucker(resolution);
            polygons_append(loops, to_polygons(islands_ex));
            islands_ex = offset_ex(std::move(islands_ex), -1.0f * float(flow.scaled_spacing()), jtRound, resolution);
        }
        return loops;
    }

    // a function create the ExtrusionEntityCollection from the brim area defined by ExPolygons
    ExtrusionEntityCollection makeBrimInfill(const ExPolygons& singleBrimArea,
        const Print& print,
        const Polygons& islands_area,
        std::map<Slic3r::ObjectID, double>& brim_width_map) {
        // output
        ExtrusionEntityCollection brim;
        Polygons      loops = tryExPolygonOffset(singleBrimArea, print);
        Flow flow = print.brim_flow();
        loops = union_pt_chained_outside_in(loops);


        std::vector<Polylines> loops_pl_by_levels;
        {
            Polylines              loops_pl = to_polylines(loops);
            loops_pl_by_levels.assign(loops_pl.size(), Polylines());
            tbb::parallel_for(tbb::blocked_range<size_t>(0, loops_pl.size()),
                [&loops_pl_by_levels, &loops_pl, &islands_area](const tbb::blocked_range<size_t>& range) {
                    for (size_t i = range.begin(); i < range.end(); ++i) {
                        loops_pl_by_levels[i] = chain_polylines(intersection_pl({ std::move(loops_pl[i]) }, islands_area));
                    }
                });
        }

        // Reduce down to the ordered list of polylines.
        Polylines all_loops;
        for (Polylines& polylines : loops_pl_by_levels)
            append(all_loops, std::move(polylines));
        loops_pl_by_levels.clear();

        // Flip orientation of open polylines to minimize travel distance.
        optimize_polylines_by_reversing(&all_loops);

        // virgil add for smart_brim_ordering  

        make_outer_brim_smart_ordering(print, all_loops);

#ifdef BRIM_DEBUG_TO_SVG
        static int irun = 0;
        ++irun;

        {
            SVG svg(debug_out_path("brim-%d.svg", irun).c_str(), get_extents(all_loops));
            svg.draw(union_ex(islands), "blue");
            svg.draw(islands_area_ex, "green");
            svg.draw(all_loops, "black", coord_t(scale_(0.1)));
        }
#endif // BRIM_DEBUG_TO_SVG

        all_loops = connect_brim_lines(std::move(all_loops), offset(singleBrimArea, float(SCALED_EPSILON)), float(flow.scaled_spacing()) * 2.f);



#ifdef BRIM_DEBUG_TO_SVG
        {
            SVG svg(debug_out_path("brim-connected-%d.svg", irun).c_str(), get_extents(all_loops));
            svg.draw(union_ex(islands), "blue");
            svg.draw(islands_area_ex, "green");
            svg.draw(all_loops, "black", coord_t(scale_(0.1)));
        }
#endif // BRIM_DEBUG_TO_SVG

        const bool could_brim_intersects_skirt = std::any_of(print.objects().begin(), print.objects().end(), [&print, &brim_width_map](const PrintObject* object) {
            const BrimType& bt = object->config().brim_type;
            return (bt == btOuterOnly || bt == btOuterAndInner || bt == btAutoBrim) && print.config().skirt_distance.value < brim_width_map[object->id()];
            });

        const bool draft_shield = print.config().draft_shield != dsDisabled;


        // If there is a possibility that brim intersects skirt, go through loops and split those extrusions
        // The result is either the original Polygon or a list of Polylines
        if (draft_shield && !print.skirt().empty() && could_brim_intersects_skirt)
        {
            // Find the bounding polygons of the skirt
            const Polygons skirt_inners = offset(dynamic_cast<ExtrusionLoop*>(print.skirt().entities.back())->polygon(),
                -float(scale_(print.skirt_flow().spacing())) / 2.f,
                ClipperLib::jtRound,
                float(scale_(0.1)));
            const Polygons skirt_outers = offset(dynamic_cast<ExtrusionLoop*>(print.skirt().entities.front())->polygon(),
                float(scale_(print.skirt_flow().spacing())) / 2.f,
                ClipperLib::jtRound,
                float(scale_(0.1)));

            // First calculate the trimming region.
            ClipperLib_Z::Paths trimming;
            {
                ClipperLib_Z::Paths input_subject;
                ClipperLib_Z::Paths input_clip;
                for (const Polygon& poly : skirt_outers) {
                    input_subject.emplace_back();
                    ClipperLib_Z::Path& out = input_subject.back();
                    out.reserve(poly.points.size());
                    for (const Point& pt : poly.points)
                        out.emplace_back(pt.x(), pt.y(), 0);
                }
                for (const Polygon& poly : skirt_inners) {
                    input_clip.emplace_back();
                    ClipperLib_Z::Path& out = input_clip.back();
                    out.reserve(poly.points.size());
                    for (const Point& pt : poly.points)
                        out.emplace_back(pt.x(), pt.y(), 0);
                }
                // init Clipper
                ClipperLib_Z::Clipper clipper;
                // add polygons
                clipper.AddPaths(input_subject, ClipperLib_Z::ptSubject, true);
                clipper.AddPaths(input_clip, ClipperLib_Z::ptClip, true);
                // perform operation
                clipper.Execute(ClipperLib_Z::ctDifference, trimming, ClipperLib_Z::pftNonZero, ClipperLib_Z::pftNonZero);
            }

            // Second, trim the extrusion loops with the trimming regions.
            ClipperLib_Z::Paths loops_trimmed;
            {
                // Produce ClipperLib_Z::Paths from polylines (not necessarily closed).
                ClipperLib_Z::Paths input_clip;
                for (const Polyline& loop_pl : all_loops) {
                    input_clip.emplace_back();
                    ClipperLib_Z::Path& out = input_clip.back();
                    out.reserve(loop_pl.points.size());
                    int64_t loop_idx = &loop_pl - &all_loops.front();
                    for (const Point& pt : loop_pl.points)
                        // The Z coordinate carries index of the source loop.
                        out.emplace_back(pt.x(), pt.y(), loop_idx + 1);
                }
                // init Clipper
                ClipperLib_Z::Clipper clipper;
                clipper.ZFillFunction([](const ClipperLib_Z::IntPoint& e1bot, const ClipperLib_Z::IntPoint& e1top, const ClipperLib_Z::IntPoint& e2bot, const ClipperLib_Z::IntPoint& e2top, ClipperLib_Z::IntPoint& pt) {
                    // Assign a valid input loop identifier. Such an identifier is strictly positive, the next line is safe even in case one side of a segment
                    // hat the Z coordinate not set to the contour coordinate.
                    pt.z() = std::max(std::max(e1bot.z(), e1top.z()), std::max(e2bot.z(), e2top.z()));
                    });
                // add polygons
                clipper.AddPaths(input_clip, ClipperLib_Z::ptSubject, false);
                clipper.AddPaths(trimming, ClipperLib_Z::ptClip, true);
                // perform operation
                ClipperLib_Z::PolyTree loops_trimmed_tree;
                clipper.Execute(ClipperLib_Z::ctDifference, loops_trimmed_tree, ClipperLib_Z::pftNonZero, ClipperLib_Z::pftNonZero);
                ClipperLib_Z::PolyTreeToPaths(std::move(loops_trimmed_tree), loops_trimmed);
            }

            // Third, produce the extrusions, sorted by the source loop indices.
            {
                std::vector<std::pair<const ClipperLib_Z::Path*, size_t>> loops_trimmed_order;
                loops_trimmed_order.reserve(loops_trimmed.size());
                for (const ClipperLib_Z::Path& path : loops_trimmed) {
                    size_t input_idx = 0;
                    for (const ClipperLib_Z::IntPoint& pt : path)
                        if (pt.z() > 0) {
                            input_idx = (size_t)pt.z();
                            break;
                        }
                    assert(input_idx != 0);
                    loops_trimmed_order.emplace_back(&path, input_idx);
                }
                std::stable_sort(loops_trimmed_order.begin(), loops_trimmed_order.end(),
                    [](const std::pair<const ClipperLib_Z::Path*, size_t>& l, const std::pair<const ClipperLib_Z::Path*, size_t>& r) {
                        return l.second < r.second;
                    });

                Point last_pt(0, 0);
                for (size_t i = 0; i < loops_trimmed_order.size();) {
                    // Find all pieces that the initial loop was split into.
                    size_t j = i + 1;
                    for (; j < loops_trimmed_order.size() && loops_trimmed_order[i].second == loops_trimmed_order[j].second; ++j);
                    const ClipperLib_Z::Path& first_path = *loops_trimmed_order[i].first;
                    if (i + 1 == j && first_path.size() > 3 && first_path.front().x() == first_path.back().x() && first_path.front().y() == first_path.back().y()) {
                        auto* loop = new ExtrusionLoop();
                        brim.entities.emplace_back(loop);
                        loop->paths.emplace_back(ExtrusionRole::Skirt, float(flow.mm3_per_mm()), float(flow.width()), float(print.skirt_first_layer_height()));
                        Points& points = loop->paths.front().polyline.points;
                        points.reserve(first_path.size());
                        for (const ClipperLib_Z::IntPoint& pt : first_path)
                            points.emplace_back(coord_t(pt.x()), coord_t(pt.y()));
                        i = j;
                    }
                    else {
                        //FIXME The path chaining here may not be optimal.
                        ExtrusionEntityCollection this_loop_trimmed;
                        this_loop_trimmed.entities.reserve(j - i);
                        for (; i < j; ++i) {
                            this_loop_trimmed.entities.emplace_back(new ExtrusionPath(ExtrusionRole::Skirt, float(flow.mm3_per_mm()), float(flow.width()), float(print.skirt_first_layer_height())));
                            const ClipperLib_Z::Path& path = *loops_trimmed_order[i].first;
                            Points& points = dynamic_cast<ExtrusionPath*>(this_loop_trimmed.entities.back())->polyline.points;
                            points.reserve(path.size());
                            for (const ClipperLib_Z::IntPoint& pt : path)
                                points.emplace_back(coord_t(pt.x()), coord_t(pt.y()));
                        }
                        chain_and_reorder_extrusion_entities(this_loop_trimmed.entities, &last_pt);
                        brim.entities.reserve(brim.entities.size() + this_loop_trimmed.entities.size());
                        append(brim.entities, std::move(this_loop_trimmed.entities));
                        this_loop_trimmed.entities.clear();
                    }
                    last_pt = brim.last_point();
                }
            }
        }
        else {
            extrusion_entities_append_loops_and_paths(brim.entities, std::move(all_loops), ExtrusionRole::Skirt, float(flow.mm3_per_mm()), float(flow.width()), float(print.skirt_first_layer_height()));
        }
        return brim;
    }

    ExtrusionEntityCollection make_brim(const Print& print, PrintTryCancel try_cancel, Polygons& islands_area)
    {
        // output
        ExtrusionEntityCollection brim;
        // Virgil : add for calculate auto_brim_width
        std::map<ObjectID, double> brim_width_map;
        const auto              scaled_resolution = scaled<double>(print.config().gcode_resolution.value);
        Flow                    flow = print.brim_flow();
        std::vector<ExPolygons> bottom_layers_expolygons = get_print_bottom_layers_expolygons(print);
        ConstPrintObjectPtrs    top_level_objects_with_brim = get_top_level_objects_with_brim(print, bottom_layers_expolygons);
        Polygons                islands = top_level_outer_brim_islands(top_level_objects_with_brim, scaled_resolution);
        ExPolygons              islands_area_ex = top_level_outer_brim_area(print, top_level_objects_with_brim, bottom_layers_expolygons, brim_width_map);
        const ExPolygons        outer_brim_exs  = islands_area_ex;
        islands_area                            = to_polygons(islands_area_ex);

        brim = makeBrimInfill(islands_area_ex, print, islands_area, brim_width_map);
        make_inner_brim(print, top_level_objects_with_brim, bottom_layers_expolygons, outer_brim_exs, brim);
        return brim;
    }

} // namespace Slic3r

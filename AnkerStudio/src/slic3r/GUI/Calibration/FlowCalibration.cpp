#include "FlowCalibration.hpp"
#include "CalibrationMaxFlowrateDialog.hpp"
#include "CalibrationPresAdvDialog.hpp"
#include "CalibrationTempDialog.hpp"
#include "CalibrationRetractionDialog.hpp"
#include "CalibrationVfaDialog.hpp"

namespace Slic3r {
    namespace GUI {

        void FlowCalibration::_calib_pa_tower(const Calib_Params& params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/pressure_advance/tower_with_seam.stl").string())}, true, false, false, true);

            auto print_config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto printer_config = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;

            const double nozzle_diameter = printer_config->option<ConfigOptionFloats>("nozzle_diameter")->get_at(0);

            filament_config->set_key_value("slowdown_below_layer_time", new ConfigOptionInts{ 1 });
            wxGetApp().sidebarnew().setCalibrationValue("jerk_print", 1.0f, new ConfigOptionFloat(1.0f));
            wxGetApp().sidebarnew().setCalibrationValue("jerk_outer_wall", 1.0f, new ConfigOptionFloat(1.0f));
            wxGetApp().sidebarnew().setCalibrationValue("jerk_inner_wall", 1.0f, new ConfigOptionFloat(1.0f));


            auto full_config = wxGetApp().preset_bundle->full_config();
            auto wall_speed = CalibPressureAdvance::find_optimal_PA_speed(
                full_config, full_config.get_abs_value("extrusion_width", nozzle_diameter),
                full_config.get_abs_value("layer_height"), 0);

            wxGetApp().sidebarnew().setCalibrationValue("external_perimeter_speed", wall_speed, new ConfigOptionFloatOrPercent(wall_speed, false));
            wxGetApp().sidebarnew().setCalibrationValue("perimeter_speed", wall_speed, new ConfigOptionFloat(wall_speed));

            const auto _wall_generator = print_config->option<ConfigOptionEnum<PerimeterGeneratorType>>("perimeter_generator");
            if (_wall_generator->value == PerimeterGeneratorType::Arachne)
                wxGetApp().sidebarnew().setCalibrationValue("wall_transition_angle", 25.f, new ConfigOptionFloat(25));
            plater->model().objects[0]->config.set_key_value("seam_position", new ConfigOptionEnum<SeamPosition>(spRear));

            plater->changed_objects({ 0 });

            auto new_height = std::ceil((params.end - params.start) / params.step) + 1;
            auto obj_bb = plater->model().objects[0]->bounding_box_approx();
            if (new_height < obj_bb.size().z()) {
                plater->cut_horizontal(0, 0, new_height, ModelObjectCutAttribute::KeepLower);
            }

            plater->_calib_pa_select_added_objects();
        }

        void FlowCalibration::_calib_pa_pattern(const Calib_Params& params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/pressure_advance/cube.stl").string())}, true, false, false, true);

            plater->changed_objects({ 0 });
            plater->_calib_pa_select_added_objects();

            DynamicPrintConfig& printer_config = wxGetApp().preset_bundle->printers.get_edited_preset().config;
            DynamicPrintConfig& print_config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
            double nozzle_diameter = printer_config.option<ConfigOptionFloats>("nozzle_diameter")->get_at(0);
            filament_config->set_key_value("filament_retract_layer_change", new ConfigOptionBoolsNullable{ false });
            filament_config->set_key_value("filament_wipe", new ConfigOptionBoolsNullable{ false });
            printer_config.set_key_value("wipe", new ConfigOptionBools{ false });
            printer_config.set_key_value("retract_layer_change", new ConfigOptionBools{ false });

            auto& model_config = plater->model().objects[0]->config;

            //Orca: find acceleration to use in the test
            auto accel = print_config.option<ConfigOptionFloat>("external_perimeter_acceleration")->value; // get the outer wall acceleration
            if (accel == 0) // if outer wall accel isnt defined, fall back to inner wall accel
                accel = print_config.option<ConfigOptionFloat>("perimeter_acceleration")->value;
            if (accel == 0) // if inner wall accel is not defined fall back to default accel
                accel = print_config.option<ConfigOptionFloat>("default_acceleration")->value;
            // Orca: Set all accelerations except first layer, as the first layer accel doesnt affect the PA test since accel
            // is set to the travel accel before printing the pattern.
            model_config.set_key_value("default_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("external_perimeter_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("perimeter_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("bridge_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("infill_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("solid_infill_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("top_solid_infill_acceleration", new ConfigOptionFloat(accel));
            model_config.set_key_value("travel_acceleration", new ConfigOptionFloat(accel));


            //Orca: find jerk value to use in the test
            if (print_config.option<ConfigOptionFloat>("jerk_print")->value > 0) { // we have set a jerk value
                auto jerk = print_config.option<ConfigOptionFloat>("jerk_outer_wall")->value; // get outer wall jerk
                if (jerk == 0) // if outer wall jerk is not defined, get inner wall jerk
                    jerk = print_config.option<ConfigOptionFloat>("jerk_inner_wall")->value;
                if (jerk == 0) // if inner wall jerk is not defined, get the default jerk
                    jerk = print_config.option<ConfigOptionFloat>("jerk_print")->value;

                //Orca: Set jerk values. Again first layer jerk should not matter as it is reset to the travel jerk before the
                // first PA pattern is printed.
                model_config.set_key_value("jerk_print", new ConfigOptionFloat(jerk));
                model_config.set_key_value("jerk_outer_wall", new ConfigOptionFloat(jerk));
                model_config.set_key_value("jerk_inner_wall", new ConfigOptionFloat(jerk));
                model_config.set_key_value("jerk_top_surface", new ConfigOptionFloat(jerk));
                model_config.set_key_value("jerk_infill", new ConfigOptionFloat(jerk));
                model_config.set_key_value("jerk_travel", new ConfigOptionFloat(jerk));
            }

            model_config.set_key_value("layer_height", new ConfigOptionFloat(nozzle_diameter / 2.0));
            for (const auto opt : SuggestedConfigCalibPAPattern().float_or_percent_pairs) {
                model_config.set_key_value(
                    opt.first,
                    new ConfigOptionFloatOrPercent(opt.second, false)
                );
            }
            model_config.set_key_value(
                "external_perimeter_speed",
                new ConfigOptionFloatOrPercent(CalibPressureAdvance::find_optimal_PA_speed(
                    wxGetApp().preset_bundle->full_config(), (fabs(print_config.get_abs_value("extrusion_width", nozzle_diameter)) <= DBL_EPSILON) ? (nozzle_diameter * 1.125) : print_config.get_abs_value("extrusion_width", nozzle_diameter),
                    print_config.get_abs_value("layer_height"), 0), false));

            for (const auto opt : SuggestedConfigCalibPAPattern().nozzle_ratio_pairs) {
                model_config.set_key_value(
                    opt.first,
                    new ConfigOptionFloatOrPercent(nozzle_diameter * opt.second / 100, false)
                );
            }

            for (const auto opt : SuggestedConfigCalibPAPattern().int_pairs) {
                model_config.set_key_value(
                    opt.first,
                    new ConfigOptionInt(opt.second)
                );
            }

            model_config.set_key_value(
                SuggestedConfigCalibPAPattern().brim_pair.first,
                new ConfigOptionEnum<BrimType>(SuggestedConfigCalibPAPattern().brim_pair.second)
            );

            const DynamicPrintConfig full_config = wxGetApp().preset_bundle->full_config();
            BoundingBox bed_bbx = plater->get_bed_shape();
            BoundingBoxf3 model_bbx = plater->model().bounding_box_approx();
            auto bc = unscale(bed_bbx.center());
            auto mc = model_bbx.center();
            Vec3d plate_origin = Vec3d(0, 0, 0);
            CalibPressureAdvancePattern pa_pattern(
                params,
                full_config,
                false,
                plater->model(),
                plate_origin
            );
            plate_origin = Vec3d(mc.x() - pa_pattern.print_size_x() / 2 - bc.x(), mc.y() - pa_pattern.print_size_y() / 2 - bc.y(), 0);
            pa_pattern.generate_custom_gcodes(
                full_config,
                false,
                plater->model(),
                plate_origin
            );

            plater->changed_objects({ 0 });
        }

        void FlowCalibration::calib_flowrate(int pass)
        {
            if (pass != 1 && pass != 2)
                return;

            auto plater = wxGetApp().plater();
            if (!plater) return;

            if (!plater->new_project())
                return;
            wxGetApp().mainframe->select_tab(size_t(MainFrame::tp3DEditor));

            //GLCanvas3D::set_warning_freeze(true);
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            if (pass == 1)
                std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/filament_flow/flowrate-test-pass1.3mf").string())}, true, false, false, true);
            else
                std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/filament_flow/flowrate-test-pass2.3mf").string())}, true, false, false, true);

            auto print_config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto printerConfig = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;

            /// --- scale ---
            // model is created for a 0.4 nozzle, scale z with nozzle size.
            const ConfigOptionFloats* nozzle_diameter_config = printerConfig->option<ConfigOptionFloats>("nozzle_diameter");
            assert(nozzle_diameter_config->values.size() > 0);
            float nozzle_diameter = nozzle_diameter_config->values[0];
            float xyScale = nozzle_diameter / 0.6;
            //scale z to have 7 layers
            double first_layer_height = print_config->option<ConfigOptionFloatOrPercent>("first_layer_height", false)->value;
            double layer_height = nozzle_diameter / 2.0; // prefer 0.2 layer height for 0.4 nozzle
            first_layer_height = std::max(first_layer_height, layer_height);

            float zscale = (first_layer_height + 6 * layer_height) / 1.4;
            // only enlarge
            if (xyScale > 1.2) {
                for (auto _obj : plater->model().objects)
                    _obj->scale(xyScale, xyScale, zscale);
            }
            else {
                for (auto _obj : plater->model().objects)
                    _obj->scale(1, 1, zscale);
            }

            Flow infill_flow = Flow(nozzle_diameter * 1.2f, layer_height, nozzle_diameter);
            double filament_max_volumetric_speed = filament_config->option<ConfigOptionFloats>("filament_max_volumetric_speed")->get_at(0);
            double max_infill_speed = filament_max_volumetric_speed / (infill_flow.mm3_per_mm() * (pass == 1 ? 1.2 : 1));
            double internal_solid_speed = max_infill_speed > 0 ? std::floor(std::min(print_config->get_abs_value("solid_infill_speed"), max_infill_speed)) : std::floor(print_config->get_abs_value("solid_infill_speed"));
            double top_surface_speed = max_infill_speed > 0 ? std::floor(std::min(print_config->get_abs_value("top_solid_infill_speed"), max_infill_speed)) : std::floor(print_config->get_abs_value("top_solid_infill_speed"));

            // adjust parameters
            for (auto _obj : plater->model().objects) {
                _obj->ensure_on_bed();
                _obj->config.set_key_value("perimeters", new ConfigOptionInt(3));
                _obj->config.set_key_value("top_surface_single_perimeter", new ConfigOptionEnum<SinglePerimeterType>(SinglePerimeterType::TopSurfaces));
                _obj->config.set_key_value("fill_density", new ConfigOptionPercent(35));
                //_obj->config.set_key_value("min_width_top_surface", new ConfigOptionFloatOrPercent(100,true));
                _obj->config.set_key_value("bottom_solid_layers", new ConfigOptionInt(1));
                _obj->config.set_key_value("top_solid_layers", new ConfigOptionInt(5));
                _obj->config.set_key_value("thin_walls", new ConfigOptionBool(true));
                //_obj->config.set_key_value("filter_out_gap_fill", new ConfigOptionFloat(0));
                _obj->config.set_key_value("fill_pattern", new ConfigOptionEnum<InfillPattern>(ipRectilinear));
                _obj->config.set_key_value("top_infill_extrusion_width", new ConfigOptionFloatOrPercent(nozzle_diameter * 1.2f, false));
                _obj->config.set_key_value("solid_infill_extrusion_width", new ConfigOptionFloatOrPercent(nozzle_diameter * 1.2f, false));
                _obj->config.set_key_value("top_fill_pattern", new ConfigOptionEnum<InfillPattern>(ipMonotonic));
                _obj->config.set_key_value("top_infill_flow_ratio", new ConfigOptionFloat(1.0f));
                _obj->config.set_key_value("fill_angle", new ConfigOptionFloat(45));
                _obj->config.set_key_value("ironing", new ConfigOptionBool(false));
                _obj->config.set_key_value("solid_infill_speed", new ConfigOptionFloatOrPercent(internal_solid_speed, false));
                _obj->config.set_key_value("top_solid_infill_speed", new ConfigOptionFloatOrPercent(top_surface_speed, false));

                // extract flowrate from name, filename format: flowrate_xxx
                std::string obj_name = _obj->name;
                assert(obj_name.length() > 9);
                obj_name = obj_name.substr(9);
                if (obj_name[0] == 'm')
                    obj_name[0] = '-';
                auto modifier = stof(obj_name);
                _obj->config.set_key_value("print_flow_ratio", new ConfigOptionFloat(1.0f + modifier / 100.f));
            }

            auto out_value = [&](double value) {
                auto str = wxString::Format(".2f", value);
                double dValue = 0.f;
                bool success = str.ToDouble(&dValue);
                return dValue;
            };

            wxGetApp().sidebarnew().setCalibrationValue("layer_height", out_value(layer_height), new ConfigOptionFloat(out_value(layer_height)));
            wxGetApp().sidebarnew().setCalibrationValue("first_layer_height", out_value(first_layer_height), new ConfigOptionFloatOrPercent(out_value(first_layer_height), false));
            wxGetApp().sidebarnew().setCalibrationValue("avoid_crossing_perimeters", true, new ConfigOptionBool(true));
        }

        void FlowCalibration::calib_temp(const Calib_Params& params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            if (!plater->new_project())
                return;
            wxGetApp().mainframe->select_tab(size_t(MainFrame::tp3DEditor));
            if (params.mode != CalibMode::Calib_Temp_Tower)
                return;

            //GLCanvas3D::set_warning_freeze(true);
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/temperature_tower/temperature_tower.stl").string())}, true, false, false, true);

            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
            auto start_temp = lround(params.start);
            filament_config->set_key_value("first_layer_temperature", new ConfigOptionInts(1, (int)start_temp));
            filament_config->set_key_value("temperature", new ConfigOptionInts(1, (int)start_temp));
            plater->model().objects[0]->config.set_key_value("brim_type", new ConfigOptionEnum<BrimType>(btOuterOnly));
            plater->model().objects[0]->config.set_key_value("brim_width", new ConfigOptionFloat(5.0));
            plater->model().objects[0]->config.set_key_value("brim_separation", new ConfigOptionFloat(0.0));

            plater->changed_objects({ 0 });

            // cut upper
            auto obj_bb = plater->model().objects[0]->bounding_box_approx();
            auto block_count = lround((350 - params.end) / 5 + 1);
            if (block_count > 0) {
                // add EPSILON offset to avoid cutting at the exact location where the flat surface is
                auto new_height = block_count * 10.0 + EPSILON;
                if (new_height < obj_bb.size().z()) {
                    plater->cut_horizontal(0, 0, new_height, ModelObjectCutAttribute::KeepLower);
                }
            }

            // cut bottom
            obj_bb = plater->model().objects[0]->bounding_box_approx();
            block_count = lround((350 - params.start) / 5);
            if (block_count > 0) {
                auto new_height = block_count * 10.0 + EPSILON;
                if (new_height < obj_bb.size().z()) {
                    plater->cut_horizontal(0, 0, new_height, ModelObjectCutAttribute::KeepUpper);
                }
            }
        }

        void FlowCalibration::calib_retraction(const Calib_Params& params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            if (!plater->new_project())
                return;
            wxGetApp().mainframe->select_tab(size_t(MainFrame::tp3DEditor));
            if (params.mode != CalibMode::Calib_Retraction_tower)
                return;

            //GLCanvas3D::set_warning_freeze(true);
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/retraction/retraction_tower.stl").string())}, true, false, false, true);

            auto print_config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
            auto printer_config = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            auto obj = plater->model().objects[0];

            double nozzle_diameter = printer_config->option<ConfigOptionFloats>("nozzle_diameter")->get_at(0);
            double layer_height = nozzle_diameter / 2.0;
            auto max_lh = printer_config->option<ConfigOptionFloats>("max_layer_height");
            if (max_lh->values[0] < layer_height)
                max_lh->values[0] = { layer_height };

            obj->config.set_key_value("perimeters", new ConfigOptionInt(2));
            obj->config.set_key_value("top_solid_layers", new ConfigOptionInt(0));
            obj->config.set_key_value("bottom_solid_layers", new ConfigOptionInt(3));
            obj->config.set_key_value("fill_density", new ConfigOptionPercent(0));
            obj->config.set_key_value("first_layer_height", new ConfigOptionFloatOrPercent(layer_height, false));
            obj->config.set_key_value("layer_height", new ConfigOptionFloat(layer_height));

            plater->changed_objects({ 0 });

            //  cut upper
            auto obj_bb = obj->bounding_box_approx();
            auto height = 1.0 + 0.4 + ((params.end - params.start)) / params.step;
            if (height < obj_bb.size().z()) {
                plater->cut_horizontal(0, 0, height, ModelObjectCutAttribute::KeepLower);
            }
        }

        void FlowCalibration::calib_max_vol_speed(const Calib_Params& params, Calib_Params& out_params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            if (!plater->new_project())
                return;

            //GLCanvas3D::set_warning_freeze(true);
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/volumetric_speed/SpeedTestStructure.stl").string())}, true, false, false, true);

            assert(objs_idx.size() == 1);

            Model& model = plater->model();

            auto print_config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
            auto printer_config = &wxGetApp().preset_bundle->printers.get_edited_preset().config;
            auto obj = model.objects[0];

            auto bed_shape = printer_config->option<ConfigOptionPoints>("bed_shape")->values;
            BoundingBoxf bed_ext = get_extents(bed_shape);
            auto scale_obj = (bed_ext.size().x() - 10) / obj->bounding_box_approx().size().x();
            if (scale_obj < 1.0)
                obj->scale(scale_obj, 1, 1);

            const ConfigOptionFloats* nozzle_diameter_config = printer_config->option<ConfigOptionFloats>("nozzle_diameter");
            assert(nozzle_diameter_config->values.size() > 0);
            double nozzle_diameter = nozzle_diameter_config->values[0];
            double line_width = nozzle_diameter * 1.75;
            double layer_height = nozzle_diameter * 0.8;

            auto max_lh = printer_config->option<ConfigOptionFloats>("max_layer_height");
            if (max_lh->values[0] < layer_height)
                max_lh->values[0] = { layer_height };

            filament_config->set_key_value("filament_max_volumetric_speed", new ConfigOptionFloats{ 200 });
            filament_config->set_key_value("slowdown_below_layer_time", new ConfigOptionInts{ 0 });

            wxGetApp().sidebarnew().setCalibrationValue("enable_dynamic_overhang_speeds", false, new ConfigOptionBool(false));
            wxGetApp().sidebarnew().setCalibrationValue("spiral_vase", true, new ConfigOptionBool(true));
            wxGetApp().sidebarnew().setCalibrationValue("external_perimeter_extrusion_width", line_width, new ConfigOptionFloatOrPercent(line_width, false));
            wxGetApp().sidebarnew().setCalibrationValue("first_layer_height", layer_height, new ConfigOptionFloatOrPercent(layer_height, false));
            wxGetApp().sidebarnew().setCalibrationValue("layer_height", layer_height, new ConfigOptionFloat(layer_height));

            //print_config->set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlTraditional));
            obj->config.set_key_value("perimeters", new ConfigOptionInt(1));
            obj->config.set_key_value("top_solid_layers", new ConfigOptionInt(0));
            obj->config.set_key_value("bottom_solid_layers", new ConfigOptionInt(0));
            obj->config.set_key_value("fill_density", new ConfigOptionPercent(0));
            //print_config->set_key_value("overhang_reverse", new ConfigOptionBool(false));
            obj->config.set_key_value("brim_type", new ConfigOptionEnum<BrimType>(btOuterAndInner));
            obj->config.set_key_value("brim_width", new ConfigOptionFloat(5.0));
            obj->config.set_key_value("brim_separation", new ConfigOptionFloat(0.0));

            plater->changed_objects({ 0 });

            //  cut upper
            auto obj_bb = obj->bounding_box_approx();
            auto height = (params.end - params.start + 1) / params.step;
            if (height < obj_bb.size().z()) {
                plater->cut_horizontal(0, 0, height, ModelObjectCutAttribute::KeepLower);
            }

            out_params = params;
            auto mm3_per_mm = Flow(line_width, layer_height, nozzle_diameter).mm3_per_mm()/* *
                filament_config->option<ConfigOptionFloats>("print_flow_ratio")->get_at(0)*/;
            out_params.end = params.end / mm3_per_mm;
            out_params.start = params.start / mm3_per_mm;
            out_params.step = params.step / mm3_per_mm;
        }

        void FlowCalibration::calib_VFA(const Calib_Params& params)
        {
            auto plater = wxGetApp().plater();
            if (!plater) return;

            if (!plater->new_project())
                return;

            wxGetApp().mainframe->select_tab(size_t(MainFrame::tp3DEditor));
            if (params.mode != CalibMode::Calib_VFA_Tower)
                return;
            //GLCanvas3D::set_warning_freeze(true);
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Load Model"));
            std::vector<size_t> objs_idx = plater->load_files(std::vector<std::string>{
                (boost::filesystem::path(Slic3r::resources_dir() + "/calib/vfa/VFA.stl").string())}, true, false, false, true);

            assert(objs_idx.size() == 1);

            auto print_config = &wxGetApp().preset_bundle->prints.get_edited_preset().config;
            auto filament_config = &wxGetApp().preset_bundle->filaments.get_edited_preset().config;
            filament_config->set_key_value("slowdown_below_layer_time", new ConfigOptionInts{ 0 });
            filament_config->set_key_value("filament_max_volumetric_speed", new ConfigOptionFloats{ 200 });

            wxGetApp().sidebarnew().setCalibrationValue("enable_dynamic_overhang_speeds", false, new ConfigOptionBool(false));
            wxGetApp().sidebarnew().setCalibrationValue("spiral_vase", true, new ConfigOptionBool(true));

            //print_config->set_key_value("timelapse_type", new ConfigOptionEnum<TimelapseType>(tlTraditional));
            plater->model().objects[0]->config.set_key_value("perimeters", new ConfigOptionInt(1));
            plater->model().objects[0]->config.set_key_value("top_solid_layers", new ConfigOptionInt(0));
            plater->model().objects[0]->config.set_key_value("bottom_solid_layers", new ConfigOptionInt(1));
            plater->model().objects[0]->config.set_key_value("fill_density", new ConfigOptionPercent(0));
            //print_config->set_key_value("overhang_reverse", new ConfigOptionBool(false));
           
            plater->model().objects[0]->config.set_key_value("brim_type", new ConfigOptionEnum<BrimType>(btOuterOnly));
            plater->model().objects[0]->config.set_key_value("brim_width", new ConfigOptionFloat(3.0));
            plater->model().objects[0]->config.set_key_value("brim_separation", new ConfigOptionFloat(0.0));

            plater->changed_objects({ 0 });

            // cut upper
            auto obj_bb = plater->model().objects[0]->bounding_box_approx();
            auto height = 5 * ((params.end - params.start) / params.step + 1);
            if (height < obj_bb.size().z()) {
                plater->cut_horizontal(0, 0, height, ModelObjectCutAttribute::KeepLower);
            }
        }
         
        std::vector<t_config_option_key> CalibrationWrapper::get_option_keys(CalibMode model)
        {
            switch (model)
            {
            case Slic3r::CalibMode::Calib_None:
                return {};
                break;
            case Slic3r::CalibMode::Calib_PA_Line:
                return {};
                break;
            case Slic3r::CalibMode::Calib_PA_Pattern:
                return {"default_acceleration", "external_perimeter_acceleration", "perimeter_acceleration", "bridge_acceleration", "infill_acceleration", "solid_infill_acceleration", "top_solid_infill_acceleration", "travel_acceleration", "jerk_print", "jerk_outer_wall", "jerk_inner_wall", "jerk_top_surface", "jerk_infill", "jerk_travel", "layer_height", "external_perimeter_speed"};
                break;
            case Slic3r::CalibMode::Calib_PA_Tower:
                return { "seam_position"};
                break;
            case Slic3r::CalibMode::Calib_Flow_Rate:
                return { "perimeters", "top_surface_single_perimeter", "fill_density", "bottom_solid_layers", "top_solid_layers", "thin_walls", "fill_pattern", "top_infill_extrusion_width", "solid_infill_extrusion_width", "top_fill_pattern", "top_infill_flow_ratio", "fill_angle", "ironing", "solid_infill_speed", "top_solid_infill_speed", "print_flow_ratio" };
                break;
            case Slic3r::CalibMode::Calib_Temp_Tower:
                return { "brim_type", "brim_width", "brim_separation" };
                break;
            case Slic3r::CalibMode::Calib_Vol_speed_Tower:
                return { "perimeters", "top_solid_layers", "bottom_solid_layers", "fill_density", "brim_type", "brim_width", "brim_separation" };
                break;
            case Slic3r::CalibMode::Calib_VFA_Tower:
                return { "perimeters", "top_solid_layers", "bottom_solid_layers", "fill_density", "brim_type", "brim_width", "brim_separation"};
                break;
            case Slic3r::CalibMode::Calib_Retraction_tower:
                return  { "perimeters", "top_solid_layers", "bottom_solid_layers", "fill_density", "first_layer_height", "layer_height"};
                break;
            default:
                return {};
                break;
            }
        }
        
        std::map<t_config_option_key,ConfigOption*> CalibrationWrapper::get_config_options(const std::vector<t_config_option_key>& option_keys,
            Slic3r::ModelConfig* config)
        {
            std::map<t_config_option_key,ConfigOption*> mp;
            if (option_keys.empty() || !config)
                return mp;

            for (auto &key : option_keys) {
                if (config->has(key)) {
                    mp.emplace(key, config->option(key)->clone());
                }
            }

            return std::move(mp);
        }
    }
}



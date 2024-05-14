#ifndef FLOW_CALIBRATION_H
#define FLOW_CALIBRATION_H
#include <map>
#include "slic3r/GUI/Plater.hpp"
namespace Slic3r {
	namespace GUI {
		class FlowCalibration
		{
		public:
			FlowCalibration() = default;
			~FlowCalibration() = default;


			void _calib_pa_pattern(const Calib_Params& params);
			void _calib_pa_tower(const Calib_Params& params);
			void calib_flowrate(int pass);
			void calib_temp(const Calib_Params& params);
			void calib_retraction(const Calib_Params& params);
			void calib_max_vol_speed(const Calib_Params& params, Calib_Params& out_params);
			void calib_VFA(const Calib_Params& params);
		};

		class CalibrationWrapper
		{
		public:
			CalibrationWrapper() = default;
			~CalibrationWrapper() = default;

			static std::vector<t_config_option_key> get_option_keys(CalibMode model);

			static std::map<t_config_option_key,ConfigOption*> get_config_options(const std::vector<t_config_option_key> & option_keys, Slic3r::ModelConfig* config);
		};

	}
}

#endif
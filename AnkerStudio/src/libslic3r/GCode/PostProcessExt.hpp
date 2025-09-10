#ifndef post_Procesee_Ext_hpp_
#define post_Procesee_Ext_hpp_
#define COOL_DOWN_TO_ZERO
#include <iostream>
#include "ProcessCommand.hpp"
#include "UntilString.h"
#include<tbb/tbb.h>

#include <fstream>
#include <string>
#include <algorithm>
#include <oneapi/tbb/parallel_pipeline.h>
#include <oneapi/tbb/concurrent_unordered_map.h>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
using namespace post_gcode;

using namespace std;
using namespace oneapi::tbb;

class PipelineRunner {
public:
	PipelineRunner() {}
	void run(const std::string& input_file, const std::string& output_file, std::vector<std::shared_ptr<process_command>>& commands) {

		boost::filesystem::ifstream infile(input_file, std::ios::binary | std::ios::in);
		boost::filesystem::ofstream outfile(output_file, ios::binary | ios::out | ios::app);
		tbb::tick_count t0 = tbb::tick_count::now();
		std::sort(commands.begin(), commands.end(), [](const std::shared_ptr<process_command>& lhs, const std::shared_ptr<process_command>& rhs) {
			return *lhs < *rhs;
			});
		cmd_map_ = group_the_cmd(commands, block_size);
		tbb::parallel_pipeline(4,
			tbb::make_filter<void, block_info>(tbb::filter_mode::serial_in_order, read_block(infile, cmd_map_, _blk_id)) &
			tbb::make_filter<block_info, std::string>(tbb::filter_mode::parallel, process_block()) &
			tbb::make_filter<std::string, void>(tbb::filter_mode::serial_in_order, write_block(outfile)));

		infile.close();
		outfile.close();
		double serial_time = (tbb::tick_count::now() - t0).seconds();
	}

private:
	constexpr static int block_size = 64 * 1024;
	size_t _blk_id = 0;

	//void cmd_fusion(std::vector<std::shared_ptr<process_command>>& commands) {
	//	for (int i = 1; i < commands.size(); i++) {
	//		auto& last = commands[i-1];
	//	}

	//};

	std::unordered_map<size_t, std::vector< std::shared_ptr<process_command>>> group_the_cmd(const std::vector< std::shared_ptr<process_command>>& cmd, size_t group_size) {

		std::unordered_map<size_t, std::vector<std::shared_ptr<process_command>>> re;

		for (auto _cmd : cmd) {
			auto range = _cmd->get_range();
			size_t block_first_no = static_cast<size_t>((double)range.first / (double)group_size);
			size_t block_second_no = static_cast<size_t>((double)range.second / (double)group_size); // floor
			size_t block_no = block_first_no;
			if (block_first_no != block_second_no) {
				auto split_cmds = _cmd->split(group_size * (block_first_no + 1) - 1);//length between (2 3)
				//split vector
				std::vector<std::shared_ptr<process_command>> firstVector(split_cmds.begin(), split_cmds.end() - 1);
				std::vector<std::shared_ptr<process_command>> lastVector(split_cmds.end() - 1, split_cmds.end());
				if (re.find(block_no) == re.end()) {
					re.emplace(block_no, firstVector);
				}
				else {
					re[block_no].insert(re[block_no].end(), firstVector.begin(), firstVector.end());
				}
				re.emplace(block_no + 1, lastVector);
				continue;
			}

			if (re.find(block_no) == re.end()) {
				re.emplace(block_no, std::vector<std::shared_ptr<process_command>>{ _cmd });
			}
			else {
				re[block_no].push_back(_cmd);
			}
		}
		return re;
	}


	struct block_info
	{
		string text;
		size_t id;
		std::vector< std::shared_ptr<process_command>> p_cmd_set;
	};

	struct read_block {
		ifstream& infile;
		size_t& cur_id;
		std::unordered_map<size_t, std::vector<std::shared_ptr<process_command>>>& _cmd_set;
		read_block(ifstream& _infile, std::unordered_map<size_t, std::vector<std::shared_ptr<process_command>>>& cmd_set, size_t& blk_id) : infile(_infile), _cmd_set(cmd_set), cur_id(blk_id) {}

		block_info operator()(flow_control& fc) const {
			block_info re;
			//static ifstream infile(text, ios::binary | ios::in);
			if (!infile.good()) {
				fc.stop();
				return re;
			}
			char buffer[block_size];
			infile.read(buffer, block_size);
			auto count = infile.gcount();
			if (count == 0) {
				fc.stop();
				return re;
			}
			re.text = string(buffer, buffer + count);
			re.id = cur_id++;
			re.p_cmd_set = _cmd_set[re.id];
			return re;
		}
	};

	struct convert_to_lower {
		string operator()(const block_info& in) const {
			string out(in.text);
			transform(out.begin(), out.end(), out.begin(), ::tolower);
			return out;
		}
	};

	struct process_block {
		string operator()(const block_info& in) const {
			string out(in.text);
			for (const auto& cmd : in.p_cmd_set)
			{
				cmd->load_block(in.id, block_size);
				cmd->execute(out);
			}
			return out;
		}
	};

	struct write_block {
		std::ofstream& outfile;
		write_block(std::ofstream& _outfile) : outfile(_outfile) {}
		void operator()(const string& in) const {
			outfile.write(in.data(), in.size());
		}
	};
	// 
	std::unordered_map<size_t, std::vector<std::shared_ptr<process_command>>> cmd_map_;
};


namespace post_gcode {
	struct linerParam {
		double k;
		double b;
	};

	struct posInfo {
		int extruder_id;
		int index;
	};

	class temperatureManager {
	public:
		temperatureManager(linerParam heat, linerParam cool,
			const std::array<std::vector<size_t>, 6>& toolChangePos,
			const std::array<std::vector<size_t>, 6>& toolChangeTemps,
			const std::vector<Slic3r::GCodeProcessorResult::MoveVertex>& moves,
			const std::vector<size_t>& lines_ends,
			size_t i_extruder
		) :
			heatParam(heat), coolParam(cool), m_toolChangePos(toolChangePos), m_toolChangeTemps(toolChangeTemps), m_lines_ends(lines_ends), init_extruder(i_extruder){
			unsigned int  moves_index = 0;
			for (size_t i = 0; i < lines_ends.size(); i++) {
				if (i < moves[moves_index].gcode_id) {
					m_line_times_cache[i] = moves[moves_index].move_time;
				}
				else {
					moves_index = moves_index + 1 >= moves.size() - 1 ? moves.size() - 1 : moves_index + 1;
					m_line_times_cache[i] = moves[moves_index].move_time;
				}
			}
		}

		void set_target_heat_temp(float t_t0) { t0 = t_t0 - cache_temp; };
		void set_target_cool_temp(float t_t1) { t1 = t_t1 - cache_temp; };
		void set_cache_temp(float t) { cache_temp = t; };

		std::vector<std::shared_ptr<process_command>> run() {
			{
				//pre process get sort extruder map
				//first extruder T0 
				for (int extruder_id = 0; extruder_id < m_toolChangePos.size(); extruder_id++) {
					for (int k = 0; k < m_toolChangePos[extruder_id].size(); k++) {
						m_pos_extruder.emplace(m_toolChangePos[extruder_id].at(k), posInfo{ extruder_id, k });
					}
				}
				std::vector<std::shared_ptr<process_command>> commands;

				//i:extruder index    k:vector index
				for (int i = 0; i < m_toolChangePos.size(); i++) {
					int cur_t = init_temp;
					if (m_toolChangePos[i].size() > 0) {
						size_t fisrt_pos = m_toolChangePos[i].at(0);
						double fisr_time = caltime(fisrt_pos);
						cur_t = calTargetTempExt(fisr_time, m_toolChangeTemps[i].at(0), m_toolChangeTemps[i].at(0));
						cur_t = cur_t < init_temp ? init_temp : cur_t;//FIXME: the first temp need to cal each extruder cool down time
					}
					size_t init_pre_pos = std::numeric_limits<size_t>::max();
					for (int k = 0; k < m_toolChangePos[i].size(); k++) {
						//need clarification:the slicer always keep T0 first. the pos of first t0 don't need to cal temp 

						//gen the heat cmd  where TN added  
						set_target_heat_temp(m_toolChangeTemps[i].at(k));
						set_target_cool_temp(m_toolChangeTemps[i].at(k));
						float heat_time = getTimeFromTg(cur_t);

						//gen the heat cmd  where TN added
						size_t cur_line_id = m_toolChangePos[i].at(k);
						auto cmd_s = genCommandFormHeatTime(heat_time, cur_line_id, i, cur_t); //heat up command
						if (i == init_extruder && k == 0 && cmd_s.size() == 1) {
							//get the init extruder pre heat up pos
						     init_pre_pos = cmd_s[0]->get_range().first;
						}
						for (auto& cmd : cmd_s) {
							commands.push_back(std::move(cmd));
						}

						//find next pos 
						 auto iter_pos_extruder= m_pos_extruder.upper_bound(cur_line_id);
						 if (iter_pos_extruder == m_pos_extruder.end()) {
							 continue;
						 }
						 size_t next_pos = iter_pos_extruder->first;
						 //cal cur extuder to next pos heatup  & cool when cache_temp > 0.0
						 if(cache_temp > 0.0){
							 posInfo cur_cache_pos_info;
							 cur_cache_pos_info.extruder_id = i;
							 cur_cache_pos_info.index = k ;
							 double cache_temp_t = diffTime_cache(*iter_pos_extruder, {cur_line_id,cur_cache_pos_info});
							 float start_temps = m_toolChangeTemps[cur_cache_pos_info.extruder_id].at(max(0, cur_cache_pos_info.index));
							 float start_p_cache_temps = start_temps - cache_temp;
							 //
							 float heat_time_cache = getHeatTime(start_p_cache_temps, start_temps) - heatParam.b + 1; // when heatParam 
							 float cool_time_cache = getCoolTime(start_temps, start_p_cache_temps);
							 float use_time = heat_time_cache + cool_time_cache;

							 if (use_time < cache_temp_t) {
								 //heat up
								 std::string cmd_str = "M104 T" + std::to_string(i) + " S" + std::to_string(int(start_temps)) + ";cache heat up set" + "\n";
								 //auto cmd1 = std::make_shared<append_command>(m_lines_ends[origin_line_id], "M104");
								 size_t offset_h = m_lines_ends[cur_line_id+1];
								 auto cmd2 = std::make_shared<append_command>(offset_h, cmd_str);
								 
								 commands.push_back(std::move(cmd2));

								 //cool down
								 auto cmd_c_c = genCommandFormEarlyTime(cool_time_cache, next_pos,i, start_p_cache_temps);
								 for (auto& cmd : cmd_c_c) {
									 commands.push_back(std::move(cmd));
								 }
							 }
							 else {
								 float start_temps = m_toolChangeTemps[cur_cache_pos_info.extruder_id].at(max(0, cur_cache_pos_info.index));
								 float start_p_cache_temps = start_temps - cache_temp ;
								 float tg = calTargetTempHeatExt(cache_temp_t, start_p_cache_temps, start_p_cache_temps);
								 tg = tg > start_temps ? start_temps : tg;
								 float cool_time = getCoolTime(tg, start_p_cache_temps);

								 //heat up  
								 std::string cmd_str = "M104 T" + std::to_string(i) + " S" + std::to_string(int(tg))+";first set" + "\n";
								 //auto cmd1 = std::make_shared<append_command>(m_lines_ends[origin_line_id], "M104");
								 size_t offset_h = m_lines_ends[cur_line_id + 1];
								 auto cmd2 = std::make_shared<append_command>(offset_h, cmd_str);
								 commands.push_back(std::move(cmd2));

								 if (cache_temp_t - cool_time > 2.0) {
									 //cool down
									 auto cmd_c_c = genCommandFormEarlyTime(cool_time, next_pos, i, start_p_cache_temps);
									 for (auto& cmd : cmd_c_c) {
										 commands.push_back(std::move(cmd));
									 }
								 }

							 }
						 }

						 //cal next pos to next cur extuder heatup  & cool 
						 if (k != m_toolChangePos[i].size() - 1) {
							 //iter_pos_extruder replace last_info
							 size_t cur_pos = m_toolChangePos[i].at(k + 1);
							 posInfo cur_pos_info;
							 cur_pos_info.extruder_id = i;
							 cur_pos_info.index = k + 1;
							 std::pair<size_t, posInfo> cur_info{cur_pos,cur_pos_info};
							 double d_t = diffTime(cur_info, *iter_pos_extruder);
							 float tg = calTargetTemp(d_t);
							 tg = tg < init_temp ? init_temp : tg;
							 tg = min(tg, t1);
							 if (tg < t0) {
								 auto cmd_c = genCommandForCoolingTimeEx(tg, i, next_pos, d_t);
								 cur_t = tg;
								 for (auto& cmd : cmd_c) {
									 commands.push_back(std::move(cmd));
								 }
							 }
						 }
						 else {
							 auto cmd_c = genCommandForCoolingTimeEx(init_temp, i, next_pos, 0);
							 cur_t = init_temp;
							 for (auto& cmd : cmd_c) {
								 commands.push_back(std::move(cmd));
							 }
						 }
					}
					//cool the int_extruder to init_temp
					if (i == init_extruder && !m_pos_extruder.empty()) {
							auto first_pos = m_pos_extruder.begin()->first;
							if (m_lines_ends[first_pos] > init_pre_pos) {
								continue;
							}
							auto cmd_c = genCommandForCoolingTimeEx(init_temp, i, first_pos, 0.0);
							cur_t = init_temp;
							for (auto& cmd : cmd_c) {
								commands.push_back(std::move(cmd));
							}
					}
				}

				return commands;
			}
		};


	private:
		double caltime(int i, int extruder_id) {
			auto& toolpos = m_toolChangePos[extruder_id];
			if (i < toolpos.size()) {
				size_t line_id = 0;
				if (i >= 0) {
					line_id = toolpos[i];
				}
				_line_id_vec.push_back(line_id);
				double re = m_line_times_cache[line_id];
				return re;
			}
			return 0.0;
		};

		double caltime(size_t pos) {
			_line_id_vec.push_back(pos);
			double re = m_line_times_cache[pos];
			return re;
		};

		double diffTime(std::pair<size_t, posInfo> cur_pos_info, std::pair<size_t, posInfo> last_pos_info) {
			_line_id_vec.clear();
			//TODO  t0  t1 always use the same color
			//bugfix: there are a condition that the first value of toolChangePos valide when the slicer layer == 1;		
			size_t end_index = max(cur_pos_info.second.index, 0);
			size_t start_index = max((int)end_index-1, 0);
			set_target_heat_temp(m_toolChangeTemps[cur_pos_info.second.extruder_id].at(start_index));
			set_target_cool_temp(m_toolChangeTemps[cur_pos_info.second.extruder_id].at(end_index));
			return caltime(cur_pos_info.first) - caltime(last_pos_info.first);
		};

		double diffTime_cache(std::pair<size_t, posInfo> cur_pos_info, std::pair<size_t, posInfo> last_pos_info) {
			return caltime(cur_pos_info.first) - caltime(last_pos_info.first);
		}


		float calTargetTemp(double deltaTime) {
			
			float tg = calTargetTempExt(deltaTime,t0,t1);
			return tg;
		};

		float calTargetTempExt(double deltaTime, float start_temp, float end_temp) {
			double k_diff = coolParam.k - heatParam.k;
			float tg = (deltaTime + coolParam.k * start_temp - heatParam.k * end_temp) / k_diff;
			return tg;
		};

		float calTargetTempHeatExt(double deltaTime, float start_temp, float end_temp) {
			double k_diff = heatParam.k - coolParam.k ;
			float tg = (deltaTime + heatParam.k * start_temp - coolParam.k * end_temp) / k_diff;
			return tg;
		};

		float getTimeFromTg(float tg) {
			float heatTime = heatParam.k * (t1 - tg)/* + heatParam.b*/;
			return heatTime;
		};

		float getHeatTime(float start_temp,float target_temp) {
			float heatTime = heatParam.k * (target_temp - start_temp) /*+ heatParam.b*/;
			return heatTime;
		};
		float getCoolTime(float start_temp, float target_temp) {
			float heatTime = coolParam.k * (target_temp - start_temp) /*+ heatParam.b*/;
			return heatTime;
		};


		std::vector<std::shared_ptr<process_command>> genCommandForCoolingTime(int cooling_tg, int extruder_id) {
			std::vector<std::shared_ptr<process_command>> cmds;
			auto minTempPos = std::min_element(_line_id_vec.begin(), _line_id_vec.end());
			if (minTempPos == _line_id_vec.end()) {
				std::cout << "pos empty" << std::endl;
			}
			//cool down cmd
			int temp = static_cast<int>(t1);
#ifdef COOL_DOWN_TO_ZERO
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(0)) + "\n";
#else
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(cooling_tg)) + "\n";
#endif // COOL_DOWN_TO_ZERO

			
			size_t offset = m_lines_ends[*minTempPos];
			auto cmd2 = std::make_shared<append_command>(offset, cmd_str);
			cmds.push_back(std::move(cmd2));
			return cmds;

		};


		std::vector<std::shared_ptr<process_command>> genCommandForCoolingTimeEx(int cooling_tg, int extruder_id, size_t pos,double d_t) {
			std::vector<std::shared_ptr<process_command>> cmds;
			//cool down cmd
#ifdef COOL_DOWN_TO_ZERO
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(0))
				+ ";cool down" + std::to_string(int(d_t)) + "  t0" + std::to_string(int(t0)) + "  t1" + std::to_string(int(t1)) + "\n";
#else
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(cooling_tg)) 
				+ ";cool down" + std::to_string(int(d_t)) + "  t0" + std::to_string(int(t0)) + "  t1" + std::to_string(int(t1)) + "\n";
#endif // COOL_DOWN_TO_ZERO
			
			size_t offset = m_lines_ends[pos];
			auto cmd2 = std::make_shared<append_command>(offset, cmd_str);
			cmds.push_back(std::move(cmd2));
			return cmds;

		};

		std::vector<std::shared_ptr<process_command>> genCommandFormHeatTime(float heatTime, size_t cur_line_id, int extruder_id,int cur_t) {
			size_t  origin_line_id = cur_line_id;
			std::vector<std::shared_ptr<process_command>> cmds;
			float t = heatTime;
			while (heatTime > 0) {
				if (m_line_times_cache[cur_line_id] < heatTime) {
					//TODO：
					return cmds;
				}
				float t = m_line_times_cache[cur_line_id] - m_line_times_cache[cur_line_id - 1];
				heatTime -= t;
				cur_line_id--;
			}
			cur_line_id = cur_line_id > 0 ? cur_line_id : 0;
			size_t offset = m_lines_ends[cur_line_id];
			//heat up cmd
			int temp = static_cast<int>(t1);
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(t1))+";pre heat up" + std::to_string(t)+"from" + std::to_string(cur_t)+"to"+ std::to_string(t1) + "\n";
			//auto cmd1 = std::make_shared<append_command>(m_lines_ends[origin_line_id], "M104");
			auto cmd2 = std::make_shared<append_command>(offset, cmd_str);
			cmds.push_back(std::move(cmd2));
			return cmds;
		};

		//get cmd  from  early time
		std::vector<std::shared_ptr<process_command>> genCommandFormEarlyTime(float early_time, size_t cur_line_id, int extruder_id,float start_temps) {
			size_t  origin_line_id = cur_line_id;
			std::vector<std::shared_ptr<process_command>> cmds;
			while (early_time > 0) {
				if (m_line_times_cache[cur_line_id] < early_time) {
					//TODO：
					return cmds;
				}
				float t = m_line_times_cache[cur_line_id] - m_line_times_cache[cur_line_id - 1];
				early_time -= t;
				cur_line_id--;
			}
			cur_line_id = cur_line_id > 0 ? cur_line_id : 0;
			size_t offset = m_lines_ends[cur_line_id];
			//heat up cmd
			int temp = static_cast<int>(start_temps);
			std::string cmd_str = "M104 T" + std::to_string(extruder_id) + " S" + std::to_string(int(start_temps))+ ";cache cool down set" + "\n";
			//auto cmd1 = std::make_shared<append_command>(m_lines_ends[origin_line_id], "M104");
			auto cmd2 = std::make_shared<append_command>(offset, cmd_str);
			cmds.push_back(std::move(cmd2));
			return cmds;
		};


	private:
		linerParam heatParam;
		linerParam coolParam;
		const std::array<std::vector<size_t>, 6>& m_toolChangePos;// to line_id
		const std::array<std::vector<size_t>, 6>& m_toolChangeTemps;// to line_id
		//const std::vector<unsigned int>& m_g1_line_pos;//line_id ->  g1 line pos
		//std::vector <G1LinesCacheItem>& m_g1_times_cache;
		std::unordered_map<size_t, float> m_line_times_cache;
		std::map<size_t, posInfo> m_pos_extruder; //  pos -> extruder ,size == all of(m_toolChangePos)
		const std::vector<size_t>& m_lines_ends;
		float t0 = 195.0;
		float t1 = 195.0;
		float cache_temp = 0.0;
		int init_temp = 30;
		int  init_extruder;
		std::vector<size_t>  _line_id_vec;
	};
}


//for ai pic
namespace post_gcode {
	using double2SS = PrecisionedDouble;
	using UM2MM = MMtoStream;
	typedef signed long long UM;
	
	class aiPicGenGcode {
	public:
		aiPicGenGcode(const std::vector<Slic3r::GCodeProcessorResult::picPosInfo>& picInfo, const std::vector<size_t>& lines_ends) :m_lines_ends(lines_ends),m_picInfo(picInfo){
		}
		std::vector<std::shared_ptr<process_command>>  run() {
			std::vector<std::shared_ptr<process_command>> cmds;
			for (const auto& pi: m_picInfo) {
				std::stringstream ss;
				std::string new_line = "\n";
				unsigned int layerNumber = pi.layer_id;
				double ms = min(anker_camera_take_picture_time, 30.0);
				int    Z_um = delayF * ms;
				long long cururenZ = static_cast<long long>(pi.Z * 1000);

				ss << "; Z_TakePictureStart: " << layerNumber << new_line;

				int coust = 0;
				bool upAgain = std::find(anker_param_ai_uplayer_array.begin(), anker_param_ai_uplayer_array.end(), layerNumber) != anker_param_ai_uplayer_array.end();
				int snapshot_count = m_picInfo.size() + anker_param_ai_uplayer_array.size() * anker_param_ai_height_array.size();
				genTakePic(upAgain, snapshot_count, pi,ss);
				//generalTakePicture1(upAgain);

				if (upAgain) {
					wipeExtruderCircle(pi, ss);
					for (double height : anker_param_ai_height_array) {
						char layerNumStr[16]{ 0 };
						sprintf(layerNumStr, "%d.%02d", layerNumber, ++coust);
						UM pic_z_offset = static_cast<UM>(height * 1000);

						if ((cururenZ + pic_z_offset + Z_um) >= machine_z)
							pic_z_offset = machine_z - cururenZ - Z_um;

						ss << "G0 F" << double2SS{ 1,    upF * 60 } << " Z" << UM2MM{ cururenZ + pic_z_offset } << new_line;
						ss << "M1024 L" << layerNumStr << "/" << snapshot_count << " 3" << new_line; // (res ? " 3" : " 1")
						ss << ";SUBLAYER:" << layerNumStr << new_line; // writeLayerComment(layer_nr);
						ss << "G0 F" << double2SS{ 1, delayF * 60 } << " Z" << UM2MM{ cururenZ + pic_z_offset + Z_um + (coust * 10) } << new_line;
					}

					wipeExtruderLine(pi,ss);
					recoveryPosition(pi,ss);
				}
				ss << "; Z_TakePictureEnd: " << layerNumber << new_line;
				std::string cmd_str = ss.str();
				size_t offset = m_lines_ends[pi.line_id];
				auto cmd1 = std::make_shared<append_command>(offset, cmd_str);
				cmds.push_back(std::move(cmd1));
			}
			return cmds;
		};
		void set_delay_time(float dt) { anker_camera_take_picture_time = dt; };
		void set_retract_speed(double rs) { retract_speed = rs; };
		void set_travel_speed(double ts) { travel_speed = ts; };
		void set_relative(bool isRelative) { relativeExtrude = isRelative; };
	private:
		const std::vector<Slic3r::GCodeProcessorResult::picPosInfo>& m_picInfo;
		bool relativeExtrude = false;
		//machine size
		float machine_x = 235.0 * 1000;
		float machine_y = 235.0 * 1000;
		float machine_z = 250.0 * 1000;
		std::vector<int> anker_param_ai_uplayer_array = { 1, 2, 3, 4, 5 };
		std::vector<double> anker_param_ai_height_array = { 70, 80, 90, 100, 110, 120 };
		double anker_camera_take_picture_time = 30;
		double retract_speed = 60;
		double retraction = 3;
		double travel_speed = 250;
		double delayF = 1.0;      //  120 mm/min = 2mm/s   um/ms == mm/s
		double upF = 20.0;
		const std::vector<size_t>& m_lines_ends;

	private:
		void genTakePic(bool upAgain, int snapshot_count, const  Slic3r::GCodeProcessorResult::picPosInfo& picInfo, std::stringstream& ss) {
			unsigned int layerNumber = picInfo.layer_id;
			double ms = min(anker_camera_take_picture_time,30.0);
			int    Z_um = delayF * ms;
			double currentF = picInfo.F;
			long long cururenZ = static_cast<long long>(picInfo.Z * 1000);


			std::string new_line = "\n";
			ss << "M1024 L" << layerNumber << "/" << snapshot_count << " 3" << new_line;
			ss << "G0" << " F" << double2SS{ 1,   upF * 60 } << " Z" << UM2MM{ cururenZ + (upAgain ? 0 : Z_um) } << new_line;
			//ss << ";LAYER:" << layerNumber << new_line;
			ss << "G0" << " F" << double2SS{ 1, delayF * 60 } << " Z" << UM2MM{ cururenZ + (upAgain ? Z_um : 0) } << new_line;
			if (!upAgain) ss << "G0" << " F" << double2SS{ 1,   currentF*60 } << new_line;
		}

		void wipeExtruderCircle(const  Slic3r::GCodeProcessorResult::picPosInfo& picInfo, std::stringstream& ss) {
			double currentX = picInfo.X;
			double currentY = picInfo.Y;
			double currentE = picInfo.E;
			std::string new_line = "\n";
			ss << "G1" << " F" << double2SS{ 1, retract_speed * 60 } << " E" << double2SS{ 5, relativeExtrude ? -2 : currentE - 2 } << new_line;   //  -2
			ss << "G3" << " F" << double2SS{ 1, travel_speed * 60 } << " I3 J0 P1" << new_line;
			ss << "G1" << " F" << double2SS{ 1, retract_speed * 60 } << " E" << double2SS{ 5, relativeExtrude ? -1 : currentE - 3 } << new_line;   //  -3
			ss << "G2" << " F" << double2SS{ 1, travel_speed * 60 } << " I3 J0 P1" << new_line;
			ss << "G0" << " X" << double2SS{ 5,  currentX } << " Y" << double2SS{ 5,  currentY } << new_line;
		}

		void wipeExtruderLine(const  Slic3r::GCodeProcessorResult::picPosInfo& picInfo, std::stringstream& ss) {
			unsigned int layerNumber = picInfo.layer_id;
			double currentE = picInfo.E;
			std::string new_line = "\n";
			UM _0X = machine_x - 120000 - (layerNumber * 2 - 1) * 10000; // 220 200, 110 90 70 50 30 10
			UM _1X = machine_x - 120000 - (layerNumber * 2 - 0) * 10000; // 210 190, 100 80 60 40 20
			UM _0Y = machine_y - 2000;
			UM _0Z = 20;

			ss << "G0" << " F" << double2SS{ 1, travel_speed * 60 } << " X" << UM2MM{ _0X } << " Y" << UM2MM{ _0Y } << new_line;
			ss << "G0" << " F" << double2SS{ 1,          upF * 60 } << " Z" << UM2MM{ _0Z } << new_line;
			ss << "G3" << " I2 J0 P2" << " E" << double2SS{ 5, relativeExtrude ? 3 : currentE } << new_line;    //  0
			ss << "G0" << " X" << UM2MM{ _1X } << " Y" << UM2MM{ _0Y } << " E" << double2SS{ 5, relativeExtrude ? 0 : currentE + retraction } << new_line;
			ss << "G1" << " F" << double2SS{ 1, retract_speed * 60 } << " E" << double2SS{ 5, relativeExtrude ? -1 : currentE + retraction - 1 } << new_line;   //  -0.5
			ss << "G2" << " I-2 J0" << new_line << "G2" << " I-2 J0" << new_line << "G2" << " I-2 J0" << new_line;    //
		}


		void recoveryPosition(const  Slic3r::GCodeProcessorResult::picPosInfo& picInfo, std::stringstream& ss) {
			std::string new_line = "\n";
			double currentF = picInfo.F;
			double currentX = picInfo.X;
			double currentY = picInfo.Y;
			double currentZ = picInfo.Z;
			ss << "G0 F" << double2SS{ 1,           upF * 60 } << " Z" << double2SS{5, currentZ} << new_line;
			ss << "G0 F" << double2SS{ 1,  travel_speed * 60 } << " X" << double2SS{5, currentX } << " Y" << double2SS{ 5,currentY} << new_line;
			ss << "G1 F" << double2SS{ 1, retract_speed * 60 } << " E" << double2SS{ 5, relativeExtrude ? 1.35 : picInfo.E + retraction + 1.35 } << new_line;
			ss << "G0 F" << double2SS{ 1,          currentF*60 } << new_line;
		}



	};


}


namespace post_gcode {
	struct NozzleInfo {
		unsigned int EID;
		std::string Color;
		std::string MID;
		std::string CID;
		std::string filamentType;
	};

	struct startCuraInfo {
		float time;
		std::string filamentName;
		std::string MachineName;
		std::string profileName;
		std::string printMode;
		std::string adhesion_type;
		double nozzleSize;
		double filamentUsed;
		double filamentWeight;
		double layerHeight;
		int firstLayerTemp;
		int speedInfill;
		int speedWall;
		int speedWallInner;
		int acceleration;
		int accelerationTravel;
		int perimeter_flow_ratio;
		float MINX;
		float MINY;
		float MINZ;
		float MAXX;
		float MAXY;
		float MAXZ;
		float MAXSPEED;
		bool multi_model;
		bool support_enable;
		bool damaged;
		bool repaired;
	};

	class genBBox {
	public:
		genBBox(const std::vector<size_t>& lines_ends, const std::array<float, 6>& bbox) :
			m_lines_ends(lines_ends), boundBox(bbox) {};

		std::vector<std::shared_ptr<process_command>>& run() {
			size_t offset = m_lines_ends[pos];
			std::string cmd_str = "";
			cmd_str += _genBBox();
			cmd_str += "\n";
			auto cmd_ = std::make_shared<append_command>(m_lines_ends[pos], cmd_str);
			res.push_back(std::move(cmd_));
			return res;
		}
	private:
		std::string _genBBox() {
			std::string genToolChangeTimeStr;

			boost::format _nzfmt(";MINX:%1%\n"
				";MINY:%2%\n"
				";MINZ:%3%\n"
				";MAXX:%4%\n"
				";MAXY:%5%\n"
				";MAXZ:%6%\n");
			_nzfmt% boundBox[0] % boundBox[1] % boundBox[2] % boundBox[3] % boundBox[4] % boundBox[5];
			genToolChangeTimeStr += _nzfmt.str();
			return genToolChangeTimeStr;
		};

	private:
		const std::array<float, 6>& boundBox;
		std::vector<std::shared_ptr<process_command>> res;
		const std::vector<size_t>& m_lines_ends;
		size_t pos = 1;
	};

	class ToolChangeTime {
	public:
		ToolChangeTime(const std::array<int, 36>& nozzleCf,float printTime, const std::vector<size_t>& lines_ends , const std::array<float, 6>& bbox):
			nozzle_change_info(nozzleCf), m_lines_ends(lines_ends), _printTime(printTime), boundBox(bbox){
		};
		std::vector<std::shared_ptr<process_command>>& run() {
			size_t offset = m_lines_ends[pos];
			std::string cmd_str = "";				
			cmd_str += "\n";
			cmd_str += genNozzleChangeInfo();
			cmd_str += "\n";
			cmd_str += genNozzleChangetime();
			cmd_str += "\n";
			cmd_str += genToolChangeTime();
			cmd_str += "\n";
			cmd_str += genAllTime();
			cmd_str += "\n";
			cmd_str += genBBox();
			cmd_str += "\n";
			auto cmd_toolChangeTime = std::make_shared<append_command>(offset, cmd_str);
			res.push_back(std::move(cmd_toolChangeTime));
			return res;
		}
	private:
		std::string genNozzleChangeInfo() {
			std::string nozzleChangeInfoStr;
			nozzleChangeInfoStr = ";NozzleChangeInfo:0"; //the first (i=0,j=0) always 0
			for (int i = 1; i < nozzle_change_info.size(); i++) {
				boost::format nzfmt(",%1%");
				nzfmt% nozzle_change_info[i];
				nozzleChangeInfoStr += nzfmt.str();
			}
			return nozzleChangeInfoStr;
		};

		std::string genNozzleChangetime() {
			std::string genNozzleChangetimeStr;
			genNozzleChangetimeStr = ";NozzleChangeTime:";
			boost::format _nzfmt("%1%");
			_nzfmt% nozzle_change_info[0];
			genNozzleChangetimeStr += _nzfmt.str();
			for (int i = 1; i < nozzle_change_info.size(); i++) {
				boost::format nzfmt(",%1%");
				nzfmt% nozzle_change_time[i];
				genNozzleChangetimeStr += nzfmt.str();
			}
			return genNozzleChangetimeStr;
		};

		std::string genToolChangeTime() {
			std::string genToolChangeTimeStr;
			genToolChangeTimeStr = ";ToolChangeTime:";
			toolChangeTime = 0.0;
			for (int i = 0; i < 36;i++) {
				toolChangeTime += nozzle_change_info[i] * nozzle_change_time[i];
			}
			boost::format _nzfmt("%1%");
			_nzfmt% toolChangeTime;
			genToolChangeTimeStr += _nzfmt.str();
			return genToolChangeTimeStr;
		};

		std::string genAllTime() {
			std::string genToolChangeTimeStr;
			double allTime = toolChangeTime + _printTime;

			boost::format _nzfmt(";TIME:%1%s");
			_nzfmt% allTime;
			genToolChangeTimeStr += _nzfmt.str();
			return genToolChangeTimeStr;
		};
		std::string genBBox() {
			std::string genToolChangeTimeStr;

			boost::format _nzfmt(";MINX:%1%\n"
				";MINY:%2%\n"
				";MINZ:%3%\n"
				";MAXX:%4%\n"
				";MAXY:%5%\n"
				";MAXZ:%6%\n");
			_nzfmt% boundBox[0]% boundBox[1]%boundBox[2]%boundBox[3]%boundBox[4]% boundBox[5];
			genToolChangeTimeStr += _nzfmt.str();
			return genToolChangeTimeStr;
		};
	private:
		std::vector<std::shared_ptr<process_command>> res;
		double toolChangeTime = 0.0;
		double _printTime;
		const std::array<int, 36>& nozzle_change_info;
		const std::array<float, 6>& boundBox;
		const std::array<float, 36> nozzle_change_time{ 0.0 ,3.0 ,6.0 ,9.0 ,12.0 ,15.0 ,
													   4.0 ,0.0 ,3.0 ,6.0 ,9.0 ,12.0 ,
													   7.0 ,4.0 ,0.0 ,3.0 ,6.0 ,9.0 ,
													   10.0 ,7.0 ,4.0 ,0.0, 3.0 ,6.0 ,
													   13.0 ,10.0 ,7.0 ,4.0 ,0.0 ,3.0 ,
													   15.0 ,13.0 ,10.0 ,7.0 ,10.0 ,0.0 };
		const std::vector<size_t>& m_lines_ends;
		size_t pos = 1;
	};

	class startGCodeGen {
	public:
		startGCodeGen(const std::vector<size_t>& lines_ends, std::vector<NozzleInfo>& nInfo,startCuraInfo& sCuraInfo,bool isV6 = true) :m_lines_ends(lines_ends), extuderInfos(nInfo), mStartCuraInfo(sCuraInfo), printModeV6(isV6) {
		};
		std::vector<std::shared_ptr<process_command>>& run() {
						
			size_t offset = m_lines_ends[pos];
			std::string cmd_str = "";
			cmd_str += genNozzleMap();
			cmd_str += genCuraHead();
			auto cmd1 = std::make_shared<append_command>(offset, cmd_str);
			res.push_back(std::move(cmd1));
			//add anker MakeVerion in end
			boost::format end_nzfmt(";AnkerMake version: V%1%");
			end_nzfmt% std::string(SLIC3R_VERSION);
			std::string cmd_str_end = end_nzfmt.str();
			size_t offset_end = m_lines_ends.back();
			auto cmd2 = std::make_shared<append_command>(offset_end, cmd_str_end);
			res.push_back(std::move(cmd2));
			return res;
		};

	private:
		std::string  genNozzleMap() {
			std::string nozzleMapStr;
			if (printModeV6) {
				nozzleMapStr = ";NozzleColorMap:";
				for (const auto& info : extuderInfos) {
					boost::format nzfmt("T%1% C=0x%2% MID=%3% CID=%4%,");
					nzfmt% info.EID% info.Color.substr(1) % info.MID% info.CID;
					nozzleMapStr += nzfmt.str();
				}
				nozzleMapStr += "\n";
			}
			nozzleMapStr = "; filament_type = ";
			for (int i = 0; i < extuderInfos.size(); i++) {
				if (i < extuderInfos.size() - 1) {
					nozzleMapStr += "\"" + extuderInfos[i].filamentType + "\";";
				}
				else {
					nozzleMapStr += "\"" + extuderInfos[i].filamentType + "\"\n";
				}
			}

			return nozzleMapStr;
		};

		std::string genCuraHead() {
			std::string curaHead;
			mStartCuraInfo.printMode.find("Normal") ;
			boost::format fmt(
				";Filament Name:%2%\n"
				";Machine Name:%22%\n"
				";Profile Name:%15%\n"
				";multi_model:%23%\n"
				";support_enable:%24%\n"
				";damaged:%25%\n"
				";repaired:%26%\n"
				";adhesion_type:%27%\n"
				";perimeter_flow_ratio:%28%\n"
				";Machine Nozzle Size:%3%\n"
				";Print Mode:%4%\n"
				";Filament weight: %6%g\n"
				";Filament used: %5%mm\n"
				";Layer height: %7%\n"
				";material_print_temperature_layer_0: %16%\n"
				";speed_infill: %17%\n"
				";speed_wall_0: %18%\n"
				";speed_wall_x: %19%\n"
				";acceleration_print: %20%\n"
				";acceleration_travel: %21%\n"
				";MAXSPEED:%14%\n");

			fmt% mStartCuraInfo.time
				% mStartCuraInfo.filamentName
				% mStartCuraInfo.nozzleSize
				% mStartCuraInfo.printMode
				% mStartCuraInfo.filamentUsed
				% mStartCuraInfo.filamentWeight
				% mStartCuraInfo.layerHeight
				% mStartCuraInfo.MINX
				% mStartCuraInfo.MINY
				% mStartCuraInfo.MINZ
				% mStartCuraInfo.MAXX
				% mStartCuraInfo.MAXY
				% mStartCuraInfo.MAXZ
				% mStartCuraInfo.MAXSPEED
				% mStartCuraInfo.profileName
				% mStartCuraInfo.firstLayerTemp
				% mStartCuraInfo.speedInfill
				% mStartCuraInfo.speedWall
				% mStartCuraInfo.speedWallInner
				% mStartCuraInfo.acceleration
				% mStartCuraInfo.accelerationTravel
				% mStartCuraInfo.MachineName
				% (mStartCuraInfo.multi_model ? "true" : "false")
				% (mStartCuraInfo.support_enable ? "true" : "false")
				% (mStartCuraInfo.damaged ? "true" : "false")
				% (mStartCuraInfo.repaired ? "true" : "false")
				% (mStartCuraInfo.adhesion_type.empty() ? "none" : mStartCuraInfo.adhesion_type)
				% mStartCuraInfo.perimeter_flow_ratio;
			curaHead = fmt.str();
			if (!printModeV6) {
				boost::format _nzfmt(";TIME:%1%s\n");
				_nzfmt% mStartCuraInfo.time;
				curaHead += _nzfmt.str();
			}
			return curaHead;
		}

	private:
		std::vector<std::shared_ptr<process_command>> res;
		std::vector<NozzleInfo>& extuderInfos;
		startCuraInfo& mStartCuraInfo ;
		const std::vector<size_t>& m_lines_ends;
		size_t pos = 1;
		bool printModeV6;
	};
}

#endif
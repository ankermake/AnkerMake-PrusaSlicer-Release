#ifndef Procesee_Command_hpp_
#define Procesee_Command_hpp_
#include <mutex>
#include <unordered_map>
#include <cassert>

namespace post_gcode {

	class process_command {
	public:
		using range_type = std::pair<size_t, size_t>;
		process_command(size_t start, size_t end) : start_(start), end_(end) {}
		virtual ~process_command() {
			std::lock_guard<std::mutex> lock(mutex_);
			count_[id_]--;
			if (count_[id_] == 0) {
				offset[id_] = 0;
			}
		};
		virtual void execute(std::string& str) = 0;
		virtual std::vector<std::shared_ptr<process_command>> split(size_t split_pos) = 0;
		virtual int changed_num() = 0;

		range_type get_range() const {
			return { start_, end_ };
		}

		void load_block(size_t id, size_t stride) {
			std::lock_guard<std::mutex> lock(mutex_);
			id_ = id;
			if (count_.find(id) == count_.end()) {
				count_.emplace(id, 1);
			}
			else {
				count_[id]++;
			}
			long long cur_c_n = 0;
			long long c_n = changed_num();
			if (offset.find(id) == offset.end()) {
				offset.emplace(id, c_n);
			}
			else {
				cur_c_n = offset[id];
				offset[id] += c_n;
			}

			long long block_pos = id * stride;
			update_range(-block_pos + cur_c_n);
			is_load_block = true;
		}

		bool has_load_block() const {
			return is_load_block;
		}

		static std::unordered_map<size_t, long long> offset;
		bool operator<(const process_command& rhs) const {
			std::pair<size_t, size_t> lhs_p = this->get_range();
			std::pair<size_t, size_t> rhs_p = rhs.get_range();
			return lhs_p.first < rhs_p.first;
		}
	private:
		void update_range(long long block_pos) {
			start_ += block_pos;
			end_ += block_pos;
		}
		static std::mutex mutex_;

	protected:
		size_t start_;
		size_t end_;
		size_t id_ = 0;
		bool is_load_block = false;
		static std::unordered_map<size_t, size_t> count_;

	};

	std::unordered_map<size_t, long long> post_gcode::process_command::offset;
	std::unordered_map<size_t, size_t> post_gcode::process_command::count_;
	std::mutex process_command::mutex_;

	class append_command : public process_command {
	public:
		append_command(size_t start, const std::string& append_str) : process_command(start, start), str_(append_str) {}
		void execute(std::string& str) override { str.insert(start_, str_); }
		std::vector<std::shared_ptr<process_command>> split(size_t split_pos) override {
			std::vector<std::shared_ptr<process_command>> result;
			return result;
		}
		int changed_num() override { return str_.size(); }

	private:
		std::string str_;
	};

	class delete_command : public process_command {
	public:
		delete_command(size_t start, size_t end) : process_command(start, end) {}
		void execute(std::string& str) override { str.erase(start_, end_ - start_); }
		std::vector<std::shared_ptr<process_command>> split(size_t split_pos) override {
			std::vector<std::shared_ptr<process_command>> result;
			if (end_ > split_pos) {
				result.push_back(std::make_shared<delete_command>(start_, split_pos));
				result.push_back(std::make_shared<delete_command>(split_pos + 1, end_));
			}
			return result;
		}
		int changed_num() override { return start_ - end_; }
	};

	class update_command :public process_command {
	public:
		update_command(size_t start, size_t end, const std::string& update_str) :process_command(start, end), str_(update_str) {}
		void execute(std::string& str) override { str.replace(start_, end_ - start_, str_); }
		std::vector<std::shared_ptr<process_command>> split(size_t split_pos) override {
			std::vector<std::shared_ptr<process_command>> result;
			if (end_ > split_pos) {
				result.push_back(std::make_shared<append_command>(split_pos, str_));
				result.push_back(std::make_shared<delete_command>(start_, split_pos));
				result.push_back(std::make_shared<delete_command>(split_pos + 1, end_));
			}
			return result;
		}
		int changed_num() override {
			return start_ - end_ + str_.size();
		}

	private:
		std::string str_;
	};


}

#endif
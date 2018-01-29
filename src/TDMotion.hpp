/*
 * TouchDesigner motion data : data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include <ostream>
#include <sstream>
#include <vector>

class TDMotion {
public:
	typedef float frameval_t;

	struct Channel {
		std::string name;
		std::vector<frameval_t> values;
		frameval_t minVal;
		frameval_t maxVal;

		Channel(std::string chanName);

		std::string node_name() const;
		std::string channel_name() const;
		std::string node_path() const;

		uint32_t length() const { return (uint32_t)values.size(); }
		bool is_const() const { return minVal == maxVal; }
	};

	static const int32_t NONE = -1;
	struct XformGrp {
		union {
			struct {
				int32_t rx, ry, rz;
				int32_t tx, ty, tz;
				int32_t sx, sy, sz;
				int32_t xOrd;
				int32_t rOrd;
			};
			int32_t idx[11] = {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE};
		};
	};

	class XformGrpFunc {
	public:
		virtual void operator ()(std::string& name, XformGrp& grpInfo) {}
	};

protected:
	std::vector<Channel> mChannels;

	void parse_chan_row(std::istringstream& ss, bool hasNames);
	Channel& add_chan(const std::string& chanName);
	std::string new_chan_name() const;

public:
	TDMotion() = default;

	bool load(const std::string& filePath, bool hasNames, bool columnChans);

	uint32_t get_chan_num() const { return (uint32_t)mChannels.size(); }
	const std::vector<Channel>& get_channels() const { return mChannels; }
	const Channel* get_chan(int32_t idx) const {
		if ((idx < 0) || (idx > mChannels.size())) {
			return nullptr;
		}
		return &mChannels[idx];
	}

	int32_t find_chan_idx(const std::string& nodeName, const std::string&) const;
	Channel& find_chan(const std::string& nodeName, const std::string&) const;

	bool find_channels(const std::string pattern, std::vector<int32_t>& foundChans) const;
	void find_xforms(XformGrpFunc& func, const std::string& path = "") const;

	bool dump_clip(std::ostream& os) const;
	void save(const std::string& path) const;

	friend std::ostream& operator << (std::ostream& os, TDMotion& mot) {
		mot.dump_clip(os);
		return os;
	}
};

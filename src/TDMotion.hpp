/*
 * TouchDesigner motion data : data loading, evaluation and conversion
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

		Channel(std::string chanName = "");

		std::string node_name() const;
		std::string channel_name() const;
		std::string node_path() const;

		unsigned length() const { return (unsigned)values.size(); }
		bool is_const() const { return minVal == maxVal; }

		frameval_t get_val(int frameNo) const {
			int len = length();
			int fno = frameNo % len;
			fno = fno < 0 ? fno + len : fno;
			return values[fno];
		}

		frameval_t eval(float frame) const;

		void clear() {}
	};

	enum class AnimChan : std::uint8_t {
		RX = 0,
		RY = 1,
		RZ = 2,
		TX = 3,
		TY = 4,
		TZ = 5,
		SX = 6,
		SY = 7,
		SZ = 8,
		XORD = 9,
		RORD = 10
	};

	static const size_t NONE = (size_t)-1;
	struct XformGrp {
		union {
			struct {
				size_t rx, ry, rz;
				size_t tx, ty, tz;
				size_t sx, sy, sz;
				size_t xOrd;
				size_t rOrd;
			};
			struct {
				size_t r[3];
				size_t t[3];
				size_t s[3];
				size_t ord[2];
			};
			size_t idx[11];
		};
		XformGrp() { std::fill_n(idx, 11, NONE); }

		bool has_rotation() const { return rx != NONE || ry != NONE || rz != NONE; }
		bool has_translation() const { return tx != NONE || ty != NONE || tz != NONE; }
		bool has_scale() const { return sx != NONE || sy != NONE || sz != NONE; }
		bool has_xord() const { return xOrd != NONE; }
		bool has_rord() const { return rOrd != NONE; }
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
	void unload();

	size_t get_chan_num() const { return mChannels.size(); }
	// In TouchDesigner all motion channels have the same length
	size_t length() const {
		return get_chan_num() ? mChannels[0].length() : 0;
	}

	const std::vector<Channel>& get_channels() const { return mChannels; }
	const Channel* get_channel(size_t idx) const {
		return idx > mChannels.size() ? nullptr : &mChannels[idx];
	}

	bool find_channels(const std::string& pattern, std::vector<size_t>& foundChans) const;
	void find_xforms(XformGrpFunc& func, const std::string& path = "") const;

	frameval_t get_val(size_t chIdx, int fno) const {
		const Channel* pChan = get_channel(chIdx);
		return pChan == nullptr ? (frameval_t)0.0f : pChan->get_val(fno);
	}
	frameval_t eval(size_t chIdx, float frame) const {
		const Channel* pChan = get_channel(chIdx);
		return pChan == nullptr ? (frameval_t)0.0f : pChan->eval(frame);
	}

	bool dump_clip(std::ostream& os) const;
	void save_clip(const std::string& path) const;

	friend std::ostream& operator << (std::ostream& os, TDMotion& mot) {
		mot.dump_clip(os);
		return os;
	}
};

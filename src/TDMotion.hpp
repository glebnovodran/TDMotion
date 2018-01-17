/*
 * TouchDesigner motion data : data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include <ostream>
#include <sstream>
#include <vector>

class cTDMotion {
public:
	typedef float frameval_t;

	struct sTrack {
		//std::string path;
		std::string name;
		std::vector<frameval_t> values;
		frameval_t minVal;
		frameval_t maxVal;

		sTrack(std::string trackName);

		std::string short_name() const;
		std::string channel_name() const;
		std::string node_path() const;

		uint32_t length() const { return (uint32_t)values.size(); }
		bool is_const() const { return minVal == maxVal; }
	};

	static const int32_t NONE = -1;
	struct sXformGrp {
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

	typedef void(*XformGroupCallback) (std::string& name, sXformGrp& grpInfo);

protected:
	std::vector<sTrack> mTracks;

	void parse_track_row(std::istringstream& ss, bool hasNames);
	sTrack& add_track(const std::string& trackName);
	std::string new_track_name() const;

public:
	cTDMotion();

	bool load(const std::string& filePath, bool hasNames, bool columnTracks);

	uint32_t get_track_num() const { return (uint32_t)mTracks.size(); }

	int32_t find_track_idx(const std::string& nodeName, const std::string&) const;
	sTrack& find_track(const std::string& nodeName, const std::string&) const;

	bool find_tracks(const std::string pattern, std::vector<uint32_t>& foundTracks) const;
	void find_xforms(XformGroupCallback callback, const std::string& path = "") const;

	bool dump_clip(std::ostream& os) const;
	void save(const std::string& path) const;

	friend std::ostream& operator << (std::ostream& os, cTDMotion& mot) {
		mot.dump_clip(os);
		return os;
	}
};

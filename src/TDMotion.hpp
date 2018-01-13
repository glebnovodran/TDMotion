/*
 * TouchDesigner motion data : data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include <ostream>
#include <sstream>
#include <vector>

typedef float frameval_t;

class cTDMotion {
public:
	struct sTrack {
		std::string name;
		std::vector<frameval_t> values;

		sTrack(std::string trackName) {
			name = trackName;
		}

		std::string short_name();
		std::string channel_name();
	};
protected:
	std::vector<sTrack> mTracks;

	void parse_track_row(std::istringstream& ss, bool hasNames);
	sTrack& add_track(const std::string& trackName);
	std::string new_track_name() const;
public:
	cTDMotion();

	bool load(const std::string& filePath, bool hasNames, bool columnTracks);

	uint32_t find_track_idx(const std::string& nodeName, const std::string&) const;
	sTrack& find_track(const std::string& nodeName, const std::string&) const;

	uint32_t get_track_num() const { return (uint32_t)mTracks.size(); }
	bool dump_clip(std::ostream& os) const;
	bool save(const std::string& path) const;
};

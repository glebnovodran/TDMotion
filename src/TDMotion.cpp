/*
 * TouchDesigner motion data : data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include "TDMotion.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <iostream>
#include <map>

static const char* TRACK_NAME_PREFIX = "track";

cTDMotion::cTDMotion() {
}

cTDMotion::sTrack::sTrack(std::string trackName) {
	name = trackName;
	minVal = std::numeric_limits<frameval_t>::max();
	maxVal = std::numeric_limits<frameval_t>::min();
}

std::string cTDMotion::sTrack::short_name() const {
	using namespace std;
	int32_t slashIdx = name.find_last_of('/');
	int32_t colonIdx = name.find_last_of(':');
	if ((slashIdx != string::npos) || (colonIdx != string::npos)) {
		slashIdx = slashIdx == string::npos ? 0 : slashIdx + 1;
		colonIdx = colonIdx == string::npos ? name.size() : colonIdx;
	}
	string res = name.substr(slashIdx, colonIdx - slashIdx);
	return res;
}

std::string cTDMotion::sTrack::channel_name() const {
	using namespace std;
	int32_t colonIdx = name.find_last_of(':');
	if (colonIdx == string::npos) { return ""; }
	string res = name.substr(colonIdx+1, name.size() - colonIdx);
	return res;
}

std::string cTDMotion::sTrack::node_path() const {
	using namespace std;
	int32_t colonIdx = name.find_last_of(':');
	if (colonIdx == string::npos) { return name; }
	return name.substr(0, colonIdx);
}

std::string cTDMotion::new_track_name() const {
	uint32_t nextId = mTracks.size();
	std::string trackName = TRACK_NAME_PREFIX + std::to_string(nextId);
	return trackName;
}

cTDMotion::sTrack& cTDMotion::add_track(const std::string& trackName) {
	mTracks.emplace_back(trackName);
	return mTracks.back();
}

void cTDMotion::parse_track_row(std::istringstream& ss, bool hasNames) {
	using namespace std;
	string trackName;
	if (hasNames) {
		getline(ss, trackName, '\t');
	} else {
		trackName = new_track_name();
	}

	sTrack& trk = add_track(trackName);
	frameval_t val;
	while (ss >> val) {
		trk.values.push_back(val);
	}
}

bool cTDMotion::load(const std::string& filePath, bool hasNames, bool columnTracks) {
	using namespace std;

	string row;
	ifstream is(filePath);
	
	if (!is.good()) { return false; }

	mTracks.clear();
	uint32_t nrow = 0;

	if (columnTracks) {
		while (getline(is, row)) {
			istringstream ss(row);
			frameval_t val;

			if (nrow == 0) {
				if (hasNames) {
					string trackName;
					while (ss >> trackName) {
						mTracks.emplace_back(trackName);
					}
				} else {
					while (ss >> val) {
						mTracks.emplace_back(new_track_name());
						mTracks.back().values.push_back(val);
					}
				}
			} else {
				uint32_t trkIdx = 0;
				while (ss >> val) {
					sTrack& track = mTracks[trkIdx];
					track.values.push_back(val);
					if (track.maxVal < val) { track.maxVal = val; }
					if (track.minVal > val) { track.minVal = val; }
					++trkIdx;
				}
			}

			++nrow;
		}
	} else {
		while (getline(is, row)) {
			istringstream ss(row);
			parse_track_row(ss, hasNames);
		}
	}

	return true;
}

bool cTDMotion::find_tracks(const std::string pattern, std::vector<uint32_t>& foundTracks) const {
	using namespace std;
	regex reg(pattern, regex::ECMAScript | regex::icase);
	uint32_t trkIdx = 0;
	for (auto& track : mTracks) {
		if (regex_match(track.name, reg)) {
			foundTracks.push_back(trkIdx);
		}
		++trkIdx;
	}

	return foundTracks.size() > 0;
}

void cTDMotion::find_xforms(XformGroupCallback callback, const std::string& path) const {
	using namespace std;
	static map<string, int32_t sXformGrp::*> chMap = {
		{ "rx", &sXformGrp::rx },
		{ "ry", &sXformGrp::ry },
		{ "rz", &sXformGrp::rz },
		{ "tx", &sXformGrp::tx },
		{ "ty", &sXformGrp::ty },
		{ "tz", &sXformGrp::tz },
		{ "sx", &sXformGrp::sx },
		{ "sy", &sXformGrp::sy },
		{ "sz", &sXformGrp::sz },
		{ "xOrd", &sXformGrp::xOrd },
		{ "rOrd", &sXformGrp::rOrd }
	};

	vector<bool> processed(mTracks.size());
	fill(processed.begin(), processed.end(), false);

	int32_t i = 0;
	size_t szPath = path.size();
	int32_t cycles = 0;
	for (auto& trackI : mTracks) {
		sXformGrp grp;
		if (!processed[i]) {
			if (0 == trackI.name.compare(0, szPath, path)) {
				string name = trackI.node_path();
				string chName = trackI.channel_name();
				grp.*chMap[chName] = i;
				processed[i] = true;

				int32_t j = 0;
				for (auto& trackJ : mTracks) {
					if (!processed[j]) {
						if (0 == name.compare(trackJ.node_path())) {
							string chName = trackJ.channel_name();
							grp.*chMap[chName] = j;
							processed[j] = true;
						}
					}
					++cycles;
					++j;
				}
				(*callback)(name, grp);
			} else {
				++cycles;
				processed[i] = true;
			}
		}
		++i;
	}
}

bool cTDMotion::dump_clip(std::ostream& os) const {
	using namespace std;

	if (!os.good()) { return false; }

	os << "{" << endl;
	os << "\trate = 30" << endl;
	os << "\tstart = -1" << endl;
	os << "\ttracklength = " << mTracks[0].length() << endl;
	os << "\ttracks = " << mTracks.size() << endl;

	for (auto& track : mTracks) {
		os << "   {" << endl;
		os << "      name = " << track.name << endl;
		os << "      data =" ;
		for (auto val : track.values) {
			os << " " << val;
		}
		os << endl;
		os << "   }" << endl;
	}
	os << "}" << endl;
	return true;
}

void cTDMotion::save(const std::string& path) const {
	using namespace std;
	ofstream os(path);
	dump_clip(os);
	os.close();
}

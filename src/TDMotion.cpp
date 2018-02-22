/*
 * TouchDesigner motion data : data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <iostream>
#include <map>

#include "TDMotion.hpp"

static const char* CHAN_NAME_PREFIX = "chan";

TDMotion::Channel::Channel(std::string chanName) {
	name = chanName;
	minVal = std::numeric_limits<frameval_t>::max();
	maxVal = std::numeric_limits<frameval_t>::lowest();
}

std::string TDMotion::Channel::node_name() const {
	using namespace std;
	size_t slashIdx = name.find_last_of('/');
	size_t colonIdx = name.find_last_of(':');
	if ((slashIdx != string::npos) || (colonIdx != string::npos)) {
		slashIdx = slashIdx == string::npos ? 0 : slashIdx + 1;
		colonIdx = colonIdx == string::npos ? name.size() : colonIdx;
	}
	string res = name.substr(slashIdx, colonIdx - slashIdx);
	return res;
}

std::string TDMotion::Channel::channel_name() const {
	using namespace std;
	size_t colonIdx = name.find_last_of(':');
	if (colonIdx == string::npos) { return ""; }
	string res = name.substr(colonIdx+1, name.size() - colonIdx);
	return res;
}

std::string TDMotion::Channel::node_path() const {
	using namespace std;
	size_t colonIdx = name.find_last_of(':');
	if (colonIdx == string::npos) { return name; }
	return name.substr(0, colonIdx);
}

TDMotion::frameval_t TDMotion::Channel::eval(float frame) const {
	float len = (float)length();
	float maxFrame = len - 1.0f;

	float f = ::fmodf(frame, len);
	f = f < 0? f + len : f;
	float fstart = ::truncf(f);
	float bias = f - fstart;

	int32_t istart = (int32_t)fstart;
	int32_t iend = (istart == maxFrame) ? 0 : istart + 1;

	frameval_t a = values[istart];
	frameval_t b = values[iend];
	return ::fma(b - a, bias, a);
}

std::string TDMotion::new_chan_name() const {
	size_t nextId = mChannels.size();
	std::string chanName = CHAN_NAME_PREFIX + std::to_string(nextId);
	return chanName;
}

TDMotion::Channel& TDMotion::add_chan(const std::string& chanName) {
	mChannels.emplace_back(chanName);
	return mChannels.back();
}

void TDMotion::parse_chan_row(std::istringstream& ss, bool hasNames) {
	using namespace std;
	string chanName;
	if (hasNames) {
		getline(ss, chanName, '\t');
	} else {
		chanName = new_chan_name();
	}

	Channel& chan = add_chan(chanName);
	frameval_t val;
	while (ss >> val) {
		chan.values.push_back(val);
	}
}

bool TDMotion::load(const std::string& filePath, bool hasNames, bool columnChans) {
	using namespace std;

	string row;
	ifstream is(filePath);
	
	if (!is.good()) { return false; }

	mChannels.clear();
	uint32_t nrow = 0;

	if (columnChans) {
		while (getline(is, row)) {
			istringstream ss(row);
			frameval_t val;

			if (nrow == 0) {
				if (hasNames) {
					string chanName;
					while (ss >> chanName) {
						mChannels.emplace_back(chanName);
					}
				} else {
					while (ss >> val) {
						mChannels.emplace_back(new_chan_name());
						mChannels.back().values.push_back(val);
					}
				}
			} else {
				uint32_t chanIdx = 0;
				while (ss >> val) {
					Channel& chan = mChannels[chanIdx];
					chan.values.push_back(val);
					if (chan.maxVal < val) { chan.maxVal = val; }
					if (chan.minVal > val) { chan.minVal = val; }
					++chanIdx;
				}
			}

			++nrow;
		}
	} else {
		while (getline(is, row)) {
			istringstream ss(row);
			parse_chan_row(ss, hasNames);
		}
	}

	return true;
}

void TDMotion::unload() {
	mChannels.clear();
	std::vector<Channel>().swap(mChannels);
}

bool TDMotion::find_channels(const std::string& pattern, std::vector<size_t>& foundChans) const {
	using namespace std;
	regex reg(pattern, regex::ECMAScript | regex::icase);
	int32_t chanIdx = 0;
	for (const auto& chan : mChannels) {
		if (regex_match(chan.name, reg)) {
			foundChans.push_back(chanIdx);
		}
		++chanIdx;
	}

	return foundChans.size() > 0;
}

void TDMotion::find_xforms(XformGrpFunc& func, const std::string& path) const {
	using namespace std;
	static map<string, size_t XformGrp::*> chMap = {
		{ "rx", &XformGrp::rx },
		{ "ry", &XformGrp::ry },
		{ "rz", &XformGrp::rz },
		{ "tx", &XformGrp::tx },
		{ "ty", &XformGrp::ty },
		{ "tz", &XformGrp::tz },
		{ "sx", &XformGrp::sx },
		{ "sy", &XformGrp::sy },
		{ "sz", &XformGrp::sz },
		{ "xOrd", &XformGrp::xOrd },
		{ "rOrd", &XformGrp::rOrd }
	};

	vector<bool> processed(mChannels.size());
	fill(processed.begin(), processed.end(), false);

	size_t i = 0;
	size_t szPath = path.size();
	int32_t cycles = 0;
	for (const auto& chanI : mChannels) {
		XformGrp grp;
		if (!processed[i]) {
			if (0 == chanI.name.compare(0, szPath, path)) {
				string name = chanI.node_path();
				string chName = chanI.channel_name();
				grp.*chMap[chName] = i;
				processed[i] = true;

				size_t j = 0;
				for (const auto& chanJ : mChannels) {
					if (!processed[j]) {
						if (0 == name.compare(chanJ.node_path())) {
							string chName = chanJ.channel_name();
							grp.*chMap[chName] = j;
							processed[j] = true;
						}
					}
					++cycles;
					++j;
				}
				func(name, grp);
			} else {
				++cycles;
				processed[i] = true;
			}
		}
		++i;
	}
}

bool TDMotion::dump_clip(std::ostream& os) const {
	using namespace std;

	if (!os.good()) { return false; }

	os << "{" << endl;
	os << "\trate = 30" << endl;
	os << "\tstart = -1" << endl;
	os << "\ttracklength = " << mChannels[0].length() << endl;
	os << "\ttracks = " << mChannels.size() << endl;

	for (const auto& chan : mChannels) {
		os << "   {" << endl;
		os << "      name = " << chan.name << endl;
		os << "      data =" ;
		for (auto val : chan.values) {
			os << " " << val;
		}
		os << endl;
		os << "   }" << endl;
	}
	os << "}" << endl;
	return true;
}

void TDMotion::save_clip(const std::string& path) const {
	using namespace std;
	ofstream os(path);
	dump_clip(os);
	os.close();
}

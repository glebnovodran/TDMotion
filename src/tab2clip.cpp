/*
* TouchDesigner motion data : test conversion program
* Author: Gleb Novodran <novodran@gmail.com>
*/

#include <vector>
#include <iostream>
#include <fstream>

#include <regex>

#include "TDMotion.hpp"

using namespace std;

void showHelp() {
	cout << "Usage:" << endl;
}

void onGroupFound(std::string& name, cTDMotion::sXformGrp& grpInfo) {
	cout << name << endl;
	for (auto i : grpInfo.idx) {
		if (i == cTDMotion::NONE) { continue; }
		cout << i << " ";
	}
	cout << endl;
}

int main(int argc, char* argv[]) {
	cTDMotion mot;

	if (argc == 2) {
		if (mot.load(argv[1], true, false)) {

			vector<uint32_t> foundIds;
			if (mot.find_tracks("/obj/n_Move:t[x|y|z]", foundIds)) {
				cout << "Found " << endl;
			} else {
				cout << "Nothing's found" << endl;
			}

			mot.find_xforms(&onGroupFound, "/obj");

			mot.save("out.clip");
		}
	}

	return 0;
}

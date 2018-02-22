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

class PrintFunc : public TDMotion::XformGrpFunc {
public:
	virtual void operator ()(std::string& name, TDMotion::XformGrp& grpInfo) {
		cout << name << endl;
		for (auto i : grpInfo.idx) {
			if (i == TDMotion::NONE) { continue; }
			cout << i << " ";
		}
		cout << endl;
	}
};

int main(int argc, char* argv[]) {
	TDMotion mot;

	if (argc == 2) {
		if (mot.load(argv[1], true, false)) {
			PrintFunc grpFunc;
			vector<size_t> foundIds;
			if (mot.find_channels("/obj/n_Move:t[x|y|z]", foundIds)) {
				cout << "Found " << endl;
			} else {
				cout << "Nothing's found" << endl;
			}

			mot.find_xforms(grpFunc, "/obj");

			mot.save_clip("out.clip");

			float val = mot.eval(0, -25.0f);
			float val1 = mot.get_val(0, -25);

			mot.unload();
			mot.load(argv[1], true, false);
			mot.unload();
		}
	}

	return 0;
}

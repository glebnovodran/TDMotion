/*
* TouchDesigner motion data : test conversion program
* Author: Gleb Novodran <novodran@gmail.com>
*/

#include <vector>
#include <iostream>
#include <fstream>

#include "TDMotion.hpp"

using namespace std;

void showHelp() {
	cout << "Usage:" << endl;
}

int main(int argc, char* argv[]) {
	cTDMotion mot;

	if (argc == 2) {
		mot.load(argv[1], true, true);
	}

	return 0;
}

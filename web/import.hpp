#pragma once
#include "common.hpp"
#include <fstream>
#include <stdlib.h>
#include <dirent.h>

namespace cses {

class Import {
public:
	Import();
	~Import();

	void process(std::istream &zipData);
	
	vector<string> tasks();
	
// 	map<string,vector<pair<string,string>>> inputs;
// 	map<string,vector<pair<string,string>>> outputs;


private:
};

}
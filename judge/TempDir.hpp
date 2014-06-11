#pragma once
#include "gen-cpp/Judge.h"
#include <vector>

namespace cses {

struct TempDir {
	TempDir();
	~TempDir();
	const std::string& getName() const { return name; }
	void saveContents(const std::string& subdir, protocol::RunResult& res);
	void hardlinkInputs(const std::vector<protocol::FileRef>& inputs);

private:
	std::string name;
};

}

#pragma once
#include "gen-cpp/Judge.h"

namespace cses {

struct TempDir {
	TempDir();
	~TempDir();
	const std::string& getName() const { return name; }
	void saveContents(const std::string& subdir, protocol::RunResult& res);

private:
	std::string name;
};

}

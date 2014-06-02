#include "common.hpp"
#include "gen-cpp/Judge.h"

namespace cses {
void runDocker(
	protocol::RunResult& _return,
	const string& imageRepository,
	const string& imageID,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options);
}

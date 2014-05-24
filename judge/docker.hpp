#pragma once
#include "common.hpp"

namespace cses {

// Validates all input.
// Throws protocol exceptions on errors that should be propagated back to
// client.
unordered_map<string, string> runInDocker(
	const string& imageRepository,
	const string& imageID,
	const unordered_map<string, string>& input,
	double timeLimit,
	uint64_t memoryLimit
);

}

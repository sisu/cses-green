#pragma once
#include "common.hpp"
#include "gen-cpp/Judge.h"
namespace cses {
void runPTrace(
	protocol::RunResult& _return,
	const protocol::PTraceConfig& config,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options);
}

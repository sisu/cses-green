#include "TempDir.hpp"
#include "common/common.hpp"
#include "common/file.hpp"
#include "gen-cpp/Judge.h"
#include <cmath>

namespace {

template <typename T>
T withMsg(const std::string& msg) {
	T ret;
	ret.msg = msg;
	return ret;
}

}

namespace cses {

void runPTrace(
	protocol::RunResult& _return,
	const protocol::PTraceConfig& config,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options
) {
	TempDir inputDir;
	TempDir outputDir;

	// TODO: work correctly when running from different directory
	using protocol::SyscallPolicy;
	SyscallPolicy::type policy = config.policy;
	string type = policy == SyscallPolicy::NO_RESTRICT ? "NONE"
		: policy == SyscallPolicy::PTRACE ? "PTRACE"
		: policy == SyscallPolicy::SECCOMP ? "SECCOMP"
		: throw withMsg<protocol::InvalidDataError>("Unknown syscall restrict policy");
	char buf[1024];
	long long spaceKiB = 4096;
	sprintf(buf, "sudo ./run_boxed.sh %d %lld %lld ./restrict_syscalls -type \"%s\" -allowed \"%s\"",
		int(ceil(options.timeLimit)),
		options.memoryLimitBytes / 1024LL,
		spaceKiB,
		type.c_str(),
		config.allowedSyscalls.c_str());
	int res = system(buf);
	if(res == -1) {
		throw Error("runCommand: Calling system failed.");
	}
	int retval = WEXITSTATUS(res);
	if (retval != 0) {
		_return.type = protocol::RunResultType::NONZERO_EXIT_STATUS;
		return;
	}

	outputDir.saveContents("", _return);
}

}

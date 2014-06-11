#include "run_ptrace.hpp"
#include "TempDir.hpp"
#include "common/common.hpp"
#include "common/file.hpp"
#include "gen-cpp/Judge.h"
#include <cmath>

namespace {
using namespace std;

template <typename T>
T withMsg(const std::string& msg) {
	T ret;
	ret.msg = msg;
	return ret;
}

const string RESTRICT_SYSCALLS = "restrict_syscalls";
const string RUN_BOXED = "run_boxed.sh";

}

namespace cses {

void runPTrace(
	protocol::RunResult& _return,
	const protocol::PTraceConfig& config,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options
) {
	// TODO: work correctly when running from different directory
	string programPath = "syscalls";

	TempDir inputDir;
	TempDir outputDir;

	inputDir.hardlinkInputs(inputs);

	using protocol::SyscallPolicy;
	SyscallPolicy::type policy = config.policy;
	string type = policy == SyscallPolicy::NO_RESTRICT ? "NONE"
		: policy == SyscallPolicy::PTRACE ? "PTRACE"
		: policy == SyscallPolicy::SECCOMP ? "SECCOMP"
		: throw withMsg<protocol::InvalidDataError>("Unknown syscall restrict policy");
	char buf[4096];
	long long spaceKiB = 4096;
	string runScript = getFileStoragePath(config.runnerHash);
	sprintf(buf, "sudo %s/%s %d %lld %lld %s/%s -type \"%s\" -allowed \"%s\" \"%s\"",
		programPath.c_str(),
		RUN_BOXED.c_str(),
		int(ceil(options.timeLimit)),
		options.memoryLimitBytes / 1024LL,
		spaceKiB,
		programPath.c_str(),
		RESTRICT_SYSCALLS.c_str(),
		type.c_str(),
		config.allowedSyscalls.c_str(),
		runScript.c_str());
	cerr<<"Running: "<<buf<<'\n';
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

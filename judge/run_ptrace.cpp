#include "run_ptrace.hpp"
#include "TempDir.hpp"
#include "common/common.hpp"
#include "common/file.hpp"
#include "gen-cpp/Judge.h"
#include <cmath>
#include <unistd.h>

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
// TODO: work correctly when running from different directory
string programPath = "syscalls";

void pathPermissions(string path, string per) {
	while(path.size()>1) {
		system(("chmod " + per + " " + path).c_str());
		size_t idx = path.find_last_of('/');
		if (idx == string::npos) break;
		path = path.substr(0, idx);
	}
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

	inputDir.hardlinkInputs(inputs);

	// FIXME: this is not thread safe
	cerr<<"Calling setenv "<<inputDir.getName()<<' '<<outputDir.getName()<<'\n';
	setenv("IN", inputDir.getName().c_str(), true);
	setenv("OUT", outputDir.getName().c_str(), true);
	string runSafe = programPath + "/" + RESTRICT_SYSCALLS;
	setenv("RUNSAFE", runSafe.c_str(), true);
	setenv("ALLOWED_SYSCALLS", config.allowedSyscalls.c_str(), true);

	using protocol::SyscallPolicy;
	SyscallPolicy::type policy = config.policy;
	string type = policy == SyscallPolicy::NO_RESTRICT ? "NONE"
		: policy == SyscallPolicy::PTRACE ? "PTRACE"
		: policy == SyscallPolicy::SECCOMP ? "SECCOMP"
		: throw withMsg<protocol::InvalidDataError>("Unknown syscall restrict policy");
	char buf[4096];
	long long spaceKiB = 4096;
	string runScript = getFileStoragePath(config.runnerHash);
	cerr<<"runscript "<<runScript<<'\n';
	link(runScript.c_str(), (inputDir.getName() + "/__run").c_str());
//	sprintf(buf, "sudo %s/%s %d %lld %lld %s/%s -type \"%s\" -allowed \"%s\" \"%s\"",
	sprintf(buf, "sudo -E -u judgerun %s/%s %d %lld %lld \"%s/__run\"",
		programPath.c_str(),
		RUN_BOXED.c_str(),
		int(ceil(options.timeLimit)),
		options.memoryLimitBytes / 1024LL,
		spaceKiB,
		inputDir.getName().c_str());
	cerr<<"Setting permissions for "<<inputDir.getName()<<" "<<outputDir.getName()<<'\n';
	pathPermissions(inputDir.getName(), "777");
	system(("chmod -R 777 " + inputDir.getName()).c_str());
	pathPermissions(outputDir.getName(), "777");
	system(("chmod -R 777 " + outputDir.getName()).c_str());
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

	cerr<<"Saving results\n";
	system(("sudo chmod -R 777 " + outputDir.getName()).c_str());
	outputDir.saveContents("", _return);
}

}

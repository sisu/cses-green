#include "gen-cpp/Judge.h"
#include "judging.hpp"
#include "common/file.hpp"
#include "common/io_util.hpp"
#include "model.hpp"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <algorithm>
#include <unordered_map>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

namespace {
using namespace cses;

typedef std::unordered_map<string,string> StringMap;

struct JudgeConnection {
	JudgeHost host;
	const shared_ptr<protocol::JudgeClient> client;
	string token = "uolevi";

	JudgeConnection(JudgeHost host): host(host), client(new protocol::JudgeClient(makeProtocol(host.host, host.port))) {
	}

	StringMap runOnJudge(Sandbox sandbox, const StringMap& inputs, double timeLimit, int memoryLimit) {
		cerr<<"running on judge "<<host.name<<' '<<inputs.size()<<'\n';
		vector<protocol::FileRef> fileRefs;
		for(const auto& i: inputs) {
			cerr<<"input "<<i.first<<' '<<i.second<<'\n';
			if (!client->hasFile(token, i.second)) {
				cerr<<"sending input\n";
				client->sendFile(token, readFileByHash(i.second));
			}
			protocol::FileRef ref;
			ref.hash = i.second;
			ref.name = i.first;
			fileRefs.push_back(move(ref));
		}
		protocol::RunOptions options;
		options.timeLimit = timeLimit;
		options.memoryLimitBytes = memoryLimit;
		protocol::RunResult result;
		cerr<<"calling run\n";
		client->run(result, token, makeSandbox(sandbox), fileRefs, options);
		cerr<<"return from run\n";
		StringMap map;
		for(protocol::FileRef outFile: result.outputs) {
			if (!fileHashExists(outFile.hash)) {
				string data;
				client->getFile(data, token, outFile.hash);
				FileSave save;
				save.write(&data[0], data.size());
				save.save();
			}
			map[outFile.name] = outFile.hash;
			cerr<<' '<<outFile.name;
		}
		cerr<<'\n';
		return map;
	}

	bool operator<(const JudgeConnection& c) const {
		if (host.name != c.host.name) return host.name < c.host.name;
		if (host.host != c.host.host) return host.host < c.host.host;
		return host.port < c.host.port;
	}

private:
	cses::protocol::Sandbox makeSandbox(Sandbox sandbox) {
		cses::protocol::Sandbox res;
		cerr<<"sandbox type "<<sandbox.type<<'\n';
		switch(sandbox.type) {
			case Sandbox::DOCKER:
				{
					cses::protocol::DockerImage d;
					d.repository = sandbox.docker.getRepositoryName();
					d.id = sandbox.docker.getImageID();
					res.__set_docker(d);
				}
				break;
			case Sandbox::PTRACE:
				{
					if (!sandbox.ptrace.runner) throw Error("Missing runner.");
					cses::protocol::PTraceConfig p;
					p.policy = cses::protocol::SyscallPolicy::SECCOMP;
					p.allowedSyscalls = sandbox.ptrace.allowedSyscalls;
					p.runnerHash = sandbox.ptrace.runner.hash;
					res.__set_ptrace(p);
					if (!client->hasFile(token, p.runnerHash)) {
						client->sendFile(token, readFileByHash(p.runnerHash));
					}
				}
				break;
		}
		return res;
	}

	static boost::shared_ptr<apache::thrift::protocol::TProtocol> makeProtocol(string host, int port) {
		using namespace apache::thrift;
		using namespace apache::thrift::protocol;
		using namespace apache::thrift::transport;

		boost::shared_ptr<TTransport> socket(new TSocket(host, port));
		boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
		transport->open();
		return protocol;
	}
};

class JudgeMaster;
struct ReturnConnection {
	JudgeMaster& master;
	JudgeConnection& connection;
	~ReturnConnection();
};

template<class...X>
struct PrintError;

template<>
struct PrintError<> { static void printError(const ::apache::thrift::TException&) {} };

template<class T, class...X>
struct PrintError<T,X...> {
	static void printError(const ::apache::thrift::TException& e) {
		try {
			const T& t = dynamic_cast<const T&>(e);
			cerr<<"Exception from judge: "<<typeid(t).name()<<" : "<<t.msg<<'\n';
		} catch(const std::bad_cast& b) {
			PrintError<X...>::printError(e);
		}
	}
};
template<class...X>
void printErrorForTypes(const ::apache::thrift::TException& e) {
	PrintError<X...>::printError(e);
}

class UnitTask {
public:
	virtual ~UnitTask() {}

	void execute(JudgeConnection connection, JudgeMaster& master) {
		unique_ptr<UnitTask> self(this);
		ReturnConnection ret{master, connection};
		(void)ret;
		this->connection = &connection;
		this->master = &master;
		try {
			run();
		} catch(const ::apache::thrift::TException& e) {
			namespace P = protocol;
			printErrorForTypes<P::InternalError, P::InvalidDataError, P::AuthError, P::DockerError>(e);
		} catch(const std::exception& e) {
			cerr<<"Internal judging exception "<<e.what()<<'\n';
		} catch(...) {
			cerr<<"Unknown judging exception\n";
		}
	}

	JudgeConnection* connection;
	JudgeMaster* master;
	virtual void run() = 0;

private:
};

class JudgeMaster {
public:
	static JudgeMaster& instance() {
		static JudgeMaster master;
		static bool initDone;
		if (!initDone) {
			initDone=1;
		}
		return master;
	}

	void judgeLoop() {
		while(1) {
			auto lock = getLock();
			cerr<<"checking for tasks and judges.\n";
			condition.wait(lock);
			startJudgings();
		}
	}

	void addTask(UnitTask* task) {
		auto lock = getLock();
		pendingTasks.push_back(task);
		condition.notify_one();
	}

	void updateJudgeHosts() {
		std::vector<JudgeHost> hosts;
		{
			odb::transaction t(db::begin());
			odb::result<JudgeHost> result = db::query<JudgeHost>();
			hosts.assign(result.begin(), result.end());
		}
		allJudgeHosts.clear();
		for(JudgeHost host: hosts) {
			std::thread(&JudgeMaster::connectToJudgeHostLoop, this, host).detach();
		}
	}

	void addConnectedJudgeHost(JudgeConnection conn) {
		auto lock = getLock();
		allJudgeHosts.insert(JudgeConnection(conn));
		condition.notify_one();
	}

	void returnConnection(JudgeConnection conn) {
		usedJudgeHosts.erase(conn);
	}

private:
	JudgeMaster(): mainThread(&JudgeMaster::judgeLoop, this) {}

	std::thread mainThread;

	std::unique_lock<std::mutex> getLock() {
		return std::unique_lock<std::mutex>(mutex);
	}

	void startJudgings() {
		std::vector<JudgeConnection> freeHosts;
		std::set_difference(allJudgeHosts.begin(), allJudgeHosts.end(), usedJudgeHosts.begin(), usedJudgeHosts.end(), std::back_inserter(freeHosts));
		while(!freeHosts.empty() && !pendingTasks.empty()) {
			cerr<<"Starting tasks\n";
			UnitTask* task = pendingTasks.front();
			pendingTasks.pop_front();
			JudgeConnection host = freeHosts.back();
			freeHosts.pop_back();
			cerr<<"starting on host "<<host.host.name<<'\n';
			std::thread(&UnitTask::execute, task, host, std::ref(*this)).detach();
		}
	}

	void connectToJudgeHostLoop(JudgeHost host) {
		while(1) {
			try {
				addConnectedJudgeHost(JudgeConnection(host));
				cerr<<"Connected to jugehost "<<host.name<<'\n';
				return;
			} catch(const apache::thrift::transport::TTransportException& e) {
//				cerr<<"Connecting to "<<host.name<<" failed: "<<e.what()<<'\n';
				sleep(5);
			}
		}
	}

	std::condition_variable condition;
	std::mutex mutex;

	std::deque<UnitTask*> pendingTasks;

	std::set<JudgeConnection> usedJudgeHosts;
	std::set<JudgeConnection> allJudgeHosts;
};

ReturnConnection::~ReturnConnection() {
	master.returnConnection(connection);
}

class RunTestGroup: public UnitTask {
public:
	RunTestGroup(SubmissionPtr submission, shared_ptr<TestGroup> testGroup):
		submissionID(submission->id), testGroupID(testGroup->id) {}
protected:
	void run() override {
		odb::session session;
		shared_ptr<TestGroup> group;
		shared_ptr<Submission> submission;
		{
			odb::transaction t(db::begin());
			group = db::load<TestGroup>(testGroupID);
			submission = db::load<Submission>(submissionID);
		}
		try {
			SubmissionUpdate update{submission, 0};
			bool allOK = 1;
			for(shared_ptr<TestCase> test: group->tests) {
				Result result = runForInput(submission, test);
				if (result.output) {
					allOK &= evaluateOutput(submission, move(result));
				} else {
					allOK = 0;
				}
				if (!allOK) break;
			}
			update.score = allOK ? group->points : 0;
		} catch(const ::apache::thrift::TException&) {
			auto lock = getLock();
			odb::transaction t(db::begin());
			submission->status = SubmissionStatus::ERROR;
			db::update(submission);
			t.commit();
			throw;
		}
	}
private:
	ID submissionID;
	ID testGroupID;

	Result runForInput(SubmissionPtr submission, shared_ptr<TestCase> test) {
		shared_ptr<Language> lang = submission->program.language;
		TaskPtr task = submission->task;
		StringMap inputs;
		inputs["binary"] = submission->program.binary.hash;
		inputs["input"] = test->input.hash;
		StringMap result = connection->runOnJudge(lang->runner, inputs, task->timeInSeconds, task->memoryInBytes);
		Result res;
		res.submission = submission;
		res.testCase = test;
		if (result.count("stdout")) {
			res.output.hash = result["stdout"];
		} else {
			res.status = ResultStatus::INTERNAL_ERROR;
		}
		if (result.count("stderr")) {
			res.errOutput.hash = result["stderr"];
		}
		{
			odb::transaction t(db::begin());
			db::persist(res);
			t.commit();
		}
		return res;
	}

	bool evaluateOutput(SubmissionPtr submission, Result result) {
		TaskPtr task = submission->task;
		Sandbox sandbox = task->evaluator.language->runner;
		StringMap inputs;
		inputs["binary"] = task->evaluator.binary.hash;
		inputs["output"] = result.output.hash;
		inputs["input"] = result.testCase->input.hash;
		inputs["correct"] = result.testCase->output.hash;
		StringMap resMap = connection->runOnJudge(sandbox, inputs, 1.0, 100<<20);
		cerr<<"result file "<<resMap.count("result")<<'\n';
		string resStr = readFileByHash(resMap["result"]);
		cerr<<"result string "<<resStr<<'\n';
		bool ok = *stringToInteger<int>(resStr);
		result.status = ok ? ResultStatus::CORRECT : ResultStatus::WRONG_ANSWER;

		odb::transaction t(db::begin());
		db::update(result);
		t.commit();
		return ok;
	}

	struct SubmissionUpdate {
		SubmissionPtr submission;
		int score;
		~SubmissionUpdate() {
			auto lock = getLock();
			odb::transaction t(db::begin());
			db::reload(submission);
			--submission->missingResults;
			submission->score += score;
			if (submission->missingResults == 0 && submission->status == SubmissionStatus::JUDGING) {
				submission->status = SubmissionStatus::READY;
			}
			db::update(submission);
			t.commit();
		}
	};

	static std::unique_lock<std::mutex> getLock() {
		return std::unique_lock<std::mutex>(submissionMutex);
	}
	// TODO: make per-submission mutex
	static std::mutex submissionMutex;
};
std::mutex RunTestGroup::submissionMutex;

template<class Owner, class Program>
void compileProgram(shared_ptr<Owner> owner, Program& program, JudgeConnection connection) {
	shared_ptr<Language> lang = program.language;
	StringMap inputs;
	inputs["source"] = program.source.hash;
	cerr<<"compiling with lang "<<lang->getName()<<" program "<<program.source.hash<<'\n';
	StringMap result = connection.runOnJudge(lang->compiler, inputs, 10.0, 50<<20);
	bool changed = 0;
	if (result.count("stderr") || result.count("stderr")) {
		program.compileMessage = result["stdout"] + result["stderr"];
		changed = 1;
	}
	if (result.count("binary")) {
		program.binary.hash = result["binary"];
		changed = 1;
	}
	cerr<<"changed: "<<changed<<'\n';
	if (changed) {
		odb::transaction t(db::begin());
		db::update(owner);
		t.commit();
	}
}

class CompileEvaluatorTask: public UnitTask {
public:
	CompileEvaluatorTask(ID id): id(id) {}
	void run() override {
		odb::session session;
		shared_ptr<Task> task;
		{
			odb::transaction t(db::begin());
			task = db::load<Task>(id);
		}
		compileProgram(task, task->evaluator, *connection);
	}
private:
	ID id;
};

class CompileAndRunTask: public UnitTask {
public:
	CompileAndRunTask(SubmissionPtr submission): submissionID(submission->id) {
	}

protected:
	void run() override {
		odb::session session;
		SubmissionPtr submission;
		{
			odb::transaction t(db::begin());
			submission = db::load<Submission>(submissionID);
			submission->status = SubmissionStatus::JUDGING;
			db::update(submission);
			t.commit();
		}
		try {
			compileProgram(submission, submission->program, *connection);
		} catch(const ::apache::thrift::TException&) {
			odb::transaction t(db::begin());
			submission->status = SubmissionStatus::ERROR;
			db::update(submission);
			t.commit();
			throw;
		}
		if (submission->program.binary) {
			startTestGroups(submission);
		} else {
			odb::transaction t(db::begin());
			submission->status = SubmissionStatus::COMPILE_ERROR;
			db::update(submission);
			t.commit();
		}
	}

private:
	ID submissionID;

	void startTestGroups(SubmissionPtr submission) {
		TaskPtr task = submission->task;
		{
			odb::transaction t(db::begin());
			db::load(*task, task->sec);
			submission->missingResults = task->testGroups.size();
			db::update(submission);
			t.commit();
		}
		for(auto group: task->testGroups) {
			master->addTask(new RunTestGroup(submission, group));
		}
	}
};

}

namespace cses {

void addForJudging(SubmissionPtr submission) {
	cerr<<"adding task for judging\n";
	CompileAndRunTask* task = new CompileAndRunTask(submission);
	JudgeMaster::instance().addTask(task);
}

void updateJudgeHosts() {
	JudgeMaster::instance().updateJudgeHosts();
}

void compileEvaluator(TaskPtr task) {
	JudgeMaster::instance().addTask(new CompileEvaluatorTask(task->id));
}

}

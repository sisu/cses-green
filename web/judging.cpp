#include "gen-cpp/Judge.h"
#include "judging.hpp"
#include "common/file.hpp"
#include "common/io_util.hpp"
#include "model_db-odb.hxx"
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
	protocol::JudgeClient client;
	string token = "uolevi";

	JudgeConnection(JudgeHost host): host(host), client(makeProtocol(host.host, host.port)) {
	}

	StringMap runOnJudge(DockerImage image, const StringMap& inputs, double timeLimit, int memoryLimit) {
		vector<File> files;
		for(auto& input: inputs) {
			File file;
			file.hash = input.second;
			file.name = input.first;
		}
		return runOnJudge(image, files, timeLimit, memoryLimit);
	}

	StringMap runOnJudge(DockerImage image, vector<File> inputs, double timeLimit, int memoryLimit) {
		vector<protocol::FileRef> fileRefs;
		for(File file: inputs) {
			if (!client.hasFile(token, file.hash)) {
				client.sendFile(token, readFileByHash(file.hash));
			}
			protocol::FileRef ref;
			ref.hash = file.hash;
			ref.name = file.name;
			fileRefs.push_back(move(ref));
		}
		protocol::RunOptions options;
		options.timeLimit = timeLimit;
		options.memoryLimitBytes = memoryLimit;
		protocol::RunResult result;
		client.run(result, token, image.repository, image.id, fileRefs, options);
		StringMap map;
		for(protocol::FileRef outFile: result.outputs) {
			if (!fileHashExists(outFile.hash)) {
				string data;
				client.getFile(data, token, outFile.hash);
				FileSave save;
				save.write(&data[0], data.size());
				save.save();
			}
			map[outFile.name] = outFile.hash;
		}
		return map;
	}

	bool operator<(const JudgeConnection& c) const {
		if (host.name != c.host.name) return host.name < c.host.name;
		if (host.host != c.host.host) return host.host < c.host.host;
		return host.port < c.host.port;
	}

private:
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
	JudgeConnection connection;
	~ReturnConnection();
};

class UnitTask {
public:
	virtual ~UnitTask() {}

	void execute(JudgeConnection connection, JudgeMaster& master) {
		unique_ptr<UnitTask> self(this);
		ReturnConnection ret{master, connection};
		(void)ret;
		run(connection, master);
	}

protected:
	virtual void run(JudgeConnection connection, JudgeMaster& master) = 0;

private:
};

class JudgeMaster {
public:
	static JudgeMaster& instance() {
		static JudgeMaster master;
		return master;
	}

	void judgeLoop() {
		while(1) {
			auto lock = getLock();
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
			odb::transaction t(db->begin());
			odb::result<JudgeHost> result = db->query<JudgeHost>();
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
	JudgeMaster() {}

	std::unique_lock<std::mutex> getLock() {
		return std::unique_lock<std::mutex>(mutex);
	}

	void startJudgings() {
		std::vector<JudgeConnection> freeHosts;
		std::set_difference(allJudgeHosts.begin(), allJudgeHosts.end(), usedJudgeHosts.begin(), usedJudgeHosts.end(), std::back_inserter(freeHosts));
		while(!freeHosts.empty() && !pendingTasks.empty()) {
			UnitTask* task = pendingTasks.front();
			pendingTasks.pop_front();
			JudgeConnection host = freeHosts.back();
			freeHosts.pop_back();
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
	RunTestGroup(SubmissionPtr submission, vector<shared_ptr<TestCase>>&& testCases):
		submission(submission), testCases(move(testCases)) {
	}
protected:
	void run(JudgeConnection connection, JudgeMaster&) override {
		(void)connection;
		for(shared_ptr<TestCase> test: testCases) {
			Result result = runForInput(connection, test);
			if (result.output) {
				evaluateOutput(connection, move(result));
			}
		}
	}
private:
	SubmissionPtr submission;
	vector<shared_ptr<TestCase>> testCases;

	Result runForInput(JudgeConnection connection, shared_ptr<TestCase> test) {
		shared_ptr<Language> lang = submission->program.language;
		TaskPtr task = submission->task;
		StringMap inputs;
		inputs["binary"] = submission->program.binary->hash;
		inputs["input"] = test->input->hash;
		StringMap result = connection.runOnJudge(lang->runner, inputs, task->timeInSeconds, task->memoryInBytes);
		Result res;
		res.submission = submission;
//		res.testCase = test;
		if (result.count("stdout")) {
			res.output.reset(new File(result["stdout"], "stdout"));
		}
		if (result.count("stderr")) res.errOutput.reset(new File(result["stderr"], "stderr"));
		{
			odb::transaction t(db->begin());
			db->persist(res);
			t.commit();
		}
		return res;
	}

	void evaluateOutput(JudgeConnection connection, Result result) {
		DockerImage image;
		StringMap inputs;
		inputs["output"] = result.output->hash;
		inputs["correct"] = result.testCase->output->hash;
		StringMap resMap = connection.runOnJudge(image, inputs, 1.0, 100<<20);
		string resStr = readFileByHash(resMap["result"]);

		odb::transaction t(db->begin());
		result.status = (ResultStatus)*stringToInteger<int>(resStr);
		db->update(result);
		t.commit();
	}
};

class CompileTask: public UnitTask {
public:
	CompileTask(SubmissionPtr submission): submission(submission) {
	}
protected:
	void run(JudgeConnection connection, JudgeMaster& master) override {
		shared_ptr<Language> lang = submission->program.language;
		StringMap inputs;
		inputs["source"] = submission->program.source->hash;
		StringMap result = connection.runOnJudge(lang->compiler, inputs, 10.0, 50<<20);
		if (result.count("binary")) {
			odb::transaction t(db->begin());
			submission->program.binary.reset(new File(result["binary"], "binary"));
			db->persist(submission->program.binary.get());
			db->update(submission.get());
			t.commit();
			startTestGroups(master);
		}
	}
private:
	SubmissionPtr submission;

	void startTestGroups(JudgeMaster& master) {
		TaskPtr task = submission->task;
		db->load(*task, task->sec);
		std::map<int, vector<shared_ptr<TestCase>>> testCasesByGroup;
		for(auto test: task->testCases) {
			testCasesByGroup[test->group].push_back(test);
		}
		for(auto& i: testCasesByGroup) {
			master.addTask(new RunTestGroup(submission, move(i.second)));
		}
	}
};

}

namespace cses {

void addForJudging(SubmissionPtr submission) {
	CompileTask* task = new CompileTask(submission);
	JudgeMaster::instance().addTask(task);
}

void updateJudgeHosts() {
	JudgeMaster::instance().updateJudgeHosts();
}

}

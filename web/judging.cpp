#include "gen-cpp/Judge.h"
#include "judging.hpp"
#include "common/file.hpp"
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
	string host;
	protocol::JudgeClient client;
	string token = "uolevi";

	JudgeConnection(string host): host(host), client(makeProtocol(host)) {
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
		return host < c.host;
	}

private:
	static boost::shared_ptr<apache::thrift::protocol::TProtocol> makeProtocol(string host) {
		using namespace apache::thrift;
		using namespace apache::thrift::protocol;
		using namespace apache::thrift::transport;

		boost::shared_ptr<TTransport> socket(new TSocket(host, 9090));
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
		odb::transaction t(db->begin());
		odb::result<JudgeHost> hosts = db->query<JudgeHost>();
		allJudgeHosts.clear();
		for(JudgeHost host: hosts) {
		}
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
			std::thread t(&UnitTask::execute, task, host, std::ref(*this));
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
		}
	}
private:
	SubmissionPtr submission;
	vector<shared_ptr<TestCase>> testCases;

	Result runForInput(JudgeConnection connection, shared_ptr<TestCase> test) {
		shared_ptr<Language> lang = submission->language;
		TaskPtr task = submission->task;
		StringMap inputs;
		inputs["binary"] = submission->binary->hash;
		inputs["input"] = test->input->hash;
		StringMap result = connection.runOnJudge(lang->runner, inputs, task->timeInSeconds, task->memoryInBytes);
		Result res;
		res.submission = submission;
//		res.testCase = test;
		if (result.count("stdout")) res.output.reset(new File(result["stdout"], "stdout"));
		if (result.count("stderr")) res.errOutput.reset(new File(result["stderr"], "stderr"));
		{
			odb::transaction t(db->begin());
			db->persist(res);
			t.commit();
		}
		return res;
	}
};

class CompileTask: public UnitTask {
public:
	CompileTask(SubmissionPtr submission): submission(submission) {
	}
protected:
	void run(JudgeConnection connection, JudgeMaster& master) override {
		shared_ptr<Language> lang = submission->language;
		StringMap inputs;
		inputs["source"] = submission->source->hash;
		StringMap result = connection.runOnJudge(lang->compiler, inputs, 10.0, 50<<20);
		if (result.count("binary")) {
			odb::transaction t(db->begin());
			submission->binary.reset(new File(result["binary"], "binary"));
			db->persist(submission->binary.get());
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

}

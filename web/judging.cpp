#include "judging.hpp"
#include "model_db-odb.hxx"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <set>
#include <algorithm>

namespace {
using namespace cses;

struct JudgeConnection {
	string host;

	bool operator<(const JudgeConnection& c) const {
		return host < c.host;
	}
};

class UnitTask {
public:
	virtual ~UnitTask() {}

	void execute(JudgeConnection connection) {
		unique_ptr<UnitTask> self(this);
		ReturnConnection ret{connection};
		(void)ret;
		run(connection);
	}

protected:
	virtual void run(JudgeConnection connection) = 0;

private:
	struct ReturnConnection {
		JudgeConnection connection;
		~ReturnConnection() {
		}
	};
};

class CompileTask: public UnitTask {
public:
	CompileTask(SubmissionPtr submission): submission(submission) {
	}
protected:
	void run(JudgeConnection connection) override {
		(void)connection;
	}
private:
	SubmissionPtr submission;
};

class JudgeGroupTask: public UnitTask {
protected:
	void run(JudgeConnection connection) override {
		(void)connection;
	}
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
			std::thread t(&UnitTask::execute, task, host);
		}
	}

	std::condition_variable condition;
	std::mutex mutex;

	std::deque<UnitTask*> pendingTasks;

	std::set<JudgeConnection> usedJudgeHosts;
	std::set<JudgeConnection> allJudgeHosts;
};

}

namespace cses {

void addForJudging(SubmissionPtr submission) {
	CompileTask* task = new CompileTask(submission);
	JudgeMaster::instance().addTask(task);
}

}

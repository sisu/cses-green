#include "model.hpp"
#include "io_util.hpp"
#include <odb/database.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/schema-catalog.hxx>
#include <openssl/sha.h>
#include <random>
#include <stdexcept>
#include "common/time.hpp"

namespace cses {

using namespace odb::core;

namespace {
	shared_ptr<EvaluatorLanguage> cppEvaluatorLanguage;
}

namespace db {

namespace detail {
	unique_ptr<odb::database> database;
}

void init(bool reset) {
	using detail::database;
	
	if (!reset) {
		database.reset(new odb::sqlite::database("cses.db"));
		return;
	}
	system("rm -f cses.db");
	database.reset(new odb::sqlite::database("cses.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#if 1
	{
		transaction t(db::begin());
		odb::schema_catalog::create_schema(*database);
		t.commit();
	}
#endif
	try {
		transaction t(db::begin());
		shared_ptr<User> testUser(new User);
		testUser->name = "a";
		testUser->password = Password("a");
		testUser->admin = true;
		db::persist(*testUser);
		
#if 0
		DockerImage cppCompiler("uolevin_repo", "64c4e93d9516515427c596281a87b04c4897d0300d350d15568e18cf6b8289f4");
		DockerImage binaryRunner("uolevin_repo", "0d410d21d84053a2d15d2509528e34100c6be9fdfd57683a142132e4b66f6301");
		DockerImage binaryEvaluator("uolevin_repo", "4e3652eaf1d4f603ad3cd43f1b4636468ea5e8818c85e753b9a3768122c178f5");
#else
		using std::ifstream;
		ifstream compilerScript("evaluators/compile_cpp.sh");
		ifstream runnerScript("evaluators/run_binary.sh");
		ifstream evaluatorScript("evaluators/run_evaluator.sh");
		PTraceConfig cppCompiler(PTraceConfig::NO_RESTRICT, "", {saveStreamToFile(compilerScript)});
		PTraceConfig binaryRunner(PTraceConfig::PTRACE, "0,1,2,3,4,5,9,10,11,12,21,59,158,231,", {saveStreamToFile(runnerScript)});
		PTraceConfig binaryEvaluator(PTraceConfig::NO_RESTRICT, "", {saveStreamToFile(evaluatorScript)});
#endif
		
		shared_ptr<SubmissionLanguage> cppSubmissionLanguage(
			new SubmissionLanguage("C++", "cpp", cppCompiler, binaryRunner)
		);
		db::persist(cppSubmissionLanguage);
		
		cppEvaluatorLanguage.reset(new EvaluatorLanguage("C++", "cpp", cppCompiler, binaryEvaluator));
		db::persist(cppEvaluatorLanguage);

		JudgeHost host;
		host.name = host.host = "localhost";
		host.port = 9090;
		db::persist(host);
		
		shared_ptr<Contest> cnt(new Contest());
		cnt->name = "testikisa";
		cnt->beginTime = current_time();
		cnt->endTime = cnt->beginTime+2*3600;
		shared_ptr<Task> lastTask;
		shared_ptr<TestCase> cases[20];
		db::persist(cnt);
		for (int i = 0; i < 3; i++) {
			shared_ptr<Task> tsk(new Task());
			if (i == 2) lastTask = tsk;
			if (i == 0) tsk->name = "apina";
			if (i == 1) tsk->name = "banaani";
			if (i == 2) tsk->name = "cembalo";
			tsk->timeInSeconds = 2;
			tsk->memoryInBytes = 16*1024*1024;
			tsk->contest = cnt;
			db::persist(tsk);
			
			for(int j=0; j<3; ++j) {
				shared_ptr<TestGroup> group(new TestGroup);
				group->task = tsk;
				if (j == 0) group->points = 15;
				if (j == 1) group->points = 35;
				if (j == 2) group->points = 50;
				tsk->testGroups.push_back(group);
				db::persist(group);
			}
			
			for (int j = 0; j < 20; j++) {
				shared_ptr<TestCase> tcase(new TestCase());
				cases[j] = tcase;
				int group = 0;
				if (j < 5) group = 0;
				else if (j < 12) group = 1;
				else if (j < 20) group = 2;
				tcase->group = tsk->testGroups[group];
				tsk->testGroups[group]->tests.push_back(tcase);
				db::persist(tcase);
			}
			cnt->tasks.push_back(tsk);
		}
		
// 		t.commit();
// 		return;
		
		shared_ptr<Submission> s(new Submission());
		s->user = testUser;
		s->task = lastTask;
		s->program.language = cppSubmissionLanguage;
		s->time = current_time();
		db::persist(s);
		for (int j = 0; j < 20; j++) {
			Result r;
			r.testCase = cases[j];
			r.timeInSeconds = 0.1*j;
			r.memoryInBytes = 10*j;
			if (j < 15) r.status = ResultStatus::CORRECT;
			else r.status = ResultStatus::TIME_LIMIT;
			r.submission = s;
			db::persist(r);
		}
		
// 		TestCase case[20];
// 		for (int i = 0; i < 20; i++) {
// 			case[i].
// 		}
		
		t.commit();
	} catch(object_already_persistent) { }
}

odb::transaction_impl* begin() {
	return detail::database->begin();
}

} // namespace db

EvaluatorProgram getDefaultEvaluator() {
	EvaluatorProgram e;
	std::ifstream in("evaluators/compare_ignore_ws.cpp");
	e.source = {saveStreamToFile(in)};
	e.language = cppEvaluatorLanguage;
	return e;
}

namespace {

uint64_t generateTrueRandom() {
	std::random_device device;
	std::uniform_int_distribution<uint64_t> distribution;
	return distribution(device);
}

//static __thread std::mt19937_64 rng(generateTrueRandom());
static std::mt19937_64 rng(generateTrueRandom());

string computeHash(string pass) {
	string res;
	res.resize(SHA_DIGEST_LENGTH);
	SHA1((const unsigned char*)&pass[0], pass.size(), (unsigned char*)&res[0]);
	return res;
}

string genSalt() {
	string ret;
	std::uniform_int_distribution<int> distribution(33, 126);
	for(int i = 0; i < 16; ++i) {
		ret.push_back(distribution(rng));
	}
	return ret;
}

} // end anonymous namespace

Password::Password(string newPassword) {
	salt = genSalt();
	hash = computeHash(salt + newPassword);
}

bool Password::matches(string cmpPassword) {
	string cmpHash = computeHash(salt + cmpPassword);
	return hash == cmpHash;
}

optional<ID> testLogin(string user, string pass) {
	transaction t(db::begin());
	result<User> res = db::query<User>(query<User>::name == user);
	if(res.empty()) return optional<ID>();
	User u = *res.begin();
	if(u.active && u.password.matches(pass)) {
		return u.id;
	} else {
		return optional<ID>();
	}
}

}

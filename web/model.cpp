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

unique_ptr<odb::database> db;

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


DockerImage::DockerImage(const string& repository, const string& id)
	: repository(repository),
	  id(id)
{
	if(!isValidRepositoryName(repository)) {
		throw Error("DockerImage::DockerImage: Invalid repository name.");
	}
	if(!isValidImageID(id)) {
		throw Error("DockerImage::DockerImage: Invalid image ID.");
	}
}


Language::Language(
	const string& name,
	const string& suffix,
	const DockerImage& compiler,
	const DockerImage& runner
) : compiler(compiler), runner(runner) {
	setName(name);
	setSuffix(suffix);
}

void Language::setName(const string& value) {
	if(!isValidName(value)) throw Error("Language::setName: Invalid value.");
	name = value;
}

bool Language::isValidName(const string& value) {
	size_t codepointCount = countCodePoints(value);
	return codepointCount != 0 && codepointCount <= 255;
}

void Language::setSuffix(const string& value) {
	if(!isValidSuffix(value)) throw Error("Language::setSuffix: Invalid value.");
	suffix = value;
}

bool Language::isValidSuffix(const string& value) {
	size_t codepointCount = countCodePoints(value);
	return codepointCount <= 16;
}


User::User(string name, string password, bool admin, bool active) {
	setName(name);
	setPassword(password);
	setAdmin(admin);
	setActive(active);
}

void User::setName(const string& newName) {
	if(!isValidName(newName)) throw Error("User::setName: Invalid name.");
	name = newName;
}

bool User::isPasswordMatch(const string& cmpPassword) const {
	string cmpHash = computeHash(salt + cmpPassword);
	return cmpHash == hash;
}

void User::setPassword(const string& newPassword) {
	if(!isValidPassword(newPassword)) throw Error("User::setPassword: Invalid password.");
	salt = genSalt();
	hash = computeHash(salt + newPassword);
}

bool User::isValidName(const string& name) {
	size_t codepointCount = countCodePoints(name);
	return codepointCount != 0 && codepointCount <= 255;
}
bool User::isValidPassword(const string& password) {
	size_t codepointCount = countCodePoints(password);
	return codepointCount != 0 && codepointCount <= 255;
}

void makeDB() {
	system("rm -f cses.db");
	db.reset(new odb::sqlite::database("cses.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#if 1
	{
		transaction t(db->begin());
		odb::schema_catalog::create_schema(*db);
		t.commit();
	}
#endif
	try {
		transaction t(db->begin());
		shared_ptr<User> testUser(new User("a", "a", true));
		db->persist(*testUser);
		
		DockerImage cppCompiler("uolevin_repo", "64c4e93d9516515427c596281a87b04c4897d0300d350d15568e18cf6b8289f4");
		DockerImage binaryRunner("uolevin_repo", "8ee631ba486ce7b4dfcf85fb5c035433670dd8af16d95c0b019e9759777ad82e");
		
		shared_ptr<SubmissionLanguage> cpp(
			new SubmissionLanguage("C++", "cpp", cppCompiler, binaryRunner)
		);
		db->persist(cpp);

		JudgeHost host;
		host.name = host.host = "localhost";
		host.port = 9090;
		db->persist(host);
		
		shared_ptr<Contest> cnt(new Contest());
		cnt->name = "testikisa";
		cnt->beginTime = current_time();
		cnt->endTime = cnt->beginTime+2*3600;
		shared_ptr<Task> lastTask;
		shared_ptr<TestCase> cases[20];
		db->persist(cnt);
		for (int i = 0; i < 3; i++) {
			shared_ptr<Task> tsk(new Task());
			if (i == 2) lastTask = tsk;
			if (i == 0) tsk->name = "apina";
			if (i == 1) tsk->name = "banaani";
			if (i == 2) tsk->name = "cembalo";
			tsk->timeInSeconds = 2;
			tsk->memoryInBytes = 16*1024*1024;
			tsk->contest = cnt;
			db->persist(tsk);
			
			for(int j=0; j<3; ++j) {
				shared_ptr<TestGroup> group(new TestGroup);
				group->task = tsk;
				if (j == 0) group->points = 15;
				if (j == 1) group->points = 35;
				if (j == 2) group->points = 50;
				tsk->testGroups.push_back(group);
				db->persist(group);
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
				db->persist(tcase);
			}
			cnt->tasks.push_back(tsk);
		}
		
// 		t.commit();
// 		return;
		
		shared_ptr<Submission> s(new Submission());
		s->user = testUser;
		s->task = lastTask;
		s->program.language = cpp;
		s->time = current_time();
		db->persist(s);
		for (int j = 0; j < 20; j++) {
			Result r;
			r.testCase = cases[j];
			r.timeInSeconds = 0.1*j;
			r.memoryInBytes = 10*j;
			if (j < 15) r.status = ResultStatus::CORRECT;
			else r.status = ResultStatus::TIME_LIMIT;
			r.submission = s;
			db->persist(r);
		}
		
// 		TestCase case[20];
// 		for (int i = 0; i < 20; i++) {
// 			case[i].
// 		}
		
		t.commit();
	} catch(object_already_persistent) { }
}

optional<ID> testLogin(string user, string pass) {
	transaction t(db->begin());
	result<User> res = db->query<User>(query<User>::name == user);
	if(res.empty()) return optional<ID>();
	User u = *res.begin();
	if(u.isActive() && u.isPasswordMatch(pass)) {
		return u.id;
	} else {
		return optional<ID>();
	}
}

}

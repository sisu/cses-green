#pragma once
#include "common.hpp"

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

#pragma db namespace session pointer(std::shared_ptr)
namespace cses {

extern unique_ptr<odb::database> db;

struct Contest;
struct TestCase;
struct Submission;
struct Group;
struct Language;
struct TestGroup;

typedef unsigned ID;

#pragma db object abstract
struct HasID {
#pragma db id auto
	ID id;
};

#define CHAR_FIELD(n) "VARCHAR(" #n ")"
#define STR_FIELD CHAR_FIELD(255)
#define ID_FIELD "INT UNSIGNED"

typedef string StrField;
#pragma db value(StrField) type(STR_FIELD)

#pragma db object
struct File: HasID {
#pragma db type(STR_FIELD)
	string hash;
#pragma db type(STR_FIELD)
	string name;

	File() {}
	File(string hash, string name): hash(hash), name(name) {}
};
typedef shared_ptr<File> UniqueFile;
#pragma db value(UniqueFile) not_null
typedef shared_ptr<File> MaybeFile;
#pragma db value(MaybeFile) null


#pragma db value
struct DockerImage {
#pragma db type(STR_FIELD)
	string repository;
#pragma db type(STR_FIELD)
	string id;
};


#pragma db object pointer(shared_ptr)
struct User: HasID {
public:
	User(string name, string password, bool admin = false, bool active = true);
	
	const string& getName() const {
		return name;
	}
	void setName(const string& newName);
	
	bool isPasswordMatch(const string& cmpPassword) const;
	void setPassword(const string& newPassword);
	
	bool isAdmin() const {
		return admin;
	}
	void setAdmin(bool newAdmin) {
		admin = newAdmin;
	}
	
	bool isActive() const {
		return active;
	}
	void setActive(bool newActive) {
		active = newActive;
	}
	
	// Usernames must consist of 1-255 code points.
	static bool isValidName(const string& name);
	
	// Passwords must consist of 1-255 code points.
	static bool isValidPassword(const string& password);
	
private:
	User() { }
	
#pragma db unique
	StrField name;
	StrField hash;
#pragma db type(CHAR_FIELD(16))
	string salt;
	bool admin;
	bool active;
	
#if 0
#pragma db value_not_null inverse(users) section(sec)
	vector<Group*> groups;
#pragma db load(lazy)
	odb::section sec;
#endif
	
	friend class odb::access;
};
typedef shared_ptr<User> UserPtr;
#pragma db value(UserPtr) not_null

#if 0
#pragma db object
struct Group: HasID {
	StrField name;
#pragma db unordered
	vector<UserPtr> users;

private:
	Group() {}
	friend class odb::access;
};
#endif

typedef shared_ptr<Contest> ContestPtr;

#pragma db value
struct Program {
#pragma db null
	shared_ptr<Language> language;
	MaybeFile source;
	MaybeFile binary;
};

#pragma db object pointer(shared_ptr)
struct Task: HasID {
#pragma db unique
	StrField name;
	//ContestPtr contest;

	Program evaluator;

	shared_ptr<Language> evaluatorLanguage;
	MaybeFile evaluatorSource;

#pragma db value_not_null inverse(task) section(sec)
	vector<shared_ptr<TestGroup>> testGroups;
//#pragma db value_not_null inverse(task) section(sec)
//	vector<shared_ptr<Submission>> submissions;
	double timeInSeconds;
	int memoryInBytes;

#pragma db load(lazy) update(manual)
	odb::section sec;

	Task() {}
private:
	friend class odb::access;
};
typedef shared_ptr<Task> TaskPtr;
#pragma db value(TaskPtr) not_null

#pragma db object pointer(shared_ptr)
struct TestGroup: HasID {
	weak_ptr<Task> task;
#pragma db value_not_null inverse(group)
	vector<shared_ptr<TestCase>> tests;
	int points = 1;
};

#pragma db object
struct TestCase: HasID {
	weak_ptr<TestGroup> group;
	UniqueFile input;
	UniqueFile output;

private:
	friend class odb::access;
};

#pragma db object
struct Contest: HasID {
#pragma db unique
	StrField name;
//#pragma db value_not_null inverse(contest) section(sec)	
#pragma db value_not_null section(sec)	
	vector<TaskPtr> tasks;

#if 0
#pragma db unordered
	vector<UserPtr> users;
#pragma db value_not_null unordered
	vector<shared_ptr<Group>> groups;
#endif

#pragma db load(lazy) update(manual)	
	odb::section sec;
	
	Contest() {}
private:
	friend class odb::access;
};

#pragma db object pointer(shared_ptr)
struct Language: HasID, std::enable_shared_from_this<Language> {
#pragma db unique
	StrField name;
#pragma db type(CHAR_FIELD(16))
	string suffix;
	DockerImage compiler;
	DockerImage runner;
};

enum class SubmissionStatus {
	PENDING,
	JUDGING,
	READY,
	ERROR,
};

#pragma db object
struct Submission: HasID {
	UserPtr user;
	TaskPtr task;
#pragma db not_null
	Program program;
	SubmissionStatus status;
};
typedef shared_ptr<Submission> SubmissionPtr;
#pragma db value(SubmissionPtr) not_null

enum class ResultStatus {
	CORRECT,
	WRONG_ANSWER,
	TIME_LIMIT,
	RUNTIME_ERROR,
	INTERNAL_ERROR
};

#pragma db object
struct Result: HasID {
	SubmissionPtr submission;
	shared_ptr<TestCase> testCase;
	MaybeFile output;
	MaybeFile errOutput;
	ResultStatus status;
	float timeInSeconds;
	int memoryInBytes;
};

#pragma db object
struct JudgeHost: HasID {
	StrField name;
	StrField host;
	int port;
};

}

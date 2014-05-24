#pragma once
#include "common.hpp"
#include "judge_interface.hpp"

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

#pragma db value
struct File {
	string hash;
};

#pragma db value
struct MaybeFile {
	string hash;
	
	operator UnpromotableBoolean::Type() {
		if(hash.empty()) {
			return UnpromotableBoolean::falseValue();
		} else {
			return UnpromotableBoolean::trueValue();
		}
	}
};

#pragma db value
struct DockerImage {
public:
	DockerImage(const string& repository, const string& id);
	
	static bool isValidRepositoryName(const string& value) {
		return judge_interface::isSafeIdentifier(value);
	}
	static bool isValidImageID(const string& value) {
		return judge_interface::isValidImageID(value);
	}
	
	const string& getRepositoryName() const {
		return repository;
	}
	const string& getImageID() const {
		return id;
	}
	
private:
	DockerImage() { }
	
	StrField repository;
	StrField id;
	
	friend class odb::access;
	friend class Language;
};


#pragma db object abstract
struct Language: HasID {
public:
	Language(
		const string& name,
		const string& suffix,
		const DockerImage& compiler,
		const DockerImage& runner
	);
	
	DockerImage compiler;
	DockerImage runner;
	
	const string& getName() const {
		return name;
	}
	void setName(const string& value);
	
	// Language names must be strings of length 1-255.
	static bool isValidName(const string& value);
	
	const string& getSuffix() const {
		return suffix;
	}
	void setSuffix(const string& value);
	
	// Language suffixes must be strings of length 0-16.
	// Empty suffix denotes no suffix.
	static bool isValidSuffix(const string& value);
protected:
	Language() { }
	
private:
	StrField name;
#pragma db type(CHAR_FIELD(16))
	string suffix;
	
	friend class odb::access;
};

#pragma db object pointer(shared_ptr)
struct SubmissionLanguage: Language {
public:
#pragma db index unique member(name)
	SubmissionLanguage(
		const string& name,
		const string& suffix,
		const DockerImage& compiler,
		const DockerImage& runner
	) : Language(name, suffix, compiler, runner) { }
	
private:
	SubmissionLanguage() { }
	
	friend class odb::access;
};

#pragma db object pointer(shared_ptr)
struct EvaluatorLanguage: Language {
public:
#pragma db index unique member(name)
	EvaluatorLanguage(
		const string& name,
		const string& suffix,
		const DockerImage& compiler,
		const DockerImage& runner
	) : Language(name, suffix, compiler, runner) { }
	
private:
	EvaluatorLanguage() { }
	friend class odb::access;
};

#pragma db value
struct SubmissionProgram {
	shared_ptr<SubmissionLanguage> language;
	MaybeFile source;
	MaybeFile binary;
};

#pragma db value
struct EvaluatorProgram {
	shared_ptr<EvaluatorLanguage> language;
	MaybeFile source;
	MaybeFile binary;
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

#pragma db object pointer(shared_ptr)
struct Task: HasID {
#pragma db unique
	StrField name;
	weak_ptr<Contest> contest;

	EvaluatorProgram evaluator;

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
	File input;
	File output;

private:
	friend class odb::access;
};

#pragma db object
struct Contest: HasID {
#pragma db unique
	StrField name;
#pragma db value_not_null inverse(contest) section(sec)	
//#pragma db value_not_null section(sec)	
	vector<TaskPtr> tasks;
	long long beginTime;
	long long endTime;

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
	SubmissionProgram program;
	SubmissionStatus status;
	int score = 0;
	long long time = 0;
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

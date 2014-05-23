#pragma once
#include "common.hpp"

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

namespace cses {

extern unique_ptr<odb::database> db;

struct Contest;
struct TestCase;
struct Submission;
struct Group;

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
};
typedef unique_ptr<File> UniqueFile;
#pragma db value(UniqueFile) not_null


#pragma db object
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

#pragma db object
struct Task: HasID {
#pragma db unique
	StrField name;
	
	UniqueFile evaluator;
	
#pragma db value_not_null inverse(task) section(sec)
	vector<unique_ptr<TestCase>> testCases;
//#pragma db value_not_null inverse(task)
//	vector<unique_ptr<Submission>> submissions;

#pragma db load(lazy) update(manual)
	odb::section sec;

private:
	Task() {}
	friend class odb::access;
};
typedef shared_ptr<Task> TaskPtr;
#pragma db value(TaskPtr) not_null

#pragma db object
struct TestCase: HasID {
	TaskPtr task;
	UniqueFile input;
	UniqueFile output;

private:
	TestCase() {}
	friend class odb::access;
};

#pragma db object
struct Contest: HasID {
#pragma db unique
	StrField name;
	vector<TaskPtr> tasks;

#if 0
#pragma db unordered
	vector<UserPtr> users;
#pragma db value_not_null unordered
	vector<shared_ptr<Group>> groups;
#endif

private:
	Contest() {}
	friend class odb::access;
};

#pragma db object
struct Language: HasID {
#pragma db unique
	StrField name;
#pragma db type(CHAR_FIELD(16))
	string suffix;
	UniqueFile compiler;
	UniqueFile runner;
};

#pragma db object
struct Submission: HasID {
	UserPtr user;
	TaskPtr task;
#pragma db not_null
	shared_ptr<Language> language;
	UniqueFile source;
	UniqueFile binary;
	int status;
};

#pragma db object
struct Result: HasID {
	shared_ptr<Submission> submission;
	shared_ptr<TestCase> testCase;
	UniqueFile output;
	UniqueFile errOutput;
	int result;
	float time;
	int memory;
};

#pragma db object
struct JudgeHost: HasID {
	StrField name;
	StrField host;
};

}

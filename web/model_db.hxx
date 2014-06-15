#pragma once
#include "common.hpp"
#include "file.hpp"
#include "io_util.hpp"
#include "judge_interface.hpp"

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>
#include <boost/optional.hpp>

#pragma db namespace session pointer(std::shared_ptr)
namespace cses {

struct Contest;
struct TestCase;
struct Submission;
struct Group;
struct TestGroup;

typedef unsigned ID;

#pragma db object abstract
struct DBObject {
#pragma db id auto
	ID id;
	
	// Default validate does nothing, override and throw ValidationFailure or
	// Error on errors.
	void validate() { }
};

#define CHAR_FIELD(n) "VARCHAR(" #n ")"
#define STR_FIELD CHAR_FIELD(255)
#define ID_FIELD "INT UNSIGNED"

typedef string StrField;
#pragma db value(StrField) type(STR_FIELD)


#pragma db value
struct Password {
	StrField hash;
#pragma db type(CHAR_FIELD(16))
	string salt;
	
	
	Password(string newPassword); 
	Password() { }
	
	bool matches(string cmpPassword);
	
	void validate() {
		if(salt.empty()) throw Error("Password::validate: Field not set.");
		if(matches("")) throw ValidationFailure("Empty password is not allowed.");
	}
};

#pragma db object
struct User: DBObject {
#pragma db unique
	StrField name;
	
	Password password;
	bool admin = false;
	bool active = true;
	
	void validate() {
		size_t nameLength = countCodePoints(name);
		if(nameLength == 0 || nameLength > 255) {
			throw ValidationFailure("Username must consist of 1-255 characters.");
		}
		
		password.validate();
	}
};
typedef shared_ptr<User> UserPtr;
#pragma db value(UserPtr) not_null

#pragma db value
struct File {
	string hash;
	
	
	void validate() {
		if(hash.empty()) return;
		if(!isValidFileHash(hash)) {
			throw Error("MaybeFile::validate: Invalid file hash.");
		}
		if(!fileHashExists(hash)) {
			throw Error("MaybeFile::validate: File not saved.");
		}
	}
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
	
	void validate() {
		if(hash.empty()) return;
		if(!isValidFileHash(hash)) {
			throw Error("MaybeFile::validate: Invalid file hash.");
		}
		if(!fileHashExists(hash)) {
			throw Error("MaybeFile::validate: File not saved.");
		}
	}
};

#pragma db value
struct DockerImage {
public:
	StrField repository;
	StrField imageID;
	
	
	DockerImage() { }
	DockerImage(const string& repository, const string& imageID)
		: repository(repository), imageID(imageID) { }
	
	void validate() {
		if(!judge_interface::isSafeIdentifier(repository)) {
			throw ValidationFailure("Repository name is not a safe identifier (a-zA-Z letters, numbers and underscores, length 1-64).");
		}
		if(!judge_interface::isValidImageID(imageID)) {
			throw ValidationFailure("Invalid image ID (image IDs consist of 64 characters 0-9a-f).");
		}
	}
};

#pragma db value
struct PTraceConfig {
	enum SyscallPolicy {
		NO_RESTRICT,
		PTRACE,
		SECCOMP,
	};
	SyscallPolicy policy = SECCOMP;
	StrField allowedSyscalls;
	MaybeFile runner;
	
	
	PTraceConfig() { }
	PTraceConfig(SyscallPolicy policy, StrField syscalls, MaybeFile runner)
		: policy(policy), allowedSyscalls(syscalls), runner(runner) { }
	
	void validate() {
		// TODO: check policy and allowedSyscalls.
		runner.validate();
	}
};

#pragma db value
struct Sandbox {
	enum Type { DOCKER, PTRACE };
	Type type = DOCKER;
	DockerImage docker;
	PTraceConfig ptrace;
	
	
	Sandbox(DockerImage docker): type(DOCKER), docker(docker) { }
	Sandbox(PTraceConfig ptrace): type(PTRACE), ptrace(ptrace) { }
	Sandbox(Type type): type(type) { }
	Sandbox() { }
	
	void validate() {
		if(type == DOCKER) {
			docker.validate();
		} else if(type == PTRACE) {
			ptrace.validate();
		} else {
			throw Error("Sandbox::validate: type not set to any valid enum value.");
		}
	}
};


#pragma db object abstract
struct Language: DBObject {
public:
	StrField name;
	
#pragma db type(CHAR_FIELD(16))
	string suffix;
	
	Sandbox compiler;
	Sandbox runner;
	
	
	Language(
		const string& name,
		const string& suffix,
		const Sandbox& compiler,
		const Sandbox& runner
	) : name(name), suffix(suffix), compiler(compiler), runner(runner) { }
	Language() { }
	
	void validate() {
		size_t nameLength = countCodePoints(name);
		if(nameLength == 0 || nameLength > 255) {
			throw ValidationFailure("Language name must consist of 1-255 characters.");
		}
		
		size_t suffixLength = countCodePoints(suffix);
		if(suffixLength == 0 || suffixLength > 16) {
			throw ValidationFailure("Language suffix must be empty or consist of 1-16 characters.");
		}
		
		compiler.validate();
		runner.validate();
	}
};

#pragma db object
struct SubmissionLanguage: Language {
#pragma db index unique member(name)
	SubmissionLanguage(
		const string& name,
		const string& suffix,
		const Sandbox& compiler,
		const Sandbox& runner
	) : Language(name, suffix, compiler, runner) { }
	SubmissionLanguage() { }
	
	// All validation done in Language::validate.
};

#pragma db object
struct EvaluatorLanguage: Language {
#pragma db index unique member(name)
	EvaluatorLanguage(
		const string& name,
		const string& suffix,
		const Sandbox& compiler,
		const Sandbox& runner
	) : Language(name, suffix, compiler, runner) { }
	EvaluatorLanguage() { }
	
	// All validation done in Language::validate.
};

#pragma db value
struct SubmissionProgram {
	shared_ptr<SubmissionLanguage> language;
	MaybeFile source;
	MaybeFile binary;
	string compileMessage;
	
	
	void validate() {
		source.validate();
		binary.validate();
	}
};

#pragma db value
struct EvaluatorProgram {
	shared_ptr<EvaluatorLanguage> language;
	MaybeFile source;
	MaybeFile binary;
	string compileMessage;
	
	
	void validate() {
		source.validate();
		binary.validate();
	}
};

#if 0
#pragma db object
struct Group: DBObject {
	StrField name;
#pragma db unordered
	vector<UserPtr> users;

private:
	Group() {}
	friend class odb::access;
};
#endif

typedef shared_ptr<Contest> ContestPtr;

#pragma db object
struct Task: DBObject {
	StrField name;
	weak_ptr<Contest> contest;
	
	EvaluatorProgram evaluator;
	
#pragma db value_not_null inverse(task) section(sec)
	vector<shared_ptr<TestGroup>> testGroups;
//#pragma db value_not_null inverse(task) section(sec)
//	vector<shared_ptr<Submission>> submissions;
	double timeInSeconds = 1.0;
	int64_t memoryInBytes = 64 * 1024 * 1024;
	
#pragma db load(lazy) update(manual)
	odb::section sec;
	
	
	void validate() {
		size_t nameLength = countCodePoints(name);
		if(nameLength == 0 || nameLength > 255) {
			throw ValidationFailure("Task name must consist of 1-255 characters.");
		}
		
		evaluator.validate();
		
		if(!std::isfinite(timeInSeconds) || timeInSeconds <= 0) {
			throw ValidationFailure("Time limit must be positive.");
		}
		
		if(memoryInBytes <= 0) {
			throw ValidationFailure("Memory limit must be positive.");
		}
	}
};
typedef shared_ptr<Task> TaskPtr;
#pragma db value(TaskPtr) not_null

#pragma db object
struct TestGroup: DBObject {
	weak_ptr<Task> task;
#pragma db value_not_null inverse(group)
	vector<shared_ptr<TestCase>> tests;
	int points = 1;
	
	
	void validate() {
		if(points <= 0) {
			throw ValidationFailure("Points for test group must be positive.");
		}
	}
};

#pragma db object
struct TestCase: DBObject {
	weak_ptr<TestGroup> group;
	File input;
	File output;
	string inputName;
	string outputName;

	
	void validate() {
		input.validate();
		output.validate();
		// TODO: validate inputName and outputName.
	}
};

#pragma db object
struct Contest: DBObject {
#pragma db unique
	StrField name;
#pragma db value_not_null inverse(contest) section(sec)	
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
	COMPILE_ERROR,
	READY,
	ERROR,
};

#pragma db object
struct Submission: DBObject {
	UserPtr user;
	TaskPtr task;
	SubmissionProgram program;
	SubmissionStatus status;
	int score = 0;
	long long time = 0;
	int missingResults = 0;
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
struct Result: DBObject {
	SubmissionPtr submission;
	shared_ptr<TestCase> testCase;
	MaybeFile output;
	MaybeFile errOutput;
	ResultStatus status = ResultStatus::INTERNAL_ERROR;
	float timeInSeconds = 0;
	int memoryInBytes = 0;
};

#pragma db object
struct JudgeHost: DBObject {
	StrField name;
	StrField host;
	int port;
};

}

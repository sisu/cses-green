#pragma once
#include "common.hpp"
#include "widgets.hpp"
#include "model.hpp"
#include "form.hpp"
#include "common/time.hpp"

namespace cses {

struct PasswordWidgetProvider: WidgetProvider {
	ws::base_widget& getWidget() override {
		return widget;
	}
	void readWidget() override {
		ref = Password(widget.value());
	}

	PasswordWidgetProvider(Password& v, const string& name): ref(v) {
		widget.message(name);
	}
	ws::password widget;
	Password& ref;
};
struct ChangePasswordWidgetProvider : PasswordWidgetProvider {
	void readWidget() override {
		if(!widget.value().empty()) {
			ref = Password(widget.value());
		}
	}
	
	ChangePasswordWidgetProvider(Password& v, const string& name)
		: PasswordWidgetProvider(v, name) { }
};

struct Page: cppcms::base_content {
	string user;
	bool logged=0;
	bool admin=0;
	void set(UserPtr u) {
		if (u) {
			user = u->name;
			logged = 1;
			admin = u->admin;
		} else {
			user.clear();
			logged = 0;
			admin = 0;
		}
	}
	Page(UserPtr u) {
		set(u);
	}
};

struct ContestsPage: Page {
	vector<Contest> contests;
	
	ContestsPage(UserPtr user): Page(user) { }
};

struct RegistrationPage: Page {
	RegistrationPage(UserPtr u, User& newUser) : Page(u), builder(form) {
		builder
			.add(newUser.name, "Name")
			.addProvider<PasswordWidgetProvider, Password>(newUser.password, "Password")
			.addSubmit();
	};
	
	string msg;
	bool showForm;
	cppcms::form form;
	FormBuilder builder;
};

struct LoginPage: Page {
	LoginPage(UserPtr u) : Page(u) { }
	
	struct Info: cppcms::form {
		ws::text name;
		ws::password password;
		ws::submit submit;
		Info() {
			name.message("Name");
			password.message("Password");
			submit.value("Login");
			add(name);
			add(password);
			add(submit);
		}
	};
	bool loginFailed = false;
	Info info;
};

struct LanguagesPage: Page {
	LanguagesPage(UserPtr u) : Page(u) { }
	
	vector<SubmissionLanguage> submissionLanguages;
	vector<EvaluatorLanguage> evaluatorLanguages;
	const string NEW_LANGUAGE = "new";
	const string SUBMISSION_LANGUAGE = "submission";
	const string EVALUATOR_LANGUAGE = "evaluator";
};

struct UsersPage: Page {
	UsersPage(UserPtr u) : Page(u) { }
	
	vector<User> users;
};

struct EditUserPage: Page {
	EditUserPage(UserPtr u, User& editUser) : Page(u), builder(form) {
		builder
			.add(editUser.name, "Name")
			.addProvider<ChangePasswordWidgetProvider, Password>(
				editUser.password, "Change password"
			)
			.addProvider<CheckboxProvider, bool>(editUser.admin, "Admin")
			.addProvider<CheckboxProvider, bool>(editUser.active, "Active")
			.addSubmit();
	};
	
	string msg;
	cppcms::form form;
	FormBuilder builder;
};

struct EditLanguagePage: Page {
	EditLanguagePage(UserPtr u) : Page(u) { }
	
	struct DockerForm: cppcms::form {
		ws::text repository;
		ws::text imageID;
		DockerForm(const string& name) {
			repository.message(name + " repository");
			imageID.message(name + " image ID");
			add(repository);
			add(imageID);
		}
	};
	struct PTraceForm: cppcms::form {
		ws::radio policy;
		ws::text allowedCalls;
		ws::file runner;
		PTraceForm(const string& name) {
			using std::to_string;
			policy.message(name + " syscall policy");
			policy.add("no restrictions (unsafe)", to_string(PTraceConfig::NO_RESTRICT));
			policy.add("ptrace (slow syscalls)", to_string(PTraceConfig::PTRACE));
			policy.add("seccomp (required modern kernel)", to_string(PTraceConfig::SECCOMP));
			policy.selected(2);
			allowedCalls.message(name + " allowed system calls");
			runner.message(name + " run script");
			add(policy);
			add(allowedCalls);
			add(runner);
		}
	};
	struct RunnerForm: cppcms::form {
		ws::radio type;
		DockerForm docker;
		PTraceForm ptrace;
		RunnerForm(const string& name): docker(name), ptrace(name) {
			using std::to_string;
			type.message(name + " type");
			type.add("Docker", to_string(Sandbox::DOCKER));
			type.add("PTrace", to_string(Sandbox::PTRACE));
			type.selected(1);
			add(type);
			add(docker);
			add(ptrace);
		}
		bool validate() override {
			if (!type.validate()) return false;
			switch(getType()) {
				case Sandbox::DOCKER:
					return docker.validate();
					break;
				case Sandbox::PTRACE:
					return ptrace.validate();
					break;
				default:
					type.error_message("Invalid type");
					return false;
			}
		}
		Sandbox::Type getType() {
			return Sandbox::Type(atoi(type.selected_id().c_str()));
		}
	};
	
	struct Form: cppcms::form {
		ws::text name;
		ws::text suffix;
		RunnerForm compilerForm;
		RunnerForm runnerForm;
		ws::submit submit;
		Form(): compilerForm("Compiler"), runnerForm("Runner") {
			name.message("Name");
			suffix.message("Suffix");
			submit.value("Save");
			add(name);
			add(suffix);
			add(compilerForm);
			add(runnerForm);
			add(submit);
		}
	};
	Form form;
	bool success = false;
	bool nameInUse = false;
};

struct ImportPage: Page {
	ImportPage(UserPtr u) : Page(u) { }
	
	struct Form: cppcms::form {
		ws::text name;
		ws::file package;
		ws::submit submit;
		Form() {
			name.message("Contest name");
			package.message("ZIP file");
			submit.value("Create");
			add(name);
			add(package);
			add(submit);
		}
	};
	Form form;
};

struct InContestPage: Page {
	string name;
	ID id;
	string formatTime1;
	string formatTime2;
	long long beginTime;
	long long endTime;
	long long curTime;

	InContestPage(UserPtr user, const Contest& cnt): Page(user),
		name(cnt.name),
		id(cnt.id),
		formatTime1(formatTime(cnt.beginTime)),
		formatTime2(formatTime(cnt.endTime)),
		beginTime(cnt.beginTime),
		endTime(cnt.endTime),
		curTime(currentTime())
	{}
};

struct SubmitPage: InContestPage {
	struct Form: cppcms::form {
		ws::select task;
		ws::file file;
		ws::select language;
		ws::submit submit;
		Form() {
			task.message("Task");
			file.message("Solution");
			language.message("Language");
			submit.value("Submit");
			add(task);
			add(file);
			add(language);
			add(submit);
		}
	};
	Form form;

	SubmitPage(UserPtr user, const Contest& cnt): InContestPage(user, cnt) {}
};

struct ViewPage: InContestPage {	
	struct result {
		int number;
		float timeInSeconds;
		int memoryInKBytes;
		string status;
		string color;
	};
	struct group {
		int number;
		vector<result> results;
		int points, total;
	};
	vector<group> groups;
	int points, total;
	string status;
	int ownID;
	string taskName;

	ViewPage(UserPtr user, const Contest& cnt): InContestPage(user, cnt) {}
};

struct CodePage: InContestPage {	
	string code;
	int ownID;
	string taskName;

	CodePage(UserPtr user, const Contest& cnt): InContestPage(user, cnt) {}
};

struct ListPage: InContestPage {
	struct item {
		string id;
		string task;
		string time;
		string status;
	};
	vector<item> items;

	ListPage(UserPtr user, const Contest& cnt): InContestPage(user, cnt) {}
};

struct ScoresPage: InContestPage {
	struct Cell {
		bool has = 0;
		int score = 0;
		string color;
	};
	struct Row {
		string user;
		vector<Cell> cells;
		int score = 0;

		bool operator<(const Row& c) const {
			return score < c.score;
		}
	};
	vector<string> tasks;
	vector<Row> rows;

	ScoresPage(UserPtr user, const Contest& cnt): InContestPage(user, cnt) {}
};

struct EditContestPage: InContestPage {
	struct Task {
		string name;
		ID id;
	};

	EditContestPage(UserPtr user, Contest& c): InContestPage(user, c), builder(form) {
		builder.add(c.name, "Name")
			.add(c.beginTime, "Begin time")
			.add(c.endTime, "End time")
			.addProvider<CheckboxProvider, bool>(c.active, "Active")
			.addSubmit();
	}
	cppcms::form form;
	FormBuilder builder;
	vector<Task> tasks;
};

struct TaskPage: InContestPage {
	TaskPage(UserPtr user, const Contest& cnt, Task& t, vector<shared_ptr<EvaluatorLanguage>> languages): InContestPage(user, cnt), builder(form) {
		auto& e = t.evaluator;
		vector<pair<string,shared_ptr<EvaluatorLanguage>>> choises;
		for(auto lang: languages) {
			choises.emplace_back(lang->name, lang);
		}
		builder.add(t.name, "Name")
			.add(t.timeInSeconds, "Time (s)")
			.add(t.memoryInBytes, "Memory (B)")
			.addProvider<FileUploadProvider<MaybeFile>>(e.source, "Evaluator source")
			.add(makeSelectProvider(e.language, "Evaluator language", choises))
			.addSubmit();
		name = t.name;
	}
	string name;
	string compileMessage;

	cppcms::form form;
	FormBuilder builder;
};

}

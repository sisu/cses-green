#pragma once
#include "common.hpp"
#include "widgets.hpp"
#include "model.hpp"

namespace cses {

typedef optional<string> OptString;

struct SetUsernameWidget: ValidatingWidget<ws::text> {
	virtual CheckResult checkValue(const string& name) override {
		if(User::isValidName(name)) return ok();
		return error("Invalid username. Username must consist of 1-255 characters.");
	}
};

struct SetPasswordWidget: ValidatingWidget<ws::password> {
	virtual CheckResult checkValue(const string& password) override {
		if(User::isValidPassword(password)) return ok();
		return error("Invalid password. Password must consist of 1-255 characters.");
	}
};

struct SetOptionalPasswordWidget: ValidatingWidget<ws::password> {
	virtual CheckResult checkValue(const string& password) override {
		if(password.empty() || User::isValidPassword(password)) return ok();
		return error("Invalid password. Password must consist of 1-255 characters.");
	}
};

struct Page: cppcms::base_content {
};

struct ContestsPage: Page {
	vector<pair<unsigned,string>> contests;
};

struct ContestPage: Page {
	struct Info: cppcms::form {
		ws::text name;
	};
	Info info;
//	models::Contest* cnt;
};


struct RegistrationPage: Page {
	struct Info: cppcms::form {
		SetUsernameWidget name;
		SetPasswordWidget password;
		ws::submit submit;
		Info() {
			name.message("Name");
			password.message("Password");
			submit.value("Register");
			add(name);
			add(password);
			add(submit);
		}
	};
	Info info;
};

struct LoginPage: Page {
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

struct AdminPage: Page {
	vector<User> users;
	vector<SubmissionLanguage> submissionLanguages;
	vector<EvaluatorLanguage> evaluatorLanguages;
	const string NEW_LANGUAGE = "new";
	const string SUBMISSION_LANGUAGE = "submission";
	const string EVALUATOR_LANGUAGE = "evaluator";
};

struct AdminEditUserPage: Page {
	struct Form: cppcms::form {
		SetUsernameWidget name;
		SetOptionalPasswordWidget password;
		ws::checkbox admin;
		ws::checkbox active;
		ws::submit submit;
		
		Form() {
			name.message("Username");
			password.message("New password (empty to keep old)");
			admin.message("Admin");
			active.message("Active");
			submit.value("Save");
			add(name);
			add(password);
			add(admin);
			add(active);
			add(submit);
		}
	};
	
	Form form;
	bool success = false;
	bool nameInUse = false;
};

struct AdminEditLanguagePage: Page {
	struct SetLanguageNameWidget: ValidatingWidget<ws::text> {
		virtual CheckResult checkValue(const string& value) override {
			if(Language::isValidName(value)) return ok();
			return error("Invalid name. Name must consist of 1-255 characters.");
		}
	};
	struct SetLanguageSuffixWidget: ValidatingWidget<ws::text> {
		virtual CheckResult checkValue(const string& value) override {
			if(Language::isValidSuffix(value)) return ok();
			return error("Invalid suffix. Suffix must consist of 0-16 characters (0 if no suffix).");
		}
	};
	struct SetCompilerRepositoryWidget: ValidatingWidget<ws::text> {
		virtual CheckResult checkValue(const string& value) override {
			if(DockerImage::isValidRepositoryName(value)) return ok();
			return error("Unsupported repository name. Name must consist of 1-64 letters, numbers and underscores.");
		}
	};
	struct SetCompilerImageIDWidget: ValidatingWidget<ws::text> {
		virtual CheckResult checkValue(const string& value) override {
			if(DockerImage::isValidImageID(value)) return ok();
			return error("Invalid image ID. Image IDs consist of 64 digits 0-9 or letters a-f.");
		}
	};
	
	struct Form: cppcms::form {
		SetLanguageNameWidget name;
		SetLanguageSuffixWidget suffix;
		SetCompilerRepositoryWidget compilerRepository;
		SetCompilerImageIDWidget compilerImageID;
		SetCompilerRepositoryWidget runnerRepository;
		SetCompilerImageIDWidget runnerImageID;
		ws::submit submit;
		Form() {
			name.message("Name");
			suffix.message("Suffix");
			compilerRepository.message("Compiler repository");
			compilerImageID.message("Compiler image ID");
			runnerRepository.message("Runner repository");
			runnerImageID.message("Runner image ID");
			submit.value("Save");
			add(name);
			add(suffix);
			add(compilerRepository);
			add(compilerImageID);
			add(runnerRepository);
			add(runnerImageID);
			add(submit);
		}
	};
	Form form;
	bool success = false;
	bool nameInUse = false;
};

struct AdminImportPage: Page {
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
};

struct CodePage: InContestPage {	
	string code;
	int ownID;
	string taskName;
};

struct ListPage: InContestPage {
	struct item {
		string id;
		string task;
		string time;
		string status;
	};
	vector<item> items;
};

struct ScoresPage: InContestPage {
	struct Cell {
		bool has = 0;
		int score = 0;
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
};

}

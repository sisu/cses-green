#pragma once
#include "common.hpp"
#include "widgets.hpp"
#include "model.hpp"

namespace cses {

struct Page: cppcms::base_content {
};

struct ContestsPage: Page {
	vector<string> contests;
};

struct SubmitPage: Page {
};

struct ContestPage: Page {
	struct Info: cppcms::form {
		ws::text name;
	};
	Info info;
//	models::Contest* cnt;
};


struct UserPage: Page {
	struct Info: cppcms::form {
		ws::text name;
		ws::password password;
		ws::submit submit;

		Info() {
			name.message("Name");
			password.message("Password");
			submit.value("Submit");
			add(name);
			add(password);
			add(submit);
		}
	};
	Info info;
};

struct RegistrationPage: Page {
	struct Info: cppcms::form {
		SetUsernameWidget name;
		ws::password password;
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
	Info info;
};

struct LanguagesPage: Page {
	struct Language: cppcms::form {
		ws::text name;
		ws::text suffix;
		ws::file compiler;
		ws::file runner;
		ws::submit submit;
		Language() {
			name.message("Name");
			suffix.message("Suffix");
			compiler.message("Compiler");
			runner.message("Runner");
			submit.value("Save");
			add(name);
			add(suffix);
			add(compiler);
			add(runner);
			add(submit);
		}
	};
	vector<Language> languages;
	Language newLang;
};

struct AdminPage: Page {
	vector<User> userlist;
};

struct AdminEditUserPage: Page {
	struct BasicForm: cppcms::form {
		SetUsernameWidget name;
		ws::select_multiple groups;
		ws::checkbox active;
		ws::submit submit;
		
		BasicForm() {
			name.message("Username");
			groups.message("Groups");
			active.message("Active");
			submit.message("Save");
			add(name);
			add(groups);
			add(active);
			add(submit);
		}
	};
	struct PasswordForm: cppcms::form {
		ws::password password;
		ws::submit submit;
		
		PasswordForm() {
			password.message("New password");
			submit.message("Set password");
			add(password);
			add(submit);
		}
	};
	
	BasicForm basicForm;
	PasswordForm passwordForm;
};

}

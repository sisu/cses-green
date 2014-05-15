#pragma once
#include "models.hxx"
#include <cppcms/view.h>
#include <cppcms/form.h>
#include <vector>
#include <string>

namespace content {

struct Page: cppcms::base_content {
};

struct Contests: Page {
	std::vector<std::string> contests;
};

struct Submit: Page {
};

namespace ws = cppcms::widgets;

struct Contest: Page {
	struct Info: cppcms::form {
		ws::text name;
	};
	Info info;
//	models::Contest* cnt;
};


struct User: Page {
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

struct Registration: Page {
	struct Info: cppcms::form {
		ws::text name;
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
		
		virtual bool validate() override {
			if(!cppcms::form::validate()) return false;
			if(!isValidUsername(name.value())) {
				name.valid(false);
				name.error_message("Invalid username. Username must contain 1-255 characters.");
				return false;
			}
			return true;
		}
	};
	Info info;
};

struct Login: Page {
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

struct Languages: Page {
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
	std::vector<Language> languages;
	Language newLang;
};

}

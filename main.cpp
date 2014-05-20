#include "common.hpp"
#include "content.hpp"
#include "model.hpp"
#include <booster/log.h>
#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <iostream>
#include <fstream>

using namespace cses;

struct Server: cppcms::application {
	Server(cppcms::service& srv): cppcms::application(srv) {
		dispatcher().assign("/", &Server::contests, this);
		mapper().assign("");
		
		dispatcher().assign("/contest/.+", &Server::contest, this, 1);
		mapper().assign("contest", "contest/{1}");
		
		dispatcher().assign("/user/.*", &Server::user, this, 1);
		mapper().assign("user", "user/{1}");
		
		dispatcher().assign("/register/", &Server::registration, this);
		mapper().assign("register", "register/");
		
		dispatcher().assign("/login/", &Server::login, this);
		mapper().assign("login", "login/");
		
		dispatcher().assign("/languages/", &Server::languages, this);
		mapper().assign("languages", "languages/");
		
		dispatcher().assign("/admin/", &Server::admin, this);
		mapper().assign("admin", "admin/");
		
		dispatcher().assign("/admin/user/(\\d+)/", &Server::admin_user, this, 1);
		mapper().assign("admin_user", "admin/user/{1}/");
		
		mapper().root("/");
	}

#if 0
	void main(string url) override {
//		response().out() << "<html><body>lololol</body></html>\n";
		content::message c;
		c.text = "asd";
		render("message", c);
	}
#endif
	void contests() {
		optional<User> user = getUser();
		if(user) {
			BOOSTER_DEBUG("cses_contests") << "User " << user->id << ": \"" << user->name << "\" visited the page.";
		} else {
			BOOSTER_DEBUG("cses_contests") << "Outsider visited the page.";
		}
//		response().out() << "<html><body>lololol</body></html>\n";
#if 1
		ContestsPage c;
		c.contests = {"a", "b"};
		render("contests", c);
#else
		content::message c;
		c.text = "asd";
		render("message", c);
#endif
	}

	void contest(string cnt) {
		(void)cnt;
		ContestPage c;
#if 0
		c.info.load(context());
		if (c.info.validate()) {
		}
#endif
		render("contest", c);
	}

	void user(string user) {
		(void)user;
		UserPage u;
		render("user", u);
	}
	
	void registration() {
		RegistrationPage c;
		if(isPost()) {
			c.info.load(context());
			if(c.info.validate()) {
				optional<ID> newUserID = registerUser(c.info.name.value(), c.info.password.value());
				if(newUserID) {
					BOOSTER_INFO("cses_register")
						<< "Registered user " << *newUserID << ": \"" << c.info.name.value() << "\".";
				} else {
					c.info.name.valid(false);
					c.info.name.error_message("Username already in use.");
				}
			}
		}
		render("registration", c);
	}

	void login() {
		LoginPage c;
		if(isPost() && session().is_set("prelogin")) {
			c.info.load(context());
			if(c.info.validate()) {
				bool success;
				optional<ID> userID = testLogin(c.info.name.value(), c.info.password.value());
				if(userID) {
					session().reset_session();
					session().erase("prelogin");
					session().set("id", *userID);
					response().set_redirect_header("/");
					return;
				}
			}
		}
		session().set("prelogin", "");
		render("login", c);
	}

	void languages() {
		throw 5;
		LanguagesPage c;
		if (isPost()) {
			c.newLang.load(context());
			if (c.newLang.validate()) {
				Language lang;
				lang.name = c.newLang.name.value();
				lang.suffix = c.newLang.suffix.value();
//				lang.compiler = makeFile(c.newLang.compiler.value());
			}
		}
		render("languages", c);
	}
	
	void admin() {
		optional<User> user = getUser();
		if(!user || !user->admin) {
			return;
		}
		
		AdminPage c;
		using namespace odb;
		transaction t(db->begin());
		result<User> res = db->query<User>();
		c.userlist.assign(res.begin(), res.end());
		render("admin", c);
	}

	void admin_user(string userIDString) {
		std::cout << userIDString << "\n";
	}

	bool isPost() const {
		return const_cast<Server*>(this)->request().request_method() == "POST";
	}
	
	optional<User> getUser() {
		if(session().is_set("id")) {
			ID userID = session().get<ID>("id");
			optional<User> user = getObjectIfExists<User>(userID);
			if(!user) {
				session().reset_session();
				session().erase("id");
			}
			return user;
		}
		return optional<User>();
	}
};

int main() {
	makeDB();
	std::ifstream configFile("config.js");
	cppcms::json::value config;
	int line=0;
	bool ok = config.load(configFile, 1, &line);
	if (!ok) {
		std::cerr<<"Config syntax error on line "<<line<<'\n';
		return 1;
	}
	cppcms::service srv(config);
	srv.applications_pool().mount(cppcms::applications_factory<Server>());
	srv.run();
}

#include "common.hpp"
#include "content.hpp"
#include "model.hpp"
#include "io_util.hpp"
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
		
		dispatcher().assign("/admin/user/(\\d+)/", &Server::adminEditUser, this, 1);
		mapper().assign("adminEditUser", "admin/user/{1}/");
		
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
		optional<User> user = getCurrentUser();
		if(user) {
			BOOSTER_DEBUG("cses_contests") << "User " << user->id << ": \"" << user->getName() << "\" visited the page.";
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
				try {
					odb::transaction t(db->begin());
					User newUser(c.info.name.value(), c.info.password.value());
					db->persist(newUser);
					t.commit();
					BOOSTER_INFO("cses_register")
						<< "Registered user " << newUser.id << ": \"" << newUser.getName() << "\".";
				} catch(odb::object_already_persistent) {
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
				optional<ID> userID = testLogin(c.info.name.value(), c.info.password.value());
				if(userID) {
					session().reset_session();
					session().erase("prelogin");
					session().set("id", *userID);
					response().set_redirect_header("/");
					return;
				} else {
					BOOSTER_INFO("cses_login") << "Login failed with username \"" << c.info.name.value() << "\".\n";
					c.loginFailed = true;
				}
			}
		}
		session().set("prelogin", "");
		render("login", c);
	}

	void languages() {
		throw 5;
		LanguagesPage c;
		if(isPost()) {
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
		if(!isCurrentUserAdmin()) {
			sendRedirectHeader("/");
			return;
		}
		
		AdminPage c;
		odb::transaction t(db->begin());
		odb::result<User> res = db->query<User>();
		c.userlist.assign(res.begin(), res.end());
		render("admin", c);
	}

	void adminEditUser(string userIDString) {
		if(!isCurrentUserAdmin()) {
			sendRedirectHeader("/");
			return;
		}
		
		optional<ID> userID = stringToInteger<ID>(userIDString);
		if(!userID) {
			sendRedirectHeader("/admin");
			return;
		}
		
		optional<User> user = getObjectIfExists<User>(*userID);
		if(!user) {
			sendRedirectHeader("/admin");
			return;
		}
		
		AdminEditUserPage c;
		if(isPost()) {
			c.form.load(context());
			if(c.form.validate()) {
				user->setName(c.form.name.value());
				if(!c.form.password.value().empty()) {
					user->setPassword(c.form.password.value());
				}
				user->setAdmin(c.form.admin.value());
				user->setActive(c.form.active.value());
				try {
					odb::transaction t(db->begin());
					
					// FIXME: find better way to detect username in use.
					// can't reliably detect UNIQUE constraint failure,
					// current workaround throws if someone creates user with
					// colliding name during the transaction.
					odb::result<User> res = db->query<User>(
						odb::query<User>::name == user->getName()
					);
					c.nameInUse = false;
					for(const User& other : res) {
						if(other.id != user->id) c.nameInUse = true;
					}
					
					if(!c.nameInUse) {
						db->update(*user);
						t.commit();
						c.success = true;
					}
				} catch(odb::object_not_persistent& e) {
					sendRedirectHeader("/admin");
					return;
				}
			}
		} else {
			c.form.name.value(user->getName());
			c.form.admin.value(user->isAdmin());
			c.form.active.value(user->isActive());
		}
		
		render("adminEditUser", c);
	}

	bool isPost() const {
		return const_cast<Server*>(this)->request().request_method() == "POST";
	}
	
	// If the user has logged in and is active, return it.
	optional<User> getCurrentUser() {
		if(session().is_set("id")) {
			ID userID = session().get<ID>("id");
			optional<User> user = getObjectIfExists<User>(userID);
			if(user && user->isActive()) {
				return user;
			}
			session().reset_session();
			session().erase("id");
		}
		return optional<User>();
	}
	
	bool isCurrentUserAdmin() {
		optional<User> user = getCurrentUser();
		if(!user) return false;
		return user->isAdmin();
	}
	
	template <typename... Params>
	void sendRedirectHeader(const char* path, const Params&... params) {
		std::stringstream url;
		mapper().map(url, path, params...);
		response().set_redirect_header(url.str());
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

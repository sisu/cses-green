#include "content.hpp"
#include "models.hxx"
#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <iostream>
#include <fstream>
using namespace std;

struct Server: cppcms::application {
	Server(cppcms::service& srv): cppcms::application(srv) {
		dispatcher().assign("/", &Server::contests, this);
		mapper().assign("");

		dispatcher().assign("/contest/.+", &Server::contest, this, 1);
		mapper().assign("contest", "contest/{1}");

		dispatcher().assign("/user/.*", &Server::user, this, 1);
		mapper().assign("user", "user/{1}");

		dispatcher().assign("/login/", &Server::login, this);
		mapper().assign("login", "login/");

		dispatcher().assign("/languages/", &Server::languages, this);
		mapper().assign("languages", "languages/");

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
//		response().out() << "<html><body>lololol</body></html>\n";
#if 1
		content::Contests c;
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
		content::Contest c;
#if 0
		c.info.load(context());
		if (c.info.validate()) {
		}
#endif
		render("contest", c);
	}

	void user(string user) {
		(void)user;
		content::User u;
		render("user", u);
	}

	void login() {
		content::Login c;
		if(isPost() && session().is_set("prelogin")) {
			c.info.load(context());
			if (c.info.validate() && testLogin(c.info.name.value(), c.info.password.value())) {
				session().reset_session();
				session().erase("prelogin");
				session().set("id", c.info.name.id());
				response().set_redirect_header("/");
				return;
			}
		}
		session().set("prelogin", "");
		render("login", c);
	}

	void languages() {
		throw 5;
		content::Languages c;
		if (isPost()) {
			c.newLang.load(context());
			if (c.newLang.validate()) {
				models::Language lang;
				lang.name = c.newLang.name.value();
				lang.suffix = c.newLang.suffix.value();
//				lang.compiler = makeFile(c.newLang.compiler.value());
			}
		}
		render("languages", c);
	}

	bool isPost() const {
		return const_cast<Server*>(this)->request().request_method() == "POST";
	}
};

int main() {
	makeDB();
	ifstream configFile("config.js");
	cppcms::json::value config;
	int line=0;
	bool ok = config.load(configFile, 1, &line);
	if (!ok) {
		cerr<<"Config syntax error on line "<<line<<'\n';
		return 1;
	}
	cppcms::service srv(config);
	srv.applications_pool().mount(cppcms::applications_factory<Server>());
	srv.run();
}

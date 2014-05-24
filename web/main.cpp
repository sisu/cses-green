#include "common.hpp"
#include "content.hpp"
#include "model.hpp"
#include "io_util.hpp"
#include "file.hpp"
#include "judging.hpp"
#include <booster/log.h>
#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_file.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>

using namespace cses;

struct Server: cppcms::application {
	Server(cppcms::service& srv): cppcms::application(srv) {
		dispatcher().assign("/", &Server::contests, this);
		mapper().assign("");
		
		dispatcher().assign("/contest/(\\d+)/", &Server::contest, this, 1);
		mapper().assign("contest", "contest/{1}/");

		dispatcher().assign("/submit/(\\d+)/", &Server::submit, this, 1);
		mapper().assign("submit", "submit/{1}/");

		dispatcher().assign("/view/(\\d+)/", &Server::view, this, 1);
		mapper().assign("view", "view/{1}/");
		
		dispatcher().assign("/user/(\\d*)/", &Server::user, this, 1);
		mapper().assign("user", "user/{1}/");
		
		dispatcher().assign("/register/", &Server::registration, this);
		mapper().assign("register", "register/");
		
		dispatcher().assign("/login/", &Server::login, this);
		mapper().assign("login", "login/");
		
		dispatcher().assign("/admin/", &Server::admin, this);
		mapper().assign("admin", "admin/");
		
		dispatcher().assign("/admin/user/(\\d+)/", &Server::adminEditUser, this, 1);
		mapper().assign("adminEditUser", "admin/user/{1}/");
		
		dispatcher().assign("/admin/language/(\\d+|new)/", &Server::adminEditLanguage, this, 1);
		mapper().assign("adminEditLanguage", "admin/language/{1}/");
                
                dispatcher().assign("/admin/import/", &Server::adminImport, this);
                mapper().assign("adminImport", "admin/import/");
		
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
		
		odb::transaction t(db->begin());		
		odb::result<Contest> contestRes = db->query<Contest>();
		for (auto x : contestRes) {
			c.contests.push_back(make_pair(x.id, x.name));
		}
		
		//c.contests = {"a", "b"};
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
	
	void view(string id) {
		ViewPage c;
		optional<ID> submissionID = stringToInteger<ID>(id);
		render("view", c);
	}
	
	void submit(string id) {
		SubmitPage c;
 		optional<ID> contestID = stringToInteger<ID>(id);
		
		optional<Contest> cnt = getObjectIfExists<Contest>(*contestID);
		
		{
			odb::transaction t(db->begin());
			db->load(*cnt, cnt->sec);
			
			for (auto x : cnt->tasks) {
				c.form.task.add(x->name, std::to_string(x->id));
			}  		
  		
			odb::result<Language> languageRes = db->query<Language>();
			for (auto x : languageRes) {
				c.form.language.add(x.name, std::to_string(x.id));
			}
		}
		
		if (isPost()) {
			c.form.load(context());
			if (c.form.validate()) {
				optional<ID> taskID = stringToInteger<ID>(c.form.task.selected_id());
				optional<ID> languageID = stringToInteger<ID>(c.form.language.selected_id());
				BOOSTER_DEBUG("lol") << c.form.task.selected_id() << " " << c.form.language.selected_id() << " " << c.form.file.value()->name();
				std::istream &is = c.form.file.value()->data();
				is.seekg(0, is.end);
				int len = is.tellg();
				is.seekg(0, is.beg);
				char *buffer = new char[len];
				is.read(buffer, len);
				FileSave saver;
				saver.write(buffer, len);
				delete[] buffer;
				string hash = saver.save();
				unique_ptr<File> codeFile(new File());
				codeFile->hash = hash;
				codeFile->name = c.form.file.value()->name();
				shared_ptr<Submission> submission(new Submission);
				shared_ptr<User> user = getCurrentUserPtr();
				submission->user = user;
				shared_ptr<Task> task = getSharedPtr<Task>(*taskID);
				submission->task = task;
				submission->program.language = getSharedPtr<Language>(*languageID);
				submission->status = SubmissionStatus::PENDING;
				odb::transaction t(db->begin());
				db->persist(*codeFile);
				submission->program.source = move(codeFile);
				db->persist(*submission);
				t.commit();
				addForJudging(submission);
			}
			sendRedirectHeader("/");
			return;
		}
		
  		
  		render("submit", c);
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
	
	void admin() {
		if(!isCurrentUserAdmin()) {
			sendRedirectHeader("/");
			return;
		}
		
		AdminPage c;
		odb::transaction t(db->begin());		
		odb::result<User> userRes = db->query<User>();
		c.users.assign(userRes.begin(), userRes.end());
		
//		odb::result<User> languageRes = db->query<Language>();
//		c.languages.assign(languageRes.begin(), languageRes.end());
		
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
	
	void adminEditLanguage(string langIDString) {
		if(!isCurrentUserAdmin()) {
			sendRedirectHeader("/");
			return;
		}
		
		bool createNew = langIDString == "new";
		
		optional<ID> langID;
		if(!createNew) {
			langID = stringToInteger<ID>(langIDString);
			if(!langID) {
				sendRedirectHeader("/admin");
				return;
			}
		}
		
		optional<Language> lang;
		if(!createNew) {
			optional<Language> lang = getObjectIfExists<Language>(*langID);
			if(!lang) {
				sendRedirectHeader("/admin");
				return;
			}
		}
		
		AdminEditLanguagePage c;
		if(isPost()) {
			c.form.load(context());
			if(c.form.validate()) {
				
			}
		} else if(lang) {
			c.form.name.value(lang->name);
			c.form.suffix.value(lang->suffix);
		}
		
		render("adminEditLanguage", c);
	}
	
        void adminImport() {
                if(!isCurrentUserAdmin()) {
                        sendRedirectHeader("/");
                        return;
                }
                
                AdminImportPage c;
		if (isPost()) {
			c.form.load(context());
			if(c.form.validate()) {
				string contestName = c.form.name.value();
				char directoryName[] = "zipXXXXXX";
				mkdtemp(directoryName);
				string newName = directoryName;				
				c.form.package.value()->save_to(newName + "/pelle.zip");
				string command = "unzip " + newName + "/pelle.zip -d " + newName;
				system(command.c_str());
				DIR *dir = opendir((newName + "/.").c_str());
				struct dirent *d;
				map<string, pair<vector<string>, vector<string>>> data;
				while ((d = readdir(dir)) != NULL) {
					string dirName = d->d_name;
					if (dirName == ".") continue;
					if (dirName == "..") continue;
					if (dirName == "pelle.zip") continue;
					DIR *dir2 = opendir((newName + "/" + dirName + "/.").c_str());
					struct dirent *d2;
					vector<string> inputs, outputs;
					while ((d2 = readdir(dir2)) != NULL) {
						string fileName = d2->d_name;
						if (fileName == ".") continue;
						if (fileName == "..") continue;
						if (fileName.find(".in") != string::npos) inputs.push_back(fileName);
						if (fileName.find(".IN") != string::npos) inputs.push_back(fileName);
						if (fileName.find(".out") != string::npos) outputs.push_back(fileName);
						if (fileName.find(".OUT") != string::npos) outputs.push_back(fileName);
					}
					sort(inputs.begin(), inputs.end());
					sort(outputs.begin(), outputs.end());
					while (outputs.size() < inputs.size()) outputs.push_back("");
					data[dirName] = make_pair(inputs, outputs);
				}
				BOOSTER_INFO("lol") << "apina";
				odb::transaction t(db->begin());
				shared_ptr<Contest> newContest(new Contest());
				newContest->name = contestName;
				//db->persist(newContest.get());				
				for (auto x : data) {
					BOOSTER_INFO("lol") << "banaani";
					shared_ptr<Task> newTask(new Task());
					string taskName = x.first;
					newTask->name = taskName;
					//newTask->contest = newContest;					
					for (size_t i = 0; i < data[x.first].first.size(); i++) {
						BOOSTER_INFO("lol") << "cembalo";
						unique_ptr<File> newInput(new File()), newOutput(new File());
						newInput->name = data[x.first].first[i];
						newOutput->name = data[x.first].second[i];
						FileSave inputSaver, outputSaver;
						
						char *buffer;
						FILE *file;
						int fsize;
						
						file = fopen((newName + "/" + x.first + "/" + newInput->name).c_str(), "rb");
						fseek(file, 0, SEEK_END);
						fsize = ftell(file);
						rewind(file);
						buffer = (char*)malloc(sizeof(char)*fsize);
						fread(buffer, 1, fsize, file);
						fclose(file);
						//inputSaver.write(buffer, fsize);
						free(buffer);

						file = fopen((newName + "/" + x.first + "/" + newOutput->name).c_str(), "rb");
						fseek(file, 0, SEEK_END);
						fsize = ftell(file);
						rewind(file);
						buffer = (char*)malloc(sizeof(char)*fsize);
						fread(buffer, 1, fsize, file);
						fclose(file);
						//outputSaver.write(buffer, fsize);
						free(buffer);

						string inputHash = inputSaver.save();
						string outputHash = outputSaver.save();
						
						newInput->hash = inputHash;
						newOutput->hash = outputHash;
						unique_ptr<TestCase> newCase(new TestCase());
 						db->persist(newInput.get());
 						db->persist(newOutput.get());
						newCase->input = move(newInput);
						newCase->output = move(newOutput);
						newCase->group = 1;
						//newCase->task = newTask;
 						db->persist(newCase.get());
 						newTask->testCases.push_back(move(newCase));
					}
					db->persist(newTask);
					newContest->tasks.push_back(move(newTask));
				}
				db->persist(newContest.get());
				t.commit();
				command = "rm -rf " + newName;
				system(command.c_str());
			}
		}
                render("adminImport", c);
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

	shared_ptr<User> getCurrentUserPtr() {
		if(session().is_set("id")) {
			ID userID = session().get<ID>("id");
			shared_ptr<User> user = getSharedPtr<User>(userID);
			if(user && user->isActive()) {
				return user;
			}
			session().reset_session();
			session().erase("id");
		}
		return {};
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
	updateJudgeHosts();
	cppcms::service srv(config);
	srv.applications_pool().mount(cppcms::applications_factory<Server>());
	srv.run();
}

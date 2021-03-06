#include "common.hpp"
#include "content.hpp"
#include "model.hpp"
#include "io_util.hpp"
#include "file.hpp"
#include "judging.hpp"
#include "import.hpp"
#include <booster/log.h>
#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_file.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <iostream>

using namespace cses;

struct Server: cppcms::application {
	Server(cppcms::service& srv): cppcms::application(srv) {
		dispatcher().assign("/", &Server::wrap<&Server::contests>, this);
		mapper().assign("");
		
		dispatcher().assign("/contest/(\\d+)/", &Server::wrap<&Server::editContest>, this, 1);
		mapper().assign("contest", "contest/{1}/");

		dispatcher().assign("/task/(\\d+)/", &Server::wrap<&Server::editTask>, this, 1);
		mapper().assign("task", "task/{1}/");

		dispatcher().assign("/submit/(\\d+)/", &Server::wrap<&Server::submit>, this, 1);
		mapper().assign("submit", "submit/{1}/");

		dispatcher().assign("/list/(\\d+)/", &Server::wrap<&Server::listSubmissions>, this, 1);
		mapper().assign("list", "list/{1}/");

		dispatcher().assign("/scores/(\\d+)/", &Server::wrap<&Server::scoreBoard>, this, 1);
		mapper().assign("scores", "scores/{1}/");		
		
		dispatcher().assign("/view/(\\d+)/", &Server::wrap<&Server::viewSubmission>, this, 1);
		mapper().assign("view", "view/{1}/");

		dispatcher().assign("/code/(\\d+)/", &Server::wrap<&Server::viewCode>, this, 1);
		mapper().assign("code", "code/{1}/");		
		
		dispatcher().assign("/register/", &Server::wrap<&Server::registration>, this);
		mapper().assign("register", "register/");
		
		dispatcher().assign("/login/", &Server::wrap<&Server::login>, this);
		mapper().assign("login", "login/");

		dispatcher().assign("/logout/", &Server::wrap<&Server::logout>, this);
		mapper().assign("logout", "logout/");

		dispatcher().assign("/languages/", &Server::wrap<&Server::languages>, this);
		mapper().assign("languages", "languages/");
		
		dispatcher().assign("/users/", &Server::wrap<&Server::users>, this);
		mapper().assign("users", "users/");
		
		dispatcher().assign("/user/(\\d+)/", &Server::wrap<&Server::editUser>, this, 1);
		mapper().assign("editUser", "user/{1}/");
		
		dispatcher().assign("/language/(submission|evaluator)/(\\d+|new)/", &Server::wrap<&Server::editLanguage>, this, 1, 2);
		mapper().assign("editLanguage", "language/{1}/{2}/");
		
		dispatcher().assign("/import/", &Server::wrap<&Server::import>, this);
		mapper().assign("import", "import/");

		dispatcher().assign("/static/([a-z_0-9\\.]+)", &Server::wrap<&Server::staticServe>, this, 1);
		mapper().assign("static", "static/{1}");
		
		mapper().root("/");
	}
	
	struct Redirect {
		Redirect(const string& destination) : destination(destination) { }
		string destination;
	};
	
	// Wrapping member function makes it possible to redirect using Redirect
	// exceptions.
	template <void (Server::*Func)()>
	void wrap() {
		try {
			(*this.*Func)();
		} catch(const Redirect& e) {
			sendRedirectHeader(e.destination.c_str());
		}
	}
	template <void (Server::*Func)(string)>
	void wrap(string a) {
		try {
			(*this.*Func)(a);
		} catch(const Redirect& e) {
			sendRedirectHeader(e.destination.c_str());
		}
	}
	template <void (Server::*Func)(string, string)>
	void wrap(string a, string b) {
		try {
			(*this.*Func)(a, b);
		} catch(const Redirect& e) {
			sendRedirectHeader(e.destination.c_str());
		}
	}
	
	void contests() {
		UserPtr user = getRequiredUser();
		
		ContestsPage c(user);
		
		odb::transaction t(db::begin());
		odb::result<Contest> contestRes = db::query<Contest>();
		for(const Contest& x : contestRes) {
			if (!x.active && !user->admin) continue;
			c.contests.push_back(x);
		}
		
		render("contests", c);
	}

	void editContest(string id) {
		UserPtr user = getRequiredAdminUser();
		
		shared_ptr<Contest> cnt = getByStringOrFail<Contest>(id);
		EditContestPage c(user, *cnt);
		if (isPost()) {
			c.form.load(context());
			if (c.form.validate()) {
				c.builder.readForm();
				odb::transaction t(db::begin());
				db::update(cnt);
				t.commit();
			}
		}
		{
			odb::session s;
			odb::transaction t(db::begin());
			db::load(*cnt, cnt->sec);
			for(auto t: cnt->tasks) {
				c.tasks.push_back({t->name, t->id});
			}
		}
		render("editContest", c);
	}

	void editTask(string id) {
		UserPtr user = getRequiredAdminUser();
		
		odb::session ss;
		auto task = getByStringOrFail<Task>(id);
		auto languages = loadAllObjects<EvaluatorLanguage>();
		TaskPage t(user, *task->contest.lock(), *task, languages);
		auto& eval = task->evaluator;
		if (isPost()) {
			t.form.load(context());
			if (t.form.validate()) {
				t.builder.readForm();
				odb::transaction tr(db::begin());
				db::update(task);
				tr.commit();
				BOOSTER_INFO("edit task")<<"got evaluator: "<<task->evaluator.source.hash<<'\n';
				if (!eval.source.hash.empty()) {
					compileEvaluator(task);
				}
			}
		}
		t.compileMessage = eval.source ? eval.binary ? eval.compileMessage : "Compiling..." : "No evaluator";
		render("task", t);
	}

	void listSubmissions(string id) {
		UserPtr user = getRequiredUser();
		
		odb::session s2;

		shared_ptr<Contest> cnt = getByStringOrFail<Contest>(id);
		ID userID = user->id;
		ListPage p(user, *cnt);

		odb::transaction t(db::begin());
		
		db::load(*cnt, cnt->sec);
		
		vector<tuple<long long, int, string, string>> data;
		
		for (auto x : cnt->tasks) {
			ID taskID = x->id;
			
			odb::query<Submission> q (odb::query<Submission>::task == taskID &&
			                          odb::query<Submission>::user == userID);
			odb::result<Submission> sRes = db::query<Submission>(q);
			
			for (auto s : sRes) {
				long long time = s.time;
				string task = x->name;
				SubmissionStatus ss = s.status;
				string status;
				if (ss == SubmissionStatus::PENDING) status = "PENDING";
				if (ss == SubmissionStatus::JUDGING) status = "JUDGING";
				if (ss == SubmissionStatus::COMPILE_ERROR) status = "COMPILE ERROR";
				if (ss == SubmissionStatus::READY) status = std::to_string(s.score);
				if (ss == SubmissionStatus::ERROR) status = "WTF";
				int submissionID = s.id;
				data.push_back(make_tuple(-time, submissionID, task, status));
			}
		}
		sort(data.begin(), data.end());
		p.items.resize(data.size());
		for (size_t i = 0; i < data.size(); i++) {
			p.items[i].id = std::to_string(std::get<1>(data[i]));
			p.items[i].time = formatTime(-std::get<0>(data[i]));
			p.items[i].task = std::get<2>(data[i]);
			p.items[i].status = std::get<3>(data[i]);
		}
		
		render("list", p);
	}

	void scoreBoard(string id) {
		UserPtr user = getRequiredUser();
		
		odb::session s2;
		
		shared_ptr<Contest> cnt = getByStringOrFail<Contest>(id);
		ScoresPage p(user, *cnt);
		
		odb::transaction t(db::begin());
		odb::result<User> users = db::query<User>();

		db::load(*cnt, cnt->sec);
		vector<int> taskIds;
		for(auto task: cnt->tasks) {
			taskIds.push_back(task->id);
			p.tasks.push_back(task->name);
		}

		typedef odb::query<Submission> query;
		odb::result<Submission> submissions = db::query<Submission>(query::task.in_range(taskIds.begin(), taskIds.end()) + "ORDER BY" + query::time);

		map<int, map<int, int>> scores;

		for(auto& sub: submissions) {
			scores[sub.user->id][sub.task->id] = sub.score;
		}

		for(auto& user: users) {
			ScoresPage::Row row;
			row.user = user.name;
			int total = 0;
			auto& uscores = scores[user.id];
			for(auto task: cnt->tasks) {
				ScoresPage::Cell cell;
				if (uscores.count(task->id)) {
					cell.has = 1;
					cell.score = uscores[task->id];
					// TODO: proper color, check judge status etc
					cell.color = cell.score == 0 ? "sol_incorrect" : "sol_correct";
					total += cell.score;
				} else {
					cell.has = 0;
				}
				row.cells.push_back(cell);
			}
			row.score = total;
			p.rows.push_back(row);
		}
		sort(p.rows.begin(), p.rows.end());
		
		p.id = cnt->id;
		render("scores", p);
	}
	
	void viewCode(string id) {
		UserPtr user = getRequiredUser();
		
		odb::session s2;
		
		shared_ptr<Submission> s = getByStringOrFail<Submission>(id);
		shared_ptr<Task> task = s->task;
		CodePage c(user, *task->contest.lock());
		c.ownID = s->id;
		c.taskName = task->name;
		c.code = readFileByHash(s->program.source.hash);
		render("code", c);
	}
	
	void viewSubmission(string id) {
		odb::session s2;

		auto user = getOptionalUser();
		shared_ptr<Submission> s = getByStringOrFail<Submission>(id);
		shared_ptr<Task> task = s->task;
		ViewPage c(user, *task->contest.lock());

		{
			odb::transaction t(db::begin());
			db::load(*task, task->sec);
		}

		odb::transaction t(db::begin());
		
		int cnt = 0, ind = 0;
		
		map<int,int> id2group;
		map<int,int> remaining;
		map<int,string> status;
		map<int,float> time;
		map<int,int> memory;
		map<int,string> color;
		
		c.ownID = s->id;
		c.taskName = task->name;
		
		c.groups.resize(task->testGroups.size());
		for (auto group : task->testGroups) {
			c.groups[ind].number = ind+1;
			c.groups[ind].points = 0;
			c.groups[ind].total = group->points;			
			cnt += group->points;
			c.groups[ind].results.resize(group->tests.size());
			remaining[ind] = group->tests.size();
			for (auto test : group->tests) {
				id2group[test->id] = ind;
				status[test->id] = "NOT AVAILABLE";
				time[test->id] = 0;
				memory[test->id] = 0;
				color[test->id] = "gray";
			}
			ind++;
		}
		c.points = 0;
		c.total = cnt;

 		if (s->status == SubmissionStatus::PENDING) c.status = "PENDING";
 		if (s->status == SubmissionStatus::JUDGING) c.status = "JUDGING";
 		if (s->status == SubmissionStatus::COMPILE_ERROR) c.status = "COMPILE ERROR";
 		if (s->status == SubmissionStatus::READY) c.status = "READY";
 		if (s->status == SubmissionStatus::ERROR) c.status = "ERROR";
		
		odb::query<Result> q (odb::query<Result>::submission == s->id);
		odb::result<Result> sRes = db::query<Result>(q);
 		for (auto x : sRes) {
 			int testId = x.testCase->id;
			time[testId] = x.timeInSeconds;
			memory[testId] = x.memoryInBytes;
			ResultStatus rs = x.status;
			color[testId] = "red";
			if (rs == ResultStatus::CORRECT) {
				status[testId] = "CORRECT";
				remaining[id2group[testId]]--;
				color[testId] = "green";
			} else if (rs == ResultStatus::WRONG_ANSWER) {
				status[testId] = "WRONG ANSWER";
			} else if (rs == ResultStatus::TIME_LIMIT) {
				status[testId] = "TIME LIMIT EXCEEDED";
			} else if (rs == ResultStatus::RUNTIME_ERROR) {
				status[testId] = "RUNTIME ERROR";
			} else if (rs == ResultStatus::INTERNAL_ERROR) {
				status[testId] = "INTERNAL ERROR";
			}
		}
		
		ind = 0;
		for (auto group : task->testGroups) {
			if (remaining[ind] == 0) {
				c.groups[ind].points = group->points;
				c.points += group->points;
			}
			ind++;
		}
 		
 		int i = 0;
 		for (auto group : task->testGroups) {
			int j = 0;
 			for (auto test : group->tests) {
 				c.groups[i].results[j].number = j+1;
				c.groups[i].results[j].timeInSeconds = time[test->id];
				c.groups[i].results[j].memoryInKBytes = memory[test->id];
				c.groups[i].results[j].status = status[test->id];
				c.groups[i].results[j].color = color[test->id];
 				j++;
 			}
 			i++;
 		}
		
		render("view", c);
	}
	
	void submit(string id) {
		odb::session s2;
		
		UserPtr user = getRequiredUser();
		shared_ptr<Contest> cnt = getByStringOrFail<Contest>(id);
		SubmitPage c(user, *cnt);

		{
			odb::transaction t(db::begin());
			db::load(*cnt, cnt->sec);
			
			for(const auto& x : cnt->tasks) {
				c.form.task.add(x->name, std::to_string(x->id));
			}  		
  		
			odb::result<SubmissionLanguage> languageRes = db::query<SubmissionLanguage>();
			for(const auto& x : languageRes) {
				c.form.language.add(x.name, std::to_string(x.id));
			}
		}
		
		if (isPost()) {
			c.form.load(context());
			if (c.form.validate()) {
				auto task = getByStringOrFail<Task>(c.form.task.selected_id());
				auto language = getByStringOrFail<SubmissionLanguage>(c.form.language.selected_id());
				BOOSTER_DEBUG("lol") << c.form.task.selected_id() << " " << c.form.language.selected_id() << " " << c.form.file.value()->name();
				string hash = saveStreamToFile(c.form.file.value()->data());
				MaybeFile codeFile;
				codeFile.hash = hash;
				shared_ptr<Submission> submission(new Submission);
				submission->user = user;
				submission->task = task;
				submission->time = currentTime();
				submission->program.language = language;
				submission->status = SubmissionStatus::PENDING;
				odb::transaction t(db::begin());
				submission->program.source = codeFile;
				db::persist(submission);
				t.commit();
				addForJudging(submission);
			}
			sendRedirectHeader("/list", id);
			return;
		}
		
  		
  		render("submit", c);
	}

	void registration() {
		User newUser;
		RegistrationPage page(getOptionalUser(), newUser);
		page.showForm = 1;
		if(isPost()) {
			page.form.load(context());
			if(page.form.validate()) {
				page.builder.readForm();
				try {
					odb::transaction t(db::begin());
					db::persist(newUser);
					t.commit();
					BOOSTER_INFO("cses_register")
						<< "Registered user " << newUser.id << ": \"" << newUser.name << "\".";
					page.msg = "Registration was successful.";
					page.showForm = 0;
				} catch(odb::object_already_persistent) {
					page.msg = "Username already in use.";
				} catch(const ValidationFailure& e) {
					page.msg = e.msg;
				}
			}
		}
		render("registration", page);
	}

	void login() {
		LoginPage c(getOptionalUser());
		if(isPost() && session().is_set("prelogin")) {
			c.info.load(context());
			if(c.info.validate()) {
				optional<ID> userID = testLogin(c.info.name.value(), c.info.password.value());
				if(userID) {
					session().reset_session();
					session().erase("prelogin");
					session().set("id", *userID);
					//response().set_redirect_header("/");
					sendRedirectHeader("/");
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
	
	void logout() {
		session().erase("id");
		sendRedirectHeader("/");
	}
	
	void languages() {
		UserPtr user = getRequiredAdminUser();
		
		LanguagesPage c(user);
		odb::transaction t(db::begin());		
		
		odb::result<SubmissionLanguage> submissionLanguageRes = db::query<SubmissionLanguage>();
		c.submissionLanguages.assign(submissionLanguageRes.begin(), submissionLanguageRes.end());
		
		odb::result<EvaluatorLanguage> evaluatorLanguageRes = db::query<EvaluatorLanguage>();
		c.evaluatorLanguages.assign(evaluatorLanguageRes.begin(), evaluatorLanguageRes.end());
		
		render("languages", c);
	}
	
	void users() {
		UserPtr user = getRequiredAdminUser();
		
		UsersPage c(user);
		odb::transaction t(db::begin());		
		
		odb::result<User> userRes = db::query<User>();
		c.users.assign(userRes.begin(), userRes.end());
		
		render("users", c);
	}

	void editUser(string userIDString) {
		UserPtr user = getRequiredAdminUser();
		
		UserPtr targetUser = getByStringOrFail<User>(userIDString);
		
		EditUserPage page(user, *targetUser);
		if(isPost()) {
			page.form.load(context());
			if(page.form.validate()) {
				page.builder.readForm();
				try {
					odb::transaction t(db::begin());
					
					if(db::query<User>(
						odb::query<User>::name == targetUser->name &&
						odb::query<User>::id != targetUser->id
					).empty()) {
						db::update(targetUser);
						t.commit();
					} else {
						page.msg = "Username already in use.";
					}
				} catch(odb::object_not_persistent& e) {
					sendRedirectHeader("/users");
					return;
				}
			}
		}
		
		render("editUser", page);
	}
	
	template <typename LanguageT>
	void editLanguage(string langIDString) {
		UserPtr user = getRequiredAdminUser();
		
		optional<ID> langID;
		shared_ptr<LanguageT> lang;
		if(langIDString != "new") {
			lang = getByStringOrFail<LanguageT>(langIDString);
		}
		
		EditLanguagePage c(user);
		if(isPost()) {
			c.form.load(context());
			if(c.form.validate()) {
				try {
					odb::transaction t(db::begin());
					
					// FIXME: find better way to detect name in use.
					odb::result<LanguageT> res = db::query<LanguageT>(
						odb::query<LanguageT>::name == c.form.name.value()
					);
					for(const LanguageT& other : res) {
						if(!langID || other.id != *langID) c.nameInUse = true;
					}
					
					if(!c.nameInUse) {
						LanguageT newLang(
							c.form.name.value(),
							c.form.suffix.value(),
							readRunnerForm(c.form.compilerForm, lang ? &lang->compiler : nullptr),
							readRunnerForm(c.form.runnerForm, lang ? &lang->runner : nullptr));
//						);
						
						if(langID) {
							newLang.id = *langID;
							db::update(newLang);
						} else {
							db::persist(newLang);
						};
						t.commit();
						c.success = true;
					}
				} catch(odb::object_not_persistent& e) {
					sendRedirectHeader("/languages");
					return;
				}
			}
		} else if(lang) {
			c.form.name.value(lang->name);
			c.form.suffix.value(lang->suffix);
			fillRunnerForm(c.form.compilerForm, lang->compiler);
			fillRunnerForm(c.form.runnerForm, lang->runner);
		}
		
		render("editLanguage", c);
	}

	void editLanguage(string type, string langIDString) {
		if(type == "submission") {
			editLanguage<SubmissionLanguage>(langIDString);
		}
		if(type == "evaluator") {
			editLanguage<EvaluatorLanguage>(langIDString);
		}
	}
	
	void import() {
		if(!isCurrentUserAdmin()) {
			sendRedirectHeader("/");
			return;
		}

		ImportPage c(getOptionalUser());
		if (isPost()) {
			c.form.load(context());
			if(c.form.validate()) {
				string contestName = c.form.name.value();
				Import import;
				import.process(c.form.package.value()->data());
				
				odb::transaction t(db::begin());
				shared_ptr<Contest> newContest(new Contest());
				newContest->name = contestName;
				newContest->beginTime = currentTime()+3600;
				newContest->endTime = newContest->beginTime+2*3600;
				newContest->active = 0;
				db::persist(newContest);
				
				auto tasks = import.tasks;
				for (auto task : tasks) {
					shared_ptr<Task> newTask(new Task());
					newTask->name = task;
					
					vector<shared_ptr<TestGroup>> groups;
					bool oneGroup;
					map<string,int> groupOfCase;
					if (import.groups[task].size() == 0) {
						oneGroup = true;
						shared_ptr<TestGroup> newGroup(new TestGroup());
						groups.push_back(newGroup);
						groups[0]->task = newTask;
						groups[0]->points = 100;
					} else {
						oneGroup = false;
						int c = 0;
						for (auto group : import.groups[task]) {
							shared_ptr<TestGroup> newGroup(new TestGroup());
							groups.push_back(newGroup);
							groups[c]->task = newTask;
							groups[c]->points = group.first;
							for (auto newCase : group.second) {
								groupOfCase[newCase] = c;
							}
							c++;
						}
					}
					
// 					shared_ptr<TestGroup> group(new TestGroup());
// 					group->task = newTask;
// 					group->points = 100;
					for (auto group : groups) {
						newTask->testGroups.push_back(group);
					}
					newTask->contest = newContest;
					newTask->evaluator = getDefaultEvaluator();
					db::persist(newTask);
					for (auto group : groups) {
						db::persist(group);
					}

 					vector<pair<string,string>> inputs = import.inputs[task];
 					vector<pair<string,string>> outputs = import.outputs[task];
					int testCount = inputs.size();
					
					for (int i = 0; i < testCount; i++) {
						shared_ptr<TestCase> newCase(new TestCase());
						newCase->input = {inputs[i].first};
						newCase->output = {outputs[i].first};
						newCase->inputName = inputs[i].second;
						newCase->outputName = outputs[i].second;
						if (oneGroup) {
							newCase->group = groups[0];
							db::persist(newCase);
							groups[0]->tests.push_back(newCase);
						} else {
							string iname = inputs[i].second;
							string suffix = "";
							for (int i = iname.size()-1; i >= 0; i--) {
								suffix = iname[i] + suffix;
								if (iname[i] == '.') break;
							}
							int id = groupOfCase[suffix];
							BOOSTER_INFO("lol") << suffix << " " << id << "\n";
							newCase->group = groups[id];
							db::persist(newCase);						
							groups[id]->tests.push_back(newCase);
						}
					}
					newContest->tasks.push_back(newTask);
					compileEvaluator(newTask);
				}
				t.commit();
				sendRedirectHeader("/contest", newContest->id);
				return;
			}
		}
		render("import", c);
	}

	void staticServe(string file) {
		std::ifstream f(("static/" + file).c_str());
		if(!f) {
			response().status(404);
		} else {
			bool isCSS = file.substr(file.size()-4)==".css";
			response().content_type(isCSS ? "text/css" : "application/octet-stream");
			response().out() << f.rdbuf();
		}
	}
	
	bool isPost() const {
		return const_cast<Server*>(this)->request().request_method() == "POST";
	}
	
	UserPtr getOptionalUser() {
		if(session().is_set("id")) {
			ID userID = session().get<ID>("id");
			UserPtr user = getSharedPtr<User>(userID);
			if(user && user->active) {
				return user;
			}
			session().reset_session();
			session().erase("id");
		}
		return nullptr;
	}
	UserPtr getRequiredUser() {
		UserPtr res = getOptionalUser();
		if(!res) throw Redirect("/login");
		return res;
	}
	
	UserPtr getRequiredAdminUser() {
		UserPtr user = getRequiredUser();
		if(!user->admin) throw Redirect("/login");
		return user;
	}
	
	bool isCurrentUserAdmin() {
		UserPtr user = getOptionalUser();
		if(!user) return false;
		return user->admin;
	}
	
	template <typename... Params>
	void sendRedirectHeader(const char* path, const Params&... params) {
		std::stringstream url;
		mapper().map(url, path, params...);
		response().set_redirect_header(url.str());
	}

	static Sandbox readRunnerForm(EditLanguagePage::RunnerForm& form, const Sandbox* old) {
		switch(form.getType()) {
			case Sandbox::DOCKER:
				return DockerImage(form.docker.repository.value(),
						form.docker.imageID.value());
			case Sandbox::PTRACE:
				return PTraceConfig(
						PTraceConfig::SyscallPolicy(atoi(form.ptrace.policy.selected_id().c_str())),
						form.ptrace.allowedCalls.value(),
						getMaybeFile(form.ptrace.runner, old ? &old->ptrace.runner : nullptr));
			default:
				throw Error("Unknown sandbox type.");
		}
	}

	static MaybeFile getMaybeFile(ws::file& file, const MaybeFile* old) {
		if (!file.set()) return old ? *old : MaybeFile();
		MaybeFile res;
		res.hash = saveStreamToFile(file.value()->data());
		return res;
	}

	static void fillRunnerForm(EditLanguagePage::RunnerForm& form, Sandbox sandbox) {
		using std::to_string;
		form.type.selected_id(to_string(sandbox.type));
		form.docker.repository.value(sandbox.docker.repository);
		form.docker.imageID.value(sandbox.docker.imageID);
		form.ptrace.policy.selected_id(to_string(sandbox.ptrace.policy));
		form.ptrace.allowedCalls.value(sandbox.ptrace.allowedSyscalls);
	}
};

int main(int argc, char** argv) {
	bool resetDB = 0;
	bool connectToJudge = 0;
	for(int i=1; i<argc; ++i) {
		string s = argv[i];
		if (s=="-d") resetDB = 1;
		else if (s=="-c") connectToJudge = 1;
		else cerr << "Unknown argument " << s << '\n';
	}
	db::init(resetDB);
	std::ifstream configFile("config.js");
	cppcms::json::value config;
	int line=0;
	bool ok = config.load(configFile, 1, &line);
	if (!ok) {
		std::cerr<<"Config syntax error on line "<<line<<'\n';
		return 1;
	}
	if (connectToJudge) {
		updateJudgeHosts();
	}
	cppcms::service srv(config);
	srv.applications_pool().mount(cppcms::applications_factory<Server>());
	srv.run();
}

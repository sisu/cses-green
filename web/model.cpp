#include "model.hpp"
#include "io_util.hpp"
#include <odb/database.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/schema-catalog.hxx>
#include <openssl/sha.h>
#include <random>
#include <stdexcept>

namespace cses {

using namespace odb::core;

unique_ptr<odb::database> db;

namespace {

uint64_t generateTrueRandom() {
	std::random_device device;
	std::uniform_int_distribution<uint64_t> distribution;
	return distribution(device);
}

//static __thread std::mt19937_64 rng(generateTrueRandom());
static std::mt19937_64 rng(generateTrueRandom());

string computeHash(string pass) {
	string res;
	res.resize(SHA_DIGEST_LENGTH);
	SHA1((const unsigned char*)&pass[0], pass.size(), (unsigned char*)&res[0]);
	return res;
}

string genSalt() {
	string ret;
	std::uniform_int_distribution<int> distribution(33, 126);
	for(int i = 0; i < 16; ++i) {
		ret.push_back(distribution(rng));
	}
	return ret;
}

} // end anonymous namespace

User::User(string name, string password, bool admin, bool active) {
	setName(name);
	setPassword(password);
	setAdmin(admin);
	setActive(active);
}

void User::setName(const string& newName) {
	if(!isValidName(newName)) throw Error("User::setName: Invalid name.");
	name = newName;
}

bool User::isPasswordMatch(const string& cmpPassword) const {
	string cmpHash = computeHash(salt + cmpPassword);
	return cmpHash == hash;
}

void User::setPassword(const string& newPassword) {
	if(!isValidPassword(newPassword)) throw Error("User::setPassword: Invalid password.");
	salt = genSalt();
	hash = computeHash(salt + newPassword);
}

bool User::isValidName(const string& name) {
	size_t codepointCount = countCodePoints(name);
	return codepointCount != 0 && codepointCount <= 255;
}
bool User::isValidPassword(const string& password) {
	size_t codepointCount = countCodePoints(password);
	return codepointCount != 0 && codepointCount <= 255;
}

void makeDB() {
	system("rm -f cses.db");
	db.reset(new odb::sqlite::database("cses.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#if 1
	{
		transaction t(db->begin());
		odb::schema_catalog::create_schema(*db);
		t.commit();
	}
#endif
	try {
		transaction t(db->begin());
		User testUser("a", "a", true);
		db->persist(testUser);
		Language cpp;
		cpp.name = "C++";
		cpp.suffix = "cpp";
		db->persist(cpp);
		Language java;
		java.name = "Java";
		java.suffix = "java";
		db->persist(java);
		JudgeHost host;
		host.name = host.host = "localhost";
		host.port = 9666;
		db->persist(host);
		t.commit();
	} catch(object_already_persistent) { }
}

optional<ID> testLogin(string user, string pass) {
	transaction t(db->begin());
	result<User> res = db->query<User>(query<User>::name == user);
	if(res.empty()) return optional<ID>();
	User u = *res.begin();
	if(u.isActive() && u.isPasswordMatch(pass)) {
		return u.id;
	} else {
		return optional<ID>();
	}
}

}

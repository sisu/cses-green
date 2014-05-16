#include "models.hxx"
#include "models-odb.hxx"
#include <cppcms/encoding.h>
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

static thread_local std::mt19937_64 rng(generateTrueRandom());

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

void makeDB() {
	db.reset(new odb::sqlite::database("cses.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#if 1
	{
		transaction t(db->begin());
		odb::schema_catalog::create_schema(*db);
		t.commit();
	}
#endif
	bool success;
	ID userID;
	tie(success, userID) = registerUser("a", "a");
	if(success) {
		transaction t(db->begin());
		User* user = db->load<User>(userID);
		user->admin = 1;
		db->update(user);
		t.commit();
	}
}

bool isValidUsername(string name) {
	size_t codepointCount = 0;
	if(!cppcms::encoding::valid_utf8(name.data(), name.data() + name.size(), codepointCount)) {
		return false;
	}
	return codepointCount != 0 && codepointCount <= 255;
}

pair<bool, ID> testLogin(string user, string pass) {
	transaction t(db->begin());
	result<User> res = db->query<User>(query<User>::name == user);
	if(res.empty()) return pair<bool, ID>(false, 0);
	User u = *res.begin();
	return make_pair(computeHash(u.salt + pass) == u.hash, u.id);
}

pair<bool, ID> registerUser(string name, string pass) {
	if(!isValidUsername(name)) {
		throw std::invalid_argument("Invalid username passed to registerUser.");
	}
	
	User user;
	user.name = name;
	user.salt = genSalt();
	user.hash = computeHash(user.salt + pass);
	user.admin = 0;
	ID id;
	try {
		transaction t(db->begin());
		id = db->persist(user);
		t.commit();
	} catch(object_already_persistent) {
		return pair<bool, ID>(false, 0);
	}
	return pair<bool, ID>(true, id);
}

pair<bool, User> getUserByID(ID id) {
	transaction t(db->begin());
	result<User> res = db->query<User>(query<User>::id == id);
	if(res.empty()) {
		return make_pair(false, User());
	} else {
		return make_pair(true, *res.begin());
	}
}

}

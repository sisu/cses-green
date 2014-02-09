#include "models.hxx"
#include "models-odb.hxx"
#include <odb/database.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/schema-catalog.hxx>
#include <openssl/sha.h>

using namespace std;
using namespace odb::core;
using namespace models;

unique_ptr<odb::sqlite::database> db;

//int main(int argc, char* argv[]) {
void makeDB() {
	db.reset(new odb::sqlite::database("cses.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#if 1
	{
		transaction t(db->begin());
		odb::schema_catalog::create_schema(*db);
		t.commit();
	}
#endif
	ID id = registerUser("a", "a");
	{
		transaction t(db->begin());
		User* user = db->load<User>(id);
		user->admin = 1;
		db->update(user);
		t.commit();
	}
}

string computeHash(string pass) {
	string res;
	res.resize(SHA_DIGEST_LENGTH);
	SHA1((const unsigned char*)&pass[0], pass.size(), (unsigned char*)&res[0]);
	return res;
}
string genSalt() {
	return "LOL";
}
bool testLogin(string user, string pass) {
	result<User> res = db->query<User>(query<User>::name == user);
	if (res.empty()) return 0;
	User u = *res.begin();
	return computeHash(u.salt + pass) == u.hash;
}
ID registerUser(string name, string pass) {
	User user;
	user.name = name;
	user.salt = genSalt();
	user.hash = computeHash(user.salt + pass);
	user.admin = 0;
	transaction t(db->begin());
	ID id = db->persist(user);
	t.commit();
	return id;
}

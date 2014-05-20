#pragma once
#include "common.hpp"
#include "model_db.hxx"
#include "model_db-odb.hxx"

namespace cses {

void makeDB();
bool isValidUsername(string name);
optional<ID> testLogin(string user, string pass);
optional<ID> registerUser(string name, string pass);

// Get database object with given id if exists.
template <typename T>
optional<T> getObjectIfExists(const typename odb::object_traits<T>::id_type& id) {
	odb::transaction t(db->begin());
	odb::result<User> res = db->query<T>(odb::query<User>::id == id);
	if(res.empty()) {
		return optional<T>();
	} else {
		return *res.begin();
	}
}

//UniqueFile makeFile();

}

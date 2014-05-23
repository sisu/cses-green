#pragma once
#include "common.hpp"
#include "model_db.hxx"
#include "model_db-odb.hxx"

namespace cses {

void makeDB();

// Return ID of active user with given username and password, if exists.
optional<ID> testLogin(string user, string pass);

// Get database object with given id if exists.
template <typename T>
optional<T> getObjectIfExists(const typename odb::object_traits<T>::id_type& id) {
	odb::transaction t(db->begin());
	odb::result<T> res = db->query<T>(odb::query<T>::id == id);
	if(res.empty()) {
		return optional<T>();
	} else {
		return move(*res.begin());
	}
}

//UniqueFile makeFile();

}

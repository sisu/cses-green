#pragma once
#include "common.hpp"
#include "common/io_util.hpp"
#include "model_db.hxx"
#include "model_db-odb.hxx"
#include <stdexcept>

namespace cses {

void makeDB(bool reset);

// Return ID of active user with given username and password, if exists.
optional<ID> testLogin(string user, string pass);

template <typename T>
shared_ptr<T> getSharedPtr(const typename odb::object_traits<T>::id_type& id) {
	odb::transaction t(db->begin());
	odb::result<T> res = db->query<T>(odb::query<T>::id == id);
	if (res.empty()) return nullptr;
	return res.begin().load();
}

template <typename T>
shared_ptr<T> getByStringId(string idString) {
	optional<ID> id = stringToInteger<typename odb::object_traits<T>::id_type>(idString);
	if (!id) return nullptr;
	odb::transaction t(db->begin());
	odb::result<T> res = db->query<T>(odb::query<T>::id == *id);
	if (res.empty()) return nullptr;
	return res.begin().load();
}

struct InvalidID: std::runtime_error {
	InvalidID(string id): std::runtime_error("ID not found: " + id) {}
};

template <typename T>
shared_ptr<T> getByStringOrFail(string id) {
	auto res = getByStringId<T>(id);
	if (res) return res;
	throw InvalidID(id);
}

template<class T>
inline vector<shared_ptr<T>> loadAllObjects() {
	odb::transaction t(db->begin());
	odb::result<T> res = db->query<T>();
	vector<shared_ptr<T>> objects;
	for(auto i=res.begin(); i!=res.end(); ++i) {
		objects.push_back(i.load());
	}
	return objects;
}

//File makeFile();

}

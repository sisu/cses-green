#pragma once
#include "common.hpp"
#include "common/io_util.hpp"
#include "model_db.hxx"
#include "model_db-odb.hxx"
#include <stdexcept>

namespace cses {

// Database access functions. Call init before other functions.
namespace db {

namespace detail {
	// Do not use directly.
	extern unique_ptr<odb::database> database;
}

void init(bool reset);

// odb::transaction has no move semantics, so we return odb::transaction_impl*
// to be passed to the constructor of odb::transaction (as in odb).
odb::transaction_impl* begin();

template <typename T>
odb::result<T> query() {
	return detail::database->query<T>();
}

template <typename T>
odb::result<T> query(const odb::query<T>& q) {
	return detail::database->query<T>(q);
}

template <typename T>
shared_ptr<T> load(ID id) {
	return detail::database->load<T>(id);
}

template <typename T>
void load(T& obj, odb::section& sec) {
	detail::database->load<T>(obj, sec);
}

template <typename T>
void reload(T& obj) {
	detail::database->reload<T>(obj);
}

template <typename T>
void reload(shared_ptr<T> obj) {
	detail::database->reload<T>(obj);
}

// Functions persist and update run the validate-function of T before persisting
// the object, so throw exceptions if the objects contents are invalid to
// prevent persisting.

template <typename T>
ID persist(T& obj) {
	obj.validate();
	return detail::database->persist<T>(obj);
}

template <typename T>
ID persist(shared_ptr<T> obj) {
	obj->validate();
	return detail::database->persist<T>(obj);
}

template <typename T>
void update(T& obj) {
	obj.validate();
	return detail::database->update<T>(obj);
}

template <typename T>
void update(shared_ptr<T> obj) {
	obj->validate();
	return detail::database->update<T>(obj);
}

}

// Return ID of active user with given username and password, if exists.
optional<ID> testLogin(string user, string pass);

template <typename T>
shared_ptr<T> getSharedPtr(const typename odb::object_traits<T>::id_type& id) {
	odb::transaction t(db::begin());
	odb::result<T> res = db::query<T>(odb::query<T>::id == id);
	if (res.empty()) return nullptr;
	return res.begin().load();
}

template <typename T>
shared_ptr<T> getByStringId(string idString) {
	optional<ID> id = stringToInteger<typename odb::object_traits<T>::id_type>(idString);
	if (!id) return nullptr;
	odb::transaction t(db::begin());
	odb::result<T> res = db::query<T>(odb::query<T>::id == *id);
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
	odb::transaction t(db::begin());
	odb::result<T> res = db::query<T>();
	vector<shared_ptr<T>> objects;
	for(auto i=res.begin(); i!=res.end(); ++i) {
		objects.push_back(i.load());
	}
	return objects;
}

//File makeFile();

}

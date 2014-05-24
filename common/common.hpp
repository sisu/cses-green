#pragma once

#include <vector>
#include <exception>
#include <utility>
#include <tuple>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <set>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
//#include <boost/optional.hpp>

namespace cses {

using std::vector;
using std::pair;
using std::make_pair;
using std::tuple;
using std::tie;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::size_t;
using std::move;
using std::swap;
using std::set;
using std::map;
using std::unordered_map;
using std::set;
using std::unordered_set;
using std::cin;
using std::cout;
using std::cerr;
using std::stringstream;

struct Error : std::exception {
public:
	Error(const string& msg) {
		this->msg = "CSES Error: " + msg;
	}
	
	virtual const char* what() const throw() override {
		return msg.c_str();
	}
	
private:
	string msg;
};

class UnpromotableBoolean {
public:
	typedef void (UnpromotableBoolean::*Type)();
	
	static Type falseValue() {
		return 0;
	}
	
	static Type trueValue() {
		return &UnpromotableBoolean::dummy;
	}
	
private:
	void dummy() { }
};

// Optional supporting move semantics, because boost::optional doesn't.
// TODO: optimize
template <typename T>
class optional {
public:
	optional() { }
	optional(const T& t) {
		val.reset(new T(t));
	}
	optional(T&& t) {
		val.reset(new T(move(t)));
	}
	optional(const optional<T>& t) {
		if(t.val) {
			val.reset(new T(*t.val));
		}
	}
	optional(optional<T>&& t) {
		swap(val, t.val);
	}
	optional<T>& operator=(const T& t) {
		val.reset(new T(t));
		return *this;
	}
	optional<T>& operator=(T&& t) {
		val.reset(new T(move(t)));
		return *this;
	}
	optional<T>& operator=(const optional<T>& t) {
		if(t.val) {
			val.reset(new T(*t.val));
		} else {
			val.reset(nullptr);
		}
		return *this;
	}
	optional<T>& operator=(optional<T>&& t) {
		swap(val, t.val);
		return *this;
	}
	
	const T& operator*() const {
		if(!val) throw Error("optional::operator*: Value not set.");
		return *val;
	}
	T& operator*() {
		if(!val) throw Error("optional::operator*: Value not set.");
		return *val;
	}
	const T* operator->() const {
		if(!val) throw Error("optional::operator*: Value not set.");
		return val.get();
	}
	T* operator->() {
		if(!val) throw Error("optional::operator*: Value not set.");
		return val.get();
	}
	
	operator UnpromotableBoolean::Type() {
		if(val) {
			return UnpromotableBoolean::trueValue();
		} else {
			return UnpromotableBoolean::falseValue();
		}
	}
	
private:
	unique_ptr<T> val;
};

}

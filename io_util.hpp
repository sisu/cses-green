#pragma once
#include "common.hpp"
#include <limits>
#include <sstream>

namespace cses {

// Convert string to basic integer of given type if well-formed and in range.
template <typename T>
optional<T> stringToInteger(const string& src) {
	typedef std::numeric_limits<T> limits;
	static_assert(
		limits::is_specialized && limits::is_integer,
		"stringToInteger called for non-integer type."
	);
	if(!limits::is_signed && !src.empty() && src[0] == '-') {
		return optional<T>();
	}
	std::stringstream ss;
	ss << src;
	ss << ' ';
	ss.seekg(0);
	T ret;
	ss >> ret;
	if(!ss.good()) return optional<T>();
	return ret;
}

// Count unicode code points. Throws Error if invalid UTF-8.
size_t countCodePoints(const string& str);

}

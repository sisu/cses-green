#include "io_util.hpp"
#include <cppcms/encoding.h>

namespace cses {

size_t countCodePoints(const string& str) {
	size_t ret = 0;
	if(!cppcms::encoding::valid_utf8(str.data(), str.data() + str.size(), ret)) {
		throw Error("countCodePoints: Invalid UTF-8.");
	}
	return ret;
}

}
#pragma once

#include <vector>
#include <exception>
#include <utility>
#include <tuple>
#include <string>
#include <memory>
#include <cstdlib>
#include <boost/optional.hpp>

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
using boost::optional;

struct Error : std::exception {
public:
	Error(const string& msg) : msg(msg) { }
	
	virtual const char* what() const throw() override {
		return ("CSES Error: " + msg).c_str();
	}
	
private:
	string msg;
};

}

#include <time.hpp>

namespace cses {

long long current_time() {
	return (long long)std::time(NULL);
}

string format_time(long long t) {
	time_t x = (time_t)t;
	string result = std::asctime(std::localtime(&x));
	return result;
}

}

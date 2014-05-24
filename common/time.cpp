#include <time.hpp>

namespace cses {

long long current_time() {
	return (long long)std::time(NULL);
}

string format_time(long long t) {
	time_t x = (time_t)t;
	char buffer[100];
	strftime(buffer, 50, "%F %T", std::localtime(&x));
	string result = buffer;
	return result;
}

}

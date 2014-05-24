#include "judge_interface.hpp"

namespace cses {
namespace judge_interface {

bool isSafeIdentifier(const string& str) {
	if(str.empty() || str.size() > 64) return false;
	for(char c : str) {
		if(!(
			(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '_'
		)) return false;
	}
	return true;
}

bool isValidImageID(const string& str) {
	if(str.size() != 64) return false;
	for(char c : str) {
		if(!(
			(c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'f')
		)) return false;
	}
	return true;
}

}
}
#pragma once
#include "common.hpp"

namespace cses {
namespace judge_interface {

// Judge requires file and repo names to pass this check.
// Safe identifiers consist of a-zA-Z letters, numbers and underscores and
// have length 1-64.
bool isSafeIdentifier(const string& str);

// Judge (and docker) require image IDs to pass this check. Valid image IDs
// consist of 64 numbers and lowercase letters a-f.
bool isValidImageID(const string& str);

}
};
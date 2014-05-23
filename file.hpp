#pragma once
#include "common.hpp"
#include <fstream>
#include <cstdlib>
#include <openssl/sha.h>

namespace cses {

// Process for saving a file.
class FileSave {
public:
	FileSave();
	~FileSave();
	
	void write(const char* data, size_t length);
	
	// Save the file so that it can be opened with openFileByHash. Returns
	// the hash. Should be called only once.
	string save();
	
private:
	void reset();
	
	bool saveCalled;
	SHA_CTX shaCtx;
	std::string tmpfilename;
	std::ofstream tmpfile;
};

// Open file previously stored using FileSave by its hash. The stream is
// returned as a pointer because of GCC bug, but it is never nullptr.
// Parameter is not checked for sanity.
unique_ptr<std::ifstream> openFileByHash(const string& hash);

// Check if file of given hash can be opened with openFileByHash.
// Parameter is not checked for sanity.
bool fileHashExists(const string& hash);

}

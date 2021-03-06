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
	void writeFileContents(const string& filename);
	void writeStream(std::istream& in);
	
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

string saveStringToFile(const string& data);

string saveStreamToFile(std::istream& in);

// Open file previously stored using FileSave by its hash. The stream is
// returned as a pointer because of GCC bug, but it is never nullptr.
// Parameter is not checked for sanity.
unique_ptr<std::ifstream> openFileByHash(const string& hash);

// Get path where file is stored by its hash.
// Parameter is not checked for sanity or existence.
string getFileStoragePath(const string& hash);

// Check if file of given hash can be opened with openFileByHash.
// Parameter is not checked for sanity.
bool fileHashExists(const string& hash);

// Check whether string is a valid file hash.
bool isValidFileHash(const string& str);

string readFile(std::istream& in);

//string readFileByName(const string& name);

string readFileByHash(const string& hash);
}

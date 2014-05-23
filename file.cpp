#include "file.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace cses {

FileSave::FileSave() : saveCalled(false) {
	tmpfile.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
	if(!SHA1_Init(&shaCtx)) {
		throw Error("FileSave::FileSave: SHA1_Init failed.");
	}
	
	mkdir("files", 0700);
	
	char tmpfilenameArray[] = "files/tmp_XXXXXX";
	int fd = mkstemp(tmpfilenameArray);
	if(fd == -1) throw Error("FileSave::FileSave: Creating temporary file failed.");
	close(fd);
	
	tmpfilename = tmpfilenameArray;
	tmpfile.open(tmpfilename, std::ios_base::out | std::ios_base::binary);
}

FileSave::~FileSave() {
	if(tmpfilename != "") {
		unlink(tmpfilename.c_str());
	}
}

void FileSave::write(const char* data, size_t length) {
	if(!SHA1_Update(&shaCtx, data, length)) {
		throw Error("FileSave::write: SHA1_Update failed");
	}
	tmpfile.write(data, length);
}

string FileSave::save() {
	if(saveCalled) throw Error("FileSave::save: Called multiple times.");
	saveCalled = true;
	
	tmpfile.close();
	
	uint8_t rawHash[SHA_DIGEST_LENGTH];
	if(!SHA1_Final(rawHash, &shaCtx)) {
		throw Error("FileSave::save: SHA1_Final failed");
	}
	
	string hash;
	const char* enc = "0123456789abcdef";
	for(size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
		uint8_t byte = rawHash[i];
		hash.push_back(enc[byte >> 4]);
		hash.push_back(enc[byte & 15]);
	}
	
	string filename = "files/" + hash;
	if(rename(tmpfilename.c_str(), filename.c_str()) == -1) {
		throw Error("FileSave::save: Could not move temporary file.");
	} else {
		tmpfilename = "";
	}
	
	return hash;
}

unique_ptr<std::ifstream> openFileByHash(const string& hash) {
	unique_ptr<std::ifstream> ret(new std::ifstream);
	
	string filename = "files/" + hash;
	ret->open(filename);
	
	if(!ret->good()) throw Error("openFileByHash: Could not open file.");
	
	return ret;
}

bool fileHashExists(const string& hash) {
	string filename = "files/" + hash;
	
	struct stat statBuf;
	int res = stat(filename.c_str(), &statBuf);
	if(res == -1) {
		if(errno == ENOENT) {
			return false;
		} else {
			throw Error("fileHashExists: stat returned error other than ENOENT.");
		}
	}
	
	return true;
}

}

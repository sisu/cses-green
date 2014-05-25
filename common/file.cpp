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

void FileSave::writeFileContents(const string& filename) {
	std::ifstream in;
	in.open(filename, std::ios_base::in | std::ios_base::binary);
	if(!in.good()) throw Error("FileSave::writeFileContents: Opening file failed.");
	
	writeStream(in);
}

void FileSave::writeStream(std::istream& in) {
	if(!in.good()) {
		throw Error("FileSave::writeStream: Stream not good().");
	}
	
	const size_t BUFSIZE = 4096;
	
	while(!in.eof()) {
		char buf[BUFSIZE];
		in.read(buf, BUFSIZE);
		if(in.bad() || (in.fail() && !in.eof())) {
			throw Error("FileSave::writeStream: Reading stream failed.");
		}
		write(buf, in.gcount());
	}
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

string saveStringToFile(const string& data) {
	FileSave saver;
	saver.write(&data[0], data.size());
	return saver.save();
}

string saveStreamToFile(std::istream& in) {
	FileSave saver;
	saver.writeStream(in);
	return saver.save();
}

unique_ptr<std::ifstream> openFileByHash(const string& hash) {
	unique_ptr<std::ifstream> ret(new std::ifstream);
	
	string filename = "files/" + hash;
	ret->open(filename);
	
	if(!ret->good()) throw Error("openFileByHash: Could not open file.");
	
	return ret;
}

string getFileStoragePath(const string& hash) {
	return "files/" + hash;
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

bool isValidFileHash(const string& str) {
	if(str.size() != 2 * SHA_DIGEST_LENGTH) return false;
	for(char c : str) {
		if(!(
			(c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'f')
		)) return false;
	}
	return true;
}

string readFile(std::istream& in) {
	in.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
	in.seekg(0,std::ios::end);
	std::streampos length = in.tellg();
	in.seekg(0,std::ios::beg);
	string buffer(length, '\0');
	in.read(&buffer[0],length);
	
	return buffer;
}

string readFileByHash(const string& hash) {
	unique_ptr<std::ifstream> inPtr = openFileByHash(hash);
	return readFile(*inPtr);
}

}

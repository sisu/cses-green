#include "docker.hpp"
#include "file.hpp"
#include "Judge.hpp"
#include <cmath>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace cses {

namespace {

template <typename T>
T withMsg(const string& msg) {
	T ret;
	ret.msg = msg;
	return ret;
}

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

unordered_set<string> imagesPulledCache;

void ensureImagePulled(const string& imageRepository, const string& imageID) {
	if(imagesPulledCache.count(imageID)) return;
	
	// Not in cache, see if we have it and update cache.
	mkdir("tmp", 0700);
	char tmpfilename[] = "tmp/XXXXXX";
	int fd = mkstemp(tmpfilename);
	if(fd == -1) {
		throw Error("Could not create temporary file.");
	}
	close(fd);
	
	string command = string("sudo docker.io images -aq --no-trunc > ") + tmpfilename;
	int status = system(command.c_str());
	if(status == -1) {
		throw Error("Calling system(\"" + command + "\") failed.");
	}
	
	std::ifstream in(tmpfilename, std::ios_base::in | std::ios_base::binary);
	if(!in.good()) throw Error("Opening temporary file failed.");
	
	while(true) {
		char buf[65];
		in.read(buf, 65);
		if(!in.good()) break;
		
		if(buf[64] != '\n') throw Error("Unexpected docker output format.");
		buf[64] = '\0';
		string hash = buf;
		if(!isValidImageID(hash)) throw Error("Unexpected docker output format.");
		
		imagesPulledCache.insert(hash);
	}
	
	unlink(tmpfilename);
	
	if(imagesPulledCache.count(imageID)) return;
	
	// TODO: pull if doesn't exist.
	throw Error("Image ID " + imageID + " from repository " + imageRepository + " missing, too lazy to go get it...");
}

} // end anonymous namespace

unordered_map<string, string> runInDocker(
	const string& imageRepository,
	const string& imageID,
	const unordered_map<string, string>& input,
	double timeLimit,
	uint64_t memoryLimit
) {
	if(!isSafeIdentifier(imageRepository)) {
		throw withMsg<protocol::InvalidDataError>("Image repository is unsafe identifier.");
	}
	
	if(!isValidImageID(imageID)) {
		throw withMsg<protocol::InvalidDataError>("Invalid image ID.");
	}
	
	for(const auto& p : input) {
		if(!isSafeIdentifier(p.first)) {
			throw withMsg<protocol::InvalidDataError>("Input file name is unsafe identifier.");
		}
		if(!isValidFileHash(p.second)) {
			throw withMsg<protocol::InvalidDataError>("Malformed input file hash.");
		}
		if(!fileHashExists(p.second)) {
			throw withMsg<protocol::InvalidDataError>("Input file does not exist.");
		}
	}
	
	if(!std::isfinite(timeLimit)) {
		throw withMsg<protocol::InvalidDataError>("Infinite time limit.");
	}
	if(timeLimit <= 0.0) {
		throw withMsg<protocol::InvalidDataError>("Nonpositive time limit.");
	}
	if(memoryLimit < 512 * 1024) {
		throw withMsg<protocol::InvalidDataError>("Memory limit under 512k.");
	}
	if(memoryLimit > 512 * 1024 * 1024) { // TODO: adapt to server
		throw withMsg<protocol::InvalidDataError>("Memory limit over 512M.");
	}
	
	ensureImagePulled(imageRepository, imageID);
	
	// Create input and output directories.
	mkdir("tmp", 0700);
	
	auto tmpdirFail = []() { throw Error("Could not create temporary directories."); };
	
	char tmpdirnameBuf[] = "tmp/XXXXXX";
	if(mkdtemp(tmpdirnameBuf) == nullptr) tmpdirFail();
	string tmpdirname = tmpdirnameBuf;
	
	string indirName = tmpdirname + "/in";
	if(mkdir(indirName.c_str(), 0700) == -1) tmpdirFail();
	
	string outdirName = tmpdirname + "/out";
	if(mkdir(outdirName.c_str(), 0700) == -1) tmpdirFail();
	
	// Hardlink input files.
	for(const pair<string, string>& file : input) {
		string from = getFileStoragePath(file.second);
		string to = indirName + "/" + file.first;
		if(link(from.c_str(), to.c_str()) == -1) {
			throw Error("Could not hardlink input file.");
		}
	}
	
	// TODO: remove temporary directory.
	
	return unordered_map<string, string>();
}

}

#include "Judge.hpp"
#include "file.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
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

bool isValidContainerID(const string& str) {
	return isValidImageID(str);
}

string getFullPath(string path) {
	char buf[PATH_MAX + 1];
	if(realpath(path.c_str(), buf) == nullptr) {
		throw Error("Resolving absolute path with realpath failed.");
	}
	return string(buf);
}

string readWholeFile(const string& filename) {
	std::ifstream in;
	in.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
	in.open(filename, std::ios_base::in | std::ios_base::binary);
	in.seekg(0,std::ios::end);
	std::streampos length = in.tellg();
	in.seekg(0,std::ios::beg);
	string buffer(length, '\0');
	in.read(&buffer[0],length);
	return buffer;
}

// Run command using system(). If runs with 0 exit status, returns stdout.
// Otherwise throws error. Logs extra stderr output.
string runCommand(const string& command) {
	mkdir("tmp", 0700);
	char stdoutfilename[] = "tmp/XXXXXX";
	int fd = mkstemp(stdoutfilename);
	if(fd == -1) {
		throw Error("runCommand: Could not create temporary file.");
	}
	close(fd);
	char stderrfilename[] = "tmp/XXXXXX";
	fd = mkstemp(stderrfilename);
	if(fd == -1) {
		throw Error("runCommand: Could not create temporary file.");
	}
	close(fd);
	
	string ourCommand = command;
	ourCommand += " > ";
	ourCommand += stdoutfilename;
	ourCommand += " 2> ";
	ourCommand += stderrfilename;
	int res = system(ourCommand.c_str());
	if(res == -1) {
		throw Error("runCommand: Calling system failed.");
	}
	if(WEXITSTATUS(res) != 0) {
		throw Error("runCommand: Command returned nonzero exit status.");
	}
	
	string out = readWholeFile(stdoutfilename);
	string err = readWholeFile(stderrfilename);
	
	if(!err.empty()) {
		cerr << "Stderr output from command \"" << command << "\": "
			<< "\"" << err << "\".\n";
	}
	
	unlink(stdoutfilename);
	unlink(stderrfilename);
	
	return out;
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
	
	string images = runCommand("sudo docker.io images -aq --no-trunc");
	
	if(images.size() % 65 != 0) {
		throw Error("Unexpected docker output format.");
	}
	
	for(size_t i = 0; i < images.size(); i += 65) {
		if(images[i + 64] != '\n') throw Error("Unexpected docker output format.");
		string hash = images.substr(i, 64);
		if(!isValidImageID(hash)) throw Error("Unexpected docker output format.");
		
		imagesPulledCache.insert(hash);
	}
	
	if(imagesPulledCache.count(imageID)) return;
	
	// TODO: pull if doesn't exist.
	throw Error("Image ID " + imageID + " from repository " + imageRepository + " missing, too lazy to go get it...");
}

} // end anonymous namespace

void Judge::run(
	protocol::RunResult& _return,
	const string& token,
	const string& imageRepository,
	const string& imageID,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options
) {
	try {
		if(token != correctToken) {
			throw withMsg<protocol::AuthError>("Invalid token.");
		}
		
		// First validate input.
		if(!isSafeIdentifier(imageRepository)) {
			throw withMsg<protocol::InvalidDataError>("Image repository is unsafe identifier.");
		}
		
		if(!isValidImageID(imageID)) {
			throw withMsg<protocol::InvalidDataError>("Invalid image ID.");
		}
		
		for(const protocol::FileRef& input : inputs) {
			if(!isSafeIdentifier(input.name)) {
				throw withMsg<protocol::InvalidDataError>("Input file name is unsafe identifier.");
			}
			if(!isValidFileHash(input.hash)) {
				throw withMsg<protocol::InvalidDataError>("Malformed input file hash.");
			}
			if(!fileHashExists(input.hash)) {
				throw withMsg<protocol::InvalidDataError>("Input file does not exist.");
			}
		}
		
		if(!std::isfinite(options.timeLimit)) {
			throw withMsg<protocol::InvalidDataError>("Infinite time limit.");
		}
		if(options.timeLimit <= 0.0) {
			throw withMsg<protocol::InvalidDataError>("Nonpositive time limit.");
		}
		if(options.memoryLimitBytes < 512 * 1024) {
			throw withMsg<protocol::InvalidDataError>("Memory limit under 512k.");
		}
		if(options.memoryLimitBytes > 512 * 1024 * 1024) { // TODO: adapt to server
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
		for(const protocol::FileRef& input : inputs) {
			string from = getFileStoragePath(input.hash);
			string to = indirName + "/" + input.name;
			if(link(from.c_str(), to.c_str()) == -1) {
				throw Error("Could not hardlink input file.");
			}
		}
		
		// Start container. TODO: handle full paths containing spaces correctly.
		stringstream containerCommand;
		containerCommand << "sudo docker.io run ";
		containerCommand << "--detach=true ";
		containerCommand << "--networking=false ";
		containerCommand << "--memory=" << options.memoryLimitBytes << "b ";
		containerCommand << "--volume=" << getFullPath(indirName) << ":/cses_judge/input/:ro ";
		containerCommand << "--volume=" << getFullPath(outdirName) << ":/cses_judge/output/ ";
		containerCommand << imageID;
		string containerID = runCommand(containerCommand.str());
		
		if(containerID.size() != 65 || containerID[64] != '\n') {
			throw Error("Unexpected docker output format.");
		}
		containerID.resize(64);
		if(!isValidContainerID(containerID)) {
			throw Error("Unexpected docker output format.");
		}
		
		// After time limit, check exit status and remove it.
		uint64_t sleeptime = options.timeLimit * 1e9;
		struct timespec sleeptimespec;
		sleeptimespec.tv_sec = sleeptime / 1000000000;
		sleeptimespec.tv_nsec = sleeptime % 1000000000;
		nanosleep(&sleeptimespec, nullptr);
		
		string running = runCommand("sudo docker.io inspect --format={{.State.Running}} " + containerID);
		if(running == "false\n") {
			string exitCode = runCommand("sudo docker.io inspect --format={{.State.ExitCode}} " + containerID);
			if(exitCode != "0\n") {
				_return.type = protocol::RunResultType::NONZERO_EXIT_STATUS;
			} else {
				_return.type = protocol::RunResultType::SUCCESS;
			}
		} else {
			_return.type = protocol::RunResultType::TIME_LIMIT_EXCEEDED;
		}
		
		runCommand("sudo docker.io rm --force=true " + containerID);
		
		// Save output files.
		DIR* outdir = opendir(outdirName.c_str());
		if(outdir == nullptr) throw Error("Opening sandbox output directory failed.");
		
		struct dirent* ent;
		while((ent = readdir(outdir)) != nullptr) {
			if(ent->d_type != DT_REG) continue;
			
			string name = ent->d_name;
			if(!isSafeIdentifier(name)) continue;
			
			string fullName = outdirName + "/" + name;
			
			runCommand("sudo chmod 666 " + fullName);
			
			FileSave save;
			save.writeFileContents(fullName);
			
			cses::protocol::FileRef ref;
			ref.name = name;
			ref.hash = save.save();
			
			_return.outputs.push_back(ref);
		}
		
		closedir(outdir);
		
		// TODO: remove temporary directory.
		
	} catch(::apache::thrift::TException& e) {
		throw;
	} catch(std::exception& e) {
		cerr << "Judge::run exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}

}

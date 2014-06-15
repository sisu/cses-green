#include "TempDir.hpp"
#include "common/common.hpp"
#include "common/file.hpp"
#include "common/judge_interface.hpp"
#include <dirent.h>
#include <unistd.h>

namespace cses {
using namespace judge_interface;

TempDir::TempDir() {
	char tmpdirnameBuf[] = "tmp/XXXXXX";
	if(mkdtemp(tmpdirnameBuf) == nullptr) throw Error("Creating temporary directory failed.");
	char* cwd = getcwd(0,0);
	name = string(cwd) + "/" + tmpdirnameBuf;
	free(cwd);
}
TempDir::~TempDir() {
	system(("rm -rf " + name).c_str());
}
void TempDir::saveContents(const std::string& subdir, protocol::RunResult& res) {
	std::string outdirName = name + "/" + subdir;
	DIR* outdir = opendir(outdirName.c_str());
	if(outdir == nullptr) throw Error("Opening sandbox output directory failed.");
	struct dirent* ent;
	while((ent = readdir(outdir)) != nullptr) {
		if(ent->d_type != DT_REG) continue;
		
		string name = ent->d_name;
		cerr<<"try save "<<name<<'\n';
		if(!isSafeIdentifier(name)) continue;
		
		string fullName = outdirName + "/" + name;
		
		string cmd = "sudo chmod 666 " + fullName;
		int err = system(cmd.c_str());
		if(err==-1 || WEXITSTATUS(err) != 0) {
			throw Error("Command failed: " + cmd);
		}
		
		FileSave save;
		cerr<<"saving "<<fullName<<'\n';
		save.writeFileContents(fullName);
		
		cses::protocol::FileRef ref;
		ref.name = name;
		ref.hash = save.save();
		cerr<<"done "<<ref.hash<<'\n';
		
		res.outputs.push_back(ref);
	}
	
	closedir(outdir);
}
void TempDir::hardlinkInputs(const vector<protocol::FileRef>& inputs) {
	for(const protocol::FileRef& input : inputs) {
		string from = getFileStoragePath(input.hash);
		string to = name + "/" + input.name;
		cerr<<"Linkin "<<from<<" to "<<to<<'\n';
		if(link(from.c_str(), to.c_str()) == -1) {
			throw Error("Could not hardlink input file.");
		}
	}
}

}

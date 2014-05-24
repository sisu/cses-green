#include "Judge.hpp"
#include "file.hpp"
#include "docker.hpp"

namespace cses {

namespace {
	template <typename T>
	T withMsg(const string& msg) {
		T ret;
		ret.msg = msg;
		return ret;
	}
}

bool Judge::hasFile(const string& token, const string& hash) {
	try {
		if(token != correctToken) {
			throw withMsg<protocol::AuthError>("Invalid token.");
		}
		if(!isValidFileHash(hash)) {
			throw withMsg<protocol::InvalidDataError>("Malformed hash.");
		}
		
		return fileHashExists(hash);
	} catch(std::exception& e) {
		std::cerr << "Judge::hasFile exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}
void Judge::sendFile(const string& token, const string& data) {
	try {
		if(token != correctToken) {
			throw withMsg<protocol::AuthError>("Invalid token.");
		}
		
		FileSave save;
		save.write(data.data(), data.size());
		save.save();
	} catch(std::exception& e) {
		std::cerr << "Judge::sendFile exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}
void Judge::getFile(
	string& _return,
	const string& token,
	const string& hash
) {
	try {
		if(token != correctToken) {
			throw withMsg<protocol::AuthError>("Invalid token.");
		}
		if(!isValidFileHash(hash)) {
			throw withMsg<protocol::InvalidDataError>("Malformed hash.");
		}
		if(!fileHashExists(hash)) {
			throw withMsg<protocol::InvalidDataError>("File does not exist.");
		}
		
		vector<char> buf;
		
		unique_ptr<std::ifstream> in = openFileByHash(hash);
		in->exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
		in->seekg(0, std::ios::end);
		buf.resize(in->tellg());
		in->seekg(0, std::ios::beg);
		in->read(buf.data(), buf.size());
		
		_return.assign(buf.begin(), buf.end());
	} catch(std::exception& e) {
		std::cerr << "Judge::getFile exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}

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
		
		unordered_map<string, string> inputMap;
		for(const protocol::FileRef& input : inputs) {
			inputMap[input.name] = input.hash;
		}
		
		if(options.memoryLimitBytes < 0) {
			throw withMsg<protocol::InvalidDataError>("Negative memory limit.");
		}
		
		unordered_map<string, string> result = runInDocker(
			imageRepository,
			imageID,
			inputMap,
			options.timeLimit,
			options.memoryLimitBytes
		);
		
	} catch(std::exception& e) {
		std::cerr << "Judge::run exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}

}

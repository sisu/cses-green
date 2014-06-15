#include "run_ptrace.hpp"
#include "run_docker.hpp"
#include "Judge.hpp"
#include "file.hpp"

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
			throw withMsg<protocol::InvalidDataError>("Malformed hash " + hash);
		}
		
		return fileHashExists(hash);
	} catch(::apache::thrift::TException& e) {
		throw;
	} catch(std::exception& e) {
		cerr << "Judge::hasFile exception: " << e.what() << "\n";
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
	} catch(::apache::thrift::TException& e) {
		throw;
	} catch(std::exception& e) {
		cerr << "Judge::sendFile exception: " << e.what() << "\n";
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
		_return = readFileByHash(hash);
	} catch(::apache::thrift::TException& e) {
		throw;
	} catch(std::exception& e) {
		cerr << "Judge::getFile exception: " << e.what() << "\n";
		throw protocol::InternalError();
	}
}

void Judge::run(
	protocol::RunResult& _return,
	const string& token,
	const protocol::Sandbox& sandbox,
	const vector<protocol::FileRef>& inputs,
	const protocol::RunOptions& options
) {
	try {
		if(token != correctToken) {
			throw withMsg<protocol::AuthError>("Invalid token.");
		}
		if (sandbox.__isset.docker) {
			runDocker(_return, sandbox.docker.repository, sandbox.docker.id, inputs, options);
		} else if (sandbox.__isset.ptrace) {
			runPTrace(_return, sandbox.ptrace, inputs, options);
		} else {
			cerr << "Unknown sandbox type.\n";
			throw protocol::InternalError();
		}
	} catch(std::exception& e) {
		cerr << "Judge::run exception: " << e.what() << "\n";
		protocol::InternalError err;
		err.msg = "secret";
		throw err;
	}
}

}

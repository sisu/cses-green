#include "Judge.hpp"
#include "file.hpp"

namespace cses {

bool Judge::hasFile(const string& token, const string& hash) {
	try {
		if(token != correctToken) throw protocol::AuthError();
		if(!isValidFileHash(hash)) throw protocol::InvalidDataError();
		
		return fileHashExists(hash);
	} catch(std::exception& e) {
		throw protocol::InternalError();
	}
}
void Judge::sendFile(const string& token, const string& data) {
	try {
		if(token != correctToken) throw protocol::AuthError();
		
		FileSave save;
		save.write(data.data(), data.size());
		save.save();
	} catch(std::exception& e) {
		throw protocol::InternalError();
	}
}
void Judge::getFile(
	string& _return,
	const string& token,
	const string& hash
) {
	try {
		if(token != correctToken) throw protocol::AuthError();
		if(!isValidFileHash(hash)) throw protocol::InvalidDataError();
		
		vector<char> buf;
		
		unique_ptr<std::ifstream> in = openFileByHash(hash);
		in->exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
		in->seekg(0, std::ios::end);
		buf.resize(in->tellg());
		in->seekg(0, std::ios::beg);
		in->read(buf.data(), buf.size());
		
		_return.assign(buf.begin(), buf.end());
	} catch(std::exception& e) {
		throw protocol::InternalError();
	}
}
void Judge::run(
	protocol::RunResult& _return,
	const string& token,
	const string& imageRepository,
	const string& imageID,
	const vector<protocol::FileRef> & inputs,
	const protocol::RunOptions& options
) {
}

}

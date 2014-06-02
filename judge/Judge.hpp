#include "common.hpp"
#include "gen-cpp/Judge.h"

namespace cses {

class Judge: public protocol::JudgeIf {
public:
	Judge(const string& authToken) : correctToken(authToken) { }
	
	virtual bool hasFile(const string& token, const string& hash) override;
	virtual void sendFile(const string& token, const string& data) override;
	
	virtual void getFile(
		string& _return,
		const string& token,
		const string& hash
	) override;
	
	virtual void run(
		protocol::RunResult& _return,
		const string& token,
		const protocol::Sandbox& sandbox,
		const vector<protocol::FileRef>& inputs,
		const protocol::RunOptions& options
	) override;
	
private:
	string correctToken;
};

}

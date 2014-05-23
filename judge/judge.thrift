namespace cpp cses.protocol

struct FileRef {
	1:string hash,
	2:string name,
}

struct RunOptions {
	1:double timeLimit,
	2:i32 memoryLimitBytes,
}

struct RunResult {
	1:list<FileRef> outputs,
	2:i32 status,
}

service Judge {
	bool hasFile(1:string token, 2:string hash),
	void sendFile(1:string token, 2:string data),
	string getFile(1:string token, 2:string hash),

	RunResult run(1:string token, 2:string image, 3:list<FileRef> inputs, 4:RunOptions options),
}

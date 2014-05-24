namespace cpp cses.protocol

struct FileRef {
	1:string hash,
	2:string name,
}

struct RunOptions {
	1:double timeLimit,
	2:i64 memoryLimitBytes,
}

enum RunResultType {
	SUCCESS,
	NONZERO_EXIT_STATUS,
	TIME_LIMIT_EXCEEDED,
	DISALLOWED_DISK_IO,
}

struct RunResult {
	1:RunResultType type,
	2:list<FileRef> outputs,
}

exception InternalError {
	1:string msg,
}
exception InvalidDataError {
	1:string msg,
}
exception AuthError {
	1:string msg,
}
exception DockerError {
	1:string msg,
}

service Judge {
	bool hasFile(1:string token, 2:string hash)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DockerError d),
	void sendFile(1:string token, 2:string data)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DockerError d),
	string getFile(1:string token, 2:string hash)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DockerError d),
	
	RunResult run(1:string token, 2:string imageRepository, 3:string imageID, 4:list<FileRef> inputs, 5:RunOptions options)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DockerError d),
}

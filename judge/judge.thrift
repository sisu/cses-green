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

exception InternalError { }
exception InvalidDataError { }
exception AuthError { }
exception DoesNotExistError { }

service Judge {
	bool hasFile(1:string token, 2:string hash)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DoesNotExistError d),
	void sendFile(1:string token, 2:string data)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DoesNotExistError d),
	string getFile(1:string token, 2:string hash)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DoesNotExistError d),
	
	RunResult run(1:string token, 2:string imageRepository, 3:string imageID, 4:list<FileRef> inputs, 5:RunOptions options)
		throws (1:InternalError a, 2:InvalidDataError b, 3:AuthError c, 4:DoesNotExistError d),
}

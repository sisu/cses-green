#include "common.hpp"
#include "Judge.hpp"
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>




#include "docker.hpp"
#include "file.hpp"



using namespace cses;

int main() {
	unordered_map<string, string> input;
	{
		FileSave save;
		save.write("lollero", 7);
		input["plorts"] = save.save();
	}
	runInDocker("fasd", "99ec81b80c55d906afd8179560fdab0ee93e32c52053816ca1d531597c1ff48f", input, 1.5, 1500000);
	
	using namespace apache::thrift;
	using namespace apache::thrift::protocol;
	using namespace apache::thrift::transport;
	using namespace apache::thrift::concurrency;
	using namespace apache::thrift::server;
	
	boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
	boost::shared_ptr<Judge> judge(new Judge("uolevi"));
	boost::shared_ptr<TProcessor> processor(new cses::protocol::JudgeProcessor(judge));
	boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(9090));
	boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
	
	boost::shared_ptr<ThreadManager> threadManager =
		ThreadManager::newSimpleThreadManager(16);
	boost::shared_ptr<PosixThreadFactory> threadFactory =
		boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
	threadManager->threadFactory(threadFactory);
	threadManager->start();
	
	TThreadPoolServer server(
		processor,
		serverTransport,
		transportFactory,
		protocolFactory,
		threadManager
	);
	
	server.serve();
}

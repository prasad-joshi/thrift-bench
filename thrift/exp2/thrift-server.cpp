#include "Benchmark.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <cassert>
#include <chrono>
#include <boost/algorithm/string.hpp>

#include "gflags/gflags.h"
#include "google/gflags.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using boost::shared_ptr;
using namespace  ::example;

using namespace google;
using std::string;
using std::cout;

class BenchmarkHandler : virtual public BenchmarkIf {
private:
	void copy_benchmark_data(BenchmarkData &dest, const BenchmarkData &src) {
		dest.request_id = src.request_id;
		dest.client_latency = src.client_latency;
		dest.db_latency = src.db_latency;
		dest.dict_latency = src.dict_latency;
		dest.leaf_latency = src.leaf_latency;
		dest.io_latency = src.io_latency;
		dest.bypass_dict = src.bypass_dict;
		dest.bypass_cpu = src.bypass_cpu;
		dest.bypass_leaf = src.bypass_leaf;
		dest.bypass_io = src.bypass_io;

		// dest.data = src->data;
		dest.data_size = src.data_size;
		dest.empty_data_on_resp = src.empty_data_on_resp;
	}
private:
	size_t data_size;
	string data;
	void init_data(size_t size, string &ret_data) {
		if (data_size != size) {
			data = string(size, 'A');
			data_size = size;
		}
		ret_data = data;
	}

public:
	BenchmarkHandler() : data_size(0) {
	}

	void key_put(BenchmarkData& _return, const std::string& key, const BenchmarkData& bd) {
		auto s = std::chrono::high_resolution_clock::now();

		copy_benchmark_data(_return, bd);

		_return.data.clear();
		_return.data_size = 0;

		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();
		_return.db_latency = d;
	}

	void key_get(BenchmarkData& _return, const std::string& key, const BenchmarkData& bd) {
		// cout << "key_get called.\n";
		auto s = std::chrono::high_resolution_clock::now();

		copy_benchmark_data(_return, bd);

		init_data(_return.data_size, _return.data);

		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();
		_return.db_latency = d;
	}
};

static bool validateServerType(const char *flagname, string value) {
	boost::algorithm::to_lower(value);
	if (value == "nonblocking" || value == "threaded" || value == "threadpool") {
		return true;
	}
	return false;
}

DEFINE_int32(nthreads, 32, "Number of thread server should use.");
DEFINE_int32(server_port, 9090, "Server Port");
DEFINE_string(server_type, "nonblocking", "Type of server either nonblocking, threaded, threadpool");
//DEFINE_validator(port, &validateServerType);

int main(int argc, char **argv) {
	google::ParseCommandLineFlags(&argc, &argv, true);

	int port      = FLAGS_server_port;
	auto nthreads = FLAGS_nthreads;
	auto stype    = FLAGS_server_type;
	boost::algorithm::to_lower(stype);

    shared_ptr<BenchmarkHandler>  handler(new BenchmarkHandler());
    shared_ptr<TProcessor>        processor(new BenchmarkProcessor(handler));
    shared_ptr<TServerTransport>  transport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    shared_ptr<TProtocolFactory>  protocolFactory(new TBinaryProtocolFactory());
    shared_ptr<ThreadManager>     threadManager = ThreadManager::newSimpleThreadManager(nthreads);
	shared_ptr<ThreadFactory>     threadFactory(new PosixThreadFactory());

	if (stype != "threaded") {
		threadManager->threadFactory(threadFactory);
		threadManager->start();
	}

	TServer *server = nullptr;
	if (stype == "nonblocking") {
		server = new TNonblockingServer(processor, protocolFactory, port, threadManager);
	} else if (stype == "threaded") {
		server = new TThreadedServer(processor, transport, transportFactory, protocolFactory);
	} else if (stype == "threadpool") {
		server = new TThreadPoolServer(processor, transport, transportFactory, protocolFactory, threadManager);
	} else {
		assert(0);
	}

	server->serve();
	return 0;
}

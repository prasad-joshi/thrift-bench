#include "Benchmark.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <gflags/gflags.h>
#include <google/gflags.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace  ::example;

using std::string;
using namespace google;

DEFINE_int32(server_port, 9090, "Server Port");
DEFINE_string(server_ip, "", "Server IP");

void start_benchmark(uint64_t nrequests, uint32_t req_size) {
	boost::shared_ptr<TTransport> socket(new TSocket(FLAGS_server_ip, FLAGS_server_port));
	boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	BenchmarkClient client(protocol);

	try {
		transport->open();
		string key("1");
		string data(req_size, 'A');

		auto s = std::chrono::high_resolution_clock::now();
		for (auto i = 0; i < nrequests; i++) {
			BenchmarkData bd{};
			bd.data_size = req_size;
			bd.data      = data;
			client.key_put(bd, key, bd);
			assert(bd.data.length() == 0);
		}
		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();
		//cout << "Requests, Total Time, Avg Latency\n";
		//cout << nrequests << "," << d << "," << d / nrequests << endl;
		transport->close();
	} catch (TException& tx) {
		cout << "ERROR: " << tx.what() << endl;
	}
}

int main(int argc, char *argv[]) {
	google::ParseCommandLineFlags(&argc, &argv, true);

	const auto nrequests = 10000;
	std::vector<uint32_t> nthreads{1, 10, 100, 1000, 10000, 100000};
	std::vector<uint32_t> req_size{1, 128, 512, 1024, 4096, 8192, 32*1024, 64*1024, 128*1024};//, 1024*1024};

	cout << "#Threads, ReqSize, Requests/Thread, Avg Latency, #TotRequest, TotAvgLatency\n";
	for (auto nt : nthreads) {
		for (auto rs : req_size) {
			std::thread *tp = new std::thread[nt];
			assert(tp);

			// cout << "Using #threads " << nt << " Request Size " << rs << endl;
			auto s = std::chrono::high_resolution_clock::now();
			for (auto i = 0; i < nt; i++) {
				tp[i] = std::thread(start_benchmark, nrequests, rs);
			}

			for (auto i = 0; i < nt; i++) {
				tp[i].join();
			}

			auto e = std::chrono::high_resolution_clock::now();
			auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();

			cout << nt << "," << rs << "," << nrequests << "," << d/nrequests << ","
				 << nrequests*nt << "," << d/(nrequests*nt) << endl;

			delete[] tp;
			tp = nullptr;
		}
	}
}
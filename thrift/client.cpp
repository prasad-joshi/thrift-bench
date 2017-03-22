#include "Benchmark.h"

#include <iostream>
#include <chrono>

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

int main(int argc, char *argv[]) {
	google::ParseCommandLineFlags(&argc, &argv, true);

	boost::shared_ptr<TTransport> socket(new TSocket(FLAGS_server_ip, FLAGS_server_port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	BenchmarkClient client(protocol);

	try {
		transport->open();
		string key("1");
		string data(4096, 'A');
		const auto nrequests = 100000ull;

		auto s = std::chrono::high_resolution_clock::now();
		for (auto i = 0; i < nrequests; i++) {
			BenchmarkData bd{};
			bd.data_size = 4096;
			bd.data      = data;
			client.key_put(bd, key, bd);
			assert(bd.data.length() == 0);
		}
		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();
		cout << "Requests, Total Time, Avg Latency\n";
		cout << nrequests << "," << d << "," << d / nrequests << endl;
		transport->close();
	} catch (TException& tx) {
		cout << "ERROR: " << tx.what() << endl;
	}
}
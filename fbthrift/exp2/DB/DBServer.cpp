/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "gen-cpp2/DBBenchmark.h"
#include "../LEAF/gen-cpp2/LeafBenchmark.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <chrono>

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace std;
using namespace apache::thrift;
using namespace  ::example;
using namespace  ::example::cpp2;
using namespace folly;

using apache::thrift::async::TAsyncSocket;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::make_shared;
using std::make_unique;

class Leaf : public CloseCallback {
private:
	uint32_t port_;
	string   ip_;
	bool     connected = false;

	unique_ptr<LeafBenchmarkAsyncClient> client_;
public:
	void channelClosed() {
		setDisconnected();
	}

	Leaf(const string &ip, const uint32_t port) : ip_(ip), port_(port) {
	}

	bool isConnected(void) const {
		return connected;
	}

	void connect(EventBase *basep) {
		assert(!client_ && !connected && basep && basep->isInEventBaseThread());

		auto socket{TAsyncSocket::newSocket(basep, ip_, port_)};
		auto channel{HeaderClientChannel::newChannel(socket)};
		channel->setCloseCallback(this);
		client_   = make_unique<LeafBenchmarkAsyncClient>(std::move(channel));
		connected = true;
	}

	void setDisconnected(void) {
		client_   = nullptr;
		connected = false;
	}

	LeafBenchmarkAsyncClient *getConnection() {
		return client_.get();
	}
};

class DBHandler : virtual public DBBenchmarkSvIf {
private:
	const vector<unique_ptr<Leaf>> &leafs_;
	uint32_t curLeaf = 0;
private:
	void copy_benchmark_data(DBBenchmarkData &dest, const unique_ptr<DBBenchmarkData> &src) {
		dest.request_id = src->request_id;
		dest.db_latency = src->db_latency;
		dest.leaf_latency = src->leaf_latency;
		dest.io_latency = src->io_latency;
		dest.cpu_consumed = src->cpu_consumed;

		// dest.data = src->data;
		dest.data_size = src->data_size;
		dest.empty_data_on_resp = src->empty_data_on_resp;
	}

	Leaf *getLeaf() {
		curLeaf = (curLeaf + 1) % leafs_.size();
		return leafs_[curLeaf].get();
	}
public:
	DBHandler(const vector<unique_ptr<Leaf>> &leafs) : leafs_(leafs) {
	}

	void async_tm_key_put(unique_ptr<HandlerCallback<unique_ptr<DBBenchmarkData>>> cb, 
				unique_ptr<string> key, unique_ptr<DBBenchmarkData> bench) {

		auto s      = std::chrono::high_resolution_clock::now();
		auto leafp  = getLeaf();
		auto *basep = cb->getEventBase();
		basep->runInEventBaseThread([cb1 = std::move(cb), bench1 = std::move(bench),
					k = std::move(key), leafp, s] ()
					mutable {

			if (!leafp->isConnected()) {
				leafp->connect(cb1->getEventBase());
			}
			auto clientp = leafp->getConnection();
			assert(clientp);

			/* invoke operation on leaf */
			LeafBenchmarkData lbd;
			lbd.set_request_id(bench1->get_request_id());
			lbd.set_data(bench1->get_data());
			ldb.set_data_size(bench1->get_data_size());
			auto f = clientp->future_key_put(*k, lbd);
			f.then([cb = std::move(cb1), bench = std::move(bench1), s]
						(const LeafBenchmarkData &result) mutable {
				auto e = std::chrono::high_resolution_clock::now();
				auto l = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();

				assert(result.get_request_id() == bench->get_request_id());
				DBBenchmarkData _ret;
				_ret.set_request_id(result.get_request_id());
				_ret.data.clear();
				_ret.data_size = 0;
				_ret.set_db_latency(l);
				_ret.set_leaf_latency(result.get_leaf_latency());
				_ret.set_io_latency(result.get_io_latency());
				_ret.set_cpu_consumed(result.get_cpu_consumed());
				cb->result(_ret);
			});
		});
	}

#if 0
	void async_tm_key_get(unique_ptr<HandlerCallback<unique_ptr<BenchmarkData>>> callback, unique_ptr<string> key, unique_ptr<BenchmarkData> bench) {
		auto s = std::chrono::high_resolution_clock::now();

		BenchmarkData _ret;
		copy_benchmark_data(_ret, bench);

		init_data(_ret.data_size, _ret.data);

		auto e = std::chrono::high_resolution_clock::now();
		auto l = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();
		_ret.set_db_latency(l);
		callback->result(_ret);
	}
#endif
};

DEFINE_int32(server_port, 9090, "Server Port");
DEFINE_string(leaf_server, "127.0.0.1:9091", "List of Leaf Servers");

vector<string> split(const string &input, const char delim) {
	size_t s = 0u;
	size_t e = 0u;
	std::vector<string> v;
	while ((e = input.find(delim, s)) != string::npos) {
		if (e != s) {
			v.push_back(input.substr(s, e - s));
			s = e + 1;
		}
	}
	if (e != s) {
		v.push_back(input.substr(s));
	}
	return v;
}

int main(int argc, char **argv) {
	folly::init(&argc, &argv, true);

	auto token = split(FLAGS_leaf_server, ',');
	if (!token.size()) {
		throw std::invalid_argument("Please provide list of leaf servers and their port\n");
	}

	std::vector<unique_ptr<Leaf>> leafs;
	for (auto &t : token) {
		auto e = t.find(':');
		if (e == string::npos) {
			throw std::invalid_argument("Incorrect leaf IP or Port.\n");
		}

		auto ip  = t.substr(0, e);
		auto port = std::stoul(t.substr(e+1));
		leafs.emplace_back(make_unique<Leaf>(ip, port));
	}

	auto handler = make_shared<DBHandler>(leafs);
	auto server = make_shared<ThriftServer>();
	server->setInterface(handler);
	server->setPort(FLAGS_server_port);

	printf("Starting the server...\n");
	server->serve();
	printf("Done\n");

	return 0;
}

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
	Leaf *getLeaf() {
		curLeaf = (curLeaf + 1) % leafs_.size();
		return leafs_[curLeaf].get();
	}
public:
	DBHandler(const vector<unique_ptr<Leaf>> &leafs) : leafs_(leafs) {
	}

	void async_tm_write(unique_ptr<HandlerCallback<void>> cb,
					unique_ptr<string> filename, unique_ptr<string> data) {

		auto *basep = cb->getEventBase();
		basep->runInEventBaseThread([cb1 = std::move(cb), k = std::move(filename),
					d = std::move(data), this] () mutable {

			std::vector<Future<Unit>> v;

			for (auto &e : this->leafs_) {
				if (!e->isConnected()) {
					e->connect(cb1->getEventBase());
				}
				auto clientp = e->getConnection();
				assert(clientp);

				v.emplace_back(clientp->future_write(*k, *d));
			}

			auto f = collectAll(v);
			f.then([cb = std::move(cb1)] () mutable {
				cb->done();
			});
		});
	}

	void async_tm_read(unique_ptr<HandlerCallback<unique_ptr<DBBenchmarkData>>> cb,
				int64_t request_id, unique_ptr<string> filename, int32_t size) {

		auto s      = std::chrono::high_resolution_clock::now();
		auto leafp  = getLeaf();
		auto *basep = cb->getEventBase();
		basep->runInEventBaseThread([cb1 = std::move(cb), fn = std::move(filename),
				size, leafp, reqid = request_id, s] () mutable {

			if (!leafp->isConnected()) {
				leafp->connect(cb1->getEventBase());
			}
			auto clientp = leafp->getConnection();
			assert(clientp);

			/* invoke operation on leaf */
			auto f = clientp->future_read(*fn, size);
			f.then([cb = std::move(cb1), reqid, s] (const LeafBenchmarkData &result) mutable {
				auto e = std::chrono::high_resolution_clock::now();
				auto l = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();

				DBBenchmarkData _ret;
				_ret.set_request_id(reqid);
				_ret.set_data(result.get_data());
				_ret.set_leaf_latency(l);
				_ret.set_io_latency(result.get_io_latency());
				_ret.set_cpu_consumed(result.get_cpu_consumed());
				cb->result(_ret);
			});
		});

	}
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

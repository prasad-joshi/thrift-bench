/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "gen-cpp2/Benchmark.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <atomic>
#include <chrono>

#include <folly/init/Init.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace  ::example;
using namespace  ::example::cpp2;

using std::map;
using std::vector;
using std::cout;
using std::endl;

//using namespace apache::thrift::tutorial;

#define ONE_K 1024
#define ONE_M (ONE_K * ONE_K)

#define NANOSEC_TO_SEC(nsec) ((nsec) / 1000 / 1000 / 1000)

DEFINE_string(server_ip, "127.0.0.1", "Server IP Address");
DEFINE_int32(server_port, 0, "Server Port");

struct benchmark {
	uint64_t run_time;
	uint64_t nrequests;
	uint32_t req_size;
	uint32_t qdepth;

	uint64_t client_latency;
	uint64_t db_latency;
	uint64_t dict_latency;
	uint64_t leaf_latency;
	uint64_t io_latency;
};

vector<benchmark> bench_result;

#if 0
void start_benchmark(uint32_t req_size, const uint32_t queue_depth, const uint64_t nrequests) {
	EventBase eb;
	auto client = folly::make_unique<BenchmarkAsyncClient>(
			HeaderClientChannel::newChannel(
				async::TAsyncSocket::newSocket(
					&eb, {"127.0.0.1", 9090})));

	auto chan = dynamic_cast<apache::thrift::HeaderClientChannel*>(client->getHeaderChannel());
	auto socket = dynamic_cast<async::TAsyncSocket*>(chan->getTransport());
	socket->setNoDelay(true);
	socket->setQuickAck(true);

	struct benchmark bm {};
	map <int64_t, std::chrono::high_resolution_clock::time_point> req_time;

	BenchmarkData bd;
	auto bench_start = std::chrono::high_resolution_clock::now();
	uint64_t req_id = 1;
	while (req_id <= nrequests) {
		bd.set_request_id(req_id);
		bd.set_data(string());
		assert(bd.get_data().size() == 0);
		bd.set_data_size(req_size);

		string key("1");
		client->sync_key_get(bd, key, bd);
		req_id++;
	}

	auto bench_end = std::chrono::high_resolution_clock::now();
	auto run_time  = std::chrono::duration_cast<std::chrono::nanoseconds>(bench_end - bench_start).count();
	chan->closeNow();
	cout << "latency : " << run_time / nrequests << endl;
}
#else
void start_benchmark(uint32_t req_size, const uint32_t queue_depth, const uint64_t nrequests) {
	EventBase eb;
	auto client = folly::make_unique<BenchmarkAsyncClient>(
			HeaderClientChannel::newChannel(
				async::TAsyncSocket::newSocket(
					&eb, {FLAGS_server_ip, FLAGS_server_port})));

	auto chan = dynamic_cast<apache::thrift::HeaderClientChannel*>(client->getHeaderChannel());
	//auto socket = dynamic_cast<async::TAsyncSocket*>(chan->getTransport());
	//socket->setNoDelay(true);
	//socket->setQuickAck(true);

	struct benchmark bm {};
	map <int64_t, std::chrono::high_resolution_clock::time_point> req_time;

	auto bench_start = std::chrono::high_resolution_clock::now();

	atomic<uint64_t> req_complete{0};
	atomic<uint64_t> req_tosend{queue_depth};
	atomic<uint64_t> req_out{0};
	atomic<uint64_t> req_sched{0};
	atomic<int64_t>  req_id{1};
	while (req_complete.load() < nrequests) {
		for (auto i = 0; i < req_tosend.load(); i++) {
			BenchmarkData bd;

			req_time[req_id] = std::chrono::high_resolution_clock::now();

			bd.set_request_id(req_id);
			bd.set_data(string());
			assert(bd.get_data().size() == 0);
			bd.set_data_size(req_size);

			req_sched++;
			string key("1");
			auto f = client->future_key_get(key, bd);
			f.then([&] (BenchmarkData retbd) {
				//assert(retbd.get_data().size() == req_size);

				auto it = req_time.find(retbd.get_request_id());
				assert(it != req_time.end());
				auto req_start = it->second;
				auto req_end   = std::chrono::high_resolution_clock::now();
				auto cl = std::chrono::duration_cast<std::chrono::nanoseconds>(req_end - req_start).count();
				req_time.erase(it);

				bm.client_latency += cl;
				bm.db_latency     += retbd.get_db_latency();
				bm.dict_latency   += retbd.get_dict_latency();
				bm.leaf_latency   += retbd.get_leaf_latency();
				bm.io_latency     += retbd.get_io_latency();

				req_complete++;
				req_out--;
			});
			req_out++;
			req_id++;
		}

		eb.loopOnce();
		req_tosend = queue_depth - req_out;
		if (req_tosend + req_complete > nrequests) {
			if (req_complete < nrequests) {
				req_tosend = nrequests - req_complete;
			} else {
				req_tosend = 0;
			}
		}
	}

	auto bench_end  = std::chrono::high_resolution_clock::now();
	bm.run_time     = std::chrono::duration_cast<std::chrono::nanoseconds>(bench_end - bench_start).count();
	bm.nrequests    = nrequests;
	bm.req_size     = req_size;
	bm.qdepth       = queue_depth;

	chan->closeNow();

	// cout << "latency : " << bm.run_time / nrequests << endl;
	bench_result.push_back(bm);
}
#endif

int main(int argc, char** argv) {
	folly::init(&argc, &argv, true);

#if 0
	vector<uint32_t> req_sizes {1, 16, 512, ONE_K, 4*ONE_K, 8*ONE_K, 16*ONE_K,
				32*ONE_K, 64*ONE_K, 128*ONE_K, 256*ONE_K, 512*ONE_K, ONE_M,
				10 * ONE_M, 1000, 2000, 4000, 10000, 15600, 520000, 1040000
			};
#else
	vector<uint32_t> req_sizes{4096};
#endif
	vector<uint32_t> queue_depths {1, 2, 16, 32, 64, 256, 512, 1024};

	const uint64_t MaxRequests = 1000000;
	try {
		for (auto qd : queue_depths) {
			for (auto sz: req_sizes) {
				cout << "Using queue_depths " << qd << " and req_size " << sz << endl;
				start_benchmark(sz, qd, MaxRequests);
			}
		}
	} catch (TException &tx) {
		printf("ERROR: %s\n", tx.what());
		return 1;
	}

#if 0
struct benchmark {
	uint64_t run_time;
	uint64_t nrequests;
	uint32_t req_size;
	uint32_t qdepth;

	uint64_t client_latency;
	uint64_t db_latency;
	uint64_t dict_latency;
	uint64_t leaf_latency;
	uint64_t io_latency;
};
#endif

	cout << "run time,requests,request size,queue depth,bandwidth,tot client latency, avg client latency,"
		 << "tot DB latency, avg DB latency,tot dict latency,avg dict latency, tot leaf latency, avg leaf latency,"
		 << "tot io latency, avg io latency" << endl;
	for (auto &br : bench_result) {
		auto nr     = br.nrequests;
		assert(nr);
		auto rs     = br.req_size;
		auto cl     = br.client_latency;
		auto acl    = br.client_latency / nr;
		auto dbl    = br.db_latency;
		auto adbl   = dbl / nr;
		auto dictl  = br.dict_latency;
		auto adictl = dictl / nr;
		auto leafl  = br.leaf_latency;
		auto aleafl = leafl / nr;
		auto iol    = br.io_latency;
		auto aiol   = iol / nr;

		auto secs   = NANOSEC_TO_SEC(br.run_time);
		uint64_t bw = 0;
		if (secs != 0) {
			bw = nr * rs / secs;
		}
		cout << br.run_time << "," << nr << "," << rs << "," << br.qdepth << "," << bw << "," << cl << "," << acl << ","
				<< dbl << "," << adbl << "," << dictl << "," << adictl << "," << leafl << "," << aleafl << ","
				<< iol << "," << aiol << endl;
	}
	return 0;
}
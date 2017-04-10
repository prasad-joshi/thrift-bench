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

#include "gen-cpp2/DBBenchmark.h"
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

struct ThreadBenchmark {
	uint64_t db_latency;
	uint64_t leaf_latency;
	uint64_t io_latency;
	uint64_t cpu_consumed;
};

void laydown_files(DBBenchmarkAsyncClient *clientp, const uint64_t nfiles, const uint32_t filesize) {
	string d(filesize, 'A');
	for (auto i = 0; i < nfiles; i++) {
		string filename{std::to_string(i)};
		clientp->sync_write(filename, d);
	}
}

void start_benchmark(struct ThreadBenchmark *resultp, uint32_t req_size,
				const uint32_t queue_depth, const uint64_t nrequests) {
	EventBase eb;
	auto client = folly::make_unique<DBBenchmarkAsyncClient>(
			HeaderClientChannel::newChannel(
				async::TAsyncSocket::newSocket(
					&eb, {FLAGS_server_ip, FLAGS_server_port})));

	auto chan = dynamic_cast<apache::thrift::HeaderClientChannel*>(client->getHeaderChannel());

	/* write files on to each client */
	laydown_files(client.get(), nrequests, req_size);

	map <int64_t, std::chrono::high_resolution_clock::time_point> req_time;

	atomic<uint64_t> req_complete{0};
	atomic<uint64_t> req_tosend{queue_depth};
	atomic<uint64_t> req_out{0};
	atomic<uint64_t> req_sched{0};
	atomic<int64_t>  req_id{1};
	while (req_complete.load() < nrequests) {
		for (auto i = 0; i < req_tosend; i++) {
			DBBenchmarkData bench;
			string filename{std::to_string(i)};

			req_time[req_id] = std::chrono::high_resolution_clock::now();

			req_sched++;
			auto f = client->future_read(req_id, filename, req_size);
			f.then([&] (DBBenchmarkData retbd) {
				assert(retbd.get_data().size() == req_size);

				auto it = req_time.find(retbd.get_request_id());
				assert(it != req_time.end());
				auto req_start = it->second;
				auto req_end   = std::chrono::high_resolution_clock::now();
				auto cl = std::chrono::duration_cast<std::chrono::nanoseconds>(req_end - req_start).count();
				req_time.erase(it);

				resultp->db_latency   += cl;
				resultp->leaf_latency += retbd.get_leaf_latency();
				resultp->io_latency   += retbd.get_io_latency();
				resultp->cpu_consumed += retbd.get_cpu_consumed();

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

	chan->closeNow();
}

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
	std::vector<uint32_t> nthreads{1, 10, 100, 1000};
	const uint64_t MaxRequests = 2048;

//	cout << "#Threads, ReqSize, QD, Requests/Thread, Avg Latency, #TotRequest, TotAvgLatency\n";
	cout << "#Threads, ReqSize, QD, Requests/Thread, TotRunTime, TotNRequsts, "
	        "DBLat, LeafLat, IOLat, CPUConsumed, DB-Leaf Latency, Leaf - IO - CPU \n";
	try {
		for (auto nt : nthreads) {
			for (auto qd : queue_depths) {
				for (auto sz: req_sizes) {
					std::thread     *tp = new std::thread[nt];
					ThreadBenchmark bench[nt];
					std::memset(&bench, 0, sizeof(bench));

					auto s = std::chrono::high_resolution_clock::now();
					for (auto i = 0; i < nt; i++) {
						tp[i] = std::thread(start_benchmark, &bench[i], sz, qd, MaxRequests);
					}

					for (auto i = 0; i < nt; i++) {
						tp[i].join();
					}

					auto e = std::chrono::high_resolution_clock::now();
					auto total_run_time = std::chrono::duration_cast<std::chrono::nanoseconds>(e-s).count();

					uint64_t db_latency{0};
					uint64_t leaf_latency{0};
					uint64_t io_latency{0};
					uint64_t cpu_consumed{0};

					for (auto i = 0; i < nt; i++) {
						db_latency     += bench[i].db_latency;
						leaf_latency   += bench[i].leaf_latency;
						io_latency     += bench[i].io_latency;
						cpu_consumed   += bench[i].cpu_consumed;
					}

					auto nr = MaxRequests * nt;
					db_latency /= nr;
					leaf_latency /= nr;
					io_latency /= nr;
					cpu_consumed /= nr;

					cout << nt << "," << sz << "," << qd << "," << MaxRequests << ","
					     << total_run_time << "," << nr << ","  << db_latency << ","
					     << leaf_latency << "," << io_latency << "," << cpu_consumed << ","
					     << db_latency - leaf_latency << ","
					     << leaf_latency - io_latency - cpu_consumed <<  endl;
					/*
					cout << nt << "," << sz << "," << qd << "," << MaxRequests << "," << d/MaxRequests << ","
						<< MaxRequests*nt << "," << d/(MaxRequests*nt) << endl;
					*/
					delete []tp;
				}
			}
		}
	} catch (TException &tx) {
		printf("ERROR: %s\n", tx.what());
		return 1;
	}
	return 0;
}

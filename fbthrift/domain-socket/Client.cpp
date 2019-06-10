#include "gen-cpp2/StorageRpc.h"

#include <cassert>
#include <memory>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <sstream>

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace pio_thrift;

DEFINE_uint64(queue_depth, 128, "Max Queue Depth");
DEFINE_uint64(block_size, 4096, "Block size");
DEFINE_uint64(requests, 1ull << 18, "Number of requests to send");
DEFINE_bool(unlimited, true, "Ignore number of requests run unlimited");
DEFINE_uint64(iops_rate_limit, 1024, "Limit IOPS");

using ClientId = uint64_t;
using Clock = std::chrono::high_resolution_clock;

class StatsCollector {
public:
	struct Stats {
		Stats() : start_(Clock::now()) {

		}

		std::chrono::high_resolution_clock::time_point start_;
		uint64_t current_queue_depth_{0};
		uint64_t current_iops_{0};
		uint64_t total_ios_{0};
		uint64_t total_updates_{0};
	};
public:
	StatsCollector() {

	}

	~StatsCollector() {
		stop_ = true;
		thread_.join();
	}

	void StartDumpThread() {
		thread_ = std::thread([this] () mutable {
			this->Run();
		});
	}

	void Run() {
		while (not stop_) {
			::sleep(30);
			DumpInfo();
		}
	}

	void ClientAdd(ClientId client_id) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = stats_.find(client_id);
		assert(it == stats_.end());
		stats_.insert(std::make_pair(client_id, Stats()));
	}

	void Update(ClientId id, uint64_t queue_depth, uint64_t iops, uint64_t new_ios) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = stats_.find(id);
		assert(it != stats_.end());
		it->second.current_queue_depth_ = queue_depth;
		it->second.current_iops_ = iops;
		it->second.total_ios_ += new_ios;
		++it->second.total_updates_;
	}

	void DumpInfo() {
		std::ostringstream os;
		auto now = Clock::now();
		auto cur_iops = 0;
		auto cur_depth = 0;
		auto clients = 0;

		os << "========" << std::endl;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& c : stats_) {
				auto t = std::chrono::duration_cast<std::chrono::seconds>
					(now - c.second.start_).count();
				os << "* Client" << c.first
					<< ",Run Time(sec)=" << t
					<< ",Current Q Depth=" << c.second.current_queue_depth_
					<< ",Current IOPS=" << c.second.current_iops_
					<< ",Total IOs=" << c.second.total_ios_
					<< ",Total Updates=" << c.second.total_updates_ << std::endl;
				cur_iops += c.second.current_iops_;
				cur_depth += c.second.current_queue_depth_;
			}
			clients = stats_.size();
		}

		if (clients) {
			os << "Total Clients " << clients << std::endl
				<< "Average Current IOPS " << cur_iops / clients << std::endl
				<< "Average Current Q Depth " << cur_depth / clients << std::endl;
		}
		os << "========" << std::endl;
		std::cout << os.str();
	}

private:
	std::mutex mutex_;
	std::unordered_map<ClientId, Stats> stats_;
	bool stop_{false};
	std::thread thread_;
};

const std::string kPath = "/home/prasad/socket";
static constexpr size_t kBufferSize = 8 * 1024 * 1024;

int Connect() {
	struct sockaddr_un addr;
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, kPath.c_str(), sizeof(addr.sun_path)-1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		return -1;
	}

	int sndbuf;
	socklen_t sa_len = sizeof(sndbuf);
	for (int i = 0; i < 12; ++i) {
		sndbuf = kBufferSize >> i;
		auto rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sa_len);
		if (rc >= 0) {
			LOG(ERROR) << "Set size " << sndbuf << std::endl;
			break;
		}
	}

	return fd;
}

auto CreateAsyncRpcClient(folly::EventBase* basep) {
	int fd = Connect();
	assert(fd >= 0);
	return std::make_unique<StorageRpcAsyncClient>(
		HeaderClientChannel::newChannel(
			async::TAsyncSocket::newSocket(basep, fd)
		)
	);
}

template <typename Lambda>
void Benchmark(StatsCollector* statsp,
			ClientId client_id,
			folly::EventBase* basep ,
			Lambda send_func) {
	const uint64_t kQueueDepth = FLAGS_queue_depth;
	const uint64_t kBlockSize = FLAGS_block_size;
	const uint64_t kRequests = FLAGS_requests;
	const bool kUnlimited = FLAGS_unlimited;
	const uint64_t kIopsRateLimit = FLAGS_iops_rate_limit;

	std::string data(kBlockSize, 'A');
	uint64_t current_depth = 1;
	uint64_t to_send = current_depth;
	auto iops_timer = Clock::now();
	auto dump_timer_start = iops_timer;
	uint64_t ios_in_last_second{0};
	uint64_t req_id{0};
	uint64_t req_sent{0};
	uint64_t req_complete{0};
	uint64_t req_out{0};
	while (req_complete < kRequests or kUnlimited) {
		for (auto i = 0u; i < to_send; ++i) {
			send_func(++req_id, data)
			.then([&] (int rc) mutable {
				++req_complete;
				--req_out;
			});
		}
		ios_in_last_second += to_send;

		basep->loopOnce();

		auto e = Clock::now();
		auto c = std::chrono::duration_cast<std::chrono::seconds>(e - iops_timer).count();
		if (c >= 1) {
			auto new_ios = ios_in_last_second;
			statsp->Update(client_id, current_depth, new_ios, new_ios);

			if (ios_in_last_second < kIopsRateLimit) {
				++current_depth;
			} else if (ios_in_last_second > kIopsRateLimit) {
				--current_depth;
			}
			if (current_depth > kQueueDepth) {
				current_depth = kQueueDepth;
			} else if (current_depth == 0) {
				current_depth = 1;
			}
			ios_in_last_second = 0;
			iops_timer = e;
		}

		if (current_depth >= req_out) {
			to_send = current_depth - req_out;
		} else {
			to_send = 1;
		}
		if (req_sent > kRequests and not kUnlimited) {
			to_send = 0;
		}
	}
}

void BenchmarkEcho(StatsCollector* statsp, ClientId client_id) {
	folly::EventBase eb;
	auto client = CreateAsyncRpcClient(&eb);
	auto chan = dynamic_cast<HeaderClientChannel*>(client->getHeaderChannel());

	statsp->ClientAdd(client_id);

	auto func = [clientp = client.get()]
			(uint64_t req_id, const std::string& data) mutable {
		auto buf = std::make_unique<folly::IOBuf>(IOBuf::WRAP_BUFFER,
			data.data(), data.size());
		return clientp->future_Echo(req_id, buf)
		.then([buf = std::move(buf)] (auto rc) mutable {
			return 0;
		});
	};

	Benchmark(statsp, client_id, &eb, func);

	chan->closeNow();
}

void BenchmarkSend(StatsCollector* statsp, ClientId client_id) {
	folly::EventBase eb;
	auto client = CreateAsyncRpcClient(&eb);
	auto chan = dynamic_cast<HeaderClientChannel*>(client->getHeaderChannel());

	statsp->ClientAdd(client_id);

	auto func = [clientp = client.get()]
			(uint64_t req_id, const std::string& data) mutable {
		auto buf = std::make_unique<folly::IOBuf>(IOBuf::WRAP_BUFFER,
			data.data(), data.size());
		return clientp->future_Send(req_id, buf)
		.then([buf = std::move(buf)] (auto rc) mutable {
			return 0;
		});
	};

	Benchmark(statsp, client_id, &eb, func);

	chan->closeNow();
}

void BenchmarkSendNoData(StatsCollector* statsp, ClientId client_id) {
	folly::EventBase eb;
	auto client = CreateAsyncRpcClient(&eb);
	auto chan = dynamic_cast<HeaderClientChannel*>(client->getHeaderChannel());

	statsp->ClientAdd(client_id);

	auto func = [clientp = client.get()]
			(uint64_t req_id, const std::string& data) mutable {
		return clientp->future_SendNoData(req_id)
		.then([] (auto rc) mutable {
			return 0;
		});
	};

	Benchmark(statsp, client_id, &eb, func);

	chan->closeNow();
}

std::vector<std::string> Split(const std::string &str, char delim) {
	std::vector<std::string> tokens;
	size_t s = 0;
	size_t e = 0;

	while ((e = str.find(delim, s)) != std::string::npos) {
		if (e != s) {
			tokens.push_back(str.substr(s, e - s));
		}
		s = e + 1;
	}
	if (e != s) {
		tokens.push_back(str.substr(s));
	}
	return tokens;
}

static const std::string kQuit = "quit";
static const std::string kAddClient = "client_add";
static const std::string kStopClient = "client_stop";

int main(int argc, char* argv[]) {
	folly::init(&argc, &argv, true);
	{
		StatsCollector stats;
		stats.StartDumpThread();
		ClientId client_id{0};

		std::unordered_map<ClientId, std::thread> clients;

		while (1) {
			std::string command;
			std::cout << ">> ";
			std::getline(std::cin, command);
			auto tokens = Split(command, ' ');
			if (tokens.size() == 0) {
				continue;
			}

			auto cmd = tokens[0];
			if (cmd == kQuit) {
				std::cout << "Not properly implemented" << std::endl;
				break;
			} else if (cmd == kAddClient) {
				auto id = ++client_id;
				auto thread = std::thread([&, id] () mutable {
					BenchmarkSendNoData(&stats, id);
				});

				clients.insert(std::make_pair(id, std::move(thread)));
			} else if (cmd == kStopClient) {
				std::cout << "Not implemented" << std::endl;
			}
		}
	}
//	BenchmarkSend();
//	BenchmarkSendNoData();
	return 0;
}

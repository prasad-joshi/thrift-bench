#include <iostream>
#include <cstdlib>
#include <folly/io/async/AsyncSocket.h>
#include <folly/portability/Sockets.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/init/Init.h>

#include <chrono>
#include <map>

#include "request.h"

using namespace folly;
using std::shared_ptr;
using std::unique_ptr;
using std::cout;
using std::endl;
using std::string;
using std::map;

#define NANOSEC_TO_SEC(nsec) ((nsec) / 1000.0 / 1000.0 / 1000.0)
void requestRead(void *opaque,  std::shared_ptr<AsyncSocket> socket, ManagedBuffer buffer, size_t size);

class Benchmark {
private:
	EventBase               base_;
	shared_ptr<AsyncSocket> socket_;
	int32_t                 qdepth_;
	string                  server_;
	int                     port_;
	std::atomic<uint32_t>   req_sent;
	std::atomic<uint32_t>   req_outstanding;
	std::atomic<uint32_t>   req_completed;
	const uint64_t          max_requests = 1000000;

	std::chrono::high_resolution_clock::time_point start_time;

private:
	std::atomic<uint64_t>   request_id;
	map <uint64_t, std::chrono::system_clock::time_point> req_time;
	uint64_t tot_req_latency;
public:
	class WriteCallback : public AsyncTransportWrapper::WriteCallback {
	private:
		struct request *req_;
		Benchmark      *benchp_;
	public:
		WriteCallback(Benchmark *bencp, struct request *req) : benchp_(bencp), req_(req) {
		}

		void writeSuccess() noexcept {
			benchp_->request_sent(req_);
			req_ = nullptr;
		}

		void writeErr(size_t bytesWritten, const AsyncSocketException& ex) noexcept {
			assert(0);
		}
	};

	class ConnectCallback : public AsyncSocket::ConnectCallback {
	private:
		Benchmark *bencp_;

	public:
		ConnectCallback(Benchmark *bencp) : bencp_(bencp) {
		}

		virtual void connectSuccess() noexcept {
			bencp_->start_benchmark();
		}

		virtual void connectErr(const AsyncSocketException& ex) noexcept {
			auto en = ex.getErrno();
			auto ty = ex.getType();
			cout << "Error : " << en << " Type: " << ty << endl;
			assert(0);
		}
	};

	Benchmark(string server, int port, int32_t qdepth) {
		server_         = server;
		port_           = port;
		qdepth_         = qdepth;
		req_sent        = 0;
		req_outstanding = 0;
		req_completed   = 0;
		request_id      = 0;
		tot_req_latency = 0;
	}

	~Benchmark() {
		if (socket_) {
			socket_->close();
		}
	}

	void run() {
		socket_ = AsyncSocket::newSocket(&base_);

		ConnectCallback ccb(this);
		socket_->connect(&ccb, server_, port_);

		ReadCallback rcb{&base_, socket_, socket_->getFd(), sizeof(struct request)};
		rcb.registerReadCallback(requestRead, this);
		socket_->setReadCB(&rcb);

		base_.loopForever();

		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e - start_time).count();

		socket_->close();
		//socket_->destroy();
		socket_=nullptr;

		// cout << "QDepth, Requests, Benchmark Time, Benchmark Latency, Request Time, Request Latency, BW\n";
		uint64_t bw;
		auto seconds = NANOSEC_TO_SEC(d);
		if (seconds) {
			bw = (max_requests * sizeof(request)) / seconds;
		}
		cout << qdepth_ << "," << max_requests << "," << d << "," << d / max_requests << ","
			<< tot_req_latency << "," << tot_req_latency / max_requests << "," << bw << endl;
	}

	void start_benchmark() {
		start_time = std::chrono::high_resolution_clock::now();
		requests_send(qdepth_);
	}

	void stop_benchmark() {
		base_.terminateLoopSoon();
	}

	void requests_send(uint32_t nrequests) {
		// cout << "*** sending nrequests " << nrequests << endl;
		for (auto i = 0; i < nrequests; i++) {
			request_id++;
			req_time[request_id] = std::chrono::high_resolution_clock::now();

			// cout << "Sending req with id " << request_id << endl;

			request *reqp = new request();
			reqp->requestid = request_id;
			WriteCallback wcb(this, reqp);
			char *bufp = reinterpret_cast<char *>(reqp);
			socket_->write(&wcb, bufp, sizeof(*reqp));

			req_sent++;
			req_outstanding++;
		}
	}

	void request_sent(struct request *reqp) {
		// cout << "One request sent.\n";
		delete reqp;
	}

	void request_received(struct request *reqp) {
		uint64_t id = reqp->requestid;
		// cout << "Received id " << id << endl;
		auto it     = req_time.find(id);
		assert(it  != req_time.end());
		auto s = it->second;
		auto e = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();

		tot_req_latency += d;

		req_completed++;
		req_outstanding--;
		if (req_completed < max_requests) {
			requests_send(1);
		} else if (req_outstanding != 0 ) {
			/* do nothing */
		} else {
			stop_benchmark();
		}
	}

	void request_received(ManagedBuffer buffer) {
		char    *bufp = buffer.get();
		request *reqp = reinterpret_cast<request *>(bufp);
		request_received(reqp);
	}

	void get_benchmark_data(uint64_t *nrequestsp, uint64_t *latencyp) {
		*nrequestsp = max_requests;
		*latencyp   = tot_req_latency;
	}
};

void requestRead(void *opaque,  std::shared_ptr<AsyncSocket> socket, ManagedBuffer buffer, size_t size) {
	assert(opaque && size == sizeof(struct request));
	Benchmark *bencp = reinterpret_cast<Benchmark *>(opaque);
	bencp->request_received(std::move(buffer));
}

DEFINE_string(server_ip, "127.0.0.1", "Server IP Address");
DEFINE_int32(server_port, 0, "Server Port");

int main(int argc, char *argv[]) {
	folly::init(&argc, &argv);
	cout << "QDepth, Requests, Benchmark Time, Benchmark Latency, Request Time, Request Latency, BW\n";

	std::vector<int> qdepths{1, 2, 8, 16, 32, 64, 128, 256, 512, 1024};
	for (auto &d : qdepths) {
		uint64_t nrequests;
		uint64_t latency;

		Benchmark b{FLAGS_server_ip, FLAGS_server_port, d};
		b.run();
		b.get_benchmark_data(&nrequests, &latency);
	}
}

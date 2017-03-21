#include <iostream>
#include <cstdlib>
#include <folly/io/async/AsyncSocket.h>
#include <folly/portability/Sockets.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/init/Init.h>

#include "request.h"

using namespace folly;
using std::shared_ptr;
using std::unique_ptr;
using std::cout;
using std::endl;

class SAcceptCallback : public AsyncServerSocket::AcceptCallback {
private:
	std::function<void(int, const folly::SocketAddress&)> connectionAcceptedFn_;

public:
	SAcceptCallback() : connectionAcceptedFn_() {
	}

	void setConnectionAcceptedFn(const std::function<void(int, const folly::SocketAddress&)>& fn) {
		connectionAcceptedFn_ = fn;
	}

	void connectionAccepted(int fd, const folly::SocketAddress& clientAddr) noexcept override {
		assert(connectionAcceptedFn_);
		// cout << "Connection Accepted. " << fd << endl;
		if (connectionAcceptedFn_) {
			connectionAcceptedFn_(fd, clientAddr);
		}
	}

	void acceptError(const std::exception& ex) noexcept {
		cout << "Accept Failed.\n";
		assert(0);
	}
};

class WriteCallback : public AsyncTransportWrapper::WriteCallback {
private:
	ManagedBuffer mbufp;
public:
	WriteCallback(ManagedBuffer b) : mbufp(std::move(b)) {
	}

	void writeSuccess() noexcept {
		// cout << "response written.\n";
		delete this;
	}

	void writeErr(size_t bytesWritten, const AsyncSocketException& ex) noexcept {
	}
};

void readDataAvailable(void *unused, std::shared_ptr<AsyncSocket> socket, ManagedBuffer buffer, size_t size) {
	// cout << "Received a request.\n";
	assert(size == sizeof(struct request));
	char    *bufp = buffer.get();
	request *req  = reinterpret_cast<request *>(bufp);
	WriteCallback *wcb = new WriteCallback(std::move(buffer));

	// cout << "Sending response to reaid " << req->requestid << endl;
	socket->write(wcb, bufp, size);
}

DEFINE_int32(server_port, 9091, "Server Port");

int main(int argc, char *argv[]) {
	folly::init(&argc, &argv);

	EventBase base;
	auto      socket = AsyncServerSocket::newSocket(&base);
	socket->bind(FLAGS_server_port);
	socket->listen(16);

	folly::SocketAddress serverAddress;
	socket->getAddress(&serverAddress);

	SAcceptCallback callback;
	callback.setConnectionAcceptedFn(
		[&] (int fd, const SocketAddress& clientAddr) {
			// cout << "Lamda Connection Accepted. Starting A Thread. " << fd << endl;

#if 0
			auto rc = fcntl(fd, F_GETFD);
			assert(rc >= 0);
			rc = fcntl(fd, F_GETFL);
			assert(rc >= 0);
#endif

#if 1
			auto rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
			assert(rc == 0);

			auto thread = std::thread([fdCap = fd] () {
				// cout << "In thread. " << fdCap << endl;
				EventBase    base;
				auto         sock = AsyncSocket::newSocket(&base, fdCap);
				ReadCallback rcb{&base, sock, fdCap, sizeof(struct request)};

				rcb.registerReadCallback(readDataAvailable, nullptr);
				sock->setReadCB(&rcb);

				// cout << "base.loopForever in thread.\n";
				base.loopForever();
			});
			thread.detach();
#else
			auto         sock = AsyncSocket::newSocket(&base, fd);
			ReadCallback rcb{&base, sock, fd};

			rcb.registerReadCallback(readDataAvailable, nullptr);
			sock->setReadCB(&rcb);
#endif
	});

	socket->addAcceptCallback(&callback, &base);
	socket->startAccepting();
	base.loopForever();
}
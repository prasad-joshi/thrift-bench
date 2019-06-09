#include <memory>
#include <folly/init/Init.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "gen-cpp2/StorageRpc.h"

using namespace apache::thrift;
using namespace pio_thrift;
using namespace std;

class StorRpcSimpleImpl : virtual public StorageRpcSvIf {
public:
	void async_tm_Echo(
			std::unique_ptr<HandlerCallback<std::unique_ptr<EchoResult>>> cb,
			int64_t request_id, std::unique_ptr<IOBufPtr> data) override {
		auto d1 = std::move(*data);
		auto rr = std::make_unique<EchoResult>();
		rr->request_id = request_id;
		rr->data = std::move(d1);
		cb->result(std::move(rr));
	}

	void async_tm_Send(
			std::unique_ptr<HandlerCallback<std::unique_ptr<SendResult>>> cb,
			int64_t request_id, std::unique_ptr<IOBufPtr> data) override {
		auto rr = std::make_unique<SendResult>();
		rr->request_id = request_id;
		cb->result(std::move(rr));
	}

	void async_tm_SendNoData(
			unique_ptr<HandlerCallback<unique_ptr<SendResult>>> cb,
			int64_t request_id) override {
		auto rr = std::make_unique<SendResult>();
		rr->request_id = request_id;
		cb->result(std::move(rr));
	}
};

static const std::string kPath = "/home/prasad/socket";
static constexpr size_t kBufferSize = 8 * 1024 * 1024;

int CreateUnixDomainSocket() {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, kPath.c_str(), sizeof(addr.sun_path)-1);

	auto rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (rc < 0) {
		LOG(ERROR) << "Bind failed with error " << errno;
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

int main(int argc, char* argv[]) {
	folly::init(&argc, &argv, true);
	unlink(kPath.c_str());

	int domain_soket = CreateUnixDomainSocket();
	assert(domain_soket >= 0);

	folly::AsyncServerSocket::UniquePtr server_socket(new folly::AsyncServerSocket);
	server_socket->useExistingSocket(domain_soket);
	folly::SocketAddress address;
	server_socket->getAddress(&address);
	// server_socket->bind(address);

	auto handler = std::make_shared<StorRpcSimpleImpl>();
	auto server = std::make_shared<ThriftServer>();
	server->setInterface(handler);
	server->setPort(3000);
	server->useExistingSocket(std::move(server_socket));

	printf("Starting the server...\n");
	server->serve();
	printf("Done\n");
	return 0;
}

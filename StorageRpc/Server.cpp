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

int main(int argc, char* argv[]) {
	folly::init(&argc, &argv, true);

	auto handler = std::make_shared<StorRpcSimpleImpl>();

	auto server = std::make_shared<ThriftServer>();
	server->setInterface(handler);
	server->setPort(3000);

	printf("Starting the server...\n");
	server->serve();
	printf("Done\n");
	return 0;
}
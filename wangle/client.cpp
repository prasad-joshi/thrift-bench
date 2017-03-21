/*
 *  Copyright (c) 2017, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gflags/gflags.h>
#include <iostream>

#include <wangle/service/Service.h>
#include <wangle/service/ExpiringFilter.h>
#include <wangle/service/ClientDispatcher.h>

#include <wangle/bootstrap/ClientBootstrap.h>
#include <wangle/channel/AsyncSocketHandler.h>
#include <wangle/channel/EventBaseHandler.h>
#include <wangle/codec/LengthFieldBasedFrameDecoder.h>
#include <wangle/codec/LineBasedFrameDecoder.h>
#include <wangle/codec/LengthFieldPrepender.h>
#include <wangle/codec/StringCodec.h>
#include <atomic>

#include "message.h"

using namespace folly;
using namespace wangle;

DEFINE_int32(port, 8080, "echo server port");
DEFINE_string(host, "::1", "echo server address");

typedef Pipeline<folly::IOBufQueue&, Message::Ptr> MsgPipeline;

// chains the handlers together to define the response pipeline
class MsgPipelineFactory : public PipelineFactory<MsgPipeline> {
 public:
  MsgPipeline::Ptr newPipeline(std::shared_ptr<AsyncTransportWrapper> sock) {
    auto pipeline = MsgPipeline::create();
    pipeline->addBack(AsyncSocketHandler(sock));
    pipeline->addBack(
        EventBaseHandler()); // ensure we can write from any thread
    pipeline->addBack(LengthFieldBasedFrameDecoder());
    pipeline->addBack(LengthFieldPrepender());
    pipeline->addBack(MessageCodec());
	//    pipeline->addBack(MsgHandler());
    pipeline->finalize();
    return pipeline;
  }
};

class MsgClientDispatcher
	:  public ClientDispatcherBase<MsgPipeline, Message::Ptr, Message::Ptr> {
public:
	void read(Context *, Message::Ptr in) override {
	    auto search = requests_.find(in->reqId);
	    CHECK(search != requests_.end());
	    auto p = std::move(search->second);
	    requests_.erase(in->reqId);
	    p.setValue(in);
	}

	Future<Message::Ptr > operator()(Message::Ptr in) override {
	    auto& p = requests_[in->reqId];
	    auto f = p.getFuture();
	    p.setInterruptHandler([in, this](const folly::exception_wrapper&) {
	        this->requests_.erase(in->reqId);
	    });
	    this->pipeline_->write(in);
	    return f;
	}
	// Print some nice messages for close

	virtual Future<Unit> close() override {
		printf("Channel closed\n");
		return ClientDispatcherBase::close();
	}

	virtual Future<Unit> close(Context* ctx) override {
		printf("Channel closed\n");
		return ClientDispatcherBase::close(ctx);
	}

private:
	std::unordered_map<int64_t, Promise<Message::Ptr>> requests_;

};

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  ClientBootstrap<MsgPipeline> client;
  client.group(std::make_shared<wangle::IOThreadPoolExecutor>(1));
  client.pipelineFactory(std::make_shared<MsgPipelineFactory>());
  auto pipeline = client.connect(SocketAddress(FLAGS_host, FLAGS_port)).get();
  auto dispatcher = std::make_shared<MsgClientDispatcher>();
  dispatcher->setPipeline(pipeline);
  ExpiringFilter<Message::Ptr, Message::Ptr> service(dispatcher, std::chrono::seconds(50));
  std::atomic<uint64_t> latency {0};
  std::atomic<uint64_t> count {0};
  
  try {
	int seq = 1;
	for (auto datasize : std::vector<int> {1024, 4*1024, 8*1024, 16*1024, 64*1024}) {
	auto start_time = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() < 5) {
      std::string line;
      auto msg = Message::Ptr(new(datasize) Message(datasize));
      msg->reqId = seq++;
	  msg->client_send = std::chrono::high_resolution_clock::now();
	  auto reply = service(msg);
      auto fut = reply.then([&latency, &count, msg](Message::Ptr resp) {
           CHECK(msg->reqId == resp->reqId);
		   auto current = std::chrono::high_resolution_clock::now();
		   auto cl = std::chrono::duration_cast<std::chrono::nanoseconds>(current - resp->client_send).count();
		   latency += cl;
		   count++;
       });
	  fut.wait();
    }
	std::cout <<  datasize << ","  << latency/count <<std::endl;
	}
  } catch (const std::exception& e) {
    std::cout << exceptionStr(e) << std::endl;
  }

  return 0;
}

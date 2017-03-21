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

#include <wangle/service/Service.h>
#include <wangle/service/ExecutorFilter.h>
#include <wangle/service/ServerDispatcher.h>

#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/channel/AsyncSocketHandler.h>
#include <wangle/codec/LengthFieldBasedFrameDecoder.h>
#include <wangle/codec/LineBasedFrameDecoder.h>
#include <wangle/codec/LengthFieldPrepender.h>
#include <wangle/codec/StringCodec.h>
#include <wangle/concurrent/CPUThreadPoolExecutor.h>
#include <wangle/channel/EventBaseHandler.h>

#include "message.h"

using namespace folly;
using namespace wangle;

DEFINE_int32(port, 8080, "echo server port");

typedef Pipeline<IOBufQueue&, Message::Ptr> MsgPipeline;

class MsgService : public Service<Message::Ptr, Message::Ptr> {
public:
	virtual Future<Message::Ptr> operator()(Message::Ptr in) override {
		return in;
	}
};

// where we define the chain of handlers for each messeage received
class MsgPipelineFactory : public PipelineFactory<MsgPipeline> {
 public:
  MsgPipeline::Ptr newPipeline(std::shared_ptr<AsyncTransportWrapper> sock) {
    auto pipeline = MsgPipeline::create();
    pipeline->addBack(AsyncSocketHandler(sock));
    pipeline->addBack(EventBaseHandler());
    pipeline->addBack(LengthFieldBasedFrameDecoder());
    pipeline->addBack(LengthFieldPrepender());
    pipeline->addBack(MessageCodec());
    pipeline->addBack(MultiplexServerDispatcher<Message::Ptr, Message::Ptr>(&service_));
	//    pipeline->addBack(SerialServerDispatcher<Message::Ptr, Message::Ptr>(&service_));
    pipeline->finalize();
    return pipeline;
  }
 private:
  ExecutorFilter<Message::Ptr, Message::Ptr> service_ {
      std::make_shared<CPUThreadPoolExecutor>(10),
      std::make_shared<MsgService>()};
};

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  ServerBootstrap<MsgPipeline> server;
  server.childPipeline(std::make_shared<MsgPipelineFactory>());
  server.bind(FLAGS_port);
  server.waitForStop();

  return 0;
}

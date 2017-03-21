/*
 * message.h
 *
 * Copyright DC Engines Pvt. Ltd.
 */

#pragma once

#include <wangle/channel/Handler.h>
#include <memory>
#include <chrono>

using namespace folly;
using namespace wangle;

struct Message {
	using Ptr = std::shared_ptr<Message>;
	uint64_t reqId; // request ID to co-relate req/response
	uint64_t extraLen;
	std::chrono::time_point<std::chrono::high_resolution_clock> client_send;
	std::chrono::time_point<std::chrono::high_resolution_clock> client_resp;
	Message(uint64_t extra) { extraLen =  extra;}
	void * operator new(std::size_t count, std::size_t extra) {
		return ::new char [count + extra];
	}
};

class MessageCodec : public Handler<std::unique_ptr<folly::IOBuf>,
	Message::Ptr, Message::Ptr ,
		std::unique_ptr<folly::IOBuf>>
{
public:
	typedef typename Handler<std::unique_ptr<folly::IOBuf>, Message::Ptr,
		Message::Ptr, std::unique_ptr<folly::IOBuf>>::Context Context;

	void
	read (Context* ctx, std::unique_ptr<folly::IOBuf> buf) override
	{
//		std::cout << "in read " << std::endl;
		if (buf) {
			buf->coalesce ();
			Message *msg = (Message *)buf->data();
			auto data = Message::Ptr(new (0)Message(*msg));
			// would be better to not duplicate the message and just over lay it
			// above the 
			/* auto data = MsgFactory::msgFactory(std::move(buf)); */
//			std::cout << "converted to message " << data << std::endl;
			ctx->fireRead (data);
		}
	}

	folly::Future<folly::Unit>
	write (Context* ctx, Message::Ptr msg) override
	{
//		std::cout << "in write " << msg << std::endl;
		return ctx->fireWrite (folly::IOBuf::copyBuffer (msg.get(),
								 sizeof(Message) + msg->extraLen));
	}
};

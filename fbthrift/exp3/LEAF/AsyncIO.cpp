#include <memory>
#include <vector>

#include <libaio.h>
#include <sys/eventfd.h>

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>

#include "AsyncIO.h"

using namespace folly;
using std::unique_ptr;
using std::vector;
using std::make_unique;

IO::IO(int fd, size_t size, uint64_t offset, IOType type) :
		fd_(fd), size_(size), offset_(offset), type_(type), bufp_(allocBuffer(size)) {
	prepare();
	assert(p_.isFulfilled() == false && bufp_);
}

void IO::prepare() {
	switch (type_) {
	case IOType::READ:
		io_prep_pread(&iocb_, fd_, reinterpret_cast<void *>(bufp_.get()), size_, offset_);
		break;
	case IOType::WRITE:
		io_prep_pwrite(&iocb_, fd_, reinterpret_cast<void *>(bufp_.get()), size_, offset_);
	}
}

ManagedBuffer IO::allocBuffer(size_t size) {
	void *bufp{nullptr};
	auto rc = posix_memalign(&bufp, PAGE_SIZE, size);
	assert(rc == 0 && bufp);
	return ManagedBuffer(reinterpret_cast<char *>(bufp), free);
}

char *IO::getIOBuffer() {
	return bufp_.get();
}

struct iocb *IO::getIOCB() {
	return &iocb_;
}

void IO::setResult(ssize_t result) {
	assert(p_.isFulfilled() == false);
	this->result_ = result;
}

ssize_t IO::getResult() const {
	return result_;
}

void IO::completePromise(unique_ptr<IO> iop) {
	assert(p_.isFulfilled() == false && this == iop.get());
	p_.setValue(std::move(iop));
}

Future<unique_ptr<IO>> IO::getFuture() {
	return p_.getFuture();
}

AsyncIO::AsyncIO(uint16_t capacity) : capacity_(capacity) {
	std::memset(&context_, 0, sizeof(context_));

	auto rc = io_setup(capacity, &context_);
	assert(rc == 0);
}

AsyncIO::~AsyncIO() {
	handlerp_->unregisterHandler();
	io_destroy(context_);
}

void AsyncIO::init(EventBase *basep) {
	eventfd_ = eventfd(0, EFD_NONBLOCK);
	assert(eventfd_ >= 0);

	handlerp_ = make_unique<EventFDHandler>(this, basep, eventfd_);
	assert(handlerp_);
	handlerp_->registerHandler(EventHandler::READ | EventHandler::PERSIST);
}

Future<unique_ptr<IO>> AsyncIO::ioSubmit(unique_ptr<IO> io) {
	IO *iop = io.get();
	
	mutex_.lock();
	inflight_.push_back(std::move(io));
	mutex_.unlock();

	struct iocb *iocb[1];
	auto iocbp  = iop->getIOCB();
	iocbp->data = reinterpret_cast<void *>(iop);
	iocb[0]     =  iocbp;
	io_set_eventfd(iocbp, eventfd_);
	io_submit(context_, 1, iocb);
	return iop->getFuture();
}

#if 0
vector<Future<ssize_t>> AsyncIO::iosSubmit(vector<unique_ptr<IO>> &ios) {
	vector<Future<ssize_t>> v;

	size_t nios = ios.size();
	struct iocb *iocbs[nios];
	uint32_t i = 0;
	for (auto &io : ios) {
		auto iocbp  = io.getIOCB();
		iocbp->data = reinterpret_cast<void *>(&io);
		iocbs[i]    =  iocbp;
		io_set_eventfd(iocbp, eventfd_);
		i++;
		v.emplace_back(io.getFuture());
	}
	io_submit(context_, nios, iocbs);

	return v;
}
#endif

ssize_t ioResult(struct io_event *ep) {
	return ((ssize_t)(((uint64_t)ep->res2 << 32) | ep->res));
}

void AsyncIO::iosCompleted() {
	assert(eventfd_ > 0);

	while (1) {
		eventfd_t nevents;
		auto rc = eventfd_read(eventfd_, &nevents);
		if (rc < 0 || nevents == 0) {
			if (rc < 0 && errno != EAGAIN) {
				assert(0);
			}
			assert(errno == EAGAIN);
			break;
		}

		assert(nevents > 0);
		struct io_event events[nevents];
		rc = io_getevents(context_, nevents, nevents, events, NULL);
		assert(rc == nevents);

		for (auto ep = events; ep < events + nevents; ep++) {
			auto *iop   = reinterpret_cast<IO*>(ep->data);
			auto result = ioResult(ep);

			/*
			 * TODO: Add exception handling below
			 */
			mutex_.lock();
			auto it = find_if(inflight_.begin(), inflight_.end(), [iop] (const unique_ptr<IO> &t) {
				return t.get() == iop;
			});
			assert(it != inflight_.end());

			unique_ptr<IO> ioup;
			if (it != inflight_.end()) {
				if (inflight_.size() >= 2) {
					std::swap(*it, inflight_.back());
				}
				ioup = std::move(inflight_.back());
				inflight_.pop_back();
			}
			mutex_.unlock();

			assert(ioup.get() == iop);
			iop->setResult(result);
			iop->completePromise(std::move(ioup));
		}
	}
}

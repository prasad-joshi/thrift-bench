#include <memory>
#include <vector>

#include <libaio.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>

#define PAGE_SIZE 4096

using namespace folly;
using std::unique_ptr;
using std::vector;

using ManagedBuffer = std::unique_ptr<char, void(*)(void*)>;

enum class IOType {
	READ,
	WRITE,
};

class IO {
private:
	ManagedBuffer bufp_;
	uint64_t      offset_;
	size_t        size_;
	ssize_t       result_;
	int           fd_;
	IOType        type_;
	struct iocb   iocb_;

	Promise<unique_ptr<IO>> p_;
private:
	void prepare();
	ManagedBuffer allocBuffer(size_t size);

public:
	IO(int fd, size_t size, uint64_t offset, IOType type);
	char *getIOBuffer();
	struct iocb *getIOCB();
	Future<unique_ptr<IO>> getFuture();
	void setResult(ssize_t result);
	ssize_t getResult() const;
	void completePromise(unique_ptr<IO> iop);
};

class AsyncIO {
private:
	io_context_t    context_;
	uint16_t        capacity_;
	int             eventfd_;

	std::mutex                       mutex_;
	std::vector<std::unique_ptr<IO>> inflight_;
public:
	class EventFDHandler : public EventHandler {
	private:
		int       fd_;
		EventBase *basep_;
		AsyncIO   *asynciop_;
	public:
		EventFDHandler(AsyncIO *asynciop, EventBase *basep, int fd) :
				fd_(fd), basep_(basep), asynciop_(asynciop), EventHandler(basep, fd) {
			assert(fd >= 0 && basep);
		}

		void handlerReady(uint16_t events) noexcept {
			assert(events & EventHandler::READ);
			if (events & EventHandler::READ) {
				asynciop_->iosCompleted();
			}
		}
	};

	AsyncIO(uint16_t capacity);
	~AsyncIO();

	void init(EventBase *basep);

	void iosCompleted();

	Future<unique_ptr<IO>> ioSubmit(unique_ptr<IO> io);
	//std::vector<Future<unique_ptr<IO>>> iosSubmit(std::vector<unique_ptr<IO>> &ios);

private:
	std::unique_ptr<EventFDHandler> handlerp_;
};
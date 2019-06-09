namespace cpp2 pio_thrift

typedef binary (cpp.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct EchoResult {
	1:i64 request_id;
	2:IOBufPtr data;
}

struct SendResult {
	1:i64 request_id;
}

service StorageRpc {
	EchoResult Echo(1:i64 request_id, 2:IOBufPtr data);
	SendResult Send(1:i64 request_id, 2:IOBufPtr data);
	SendResult SendNoData(1:i64 request_id);
}

namespace cpp2 pio_thrift

typedef i64 RequestID

typedef binary (cpp.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct WriteRequest {
    1: required RequestID reqid;
    3: required i32 size;
    4: required i64 offset;
	5: required i64 hits;
	6: required i64 miss;

    2: required IOBufPtr data;
}

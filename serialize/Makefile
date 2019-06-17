THRIFT := /usr/local/bin/thrift1
THRIFT_TEMPLATE := /usr/local/include/thrift/templates

INC  := -I. -Igen-cpp2/
CPPFLAGS := -std=c++17 -O3

SYS_LIBS := -lglog -lfolly -lzstd -lgflags -lssl -lcrypto #-lgss -lgssapi_krb5
SYS_LIBS += -ldouble-conversion -lpthread -lboost_system -ldl -levent -lboost_context -liberty
SYS_LIBS += -lunwind -latomic -lbenchmark # -lasan

THRIFT_LIBS := -lthrift-core -lconcurrency -lprotocol -ltransport -lasync
THRIFT_LIBS += -lsecurity -lserver -lthriftfrozen2 -lthriftprotocol -lthriftcpp2
THRIFT_LIBS += -lwangle -lconcurrency -llzma -lz -lbz2 -lsnappy -llz4 -lgssapi_krb5

LIBS += $(SYS_LIBS) $(THRIFT_LIBS)

all: Serialize

Serialize: Serialize.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

gen-cpp2/libStorageRpc.a: gen-cpp2/StorageRpc_data.cpp gen-cpp2/StorageRpc_constants.cpp gen-cpp2/StorageRpc_types.cpp
	make -C gen-cpp2/

gen-cpp2/StorageRpc_constants.cpp:
gen-cpp2/StorageRpc_types.cpp:
gen-cpp2/StorageRpc_data.cpp:
	make thrift

thrift: StorageRpc.thrift
	$(THRIFT) --gen mstch_cpp2 -o . --templates $(THRIFT_TEMPLATE) StorageRpc.thrift

clean:
	rm -f Serialize
	make -C gen-cpp2/ clean

dist-clean: clean
	make -C gen-cpp2/ dist-clean

THRIFT := /usr/local/bin/thrift1
THRIFT_TEMPLATE := /usr/local/include/thrift/templates

INC  := -I. -Igen-cpp2/
CPPFLAGS := -std=c++17 -O3

SYS_LIBS := -lglog -lfolly -lzstd -lgflags -lssl -lcrypto #-lgss -lgssapi_krb5
SYS_LIBS += -ldouble-conversion -lpthread -lboost_system -ldl -levent -lboost_context -liberty
SYS_LIBS += -lunwind -latomic -lasan

THRIFT_LIBS := -lthrift-core -lconcurrency -lprotocol -ltransport -lasync
THRIFT_LIBS += -lsecurity -lserver -lthriftfrozen2 -lthriftprotocol -lthriftcpp2
THRIFT_LIBS += -lwangle -lconcurrency -llzma -lz -lbz2 -lsnappy -llz4 -lgssapi_krb5

LIBS += $(SYS_LIBS) $(THRIFT_LIBS)

all: Server Client

Server: Server.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

Client: Client.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

thrift: StorageRpc.thrift
	$(THRIFT) --gen mstch_cpp2 -o . --templates $(THRIFT_TEMPLATE) StorageRpc.thrift

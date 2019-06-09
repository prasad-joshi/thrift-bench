THRIFT := /usr/local/bin/thrift1
THRIFT_TEMPLATE := /usr/local/include/thrift/templates

OBJS := Server
INC  := -I. -Igen-cpp2/

SYS_LIBS := -lglog -lfolly -lzstd -lgflags -lssl -lcrypto #-lgss -lgssapi_krb5
SYS_LIBS += -ldouble-conversion -lpthread -lboost_system -ldl -levent -lboost_context -liberty
SYS_LIBS += -lunwind -latomic

THRIFT_LIBS := -lthrift-core -lconcurrency -lprotocol -ltransport -lasync
THRIFT_LIBS += -lsecurity -lserver -lthriftfrozen2 -lthriftprotocol -lthriftcpp2
THRIFT_LIBS += -lwangle -lconcurrency -llzma -lz -lbz2 -lsnappy -llz4 -lgssapi_krb5

LIBS += $(SYS_LIBS) $(THRIFT_LIBS)

#DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++17 -ggdb -g #-fsanitize=address
else
CPPFLAGS := -std=c++17 -O3
endif

all: cpp2 Server Client Serialize

Server: Server.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

Client: Client.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

Serialize: SerializationTest.cpp gen-cpp2/libStorageRpc.a
	$(CXX) $(CPPFLAGS) -o $@ $^ $(INC) -Wl,--start-group $(LIBS) -Wl,--end-group

cpp2:
	make -C gen-cpp2/ -j 3

thrift: StorageRpc.thrift
	$(THRIFT) --gen mstch_cpp2 -o . --templates $(THRIFT_TEMPLATE) StorageRpc.thrift

dist-clean: clean
	make -C gen-cpp2/ dist-clean

clean:
	make -C gen-cpp2/ clean
	rm -f Server Client Serialize

#	thrift1 --gen mstch_cpp2 -o . \
#		--templates /home/prasad/workspace/cb/fbthrift/thrift/compiler/generate/templates \
#		Calculator.thrift

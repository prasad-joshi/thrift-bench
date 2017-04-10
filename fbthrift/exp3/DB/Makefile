OBJS := DBServer DBClient
LEAFDIR := ../Leaf/
LEAFSRCDIR = $(LEAFDIR)/gen-cpp2
INC  := -I. -Igen-cpp2/ -I$(LEAFDIR)
LIBS := -lboost_system -lpthread -lglog -lfolly -lthrift -lthriftcpp2 -lthriftprotocol -lzstd -lcrypto -lgflags

ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g -O0
else
CPPFLAGS := -std=c++14 -O2
endif

all: LeafLib DBLib $(OBJS)

DBLib:
	make DEBUG=$(DEBUG) -C gen-cpp2/

LeafLib:
	make DEBUG=$(DEBUG) -C ../LEAF/

DBServer: DBServer.cpp gen-cpp2/libdbserver.a ../LEAF/gen-cpp2/libleaf.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

DBClient: client.cpp gen-cpp2/libdbserver.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)
	make -C gen-cpp2/ clean
	make -C ../LEAF/ clean

dist-clean: clean
	make -C gen-cpp2/ dist-clean
	make -C ../LEAF/ dist-clean

thrift:
	python -m thrift_compiler.main --gen cpp2 db_benchmark.thrift
	make -C ../LEAF/ thrift

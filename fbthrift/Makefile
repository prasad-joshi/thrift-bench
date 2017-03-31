OBJS := GaneshaDBServer client
LEAFDIR := ../Leaf/
LEAFSRCDIR = $(LEAFDIR)/gen-cpp2
INC  := -I. -Igen-cpp2/ -I$(LEAFDIR)
LIBS := -lboost_system -lpthread -lglog -lfolly -lthrift -lthriftcpp2 -lthriftprotocol -lzstd -lcrypto -lgflags

# DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g
else
CPPFLAGS := -std=c++14 -O2
endif

all: gencpp2 $(OBJS)

gencpp2:
	make -C gen-cpp2/

thrift:
	python -m thrift_compiler.main --gen cpp2 benchmark.thrift 

GaneshaDBServer: GaneshaDBServer.cpp  gen-cpp2/libbenchmark.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

client: client.cpp  gen-cpp2/libbenchmark.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)
	make -C gen-cpp2/ clean

dist-clean: clean
	make -C gen-cpp2/ dist-clean

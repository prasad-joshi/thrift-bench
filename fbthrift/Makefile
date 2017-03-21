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

all: $(OBJS)

thrift:
	python -m thrift_compiler.main --gen cpp2 benchmark.thrift 

GaneshaDBServer: GaneshaDBServer.cpp gen-cpp2/Benchmark.cpp gen-cpp2/benchmark_constants.cpp \
			gen-cpp2/benchmark_types.cpp gen-cpp2/benchmark_data.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

client: client.cpp gen-cpp2/Benchmark.cpp gen-cpp2/benchmark_constants.cpp \
		gen-cpp2/benchmark_types.cpp gen-cpp2/benchmark_data.cpp \
	       	gen-cpp2/Benchmark_client.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)

OBJS := GaneshaDBServer client
LEAFDIR := ../Leaf/
INC  := -I. -Igen-cpp/
LIBS := -lboost_system -lpthread -lthrift -lgflags -lglog

# DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g
else
CPPFLAGS := -std=c++14 -O2
endif

all: $(OBJS)

thrift: benchmark.thrift
	thrift -r  --gen cpp  benchmark.thrift

GaneshaDBServer: thrift-server.cpp gen-cpp/Benchmark.cpp \
		gen-cpp/benchmark_constants.cpp gen-cpp/benchmark_types.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

client: client.cpp gen-cpp/Benchmark.cpp \
		gen-cpp/benchmark_constants.cpp gen-cpp/benchmark_types.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)

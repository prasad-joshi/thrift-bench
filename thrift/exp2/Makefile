OBJS := db-server client
INC  := -I. -Igen-cpp/
LIBS := -lboost_system -lpthread -lthrift -lgflags -lglog -lthriftnb

# DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g
else
CPPFLAGS := -std=c++14 -O2
endif

all: gencpp $(OBJS)

gencpp:
	make -C gen-cpp/

thrift: benchmark.thrift
	thrift -r  --gen cpp  benchmark.thrift

db-server: thrift-server.cpp gen-cpp/libbenchmark.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

client: client.cpp gen-cpp/libbenchmark.a
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)
	make -C gen-cpp/ clean

dist-clean: clean
	make -C gen-cpp/ dist-clean

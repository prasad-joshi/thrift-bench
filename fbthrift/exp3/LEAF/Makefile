EXE := LeafServer
LEAFDIR := ../Leaf/
LEAFSRCDIR = $(LEAFDIR)/gen-cpp2
INC  := -I. -Igen-cpp2/ -I$(LEAFDIR)
LIBS := -lboost_system -lpthread -lglog -lfolly -lzstd -lthrift -lthriftcpp2 -lthriftprotocol -lzstd -lcrypto -lgflags -laio

ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g -O0
else
CPPFLAGS := -std=c++14 -O2
endif

SOURCES := LeafServer.cpp AsyncIO.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: gencpp2 $(EXE)

gencpp2:
	make DEBUG=$(DEBUG) -C gen-cpp2/

thrift:
	python -m thrift_compiler.main --gen cpp2 leaf_benchmark.thrift

LeafServer: $(OBJECTS) gen-cpp2/
	g++ $(CPPFLAGS) -o $@ $(OBJECTS) gen-cpp2/libleaf.a $(INC) $(LIBS)

%.o : %.cpp
	g++ $(CPPFLAGS) -fPIC -c $^

clean:
	rm -f $(EXE) $(OBJECTS)
	make -C gen-cpp2/ clean

dist-clean: clean
	make -C gen-cpp2/ dist-clean

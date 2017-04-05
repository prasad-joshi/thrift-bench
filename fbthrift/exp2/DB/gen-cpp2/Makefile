SOURCES := DBBenchmark.cpp DBBenchmark_client.cpp db_benchmark_constants.cpp db_benchmark_data.cpp db_benchmark_types.cpp
OBJECTS=$(SOURCES:.cpp=.o)

DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g -O0
else
CPPFLAGS := -std=c++14 -O2
endif

LIB = libdbserver.so
A = libdbserver.a

all: LEAF $(SOURCES) $(LIB) $(A)

LEAF:
	make -C ../../LEAF/gen-cpp2/

$(LIB): $(OBJECTS)
	g++ $(CPPFLAGS) -shared -Wl,-soname,$@ -o $@ $^

$(A): $(OBJECTS)
	ar rs $@ $^

%.o : %.cpp
	g++ $(CPPFLAGS) -fPIC -c -o $@ $^

clean:
	rm -f *.o $(LIB) $(A)

dist-clean: clean
	rm -f *[cpp,h,o,tcc]

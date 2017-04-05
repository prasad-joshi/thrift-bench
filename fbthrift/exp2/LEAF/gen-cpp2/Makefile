SOURCES := LeafBenchmark.cpp LeafBenchmark_client.cpp leaf_benchmark_constants.cpp leaf_benchmark_data.cpp leaf_benchmark_types.cpp
OBJECTS=$(SOURCES:.cpp=.o)

DEBUG=1
ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g -O0
else
CPPFLAGS := -std=c++14 -O2
endif

LIB = libleaf.so
A = libleaf.a

all: $(SOURCES) $(LIB) $(A)

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

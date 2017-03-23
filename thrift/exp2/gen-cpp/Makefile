SOURCES := Benchmark.cpp benchmark_constants.cpp benchmark_types.cpp
OBJECTS=$(SOURCES:.cpp=.o)

ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g
else
CPPFLAGS := -std=c++14 -O2
endif

LIB = libbenchmark.so
A = libbenchmark.a

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
	rm -f *[cpp,h,o]

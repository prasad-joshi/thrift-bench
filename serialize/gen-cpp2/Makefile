SOURCES := StorageRpc_constants.cpp  StorageRpc_data.cpp  StorageRpc_types.cpp
OBJECTS=$(SOURCES:.cpp=.o)

ifdef DEBUG
CPPFLAGS := -std=c++17 -ggdb -g -O0
else
CPPFLAGS := -std=c++17 -O3
endif

LIB = libStorageRpc.so
A = libStorageRpc.a

all: $(SOURCES) $(LIB) $(A)

$(LIB): $(OBJECTS)
	$(CXX) $(CPPFLAGS) -shared -Wl,-soname,$@ -o $@ $^

$(A): $(OBJECTS)
	ar rs $@ $^

%.o : %.cpp
	$(CXX) $(CPPFLAGS) -fPIC -c -o $@ $^

clean:
	rm -f *.o $(LIB) $(A)

dist-clean: clean
	rm -f *[cpp,h,o,tcc]


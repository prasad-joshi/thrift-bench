SOURCES := StorageRpc.cpp StorageRpc_client.cpp StorageRpc_constants.cpp
SOURCES += StorageRpc_data.cpp StorageRpc_processmap_binary.cpp
SOURCES += StorageRpc_processmap_compact.cpp StorageRpc_types.cpp
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
	g++ $(CPPFLAGS) -shared -Wl,-soname,$@ -o $@ $^

$(A): $(OBJECTS)
	ar rs $@ $^

%.o : %.cpp
	g++ $(CPPFLAGS) -fPIC -c -o $@ $^

clean:
	rm -f *.o $(LIB) $(A)

dist-clean: clean
	rm -f *[cpp,h,o,tcc]


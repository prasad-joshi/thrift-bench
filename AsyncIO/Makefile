INC      :=
LIBS     := -lpthread -lfolly -lglog -lgflags
CPPCLAGS := -O2 #-g -ggdb -O0
OBJS     := net-server net-client

all: $(OBJS)

net-server: net-server.cpp
	g++ -std=c++14 $(CPPCLAGS) $(INC) -o $@ $^ $(LIBS)

net-client: net-client.cpp
	g++ -std=c++14 $(CPPCLAGS) $(INC) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJS)

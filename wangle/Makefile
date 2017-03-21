OBJS := client server
LIBS := -lboost_system -lpthread -lglog -lfolly -lgflags -lwangle -lssl -lcrypto

ifdef DEBUG
CPPFLAGS := -std=c++14 -ggdb -g
else
CPPFLAGS := -std=c++14 -O2
endif

all: $(OBJS)

server: server.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

client:  client.cpp
	g++ $(CPPFLAGS) -o $@ $^ $(INC) $(LIBS)

clean:
	rm -f $(OBJS)

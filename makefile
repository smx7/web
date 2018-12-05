bin=HttpServer
cc=g++
LDFLAGS=-lpthread

$(bin):HttpServer.cc
		$(cc) -o $@ $^ $(LDFLAGS) -std=c++11

.PHONY:clean
clean:
	rm -rf $(bin)

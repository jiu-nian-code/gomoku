.PHONY : ALL
ALL : server

server : websocket_server.cc
	g++ -o $@ $^ -std=c++11
.PHONY : clean
clean : 
	rm -f server
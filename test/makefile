.PHONY : ALL
ALL : server jtest mtest

server : websocket_server.cc
	g++ -o $@ $^ -std=c++11 -lpthread -lboost_system
jtest : json_test.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp
mtest : mysql_test.cc
	g++ -o $@ $^ -std=c++11 -L /lib64/mysql/ -lmysqlclient
.PHONY : clean
clean : 
	rm -f server
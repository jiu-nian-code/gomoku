.PHONY : ALL
ALL : main

main : main.cc
	g++ -o $@ $^ -std=c++11 -L /lib64/mysql/ -lmysqlclient -lpthread -lboost_system -ljsoncpp
.PHONY : clean
clean :
	rm -f main
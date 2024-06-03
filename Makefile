all: threadApp

threadApp: threadApp.cpp
	g++ -g threadApp.cpp -o threadApp -lncurses -pthread

clean:
	rm -f threadApp
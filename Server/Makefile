# Server compilation
server: server.o main.o
	g++ server.o main.o -o /home/amitabh/Documents/Projects/TCP-IPChatRoom/server

main.o: main.cpp
	g++ -c main.cpp

server.o: server.cpp
	g++ -c server.cpp

clean:
	rm -f *.o server
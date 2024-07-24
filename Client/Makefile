# Client compilation
client: main.o client.o 
	g++ main.o client.o -o /home/amitabh/Documents/Projects/TCP-IPChatRoom/client
	
main.o: main.cpp
	g++ -c main.cpp

client.o: client.cpp
	g++ -c client.cpp

clean:
	rm -f *.o client
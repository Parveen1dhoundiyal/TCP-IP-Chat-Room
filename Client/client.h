#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define PORT 9909 // Default port number

class Client {
public:
    int client_socket;
    string server_ip;
    int port;
    Client(const string& server_ip, int port = PORT);
    ~Client();

    void connectToServer();
    void sendMessage(const string& message);
    void receiveMessages();
    
private:
    

    void handleReceivedMessage(const string& message);
};

#endif // CLIENT_H

#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <thread>
#include <map>
#include <algorithm>
#include <pthread.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
using namespace std;

#define PORT 9909       // Default port number
#define MAX_CLIENTS 10000   // Maximum number of clients allowed

struct Client {
    int socket;
    string alias;
};

class Server {
public:
    Server(int port = PORT);
    ~Server();

    void start();
    void stop();

private:
    int server_socket;
    int port;
    vector<Client> clients;
    map<int, string> client_alias_map;
    map<string, vector<int>> chat_rooms;
    string chat_room_id;
    bool is_admin;
    int admin_socket;

    string get_local_ip();
    void broadcast(const string& message, const string& room_id, const vector<int>& exclude_sockets);
    void handle_client(int client_socket);
};

#endif // SERVER_H

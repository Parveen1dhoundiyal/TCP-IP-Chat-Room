#include "client.h"

Client::Client(const string& server_ip, int port)
    : server_ip(server_ip), port(port), client_socket(-1) {}

Client::~Client() {
    close(client_socket);
}

void Client::connectToServer() {
    int nRet;
    sockaddr_in srv;

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0) {
        cerr << "Socket not opened" << endl;
        exit(EXIT_FAILURE);
    }

    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip.c_str(), &srv.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        exit(EXIT_FAILURE);
    }

    memset(srv.sin_zero, 0, 8);

    nRet = connect(client_socket, reinterpret_cast<sockaddr*>(&srv), sizeof(srv));
    if (nRet < 0) {
        cerr << "Connect failed" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Connect successful" << endl;
}

void Client::sendMessage(const string& message) {
    int nRet = send(client_socket, message.c_str(), message.size(), 0);
    if (nRet <= 0) {
        cerr << "Failed to send message" << endl;
        exit(EXIT_FAILURE);
    }
}

void Client::receiveMessages() {
    char buffer[256 + 1];
    pthread_setname_np(pthread_self(), "ClientReceiver");

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int nRet = recv(client_socket, buffer, 256, 0);
        if (nRet <= 0) {
            cerr << "Disconnected from the server" << endl;
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        buffer[nRet] = '\0';
        handleReceivedMessage(buffer);
    }
}

void Client::handleReceivedMessage(const string& message) {
    // Check special messages
    if (message == "You have been kicked from the chat room.") {
        cerr << "You have been kicked from the chat room." << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    } else if (message == "Connection rejected Limit reached.") {
        cerr << "Connection rejected Limit reached." << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    } else if (message == "Server is shutting down.") {
        cerr << "Server is shutting down." << endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    } else {
        // Normal message, print to console
        cout << message << endl;
    }
}

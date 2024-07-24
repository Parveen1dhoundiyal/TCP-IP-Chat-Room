#include "server.h"

Server::Server(int port)
    : port(port), server_socket(-1), is_admin(false), admin_socket(-1) {}

Server::~Server() {
    // Clean up any resources if needed
    close(server_socket);
}

void Server::start() {
    int nRet;

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        cerr << "Socket not opened" << endl;
        exit(EXIT_FAILURE);
    } else {
        cout << "Socket opened successfully: " << server_socket << endl;
    }

    sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(srv.sin_zero, 0, 8);

    int optval = 1;
    int optlen = sizeof(optval);
    nRet = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    if (nRet < 0) {
        cerr << "setsockopt call failed" << endl;
        exit(EXIT_FAILURE);
    }

    nRet = bind(server_socket, (sockaddr*)&srv, sizeof(sockaddr));
    if (nRet < 0) {
        cerr << "Failed to bind to local port" << endl;
        exit(EXIT_FAILURE);
    } else {
        cout << "Successfully bound to port" << endl;
    }

    nRet = listen(server_socket, MAX_CLIENTS);
    if (nRet < 0) {
        cerr << "Failed to listen to local port" << endl;
        exit(EXIT_FAILURE);
    } else {
        cout << "Listening to local port" << endl;
    }

    string ip_address = get_local_ip();
    cout << "Server is running on " << ip_address << ":" << port << endl;
    cout << "Listening for connections..." << endl;
    
    thread command_thread([this]() {
    string command;
    while (true) {
        getline(cin, command);
        if (command == "/server stop") {
            stop();
            exit(EXIT_SUCCESS);
        }
    }
});
command_thread.detach();
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            cerr << "Failed to accept new connection" << endl;
            continue;
        }

        if (clients.size() >= MAX_CLIENTS) {
            string msg = "Connection rejected Limit reached.";
            send(client_socket, msg.c_str(), msg.size(), 0);
            close(client_socket);
            continue;
        }

        thread(&Server::handle_client, this, client_socket).detach();
    }
}

void Server::stop() {
    string shutdown_message = "Server is shutting down.";
    for (const auto& client : clients) {
        send(client.socket, shutdown_message.c_str(), shutdown_message.size(), 0);
        close(client.socket);  // Close the client socket
    }
    clients.clear(); 
    close(server_socket);
}

string Server::get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    string ip_address;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                cout << "getnameinfo() failed: " << gai_strerror(s) << endl;
                exit(EXIT_FAILURE);
            }

            if (strcmp(host, "127.0.0.1") != 0) {
                ip_address = host;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip_address;
}

void Server::broadcast(const string& message, const string& room_id, const vector<int>& exclude_sockets) {
    auto& room_clients = chat_rooms[room_id];
    for (int client_socket : room_clients) {
        if (find(exclude_sockets.begin(), exclude_sockets.end(), client_socket) == exclude_sockets.end()) {
            send(client_socket, message.c_str(), message.size(), 0);
        }
    }
}

void Server::handle_client(int client_socket) {
    char buffer[256 + 1];
    string alias;

    pthread_setname_np(pthread_self(), "ServerHandler");

    // Receive chat room ID from the client
    int nRet = recv(client_socket, buffer, 256, 0);
    if (nRet <= 0) {
        close(client_socket);
        return;
    }
    buffer[nRet] = '\0';
    string received_chat_room_id = string(buffer);

    if (received_chat_room_id == "NEW") {
        chat_room_id = "room" + to_string(rand() % 1000);
        chat_rooms[chat_room_id] = vector<int>();
        send(client_socket, chat_room_id.c_str(), chat_room_id.size(), 0);

        if (clients.empty()) {
            is_admin = true;
            admin_socket = client_socket;
            string admin_msg = "You are the admin of chat room " + chat_room_id + ".";
            send(client_socket, admin_msg.c_str(), admin_msg.size(), 0);
        }
    } else {
        if (chat_rooms.find(received_chat_room_id) == chat_rooms.end()) {
            string msg = "Invalid chat room ID. Connection rejected.";
            send(client_socket, msg.c_str(), msg.size(), 0);
            close(client_socket);
            return;
        }
        chat_room_id = received_chat_room_id;
    }

    chat_rooms[chat_room_id].push_back(client_socket);
    string ask = "ChatRoom found.";
    send(client_socket, ask.c_str(), ask.size(), 0);

    // Ask the client for their alias
    nRet = recv(client_socket, buffer, 256, 0);
    if (nRet <= 0) {
        close(client_socket);
        return;
    }
    buffer[nRet] = '\0';
    alias = string(buffer);

    client_alias_map[client_socket] = alias;

    string join_message = "You have joined chat room " + chat_room_id + ".";
    send(client_socket, join_message.c_str(), join_message.size(), 0);

    string welcome_message = alias + " has joined the chat room " + chat_room_id + "!";
    vector<int> exclude_sockets{client_socket};
    broadcast(welcome_message, chat_room_id, exclude_sockets);

    clients.push_back({client_socket, alias});

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        nRet = recv(client_socket, buffer, 256, 0);
        if (nRet <= 0) {
            string leave_message = alias + " has left the chat room.";
            broadcast(leave_message, chat_room_id, exclude_sockets);

            close(client_socket);
            clients.erase(remove_if(clients.begin(), clients.end(),
                                    [client_socket](const Client& client) { return client.socket == client_socket; }),
                          clients.end());
            chat_rooms[chat_room_id].erase(remove(chat_rooms[chat_room_id].begin(), chat_rooms[chat_room_id].end(), client_socket), chat_rooms[chat_room_id].end());
            client_alias_map.erase(client_socket);
            if (client_socket == admin_socket) {
                is_admin = false;
                admin_socket = -1;
            }
            break;
        }

        buffer[nRet] = '\0';
        string message = string(buffer);

        if (client_socket == admin_socket && message.substr(0, 6) == "/admin") {
            string command = message.substr(7);
            if (command.substr(0, 5) == "kick ") {
                string kick_alias = command.substr(5);
                auto it = find_if(clients.begin(), clients.end(), [kick_alias](const Client& client) {
                    return client.alias == kick_alias;
                });
                if (it != clients.end()) {
                    int kick_socket = it->socket;
                    string kick_message = "You have been kicked from the chat room.";
                    send(kick_socket, kick_message.c_str(), kick_message.size(), 0);
                    close(kick_socket);
                    clients.erase(it);
                    chat_rooms[chat_room_id].erase(remove(chat_rooms[chat_room_id].begin(), chat_rooms[chat_room_id].end(), kick_socket), chat_rooms[chat_room_id].end());
                    client_alias_map.erase(kick_socket);

                    string notify_message = "User " + kick_alias + " has been kicked from the chat room.";
                    broadcast(notify_message, chat_room_id, exclude_sockets);
                }
            } else if (command.substr(0, 3) == "id ") {
                string new_chat_room_id = command.substr(3);
                vector<int> old_clients = chat_rooms[chat_room_id];  // Get clients in the current chat room
                chat_rooms.erase(chat_room_id); // Clear old chat room
                chat_room_id = new_chat_room_id;
                chat_rooms[chat_room_id] = old_clients; // Assign clients to new chat room ID

    // Broadcast the ID change message
    string id_change_message = "Chat room ID changed to " + chat_room_id + ".";
    broadcast(id_change_message, chat_room_id, exclude_sockets);
            }
            else if (command == "checkid") {
                string id_message = "Current chat room ID: " + chat_room_id;
                send(client_socket, id_message.c_str(), id_message.size(), 0);
            }
        } else {
            string formatted_message = alias + ": " + message;
            broadcast(formatted_message, chat_room_id, exclude_sockets);
        }
    }
}

#include "client.h"

int main() {
    string server_ip;
    int port = PORT; // Defined in Client.h as 9909

    cout << "Enter server IP address: ";
    getline(cin, server_ip);

    Client client(server_ip, port);
    client.connectToServer();

    // Start receiving messages in a separate thread
    thread receive_thread(&Client::receiveMessages, &client);
    receive_thread.detach();

    string chat_room_id;
    string alias;

    cout << "Enter chat room ID or type NEW to create a new chat room: ";
    getline(cin, chat_room_id);
    client.sendMessage(chat_room_id);

    // Receive and display chat room ID from the server
    char buffer[256 + 1];
    memset(buffer, 0, sizeof(buffer));
    int nRet = recv(client.client_socket, buffer, 256, 0);
    if (nRet <= 0) {
        cerr << "Failed to receive chat room ID from server" << endl;
        close(client.client_socket);
        exit(EXIT_FAILURE);
    }
    buffer[nRet] = '\0';
    if (string(buffer) == "Invalid chat room ID. Connection rejected.") {
        cerr << "Chat room request rejected by the server" << endl;
        close(client.client_socket);
        exit(EXIT_FAILURE);
    }
    cout << buffer << endl;

    cout << "Enter your alias name: ";
    getline(cin, alias);
    client.sendMessage(alias);
    cout << "Now You can start Messaging "<<endl;
    while (true) {
        string message;
        
        getline(cin, message);

        // Check if the message is the exit command
        if (message == "/client exit") {
            cout << "Exiting the chat room." << endl;
            client.sendMessage(message);
            close(client.client_socket); // Close socket before exit
            exit(0);
        }

        if (!message.empty()) {
            client.sendMessage(message);
        }
    }

    return 0;
}

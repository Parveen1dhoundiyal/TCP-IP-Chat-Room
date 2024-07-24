#include "server.h"

int main() {
    Server server(9909); // Example port
    server.start();

    return 0;
}

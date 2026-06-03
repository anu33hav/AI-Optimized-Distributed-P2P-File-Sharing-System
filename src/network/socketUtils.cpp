#include "network/socketUtils.h"
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

int createServerSocket(const int port) {

    // 01: create server socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd == -1) {
        cerr << "Failed to create server Socket" << endl;
        return -1;
    }

    // 02: prepare address for server
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt (serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        cerr << "Failed to resuse same socket" << endl;
        close(serverSocketFd);
        return -1;
    }

    // 03: bind
    if (bind(serverSocketFd, (sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to bind" << endl;
        close(serverSocketFd);
        return -1;
    }

    // 04: listen
    if (listen(serverSocketFd, 5) == -1) {
        cerr << "Failed to listen" << endl;
        close(serverSocketFd);
        return -1;
    }

    return serverSocketFd;
}

int acceptClient(int serverSocketFd) {

    int serverToClientSocketFd = accept(serverSocketFd, nullptr, nullptr);
    if (serverToClientSocketFd == -1) {
        cerr << "Failed to accept clients" << endl;
        close(serverSocketFd);
        return -1;
    }

    return serverToClientSocketFd;
}

int connectToServer(const char* ip, const int port) {

    // 01: create socket
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        cerr << "FAiled to create client socket" << endl;
        return -1;
    }

    // 02: prepare address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serverAddress.sin_addr) <= 0) {
        cerr << "Failed to convert IP address" << endl;
        close(clientSocketFd);
        return -1;
    }

    // 03: connect to server
    if (connect(clientSocketFd, (sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "failed to connect with server" << endl;
        close(clientSocketFd);
        return -1;
    }

    return clientSocketFd;
}

void closeSocket(int socketFd) {
    close(socketFd);
}

ssize_t sendAll(int socketFd, const char* buffer, int byteRead) {
    ssize_t byteSent = 0;
    // send till whole byteRead sent
    while (byteSent < byteRead) {

        ssize_t byteSend = send(socketFd, buffer + byteSent, byteRead - byteSent, 0);
        if (byteSend == -1) {
            cerr << "Failed to send file" << endl;
            return -1;
        }
        else if (byteSend == 0) {
            cerr << "Internal problem to send file" << endl;
            return -1;
        }

        byteSent += byteSend;
    }

    return byteSent;
}
#include <network/socketUtils.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
using namespace std;

int createServerSocket(int serverPort) {
    // 01: server socket creation
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd == -1) {
        cerr  << "Server socket creation failed" << endl;
        return -1;
    }
    // 02: reuse the same ip + port immediately after restart
    int opt = 1; // enable
    if (setsockopt (serverSocketFd, SOL_SOCKET, SO_REUSEADDR,
        &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    // 03: prepare server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY; // accepts from its own wifi, ethernet, any interface || serverAddress.sin_addr.s_addr is actual ip number [unit32_t]
    serverAddress.sin_port = htons(serverPort);

    // 04: bind server address to server socket
    if (bind(serverSocketFd, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Bind failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    // 05: listen on server socket
    if (listen(serverSocketFd, 5) == -1) {
        cerr << "Listen failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    return serverSocketFd;
}

int acceptClient(int serverSocketFd) {
    cout << "Server waiting for client...." << endl;
    int serverToClientSocketFd = accept(serverSocketFd, nullptr, nullptr);
    if (serverToClientSocketFd == -1) {
        cerr << "Accept failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    return serverToClientSocketFd;
}

int connectToServer(const char* serverIp, int serverPort) {

    // 01: create client socket
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        cerr << "Client Socket creation failed" << endl;
    }

    // 02: prepare server address 
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0) { // convertion and also used for assign ip
        cerr << "Invalid server IP address" << endl;
        close(clientSocketFd);
        return -1;
    }

    // 03: connect client to server
    if (connect(clientSocketFd, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Connection failed" << endl;
        close(clientSocketFd);
        return -1;
    }

    return clientSocketFd;
}

ssize_t sendAll(int clientSocketFd, const char* readFileBuffer, size_t bytesReadFromFileBuffer) {
    size_t totalSent = 0;

    while (totalSent < bytesReadFromFileBuffer) {
        ssize_t bytesSentToServer = send(clientSocketFd, readFileBuffer + totalSent, bytesReadFromFileBuffer - totalSent, 0);
    
        if (bytesSentToServer == -1) {
            cerr << "send failed" << endl;
            close(clientSocketFd);
            return -1;
        }
        if (bytesSentToServer == 0) {
            cerr << "internal problem failed to send file data" << endl;
            close(clientSocketFd);
            return -1;
        }

        totalSent += bytesSentToServer;
    }

    return totalSent;

}

void closeSocket(int socketFd) {
    close(socketFd);
}
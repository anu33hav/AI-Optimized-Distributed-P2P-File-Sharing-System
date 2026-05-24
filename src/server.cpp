#include <file/fileTransfer.h>
#include <network/socketUtils.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <fstream>
using namespace std;

int main() {

    // 01: create socket
    int serverSocketFd = createServerSocket(9000);
    if (serverSocketFd == -1) return -1;


    // 02: accept clients
    int clientToServerSocketFd = acceptClient(serverSocketFd);
    if (clientToServerSocketFd == -1) {
        closeSocket(serverSocketFd);
        return -1;
    }

    // 03: receive file
    long long totalBytesReceived = receiveFile(clientToServerSocketFd, "data/received.txt");
    if (totalBytesReceived == -1) {
        closeSocket(clientToServerSocketFd);
        closeSocket(serverSocketFd);
        return -1;
    }
    cout  << "Total bytes received: " << totalBytesReceived << endl;

    // 04: close socket
    closeSocket(clientToServerSocketFd);
    closeSocket(serverSocketFd);
    return 0;
}
#include <file/fileTransfer.h>
#include <network/socketUtils.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
using namespace std;

int main() {

    // 01: connect to server
    int clientSocketFd = connectToServer("127.0.0.1", 9000);
    if (clientSocketFd == -1) return -1;

    // 02: send file
    long long totalBytesSent = sendFile(clientSocketFd, "data/input.txt");
    if (totalBytesSent == -1) {
        closeSocket(clientSocketFd);
        return -1;
    }

    cout << "Total bytes sent: " << totalBytesSent << endl;
    cout << "File sent successfully" << endl;

    // 03: close socket
    closeSocket(clientSocketFd);


    return 0;
}
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
using namespace std;

int main() {

    // 01: create socket
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        cerr << "Client socket failed" << endl; // show immediately not buffered
        return 1;
    }

    // 02: server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9000);

    // 03: convert ip address into binary form (internet presentation to numeric) -> "127.0.0.1" to 2130706433
    int convertIPAddress = inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr); // serverAddress.sin_addr passing whole ip address structure [struct in_addr]
        
    if (convertIPAddress == -1) {
        cerr << "Error while converting" << endl;
        close(clientSocketFd);
        return 1;
    }
    else if (convertIPAddress == 0) {
        cerr << "Invalid address format" << endl;
        close(clientSocketFd);
        return 1;
    }

    // 04: connect to server
    if (connect(clientSocketFd, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Client to server connection failed" << endl;
        close(clientSocketFd);
        return 1;
    }

    // 05: send message
    const char* sendMsgToServer = "Hello from client to server";
    ssize_t bytesSentToServer = send(clientSocketFd, sendMsgToServer, strlen(sendMsgToServer), 0);

    if (bytesSentToServer == -1) {
        cerr << "Msg send to server is failed";
        close(clientSocketFd);
        return 1;
    }

    // 06: receive reply
    char buffer[1024] = {0};
    ssize_t bytesReceivedFromServer = recv(clientSocketFd, buffer, sizeof(buffer)-1, 0);

    if (bytesReceivedFromServer == -1) {
        cerr << "Msg received from client failed" << endl;
        close(clientSocketFd);
        return 1;
    }
    else if (bytesReceivedFromServer == 0) {
        cerr << "Server closed connection" << endl;
        close(clientSocketFd);
        return 1;
    }
    else {
        buffer[bytesReceivedFromServer] = '\0';
        cout << "Server says: " << buffer << endl;
    }

    close(clientSocketFd);

    return 0;
}
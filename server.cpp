#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
using namespace std;

int main() {

    // 01: create server socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0); // listening socket
    if (serverSocketFd == -1) {
        cout << "Server socket creation failed" << endl;
        return 1;
    }

    // 02: prepare address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY; // accepts from wifi, ethernet, any interface (these are the interfaces on this system) || serverAddress.sin_addr.s_addr is actual ip number [unit32_t]
    serverAddress.sin_port = htons(9000); // htons convert computer format into network format. 9000 -> 00 23 28

    // 03: bind address to socket
    if (bind(serverSocketFd, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) { // size of use bcz os doesnt know it is ipv6 or v4 || sockaddr_in, sockaddr_in6 both are new and bind is old, so we haave to convert this into sockaddr
        cout << "Binding in server failed" << endl;
        close(serverSocketFd);
        return 1; // if this fails then port already in use
    }

    // 04: listen from all the clients
    if (listen(serverSocketFd, 5) == -1) {
        cout << "Server listening failed" << endl;
        close(serverSocketFd);
        return 1;
    }

    // 05: accept client
    cout << "Server waiting for client....." << endl;
    int serverToClientSocketFd = accept(serverSocketFd, nullptr, nullptr); // actual communication socket
    if (serverToClientSocketFd == -1) {
        cout << "Server accept failed" << endl;
        close(serverSocketFd);
        return 1;
    }

    // 06: receive data
    // consider TCP is water pipe
    // TCP is stream protocol not message protocol
    // TCP gurantees order + reliability, but not grouping bcz TCP splits data into segments
    // so there is chances that server get 5 bytes at once, or 2 bytes then remaning 3 bytes and so on, recv() call multiple times
    char buffer[1024] = {0};
    ssize_t bytesReceivedFromClient = recv(serverToClientSocketFd, buffer, sizeof(buffer)-1, 0); // copy data from client socket from kernel TCP buffer into buffer || buffer -1 for last char '\0'

    if (bytesReceivedFromClient == -1) cout << "Server Received failed" << endl;
    else if (bytesReceivedFromClient == 0) cout << "Client disconnected" << endl;
    else {
        buffer[bytesReceivedFromClient] = '\0'; // convert into valid string
        cout << "Client says: " << buffer << endl;
    }
 
    // 07: send reply
    const char* sendMsgToClient = "Hello from server to client";
    ssize_t bytesSentToClient = send(serverToClientSocketFd, sendMsgToClient, strlen(sendMsgToClient), 0); // it only gurantees that data is handover to client
    if (bytesSentToClient == -1) cout << "Reply send to client failed" << endl;
    
    // 08: close sockets
    close(serverToClientSocketFd); // release structure from kernel
    close(serverSocketFd);

    return 0;
}
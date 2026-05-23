#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <fstream>
using namespace std;

int main() {

    // 01: create server socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0); // listening socket
    if (serverSocketFd == -1) {
        cout << "Server socket creation failed" << endl;
        return 1;
    }

    // after restarting the server its bind fails bcz OS keeps port in TIME_WAIT state for a while
    // it helps server to reuse the same IP + port immediately after restarting 
    int opt = 1; // enable
    setsockopt(
        serverSocketFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

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

    /* // 06: receive data
    // consider TCP is water pipe
    // TCP is stream protocol not message protocol
    // TCP gurantees order + reliability, but not grouping bcz TCP splits data into segments
    // so there is chances that server get 5 bytes at once, or 2 bytes then remaning 3 bytes and so on, recv() call multiple times

    // loop bcz i want TCP connection continuous
    // Deadlock:
    // Case 01: if server in recv and client dont send data -> forever stuck
    // Case 02: if server and client both are on recv -> deadlock
    // so, we decided for client first and server
    while (true) {
        char buffer[1024] = {0};
        ssize_t bytesReceivedFromClient = recv(serverToClientSocketFd, buffer, sizeof(buffer)-1, 0); // copy data from client socket from kernel TCP buffer into buffer || buffer -1 for last char '\0'

        if (bytesReceivedFromClient == -1) {
            cerr << "Server Received failed" << endl;
            break;
        }
        else if (bytesReceivedFromClient == 0) { // eof
            cerr << "Client disconnected" << endl;
            break;
        }

        buffer[bytesReceivedFromClient] = '\0'; // convert into valid string
        string messageFromClient(buffer);

        if (messageFromClient == "exit") {
            cout << "Client requested disconnected" << endl;
            break;
        }
        cout << "Server received msg client says: " << messageFromClient << endl;

        // 07: send reply
        string sendMsgToClient = "Hello from server to client, client said: " + messageFromClient;
        ssize_t bytesSentToClient = send(serverToClientSocketFd, sendMsgToClient.c_str(), sendMsgToClient.size(), 0); // it only gurantees that data is handover to client
        if (bytesSentToClient == -1) {
            cout << "Reply send to client failed" << endl;
            break;
        }
    } */

    // 06A: receive file
    ofstream outputFile("received.txt", ios::binary);
    if (!outputFile) {
        cerr << "Failed to open received.txt" << endl;
        close(serverToClientSocketFd);
        close(serverSocketFd);
        return 1; 
    }


    const size_t BUFFER_SIZE = 4096;
    char writeFileBuffer[BUFFER_SIZE];
    long long totalBytesReceived = 0;
    while (true) {
        ssize_t bytesReceivedForFile = recv(serverToClientSocketFd, writeFileBuffer, sizeof(writeFileBuffer), 0);

        if (bytesReceivedForFile == -1) {
            cerr << "recieved file failed" << endl;
            break;
        }
        else if (bytesReceivedForFile == 0) {
            cerr << "client closed connection" << endl;
            break;
        }

        // dont setup '\0' bcz file bite is not a string
        outputFile.write(writeFileBuffer, bytesReceivedForFile);
        totalBytesReceived += bytesReceivedForFile;
    }

    cout << "Total bytes received: " << totalBytesReceived << endl;

    outputFile.close();
    // 08: close sockets
    close(serverToClientSocketFd); // release structure from kernel
    close(serverSocketFd);

    return 0;
}
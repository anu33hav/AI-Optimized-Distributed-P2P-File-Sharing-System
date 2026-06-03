#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
using namespace std;

int main() {

    // create socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd == -1) {
        cerr << "server socket creation failed" << endl;
        return -1;
    }

    // to reuse the port instantly after 
    int opt = 1; // enable
    if (setsockopt(
        serverSocketFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    ) == -1){
        cerr << "failed to reuse same socket" << endl;
        close(serverSocketFd);
        return -1;
    }

    // prepare address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9001);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind address with socket
    if (bind(serverSocketFd, (sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "binding failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    // listen
    if (listen(serverSocketFd, 5) == -1) {
        cerr << "listening failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    // accept
    int serverToClientSocketFd = accept(serverSocketFd, nullptr, nullptr);
    if (serverToClientSocketFd == -1) {
        cerr << "serverToClient socket creation failed" << endl;
        close(serverSocketFd);
        return -1;
    }

    // open file
    ofstream outputFile("output.txt", ios::binary);
    if (!outputFile) {
        cerr << "Failed to open output file" << endl;
        close(serverToClientSocketFd);
        close(serverSocketFd);
        return -1;
    }

    // recv and write
    long long totalByteReceive = 0;
    const size_t BUFFER_SIZE = 4096;
    while (true) {
        // recv
        char receiveBuffer[BUFFER_SIZE] = {0};

        ssize_t byteReceive = recv(serverToClientSocketFd, receiveBuffer, BUFFER_SIZE-1, 0);
        if (byteReceive == -1) {
            cerr << "Server receive failed" << endl;
            break;
        }
        else if (byteReceive == 0) {
            cerr << "Client disconnected" << endl;
            break;
        }

        // '\0' not handled bcz file transfer is not a
        outputFile.write(receiveBuffer, byteReceive); // byteReceive how much it write
        totalByteReceive += byteReceive;
    }

    outputFile.close();
    // close
    close(serverToClientSocketFd);
    close(serverSocketFd);

    cout << "Total byte Receive: " << totalByteReceive << endl;

    return 0;

}
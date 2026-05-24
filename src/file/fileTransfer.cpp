#include <file/fileTransfer.h>
#include <network/socketUtils.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

long long sendFile(int clientSocketFd, const char* filePath) {
    
    ifstream inputFile(filePath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open file" << endl;
        close(clientSocketFd);
        return -1;
    }

    const size_t BUFFER_SIZE = 4096;
    char readFileBuffer[BUFFER_SIZE];
    
    long long totalBytesSent = 0;

    while (inputFile) {
        inputFile.read(readFileBuffer, BUFFER_SIZE);

        streamsize bytesReadFromFileBuffer = inputFile.gcount();

        if (bytesReadFromFileBuffer > 0) {
            ssize_t bytesSentToServer = sendAll(clientSocketFd, readFileBuffer, bytesReadFromFileBuffer);
            
            if (bytesSentToServer == -1) {
                close(clientSocketFd);
                return -1;
            }

            totalBytesSent += bytesReadFromFileBuffer;
        }
    }

    return totalBytesSent;
}

long long receiveFile(int serverToClientSocketFd, const char* outputPath) {

    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) {
        cerr << "Failed to open output file" << endl;
        close(serverToClientSocketFd);
        return -1;
    }

    const size_t BUFFER_SIZE = 4096;
    char writeFileBuffer[BUFFER_SIZE];
    
    long long totalBytesReceived = 0;
    while (true) {
        ssize_t bytesReceivedForFile = recv(serverToClientSocketFd, writeFileBuffer, sizeof(writeFileBuffer), 0);

        if (bytesReceivedForFile == -1) {
            cerr << "Received failed" << endl;
            return -1;
        }
        else if (bytesReceivedForFile == 0) {
            cerr << "Client closed connection" << endl;
            break;
        }

        outputFile.write(writeFileBuffer, bytesReceivedForFile);
        totalBytesReceived += bytesReceivedForFile;
    }

    return totalBytesReceived;
}
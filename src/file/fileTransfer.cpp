#include "file/fileTransfer.h"
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include "network/socketUtils.h"
using namespace std;

static constexpr size_t kTransferBufferSize = 16*1024;

long long sendFile(int socketFd, const char* inputPath) {

    // 01: open file in read mode
    ifstream inputFile(inputPath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open inputFile" << endl;
        return -1;

    }

    // 02: read till eof
    long long sendFileByte = 0;
    char readBuffer[kTransferBufferSize];

    while (inputFile) {
        // read file
        inputFile.read(readBuffer, (streamsize)kTransferBufferSize);
        // how much bytes it read
        streamsize byteRead = inputFile.gcount();

        if (byteRead > 0) {
            ssize_t byteSend = sendAll(socketFd, readBuffer, (size_t)byteRead);
            if (byteSend == -1) {
                cerr << "failed to send data" << endl;
                break;
            }

            sendFileByte += byteRead;
        }
    }

    // 03: close file
    inputFile.close();

    return sendFileByte;
}

long long recvFile(int socketFd, const char* outputPath) {

    // 01: open output file
    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) {
        cerr << "Failed to outputFile" << endl;
        return -1;
    }

    // 02: write in file
    long long receiveFileByte = 0;
    char writeBuffer[kTransferBufferSize];
    while (true) {
        ssize_t byteReceive = recv(socketFd, writeBuffer, kTransferBufferSize, 0);
        if (byteReceive == -1) {
            cerr << "Failed to recv file" << endl;
            break;
        }
        else if (byteReceive == 0) {
            cerr << "Client disconnected" << endl;
            break;
        }

        // write in file
        outputFile.write(writeBuffer, byteReceive);
        receiveFileByte += byteReceive;
    }

    // 03: close file
    outputFile.close();

    return receiveFileByte;
}
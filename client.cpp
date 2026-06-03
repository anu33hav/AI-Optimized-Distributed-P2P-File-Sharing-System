#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
using namespace std;

int main() {

    // create client socket
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        cerr << "client socket creation failed" << endl;
        return -1;
    }

    // prepare address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9001);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        cerr << "failed to assign address" << endl; 
        close(clientSocketFd);
        return -1;
    }

    // connect with server
    if (connect(clientSocketFd, (sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to connect with server" << endl;
        close(clientSocketFd);
        return -1;
    }


    ifstream inputFile("input.txt", ios::binary);
    if (!inputFile) {
        cerr << "Failed to open input file" << endl;
        close(clientSocketFd);
        return -1;
    }

    // read and send
    long long totalByteSent = 0;
    const size_t BUFFER_SIZE = 4096;
    while (inputFile) {

        // read file
        char sendBuffer[BUFFER_SIZE];
        inputFile.read(sendBuffer, BUFFER_SIZE);

        int currentSend = 0;
        streamsize byteRead = inputFile.gcount();

        while (currentSend < byteRead) {

            if (byteRead > 0) {
                ssize_t byteSend = send(clientSocketFd, sendBuffer + currentSend, byteRead - currentSend, 0);
                if (byteSend == -1) {
                    cerr << "Failed to send the file" << endl;
                    break;
                }
                else if (byteSend == 0) {
                    cerr << "Not send any data" << endl;
                    break;
                }

                currentSend += byteSend;
                totalByteSent += currentSend;
            }
        }
    }


    inputFile.close();
    close(clientSocketFd);
    cout << "File sent successfully" << endl;
    cout << "Total Byte Sent " << totalByteSent << endl; 
    return 0;
}
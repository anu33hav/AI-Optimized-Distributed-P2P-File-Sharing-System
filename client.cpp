#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
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
        cerr << "Client to inet_ptonserver connection failed" << endl;
        close(clientSocketFd);
        return 1;
    }

    /* // 05: send message
    while (true) { 
        string sendMsgToServer;
        
        cout << "Enter msg for server: ";
        if (!getline(cin, sendMsgToServer)) { // if terminal closes with input ctrl+d or input pipe ends then getline() fails and infinite loop for (enter msg for server)
            cout << "Input closed" << endl;
            break;
        }
        if (sendMsgToServer.empty()) continue;

        ssize_t bytesSentToServer = send(clientSocketFd, sendMsgToServer.c_str(), sendMsgToServer.size(), 0);

        if (bytesSentToServer == -1) {
            cerr << "Msg send to server is failed";
            break;
        }
        if (sendMsgToServer == "exit") {
            cout << "Disconnected" << endl;
            break;
        }


        // 06: receive reply
        char buffer[1024] = {0};
        ssize_t bytesReceivedFromServer = recv(clientSocketFd, buffer, sizeof(buffer)-1, 0);

        if (bytesReceivedFromServer == -1) {
            cerr << "Msg received from client failed" << endl;
            break;
        }
        else if (bytesReceivedFromServer == 0) {
            cerr << "Server closed connection" << endl;
            break;
        }

        buffer[bytesReceivedFromServer] = '\0';
        cout << "Server says: " << buffer << endl;
    } */

    // 05A: read and send file
    ifstream inputFile("input.txt", ios::binary); // used to read || ifstream means pipe from file to program
    if (!inputFile) {
        cerr << "Failed to open input file in client side" << endl;
        close(clientSocketFd);
        return 1;
    }

    const size_t BUFFER_SIZE = 4096;
    char readFileBuffer[BUFFER_SIZE]; // 1st iteration abcdefg and on 2nd iteration it contains hijk, now efg will also retain, but i am using gcount() and to sent i have also uses how much byte i want to send
    long long totalBytesSent = 0;
    while (inputFile) { // loop bcz if file > 1024 || loop till eof valid, not corrupted or read not failed
        
        inputFile.read(readFileBuffer, BUFFER_SIZE);
        
        streamsize bytesReadFromFileBuffer = inputFile.gcount(); // suppose out 1024 300 is reamaning then its only store upto 724
        
        streamsize totalSent = 0;

        while (totalSent < bytesReadFromFileBuffer) { // suppose kernel buffer have limited space, so for this we use loop
            ssize_t bytesSentToServer = send(clientSocketFd, readFileBuffer + totalSent, bytesReadFromFileBuffer - totalSent, 0);
            
            if (bytesSentToServer == -1) {
                cerr << "Failed to send file data" << endl; 
                close(clientSocketFd);
                return 1;
            }
            else if (bytesSentToServer == 0) {
                cerr << "Internal problem failed to send file data" << endl;
                close(clientSocketFd);
                return 1;
            }

            totalSent += bytesSentToServer;
            totalBytesSent += bytesSentToServer;
        }
    }

    cout << "Total bytes sent: " << totalBytesSent << endl;

    inputFile.close();
    close(clientSocketFd);
    cout << "File sent successfully" << endl;

    return 0;
}
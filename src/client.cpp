#include "network/socketUtils.h"
#include "file/fileTransfer.h"
#include <iostream>
using namespace std;

int main() {

    int clientSocketFd = connectToServer("127.0.0.1", 9001);
    if (clientSocketFd == -1) {
        cerr << "failed to create client socket" << endl;
        return -1;
    }

    long long totalByteSend = sendFile(clientSocketFd, "data/input.txt");
    cout << "Total Byte Sent: " << totalByteSend << endl;

    closeSocket(clientSocketFd);

    return 0;
}
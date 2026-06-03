#include "network/socketUtils.h"
#include "file/fileTransfer.h"
#include <iostream>
using namespace std;

int main() {

    int serverSocketFd = createServerSocket(9001);
    if (serverSocketFd == -1) {
        cerr << "Failed to create server socket" << endl;
        return -1;
    }
    int serverToClientSocketFd = acceptClient(serverSocketFd);
    if (serverToClientSocketFd == -1) {
        cerr << "Failed to create accept client" << endl;
    }

    long long totalByteRecv = recvFile(serverToClientSocketFd, "data/output.txt");
    cout << "Total byte recv: " << totalByteRecv << endl;

    closeSocket(serverToClientSocketFd);
    closeSocket(serverSocketFd);

    return 0;
}
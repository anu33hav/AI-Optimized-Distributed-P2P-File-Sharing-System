#include <protocol/protocolParser.h>
#include <iostream>
using namespace std;

int main() {
    auto a = parseMessage("CONNECT peer1\n");
    auto b = parseMessage("REQUEST file1 2\n");
    auto c = parseMessage("ERROR not_found\n");

    cout << messageTypeToString(a.type) << " " << a.payload << endl;
    cout << messageTypeToString(b.type) << " " << b.fileId << " " << b.chunkId << " " << b.payload << endl;
    cout << messageTypeToString(c.type) << " " << c.payload << endl;
    
    return 0;
}
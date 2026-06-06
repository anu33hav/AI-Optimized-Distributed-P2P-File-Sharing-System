#include "peer/peer.h"
#include <iostream>
using namespace std;

int main() {
    PeerConfig config;
    config.peerId = "peer1";
    config.port = 9001;
    config.baseChunkDir = "data/chunks";

    if (!startPeerServer(config)) {
        cerr << "Peer server failed" << endl;
        return -1;
    }

    return 0;
}


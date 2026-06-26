#include "peer/peer.h"
#include <iostream>
using namespace std;

int main() {
    PeerConfig config;
    config.peerId = "peer1";
    config.port = 9001;
    config.localRootDir = "data/peers/peer1";
    config.chunkDir = "data/peers/peer1/chunks";
    config.downloadDir = "data/peers/peer1/downloads";
    config.reconstructedDir = "data/peers/peer1/reconstructed";



    if (!startPeerServer(config)) {
        cerr << "Peer server failed" << endl;
        return -1;
    }

    return 0;
}


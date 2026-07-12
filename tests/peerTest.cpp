#include "peer/peer.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void heartbeat(PeerConfig config) {
    
    while (true) {
        if (!sendHeartbeatToService(config, "127.0.0.1", 8080)) {
            cerr << "heartbeat failed" << endl;
        }
        this_thread::sleep_for(chrono::seconds(5));
    }
}

int main() {
    PeerConfig config;
    config.peerId = "peer1";
    config.port = 9001;
    config.localRootDir = "data/peers/peer1";
    config.chunkDir = "data/peers/peer1/chunks";
    config.downloadDir = "data/peers/peer1/downloads";
    config.reconstructedDir = "data/peers/peer1/reconstructed";


    if (!registerPeerWithService(config, "127.0.0.1", "127.0.0.1", 8080)) {
        cerr << "Peer registration failed" << endl;
        return -1;
    }

    thread heartbeatThread(heartbeat, config);
    heartbeatThread.detach();

    if (!startPeerServer(config)) {
        cerr << "Peer server failed" << endl;
        return -1;
    }



    return 0;
}


#include "peer/peer.h"
#include <chrono>
#include <iostream>
#include <string> 
#include <thread>
#include <vector>

using namespace std;


void heartbeatLoop(PeerConfig config, const string &serviceIp, int servicePort, int rounds) {

    for (int i = 0; i < rounds; i++) {
        if (!sendHeartbeatToService(config, serviceIp, servicePort)) {
            cerr << "heartbeat failed for " << config.peerId << endl;
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

void queryLoop(const string &serviceIp, int servicePort, const string &fileId, const string &peerId, int rounds) {
    
    for (int i = 0; i < rounds; i++) {
        vector<PeerEndpoint> peers;
        if (requestPeersForFileService(serviceIp, servicePort, fileId, peers)) {
            cout << "[" << peerId << "] discovered " << peers.size() << " peers for " << fileId << endl;

            for (const auto &peer : peers) {
                cout << " " << peer.peerId << " " << peer.ip << " " << peer.port << endl;
            }
        }
        else {
            cerr << "[" << peerId << "] tracker lookup failed" << endl;
        }

        this_thread::sleep_for(chrono::seconds(2));
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        cerr << "usage: " << argv[0] << " <peerId> <port> <localRootDir> <fileId>" << endl;
        return -1;
    }

    string peerId = argv[1];
    int port = stoi(argv[2]);
    string localRootDir = argv[3];
    string fileId = argv[4];

    PeerConfig config;
    config.peerId = peerId;
    config.port = port;
    config.localRootDir = localRootDir;
    config.chunkDir = localRootDir + "/chunks";
    config.downloadDir = localRootDir + "/downloads";
    config.reconstructedDir = localRootDir + "/reconstructed";
    config.trackerIp = "127.0.0.1";
    config.trackerPort = 8080;

    const string serviceIp = "127.0.0.1";
    const int servicePort = 8080;

    if (!registerPeerWithService(config, serviceIp, serviceIp, servicePort)) {
        cerr << "Peer registration failed" << endl;
        return -1;
    }

    thread serverThread([&]() {
        if (!startPeerServer(config)) {
            cerr << "peer server failed for " << peerId << endl;
        }
    });
    serverThread.detach();

    thread hbThread(heartbeatLoop, config, serviceIp, servicePort, 8);
    thread lookupThread(queryLoop, serviceIp, servicePort, fileId, peerId, 8);

    hbThread.join();
    lookupThread.join();

    cout << "peer finished: " << peerId << endl;
    
    return 0;
}
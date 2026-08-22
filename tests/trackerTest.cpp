#include "peer/peer.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void heartbeatLoop(PeerConfig config, const string &serviceIp, int servicePort, int rounds) {
    for (int i = 0; i < rounds; i++) {
        if (!sendHeartbeatToService(config, serviceIp, servicePort)) {
            cerr << "heartbeat failed for " << config.peerId << endl;
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

void printPeers(const vector<PeerEndpoint> &peers) {
    cout << "discovered peers: " << peers.size() << endl;
    for (const auto &peer : peers) {
        cout << peer.peerId << " " << peer.ip << " " << peer.port << endl;
    }
}

void queryLoop(const string &serviceIp, int servicePort, const string &fileId, const string &peerId, int rounds) {
    for (int i = 0; i < rounds; i++) {
        vector<PeerEndpoint> peers;
        if (requestPeersForFileService(serviceIp, servicePort, fileId, peers)) {
            cout << "[" << peerId << "] discovered " << peers.size()
                 << " peers for " << fileId << endl;
            for (const auto &peer : peers) {
                cout << "  " << peer.peerId << " " << peer.ip << " " << peer.port << endl;
            }
        } else {
            cerr << "[" << peerId << "] tracker lookup failed" << endl;
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

int main(int argc, char *argv[]) {

    if (argc < 5) {
        cerr << "usage: " << argv[0] << " <peerId> <port> <localRootDir> <fileId> [totalChunks]" << endl;
        return -1;
    }

    string peerId = argv[1];
    int port = stoi(argv[2]);
    string localRootDir = argv[3];
    string fileId = argv[4];
    // int totalChunks = (argc >= 6) ? stoi(argv[5]) : 3;

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

    // thread hbThread(heartbeatLoop, config, serviceIp, servicePort);

    thread hbThread(heartbeatLoop, config, serviceIp, servicePort, 8);
    thread lookupThread(queryLoop, serviceIp, servicePort, fileId, peerId, 8);

    hbThread.join();
    lookupThread.join();

    // thread serverThread([&]() {
    //     if (!startPeerServer(config)) {
    //         cerr << "peer server failed" << endl;
    //     }
    // });
    // serverThread.detach();

    // this_thread::sleep_for(chrono::seconds(1));

    // vector<PeerEndpoint> peers;
    // if (!requestPeersForFileService(serviceIp, servicePort, fileId, peers)) {
    //     cerr << "failed to get peers from tracker" << endl;
    //     return -1;
    // }

    // printPeers(peers);

    // bool downloaded = false;          
    // for (const auto &peer : peers) {
    //     if (peer.peerId == config.peerId) continue;

    //     cout << "trying peer " << peer.peerId << " at " << peer.ip << ":" << peer.port << endl;

    //     string outputPath = config.reconstructedDir + "/" + fileId + ".txt";
    //     if (downloadFileFromPeer(config, peer.ip, peer.port, fileId, totalChunks, outputPath)) {
    //         cout << "download and reconstruction successful" << endl;
    //         downloaded = true;
    //         break;
    //     }

    //     cout << "download failed from this peer, trying next" << endl;
    // }

    // if (!downloaded) {
    //     cerr << "no peer could serve the file" << endl;
    //     return -1;
    // }
    // while (true) {
    //     this_thread::sleep_for(chrono::seconds(10));
    // }

    cout << "peer finished: " << peerId << endl;
    return 0;
}


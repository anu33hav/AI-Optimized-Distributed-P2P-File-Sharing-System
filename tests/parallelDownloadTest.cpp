#include "peer/peer.h"

#include <iostream>
#include <thread>
#include <chrono>



using namespace std;

void heartbeatLoop(PeerConfig config, const string &serviceIp, int servicePort) {

    while (true) {
        if (!sendHeartbeatToService(config, serviceIp, servicePort)) {
            cerr << "heartbeat failed for " << config.peerId << endl;
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

PeerConfig buildConfig(const string &peerId, int port, const string &rootDir) {
    PeerConfig config;
    config.peerId = peerId;
    config.port = port;
    config.localRootDir = rootDir;
    config.chunkDir = rootDir + "/chunks";
    config.downloadDir = rootDir + "/downloads";
    config.reconstructedDir = rootDir + "/reconstructed";
    return config;
}

int runServerPeer(const string &peerId, int port, const string &rootDir) {
    const string serviceIp = "127.0.0.1";
    const int servicePort = 8080;

    PeerConfig config = buildConfig(peerId, port, rootDir);

    if (!registerPeerWithService(config, serviceIp, serviceIp, servicePort)) {
        cerr << "registration failed for " << peerId << endl;
        return -1;
    }

    thread hbThread(heartbeatLoop, config, serviceIp, servicePort);
    hbThread.detach();

    cout << "server peer started: " << peerId << " on port " << port << endl;

    if (!startPeerServer(config)) {
        cerr << "peer server failed for " << peerId << endl;
        return -1;
    }

    return 0;
}

int runDownloaderPeer(const string &peerId, int port, const string &rootDir, const string &fileId, int totalChunks) {
    const string serviceIp = "127.0.0.1";
    const int servicePort = 8080;

    PeerConfig config = buildConfig(peerId, port, rootDir);
    if (!registerPeerWithService(config, serviceIp, serviceIp, servicePort)) {
        cerr << "registration failed for " << peerId << endl;
        return -1;
    }

    thread hbThread(heartbeatLoop, config, serviceIp, servicePort);
    hbThread.detach();

    vector<PeerEndpoint> peers;
    if (!requestPeersForFileService(serviceIp, servicePort, fileId, peers)) {
        cerr << "failed to get peers from tracker" << endl;
        return -1;
    }

    cout << "tracker returned " << peers.size() << " peers" << endl;
    for (const auto &peer : peers) {
        cout << peer.peerId << " " << peer.ip << " " << peer.port << endl;
    }

    string outputPath = config.reconstructedDir + "/" + fileId + ".txt";

    if (!downloadFileFromMuliplePeers(config, peers, fileId, totalChunks, outputPath)) {
        cerr << "parallel download failed" << endl;
        return -1;
    }

    cerr << "parallel donwlaod successful" << endl;
    cout << "output: " << outputPath << endl;

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        cerr << "usage:" << endl;
        cerr << "   server mode:    " << argv[0] << " serve <peerId> <port> <rootDir>" << endl;
        cerr << "   download mode: " << argv[0] << " download <peerId> <port> <rootDir> <fileId> <totalChunks>" << endl;
        return -1;
    }

    string mode = argv[1];
    string peerId = argv[2];
    int port = stoi(argv[3]);
    string rootDir = argv[4];

    if (mode == "serve") {
        return runServerPeer(peerId, port, rootDir);
    }

    if (mode == "download") {
        if (argc < 7) {
            cerr << "download mode needs <fileId> <totalChunks>" << endl;
            return -1;
        }

        string fileId = argv[5];
        int totalChunks = stoi(argv[6]);

        return runDownloaderPeer(peerId, port, rootDir, fileId, totalChunks);
    }

    cerr << "unknown mode: " << mode << endl;
    return -1;
}
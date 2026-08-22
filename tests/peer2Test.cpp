#include "peer/peer.h"
#include <filesystem>
#include <iostream>

int main() {
    PeerConfig config;
    config.peerId = "peer2";
    config.port = 9002;
    config.localRootDir = "data/peers/peer2";
    config.chunkDir = "data/peers/peer2/chunks";
    config.downloadDir = "data/peers/peer2/downloads";
    config.reconstructedDir = "data/peers/peer2/reconstructed";
    config.trackerIp = "127.0.0.1";
    config.trackerPort = 8080;

    std::filesystem::create_directories(config.reconstructedDir);

    if (!downloadFileFromPeer(
            config,
            "127.0.0.1",
            9001,
            "inputA",
            3,
            config.reconstructedDir + "/inputA.txt"
        )) {
        std::cerr << "Download failed" << std::endl;
        return 1;
    }

    std::cout << "Download and reconstruction passed" << std::endl;
    return 0;
}
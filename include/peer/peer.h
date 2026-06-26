#pragma once
#include <string>

struct PeerConfig {
    std::string peerId;
    int port;

    std::string localRootDir;
    std::string chunkDir;
    std::string downloadDir;
    std::string reconstructedDir;
};


bool startPeerServer(const PeerConfig &config); // server side
bool connectToPeer(const std::string &ip, int port); // client side
bool requestChunkFromPeer(const PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int chunkId); // client side
bool downloadFileFromPeer(const  PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int totalChunks, const std::string &outputPath);
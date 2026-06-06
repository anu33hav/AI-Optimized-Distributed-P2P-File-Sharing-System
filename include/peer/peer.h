#pragma once
#include <string>

struct PeerConfig {
    std::string peerId;
    int port;
    std::string baseChunkDir;
};


bool startPeerServer(const PeerConfig &config);
bool connectToPeer(const std::string &ip, int port);
bool requestChunkFromPeer(const std::string &ip, int port, const std::string &fileId, int chunkId);
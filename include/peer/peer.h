#pragma once
#include <string>

struct PeerConfig {
    std::string peerId;
    int port;
    std::string baseChunkDir;
};


bool startPeerServer(const PeerConfig &config); // server side
bool connectToPeer(const std::string &ip, int port); // client side
bool requestChunkFromPeer(const PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int chunkId); // client side
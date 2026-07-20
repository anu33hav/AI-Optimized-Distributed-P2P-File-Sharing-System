#pragma once
#include <string>
#include <vector>

struct PeerConfig {
    std::string peerId;
    int port;

    std::string localRootDir;
    std::string chunkDir;
    std::string downloadDir;
    std::string reconstructedDir;
};

struct PeerEndpoint {
    std::string peerId;
    std::string ip;
    int port;
};

enum class ChunkDownloadStatus {
    NOT_STARTED,
    IN_PROGRESS,
    DONE,
    FAILED
};

bool startPeerServer(const PeerConfig &config); // server side
bool connectToPeer(const std::string &ip, int port); // client side
bool requestChunkFromPeer(const PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int chunkId); // client side
bool downloadFileFromPeer(const  PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int totalChunks, const std::string &outputPath);

bool registerPeerWithService(const PeerConfig &config, const std::string &peerIp, const std::string &serviceIp, int servicePort);

bool sendHeartbeatToService(const PeerConfig &config, const std::string &serviceIp, int servicePort);

bool requestPeersForFileService(const std::string &serviceIp, int servicePort, const std::string &fileId, std::vector<PeerEndpoint> &peers);

bool downloadFileFromMultiplePeers(const PeerConfig &config, const std::vector<PeerEndpoint> &peers, const std::string &fileId, int totalChunks, const std::string &outputFilePath);

bool requestChunkFromAnyPeer(const PeerConfig &config, const std::vector<PeerEndpoint> &usablePeers, const std::string &fileId, int chunkId, size_t startPeerIndex);
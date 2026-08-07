#pragma once
#include <string>

// for setting only these options are allowed
enum class MessageType {
    CONNECT,
    REQUEST,
    CHUNK,
    COMPLETE,
    ERROR,
    UNKNOWN
};

std::string buildConnectMessage(const std::string &peerId);
std::string buildRequestMessage(const std::string &fileId, int chunkId);
std::string buildChunkMessage(const std::string &fileId, int chunkId, std::size_t chunkSize, const std::string &chunkHash);
std::string buildCompleteMessage();
std::string buildErrorMessage(const std::string &errorText);
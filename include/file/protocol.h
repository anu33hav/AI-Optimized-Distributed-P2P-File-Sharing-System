#pragma once
#include <string>

enum class MessageType {
    CONNECT,
    REQUEST,
    CHUNK,
    COMPLETE,
    ERROR
};

std::string buildConnectMessage(const std::string &peerId);
std::string buildRequestMessage(const std::string &fileId, int chunkId);
std::string buildChunkMessage(const std::string &fileId, int chunkId, const std::string &data);
std::string buildCompleteMessage();
std::string buildErrorMessage(const std::string &errorText);
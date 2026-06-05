#pragma once
#include <string>

enum class MessageType {
    CONNECT,
    REQUEST,
    CHUNK,
    COMPLETE,
    ERROR,
    UNKNOWN
};

struct ParsedMessage {
    MessageType type = MessageType::UNKNOWN;
    std::string command;
    std::string fileId;
    int chunkId = -1;
    std::string payload;
    bool valid = false;
};

ParsedMessage parseMessage(const std::string &rawMessage);
std::string messageTypeToString(MessageType type);
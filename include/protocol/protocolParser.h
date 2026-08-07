#pragma once
#include <string>
#include "protocol/protocol.h"

struct ParsedMessage {
    MessageType type = MessageType::UNKNOWN;
    std::string command;
    std::string fileId;
    int chunkId = -1;
    std::size_t chunkSize = 0;
    std::string chunkHash;
    std::string payload;
    bool valid = false;
};

ParsedMessage parseMessage(const std::string &rawMessage);
std::string messageTypeToString(MessageType type);
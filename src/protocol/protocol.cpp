#include "protocol/protocol.h"
#include <string>
using namespace std;

string buildConnectMessage(const string &peerId) {
    return "CONNECT " + peerId + "\n";
}

string buildRequestMessage(const std::string &fileId, int chunkId) {
    return "REQUEST " + fileId + " " + to_string(chunkId) + "\n";
}

string buildChunkMessage(const std::string &fileId, int chunkId, size_t chunkSize) {
    return "CHUNK " + fileId + " " + to_string(chunkId) + " " + to_string(chunkSize) + "\n";
}

string buildCompleteMessage() {
    return string("COMPLETE") + "\n";
}

string buildErrorMessage(const std::string &errorText) {
    return "ERROR " + errorText + "\n";
}
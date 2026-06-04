#include "file/protocol.h"
#include <string>
using namespace std;

string buildConnectMessage(const string &peerId) {
    return "CONNECT " + peerId + "\n";
}

string buildRequestMessage(const std::string &fileId, int chunkId) {
    return "REQUEST " + fileId + " " + to_string(chunkId) + "\n";
}

string buildChunkMessage(const std::string &fileId, int chunkId, const std::string &data) {
    return "CHUNK " + fileId + " " + to_string(chunkId) + " " + data + "\n";
}

string buildCompleteMessage() {
    return string("COMPLETE") + "\n";
}

string buildErrorMessage(const std::string &errorText) {
    return "ERROR " + errorText + "\n";
}
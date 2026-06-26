#include "peer/peer.h"
#include "network/socketUtils.h"
#include <unistd.h>
#include <iostream>
#include "protocol/protocol.h"
#include <sys/socket.h>
#include "protocol/protocolParser.h"
#include "file/chunkManager.h"
#include <fstream>
#include <filesystem>
using namespace std;


bool handlePeerRequest(int serverToClientSocketFd, const PeerConfig &config); // startPeerServer helper
bool serveRequestedChunk(int serverToClientSocketFd, const PeerConfig &config, const std::string &fileId, int chunkId); // handlePeerRequest helper
bool sendChunkFileInBuffers(int serverToClientSocketFd, const string &chunkPath);

bool receiveChunkData(int clientSocketFd, const string &outputPath, size_t chunkSize);

// peer storage check
bool ensurePeerStorageLayout(const PeerConfig& config);
string findServableChunkPath(const PeerConfig &config, const string &fileId, int chunkId);

string findServableChunkPath(const PeerConfig &config, const string &fileId, int chunkId);

bool startPeerServer(const PeerConfig &config) { // server side

    // make peer structure
    if (!ensurePeerStorageLayout(config)) return false;

    // creeate peer serversocket
    int serverSocketFd = createServerSocket(config.port);
    if (serverSocketFd == -1) return false;

    cout << "Peer " << config.peerId << " waiting on port " << config.port << endl;

    // accept another peer
    int serverToClientSocketFd = acceptClient(serverSocketFd);
    if (serverToClientSocketFd == -1) {
        close(serverSocketFd);
        return false;
    }

    // need to handle the peer request
    bool requestFromPeer = handlePeerRequest(serverToClientSocketFd, config);

    // close sockets
    closeSocket(serverToClientSocketFd);
    closeSocket(serverSocketFd);

    // all done
    return requestFromPeer;
}

bool connectToPeer(const string &ip, int port) { // client side - use to check heartbeat

    // connect to server
    int clientSocketFd = connectToServer(ip.c_str(), port);
    if (clientSocketFd == -1) return false;

    // close socket and return
    closeSocket(clientSocketFd);

    // all good
    return true;
}

bool requestChunkFromPeer(const PeerConfig &config, const string &ip, int port, const string &fileId, int chunkId) { // client side
    
    // connect to the server/another peer
    int clientSocketFd = connectToServer(ip.c_str(), port);
    if (clientSocketFd == -1) return false;

    // build request message to send
    string requestMessage = buildRequestMessage(fileId, chunkId);
    cout << "Sending: " << requestMessage;
    if (sendAll(clientSocketFd, requestMessage.c_str(), requestMessage.size()) == -1) {
        closeSocket(clientSocketFd);
        return false;
    }

    // recv and process Header
    char headerBuffer[1024] = {0};
    ssize_t byteReceived = recv(clientSocketFd, headerBuffer, sizeof(headerBuffer)-1, 0);
    if (byteReceived <= 0) {
        closeSocket(clientSocketFd);
        return false;
    }
    headerBuffer[byteReceived] = '\0';
    string rawHeader(headerBuffer);
    // parse the header
    ParsedMessage header = parseMessage(rawHeader);
    if (!header.valid || header.type != MessageType::CHUNK) { // other than chunk
        cerr << "Invalid response" << endl;
        closeSocket(clientSocketFd);
        return false;
    }

    string outputPath = getChunkPath(config.downloadDir.c_str(), fileId.c_str(), chunkId);
    filesystem::create_directories(filesystem::path(outputPath).parent_path());

    bool received = receiveChunkData(clientSocketFd, outputPath, header.chunkSize);
    closeSocket(clientSocketFd);
    return received;
}

bool handlePeerRequest(int serverToClientSocketFd, const PeerConfig &config) { // serverside

    // recv request
    char buffer[1024] = {0};
    ssize_t byteReceive = recv(serverToClientSocketFd, buffer, sizeof(buffer)-1, 0);
    if (byteReceive <= 0) {
        cerr << "Failed to receive peer request" << endl;
        return false;
    }
    buffer[byteReceive] = '\0';
    string rawMessage(buffer);

    // parse message
    ParsedMessage msg = parseMessage(rawMessage);

    // not valid msg
    if (!msg.valid) {
        string errorMsg = buildErrorMessage("invalid_request");
        sendAll(serverToClientSocketFd, errorMsg.c_str(), errorMsg.size());
        return false;
    }

    // valid
    if (msg.type == MessageType::REQUEST) {
        return serveRequestedChunk(serverToClientSocketFd, config, msg.fileId, msg.chunkId);
    }

    // not valid req bcz this funcn is for only for REQUEST
    string errorMsg = buildErrorMessage("unsupported_command");
    sendAll(serverToClientSocketFd, errorMsg.c_str(), errorMsg.size());
    return false;
}

bool serveRequestedChunk(int serverToClientSocketFd, const PeerConfig &config, const std::string &fileId, int chunkId) {
    
    // find chunk Path
    string chunkPath = findServableChunkPath(config, fileId, chunkId);
    // check if chunk exists or not
    if (chunkPath.empty()) {
        string errorMsg = buildErrorMessage("chunk_not_found");
        sendAll(serverToClientSocketFd, errorMsg.c_str(), errorMsg.size());
        return false;
    }

    size_t chunkSize = filesystem::file_size(chunkPath);

    // prepare response and send -> buildChunkMessage want data at once, cant do that 
    string header = buildChunkMessage(fileId, chunkId, chunkSize);
    if (sendAll(serverToClientSocketFd, header.c_str(), header.size()) == -1) {
        return false;
    }

    // send file in buffers
    return sendChunkFileInBuffers(serverToClientSocketFd, chunkPath);
}

bool sendChunkFileInBuffers(int serverToClientSocketFd, const string &chunkPath) {

    // open file
    ifstream chunkFile(chunkPath, ios::binary);
    if (!chunkFile) {
        cerr << "Failed to open chunk file: " << chunkPath << endl;
        return false;
    }

    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    // till eof
    while (chunkFile) {
        // read file
        chunkFile.read(buffer, sizeof(buffer));
        streamsize byteRead = chunkFile.gcount();

        if (byteRead > 0) {
            if (sendAll(serverToClientSocketFd, buffer, byteRead) == -1) {
                cerr << "Failed while sending chunk bytes" << endl;
                return false;
            }
        }
    }

    // all good
    return true;
}

bool receiveChunkData(int clientSocketFd, const string &outputPath, size_t chunkSize) {

    // open outputFile
    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) return false;

    // recv in buffer
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t remaining = chunkSize;

    while (remaining > 0) {
        size_t toRead = min(BUFFER_SIZE, remaining);
        ssize_t byteReceived = recv(clientSocketFd, buffer, toRead, 0);

        if (byteReceived <= 0) return false;

        /// write in file
        outputFile.write(buffer, byteReceived);
        remaining -= byteReceived;
    }

    // all good
    return true;
}

bool ensurePeerStorageLayout(const PeerConfig& config) {
    
    try {
        if (!config.localRootDir.empty()) {
            filesystem::create_directories(config.localRootDir);
        }
        if (!config.chunkDir.empty()) {
            filesystem::create_directories(config.chunkDir);
        }
        if (!config.downloadDir.empty()) {
            filesystem::create_directories(config.downloadDir);
        }
        if (!config.reconstructedDir.empty()) {
            filesystem::create_directories(config.reconstructedDir);
        }
    }
    catch (filesystem::filesystem_error &err) {
        cerr << "failed to create peer directory" << err.what() << endl;
        return false;
    }

    return true;
}

string findServableChunkPath(const PeerConfig &config, const string &fileId, int chunkId) {
    
    // check upload path
    string uploadPath = getChunkPath(config.chunkDir.c_str(), fileId.c_str(), chunkId);
    if (filesystem::exists(uploadPath)) return uploadPath;

    // not find upload path then check in downloadPath
    string downloadPath = getChunkPath(config.downloadDir.c_str(), fileId.c_str(), chunkId);
    if (filesystem::exists(downloadPath)) return downloadPath;

    return string();
}
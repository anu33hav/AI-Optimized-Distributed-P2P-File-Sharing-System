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
#include <set>
#include <vector>
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

    // look for another peer also
    while (true) {
        // accept another peer
        int serverToClientSocketFd = acceptClient(serverSocketFd);
        if (serverToClientSocketFd == -1) {
            close(serverSocketFd);
            return false;
        }

        // need to handle the peer request
        bool requestFromPeer = handlePeerRequest(serverToClientSocketFd, config);
        
        // close socket
        closeSocket(serverToClientSocketFd);
        
        if (!requestFromPeer) break;
    }

    // close socketet
    closeSocket(serverSocketFd);

    // all done
    return true;
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

bool downloadFileFromPeer(const  PeerConfig &config,  const std::string &ip, int port, const std::string &fileId, int totalChunks, const std::string &outputFilePath) {
    
    // check for vald chunkcount
    if (totalChunks < 0) return false;

    // req for each chunks 
    for (int chunkId = 0; chunkId < totalChunks; chunkId++) {
        if (!requestChunkFromPeer(config, ip, port, fileId, chunkId)) {
            cerr << "Failed to reconstruct chunk " << chunkId << endl;
            return false;
        }
    }

    // check for reconstructed exists
    filesystem::create_directories(config.reconstructedDir);
    string reconstructedPath = config.reconstructedDir + "/" + fileId + ".txt";

    // merge chunks
    if (!mergeChunks(config.downloadDir.c_str(), fileId.c_str(), totalChunks, outputFilePath.c_str())) {
        cerr << "Failed to reconstruct file" << endl;
        return false;
    }

    // all good
    return true;
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


void scanDirectory(const string &folder, set<string> &fileIds) {

    // if folder empty or didnt exists 
    if (folder.empty()) return;;
    if (!filesystem::exists(folder)) return;

    for (const auto &entry : filesystem::directory_iterator(folder)) {
        if (entry.is_directory()) {
            string folderName = entry.path().filename().string();
            fileIds.insert(folderName);
        }
    }
}

vector<string> collectSharedFiles(const PeerConfig &config) {
    
    set<string> fileIds; // dont want duplicate file name

    scanDirectory(config.chunkDir, fileIds);
    scanDirectory(config.downloadDir, fileIds);

    return vector<string> (fileIds.begin(), fileIds.end());
}

string buildRegisterBody(const PeerConfig &config, const string &peerIp) {
    
    auto files = collectSharedFiles(config);

    ostringstream oss;
    oss << "{";
    oss << "\"peerId\":\"" << config.peerId << "\",";
    oss << "\"ip\":\"" << peerIp << "\",";
    oss << "\"port\":" << config.port << ",";
    oss << "\"files\":[";
    for (size_t i = 0; i < files.size(); i++) {
        if (i > 0) oss << ","; // add comma to after first file
        oss << "\"" << files[i] << "\"";
    }

    oss << "]";
    oss << "}";

    return oss.str();
}

bool registerPeerWithService(const PeerConfig &config, const std::string &peerIp, const std::string &serviceIp, int servicePort) {

    // connect peer to tracker
    int clientSocketFd = connectToServer(serviceIp.c_str(), servicePort);
    if (clientSocketFd == -1) return false;

    // json body to send to tracker
    string body = buildRegisterBody(config, peerIp);

    // create a big string for http request
    ostringstream request;
    request << "POST /register HTTP/1.1\r\n";
    request << "Host: " << serviceIp << ":" << servicePort << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n\r\n"; // blank line tells header is closed
    request << body;

    string requestText = request.str();
    if (sendAll(clientSocketFd, requestText.c_str(), (int)requestText.size()) == -1) {
        closeSocket(clientSocketFd);
        return false;
    }

    string response;
    char buffer[1024];
    while (true) {
        ssize_t n = recv(clientSocketFd, buffer, sizeof(buffer), 0);
        if (n < 0) {
            closeSocket(clientSocketFd);
            return false;
        }
        if (n == 0) {
            break;
        }
        response.append(buffer, (size_t)n);
    }

    closeSocket(clientSocketFd);
    return response.find("200 OK") != string::npos;
}
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
#include <mutex>
#include <thread>
#include "file/hashUtils.h"
using namespace std;


bool handlePeerRequest(int serverToClientSocketFd, const PeerConfig &config); // startPeerServer helper
bool serveRequestedChunk(int serverToClientSocketFd, const PeerConfig &config, const std::string &fileId, int chunkId); // handlePeerRequest helper
bool sendChunkFileInBuffers(int serverToClientSocketFd, const string &chunkPath);

bool readChunkHeader(int clientSocketFd, string &headerLine, string &remainder);
bool receiveChunkData(int clientSocketFd, const string &outputPath, size_t chunkSize, const string &initialPayload, const string &expectedChunkHash);

// peer storage check
bool ensurePeerStorageLayout(const PeerConfig& config);
string findServableChunkPath(const PeerConfig &config, const string &fileId, int chunkId);

string findServableChunkPath(const PeerConfig &config, const string &fileId, int chunkId);

string extractHttpBody(const string &response);
string extractJsonStringField(const string &objectText, const string &key);
bool extractJsonIntfield(const string &objectText, const string &key, int &value);

bool localChunkAlreadyExists(const PeerConfig &config, const string &fileId, int chunkId);
vector<PeerEndpoint> filterUsablePeers(const PeerConfig &config, const vector<PeerEndpoint> &peers);

void printChunkStatuses(const vector<ChunkDownloadStatus> &chunkStatus);
bool allChunksDone(const vector<ChunkDownloadStatus> &chunkStatus);
string chunkStatusToString(ChunkDownloadStatus status);

bool loadDownloadState(const PeerConfig &config, const string &fileId, vector<ChunkDownloadStatus> &chunkStatus, int totalChunks);
bool saveDownloadState(const PeerConfig &config, const string &fileId, const vector<ChunkDownloadStatus> &chunkStatus);
string getDownloadStatePath(const PeerConfig &config, const string &fileId);
ChunkDownloadStatus stringToChunkStatus(const string &text);

vector<int> findMissingChunks(const vector<ChunkDownloadStatus> &chunkStatus);
bool verifyAllChunksComplete(const PeerConfig &config, const string &fileId, const vector<ChunkDownloadStatus> &chunkStatus);
bool isChunkPresentAndValid(const PeerConfig &config, const string &fileId, int chunkId);

bool removeFileIfExists(const string &path);

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
        
        if (!requestFromPeer) continue; // dont kill the whole server because one request was bad
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
    if (clientSocketFd == -1) {
        cerr << "Failed to connect to peer " << ip << ":" << port << " for chunk " << chunkId << endl;
        return false;
    }

    // add timout to the socket, recv && send
    if (!setSocketRecvTimeout(clientSocketFd, 5)) {
        closeSocket(clientSocketFd);
        return false;
    }
    if (!setSocketSendTimeout(clientSocketFd, 5)) {
        closeSocket(clientSocketFd);
        return false;
    }

    // build request message to send
    string requestMessage = buildRequestMessage(fileId, chunkId);
    cout << "Sending: " << requestMessage;
    if (sendAll(clientSocketFd, requestMessage.c_str(), requestMessage.size()) == -1) {
        cerr << "Failed to send request for chunk " << chunkId << " to peer " << ip << ":" << port << endl;
        closeSocket(clientSocketFd);
        return false;
    }

    // recv and process header line without losing any chunk bytes that arrive in the same TCP read
    string rawHeader;
    string bufferedChunkBytes;
    if (!readChunkHeader(clientSocketFd, rawHeader, bufferedChunkBytes)) {
        closeSocket(clientSocketFd);
        return false;
    }
    // parse the header
    ParsedMessage header = parseMessage(rawHeader);
    if (!header.valid || header.type != MessageType::CHUNK) { // other than chunk
        cerr << "Invalid response for chunk " << chunkId << " from peer " << ip << ":" << port << ". Raw response: " << rawHeader << endl;
        closeSocket(clientSocketFd);
        return false;
    }

    string outputPath = getChunkPath(config.downloadDir.c_str(), fileId.c_str(), chunkId);
    filesystem::create_directories(filesystem::path(outputPath).parent_path());

    bool received = receiveChunkData(clientSocketFd, outputPath, header.chunkSize, bufferedChunkBytes, header.chunkHash);
    if (!received) {
        cerr << "Failed to receive chunk bytes for chunk " << chunkId << " from peer " << ip << ":" << port << endl;
    }
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

    // compute string
    string chunkHash;
    if (!computeFileSha256(chunkPath, chunkHash)) {
        string errorMsg = buildErrorMessage("chunk_hash_failed");
        sendAll(serverToClientSocketFd, errorMsg.c_str(), errorMsg.size());
        return false;
    }

    // prepare response and send -> buildChunkMessage want data at once, cant do that 
    string header = buildChunkMessage(fileId, chunkId, chunkSize, chunkHash);
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

bool receiveChunkData(int clientSocketFd, const string &outputPath, size_t chunkSize, const string &initialPayload, const string &expectedChunkHash) {

    // open outputFile
    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) return false;

    // recv in buffer
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t remaining = chunkSize;

    // got from header \n last chars
    if (!initialPayload.empty()) {
        size_t bytesToWrite = min(remaining, initialPayload.size());
        // write in output file
        outputFile.write(initialPayload.data(), (streamsize)bytesToWrite);
        if (!outputFile) {
            outputFile.close();
            removeFileIfExists(outputPath);
            return false;
        }
        remaining -= bytesToWrite;
    }

    while (remaining > 0) {
        size_t toRead = min(BUFFER_SIZE, remaining);
        ssize_t byteReceived = recv(clientSocketFd, buffer, toRead, 0);

        if (byteReceived < 0) {
            if (isSocketTimeoutError()) {
                cerr << "timeout while receiving chunk data" << endl;
            }
            else {
                cerr << "failed while receiving chunk data" << endl;
            }
            outputFile.close();
            removeFileIfExists(outputPath);
            return false;
        }
        if (byteReceived == 0) {
            cerr << "peer closed connection while sending chunk data" << endl;
            outputFile.close();
            removeFileIfExists(outputPath);
            return false;
        }

        // write in file
        outputFile.write(buffer, byteReceived);
        if (!outputFile) {
            outputFile.close();
            removeFileIfExists(outputPath);
            return false;
        }
        remaining -= (size_t)byteReceived;
    }
    // receive chunk data - close the file
    outputFile.close();

    // now, copmute the hash
    string actualHash;
    if (!expectedChunkHash.empty()) {
        // compute receive chunk hash
        if (!computeFileSha256(outputPath, actualHash)) {
            cerr << "failed to hash download chunk" << endl;
            removeFileIfExists(outputPath);
            return false;
        }
        // compare receive chunkhash to expectedChunkHash
        if (actualHash != expectedChunkHash) {
            cerr << "chunk hash mismatach for " << outputPath << endl;
            removeFileIfExists(outputPath);
            return false;
        }
    }

    // all good
    return true;
}

bool readChunkHeader(int clientSocketFd, string &headerLine, string &remainder) {
    headerLine.clear();
    remainder.clear();

    string accumulated;
    char buffer[1024];

    while (true) {
        // recv data
        ssize_t n = recv(clientSocketFd, buffer, sizeof(buffer), 0);
        // invalid data
        if (n < 0) {
            if (isSocketTimeoutError()) {
                cerr << "timeout waiting for chunk response header" << endl;
            }
            else {
                cerr << "failed to receive chunk response header" << endl;
            }
            return false;
        }
        if (n == 0) {
            cerr << "peer closed connection before sending chunk response header" << endl;
            return false;
        }

        // valid
        accumulated.append(buffer, (size_t)n);
        size_t newlinePos = accumulated.find('\n');
        if (newlinePos != string::npos) {
            headerLine = accumulated.substr(0, newlinePos + 1);
            remainder = accumulated.substr(newlinePos + 1);
            return true;
        }

        if (accumulated.size() > 8192) {
            cerr << "chunk response header too large" << endl;
            return false;
        }
    }

    // not done
    return false;
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
    if (folder.empty()) return;
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


string buildHeartbeatBody(const PeerConfig &config) {
    return "{\"peerId\":\"" + config.peerId + "\"}";
}

bool sendHeartbeatToService(const PeerConfig &config, const std::string &serviceIp, int servicePort) {

    // connect to service
    int clientSocketFd = connectToServer(serviceIp.c_str(), servicePort);
    if (clientSocketFd == -1) return false;

    // convert into json
    string body = buildHeartbeatBody(config);

    // convert into POST req
    ostringstream request;
    request << "POST /heartbeat HTTP/1.1\r\n";
    request << "Host: " << serviceIp << ":" << servicePort << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n\r\n";
    request << body;

    // convert into string and send
    string requestText = request.str();
    if (sendAll(clientSocketFd, requestText.c_str(), (int)requestText.size()) == -1) {
        closeSocket(clientSocketFd);
        return false;
    }

    // prepare response
    string response;
    char buffer[1024];
    while (true) {
        ssize_t n = recv(clientSocketFd, buffer, sizeof(buffer), 0);
        if (n < 0) { // failed
            closeSocket(clientSocketFd);
            return false;
        }
        if (n == 0) { // no response
            break;
        }
        // get response
        response.append(buffer, (size_t)n); 
    }
enum class ChunkDownloadStatus {
    NOT_STARTED,
    IN_PROGRESS,
    DONE,
    FAILED
};
    closeSocket(clientSocketFd);
    return response.find("200 OK") != string::npos;
}


bool requestPeersForFileService(const std::string &serviceIp, int servicePort, const std::string &fileId, std::vector<PeerEndpoint> &peers) {
    
    // want new peers vector alsways, dont want to prev peers data
    peers.clear();

    // connect to tracker
    int clientSocketFd = connectToServer(serviceIp.c_str(), servicePort);
    if (clientSocketFd == -1) return false;

    // build request
    ostringstream request;
    request << "GET /files?fileId=" << fileId << " HTTP/1.1\r\n";
    request << "Host: " << serviceIp << ":" << servicePort << "\r\n";
    request << "Connection: close\r\n\r\n";
    // send this request
    string requestText = request.str();
    if (sendAll(clientSocketFd, requestText.c_str(), (int)requestText.size()) == -1) {
        closeSocket(clientSocketFd);
        return false;
    }

    // get response
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

    // close socket after work
    closeSocket(clientSocketFd);

    // not get ok response
    if (response.find("200 OK") == string::npos) {
        return false;
    }

    // HTTP/1.1 200 OK
    // Content-Type: application/json
    // [{"peerId":"peer1","ip":"127.0.0.1","port":9001}]
    string body = extractHttpBody(response);
    if (body.empty()) {
        return true;
    }

    // parse json like obj
    size_t pos = 0;
    while (true) {
        // find start of the json
        size_t objStart = body.find('{', pos);
        if (objStart == string::npos) break;
        // find end of the json
        size_t objEnd = body.find('}', objStart);
        if (objEnd == string::npos) break;

        // get obj then assign into PeerEndPoint obj
        string objectText = body.substr(objStart, objEnd-objStart+1);
        PeerEndpoint peer;
        int port = 0;
        peer.peerId = extractJsonStringField(objectText, "peerId");
        peer.ip = extractJsonStringField(objectText, "ip");

        if (!peer.peerId.empty() && !peer.ip.empty() && extractJsonIntfield(objectText, "port", port) && port > 0) {
            peer.port = port;
            peers.push_back(peer);
        }

        pos = objEnd + 1;
    }

    return true;
}

string extractHttpBody(const string &response) {
    size_t bodyStart = response.find("\r\n\r\n");
    // not found
    if (bodyStart == string::npos) {
        return string();
    }

    // found
    return response.substr(bodyStart+4);
}

string extractJsonStringField(const string &objectText, const string &key) { // peerId and ip
    // {"peerId":"peer1","ip":"127.0.0.1","port":9001} + key="peerId" -> "peer1"
    // {"peerId":"peer1","ip":"127.0.0.1","port":9001} + key="ip" -> "127.0.0.1"
    string needle = "\"" + key + "\":\""; // /" == "

    size_t start = objectText.find(needle);
    if (start == string::npos) return string();

    start += needle.size();
    size_t end = objectText.find('"', start);
    if (end == string::npos) return string();

    return objectText.substr(start, end-start);
}

bool extractJsonIntfield(const string &objectText, const string &key, int &value) { // for port
    
    // find key
    string needle = "\"" + key + "\":";
    size_t start = objectText.find(needle);
    if (start == string::npos) return false;

    start += needle.size();
    size_t end = start;
    while (end < objectText.size() && isdigit(objectText[(int)end])) {
        end++;
    }
    if (end == start) return false;

    // try assigning value to port = value
    try {
        value = stoi(objectText.substr(start, end-start));
    }
    catch (...) {
        return false;
    }

    return true;
}


bool downloadFileFromMultiplePeers(const PeerConfig &config, const std::vector<PeerEndpoint> &peers, const std::string &fileId, int totalChunks, const std::string &outputFilePath) {

    // check valid chunk count
    if (totalChunks <= 0) return false;

    // valid count
    // filter peers, no duplicate, no invalid ip+port, no self
    vector<PeerEndpoint> usablePeers = filterUsablePeers(config, peers);
    if (usablePeers.empty()) {
        cerr << "No usable peers found" << endl;
        return false;
    }

    // set all chunk to NOT_STARTED
    vector<ChunkDownloadStatus> chunkStatus(totalChunks, ChunkDownloadStatus::NOT_STARTED);
    if (!loadDownloadState(config, fileId, chunkStatus, totalChunks)) {
        cerr << "Failed to load prev download state" << endl;
        return false;
    }

    set<int> requestedChunks; // should be unique
    mutex stateMutex;
    vector<thread> workers;

    // traverse on all chunks
    for (int chunkId = 0; chunkId < totalChunks; chunkId++) {

        // prev downloaded || already exists
        if (chunkStatus[chunkId] == ChunkDownloadStatus::DONE || localChunkAlreadyExists(config, fileId, chunkId)) {
            // mark chunk status done bc its already exxists
            chunkStatus[chunkId] = ChunkDownloadStatus::DONE;
            cout << "Skipping chunk " << chunkId << "because it already completed or exists" << endl;
            continue;
        }

        // not exists
        workers.emplace_back([&, chunkId] () { // use all varible in this func by reference, copy, copy

            // peer assignment
            size_t startPeerIndex = 0;
            {
                lock_guard<mutex> lock(stateMutex);

                // skip duplicate request
                if (requestedChunks.count(chunkId)) {
                    cout << "Skipping duplicate request for chunk: " << chunkId << endl;
                    return;
                }

                // not a duplicate request - store
                requestedChunks.insert(chunkId);
                // mark status for this particular chunkId to INPROGRESS
                chunkStatus[chunkId] = ChunkDownloadStatus::IN_PROGRESS;

                // scheduling round robin - load is distributed
                startPeerIndex = chunkId % usablePeers.size();
                const PeerEndpoint &assignedPeer = usablePeers[startPeerIndex];

                cout << "Chunk " << chunkId << " initially assigned to " << assignedPeer.peerId << " " << assignedPeer.ip << ":" << assignedPeer.port << endl;
            }

            // request chunk from peer
            bool ok = requestChunkFromAnyPeer(config, usablePeers, fileId, chunkId, startPeerIndex);
            {
                lock_guard<mutex> lock(stateMutex);
                // got chunk
                if (ok) {
                    chunkStatus[chunkId] = ChunkDownloadStatus::DONE;
                    saveDownloadState(config, fileId, chunkStatus);
                } 
                else {
                    chunkStatus[chunkId] = ChunkDownloadStatus::FAILED;
                    saveDownloadState(config, fileId, chunkStatus);
                }
            }
        });
    }

    // pause main and wait for thread to execute
    for (auto &worker : workers) {
        worker.join();
    }

    // save final state
    saveDownloadState(config, fileId, chunkStatus);

    // print all the chunkstatus
    printChunkStatuses(chunkStatus);
    if (!allChunksDone(chunkStatus)) {
        cerr << "Not all chunks downloaded. Merge skipped." << endl;
        return false;
    }

    // find missing chunks
    vector<int> missingChunks = findMissingChunks(chunkStatus);
    if (!missingChunks.empty()) {
        cerr << "Missing chunks:";
        for (int chunkId : missingChunks) {
            cerr << " " << chunkId;
        }
        cerr << endl;
        cerr << "Not all chunks downloaded. Merge skipped." << endl;
        return false;
    }

    // verfiy chunks is complete
    if (!verifyAllChunksComplete(config, fileId, chunkStatus)) {
        cerr << "Chunk integrity check failed" << endl;
        return false;
    }

    // create reconstructed dir
    filesystem::create_directories(config.reconstructedDir);
    // try to merge all the chunks
    if (!mergeChunks(config.downloadDir.c_str(), fileId.c_str(), totalChunks, outputFilePath.c_str())) {
        cerr << "Failed to reconstruct file" << endl;
        return false;
    }

    return true;
}

vector<PeerEndpoint> filterUsablePeers(const PeerConfig &config, const vector<PeerEndpoint> &peers) {

    vector<PeerEndpoint> usable;
    set<string> seen;

    for (const auto &peer : peers) {

        // the peer requested for file have this some file chunks
        if (peer.peerId == config.peerId) continue;
        // not valid ip or port
        if (peer.ip.empty() || peer.port <= 0) continue;

        // create ip + port string
        string key = peer.ip + ":" + to_string(peer.port);
        // already seen dont, process again
        if (seen.count(key)) continue;

        seen.insert(key);
        usable.push_back(peer);
    }

    return usable;
}

bool loadDownloadState(const PeerConfig &config, const string &fileId, vector<ChunkDownloadStatus> &chunkStatus, int totalChunks) {
    
    // check invalid totalChunks
    if (totalChunks <= 0) return false;

    // donwloadPath = statePath
    string statePath = getDownloadStatePath(config, fileId);
    if (!filesystem::exists(statePath)) return true;

    // open state file
    ifstream in(statePath);
    if (!in) {
        cerr << "failed to open download state file for reading" << endl;
        return false;
    }

    // mark state in chunkStatus
    int chunkId;
    string statusText;
    while (in >> chunkId >> statusText) {
        // invalid chunk
        if (chunkId < 0 || chunkId >= totalChunks) continue;
        // valid chunk - mark the status
        chunkStatus[chunkId] = stringToChunkStatus(statusText);
    }

    return true;
}

string getDownloadStatePath(const PeerConfig &config, const string &fileId) {
    return config.downloadDir + "/" + fileId +  ".state";
}

ChunkDownloadStatus stringToChunkStatus(const string &text) {

    if (text == "NOT_STARTED") return ChunkDownloadStatus::NOT_STARTED;
    if (text == "IN_PROGRESS") return ChunkDownloadStatus::IN_PROGRESS;
    if (text == "DONE") return ChunkDownloadStatus::DONE;
    if (text == "FAILED") return ChunkDownloadStatus::FAILED;
    return ChunkDownloadStatus::NOT_STARTED;
}

bool localChunkAlreadyExists(const PeerConfig &config, const string &fileId, int chunkId) {
    
    // if chunk exists in downloadDir or chunkDir
    if (chunkExists(config.downloadDir.c_str(), fileId.c_str(), chunkId)
        || chunkExists(config.chunkDir.c_str(), fileId.c_str(), chunkId)) return true;


    // not found chunk
    return false;
}

bool saveDownloadState(const PeerConfig &config, const string &fileId, const vector<ChunkDownloadStatus> &chunkStatus) {
    
    // try to create downloadd dir
    try {
        filesystem::create_directories(config.downloadDir);
    }
    catch (...) {
        cerr << "failed to create downlooad dir for state file" << endl;
        return false;
    }

    // open downloaddir file
    ofstream out(getDownloadStatePath(config, fileId));
    if (!out) {
        cerr << "failed to open download state file for writing" << endl;
        return false;
    }
    // save status
    for (size_t i = 0; i < chunkStatus.size(); i++) {
        out << i << " " << chunkStatusToString(chunkStatus[i]) << "\n";
    }

    return true;
}


void printChunkStatuses(const vector<ChunkDownloadStatus> &chunkStatus) {
    
    cout << "Chunk status:" << endl;
    for (size_t i = 0; i < chunkStatus.size(); i++) {
        cout << "   chunk " << i << ": " << chunkStatusToString(chunkStatus[i]) << endl;
    }
}

bool allChunksDone(const vector<ChunkDownloadStatus> &chunkStatus) {

    for (ChunkDownloadStatus status : chunkStatus) {
        // NOT DONE
        if (status != ChunkDownloadStatus::DONE) {
            return false;
        }
    }

    // all done
    return true;
}

string chunkStatusToString(ChunkDownloadStatus status) {
    switch (status) {
        case ChunkDownloadStatus::NOT_STARTED:
            return "NOT_STARTED";
        case ChunkDownloadStatus::IN_PROGRESS:
            return "IN_PROGRESS";
        case ChunkDownloadStatus::DONE:
            return "DONE";
        case ChunkDownloadStatus::FAILED:
            return "FAILED";
    }

    return "UNKNOWN";
}


vector<int> findMissingChunks(const vector<ChunkDownloadStatus> &chunkStatus) {
    
    vector<int> missing;
    for (size_t i = 0; i < chunkStatus.size(); i++) {
        // not done, means missing
        if (chunkStatus[i] != ChunkDownloadStatus::DONE) {
            missing.push_back((int)i);
        }
    }

    return missing;
}

bool verifyAllChunksComplete(const PeerConfig &config, const string &fileId, const vector<ChunkDownloadStatus> &chunkStatus) {
    
    // traverse on all chunks
    for (size_t i = 0; i < chunkStatus.size(); i++) {
        // not done
        if (chunkStatus[i] != ChunkDownloadStatus::DONE) return false;
        // chunk is present and valid
        if (!isChunkPresentAndValid(config, fileId, (int)i)) return false;
    }

    // all valid
    return true;
}

bool isChunkPresentAndValid(const PeerConfig &config, const string &fileId, int chunkId) {

    // get path
    string downloadPath = getChunkPath(config.downloadDir.c_str(), fileId.c_str(), chunkId);
    string chunkPath = getChunkPath(config.chunkDir.c_str(), fileId.c_str(), chunkId);

    // get chunk path
    string pathToCheck;
    if (filesystem::exists(downloadPath)) pathToCheck = downloadPath;
    else if (filesystem::exists(chunkPath)) pathToCheck = chunkPath;
    else return false;

    try {
        return filesystem::file_size(pathToCheck) > 0; // file inc some bytes or non empty
    }
    catch (...) {
        return false;
    }
}

bool requestChunkFromAnyPeer(const PeerConfig &config, const vector<PeerEndpoint> &usablePeers, const string &fileId, int chunkId, size_t startPeerIndex) {

    // any peer exists
    if (usablePeers.empty()) {
        cerr << "No usable peers found" << endl;
        return false;
    }

    // retry on all peers that have fileId, retry attempt only usablePeers.size()
    for (size_t attempt = 0; attempt < usablePeers.size(); attempt++) {

        // deciding which peer for retry, 
        size_t peerIndex = (startPeerIndex + attempt) % usablePeers.size();
        const PeerEndpoint &peer = usablePeers[peerIndex];

        cout << "Trying chunk " << chunkId << " from " << peer.peerId << " " << peer.ip << ":" << peer.port << endl;

        // got the request from chunk - PASSED
        if (requestChunkFromPeer(config, peer.ip, peer.port, fileId, chunkId)) {
            cout << "Chunk " << chunkId << " downloaded from " << peer.peerId << endl;
            return true;
        }

        // failed - retry on another chunk
        cerr << "Chunk " << chunkId << " failed from " << peer.peerId << ", trying next peer" << endl;
    }

    // all retry attempt failed
    cerr << "Chunk " << chunkId << " failed from all peers" << endl;
    return false;
}

bool removeFileIfExists(const string &path) {
    try {
        if (filesystem::exists(path)) {
            return filesystem::remove(path);
        }
    }
    catch (...) {
        return false;
    }

    return true;
}

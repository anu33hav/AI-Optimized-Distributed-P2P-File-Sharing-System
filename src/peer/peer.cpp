#include "peer/peer.h"
#include "network/socketUtils.h"
#include <unistd.h>
#include <iostream>
#include "protocol/protocol.h"
using namespace std;

bool startPeerServer(const PeerConfig &config) { // server side

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

    // i will write recv, parse, etc later

    // close sockets
    closeSocket(serverToClientSocketFd);
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

bool requestChunkFromPeer(const std::string &ip, int port, const std::string &fileId, int chunkId) { // client side
    int clientSocketFd = connectToServer(ip.c_str(), port);
    if (clientSocketFd == -1) return false;

    string requestMessage = buildRequestMessage(fileId, chunkId);
    cout << "Sending: " << requestMessage;

    // same: later we do send, rec, reply, etc

    closeSocket(clientSocketFd);
    return true;
}

package main

import ("sync" 
		"time"
		)

var (
	peerStore = make(map[string] PeerInfo)
	storeMu sync.RWMutex
)

func upsertPeer(peer PeerInfo) {
	storeMu.Lock()
	defer storeMu.Unlock()
	peerStore[peer.PeerId] = peer // insert new or update
}

func listPeers() [] PeerInfo {
	storeMu.RLock()
	defer storeMu.RUnlock()

	peers := make([]PeerInfo, 0, len(peerStore))
	for _, peer := range peerStore {
		peers = append(peers, peer)
	}

	return peers
}

func peerForFile(fileId string) [] PeerInfo {
	storeMu.RLock()
	defer storeMu.RUnlock()

	// find those peer that have this file
	result := make([]PeerInfo, 0)
	for _, peer := range peerStore {
		// not online
		if !peer.Online {
			continue
		}

		// onilne peer
		for _, f := range peer.Files {
			if (f == fileId) {
				result = append(result, peer)
				break;
			}
		}
	}

	return result
}

func touchPeer(peerId string) bool {
	storeMu.Lock()
	defer storeMu.Unlock()

	peer, ok := peerStore[peerId]
	if !ok { // check if its exists in map or not
		return false;
	}

	// assigning lastseen and online
	peer.LastSeen = time.Now().Unix()
	peer.Online = true
	peerStore[peerId] = peer
	return true
}
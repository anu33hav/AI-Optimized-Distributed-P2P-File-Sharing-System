package main

import (
		"time"
		)

// var (
// 	peerStore = make(map[string] PeerInfo)
// 	storeMu sync.RWMutex
// )

// func upsertPeer(peer PeerInfo) {
// 	storeMu.Lock()
// 	defer storeMu.Unlock()
// 	peerStore[peer.PeerId] = peer // insert new or update
// }

// func listPeers() [] PeerInfo {
// 	storeMu.RLock()
// 	defer storeMu.RUnlock()

// 	peers := make([]PeerInfo, 0, len(peerStore))
// 	for _, peer := range peerStore {
// 		peers = append(peers, peer)
// 	}

// 	return peers
// }

// func peerForFile(fileId string) [] PeerInfo {
// 	storeMu.RLock()
// 	defer storeMu.RUnlock()

// 	// find those peer that have this file
// 	result := make([]PeerInfo, 0)
// 	for _, peer := range peerStore {
// 		// not online
// 		if !peer.Online {
// 			continue
// 		}

// 		// onilne peer
// 		for _, f := range peer.Files {
// 			if (f == fileId) {
// 				result = append(result, peer)
// 				break;
// 			}
// 		}
// 	}

// 	return result
// }

// func touchPeer(peerId string) bool {
// 	storeMu.Lock()
// 	defer storeMu.Unlock()

// 	peer, ok := peerStore[peerId]
// 	if !ok { // check if its exists in map or not
// 		return false;
// 	}

// 	// assigning lastseen and online
// 	peer.LastSeen = time.Now().Unix()
// 	peer.Online = true
// 	peerStore[peerId] = peer
// 	return true
// }

const peerRecordTTL = 20*time.Second;

func peerKey(peerId string) string {
	return "peer:" + peerId
}

func filePeersKey(fileId string) string {
	return "file:" + fileId + ":peers"
}

func isPeerFresh(lastSeen int64) bool {
	if lastSeen == 0 {
		return false;
	}

	return time.Since(time.Unix(lastSeen, 0)) <= peerRecordTTL
}

func removePeerFromFileSets(peerId string, files []string) {
	// from each file that peer hold
	// peer1 -> file1, file2, file3
	for _, fileId := range files {
		// file1 -> peer1, peer2, peer3, remove peer1 from mapping
		_ = redisClient.SRem(redisCtx, filePeersKey(fileId), peerId).Err()
	}
}

func upsertPeer(peer PeerInfo) {

	// update last seen
	peer.LastSeen = time.Now().Unix()
	peer.Online = true

	// if peer already existed, remove old file mapping first, fileA -> peer1, peer2, peer3
	var oldPeer PeerInfo
	if err := redisGetValJSON(peerKey(peer.PeerId), &oldPeer); err == nil {
		removePeerFromFileSets(oldPeer.PeerId, oldPeer.Files)
	}

	// store latest peer info
	if err := redisSetKeyValJSON(peerKey(peer.PeerId), peer, peerRecordTTL); err != nil {
		return
	}

	// add this peer to each file, eg. file1 -> peer2, peer1, file2 -> peer3, peer1
	for _, fileId := range peer.Files {
		_ = redisSetKeyValAdd(filePeersKey(fileId), peer.PeerId)
		_ = redisExpire(filePeersKey(fileId), peerRecordTTL) // update ttl
	}
}

func listPeers() []PeerInfo {

	// get peer
	keys, err := redisScanKeys("peer:*")
	if (err != nil) {
		return []PeerInfo{}
	}

	// create slice string
	peers := make([]PeerInfo, 0, len(keys)) // len = 0, capacity
	for _, key := range keys {

		//get peer info
		var peer PeerInfo
		if err := redisGetValJSON(key, &peer); err != nil {
			continue
		}

		// peer with online and lastSeen <= 20 - store
		if peer.Online && isPeerFresh(peer.LastSeen) {
			peers = append(peers, peer)
		}
	}

	return peers
}

func peerForFile(fileId string) []PeerInfo {
	// get key value
	peerIds, err := redisGetSetValMembers(filePeersKey(fileId))
	if err != nil {
		return []PeerInfo{}
	}

	// create slice string
	peers := make([]PeerInfo, 0, len(peerIds))
	for _, peerId := range peerIds {
		
		// get peerinfo of peerId
		var peer PeerInfo
		if err := redisGetValJSON(peerKey(peerId), &peer); err != nil {
			// if info not present, then just delete it
			_ = redisClient.SRem(redisCtx, filePeersKey(fileId), peerId).Err()
			continue
		}
		
		// online and lastSeen <= 20
		if peer.Online && isPeerFresh(peer.LastSeen) {
			peers = append(peers, peer)
		} else { // otherwise delete val from key
			_ = redisClient.SRem(redisCtx, filePeersKey(fileId), peerId).Err()
		}
	}

	return peers
}

func touchPeer(peerId string) bool {

	// get peerInfo
	var peer PeerInfo
	if err := redisGetValJSON(peerKey(peerId), &peer); err != nil {
		return false
	}

	// mark lastseen and online
	peer.Online = true
	peer.LastSeen = time.Now().Unix()

	// update peer with info Onine and lastSeen
	if err := redisSetKeyValJSON(peerKey(peerId), peer, peerRecordTTL); err != nil {
		return false;
	}

	// store peer in fileId that peer have
	for _, fileId := range peer.Files {
		_ = redisExpire(filePeersKey(fileId), peerRecordTTL)
	}

	return true;
}
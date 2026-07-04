package main

import (
	"encoding/json"
	"net/http"
	"time"
)


// go alreaady created a network connection that have r, w and calls your handler
// reponseWriter is a interface -> Header, write (int, error) and writeHeader (statuscode ints)
func healthHandler(w http.ResponseWriter, r *http.Request) { 
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] string {"status" : "ok",})
}

func registerHandler(w http.ResponseWriter, r *http.Request) {
	// only post method allowed -> POST /register
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	var req RegisterRequest
	// check for valid json
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}

	// create peer
	peer := PeerInfo {
		PeerId:    req.PeerId,
		IP:    req.IP,
		Port:    req.Port,
		Files:    req.Files,
		LastSeen:    time.Now().Unix(),
		Online:    true,
	}

	// store the peer
	upsertPeer(peer)

	w.Header().Set("Content-Type", "application/json") // i am sending json
	_ = json.NewEncoder(w).Encode(peer) // convert struct into json and immediately encode this json unto http response w
	// we can also write 
	// encoder := json.NewEncoder(w) // used to create encoder object
	// encoder.Encode(peer)
}

func peersHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(listPeers())
}

func filePeersHandler(w http.ResponseWriter, r *http.Request) {
	fileId := r.URL.Query().Get("fileId") // eg: /filePeers/fileId=movie.mp4, fileId = movie.mp4
	if fileId == "" {
		http.Error(w, "missing fileId", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(peerForFile(fileId)) // sends peers twith that have fileId
}

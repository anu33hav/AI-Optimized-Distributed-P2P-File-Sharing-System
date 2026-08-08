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


func heartbeatHandler(w http.ResponseWriter, r *http.Request) {
	// check if correct post method
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// read the jbody and convert it into json
	var req HeartbeatRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}
	
	// update the peer lastseen, online
	if !touchPeer(req.PeerId) {
		http.Error(w, "peer not found", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-type", "application/json") // tells the client the response is json
	_ = json.NewEncoder(w).Encode(map[string] string{"status": "ok"}) // convert map into json then send {"status": "ok"}
}

func redisHealthHandler(w http.ResponseWriter, r *http.Request) {
	// check if correct req
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// ping to redis server
	if err := redisClient.Ping(redisCtx).Err(); err != nil {
		http.Error(w, "redis unavailable", http.StatusServiceUnavailable)
		return
	}

	// sending json
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string]string {
		"status" : "ok",
	})
}

func redisSetHandler(w http.ResponseWriter, r *http.Request) {
	// check if req is correct
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// set in redis, req.key -> req.value
	var req RedisKVRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil { // takes json and store into req and decode it into key value
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}

	// check key exists
	if req.Key == "" {
		http.Error(w, "missing key", http.StatusBadRequest)
		return
	}
	// store into redis
	if err := redisSetKeyValString(req.Key, req.Value, 0); err != nil { // 0 -> dont expire key
		http.Error(w, "failed to write redis", http.StatusInternalServerError)
		return
	}

	// write status : ok
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] string{"status": "ok"})
}

func redisGetHandler(w http.ResponseWriter, r *http.Request) {
	// check for correct method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	key := r.URL.Query().Get("key") // /redis/get?key=hello
	// check valid key
	if key == "" {
		http.Error(w, "missing key", http.StatusBadRequest)
		return
	}

	// get val
	value, err := redisGetValString(key)
	// check for err
	if err != nil {
		http.Error(w, "key not found", http.StatusNotFound)
		return
	}

	// set to write
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] string{"key": key, "value": value})
}
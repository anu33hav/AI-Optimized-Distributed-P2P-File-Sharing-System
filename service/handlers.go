package main

import (
	"encoding/json"
	"net/http"
	"strconv"
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

	if err := postgresUpsertPeer(peer); err != nil {
		http.Error(w, "failed to write postgres", http.StatusInternalServerError)
		return
	}

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
	_ = json.NewEncoder(w).Encode(peerForFileBalanced(fileId)) // sends peers twith that have fileId
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

	if err := postgresTouchPeer(req.PeerId); err != nil {
		http.Error(w, "failed to update postgres", http.StatusInternalServerError)
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

func postgresHealthHandler(w http.ResponseWriter, r *http.Request) {
	// check for correct method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// ping postgres
	if err := postgresPing(); err != nil {
		http.Error(w, "postgres unavailable", http.StatusServiceUnavailable)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] string{"status": "ok"})
}

func postgresPeersHandler(w http.ResponseWriter, r *http.Request) {
	// check for method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// get list for peers
	peers, err := postgresListPeers()
	if err != nil {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// send peers
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(peers)
}

func postgresFilePeersHandler(w http.ResponseWriter, r *http.Request) {
	// check method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// filter fileId
	fileId := r.URL.Query().Get("fileId")
	// invalid
	if fileId == "" {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// get peer list with particular fileId
	peers, err := postgresListPeersForFile(fileId)
	if err != nil {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// send peers list
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(peers)
}

func fileMetadataRegisterHandler(w http.ResponseWriter, r *http.Request) {
	// check correct method POST
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// decode req
	var req FileMetadataRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}
	// check valid metadata
	if req.FileId == "" || req.FileName == "" || req.FileHash == "" {
		http.Error(w, "missing file metadata fields", http.StatusBadRequest)
		return
	}

	// assign req into filemetadata
	meta := FileMetadata {
		FileId: req.FileId,
		FileName: req.FileName,
		FileSize: req.FileSize,
		ChunkSize: req.ChunkSize,
		ChunkCount: req.ChunkCount,
		FileHash: req.FileHash,
	}
	// upsertfile into postgres
	if err := postgresUpsertFile(meta); err != nil {
		http.Error(w, "failed to write postgres", http.StatusInternalServerError)
		return
	}

	// send meta
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(meta)
}

func fileMetadataGetHandler(w http.ResponseWriter, r *http.Request) {
	// check method - GET
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// from request get fileId
	fileId := r.URL.Query().Get("fileId")
	if fileId == "" {
		http.Error(w, "missing fileId", http.StatusBadRequest)
		return
	}

	// get file metadata
	meta, err := postgresGetFileMetadata(fileId)
	if err != nil {
		http.Error(w, "file not found", http.StatusNotFound)
		return
	}

	// send
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(meta)
}

func fileMetadataListHandler(w http.ResponseWriter, r *http.Request) {
	// get method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// get list of files
	files, err := postgresListFiles()
	if err != nil {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// send file
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(files);
}

func chunkMappingRegisterHandler(w http.ResponseWriter, r *http.Request) {
	/// check method
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// decode req
	var req ChunkMappingRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}

	// valid req
	if req.FileId == "" || req.PeerId == "" || req.ChunkId < 0 {
		http.Error(w, "missing chunk mapping fields", http.StatusBadRequest)
		return
	}

	// save in chunkmap
	mapping := ChunkMapping {
		FileId: req.FileId,
		ChunkId: req.ChunkId,
		PeerId: req.PeerId,
		HasChunk: true,
	}
	// store
	if err := postgresUpsertChunkMapping(mapping); err != nil {
		http.Error(w, "failed to write postgres", http.StatusInternalServerError)
		return
	}

	// send mapping
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(mapping)
}

func chunkMappingsForFileHandler(w http.ResponseWriter, r *http.Request) {
	// check get method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// filter fileId
	fileId := r.URL.Query().Get("fileId")
	if fileId == "" {
		http.Error(w, "missing fileId", http.StatusBadRequest)
		return
	}

	// get chunk mapping list for file
	mappings, err := postgresListChunkMappingsForFile(fileId)
	if err != nil {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// send mapping
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(mappings)
}

func chunkOwnersHandler(w http.ResponseWriter, r *http.Request) {
	// check get method
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// filter fileId
	fileId := r.URL.Query().Get("fileId")
	if fileId == "" {
		http.Error(w, "missing fileId", http.StatusBadRequest)
		return
	}

	// filter chunkId
	chunkIdStr := r.URL.Query().Get("chunkId")
	if fileId == "" {
		http.Error(w, "missing fileId", http.StatusBadRequest)
		return
	}

	// convert string to int
	chunkId, err := strconv.Atoi(chunkIdStr)
	if err != nil {
		http.Error(w, "invalid chunkid", http.StatusBadRequest)
		return
	}

	// get peers list for chunk
	peers, err := postgresListPeersForChunk(fileId, chunkId)
	if err != nil {
		http.Error(w, "failed to read postgres", http.StatusInternalServerError)
		return
	}

	// send list
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(peers)

}

func peerLoadStatusHandler(w http.ResponseWriter, r *http.Request) {
	// method check
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// filter peerId
	peerId := r.URL.Query().Get("peerId")
	if peerId == "" {
		http.Error(w, "missing peerId", http.StatusBadRequest)
		return
	}

	// send peer load
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] any {
		"peerId": peerId,
		"activeRequests": getPeerLoad(peerId),
	})
}

func peerLoadReserveHandler(w http.ResponseWriter, r *http.Request) {
	// POST method
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// decode req
	var req LoadRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}
	// invalid peerId
	if req.PeerId == "" {
		http.Error(w, "missing peerId", http.StatusBadRequest)
		return
	}

	// inc the load
	load, err := reservePeerLoad(req.PeerId)
	if err != nil {
		http.Error(w, "failed to reserve load", http.StatusInternalServerError)
		return
	}

	// send load
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] any {
		"peerId": req.PeerId,
		"activeRequests": load,
	})
}

func peerLoadReleaseHandler(w http.ResponseWriter, r *http.Request) {
	// check post method
	if r.Method != http.MethodPost {
		w.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	// decode req
	var req LoadRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}
	// invalid peerId
	if req.PeerId == "" {
		http.Error(w, "missing peerId", http.StatusBadRequest)
		return
	}

	// dec load
	load, err := releasePeerLoad(req.PeerId)
	if err != nil {
		http.Error(w, "failed to release load", http.StatusInternalServerError)
		return
	}

	// send load
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string] any {
		"peerId": req.PeerId,
		"activeRequests": load,
	})
}
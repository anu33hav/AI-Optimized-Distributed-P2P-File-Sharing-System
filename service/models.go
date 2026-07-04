package main

type PeerInfo struct {
	PeerId    string    `json:"peerId"`
	IP    string    `json:"ip"`
	Port    int    `json:"port"`
	Files    []string    `json:"files"`
	LastSeen    int64    `json:"lastSeen"`
	Online    bool    `json:"online"`	
}

type RegisterRequest struct {
	PeerId    string    `json:"peerId"`
	IP    string    `json:"ip"`
	Port    int    `json:"port"`
	Files    []string    `json:"files"`
}
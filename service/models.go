package main

import "time"

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

type HeartbeatRequest struct {
	PeerId string `json:"peerId"`
}

type RedisKVRequest struct {
	Key    string    `json:"key"`
	Value    string    `json:"value"`
}

type FileMetadata struct {
	FileId		string		`json:"fileID"`
	FileName		string `json:"fileName"`
	FileSize		int		`json:"fileSize"`
	ChunkSize		int		`json:"chunkSize"`
	ChunkCount		int		`json:"chunkCount"`
	FileHash		string		`json:"fileHash"`
	CreatedAt		time.Time	`json:"createdAt,omitempty"`
	UpdatedAt		time.Time	`json:"updatedAt,omitempty"`
}

type FileMetadataRequest struct {
	FileId		string		`json:"fileId"`
	FileName		string		`json:"fileName"`
	FileSize		int		`json:"fileSize"`
	ChunkSize		int		`json:"chunkSize"`
	ChunkCount		int		`json:"chunkCount"`
	FileHash		string		`json:"fileHash"`
}

type ChunkMapping struct {
	FileId		string		`json:"fileId"`
	ChunkId		int		`json:"chunkId"`
	PeerId		string		`json:peerId`
	HasChunk		bool		`json:"hasChunk"`
	UpdatedAt		time.Time		`json:"UpdateAt,omitempty"`
}

type ChunkMappingRequest struct {
	FileId		string		`json:"fileId"`
	ChunkId		int		`json:"ChunkId"`
	PeerId		string		`json:"peerId"`
}
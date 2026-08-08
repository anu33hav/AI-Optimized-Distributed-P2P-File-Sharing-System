package main

import (
	"net/http"
	"log"
)

func main() {
	
	if err := initRedis(); err != nil {
		log.Fatalf("failed to connect to redis: %v", err)
	}

	http.HandleFunc("/health", healthHandler) // if someone req /health thn call healthHandler
	http.HandleFunc("/register", registerHandler)
	http.HandleFunc("/heartbeat", heartbeatHandler)
	http.HandleFunc("/peers", peersHandler)
	http.HandleFunc("/files", filePeersHandler)

	http.HandleFunc("/redis/health", redisHealthHandler)
	http.HandleFunc("/redis/set", redisSetHandler)
	http.HandleFunc("/redis/get", redisGetHandler)
	

	log.Println("Go servcie listening on :8080") // testing purpose
	log.Fatal(http.ListenAndServe(":8080", nil))
}
package main

import (
	"net/http"
	"log"
)

func main() {
	
	if err := initRedis(); err != nil {
		log.Fatalf("failed to connect to redis: %v", err)
	}
	if err := initPostgres(); err != nil {
		log.Fatalf("failed to connect to postgres: %v", err)
	}

	http.HandleFunc("/health", healthHandler) // if someone req /health thn call healthHandler
	http.HandleFunc("/register", registerHandler)
	http.HandleFunc("/heartbeat", heartbeatHandler)
	http.HandleFunc("/peers", peersHandler)
	http.HandleFunc("/files", filePeersHandler)

	http.HandleFunc("/redis/health", redisHealthHandler)
	http.HandleFunc("/redis/set", redisSetHandler)
	http.HandleFunc("/redis/get", redisGetHandler)
	
	http.HandleFunc("/postgres/health", postgresHealthHandler)
	http.HandleFunc("/postgres/peers", postgresPeersHandler)
	http.HandleFunc("/postgres/files", postgresFilePeersHandler)

	http.HandleFunc("/postgres/file/register", fileMetadataRegisterHandler)
	http.HandleFunc("/postgres/file/get", fileMetadataGetHandler)
	http.HandleFunc("/postgres/file/list", fileMetadataListHandler)

	log.Println("Go servcie listening on :8080") // testing purpose
	log.Fatal(http.ListenAndServe(":8080", nil))
}
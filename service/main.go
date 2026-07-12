package main

import (
	"net/http"
	"log"
)

func main() {
	http.HandleFunc("/health", healthHandler) // if someone req /health thn call healthHandler
	http.HandleFunc("/register", registerHandler)
	http.HandleFunc("/heartbeat", heartbeatHandler)
	http.HandleFunc("/peers", peersHandler)
	http.HandleFunc("/files", filePeersHandler)

	log.Println("Go servcie listening on : 8080") // testing purpose
	log.Fatal(http.ListenAndServe(":8080", nil))
}
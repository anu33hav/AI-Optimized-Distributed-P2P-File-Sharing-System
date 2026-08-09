package main

import (
	"database/sql"
	"os"
	"time"
	"encoding/json"
	"fmt"

	_ "github.com/jackc/pgx/v5/stdlib"
)

var postgresDB *sql.DB // hold db connection pool

func initPostgres() error {
	// get postgresql info
	dsn := os.Getenv("POSTGRES_DSN") // address/login info
	if dsn == "" {
		dsn = "postgres://tracker:tracker@127.0.0.1:5432/tracker?sslmode=disable"
		// db type: postgresql, username: tracker, password: tracker, Host: 127.0.0.1, port: 5432, db: tracker, ssl: disabled
	}

	// open db
	db, err := sql.Open("pgx", dsn)
	if err != nil {
		return err
	}

	// set configuration
	db.SetMaxOpenConns(5) // max simulateous req
	db.SetMaxIdleConns(5) // reuse it without closing connection
	db.SetConnMaxLifetime(5*time.Minute) // max life of connection

	// ping the postgresql
	if err := db.Ping(); err != nil {
		_ = db.Close()
		return err
	}
	// point out pointer to db
	postgresDB = db
	return ensurePostgresSchema() // db has req table
}

func ensurePostgresSchema() error {

	const createTable = `CREATE TABLE IF NOT EXISTS tracker_peers (
		peerId TEXT PRIMARY KEY,
		ip TEXT NOT NULL,
		port INT NOT NULL,
		files JSONB NOT NULL,
		lastSeen BIGINT NOT NULL,
		online BOOLEAN NOT NULL,
		updatedAt TIMESTAMPTZ NOT NULL DEFAULT NOW()
	);`

	_, err := postgresDB.Exec(createTable)
	return err
}

func postgresPing() error {
	return postgresDB.Ping()
}

func postgresUpsertPeer(peer PeerInfo) error {
	// convert into peer files into json
	filesJSON, err := json.Marshal(peer.Files)
	if err != nil {
		return err
	}

	// query to insert/update the peerinfo
	const query = `INSERT INTO tracker_peers (peerId, ip, port, files, lastSeen, online, updatedAt)
	VALUES ($1, $2, $3, $4::jsonb, $5, $6, NOW()) -- prevent sql injection
	ON CONFLICT (peerId)
	DO UPDATE SET
		ip = EXCLUDED.ip,
		port = EXCLUDED.port,
		files = EXCLUDED.files,
		lastSeen = EXCLUDED.lastSeen,
		online = EXCLUDED.online,
		updatedAt = NOW();
	`
	// exec the query
	_, err = postgresDB.Exec (
		query,
		peer.PeerId,
		peer.IP,
		peer.Port,
		string(filesJSON),
		peer.LastSeen,
		peer.Online,
	)

	return err
}

func postgresTouchPeer(peerId string) error {
	// query to update lastSeen
	const query = `UPDATE tracker_peers SET lastSeen = $2, online = TRUE, updatedAt = NOW() WHERE peerId = $1;`

	// update in postgresql
	result, err := postgresDB.Exec(query, peerId, time.Now().Unix())
	if err != nil {
		return err
	}

	// how many rows affected by updating only lastseen of a peer
	rows, err := result.RowsAffected()
	if err != nil {
		return err
	}
	// no peer updated means not found
	if rows == 0 {
		return fmt.Errorf("peer not found")
	}

	return nil
}

func postgresListPeers() ([]PeerInfo, error) {

	// get query
	const query = `SELECT peerId, ip, port, files, lastSeen, online FROM tracker_peers ORDER BY updatedAt DESC;`

	// represent the select
	rows, err := postgresDB.Query(query) // rows -> pointer
	if err != nil {
		return nil, err
	}
	// close the rows
	defer rows.Close()

	// create slice of PeerInfo
	peers := make([]PeerInfo, 0)
	for rows.Next() {
		var (
			peer PeerInfo
			filesJSON []byte
		)

		// fetch from db
		if err := rows.Scan(&peer.PeerId, &peer.IP, &peer.Port, &filesJSON, &peer.LastSeen, &peer.Online); err != nil {
			return nil, err
		}

		// convert json to go
		if len(filesJSON) > 0 {
			if err := json.Unmarshal(filesJSON, &peer.Files); err != nil {
				return nil, err
			}
		}

		// save the peer
		peers = append(peers, peer)
	}

	return peers, rows.Err()
}

func postgresListPeersForFile(fileId string) ([]PeerInfo, error) {

	// get all peer that have file
	const query = `SELECT peerId, ip, port, files, lastSeen, online FROM tracker_peers WHERE online = TRUE AND lastSeen > $2 AND files ? $1 ORDER BY updatedAt DESC;`

	// exec with parameter
	rows, err := postgresDB.Query(query, fileId, time.Now().Add(-peerRecordTTL).Unix())
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	// peers with particular file
	peers := make([]PeerInfo, 0)
	for rows.Next() {
		// create var to store
		var (
			peer PeerInfo
			filesJSON []byte
		)

		// copy paste from db to our var
		if err := rows.Scan(&peer.PeerId, &peer.IP, &peer.Port, &filesJSON, &peer.LastSeen, &peer.Online); err != nil {
			return nil, err
		}

		// store the peer
		if len(filesJSON) > 0 {
			if err := json.Unmarshal(filesJSON, &peer.Files); err != nil {
				return nil, err
			}
		}

		// store peer
		peers = append(peers, peer)
	}

	return peers, rows.Err()

}
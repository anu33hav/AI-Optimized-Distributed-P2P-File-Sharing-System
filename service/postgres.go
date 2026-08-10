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

	const createFilesTable = `CREATE TABLE IF NOT EXISTS tracker_files (
		fileId TEXT PRIMARY KEY,
		fileName TEXT NOT NULL,
		fileSize BIGINT NOT NULL,
		chunkSize BIGINT NOT NULL,
		chunkCount INT NOT NULL,
		fileHash TEXT NOT NULL,
		createdAt TIMESTAMPTZ NOT NULL DEFAULT NOW(),
		updatedAt TIMESTAMPTZ NOT NULL DEFAULT NOW()
	);`

	if _, err := postgresDB.Exec(createTable); err != nil {
		return err
	}

	_, err := postgresDB.Exec(createFilesTable)
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

func postgresUpsertFile(meta FileMetadata) error {
	// inesrt query
	const query = `INSERT INTO tracker_files (fileId, fileName, fileSize, chunkSize, chunkCount, fileHash, createdAt, updatedAt)
	VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW())
	ON CONFLICT (fileId)
	DO UPDATE SET
		fileName = EXCLUDED.fileName,
		fileSize = EXCLUDED.fileSize,
		chunkSize = EXCLUDED.chunkSize,
		chunkCount = EXCLUDED.chunkCount,
		fileHash = EXCLUDED.fileHash,
		updatedAT = NOW();`
	// exec the query
	_, err := postgresDB.Exec(
		query,
		meta.FileId,
		meta.FileName,
		meta.FileSize,
		meta.ChunkSize,
		meta.ChunkCount,
		meta.FileHash,
	)
	return err
}

func postgresGetFileMetadata(fileId string) (*FileMetadata, error) {
	// make query for select filter fileId
	const query = `SELECT fileId, fileName, fileSize, chunkSize, chunkCount, fileHash, createdAt, updatedAt FROM tracker_files WHERE fileId = $1;`

	// fetch whol row and past into meta
	var meta FileMetadata
	err := postgresDB.QueryRow(query, fileId).Scan(
		&meta.FileId,
		&meta.FileName,
		&meta.FileSize,
		&meta.ChunkSize,
		&meta.ChunkCount,
		&meta.FileHash,
		&meta.CreatedAt,
		&meta.UpdatedAt,
	)
	if err != nil {
		return nil, err
	}
	return &meta, nil
}

func postgresListFiles() ([]FileMetadata, error) {
	// make query for get files
	const query = `SELECT fileId, fileName, fileSize, chunkSize, chunkCount, fileHash, createdAt, updatedAt FROM tracker_files ORDER BY updatedAt DESC;`

	// exec query
	rows, err := postgresDB.Query(query)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	
	files := make([]FileMetadata, 0)
	for rows.Next() {
		//fetch from db and paste
		var meta FileMetadata
		if err := rows.Scan(
			&meta.FileId,
			&meta.FileName,
			&meta.FileSize,
			&meta.ChunkSize,
			&meta.ChunkCount,
			&meta.FileHash,
			&meta.CreatedAt,
			&meta.UpdatedAt,
		); err != nil {
			return nil, err
		}
		// store in files
		files = append(files, meta)
	}

	return files, rows.Err()
}
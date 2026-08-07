#pragma once
#include <cstddef>
#include <string>
#include <vector>

struct ChunkMetadata {
    int chunkIndex;
    std::string chunkPath;
    long long chunkSize;
    std::string chunkHash;

    ChunkMetadata() = default;
    ChunkMetadata(int chunkIndex, const std::string& chunkPath, long long chunkSize, const std::string &chunkHash)
    : chunkIndex(chunkIndex), chunkPath(chunkPath), chunkSize(chunkSize), chunkHash(chunkHash) {};
};

struct FileMetadata {
    std::string fileId;
    std::string fileName;
    long long fileSize;
    std::size_t chunkSize;
    int totalChunks;
    std::string fileHash;
    std::vector<ChunkMetadata> chunks;

    FileMetadata() = default;
    FileMetadata(const std::string &fileId, const std::string &fileName, long long fileSize, std::size_t chunkSize, int totalChunks)
    : fileId(fileId), fileName(fileName), fileSize(fileSize), chunkSize(chunkSize), totalChunks(totalChunks) {};
};

bool addFileMetadata(const FileMetadata &metadata);
const FileMetadata* getFileMetadata(const std::string& fileId);
bool hasFileMetadata(const std::string& fileId);
void printFileMetadata(const std::string& fileId);
bool buildAndStoreMetadata(const char* inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize, int chunkCount);
bool verifyMergedFileHash(const std::string &fileId, const std::string &reconstructedPath);
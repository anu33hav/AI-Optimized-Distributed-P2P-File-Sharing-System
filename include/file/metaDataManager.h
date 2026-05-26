#pragma once
#include <string>
#include <vector>

struct ChunkMetaData {
    int chunkIndex;
    string chunkPath;
    long long chunkSize;
};

struct FileMetaData {
    string fileId;
    string fileName;
    long long fileSize;
    size_t chunkSize;
    int totalChunks;
    vector<ChunkMetaData> chunks;
};

bool addFileMetaData (const FileMetaData &metaData);
const FileMetaData* getFileMetaData(const string &fileId);
bool hasFileMetaData (const string &fileId);
void printFileMetaData (const string &fileId);
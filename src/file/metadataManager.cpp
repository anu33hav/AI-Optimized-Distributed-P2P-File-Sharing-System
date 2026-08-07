#include <unordered_map>
#include "file/metadataManager.h"
#include <iostream>
#include <filesystem>
#include "file/chunkManager.h"
#include "file/hashUtils.h"
using namespace std;

static unordered_map<string, FileMetadata> metadataStore;

bool addFileMetadata(const FileMetadata &metadata) {

    // check for valid metadata
    if (metadata.fileId.empty()) return false;

    // store valid
    metadataStore[metadata.fileId] = metadata;
    return true;
}

const FileMetadata* getFileMetadata(const string& fileId) {

    auto found = metadataStore.find(fileId); // check if its exists in metadataStore
    
    // not found
    if (found == metadataStore.end()) return nullptr;
    // found
    return &found->second;
}

bool hasFileMetadata(const string &fileId) {
    return metadataStore.find(fileId) != metadataStore.end();
}

void printFileMetadata(const string &fileId) {

    // get metadata
    const FileMetadata* metadata = getFileMetadata(fileId);

    // check if it exists
    if (!metadata) {
        cerr << "fileId not exists" << endl;
        return;
    }

    // print valid fileId
    cout << "FileId: " << metadata->fileId << endl;
    cout << "File Name: " << metadata->fileName << endl;
    cout << "File Size: " << metadata->fileSize << endl;
    cout << "Chunk Size: " << metadata->chunkSize << endl;
    cout << "Total Chunks: " << metadata->totalChunks << endl;

    // print chunks

    for (const auto &chunk : metadata->chunks) {
        cout << "Chunk " << chunk.chunkIndex
        << " | Path: " << chunk.chunkPath
        << " | Size: " << chunk.chunkSize
        << " | Hash: " << chunk.chunkHash << endl;
    }
}

bool buildAndStoreMetadata(const char* inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize, int chunkCount) {

    try {
        // create a metadata
        FileMetadata metadata(fileId, filesystem::path(inputPath).filename().string(), filesystem::file_size(inputPath), chunkSize, chunkCount);
        // for all chunks
        for (int chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
            string chunkPath = getChunkPath(baseChunkDir, fileId, chunkIndex);

            string chunkHash;
            if (!computeFileSha256(chunkPath, chunkHash)) {
                cerr << "failed to hash chunk: " << endl;
                return false;
            }
            // create chunk metadata
            ChunkMetadata chunk(chunkIndex, chunkPath, filesystem::file_size(chunkPath), chunkHash);
            metadata.chunks.push_back(chunk);
        }
        // add info into metadataStore map
        return addFileMetadata(metadata);
    }
    catch(...) {
        return false;
    }
}
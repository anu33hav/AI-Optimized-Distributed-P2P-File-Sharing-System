#include <file/metaDataManager.h>
#include <iostream>
#include <unordered_map>

using namespace std;

static unordered_map<string, FileMetaData> metaDataStore;


bool addFileMetaData(const FileMetaData &metaData) {
    if (metaData.fileId.empty()) {
        return false;
    }
    
    metaDataStore[metaData.fileId] = metaData;
    return true;
}

const FileMetaData* getFileMetaData(const string &fileId) {
    auto it = metaDataStore.find(fileId);
    if (it == metaDataStore.end()) {
        return nullptr;
    }

    return &it->second;
}

bool hasFileMetaData (const string &fileId) {
    return metaDataStore.find(fileId) != metaDataStore.end();
}

void printFileMetaData(const string &fileId) {
    const FileMetaData* metaData = getFileMetaData(fileId);
    if (!metaData) {
        cout << "MetaData not found for file: " << fileId << endl;
        return;
    }

    cout << "File ID: " << metaData->fileId << endl;
    cout << "File Name: " << metaData->fileName << endl;
    cout << "File Size: " << metaData->fileSize << endl;
    cout << "Chunk Size: " << metaData->chunkSize << endl;
    cout << "Total Chunks: " << metaData->totalChunks << endl;

    for (const auto& chunk : metaData->chunks) {
        cout << "Chunk " << chunk.chunkIndex
                  << " | Path: " << chunk.chunkPath
                  << " | Size: " << chunk.chunkSize
                  << "\n";
    }
}
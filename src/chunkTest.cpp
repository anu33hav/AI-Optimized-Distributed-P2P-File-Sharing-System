#include <file/chunkManager.h>
#include <file/metaDataManager.h>
#include <filesystem>
#include <iostream>
using namespace std;

int main() {
    const char* inputPath = "data/input.txt";
    const char* baseChunkDir = "data/chunks";
    const char* fileId = "file1";
    size_t chunkSize = 512*1024;
    const char* outputPath = "data/reconstructed.txt";
    long long fileSize = filesystem::file_size(inputPath);

    int chunkCount = splitFileIntoChunks(inputPath, baseChunkDir, fileId, chunkSize);
    if (chunkCount == -1) {
        cerr << "Chunking failed" << endl;
        return -1;
    }
    cout << "Chunks created: " << chunkCount << endl;

    FileMetaData metaData;
    metaData.fileId = fileId;
    metaData.fileName = "input.txt";
    metaData.fileSize = fileSize;
    metaData.chunkSize = chunkSize;
    metaData.totalChunks = chunkCount;

    for (int i = 0; i < chunkCount; i++) {
        string chunkPath = getChunkPath(baseChunkDir, fileId, i);
        
        ChunkMetaData chunk;
        chunk.chunkIndex = i;
        chunk.chunkPath = chunkPath;
        chunk.chunkSize = filesystem::file_size(chunkPath);
        metaData.chunks.push_back(chunk);
    }

    if (!addFileMetaData(metaData)) {
        cerr << "Failed to add metaData" << endl;
        return -1;
    }

    printFileMetaData(fileId);

    const FileMetaData* storedMetaData = getFileMetaData(fileId);

    if (!storedMetaData) {
        cerr << "metaData not found" << endl;
        return -1;
    }


    if (!mergeChunks(baseChunkDir, storedMetaData->fileId.c_str(), storedMetaData->totalChunks, outputPath)) {
        cerr << "Reconstruction failed" << endl;
        return -1;
    }
    cout << "Reconstruction successful" << endl;
    cout << "Output file: " << outputPath << endl; 

    // test not found case
    if (!getFileMetaData("missing_file")) {
        cout << "Missing metaData" << endl;
    }
    return 0;

}
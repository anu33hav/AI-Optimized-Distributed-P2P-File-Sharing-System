#include <file/chunkManager.h>
#include <iostream>
using namespace std;

int main() {
    const char* inputPath = "data/input.txt";
    const char* baseChunkDir = "data/chunks";
    const char* fileId = "file1";
    size_t chunkSize = 512*1024;

    int chunkCount = splitFileIntoChunks(inputPath, baseChunkDir, fileId, chunkSize);
    if (chunkCount == -1) {
        cerr << "Chunking failed" << endl;
        return -1;
    }
    cout << "Chunks created: " << chunkCount << endl;

    cout << "Chunk 0 Path: " << getChunkPath(baseChunkDir, fileId, 0) << endl;
    cout << "Chunk 0 exists: " << chunkExists(baseChunkDir, fileId, 0) << endl;
    cout << "Chunk 99 exists: " << chunkExists(baseChunkDir, fileId, 99) << endl;
    cout << "Chunk -1 exists: " << chunkExists(baseChunkDir, fileId, -1) << endl;

    return 0;

}
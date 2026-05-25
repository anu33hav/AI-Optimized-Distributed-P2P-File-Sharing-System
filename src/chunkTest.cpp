#include <file/chunkManager.h>
#include <iostream>
using namespace std;

int main() {
    const char* inputPath = "data/input.txt";
    const char* baseChunkDir = "data/chunks";
    const char* fileId = "file1";
    size_t chunkSize = 512*1024;
    const char* outputPath = "data/reconstructed.txt";

    int chunkCount = splitFileIntoChunks(inputPath, baseChunkDir, fileId, chunkSize);
    if (chunkCount == -1) {
        cerr << "Chunking failed" << endl;
        return -1;
    }
    cout << "Chunks created: " << chunkCount << endl;

    if (!mergeChunks(baseChunkDir, fileId, chunkCount, outputPath)) {
        cerr << "Reconstruction failed" << endl;
        return -1;
    }
    cout << "Reconstruction successful" << endl;
    cout << "Output file: " << outputPath << endl; 

    return 0;

}
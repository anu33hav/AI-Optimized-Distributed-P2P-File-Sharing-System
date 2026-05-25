#include <file/chunkManager.h>
#include <iostream>
using namespace std;

int main() {
    const char* inputPath = "data/input.txt";
    const char* chunkDir = "data/chunks";
    size_t chunkSize = 512*1024;

    int chunkCount = splitFileIntoChunks(inputPath, chunkDir, chunkSize);
    if (chunkCount == -1) {
        cerr << "Chunking failed" << endl;
        return -1;
    }
    cout << "Chunks created: " << chunkCount << endl;

    return 0;

}
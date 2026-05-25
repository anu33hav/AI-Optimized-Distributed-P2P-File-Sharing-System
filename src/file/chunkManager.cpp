#include <file/chunkManager.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
using namespace std;

int splitFileIntoChunks(const char* inputPath, const char* chunkDir, size_t chunkSize) {

    // 01: open file
    ifstream inputFile(inputPath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open input file for chunking" << endl;
        return -1;
    }

    // 02: create dir for chunks, if it already exists nothing happens
    filesystem::create_directories(chunkDir);
    
    // 03: make it input chunks
    vector<char> buffer(chunkSize);
    int chunkIndex = 0;
    while (true) {
        // read the file
        inputFile.read(buffer.data(), chunkSize);
        streamsize bytesRead = inputFile.gcount(); // bcz last chunk may be smaller
        if (bytesRead <= 0) break; // eof check

        // create chunnk file name
        string chunkPath = string(chunkDir) + "/chunk_" + to_string(chunkIndex); // chunks/chunk_0
        
        // open chunk file
        ofstream chunkFile(chunkPath, ios::binary);
        if (!chunkFile) {
            cerr << "Failed to create chunk file: " << chunkPath << endl;
            return -1;
        }

        // write in chunkFile
        chunkFile.write(buffer.data(), bytesRead);
        if (!chunkFile) {
            cerr << "Failed to write chunk file: " << chunkPath << endl;
            return -1;
        }

        chunkIndex++;
    }

    return chunkIndex;
}
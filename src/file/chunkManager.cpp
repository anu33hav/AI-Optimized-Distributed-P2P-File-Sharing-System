#include <file/chunkManager.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
using namespace std;

int splitFileIntoChunks(const char* inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize) {

    // 01: open file
    ifstream inputFile(inputPath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open input file for chunking" << endl;
        return -1;
    }

    // 02: create dir for chunks, if it already exists nothing happens
    // filesystem::create_directories(chunkDir);
    string fileChunkDir = string(baseChunkDir) + "/" + fileId;
    filesystem::create_directories(fileChunkDir);
    
    // 03: make it input chunks
    vector<char> buffer(chunkSize);
    int chunkIndex = 0;
    while (true) {
        // read the file
        inputFile.read(buffer.data(), chunkSize);
        streamsize bytesRead = inputFile.gcount(); // bcz last chunk may be smaller
        if (bytesRead <= 0) break; // eof check

        // create chunnk file name
        // string chunkPath = string(chunkDir) + "/chunk_" + to_string(chunkIndex); // chunks/chunk_0
        string chunkPath = getChunkPath(baseChunkDir, fileId, chunkIndex);

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

string getChunkPath (const char* baseChunkDir, const char* fileId, int chunkIndex) {
    return string(baseChunkDir) + "/" + fileId + "/chunk_" + to_string(chunkIndex);
}

bool chunkExists (const char* baseChunkDir, const char* fileId, int chunkIndex) {
    if (chunkIndex < 0) return false;
    return filesystem::exists(getChunkPath(baseChunkDir, fileId, chunkIndex));
}

bool mergeChunks (const char* baseChunkDir, const char* fileId, int totalChunks, const char* outputPath) {

    if (totalChunks < 0) return false;

    // open file to write
    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) {
        cerr << "Failed to open output file for reconstruction" << endl;
        return false;
    }

    const size_t BUFFER_SIZE = 4096;
    vector<char> buffer(BUFFER_SIZE);

    // iterate on each chunk
    for (int chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {

        // check if chunk exists
        if (!chunkExists(baseChunkDir, fileId, chunkIndex)) {
            cerr << "Missing chunk: " << chunkIndex << endl;
            return false;
        }

        // open file
        string chunkPath = getChunkPath(baseChunkDir, fileId, chunkIndex);
        ifstream chunkFile(chunkPath, ios::binary);
        if (!chunkFile) {
            cerr << "Failed to open chunk: " << chunkIndex << endl;
            return false;
        }

        // write into outputfile
        while (chunkFile) {
            // read from file and copying it into buffer
            chunkFile.read(buffer.data(), BUFFER_SIZE);
            streamsize bytesRead = chunkFile.gcount();

            if (bytesRead > 0) {
                outputFile.write(buffer.data(), bytesRead);
                if (!outputFile) {
                    cerr << "Failed while writing output file" << endl;
                    return false;
                }
            }
        }
    }

    // successfully merge
    return true;
}
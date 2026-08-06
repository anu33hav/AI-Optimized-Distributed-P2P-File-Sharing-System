#include "file/chunkManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include "file/hashUtils.h"
using namespace std;

int splitFileIntoChunks(const char* inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize) {

    // open file
    ifstream inputFile(inputPath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open inputFile" << endl;
        return -1;
    }

    // create chunk dir
    filesystem::create_directories(string(baseChunkDir) + "/" + fileId);

    // make chunks
    vector<char> readBuffer(chunkSize);
    int chunkIndex = 0;
    while (inputFile) {
        
        // read file
        inputFile.read(readBuffer.data(), chunkSize);
        streamsize byteRead = inputFile.gcount();
        if (byteRead <= 0) {
            break;
        }

        // open output file
        string chunkPath = getChunkPath(baseChunkDir, fileId, chunkIndex);
        ofstream chunkFile(chunkPath, ios::binary);
        if (!chunkFile) {
            cerr << "Failed to open chunkFile: " << chunkPath << endl;
            break;
        }

        // write in chunk
        chunkFile.write(readBuffer.data(), byteRead);
        if (!chunkFile) {
            cerr << "Failed to write chunkFile: " << chunkPath << endl;
            chunkFile.close();
            break;
        }

        // hash chunkFile
        string chunkHash;
        if (!computeFileSha256(chunkPath, chunkHash)) {
            cerr << "Failed to hash chunk file: " << chunkPath << endl;
            // close the chunkFile
            chunkFile.close();
            break;
        }

        chunkIndex++;
    }

    inputFile.close();
    return chunkIndex;
}

string getChunkPath(const char* baseChunkDir, const char* fileId, int chunkIndex) {
    return string(baseChunkDir) + "/" + string(fileId) + "/chunk_" + to_string(chunkIndex);
}

bool chunkExists(const char* baseChunkDir, const char* fileId, int chunkIndex) {
    if (chunkIndex < 0) return false;
    return filesystem::exists(getChunkPath(baseChunkDir, fileId, chunkIndex));
}

bool mergeChunks(const char* baseChunkDir, const char* fileId, int totalChunks, const char* outputPath) {
    
    if (totalChunks < 0) return false;

    // open output file
    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) {
        cerr << "Failed to open output file" << endl;
        return false;
    }

    const size_t BUFFER_SIZE = 4096;
    vector<char> readBuffer(BUFFER_SIZE);
    // traverse on each chunk
    for (int chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {
        
        // chunk exists or not
        if (!chunkExists(baseChunkDir, fileId, chunkIndex)) {
            cerr << "chunk doesnt exists" << endl;
            return false;
        }
        
        // open chunk file
        string chunkPath = getChunkPath(baseChunkDir, fileId, chunkIndex);
        ifstream chunkFile(chunkPath, ios::binary);
        if (!chunkFile) {
            cerr << "Failed to open chunkFile" << endl;
            return false;
        }

        // extract till eof
        while (chunkFile) {
            // read file
            chunkFile.read(readBuffer.data(), BUFFER_SIZE);
            streamsize byteRead = chunkFile.gcount();

            // write in output file
            if (byteRead > 0) {
                outputFile.write(readBuffer.data(), byteRead);
                if (!outputFile) {
                    cerr << "Write Failed" << endl;
                    return false;
                }
            }
        }
    }

    // all done
    return true;
}
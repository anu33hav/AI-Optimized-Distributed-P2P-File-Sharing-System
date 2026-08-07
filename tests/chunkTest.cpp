#include "file/chunkManager.h"
#include <iostream>
#include "file/metadataManager.h"
#include <filesystem>
#include "protocol/protocol.h"
#include "file/hashUtils.h"
using namespace std;

bool validateChunkMetadata(const FileMetadata &metadata, const char* baseChunkDir);
bool runPipelineTest(const char *inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize, const char *outputPath);

int main() {
    cout << "running pipeline" << endl;
    const char* inputPath = "data/input.txt";
    const char* baseChunkDir = "data/chunks";
    const char* fileId = "inputA";
    const char* outputPath = "data/reconstruction.txt";
    const size_t chunkSize = 512*1024;

    if (runPipelineTest(inputPath, baseChunkDir, fileId, chunkSize, outputPath)) {
        cout << "Pipeline test passed" << endl;
    }
    else {
        cerr << "Pipeline test failed" << endl;
        return -1;
    }

    cout << buildConnectMessage("peer1");
    cout << buildRequestMessage("file1", 2);
    cout << buildChunkMessage("file1", 2, 1024, "sample_hash");
    cout << buildCompleteMessage();
    cout << buildErrorMessage("outOfBound");

    return 0;
}

bool validateChunkMetadata(const FileMetadata &metadata, const char* baseChunkDir) {

    // check chunks 
    if (metadata.totalChunks != (int)metadata.chunks.size()) return false;

    // check for each chunks
    for (const auto &chunk : metadata.chunks) {

        // check if it exists via file
        if (!chunkExists(baseChunkDir, metadata.fileId.c_str(), chunk.chunkIndex)) return false;
        // check if it exists via chunk
        if (!filesystem::exists(chunk.chunkPath)) return false;

        // check file path
        string expectedPath = getChunkPath(baseChunkDir, metadata.fileId.c_str(), chunk.chunkIndex);
        if (expectedPath != chunk.chunkPath) return false;

        // check for chunk size
        long long actualSize = filesystem::file_size(chunk.chunkPath);
        if (actualSize != chunk.chunkSize) return false;
    }

    // all good
    return true;
}

bool validateChunkIntegrity(const FileMetadata &metadata) {
    for (const auto &chunk : metadata.chunks) {
        string actualHash;
        if (!computeFileSha256(chunk.chunkPath, actualHash)) {
            cerr << "failed to hash chunk: " << chunk.chunkPath << endl;
            return false;
        }

        if (actualHash != chunk.chunkHash) {
            cerr << "hash mismatch for chunk " << chunk.chunkIndex << endl;
            return false;
        }
    }

    return true;
}

bool runPipelineTest(const char *inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize, const char *outputPath) {

    // split file into chunks
    int chunkCount = splitFileIntoChunks(inputPath, baseChunkDir, fileId, chunkSize);
    if (chunkCount == -1) {
        cerr << "chunking failed" << endl;
        return false;
    }

    // build and store metadata
    if (!buildAndStoreMetadata(inputPath, baseChunkDir, fileId, chunkSize, chunkCount)) return false;

    // get metadata
    const FileMetadata* metadata = getFileMetadata(fileId);
    if (!metadata) return false;

    printFileMetadata(fileId);

    // validate chunk metadata
    if (!validateChunkMetadata(*metadata, baseChunkDir)) return false;

    // validate chunk integrity
    if (!validateChunkIntegrity(*metadata)) return false;

    // merge the splitted chunks
    if (!mergeChunks(baseChunkDir, fileId, metadata->totalChunks, outputPath)) return false;

    // check of input and output file sizze
    if (filesystem::file_size(inputPath) != filesystem::file_size(outputPath)) return false;

    // all good
    return true;
}

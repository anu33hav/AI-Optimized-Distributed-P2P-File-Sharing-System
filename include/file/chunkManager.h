#pragma once
#include <cstddef>
#include <string>

int splitFileIntoChunks(const char* inputPath, const char* baseChunkDir, const char* fileId, size_t chunkSize);
std::string getChunkPath(const char* baseChunkDir, const char* fileId, int chunkIndex);
bool chunkExists(const char* baseChunkDir, const char* fileId, int chunkIndex);
bool mergeChunks(const char* baseChunkDir, const char* fileId, int totalChunks, const char* outputPath);
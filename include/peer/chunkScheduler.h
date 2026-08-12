#pragma once

#include <mutex>
#include <vector>

class ChunkScheduler {
private:
    int totalChunks;
    mutable std::mutex mtx;
    std::vector<bool> done;
    std::vector<bool> claimed;

public:
    ChunkScheduler(int totalChunks);
    int getNextChunk();
    void markDone(int chunkId);
    void markFailed(int chunkId);
    bool allDone() const;
};
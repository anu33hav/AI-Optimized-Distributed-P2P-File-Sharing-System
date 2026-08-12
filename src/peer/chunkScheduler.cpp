#include "peer/chunkScheduler.h"

using namespace std;

// implementation of ctor
ChunkScheduler::ChunkScheduler(int totalChunks) : totalChunks(totalChunks), done(totalChunks, false), claimed(totalChunks, false) {}

int ChunkScheduler::getNextChunk() {

    lock_guard<mutex> lock(mtx);

    for (int i = 0; i < totalChunks; i++) {
        // not done or claimed by other threads
        if (!done[i] && !claimed[i]) {
            claimed[i] = true;
            return i;
        }
    }

    // no next chunk available
    return -1;
}

void ChunkScheduler::markDone(int chunkId) {

    lock_guard<mutex> lock(mtx);
    
    // mark chunk done
    if (chunkId >= 0 && chunkId < totalChunks) {
        done[chunkId] = true;
        claimed[chunkId] = false;
    }
}

void ChunkScheduler::markFailed(int chunkId) {

    lock_guard<mutex> lock(mtx);
    
    // failed
    if (chunkId >= 0 && chunkId < totalChunks) {
        claimed[chunkId] = false;
    }
}

bool ChunkScheduler::allDone() const {

    lock_guard<mutex> lock(mtx);

    // check for all chunk
    for (bool chunkD : done) {
        // not done
        if (!chunkD) {
            return false;
        }
    }

    // all done
    return true;
}
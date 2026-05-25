#pragma once

#include <cstddef>

int splitFileIntoChunks (const char* inputPath, const char* chunkDir, size_t chunkSize);
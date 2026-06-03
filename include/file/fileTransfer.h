#pragma once

long long sendFile(int socketFd, const char* inputPath);
long long recvFile(int socketFd, const char* outputPath);
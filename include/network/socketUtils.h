#pragma once
#include <sys/types.h>

int createServerSocket(int serverPort);
int acceptClient(int serverSocketFd);
int connectToServer(const char* serverIp, int serverPort);
ssize_t sendAll(int clientSocketFd, const char* readFileBuffer, size_t bytesReadFromFileBuffer);
void closeSocket(int socketFd);

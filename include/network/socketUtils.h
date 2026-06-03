#pragma once
#include <sys/types.h>
int createServerSocket(const int port);
int acceptClient(int serverSocketFd);
int connectToServer(const char* ip, const int port);
void closeSocket(int socketFd);
ssize_t sendAll(int socketFd, const char* buffer, int byteRead);
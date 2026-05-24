#pragma once

long long sendFile(int clientSocketFd, const char* filePath);
long long receiveFile(int serverToClientSocketFd, const char* ouputPath);
#pragma once
#include <string>

bool computeFileSha256(const std::string &filePath, std::string &hexDigest);
bool computeStringSha256(const std::string &input, std::string &hexDigest);

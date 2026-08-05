#include "file/hashUtils.h"
#include <string>
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <vector>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std;

string bytesToHex(const unsigned char* data, size_t len);

bool computeFileSha256(const string &filePath, string &hexDigest) {

    // make a input file
    ifstream input(filePath, ios::binary);
    if (!input) { // missing file or permission denied
        cerr << "failed to open input file: " << filePath << endl;
        return false;
    }

    // to save the state/context, the hashing will done in chunks, not in whole
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        cerr << "failed to create sha 256 context" << endl;
        return false;
    }

    vector<char> data(4096);

    bool ok = true;

    // init. context, that it should perform sha 256
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        ok = false;
    }

    while (ok && input) {
        input.read(data.data(), (streamsize)data.size());
        streamsize bytesRead = input.gcount();
        if (bytesRead > 0) {
            if (EVP_DigestUpdate(ctx, data.data(), (size_t)bytesRead) != 1) {
                ok = false;
            }
        }
    }

    // hashed the file
    // save create memory so OpenSSl will put the final hash
    unsigned int digestLen = 0; // unsigned int -> bc OpenSSL uses this internally
    unsigned char digest[EVP_MAX_MD_SIZE]; // 64 byte but we only use 32 byte
    // create a final hash
    if (ok && EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1) {
        ok = false;
    }

    // free up context from memory
    EVP_MD_CTX_free(ctx);

    if (!ok) {
        return false;
    }

    hexDigest = bytesToHex(digest, digestLen);
    return true;
}

bool computeStringSha256(const string &input, string &hexDigest) {

    // to save the state/context, the hashing will done in chunks, not in whole
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        cerr << "failed to create sha256 context for string input" << endl;
        return false;
    }

    bool ok = true;
    // init. context
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        ok = false;
    }

    // hash it
    if (ok && EVP_DigestUpdate(ctx, input.data(), input.size()) != 1) {
        ok = false;
    }

    // create a buffer to store final hash
    unsigned int digestLen = 0;
    unsigned char digest[EVP_MAX_MD_SIZE];
    if (ok && EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1) {
        ok = false;
    }

    // free context
    EVP_MD_CTX_free(ctx);
    if (!ok) {
        return false;
    }

    hexDigest = bytesToHex(digest, digestLen);
    return true;
}

string bytesToHex(const unsigned char* data, size_t len)  {
    // open output
    ostringstream oss;
    oss << hex << setfill('0'); // write in hexdecimal, hexDecimal have to 2 char
    for (size_t i = 0; i < len; i++) {
        oss << setw(2) << (int)data[i]; // take two storage
    }

    return oss.str();
}
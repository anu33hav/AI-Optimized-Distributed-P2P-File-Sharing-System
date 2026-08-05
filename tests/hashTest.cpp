#include "file/hashUtils.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "usage: " << endl;
        cerr << " " << argv[0] << " <file1> [file2]" << endl;
        return 1;
    }

    string hash1;
    if (!computeFileSha256(argv[1], hash1)) {
        cerr << "failed to hash file: " << argv[1] << endl;
        return 1;
    }

    cout << argv[1] << ": " << hash1 << endl;

    if (argc >= 3) {
        string hash2;
        if (!computeFileSha256(argv[2], hash2)) {
            cerr << "failed to hash file: " << argv[2] << endl;
            return 1;
        }

        cout << argv[2] << ": " << hash2 << endl;
        cout << (hash1 == hash2 ? "same" : "different") << endl;
    }

    return 0;
}
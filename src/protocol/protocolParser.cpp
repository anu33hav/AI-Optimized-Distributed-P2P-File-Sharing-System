#include "protocol/protocolParser.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

static string trimNewLine(const string &s) {

    // not empty && last char is newline
    if (!s.empty() && s.back() == '\n') return s.substr(0, s.length()-1);
    
    // no newline in last
    return s;
}

ParsedMessage parseMessage(const string &rawMessage) {
    
    ParsedMessage result;
    string message = trimNewLine(rawMessage);
    
    // empty message
    if (message.empty()) return result;

    // convert string into stream
    istringstream iss(message);

    // set command
    string command;
    if (!(iss >> command)) return result; // store from iss to command
    result.command = command;

    // for CONNECT
    if (command == "CONNECT") {

        string peerId;
        // not valid message
        if (!(iss >> peerId)) return result;

        // valid set
        result.type = MessageType::CONNECT;
        result.payload = peerId;
        result.valid = true;
        return result;
    }
    // for REQUEST
    else if (command == "REQUEST") {

        string fileId;
        string chunkText;
        if (!(iss >> fileId >> chunkText)) return result;

        try {
            result.chunkId = stoi(chunkText);
        }
        catch (...) {
            return result;
        }

        result.type = MessageType::REQUEST;
        result.fileId = fileId;
        result.valid = true;
        return result;
    }
    // for CHUNK
    else if (command == "CHUNK") {

        string fileId;
        string chunkText;
        string chunkSizeText;
        if (!(iss >> fileId >> chunkText >> chunkSizeText)) return result;

        try {
            result.chunkId = stoi(chunkText);
            result.chunkSize = stoi(chunkSizeText);
        }
        catch (...) {
            return result;
        }

        // string payload;
        // getline(iss, payload); // iss >> payload only read till space, newline, 00
        // if (!payload.empty() && payload.front() == ' ') {
        //     payload.erase(0, 1); // erase first space
        // }

        result.type = MessageType::CHUNK;
        result.fileId = fileId;
        result.valid = true;
        return result;
    }
    // for COMPLETE
    else if (command == "COMPLETE") {

        result.type = MessageType::COMPLETE;
        result.valid = true;
        return result;
    }
    // for ERROR
    else if (command == "ERROR") {

        string errorText;
        getline(iss, errorText);
        if (!errorText.empty() && errorText.front() == ' ') {
            errorText.erase(0, 1);
        }

        result.type = MessageType::ERROR;
        result.payload = errorText;
        result.valid = true;
        return result;
    }

    return result; // return unknown
}

string messageTypeToString(MessageType type) {
    if (type == MessageType::CONNECT) return "CONNECT";
    else if (type == MessageType::REQUEST) return "REQUEST";
    else if(type == MessageType::CHUNK) return "CHUNK";
    else if (type == MessageType::COMPLETE) return "COMPLETE";
    else if (type == MessageType::ERROR) return "ERROR";
    else return "UNKNOWN";
}
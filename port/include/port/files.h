#pragma once

#include <stdint.h>
#include <stdio.h>

#include <string>
#include <vector>

namespace port {
namespace files {

bool openGame(const std::string& source, std::string* error);
const std::string& gameSource();

const std::string& bundle();

const std::string& dataDir();

struct Info {
    bool exists = false;
    bool dir = false;
    uint64_t size = 0;
};
Info stat(const std::string& path);
std::vector<std::string> list(const std::string& dir);
bool read(const std::string& path, std::vector<unsigned char>* out);
FILE* open(const std::string& path);
FILE* memoryFile(const std::vector<unsigned char>& data);

}
}

#pragma once

#include <string>

namespace port {
namespace platform {
namespace update {

enum class Stage { Off, Checking, Current, Available, Downloading, Installing, Ready, Failed };

struct Status {
    Stage stage = Stage::Off;
    std::string version;
    std::string notes;
    std::string page;
    bool installable = false;
    long long done = 0, total = 0;
    std::string error;
};

void setup(int argc, char** argv, const char* version);
std::string currentVersion();
bool enabled();
void check(bool russian);
void install(bool russian);
Status status();
bool restart();
void cleanup();
int compareVersions(const std::string& a, const std::string& b);

}
}
}

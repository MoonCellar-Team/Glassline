#pragma once

#include <string>

namespace port {
namespace platform {
namespace hd {

struct Status {
    bool running = false;
    std::string stage;
    std::string error;
    int done = 0, total = 0;
};

void start(const std::string& game, bool russian);
void stop(bool wait);
Status status();

}
}
}

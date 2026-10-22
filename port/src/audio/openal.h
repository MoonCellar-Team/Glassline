#pragma once

namespace port {
namespace audio {

void setSessionActive(bool active);

bool logOn();
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}
}

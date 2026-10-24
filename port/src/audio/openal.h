#pragma once

namespace port {
namespace audio {

void setSessionActive(bool active);

void useLoopback(int rate);
void renderLoopback(float* out, int frames);

bool logOn();
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}
}

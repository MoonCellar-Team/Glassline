#pragma once

namespace port {
namespace platform {
namespace capture {

void start();
bool active();
bool replaying();

void postPointer(int kind, int x, int y, int px, int py, int tap);

int frameBegin();
void frameEnd();
bool wantsVideoFrame();
void videoFrame(const unsigned char* rgba, int w, int h);

void finish();

}
}
}

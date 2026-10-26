#pragma once

union SDL_Event;

namespace port {
namespace platform {

bool inputEvent(const SDL_Event& e);
void inputUpdate();
int inputPollMs();
void inputReset();

const int kPointerPressed = 0;
const int kPointerDragged = 1;
const int kPointerReleased = 2;
int pointerCall(int kind, int x, int y, int px, int py, int tap, bool mouse);
void pointerApply(int kind, int x, int y, int pointer, int tap);

}
}

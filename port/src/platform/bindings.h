#pragma once

#include <SDL.h>

namespace port {
namespace platform {
namespace bindings {

enum Action {
    ActUp = 0,
    ActDown,
    ActLeft,
    ActRight,
    ActTwoLeft,
    ActTwoRight,
    ActTap,
    ActTiltLeft,
    ActTiltRight,
    ActPause,
    ActCount
};

SDL_Keycode key(Action action);
int button(Action action);
float deadzone();

void setKey(Action action, SDL_Keycode code);
void setButton(Action action, int sdlButton);
void setDeadzone(float zone);
void reset();
void reload();

const char* keyLabel(Action action);
const char* buttonLabel(Action action);

void capture(int action, bool pad);
bool capturing();
bool capturePad();
int captureAction();
bool takeEvent(const SDL_Event& e);

}
}
}

#pragma once

union SDL_Event;

namespace port {
namespace platform {

bool inputEvent(const SDL_Event& e);
void inputUpdate();
int inputPollMs();
void inputReset();

}
}

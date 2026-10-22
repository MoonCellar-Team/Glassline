#pragma once

#include <SDL.h>

namespace port {
namespace platform {
namespace turn {

int quarter(SDL_Window* window);
void size(SDL_Window* window, int* w, int* h);
void event(SDL_Event* e);
void point(SDL_Window* window, int* x, int* y);

void imgui(SDL_Window* window);
void begin(SDL_Renderer* renderer, SDL_Window* window);
void end(SDL_Renderer* renderer, SDL_Window* window);
void release(SDL_Renderer* renderer);

}
}
}

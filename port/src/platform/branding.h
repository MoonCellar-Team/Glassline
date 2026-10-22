#pragma once

#include <SDL.h>

#include <string>

#include "../gfx/png.h"

namespace port {
namespace platform {

const char* const kAppId = "space.mooncellar.rooftops";

std::string brandingDir();
bool loadBrandingImage(const std::string& file, port::gfx::Image* out);
void setAppId();
void setWindowIcon(SDL_Window* window);

}
}

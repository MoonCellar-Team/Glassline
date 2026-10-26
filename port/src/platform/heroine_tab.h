#pragma once

#include <string>
#include <vector>

#include "port/platform.h"

struct SDL_Renderer;

namespace port {
namespace platform {
namespace heroine {

void update(SDL_Renderer* renderer, const std::string& game);
void tab(float scale, bool russian);
float barHeight(float scale);
void bar(float scale);
void shutdown();
void reloadSkins();
void picked(bool texture, const std::string& path);
#ifdef MEDECOMP_ONE_PROCESS
bool previewRequested();
bool takePreviewRequest(std::vector<std::string>* env);
#endif

}
}
}

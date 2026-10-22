#pragma once

#include <string>
#include <vector>

struct SDL_Renderer;

namespace port {
namespace platform {
namespace heroine {

void update(SDL_Renderer* renderer, const std::string& game);
void tab(float scale, bool russian);
void shutdown();
void reloadSkins();
void picked(bool texture, const std::string& path);
#ifdef __ANDROID__
bool previewRequested();
bool takePreviewRequest(std::vector<std::string>* env);
#endif

}
}
}

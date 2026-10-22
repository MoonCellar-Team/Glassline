#include "branding.h"

#include <stdio.h>

#include <fstream>
#include <iterator>

namespace port {
namespace platform {

std::string brandingDir() {
    static const std::string dir = [] {
        char* base = SDL_GetBasePath();
        std::string out = base ? std::string(base) : std::string("./");
        SDL_free(base);
        return out + "branding/";
    }();
    return dir;
}

bool loadBrandingImage(const std::string& file, port::gfx::Image* out) {
    std::ifstream in(brandingDir() + file, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.empty() ||
        !port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), *out, false)) {
        fprintf(stderr, "medecomp: знак MoonCellar: нет %s%s\n", brandingDir().c_str(), file.c_str());
        return false;
    }
    return true;
}

void setAppId() {
    for (const char* name : {"SDL_APP_ID", "SDL_VIDEO_X11_WMCLASS", "SDL_VIDEO_WAYLAND_WMCLASS"})
        SDL_setenv(name, kAppId, 0);
    SDL_SetHint("SDL_APP_ID", SDL_getenv("SDL_APP_ID"));
}

void setWindowIcon(SDL_Window* window) {
    static port::gfx::Image icon;
    static const bool ok = loadBrandingImage("app-icon-256.png", &icon);
    if (!ok || !window) return;
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(icon.rgba.data(), icon.width, icon.height, 32,
                                                              icon.width * 4, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return;
    SDL_SetWindowIcon(window, surface);
    SDL_FreeSurface(surface);
}

}
}

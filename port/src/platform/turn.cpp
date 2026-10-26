#include "turn.h"
#include "settings.h"

#include <stdlib.h>

#include <utility>

#include "imgui.h"

namespace port {
namespace platform {
namespace turn {

namespace {

#ifdef __ANDROID__
SDL_Sensor* s_accel;
bool s_accelTried;
int s_quarter = 1;

int side() {
    if (!s_accel && !s_accelTried) {
        s_accelTried = true;
        if (SDL_InitSubSystem(SDL_INIT_SENSOR) == 0)
            for (int i = 0; i < SDL_NumSensors() && !s_accel; i++)
                if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_ACCEL) s_accel = SDL_SensorOpen(i);
    }
    float a[3];
    if (s_accel && SDL_SensorGetData(s_accel, a, 3) == 0) {
        const float kSideways = 6.0f;
        if (a[0] > kSideways) s_quarter = 1;
        else if (a[0] < -kSideways) s_quarter = 3;
    }
    return s_quarter;
}
#endif

int forced() {
    static const int value = [] {
        const char* v = getenv("MEDECOMP_TURN");
        return v && (atoi(v) == 1 || atoi(v) == 3) ? atoi(v) : 0;
    }();
    return value;
}

struct Target {
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int w = 0, h = 0;
    bool active = false;
};
Target s_target;

}

int quarter(SDL_Window* window) {
    if (!window) return 0;
    if (forced()) return forced();
#ifdef __ANDROID__
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    return h > w ? side() : 0;
#elif defined(__EMSCRIPTEN__)
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    const std::string orientation = settings::get("web_orientation", "auto");
    if (orientation == "landscape" && h > w) return 1;
    if (orientation == "portrait" && w > h) return 3;
    return 0;
#else
    return 0;
#endif
}

void size(SDL_Window* window, int* w, int* h) {
    SDL_GetWindowSize(window, w, h);
    if (quarter(window)) std::swap(*w, *h);
}

template <class T>
void toLandscape(int q, T pw, T ph, T* x, T* y) {
    T px = *x, py = *y;
    if (q == 1) {
        *x = py;
        *y = pw - px;
    } else {
        *x = ph - py;
        *y = px;
    }
}

template <class T>
void deltaToLandscape(int q, T* dx, T* dy) {
    T px = *dx, py = *dy;
    if (q == 1) {
        *dx = py;
        *dy = -px;
    } else {
        *dx = -py;
        *dy = px;
    }
}

void event(SDL_Event* e) {
    Uint32 id = 0;
    switch (e->type) {
    case SDL_MOUSEMOTION: id = e->motion.windowID; break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: id = e->button.windowID; break;
    case SDL_MOUSEWHEEL: id = e->wheel.windowID; break;
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION: id = e->tfinger.windowID; break;
    default: return;
    }
    SDL_Window* window = id ? SDL_GetWindowFromID(id) : nullptr;
    if (!window) window = SDL_GetMouseFocus();
    if (!window) window = SDL_GetKeyboardFocus();
    int q = quarter(window);
    if (!q) return;
    int pw = 0, ph = 0;
    SDL_GetWindowSize(window, &pw, &ph);
    switch (e->type) {
    case SDL_MOUSEMOTION:
        toLandscape(q, pw, ph, &e->motion.x, &e->motion.y);
        deltaToLandscape(q, &e->motion.xrel, &e->motion.yrel);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: toLandscape(q, pw, ph, &e->button.x, &e->button.y); break;
    case SDL_MOUSEWHEEL: toLandscape(q, pw, ph, &e->wheel.mouseX, &e->wheel.mouseY); break;
    default:
        toLandscape(q, 1.0f, 1.0f, &e->tfinger.x, &e->tfinger.y);
        deltaToLandscape(q, &e->tfinger.dx, &e->tfinger.dy);
        break;
    }
}

void point(SDL_Window* window, int* x, int* y) {
    int q = quarter(window);
    if (!q) return;
    int pw = 0, ph = 0;
    SDL_GetWindowSize(window, &pw, &ph);
    toLandscape(q, pw, ph, x, y);
}

void imgui(SDL_Window* window) {
    if (!quarter(window)) return;
    ImGuiIO& io = ImGui::GetIO();
    std::swap(io.DisplaySize.x, io.DisplaySize.y);
}

void begin(SDL_Renderer* renderer, SDL_Window* window) {
    if (!quarter(window)) return;
    SDL_SetRenderTarget(renderer, nullptr);
    int pw = 0, ph = 0;
    SDL_GetRendererOutputSize(renderer, &pw, &ph);
    if (s_target.renderer != renderer || s_target.w != ph || s_target.h != pw || !s_target.texture) {
        if (s_target.renderer == renderer && s_target.texture) SDL_DestroyTexture(s_target.texture);
        SDL_RendererInfo info;
        Uint32 format = SDL_GetRendererInfo(renderer, &info) == 0 && info.num_texture_formats > 0 ? info.texture_formats[0]
                                                                                                    : SDL_PIXELFORMAT_ARGB8888;
        s_target.renderer = renderer;
        s_target.w = ph;
        s_target.h = pw;
        s_target.texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, ph, pw);
    }
    s_target.active = s_target.texture && SDL_SetRenderTarget(renderer, s_target.texture) == 0;
}

void end(SDL_Renderer* renderer, SDL_Window* window) {
    if (s_target.renderer != renderer || !s_target.active) return;
    s_target.active = false;
    int q = quarter(window);
    SDL_SetRenderTarget(renderer, nullptr);
    float sx = 1.0f, sy = 1.0f;
    SDL_RenderGetScale(renderer, &sx, &sy);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    int pw = 0, ph = 0;
    SDL_GetRendererOutputSize(renderer, &pw, &ph);
    const SDL_Rect dst = {(pw - s_target.w) / 2, (ph - s_target.h) / 2, s_target.w, s_target.h};
    SDL_RenderCopyEx(renderer, s_target.texture, nullptr, &dst, q == 3 ? 270.0 : 90.0, nullptr, SDL_FLIP_NONE);
    SDL_RenderSetScale(renderer, sx, sy);
}

void release(SDL_Renderer* renderer) {
    if (s_target.renderer != renderer) return;
    if (s_target.texture) SDL_DestroyTexture(s_target.texture);
    s_target = Target();
}

}
}
}

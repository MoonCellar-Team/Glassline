#include <SDL.h>
#ifdef __ANDROID__
#include <jni.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "port/platform.h"
#include "fsr.h"
#include "../gfx/attribs.h"
#include "overlay.h"
#include "gl_shared.h"
#include "settings.h"
#include "turn.h"

namespace port {
namespace platform {

GLFuncs gl;
PfnRenderbufferStorageMultisample glRenderbufferStorageMultisample_;
PfnBlitFramebuffer glBlitFramebuffer_;
GLint s_attrPos = -1, s_attrUV = -1;

namespace {

bool loadGL() {
    bool ok = true;
#define GL_LOAD(U, N)                                                                   \
    gl.N = reinterpret_cast<PFNGL##U##PROC>(SDL_GL_GetProcAddress("gl" #N));            \
    if (!gl.N) {                                                                        \
        fprintf(stderr, "medecomp: в драйвере GL нет gl" #N "\n");                      \
        ok = false;                                                                     \
    }
    GL_FUNCS(GL_LOAD)
    if (!ok) return false;
    const char* version = reinterpret_cast<const char*>(gl.GetString(GL_VERSION));
    int major = 0;
    if (version) sscanf(version, "OpenGL ES %d", &major);
    if (major >= 3) {
        glRenderbufferStorageMultisample_ = reinterpret_cast<PfnRenderbufferStorageMultisample>(
            SDL_GL_GetProcAddress("glRenderbufferStorageMultisample"));
        glBlitFramebuffer_ = reinterpret_cast<PfnBlitFramebuffer>(SDL_GL_GetProcAddress("glBlitFramebuffer"));
    }
    return ok;
}

SDL_Window* s_window;
SDL_GLContext s_context;
int s_requested;
GLuint s_fbo, s_color, s_depth;
GLuint s_msFbo, s_msColor, s_msDepth;
int s_samples;
GLuint s_program;
GLint s_uniTex = -1;
std::atomic<int> s_upscale(0);
float s_sharp = 1.0f;
GLuint s_easu, s_rcas, s_upTex, s_upFbo;
GLint s_easuCon0 = -1, s_easuTexel = -1, s_easuTex = -1, s_rcasTex = -1, s_rcasTexel = -1, s_rcasSharp = -1;
int s_upW, s_upH, s_colorFilter = GL_LINEAR;
int s_wideExtra;
float s_renderScale = 1.0f;
int frameWidth() { return kScreenWidth + 2 * s_wideExtra; }
Uint32 s_wakeEvent = (Uint32)-1;

std::mutex s_ctxMutex;
std::condition_variable s_ctxFree;
std::thread::id s_owner;
int s_waiting;
std::atomic<bool> s_background(false);
std::thread::id s_mainThread;

#ifdef __ANDROID__
int SDLCALL appEvents(void*, SDL_Event* e) {
    if (e->type == SDL_APP_DIDENTERFOREGROUND) s_background = false;
    return 1;
}
#endif

GLuint compile(GLenum type, const char* src) {
    GLuint s = gl.CreateShader(type);
    gl.ShaderSource(s, 1, &src, nullptr);
    gl.CompileShader(s);
    GLint ok = 0;
    gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = "";
        gl.GetShaderInfoLog(s, sizeof log, nullptr, log);
        fprintf(stderr, "medecomp: шейдер вывода кадра: %s\n", log);
    }
    return s;
}

}

GLuint link(const char* vs, const char* fs) {
    GLuint p = gl.CreateProgram();
    gl.AttachShader(p, compile(GL_VERTEX_SHADER, vs));
    gl.AttachShader(p, compile(GL_FRAGMENT_SHADER, fs));
    gl.BindAttribLocation(p, s_attrPos, "a_pos");
    gl.BindAttribLocation(p, s_attrUV, "a_uv");
    gl.LinkProgram(p);
    GLint ok = 0;
    gl.GetProgramiv(p, GL_LINK_STATUS, &ok);
    return ok ? p : 0;
}

namespace {

void createFsr() {
    const char* vs =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "varying vec2 v_uv;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";
    s_easu = link(vs, kFsrEasuFragment);
    s_rcas = link(vs, kFsrRcasFragment);
    if (!s_easu || !s_rcas) {
        fprintf(stderr, "medecomp: FSR не собран — кадр увеличивается билинейно\n");
        s_easu = s_rcas = 0;
        return;
    }
    s_easuTex = gl.GetUniformLocation(s_easu, "u_tex");
    s_easuCon0 = gl.GetUniformLocation(s_easu, "u_con0");
    s_easuTexel = gl.GetUniformLocation(s_easu, "u_texel");
    s_rcasTex = gl.GetUniformLocation(s_rcas, "u_tex");
    s_rcasTexel = gl.GetUniformLocation(s_rcas, "u_texel");
    s_rcasSharp = gl.GetUniformLocation(s_rcas, "u_sharp");
}

bool upscaleTarget(int w, int h) {
    if (s_upTex && s_upW == w && s_upH == h) return true;
    if (!s_upTex) {
        gl.GenTextures(1, &s_upTex);
        gl.GenFramebuffers(1, &s_upFbo);
    }
    gl.BindTexture(GL_TEXTURE_2D, s_upTex);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindFramebuffer(GL_FRAMEBUFFER, s_upFbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_upTex, 0);
    bool ok = gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    s_upW = w;
    s_upH = h;
    if (!ok) fprintf(stderr, "medecomp: буфер FSR %dx%d не собран — кадр увеличивается билинейно\n", w, h);
    return ok;
}

bool createProgram() {
    const char* vs =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "varying vec2 v_uv;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";
    const char* fs =
        "precision mediump float;\n"
        "varying vec2 v_uv;\n"
        "uniform sampler2D u_tex;\n"
        "void main() { gl_FragColor = texture2D(u_tex, v_uv); }\n";
    s_program = gl.CreateProgram();
    gl.AttachShader(s_program, compile(GL_VERTEX_SHADER, vs));
    gl.AttachShader(s_program, compile(GL_FRAGMENT_SHADER, fs));
    GLint maxAttribs = 8;
    gl.GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    s_attrPos = maxAttribs - 2;
    s_attrUV = maxAttribs - 1;
    gl.BindAttribLocation(s_program, s_attrPos, "a_pos");
    gl.BindAttribLocation(s_program, s_attrUV, "a_uv");
    gl.LinkProgram(s_program);
    GLint ok = 0;
    gl.GetProgramiv(s_program, GL_LINK_STATUS, &ok);
    s_uniTex = gl.GetUniformLocation(s_program, "u_tex");
    if (!ok) fprintf(stderr, "medecomp: программа вывода кадра не собрана\n");
    if (ok) createFsr();
    return ok != 0;
}

GLenum depthFormat() { return postNeedsDepth() ? kDepth24 : GLenum(GL_DEPTH_COMPONENT16); }

void createMultisample() {
    int want = originalGraphics() ? 0 : msaaSetting();
    const char* version = reinterpret_cast<const char*>(gl.GetString(GL_VERSION));
    int major = 0;
    if (version) sscanf(version, "OpenGL ES %d", &major);
    if (want == 0 || major < 3 || !glRenderbufferStorageMultisample_ || !glBlitFramebuffer_) {
        if (want) fprintf(stderr, "medecomp: сглаживание MSAA — нужен OpenGL ES 3.0, выключено\n");
        return;
    }
    GLint max = 0;
    gl.GetIntegerv(kMaxSamples, &max);
    s_samples = std::min(want, int(max));
    if (s_samples < 2) {
        s_samples = 0;
        return;
    }
    int w = framebufferPixelWidth(), h = framebufferPixelHeight();
    gl.GenRenderbuffers(1, &s_msColor);
    gl.BindRenderbuffer(GL_RENDERBUFFER, s_msColor);
    glRenderbufferStorageMultisample_(GL_RENDERBUFFER, s_samples, kRgba8, w, h);
    gl.GenRenderbuffers(1, &s_msDepth);
    gl.BindRenderbuffer(GL_RENDERBUFFER, s_msDepth);
    glRenderbufferStorageMultisample_(GL_RENDERBUFFER, s_samples, depthFormat(), w, h);
    gl.BindRenderbuffer(GL_RENDERBUFFER, 0);
    gl.GenFramebuffers(1, &s_msFbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, s_msFbo);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, s_msColor);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_msDepth);
    GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "medecomp: буфер MSAA %dx не собран: 0x%x — без сглаживания\n", s_samples, status);
        s_samples = 0;
        gl.BindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        return;
    }
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fprintf(stderr, "medecomp: сглаживание MSAA %dx\n", s_samples);
}

bool createFramebuffer() {
    gl.GenTextures(1, &s_color);
    gl.BindTexture(GL_TEXTURE_2D, s_color);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebufferPixelWidth(), framebufferPixelHeight(), 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindTexture(GL_TEXTURE_2D, 0);
    gl.GenRenderbuffers(1, &s_depth);
    gl.BindRenderbuffer(GL_RENDERBUFFER, s_depth);
    gl.RenderbufferStorage(GL_RENDERBUFFER, depthFormat(), framebufferPixelWidth(), framebufferPixelHeight());
    gl.BindRenderbuffer(GL_RENDERBUFFER, 0);
    gl.GenFramebuffers(1, &s_fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_color, 0);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);
    GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "medecomp: буфер кадра не собран: 0x%x\n", status);
        return false;
    }
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    createMultisample();
    gl.Viewport(0, 0, framebufferPixelWidth(), framebufferPixelHeight());
    return true;
}

GLuint s_turnFbo, s_turnTex;
int s_turnW, s_turnH;

GLuint outputTarget(int quarter, int* w, int* h) {
    if (!quarter) return 0;
    std::swap(*w, *h);
    if (!s_turnFbo || s_turnW != *w || s_turnH != *h) {
        if (!s_turnTex) gl.GenTextures(1, &s_turnTex);
        gl.BindTexture(GL_TEXTURE_2D, s_turnTex);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *w, *h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (!s_turnFbo) gl.GenFramebuffers(1, &s_turnFbo);
        gl.BindFramebuffer(GL_FRAMEBUFFER, s_turnFbo);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_turnTex, 0);
        s_turnW = *w;
        s_turnH = *h;
    }
    return s_turnFbo;
}

void outputTurned(int quarter, int pw, int ph) {
    static const GLfloat pos[] = {-1, 1, 1, 1, -1, -1, 1, -1};
    static const GLfloat cw[] = {0, 0, 0, 1, 1, 0, 1, 1};
    static const GLfloat ccw[] = {1, 1, 1, 0, 0, 1, 0, 0};
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Viewport(0, 0, pw, ph);
    gl.UseProgram(s_program);
    gl.Uniform1i(s_uniTex, 0);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, s_turnTex);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.EnableVertexAttribArray(s_attrPos);
    gl.EnableVertexAttribArray(s_attrUV);
    gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, 0, pos);
    gl.VertexAttribPointer(s_attrUV, 2, GL_FLOAT, GL_FALSE, 0, quarter == 3 ? ccw : cw);
    gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

std::mutex s_shotMutex;
std::string s_shotPath;
Uint32 s_shotAt;
bool s_shotFrame;

void put32(std::string& s, uint32_t v) {
    char b[4] = {char(v >> 24), char(v >> 16), char(v >> 8), char(v)};
    s.append(b, 4);
}

void chunk(std::string& out, const char* type, const std::string& body) {
    put32(out, uint32_t(body.size()));
    std::string t = std::string(type, 4) + body;
    out += t;
    put32(out, uint32_t(crc32(0, reinterpret_cast<const Bytef*>(t.data()), uInt(t.size()))));
}

void writeShot(const std::string& path, int x0, int y0, int w, int h) {
    std::vector<uint8_t> px(size_t(w) * h * 4);
    gl.ReadPixels(x0, y0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::string raw;
    raw.reserve(size_t(h) * (w * 3 + 1));
    for (int y = h - 1; y >= 0; y--) {
        raw += '\0';
        for (int x = 0; x < w; x++) raw.append(reinterpret_cast<const char*>(&px[(size_t(y) * w + x) * 4]), 3);
    }
    uLongf size = compressBound(uLong(raw.size()));
    std::string z(size, '\0');
    compress2(reinterpret_cast<Bytef*>(&z[0]), &size, reinterpret_cast<const Bytef*>(raw.data()), uLong(raw.size()), 6);
    z.resize(size);
    std::string ihdr;
    put32(ihdr, uint32_t(w));
    put32(ihdr, uint32_t(h));
    ihdr += std::string("\x08\x02\x00\x00\x00", 5);
    std::string out = "\x89PNG\r\n\x1a\n";
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", "");
    FILE* f = fopen(path.c_str(), "wb");
    bool ok = f && fwrite(out.data(), 1, out.size(), f) == out.size();
    if (f) fclose(f);
    fprintf(stderr, ok ? "medecomp: снимок кадра: %s\n" : "medecomp: снимок не записан: %s\n", path.c_str());
}

void fit(int w, int h, float* scale, float* ox, float* oy) {
    *scale = w * kScreenHeight < h * frameWidth() ? float(w) / frameWidth() : float(h) / kScreenHeight;
    if (Upscale(s_upscale.load()) == Upscale::Nearest && *scale >= 1.0f) *scale = floorf(*scale);
    *ox = (w - frameWidth() * *scale) / 2;
    *oy = (h - kScreenHeight * *scale) / 2;
}

}

namespace {

const char* const kModeNames[] = {"window", "fullscreen", "borderless"};
WindowMode s_fullKind = WindowMode::Borderless;

bool fullscreenDisplayMode(int display, SDL_DisplayMode* out) {
    SDL_DisplayMode desktop;
    if (SDL_GetDesktopDisplayMode(display, &desktop) != 0) return false;
    *out = desktop;
    int w = 0, h = 0;
    if (resolutionSetting(&w, &h) && (w != desktop.w || h != desktop.h)) {
        SDL_DisplayMode want = desktop, got;
        want.w = w;
        want.h = h;
        if (SDL_GetClosestDisplayMode(display, &want, &got)) *out = got;
    }
    return true;
}

void applyMode(WindowMode mode) {
    if (mode == WindowMode::Window) {
        SDL_SetWindowFullscreen(s_window, 0);
    } else if (mode == WindowMode::Fullscreen) {
        SDL_DisplayMode m;
        int display = SDL_GetWindowDisplayIndex(s_window);
        if (fullscreenDisplayMode(display < 0 ? 0 : display, &m)) SDL_SetWindowDisplayMode(s_window, &m);
        SDL_SetWindowFullscreen(s_window, SDL_WINDOW_FULLSCREEN);
    } else {
        SDL_SetWindowFullscreen(s_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

}

WindowMode modeByName(const std::string& name) {
    for (int i = 0; i < 3; i++)
        if (name == kModeNames[i]) return WindowMode(i);
    return WindowMode::Window;
}

bool gamescopeSession() {
#if defined(__linux__) && !defined(__ANDROID__)
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    const char* gamepadUi = getenv("SteamGamepadUI");
    return getenv("GAMESCOPE_WAYLAND_DISPLAY") || (desktop && strcasecmp(desktop, "gamescope") == 0) ||
           (gamepadUi && *gamepadUi && strcmp(gamepadUi, "0") != 0);
#else
    return false;
#endif
}

WindowMode windowModeSetting() {
#ifdef __ANDROID__
    return WindowMode::Borderless;
#else
    std::string mode = settings::get("window");
    if (mode.empty() && gamescopeSession()) return WindowMode::Borderless;
    return modeByName(mode);
#endif
}

void setWindowModeSetting(WindowMode mode) {
    settings::set("window", kModeNames[int(mode)]);
    if (mode != WindowMode::Window) settings::set("fullscreen", kModeNames[int(mode)]);
}

namespace {
std::atomic<int> s_timerRate(60);
std::atomic<unsigned long> s_presents(0);
}

int frameRateSetting() {
    std::string v = settings::get("fps", "display");
    if (v == "display") return kFrameRateDisplay;
    if (v == "unlimited") return kFrameRateUnlimited;
    int fps = atoi(v.c_str());
    return fps >= 10 && fps <= 1000 ? fps : kFrameRateDisplay;
}

void setFrameRateSetting(int fps) {
    settings::set("fps", fps == kFrameRateDisplay     ? std::string("display")
                         : fps == kFrameRateUnlimited ? std::string("unlimited")
                                                      : std::to_string(fps));
}

bool vsyncSetting() { return settings::get("vsync", "on") != "off"; }

void setVsyncSetting(bool on) { settings::set("vsync", on ? "on" : "off"); }

VideoDriver videoDriverSetting() {
    std::string v = settings::get("video_driver", "auto");
    return v == "wayland" ? VideoDriver::Wayland : v == "x11" ? VideoDriver::X11 : VideoDriver::Auto;
}

void setVideoDriverSetting(VideoDriver driver) {
    settings::set("video_driver", driver == VideoDriver::Wayland ? "wayland" : driver == VideoDriver::X11 ? "x11" : "auto");
}

void applyVideoDriver(bool reinit) {
#ifdef __linux__
    const char* forced = getenv("SDL_VIDEODRIVER");
    if ((forced && *forced) || !getenv("WAYLAND_DISPLAY")) return;
    const char* hint = nullptr;
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    switch (videoDriverSetting()) {
    case VideoDriver::Wayland: hint = "wayland,x11"; break;
    case VideoDriver::X11: hint = "x11,wayland"; break;
    case VideoDriver::Auto:
        if (gamescopeSession())
            hint = "x11,wayland";
        else if (!desktop || !strstr(desktop, "GNOME"))
            hint = "wayland,x11";
        break;
    }
    const char* current = SDL_WasInit(SDL_INIT_VIDEO) ? SDL_GetCurrentVideoDriver() : nullptr;
    if (reinit && hint && current && strncmp(hint, current, strlen(current)) != 0) {
        fprintf(stderr, "medecomp: оконная система %s -> %.*s\n", current, int(strcspn(hint, ",")), hint);
        for (int i = 0; i < 4 && SDL_WasInit(SDL_INIT_VIDEO); i++) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    if (hint)
        SDL_SetHint(SDL_HINT_VIDEODRIVER, hint);
    else
        SDL_ResetHint(SDL_HINT_VIDEODRIVER);
#else
    (void)reinit;
#endif
}

int displaySetting() {
    std::string v = settings::get("display", "");
    int count = SDL_GetNumVideoDisplays();
    if (v.empty() || count <= 0) return 0;
    size_t colon = v.find(':');
    int index = colon == std::string::npos ? -1 : atoi(v.substr(0, colon).c_str());
    std::string name = colon == std::string::npos ? v : v.substr(colon + 1);
    auto nameOf = [](int i) {
        const char* n = SDL_GetDisplayName(i);
        return std::string(n ? n : "");
    };
    if (index >= 0 && index < count && nameOf(index) == name) return index;
    for (int i = 0; i < count; i++)
        if (nameOf(i) == name) return i;
    return 0;
}

void setDisplaySetting(int display) {
    const char* name = SDL_GetDisplayName(display);
    settings::set("display", std::to_string(display) + ":" + (name ? name : ""));
}

bool resolutionSetting(int* width, int* height) {
    int w = 0, h = 0;
    if (sscanf(settings::get("resolution", "").c_str(), "%dx%d", &w, &h) != 2 || w <= 0 || h <= 0) return false;
    *width = w;
    *height = h;
    return true;
}

void setResolutionSetting(int width, int height) {
    settings::set("resolution", width > 0 && height > 0 ? std::to_string(width) + "x" + std::to_string(height)
                                                        : std::string("desktop"));
}

int displayRefreshRate(int display) {
    int best = 0;
    for (int i = 0, n = SDL_GetNumDisplayModes(display); i < n; i++) {
        SDL_DisplayMode m;
        if (SDL_GetDisplayMode(display, i, &m) == 0) best = std::max(best, m.refresh_rate);
    }
    SDL_DisplayMode desktop;
    if (best == 0 && SDL_GetDesktopDisplayMode(display, &desktop) == 0) best = desktop.refresh_rate;
    return best > 0 ? best : 60;
}

std::vector<int> frameRateChoices(int display) {
    int top = displayRefreshRate(display);
    std::vector<int> out = {30};
    for (int fps : {60, 120, 144, 165})
        if (fps <= top + 1) out.push_back(fps);
    return out;
}

int frameTimerRate() { return s_timerRate.load(); }

Upscale upscaleSetting() {
    std::string v = settings::get("upscale", "fsr");
    return v == "linear" ? Upscale::Linear : v == "nearest" ? Upscale::Nearest : Upscale::Fsr;
}

float sharpnessSetting() {
    float v = float(atof(settings::get("sharpness", "0.8").c_str()));
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

void setUpscaleSetting(Upscale mode, float sharpness) {
    settings::set("upscale", mode == Upscale::Linear ? "linear" : mode == Upscale::Nearest ? "nearest" : "fsr");
    char buf[16];
    snprintf(buf, sizeof buf, "%.2f", sharpness);
    settings::set("sharpness", buf);
}

unsigned long presentCount() { return s_presents.load(); }

int wideExtra() { return s_wideExtra; }

int framebufferHeight() { return kViewHeight + 2 * s_wideExtra; }

bool widescreenSetting() { return settings::get("widescreen", "on") != "off"; }

float renderScale() { return s_renderScale; }

int framebufferPixelWidth() { return int(lroundf(kViewWidth * s_renderScale)); }

int framebufferPixelHeight() { return int(lroundf(framebufferHeight() * s_renderScale)); }

int renderSetting() {
    int v = atoi(settings::get("render", "100").c_str());
    return v == 0 || v == 50 || v == 75 ? v : 100;
}

void setRenderSetting(int percent) { settings::set("render", std::to_string(percent)); }

int textureFilterSetting() {
    int v = atoi(settings::get("texture_filter", "16").c_str());
    return v == 4 || v == 8 || v == 16 ? v : 0;
}

int msaaSetting() {
    int v = atoi(settings::get("msaa", "4").c_str());
    return v == 2 || v == 4 || v == 8 ? v : 0;
}

void setMsaaSetting(int samples) {
    settings::set("msaa", samples == 2 || samples == 4 || samples == 8 ? std::to_string(samples) : "off");
}

void setTextureFilterSetting(int level) {
    settings::set("texture_filter", level == 4 || level == 8 || level == 16 ? std::to_string(level) : "ipad");
}

void setWidescreenSetting(bool on) { settings::set("widescreen", on ? "on" : "off"); }

bool originalGraphics() { return settings::get("graphics", "remastered") == "original"; }

void setOriginalGraphics(bool on) { settings::set("graphics", on ? "original" : "remastered"); }

float displayScale(SDL_Window* window, int drawableWidth) {
    if (const char* v = getenv("MEDECOMP_UI_SCALE")) {
        float s = float(atof(v));
        if (s >= 0.5f && s <= 4.0f) return s;
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (w > 0 && drawableWidth > w + w / 100) return 1.0f;
    int display = SDL_GetWindowDisplayIndex(window);
    float dpi = 0;
    if (SDL_GetDisplayDPI(display < 0 ? 0 : display, &dpi, nullptr, nullptr) != 0 || dpi < 96.0f) return 1.0f;
    return dpi / 96.0f > 4.0f ? 4.0f : dpi / 96.0f;
}

bool createWindow(const char* title) {
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
    setAppId();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "medecomp: SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    s_wakeEvent = SDL_RegisterEvents(1);
    if (const char* shot = getenv("MEDECOMP_SHOT")) {
        const char* at = getenv("MEDECOMP_SHOT_AT");
        s_shotPath = shot;
        s_shotAt = Uint32((at ? atof(at) : 10.0) * 1000);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    int forcedW = 0, forcedH = 0;
    if (const char* size = getenv("MEDECOMP_WINDOW_SIZE")) sscanf(size, "%dx%d", &forcedW, &forcedH);
    bool forced = forcedW > 0 && forcedH > 0;
    const int screen = displaySetting();
    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED_DISPLAY(screen), SDL_WINDOWPOS_CENTERED_DISPLAY(screen),
                                forced ? forcedW : kScreenWidth, forced ? forcedH : kScreenHeight,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!s_window) {
        fprintf(stderr, "medecomp: окно: %s\n", SDL_GetError());
        return false;
    }
    setWindowIcon(s_window);
    WindowMode mode = windowModeSetting();
    WindowMode full = modeByName(settings::get("fullscreen"));
    s_fullKind = mode != WindowMode::Window ? mode : full != WindowMode::Window ? full : WindowMode::Borderless;
    float scale = 1.0f;
    if (mode != WindowMode::Window) {
        applyMode(mode);
    } else if (!forced) {
        int dw = 0, dh = 0;
        SDL_GL_GetDrawableSize(s_window, &dw, &dh);
        scale = displayScale(s_window, dw);
        SDL_Rect area;
        int display = SDL_GetWindowDisplayIndex(s_window);
        if (SDL_GetDisplayUsableBounds(display < 0 ? 0 : display, &area) == 0) {
            float fit = std::min(0.92f * area.w / kScreenWidth, 0.92f * area.h / kScreenHeight);
            scale = std::max(1.0f, std::min(scale, fit));
        }
        if (scale > 1.01f) {
            SDL_SetWindowSize(s_window, int(kScreenWidth * scale), int(kScreenHeight * scale));
            SDL_SetWindowPosition(s_window, SDL_WINDOWPOS_CENTERED_DISPLAY(screen), SDL_WINDOWPOS_CENTERED_DISPLAY(screen));
        }
    }
    {
        int at = SDL_GetWindowDisplayIndex(s_window);
        const char* name = SDL_GetDisplayName(at < 0 ? 0 : at);
        fprintf(stderr, "medecomp: окно: %s, масштаб экрана %.2f, монитор %d (%s)\n", kModeNames[int(mode)], scale,
                at, name ? name : "?");
    }
    static const int kVersions[][2] = {{3, 1}, {3, 0}, {2, 0}};
    for (const auto& v : kVersions) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, v[0]);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, v[1]);
        s_context = SDL_GL_CreateContext(s_window);
        if (s_context) {
            s_requested = v[0] * 10 + v[1];
            break;
        }
        fprintf(stderr, "medecomp: контекст OpenGL ES %d.%d: %s\n", v[0], v[1], SDL_GetError());
    }
    if (!s_context) return false;
    s_owner = std::this_thread::get_id();
    s_mainThread = s_owner;
#ifdef __ANDROID__
    SDL_AddEventWatch(appEvents, nullptr);
#endif
    int aw = 0, ah = 0;
    {
        SDL_DisplayMode desktop;
        int at = SDL_GetWindowDisplayIndex(s_window);
        if (forced) {
            aw = forcedW;
            ah = forcedH;
        } else if (mode == WindowMode::Fullscreen && fullscreenDisplayMode(at < 0 ? 0 : at, &desktop)) {
            aw = desktop.w;
            ah = desktop.h;
            fprintf(stderr, "medecomp: во весь экран: %dx%d, %d Гц\n", desktop.w, desktop.h, desktop.refresh_rate);
        } else if (mode != WindowMode::Window && SDL_GetDesktopDisplayMode(at < 0 ? 0 : at, &desktop) == 0) {
            aw = desktop.w;
            ah = desktop.h;
        } else {
            SDL_GetWindowSize(s_window, &aw, &ah);
        }
#ifdef __ANDROID__
        if (aw < ah) std::swap(aw, ah);
#endif
    }
    const bool original = originalGraphics();
    const Upscale upscale = original ? Upscale::Nearest : upscaleSetting();
    if (original) fprintf(stderr, "medecomp: графика: Original — кадр как на iPad\n");
    if (!original && widescreenSetting() && aw > 0 && ah > 0) {
        int width = std::min(kScreenHeight * aw / ah, kScreenHeight * 21 / 9) & ~1;
        s_wideExtra = std::max(0, (width - kScreenWidth) / 2);
    }
    if (aw > 0 && ah > 0 && renderSetting() != 0 && upscale != Upscale::Nearest) {
        float out = std::min(float(aw) / frameWidth(), float(ah) / kScreenHeight) * renderSetting() / 100.0f;
        float limit = std::min(4.0f, 8192.0f / framebufferHeight());
        s_renderScale = std::max(1.0f, std::min(out, limit));
    }
    fprintf(stderr, "medecomp: кадр %dx%d (холст 1024×768 по центру, +%d с каждой стороны), буфер %dx%d пикселей\n",
            frameWidth(), kScreenHeight, s_wideExtra, framebufferPixelHeight(), framebufferPixelWidth());
    int fps = original ? 30 : frameRateSetting();
    int display = SDL_GetWindowDisplayIndex(s_window);
    SDL_DisplayMode current;
    int refresh = SDL_GetDesktopDisplayMode(display < 0 ? 0 : display, &current) == 0 && current.refresh_rate > 0
                      ? current.refresh_rate
                      : displayRefreshRate(display < 0 ? 0 : display);
    bool vsync = fps == kFrameRateDisplay || (fps != kFrameRateUnlimited && vsyncSetting());
    s_timerRate = fps == kFrameRateUnlimited ? 1000 : fps > 0 ? fps : refresh * 3 / 2;
    if (SDL_GL_SetSwapInterval(vsync ? 1 : 0) != 0)
        fprintf(stderr, "medecomp: вертикальная синхронизация: %s\n", SDL_GetError());
    s_upscale = int(upscale);
    s_sharp = exp2f(-2.0f * (1.0f - sharpnessSetting()));
    fprintf(stderr, "medecomp: частота кадров: %s, экран %d Гц, вертикальная синхронизация %s, таймер %d\n",
            fps == kFrameRateDisplay ? "как у экрана" : fps == kFrameRateUnlimited ? "без ограничения" : std::to_string(fps).c_str(),
            refresh, vsync ? "вкл." : "выкл.", s_timerRate.load());
    if (!loadGL()) return false;
    fprintf(stderr, "medecomp: %s, %s (запрошен ES %d.%d), окно — %s\n", gl.GetString(GL_VERSION), gl.GetString(GL_RENDERER),
            s_requested / 10, s_requested % 10, SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?");
    return createProgram() && createFramebuffer();
}

unsigned framebuffer() { return s_samples ? s_msFbo : s_fbo; }

bool inBackground() { return s_background; }

void surfaceDestroying() {
    s_background = true;
    std::unique_lock<std::mutex> l(s_ctxMutex);
    if (!s_ctxFree.wait_for(l, std::chrono::milliseconds(400),
                            [] { return s_owner == std::thread::id() || s_owner == s_mainThread; }))
        fprintf(stderr, "medecomp: уход в фон — поток игры не отдал контекст GL за 400 мс\n");
}

void acquireContext() {
    std::thread::id me = std::this_thread::get_id();
    std::unique_lock<std::mutex> l(s_ctxMutex);
    if (s_owner == me) return;
    ++s_waiting;
    while (!s_ctxFree.wait_for(l, std::chrono::seconds(2), [] { return s_owner == std::thread::id(); }))
        fprintf(stderr, "medecomp: жду контекст GL у другого потока\n");
    --s_waiting;
    if (SDL_GL_MakeCurrent(s_window, s_context) != 0)
        fprintf(stderr, "medecomp: контекст GL не сделать текущим: %s\n", SDL_GetError());
    s_owner = me;
}

void beginFrame() {
    if (s_wideExtra <= 0) return;
    gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer());
    gl.Viewport(0, 0, framebufferPixelWidth(), framebufferPixelHeight());
    gl.ClearColor(0, 0, 0, 1);
    gl.Clear(GL_COLOR_BUFFER_BIT);
}

void releaseContext() {
    std::lock_guard<std::mutex> l(s_ctxMutex);
    if (s_owner != std::this_thread::get_id()) return;
    SDL_GL_MakeCurrent(s_window, nullptr);
    s_owner = std::thread::id();
    s_ctxFree.notify_all();
}

void yieldContext() {
    {
        std::lock_guard<std::mutex> l(s_ctxMutex);
        if (s_waiting == 0) return;
    }
    releaseContext();
}

bool toScreen(float wx, float wy, int* x, int* y) {
    int w = kScreenWidth, h = kScreenHeight;
    turn::size(s_window, &w, &h);
    float scale, ox, oy;
    fit(w, h, &scale, &ox, &oy);
    float sx = (wx - ox) / scale - s_wideExtra, sy = (wy - oy) / scale;
    bool inside = sx >= 0 && sy >= 0 && sx < kScreenWidth && sy < kScreenHeight;
    *x = SDL_clamp(int(sx), 0, kScreenWidth - 1);
    *y = SDL_clamp(int(sy), 0, kScreenHeight - 1);
    return inside;
}

void drawTextField(int viewW, int viewH) {
    if (!textInputActive() || viewW < 1 || viewH < 1) return;
    std::vector<unsigned char> rgba;
    int iw = 0, ih = 0;
    if (!textFieldImage(&rgba, &iw, &ih) || iw < 1 || ih < 1) return;
    static GLuint tex = 0;
    static int tw = 0, th = 0;
    if (!tex) gl.GenTextures(1, &tex);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iw, ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    tw = iw;
    th = ih;
    float x0 = (viewW - iw) * 0.5f, y0 = (viewH - ih) * 0.5f;
    auto nx = [&](float x) { return x / float(viewW) * 2.0f - 1.0f; };
    auto ny = [&](float y) { return 1.0f - y / float(viewH) * 2.0f; };
    const GLfloat qpos[] = {nx(x0), ny(y0), nx(x0 + iw), ny(y0), nx(x0), ny(y0 + ih), nx(x0 + iw), ny(y0 + ih)};
    const GLfloat quv[] = {0, 0, 1, 0, 0, 1, 1, 1};
    gl.UseProgram(s_program);
    gl.Uniform1i(s_uniTex, 0);
    gl.EnableVertexAttribArray(s_attrPos);
    gl.EnableVertexAttribArray(s_attrUV);
    gl.VertexAttribPointer(s_attrUV, 2, GL_FLOAT, GL_FALSE, 0, quv);
    gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, 0, qpos);
    gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void present() {
    if (s_background) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return;
    }
    acquireContext();
    port::gfx::releaseGameArrays();
    GLint fbo, program, active, tex, array, viewport[4];
    GLfloat clear[4];
    GLboolean mask[4];
    gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &program);
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &active);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    gl.GetIntegerv(GL_VIEWPORT, viewport);
    gl.GetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    gl.GetBooleanv(GL_COLOR_WRITEMASK, mask);
    const GLenum caps[] = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_DITHER};
    GLboolean was[6];
    for (int i = 0; i < 6; i++) {
        was[i] = gl.IsEnabled(caps[i]);
        gl.Disable(caps[i]);
    }

    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (s_samples) {
        int fw = framebufferPixelWidth(), fh = framebufferPixelHeight();
        gl.BindFramebuffer(kReadFramebuffer, s_msFbo);
        gl.BindFramebuffer(kDrawFramebuffer, s_fbo);
        glBlitFramebuffer_(0, 0, fw, fh, 0, 0, fw, fh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    int w = kScreenWidth, h = kScreenHeight;
    SDL_GL_GetDrawableSize(s_window, &w, &h);
    const int pw = w, ph = h, quarter = turn::quarter(s_window);
    const GLuint out = outputTarget(quarter, &w, &h);
    gl.BindFramebuffer(GL_FRAMEBUFFER, out);
    gl.Viewport(0, 0, w, h);
    gl.ClearColor(0, 0, 0, 1);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    float scale, ox, oy;
    fit(w, h, &scale, &ox, &oy);
    int vw = int(frameWidth() * scale + 0.5f), vh = int(kScreenHeight * scale + 0.5f);
    int vx = (w - vw) / 2, vy = (h - vh) / 2;
    Upscale mode = Upscale(s_upscale.load());
    bool fsr = mode == Upscale::Fsr && scale / s_renderScale > 1.01f && s_easu && upscaleTarget(vh, vw);
    const GLfloat pos[] = {-1, 1, 1, 1, -1, -1, 1, -1};
    const GLfloat uv[] = {1, 1, 1, 0, 0, 1, 0, 0};
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.EnableVertexAttribArray(s_attrPos);
    gl.EnableVertexAttribArray(s_attrUV);
    gl.VertexAttribPointer(s_attrUV, 2, GL_FLOAT, GL_FALSE, 0, uv);
    if (fsr) {
        static const GLfloat full[] = {-1, -1, 1, -1, -1, 1, 1, 1};
        gl.BindFramebuffer(GL_FRAMEBUFFER, s_upFbo);
        gl.Viewport(0, 0, vh, vw);
        gl.UseProgram(s_easu);
        gl.BindTexture(GL_TEXTURE_2D, s_color);
        gl.Uniform1i(s_easuTex, 0);
        float kx = float(framebufferPixelWidth()) / vh, ky = float(framebufferPixelHeight()) / vw;
        gl.Uniform4f(s_easuCon0, kx, ky, 0.5f * kx - 0.5f, 0.5f * ky - 0.5f);
        gl.Uniform2f(s_easuTexel, 1.0f / framebufferPixelWidth(), 1.0f / framebufferPixelHeight());
        gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, 0, full);
        gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        gl.BindFramebuffer(GL_FRAMEBUFFER, out);
        gl.Viewport(vx, vy, vw, vh);
        gl.UseProgram(s_rcas);
        gl.BindTexture(GL_TEXTURE_2D, s_upTex);
        gl.Uniform1i(s_rcasTex, 0);
        gl.Uniform2f(s_rcasTexel, 1.0f / vh, 1.0f / vw);
        gl.Uniform1f(s_rcasSharp, s_sharp);
    } else {
        int filter = mode == Upscale::Nearest ? GL_NEAREST : GL_LINEAR;
        gl.BindTexture(GL_TEXTURE_2D, s_color);
        if (filter != s_colorFilter) {
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            s_colorFilter = filter;
        }
        gl.Viewport(vx, vy, vw, vh);
        gl.UseProgram(s_program);
        gl.Uniform1i(s_uniTex, 0);
    }
    gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, 0, pos);
    gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl.Viewport(0, 0, w, h);
    drawTextField(w, h);
    overlayDraw(w, h);
    {

        std::lock_guard<std::mutex> l(s_shotMutex);
        if (!s_shotPath.empty() && SDL_GetTicks() >= s_shotAt) {
            if (s_shotFrame) writeShot(s_shotPath, vx, vy, vw, vh);
            else writeShot(s_shotPath, 0, 0, w, h);
            s_shotPath.clear();
        }
    }
    overlayCover(w, h);
    if (quarter) outputTurned(quarter, pw, ph);
    SDL_GL_SwapWindow(s_window);
    s_presents++;

    gl.DisableVertexAttribArray(s_attrPos);
    gl.DisableVertexAttribArray(s_attrUV);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    gl.ClearColor(clear[0], clear[1], clear[2], clear[3]);
    gl.ColorMask(mask[0], mask[1], mask[2], mask[3]);
    gl.UseProgram(program);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.ActiveTexture(active);
    gl.BindBuffer(GL_ARRAY_BUFFER, array);
    for (int i = 0; i < 6; i++)
        if (was[i]) gl.Enable(caps[i]);
}

bool presentSplash() {
    if (!splashActive() || !s_window) return false;
    if (s_background) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return true;
    }
    acquireContext();
    port::gfx::releaseGameArrays();
    GLint fbo, program, active, tex, array, viewport[4];
    GLfloat clear[4];
    gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &program);
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &active);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    gl.GetIntegerv(GL_VIEWPORT, viewport);
    gl.GetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    const GLenum caps[] = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_STENCIL_TEST};
    GLboolean was[5];
    for (int i = 0; i < 5; i++) {
        was[i] = gl.IsEnabled(caps[i]);
        gl.Disable(caps[i]);
    }
    int w = kScreenWidth, h = kScreenHeight;
    SDL_GL_GetDrawableSize(s_window, &w, &h);
    const int pw = w, ph = h, quarter = turn::quarter(s_window);
    gl.BindFramebuffer(GL_FRAMEBUFFER, outputTarget(quarter, &w, &h));
    gl.Viewport(0, 0, w, h);
    gl.ClearColor(0, 0, 0, 1);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    overlayDraw(w, h);
    {
        std::lock_guard<std::mutex> l(s_shotMutex);
        if (!s_shotPath.empty() && SDL_GetTicks() >= s_shotAt) {
            writeShot(s_shotPath, 0, 0, w, h);
            s_shotPath.clear();
        }
    }
    if (quarter) outputTurned(quarter, pw, ph);
    SDL_GL_SwapWindow(s_window);
    s_presents++;
    gl.DisableVertexAttribArray(s_attrPos);
    gl.DisableVertexAttribArray(s_attrUV);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    gl.ClearColor(clear[0], clear[1], clear[2], clear[3]);
    gl.UseProgram(program);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.ActiveTexture(active);
    gl.BindBuffer(GL_ARRAY_BUFFER, array);
    for (int i = 0; i < 5; i++)
        if (was[i]) gl.Enable(caps[i]);
    yieldContext();
    return true;
}

void destroyWindow() {
    if (s_context) SDL_GL_DeleteContext(s_context);
    if (s_window) SDL_DestroyWindow(s_window);
    s_context = nullptr;
    s_window = nullptr;
    SDL_Quit();
}

void windowSize(int* w, int* h) {
    *w = kScreenWidth;
    *h = kScreenHeight;
    if (s_window) turn::size(s_window, w, h);
}

void warpPointer(int x, int y) {
    if (!s_window || turn::quarter(s_window)) return;
    int w = kScreenWidth, h = kScreenHeight;
    SDL_GetWindowSize(s_window, &w, &h);
    float scale, ox, oy;
    fit(w, h, &scale, &ox, &oy);
    SDL_WarpMouseInWindow(s_window, int(ox + (x + s_wideExtra + 0.5f) * scale), int(oy + (y + 0.5f) * scale));
}

void toggleFullscreen() {
    if (!s_window) return;
    bool full = SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN;
    WindowMode mode = full ? WindowMode::Window : s_fullKind;
    applyMode(mode);
    setWindowModeSetting(mode);
    settings::set("fullscreen", kModeNames[int(s_fullKind)]);
    if (!settings::save()) fprintf(stderr, "medecomp: не записать %s\n", settings::path().c_str());
    fprintf(stderr, "medecomp: окно: %s\n", kModeNames[int(mode)]);
}

void requestScreenshot(const std::string& path) {
    std::lock_guard<std::mutex> l(s_shotMutex);
    s_shotPath = path;
    s_shotAt = 0;
    s_shotFrame = false;
}

void requestFrameShot(const std::string& path) {
    std::lock_guard<std::mutex> l(s_shotMutex);
    s_shotPath = path;
    s_shotAt = 0;
    s_shotFrame = true;
}

void setWindowTitle(const std::string& title) {
    if (s_window) SDL_SetWindowTitle(s_window, title.c_str());
}

void startTextInput() { SDL_StartTextInput(); }
void stopTextInput() { SDL_StopTextInput(); }

void openURL(const char* url) {
    if (SDL_OpenURL(url) != 0) fprintf(stderr, "medecomp: не открыть %s: %s\n", url, SDL_GetError());
}

}
}

extern "C" int medecomp_anisotropy(void) {
    using namespace port::platform;
    static int level = -1;
    if (level < 0) {
        level = 0;
        const char* ext = reinterpret_cast<const char*>(gl.GetString(GL_EXTENSIONS));
        GLfloat max = 0;
        int want = originalGraphics() ? 0 : textureFilterSetting();
        if (want > 0 && ext && strstr(ext, "GL_EXT_texture_filter_anisotropic")) {
            gl.GetFloatv(0x84ff, &max);
            level = std::min(want, int(max));
        }
        fprintf(stderr, "medecomp: фильтрация текстур: %s\n",
                level > 1 ? (std::to_string(level) + "x").c_str() : "как на iPad");
    }
    return level > 1 ? level : 0;
}

extern "C" const char* medecomp_texture_bias(void) {
    using namespace port::platform;
    return textureFilterSetting() > 0 && !originalGraphics() ? "0.0" : "-2.0";
}

namespace port {
namespace platform {

void wakeMainLoop() {
    if (s_wakeEvent == (Uint32)-1) return;
    SDL_Event e;
    SDL_zero(e);
    e.type = s_wakeEvent;
    SDL_PushEvent(&e);
}

void presentTextField() {
    if (!textInputActive() || !s_window) return;
    acquireContext();
    present();
    releaseContext();
}

}
}

extern "C" void medecomp_on_yield() {
    if (port::platform::textInputActive()) port::platform::releaseContext();
}

extern "C" int medecomp_wide_extra(void) { return port::platform::wideExtra(); }

extern "C" float medecomp_render_scale(void) { return port::platform::renderScale(); }

#ifdef __ANDROID__
extern "C" JNIEXPORT void JNICALL Java_space_mooncellar_rooftops_GameActivity_nativeSurfaceDestroying(JNIEnv*, jclass) {
    port::platform::surfaceDestroying();
}
#endif

#include "overlay.h"

#include <SDL.h>
#include <cairo.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "gl_shared.h"
#include "imgui.h"
#include "launcher.h"
#include "levels.h"
#include "mcui.h"
#include "settings.h"

namespace port {
namespace platform {

namespace {

std::mutex s_iconsMutex;
std::map<std::string, std::unique_ptr<mcui::Icon>> s_icons;

ImGuiContext* s_ctx = nullptr;
bool s_failed = false;

constexpr double kSplashSeconds = 1.5, kSplashFade = 0.25;
std::atomic<int> s_splash(0);
double s_splashStart = 0;
GLuint s_splashLogo = 0;
int s_splashLogoW = 0, s_splashLogoH = 0;
float s_scale = 0;
GLuint s_program = 0, s_mark = 0;
GLint s_uniProj = -1, s_uniTex = -1, s_attrCol = -1;
Uint64 s_lastTicks = 0;

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = gl.CreateShader(type);
    gl.ShaderSource(s, 1, &src, nullptr);
    gl.CompileShader(s);
    GLint ok = 0;
    gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = {};
        gl.GetShaderInfoLog(s, sizeof log, nullptr, log);
        fprintf(stderr, "medecomp: вывод поверх игры: шейдер: %s\n", log);
    }
    return s;
}

bool createProgram() {
    const char* vs =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "attribute vec4 a_col;\n"
        "uniform vec4 u_proj;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_col;\n"
        "void main() { v_uv = a_uv; v_col = a_col; gl_Position = vec4(a_pos * u_proj.xy + u_proj.zw, 0.0, 1.0); }\n";
    const char* fs =
        "precision mediump float;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_col;\n"
        "uniform sampler2D u_tex;\n"
        "void main() { gl_FragColor = v_col * texture2D(u_tex, v_uv); }\n";
    s_program = gl.CreateProgram();
    gl.AttachShader(s_program, compileShader(GL_VERTEX_SHADER, vs));
    gl.AttachShader(s_program, compileShader(GL_FRAGMENT_SHADER, fs));
    s_attrCol = s_attrPos - 1;
    gl.BindAttribLocation(s_program, s_attrPos, "a_pos");
    gl.BindAttribLocation(s_program, s_attrUV, "a_uv");
    gl.BindAttribLocation(s_program, s_attrCol, "a_col");
    gl.LinkProgram(s_program);
    GLint ok = 0;
    gl.GetProgramiv(s_program, GL_LINK_STATUS, &ok);
    s_uniProj = gl.GetUniformLocation(s_program, "u_proj");
    s_uniTex = gl.GetUniformLocation(s_program, "u_tex");
    if (!ok) fprintf(stderr, "medecomp: вывод поверх игры: программа не собрана\n");
    return ok != 0;
}

GLuint uploadRgba(const unsigned char* rgba, int w, int h) {
    GLuint tex = 0;
    gl.GenTextures(1, &tex);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    return tex;
}

GLuint loadMark(const std::string& path) {
    cairo_surface_t* s = cairo_image_surface_create_from_png(path.c_str());
    if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(s);
        fprintf(stderr, "medecomp: вывод поверх игры: нет знака %s\n", path.c_str());
        return 0;
    }
    int w = cairo_image_surface_get_width(s), h = cairo_image_surface_get_height(s);
    int stride = cairo_image_surface_get_stride(s);
    const unsigned char* src = cairo_image_surface_get_data(s);
    std::vector<unsigned char> rgba(size_t(w) * h * 4);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint32_t p = *reinterpret_cast<const uint32_t*>(src + y * stride + x * 4);
            unsigned a = p >> 24, r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;
            unsigned char* d = &rgba[(size_t(y) * w + x) * 4];
            d[0] = a ? (unsigned char)std::min(255u, r * 255 / a) : 0;
            d[1] = a ? (unsigned char)std::min(255u, g * 255 / a) : 0;
            d[2] = a ? (unsigned char)std::min(255u, b * 255 / a) : 0;
            d[3] = (unsigned char)a;
        }
    cairo_surface_destroy(s);
    return uploadRgba(rgba.data(), w, h);
}

bool init() {
    if (!createProgram()) return false;
    ImGuiContext* prev = ImGui::GetCurrentContext();
    s_ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(s_ctx);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendRendererName = "medecomp_overlay_gles";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    const std::string dir = brandingDir();
    mcui::FontFiles files;
    files.regular = dir + "fonts/ApercuPro-Regular.ttf";
    files.medium = dir + "fonts/ApercuPro-Medium.ttf";
    files.bold = dir + "fonts/ApercuPro-Bold.ttf";
    std::string system = launcherFont();
    if (!system.empty()) files.fallback.push_back({system, 0});
    mcui::LoadFonts(files);
    s_mark = loadMark(dir + "logo-icon.png");
    if (s_mark) mcui::SetNotificationMark((ImTextureID)(intptr_t)s_mark);
    port::gfx::Image logo;
    if (loadBrandingImage("logo-full.png", &logo)) {
        s_splashLogo = uploadRgba(logo.rgba.data(), logo.width, logo.height);
        s_splashLogoW = logo.width;
        s_splashLogoH = logo.height;
    }
    ImGui::SetCurrentContext(prev);
    return true;
}

void updateTexture(ImTextureData* tex) {
    if (tex->Status == ImTextureStatus_WantCreate) {
        GLuint id = uploadRgba(static_cast<const unsigned char*>(tex->GetPixels()), tex->Width, tex->Height);
        tex->SetTexID((ImTextureID)(intptr_t)id);
        tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantUpdates) {
        gl.BindTexture(GL_TEXTURE_2D, GLuint(intptr_t(tex->GetTexID())));
        gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
        std::vector<unsigned char> rows;
        for (const ImTextureRect& r : tex->Updates) {
            rows.resize(size_t(r.w) * r.h * 4);
            for (int y = 0; y < r.h; y++)
                memcpy(&rows[size_t(y) * r.w * 4], tex->GetPixelsAt(r.x, r.y + y), size_t(r.w) * 4);
            gl.TexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.w, r.h, GL_RGBA, GL_UNSIGNED_BYTE, rows.data());
        }
        tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0) {
        GLuint id = GLuint(intptr_t(tex->GetTexID()));
        gl.DeleteTextures(1, &id);
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

void render(ImDrawData* dd, int w, int h) {
    if (dd->Textures)
        for (ImTextureData* tex : *dd->Textures)
            if (tex->Status != ImTextureStatus_OK) updateTexture(tex);
    gl.Enable(GL_BLEND);
    gl.BlendEquation(GL_FUNC_ADD);
    gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl.Enable(GL_SCISSOR_TEST);
    gl.Viewport(0, 0, w, h);
    gl.UseProgram(s_program);
    const float l = dd->DisplayPos.x, r = l + dd->DisplaySize.x, t = dd->DisplayPos.y, b = t + dd->DisplaySize.y;
    gl.Uniform4f(s_uniProj, 2.0f / (r - l), -2.0f / (b - t), -(r + l) / (r - l), (b + t) / (b - t));
    gl.Uniform1i(s_uniTex, 0);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl.EnableVertexAttribArray(s_attrPos);
    gl.EnableVertexAttribArray(s_attrUV);
    gl.EnableVertexAttribArray(s_attrCol);
    const GLenum indexType = sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    for (const ImDrawList* list : dd->CmdLists) {
        const ImDrawVert* vtx = list->VtxBuffer.Data;
        const ImDrawIdx* idx = list->IdxBuffer.Data;
        gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), &vtx->pos);
        gl.VertexAttribPointer(s_attrUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), &vtx->uv);
        gl.VertexAttribPointer(s_attrCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), &vtx->col);
        for (const ImDrawCmd& cmd : list->CmdBuffer) {
            if (cmd.UserCallback) continue;
            const float x0 = cmd.ClipRect.x - l, y0 = cmd.ClipRect.y - t;
            const float x1 = cmd.ClipRect.z - l, y1 = cmd.ClipRect.w - t;
            if (x1 <= x0 || y1 <= y0) continue;
            gl.Scissor(GLint(x0), GLint(h - y1), GLsizei(x1 - x0), GLsizei(y1 - y0));
            gl.BindTexture(GL_TEXTURE_2D, GLuint(intptr_t(cmd.GetTexID())));
            gl.DrawElements(GL_TRIANGLES, GLsizei(cmd.ElemCount), indexType, idx + cmd.IdxOffset);
        }
    }
    gl.DisableVertexAttribArray(s_attrCol);
    gl.Disable(GL_SCISSOR_TEST);
    gl.Disable(GL_BLEND);
}

}

bool helperRun() {
    return getenv("MEDECOMP_SKIN_PREVIEWS") || getenv("MEDECOMP_PIECE_PREVIEWS") || getenv("MEDECOMP_MAP_PREVIEW");
}

void notify(const std::string& title, const std::string& text, const std::string& iconSvg, float seconds) {
    if (helperRun() || testRun()) return;
    mcui::Notification n;
    n.title = title;
    n.text = text;
    n.seconds = seconds;
    if (!iconSvg.empty()) {
        std::lock_guard<std::mutex> lock(s_iconsMutex);
        std::unique_ptr<mcui::Icon>& icon = s_icons[iconSvg];
        if (!icon) icon.reset(new mcui::Icon(mcui::ParseIcon(iconSvg)));
        n.icon = icon.get();
    }
    mcui::Notify(n);
}

static void drawSplash(double now, float w, float h, float scale) {
    int waiting = 1;
    if (s_splash.compare_exchange_strong(waiting, 2)) s_splashStart = now;
    const double t = now - s_splashStart;
    if (t >= kSplashSeconds) {
        int shown = 2;
        s_splash.compare_exchange_strong(shown, 0);
    }
    const float alpha = float(std::min(std::max(std::min(t, kSplashSeconds - t) / kSplashFade, 0.0), 1.0));
    ImDrawList* list = ImGui::GetBackgroundDrawList();
    list->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), mcui::Packed(mcui::tokens::kColorBgPrimary));
    const char* caption = "MoonCellar Team";
    ImFont* font = mcui::GetFonts().medium;
    const float captionSize = mcui::kTextTitle * scale;
    const ImVec2 captionExtent = font->CalcTextSizeA(captionSize, FLT_MAX, 0.0f, caption);
    float logoH = s_splashLogo ? h * 0.25f : 0.0f;
    float logoW = s_splashLogo ? logoH * float(s_splashLogoW) / float(s_splashLogoH) : 0.0f;
    if (logoW > w * 0.5f) {
        logoH *= w * 0.5f / logoW;
        logoW = w * 0.5f;
    }
    const float gap = logoH * 0.2f;
    float y = (h - (logoH + gap + captionExtent.y)) / 2;
    if (s_splashLogo) {
        ImVec2 a((w - logoW) / 2, y);
        list->AddImage(ImTextureRef((ImTextureID)(intptr_t)s_splashLogo), a, ImVec2(a.x + logoW, a.y + logoH),
                       ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, int(alpha * 255)));
        y += logoH + gap;
    }
    list->AddText(font, captionSize, ImVec2((w - captionExtent.x) / 2, y),
                  mcui::Packed(mcui::tokens::kColorTextMuted, alpha), caption);
}

bool coverActive() {
#ifdef __ANDROID__
    return helperRun();
#else
    return helperRun() && getenv("MEDECOMP_PREVIEW_COVER");
#endif
}

extern "C" void medecomp_skin_preview_progress(int* done, int* total);
extern "C" void medecomp_level_preview_progress(int* done, int* total);

static void drawCover(float w, float h, float scale) {
    ImDrawList* list = ImGui::GetBackgroundDrawList();
    list->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), mcui::Packed(mcui::tokens::kColorBgPrimary));
    const bool ru = launcherRussian();
    int done = 0, total = 0;
    std::string caption;
    if (getenv("MEDECOMP_SKIN_PREVIEWS")) {
        medecomp_skin_preview_progress(&done, &total);
        caption = ru ? "Готовлю превью скинов" : "Preparing skin previews";
    } else if (getenv("MEDECOMP_PIECE_PREVIEWS")) {
        medecomp_level_preview_progress(&done, &total);
        caption = ru ? "Готовлю картинки кусков" : "Preparing piece previews";
    } else {
        caption = ru ? "Рисую область уровня" : "Drawing the level area";
    }
    caption += total > 0 ? " — " + std::to_string(done) + " / " + std::to_string(total) : std::string("…");
    ImFont* font = mcui::GetFonts().medium;
    const float captionSize = mcui::kTextTitle * scale;
    const ImVec2 extent = font->CalcTextSizeA(captionSize, FLT_MAX, 0.0f, caption.c_str());
    float logoH = s_splashLogo ? h * 0.2f : 0.0f;
    float logoW = s_splashLogo ? logoH * float(s_splashLogoW) / float(s_splashLogoH) : 0.0f;
    const float gap = logoH * 0.25f;
    float y = (h - (logoH + gap + extent.y)) / 2;
    if (s_splashLogo) {
        ImVec2 a((w - logoW) / 2, y);
        list->AddImage(ImTextureRef((ImTextureID)(intptr_t)s_splashLogo), a, ImVec2(a.x + logoW, a.y + logoH));
        y += logoH + gap;
    }
    list->AddText(font, captionSize, ImVec2((w - extent.x) / 2, y), mcui::Packed(mcui::tokens::kColorTextMuted), caption.c_str());
}

template <class F>
static void overlayFrame(int w, int h, F&& body) {
    if (!s_ctx && !init()) {
        s_failed = true;
        s_splash = 0;
        return;
    }
    ImGuiContext* prev = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(s_ctx);
    float scale = std::min(std::max(float(h) / 720.0f, 1.0f), 3.0f);
    if (std::fabs(scale - s_scale) > 0.05f) {
        mcui::ApplyStyle(scale);
        s_scale = scale;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(w), float(h));
    io.DisplayFramebufferScale = ImVec2(1, 1);
    Uint64 now = SDL_GetPerformanceCounter(), freq = SDL_GetPerformanceFrequency();
    io.DeltaTime = s_lastTicks ? std::min(std::max(float(double(now - s_lastTicks) / double(freq)), 1e-4f), 0.1f)
                               : 1.0f / 60.0f;
    s_lastTicks = now;
    ImGui::NewFrame();
    body(double(now) / double(freq), scale);
    ImGui::Render();
    GLint blendEq = 0, srcRgb = 0, dstRgb = 0, srcAlpha = 0, dstAlpha = 0, unpack = 4, elements = 0, scissor[4] = {};
    gl.GetIntegerv(GL_BLEND_EQUATION_RGB, &blendEq);
    gl.GetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    gl.GetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    gl.GetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    gl.GetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    gl.GetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
    gl.GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elements);
    gl.GetIntegerv(GL_SCISSOR_BOX, scissor);
    render(ImGui::GetDrawData(), w, h);
    gl.BlendEquation(GLenum(blendEq));
    gl.BlendFuncSeparate(GLenum(srcRgb), GLenum(dstRgb), GLenum(srcAlpha), GLenum(dstAlpha));
    gl.PixelStorei(GL_UNPACK_ALIGNMENT, unpack);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLuint(elements));
    gl.Scissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    ImGui::SetCurrentContext(prev);
}

void overlayDraw(int w, int h) {
    const bool splash = s_splash.load() != 0;
    if (s_failed || w < 1 || h < 1 || (!splash && !mcui::NotificationsActive())) return;
    overlayFrame(w, h, [&](double now, float scale) {
        if (splash)
            drawSplash(now, float(w), float(h), scale);
        else
            mcui::DrawNotifications(now);
    });
}

void overlayCover(int w, int h) {
    if (s_failed || w < 1 || h < 1 || !coverActive()) return;
    overlayFrame(w, h, [&](double, float scale) { drawCover(float(w), float(h), scale); });
}

bool splashSetting() { return settings::get("splash", "on") != "off"; }

void setSplashSetting(bool on) { settings::set("splash", on ? "on" : "off"); }

void splashBegin() {
    const char* force = getenv("MEDECOMP_SPLASH");
    const bool scripted = getenv("MEDECOMP_SCRIPT") || getenv("MEDECOMP_SHOT") || helperRun() || testRun();
    if (force ? strcmp(force, "0") != 0 : splashSetting() && !scripted) s_splash = 1;
}

bool splashActive() { return s_splash.load() != 0; }

bool splashSkip() { return s_splash.exchange(0) != 0; }

}
}

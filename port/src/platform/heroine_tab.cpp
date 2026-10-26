#include "heroine_tab.h"

#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../ea/cloud.h"
#include "../gfx/png.h"
#include "file_dialog.h"
#include "gltf.h"
#include "autorig.h"
#include "heroine_model.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "launcher_help.h"
#include "mcui.h"
#include "port/files.h"
#include "port/platform.h"
#include "process.h"
#include "skins.h"

namespace skins = port::platform::skins;
namespace process = port::platform::process;

namespace port {
namespace platform {
namespace heroine {

namespace {

bool s_russian;
const char* L(const char* ru, const char* en) { return s_russian ? ru : en; }

std::string s_game;
long s_child = 0;
std::string s_export;
bool s_offscreenFailed = false;
std::string s_pendingSelect;
std::string s_pendingFolder;
std::string s_error;
std::string s_note;
std::string s_cloudNote;
std::vector<skins::Choice> s_choices;
std::map<std::string, port::online::cloud::SkinPlace> s_places;
bool s_placesDirty = true, s_placesActive = false;
Uint32 s_placesAt = 0;
std::map<std::string, SDL_Texture*> s_textures;
std::map<std::string, ImVec2> s_sizes;
bool s_reload = true;
bool s_tabShown = false;
#ifdef MEDECOMP_ONE_PROCESS
bool s_stale = false;
std::vector<std::string> s_previewEnv;
bool s_previewPending = false;
#endif
#ifdef __ANDROID__

std::string externalDir() {
    const char* path = SDL_AndroidGetExternalStoragePath();
    return path ? path : std::string();
}
#endif

std::string cacheDir() { return port::files::dataDir() + "/cache/skins"; }

void makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++)
        if (i == path.size() || path[i] == '/') {
#ifdef _WIN32
            mkdir(path.substr(0, i).c_str());
#else
            mkdir(path.substr(0, i).c_str(), 0755);
#endif
        }
}

long mtime(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 ? long(st.st_mtime) : 0;
}

std::string readFile(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

void start(const std::string& exportSpec) {
    if (s_child || s_game.empty()) return;
#ifdef MEDECOMP_ONE_PROCESS
    makeDirs(cacheDir());
    remove((cacheDir() + "/done").c_str());
    s_previewEnv = {"MEDECOMP_SKIN_PREVIEWS=" + cacheDir(), "MEDECOMP_SKIN_TEMPLATE=" + cacheDir() + "/faith-template.glb",
                    "ALSOFT_DRIVERS=null"};
    if (!exportSpec.empty()) s_previewEnv.push_back("MEDECOMP_SKIN_EXPORT=" + exportSpec);
    s_previewPending = true;
    return;
#endif
    std::string self = process::selfPath();
    if (self.empty()) {
        s_error = L("не найти свой исполняемый файл", "cannot find own executable");
        return;
    }
    makeDirs(cacheDir());
    remove((cacheDir() + "/done").c_str());
    std::vector<std::string> env = {"MEDECOMP_SKIN_PREVIEWS=" + cacheDir(), "MEDECOMP_WINDOW_SIZE=1024x768",
                                    "MEDECOMP_SKIN_TEMPLATE=" + cacheDir() + "/faith-template.glb",
                                    "ALSOFT_DRIVERS=null"};
    if (!s_offscreenFailed) env.push_back("SDL_VIDEODRIVER=offscreen");
    if (!exportSpec.empty()) env.push_back("MEDECOMP_SKIN_EXPORT=" + exportSpec);
    s_export = exportSpec;
    s_child = process::spawn({self, s_game}, env);
    if (!s_child) s_error = L("не запустить игру для превью", "cannot start the game for previews");
}

void loadTextures(SDL_Renderer* renderer) {
    for (auto& t : s_textures) SDL_DestroyTexture(t.second);
    s_textures.clear();
    s_sizes.clear();
    for (const skins::Choice& c : s_choices) {
        std::string bytes = readFile(cacheDir() + "/" + skins::previewFile(c));
        port::gfx::Image image;
        if (bytes.empty() ||
            !port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), image, false))
            continue;
        SDL_Texture* tex =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
        if (!tex) continue;
        SDL_UpdateTexture(tex, nullptr, image.rgba.data(), image.width * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
        s_textures[c.key] = tex;
        s_sizes[c.key] = ImVec2(float(image.width), float(image.height));
    }
}

std::string baseOfSelected() {
    std::string base = skins::selected();
    for (const skins::Choice& c : s_choices)
        if (c.key == base && c.custom) {
            std::string model = readFile(skins::folder() + "/" + c.key + "/model");
            while (!model.empty() && (model.back() == '\n' || model.back() == '\r' || model.back() == ' '))
                model.pop_back();
            base = model;
        }
    if (base.empty() || base.compare(0, 5, "game:") != 0) base = "game:faith-day";
    return base;
}

std::string newSkin(const std::string& base, const std::string& name, std::string* id) {
    namespace cloud = port::online::cloud;
    *id = cloud::newSkinId();
    std::string dir = skins::folder() + "/" + *id;
    makeDirs(dir);
    std::string label = name;
    if (label.empty()) {
        int n = 1;
        for (const skins::Choice& c : s_choices) {
            if (c.key == base) label = s_russian ? c.ru : c.en;
            n += c.custom;
        }
        label += std::string(" — ") + L("свой", "custom") + " " + std::to_string(n);
    }
    std::ofstream((dir + "/name").c_str()) << label << "\n";
    std::ofstream((dir + "/model").c_str()) << base << "\n";
    s_cloudNote.clear();
    if (cloud::placeNewSkin(*id))
        s_cloudNote = std::string(L("В облаке уже ", "The cloud already holds ")) + std::to_string(cloud::kSkinLimit) +
                      L(" скинов — новый только здесь. «В облако» — когда освободится место («Убрать из облака» у другого).",
                        " skins — the new one stays here only. Send to cloud once there is room (Remove from cloud on another one).");
    s_placesDirty = true;
    return dir;
}

void createSkin() {
    std::string base = baseOfSelected(), id;
    std::string dir = newSkin(base, std::string(), &id);
    skins::rescan();
    s_choices = skins::list();
    s_pendingSelect = id;
    s_pendingFolder = dir;
#ifdef MEDECOMP_ONE_PROCESS
    skins::select(id);
#endif
    start(base + "|" + dir + "/texture.png");
}

void importTexture(const std::string& path) {
    std::string bytes = readFile(path);
    port::gfx::Image image;
    if (bytes.empty() ||
        !port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), image, false)) {
        s_error = std::string(L("не PNG или не прочитать: ", "not a PNG or unreadable: ")) + path;
        return;
    }
    std::string chosen = skins::selected(), dir;
    for (const skins::Choice& c : s_choices)
        if (c.key == chosen && c.custom) dir = skins::folder() + "/" + c.key;
    if (dir.empty()) {
        std::string stem = path.substr(path.find_last_of("/\\") + 1);
        stem = stem.substr(0, stem.find_last_of('.'));
        dir = newSkin(baseOfSelected(), stem, &chosen);
    }
    std::ofstream out((dir + "/texture.png").c_str(), std::ios::binary);
    out.write(bytes.data(), std::streamsize(bytes.size()));
    if (!out) {
        s_error = std::string(L("не записать ", "cannot write ")) + dir + "/texture.png";
        return;
    }
    out.close();
    s_error.clear();
    s_note = image.width == 512 && image.height == 512
                 ? std::string()
                 : std::string(L("Картинка ", "The image is ")) + std::to_string(image.width) + "×" +
                       std::to_string(image.height) +
                       L(": у модели — 512×512. Ляжет, если раскладка та же, что у оригинала (растягивается).",
                         ": the model's is 512×512. It fits if the layout is the original's (it gets scaled).");
    skins::rescan();
    s_choices = skins::list();
    skins::select(chosen);
    s_reload = true;
    port::online::cloud::syncSkins(s_russian);
}

const mcui::Icon& placeIcon(port::online::cloud::SkinPlace place) {
    using port::online::cloud::SkinPlace;
    static const mcui::Icon saved = mcui::ParseIcon(R"(<path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>)");
    static const mcui::Icon waiting = mcui::ParseIcon(
        R"(<path d="M12 13v8"/><path d="M4 14.899A7 7 0 1 1 15.71 8h1.79a4.5 4.5 0 0 1 2.5 8.242"/><path d="m8 17 4-4 4 4"/>)");
    static const mcui::Icon here = mcui::ParseIcon(
        R"(<path d="m2 2 20 20"/><path d="M5.782 5.782A7 7 0 0 0 9 19h8.5a4.5 4.5 0 0 0 1.307-.193"/>)"
        R"(<path d="M21.532 16.5A4.5 4.5 0 0 0 17.5 10h-1.79A7.008 7.008 0 0 0 10 5.07"/>)");
    return place == SkinPlace::Saved ? saved : place == SkinPlace::Here ? here : waiting;
}

unsigned placeColor(port::online::cloud::SkinPlace place) {
    using port::online::cloud::SkinPlace;
    return place == SkinPlace::Saved ? mcui::tokens::kColorPositive
           : place == SkinPlace::Here ? mcui::tokens::kColorTextMuted
                                      : mcui::tokens::kColorAttention;
}

const char* placeWords(port::online::cloud::SkinPlace place) {
    using port::online::cloud::SkinPlace;
    switch (place) {
    case SkinPlace::Saved: return L("в облаке", "in the cloud");
    case SkinPlace::Changed: return L("в облаке, здесь новее — уйдёт при сверке", "in the cloud, newer here — goes up on sync");
    case SkinPlace::New: return L("уйдёт в облако при сверке", "goes to the cloud on sync");
    case SkinPlace::Here: return L("только здесь, не в облаке", "here only, not in the cloud");
    default: return "";
    }
}

void refreshPlaces() {
    namespace cloud = port::online::cloud;
    bool active = cloud::skinsActive();
    if (!s_placesDirty && active == s_placesActive && SDL_GetTicks() - s_placesAt < 2000) return;
    s_placesDirty = false;
    s_placesActive = active;
    s_placesAt = SDL_GetTicks();
    s_places.clear();
    for (const skins::Choice& c : s_choices)
        if (c.custom) s_places[c.key] = cloud::skinPlace(c.key);
}

port::online::cloud::SkinPlace placeOf(const std::string& key) {
    auto it = s_places.find(key);
    return it == s_places.end() ? port::online::cloud::SkinPlace::Off : it->second;
}

}

void update(SDL_Renderer* renderer, const std::string& game) {
    if (game != s_game) {
        s_game = game;
        s_reload = true;
    }
    if (s_child) {
        int code = process::poll(s_child);
        if (code < 0) return;
        s_child = 0;
        if (code != 0 && !s_offscreenFailed) {
            s_offscreenFailed = true;
            start(s_export);
            return;
        }
        if (code == 0) {
            if (!s_pendingSelect.empty()) {
                skins::rescan();
                s_choices = skins::list();
                skins::select(s_pendingSelect);
                port::platform::openURL(("file://" + s_pendingFolder).c_str());
                s_pendingSelect.clear();
                s_pendingFolder.clear();
                s_placesDirty = true;
                port::online::cloud::syncSkins(s_russian);
            }
        } else {
            s_error = L("превью не вышли — журнал игры в папке игрока", "previews failed — see the game log");
        }
        loadTextures(renderer);
        return;
    }
    if (!s_reload) return;
    s_reload = false;
    s_choices = skins::list();
    s_placesDirty = true;
    if (s_game.empty()) return;
    static bool imported = false;
    if (const char* file = getenv("MEDECOMP_HEROINE_IMPORT"); file && *file && !imported && s_tabShown) {
        imported = true;
        std::string f = file;
        picked(f.size() > 4 && f.compare(f.size() - 4, 4, ".png") == 0, f);
        if (s_reload) return update(renderer, game);
    }
    bool fresh = readFile(cacheDir() + "/stamp") == skins::previewStamp(port::files::gameSource()) &&
                 mtime(cacheDir() + "/done") != 0;
    loadTextures(renderer);
#ifdef MEDECOMP_ONE_PROCESS
    s_stale = !fresh;
#else
    if (!fresh) start(std::string());
#endif
}

void exportTemplate();
void deleteSkin(const std::string& key);

namespace {

struct Icons {
    mcui::Icon refresh, folder, info, plus, image, upload, model, download;
};

const Icons& icons() {
    static const Icons i = {
        mcui::ParseIcon(R"(<path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/>)"
                        R"(<path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/>)"),
        mcui::ParseIcon(R"(<path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/>)"),
        mcui::ParseIcon(R"(<path d="M12 2a10 10 0 1 0 0 20a10 10 0 1 0 0-20Z"/><path d="M12 16v-4"/><path d="M12 8h.01"/>)"),
        mcui::ParseIcon(R"(<path d="M5 12h14"/><path d="M12 5v14"/>)"),
        mcui::ParseIcon(R"(<path d="M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2Z"/>)"
                        R"(<path d="M9 7a2 2 0 1 0 0 4a2 2 0 1 0 0-4Z"/><path d="m21 15-3.086-3.086a2 2 0 0 0-2.828 0L6 21"/>)"),
        mcui::ParseIcon(R"(<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><path d="m17 8-5-5-5 5"/><path d="M12 3v12"/>)"),
        mcui::ParseIcon(R"(<path d="M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16Z"/>)"
                        R"(<path d="m3.3 7 8.7 5 8.7-5"/><path d="M12 22V12"/>)"),
        mcui::ParseIcon(R"(<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><path d="m7 10 5 5 5-5"/><path d="M12 15V3"/>)"),
    };
    return i;
}

ImU32 tokenColor(unsigned token, float alpha = 1.0f) { return ImGui::GetColorU32(mcui::Color(token, alpha)); }

bool focusButton(const char* id, ImVec2 size, float radius) {
    float thickness = float(int(std::max(2.0f, 1.5f * ImGui::GetScale())));
    float distance = float(int(3.0f + thickness * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, radius + distance);
    bool pressed = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_EnableNav);
    ImGui::PopStyleVar();
    return pressed;
}

bool iconButton(const char* id, const mcui::Icon& icon, const char* tip, float size, bool active = false) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = focusButton(id, ImVec2(size, size), size / 2);
    bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 c(p.x + size / 2, p.y + size / 2);
    draw->AddCircleFilled(c, size / 2, tokenColor(hovered || active ? mcui::tokens::kColorBgHover : mcui::tokens::kColorBgTertiary));
    float s = size * 0.5f;
    mcui::DrawIcon(draw, icon, ImVec2(c.x - s / 2, c.y - s / 2), s, tokenColor(mcui::tokens::kColorTextPrimary));
    if (hovered) ImGui::SetTooltip("%s", tip);
    return pressed;
}

int columns(float avail, float minW, float gap, float* w, int count = 0) {
    int n = std::max(1, int((avail + gap) / (minW + gap)));
    if (count > 0) {
        int rows = (count + n - 1) / n;
        n = (count + rows - 1) / rows;
    }
    *w = (avail - gap * float(n - 1)) / float(n);
    return n;
}

float labelHeight(const std::vector<const skins::Choice*>& choices, float width, float textSize, float scale, int maxLines) {
    ImFont* font = ImGui::GetFont();
    float line = font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, "Ag").y, h = line;
    for (const skins::Choice* c : choices)
        h = std::max(h, font->CalcTextSizeA(textSize, FLT_MAX, width - 8.0f * scale, (s_russian ? c->ru : c->en).c_str()).y);
    return std::min(h, line * float(maxLines));
}

bool card(const skins::Choice& c, ImVec2 p, ImVec2 size, float imageH, float textSize, bool selected, float scale) {
    namespace cloud = port::online::cloud;
    ImGui::SetCursorScreenPos(p);
    ImGui::PushID(c.key.c_str());
    const float pad = 8.0f * scale, radius = 8.0f * scale;
    bool pressed = focusButton("##card", size, radius);
    bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + size.x, p.y + size.y);
    draw->AddRectFilled(p, q, ImGui::GetColorU32(selected ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_FrameBg), radius);
    if (selected) draw->AddRect(p, q, tokenColor(mcui::tokens::kColorAccent), radius, 0, 2.0f * scale);
    auto tex = s_textures.find(c.key);
    if (tex != s_textures.end()) {
        ImVec2 img = s_sizes[c.key];
        float k = std::min((size.x - 2 * pad) / img.x, (imageH - 2 * pad) / img.y);
        ImVec2 a(p.x + (size.x - img.x * k) / 2, p.y + pad + (imageH - 2 * pad - img.y * k));
        draw->AddImage(ImTextureRef((ImTextureID)(intptr_t)tex->second), a, ImVec2(a.x + img.x * k, a.y + img.y * k));
    }
    const std::string& name = s_russian ? c.ru : c.en;
    float wrap = size.x - pad;
    ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(textSize, FLT_MAX, wrap, name.c_str());
    draw->PushClipRect(ImVec2(p.x + pad / 2, p.y + imageH), ImVec2(q.x - pad / 2, q.y - pad / 2), true);
    draw->AddText(ImGui::GetFont(), textSize, ImVec2(p.x + (size.x - ts.x) / 2, p.y + imageH + pad / 2),
                  ImGui::GetColorU32(ImGuiCol_Text), name.c_str(), nullptr, wrap);
    draw->PopClipRect();
    cloud::SkinPlace place = c.custom ? placeOf(c.key) : cloud::SkinPlace::Off;
    if (place != cloud::SkinPlace::Off) {
        float s = 16.0f * scale, r = s * 0.8f;
        ImVec2 center(q.x - pad - r, p.y + pad + r);
        draw->AddCircleFilled(center, r, tokenColor(mcui::tokens::kColorBgPrimaryTint));
        mcui::DrawIcon(draw, placeIcon(place), ImVec2(center.x - s / 2, center.y - s / 2), s, tokenColor(placeColor(place)));
        if (hovered) ImGui::SetTooltip("%s", placeWords(place));
    }
    ImGui::PopID();
    return pressed;
}

void dashedRect(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float radius, float thickness, float dash) {
    auto line = [&](ImVec2 from, ImVec2 to) {
        float dx = to.x - from.x, dy = to.y - from.y, len = sqrtf(dx * dx + dy * dy);
        if (len <= 0) return;
        dx /= len, dy /= len;
        for (float t = 0; t < len; t += dash * 1.8f) {
            float e = std::min(t + dash, len);
            draw->AddLine(ImVec2(from.x + dx * t, from.y + dy * t), ImVec2(from.x + dx * e, from.y + dy * e), color, thickness);
        }
    };
    line(ImVec2(a.x + radius, a.y), ImVec2(b.x - radius, a.y));
    line(ImVec2(b.x, a.y + radius), ImVec2(b.x, b.y - radius));
    line(ImVec2(b.x - radius, b.y), ImVec2(a.x + radius, b.y));
    line(ImVec2(a.x, b.y - radius), ImVec2(a.x, a.y + radius));
    const ImVec2 corners[] = {{a.x + radius, a.y + radius}, {b.x - radius, a.y + radius}, {b.x - radius, b.y - radius}, {a.x + radius, b.y - radius}};
    for (int i = 0; i < 4; i++) {
        float from = 3.14159265f * (1.0f + 0.5f * float(i));
        draw->PathArcTo(corners[i], radius, from, from + 3.14159265f / 2, 6);
        draw->PathStroke(color, 0, thickness);
    }
}

bool newTile(ImVec2 p, ImVec2 size, float scale) {
    ImGui::SetCursorScreenPos(p);
    bool pressed = focusButton("##new-skin", size, 8.0f * scale);
    bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + size.x, p.y + size.y);
    if (hovered) draw->AddRectFilled(p, q, ImGui::GetColorU32(ImGuiCol_HeaderHovered), 8.0f * scale);
    dashedRect(draw, p, q, tokenColor(hovered ? mcui::tokens::kColorAccent : mcui::tokens::kColorGray, hovered ? 1.0f : 0.45f),
               8.0f * scale, 1.5f * scale, 6.0f * scale);
    const char* label = L("Новый скин", "New skin");
    float r = 20.0f * scale, textH = ImGui::GetTextLineHeight(), gap = 10.0f * scale;
    float top = p.y + (size.y - (2 * r + gap + textH)) / 2;
    ImVec2 c(p.x + size.x / 2, top + r);
    draw->AddCircleFilled(c, r, tokenColor(mcui::tokens::kColorBgTertiary));
    float s = 20.0f * scale;
    mcui::DrawIcon(draw, icons().plus, ImVec2(c.x - s / 2, c.y - s / 2), s, tokenColor(mcui::tokens::kColorAccent));
    ImVec2 ts = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(p.x + (size.x - ts.x) / 2, top + 2 * r + gap), ImGui::GetColorU32(ImGuiCol_Text), label);
    return pressed;
}

bool menuItem(const char* id, const mcui::Icon& icon, const char* title, const char* caption, float width, float scale,
              bool enabled = true) {
    ImGui::PushID(id);
    const float size = std::max(28.0f, ImGui::GetFrameHeight());
    const float pad = 8.0f * scale, iconSize = 20.0f * scale, textX = 2 * pad + iconSize;
    const float rowWidth = std::min(width, ImGui::GetContentRegionAvail().x) - size - ImGui::GetStyle().ItemSpacing.x;
    const float wrap = std::max(1.0f, rowWidth - textX - pad);
    const float height = std::max(size, ImGui::CalcTextSize(title, nullptr, false, wrap).y + pad * 2);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::Selectable(id, false, enabled ? 0 : ImGuiSelectableFlags_Disabled, ImVec2(rowWidth, height));
    ImDrawList* draw = ImGui::GetWindowDrawList();
    mcui::DrawIcon(draw, icon, ImVec2(p.x + pad, p.y + pad), iconSize, tokenColor(mcui::tokens::kColorAccent));
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p.x + textX, p.y + pad),
                  ImGui::GetColorU32(ImGuiCol_Text), title, nullptr, wrap);
    ImGui::SameLine();
    launcherHelp(caption, false);
    ImGui::PopID();
    return pressed;
}

ImVec2 sectionRow(const char* title, float height, float scale) {
    ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::PushFont(mcui::GetFonts().medium, mcui::kTextBody + 1.0f);
    float h = ImGui::GetTextLineHeight(), bar = 4.0f * scale, top = start.y + (height - h) / 2;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(start.x, top), ImVec2(start.x + bar, top + h), tokenColor(mcui::tokens::kColorAccent),
                                              mcui::tokens::kRadiusX2 * scale);
    ImGui::SetCursorScreenPos(ImVec2(start.x + bar + mcui::tokens::kGapX2 * scale, top));
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    return start;
}

void colored(unsigned token, const std::string& text) {
    ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(token));
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
}

void infoPopup(float scale) {
    namespace cloud = port::online::cloud;
    ImGui::PushFont(mcui::GetFonts().bold, mcui::kTextBody + 1.0f);
    ImGui::TextUnformatted(L("Скины героини", "Heroine skins"));
    ImGui::PopFont();
    ImGui::TextWrapped("%s", L("Модель героини в забеге. Соперники, полиция и призрак — как раньше.",
                               "The heroine's model in runs. Rivals, police and the ghost stay as they are."));
    ImGui::TextWrapped("%s", L("Свой скин — модель игры со своей текстурой (texture.png: «Из текстуры модели» или «Импорт "
                               "текстуры…») или своя модель glTF или VRM («Импорт модели…»): со своим скелетом (Mixamo, VRM, "
                               "Unreal, Blender…) или без него она привязывается к скелету Фейт сама; точнее всего — привязать "
                               "в Blender к «Шаблону для Blender». Всё это — в меню «Новый скин».",
                               "A custom skin is a game model with your texture (texture.png: From the model's texture or "
                               "Import texture…) or your own glTF or VRM model (Import model…): with its own skeleton (Mixamo, "
                               "VRM, Unreal, Blender…) or without one it is bound to Faith's skeleton automatically; the most "
                               "precise way is to bind it in Blender to the Template for Blender. All of these are in the New "
                               "skin menu."));
    if (cloud::skinsActive()) {
        ImGui::TextWrapped("%s %d %s", L("В облаке MoonCellar — до", "The MoonCellar cloud keeps up to"), cloud::kSkinLimit,
                           L("своих скинов, остальные только на этом компьютере.", "custom skins, the rest stay on this computer only."));
        float s = ImGui::GetTextLineHeight();
        for (cloud::SkinPlace place : {cloud::SkinPlace::Saved, cloud::SkinPlace::New, cloud::SkinPlace::Here}) {
            mcui::DrawIcon(ImGui::GetWindowDrawList(), placeIcon(place), ImGui::GetCursorScreenPos(), s, tokenColor(placeColor(place)));
            ImGui::Dummy(ImVec2(s, s));
            ImGui::SameLine();
            ImGui::TextUnformatted(placeWords(place));
        }
    } else {
        ImGui::TextWrapped("%s %d %s", L("Войдите в аккаунт MoonCellar (вкладка «Аккаунт») — до", "Sign in to MoonCellar (Account tab) and up to"),
                           cloud::kSkinLimit, L("своих скинов будут и в облаке.", "custom skins are kept in the cloud too."));
    }
}

void selectionBar(float scale, float height) {
    namespace cloud = port::online::cloud;
    std::string key = skins::selected();
    const skins::Choice* chosen = nullptr;
    for (const skins::Choice& c : s_choices)
        if (c.key == key) chosen = &c;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x, pad = 10.0f * scale, thumb = height - 2 * pad;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), tokenColor(mcui::tokens::kColorBgSecondary, 0.9f),
                        mcui::tokens::kRadiusX5 * scale);
    ImVec2 t(p.x + pad, p.y + pad);
    draw->AddRectFilled(t, ImVec2(t.x + thumb, t.y + thumb), tokenColor(mcui::tokens::kColorBgTertiary), 6.0f * scale);
    auto tex = s_textures.find(key);
    if (tex != s_textures.end()) {
        ImVec2 img = s_sizes[key];
        float k = std::min((thumb - 4 * scale) / img.x, (thumb - 4 * scale) / img.y);
        ImVec2 a(t.x + (thumb - img.x * k) / 2, t.y + thumb - 2 * scale - img.y * k);
        draw->AddImage(ImTextureRef((ImTextureID)(intptr_t)tex->second), a, ImVec2(a.x + img.x * k, a.y + img.y * k));
    }
    bool custom = chosen && chosen->custom;
    cloud::SkinPlace place = custom ? placeOf(key) : cloud::SkinPlace::Off;
    std::string kind = custom ? std::string(L("Свой скин", "Custom skin")) +
                                    (place != cloud::SkinPlace::Off ? std::string(" · ") + placeWords(place) : std::string())
                       : key.empty() ? std::string(L("Модель, как задумано в уровне", "The model the level was made with"))
                                     : std::string(L("Модель игры", "Game model"));
    struct Action {
        const char* label;
        int id;
        bool enabled;
        mcui::ButtonKind kind;
    };
    std::vector<Action> actions;
    bool idle = s_child == 0 && !s_game.empty();
    if (custom && cloud::skinsActive()) {
        if (place == cloud::SkinPlace::Here)
            actions.push_back({L("В облако", "Send to cloud"), 1, cloud::skinsForCloud() < cloud::kSkinLimit && s_child == 0,
                               mcui::ButtonKind::Default});
        else
            actions.push_back({L("Убрать из облака", "Remove from cloud"), 2, s_child == 0, mcui::ButtonKind::Default});
    }
    if (custom) actions.push_back({L("Удалить", "Delete"), 3, s_child == 0, mcui::ButtonKind::Ghost});
    if (chosen && !custom && !key.empty())
        actions.push_back({L("Свой скин на её основе", "Custom skin based on it"), 4, idle, mcui::ButtonKind::Default});
    const ImGuiStyle& style = ImGui::GetStyle();
    float buttonsW = 0;
    for (const Action& a : actions) buttonsW += ImGui::CalcTextSize(a.label).x + style.FramePadding.x * 2 + style.ItemSpacing.x;
    float textX = t.x + thumb + pad, textRight = p.x + width - pad - buttonsW;
    ImGui::PushClipRect(ImVec2(textX, p.y), ImVec2(std::max(textX, textRight), p.y + height), true);
    float line = ImGui::GetTextLineHeight(), small = ImGui::GetFontSize() * mcui::kTextCaption / mcui::kTextBody;
    float textTop = p.y + (height - line - small * 1.25f) / 2;
    const mcui::Fonts& fonts = mcui::GetFonts();
    std::string name = chosen ? (s_russian ? chosen->ru : chosen->en) : std::string();
    draw->AddText(fonts.medium ? fonts.medium : ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textX, textTop),
                  ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
    draw->AddText(ImGui::GetFont(), small, ImVec2(textX, textTop + line), tokenColor(mcui::tokens::kColorTextMuted), kind.c_str());
    ImGui::PopClipRect();
    ImGui::SetCursorScreenPos(ImVec2(p.x + width - pad - buttonsW + style.ItemSpacing.x, p.y + (height - ImGui::GetFrameHeight()) / 2));
    for (size_t i = 0; i < actions.size(); i++) {
        const Action& a = actions[i];
        if (i) ImGui::SameLine();
        ImGui::BeginDisabled(!a.enabled);
        bool pressed = mcui::Button(a.label, a.kind);
        ImGui::EndDisabled();
        if (a.id == 1 && !a.enabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s %d %s", L("В облаке уже", "The cloud already holds"), cloud::kSkinLimit,
                              L("скинов: сначала уберите из облака другой.", "skins: remove another one from the cloud first."));
        if (!pressed) continue;
        if (a.id == 1 && cloud::sendSkin(key, s_russian)) s_cloudNote.clear(), s_placesDirty = true;
        if (a.id == 2) cloud::keepSkinHere(key, s_russian), s_cloudNote.clear(), s_placesDirty = true;
        if (a.id == 3) ImGui::OpenPopup("##delete-skin");
        if (a.id == 4) createSkin();
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + height));
    ImGui::Dummy(ImVec2(width, 0));
}

}

void tab(float scale, bool russian) {
    namespace cloud = port::online::cloud;
    s_russian = russian;
    if (!s_tabShown) s_tabShown = true, s_reload = true;
    refreshPlaces();
    const float gap = 10.0f * scale, iconSize = 34.0f * scale;
    static std::string opening = getenv("MEDECOMP_HEROINE_OPEN") ? getenv("MEDECOMP_HEROINE_OPEN") : "";
    ImVec2 row = sectionRow(L("Модели игры", "Game models"), iconSize, scale);
    float right = row.x + ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorScreenPos(ImVec2(right - 3 * iconSize - 2 * gap, row.y));
    ImGui::BeginDisabled(s_game.empty() || s_child != 0);
    if (iconButton("##refresh", icons().refresh, L("Обновить превью", "Refresh previews"), iconSize)) {
        skins::rescan();
        remove((cacheDir() + "/stamp").c_str());
        s_error.clear();
        s_reload = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, gap);
    if (iconButton("##folder", icons().folder, L("Папка скинов", "Skins folder"), iconSize)) {
        makeDirs(skins::folder());
#ifdef __ANDROID__
        s_note = L("Свои скины лежат в папке приложения — их ставят «Импорт текстуры…» и «Импорт модели…» из любой папки "
                   "телефона. Шаблон для Blender и основы текстур игра кладёт в «Загрузки», папку MoonCellar.",
                   "Custom skins live in the app's folder — install them with Import texture… and Import model… from any "
                   "folder on the phone. The game puts the Blender template and texture bases into Downloads, the "
                   "MoonCellar folder.");
#else
        port::platform::openURL(("file://" + skins::folder()).c_str());
#endif
    }
    ImGui::SameLine(0, gap);
    launcherHelpContent("##heroine-info", [scale] { infoPopup(scale); }, false, opening == "info");
    ImGui::SetCursorScreenPos(ImVec2(row.x, row.y + iconSize + gap));

    if (s_game.empty())
        colored(mcui::tokens::kColorTextMuted, L("Сначала укажите копию игры на вкладке «Игра».", "Choose the game copy on the Game tab first."));
    else if (s_child)
        colored(mcui::tokens::kColorTextMuted, L("Готовлю превью…", "Rendering previews…"));
    if (!s_error.empty()) colored(mcui::tokens::kColorNegative, s_error);
#ifdef MEDECOMP_ONE_PROCESS
    if (s_stale && !s_game.empty()) {
        colored(mcui::tokens::kColorAttention,
                L("Превью скинов (и шаблон для Blender) рисует сама игра: приложение закроется и откроется снова, "
                  "это около полуминуты.",
                  "The game itself renders the skin previews (and the Blender template): the app closes and opens "
                  "again, about half a minute."));
        if (mcui::Button(L("Подготовить превью", "Prepare previews"), mcui::ButtonKind::Accent)) start(std::string());
    }
#endif

    std::string chosen = skins::selected();
    enum { NoAction, FromTexture, ImportTexture, ImportModel, Template } action = NoAction;
    std::vector<const skins::Choice*> models, customs;
    for (const skins::Choice& c : s_choices) (c.custom ? customs : models).push_back(&c);
    {
        float w = 0, textSize = ImGui::GetFontSize() * mcui::kTextCaption / mcui::kTextBody;
        int cols = columns(ImGui::GetContentRegionAvail().x, 72.0f * scale, gap, &w, int(models.size()));
        float imageH = std::min(w * 1.3f, 100.0f * scale), h = imageH + labelHeight(models, w, textSize, scale, 3) + 10.0f * scale;
        ImVec2 origin = ImGui::GetCursorScreenPos();
        for (size_t i = 0; i < models.size(); i++) {
            ImVec2 p(origin.x + float(i % size_t(cols)) * (w + gap), origin.y + float(i / size_t(cols)) * (h + gap));
            if (card(*models[i], p, ImVec2(w, h), imageH, textSize, models[i]->key == chosen, scale)) skins::select(models[i]->key);
        }
        size_t rows = (models.size() + size_t(cols) - 1) / size_t(cols);
        ImGui::SetCursorScreenPos(origin);
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, float(rows) * (h + gap) - gap));
    }

    ImGui::Dummy(ImVec2(0, 4.0f * scale));
    float chipH = ImGui::GetTextLineHeight() + 8.0f * scale;
    ImVec2 customRow = sectionRow(L("Свои скины", "Custom skins"), chipH, scale);
    if (cloud::skinsActive()) {
        int saved = 0, waiting = 0, here = 0;
        for (const auto& p : s_places) {
            saved += p.second == cloud::SkinPlace::Saved || p.second == cloud::SkinPlace::Changed;
            waiting += p.second == cloud::SkinPlace::New;
            here += p.second == cloud::SkinPlace::Here;
        }
        std::string text = std::to_string(saved + waiting) + " / " + std::to_string(cloud::kSkinLimit) + " " + L("в облаке", "in the cloud");
        ImGui::SameLine(0, 12.0f * scale);
        ImVec2 c = ImGui::GetCursorScreenPos();
        c.y = customRow.y;
        float s = 14.0f * scale, padX = 10.0f * scale, w = padX + s + 6.0f * scale + ImGui::CalcTextSize(text.c_str()).x + padX;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(c, ImVec2(c.x + w, c.y + chipH), tokenColor(mcui::tokens::kColorBgPrimary, 0.7f), chipH / 2);
        mcui::DrawIcon(draw, placeIcon(cloud::SkinPlace::Saved), ImVec2(c.x + padX, c.y + (chipH - s) / 2), s,
                       tokenColor(placeColor(cloud::SkinPlace::Saved)));
        draw->AddText(ImVec2(c.x + padX + s + 6.0f * scale, c.y + (chipH - ImGui::GetTextLineHeight()) / 2),
                      tokenColor(mcui::tokens::kColorTextSecondary), text.c_str());
        ImGui::SetCursorScreenPos(c);
        ImGui::Dummy(ImVec2(w, chipH));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s %d, %s %d, %s %d", L("в облаке", "in the cloud"), saved, L("ждут отправки", "waiting to upload"),
                              waiting, L("только здесь", "here only"), here);
    }
    ImGui::SetCursorScreenPos(ImVec2(customRow.x, customRow.y + chipH + gap));
    std::string notice = cloud::skinsActive() ? cloud::skinsNotice() : std::string();
    if (!notice.empty()) colored(mcui::tokens::kColorAttention, notice);
    if (!s_note.empty()) colored(mcui::tokens::kColorAttention, s_note);
    if (!s_cloudNote.empty()) colored(mcui::tokens::kColorAttention, s_cloudNote);
#ifdef __ANDROID__
    {
        std::string shown = chosen.empty() ? std::string() : readFile(cacheDir() + "/exported-" + chosen);
        if (!shown.empty())
            launcherHelp((std::string(L("Основа текстуры этого скина — ", "This skin's texture base is ")) + shown +
                          L(": правьте её в любом редакторе картинок и ставьте «Импорт текстуры…».",
                            ": edit it in any image editor and install it with Import texture….")).c_str());
    }
#endif
    {
        float w = 0, textSize = ImGui::GetFontSize();
        int cols = columns(ImGui::GetContentRegionAvail().x, 136.0f * scale, gap, &w);
        float imageH = std::min(w * 1.2f, 200.0f * scale), h = imageH + labelHeight(customs, w, textSize, scale, 2) + 10.0f * scale;
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::BeginDisabled(s_game.empty() || s_child != 0);
        if (newTile(origin, ImVec2(w, h), scale) || opening == "new") ImGui::OpenPopup("##new-skin-menu");
        ImGui::EndDisabled();
        ImGui::SetNextWindowPos(ImVec2(origin.x + w * 0.5f, origin.y + h * 0.35f), ImGuiCond_Appearing);
        if (ImGui::BeginPopup("##new-skin-menu")) {
            const float menuW = 340.0f * scale;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 2.0f * scale));
            namespace dialog = port::platform::dialog;
            if (menuItem("##from-texture", icons().image, L("Из текстуры модели", "From the model's texture"),
                         L("Текстура выбранной модели — в папку нового скина: правьте её в редакторе картинок",
                           "The chosen model's texture goes into a new skin's folder: edit it in an image editor"),
                         menuW, scale))
                action = FromTexture;
            if (menuItem("##import-texture", icons().upload, L("Импорт текстуры…", "Import texture…"),
                         L("PNG 512×512 на выбранную модель (на выбранный свой скин — вместо его текстуры)",
                           "A 512×512 PNG on the chosen model (on a chosen custom skin — instead of its texture)"),
                         menuW, scale, !dialog::busy()))
                action = ImportTexture;
            if (menuItem("##import-model", icons().model, L("Импорт модели…", "Import model…"),
                         L("glTF или VRM — привяжется к скелету Фейт", "glTF or VRM — bound to Faith's skeleton"), menuW, scale,
                         !dialog::busy()))
                action = ImportModel;
            ImGui::Separator();
            if (menuItem("##template", icons().download, L("Шаблон для Blender", "Template for Blender"),
                         L("Скелет Фейт: привяжите к нему свою модель для точной привязки",
                           "Faith's skeleton: bind your model to it for the most precise fit"),
                         menuW, scale))
                action = Template;
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        for (size_t i = 0; i < customs.size(); i++) {
            size_t at = i + 1;
            ImVec2 p(origin.x + float(at % size_t(cols)) * (w + gap), origin.y + float(at / size_t(cols)) * (h + gap));
            if (card(*customs[i], p, ImVec2(w, h), imageH, textSize, customs[i]->key == chosen, scale)) skins::select(customs[i]->key);
        }
        size_t rows = (customs.size() + 1 + size_t(cols) - 1) / size_t(cols);
        ImGui::SetCursorScreenPos(origin);
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, float(rows) * (h + gap) - gap));
    }
    opening.clear();
    {
        namespace dialog = port::platform::dialog;
        if (action == FromTexture) createSkin();
        if (action == ImportTexture) dialog::pick(dialog::Png, std::string(), L("Текстура героини (PNG)", "Heroine texture (PNG)"));
        if (action == ImportModel)
            dialog::pick(dialog::Gltf, std::string(), L("Своя модель героини (glTF 2.0, VRM)", "Custom heroine model (glTF 2.0, VRM)"));
        if (action == Template) exportTemplate();
    }
}

float barHeight(float scale) { return 60.0f * scale; }

void bar(float scale) {
    namespace cloud = port::online::cloud;
    selectionBar(scale, barHeight(scale));
    std::string chosen = skins::selected();
    if (ImGui::BeginPopup("##delete-skin")) {
        std::string label;
        for (const skins::Choice& c : s_choices)
            if (c.key == chosen) label = s_russian ? c.ru : c.en;
        ImGui::Text("%s «%s»?", L("Удалить скин", "Delete skin"), label.c_str());
        cloud::SkinPlace place = placeOf(chosen);
        if (place == cloud::SkinPlace::Saved || place == cloud::SkinPlace::Changed)
            ImGui::TextDisabled("%s", L("Он удалится и из облака, и с других компьютеров при их сверке. Оставить на других — "
                                        "«Убрать из облака».",
                                        "It is deleted from the cloud and from other computers when they sync. To keep it "
                                        "there — Remove from cloud."));
        if (ImGui::Button(L("Удалить", "Delete"))) {
            deleteSkin(chosen);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(L("Отмена", "Cancel"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

std::string boneWords(const std::string& bone) {
    static const struct {
        const char *name, *ru, *en;
    } kWords[] = {
        {"pelvis", "таз", "pelvis"}, {"head", "голова", "head"},
        {"upperarm_l", "левое плечо", "left upper arm"}, {"upperarm_r", "правое плечо", "right upper arm"},
        {"forearm_l", "левое предплечье", "left forearm"}, {"forearm_r", "правое предплечье", "right forearm"},
        {"thigh_l", "левое бедро", "left thigh"}, {"thigh_r", "правое бедро", "right thigh"},
        {"calf_l", "левая голень", "left calf"}, {"calf_r", "правая голень", "right calf"},
        {"?", "скелет не понят", "the skeleton is not understood"},
    };
    for (const auto& w : kWords)
        if (bone == w.name) return L(w.ru, w.en);
    return bone;
}

bool onTemplate(const port::platform::gltf::Scene& scene, const port::platform::gltf::Model& faith,
                port::platform::gltf::Model* model) {
    std::vector<int> map;
    std::string error;
    if (!port::platform::gltf::toModel(scene, model, &error) || !checkModel(*model, &map, &error)) return false;
    for (size_t b = 0; b < model->bones.size(); b++) {
        const float* m = &model->inverseBind[b * 16];
        const float* f = &faith.inverseBind[size_t(map[b]) * 16];
        for (int i = 0; i < 16; i++)
            if (fabsf(m[i] - f[i]) > 0.02f) return false;
    }
    return true;
}

void importModel(const std::string& path) {
    namespace gltf = port::platform::gltf;
    gltf::setRussian(s_russian);
    gltf::Scene scene;
    std::string error;
    if (!gltf::readScene(path, &scene, &error)) {
        s_error = std::string(L("модель не подходит: ", "the model does not fit: ")) + error;
        fprintf(stderr, "medecomp: импорт модели %s — %s\n", path.c_str(), s_error.c_str());
        return;
    }
    gltf::Model faith;
    if (!gltf::read(cacheDir() + "/faith-template.glb", &faith, &error)) {
        s_error = L("шаблона Фейт ещё нет — дождитесь превью", "no Faith template yet — wait for the previews");
        return;
    }
    gltf::Model model;
    autorig::Report report;
    bool asIs = onTemplate(scene, faith, &model);
    if (!asIs) {
        model = gltf::Model();
        if (!autorig::fit(scene, faith, &model, &report, &error)) {
            s_error = std::string(L("модель не подходит: ", "the model does not fit: ")) + error;
            fprintf(stderr, "medecomp: импорт модели %s — %s\n", path.c_str(), s_error.c_str());
            return;
        }
    }
    std::string stem = path.substr(path.find_last_of("/\\") + 1), id;
    stem = stem.substr(0, stem.find_last_of('.'));
    std::string dir = newSkin("game:faith-day", stem, &id);
    if (!gltf::write(dir + "/model.glb", model, &error)) {
        s_error = error;
        return;
    }
    s_error.clear();
    s_note.clear();
    if (asIs) {
        if (!model.hasImage())
            s_note = L("В модели нет текстуры: положите texture.png в папку скина или «Импорт текстуры…».",
                       "The model has no texture: put texture.png into the skin folder or use Import texture….");
    } else {
        if (report.bySkeleton) {
            s_note = std::string(L("Автопривязка: скелет", "Auto-rig: skeleton")) + (report.rig.empty() ? "" : " " + report.rig) +
                     L(", костей сопоставлено: ", ", bones matched: ") + std::to_string(report.mapped) +
                     L(", слито в ближайшие (пальцы, волосы, одежда): ", ", merged into the nearest (fingers, hair, clothes): ") +
                     std::to_string(report.merged) + ".";
            if (report.transferred)
                s_note += std::string(" ") + L("Вершин без костей Фейт (веса — с тела Фейт): ", "Vertices without Faith's bones (weights from Faith's body): ") +
                          std::to_string(report.transferred) + ".";
        } else {
            std::string missing;
            for (const std::string& b : report.missing) missing += (missing.empty() ? "" : ", ") + boneWords(b);
            s_note = std::string(L("Автопривязка: ", "Auto-rig: ")) +
                     (scene.skins.empty() ? std::string(L("скелета нет", "no skeleton"))
                                          : std::string(L("в скелете нет: ", "the skeleton lacks: ")) + missing) +
                     L(" — суставы найдены по форме тела, веса взяты с тела Фейт.",
                       " — joints found by the body shape, weights taken from Faith's body.");
        }
        if (report.turned) s_note += std::string(" ") + L("Модель повёрнута, как стоит Фейт.", "The model was turned to stand like Faith.");
        if (report.downscaled)
            s_note += std::string(" ") + L("Картинки уменьшены: скин уходит сопернику по сети (до 8 МБ).",
                                           "Images were downscaled: the skin is sent to the online rival (up to 8 MB).");
        s_note += std::string(" ") + L("Модель встала в позу Фейт и двигается её анимациями; длинная одежда (юбка, плащ, "
                                       "длинные волосы) может тянуться за ногами. Не понравилось — «Удалить скин».",
                                       "The model now has Faith's pose and moves with her animations; long clothes (a "
                                       "skirt, a cloak, long hair) may stretch with the legs. Not happy — Delete skin.");
    }
    fprintf(stderr, "medecomp: импорт модели %s — скин %s, %zu вершин, частей %zu. %s\n", path.c_str(), id.c_str(),
            model.vertexCount(), model.parts.size(), s_note.c_str());
    skins::rescan();
    s_choices = skins::list();
    skins::select(id);
    s_reload = true;
    port::online::cloud::syncSkins(s_russian);
}

void deleteSkin(const std::string& key) {
    std::string dir = skins::folder() + "/" + key;
    for (const std::string& name : port::files::list(dir)) remove((dir + "/" + name).c_str());
    if (rmdir(dir.c_str()) != 0) {
        s_error = std::string(L("не удалить ", "cannot delete ")) + dir;
        return;
    }
    s_error.clear();
    s_note.clear();
    skins::select(std::string());
    skins::rescan();
    s_choices = skins::list();
    remove((cacheDir() + "/stamp").c_str());
    s_reload = true;
    s_cloudNote.clear();
    port::online::cloud::syncSkins(s_russian);
}

void exportTemplate() {
    std::string bytes = readFile(cacheDir() + "/faith-template.glb");
    if (bytes.empty()) {
        s_error = L("шаблона ещё нет — дождитесь превью", "no template yet — wait for the previews");
        return;
    }
#ifdef __ANDROID__
    std::string shown = androidSaveToDownloads(cacheDir() + "/faith-template.glb", "faith-template.glb", "model/gltf-binary");
    if (shown.empty()) {
        shown = externalDir() + "/faith-template.glb";
        std::ofstream(shown.c_str(), std::ios::binary).write(bytes.data(), std::streamsize(bytes.size()));
    }
    s_note = std::string(L("Шаблон — ", "The template is ")) + shown +
             L(": на компьютере (по USB) привяжите к этому скелету свою модель (кости и позу не менять), выгрузите в "
               "glTF 2.0 (.glb) с текстурой, скопируйте на телефон и — «Импорт модели…».",
               ": on a computer (over USB) bind your model to this skeleton (keep the bones and the pose), export glTF "
               "2.0 (.glb) with a texture, copy it to the phone and use Import model….");
    return;
#endif
    makeDirs(skins::folder());
    std::ofstream((skins::folder() + "/faith-template.glb").c_str(), std::ios::binary).write(bytes.data(), std::streamsize(bytes.size()));
    s_note = L("faith-template.glb — в папке скинов: привяжите свою модель к этому скелету (кости и позу не менять) и "
               "выгрузите в glTF 2.0 (.glb) с текстурой; затем «Импорт модели…».",
               "faith-template.glb is in the skins folder: bind your model to this skeleton (keep the bones and the "
               "pose) and export glTF 2.0 (.glb) with a texture; then Import model….");
    port::platform::openURL(("file://" + skins::folder()).c_str());
}

void picked(bool texture, const std::string& path) {
    if (texture) importTexture(path);
    else importModel(path);
}

void reloadSkins() {
    skins::rescan();
    s_choices = skins::list();
    s_reload = true;
}

void shutdown() {
    for (auto& t : s_textures) SDL_DestroyTexture(t.second);
    s_textures.clear();
}

#ifdef MEDECOMP_ONE_PROCESS
bool previewRequested() { return s_previewPending; }

bool takePreviewRequest(std::vector<std::string>* env) {
    if (!s_previewPending) return false;
    s_previewPending = false;
    *env = s_previewEnv;
    return true;
}
#endif

}
}
}

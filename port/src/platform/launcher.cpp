#include "../audio/music.h"
#include "../ea/cloud.h"
#include "../ea/json.h"
#include "../ea/levels.h"
#include "../editor/editor.h"
#include "../gfx/shadows.h"
#include "../gfx/texture_pack.h"
#include "bindings.h"
#include "file_dialog.h"
#include "hd_textures.h"
#include "heroine_tab.h"
#include "launcher.h"
#include "touch_device.h"
#include "launcher_help.h"
#include "level_file.h"
#include "overlay.h"
#include "update.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif
#ifndef __EMSCRIPTEN__
#include <fontconfig/fontconfig.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "game/ResourceManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "mcui.h"
#include "port/cf.h"
#include "port/achievements.h"
#include "port/brand.h"
#include "port/files.h"
#include "port/online.h"
#include "port/platform.h"
#include "port/touch.h"
#include "settings.h"
#include "turn.h"

namespace {

#ifdef __EMSCRIPTEN__
SDL_Window* s_webLauncherWindow = nullptr;
#endif

using port::platform::launcherHelp;

const char* kVersion = "1.1.48";
const char* const kTestedNewer[] = {"1.1.52"};

bool s_russian;
bool s_arrowGlyph;
const char* L(const char* ru, const char* en) { return s_russian ? ru : en; }

bool russianSystem() {
    for (const char* var : {"MEDECOMP_LANG", "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* v = getenv(var);
        if (v && *v) return strncmp(v, "ru", 2) == 0;
    }
    return false;
}

const int kResourceCount = 465;

struct Check {
    bool ok = false;
    bool warn = false;
    std::string text;
};

int compareVersions(const std::string& a, const std::string& b) {
    const char* p = a.c_str();
    const char* q = b.c_str();
    while (*p || *q) {
        char* end;
        long x = strtol(p, &end, 10);
        p = *end == '.' ? end + 1 : end;
        long y = strtol(q, &end, 10);
        q = *end == '.' ? end + 1 : end;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

void checkResources(std::vector<std::string>* missing, int* changed) {
    const std::string res = port::files::bundle() + "/res/";
    *changed = 0;
    for (int id = 0; id < kResourceCount; id++) {
        port::files::Info info = port::files::stat(res + ResourceManager::RESOURCE_FILENAMES_LIST[id]);
        if (!info.exists)
            missing->push_back(ResourceManager::RESOURCE_FILENAMES_LIST[id]);
        else if (info.size != uint64_t(ResourceManager::RESOURCE_FILESIZE_LIST[id]))
            ++*changed;
    }
}

Check check(const std::string& path) {
    Check c;
    if (path.empty()) {
#ifdef __EMSCRIPTEN__
        c.text = L("Выберите свою копию .ipa кнопкой «Обзор»", "Choose your .ipa copy with Browse");
#else
        c.text = L("Укажите файл .ipa или папку .app", "Choose an .ipa file or an .app folder");
#endif
        return c;
    }
    if (!port::files::stat(path).exists) {
        c.text = L("Файл не найден", "File not found");
        return c;
    }
    std::string error;
    if (!port::files::openGame(path, &error)) {
        c.text = s_russian ? "Не подходит: " + error : std::string("Not a game copy (.ipa with Payload/*.app, or .app)");
        return c;
    }
    std::string id = port::cf::infoString("CFBundleIdentifier");
    std::string name = port::cf::infoString("CFBundleDisplayName");
    std::string version = port::cf::infoString("CFBundleShortVersionString");
    if (version.empty()) version = port::cf::infoString("CFBundleVersion");
    std::string title = (name.empty() ? std::string("Mirror's Edge") : name) + " " + version + " — " + id;
    if (id.find("mirrorsedge") == std::string::npos) {
        c.text = std::string(L("Это другое приложение: ", "This is a different app: ")) +
                 (name.empty() ? id : name + " (" + id + ")");
        return c;
    }
    if (id.find("ipad") == std::string::npos) {
        c.text = title + (s_russian ? std::string("\nЭто версия для iPhone, нужна версия для iPad (") + kVersion +
                                          " или новее): в версии для iPhone нет мультиплеера, а экран размечен под телефон"
                                    : std::string("\nThis is the iPhone version; the iPad version is required (") +
                                          kVersion + " or newer): the iPhone version has no multiplayer and a phone screen layout");
        return c;
    }
    if (compareVersions(version, kVersion) < 0) {
        c.text = title + (s_russian ? std::string("\nВерсия слишком старая: нужна ") + kVersion + " или новее"
                                    : std::string("\nThis version is too old: ") + kVersion + " or newer is required");
        return c;
    }
    std::vector<std::string> missing;
    int changed = 0;
    checkResources(&missing, &changed);
    if (!missing.empty()) {
        std::string list;
        for (size_t i = 0; i < missing.size() && i < 3; i++) list += (i ? ", " : "") + missing[i];
        if (missing.size() > 3) list += ", …";
        c.text = title + (s_russian ? "\nКопия неполная: нет файлов игры — " + std::to_string(missing.size()) + " (" + list + ")"
                                    : "\nThe copy is incomplete: " + std::to_string(missing.size()) + " game files are missing (" +
                                          list + ")");
        return c;
    }
    c.ok = true;
    c.text = title;
    bool newer = compareVersions(version, kVersion) > 0;
    bool tested = false;
    for (const char* v : kTestedNewer) tested = tested || version == v;
    if ((newer || changed) && !tested) {
        c.warn = true;
        std::string files = std::string(L(" (отличаются от ", " (files differing from ")) + kVersion + ": " +
                            std::to_string(changed) + ")";
        if (newer)
            c.text += std::string(L("\nПорт сделан по версии ", "\nThe port is based on version ")) + kVersion +
                      L(", более новые должны подойти, но не проверялись", "; newer ones should work but are untested") +
                      (changed ? files : std::string());
        else
            c.text += L("\nФайлы игры изменены", "\nThe game files are modified") + files +
                      L(": возможны ошибки", ": expect issues");
    }
    return c;
}

struct GameDownload {
    std::thread thread;
    std::atomic<bool> busy{false}, cancel{false}, done{false};
    std::atomic<long long> got{0}, total{0};
    std::mutex mu;
    std::string error;
};
GameDownload s_download;
std::string s_downloadNote;
bool s_downloadOk = false;
char s_url[1024];
Uint32 s_downloadStarted, s_downloadCancelAfter;

std::string downloadDir() { return port::files::dataDir() + "/game"; }
std::string downloadPart() { return downloadDir() + "/download.part"; }

void startDownload(const std::string& url) {
    if (s_download.busy) return;
    if (s_download.thread.joinable()) s_download.thread.join();
    mkdir(port::files::dataDir().c_str(), 0755);
    mkdir(downloadDir().c_str(), 0755);
    s_download.busy = true;
    s_download.cancel = false;
    s_download.done = false;
    s_download.got = 0;
    s_download.total = 0;
    s_downloadNote.clear();
    s_downloadStarted = SDL_GetTicks();
    fprintf(stderr, "medecomp: копия игры — качаю по ссылке игрока\n");
    s_download.thread = std::thread([url] {
        std::string error;
        bool ok = port::online::download(url, downloadPart(), [](long long got, long long total) {
            s_download.got = got;
            s_download.total = total;
            return !s_download.cancel.load();
        }, &error);
        {
            std::lock_guard<std::mutex> lock(s_download.mu);
            s_download.error = ok ? std::string() : (error.empty() ? std::string("?") : error);
        }
        s_download.done = true;
    });
}

void finishDownload(char* buf, size_t size, Check* status, const std::string& checked) {
    if (s_download.busy && s_downloadCancelAfter && SDL_GetTicks() - s_downloadStarted >= s_downloadCancelAfter)
        s_download.cancel = true;
    if (!s_download.done) return;
    if (s_download.thread.joinable()) s_download.thread.join();
    s_download.done = false;
    s_download.busy = false;
    std::string error;
    {
        std::lock_guard<std::mutex> lock(s_download.mu);
        error = s_download.error;
    }
    s_downloadOk = false;
    if (!error.empty()) {
        remove(downloadPart().c_str());
        bool cancelled = s_download.cancel;
        s_downloadNote = cancelled ? std::string(L("Загрузка отменена — файл удалён, копия игры прежняя.",
                                                   "The download is cancelled — the file is deleted, the game copy is unchanged."))
                                   : std::string(L("Загрузка не удалась: ", "The download failed: ")) + error +
                                         L(" — файл удалён, копия игры прежняя.", " — the file is deleted, the game copy is unchanged.");
        fprintf(stderr, "medecomp: копия игры по ссылке: %s\n", cancelled ? "отменено" : error.c_str());
        return;
    }
    std::string target = downloadDir() + "/download-" + std::to_string((long long)time(nullptr)) + ".ipa";
    if (rename(downloadPart().c_str(), target.c_str()) != 0) {
        remove(downloadPart().c_str());
        s_downloadNote = std::string(L("Не переименовать скачанный файл: ", "Cannot rename the downloaded file: ")) + target;
        return;
    }
    Check c = check(target);
    if (!c.ok) {
        *status = check(checked);
        remove(target.c_str());
        s_downloadNote = std::string(L("Скачанный файл не подходит — удалён, копия игры прежняя. ",
                                       "The downloaded file does not fit — it is deleted, the game copy is unchanged. ")) +
                         c.text;
        fprintf(stderr, "medecomp: копия игры по ссылке не подошла: %s\n", c.text.c_str());
        return;
    }
    for (const std::string& name : port::files::list(downloadDir())) {
        std::string file = downloadDir() + "/" + name;
        if (name.compare(0, 9, "download-") == 0 && file != target) remove(file.c_str());
    }
    snprintf(buf, size, "%s", target.c_str());
    s_downloadOk = true;
    s_downloadNote = std::string(L("Скачано и подошло: ", "Downloaded and it fits: ")) + c.text;
    fprintf(stderr, "medecomp: копия игры по ссылке подошла: %s\n", target.c_str());
}

void downloadSection(float scale) {
    ImGui::Spacing();
    const char* help = L("Или скачать по ссылке на .ipa (https://): ссылку указываете вы — в порте ни одной нет. "
                         "Скачанное проверяется так же, как выбранный файл; не подошло — удаляется.",
                         "Or download from a link to the .ipa (https://): you give the link — the port has none. "
                         "The download is checked like a chosen file; if it does not fit, it is deleted.");
    const char* get = L("Скачать", "Download");
    const char* stop = L("Отмена", "Cancel");
    float button = std::max(ImGui::CalcTextSize(get).x, ImGui::CalcTextSize(stop).x) + ImGui::GetStyle().FramePadding.x * 2;
    bool busy = s_download.busy;
    ImGui::SetNextItemWidth(-button - port::platform::launcherHelpButtonSize() - ImGui::GetStyle().ItemSpacing.x * 2);
    ImGui::BeginDisabled(busy);
    ImGui::InputTextWithHint("##url", "https://…", s_url, sizeof s_url);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (busy) {
        if (ImGui::Button(stop, ImVec2(button, 0))) s_download.cancel = true;
    } else {
        ImGui::BeginDisabled(strncmp(s_url, "https://", 8) != 0);
        if (ImGui::Button(get, ImVec2(button, 0))) startDownload(s_url);
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    launcherHelp(help);
    if (busy) {
        long long got = s_download.got, total = s_download.total;
        mcui::ProgressBar(total > 0 ? float(double(got) / double(total)) : 0.0f, ImVec2(-1.0f, 6.0f * scale));
        if (total > 0)
            ImGui::TextDisabled("%s %.1f / %.1f %s", L("Скачано", "Downloaded"), double(got) / 1048576.0,
                                double(total) / 1048576.0, L("МБ", "MB"));
        else
            ImGui::TextDisabled("%s %.1f %s", L("Скачано", "Downloaded"), double(got) / 1048576.0, L("МБ", "MB"));
    }
    if (!s_downloadNote.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(s_downloadOk ? mcui::tokens::kColorPositive : mcui::tokens::kColorNegative));
        ImGui::TextWrapped("%s", s_downloadNote.c_str());
        ImGui::PopStyleColor();
    }
}

ImVec4 statusColor(const Check& c) {
    using namespace mcui::tokens;
    return mcui::Color(!c.ok ? kColorNegative : c.warn ? kColorAttention : kColorPositive);
}

std::string fontFile() {
    std::string out;
#ifdef _WIN32
    if (const char* windir = getenv("WINDIR"))
        for (const char* name : {"segoeui.ttf", "arial.ttf", "tahoma.ttf"}) {
            std::string path = std::string(windir) + "\\Fonts\\" + name;
            if (FILE* f = fopen(path.c_str(), "rb")) {
                fclose(f);
                return path;
            }
        }
#endif
#ifdef __EMSCRIPTEN__
    return "/fonts/LiberationSans-Regular.ttf";
#else
    if (!FcInit()) return out;
    FcPattern* p = FcNameParse(reinterpret_cast<const FcChar8*>("sans-serif:lang=ru"));
    FcConfigSubstitute(nullptr, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    FcResult r;
    FcPattern* m = FcFontMatch(nullptr, p, &r);
    FcChar8* file = nullptr;
    if (m && FcPatternGetString(m, FC_FILE, 0, &file) == FcResultMatch) out = reinterpret_cast<const char*>(file);
    if (m) FcPatternDestroy(m);
    FcPatternDestroy(p);
    size_t dot = out.rfind('.');
    std::string ext = dot == std::string::npos ? "" : out.substr(dot);
    for (char& ch : ext) ch = char(tolower(ch));
    if (ext != ".ttf" && ext != ".otf" && ext != ".ttc") out.clear();
    return out;
#endif
}

struct ArrowFont {
    std::string path;
    int index = 0;
};

ArrowFont arrowFont() {
    ArrowFont out;
#ifdef __EMSCRIPTEN__
    out.path = "/fonts/LiberationSans-Regular.ttf";
#elif !defined(_WIN32)
    if (!FcInit()) return out;
    FcPattern* p = FcNameParse(reinterpret_cast<const FcChar8*>("sans-serif:charset=2192:fontformat=TrueType"));
    if (!p) return out;
    FcConfigSubstitute(nullptr, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    FcResult r;
    FcPattern* m = FcFontMatch(nullptr, p, &r);
    FcCharSet* cs = nullptr;
    FcChar8* file = nullptr;
    if (m && FcPatternGetCharSet(m, FC_CHARSET, 0, &cs) == FcResultMatch && FcCharSetHasChar(cs, 0x2192) &&
        FcPatternGetString(m, FC_FILE, 0, &file) == FcResultMatch) {
        out.path = reinterpret_cast<const char*>(file);
        size_t dot = out.path.rfind('.');
        std::string ext = dot == std::string::npos ? "" : out.path.substr(dot);
        for (char& ch : ext) ch = char(tolower(ch));
        if (ext != ".ttf" && ext != ".otf" && ext != ".ttc") out.path.clear();
        else FcPatternGetInteger(m, FC_INDEX, 0, &out.index);
    }
    if (m) FcPatternDestroy(m);
    FcPatternDestroy(p);
#endif
    return out;
}

void mcuiClearColor(SDL_Renderer* renderer) {
    ImVec4 c = mcui::Color(mcui::tokens::kColorBgPrimary);
    SDL_SetRenderDrawColor(renderer, Uint8(c.x * 255), Uint8(c.y * 255), Uint8(c.z * 255), 255);
}

bool loadLauncherFont() {
    const std::string dir = port::platform::brandingDir();
    mcui::FontFiles files;
    files.regular = dir + "fonts/ApercuPro-Regular.ttf";
    files.medium = dir + "fonts/ApercuPro-Medium.ttf";
    files.bold = dir + "fonts/ApercuPro-Bold.ttf";
    std::string system = fontFile();
    if (!system.empty()) files.fallback.push_back({system, 0});
#ifdef _WIN32
    s_arrowGlyph = !system.empty();
#else
    ArrowFont arrows = arrowFont();
    if (!arrows.path.empty() && !(arrows.path == system && arrows.index == 0))
        files.fallback.push_back({arrows.path, arrows.index});
    s_arrowGlyph = !arrows.path.empty();
#endif
    const mcui::Fonts& fonts = mcui::LoadFonts(files);
    if (!fonts.brand)
        fprintf(stderr, "medecomp: лаунчер: шрифта MoonCellar нет в %sfonts — системный шрифт\n", dir.c_str());
    if (!fonts.brand && system.empty()) s_arrowGlyph = false;
    return fonts.brand || !system.empty();
}

struct Language {
    const char* code;
    const char* ru;
    const char* en;
};
const Language kLanguages[] = {
    {"en", "English", "English"},      {"de", "Deutsch", "Deutsch"},     {"es_ES", "Español", "Español"},
    {"fr", "Français", "Français"},    {"it", "Italiano", "Italiano"},   {"ja", "Японский", "Japanese"},
    {"ko", "Корейский", "Korean"},     {"zh_CN", "Китайский (упрощённый)", "Chinese (Simplified)"},
    {"ru", "Русский", "Русский"},
};

void languageSection(std::string* language, float scale) {
    mcui::SectionTitle(L("Язык игры", "Game language"));
    const char* automatic = L("Как в игре", "As in the game");
    const char* current = automatic;
    for (const Language& l : kLanguages)
        if (*language == l.code) current = L(l.ru, l.en);
    ImGui::SetNextItemWidth(260 * scale);
    if (ImGui::BeginCombo("##language", current)) {
        if (ImGui::Selectable(automatic, current == automatic)) *language = "auto";
        for (const Language& l : kLanguages)
            if (ImGui::Selectable(L(l.ru, l.en), *language == l.code)) *language = l.code;
        ImGui::EndCombo();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const char* step = s_arrowGlyph ? "→" : "->";
    std::string ru = std::string("«Как в игре» — выбранный в её окне «Язык», а в первый раз — язык системы. Сменить "
                                 "можно и в самой игре: Настройки ") +
                     step + " Язык.";
    std::string en = std::string("As in the game — the one chosen in its Language window, the first time — the "
                                 "system language. It can also be changed in the game: Options ") +
                     step + " Language.";
    launcherHelp(L(ru.c_str(), en.c_str()));
    ImGui::PopStyleColor();
}

std::string s_hdGame;

void hdSection(float scale) {
    namespace hd = port::platform::hd;
    hd::Status st = hd::status();
    if (st.running) {
        float part = st.total ? float(st.done) / float(st.total) : 0.0f;
        std::string count = st.total ? std::to_string(st.done) + " / " + std::to_string(st.total) : "";
        ImGui::ProgressBar(part, ImVec2(260 * scale, 0), count.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(L("Стоп", "Stop"))) hd::stop(false);
        ImGui::TextDisabled("%s", st.stage.c_str());
        return;
    }
    ImGui::BeginDisabled(s_hdGame.empty());
    if (ImGui::Button(L("Сделать HD-текстуры", "Make HD textures"))) hd::start(s_hdGame, s_russian);
    ImGui::EndDisabled();
    launcherHelp(L("Real-ESRGAN ×4 на видеокарте (Vulkan): полчаса и больше, до 3 ГБ; только у вас",
                                "Real-ESRGAN ×4 on the GPU (Vulkan): half an hour or more, up to 3 GB; stays on your PC"));
    if (!st.stage.empty()) ImGui::TextDisabled("%s", st.stage.c_str());
    if (!st.error.empty()) ImGui::TextColored(mcui::Color(mcui::tokens::kColorNegative), "%s", st.error.c_str());
}

struct Screen {
    int display;
    int width, height;
    bool vsync;
    int videoDriver;
};

std::string modeText(int w, int h) { return std::to_string(w) + "×" + std::to_string(h); }

void displayTab(bool* original, int* mode, int* fps, int* upscale, float* sharpness, bool* wide, int* render, int* filter,
                int* msaa, bool* pack, int* post, float* postStrength, int* ssao, bool* godRays, bool* motionBlur,
                bool* ssr, float* ssrStrength, bool* shadows, bool* flare, int* grade, bool* taa, Screen* screen,
                float scale) {
    using port::platform::WindowMode;
    mcui::SectionTitle(L("Графика", "Graphics"));
    if (ImGui::RadioButton("Original", *original)) *original = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Remastered", !*original)) *original = false;
    launcherHelp(*original ? L("Кадр как на iPad: 4:3, 768×1024, 30 кадров, по целым пикселям, без улучшений",
                                             "The frame as on the iPad: 4:3, 768×1024, 30 fps, integer pixels, no enhancements")
                                        : L("Свои настройки — ниже", "Your own settings — below"));
#ifdef __EMSCRIPTEN__
    mcui::SectionTitle(L("Ориентация", "Orientation"));
    const std::string orientation = port::platform::settings::get("web_orientation", "auto");
    for (const auto& option : {std::make_pair("auto", L("По устройству", "Follow device")),
                               std::make_pair("landscape", L("Альбомная", "Landscape")),
                               std::make_pair("portrait", L("Книжная", "Portrait"))}) {
        if (ImGui::RadioButton(option.second, orientation == option.first)) {
            port::platform::settings::set("web_orientation", option.first);
            port::platform::settings::save();
        }
    }
    launcherHelp(L("Выберите расположение кадра. Если браузер не умеет фиксировать ориентацию экрана, игра повернёт изображение и управление сама.",
                   "Choose the frame orientation. If the browser cannot lock the screen orientation, the game rotates its image and input itself."));
    mcui::SectionTitle(L("Экран браузера", "Browser screen"));
    const bool supported = MAIN_THREAD_EM_ASM_INT({ return !!Module.medecompFullscreenSupported?.(); }) != 0;
    bool fullscreen = supported && port::platform::settings::get("web_fullscreen", "0") == "1";
    ImGui::BeginDisabled(!supported);
    if (mcui::Toggle(L("Запускать во весь экран", "Start in fullscreen"), &fullscreen)) {
        port::platform::settings::set("web_fullscreen", fullscreen ? "1" : "0");
        port::platform::settings::save();
    }
    ImGui::EndDisabled();
    launcherHelp(supported ? L("Браузерный fullscreen включится при нажатии «Играть». Если браузер отклонит запрос, игра продолжит работать во вкладке.",
                              "Browser fullscreen starts when you press Play. If the browser denies the request, the game continues in the tab.")
                           : L("Этот браузер не поддерживает полноэкранный режим страницы. Игра работает во вкладке.",
                               "This browser does not support fullscreen pages. The game runs in the tab."));
    const int display = screen->display;
#elif !defined(__ANDROID__)
    int displays = std::max(1, SDL_GetNumVideoDisplays());
    if (screen->display < 0 || screen->display >= displays) screen->display = 0;
    auto displayName = [](int i) {
        const char* name = SDL_GetDisplayName(i);
        std::string text = std::to_string(i + 1) + ": " + (name && *name ? name : L("Монитор", "Monitor"));
        SDL_DisplayMode desktop;
        if (SDL_GetDesktopDisplayMode(i, &desktop) == 0)
            text += " — " + modeText(desktop.w, desktop.h) + ", " + std::to_string(desktop.refresh_rate) + L(" Гц", " Hz");
        return text;
    };
    mcui::SectionTitle(L("Монитор", "Monitor"));
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::BeginDisabled(displays < 2);
    std::string currentDisplay = displayName(screen->display);
    if (ImGui::BeginCombo("##display", currentDisplay.c_str())) {
        for (int i = 0; i < displays; i++)
            if (ImGui::Selectable(displayName(i).c_str(), i == screen->display)) {
                screen->display = i;
                screen->width = screen->height = 0;
            }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    const int display = screen->display;

    mcui::SectionTitle(L("Режим окна игры", "Game window"));
#ifndef __ANDROID__
    launcherHelp(L("В игре F11 или Alt+Enter переключают окно и весь экран — выбор запоминается.",
                               "In game, F11 or Alt+Enter switch between windowed and fullscreen; the choice is kept."));
#endif
    struct Option {
        WindowMode mode;
        const char* name;
        const char* note;
    };
    const Option options[] = {
        {WindowMode::Window, L("В окне", "Windowed"), L("1024×768 в масштабе экрана; размер можно менять.",
                                                         "1024×768 at the display scale; resizable.")},
        {WindowMode::Fullscreen, L("Во весь экран", "Fullscreen"),
         L("Свой режим монитора — разрешение ниже.", "Its own monitor mode — resolution below.")},
        {WindowMode::Borderless, L("Во весь экран без рамки", "Borderless fullscreen"),
         L("Окно размером с экран: быстрее переключаться на другие окна.",
           "A screen-sized window: quicker to switch to other windows.")},
    };
    for (const Option& o : options) {
        ImGui::RadioButton(o.name, mode, int(o.mode));
        launcherHelp(o.note);
    }
    {
        SDL_DisplayMode desktop{};
        SDL_GetDesktopDisplayMode(display, &desktop);
        std::vector<std::pair<int, int>> sizes;
        for (int i = 0, n = SDL_GetNumDisplayModes(display); i < n; i++) {
            SDL_DisplayMode m;
            if (SDL_GetDisplayMode(display, i, &m) != 0) continue;
            std::pair<int, int> size(m.w, m.h);
            if (std::find(sizes.begin(), sizes.end(), size) == sizes.end()) sizes.push_back(size);
        }
        std::sort(sizes.begin(), sizes.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return a.first * a.second > b.first * b.second;
        });
        std::string desktopName = std::string(L("Как у рабочего стола — ", "Same as the desktop — ")) +
                                  modeText(desktop.w, desktop.h);
        bool isDesktop = screen->width <= 0 || (screen->width == desktop.w && screen->height == desktop.h);
        std::string current = isDesktop ? desktopName : modeText(screen->width, screen->height);
        ImGui::BeginDisabled(*mode != int(WindowMode::Fullscreen));
        ImGui::SetNextItemWidth(260 * scale);
        if (ImGui::BeginCombo(L("Разрешение", "Resolution"), current.c_str())) {
            if (ImGui::Selectable(desktopName.c_str(), isDesktop)) screen->width = screen->height = 0;
            for (const auto& size : sizes) {
                if (size.first == desktop.w && size.second == desktop.h) continue;
                bool chosen = !isDesktop && size.first == screen->width && size.second == screen->height;
                if (ImGui::Selectable(modeText(size.first, size.second).c_str(), chosen)) {
                    screen->width = size.first;
                    screen->height = size.second;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
    }
#else
    const int display = screen->display;
#endif
    ImGui::BeginDisabled(*original);
    mcui::Toggle(L("Широкий экран", "Widescreen"), wide);
    launcherHelp(L("3D на всю ширину, интерфейс — 4:3 по центру", "3D fills the width, the interface stays 4:3"));
    ImGui::EndDisabled();
#ifdef __linux__
    if (getenv("WAYLAND_DISPLAY")) {
        const char* names[] = {L("Авто", "Auto"), "Wayland", "X11 (XWayland)"};
        int& driver = screen->videoDriver;
        if (driver < 0 || driver > 2) driver = 0;
        const char* env = getenv("SDL_VIDEODRIVER");
        bool forced = env && *env;
        ImGui::BeginDisabled(forced);
        ImGui::SetNextItemWidth(260 * scale);
        if (ImGui::BeginCombo(L("Оконная система", "Window system"), names[driver])) {
            for (int i = 0; i < 3; i++)
                if (ImGui::Selectable(names[i], i == driver)) driver = i;
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        const char* current = SDL_GetCurrentVideoDriver();
        std::string note = forced ? std::string(L("задана переменной SDL_VIDEODRIVER", "set by SDL_VIDEODRIVER"))
                                  : std::string(L("сейчас — ", "now — ")) + (current ? current : "?");
        ImGui::TextWrapped("%s", note.c_str());
        if (!forced)
            launcherHelp(L("Игре — сразу, лаунчеру — со следующего запуска.",
                                        "The game switches at once, the launcher on its next start."));
    }
#endif

    ImGui::BeginDisabled(*original);
    mcui::SectionTitle(L("Частота кадров", "Frame rate"));
    launcherHelp(L("Время игра считает по часам: скорость от частоты не зависит, выше — только плавнее.",
                                "The game keeps time by the clock: speed does not depend on the rate, higher is smoother."));
    int refresh = port::platform::displayRefreshRate(display);
    std::vector<int> values = {port::platform::kFrameRateDisplay};
    std::vector<std::string> names = {std::string(L("Как у экрана — ", "Same as the display — ")) +
                                      std::to_string(refresh) + L(" Гц", " Hz")};
    for (int rate : port::platform::frameRateChoices(display)) {
        values.push_back(rate);
        names.push_back(rate == 30 ? std::string(L("30 — как на iPad", "30 — as on the iPad")) : std::to_string(rate));
    }
    values.push_back(port::platform::kFrameRateUnlimited);
    names.push_back(L("Без ограничения (без вертикальной синхронизации)", "Unlimited (no vertical sync)"));
    size_t current = 0;
    for (size_t i = 0; i < values.size(); i++)
        if (values[i] == *fps) current = i;
    *fps = values[current];
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(names.back().c_str()).x + ImGui::GetFrameHeight() * 2);
    if (ImGui::BeginCombo("##fps", names[current].c_str())) {
        for (size_t i = 0; i < values.size(); i++)
            if (ImGui::Selectable(names[i].c_str(), i == current)) *fps = values[i];
        ImGui::EndCombo();
    }
    {
        bool fixed = *fps > 0;
        bool on = *fps == port::platform::kFrameRateDisplay ? true : fixed ? screen->vsync : false;
        ImGui::BeginDisabled(!fixed);
        if (mcui::Toggle(L("Вертикальная синхронизация", "Vertical sync"), &on) && fixed) screen->vsync = on;
        ImGui::EndDisabled();
        launcherHelp(fixed ? L("без неё — разрывы кадра, зато меньше задержка", "without it — tearing, but less lag")
                                        : L("задаёт частота кадров", "set by the frame rate"));
    }

    mcui::SectionTitle(L("Увеличение кадра", "Upscaling"));
    using port::platform::Upscale;
    struct UpscaleOption {
        Upscale mode;
        const char* name;
        const char* note;
    };
    const UpscaleOption upscales[] = {
        {Upscale::Fsr, "AMD FSR 1.0", L("чётко, края без лесенки", "sharp, smooth edges")},
        {Upscale::Linear, L("Билинейное", "Bilinear"), L("мягко, как раньше", "soft, as before")},
        {Upscale::Nearest, L("По целым пикселям", "Integer pixels"), L("пиксели квадратами, поля шире", "square pixels, wider borders")},
    };
    const UpscaleOption* chosen = &upscales[0];
    for (const UpscaleOption& o : upscales)
        if (int(o.mode) == *upscale) chosen = &o;
    ImGui::SetNextItemWidth(260 * scale);
    if (ImGui::BeginCombo("##upscale", chosen->name)) {
        for (const UpscaleOption& o : upscales)
            if (ImGui::Selectable(o.name, &o == chosen)) *upscale = int(o.mode);
        ImGui::EndCombo();
    }
    launcherHelp(chosen->note);
    ImGui::BeginDisabled(*upscale != int(Upscale::Fsr));
    ImGui::SetNextItemWidth(220 * scale);
    ImGui::SliderFloat(L("Резкость", "Sharpness"), sharpness, 0.0f, 1.0f, "%.2f");
    ImGui::EndDisabled();
    struct RenderOption {
        int percent;
        const char* name;
    };
    const RenderOption renders[] = {
        {100, L("Как у окна", "Same as the window")},
        {75, "75 %"},
        {50, "50 %"},
        {0, L("Как на iPad (768×1024)", "As on the iPad (768×1024)")},
    };
    const RenderOption* render0 = &renders[0];
    for (const RenderOption& o : renders)
        if (o.percent == *render) render0 = &o;
    ImGui::BeginDisabled(*upscale == int(Upscale::Nearest));
    ImGui::SetNextItemWidth(260 * scale);
    if (ImGui::BeginCombo(L("Внутреннее разрешение", "Render resolution"), render0->name)) {
        for (const RenderOption& o : renders)
            if (ImGui::Selectable(o.name, &o == render0)) *render = o.percent;
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();

    mcui::SectionTitle(L("Качество 3D", "3D quality"));
    struct FilterOption {
        int level;
        const char* name;
    };
    const FilterOption filters[] = {
        {0, L("Как на iPad", "As on the iPad")}, {4, "4×"}, {8, "8×"}, {16, "16×"},
    };
    const FilterOption* filter0 = &filters[0];
    for (const FilterOption& o : filters)
        if (o.level == *filter) filter0 = &o;
    ImGui::SetNextItemWidth(260 * scale);
    if (ImGui::BeginCombo(L("Анизотропная фильтрация", "Anisotropic filtering"), filter0->name)) {
        for (const FilterOption& o : filters)
            if (ImGui::Selectable(o.name, &o == filter0)) *filter = o.level;
        ImGui::EndCombo();
    }
    launcherHelp(*filter ? L("крыши и стены под углом чёткие вдаль, узор не рябит",
                                          "roofs and walls at an angle stay sharp, patterns do not shimmer")
                                      : L("мягче вдаль, мелкий узор рябит на бегу", "softer at a distance, fine patterns shimmer"));
    struct MsaaOption {
        int samples;
        const char* name;
    };
    const MsaaOption msaas[] = {{0, L("Выкл.", "Off")}, {2, "2×"}, {4, "4×"}, {8, "8×"}};
    const MsaaOption* msaa0 = &msaas[0];
    for (const MsaaOption& o : msaas)
        if (o.samples == *msaa) msaa0 = &o;
    ImGui::SetNextItemWidth(260 * scale);
    if (ImGui::BeginCombo(L("Сглаживание MSAA", "MSAA anti-aliasing"), msaa0->name)) {
        for (const MsaaOption& o : msaas)
            if (ImGui::Selectable(o.name, &o == msaa0)) *msaa = o.samples;
        ImGui::EndCombo();
    }
    mcui::Toggle(L("Сглаживание TAA", "TAA anti-aliasing"), taa);
    launcherHelp(L("решётки и провода без мерцания, кадр чуть мягче", "no shimmer on grates and wires, slightly softer"));
    const char* posts[] = {L("Как на iPad", "As on the iPad"), L("Улучшенная", "Enhanced")};
    ImGui::SetNextItemWidth(260 * scale);
    ImGui::Combo(L("Постобработка", "Post-processing"), post, posts, 2);
    ImGui::BeginDisabled(*post == 0);
    ImGui::SetNextItemWidth(220 * scale);
    ImGui::SliderFloat(L("Сила", "Strength"), postStrength, 0.0f, 1.0f, "%.2f");
    ImGui::EndDisabled();
    launcherHelp(*post ? L("свечение неба и белых стен, мягкие света, цвет по LUT (<папка игрока>/lut.png)",
                                        "glow of the sky and white walls, soft highlights, LUT grading (<player folder>/lut.png)")
                                    : L("кадр как в оригинале", "the frame as in the original"));
    const char* aos[] = {L("Выкл.", "Off"), "SSAO", "HBAO", "GTAO"};
    ImGui::SetNextItemWidth(260 * scale);
    ImGui::Combo(L("Затенение углов", "Ambient occlusion"), ssao, aos, 4);
    const char* aoHints[] = {
        L("без тени в углах, как на iPad", "no shadow in corners, as on the iPad"),
        L("мягкая тень в углах и под выступами", "soft shadow in corners and under ledges"),
        L("тень по горизонтам: плотнее в углах и на стыках", "horizon-based: denser in corners and at joints"),
        L("как HBAO, с весом по нормали — ближе к эталону, чуть дороже",
          "like HBAO, cosine-weighted by the normal — closer to reference, a bit slower"),
    };
    launcherHelp(aoHints[*ssao]);
    mcui::Toggle(L("Лучи солнца", "God rays"), godRays);
    launcherHelp(L("против солнца — лучи из-за краёв зданий", "against the sun — rays from behind buildings"));
    mcui::Toggle(L("Размытие в движении", "Motion blur"), motionBlur);
    launcherHelp(L("на бегу края кадра размываются по движению", "edges of the frame blur when running"));
    const bool wine = !port::platform::nativeShaderCompiler();
    ImGui::BeginDisabled(wine);
    mcui::Toggle(L("Отражения на полах", "Floor reflections"), ssr);
    ImGui::SameLine();
    ImGui::BeginDisabled(!*ssr);
    ImGui::SetNextItemWidth(160 * scale);
    ImGui::SliderFloat(L("Сила##ssr", "Strength##ssr"), ssrStrength, 0.05f, 1.0f, "%.2f");
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (wine)
        launcherHelp(L("под Wine и Proton — только с d3dcompiler_47.dll из сборки рядом с игрой; на Linux и "
                                    "Steam Deck лучше Linux-сборка",
                                    "under Wine and Proton — only with d3dcompiler_47.dll from the build next to the game; "
                                    "on Linux and Steam Deck the Linux build is better"));
    else
        launcherHelp(L("в крышах и полах отражаются дома и бегун", "buildings and the runner reflect in roofs and floors"));
    mcui::Toggle(L("Тени от бегунов", "Runner shadows"), shadows);
    launcherHelp(L("тень героини, соперника и полиции на полу и стенах", "shadows of the heroine, rival and police"));
    mcui::Toggle(L("Блик солнца", "Lens flare"), flare);
    launcherHelp(L("ореол и блики объектива, когда солнце в кадре", "glare and lens ghosts when the sun is in view"));
    const char* grades[] = {L("Выкл.", "Off"), L("Тёплая", "Warm"), L("Холодная", "Cold"), L("Как в игре 2008 года", "Like the 2008 game")};
    ImGui::SetNextItemWidth(260 * scale);
    ImGui::Combo(L("Цветокоррекция", "Color grading"), grade, grades, 4);
    mcui::Toggle(L("Свои текстуры", "Custom textures"), pack);
    ImGui::SameLine();
    if (ImGui::SmallButton(L("Папка", "Folder"))) {
        std::string dir = port::gfx::texturesDir();
        mkdir(dir.c_str(), 0755);
        port::platform::openURL(("file://" + dir).c_str());
    }
    launcherHelp(L("PNG по имени-хэшу текстуры; оригиналы выгружает medecomp --dump-textures",
                                "PNG named by the texture hash; medecomp --dump-textures dumps the originals"));
    ImGui::EndDisabled();
    hdSection(scale);

    mcui::SectionTitle(L("Масштаб", "Scale"));
    ImGui::Text(L("Масштаб экрана: %.2f", "Display scale: %.2f"), scale);
    launcherHelp(L("— из системы; вручную — MEDECOMP_UI_SCALE.", "— from the system; by hand — MEDECOMP_UI_SCALE."));

    ImGui::Spacing();
    if (ImGui::Button(L("Сбросить настройки экрана", "Reset display settings"))) {
        namespace settings = port::platform::settings;
        for (const char* key : {"graphics", "window", "fullscreen", "fps", "upscale", "sharpness", "widescreen", "render",
                                "texture_filter", "msaa", "texture_pack", "post", "post_strength", "ssao", "god_rays", "motion_blur", "ssr", "ssr_strength", "runner_shadows", "lens_flare", "grade", "taa", "display",
                                "resolution", "vsync", "video_driver"})
            settings::erase(key);
        *original = port::platform::originalGraphics();
        *mode = int(port::platform::windowModeSetting());
        *fps = port::platform::frameRateSetting();
        *upscale = int(port::platform::upscaleSetting());
        *sharpness = port::platform::sharpnessSetting();
        *wide = port::platform::widescreenSetting();
        *render = port::platform::renderSetting();
        *filter = port::platform::textureFilterSetting();
        *msaa = port::platform::msaaSetting();
        *pack = port::gfx::texturePackSetting();
        *post = port::platform::postSetting();
        *postStrength = port::platform::postStrength();
        *ssao = port::platform::ssaoSetting();
        *godRays = port::platform::godRaysSetting();
        *motionBlur = port::platform::motionBlurSetting();
        *ssr = port::platform::ssrSetting() > 0;
        *ssrStrength = port::platform::ssrStrengthSetting();
        *shadows = port::gfx::runnerShadowsSetting();
        *flare = port::platform::lensFlareSetting();
        *grade = port::platform::gradeSetting();
        *taa = port::platform::taaSetting();
        screen->display = port::platform::displaySetting();
        screen->width = screen->height = 0;
        port::platform::resolutionSetting(&screen->width, &screen->height);
        screen->vsync = port::platform::vsyncSetting();
        screen->videoDriver = int(port::platform::videoDriverSetting());
    }
    launcherHelp(L("— как при первом запуске; сохранятся по «Играть»", "— as on the first run; saved on Play"));
}

struct Sound {
    int volume;
    bool music;
    char folder[4096];
    bool shuffle;
    int musicVolume;
    int hrtf;
    bool backgroundPause;
};

void soundTab(Sound* s, float scale) {
    mcui::SectionTitle(L("Громкость", "Volume"));
    ImGui::SetNextItemWidth(260 * scale);
    ImGui::SliderInt(L("Общая", "Master"), &s->volume, 0, 100, "%d %%");
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    launcherHelp(L("Всё вместе: звуки, музыка игры и своя. Музыку и эффекты по отдельности — в "
                               "настройках звука самой игры.",
                               "Everything together: sounds, the game's music and your own. Music and effects "
                               "separately — in the game's own sound settings."));
    ImGui::PopStyleColor();

    mcui::SectionTitle(L("Своя музыка", "Your music"));
    if (!port::audio::musicSupported()) {
        ImGui::TextWrapped("%s", L("Нужна сборка с FFmpeg.", "Needs a build with FFmpeg."));
    } else {
        mcui::Toggle(L("Играть свою музыку вместо музыки игры", "Play your music instead of the game's"), &s->music);
        ImGui::BeginDisabled(!s->music);
        const char* browse = L("Обзор…", "Browse…");
        float browseWidth = ImGui::CalcTextSize(browse).x + ImGui::GetStyle().FramePadding.x * 2;
        ImGui::SetNextItemWidth(-browseWidth - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputTextWithHint("##music", L("Папка с музыкой", "Music folder"), s->folder, sizeof s->folder);
        ImGui::SameLine();
        ImGui::BeginDisabled(port::platform::dialog::busy());
        if (ImGui::Button(browse))
            port::platform::dialog::pick(port::platform::dialog::MusicFolder, s->folder,
                                         L("Папка со своей музыкой", "Folder with your music"));
        ImGui::EndDisabled();
        static std::string counted = "\x01";
        static size_t tracks = 0;
        if (counted != s->folder) {
            counted = s->folder;
            tracks = port::audio::musicTracks(counted).size();
        }
        if (s->folder[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(tracks ? mcui::tokens::kColorPositive : mcui::tokens::kColorAttention));
            if (tracks)
                ImGui::Text(L("Треков: %zu", "Tracks: %zu"), tracks);
            else
                ImGui::TextWrapped("%s", L("В папке нет музыки (mp3, ogg, opus, flac, wav, m4a…)",
                                           "No music in the folder (mp3, ogg, opus, flac, wav, m4a…)"));
            ImGui::PopStyleColor();
        }
        mcui::Toggle(L("Вперемешку", "Shuffle"), &s->shuffle);
        ImGui::SetNextItemWidth(260 * scale);
        ImGui::SliderInt(L("Громкость музыки", "Music volume"), &s->musicVolume, 0, 100, "%d %%");
        ImGui::EndDisabled();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        launcherHelp(L("Как музыка iPod на iPad: пока играет своя, музыка игры молчит, а в настройках "
                                   "звука игры остаётся только ползунок эффектов. Треки — из папки и вложенных, по "
                                   "кругу; на паузе при сворачивании окна — вместе с игрой.",
                                   "Like iPod music on the iPad: while yours plays, the game's music is silent and "
                                   "the game's sound settings keep only the effects slider. Tracks come from the "
                                   "folder and its subfolders, in a loop; they pause with the game when the window "
                                   "is minimised."));
        ImGui::PopStyleColor();
    }

    mcui::SectionTitle(L("В фоне", "In the background"));
    mcui::Toggle(L("Пауза звука при сворачивании", "Pause sound when minimised"), &s->backgroundPause);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    launcherHelp(s->backgroundPause
                                 ? L("Как на iPad: окно свернули или переключились — игра и звук на паузе, музыка "
                                     "потом продолжается с того же места.",
                                     "As on the iPad: when the window is minimised or loses focus, the game and the "
                                     "sound pause; the music then goes on from where it stopped.")
                                 : L("Игра на паузе, а музыка и звуки играют дальше.",
                                     "The game pauses, but the music and sounds keep playing."));
    ImGui::PopStyleColor();

    mcui::SectionTitle(L("Объёмный звук в наушниках (HRTF)", "Headphone surround (HRTF)"));
    int* hrtf = &s->hrtf;
    const char* names[] = {L("Авто", "Auto"), L("Вкл", "On"), L("Выкл (как на iPad)", "Off (as on the iPad)")};
    ImGui::SetNextItemWidth(260 * scale);
    ImGui::Combo("##hrtf", hrtf, names, 3);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    launcherHelp(L("Звук со всех сторон, как в пространстве. «Авто» — включает сама OpenAL Soft, если "
                               "вывод — наушники; некоторые (часто Bluetooth) она не узнаёт — тогда «Вкл». Для "
                               "колонок — «Выкл».",
                               "Sound from every direction, as in space. Auto lets OpenAL Soft turn it on when the "
                               "output is headphones; it misses some (often Bluetooth) — then choose On. For "
                               "speakers choose Off."));
    ImGui::PopStyleColor();
}

int hrtfSetting() {
    std::string v = port::platform::settings::get("hrtf", "auto");
    return v == "on" ? 1 : v == "off" ? 2 : 0;
}

void setHrtfSetting(int index) {
    const char* values[] = {"auto", "on", "off"};
    port::platform::settings::set("hrtf", values[index >= 0 && index < 3 ? index : 0]);
}

struct AccountFlow {
    enum Stage { Checking, SignedOut, Requesting, Waiting, SignedIn };
    std::mutex mu;
    Stage stage = SignedOut;
    std::string server;
    port::online::DeviceCode code;
    port::online::Account account;
    std::string token;
    bool tokenReady = false;
    bool revoked = false;
    bool offline = false;
    std::string error;
    std::string avatar;
    bool avatarReady = false;
    bool signup = false;
    bool openSite = false;
    int attempt = 0;
};

std::shared_ptr<AccountFlow> s_account = std::make_shared<AccountFlow>();
SDL_Texture* s_avatar;

void fetchAvatar(std::shared_ptr<AccountFlow> flow, std::string url) {
    if (url.empty()) return;
    std::thread([flow, url]() {
        port::online::Response r = port::online::request("GET", url, "", {}, 15000);
        if (r.status != 200 || r.body.empty()) return;
        std::lock_guard<std::mutex> lock(flow->mu);
        flow->avatar = std::move(r.body);
        flow->avatarReady = true;
    }).detach();
}

void checkAccount(const std::string& token) {
    std::shared_ptr<AccountFlow> flow = s_account;
    flow->stage = AccountFlow::Checking;
    std::string server = flow->server;
    std::thread([flow, server, token]() {
        port::online::Account account;
        bool revoked = false;
        std::string error;
        bool ok = port::online::fetchAccount(server, token, &account, &revoked, &error);
        std::lock_guard<std::mutex> lock(flow->mu);
        if (flow->stage != AccountFlow::Checking) return;
        if (ok) {
            flow->account = account;
            flow->stage = AccountFlow::SignedIn;
            fetchAvatar(flow, account.avatar);
        } else if (revoked) {
            flow->revoked = true;
            flow->stage = AccountFlow::SignedOut;
        } else {
            flow->offline = true;
            flow->stage = AccountFlow::SignedIn;
        }
    }).detach();
}

std::string linkPage(const AccountFlow& flow) {
    return flow.code.urlWithCode + (flow.signup ? "&signup=1" : "");
}

void startSignIn(bool signup) {
    std::shared_ptr<AccountFlow> flow = s_account;
    std::string server;
    int attempt;
    {
        std::lock_guard<std::mutex> lock(flow->mu);
        flow->stage = AccountFlow::Requesting;
        flow->signup = signup;
        flow->error.clear();
        attempt = ++flow->attempt;
        server = flow->server;
    }
    std::string device = port::online::deviceName();
    std::thread([flow, server, attempt, device]() {
        port::online::DeviceCode code;
        std::string error;
        bool ok = port::online::startSignIn(server, device, &code, &error);
        {
            std::lock_guard<std::mutex> lock(flow->mu);
            if (flow->attempt != attempt) return;
            if (!ok) {
                flow->error = error;
                flow->stage = AccountFlow::SignedOut;
                return;
            }
            flow->code = code;
            flow->stage = AccountFlow::Waiting;
            flow->openSite = true;
        }
        Uint32 deadline = SDL_GetTicks() + Uint32(code.expiresIn) * 1000;
        for (;;) {
            for (int i = 0; i < code.interval * 4; i++) {
                SDL_Delay(250);
                std::lock_guard<std::mutex> lock(flow->mu);
                if (flow->attempt != attempt) return;
            }
            std::string token;
            port::online::Account account;
            port::online::SignIn result = port::online::pollSignIn(server, code.deviceCode, &token, &account, &error);
            std::lock_guard<std::mutex> lock(flow->mu);
            if (flow->attempt != attempt) return;
            if (result == port::online::SignIn::Approved) {
                flow->token = token;
                flow->account = account;
                flow->tokenReady = true;
                flow->offline = false;
                flow->stage = AccountFlow::SignedIn;
                fetchAvatar(flow, account.avatar);
                return;
            }
            if (result == port::online::SignIn::Failed || SDL_TICKS_PASSED(SDL_GetTicks(), deadline)) {
                flow->error = result == port::online::SignIn::Failed
                                  ? error
                                  : L("Код истёк — войдите снова", "The code has expired — sign in again");
                flow->stage = AccountFlow::SignedOut;
                return;
            }
        }
    }).detach();
}

void cancelSignIn() {
    std::lock_guard<std::mutex> lock(s_account->mu);
    s_account->attempt++;
    if (s_account->stage == AccountFlow::Requesting || s_account->stage == AccountFlow::Waiting)
        s_account->stage = AccountFlow::SignedOut;
}

void openUrl(const std::string& url) {
    if (getenv("MEDECOMP_LAUNCHER_SHOT")) return;
#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({ window.open(UTF8ToString($0), "_blank"); }, url.c_str());
    return;
#endif
    if (SDL_OpenURL(url.c_str()) != 0) fprintf(stderr, "medecomp: не открыть %s: %s\n", url.c_str(), SDL_GetError());
}

void hint(const char* text) { launcherHelp(text); }

void drawQr(const std::string& text, float size) {
    static std::string encoded;
    static int width = 0;
    static std::vector<bool> modules;
    if (encoded != text) {
        encoded = text;
        if (!port::online::qrCode(text, &width, &modules)) width = 0;
    }
    if (width <= 0) return;
    float cell = std::floor(size / (width + 4));
    if (cell < 1) cell = 1;
    float side = cell * (width + 4);
    ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(at, ImVec2(at.x + side, at.y + side), IM_COL32(255, 255, 255, 255));
    for (int y = 0; y < width; y++)
        for (int x = 0; x < width; x++)
            if (modules[size_t(y) * width + x]) {
                ImVec2 p(at.x + (x + 2) * cell, at.y + (y + 2) * cell);
                draw->AddRectFilled(p, ImVec2(p.x + cell, p.y + cell), IM_COL32(0, 0, 0, 255));
            }
    ImGui::Dummy(ImVec2(side, side));
}

void accountUpdate(SDL_Renderer* renderer) {
    AccountFlow& flow = *s_account;
    std::lock_guard<std::mutex> lock(flow.mu);
    if (flow.tokenReady) {
        flow.tokenReady = false;
        port::online::saveAccount(flow.token, flow.account.userName);
    }
    if (flow.openSite) {
        flow.openSite = false;
        openUrl(linkPage(flow));
    }
    if (flow.revoked) {
        flow.revoked = false;
        port::online::saveAccount("", "");
        flow.error = L("Вход отозван на сайте — войдите снова", "Signed out on the site — sign in again");
    }
    if (flow.avatarReady) {
        flow.avatarReady = false;
        int w = 0, h = 0;
        std::vector<uint8_t> rgba;
        if (port::online::decodeImage(flow.avatar, &w, &h, &rgba)) {
            if (s_avatar) SDL_DestroyTexture(s_avatar);
            s_avatar = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
            if (s_avatar) {
                SDL_UpdateTexture(s_avatar, nullptr, rgba.data(), w * 4);
                SDL_SetTextureBlendMode(s_avatar, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(s_avatar, SDL_ScaleModeLinear);
            }
        }
    }
}

bool s_cloudChecked = false;
bool s_cloudInterrupted = false;
std::string s_cloudHook;

bool cloudUpdate() {
    namespace cloud = port::online::cloud;
    AccountFlow::Stage stage;
    bool offline;
    {
        std::lock_guard<std::mutex> lock(s_account->mu);
        stage = s_account->stage;
        offline = s_account->offline;
    }
    if (stage != AccountFlow::SignedIn) {
        s_cloudChecked = false;
        s_cloudInterrupted = false;
    } else if (!offline && !s_cloudChecked) {
        s_cloudChecked = true;
        s_cloudInterrupted = cloud::interrupted();
        cloud::startSync(s_russian, false, !s_cloudInterrupted);
    }
    if (cloud::takeSkinsChanged()) port::platform::heroine::reloadSkins();
    long long here = 0, there = 0;
    if (const char* pick = getenv("MEDECOMP_CLOUD_RESOLVE"); pick && cloud::conflictPending(&here, &there))
        cloud::resolve(strcmp(pick, "account") == 0, s_russian);
    if (!cloud::applyDownloadedPrefs()) return false;
    port::platform::bindings::reload();
    port::platform::heroine::reloadSkins();
    return true;
}

void cloudSection() {
    namespace cloud = port::online::cloud;
    mcui::SectionTitle(L("Сохранения в аккаунте", "Saves in the account"));
    bool on = cloud::enabled();
    if (mcui::Toggle(L("Хранить прогресс, скины, уровни и настройки в аккаунте",
                          "Keep the progress, skins, levels and settings in the account"), &on)) {
        cloud::setEnabled(on);
        if (on) cloud::startSync(s_russian);
    }
    if (cloud::busy()) {
        int done = 0, total = 0;
        cloud::progress(&done, &total);
        ImGui::TextDisabled("%s %d/%d", L("Сверяю с аккаунтом…", "Syncing with the account…"), done, total);
    } else if (!cloud::status().empty()) {
        ImGui::TextWrapped("%s", cloud::status().c_str());
    }
    ImGui::BeginDisabled(!on);
    bool skins = cloud::skinsEnabled();
    bool turnOn = s_cloudHook == "skins-on" && !skins;
    if (turnOn) s_cloudHook.clear();
    if (mcui::Toggle(L("Синхронизировать свои скины", "Sync custom skins"), &skins) || turnOn) {
        cloud::setSkinsEnabled(skins || turnOn);
        if (skins || turnOn) cloud::startSync(s_russian);
    }
    ImGui::EndDisabled();
    hint((std::string(L("В облаке — до ", "The cloud holds up to ")) + std::to_string(cloud::kSkinLimit) +
          L(" своих скинов, остальные остаются только на этом компьютере. Какие отправить и какие убрать из облака — "
            "во вкладке «Героиня».",
            " custom skins, the rest stay on this computer only. Choose which ones to send or remove from the cloud "
            "on the Heroine tab."))
             .c_str());
    ImGui::BeginDisabled(!on || cloud::busy());
    if (ImGui::Button(L("Синхронизировать сейчас", "Sync now"))) {
        s_cloudInterrupted = false;
        cloud::startSync(s_russian);
    }
    ImGui::EndDisabled();
    hint(L("Прогресс (главы, время, призраки, ачивки), свои скины и уровни, язык, звук, управление и выбранный скин "
           "сверяются с аккаунтом при входе и после игры: с другого компьютера после входа игра продолжится с того же "
           "места. Экран и графика у каждого компьютера свои. Прогресс разошёлся — лаунчер спросит, какой оставить; "
           "другой остаётся копией в папке игрока, в backups. Своих скинов, пока они синхронизируются, — не больше 10: "
           "модели и текстуры тяжёлые; выключите синхронизацию скинов — их сколько угодно, но только на этом компьютере.",
           "The progress (chapters, times, ghosts, achievements), custom skins and levels, language, sound, controls and "
           "the chosen skin are synced with the account on sign-in and after playing: on another computer the game goes on "
           "where you left off once you sign in. Display and graphics stay per computer. When the progress differs, the "
           "launcher asks which one to keep; the other stays as a copy in the player folder, under backups. While custom "
           "skins sync, there can be at most 10 of them: models and textures are heavy; turn skin sync off to keep as many "
           "as you like, on this computer only."));
}

struct AchievementsView {
    std::mutex mu;
    bool loading = false;
    bool loaded = false;
    std::map<std::string, port::achievements::Remote> remote;
    std::string error;
};
std::shared_ptr<AchievementsView> s_achievementsView = std::make_shared<AchievementsView>();
std::vector<mcui::Icon> s_achievementIcons;
const char* const kAchievementsPage = "https://mooncellar.space/games/mirrors-edge";

void refreshAchievements() {
    std::shared_ptr<AchievementsView> view = s_achievementsView;
    {
        std::lock_guard<std::mutex> lock(view->mu);
        if (view->loading) return;
        view->loading = true;
    }
    const std::string server = port::online::serverUrl(), token = port::online::token();
    const std::string account = token.empty() ? std::string() : port::online::userName();
    std::thread([view, server, token, account] {
        namespace ach = port::achievements;
        std::map<std::string, ach::Remote> remote;
        std::string error;
        bool ok = false;
        if (!token.empty()) {
            ach::queueFromSave(account);
            ach::flushAsync(server, token, account);
            for (int i = 0; i < 300 && ach::flushing(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ok = ach::fetchMine(server, token, &remote, &error);
        }
        std::lock_guard<std::mutex> lock(view->mu);
        view->loading = false;
        view->loaded = true;
        if (ok) view->remote = remote;
        view->error = token.empty() || ok ? std::string() : error;
    }).detach();
}

bool s_skeletonShown = false;

ImU32 skeletonColor() {
    const float k = 0.5f + 0.5f * sinf(float(ImGui::GetTime()) * 6.2832f / 1.4f);
    return mcui::Packed(mcui::tokens::kColorBgHover, 0.45f + 0.35f * k);
}

void skeletonBar(float w, float h) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), skeletonColor(), h / 2);
    ImGui::Dummy(ImVec2(w, h));
}

void skeletonCard(const std::string& title, const std::string& description, bool progress, bool status) {
    using namespace mcui::tokens;
    const float s = mcui::Scale();
    const mcui::Fonts& fonts = mcui::GetFonts();
    const float width = ImGui::GetContentRegionAvail().x;
    const float pad = kPaddingX3 * s, tile = 48.0f * s, gap = kGapX3 * s;
    const float textX = pad + tile + gap, wrap = std::max(1.0f, width - textX - pad);
    const float titleSize = mcui::kTextBody * s, bodySize = mcui::kTextCaption * s;
    ImFont* bold = fonts.bold ? fonts.bold : ImGui::GetFont();
    ImFont* regular = fonts.regular ? fonts.regular : ImGui::GetFont();
    const float th = bold->CalcTextSizeA(titleSize, FLT_MAX, wrap, title.c_str()).y;
    const float dh = regular->CalcTextSizeA(bodySize, FLT_MAX, wrap, description.c_str()).y;
    const float line = regular->CalcTextSizeA(bodySize, FLT_MAX, 0.0f, "Ag").y;
    const float h = std::max(th + kGapX1 * s + dh + (progress ? kGapX2 * s + 6.0f * s : 0.0f) + (status ? kGapX2 * s + line : 0.0f), tile) +
                    pad * 2;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 c = skeletonColor();
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), mcui::Packed(kColorBgTertiary), kRadiusX3 * s);
    dl->AddRectFilled(ImVec2(p.x + pad, p.y + pad), ImVec2(p.x + pad + tile, p.y + pad + tile), c, kRadiusX3 * s);
    auto bar = [&](float y, float w, float hh) {
        dl->AddRectFilled(ImVec2(p.x + textX, y + hh * 0.2f), ImVec2(p.x + textX + w, y + hh * 0.8f), c, hh * 0.3f);
    };
    float y = p.y + pad;
    bar(y, wrap * 0.55f, th);
    y += th + kGapX1 * s;
    const int lines = std::max(1, int(dh / line + 0.5f));
    for (int i = 0; i < lines; i++, y += line) bar(y, wrap * (i + 1 == lines ? 0.6f : 0.92f), line);
    if (progress) {
        y += kGapX2 * s;
        dl->AddRectFilled(ImVec2(p.x + textX, y), ImVec2(p.x + textX + wrap, y + 6.0f * s), c, kRadiusX1 * s);
        y += 6.0f * s;
    }
    if (status) bar(y + kGapX2 * s, wrap * 0.3f, line);
    ImGui::Dummy(ImVec2(width, h));
}

std::string achievementDate(const std::string& iso) {
    if (iso.size() < 10) return iso;
    return iso.substr(8, 2) + "." + iso.substr(5, 2) + "." + iso.substr(0, 4);
}

void achievementsTab() {
    namespace ach = port::achievements;
    const std::vector<ach::Entry>& list = ach::catalog();
    mcui::SectionTitle(L("Ачивки", "Achievements"));
    if (list.empty()) {
        ImGui::TextWrapped("%s", L("Каталога ачивок нет рядом с программой (config/achievements).",
               "The achievement catalog is missing next to the program (config/achievements)."));
        return;
    }
    if (s_achievementIcons.size() != list.size()) {
        s_achievementIcons.clear();
        for (const ach::Entry& e : list) s_achievementIcons.push_back(mcui::ParseIcon(e.icon));
        refreshAchievements();
    }
    static ach::Local local;
    static Uint32 localAt = 0;
    if (!localAt || SDL_GetTicks() - localAt > 2000) {
        local = ach::readLocal();
        localAt = SDL_GetTicks() | 1;
    }
    std::map<std::string, ach::Remote> remote;
    std::string error;
    bool loading, loaded;
    {
        std::lock_guard<std::mutex> lock(s_achievementsView->mu);
        remote = s_achievementsView->remote;
        error = s_achievementsView->error;
        loading = s_achievementsView->loading;
        loaded = s_achievementsView->loaded;
    }
    const bool signedIn = !port::online::token().empty();
    int unlocked = 0, inAccount = 0;
    std::vector<bool> got(list.size());
    for (size_t i = 0; i < list.size(); i++) {
        const ach::Entry& e = list[i];
        auto r = remote.find(e.key);
        bool there = r != remote.end() && r->second.unlocked;
        bool here = e.index >= 0 && size_t(e.index) < local.unlocked.size() && local.unlocked[size_t(e.index)];
        got[i] = there || here;
        unlocked += got[i];
        inAccount += there;
    }
    if (signedIn && !loaded) {
        s_skeletonShown = true;
        const float w = ImGui::GetContentRegionAvail().x, s = mcui::Scale();
        skeletonBar(std::min(w, 200 * s), 16 * s);
        skeletonBar(w, 6 * s);
        ImGui::TextDisabled("%s", L("Сверяю с аккаунтом MoonCellar…", "Syncing with the MoonCellar account…"));
        ImGui::BeginDisabled();
        mcui::Button(L("Синхронизировать", "Sync"));
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (mcui::Button(L("Страница игры на MoonCellar", "Game page on MoonCellar"))) openUrl(kAchievementsPage);
        ImGui::Spacing();
        if (ImGui::BeginTable("##achievements", 2, ImGuiTableFlags_SizingStretchSame)) {
            for (const ach::Entry& e : list) {
                ImGui::TableNextColumn();
                const size_t i = size_t(&e - list.data());
                skeletonCard(e.titleIn(s_russian), e.descriptionIn(s_russian), e.target > 0 && !got[i] && ach::currentOf(e, local) >= 0,
                             got[i] || (e.target > 0 && ach::currentOf(e, local) >= 0));
            }
            ImGui::EndTable();
        }
        return;
    }
    const int total = int(list.size());
    char line[160];
    snprintf(line, sizeof line, L("Получено %d из %d · %d%%", "Unlocked %d of %d · %d%%"), unlocked, total,
             total ? unlocked * 100 / total : 0);
    ImGui::PushFont(mcui::GetFonts().medium, 0.0f);
    ImGui::TextUnformatted(line);
    ImGui::PopFont();
    mcui::ProgressBar(total ? float(unlocked) / float(total) : 0.0f);
    if (!signedIn) {
        hint(L("Войдите на вкладке «Аккаунт» — ачивки попадут в аккаунт MoonCellar и на его сайт.",
               "Sign in on the Account tab and your achievements go to your MoonCellar account and its site."));
    } else if (loading || ach::flushing()) {
        ImGui::TextDisabled("%s", L("Сверяю с аккаунтом MoonCellar…", "Syncing with the MoonCellar account…"));
    } else if (!error.empty()) {
        ImGui::TextColored(mcui::Color(mcui::tokens::kColorNegative), "%s", error.c_str());
    } else {
        int waiting = ach::pending();
        if (waiting)
            snprintf(line, sizeof line, L("В аккаунте MoonCellar: %d из %d, ждут отправки: %d",
                                          "In the MoonCellar account: %d of %d, waiting to be sent: %d"),
                     inAccount, total, waiting);
        else
            snprintf(line, sizeof line, L("В аккаунте MoonCellar: %d из %d", "In the MoonCellar account: %d of %d"),
                     inAccount, total);
        ImGui::TextDisabled("%s", line);
    }
    ImGui::BeginDisabled(!signedIn || loading);
    if (mcui::Button(L("Синхронизировать", "Sync"))) refreshAchievements();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (mcui::Button(L("Страница игры на MoonCellar", "Game page on MoonCellar"))) openUrl(kAchievementsPage);
    ImGui::Spacing();
    if (ImGui::BeginTable("##achievements", 2, ImGuiTableFlags_SizingStretchSame)) {
        const bool ru = s_russian;
        for (size_t i = 0; i < list.size(); i++) {
            const ach::Entry& e = list[i];
            ImGui::TableNextColumn();
            mcui::AchievementView v;
            v.title = e.titleIn(ru);
            v.description = e.descriptionIn(ru);
            v.icon = &s_achievementIcons[i];
            v.unlocked = got[i];
            auto r = remote.find(e.key);
            if (got[i]) {
                if (r != remote.end() && r->second.unlocked)
                    v.status = std::string(L("Получена ", "Unlocked ")) + achievementDate(r->second.unlockedAt);
                else
                    v.status = signedIn ? L("Получена, ждёт отправки", "Unlocked, waiting to be sent") : L("Получена", "Unlocked");
            } else if (e.target > 0) {
                int current = std::max(ach::currentOf(e, local), r != remote.end() ? r->second.current : -1);
                if (current >= 0) {
                    v.current = std::min(current, e.target);
                    v.target = e.target;
                    snprintf(line, sizeof line, "%d / %d%s", std::min(current, e.target), e.target,
                             e.unit == "m" ? L(" м", " m") : e.unit == "s" ? L(" с", " s") : "");
                    v.status = line;
                }
            }
            mcui::AchievementCard(v);
        }
        ImGui::EndTable();
    }
}

#ifdef MEDECOMP_ONE_PROCESS
bool s_editorPending = false;
std::string s_editorId;
#endif
struct LevelPublish {
    std::mutex mu;
    bool busy = false, done = false;
    bool pushed = true;
    std::string message;
};
LevelPublish s_publish;
enum class LevelAsk { None, Unpublish, Take, Delete };
LevelAsk s_ask = LevelAsk::None;
std::string s_askId;
std::string s_openMenu;
struct LevelCheck {
    std::mutex mu;
    bool busy = false;
    int changed = 0;
    std::string key;
};
LevelCheck s_levelCheck;
std::string s_fileMessage;
bool s_showLevels = false;

void exportLevel(const std::string& id) {
    const std::string name = id + port::platform::kLevelFileExtension;
    std::string error;
#ifdef __ANDROID__
    const std::string path = port::files::dataDir() + "/cache/" + name;
    std::string shown;
    if (port::platform::exportLevelFile(id, path, &error)) {
        shown = port::platform::androidSaveToDownloads(path, name, "application/octet-stream");
        if (shown.empty()) {
            const char* external = SDL_AndroidGetExternalStoragePath();
            shown = std::string(external ? external : port::files::dataDir().c_str()) + "/" + name;
            if (!port::platform::exportLevelFile(id, shown, &error)) shown.clear();
        }
        remove(path.c_str());
    }
#else
    const std::string dir = port::files::dataDir() + "/exports", shown = dir + "/" + name;
    mkdir(dir.c_str(), 0755);
    if (!port::platform::exportLevelFile(id, shown, &error)) error = error.empty() ? "?" : error;
    else port::platform::openURL(("file://" + dir).c_str());
#endif
    if (!error.empty() || shown.empty()) {
        s_fileMessage = std::string(L("Не удалось экспортировать", "Could not export the level")) + (s_russian ? ": " + error : "");
        return;
    }
    s_fileMessage = std::string(L("Файл уровня — ", "The level file is ")) + shown +
                    L(". На другом устройстве — «Импорт уровня…» во вкладке «Уровни».",
                      ". On another device use Import level… on the Levels tab.");
}

void removeLevel(const std::string& id) {
    const std::string dir = port::files::dataDir() + "/levels/" + id;
    for (const std::string& name : port::files::list(dir)) remove((dir + "/" + name).c_str());
    if (rmdir(dir.c_str()) != 0) {
        s_fileMessage = std::string(L("Не удалось удалить ", "Could not delete ")) + dir;
        fprintf(stderr, "medecomp: не удалить %s\n", dir.c_str());
        return;
    }
    s_fileMessage = std::string(L("Уровень удалён: ", "Deleted the level ")) + id;
    fprintf(stderr, "medecomp: уровень %s удалён\n", id.c_str());
    port::online::cloud::syncLevels();
}

void importLevel(const std::string& path) {
    std::string id, name, error;
    if (!port::platform::importLevelFile(path, &id, &name, &error)) {
        s_fileMessage = std::string(L("Не удалось импортировать", "Could not import the level")) + (s_russian ? ": " + error : "");
        fprintf(stderr, "medecomp: импорт уровня %s: %s\n", path.c_str(), error.c_str());
        return;
    }
    s_fileMessage = std::string(L("Уровень «", "Imported \"")) + name + L("» добавлен — ", "\" as ") + id;
    fprintf(stderr, "medecomp: импорт уровня %s -> levels/%s\n", path.c_str(), id.c_str());
    port::online::cloud::syncLevels();
}

void publishLevel(const std::string& id, const std::string& serverId, bool update) {
    {
        std::lock_guard<std::mutex> lock(s_publish.mu);
        if (s_publish.busy) return;
        s_publish.busy = true;
        s_publish.done = false;
        s_publish.message.clear();
    }
    std::thread([id, serverId, update] {
        std::string error;
        bool remove = !serverId.empty();
        bool ok = remove ? port::online::deleteLevel(serverId, &error) : port::online::uploadLevel(id, &error);
        std::string message = ok ? (remove ? L("Уровень снят с сервера", "The level is no longer online")
                                           : update ? L("Новая версия уровня на сервере", "The new version is online")
                                                    : L("Уровень выложен", "The level is online"))
                                 : (remove ? L("Не удалось снять с сервера", "Could not unpublish the level")
                                           : L("Не удалось выложить", "Could not upload the level"));
        if (!ok && s_russian && !error.empty()) message += ": " + error;
        std::lock_guard<std::mutex> lock(s_publish.mu);
        s_publish.busy = false;
        s_publish.done = true;
        s_publish.pushed = !ok;
        s_publish.message = message;
    }).detach();
}

void takeFromServer(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(s_publish.mu);
        if (s_publish.busy) return;
        s_publish.busy = true;
        s_publish.done = false;
        s_publish.message.clear();
    }
    std::thread([id] {
        std::string error;
        bool ok = port::online::takeLevel(id, &error);
        std::string message = ok ? L("Уровень взят с сервера", "Took the level from the server")
                                 : L("Не удалось взять с сервера", "Could not take the level from the server");
        if (!ok && s_russian && !error.empty()) message += ": " + error;
        std::lock_guard<std::mutex> lock(s_publish.mu);
        s_publish.busy = false;
        s_publish.done = true;
        s_publish.pushed = !ok;
        s_publish.message = message;
    }).detach();
}

void checkLevels(bool signedIn, bool busy) {
    namespace cloud = port::online::cloud;
    std::string key = signedIn ? "in:" + port::online::userName() : "out";
    {
        std::lock_guard<std::mutex> lock(s_publish.mu);
        if (s_publish.done && !s_publish.pushed) s_publish.pushed = true, cloud::syncLevels();
    }
    std::lock_guard<std::mutex> lock(s_levelCheck.mu);
    if (s_levelCheck.changed > 0) s_levelCheck.changed = 0, cloud::syncLevels();
    if (s_levelCheck.busy || s_levelCheck.key == key || cloud::busy() || busy || (signedIn && cloud::allowed() && !s_cloudChecked)) return;
    s_levelCheck.key = key;
    s_levelCheck.busy = true;
    std::thread([] {
        int changed = 0;
        std::string error;
        if (!port::online::syncUploaded(&changed, &error))
            fprintf(stderr, "medecomp: сверка своих уровней с сервером: %s\n", error.c_str());
        std::lock_guard<std::mutex> lock(s_levelCheck.mu);
        s_levelCheck.busy = false;
        s_levelCheck.changed += changed;
    }).detach();
}

struct LevelChip {
    const char* text;
    unsigned color;
    const char* hint;
};
LevelChip levelChip(const std::string& origin, const std::string& palette, bool online, const port::online::OnlineState& state) {
    namespace tokens = mcui::tokens;
    using port::online::Online;
    if (online && state.state == Online::OtherAccount)
        return {L("другой аккаунт", "other account"), tokens::kColorGray,
                L("выложен другим аккаунтом: обновить и снять — после входа им", "uploaded from another account: sign in with it to update or unpublish")};
    if (online && state.state == Online::Missing)
        return {L("снят или скрыт", "removed or hidden"), tokens::kColorAttention,
                L("на сервере его нет: сняли с другого устройства или скрыли по жалобам", "not on the server: unpublished elsewhere or hidden after reports")};
    if (online && state.hidden)
        return {L("скрыт по жалобам", "hidden after reports"), tokens::kColorAttention,
                L("на сервере, но скрыт, пока его не посмотрит модератор", "online, but hidden until a moderator looks at it")};
    if (online && state.state == Online::Differs && state.newerThere)
        return {L("на сервере новее", "newer online"), tokens::kColorAccent,
                L("на сервере версия новее этой: «Взять с сервера» в меню ⋯", "the online version is newer: Take from server in the ⋯ menu")};
    if (online && state.state == Online::Differs)
        return {L("изменён здесь", "changed here"), tokens::kColorAccent,
                L("правки ещё не на сервере: «Обновить на сервере»", "the edits are not online yet: Update online")};
    if (online) return {L("на сервере", "online"), tokens::kColorPositive, nullptr};
    if (origin == "online") return {L("скачан", "downloaded"), tokens::kColorAccent, L("чужой уровень с сервера", "someone's level from the server")};
    if (origin == "chapter") {
        const bool arena = palette.compare(0, 12, "multiplayer_") == 0;
        return {arena ? L("копия арены", "arena copy") : L("копия главы", "chapter copy"), tokens::kColorAttention,
                L("несёт разметку EA — остаётся на этом устройстве, не выкладывается", "carries EA's layout — stays on this device, can't be uploaded")};
    }
    if (origin == "peer")
        return {L("от напарника", "from a partner"), tokens::kColorAttention, L("пришёл по сети — не выкладывается", "came over the network — can't be uploaded")};
    return {L("не выложен", "not uploaded"), tokens::kColorGray, nullptr};
}

void drawMoreDots(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float scale) {
    const ImVec2 c((a.x + b.x) / 2, (a.y + b.y) / 2);
    for (int i = -1; i <= 1; i++) dl->AddCircleFilled(ImVec2(c.x + float(i) * 6.0f * scale, c.y), 1.8f * scale, color);
}

bool publishBusy() {
    std::lock_guard<std::mutex> lock(s_publish.mu);
    return s_publish.busy;
}

void levelsTab(const char* game) {
    static std::string message;
    auto edit = [&](const std::string& id) {
#ifdef MEDECOMP_ONE_PROCESS
        s_editorPending = true;
        s_editorId = id;
        return;
#endif
        std::vector<std::string> args = {"--editor"};
        if (!id.empty()) args.push_back(id);
        if (*game) args.insert(args.end(), {"--game", game});
        message = port::editor::launch(args) < 0 ? L("редактор не запустился", "the editor did not start") : "";
    };
    std::string dir = port::files::dataDir() + "/levels";
    std::vector<std::string> ids;
    for (const std::string& id : port::files::list(dir))
        if (port::files::stat(dir + "/" + id + "/level.json").exists) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    const bool signedIn = !port::online::token().empty();
#ifdef __ANDROID__
    std::string help = L("Редактор открывается вместо лаунчера; играть — в игре: EXTRAS, PLAYER LEVELS. Уровень одним файлом "
                               "(.glevel) — «Экспорт в файл» в меню ⋯ (в «Загрузки»), «Импорт уровня…» — с этого устройства.",
                               "The editor opens in place of the launcher; play in the game: EXTRAS, PLAYER LEVELS. A level as one file "
                               "(.glevel): Export to a file in the ⋯ menu (to Downloads), Import level… from this device.");
#elif defined(__EMSCRIPTEN__)
    std::string help = L("Редактор открывается вместо лаунчера; играть — в игре: EXTRAS, PLAYER LEVELS.",
                               "The editor opens in place of the launcher; play in the game: EXTRAS, PLAYER LEVELS.");
#else
    std::string help = L("Редактор открывается отдельным окном; играть — в игре: EXTRAS, PLAYER LEVELS. Уровень одним файлом "
                               "(.glevel) — «Экспорт в файл» в меню ⋯, «Импорт уровня…» или перетащите файл на окно.",
                               "The editor opens in its own window; play in the game: EXTRAS, PLAYER LEVELS. A level as one file "
                               "(.glevel): Export to a file in the ⋯ menu, Import level… or drop the file on this window.");
#endif
    if (!signedIn) {
        help += "\n\n";
        help += L("Выкладывать и снимать с сервера — после входа во вкладке «Аккаунт».",
                  "Sign in on the Account tab to upload or unpublish.");
    }
    help += "\n\n";
    help += L("Папка уровней:\n", "Levels folder:\n");
    help += dir;
    const float scale = mcui::Scale();
    {
        mcui::SectionTitle(L("Свои уровни", "My levels"));
        const float titleY = ImGui::GetItemRectMin().y, titleH = ImGui::GetItemRectSize().y;
        ImGui::SameLine();
        ImGui::PushFont(nullptr, mcui::kTextBody + 1.0f);
        ImGui::TextDisabled("· %zu", ids.size());
        ImGui::PopFont();
        ImGui::SameLine();
        port::platform::launcherHelpContent("levels-help", [&] { ImGui::TextWrapped("%s", help.c_str()); }, false);
        const float headingRight = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;
        const char* importLabel = L("Импорт уровня…", "Import level…");
        const char* createLabel = L("Создать уровень", "Create level");
        const ImGuiStyle& style = ImGui::GetStyle();
        const float both = ImGui::CalcTextSize(importLabel).x + ImGui::CalcTextSize(createLabel).x + 4 * style.FramePadding.x + style.ItemSpacing.x;
        const bool sameRow = headingRight + style.ItemSpacing.x + both <= ImGui::GetContentRegionMax().x;
        if (sameRow) ImGui::SameLine(ImGui::GetContentRegionMax().x - both);
        const float buttonsY = sameRow ? titleY + (titleH - ImGui::GetFrameHeight()) / 2 : ImGui::GetCursorScreenPos().y;
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, buttonsY));
        ImGui::BeginDisabled(port::platform::dialog::busy());
        if (mcui::Button(importLabel)) port::platform::dialog::pick(port::platform::dialog::LevelFile, std::string(), L("Файл уровня", "Level file"));
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, buttonsY));
        if (mcui::Button(createLabel, mcui::ButtonKind::Accent)) edit("");
    }
    if (static bool once = false; !once) {
        once = true;
        if (const char* test = getenv("MEDECOMP_LAUNCHER_LEVEL")) {
            std::string what(test);
            std::string id = what.substr(what.find(':') + 1);
            if (what.compare(0, 7, "upload:") == 0) publishLevel(id, "", !port::online::uploadedId(id).empty());
            if (what.compare(0, 10, "unpublish:") == 0) publishLevel(id, port::online::uploadedId(id), false);
            if (what.compare(0, 8, "confirm:") == 0) s_ask = LevelAsk::Unpublish, s_askId = id;
            if (what.compare(0, 5, "take:") == 0) takeFromServer(id);
            if (what.compare(0, 7, "export:") == 0) exportLevel(id);
            if (what.compare(0, 7, "import:") == 0) importLevel(id);
            if (what.compare(0, 7, "delete:") == 0) removeLevel(id);
            if (what.compare(0, 14, "deleteconfirm:") == 0) s_ask = LevelAsk::Delete, s_askId = id;
            if (what.compare(0, 12, "takeconfirm:") == 0) s_ask = LevelAsk::Take, s_askId = id;
            if (what.compare(0, 5, "menu:") == 0) s_openMenu = id;
        }
    }
    bool busy = publishBusy();
    if (!ids.empty()) checkLevels(signedIn, busy);
    if (ids.empty()) ImGui::TextDisabled("%s", L("пока нет", "none yet"));
    if (!ids.empty()) {
        struct Row {
            std::string id, name, sub, origin, serverId;
            LevelChip chip;
            port::online::OnlineState state;
            bool uploadable, exportable;
        };
        std::vector<Row> rows;
        float chipW = 0;
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        for (const std::string& id : ids) {
            std::vector<unsigned char> text;
            port::online::Json info;
            if (port::files::read(dir + "/" + id + "/level.json", &text)) port::online::parseJson(std::string(text.begin(), text.end()), &info);
            Row r;
            r.id = id;
            r.name = info["name"].str(id);
            r.sub = id + " · " + (info["mode"].str() == "bags" ? L("сбор сумок", "bags") : L("гонка", "race"));
            r.origin = info["origin"].str();
            r.serverId = port::online::uploadedId(id);
            r.state = port::online::onlineState(id);
            r.chip = levelChip(r.origin, info["palette"].str(), !r.serverId.empty(), r.state);
            r.uploadable = port::online::uploadable(id);
            r.exportable = port::platform::levelExportable(id);
            chipW = std::max(chipW, ImGui::CalcTextSize(r.chip.text).x);
            rows.push_back(r);
        }
        ImGui::PopFont();
        const ImGuiStyle& style = ImGui::GetStyle();
        const float pad = style.FramePadding.x, gap = 8 * scale, rowH = 54 * scale, btnH = 34 * scale, more = 34 * scale;
        const float editW = ImGui::CalcTextSize(L("Изменить", "Edit")).x + 2 * pad;
        const float mainW = std::max({ImGui::CalcTextSize(L("Выложить", "Upload")).x, ImGui::CalcTextSize(L("Обновить на сервере", "Update online")).x,
                                      ImGui::CalcTextSize(L("только здесь", "local only")).x}) + 2 * pad;
        chipW += 20 * scale;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (Row& r : rows) {
            ImGui::PushID(r.id.c_str());
            const float w = ImGui::GetContentRegionAvail().x;
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float right = editW + mainW + more + 2 * gap + 10 * scale;
            const float textW = std::max(40 * scale, w - right - chipW - 18 * scale - 2 * 16 * scale);
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + rowH), mcui::Packed(mcui::tokens::kColorBgSecondary), 12 * scale);
            dl->PushClipRect(ImVec2(p.x + 18 * scale, p.y), ImVec2(p.x + 18 * scale + textW, p.y + rowH), true);
            ImGui::PushFont(mcui::GetFonts().medium, 0.0f);
            dl->AddText(ImVec2(p.x + 18 * scale, p.y + 9 * scale), mcui::Packed(mcui::tokens::kColorTextPrimary), r.name.c_str());
            ImGui::PopFont();
            ImGui::PushFont(nullptr, mcui::kTextCaption);
            dl->AddText(ImVec2(p.x + 18 * scale, p.y + 30 * scale), mcui::Packed(mcui::tokens::kColorTextMuted), r.sub.c_str());
            dl->PopClipRect();
            const ImVec2 ts = ImGui::CalcTextSize(r.chip.text);
            const ImVec2 c0(p.x + 18 * scale + textW + 16 * scale, p.y + (rowH - 24 * scale) / 2);
            const ImVec2 c1(c0.x + ts.x + 20 * scale, c0.y + 24 * scale);
            dl->AddRectFilled(c0, c1, mcui::Packed(r.chip.color, 0.14f), 12 * scale);
            dl->AddText(ImVec2(c0.x + 10 * scale, c0.y + (24 * scale - ts.y) / 2), mcui::Packed(r.chip.color), r.chip.text);
            ImGui::PopFont();
            if (r.chip.hint) {
                ImGui::SetCursorScreenPos(c0);
                ImGui::InvisibleButton("##chip", ImVec2(c1.x - c0.x, c1.y - c0.y));
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", r.chip.hint);
            }
            float x = p.x + w - right + 10 * scale - 10 * scale;
            const float y = p.y + (rowH - btnH) / 2;
            ImGui::SetCursorScreenPos(ImVec2(x, y));
            if (mcui::Button(L("Изменить", "Edit"), mcui::ButtonKind::Default, ImVec2(editW, btnH))) edit(r.id);
            x += editW + gap;
            ImGui::SetCursorScreenPos(ImVec2(x, y));
            if (r.uploadable) {
                ImGui::BeginDisabled(!signedIn || busy);
                if (mcui::Button(r.serverId.empty() ? L("Выложить", "Upload") : L("Обновить на сервере", "Update online"), mcui::ButtonKind::Default,
                                 ImVec2(mainW, btnH)))
                    publishLevel(r.id, "", !r.serverId.empty());
                ImGui::EndDisabled();
                if (!signedIn && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("%s", L("выкладывать — после входа во вкладке «Аккаунт»", "sign in on the Account tab to upload"));
            } else {
                const char* none = r.origin == "chapter" || r.origin == "peer" ? L("только здесь", "local only") : "—";
                const ImVec2 ns = ImGui::CalcTextSize(none);
                dl->AddText(ImVec2(x + (mainW - ns.x) / 2, p.y + (rowH - ns.y) / 2), mcui::Packed(mcui::tokens::kColorTextMuted), none);
            }
            x += mainW + gap;
            ImGui::SetCursorScreenPos(ImVec2(x, y));
            if (ImGui::InvisibleButton("##more", ImVec2(more, btnH)) || s_openMenu == r.id) {
                s_openMenu.clear();
                ImGui::OpenPopup("##levelmenu");
            }
            const bool hovered = ImGui::IsItemHovered();
            if (hovered) {
                dl->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), mcui::Packed(mcui::tokens::kColorBgHover), btnH / 2);
                ImGui::SetTooltip("%s", L("ещё: снять с сервера, взять с сервера, экспорт, удалить", "more: unpublish, take from server, export, delete"));
            }
            drawMoreDots(dl, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), mcui::Packed(mcui::tokens::kColorTextMuted), scale);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y + 4 * scale), ImGuiCond_Appearing, ImVec2(1, 0));
            if (ImGui::BeginPopup("##levelmenu")) {
                using port::online::Online;
                if (r.state.state == Online::Differs && ImGui::MenuItem(L("Взять с сервера…", "Take from server…"), nullptr, false, !busy))
                    s_ask = LevelAsk::Take, s_askId = r.id;
                if (!r.serverId.empty() && r.state.state != Online::OtherAccount &&
                    ImGui::MenuItem(L("Снять с сервера…", "Unpublish…"), nullptr, false, signedIn && !busy))
                    s_ask = LevelAsk::Unpublish, s_askId = r.id;
                if (r.exportable && ImGui::MenuItem(L("Экспорт в файл", "Export to a file"))) exportLevel(r.id);
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorNegative));
                if (ImGui::MenuItem(L("Удалить…", "Delete…"))) s_ask = LevelAsk::Delete, s_askId = r.id;
                ImGui::PopStyleColor();
                ImGui::EndPopup();
            }
            ImGui::SetCursorScreenPos(p);
            ImGui::Dummy(ImVec2(w, rowH));
            ImGui::PopID();
        }
    }
    if (s_ask != LevelAsk::None && !ImGui::IsPopupOpen("##levelask")) ImGui::OpenPopup("##levelask");
    if (ImGui::BeginPopupModal("##levelask", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
        std::vector<unsigned char> text;
        port::online::Json info;
        if (port::files::read(dir + "/" + s_askId + "/level.json", &text)) port::online::parseJson(std::string(text.begin(), text.end()), &info);
        const std::string name = info["name"].str(s_askId);
        const bool online = !port::online::uploadedId(s_askId).empty();
        std::string title, body;
        const char* yes = "";
        bool danger = false;
        switch (s_ask) {
        case LevelAsk::Unpublish:
            title = std::string(L("Снять «", "Unpublish \"")) + name + L("» с сервера?", "\"?");
            body = L("Его таблица и оценки на сервере пропадут, копия на этом устройстве останется.",
                     "Its leaderboard and ratings online go away; the copy on this device stays.");
            yes = L("Снять с сервера", "Unpublish");
            break;
        case LevelAsk::Take:
            title = std::string(L("Взять «", "Take \"")) + name + L("» с сервера?", "\" from the server?");
            body = L("Карта и описание этой копии заменятся тем, что на сервере.", "This copy's map and details are replaced with the online version.");
            yes = L("Заменить", "Replace");
            break;
        default:
            title = std::string(L("Удалить «", "Delete \"")) + name + L("»?", "\"?");
            body = std::string(L("Уровень удалится с этого устройства и из аккаунта — на других устройствах тоже.",
                                 "The level is deleted from this device and the account — from your other devices too.")) +
                   (online ? L(" На сервере он останется: чтобы убрать и его, сначала «Снять с сервера».",
                               " It stays online: to remove it there too, Unpublish it first.")
                           : "");
            yes = L("Удалить", "Delete");
            danger = true;
            break;
        }
        ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
        ImGui::TextUnformatted(title.c_str());
        ImGui::PopFont();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 420 * scale);
        ImGui::TextDisabled("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (mcui::Button(yes, danger ? mcui::ButtonKind::Danger : mcui::ButtonKind::Accent)) {
            const LevelAsk ask = s_ask;
            const std::string id = s_askId;
            s_ask = LevelAsk::None, s_askId.clear();
            ImGui::CloseCurrentPopup();
            if (ask == LevelAsk::Unpublish) publishLevel(id, port::online::uploadedId(id), false);
            else if (ask == LevelAsk::Take) takeFromServer(id);
            else removeLevel(id);
        }
        ImGui::SameLine();
        if (mcui::Button(L("Отмена", "Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            s_ask = LevelAsk::None, s_askId.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    {
        std::lock_guard<std::mutex> lock(s_publish.mu);
        if (s_publish.busy) ImGui::TextDisabled("%s", L("Связь с сервером…", "Talking to the server…"));
        else if (!s_publish.message.empty()) ImGui::TextWrapped("%s", s_publish.message.c_str());
    }
    if (!s_fileMessage.empty()) ImGui::TextWrapped("%s", s_fileMessage.c_str());
    if (!message.empty()) ImGui::TextWrapped("%s", message.c_str());
}

void accountTab(float scale) {
    AccountFlow& flow = *s_account;
    std::unique_lock<std::mutex> lock(flow.mu);
    AccountFlow::Stage stage = flow.stage;

    mcui::SectionTitle(L("Аккаунт MoonCellar", "MoonCellar account"));
    if (stage == AccountFlow::Checking) {
        ImGui::TextDisabled("%s", L("Проверяем вход…", "Checking the sign-in…"));
    } else if (stage == AccountFlow::SignedIn) {
        std::string name = flow.offline ? port::online::userName() : flow.account.userName;
        float side = 64 * scale;
        ImVec2 at = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        if (s_avatar) {
            draw->AddImageRounded(ImTextureRef((ImTextureID)(intptr_t)s_avatar), at, ImVec2(at.x + side, at.y + side),
                                  ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, side / 2);
        } else {
            ImVec2 center(at.x + side / 2, at.y + side / 2);
            draw->AddCircleFilled(center, side / 2, ImGui::GetColorU32(ImGuiCol_ButtonActive));
            std::string letter = name.empty() ? "?" : name.substr(0, (unsigned char)name[0] < 0x80 ? 1 : 2);
            ImGui::PushFont(nullptr, 30.0f);
            ImVec2 size = ImGui::CalcTextSize(letter.c_str());
            draw->AddText(ImVec2(center.x - size.x / 2, center.y - size.y / 2), IM_COL32_WHITE, letter.c_str());
            ImGui::PopFont();
        }
        ImGui::Dummy(ImVec2(side, side));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::PushFont(nullptr, 24.0f);
        ImGui::TextUnformatted(name.c_str());
        ImGui::PopFont();
        if (flow.offline)
            ImGui::TextDisabled("%s", L("Нет связи с сервером — имя из прошлого входа",
                                        "No connection to the server — the name is from the last sign-in"));
        else
            ImGui::TextDisabled("%s", L("Вход выполнен", "Signed in"));
        if (ImGui::Button(L("Выйти", "Sign out"))) {
            std::string server = flow.server, token = port::online::token();
            std::thread([server, token]() { port::online::signOut(server, token); }).detach();
            port::online::saveAccount("", "");
            flow.stage = AccountFlow::SignedOut;
            flow.account = port::online::Account();
            flow.error.clear();
            if (s_avatar) SDL_DestroyTexture(s_avatar);
            s_avatar = nullptr;
        }
        ImGui::EndGroup();
        hint(L("Ваше время уходит в таблицы лидеров под этим именем, вместе с призраком заезда. Это устройство "
               "видно на сайте в настройках, в разделе Game devices, — там вход можно отозвать.",
               "Your times go to the leaderboards under this name, with the ghost of the run. This device is "
               "listed on the site under Settings, Game devices, where the sign-in can be revoked."));
        cloudSection();
    } else if (stage == AccountFlow::Waiting || stage == AccountFlow::Requesting) {
        if (stage == AccountFlow::Requesting) {
            ImGui::TextDisabled("%s", L("Получаем код…", "Getting a code…"));
        } else {
            if (flow.signup)
                ImGui::TextWrapped("%s", L("Создайте аккаунт на открывшемся сайте — код входа уже подставлен:",
                                           "Create an account on the site that opened — the sign-in code is "
                                           "already filled in:"));
            else
                ImGui::TextWrapped(L("Подтвердите вход на открывшемся сайте или откройте %s и введите код:",
                                     "Confirm on the site that opened, or open %s and enter the code:"),
                                   flow.code.url.c_str());
            mcui::DeviceCode(flow.code.userCode.c_str());
            drawQr(flow.code.urlWithCode, 180 * scale);
            hint(L("Или наведите на QR-код камеру телефона. Подтвердите вход аккаунтом MoonCellar; код действует "
                   "10 минут. Ждём подтверждения…",
                   "Or point a phone camera at the QR code. Confirm with your MoonCellar account; the code is valid "
                   "for 10 minutes. Waiting for the confirmation…"));
            if (ImGui::Button(L("Открыть сайт ещё раз", "Open the site again"))) openUrl(linkPage(flow));
            ImGui::SameLine();
        }
        if (ImGui::Button(L("Отмена", "Cancel"))) {
            lock.unlock();
            cancelSignIn();
            return;
        }
    } else {
        launcherHelp(L("Войдите аккаунтом MoonCellar — тогда ваше время попадёт в таблицы лидеров под "
                                   "вашим именем. Без входа игра работает офлайн, таблицы можно только смотреть.",
                                   "Sign in with a MoonCellar account to put your times on the leaderboards under "
                                   "your name. Without it the game works offline and the leaderboards are "
                                   "read-only."));
        if (!flow.error.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorAttention));
            ImGui::TextWrapped("%s", flow.error.c_str());
            ImGui::PopStyleColor();
        }
        bool signIn = mcui::Button(L("Войти через MoonCellar", "Sign in with MoonCellar"), mcui::ButtonKind::Accent);
        ImGui::SameLine();
        bool signUp = ImGui::Button(L("Зарегистрироваться", "Create an account"));
        hint(L("Пароль в игру не вводится: игра покажет код и откроет сайт, где вход подтверждается — с этого "
               "компьютера или с телефона по QR-коду. Нет аккаунта — «Зарегистрироваться»: регистрация откроется с "
               "тем же кодом.",
               "The game never asks for your password: it shows a code and opens the site where you confirm the "
               "sign-in — on this computer, or on a phone with the QR code. No account yet? \"Create an account\" "
               "opens registration with the same code."));
        if (signIn || signUp) {
            lock.unlock();
            startSignIn(signUp);
            return;
        }
    }
    ImGui::Spacing();
    ImGui::TextDisabled(L("Сервер: %s", "Server: %s"), flow.server.c_str());
}

void touchControls() {
    namespace touch = port::platform::touch;
    touch::Config value = touch::config();
    auto sliderWidth = [](const char* label) {
        ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x -
                                               port::platform::launcherHelpButtonSize() - ImGui::GetStyle().ItemSpacing.x * 3));
    };
    const char* distanceLabel = L("Длина свайпа", "Swipe distance");
    sliderWidth(distanceLabel);
    bool changed = ImGui::SliderInt(distanceLabel, &value.distance, 15, 80);
    hint(L("Увеличьте, если короткие движения вызывают случайные действия. Длина указана в точках игрового экрана.",
           "Increase this if short movements trigger accidental actions. Distance is measured in game screen units."));
    const char* sectorsLabel = L("Секторы вверх и вниз", "Up and down sectors");
    sliderWidth(sectorsLabel);
    changed |= ImGui::SliderInt(sectorsLabel, &value.verticalSector, 40, 90, "%d°");
    hint(L("90° — четыре равных направления. Уменьшите угол, если диагональные свайпы вызывают прыжок вместо бега.",
           "90° gives four equal directions. Lower the angle if diagonal swipes trigger a jump instead of running."));
    changed |= ImGui::Checkbox(L("Одно направление на касание", "One direction per touch"), &value.lockDirection);
    hint(L("Защищает от прыжка при загибе свайпа вверх. Для нового жеста отпустите палец. Отключите, чтобы бежать и прыгать одним движением.",
           "Prevents a swipe curving upward from triggering a jump. Lift your finger before the next gesture. Turn off to run and jump in one continuous motion."));
    if (changed) touch::setConfig(value);
    if (ImGui::Button(L("Сбросить настройки тача", "Reset touch controls"))) touch::reset();
    ImGui::Spacing();
}

void controlsTab() {
    using port::platform::bindings::Action;
    mcui::SectionTitle(L("Управление", "Controls"));
    if (port::platform::defaultTouchInput()) {
        ImGui::TextWrapped("%s", L("Проводите пальцем в нужную сторону: вверх — прыжок, вниз — подкат или спуск, влево и вправо — бег. Касание экрана останавливает Фейт. Пауза — кнопкой в углу или свайпом вниз двумя пальцами. Наклоняйте устройство для равновесия.",
                                  "Swipe in the direction you need: up to jump, down to slide or drop, left and right to run. Tap the screen to stop Faith. Pause with the corner button or a two-finger downward swipe. Tilt the device to balance."));
        touchControls();
        if (!ImGui::CollapsingHeader(L("Клавиатура и геймпад", "Keyboard and gamepad"))) return;
    } else if (ImGui::CollapsingHeader(L("Сенсорное управление", "Touch controls"), ImGuiTreeNodeFlags_DefaultOpen)) {
        touchControls();
    }
    if (!port::platform::defaultTouchInput()) mcui::SectionTitle(L("Клавиатура и геймпад", "Keyboard and gamepad"));
    launcherHelp(L("Нажмите клетку и клавишу или кнопку. Esc — отмена. Стрелки, Enter и P работают вместе с "
          "назначенным. Shift со свайпом влево или вправо — двумя пальцами. Крестовина и стик — "
          "направления всегда. В меню стрелки и крестовина ходят по кнопкам, Enter и A подтверждают, "
          "Esc и B — назад. Мышь и касание — как на планшете. Второй геймпад — второй игрок. "
          "F11 — во весь экран, F12 — снимок кадра.",
          "Click a cell, then a key or button. Esc cancels. Arrows, Enter and P stay alongside the "
          "binding. Shift with a left or right swipe is two fingers. The D-pad and stick always move. "
          "In menus the arrows and D-pad move between buttons, Enter and A confirm, Esc and B go back. "
          "Mouse and touch work as on the tablet. A second gamepad is player two. "
          "F11 is fullscreen, F12 is a screenshot."));

    const char* names[port::platform::bindings::ActCount] = {
        L("Свайп вверх", "Swipe up"),
        L("Свайп вниз", "Swipe down"),
        L("Свайп влево", "Swipe left"),
        L("Свайп вправо", "Swipe right"),
        L("Двумя пальцами влево", "Two-finger left"),
        L("Двумя пальцами вправо", "Two-finger right"),
        L("Тап", "Tap"),
        L("Наклон влево", "Tilt left"),
        L("Наклон вправо", "Tilt right"),
        L("Пауза", "Pause"),
    };
    bool wait = port::platform::bindings::capturing();
    int which = port::platform::bindings::captureAction();
    bool waitPad = port::platform::bindings::capturePad();
    if (ImGui::BeginTable("##controls", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                               ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn(L("Действие", "Action"), ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableSetupColumn(L("Клавиатура", "Keyboard"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(L("Геймпад", "Gamepad"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();
        for (int i = 0; i < port::platform::bindings::ActCount; i++) {
            Action a = Action(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", names[i]);
            ImGui::TableNextColumn();
            ImGui::PushID(i * 2);
            const char* key = wait && !waitPad && which == i ? "…" : port::platform::bindings::keyLabel(a);
            if (ImGui::Button(key, ImVec2(-FLT_MIN, 0))) port::platform::bindings::capture(i, false);
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::PushID(i * 2 + 1);
            const char* pad = wait && waitPad && which == i ? "…" : port::platform::bindings::buttonLabel(a);
            if (ImGui::Button(pad, ImVec2(-FLT_MIN, 0))) port::platform::bindings::capture(i, true);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    float zone = port::platform::bindings::deadzone();
    if (ImGui::SliderFloat(L("Мёртвая зона стика", "Stick deadzone"), &zone, 0.05f, 0.9f, "%.2f"))
        port::platform::bindings::setDeadzone(zone);
    if (ImGui::Button(L("По умолчанию", "Defaults"))) port::platform::bindings::reset();

}

struct BrandTexture {
    SDL_Texture* tex = nullptr;
    int w = 0, h = 0;
};
BrandTexture s_logoText, s_logoFull, s_appIcon;
bool s_aboutOpen = false;

void spinner(ImVec2 center, float r, float scale, Uint32 ms) {
    float t = float(ms) / 1000.0f;
    for (int i = 0; i < 10; i++) {
        float a = float(i) / 10.0f * 6.2832f;
        float bright = std::fmod(t * 1.5f - float(i) / 10.0f + 10.0f, 1.0f);
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(center.x + std::cos(a) * r, center.y + std::sin(a) * r), 2.5f * scale,
                                                    mcui::Packed(mcui::tokens::kColorAccent, 0.25f + 0.75f * (1.0f - bright)));
    }
}

BrandTexture loadBrandTexture(SDL_Renderer* renderer, const char* file) {
    BrandTexture out;
    port::gfx::Image image;
    if (!port::platform::loadBrandingImage(file, &image)) return out;
    out.tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
    if (!out.tex) return out;
    SDL_UpdateTexture(out.tex, nullptr, image.rgba.data(), image.width * 4);
    SDL_SetTextureBlendMode(out.tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(out.tex, SDL_ScaleModeLinear);
    out.w = image.width;
    out.h = image.height;
    return out;
}

void brandImage(const BrandTexture& t, float height) {
    ImGui::Image(ImTextureRef((ImTextureID)(intptr_t)t.tex), ImVec2(height * float(t.w) / float(t.h), height));
}

void appIconImage(float side) {
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + side, a.y + side);
    const float rounding = side * 0.22f;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddImageRounded(ImTextureRef((ImTextureID)(intptr_t)s_appIcon.tex), a, b, ImVec2(0, 0), ImVec2(1, 1),
                          IM_COL32_WHITE, rounding);
    draw->AddRect(a, b, mcui::Packed(mcui::tokens::kColorBgHover), rounding, 0, 1.0f);
    ImGui::Dummy(ImVec2(side, side));
}

void cloudHeaderStatus(float right, float y, float scale) {
    const float gap = 14.0f * scale;
    const float frameH = ImGui::GetFrameHeight();
    if (s_cloudInterrupted || s_cloudHook == "interrupted" || s_cloudHook == "popup") {
        const char* label = L("Внимание", "Warning");
        const float w = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2;
        ImGui::SetCursorPos(ImVec2(right - gap - w, y));
        ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorAttention));
        ImGui::PushStyleColor(ImGuiCol_Button, mcui::Color(mcui::tokens::kColorAttention, 0.14f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mcui::Color(mcui::tokens::kColorAttention, 0.24f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, mcui::Color(mcui::tokens::kColorAttention, 0.32f));
        static bool hookPopup = s_cloudHook == "popup";
        if (ImGui::Button(label) || hookPopup) ImGui::OpenPopup("##cloudwarn");
        hookPopup = false;
        ImGui::PopStyleColor(4);
    } else if (port::online::cloud::busy() || s_cloudHook == "busy") {
        const float r = frameH * 0.32f;
        ImGui::SetCursorPos(ImVec2(right - gap - frameH, y));
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 c(p.x + frameH / 2, p.y + frameH / 2);
        const float a = float(SDL_GetTicks() % 1000) / 1000.0f * 6.2831853f;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddCircle(c, r, mcui::Packed(mcui::tokens::kColorBgHover), 32, 2.0f * scale);
        draw->PathArcTo(c, r, a, a + 4.2f, 24);
        draw->PathStroke(mcui::Packed(mcui::tokens::kColorAccent), 0, 2.0f * scale);
        ImGui::Dummy(ImVec2(frameH, frameH));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", L("Сверяю сохранение с аккаунтом…", "Syncing the save with the account…"));
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x / 2, display.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(520.0f * scale, display.x - 32.0f * scale), 0));
    if (ImGui::BeginPopupModal("##cloudwarn", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings)) {
        mcui::Title(L("Сохранение не отправлено", "The save was not sent"));
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorAttention));
        ImGui::TextWrapped("%s", L("Прошлая игра закрылась, не отправив сохранение в аккаунт: её закрыли принудительно "
                                   "или не было связи.",
                                   "The last game closed without sending the save to the account: it was closed "
                                   "forcibly or there was no connection."));
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
        if (mcui::Button(L("Синхронизировать", "Sync"), mcui::ButtonKind::Accent)) {
            s_cloudInterrupted = false;
            port::online::cloud::startSync(s_russian);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (mcui::Button(L("Закрыть", "Close"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

const char* portDescription() {
#ifdef __ANDROID__
    return L("Порт для Mirror's Edge (iPad)", "Port for Mirror's Edge (iPad)");
#else
    return L("PC-порт для Mirror's Edge (iPad)", "PC port for Mirror's Edge (iPad)");
#endif
}

void aboutWindow(float scale) {
    if (s_aboutOpen && !ImGui::IsPopupOpen("##about")) ImGui::OpenPopup("##about");
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x / 2, display.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(560.0f * scale, display.x - 32.0f * scale), 0));
    if (!ImGui::BeginPopupModal("##about", nullptr,
                                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoSavedSettings))
        return;
    if (s_logoFull.tex) {
        const float height = 96.0f * scale;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - height * float(s_logoFull.w) / float(s_logoFull.h)) / 2);
        brandImage(s_logoFull, height);
        ImGui::Spacing();
    }
    if (s_appIcon.tex) {
        appIconImage(52.0f * scale);
        ImGui::SameLine(0, 14.0f * scale);
    }
    ImGui::BeginGroup();
    mcui::Title(MEDECOMP_PORT_NAME);
#ifdef MEDECOMP_VERSION
    ImGui::TextDisabled("%s · %s %s · MoonCellar Team", portDescription(), L("версия", "version"), MEDECOMP_VERSION);
#else
    ImGui::TextDisabled("%s · MoonCellar Team", portDescription());
#endif
    ImGui::EndGroup();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", L(MEDECOMP_PORT_NAME " — неофициальный порт от MoonCellar Team. Mirror's Edge — товарный "
                               "знак Electronic Arts Inc.; проект не связан с Electronic Arts и не одобрен ею. Ресурсы "
                               "игры загружаются из вашей копии игры: её вы указываете или скачиваете сами, прав на игру "
                               "у порта нет.",
                               MEDECOMP_PORT_NAME " is an unofficial port by MoonCellar Team. Mirror's Edge is a "
                               "trademark of Electronic Arts Inc.; this project is not affiliated with or endorsed by "
                               "Electronic Arts. Game assets are loaded from your own copy of the game, which you choose "
                               "or download yourself; the port holds no rights to the game."));
    ImGui::TextDisabled("Mirror's Edge © EA / DICE. All Rights Reserved.");
    ImGui::Spacing();
    ImGui::Spacing();
    if (mcui::Button("mooncellar.space")) openUrl("https://mooncellar.space");
    ImGui::SameLine();
    if (mcui::Button(L("Исходный код и выпуски", "Source code and releases"))) openUrl("https://github.com/" MEDECOMP_PUBLIC_REPO);
    ImGui::SameLine();
    const char* close = L("Закрыть", "Close");
    const float closeW = ImGui::CalcTextSize(close).x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - closeW);
    if (mcui::Button(close, mcui::ButtonKind::Accent) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        s_aboutOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

int s_readyFrames = 0;
void updateBanner(float scale) {
    namespace update = port::platform::update;
    static bool s_hidden = false;
    const update::Status st = update::status();
    const bool shown = st.stage == update::Stage::Available || st.stage == update::Stage::Downloading ||
                       st.stage == update::Stage::Installing || st.stage == update::Stage::Ready ||
                       (st.stage == update::Stage::Failed && !st.version.empty());
    if (!shown || s_hidden) return;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, mcui::Color(mcui::tokens::kColorBgSecondary));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * scale, 10.0f * scale));
    ImGui::BeginChild("##update", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
    ImGui::Text(L("Доступна версия %s", "Version %s is available"), st.version.c_str());
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextDisabled(L("у вас %s", "you have %s"), port::platform::update::currentVersion().c_str());
    if (!st.notes.empty() && ImGui::TreeNodeEx(L("Что нового", "What's new"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(st.notes.c_str());
        ImGui::PopTextWrapPos();
        ImGui::TreePop();
    }
    switch (st.stage) {
    case update::Stage::Downloading: {
        char text[96];
        if (st.total > 0)
            snprintf(text, sizeof text, L("Скачивание… %.0f из %.0f МБ", "Downloading… %.0f of %.0f MB"), double(st.done) / 1048576,
                     double(st.total) / 1048576);
        else snprintf(text, sizeof text, "%s", L("Скачивание…", "Downloading…"));
        ImGui::ProgressBar(st.total > 0 ? float(double(st.done) / double(st.total)) : 0.0f, ImVec2(-FLT_MIN, 0), text);
        break;
    }
    case update::Stage::Installing: ImGui::TextDisabled("%s", L("Проверка подписи и установка…", "Checking the signature and installing…")); break;
    case update::Stage::Ready: ImGui::TextDisabled("%s", L("Готово — перезапуск…", "Done — restarting…")); break;
    default: {
        if (st.stage == update::Stage::Failed) {
            ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorNegative));
            ImGui::TextWrapped("%s", st.error.c_str());
            ImGui::PopStyleColor();
        }
        if (st.installable) {
            if (mcui::Button(st.stage == update::Stage::Failed ? L("Ещё раз", "Try again") : L("Обновить", "Update"), mcui::ButtonKind::Accent))
                update::install(s_russian);
            launcherHelp(L("скачается, сверится подпись, лаунчер перезапустится; игра и сохранения не трогаются",
                                        "downloads, checks the signature and restarts the launcher; your game and saves stay"));
        } else {
            if (mcui::Button(L("Скачать", "Download"), mcui::ButtonKind::Accent)) openUrl(st.page);
            launcherHelp(L("эту установку лаунчер сам не обновит — страница выпуска",
                                        "the launcher can't update this install itself — the release page"));
        }
        const char* later = L("Не сейчас", "Not now");
        ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(later).x - ImGui::GetStyle().FramePadding.x * 2);
        if (mcui::Button(later, mcui::ButtonKind::Ghost)) s_hidden = true;
        break;
    }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void touchScroll(float scale) {
    static ImGuiID scrollWindow = 0;
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        scrollWindow = 0;
        return;
    }
    if (io.MouseSource != ImGuiMouseSource_TouchScreen || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        scrollWindow = 0;
        return;
    }
    ImGuiID window = ImGui::GetCurrentWindow()->ID;
    if (!scrollWindow) {
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
            return;
        ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
        if (fabsf(drag.y) < 12.0f * scale || fabsf(drag.y) < fabsf(drag.x)) return;
        scrollWindow = window;
        ImGui::ClearActiveID();
    }
    if (scrollWindow == window) ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
}

void loadingScreen(float scale, Uint32 ms, bool syncing) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##loading", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    const float logoH = 48.0f * scale, r = 14.0f * scale, gap = 28.0f * scale;
    const char* text = syncing ? L("Сверяю сохранения с аккаунтом…", "Syncing saves with the account…")
                               : L("Запускаю игру…", "Starting the game…");
#ifdef MEDECOMP_ONE_PROCESS
    if (port::platform::heroine::previewRequested()) text = L("Готовлю превью скинов…", "Preparing skin previews…");
#endif
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    float y = (io.DisplaySize.y - (logoH + gap + 2 * r + gap + textSize.y)) / 2;
    if (s_logoFull.tex) {
        float logoW = logoH * float(s_logoFull.w) / float(s_logoFull.h);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - logoW) / 2, y));
        brandImage(s_logoFull, logoH);
    }
    y += logoH + gap;
    spinner(ImVec2(io.DisplaySize.x / 2, y + r), r, scale, ms);
    y += 2 * r + gap;
    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - textSize.x) / 2, y));
    ImGui::TextDisabled("%s", text);
    ImGui::End();
}

bool runWindow(std::string* path) {
    port::platform::setAppId();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "medecomp: лаунчер: SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    const int kWidth = 1000, kHeight = 800, kMinWidth = 560, kMinHeight = 440;
    int pointsW = kWidth, pointsH = kHeight;
    bool userResized = false;
    const std::string savedSize = port::platform::settings::get("launcher_size");
    if (sscanf(savedSize.c_str(), "%dx%d", &pointsW, &pointsH) != 2 || pointsW < kMinWidth || pointsH < kMinHeight ||
        pointsW > 8000 || pointsH > 8000) {
        pointsW = kWidth;
        pointsH = kHeight;
    }
#ifdef __ANDROID__
    const Uint32 kFlags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI;
#elif defined(__EMSCRIPTEN__)
    const Uint32 kFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    const Uint32 kFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
#endif
    int createW = kWidth, createH = kHeight;
#ifdef __EMSCRIPTEN__
    {
        double cw = 0, ch = 0;
        emscripten_get_element_css_size("#canvas", &cw, &ch);
        if (cw > 0 && ch > 0) createW = int(cw), createH = int(ch);
    }
#endif
    SDL_Window* window = SDL_CreateWindow(MEDECOMP_PORT_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, createW,
                                          createH, kFlags);
    if (!window) {
        fprintf(stderr, "medecomp: лаунчер: окно: %s\n", SDL_GetError());
        return false;
    }
    port::platform::setWindowIcon(window);
    int pixelW = 0, pixelH = 0;
    SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
    float scale = port::platform::displayScale(window, pixelW);
    SDL_Rect area;
    int display = SDL_GetWindowDisplayIndex(window);
    if (SDL_GetDisplayUsableBounds(display < 0 ? 0 : display, &area) == 0) {
        float fit = std::min(0.9f * area.w / 760, 0.9f * area.h / 680);
        scale = std::max(1.0f, std::min(scale, fit));
    }
#ifdef __ANDROID__
    scale = 0.8f * std::max(1.0f, std::min(port::platform::displayScale(window, pixelW),
                                    std::min(std::max(pixelW, pixelH) / 900.0f, std::min(pixelW, pixelH) / 600.0f)));
#elif defined(__EMSCRIPTEN__)
    scale = 0.8f * std::max(1.0f, std::min(createW / 900.0f, createH / 600.0f));
    (void)userResized;
#else
    SDL_DisplayMode desktopMode;
    if (port::platform::gamescopeSession() && SDL_GetDesktopDisplayMode(display < 0 ? 0 : display, &desktopMode) == 0) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        scale = std::max(1.0f, std::min(std::max(desktopMode.w, desktopMode.h) / 900.0f,
                                        std::min(desktopMode.w, desktopMode.h) / 600.0f));
    } else {
        int w = int(pointsW * scale), h = int(pointsH * scale);
        if (SDL_GetDisplayUsableBounds(display < 0 ? 0 : display, &area) == 0) {
            w = std::min(w, int(area.w * 0.95f));
            h = std::min(h, int(area.h * 0.95f));
        }
        SDL_SetWindowSize(window, std::max(w, int(kMinWidth * scale)), std::max(h, int(kMinHeight * scale)));
        SDL_SetWindowMinimumSize(window, int(kMinWidth * scale), int(kMinHeight * scale));
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
#endif
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "medecomp: лаунчер: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }
    SDL_ShowWindow(window);
    const Uint32 shownAt = SDL_GetTicks();
#ifdef _WIN32
    std::thread([] { FcInit(); }).detach();
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ImGui::GetCurrentContext()->ConfigNavWindowingKeyNext = 0;
    ImGui::GetCurrentContext()->ConfigNavWindowingKeyPrev = 0;
    if (!loadLauncherFont()) s_russian = false;
    mcui::ApplyStyle(scale);
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    s_logoText = loadBrandTexture(renderer, "logo-text.png");
    s_logoFull = loadBrandTexture(renderer, "logo-full.png");
    s_appIcon = loadBrandTexture(renderer, "app-icon-256.png");
    s_aboutOpen = getenv("MEDECOMP_LAUNCHER_ABOUT") != nullptr;
    port::platform::update::cleanup();
#ifndef __EMSCRIPTEN__
    port::platform::update::check(s_russian);
#endif
    const char* browseHook = getenv("MEDECOMP_LAUNCHER_BROWSE");
    s_cloudHook = getenv("MEDECOMP_LAUNCHER_CLOUD") ? getenv("MEDECOMP_LAUNCHER_CLOUD") : "";
    int browseFrames = 0;
    if (const char* url = getenv("MEDECOMP_LAUNCHER_DOWNLOAD"); url && *url) {
        snprintf(s_url, sizeof s_url, "%s", url);
        startDownload(url);
    }
    if (const char* ms = getenv("MEDECOMP_LAUNCHER_DOWNLOAD_CANCEL")) s_downloadCancelAfter = Uint32(atoi(ms));

    char buf[4096];
    snprintf(buf, sizeof buf, "%s", path->c_str());
    std::string checked = buf;
    Check status = check(checked);
    s_hdGame = status.ok ? checked : std::string();
    bool original = port::platform::originalGraphics();
    int mode = int(port::platform::windowModeSetting());
    int fps = port::platform::frameRateSetting();
    int upscale = int(port::platform::upscaleSetting());
    float sharpness = port::platform::sharpnessSetting();
    bool wide = port::platform::widescreenSetting();
    int render = port::platform::renderSetting();
    int filter = port::platform::textureFilterSetting();
    int msaa = port::platform::msaaSetting();
    bool pack = port::gfx::texturePackSetting();
    int post = port::platform::postSetting();
    float postStrength = port::platform::postStrength();
    int ssao = port::platform::ssaoSetting();
    bool godRays = port::platform::godRaysSetting();
    bool motionBlur = port::platform::motionBlurSetting();
    float ssrStrength = port::platform::ssrSetting();
    bool ssr = ssrStrength > 0;
    if (!ssr) ssrStrength = port::platform::ssrStrengthSetting();
    bool shadows = port::gfx::runnerShadowsSetting();
    bool flare = port::platform::lensFlareSetting();
    int grade = port::platform::gradeSetting();
    bool taa = port::platform::taaSetting();
    std::string language = port::platform::settings::get("language", "auto");
    Screen screen;
    screen.display = port::platform::displaySetting();
    screen.width = screen.height = 0;
    port::platform::resolutionSetting(&screen.width, &screen.height);
    screen.vsync = port::platform::vsyncSetting();
    screen.videoDriver = int(port::platform::videoDriverSetting());
    Sound sound;
    sound.volume = port::audio::volumeSetting();
    sound.music = port::audio::musicSetting();
    snprintf(sound.folder, sizeof sound.folder, "%s", port::audio::musicFolderSetting().c_str());
    sound.shuffle = port::audio::musicShuffleSetting();
    sound.musicVolume = port::audio::musicVolumeSetting();
    sound.hrtf = hrtfSetting();
    sound.backgroundPause = port::audio::backgroundPauseSetting();
    s_account->server = port::online::serverUrl();
    if (!port::online::token().empty()) checkAccount(port::online::token());
    else if (getenv("MEDECOMP_LAUNCHER_SIGNIN")) startSignIn(false);
    bool play = false, quit = false, focusPlay = status.ok;
    const char* shot = getenv("MEDECOMP_LAUNCHER_SHOT");
    int startTab = getenv("MEDECOMP_LAUNCHER_TAB") ? atoi(getenv("MEDECOMP_LAUNCHER_TAB")) : 0;
    bool firstFrame = true;
    int frames = 0;
    bool wasFullScroll = false;
#ifdef __EMSCRIPTEN__
    Uint32 lastFingerAt = 0;
#endif
    while (!play && !quit) {
        SDL_Event e;
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
        double canvasW = 0, canvasH = 0;
        emscripten_get_element_css_size("#canvas", &canvasW, &canvasH);
        int windowW = 0, windowH = 0;
        SDL_GetWindowSize(window, &windowW, &windowH);
        if (canvasW > 0 && canvasH > 0 && (windowW != int(canvasW) || windowH != int(canvasH)))
            SDL_SetWindowSize(window, int(canvasW), int(canvasH));
        if (SDL_PollEvent(&e)) {
#else
        if (SDL_WaitEventTimeout(&e, port::online::cloud::busy() || s_cloudHook == "busy" || s_download.busy || s_skeletonShown ? 33 : 250)) {
#endif
            do {
                port::platform::turn::event(&e);
#ifdef __ANDROID__
                if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && e.key.keysym.sym == SDLK_AC_BACK) {
                    e.key.keysym.sym = SDLK_ESCAPE;
                    e.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
                }
#endif
                bool mouseFromBrowser = false;
#ifdef __EMSCRIPTEN__
                if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP)
                    lastFingerAt = SDL_GetTicks();
                if (lastFingerAt && SDL_GetTicks() - lastFingerAt < 800) {
                    if (e.type == SDL_MOUSEMOTION) mouseFromBrowser = e.motion.which != SDL_TOUCH_MOUSEID;
                    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
                        mouseFromBrowser = e.button.which != SDL_TOUCH_MOUSEID;
                }
#endif
                if (!mouseFromBrowser && !(port::platform::bindings::capturing() && port::platform::bindings::takeEvent(e)))
                    ImGui_ImplSDL2_ProcessEvent(&e);
                if (e.type == SDL_QUIT) quit = true;
                if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) quit = true;
                if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED &&
                    SDL_GetTicks() - shownAt > 1000)
                    userResized = true;
                if (e.type == SDL_DROPFILE) {
                    const std::string dropped = e.drop.file, ext = port::platform::kLevelFileExtension;
                    if (dropped.size() > ext.size() && dropped.compare(dropped.size() - ext.size(), ext.size(), ext) == 0)
                        importLevel(dropped), s_showLevels = true;
                    else
                        snprintf(buf, sizeof buf, "%s", e.drop.file);
                    SDL_free(e.drop.file);
                }
            } while (SDL_PollEvent(&e));
        }
        {
            namespace dialog = port::platform::dialog;
            dialog::Kind kind;
            std::string picked;
            if (dialog::take(&kind, &picked, nullptr) && !picked.empty()) {
                if (kind == dialog::MusicFolder)
                    snprintf(sound.folder, sizeof sound.folder, "%s", picked.c_str());
                else if (kind == dialog::Ipa)
                    snprintf(buf, sizeof buf, "%s", picked.c_str());
                else if (kind == dialog::LevelFile)
                    importLevel(picked);
                else
                    port::platform::heroine::picked(kind == dialog::Png, picked);
            }
        }
        finishDownload(buf, sizeof buf, &status, checked);
        if (checked != buf) {
            checked = buf;
            status = check(checked);
#ifdef __EMSCRIPTEN__
            if (status.ok) {
                port::platform::settings::set("game", checked);
                port::platform::settings::save();
            }
#endif
            focusPlay = status.ok;
            s_hdGame = status.ok ? checked : std::string();
        }

        accountUpdate(renderer);
        if (cloudUpdate()) {
            language = port::platform::settings::get("language", "auto");
            sound.volume = port::audio::volumeSetting();
            sound.music = port::audio::musicSetting();
            sound.shuffle = port::audio::musicShuffleSetting();
            sound.musicVolume = port::audio::musicVolumeSetting();
            sound.hrtf = hrtfSetting();
            sound.backgroundPause = port::audio::backgroundPauseSetting();
        }
        port::platform::heroine::update(renderer, s_hdGame);
        const bool copyingGame = port::platform::dialog::busy() && port::platform::dialog::copyProgress() >= 0;
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        port::platform::turn::imgui(window);
        ImGui::NewFrame();
#ifdef __EMSCRIPTEN__
        ImVec4 browserBrowse(0, 0, 0, 0), browserPlay(0, 0, 0, 0);
        bool browserPlayFocused = false;
        if (!port::platform::dialog::busy() && MAIN_THREAD_EM_ASM_INT({ return !!Module.medecompFilePending; }))
            port::platform::dialog::pick(port::platform::dialog::Ipa, buf,
                                         L("Копия Mirror's Edge (.ipa)", "Mirror's Edge copy (.ipa)"));
#endif
        if (browseHook && ++browseFrames == 3) {
            bool music = strcmp(browseHook, "music") == 0;
            port::platform::dialog::pick(music ? port::platform::dialog::MusicFolder : port::platform::dialog::Ipa,
                                         music ? port::audio::musicFolderSetting() : std::string(buf),
                                         music ? L("Папка со своей музыкой", "Folder with your music")
                                               : L("Копия Mirror's Edge (.ipa)", "Mirror's Edge copy (.ipa)"));
        }
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
#ifdef __ANDROID__
        const bool fullScroll = true;
        bool fixedFooter = true;
#else
        const bool fullScroll = io.DisplaySize.y < 600 * scale || io.DisplaySize.x < 720 * scale;
        bool fixedFooter = false;
#endif
        float safeBottom = 0;
#ifdef __EMSCRIPTEN__
        fixedFooter = fullScroll;
        safeBottom = float(MAIN_THREAD_EM_ASM_DOUBLE({ return document.getElementById("safe-area").getBoundingClientRect().height; }));
#endif
        const ImGuiStyle& layout = ImGui::GetStyle();
        const float buttonHeight = ImGui::GetFrameHeight() + (fixedFooter ? 0.0f : 8 * scale);
        const float playWidth = (fixedFooter ? 120.0f : 160.0f) * scale;
        const float bottomPadding = fixedFooter ? 6 * scale : layout.WindowPadding.y;
        const float footer = buttonHeight + layout.ItemSpacing.y * 2 + 1;
        std::string summary = status.text.substr(0, status.text.find('\n'));
        if (!status.ok && !checked.empty()) summary = L("Копия игры не подходит — вкладка «Игра»",
                                                        "The game copy does not fit — see the Game tab");
        if (copyingGame) summary = L("Копирую игру в хранилище браузера…", "Copying the game into browser storage…");
        const float availableWidth = io.DisplaySize.x - layout.WindowPadding.x * 2;
        const float besideButton = availableWidth - playWidth - layout.ItemSpacing.x;
        const bool stackFooter = besideButton < 220 * scale;
        const float summaryHeight = ImGui::CalcTextSize(summary.c_str(), nullptr, false,
                                                       std::max(1.0f, stackFooter ? availableWidth : besideButton)).y;
        const float footerBody = stackFooter ? buttonHeight + layout.ItemSpacing.y + summaryHeight
                                             : std::max(buttonHeight, summaryHeight);
        const float footerTop = io.DisplaySize.y - bottomPadding - safeBottom - footerBody;
        ImGuiWindowFlags launcherFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (fullScroll && !fixedFooter) launcherFlags &= ~ImGuiWindowFlags_NoScrollbar;
        if (fixedFooter) launcherFlags |= ImGuiWindowFlags_NoScrollWithMouse;
        if (fixedFooter) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(layout.WindowPadding.x, bottomPadding));
        ImGui::Begin("##launcher", nullptr, launcherFlags);
        if (fixedFooter) {
            ImGui::PopStyleVar();
            ImGui::SetCursorPosY(layout.WindowPadding.y);
            ImGui::BeginChild("##launcher-content", ImVec2(0, std::max(1.0f, footerTop - ImGui::GetCursorPosY() - layout.ItemSpacing.y)),
                              ImGuiChildFlags_None, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        }
        if (fullScroll != wasFullScroll) ImGui::SetScrollY(0);
        wasFullScroll = fullScroll;
        if (fullScroll) touchScroll(scale);
        const char* aboutLabel = L("О порте", "About");
        const float aboutW = ImGui::CalcTextSize(aboutLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        const float headerY = ImGui::GetCursorPosY();
        if (s_logoText.tex) {
            brandImage(s_logoText, 30.0f * scale);
        } else {
            mcui::Title(MEDECOMP_PORT_NAME);
        }
        ImGui::SameLine();
        const float aboutX = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - aboutW;
        cloudHeaderStatus(aboutX, headerY, scale);
        ImGui::SetCursorPos(ImVec2(aboutX, headerY));
        if (mcui::Button(aboutLabel, mcui::ButtonKind::Ghost)) s_aboutOpen = true;
        ImGui::Spacing();
        if (s_appIcon.tex) {
            const float side = ImGui::GetTextLineHeight() * 1.25f;
            const float y = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(y - (side - ImGui::GetTextLineHeight()) / 2);
            appIconImage(side);
            ImGui::SameLine(0, 8.0f * scale);
            ImGui::SetCursorPosY(y);
        }
        ImGui::PushTextWrapPos(0);
#ifdef MEDECOMP_VERSION
        ImGui::TextDisabled("%s — %s %s · %s", MEDECOMP_PORT_NAME, portDescription(),
                            L("от MoonCellar Team", "by MoonCellar Team"), MEDECOMP_VERSION);
#else
        ImGui::TextDisabled("%s — %s %s", MEDECOMP_PORT_NAME, portDescription(), L("от MoonCellar Team", "by MoonCellar Team"));
#endif
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        updateBanner(scale);
        if (long long here = 0, there = 0; !s_cloudInterrupted && !port::online::cloud::busy() &&
                                            port::online::cloud::conflictPending(&here, &there)) {
            auto when = [](long long t) {
                time_t tt = time_t(t);
                struct tm local;
#ifdef _WIN32
                localtime_s(&local, &tt);
#else
                localtime_r(&tt, &local);
#endif
                char buf[32];
                strftime(buf, sizeof buf, "%d.%m.%Y %H:%M", &local);
                return std::string(buf);
            };
            ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorAttention));
            ImGui::TextWrapped("%s", L("Сохранения на этом компьютере и в аккаунте разные. Какое оставить? Другое "
                                       "останется копией в папке игрока, в backups.",
                                       "The save on this computer and in the account differ. Which one to keep? The "
                                       "other stays as a copy in the player folder, under backups."));
            ImGui::PopStyleColor();
            std::string fromAccount = std::string(L("Из аккаунта", "From the account")) + " (" + when(there) + ")";
            std::string keepHere = std::string(L("Здешнее", "This computer's")) + " (" + when(here) + ")";
            if (ImGui::Button(fromAccount.c_str())) port::online::cloud::resolve(true, s_russian);
            ImGui::SameLine();
            if (ImGui::Button(keepHere.c_str())) port::online::cloud::resolve(false, s_russian);
            ImGui::Separator();
        }

        static int s_tab = 0;
        if (firstFrame) s_tab = std::min(std::max(startTab, 0), 7);
        if (s_showLevels) s_tab = 6, s_showLevels = false;
        const char* tabs[] = {L("Игра", "Game"),       L("Экран", "Display"),   L("Управление", "Controls"),
                              L("Звук", "Sound"),      L("Аккаунт", "Account"), L("Героиня", "Heroine"),
                              L("Уровни", "Levels"),   L("Ачивки", "Achievements")};
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Tab) && !ImGui::IsAnyItemActive() &&
            !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
            s_tab = (s_tab + (io.KeyShift ? 7 : 1)) % 8;
            ImGui::SetKeyboardFocusHere(s_tab);
        }
        int previousTab = s_tab;
        if (ImGui::GetContentRegionAvail().x < 680 * scale) {
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##tabs", tabs[s_tab])) {
                for (int i = 0; i < 8; i++) {
                    if (ImGui::Selectable(tabs[i], s_tab == i)) s_tab = i;
                    if (s_tab == i) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        } else {
            mcui::Segmented("##tabs", tabs, 8, &s_tab, true);
        }
        if (fullScroll && s_tab != previousTab) ImGui::SetScrollY(0);
        const char* panels[] = {"##game",    "##display", "##controls", "##sound",
                                "##account", "##heroine", "##levels",   "##achievements"};
        s_skeletonShown = false;
        float below = s_tab == 5 ? port::platform::heroine::barHeight(scale) + ImGui::GetStyle().ItemSpacing.y : 0.0f;
        if (fullScroll) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 340 * scale), ImVec2(FLT_MAX, FLT_MAX),
                [](ImGuiSizeCallbackData* data) {
                    data->DesiredSize.y = ceilf(data->DesiredSize.y) + 1.0f;
                });
        }
        if (mcui::BeginPanel(panels[s_tab], ImVec2(0, fullScroll ? 0 : -(footer + below)))) {
            if (!fullScroll) touchScroll(scale);
            if (shot && getenv("MEDECOMP_LAUNCHER_SCROLL")) ImGui::SetScrollY(float(atof(getenv("MEDECOMP_LAUNCHER_SCROLL"))) * scale);
            switch (s_tab) {
            case 0: {
                mcui::SectionTitle(L("Копия игры", "Game copy"));
                const char* browse = L("Обзор…", "Browse…");
                float browseWidth = ImGui::CalcTextSize(browse).x + ImGui::GetStyle().FramePadding.x * 2;
                ImGui::SetNextItemWidth(-browseWidth - port::platform::launcherHelpButtonSize() -
                                         ImGui::GetStyle().ItemSpacing.x * 2);
                ImGui::InputText("##game", buf, sizeof buf);
                ImGui::SameLine();
                ImGui::BeginDisabled(port::platform::dialog::busy());
                if (ImGui::Button(browse))
#ifdef __EMSCRIPTEN__
                    if (ImGui::GetCurrentContext()->NavActivateId == ImGui::GetItemID())
#endif
                    port::platform::dialog::pick(port::platform::dialog::Ipa, buf,
                                                 L("Копия Mirror's Edge (.ipa)", "Mirror's Edge copy (.ipa)"));
#ifdef __EMSCRIPTEN__
                if (!port::platform::dialog::busy() && !s_aboutOpen && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
                    ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                    rect.ClipWith(ImGui::GetCurrentWindow()->ClipRect);
                    if (!rect.IsInverted()) browserBrowse = ImVec4(rect.Min.x / io.DisplaySize.x, rect.Min.y / io.DisplaySize.y,
                                                                   rect.Max.x / io.DisplaySize.x, rect.Max.y / io.DisplaySize.y);
                }
#endif
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
#ifdef __ANDROID__
                launcherHelp(L("Файл .ipa Mirror's Edge для iPad, версия 1.1.48 или новее. Ресурсы игры порт читает из "
                                           "него: в порт они не входят. Выбранный файл копируется в папку приложения.",
                                           "The .ipa of Mirror's Edge for iPad, version 1.1.48 or newer. The port reads the game's "
                                           "assets from it and contains none. The chosen file is copied into the "
                                           "app's folder."));
#elif defined(__EMSCRIPTEN__)
                launcherHelp(L("Выберите .ipa Mirror's Edge для iPad, версия 1.1.48 или новее, кнопкой «Обзор». "
                                           "Копия сохранится только в этом браузере и никуда не отправляется.",
                                           "Use Browse to choose your .ipa of Mirror's Edge for iPad, version 1.1.48 or newer. "
                                           "The copy stays in this browser and is never uploaded."));
#else
                launcherHelp(L("Файл .ipa (Mirror's Edge для iPad, версия 1.1.48 или новее) или распакованная папка .app. Ресурсы "
                                           "игры читаются прямо из неё: в порт они не входят. Файл можно перетащить "
                                           "в это окно.",
                                           "An .ipa file (Mirror's Edge for iPad, version 1.1.48 or newer) or an unpacked .app folder. Game "
                                           "assets are read straight from it: the port contains none. You can drop "
                                           "the file onto this window."));
#endif
                ImGui::PopStyleColor();
#ifdef __EMSCRIPTEN__
                if (port::platform::dialog::busy() && port::platform::dialog::copyProgress() >= 0) {
                    ImGui::TextUnformatted(L("Копирую игру в хранилище браузера…", "Copying the game into browser storage…"));
                    ImGui::ProgressBar(port::platform::dialog::copyProgress() / 100.0f);
                }
                if (port::platform::dialog::copyFailed())
                    ImGui::TextWrapped("%s", L("Не удалось скопировать файл. Проверьте свободное место и выберите .ipa ещё раз.",
                                               "Could not copy the file. Check available storage and choose the .ipa again."));
#endif
                ImGui::PushStyleColor(ImGuiCol_Text, statusColor(status));
                if (!copyingGame) ImGui::TextWrapped("%s", status.text.c_str());
                ImGui::PopStyleColor();
                {
                    if (port::platform::dialog::noDialog())
#ifdef _WIN32
                        ImGui::TextWrapped("%s", L("Окно выбора файла Windows не открылось: впишите путь или перетащите "
                                                   "файл.",
                                                   "The Windows file dialog did not open: type the path or drop the "
                                                   "file."));
#elif defined(__EMSCRIPTEN__)
                        ImGui::TextWrapped("%s", L("В браузере этот файл пока не выбрать.",
                                                   "This file cannot be chosen in the browser yet."));
#else
                        ImGui::TextWrapped("%s", L("Диалог выбора файла не найден (kdialog или zenity): впишите путь "
                                                   "или перетащите файл.",
                                                   "No file dialog found (kdialog or zenity): type the path or drop "
                                                   "the file."));
#endif
                }
#ifndef __EMSCRIPTEN__
                downloadSection(scale);
#endif
                languageSection(&language, scale);
                {
                    static bool slowMotion = port::platform::slowMotionSetting();
                    mcui::SectionTitle(L("Игровой процесс", "Gameplay"));
                    if (mcui::Toggle(L("Замедление в бою", "Combat slow motion"), &slowMotion)) {
                        port::platform::settings::set("slow_motion", slowMotion ? "on" : "off");
                        port::platform::settings::save();
                    }
                    launcherHelp(L("Замедляет время и приближает камеру при ударах, в том числе в бою с боссом. "
                                               "По сети замедления нет. Применяется при запуске игры.",
                                               "Slows time and zooms the camera during attacks, including the boss fight. "
                                               "Online play has no slow motion. Applies when the game starts."));
                }
                {
                    static bool splash = port::platform::splashSetting();
                    mcui::SectionTitle(L("При запуске", "On start"));
                    if (mcui::Toggle(L("Заставка MoonCellar Team", "MoonCellar Team splash"), &splash))
                        port::platform::setSplashSetting(splash);
                    launcherHelp(L("1,5 с перед заставками игры, любая кнопка пропускает",
                                                "1.5 s before the game's own splash, any button skips it"));
                    static bool checkUpdates = port::platform::settings::get("check_updates", "on") != "off";
                    if (mcui::Toggle(L("Проверять обновления", "Check for updates"), &checkUpdates)) {
                        port::platform::settings::set("check_updates", checkUpdates ? "on" : "off");
                        port::platform::settings::save();
                        if (checkUpdates) port::platform::update::check(s_russian);
                    }
                    launcherHelp((std::string(L("Новая версия появится полосой вверху. Сейчас установлена ",
                                                               "A new version appears in the bar above. Currently installed: ")) +
                                  port::platform::update::currentVersion()).c_str());
                }
                mcui::SectionTitle(L("Сохранения", "Saves"));
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextWrapped("%s", (port::files::dataDir() + "/Documents").c_str());
                ImGui::PopStyleColor();
                break;
            }
            case 1:
                displayTab(&original, &mode, &fps, &upscale, &sharpness, &wide, &render, &filter, &msaa, &pack, &post, &postStrength, &ssao, &godRays, &motionBlur, &ssr, &ssrStrength, &shadows, &flare, &grade, &taa, &screen, scale);
                break;
            case 2: controlsTab(); break;
            case 3: soundTab(&sound, scale); break;
            case 4: accountTab(scale); break;
            case 5: port::platform::heroine::tab(scale, s_russian); break;
            case 6: levelsTab(buf); break;
            default: achievementsTab(); break;
            }
        }
        mcui::EndPanel();
        if (s_tab == 5) port::platform::heroine::bar(scale);

        if (fixedFooter) {
            ImGui::Dummy(ImVec2(0, layout.ItemSpacing.y));
            ImGui::EndChild();
            ImGui::SetCursorPosY(footerTop);
            const ImVec2 at = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(at.x, at.y - layout.ItemSpacing.y / 2),
                                                ImVec2(at.x + availableWidth, at.y - layout.ItemSpacing.y / 2),
                                                ImGui::GetColorU32(ImGuiCol_Separator));
        }

        float bottom = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - buttonHeight;
        if (!fullScroll && ImGui::GetCursorPosY() < bottom) ImGui::SetCursorPosY(bottom);
        bool canPlay = status.ok && !port::platform::dialog::busy();
#ifdef __EMSCRIPTEN__
        const bool browserPlayRequest = MAIN_THREAD_EM_ASM_INT({
            const pending = !!Module.medecompPlayPending;
            Module.medecompPlayPending = false;
            return pending;
        }) != 0;
        if (browserPlayRequest && canPlay && !s_aboutOpen && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) play = true;
#endif
        ImGui::BeginDisabled(!canPlay);
        if (mcui::Button(L("Играть", "Play"), mcui::ButtonKind::Accent, ImVec2(playWidth, buttonHeight))) play = true;
#ifdef __EMSCRIPTEN__
        if (canPlay && !s_aboutOpen && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
            const ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            browserPlay = ImVec4(rect.Min.x / io.DisplaySize.x, rect.Min.y / io.DisplaySize.y,
                                 rect.Max.x / io.DisplaySize.x, rect.Max.y / io.DisplaySize.y);
            browserPlayFocused = ImGui::IsItemFocused();
        }
#endif
        if (focusPlay) {
            ImGui::SetKeyboardFocusHere(-1);
            ImGui::SetNavCursorVisible(true);
            focusPlay = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
        if (mcui::Button(L("Выход", "Quit"), mcui::ButtonKind::Default, ImVec2(120 * scale, buttonHeight))) quit = true;
#endif
        if (fixedFooter ? stackFooter : ImGui::GetContentRegionAvail().x < 400 * scale) ImGui::NewLine();
        else ImGui::SameLine();
        if (!fixedFooter || !stackFooter)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f,
                (fixedFooter ? footerBody - summaryHeight : buttonHeight - ImGui::GetTextLineHeight()) / 2));
        ImGui::PushStyleColor(ImGuiCol_Text, status.ok || copyingGame ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                                                       : statusColor(status));
        ImGui::TextWrapped("%s", summary.c_str());
        ImGui::PopStyleColor();
        static bool popupBefore = false;
        bool popupNow = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        if (!s_aboutOpen && !ImGui::IsAnyItemActive() && !io.WantTextInput) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !popupBefore && !popupNow) quit = true;
            if (canPlay && ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false)) play = true;
        }
        popupBefore = popupNow;
#ifdef MEDECOMP_ONE_PROCESS
        if (s_editorPending && status.ok) play = true;
        else s_editorPending = false;
        if (port::platform::heroine::previewRequested()) play = true;
#endif
        if (quit) play = false;
        aboutWindow(scale);
        ImGui::End();

        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        port::platform::turn::begin(renderer, window);
        mcuiClearColor(renderer);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        port::platform::turn::end(renderer, window);
#ifdef __EMSCRIPTEN__
        MAIN_THREAD_EM_ASM({ Module.medecompBrowseRect = ([$0, $1, $2, $3]); },
                           browserBrowse.x, browserBrowse.y, browserBrowse.z, browserBrowse.w);
        MAIN_THREAD_EM_ASM({
            Module.medecompPlayRect = ([$0, $1, $2, $3]);
            Module.medecompPlayFocused = !!$4;
            Module.medecompTurnQuarter = $5;
            Module.medecompPlayOptions = ({ fullscreen: !!$6, orientation: UTF8ToString($7) });
        }, browserPlay.x, browserPlay.y, browserPlay.z, browserPlay.w, browserPlayFocused,
           port::platform::turn::quarter(window), port::platform::settings::get("web_fullscreen", "0") == "1",
           port::platform::settings::get("web_orientation", "auto").c_str());
        if (play && !s_editorPending && !port::platform::heroine::previewRequested())
            MAIN_THREAD_EM_ASM({ Module.medecompLaunchDisplay?.(); });
        if (firstFrame) MAIN_THREAD_EM_ASM({ Module.medecompLauncherReady(); });
#endif
        firstFrame = false;
        bool settled = true;
        if (shot && getenv("MEDECOMP_LAUNCHER_SIGNIN")) {
            std::lock_guard<std::mutex> lock(s_account->mu);
            settled = s_account->stage != AccountFlow::Requesting || frames > 80;
        }
        if (shot && s_tab == 7 && frames > 0) {
            std::lock_guard<std::mutex> lock(s_achievementsView->mu);
            settled = settled && (!s_achievementsView->loading || frames > 80);
        }
        if (shot && (s_download.busy || s_download.done)) settled = false;
        {
            namespace update = port::platform::update;
            const update::Status ust = update::status();
            if (getenv("MEDECOMP_LAUNCHER_UPDATE") && ust.stage == update::Stage::Available && ust.installable &&
                strcmp(getenv("MEDECOMP_LAUNCHER_UPDATE"), "install") == 0)
                update::install(s_russian);
            if (shot && frames < 600 &&
                (ust.stage == update::Stage::Checking || ust.stage == update::Stage::Downloading || ust.stage == update::Stage::Installing ||
                 (ust.stage == update::Stage::Available && getenv("MEDECOMP_LAUNCHER_UPDATE"))))
                settled = false;
            if (ust.stage == update::Stage::Ready && ++s_readyFrames >= 2) quit = true;
        }
#ifndef __ANDROID__
        const char* hook = getenv("MEDECOMP_LAUNCHER_LEVEL");
        if (shot && hook && frames < 400 &&
            (strncmp(hook, "upload:", 7) == 0 || strncmp(hook, "unpublish:", 10) == 0 || strncmp(hook, "take:", 5) == 0)) {
            std::lock_guard<std::mutex> lock(s_publish.mu);
            settled = settled && s_publish.done;
        }
        if (shot && s_tab == 6 && frames < 400) {
            std::lock_guard<std::mutex> lock(s_levelCheck.mu);
            settled = settled && !s_levelCheck.key.empty() && !s_levelCheck.busy && s_levelCheck.changed == 0 &&
                      !port::online::cloud::busy();
        }
#endif
        {
            static std::vector<std::pair<SDL_Keycode, Uint16>> keys;
            static size_t keyAt = 0;
            static int keyFrames = -1;
            if (keyFrames < 0) {
                keyFrames = 0;
                std::string list = getenv("MEDECOMP_LAUNCHER_KEYS") ? getenv("MEDECOMP_LAUNCHER_KEYS") : "";
                for (size_t at = 0; at < list.size();) {
                    size_t comma = std::min(list.find(',', at), list.size());
                    std::string name = list.substr(at, comma - at);
                    Uint16 mod = 0;
                    for (size_t plus; (plus = name.find('+')) != std::string::npos && plus + 1 < name.size(); name.erase(0, plus + 1)) {
                        std::string m = name.substr(0, plus);
                        mod |= m == "Ctrl" ? KMOD_LCTRL : m == "Shift" ? KMOD_LSHIFT : m == "Alt" ? KMOD_LALT : 0;
                    }
                    SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
                    if (key != SDLK_UNKNOWN) keys.push_back({key, mod});
                    at = comma + 1;
                }
            }
            if (keyAt < keys.size() && ++keyFrames % 3 == 0) {
                auto push = [&](Uint32 type, SDL_Keycode key, Uint16 mod) {
                    SDL_Event press = {};
                    press.type = type;
                    press.key.windowID = SDL_GetWindowID(window);
                    press.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
                    press.key.keysym.sym = key;
                    press.key.keysym.scancode = SDL_GetScancodeFromKey(key);
                    press.key.keysym.mod = mod;
                    SDL_PushEvent(&press);
                };
                push(SDL_KEYDOWN, keys[keyAt].first, keys[keyAt].second);
                push(SDL_KEYUP, keys[keyAt].first, keys[keyAt].second);
                if (keys[keyAt].second) push(SDL_KEYUP, SDLK_LCTRL, 0);
                keyAt++;
            }
            if (keyAt < keys.size()) settled = false;
            else if (!keys.empty() && keyFrames++ < int(keys.size()) * 3 + 6) settled = false;
        }
        if (shot && ++frames >= 4 && settled && frames < 100000) {
            frames = 100000;
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);
            SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (image && SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, image->pixels, image->pitch) == 0)
                SDL_SaveBMP(image, shot);
            SDL_FreeSurface(image);
            if (status.ok) play = true;
            else quit = true;
        }
        SDL_RenderPresent(renderer);
    }

    if (play && !shot) {
        const Uint32 start = SDL_GetTicks();
        for (int frame = 0; frame < 2 || (port::online::cloud::busy() && SDL_GetTicks() - start < 30000); frame++) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                port::platform::turn::event(&e);
                ImGui_ImplSDL2_ProcessEvent(&e);
            }
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            port::platform::turn::imgui(window);
            ImGui::NewFrame();
            loadingScreen(scale, SDL_GetTicks() - start, port::online::cloud::busy());
            ImGui::Render();
            SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
            port::platform::turn::begin(renderer, window);
            mcuiClearColor(renderer);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            port::platform::turn::end(renderer, window);
            SDL_RenderPresent(renderer);
        }
    }

    if (userResized && !(SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN |
                                                        SDL_WINDOW_FULLSCREEN_DESKTOP))) {
        int w = 0, h = 0;
        SDL_GetWindowSize(window, &w, &h);
        char size[32];
        snprintf(size, sizeof size, "%dx%d", int(w / scale + 0.5f), int(h / scale + 0.5f));
        if (w > 0 && h > 0 && savedSize != size) {
            port::platform::settings::set("launcher_size", size);
            if (!port::platform::settings::save())
                fprintf(stderr, "medecomp: не записать %s\n", port::platform::settings::path().c_str());
        }
    }
    cancelSignIn();
#ifdef __EMSCRIPTEN__
    MAIN_THREAD_EM_ASM({ Module.medecompBrowseRect = null; Module.medecompPlayRect = null; Module.medecompPlayFocused = false; Module.medecompPlayPending = false; Module.medecompInLauncher = false; });
#endif
    if (s_download.thread.joinable()) {
        s_download.cancel = true;
        s_download.thread.join();
        s_download.busy = false;
        s_download.done = false;
        remove(downloadPart().c_str());
    }
    if (s_avatar) SDL_DestroyTexture(s_avatar);
    s_avatar = nullptr;
    port::platform::heroine::shutdown();
    ImGui_ImplSDLRenderer2_Shutdown();
    for (BrandTexture* b : {&s_logoText, &s_logoFull, &s_appIcon})
        if (b->tex) {
            SDL_DestroyTexture(b->tex);
            *b = BrandTexture();
        }
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    port::platform::turn::release(renderer);
    SDL_DestroyRenderer(renderer);
#ifdef __EMSCRIPTEN__
    if (play && status.ok) s_webLauncherWindow = window;
    else SDL_DestroyWindow(window);
#else
    SDL_DestroyWindow(window);
#endif
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    port::platform::hd::stop(true);
    if (port::platform::update::status().stage == port::platform::update::Stage::Ready) {
        if (!port::platform::update::restart()) fprintf(stderr, "medecomp: обновление: не перезапустить — запустите заново\n");
        return false;
    }
    *path = checked;
    if (play && status.ok) {
        port::platform::setWindowModeSetting(port::platform::WindowMode(mode));
        port::platform::setFrameRateSetting(fps);
        port::platform::setUpscaleSetting(port::platform::Upscale(upscale), sharpness);
        port::platform::setWidescreenSetting(wide);
        port::platform::setRenderSetting(render);
        port::platform::setTextureFilterSetting(filter);
        port::platform::setMsaaSetting(msaa);
        port::gfx::setTexturePackSetting(pack);
        port::platform::setPostSetting(post, postStrength);
        port::platform::setSsaoSetting(ssao);
        port::platform::setGodRaysSetting(godRays);
        port::platform::setMotionBlurSetting(motionBlur);
        port::platform::setSsrSetting(ssr, ssrStrength);
        port::gfx::setRunnerShadowsSetting(shadows);
        port::platform::setLensFlareSetting(flare);
        port::platform::setGradeSetting(grade);
        port::platform::setTaaSetting(taa);
        port::platform::setOriginalGraphics(original);
        port::platform::settings::set("language", language);
        port::platform::setDisplaySetting(screen.display);
        port::platform::setResolutionSetting(screen.width, screen.height);
        port::platform::setVsyncSetting(screen.vsync);
        port::platform::setVideoDriverSetting(port::platform::VideoDriver(screen.videoDriver));
        setHrtfSetting(sound.hrtf);
        port::audio::setBackgroundPauseSetting(sound.backgroundPause);
        port::audio::setVolumeSetting(sound.volume);
        port::audio::setMusicSetting(sound.music);
        port::audio::setMusicFolderSetting(sound.folder);
        port::audio::setMusicShuffleSetting(sound.shuffle);
        port::audio::setMusicVolumeSetting(sound.musicVolume);
    }
    return play && status.ok;
}

void usage(const char* argv0) {
    std::string self = argv0 && *argv0 ? argv0 : "medecomp";
    self = self.substr(self.find_last_of("/\\") + 1);
    printf(L(MEDECOMP_PORT_NAME " — PC-порт для Mirror's Edge (iPad)\n  %s ",
             MEDECOMP_PORT_NAME " — PC port for Mirror's Edge (iPad)\n  %s "),
           self.c_str());
    printf("%s",
           L("[путь к .ipa или .app]   — лаунчер с этой копией игры\n"
             "  --game <путь>                    — то же\n"
             "  --no-launcher                    — сразу в игру (копия — из аргумента, MEDECOMP_GAME или прошлого запуска)\n"
             "  --launcher                       — показать лаунчер и при сценарии MEDECOMP_SCRIPT\n"
             "  --sign-in, --sign-out            — войти в аккаунт MoonCellar (код и QR в терминале) или выйти\n"
             "  --dump-textures                  — выгрузить текстуры глав в <папка игрока>/textures/dump\n"
             "  --hd-textures                    — сделать HD-текстуры (Real-ESRGAN ×4) в <папка игрока>/textures\n"
             "  --level <id>                     — уровень игрока <папка игрока>/levels/<id> (на время)\n"
             "  --test                           — с --level: проверка из редактора, без заставок и сверки\n"
             "  --editor [id]                    — редактор уровней игроков\n"
             "  --no-update                      — не проверять обновления\n",
             "[path to .ipa or .app]  — launcher with this game copy\n"
             "  --game <path>                    — the same\n"
             "  --no-launcher                    — start the game right away (copy from the argument, MEDECOMP_GAME or last run)\n"
             "  --launcher                       — show the launcher even with a MEDECOMP_SCRIPT scenario\n"
             "  --sign-in, --sign-out            — sign in to MoonCellar (code and QR in the terminal) or sign out\n"
             "  --dump-textures                  — dump the chapter textures to <player folder>/textures/dump\n"
             "  --hd-textures                    — make HD textures (Real-ESRGAN ×4) in <player folder>/textures\n"
             "  --level <id>                     — player level <player folder>/levels/<id> (time trial)\n"
             "  --test                           — with --level: editor test run, no splashes or sync\n"
             "  --editor [id]                    — player level editor\n"
             "  --no-update                      — don't check for updates\n"));
}

}

namespace port {
namespace platform {

int hdTexturesInTerminal(const std::string& game) {
    namespace hd = port::platform::hd;
    hd::start(game, s_russian);
    std::string last;
    for (;;) {
        hd::Status st = hd::status();
        std::string line = (st.total ? std::to_string(st.done) + "/" + std::to_string(st.total) + " " : "") + st.stage;
        if (line != last && !line.empty()) printf("%s\n", line.c_str());
        last = line;
        if (!st.running) {
            if (!st.error.empty()) fprintf(stderr, "medecomp: %s\n", st.error.c_str());
            hd::stop(true);
            return st.error.empty() ? 0 : 1;
        }
        SDL_Delay(500);
    }
}

std::string launcherFont() { return fontFile(); }
bool launcherFonts() { return loadLauncherFont(); }

void launcherStyle() { mcui::ApplyStyle(1.0f); }

bool launcherRussian() { return russianSystem(); }

void panelTouchScroll(float scale) { touchScroll(scale); }

void syncOnExit() {
    namespace cloud = port::online::cloud;
    if (!cloud::allowed()) return;
    cloud::startSync(s_russian, true);
    cloud::waitSync(300);
    if (!cloud::busy()) {
        cloud::applyDownloadedPrefs();
        return;
    }
    port::platform::applyVideoDriver(false);
    port::platform::setAppId();
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        cloud::waitSync(60000);
        return;
    }
    const int kWidth = 480, kHeight = 170;
    SDL_Window* window = SDL_CreateWindow(MEDECOMP_PORT_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, kWidth, kHeight,
                                          SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
    port::platform::setWindowIcon(window);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!renderer) {
        if (window) SDL_DestroyWindow(window);
        cloud::waitSync(60000);
        return;
    }
    int pixelW = 0, pixelH = 0;
    SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
    float scale = std::max(1.0f, port::platform::displayScale(window, pixelW));
    SDL_SetWindowSize(window, int(kWidth * scale), int(kHeight * scale));
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    bool russian = s_russian;
    if (!loadLauncherFont()) s_russian = false;
    mcui::ApplyStyle(scale);
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    bool skip = false;
    Uint32 start = SDL_GetTicks();
    while (cloud::busy() && !skip) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            port::platform::turn::event(&e);
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) skip = true;
        }
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        port::platform::turn::imgui(window);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##cloud", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        float r = 14 * scale;
        ImVec2 at = ImGui::GetCursorScreenPos();
        spinner(ImVec2(at.x + r + 2 * scale, at.y + r + 2 * scale), r, scale, SDL_GetTicks() - start);
        ImGui::Dummy(ImVec2(2 * r + 4 * scale, 2 * r + 4 * scale));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted(L("Отправляю сохранение в аккаунт…", "Sending the save to the account…"));
        int done = 0, total = 0;
        cloud::progress(&done, &total);
        ImGui::TextDisabled("%s %d / %d", L("Файлов:", "Files:"), done, total);
        ImGui::EndGroup();
        ImGui::Spacing();
        ImGui::TextDisabled("%s", L("Не ждать — лаунчер предложит отправить при следующем запуске.",
                                    "Don't wait — the launcher will offer to send it on the next start."));
        if (ImGui::Button(L("Не ждать", "Don't wait"))) skip = true;
        ImGui::End();
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        port::platform::turn::begin(renderer, window);
        mcuiClearColor(renderer);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        port::platform::turn::end(renderer, window);
        static bool shotTaken = false;
        if (const char* shot = getenv("MEDECOMP_CLOUD_SHOT"); shot && !shotTaken && SDL_GetTicks() - start > 500) {
            shotTaken = true;
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);
            SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (image && SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, image->pixels, image->pitch) == 0)
                SDL_SaveBMP(image, shot);
            SDL_FreeSurface(image);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    port::platform::turn::release(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    s_russian = russian;
    if (!skip) cloud::applyDownloadedPrefs();
    fprintf(stderr, "medecomp: облако после игры — %s\n", skip ? "не дождались (отправится при следующем запуске)" : "готово");
}

bool chooseGame(int argc, char** argv) {
    s_russian = russianSystem();
#ifdef MEDECOMP_VERSION
    port::platform::update::setup(argc, argv, MEDECOMP_VERSION);
#else
    port::platform::update::setup(argc, argv, "dev");
#endif
    std::string path;
    bool force = false, skip = false, dump = false, hdTextures = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--launcher") force = true;
        else if (a == "--no-launcher") skip = true;
        else if (a == "--dump-textures") dump = skip = true;
        else if (a == "--hd-textures") hdTextures = skip = true;
        else if (a == "--level" && i + 1 < argc) {
            ++i;
            skip = true;
        } else if (a == "--editor" || a == "--editor-check") {
            skip = true;
            if (a == "--editor" && i + 1 < argc && argv[i + 1][0] != '-') ++i;
        }
        else if (a == "--game" && i + 1 < argc) path = argv[++i];
        else if (a.compare(0, 7, "--game=") == 0) path = a.substr(7);
        else if (a == "--help" || a == "-h") {
            usage(argv[0]);
            return false;
        } else if (!a.empty() && a[0] != '-') path = a;
    }
    for (const char* var : {"MEDECOMP_GAME", "MEDECOMP_BUNDLE"})
        if (path.empty() && getenv(var)) path = getenv(var);
    if (path.empty()) path = settings::get("game");
    bool scripted = getenv("MEDECOMP_SCRIPT") || getenv("MEDECOMP_SHOT") || getenv("MEDECOMP_SKIN_PREVIEWS");
#if defined(_WIN32) || defined(__APPLE__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    bool display = true;
#else
    bool display = getenv("DISPLAY") || getenv("WAYLAND_DISPLAY");
#endif
    if ((skip || scripted || !display) && !force) {
        std::string error = L("копия игры не указана", "no game copy given");
        bool opened = (!path.empty() && port::files::openGame(path, &error)) ||
                      (path.empty() && !port::files::gameSource().empty());
        if (opened && dump) exit(port::gfx::dumpTextures());
        if (opened && hdTextures) exit(hdTexturesInTerminal(path.empty() ? port::files::gameSource() : path));
        if (opened) return true;
        fprintf(stderr, "medecomp: %s%s\n", display ? "" : L("нет экрана для лаунчера; ", "no display for the launcher; "),
                error.c_str());
        fprintf(stderr, "%s", L("medecomp: укажите .ipa: medecomp <путь> или MEDECOMP_GAME=<путь>\n",
                                "medecomp: give the .ipa: medecomp <path> or MEDECOMP_GAME=<path>\n"));
        return false;
    }
    if (path.empty()) path = port::files::gameSource();
    if (!runWindow(&path)) return false;
    settings::set("game", path);
    if (!settings::save()) fprintf(stderr, "medecomp: не записать %s\n", settings::path().c_str());
    bool opened = port::files::openGame(path, nullptr);
#ifdef __EMSCRIPTEN__
    if (opened) webGameSelected();
#endif
    return opened;
}

#ifdef MEDECOMP_ONE_PROCESS
bool takeEditorRequest(std::string* id) {
    if (!s_editorPending) return false;
    s_editorPending = false;
    *id = s_editorId;
    return true;
}
#endif

#ifdef __EMSCRIPTEN__
SDL_Window* webTakeLauncherWindow() {
    SDL_Window* window = s_webLauncherWindow;
    s_webLauncherWindow = nullptr;
    return window;
}
#endif

}
}

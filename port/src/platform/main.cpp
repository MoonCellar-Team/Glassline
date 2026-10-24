#include <SDL.h>
#ifndef _WIN32
#include <fontconfig/fontconfig.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#if !defined(_WIN32) && !defined(__ANDROID__)
#include <execinfo.h>
#include <signal.h>
#include <sys/prctl.h>
#endif

#include <atomic>
#include <chrono>
#include <new>
#include <string>
#include <thread>

#include "input.h"
#include "launcher.h"
#include "overlay.h"
#include "levels.h"
#include "../ea/cloud.h"
#include "game/LocaleManager.h"
#include "process.h"
#include "turn.h"
#include "heroine_tab.h"
#include "../editor/editor.h"
#include "../editor/level.h"
#include "port/achievements.h"
#include "port/brand.h"
#include "port/files.h"
#include "port/net.h"
#include "port/objc.h"
#include "port/online.h"
#include "port/platform.h"
#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/GLES.h"
#include "runtime/midp/OpenGLESManager.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "sdk/Foundation.h"

namespace m3g {
extern bool g_usingGLES20;
}

namespace {

using namespace port::platform;

const int GL_FRAMEBUFFER = 0x8D40;

midp::RuntimeIPhone* runtime() { return static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime()); }

bool s_active;

void becomeActive() {
    if (s_active) return;
    s_active = true;
    runtime()->applicationDidBecomeActive(nullptr);
}

void resignActive() {
    if (!s_active) return;
    s_active = false;
    inputReset();
    runtime()->applicationWillResignActive(nullptr);
}

std::string screenshotPath() {
    const char* home = getenv("HOME");
    std::string dir = home ? home : ".";
    struct stat st;
    if (stat((dir + "/Pictures").c_str(), &st) == 0 && S_ISDIR(st.st_mode)) dir += "/Pictures";
    dir += "/" MEDECOMP_PORT_NAME;
    mkdir(dir.c_str(), 0755);
    char name[64];
    time_t now = time(nullptr);
    strftime(name, sizeof name, "/screen-%Y%m%d-%H%M%S.png", localtime(&now));
    return dir + name;
}

}

extern "C" bool medecomp_achievement_unlocked(int index) {
    namespace ach = port::achievements;
    const ach::Entry* e = ach::byIndex(index);
    if (!e) return false;
    ach::queueUnlock(e->key, (long long)time(nullptr));
    const std::string token = port::online::token();
    ach::flushAsync(port::online::serverUrl(), token, token.empty() ? std::string() : port::online::userName());
    if (e->icon.empty()) return false;
    const bool ru = port::platform::launcherRussian();
    port::platform::notify(e->titleIn(ru), e->descriptionIn(ru), e->icon, 6.0f);
    return true;
}

extern "C" void medecomp_achievements_saved(void) {
    namespace ach = port::achievements;
    const std::string token = port::online::token();
    const std::string account = token.empty() ? std::string() : port::online::userName();
    ach::queueFromSave(account);
    ach::flushAsync(port::online::serverUrl(), token, account);
}

extern "C" void medecomp_quit_game(void) {
    static std::atomic<bool> sent{false};
    if (sent.exchange(true)) return;
    SDL_Event e{};
    e.type = SDL_QUIT;
    SDL_PushEvent(&e);
}

std::atomic<bool> s_toLauncher(false);

extern "C" void medecomp_back_to_launcher(void) {
    s_toLauncher = true;
    medecomp_quit_game();
}

namespace {

void handle(const SDL_Event& e, bool* running) {
    switch (e.type) {
    case SDL_QUIT:
        *running = false;
        return;
    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            resignActive();
        if (e.window.event == SDL_WINDOWEVENT_RESTORED || e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            becomeActive();
        return;
    case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_F11 || (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT))) {
            if (!e.key.repeat) toggleFullscreen();
            return;
        }
        if (e.key.keysym.sym == SDLK_F12) {
            if (!e.key.repeat) requestScreenshot(screenshotPath());
            return;
        }
        break;
    }
    if (splashActive()) {
        if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONDOWN ||
            e.type == SDL_FINGERDOWN)
            splashSkip();
        if (e.type != SDL_CONTROLLERDEVICEADDED && e.type != SDL_CONTROLLERDEVICEREMOVED) return;
    }
    inputEvent(e);
}

std::atomic<bool> s_quitDone(false);

void openLauncher() {
#if defined(__ANDROID__)
    port::platform::androidRestart();
#else
    std::string self = port::platform::process::selfPath();
    if (self.empty()) return;
    fprintf(stderr, "medecomp: возврат в лаунчер\n");
#ifdef _WIN32
    port::platform::process::spawn({self, "--launcher"}, {});
#else
    fflush(stdout);
    fflush(stderr);
    char launcher[] = "--launcher";
    char* argv[] = {&self[0], launcher, nullptr};
    execv(self.c_str(), argv);
    fprintf(stderr, "medecomp: лаунчер не открыть: %s\n", strerror(errno));
#endif
#endif
}

void quitWatchdog() {
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (s_quitDone) return;
        fprintf(stderr, "medecomp: игра не завершилась за 5 с — выхожу\n");
        if (s_toLauncher) openLauncher();
        _exit(0);
    }).detach();
}

int run() {
    std::string documents = port::files::dataDir() + "/Documents";
    if (mkdir(documents.c_str(), 0755) != 0 && errno != EEXIST)
        fprintf(stderr, "medecomp: не создать %s\n", documents.c_str());
    runtime()->resumeAudio();
    if (!createWindow(MEDECOMP_PORT_NAME)) return 1;
    splashBegin();
    port::objc::setMainThreadWaker(wakeMainLoop);

    View& v = view();
    v.glesMajor = 2;
    v.glesMinor = 0;
    m3g::g_usingGLES20 = true;
    v.gl = reinterpret_cast<midp::GLES*>(midp::GLES::InstanceCachedGLES20());
    midp::OpenGLESManager::setGLES(v.gl);
    v.gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer());
    v.gl->glViewport(0, 0, framebufferPixelWidth(), framebufferPixelHeight());
    v.display = new midp::DisplayIPhone(kViewWidth, kViewHeight, &v);

    runtime()->applicationDidFinishLaunching(nullptr);
    becomeActive();
    releaseContext();

    bool running = true;
    Uint32 lastAccel = 0;
    while (running) {
        NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
        int wait = msUntilNextTimer(), poll = inputPollMs();
        if (wait < 0 || wait > poll) wait = poll;
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, wait)) {
            port::platform::turn::event(&e);
            handle(e, &running);
            while (running && SDL_PollEvent(&e)) {
                port::platform::turn::event(&e);
                handle(e, &running);
            }
        }
        inputUpdate();
        if (textInputActive()) presentTextField();
        port::objc::runMainThreadQueue();
        runTimers();
        Uint32 now = SDL_GetTicks();
        if (now - lastAccel >= 33) {
            lastAccel = now;
            runAccelerometer();
        }
        yieldContext();
        pool->release();
    }

    quitWatchdog();
    resignActive();
    runtime()->applicationWillTerminate(nullptr);
    destroyWindow();
    return 0;
}

}

#ifndef _WIN32
void bundledFonts() {
    std::string self = port::platform::process::selfPath();
    std::string dir = self.substr(0, self.find_last_of('/') + 1) + "fonts";
    struct stat st;
    if (self.empty() || stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) return;
    if (!getenv("FONTCONFIG_FILE") && stat("/etc/fonts/fonts.conf", &st) != 0 && stat((dir + "/fonts.conf").c_str(), &st) == 0)
        setenv("FONTCONFIG_FILE", (dir + "/fonts.conf").c_str(), 1);
    if (FcInit() && FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>(dir.c_str())))
        fprintf(stderr, "medecomp: шрифты сборки — %s\n", dir.c_str());
}
#endif

#ifndef _WIN32
void logToFile() {
    struct stat st;
    if (getenv("MEDECOMP_LOG_FILE")) return;
    setenv("MEDECOMP_LOG_FILE", "-", 1);
    if (isatty(2)) return;
    if (fstat(2, &st) == 0 && (S_ISREG(st.st_mode) || S_ISFIFO(st.st_mode))) return;
    std::string path = port::files::dataDir() + "/medecomp.log";
    mkdir(port::files::dataDir().c_str(), 0755);
    rename(path.c_str(), (port::files::dataDir() + "/medecomp.prev.log").c_str());
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;
    dup2(fileno(f), 1);
    dup2(fileno(f), 2);
    fclose(f);
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setenv("MEDECOMP_LOG_FILE", path.c_str(), 1);
}
#endif

#if !defined(_WIN32) && !defined(__ANDROID__)
void crashSignal(int sig, siginfo_t* info, void*) {
    const char* name = sig == SIGSEGV ? "SIGSEGV" : sig == SIGBUS ? "SIGBUS" : sig == SIGFPE ? "SIGFPE"
                     : sig == SIGILL ? "SIGILL" : sig == SIGABRT ? "SIGABRT" : "?";
    char thread[17] = {};
    prctl(PR_GET_NAME, thread, 0, 0, 0);
    char where[2 + 16 + 1] = "0x";
    uintptr_t at = info ? reinterpret_cast<uintptr_t>(info->si_addr) : 0;
    for (int i = 0; i < 16; i++) where[2 + i] = "0123456789abcdef"[(at >> (60 - 4 * i)) & 15];
    where[18] = 0;
    const char* parts[] = {"medecomp: сбой — ", name, " по адресу ", where, ", поток «", thread, "», стек:\n"};
    for (const char* s : parts) {
        ssize_t written = write(2, s, strlen(s));
        (void)written;
    }
    void* frames[64];
    backtrace_symbols_fd(frames, backtrace(frames, 64), 2);
    signal(sig, SIG_DFL);
    raise(sig);
}

void crashHandlers() {
    void* warm[1];
    backtrace(warm, 1);
    struct sigaction sa = {};
    sa.sa_sigaction = crashSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_SIGINFO;
    for (int sig : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT}) sigaction(sig, &sa, nullptr);
}
#endif

#if defined(_WIN32) || defined(__ANDROID__)
void systemLanguage() {
    for (const char* var : {"MEDECOMP_LANG", "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"})
        if (const char* v = getenv(var); v && *v) return;
    SDL_Locale* locales = SDL_GetPreferredLocales();
    if (!locales) return;
    if (locales[0].language) {
        std::string lang = locales[0].language;
        if (locales[0].country) lang += std::string("_") + locales[0].country;
#ifdef _WIN32
        _putenv_s("LANG", lang.c_str());
#else
        setenv("LANG", lang.c_str(), 1);
#endif
        fprintf(stderr, "medecomp: язык системы — %s\n", lang.c_str());
    }
    SDL_free(locales);
}
#endif

int main(int argc, char** argv) {
#if defined(__ANDROID__)
    port::platform::androidStartup();
    systemLanguage();
#elif defined(_WIN32)
    port::platform::windowsStartup();
    systemLanguage();
#else
    logToFile();
    crashHandlers();
    bundledFonts();
#endif
    port::platform::applyVideoDriver(false);
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--sign-in") return port::online::consoleSignIn();
        if (a == "--sign-out") return port::online::consoleSignOut();
    }
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        std::string next = i + 1 < argc ? argv[i + 1] : "";
        if (a == "--net-host" || a == "--net-connect") return port::net::selfTest(a == "--net-host", next);
        if (a == "--net-room" || a == "--net-join") return port::net::selfTestRoom(a == "--net-room", next);
    }
    bool opened = false;
#ifdef __ANDROID__
    std::string resume;
    if (port::editor::takeResume(&resume)) {
        char noLauncher[] = "--no-launcher";
        char* direct[] = {argv[0], noLauncher, nullptr};
        opened = port::platform::chooseGame(2, direct);
        if (!opened) resume.clear();
    }
#endif
    if (!opened && !port::platform::chooseGame(argc, argv)) return 0;
    port::platform::applyVideoDriver(true);
    LocaleManager* locales = LocaleManager::getInstance();
    locales->~LocaleManager();
    new (locales) LocaleManager();
#ifdef __ANDROID__
    {
        std::string id;
        std::vector<std::string> env;
        if (port::platform::heroine::takePreviewRequest(&env))
            for (const std::string& kv : env) {
                size_t eq = kv.find('=');
                setenv(kv.substr(0, eq).c_str(), kv.substr(eq + 1).c_str(), 1);
            }
        if (!resume.empty()) id = resume;
        if (!resume.empty() || port::platform::takeEditorRequest(&id)) {
            int code = port::editor::run(id, argv[0]);
            std::string level, error;
            if (port::editor::takeServiceRun(&env, &level)) {
                for (const std::string& kv : env) {
                    size_t eq = kv.find('=');
                    setenv(kv.substr(0, eq).c_str(), kv.substr(eq + 1).c_str(), 1);
                }
            } else if (port::editor::takeTestLevel(&level)) {
                port::platform::setTestRun(true);
            } else {
                port::platform::androidRestart();
                return code;
            }
            if (!level.empty() && !port::platform::preparePlayerLevel(level, &error)) {
                fprintf(stderr, "medecomp: уровень %s: %s\n", level.c_str(), error.c_str());
                return 1;
            }
        }
    }
#endif
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--editor-check") return port::editor::selfCheck();
        if (a == "--editor") return port::editor::run(i + 1 < argc && argv[i + 1][0] != '-' ? argv[i + 1] : "", argv[0]);
    }
    for (int i = 1; i < argc; i++)
        if (std::string(argv[i]) == "--test") port::platform::setTestRun(true);
    const bool test = port::platform::testRun();
    if (!test && !port::online::token().empty()) {
        const bool ru = port::platform::launcherRussian();
        const std::string name = port::online::userName();
        std::string text = ru ? (name.empty() ? std::string("Вход выполнен") : "Вход выполнен: " + name)
                              : (name.empty() ? std::string("Signed in") : "Signed in as " + name);
        text += ru ? ". Время заездов уходит в таблицы лидеров" : ". Your times go to the leaderboards";
        port::platform::notify("MoonCellar", text);
    }
    if (!test) {
        const std::string token = port::online::token();
        const std::string account = token.empty() ? std::string() : port::online::userName();
        port::achievements::queueFromSave(account);
        port::achievements::flushAsync(port::online::serverUrl(), token, account);
    }
    if (const char* a = getenv("MEDECOMP_ACHIEVEMENT")) medecomp_achievement_unlocked(atoi(a));
    if (const char* n = getenv("MEDECOMP_NOTIFY")) {
        std::string s = n, icon;
        size_t bar = s.find('|');
        if (const char* path = getenv("MEDECOMP_NOTIFY_ICON"))
            if (FILE* f = fopen(path, "rb")) {
                char chunk[4096];
                for (size_t got; (got = fread(chunk, 1, sizeof chunk, f)) > 0;) icon.append(chunk, got);
                fclose(f);
            }
        port::platform::notify(s.substr(0, bar), bar == std::string::npos ? std::string() : s.substr(bar + 1), icon);
    }
    for (int i = 1; i + 1 < argc; i++) {
        std::string error;
        if (std::string(argv[i]) == "--level" && !port::platform::preparePlayerLevel(argv[i + 1], &error)) {
            fprintf(stderr, "medecomp: уровень %s: %s\n", argv[i + 1], error.c_str());
            return 1;
        }
    }
    if (!test) {
        port::online::cloud::waitSync(30000);
        port::online::cloud::applyDownloadedPrefs();
        port::online::cloud::markPlaying();
    }
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    midp::Runtime* rt = midp::Runtime::getRuntime();
    midp::INCREF(rt);
    int code = run();
    s_quitDone = true;
    port::net::sessionClose();
    port::net::shutdown();
    if (!test) port::platform::syncOnExit();
    midp::DECREF(rt);
    pool->release();
    if (s_toLauncher) openLauncher();
    return code;
}

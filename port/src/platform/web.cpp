#ifdef __EMSCRIPTEN__

#include <SDL.h>
#include <emscripten/emscripten.h>
#include <emscripten/wasmfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <new>

#include <mutex>
#include <vector>

#include "game/LocaleManager.h"
#include "port/files.h"
#include "port/platform.h"
#include "settings.h"

namespace port {
namespace platform {

namespace {

std::mutex s_stepsMu;
std::vector<std::function<bool()>> s_newSteps;
std::vector<std::function<bool()>> s_steps;

}

void webGameStep(std::function<bool()> step) {
    std::lock_guard<std::mutex> lock(s_stepsMu);
    s_newSteps.push_back(std::move(step));
}

void webGameFrame() {
    {
        std::lock_guard<std::mutex> lock(s_stepsMu);
        for (auto& step : s_newSteps) s_steps.push_back(std::move(step));
        s_newSteps.clear();
    }
    for (size_t i = 0; i < s_steps.size();) {
        if (s_steps[i]())
            i++;
        else
            s_steps.erase(s_steps.begin() + long(i));
    }
}

void webStartup() {
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
    rmdir("/opfs/data");
    rmdir("/opfs");
    if (backend_t opfs = wasmfs_create_opfs_backend()) {
        if (wasmfs_create_directory("/opfs", 0777, opfs) != 0) fprintf(stderr, "medecomp: OPFS не подключить в /opfs\n");
    } else {
        fprintf(stderr, "medecomp: нет OPFS — копии игры и сохранений не будет\n");
    }
    std::string savedGame = settings::get("game");
    if (savedGame.empty() && port::files::stat("/opfs/game.ipa").exists) savedGame = "/opfs/game.ipa";
    if (!savedGame.empty()) setenv("MEDECOMP_GAME", savedGame.c_str(), 0);
    setenv("MEDECOMP_DATA", "/opfs/data", 0);
    setenv("HOME", "/opfs/data", 0);
    mkdir(getenv("MEDECOMP_DATA"), 0777);
    std::string error;
    if (getenv("MEDECOMP_GAME") && !port::files::openGame(getenv("MEDECOMP_GAME"), &error))
        fprintf(stderr, "medecomp: копию игры (%s) не открыть: %s\n", getenv("MEDECOMP_GAME"), error.c_str());
    const std::string imports = port::files::dataDir() + "/game-imports";
    const std::string activeGame = getenv("MEDECOMP_GAME") ? getenv("MEDECOMP_GAME") : savedGame;
    for (const auto& name : port::files::list(imports)) {
        const std::string path = imports + "/" + name;
        if (path != activeGame && name.compare(0, 5, "game-") == 0) unlink(path.c_str());
    }
    webGameSelected();
}

void webGameSelected() {
    LocaleManager::s_instance.~LocaleManager();
    new (&LocaleManager::s_instance) LocaleManager();
}

void appRestart() {
    sync();
    MAIN_THREAD_EM_ASM({ Module.medecompToLauncher(); });
}

}
}

#endif

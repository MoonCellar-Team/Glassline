#include "game/AppEngineThread.h"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <memory>
#include <thread>

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/SceneGame.h"
#include "capture.h"
#include "levels.h"
#include "port/net.h"
#include "port/online.h"
#include "port/platform.h"
#include "runtime/midp/System.h"

AppEngineThread::AppEngineThread(AppEngine* engine)
    : m_lastTime(midp::System::currentTimeMillis()), m_engine(engine) {
    midp::INCREF(engine);
}

AppEngineThread::~AppEngineThread() {
    midp::DECREF(m_engine);
}

static void finishLevelForScript() {
    AppEngine* app = AppEngine::getCanvas();
    if (!app || app->m_sceneId != 3) return;
    SceneGame* game = app->getSceneGame();
    if (game && game->getState() == SceneGame::STATE_GAME) game->completeLevel();
}

namespace {

using Clock = std::chrono::steady_clock;

struct Loop {
    Clock::time_point last, logStart;
    double carry = 0;
    int frames = 0;
    long long gameMs = 0;
    unsigned long presents = 0;
    bool log = false, capture = false;

    Loop() {
        log = getenv("MEDECOMP_FPS_LOG") != nullptr;
        last = logStart = Clock::now();
        presents = port::platform::presentCount();
        capture = port::platform::capture::active();
    }
};

void frame(AppEngineThread* t, Loop& l, bool pace) {
    AppEngine* engine = t->m_engine;
    Clock::time_point now = Clock::now();
    int rate = l.capture ? 60 : port::platform::frameTimerRate();
    if (pace && rate > 0) {
        Clock::time_point due = l.last + std::chrono::duration_cast<Clock::duration>(
                                             std::chrono::duration<double>(1.0 / rate));
        if (now < due) {
            std::this_thread::sleep_until(due);
            now = due;
        }
    }
    if (port::platform::inBackground()) {
        port::platform::releaseContext();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        l.last = Clock::now();
        l.carry = 0;
        return;
    }
    if (port::platform::presentSplash()) {
        if (pace && rate <= 0) std::this_thread::sleep_for(std::chrono::milliseconds(4));
        l.last = now;
        l.carry = 0;
        return;
    }
    double ms = std::chrono::duration<double, std::milli>(now - l.last).count() + l.carry;
    l.last = now;
    int dt = ms > 0 ? int(ms) : 0;
    l.carry = ms - dt;
    t->m_lastTime = midp::System::currentTimeMillis();
    if (engine->m_running && !engine->m_paused) {
        Clock::time_point workStart = Clock::now();
        if (l.capture) dt = port::platform::capture::frameBegin();
        l.gameMs += dt;
        engine->m_updating = true;
        engine->runLoop(dt);
        engine->m_updating = false;
        port::platform::menuFocusApply();
        port::platform::debugFrame(dt);
        port::net::raceFrame(dt);
        port::platform::playerLevelFrame();
        port::online::frame();
        if (port::platform::takeScriptFinish()) finishLevelForScript();
        if (int index = port::platform::takeScriptAchievement(); index >= 0) {
            AchievementData* data = AppEngine::getAchievementData();
            if (data && index < data->getAchievementNum()) data->registerAchievementComplete(index);
        }
        if (engine->m_running) {
            engine->m_painting = true;
            engine->repaint();
            engine->serviceRepaints();
            engine->m_painting = false;
        }
        if (l.capture) port::platform::capture::frameEnd();
        port::platform::perfFrame(std::chrono::duration<double, std::milli>(Clock::now() - workStart).count());
    }
    if (l.log && ++l.frames && now - l.logStart >= std::chrono::seconds(2)) {
        double s = std::chrono::duration<double>(now - l.logStart).count();
        unsigned long p = port::platform::presentCount();
        fprintf(stderr, "medecomp: кадров в секунду: игра %.1f, вывод %.1f; время игры %.0f мс за секунду\n",
                l.frames / s, (p - l.presents) / s, l.gameMs / s);
        l.frames = 0;
        l.gameMs = 0;
        l.presents = p;
        l.logStart = now;
    }
}

}

void AppEngineThread::run() {
    midp::INCREF(this);
#ifdef __EMSCRIPTEN__
    auto loop = std::make_shared<Loop>();
    port::platform::webGameStep([this, loop] {
        if (m_engine->m_running && m_engine->m_thread == this) {
            frame(this, *loop, false);
            return true;
        }
        midp::DECREF(this);
        return false;
    });
#else
    Loop loop;
    while (m_engine->m_running) frame(this, loop, true);
    midp::DECREF(this);
#endif
}

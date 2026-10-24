#include "game/AppEngineThread.h"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
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

void AppEngineThread::run() {
    using Clock = std::chrono::steady_clock;
    midp::INCREF(this);
    const bool log = getenv("MEDECOMP_FPS_LOG") != nullptr;
    Clock::time_point last = Clock::now(), logStart = last;
    double carry = 0;
    int frames = 0;
    long long gameMs = 0;
    unsigned long presents = port::platform::presentCount();
    const bool capture = port::platform::capture::active();
    while (m_engine->m_running) {
        Clock::time_point now = Clock::now();
        int rate = capture ? 60 : port::platform::frameTimerRate();
        if (rate > 0) {
            Clock::time_point due = last + std::chrono::duration_cast<Clock::duration>(
                                               std::chrono::duration<double>(1.0 / rate));
            if (now < due) {
                std::this_thread::sleep_until(due);
                now = due;
            }
        }
        if (port::platform::inBackground()) {
            port::platform::releaseContext();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            last = Clock::now();
            carry = 0;
            continue;
        }
        if (port::platform::presentSplash()) {
            if (rate <= 0) std::this_thread::sleep_for(std::chrono::milliseconds(4));
            last = now;
            carry = 0;
            continue;
        }
        double ms = std::chrono::duration<double, std::milli>(now - last).count() + carry;
        last = now;
        int dt = ms > 0 ? int(ms) : 0;
        carry = ms - dt;
        m_lastTime = midp::System::currentTimeMillis();
        if (m_engine->m_running && !m_engine->m_paused) {
            Clock::time_point workStart = Clock::now();
            if (capture) dt = port::platform::capture::frameBegin();
            gameMs += dt;
            m_engine->m_updating = true;
            m_engine->runLoop(dt);
            m_engine->m_updating = false;
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
            if (m_engine->m_running) {
                m_engine->m_painting = true;
                m_engine->repaint();
                m_engine->serviceRepaints();
                m_engine->m_painting = false;
            }
            if (capture) port::platform::capture::frameEnd();
            port::platform::perfFrame(std::chrono::duration<double, std::milli>(Clock::now() - workStart).count());
        }
        if (log && ++frames && now - logStart >= std::chrono::seconds(2)) {
            double s = std::chrono::duration<double>(now - logStart).count();
            unsigned long p = port::platform::presentCount();
            fprintf(stderr, "medecomp: кадров в секунду: игра %.1f, вывод %.1f; время игры %.0f мс за секунду\n",
                    frames / s, (p - presents) / s, gameMs / s);
            frames = 0;
            gameMs = 0;
            presents = p;
            logStart = now;
        }
    }
    midp::DECREF(this);
}

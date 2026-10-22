#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <string>

#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/GameCamera.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/QuadManager.h"
#include "game/SceneGame.h"
#include "port/files.h"
#include "port/platform.h"

namespace {

const int kFadeAnim = 11;

const char* const kStates[] = {
    "NONE", "START", "IDLE", "RUNNING", "RUNNING_TURNING", "BALANCE_RUNNING", "BALANCE_FELL", "STOPPING",
    "FALLING_VERTICALLY", "FALLING_FORWARDS", "10", "ROLLING", "CRASH", "WALL_CLIMB", "CLAMBER", "HANG",
    "HANG_CLAMBER", "SLIDING", "WALL_RUN", "WALL_SLIDE", "SWINGING", "SCALE_JUMP", "SCALE_HAUL_UP",
    "RAMP_RUN_UP", "RAMP_SLIDE", "ZIPLINE_JUMP_TO", "ZIPLINE", "ZIPLINE_FAIL", "MELEED_IN_THE_FACE",
    "DISARMING_COP", "LYING", "GETTING_UP", "GETUP_MELEE", "FLYKICK", "DYING", "DEAD",
};

std::string stateName(int s) {
    return s >= 0 && s < int(sizeof kStates / sizeof kStates[0]) ? kStates[s] : std::to_string(s);
}

FILE* s_file;
bool s_on;
long long s_startMs, s_lastLine;
int s_lastState = -1, s_lastCamState = -1, s_lastFadeKey = -100;
bool s_lastBad, s_lastAnim, s_lastFadeDone = true;

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void out(bool important, const char* fmt, ...) {
    char line[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    double t = (nowMs() - s_startMs) / 1000.0;
    if (s_file) {
        fprintf(s_file, "%9.3f %s\n", t, line);
        fflush(s_file);
    }
    if (important) fprintf(stderr, "medecomp: отладка %.3f %s\n", t, line);
}

bool bad(float v) { return !isfinite(v); }
bool bad(const MathVector& v) { return bad(v.x) || bad(v.y) || bad(v.z); }

std::string vec(const MathVector& v) {
    char b[96];
    snprintf(b, sizeof b, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
    return b;
}

}

namespace port {
namespace platform {

void debugFrame(int dt) {
    static bool checked = false;
    if (!checked) {
        checked = true;
        s_on = getenv("MEDECOMP_DEBUG") != nullptr;
        if (s_on) {
            std::string path = port::files::dataDir() + "/debug.log";
            s_file = fopen(path.c_str(), "w");
            s_startMs = nowMs();
            fprintf(stderr, "medecomp: отладочный журнал: %s\n", path.c_str());
        }
    }
    if (!s_on) return;
    AppEngine* app = AppEngine::getCanvas();
    if (!app) return;

    AnimPlayerQuad* fade = app->m_quads.getAnimPlayer(kFadeAnim);
    int fadeKey = fade ? fade->getCurrentKeyframe() : -1;
    bool fadeDone = app->m_fadeDone;
    if (fadeKey != s_lastFadeKey || fadeDone != s_lastFadeDone) {
        out(false, "затемнение: кадр %d, %s (сцена %d)", fadeKey, fadeDone ? "закончено" : "идёт", app->m_sceneId);
        s_lastFadeKey = fadeKey;
        s_lastFadeDone = fadeDone;
    }
    if (app->m_sceneId != 3) return;
    SceneGame* scene = app->getSceneGame();
    if (!scene || scene->m_loadingStep < 13 || scene->m_gameScreens.m_length <= 0) return;
    GameScreen* screen = scene->getGameScreen(0);
    if (!screen) return;
    GameObjectPlayer* player = screen->getPlayerObject();
    GameCamera* cam = screen->getGameCamera();
    if (!player || !cam) return;

    int state = int(player->m_state);
    if (state != s_lastState) {
        out(false, "героиня: %s -> %s, pos %s vel %s", stateName(s_lastState).c_str(), stateName(state).c_str(),
            vec(player->m_position).c_str(), vec(player->m_velocity).c_str());
        s_lastState = state;
    }
    if (cam->m_state != s_lastCamState || cam->m_dataAnimPlaying != s_lastAnim) {
        out(false, "камера: состояние %d, анимация %s", cam->m_state, cam->m_dataAnimPlaying ? "идёт" : "нет");
        s_lastCamState = cam->m_state;
        s_lastAnim = cam->m_dataAnimPlaying;
    }

    float m[16];
    cam->m_transform.get(m);
    bool badMatrix = false;
    for (float v : m) badMatrix |= bad(v);
    float lookAt[3] = {cam->m_lookAtX.m_output, cam->m_lookAtY.m_output, cam->m_lookAtZ.m_output};
    bool isBad = bad(player->m_position) || bad(player->m_velocity) || bad(cam->m_lookFrom) ||
                 bad(cam->m_lookFromTarget) || bad(cam->m_lookFromVelocity) || bad(cam->m_lookAt) ||
                 bad(lookAt[0]) || bad(lookAt[1]) || bad(lookAt[2]) || bad(cam->m_fov.m_output) || badMatrix;

    long long now = nowMs();
    bool periodic = now - s_lastLine >= 500;
    if (periodic || isBad != s_lastBad) {
        s_lastLine = now;
        out(isBad != s_lastBad,
            "%s%s dt %d | героиня %s pos %s vel %s | камера from %s target %s vel %s at %s "
            "lookAt (%.3f, %.3f, %.3f) fov %.2f blend %.2f matrix %s | наклон %.2f",
            isBad ? (s_lastBad ? "NaN держится: " : "ПОЯВИЛСЯ NaN: ") : (s_lastBad ? "NaN пропал: " : ""), "",
            dt, stateName(state).c_str(), vec(player->m_position).c_str(), vec(player->m_velocity).c_str(),
            vec(cam->m_lookFrom).c_str(), vec(cam->m_lookFromTarget).c_str(), vec(cam->m_lookFromVelocity).c_str(),
            vec(cam->m_lookAt).c_str(), lookAt[0], lookAt[1], lookAt[2], cam->m_fov.m_output,
            cam->m_animBlend.m_output, badMatrix ? "NaN" : "ok", scene->m_tiltFilter.m_output);
        s_lastBad = isBad;
    }
}

}
}

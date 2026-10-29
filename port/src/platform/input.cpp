#include "bindings.h"
#include "capture.h"
#include "input.h"
#include "turn.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <map>
#include <atomic>
#include <string>
#include <vector>

#include "game/AppEngine.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/LevelData.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "port/net.h"
#include "port/platform.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/Runtime.h"
#include "runtime/midp/String.h"

namespace port {
namespace platform {

namespace {

midp::Runtime* runtime() { return midp::Runtime::getRuntime(); }

struct Touch {
    int x, y;
};
std::map<long long, Touch> s_touches;
const long long kMouse = -1;
const long long kSynthTap = -10;
const long long kSynthSwipe = -20;
const long long kSynthCursor = -30;
const long long kScriptFinger = -40;
thread_local bool s_mouseCall = false;
struct {
    int x = -1000, y = -1000, count = 0;
    Uint32 time = 0;
} s_lastTap;

bool pointerAt(const midp::Runtime::PointerState* s, int x, int y) { return s->m_x == x && s->m_y == y; }

int tapCount(int x, int y) {
    Uint32 now = SDL_GetTicks();
    bool again = now - s_lastTap.time < 350 && abs(x - s_lastTap.x) < 30 && abs(y - s_lastTap.y) < 30;
    s_lastTap.count = again ? s_lastTap.count + 1 : 1;
    s_lastTap.x = x;
    s_lastTap.y = y;
    s_lastTap.time = now;
    return s_lastTap.count;
}

void deliver(int kind, int x, int y, int px, int py, int tap, bool mouse) {
    if (capture::active())
        capture::postPointer(kind, x, y, px, py, tap, mouse);
    else
        pointerCall(kind, x, y, px, py, tap, mouse);
}

void touchBegan(long long id, int x, int y) {
    if (s_touches.count(id)) return;
    s_touches[id] = {x, y};
    deliver(kPointerPressed, x, y, x, y, tapCount(x, y), id == kMouse);
}

void touchMoved(long long id, int x, int y) {
    auto it = s_touches.find(id);
    if (it == s_touches.end()) return;
    int px = it->second.x, py = it->second.y;
    it->second = {x, y};
    deliver(kPointerDragged, x, y, px, py, 0, id == kMouse);
}

void touchEnded(long long id, int x, int y) {
    auto it = s_touches.find(id);
    if (it == s_touches.end()) return;
    int px = it->second.x, py = it->second.y;
    s_touches.erase(it);
    deliver(kPointerReleased, x, y, px, py, 0, id == kMouse);
}

void touchEnded(long long id) {
    auto it = s_touches.find(id);
    if (it != s_touches.end()) touchEnded(id, it->second.x, it->second.y);
}

void fingerPoint(const SDL_TouchFingerEvent& f, int* x, int* y) {
    int w, h;
    windowSize(&w, &h);
    toScreen(f.x * w, f.y * h, x, y);
}

SceneGame* scene() {
    AppEngine* canvas = AppEngine::getCanvas();
    return canvas ? canvas->getSceneGame() : nullptr;
}

bool inGameplay() {
    SceneGame* s = scene();
    if (!s || s->getState() != SceneGame::STATE_GAME) return false;
    if (s->m_gameScreens.m_length == 0) return true;
    return s->getGameScreen(0)->getState() < GameScreen::SCREEN_MENU_MAIN;
}

bool popupShown() {
    SceneGame* s = scene();
    return s && s->getState() == SceneGame::STATE_POPUP_BOX;
}

GameScreen* screenOf(int player) {
    SceneGame* s = scene();
    if (!s || s->m_gameScreens.m_length == 0) return nullptr;
    if (player >= s->m_gameScreens.m_length) player = 0;
    return s->getGameScreen(player);
}

void toCanvas(GameScreen* g, int lx, int ly, int* x, int* y) {
    *x = lx;
    *y = ly;
    if (g) g->calcGlobalCoordinates(lx, ly, *x, *y);
}

void center(int player, int* lx, int* ly, GameScreen** g) {
    *g = screenOf(player);
    *lx = *g ? (*g)->m_width / 2 : kScreenWidth / 2;
    *ly = *g ? (*g)->m_height / 2 : kScreenHeight / 2;
}

struct Release {
    Uint32 at;
    long long id;
};
std::vector<Release> s_releases;
const Uint32 kSwipeHoldMs = 80;
const int kSwipeLength = 100;
const float kKeyTilt = 0.12f;
const float kStickTilt = 0.18f;
const Uint32 kRepeatMs = 220;
const int kWheelPx = 48;
const float kStickScroll = 900.0f;
float s_wheel;

void swipe(int player, int dx, int dy, bool twoFingers) {
    GameScreen* g;
    int lx, ly;
    center(player, &lx, &ly, &g);
    for (int f = 0; f < (twoFingers ? 2 : 1); f++) {
        long long id = kSynthSwipe - 2 * player - f;
        if (s_touches.count(id)) touchEnded(id);
        int ox = dy ? 60 * f : 0, oy = dx ? 60 * f : 0;
        int x0, y0, x1, y1;
        toCanvas(g, lx + ox, ly + oy, &x0, &y0);
        toCanvas(g, lx + ox + dx * kSwipeLength, ly + oy + dy * kSwipeLength, &x1, &y1);
        touchBegan(id, x0, y0);
        touchMoved(id, x1, y1);
        s_releases.push_back({SDL_GetTicks() + kSwipeHoldMs, id});
    }
}

void tapDown(int player) {
    GameScreen* g;
    int lx, ly, x, y;
    center(player, &lx, &ly, &g);
    toCanvas(g, lx, ly, &x, &y);
    touchBegan(kSynthTap - player, x, y);
}

void tapUp(int player) { touchEnded(kSynthTap - player); }

void pause(int player) { swipe(player, 0, 1, true); }

void cursorPoint(int* x, int* y) {
    int wx = 0, wy = 0;
    SDL_GetMouseState(&wx, &wy);
    turn::point(SDL_GetMouseFocus(), &wx, &wy);
    toScreen(float(wx), float(wy), x, y);
}

float s_keyTilt;
bool s_keyTiltLeft, s_keyTiltRight;
bool s_arrowTiltLeft, s_arrowTiltRight;
int s_heldDx, s_heldDy;
bool s_heldTwo;
Uint32 s_heldNext;
bool s_cursorOn = true;
bool s_fromPad = false;
std::atomic<bool> s_touchPlay(false);
std::atomic<bool> s_precise(false);
bool s_warping = false;
Uint32 s_mouseAt;
Uint32 s_fingerAt;

bool emulatedMouse() { return s_fingerAt && SDL_GetTicks() - s_fingerAt < 800; }

void gesture(const char* name) {
    const char* debug = getenv("MEDECOMP_DEBUG");
    if (debug && debug[0] && debug[0] != '0') fprintf(stderr, "medecomp: жест: %s\n", name);
}

void showCursor() {
    if (s_cursorOn) return;
    SDL_ShowCursor(SDL_ENABLE);
    s_cursorOn = true;
}

void hideCursor() {
    if (!s_cursorOn) return;
    SDL_ShowCursor(SDL_DISABLE);
    s_cursorOn = false;
}

const int kPauseSize = 72;
const int kPauseInset = 24;
const int kPauseReach = 12;

bool pauseShown(GameScreen* g) {
    SceneGame* s = scene();
    return s_touchPlay && g && s && s->getState() == SceneGame::STATE_GAME && g->isInGameState();
}

void pauseCorner(GameScreen* g, int* x, int* y) {
    int extra = wideExtra();
#ifndef MEDECOMP_SPLIT_STACKED
    if (g->m_type != GameScreen::TYPE_SINGLE) extra = 0;
#endif
    *x = kPauseInset - extra;
    *y = kPauseInset;
    if (*x + kPauseSize > 0 && port::net::pingBottom() > 0) *y = port::net::pingBottom() + kPauseInset / 2;
}

bool pauseButton(float fx, float fy) {
    SceneGame* s = scene();
    for (int p = 0; s && p < s->m_gameScreens.m_length; p++) {
        GameScreen* g = s->getGameScreen(p);
        int lx, ly, x, y;
        if (!pauseShown(g) || !g->calcLocalCoordinates(int(floorf(fx)), int(floorf(fy)), lx, ly)) continue;
        pauseCorner(g, &x, &y);
        if (lx < x - kPauseReach || ly < y - kPauseReach || lx >= x + kPauseSize + kPauseReach ||
            ly >= y + kPauseSize + kPauseReach)
            continue;
        gesture("пауза (кнопка)");
        pause(p);
        return true;
    }
    return false;
}

void keyTilt() {
    bool left = s_keyTiltLeft || s_arrowTiltLeft;
    bool right = s_keyTiltRight || s_arrowTiltRight;
    s_keyTilt = (right ? kKeyTilt : 0.0f) - (left ? kKeyTilt : 0.0f);
    setTilt(s_keyTilt);
}

bool onBalance(int player) {
    if (player != 0 || !inGameplay()) return false;
    AppEngine* app = AppEngine::getCanvas();
    if (!app || !app->getLevelData() || app->getLevelData()->isMultiplayerMode()) return false;
    GameScreen* g = screenOf(0);
    if (!g || !g->m_player) return false;
    GameObjectPlayer::PlayerState st = g->m_player->getState();
    return st == GameObjectPlayer::STATE_BALANCE_RUNNING || st == GameObjectPlayer::STATE_ZIPLINE;
}

using bindings::Action;

Action actionOf(SDL_Keycode sym) {
    if (sym == SDLK_UP) return bindings::ActUp;
    if (sym == SDLK_DOWN) return bindings::ActDown;
    if (sym == SDLK_LEFT) return bindings::ActLeft;
    if (sym == SDLK_RIGHT) return bindings::ActRight;
    if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) return bindings::ActTap;
    if (sym == SDLK_p) return bindings::ActPause;
    if (sym == SDLK_AC_BACK) return bindings::ActPause;
    for (int i = 0; i < bindings::ActCount; i++)
        if (bindings::key(Action(i)) == sym) return Action(i);
    return bindings::ActCount;
}

void dirOf(Action action, bool shift, int* dx, int* dy, bool* two) {
    *dx = *dy = 0;
    *two = shift;
    switch (action) {
    case bindings::ActUp: *dy = -1; break;
    case bindings::ActDown: *dy = 1; break;
    case bindings::ActLeft: *dx = -1; break;
    case bindings::ActRight: *dx = 1; break;
    case bindings::ActTwoLeft: *dx = -1; *two = true; break;
    case bindings::ActTwoRight: *dx = 1; *two = true; break;
    default: break;
    }
}

int focusDir(int dx, int dy) {
    if (dx < 0) return MenuFocusLeft;
    if (dx > 0) return MenuFocusRight;
    if (dy < 0) return MenuFocusUp;
    if (dy > 0) return MenuFocusDown;
    return 0;
}

void hold(int dx, int dy, bool two) {
    s_heldDx = dx;
    s_heldDy = dy;
    s_heldTwo = two;
    s_heldNext = SDL_GetTicks() + kRepeatMs;
}

bool key(const SDL_KeyboardEvent& k, bool down) {
    Action action = actionOf(k.keysym.sym);
    if (action == bindings::ActCount) return false;
    if (inGameplay()) s_precise.store(true);
    bool menu = !inGameplay() && menuFocusActive();
    if (action == bindings::ActTiltLeft || action == bindings::ActTiltRight) {
        (action == bindings::ActTiltLeft ? s_keyTiltLeft : s_keyTiltRight) = down;
        keyTilt();
        if (down) gesture(action == bindings::ActTiltLeft ? "наклон влево" : "наклон вправо");
        return true;
    }
    if ((action == bindings::ActLeft || action == bindings::ActRight) && onBalance(0)) {
        (action == bindings::ActLeft ? s_arrowTiltLeft : s_arrowTiltRight) = down;
        keyTilt();
        if (down) gesture(action == bindings::ActLeft ? "баланс влево" : "баланс вправо");
        return true;
    }
    if (action == bindings::ActTap) {
        if (k.repeat) return true;
        if (inGameplay() || popupShown()) {
            if (down) gesture("тап");
            down ? tapDown(0) : tapUp(0);
        } else if (menu) {
            if (down) menuFocusPost(MenuFocusActivate);
        } else if (down) {
            int x, y;
            cursorPoint(&x, &y);
            touchBegan(kSynthCursor - 9, x, y);
        } else {
            touchEnded(kSynthCursor - 9);
        }
        return true;
    }
    if (action == bindings::ActPause) {
        if (!down || k.repeat) return true;
        if (inGameplay()) {
            gesture("пауза");
            pause(0);
        } else if (menu)
            menuFocusPost(MenuFocusBack);
        return true;
    }
    int dx, dy;
    bool two;
    dirOf(action, (k.keysym.mod & KMOD_SHIFT) != 0, &dx, &dy, &two);
    if (!dx && !dy) return false;
    if (!down) {
        if (s_heldDx == dx && s_heldDy == dy) s_heldDx = s_heldDy = 0;
        return true;
    }
    if (menu) {
        menuFocusPost(focusDir(dx, dy));
        return true;
    }
    if (k.repeat) return true;
    gesture(two ? "свайп двумя пальцами" : "свайп");
    swipe(0, dx, dy, two);
    if (inGameplay()) hold(dx, dy, two);
    return true;
}

struct Pad {
    SDL_GameController* controller;
    SDL_JoystickID id;
    int player;
    bool flicked;
    float cursorX, cursorY;
    float scrollY;
};
std::vector<Pad> s_pads;
Uint32 s_lastUpdate;

Pad* padOf(SDL_JoystickID id) {
    for (Pad& p : s_pads)
        if (p.id == id) return &p;
    return nullptr;
}

void openPad(int device) {
    if (!SDL_IsGameController(device) || padOf(SDL_JoystickGetDeviceInstanceID(device))) return;
    SDL_GameController* c = SDL_GameControllerOpen(device);
    if (!c) return;
    int player = 0;
    for (const Pad& p : s_pads)
        if (p.player == player) player++;
    s_pads.push_back({c, SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c)), player, false, 0, 0, 0});
    fprintf(stderr, "medecomp: геймпад %s — игрок %d\n", SDL_GameControllerName(c), player + 1);
}

float axis(Pad& p, SDL_GameControllerAxis a) {
    float v = SDL_GameControllerGetAxis(p.controller, a) / 32767.0f;
    return v < -1 ? -1 : v > 1 ? 1 : v;
}

Action actionOfButton(int button) {
    for (int i = 0; i < bindings::ActCount; i++)
        if (bindings::button(Action(i)) == button) return Action(i);
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return bindings::ActUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return bindings::ActDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return bindings::ActLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return bindings::ActRight;
    default: return bindings::ActCount;
    }
}

void padButton(Pad& p, int button, bool down) {
    s_fromPad = true;
    s_touchPlay = false;
    hideCursor();
    bool game = inGameplay();
    bool menu = !game && menuFocusActive();
    if (game) s_precise.store(true);
    if (menu && down) {
        if (button == SDL_CONTROLLER_BUTTON_A || button == bindings::button(bindings::ActTap)) {
            menuFocusPost(MenuFocusActivate);
            return;
        }
        if (button == SDL_CONTROLLER_BUTTON_B || button == bindings::button(bindings::ActPause)) {
            menuFocusPost(MenuFocusBack);
            return;
        }
    }
    if (popupShown()) {
        down ? tapDown(p.player) : tapUp(p.player);
        return;
    }
    if (!menu && !game && button == SDL_CONTROLLER_BUTTON_A) {
        int x, y;
        cursorPoint(&x, &y);
        down ? touchBegan(kSynthCursor - p.player, x, y) : touchEnded(kSynthCursor - p.player);
        return;
    }
    Action action = actionOfButton(button);
    if (game && p.player == 0 && (action == bindings::ActLeft || action == bindings::ActRight) && onBalance(0)) {
        s_precise.store(true);
        (action == bindings::ActLeft ? s_arrowTiltLeft : s_arrowTiltRight) = down;
        keyTilt();
        return;
    }
    if (action == bindings::ActTap) {
        if (down) gesture("тап");
        down ? tapDown(p.player) : tapUp(p.player);
        return;
    }
    if (!down || action == bindings::ActCount) return;
    if (menu) {
        int dx, dy;
        bool two;
        dirOf(action, false, &dx, &dy, &two);
        if (int cmd = focusDir(dx, dy)) menuFocusPost(cmd);
        return;
    }
    if (!game) return;
    int dx, dy;
    bool two;
    dirOf(action, false, &dx, &dy, &two);
    if (action == bindings::ActPause) {
        gesture("пауза");
        pause(p.player);
        return;
    }
    if (dx || dy) {
        gesture(two ? "свайп двумя пальцами" : "свайп");
        swipe(p.player, dx, dy, two);
    }
}

void padSticks(Pad& p, float dt) {
    float zone = bindings::deadzone();
    float lx = axis(p, SDL_CONTROLLER_AXIS_LEFTX), ly = axis(p, SDL_CONTROLLER_AXIS_LEFTY);
    float tilt = axis(p, SDL_CONTROLLER_AXIS_RIGHTX);
    if (fabsf(tilt) < zone) tilt = 0;
    else tilt = (tilt > 0 ? tilt - zone : tilt + zone) / (1.0f - zone);
    if (p.player == 0) {
        if (tilt != 0) s_precise.store(true);
        setTilt(tilt != 0 ? tilt * kStickTilt : s_keyTilt);
    }
    float mag = sqrtf(lx * lx + ly * ly);
    float flick = zone + 0.35f;
    if (flick < 0.5f) flick = 0.5f;
    float back = zone + 0.1f;
    bool game = inGameplay();
    if (game || menuFocusActive()) {
        if (!p.flicked && mag > flick) {
            p.flicked = true;
            s_fromPad = true;
            s_touchPlay = false;
            hideCursor();
            int dx = 0, dy = 0;
            if (fabsf(lx) > fabsf(ly)) dx = lx < 0 ? -1 : 1;
            else dy = ly < 0 ? -1 : 1;
            if (game) {
                if (!(onBalance(p.player) && dx != 0)) {
                    gesture("свайп");
                    s_precise.store(true);
                    swipe(p.player, dx, dy, false);
                }
            } else
                menuFocusPost(focusDir(dx, dy));
        } else if (p.flicked && mag < back) {
            p.flicked = false;
        }
        float sy = axis(p, SDL_CONTROLLER_AXIS_RIGHTY);
        if (!game && fabsf(sy) > zone) {
            float v = (fabsf(sy) - zone) / (1.0f - zone);
            p.scrollY += (sy < 0 ? -1 : 1) * kStickScroll * v * v * dt;
            int px = int(p.scrollY);
            p.scrollY -= px;
            if (px) menuFocusScroll(px, 0);
        }
        if (game || menuFocusActive()) return;
    }
    p.flicked = mag > back;
    float rx = lx, ry = ly;
    float rmag = mag;
    float sx = axis(p, SDL_CONTROLLER_AXIS_RIGHTX), sy = axis(p, SDL_CONTROLLER_AXIS_RIGHTY);
    if (sqrtf(sx * sx + sy * sy) > rmag) {
        rx = sx;
        ry = sy;
        rmag = sqrtf(sx * sx + sy * sy);
    }
    if (rmag < zone) return;
    s_fromPad = true;
    s_touchPlay = false;
    hideCursor();
    float speed = 1100.0f * (rmag - zone) / (1.0f - zone) * (rmag - zone) / (1.0f - zone) / rmag;
    p.cursorX += rx * speed * dt;
    p.cursorY += ry * speed * dt;
    int mx = int(p.cursorX), my = int(p.cursorY);
    if (!mx && !my) return;
    p.cursorX -= mx;
    p.cursorY -= my;
    int x, y;
    cursorPoint(&x, &y);
    x = SDL_clamp(x + mx, 0, kScreenWidth - 1);
    y = SDL_clamp(y + my, 0, kScreenHeight - 1);
    s_warping = true;
    warpPointer(x, y);
    touchMoved(kSynthCursor - p.player, x, y);
}

struct Step {
    Uint32 at;
    std::string action;
};
std::vector<Step> s_script;
int s_shots;

void loadScript() {
    const char* env = getenv("MEDECOMP_SCRIPT");
    if (!env) return;
    std::string all = env;
    for (size_t pos = 0; pos < all.size();) {
        size_t end = all.find(',', pos);
        std::string item = all.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        size_t colon = item.find(':');
        if (colon != std::string::npos)
            s_script.push_back({Uint32(atof(item.c_str()) * 1000), item.substr(colon + 1)});
        if (end == std::string::npos) break;
        pos = end + 1;
    }
}

std::atomic<bool> s_scriptFinish(false);
std::atomic<int> s_scriptAchievement(-1);

SDL_Joystick* s_scriptPad;
struct PadRelease {
    Uint32 at;
    int button;
};
std::vector<PadRelease> s_padReleases;

SDL_Joystick* scriptPad() {
    if (s_scriptPad) return s_scriptPad;
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                           SDL_CONTROLLER_BUTTON_MAX, 0);
    if (device >= 0) s_scriptPad = SDL_JoystickOpen(device);
    if (!s_scriptPad) fprintf(stderr, "medecomp: сценарий: геймпада нет: %s\n", SDL_GetError());
    return s_scriptPad;
}

void scriptPadStep(const std::string& what) {
    SDL_Joystick* pad = scriptPad();
    if (!pad) return;
    size_t eq = what.find('=');
    if (eq != std::string::npos) {
        SDL_GameControllerAxis a = SDL_GameControllerGetAxisFromString(what.substr(0, eq).c_str());
        float v = SDL_clamp(float(atof(what.c_str() + eq + 1)), -1.0f, 1.0f);
        if (a != SDL_CONTROLLER_AXIS_INVALID) SDL_JoystickSetVirtualAxis(pad, a, Sint16(v * 32767));
        return;
    }
    SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(what.c_str());
    if (b == SDL_CONTROLLER_BUTTON_INVALID) return;
    SDL_JoystickSetVirtualButton(pad, b, 1);
    s_padReleases.push_back({SDL_GetTicks() + 100, b});
}

void runStep(const std::string& action) {
    fprintf(stderr, "medecomp: сценарий: %s\n", action.c_str());
    if (action == "shot") {
        const char* dir = getenv("MEDECOMP_SHOT_DIR");
        requestScreenshot(std::string(dir ? dir : ".") + "/shot-" + std::to_string(++s_shots) + ".png");
        return;
    }
    if (action.compare(0, 5, "text:") == 0) {
        textInput(action.c_str() + 5);
        return;
    }
    if (action == "finish") {
        s_scriptFinish = true;
        return;
    }
    if (action.compare(0, 12, "achievement:") == 0) {
        s_scriptAchievement = atoi(action.c_str() + 12);
        return;
    }
    if (action.compare(0, 4, "pad:") == 0) {
        scriptPadStep(action.substr(4));
        return;
    }
    if (action.compare(0, 6, "wheel:") == 0) {
        SDL_Event e;
        SDL_memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEWHEEL;
        e.wheel.y = -atoi(action.c_str() + 6);
        SDL_PushEvent(&e);
        return;
    }
    if (action == "blur" || action == "focus") {
        SDL_Event e;
        SDL_memset(&e, 0, sizeof e);
        e.type = SDL_WINDOWEVENT;
        e.window.event = action == "blur" ? SDL_WINDOWEVENT_FOCUS_LOST : SDL_WINDOWEVENT_FOCUS_GAINED;
        SDL_PushEvent(&e);
        return;
    }
    if (action.compare(0, 4, "tap@") == 0) {
        int x = atoi(action.c_str() + 4);
        size_t slash = action.find('/');
        int y = slash == std::string::npos ? 0 : atoi(action.c_str() + slash + 1);
        touchBegan(kSynthCursor - 8, x, y);
        s_releases.push_back({SDL_GetTicks() + 100, kSynthCursor - 8});
        return;
    }
    size_t at = action.find('@');
    bool mouse = action.compare(0, 5, "mouse") == 0;
    if (at != std::string::npos && (mouse || action.compare(0, 6, "finger") == 0)) {
        std::string verb = action.substr(mouse ? 5 : 6, at - (mouse ? 5 : 6));
        long long id = mouse ? kMouse : kScriptFinger;
        int x = atoi(action.c_str() + at + 1);
        size_t slash = action.find('/');
        int y = slash == std::string::npos ? 0 : atoi(action.c_str() + slash + 1);
        if (verb == "down" && !mouse && pauseButton(float(x), float(y))) return;
        if (verb == "down") s_touchPlay = !mouse;
        if (verb == "down") touchBegan(id, SDL_clamp(x, 0, kScreenWidth - 1), SDL_clamp(y, 0, kScreenHeight - 1));
        if (verb == "move") touchMoved(id, x, y);
        if (verb == "up") touchEnded(id, x, y);
        return;
    }
    SDL_Keycode code = SDL_GetKeyFromName(action.c_str());
    if (code == SDLK_UNKNOWN) {
        fprintf(stderr, "medecomp: сценарий: нет клавиши %s\n", action.c_str());
        return;
    }
    SDL_Event e;
    SDL_memset(&e, 0, sizeof e);
    e.type = SDL_KEYDOWN;
    e.key.state = SDL_PRESSED;
    e.key.keysym.sym = code;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(code);
    SDL_PushEvent(&e);
    e.type = SDL_KEYUP;
    e.key.state = SDL_RELEASED;
    SDL_PushEvent(&e);
}

}

int pointerCall(int kind, int x, int y, int px, int py, int tap, bool mouse) {
    midp::Runtime* rt = runtime();
    const midp::Runtime::PointerState* first = rt->getPointerStatus(0);
    const midp::Runtime::PointerState* second = rt->getPointerStatus(1);
    int pointer;
    if (kind == kPointerPressed) {
        pointer = !first->m_down ? 0 : !second->m_down ? 1 : 2;
    } else if (kind == kPointerDragged) {
        pointer = first->m_down && pointerAt(first, px, py) ? 0 : second->m_down && pointerAt(second, px, py) ? 1 : 2;
    } else {
        pointer = first->m_down && (pointerAt(first, px, py) || pointerAt(first, x, y))     ? 0
                  : second->m_down && (pointerAt(second, px, py) || pointerAt(second, x, y)) ? 1
                                                                                             : 2;
    }
    s_mouseCall = mouse;
    pointerApply(kind, x, y, pointer, tap);
    s_mouseCall = false;
    return pointer;
}

void pointerApply(int kind, int x, int y, int pointer, int tap) {
    midp::Runtime* rt = runtime();
    if (kind == kPointerPressed)
        rt->pointerPressed(x, y, pointer, tap);
    else if (kind == kPointerDragged)
        rt->pointerDragged(x, y, pointer);
    else
        rt->pointerReleased(x, y, pointer);
}

bool takeScriptFinish() { return s_scriptFinish.exchange(false); }

int takeScriptAchievement() { return s_scriptAchievement.exchange(-1); }

bool inputEvent(const SDL_Event& e) {
    int x, y;
    if (capture::replaying() && e.type != SDL_CONTROLLERDEVICEADDED && e.type != SDL_CONTROLLERDEVICEREMOVED) return true;
    switch (e.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.which == SDL_TOUCH_MOUSEID || emulatedMouse() || e.button.button != SDL_BUTTON_LEFT) return true;
        showCursor();
        menuFocusPointer();
        s_fromPad = false;
        s_touchPlay = false;
        s_precise.store(false);
        s_mouseAt = SDL_GetTicks();
        toScreen(e.button.x, e.button.y, &x, &y);
        touchBegan(kMouse, x, y);
        return true;
    case SDL_MOUSEMOTION:
        if (e.motion.which == SDL_TOUCH_MOUSEID || emulatedMouse()) return true;
        if (s_warping) {
            s_warping = false;
        } else {
            showCursor();
            menuFocusPointer();
            s_fromPad = false;
            s_touchPlay = false;
            s_mouseAt = SDL_GetTicks();
        }
        if (!(e.motion.state & SDL_BUTTON_LMASK)) return true;
        toScreen(e.motion.x, e.motion.y, &x, &y);
        touchMoved(kMouse, x, y);
        return true;
    case SDL_MOUSEBUTTONUP:
        if (e.button.which == SDL_TOUCH_MOUSEID || emulatedMouse() || e.button.button != SDL_BUTTON_LEFT) return true;
        toScreen(e.button.x, e.button.y, &x, &y);
        touchEnded(kMouse, x, y);
        return true;
    case SDL_MOUSEWHEEL:
        if (!inGameplay()) {
            float y = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? e.wheel.preciseY : -e.wheel.preciseY;
            s_wheel += y;
            int steps = int(s_wheel);
            s_wheel -= steps;
            menuFocusScroll(int(y * kWheelPx), steps);
        }
        return true;
    case SDL_FINGERDOWN: {
        s_fingerAt = SDL_GetTicks();
        float fx, fy;
        int w, h;
        windowSize(&w, &h);
        toFrame(e.tfinger.x * w, e.tfinger.y * h, &fx, &fy);
        if (pauseButton(fx, fy)) return true;
        s_touchPlay = true;
        s_precise.store(false);
        fingerPoint(e.tfinger, &x, &y);
        touchBegan(e.tfinger.fingerId, x, y);
        return true;
    }
    case SDL_FINGERMOTION:
        s_fingerAt = SDL_GetTicks();
        fingerPoint(e.tfinger, &x, &y);
        touchMoved(e.tfinger.fingerId, x, y);
        return true;
    case SDL_FINGERUP:
        s_fingerAt = SDL_GetTicks();
        fingerPoint(e.tfinger, &x, &y);
        touchEnded(e.tfinger.fingerId, x, y);
        return true;
    case SDL_TEXTINPUT:
        textInput(e.text.text);
        return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym != SDLK_AC_BACK) s_touchPlay = false;
        if (textInputActive() || textLineActive()) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) textKey(kKeyBackspace);
            if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER))
                textKey(kKeyReturn);
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) textKey(kKeyEscape);
            return true;
        }
        return key(e.key, e.type == SDL_KEYDOWN);
    case SDL_CONTROLLERDEVICEADDED:
        openPad(e.cdevice.which);
        return true;
    case SDL_CONTROLLERDEVICEREMOVED:
        for (size_t i = 0; i < s_pads.size(); i++)
            if (s_pads[i].id == e.cdevice.which) {
                SDL_GameControllerClose(s_pads[i].controller);
                s_pads.erase(s_pads.begin() + long(i));
                break;
            }
        return true;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        if (Pad* p = padOf(e.cbutton.which)) padButton(*p, e.cbutton.button, e.type == SDL_CONTROLLERBUTTONDOWN);
        return true;
    case SDL_CONTROLLERAXISMOTION:
        return true;
    }
    return false;
}

#ifdef __ANDROID__
SDL_Sensor* s_accel;
bool s_accelTried;
int s_rotation;
Uint32 s_rotationAt;

void accelTilt(Uint32 now) {
    if (!s_accel && !s_accelTried) {
        s_accelTried = true;
        if (SDL_InitSubSystem(SDL_INIT_SENSOR) == 0)
            for (int i = 0; i < SDL_NumSensors() && !s_accel; i++)
                if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_ACCEL) s_accel = SDL_SensorOpen(i);
        fprintf(stderr, "medecomp: акселерометр: %s\n", s_accel ? "есть" : "нет — наклон клавишами и геймпадом");
    }
    if (!s_accel || !s_pads.empty() || s_keyTiltLeft || s_keyTiltRight) return;
    float a[3];
    if (SDL_SensorGetData(s_accel, a, 3) != 0) return;
    if (!s_rotationAt || now - s_rotationAt > 500) {
        s_rotation = (androidDisplayRotation() + turn::quarter(SDL_GetKeyboardFocus())) & 3;
        s_rotationAt = now | 1;
    }
    float sx = a[0], sy = a[1];
    switch (s_rotation) {
    case 1: sx = -a[1]; sy = a[0]; break;
    case 2: sx = -a[0]; sy = -a[1]; break;
    case 3: sx = a[1]; sy = -a[0]; break;
    }
    setTilt(atan2f(-sx, sqrtf(sy * sy + a[2] * a[2])) / float(M_PI / 2));
}
#endif

void inputUpdate() {
    Uint32 now = SDL_GetTicks();
#ifdef __ANDROID__
    accelTilt(now);
#endif
    static bool loaded = (loadScript(), true);
    (void)loaded;
    static bool scanned = false;
    if (!scanned) {
        scanned = true;
        for (int i = 0; i < SDL_NumJoysticks(); i++) openPad(i);
    }
    while (!s_script.empty() && now >= s_script.front().at) {
        std::string action = s_script.front().action;
        s_script.erase(s_script.begin());
        runStep(action);
    }
    if (capture::replaying()) return;
    float dt = s_lastUpdate ? (now - s_lastUpdate) / 1000.0f : 0;
    s_lastUpdate = now;
    if (dt > 0.1f) dt = 0.1f;
    for (size_t i = 0; i < s_releases.size();) {
        if (int(now - s_releases[i].at) >= 0) {
            touchEnded(s_releases[i].id);
            s_releases.erase(s_releases.begin() + long(i));
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < s_padReleases.size();) {
        if (int(now - s_padReleases[i].at) >= 0) {
            SDL_JoystickSetVirtualButton(s_scriptPad, s_padReleases[i].button, 0);
            s_padReleases.erase(s_padReleases.begin() + long(i));
        } else {
            i++;
        }
    }
    for (Pad& p : s_pads) padSticks(p, dt);
    if (!onBalance(0) && (s_arrowTiltLeft || s_arrowTiltRight)) {
        s_arrowTiltLeft = s_arrowTiltRight = false;
        keyTilt();
    }
    if ((s_heldDx || s_heldDy) && inGameplay() && int(now - s_heldNext) >= 0) {
        swipe(0, s_heldDx, s_heldDy, s_heldTwo);
        s_heldNext = now + kRepeatMs;
    }
    if (!inGameplay()) s_heldDx = s_heldDy = 0;
    if (inGameplay() && (s_fromPad || int(now - s_mouseAt) > 800)) hideCursor();
}

int inputPollMs() {
    return s_pads.empty() && s_releases.empty() && !s_heldDx && !s_heldDy ? 33 : 16;
}

void inputReset() {
    s_touches.clear();
    s_releases.clear();
}

bool preciseInput() { return s_precise.load(); }

}
}

extern "C" int medecomp_pointer_mouse(void) { return port::platform::s_mouseCall; }

extern "C" void medecomp_pause_button(midp::Graphics* g, GameScreen* screen) {
    using namespace port::platform;
    if (!pauseShown(screen)) return;
    int x, y;
    pauseCorner(screen, &x, &y);
    const int r = kPauseSize / 2;
    auto circle = [&](int dx, int dy) {
        int cx, cy;
        screen->calcGlobalCoordinates(x + r + dx, y + r + dy, cx, cy);
        g->fillArc(cx, cy, kPauseSize, kPauseSize, 0, 360);
    };
    auto bar = [&](int bx) {
        int x0, y0, x1, y1;
        screen->calcGlobalCoordinates(x + r + bx, y + r - 14, x0, y0);
        screen->calcGlobalCoordinates(x + r + bx + 9, y + r + 14, x1, y1);
        g->fillRect(std::min(x0, x1), std::min(y0, y1), abs(x1 - x0), abs(y1 - y0));
    };
    const int clipX = g->getClipX(), clipY = g->getClipY(), clipW = g->getClipWidth(), clipH = g->getClipHeight();
    g->setClip(x, y, kPauseSize + 8, kPauseSize + 8);
    g->setColor(0, 0, 0, 80);
    circle(3, 5);
    g->setColor(236, 0, 0, 230);
    circle(0, 0);
    g->setColor(255, 255, 255, 255);
    bar(-12);
    bar(3);
    g->setClip(clipX, clipY, clipW, clipH);
}

extern "C" void medecomp_balance_meter(midp::Graphics* g, GameScreen* screen) {
    using namespace port::platform;
    if (!screen || screen->m_type != GameScreen::TYPE_SINGLE || !onBalance(0) || screen != screenOf(0)) return;
    if (!screen->m_player) return;
    float lean = screen->m_player->getAccelerometerTilt(true);
    if (lean < -1) lean = -1;
    if (lean > 1) lean = 1;

    const int bw = 220, bh = 10;
    int x0 = screen->m_width / 2 - bw / 2;
    int y0 = screen->m_height - 48;
    auto bounds = [&](int x, int y, int w, int h, int* ox, int* oy, int* ow, int* oh) {
        int ax, ay, bx, by;
        screen->calcGlobalCoordinates(x, y, ax, ay);
        screen->calcGlobalCoordinates(x + w, y + h, bx, by);
        *ox = std::min(ax, bx);
        *oy = std::min(ay, by);
        *ow = std::abs(bx - ax);
        *oh = std::abs(by - ay);
        if (*ow < 1) *ow = 1;
        if (*oh < 1) *oh = 1;
    };
    int rx, ry, rw, rh;
    bounds(x0, y0, bw, bh, &rx, &ry, &rw, &rh);
    const int clipX = g->getClipX(), clipY = g->getClipY(), clipW = g->getClipWidth(), clipH = g->getClipHeight();
    AppEngine* app = AppEngine::getCanvas();
    if (app) g->setClip(0, 0, app->getWidth(), app->getHeight());
    g->setColor(0, 0, 0, 160);
    g->fillRect(rx, ry, rw, rh);
    int danger = bw * 8 / 100;
    int dx, dy, dw, dh;
    bounds(x0, y0, danger, bh, &dx, &dy, &dw, &dh);
    g->setColor(236, 0, 0, 220);
    g->fillRect(dx, dy, dw, dh);
    bounds(x0 + bw - danger, y0, danger, bh, &dx, &dy, &dw, &dh);
    g->fillRect(dx, dy, dw, dh);
    int mark = x0 + bw / 2 + int(lean * (bw / 2 - 4));
    bounds(mark - 2, y0 - 3, 4, bh + 6, &dx, &dy, &dw, &dh);
    g->setColor(255, 255, 255, 255);
    g->fillRect(dx, dy, dw, dh);

    using bindings::Action;
    std::string side = std::string(bindings::keyLabel(Action(bindings::ActLeft))) + "/" +
                       bindings::keyLabel(Action(bindings::ActRight));
    std::string tilt = std::string(bindings::keyLabel(Action(bindings::ActTiltLeft))) + "/" +
                       bindings::keyLabel(Action(bindings::ActTiltRight));
    std::string text = side == tilt ? side : side + "  " + tilt;
    static std::string cached;
    static midp::ReferenceCountedPointer<midp::String> label;
    if (text != cached) {
        cached = text;
        label = new midp::String(cached.c_str());
    }
    int tx, ty;
    screen->calcGlobalCoordinates(x0, y0 - 6, tx, ty);
    if (app && app->getTextManager()) app->getTextManager()->drawString(g, label, 14, tx, ty, 0x41);
    g->setClip(clipX, clipY, clipW, clipH);
}

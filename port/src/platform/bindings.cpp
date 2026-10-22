#include "bindings.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "settings.h"

namespace port {
namespace platform {
namespace bindings {

namespace {

struct State {
    bool loaded = false;
    SDL_Keycode keys[ActCount];
    int buttons[ActCount];
    float zone = 0.20f;
    int capture = -1;
    bool captureIsPad = false;
};

State g;

const char* kKeySetting[ActCount] = {
    "key.up",     "key.down",      "key.left",      "key.right", "key.twoLeft",
    "key.twoRight", "key.tap",     "key.tiltLeft",  "key.tiltRight", "key.pause",
};
const char* kPadSetting[ActCount] = {
    "pad.up",     "pad.down",      "pad.left",      "pad.right", "pad.twoLeft",
    "pad.twoRight", "pad.tap",     "pad.tiltLeft",  "pad.tiltRight", "pad.pause",
};

const SDL_Keycode kDefaultKey[ActCount] = {
    SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_z, SDLK_c, SDLK_SPACE, SDLK_q, SDLK_e, SDLK_ESCAPE,
};
const int kDefaultButton[ActCount] = {
    SDL_CONTROLLER_BUTTON_A,          SDL_CONTROLLER_BUTTON_B,          -1, -1,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_X,          -1, -1, SDL_CONTROLLER_BUTTON_START,
};

void saveOne(const char* setting, const char* value) { settings::set(setting, value ? value : ""); }

void persist() {
    for (int i = 0; i < ActCount; i++) {
        saveOne(kKeySetting[i], SDL_GetKeyName(g.keys[i]));
        saveOne(kPadSetting[i], g.buttons[i] < 0 ? "" : SDL_GameControllerGetStringForButton(
                                                           SDL_GameControllerButton(g.buttons[i])));
    }
    char buf[16];
    snprintf(buf, sizeof buf, "%.2f", g.zone);
    saveOne("deadzone", buf);
    if (!settings::save()) fprintf(stderr, "medecomp: не записать %s\n", settings::path().c_str());
}

void ensure() {
    if (g.loaded) return;
    g.loaded = true;
    for (int i = 0; i < ActCount; i++) {
        g.keys[i] = kDefaultKey[i];
        g.buttons[i] = kDefaultButton[i];
        std::string name = settings::get(kKeySetting[i]);
        if (!name.empty()) {
            SDL_Keycode code = SDL_GetKeyFromName(name.c_str());
            if (code != SDLK_UNKNOWN) g.keys[i] = code;
        }
        name = settings::get(kPadSetting[i]);
        if (!name.empty()) {
            int b = SDL_GameControllerGetButtonFromString(name.c_str());
            if (b >= 0) g.buttons[i] = b;
        }
    }
    std::string zone = settings::get("deadzone");
    if (!zone.empty()) {
        float z = float(atof(zone.c_str()));
        if (z >= 0.05f && z <= 0.9f) g.zone = z;
    }
}

}

SDL_Keycode key(Action action) {
    ensure();
    return g.keys[action];
}

int button(Action action) {
    ensure();
    return g.buttons[action];
}

float deadzone() {
    ensure();
    return g.zone;
}

void setKey(Action action, SDL_Keycode code) {
    ensure();
    g.keys[action] = code;
    persist();
}

void setButton(Action action, int sdlButton) {
    ensure();
    g.buttons[action] = sdlButton;
    persist();
}

void setDeadzone(float zone) {
    ensure();
    if (zone < 0.05f) zone = 0.05f;
    if (zone > 0.9f) zone = 0.9f;
    g.zone = zone;
    persist();
}

void reload() { g.loaded = false; }

void reset() {
    ensure();
    for (int i = 0; i < ActCount; i++) {
        g.keys[i] = kDefaultKey[i];
        g.buttons[i] = kDefaultButton[i];
    }
    g.zone = 0.20f;
    g.capture = -1;
    persist();
}

const char* keyLabel(Action action) {
    ensure();
    return SDL_GetKeyName(g.keys[action]);
}

const char* buttonLabel(Action action) {
    ensure();
    switch (g.buttons[action]) {
    case SDL_CONTROLLER_BUTTON_A: return "A";
    case SDL_CONTROLLER_BUTTON_B: return "B";
    case SDL_CONTROLLER_BUTTON_X: return "X";
    case SDL_CONTROLLER_BUTTON_Y: return "Y";
    case SDL_CONTROLLER_BUTTON_BACK: return "Back";
    case SDL_CONTROLLER_BUTTON_GUIDE: return "Guide";
    case SDL_CONTROLLER_BUTTON_START: return "Start";
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return "D-up";
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "D-down";
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "D-left";
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "D-right";
    default: return "—";
    }
}

void capture(int action, bool pad) {
    ensure();
    g.capture = action;
    g.captureIsPad = pad;
}

bool capturing() { return g.capture >= 0; }
bool capturePad() { return g.captureIsPad; }
int captureAction() { return g.capture; }

bool takeEvent(const SDL_Event& e) {
    if (g.capture < 0) return false;
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            g.capture = -1;
            return true;
        }
        if (!g.captureIsPad && e.key.keysym.sym != SDLK_UNKNOWN) {
            setKey(Action(g.capture), e.key.keysym.sym);
            g.capture = -1;
            return true;
        }
    }
    if (g.captureIsPad && e.type == SDL_CONTROLLERBUTTONDOWN) {
        setButton(Action(g.capture), e.cbutton.button);
        g.capture = -1;
        return true;
    }
    return g.captureIsPad ? false : e.type == SDL_KEYUP;
}

}
}
}

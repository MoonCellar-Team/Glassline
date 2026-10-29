#include "port/touch.h"

#include <algorithm>
#include <cstdlib>

#include "settings.h"

namespace port {
namespace platform {
namespace touch {
namespace {

int readInt(const char* key, int fallback, int low, int high) {
    const std::string text = settings::get(key);
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (text.empty() || end == text.c_str() || *end || value < low || value > high) return fallback;
    return int(value);
}

Config& stored() {
    static Config value = [] {
        Config out;
        out.distance = readInt("touch_distance", out.distance, 15, 80);
        out.verticalSector = readInt("touch_vertical_sector", out.verticalSector, 40, 90);
        out.lockDirection = settings::get("touch_lock_direction", "on") != "off";
        return out;
    }();
    return value;
}

}

Config config() { return stored(); }

void setConfig(Config value) {
    value.distance = std::max(15, std::min(80, value.distance));
    value.verticalSector = std::max(40, std::min(90, value.verticalSector));
    stored() = value;
    settings::set("touch_distance", std::to_string(value.distance));
    settings::set("touch_vertical_sector", std::to_string(value.verticalSector));
    settings::set("touch_lock_direction", value.lockDirection ? "on" : "off");
    settings::save();
}

void reset() {
    stored() = Config();
    settings::erase("touch_distance");
    settings::erase("touch_vertical_sector");
    settings::erase("touch_lock_direction");
    settings::save();
}

}
}
}

extern "C" int medecomp_pointer_mouse(void);

extern "C" int medecomp_touch_swipe(int dx, int dy, int previous) {
    port::platform::touch::Config value;
    if (medecomp_pointer_mouse())
        value.lockDirection = false;
    else
        value = port::platform::touch::config();
    return port::platform::touch::direction(dx, dy, previous, value);
}

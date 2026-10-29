#pragma once

#include <cmath>

#include "game/GameScreenPointer.h"

namespace port {
namespace platform {
namespace touch {

struct Config {
    int distance = 30;
    int verticalSector = 90;
    bool lockDirection = true;
};

Config config();
void setConfig(Config value);
void reset();

inline int direction(int dx, int dy, int previous, const Config& value) {
    if (value.lockDirection && previous != 0) return 0;
    const double ax = std::abs(double(dx)), ay = std::abs(double(dy));
    const bool vertical = value.verticalSector == 90
                              ? ax <= ay
                              : ax <= ay * std::tan(value.verticalSector * 3.14159265358979323846 / 360.0);
    if ((vertical ? ay : ax) < value.distance) return 0;
    if (vertical) return dy < 0 ? GameScreenPointer::SWIPE_UP : GameScreenPointer::SWIPE_DOWN;
    return dx < 0 ? GameScreenPointer::SWIPE_LEFT : GameScreenPointer::SWIPE_RIGHT;
}

}
}
}

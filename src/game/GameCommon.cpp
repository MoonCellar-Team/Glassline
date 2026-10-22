#include "game/GameCommon.h"

bool GameCommon::isZero(float a) {
    return !(a < -0.01f || a > 0.01f);
}

bool GameCommon::compareFloats(float a, float b) {
    return !(a < b - 0.01f || a > b + 0.01f);
}

bool GameCommon::boundsIntersect(float min1, float max1, float min2, float max2) {
    if (min2 - 0.01f > max1) {
        return false;
    }
    return !(max2 + 0.01f < min1);
}

bool GameCommon::inBounds(float min, float value, float max) {
    if (min - 0.01f > value) {
        return false;
    }
    return !(max + 0.01f < value);
}

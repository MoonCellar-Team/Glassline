#pragma once

#include "game/MathVector.h"

class GameCommon {
public:
    static bool isZero(float a);
    static bool compareFloats(float a, float b);
    static bool boundsIntersect(float min1, float max1, float min2, float max2);
    static bool inBounds(float min, float value, float max);
};

static const MathVector CHOPPER_GUN_OFFSET(0.0f, 2.9f, 0.8f);
static const MathVector PLAYER_AIM_OFFSET(0.0f, 1.3f, 0.0f);
static const MathVector CAMERA_TARGET_OFFSET(0.0f, 1.0f, 0.0f);
static const MathVector CAMERA_OFFSET(0.0f, 1.0f, 10.0f);
static const MathVector CAMERA_TARGET_OFFSET_SPLIT(0.0f, 1.0f, 0.0f);
static const MathVector CAMERA_OFFSET_SPLIT(0.0f, 1.0f, 15.0f);

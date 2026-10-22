#pragma once

#include "game/MathVector.h"

class MathPlane {
public:
    MathPlane();

    MathVector m_center;
    MathVector m_up;
    MathVector m_right;
};

#pragma once

#include "game/MathVector.h"

namespace midp {
class DataInputStream;
}

class PathPoint : public MathVector {
public:
    explicit PathPoint(midp::DataInputStream& in);
    ~PathPoint();

    int m_action;
    int m_speed;
};

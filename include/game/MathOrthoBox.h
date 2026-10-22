#pragma once

#include "game/MathVector.h"

class MathLine;

namespace midp {
class DataInputStream;
}

class MathOrthoBox {
public:
    MathOrthoBox();
    MathOrthoBox(const MathOrthoBox& o);
    MathOrthoBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    ~MathOrthoBox();
    MathOrthoBox& operator=(const MathOrthoBox& o);
    MathOrthoBox& operator+=(const MathVector& offset);
    MathOrthoBox& operator-=(const MathVector& offset);
    void setCoordinates(float x1, float y1, float z1, float x2, float y2, float z2);
    void setCoordinates(const MathOrthoBox& other);
    void setF(midp::DataInputStream* in);
    bool intersects(const MathLine& line, float* tMin, float* tMax) const;

    MathVector m_min;
    MathVector m_max;
    bool m_valid;
};

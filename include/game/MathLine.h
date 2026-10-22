#pragma once

#include "game/MathVector.h"

class MathLine {
public:
    MathLine();
    MathLine(const MathVector& origin, const MathVector& direction);
    MathLine(const MathVector& origin, float dx, float dy, float dz);
    MathLine(float ox, float oy, float oz, float dx, float dy, float dz);
    MathLine(const MathLine& o);
    ~MathLine();
    void calculatePointAtT(float t, MathVector& out) const;
    float calculateTatX(float x) const;
    float calculateTatY(float y) const;
    float calculateTatZ(float z) const;
    float calculateYatT(float t) const;
    float calculateClosestTToPoint(const MathVector& point) const;
    static float calculateClosestTToPoint(const MathVector& direction, const MathVector& point);

    MathVector m_origin;
    MathVector m_direction;
};

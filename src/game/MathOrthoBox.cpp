#include "game/MathOrthoBox.h"

#include <stddef.h>

#include "game/GameCommon.h"
#include "game/MathLine.h"
#include "runtime/midp/Math.h"

MathOrthoBox::MathOrthoBox() : m_valid(false) {}

MathOrthoBox::MathOrthoBox(const MathOrthoBox& o) : m_min(o.m_min), m_max(o.m_max), m_valid(o.m_valid) {}

MathOrthoBox::~MathOrthoBox() {}

MathOrthoBox& MathOrthoBox::operator=(const MathOrthoBox& o) {
    m_min = o.m_min;
    m_max = o.m_max;
    return *this;
}

MathOrthoBox& MathOrthoBox::operator+=(const MathVector& offset) {
    m_min += offset;
    m_max += offset;
    return *this;
}

MathOrthoBox& MathOrthoBox::operator-=(const MathVector& offset) {
    m_min -= offset;
    m_max -= offset;
    return *this;
}

void MathOrthoBox::setCoordinates(float x1, float y1, float z1, float x2, float y2, float z2) {
    m_min.x = midp::Math::min(x1, x2);
    m_min.y = midp::Math::min(y1, y2);
    m_min.z = midp::Math::min(z1, z2);
    m_max.x = midp::Math::max(x1, x2);
    m_max.y = midp::Math::max(y1, y2);
    m_max.z = midp::Math::max(z1, z2);
    m_valid = true;
}

MathOrthoBox::MathOrthoBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    : m_valid(false) {
    setCoordinates(minX, minY, minZ, maxX, maxY, maxZ);
}

bool MathOrthoBox::intersects(const MathLine& line, float* tMin, float* tMax) const {
    float lo = 0.0f;
    float hi = 0.0f;
    bool any = false;
    if (!GameCommon::isZero(line.m_direction.x)) {
        float t0 = line.calculateTatX(m_min.x);
        float t1 = line.calculateTatX(m_max.x);
        lo = midp::Math::min(t0, t1);
        hi = midp::Math::max(t0, t1);
        any = true;
    }
    if (!GameCommon::isZero(line.m_direction.y)) {
        float t0 = line.calculateTatY(m_min.y);
        float t1 = line.calculateTatY(m_max.y);
        float l = midp::Math::min(t0, t1);
        float h = midp::Math::max(t0, t1);
        if (any) {
            lo = midp::Math::max(lo, l);
            hi = midp::Math::min(hi, h);
        } else {
            lo = l;
            hi = h;
        }
        any = true;
    }
    if (!GameCommon::isZero(line.m_direction.z)) {
        float t0 = line.calculateTatZ(m_min.z);
        float t1 = line.calculateTatZ(m_max.z);
        float l = midp::Math::min(t0, t1);
        float h = midp::Math::max(t0, t1);
        if (any) {
            lo = midp::Math::max(lo, l);
            hi = midp::Math::min(hi, h);
        } else {
            lo = l;
            hi = h;
        }
    }
    if (lo <= hi) {
        if (tMin != NULL) {
            *tMin = lo;
        }
        if (tMax != NULL) {
            *tMax = hi;
        }
        return true;
    }
    return false;
}

void MathOrthoBox::setF(midp::DataInputStream* in) {
    m_min.setF(in);
    m_max.setF(in);
    m_valid = true;
}

void MathOrthoBox::setCoordinates(const MathOrthoBox& other) {
    m_min = other.m_min;
    m_max = other.m_max;
    m_valid = true;
}

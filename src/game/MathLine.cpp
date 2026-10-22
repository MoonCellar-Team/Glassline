#include "game/MathLine.h"

#include "game/GameCommon.h"

MathLine::MathLine() {}

MathLine::MathLine(const MathLine& o) : m_origin(o.m_origin), m_direction(o.m_direction) {}

MathLine::MathLine(float ox, float oy, float oz, float dx, float dy, float dz)
    : m_origin(ox, oy, oz), m_direction(dx, dy, dz) {}

MathLine::MathLine(const MathVector& origin, float dx, float dy, float dz)
    : m_origin(origin), m_direction(dx, dy, dz) {}

MathLine::MathLine(const MathVector& origin, const MathVector& direction)
    : m_origin(origin), m_direction(direction) {}

MathLine::~MathLine() {}

float MathLine::calculateTatX(float x) const {
    if (GameCommon::compareFloats(m_origin.x, x)) {
        return 0.0f;
    }
    return (x - m_origin.x) / m_direction.x;
}

float MathLine::calculateTatY(float y) const {
    if (GameCommon::compareFloats(m_origin.y, y)) {
        return 0.0f;
    }
    return (y - m_origin.y) / m_direction.y;
}

float MathLine::calculateTatZ(float z) const {
    if (GameCommon::compareFloats(m_origin.z, z)) {
        return 0.0f;
    }
    return (z - m_origin.z) / m_direction.z;
}

void MathLine::calculatePointAtT(float t, MathVector& out) const {
    out.x = m_origin.x + m_direction.x * t;
    out.y = m_origin.y + m_direction.y * t;
    out.z = m_origin.z + m_direction.z * t;
}

float MathLine::calculateClosestTToPoint(const MathVector& direction, const MathVector& point) {
    float lenSq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
    if (lenSq == 0.0f) {
        return 0.0f;
    }
    return (direction.x * point.x + direction.y * point.y + direction.z * point.z) / lenSq;
}

float MathLine::calculateClosestTToPoint(const MathVector& point) const {
    MathVector rel(point.x - m_origin.x, point.y - m_origin.y, point.z - m_origin.z);
    return calculateClosestTToPoint(m_direction, rel);
}

float MathLine::calculateYatT(float t) const {
    return m_origin.y + m_direction.y * t;
}

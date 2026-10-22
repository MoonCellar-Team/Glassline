#include "game/SeperatedAxis.h"

#include "game/GameCommon.h"
#include "game/MathLine.h"
#include "game/MathOrthoBox.h"

SeperatedAxis::SeperatedAxis() : m_type(AXIS_OTHER) {}

void SeperatedAxis::ensureNoIntersection(MathVector& position, const MathOrthoBox& localBounds,
                                         const MathVector& move) {
    switch (m_type) {
    case AXIS_X:
        if (move.x < -0.01f) {
            position.x = m_ranges[2].max - localBounds.m_min.x;
        } else if (move.x > 0.01f) {
            position.x = m_ranges[2].min - localBounds.m_max.x;
        }
        break;
    case AXIS_Y:
        if (move.y < -0.01f) {
            position.y = m_ranges[2].max - localBounds.m_min.y;
        } else if (move.y > 0.01f) {
            position.y = m_ranges[2].min - localBounds.m_max.y;
        }
        break;
    case AXIS_Z:
        if (move.z < -0.01f) {
            position.z = m_ranges[2].max - localBounds.m_min.z;
        } else if (move.z > 0.01f) {
            position.z = m_ranges[2].min - localBounds.m_max.z;
        }
        break;
    default:
        break;
    }
}

float SeperatedAxis::getMoveOutMultiple(const MathVector& move, int index) {
    float along;
    if (!m_direction.isOrthogonal()) {
        along = MathLine::calculateClosestTToPoint(m_direction, move);
    } else {
        float d;
        float m;
        if (!GameCommon::isZero(m_direction.x)) {
            d = m_direction.x;
            m = move.x;
        } else if (!GameCommon::isZero(m_direction.y)) {
            d = m_direction.y;
            m = move.y;
        } else if (!GameCommon::isZero(m_direction.z)) {
            d = m_direction.z;
            m = move.z;
        } else {
            d = 0.0f;
            m = 0.0f;
        }
        along = m;
        if (d != 1.0f) {
            along /= d;
        }
    }
    if (along >= 0.0f) {
        if (along <= 0.0f) {
            return -999999.0f;
        }
        return (m_ranges[index].min - m_ranges[0].max) / along + 1.0f;
    }
    return (m_ranges[index].max - m_ranges[0].min) / along + 1.0f;
}

void SeperatedAxis::getNormal(const MathVector& move, MathVector& normal) {
    normal.set(m_direction);
    if (normal.dot(move) >= 0.0f) {
        normal *= -1.0f;
    }
}

SeperatedAxis::~SeperatedAxis() {}

void SeperatedAxis::setAxis(SeperatedAxis* other) {
    m_direction = other->m_direction;
    m_ranges[0] = other->m_ranges[0];
    m_ranges[1] = other->m_ranges[1];
    m_ranges[2] = other->m_ranges[2];
}

SeperatedAxis::AxisType SeperatedAxis::getType() {
    return m_type;
}

MathVector& SeperatedAxis::getDirection() {
    return m_direction;
}

float SeperatedAxis::getMinT(int index) {
    return m_ranges[index].min;
}

float SeperatedAxis::getMaxT(int index) {
    return m_ranges[index].max;
}

SeperatedAxis::Range* SeperatedAxis::getTValues(int index) {
    return &m_ranges[index];
}

void SeperatedAxis::setTValues(int index, float minT, float maxT) {
    m_ranges[index].min = minT;
    m_ranges[index].max = maxT;
}

bool SeperatedAxis::noIntersection() {
    return !(m_ranges[0].max > m_ranges[1].min && m_ranges[1].max > m_ranges[0].min);
}

bool SeperatedAxis::noIntersection(int a, int b) {
    return !(m_ranges[a].max > m_ranges[b].min && m_ranges[b].max > m_ranges[a].min);
}

float SeperatedAxis::getDistanceOfPoint(const MathVector& point) {
    return MathLine::calculateClosestTToPoint(m_direction, point);
}

void SeperatedAxis::setAxis(const MathVector& direction, int index, float minT, float maxT, AxisType type) {
    m_direction = direction;
    setTValues(index, minT, maxT);
    m_type = type;
}

void SeperatedAxis::setAxis(const MathVector& direction, int index, float minT, float maxT) {
    setAxis(direction, index, minT, maxT, AXIS_OTHER);
}

#include "game/CollShape.h"

#include <stddef.h>

#include "game/GameCommon.h"
#include "game/MathLine.h"
#include "game/SeperatedAxesList.h"
#include "runtime/midp/Math.h"

void CollOrthoRampPosX::setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const {
    MathVector& dir = axis->getDirection();
    float zLo = m_bounds.m_min.z;
    float zHi = m_bounds.m_max.z;
    if (dir.z < -0.01f) {
        zLo = m_bounds.m_max.z;
        zHi = m_bounds.m_min.z;
    }
    bool noY = GameCommon::isZero(dir.y);
    bool noX = GameCommon::isZero(dir.x);
    float minT;
    float maxT;
    if (noY && noX) {
        MathVector p(m_bounds.m_max.x, m_bounds.m_min.y, zLo);
        minT = axis->getDistanceOfPoint(p);
        p.z = zHi;
        maxT = axis->getDistanceOfPoint(p);
    } else if (noY) {
        if (dir.x < 0.0f) {
            MathVector p(m_bounds.m_max.x, m_bounds.m_min.y, zLo);
            minT = axis->getDistanceOfPoint(p);
            p.x = m_bounds.m_min.x;
            maxT = axis->getDistanceOfPoint(p);
        } else {
            MathVector p(m_bounds.m_min.x, m_bounds.m_min.y, zLo);
            minT = axis->getDistanceOfPoint(p);
            p.x = m_bounds.m_max.x;
            maxT = axis->getDistanceOfPoint(p);
        }
    } else if (noX) {
        if (dir.y < 0.0f) {
            MathVector p(m_bounds.m_max.x, m_bounds.m_max.y, zLo);
            minT = axis->getDistanceOfPoint(p);
            p.y = m_bounds.m_min.y;
            maxT = axis->getDistanceOfPoint(p);
        } else {
            MathVector p(m_bounds.m_max.x, m_bounds.m_min.y, zLo);
            minT = axis->getDistanceOfPoint(p);
            p.y = m_bounds.m_max.y;
            maxT = axis->getDistanceOfPoint(p);
        }
    } else if (dir.x < -0.01f && dir.y > 0.01f) {
        MathVector p(m_bounds.m_max.x, m_bounds.m_min.y, zLo);
        minT = axis->getDistanceOfPoint(p);
        p.x = m_bounds.m_min.x;
        p.z = zHi;
        float a = axis->getDistanceOfPoint(p);
        p.x = m_bounds.m_max.x;
        p.y = m_bounds.m_max.y;
        maxT = midp::Math::max(a, axis->getDistanceOfPoint(p));
    } else if (dir.x > 0.01f && dir.y < -0.01f) {
        MathVector p(m_bounds.m_max.x, m_bounds.m_min.y, zLo);
        maxT = axis->getDistanceOfPoint(p);
        p.x = m_bounds.m_min.x;
        p.z = zHi;
        float a = axis->getDistanceOfPoint(p);
        p.x = m_bounds.m_max.x;
        p.y = m_bounds.m_max.y;
        minT = midp::Math::min(a, axis->getDistanceOfPoint(p));
    } else if (dir.x > 0.01f && dir.y > 0.01f) {
        MathVector p(m_bounds.m_min.x, m_bounds.m_min.y, zLo);
        minT = axis->getDistanceOfPoint(p);
        p.set(m_bounds.m_max.x, m_bounds.m_max.y, zHi);
        maxT = axis->getDistanceOfPoint(p);
    } else if (dir.x < -0.01f && dir.y < -0.01f) {
        MathVector p(m_bounds.m_max.x, m_bounds.m_max.y, zLo);
        minT = axis->getDistanceOfPoint(p);
        p.set(m_bounds.m_min.x, m_bounds.m_min.y, zHi);
        maxT = axis->getDistanceOfPoint(p);
    } else {
        minT = 0.0f;
        maxT = 0.0f;
    }
    axis->setTValues(index, minT, maxT);
}

bool CollOrthoRampPosX::intersects(const MathLine& line, float* tMin, float* tMax) const {
    if (line.m_direction.x != 0.0f || line.m_direction.y == 0.0f || line.m_direction.z != 0.0f) {
        return false;
    }
    float x = line.m_origin.x;
    if (!GameCommon::inBounds(m_bounds.m_min.x, x, m_bounds.m_max.x)) {
        return false;
    }
    if (!GameCommon::inBounds(m_bounds.m_min.z, line.m_origin.z, m_bounds.m_max.z)) {
        return false;
    }
    float surfaceY = m_bounds.m_min.y + (m_bounds.m_max.y - m_bounds.m_min.y) * (x - m_bounds.m_min.x) /
                                            (m_bounds.m_max.x - m_bounds.m_min.x);
    float t0 = line.calculateTatY(surfaceY);
    float t1 = line.calculateTatY(midp::Math::max(m_bounds.m_min.y, surfaceY - 0.05f));
    if (tMin != NULL) {
        *tMin = midp::Math::min(t0, t1);
    }
    if (tMax != NULL) {
        *tMax = midp::Math::max(t0, t1);
    }
    return true;
}

CollOrthoRampPosX::~CollOrthoRampPosX() {}

void CollOrthoRampPosX::addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const {
    MathVector normal(m_bounds.m_min.y - m_bounds.m_max.y, m_bounds.m_max.x - m_bounds.m_min.x, 0.0f);
    normal.normalise();
    MathVector p(m_bounds.m_min);
    float t0 = MathLine::calculateClosestTToPoint(normal, p);
    p.x = m_bounds.m_max.x;
    p.y = m_bounds.m_max.y;
    float t = midp::Math::max(t0, MathLine::calculateClosestTToPoint(normal, p));
    list->addAxis(normal, index, t - 0.05f, t);
}

CollOrthoRampPosX::CollOrthoRampPosX(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    : CollShape(SHAPE_RAMP_POS_X, minX, minY, minZ, maxX, maxY, maxZ) {}

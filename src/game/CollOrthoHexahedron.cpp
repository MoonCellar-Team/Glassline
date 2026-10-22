#include "game/CollShape.h"

#include <stddef.h>

#include "game/MathLine.h"
#include "game/SeperatedAxis.h"
#include "runtime/midp/Math.h"

void CollOrthoHexahedron::addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const {}

void CollOrthoHexahedron::setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const {
    MathVector& dir = axis->getDirection();
    MathVector lo(m_bounds.m_min);
    MathVector hi(m_bounds.m_max);
    if (dir.x <= -0.01f) {
        lo.x = m_bounds.m_max.x;
        hi.x = m_bounds.m_min.x;
    }
    if (dir.y <= -0.01f) {
        lo.y = m_bounds.m_max.y;
        hi.y = m_bounds.m_min.y;
    }
    if (dir.z <= -0.01f) {
        lo.z = m_bounds.m_max.z;
        hi.z = m_bounds.m_min.z;
    }
    float minT = axis->getDistanceOfPoint(lo);
    float maxT = axis->getDistanceOfPoint(hi);
    axis->setTValues(index, minT, maxT);
}

bool CollOrthoHexahedron::intersects(const MathLine& line, float* tMin, float* tMax) const {
    MathOrthoBox box(m_bounds);
    box -= line.m_origin;
    float lo = -1e9f;
    float hi = 1e9f;
    if (line.m_direction.x != 0.0f) {
        float t0 = box.m_min.x / line.m_direction.x;
        float t1 = box.m_max.x / line.m_direction.x;
        float tNear = midp::Math::min(t0, t1);
        float tFar = midp::Math::max(t0, t1);
        if (tFar < lo || hi < tNear) {
            return false;
        }
        lo = midp::Math::max(lo, tNear);
        hi = midp::Math::min(hi, tFar);
    } else if (line.m_origin.x < m_bounds.m_min.x || m_bounds.m_max.x < line.m_origin.x) {
        return false;
    }
    if (line.m_direction.y != 0.0f) {
        float t0 = box.m_min.y / line.m_direction.y;
        float t1 = box.m_max.y / line.m_direction.y;
        float tNear = midp::Math::min(t0, t1);
        float tFar = midp::Math::max(t0, t1);
        if (tFar < lo || hi < tNear) {
            return false;
        }
        lo = midp::Math::max(lo, tNear);
        hi = midp::Math::min(hi, tFar);
    } else if (line.m_origin.y < m_bounds.m_min.y || m_bounds.m_max.y < line.m_origin.y) {
        return false;
    }
    if (line.m_direction.z != 0.0f) {
        float t0 = box.m_min.z / line.m_direction.z;
        float t1 = box.m_max.z / line.m_direction.z;
        float tNear = midp::Math::min(t0, t1);
        float tFar = midp::Math::max(t0, t1);
        if (tFar < lo || hi < tNear) {
            return false;
        }
        lo = midp::Math::max(lo, tNear);
        hi = midp::Math::min(hi, tFar);
    } else if (line.m_origin.z < m_bounds.m_min.z || m_bounds.m_max.z < line.m_origin.z) {
        return false;
    }
    if (tMin != NULL) {
        *tMin = lo;
    }
    if (tMax != NULL) {
        *tMax = hi;
    }
    return true;
}

CollOrthoHexahedron::~CollOrthoHexahedron() {}

CollOrthoHexahedron::CollOrthoHexahedron(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    : CollShape(SHAPE_ORTHO_HEXAHEDRON, minX, minY, minZ, maxX, maxY, maxZ) {}

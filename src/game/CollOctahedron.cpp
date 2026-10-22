#include "game/CollShape.h"

#include "game/GameCommon.h"
#include "game/MathLine.h"
#include "game/SeperatedAxesList.h"
#include "runtime/midp/Math.h"

bool CollOctahedron::intersects(const MathLine& line, float* tMin, float* tMax) const {
    return false;
}

void CollOctahedron::setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const {
    MathVector& dir = axis->getDirection();
    float cx = (m_bounds.m_min.x + m_bounds.m_max.x) * 0.5f;
    float cy = (m_bounds.m_min.y + m_bounds.m_max.y) * 0.5f;
    float cz = (m_bounds.m_min.z + m_bounds.m_max.z) * 0.5f;
    MathVector lo(m_bounds.m_min.x, cy, m_bounds.m_min.z);
    MathVector hi(m_bounds.m_max.x, cy, m_bounds.m_max.z);
    MathVector bottom(cx, m_bounds.m_min.y, cz);
    MathVector top(cx, m_bounds.m_max.y, cz);
    if (dir.x < 0.0f) {
        lo.x = m_bounds.m_max.x;
        hi.x = m_bounds.m_min.x;
    }
    if (dir.y < 0.0f) {
        bottom.y = m_bounds.m_max.z;
        top.y = m_bounds.m_min.z;
    }
    if (dir.z < 0.0f) {
        lo.z = m_bounds.m_max.z;
        hi.z = m_bounds.m_min.z;
    }
    float minT = axis->getDistanceOfPoint(lo);
    float maxT = axis->getDistanceOfPoint(hi);
    if (!GameCommon::isZero(dir.y)) {
        minT = midp::Math::min(minT, axis->getDistanceOfPoint(bottom));
        maxT = midp::Math::max(maxT, axis->getDistanceOfPoint(top));
    }
    axis->setTValues(index, minT, maxT);
}

CollOctahedron::~CollOctahedron() {}

void CollOctahedron::addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const {
    MathVector bottom((m_bounds.m_min.x + m_bounds.m_max.x) * 0.5f, m_bounds.m_min.y,
                      (m_bounds.m_min.z + m_bounds.m_max.z) * 0.5f);
    MathVector corner(m_bounds.m_min.x, m_bounds.m_max.y, bottom.z);
    MathVector dir((m_bounds.m_min.x - m_bounds.m_max.x) * 0.5f, (m_bounds.m_max.y - m_bounds.m_min.y) * 0.5f,
                   0.0f);
    dir.normalise();
    float t0 = MathLine::calculateClosestTToPoint(dir, bottom);
    float t1 = MathLine::calculateClosestTToPoint(dir, corner);
    list->addAxis(dir, index, t0, t1);
    dir.x = -dir.x;
    corner.x = m_bounds.m_max.x;
    t0 = MathLine::calculateClosestTToPoint(dir, bottom);
    t1 = MathLine::calculateClosestTToPoint(dir, corner);
    list->addAxis(dir, index, t0, t1);
}

CollOctahedron::CollOctahedron(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    : CollShape(SHAPE_OCTAHEDRON, minX, minY, minZ, maxX, maxY, maxZ) {}

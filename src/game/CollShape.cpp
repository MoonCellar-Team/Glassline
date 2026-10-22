#include "game/CollShape.h"

#include <stddef.h>

#include "game/SeperatedAxesList.h"
#include "runtime/midp/array.h"

SeperatedAxesList* CollShape::s_sepAxisList[3];

float CollShape::getBounce() {
    return m_bounce;
}

float CollShape::getInverseResistance() {
    return m_inverseResistance;
}

const MathOrthoBox& CollShape::getBounds() const {
    return m_bounds;
}

void CollShape::setPosition(const MathVector& position) {
    m_bounds = m_localBounds;
    m_bounds += position;
}

const MathOrthoBox& CollShape::getLocalBounds() const {
    return m_localBounds;
}

CollShape::ShapeType CollShape::getShapeType() {
    return m_shapeType;
}

bool CollShape::isRamp() {
    return m_shapeType == SHAPE_RAMP_NEG_X || m_shapeType == SHAPE_RAMP_POS_X;
}

void CollShape::setBounce(float bounce) {
    m_bounce = bounce;
}

void CollShape::setRangeOnAxis(SeperatedAxis* axis, int index) const {
    switch (axis->getType()) {
    case SeperatedAxis::AXIS_X:
        axis->setTValues(index, m_bounds.m_min.x, m_bounds.m_max.x);
        break;
    case SeperatedAxis::AXIS_Y:
        axis->setTValues(index, m_bounds.m_min.y, m_bounds.m_max.y);
        break;
    case SeperatedAxis::AXIS_Z:
        axis->setTValues(index, m_bounds.m_min.z, m_bounds.m_max.z);
        break;
    default:
        setNonOrthogonalRangeOnAxis(axis, index);
        break;
    }
}

CollShape::~CollShape() {}

CollShape::CollShape(ShapeType type, float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    : m_localBounds(minX, minY, minZ, maxX, maxY, maxZ),
      m_bounds(minX, minY, minZ, maxX, maxY, maxZ),
      m_bounce(1.0f),
      m_inverseResistance(1.0f),
      m_material(-1),
      m_shapeType(type) {}

SeperatedAxesList* CollShape::getSepAxisList(int index) {
    SeperatedAxesList* list = s_sepAxisList[index];
    if (list == NULL) {
        list = new SeperatedAxesList();
        midp::INCREF(list);
        s_sepAxisList[index] = list;
    }
    list->reset();
    return list;
}

bool CollShape::moveShape(CollShape* shape, MathVector& position, const MathVector& move,
                          midp::array<CollShape*>& shapes, int from, int to, float& multiple, CollShape** hit,
                          MathVector& normal) {
    SeperatedAxesList* own = getSepAxisList(0);
    own->addOrthogonalAxes(shape);
    shape->addNonOrthogonalAxesTo(own, 0);
    SeperatedAxesList* other = getSepAxisList(1);
    SeperatedAxesList* hitAxes = NULL;
    SeperatedAxis* moveOutAxis = NULL;
    if (from == to) {
        position += move;
        return false;
    }
    for (int i = from; i != to; i++) {
        CollShape* s = shapes[i];
        if (own->addRangeToAxesWithCheck(s, 1) != -1) {
            continue;
        }
        other->reset();
        s->addNonOrthogonalAxesTo(other, 1);
        if (other->addRangeToAxesWithCheck(shape, 0) != -1) {
            continue;
        }
        if (hitAxes == NULL) {
            SeperatedAxesList::getMoveOutMultiple(own, other, move, 1, multiple, &moveOutAxis);
            SeperatedAxesList::storeStaticBounds(own, own);
            SeperatedAxesList::storeStaticBounds(other, other);
            hitAxes = other;
            other = getSepAxisList(2);
            *hit = s;
        } else {
            float m = 0.0f;
            SeperatedAxis* axis = NULL;
            SeperatedAxesList::getMoveOutMultiple(own, other, move, 1, m, &axis);
            if (multiple > m) {
                SeperatedAxesList::storeStaticBounds(own, own);
                SeperatedAxesList::storeStaticBounds(other, other);
                multiple = m;
                moveOutAxis = axis;
                *hit = s;
                SeperatedAxesList* swap = hitAxes;
                hitAxes = other;
                other = swap;
            }
        }
    }
    if (hitAxes == NULL) {
        position += move;
        return false;
    }
    bool combined = false;
    for (int i = from; i != to; i++) {
        CollShape* s = shapes[i];
        SeperatedAxis* touching = NULL;
        if (*hit == s) {
            continue;
        }
        if (!SeperatedAxesList::checkCombineAxes(own, NULL, NULL, s, &touching, true, move)) {
            continue;
        }
        if (!SeperatedAxesList::checkCombineAxes(hitAxes, NULL, NULL, s, &touching, false, move)) {
            continue;
        }
        other->reset();
        s->addNonOrthogonalAxesTo(other, 1);
        if (!SeperatedAxesList::checkCombineAxes(other, shape, *hit, NULL, &touching, false, move)) {
            continue;
        }
        own->combineTValues();
        hitAxes->combineTValues();
        other->combineTValues();
        hitAxes->addAxes(other);
        combined = true;
    }
    if (combined) {
        if (own->noIntersection(0, 2) || hitAxes->noIntersection(0, 2)) {
            position += move;
            return false;
        }
        SeperatedAxesList::getMoveOutMultiple(own, hitAxes, move, 2, multiple, &moveOutAxis);
    }
    MathVector step(move);
    step *= multiple;
    position += step;
    moveOutAxis->ensureNoIntersection(position, shape->getLocalBounds(), move);
    moveOutAxis->getNormal(move, normal);
    return true;
}

bool CollShape::testIntersection(CollShape* a, CollShape* b) {
    if (!(a->m_bounds.m_max.x > b->m_bounds.m_min.x && b->m_bounds.m_max.x > a->m_bounds.m_min.x &&
          a->m_bounds.m_max.y > b->m_bounds.m_min.y && b->m_bounds.m_max.y > a->m_bounds.m_min.y &&
          a->m_bounds.m_max.z > b->m_bounds.m_min.z && b->m_bounds.m_max.z > a->m_bounds.m_min.z)) {
        return false;
    }
    SeperatedAxesList* axes = getSepAxisList(0);
    a->addNonOrthogonalAxesTo(axes, 0);
    if (axes->addRangeToAxesWithCheck(b, 1) != -1) {
        return false;
    }
    axes = getSepAxisList(1);
    b->addNonOrthogonalAxesTo(axes, 1);
    return axes->addRangeToAxesWithCheck(a, 0) == -1;
}

bool CollShape::intersects(const MathVector& point) const {
    if (!(point.x > m_bounds.m_min.x && m_bounds.m_max.x > point.x && point.y > m_bounds.m_min.y &&
          m_bounds.m_max.y > point.y && point.z > m_bounds.m_min.z && m_bounds.m_max.z > point.z)) {
        return false;
    }
    SeperatedAxesList* axes = getSepAxisList(0);
    addNonOrthogonalAxesTo(axes, 0);
    int count = axes->getAxisNum();
    for (int i = 0; i != count; i++) {
        SeperatedAxis* axis = axes->getAxis(i);
        float t = axis->getDistanceOfPoint(point);
        if (axis->getMinT(0) > t) {
            return false;
        }
        if (axis->getMaxT(0) < t) {
            return false;
        }
    }
    return true;
}

bool CollShape::pointIntersects(const MathVector& point) const {
    return intersects(point);
}

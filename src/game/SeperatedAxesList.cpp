#include "game/SeperatedAxesList.h"

#include <stddef.h>

#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "runtime/midp/Math.h"

bool SeperatedAxesList::checkCombineAxes(SeperatedAxesList* list, CollShape* shape0, CollShape* shape2,
                                         CollShape* shape1, SeperatedAxis** touching, bool firstThreeOnly,
                                         const MathVector& move) {
    int count = list->getAxisNum();
    for (int i = 0; i != count; i++) {
        SeperatedAxis* axis = list->getAxis(i);
        if (shape2 != NULL) {
            shape2->setRangeOnAxis(axis, 2);
        }
        if (shape1 != NULL) {
            shape1->setRangeOnAxis(axis, 1);
        }
        float min1 = axis->getMinT(1);
        float max1 = axis->getMaxT(1);
        float min2 = axis->getMinT(2);
        float max2 = axis->getMaxT(2);
        if (GameCommon::compareFloats(min1, max2) || GameCommon::compareFloats(max1, min2)) {
            if (*touching != NULL) {
                return false;
            }
            *touching = axis;
            continue;
        }
        if (shape0 != NULL) {
            shape0->setRangeOnAxis(axis, 0);
        }
        if (firstThreeOnly && i > 2) {
            continue;
        }
        float min0 = axis->getMinT(0);
        float max0 = axis->getMaxT(0);
        float mid1 = (min1 + max1) * 0.5f;
        float mid0 = (min0 + max0) * 0.5f;
        float mid2 = (min2 + max2) * 0.5f;
        if (mid0 < mid1 && mid0 < mid2) {
            if (!GameCommon::compareFloats(min1, min2)) {
                return false;
            }
        } else if (mid1 < mid0 && mid2 < mid0) {
            if (!GameCommon::compareFloats(max1, max2)) {
                return false;
            }
        } else {
            if (min1 - 0.01f >= min0 || max1 + 0.01f <= max0) {
                return false;
            }
            if (min2 - 0.01f >= min0 || max2 + 0.01f <= max0) {
                return false;
            }
        }
    }
    return true;
}

void SeperatedAxesList::storeStaticBounds(SeperatedAxesList* from, SeperatedAxesList* to) {
    int count = from->m_count;
    for (int i = 0; i != count; i++) {
        SeperatedAxis::Range* src = from->m_axes.elementAt(i)->getTValues(1);
        SeperatedAxis::Range* dst = to->m_axes.elementAt(i)->getTValues(2);
        *dst = *src;
    }
}

int SeperatedAxesList::addRangeToAxesWithCheck(CollShape* shape, int index) {
    for (int i = 0; i != m_count; i++) {
        SeperatedAxis* axis = m_axes.elementAt(i);
        shape->setRangeOnAxis(axis, index);
        if (axis->noIntersection()) {
            return i;
        }
    }
    return -1;
}

void SeperatedAxesList::combineTValues() {
    for (int i = 0; i != m_count; i++) {
        SeperatedAxis* axis = m_axes.elementAt(i);
        float minT = midp::Math::min(axis->getMinT(1), axis->getMinT(2));
        float maxT = midp::Math::max(axis->getMaxT(1), axis->getMaxT(2));
        axis->setTValues(2, minT, maxT);
    }
}

bool SeperatedAxesList::noIntersection(int a, int b) {
    int count = m_count;
    for (int i = 0; i != count; i++) {
        if (m_axes.elementAt(i)->noIntersection(a, b)) {
            return true;
        }
    }
    return false;
}

void SeperatedAxesList::getMoveOutMultiple(SeperatedAxesList* list1, SeperatedAxesList* list2,
                                           const MathVector& move, int index, float& multiple,
                                           SeperatedAxis** axis) {
    multiple = -9999999.0f;
    int count = midp::Math::min(list1->m_count, 3);
    for (int i = 0; i != count; i++) {
        SeperatedAxis* a = list1->m_axes.elementAt(i);
        float m = a->getMoveOutMultiple(move, index);
        if (multiple <= m) {
            multiple = m;
            *axis = a;
        }
    }
    count = list2->m_count;
    for (int i = 0; i != count; i++) {
        SeperatedAxis* a = list2->m_axes.elementAt(i);
        float m = a->getMoveOutMultiple(move, index);
        if (multiple <= m) {
            multiple = m;
            *axis = a;
        }
    }
}

SeperatedAxis* SeperatedAxesList::newAxis() {
    SeperatedAxis* axis;
    if (m_count == m_axes.size()) {
        axis = new SeperatedAxis();
        m_axes.addElement(axis);
    } else {
        axis = m_axes.elementAt(m_count);
    }
    m_count++;
    return axis;
}

void SeperatedAxesList::addAxes(SeperatedAxesList* other) {
    for (int i = 0; i != other->m_count; i++) {
        SeperatedAxis* axis = newAxis();
        axis->setAxis(other->getAxis(i));
    }
}

void SeperatedAxesList::addAxis(const MathVector& direction, int index, float minT, float maxT) {
    newAxis()->setAxis(direction, index, minT, maxT);
}

void SeperatedAxesList::addOrthogonalAxes(CollShape* shape) {
    const MathOrthoBox& bounds = shape->getBounds();
    MathVector direction(1.0f, 0.0f, 0.0f);
    newAxis()->setAxis(direction, 0, bounds.m_min.x, bounds.m_max.x, SeperatedAxis::AXIS_X);
    direction.set(0.0f, 1.0f, 0.0f);
    newAxis()->setAxis(direction, 0, bounds.m_min.y, bounds.m_max.y, SeperatedAxis::AXIS_Y);
    direction.set(0.0f, 0.0f, 1.0f);
    newAxis()->setAxis(direction, 0, bounds.m_min.z, bounds.m_max.z, SeperatedAxis::AXIS_Z);
}

SeperatedAxesList::~SeperatedAxesList() {
    m_axes.~VectorSE();
}

SeperatedAxesList::SeperatedAxesList() : m_axes(), m_count(0) {}

void SeperatedAxesList::reset() {
    m_count = 0;
}

int SeperatedAxesList::getAxisNum() {
    return m_count;
}

SeperatedAxis* SeperatedAxesList::getAxis(int index) {
    return m_axes.elementAt(index);
}

#include "game/InterpolationTimed.h"

InterpolationVectorTimed::InterpolationVectorTimed() {}

InterpolationVectorTimed::~InterpolationVectorTimed() {}

void InterpolationVectorTimed::start(const MathVector& from, const MathVector& to, int duration,
                                     InterpolationType type) {
    InterpolationTimed::start(duration, type);
    m_start = from;
    m_end = to;
    m_current = from;
}

void InterpolationVectorTimed::applyProgress(float t) {
    m_current.setAsLinearInterpolation(m_start, m_end, t);
}

void InterpolationVectorTimed::applyEndValue() {
    m_current = m_end;
}

MathVector& InterpolationVectorTimed::getStartValue() {
    return m_start;
}

MathVector& InterpolationVectorTimed::getEndValue() {
    return m_end;
}

MathVector& InterpolationVectorTimed::getCurrentValue() {
    return m_current;
}

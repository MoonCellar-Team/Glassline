#include "game/InterpolationTimed.h"

InterpolationFloatTimed::InterpolationFloatTimed() : m_start(0), m_end(0), m_current(0) {}

InterpolationFloatTimed::~InterpolationFloatTimed() {}

void InterpolationFloatTimed::start(float from, float to, int duration, InterpolationType type) {
    InterpolationTimed::start(duration, type);
    m_start = from;
    m_end = to;
    m_current = from;
}

void InterpolationFloatTimed::applyProgress(float t) {
    m_current = m_start + (m_end - m_start) * t;
}

void InterpolationFloatTimed::applyEndValue() {
    m_current = m_end;
}

float InterpolationFloatTimed::getCurrentValue() {
    return m_current;
}

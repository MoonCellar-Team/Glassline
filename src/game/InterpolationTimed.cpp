#include "game/InterpolationTimed.h"

InterpolationTimed::InterpolationTimed() : m_elapsed(0), m_duration(0), m_type(LINEAR) {}

InterpolationTimed::~InterpolationTimed() {}

void InterpolationTimed::start(int duration, InterpolationType type) {
    m_elapsed = 0;
    m_duration = duration;
    m_type = type;
}

void InterpolationTimed::update(int dt) {
    m_elapsed += dt;
    if (m_elapsed >= m_duration) {
        stop();
        return;
    }
    float t = (float)m_elapsed / (float)m_duration;
    switch (m_type) {
    case LINEAR:
        applyProgress(t);
        break;
    case EASE_IN_QUAD:
        applyProgress(t * t);
        break;
    case EASE_OUT_QUAD:
        applyProgress(1.0f - (1.0f - t) * (1.0f - t));
        break;
    case EASE_IN_CUBIC:
        applyProgress(t * t * t);
        break;
    case EASE_OUT_CUBIC:
        applyProgress(1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t));
        break;
    }
}

void InterpolationTimed::stop() {
    m_elapsed = m_duration;
    applyEndValue();
}

bool InterpolationTimed::isFinished() {
    return m_elapsed == m_duration;
}

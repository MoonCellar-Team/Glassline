#include "game/UI/SliderElement.h"

#include <math.h>

namespace UI {

float SliderElement::getValue() const {
    return m_value;
}

void SliderElement::update(int dt) {
    float seconds = dt * 0.001f;
    if (m_dragging) {
        m_velocity = (m_previous - m_value) / seconds;
        m_previous = m_value;
    } else {
        m_value -= m_velocity * seconds;
        float capped = m_value < 1.0f ? m_value : 1.0f;
        m_value = 0.0f < capped ? capped : 0.0f;
    }

    float drag = (float)((double)fabsf(m_velocity) * (double)seconds * 8.0);
    if (m_velocity > 0.0f) {
        m_velocity -= drag;
        if (m_velocity <= 0.0001f) {
            m_velocity = 0.0f;
        }
    } else if (m_velocity < 0.0f) {
        m_velocity += drag;
        if (m_velocity >= -0.0001f) {
            m_velocity = 0.0f;
        }
    }
}

SliderElement::SliderElement(int width, int height)
    : WindowElement(0, 0, width, height),
      m_dragging(false),
      m_previous(0.0f),
      m_value(0.0f),
      m_velocity(0.0f) {}

void SliderElement::setValue(float value) {
    m_previous = value;
    m_value = value;
}

SliderElement::~SliderElement() {}

}

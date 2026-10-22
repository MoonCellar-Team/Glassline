#include "game/UI/ScrollBar.h"

#include <math.h>

namespace UI {

void ScrollBar::setScrolling(bool scrolling) {
    m_scrolling = scrolling;
    if (!scrolling) {
        return;
    }
    m_lastOffset = getOffset();
    m_velocity = 0.0f;
}

void ScrollBar::update(int dt) {
    float seconds = dt * 0.001f;
    int offset = getOffset();
    if (m_scrolling) {
        m_velocity = (float)(m_lastOffset - offset) / seconds;
        m_lastOffset = offset;
    } else {
        int step = (int)(m_velocity * seconds);
        setOffset(offset - step);
    }

    float drag = (float)((double)fabsf(m_velocity) * (double)seconds * 4.0);
    if (m_velocity > 0.0f) {
        m_velocity -= drag;
        if (m_velocity <= 0.001f) {
            m_velocity = 0.0f;
        }
    } else if (m_velocity < 0.0f) {
        m_velocity += drag;
        if (m_velocity >= -0.001f) {
            m_velocity = 0.0f;
        }
    }
}

ScrollBar::~ScrollBar() {}

ScrollBar::ScrollBar(Window* window)
    : m_window(window),
      m_scrolling(false),
      m_lastOffset(0),
      m_velocity(0.0f) {}

}

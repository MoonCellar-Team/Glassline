#include "game/UI/NotchedSlider.h"

#include <algorithm>
#include <math.h>
#include <stddef.h>

#include "runtime/midp/Graphics.h"

namespace UI {

static void selectNotch(NotchedSlider& slider) {
    int notch = (int)(slider.m_halfNotchWidth / (float)slider.m_notchWidth);
    slider.m_selectedNotch = std::min(notch, (int)slider.m_items.size() - 1);
    slider.m_selectedNotch = std::max(0, slider.m_selectedNotch);
    slider.m_selected = slider.m_items[slider.m_selectedNotch];
}

bool NotchedSlider::canPrev() {
    if (m_looped) {
        return true;
    }
    int notch = m_targetNotch == -1 ? m_selectedNotch : m_targetNotch;
    return notch > 0;
}

bool NotchedSlider::pointerPressed(int x, int y, int id) {
    m_unk30 = x;
    return false;
}

bool NotchedSlider::pointerReleased(int x, int y, int id) {
    m_dragging = false;
    return true;
}

void NotchedSlider::prev() {
    int notch = (m_targetNotch == -1 ? m_selectedNotch : m_targetNotch) - 1;
    m_targetNotch = notch;
    if (notch >= 0) {
        return;
    }
    if (m_looped) {
        m_targetNotch = (int)m_items.size() + notch;
        return;
    }
    m_targetNotch = 0;
}

bool NotchedSlider::canNext() {
    if (m_looped) {
        return true;
    }
    int notch = m_targetNotch == -1 ? m_selectedNotch : m_targetNotch;
    return (unsigned)notch < m_items.size() - 1;
}

bool NotchedSlider::pointerDragged(int x, int y, int id) {
    m_targetNotch = -1;
    m_dragging = true;
    m_halfNotchWidth -= (float)(x - m_unk30);
    if (!m_looped) {
        int low = m_notchWidth >> 1;
        int high = m_notchWidth * (int)m_items.size() - (m_notchWidth >> 1);
        int cur = (int)m_halfNotchWidth;
        m_halfNotchWidth = (float)std::max(low, std::min(high, cur));
    }
    m_unk4C = (m_unk4C + x) - m_unk30;
    m_unk30 = x;
    return true;
}

void NotchedSlider::next() {
    int notch = (m_targetNotch == -1 ? m_selectedNotch : m_targetNotch) + 1;
    if (m_looped) {
        m_targetNotch = (int)((unsigned)notch % m_items.size());
    } else {
        int last = (int)m_items.size() - 1;
        m_targetNotch = std::min(notch, last);
    }
}

void NotchedSlider::render(midp::Graphics* g, int originY, int originX) {
    int clipX = g->getClipX();
    int clipY = g->getClipY();
    int clipW = g->getClipWidth();
    int clipH = g->getClipHeight();
    g->setClip(m_x + originX, m_y + originY, m_width, m_height);

    int index = (int)(m_halfNotchWidth / (float)m_notchWidth) - m_renderExtra;
    if (m_looped && index < 0) {
        index += (int)m_items.size();
    }

    if (((m_renderExtra << 1) | 1) >= 1) {
        int x = originX - ((int)m_halfNotchWidth % m_notchWidth) + m_x + (m_width >> 1) -
                m_renderExtra * m_notchWidth;
        for (int i = 0; i < ((m_renderExtra << 1) | 1); ++i) {
            if (index < 0 || (unsigned)index >= m_items.size()) {
                x += m_notchWidth;
                index += 1;
            } else {
                int next = index + 1;
                m_items[index]->render(g, m_y + originY, x);
                x += m_notchWidth;
                if (m_looped) {
                    int count = (int)m_items.size();
                    index = next == count ? 0 : next;
                } else {
                    index = next;
                }
            }
        }
    }

    g->setClip(clipX, clipY, clipW, clipH);
}

void NotchedSlider::update(int dt) {
    float seconds = (float)dt * 0.001f;
    if (m_dragging) {
        m_velocity = -((float)m_unk4C / seconds);
        m_unk4C = 0;
    } else {
        bool settle = m_targetNotch == -1;
        if (!settle) {
            if (m_looped) {
                int count = (int)m_items.size();
                int width = m_notchWidth;
                int total = width * count;
                int center = width * m_targetNotch + (width >> 1);
                int delta = (int)((float)center - m_halfNotchWidth);
                int dist = delta >= 0 ? delta : total + delta;
                int halfTotal = total >> 1;
                int shorter = dist > halfTotal ? total - dist : dist;
                if (shorter < 51) {
                    m_targetNotch = -1;
                    settle = true;
                } else {
                    int directed = dist >= halfTotal ? -shorter : shorter;
                    m_velocity = (float)directed * 5.0f;
                }
            } else {
                int width = m_notchWidth;
                float center = (float)(width * m_targetNotch + (width >> 1));
                int delta = (int)(m_halfNotchWidth - center);
                int distance = delta < 0 ? -delta : delta;
                if (distance < 51) {
                    m_targetNotch = -1;
                    settle = true;
                } else if (center > m_halfNotchWidth) {
                    m_velocity = (float)distance * 5.0f;
                } else if (center < m_halfNotchWidth) {
                    m_velocity = -(float)distance * 5.0f;
                }
            }
        }
        if (settle) {
            float speed = fabsf(m_velocity);
            int half = m_notchWidth >> 1;
            int mod = (int)m_halfNotchWidth % m_notchWidth;
            if (speed >= 500.5f || (unsigned)(mod + 4 - half) > 8) {
                if (mod < half) {
                    float t = 1.0f - (float)mod / (float)half;
                    m_velocity += t * 2000.0f * seconds;
                } else if (mod > half) {
                    float t = ((float)half - (float)mod) / (float)half;
                    m_velocity += t * 2000.0f * seconds;
                }
            } else {
                m_velocity = 0.0f;
                selectNotch(*this);
                m_halfNotchWidth = (float)(m_notchWidth * m_selectedNotch + half);
            }
        }
        m_halfNotchWidth += m_velocity * seconds;
        selectNotch(*this);
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

    float span = (float)(unsigned)(m_notchWidth * (int)m_items.size());
    if (m_looped) {
        if (m_halfNotchWidth < 0.0f) {
            do {
                m_halfNotchWidth += span;
            } while (m_halfNotchWidth < 0.0f);
        }
        if (m_halfNotchWidth >= span) {
            do {
                m_halfNotchWidth -= span;
            } while (m_halfNotchWidth >= span);
        }
    } else {
        float halfPos = (float)(m_notchWidth >> 1);
        if (m_halfNotchWidth < halfPos) {
            m_halfNotchWidth = halfPos;
        } else {
            float limit = (float)(int)(span - halfPos);
            if (m_halfNotchWidth > limit) {
                m_halfNotchWidth = limit;
            }
        }
    }
}

NotchedSlider::~NotchedSlider() {
    std::vector<WindowElement*>::iterator end = m_items.end();
    for (std::vector<WindowElement*>::iterator it = m_items.begin(); it != end; ++it) {
        if (*it != NULL) {
            delete *it;
        }
    }
}

NotchedSlider::NotchedSlider()
    : m_halfNotchWidth(0.0f),
      m_velocity(0.0f),
      m_unk30(0),
      m_dragging(false),
      m_selected(NULL),
      m_selectedNotch(0),
      m_targetNotch(-1),
      m_notchWidth(0),
      m_renderExtra(0),
      m_unk4C(0),
      m_looped(true) {}

void NotchedSlider::addItem(WindowElement* item) {
    item->setX(0);
    item->setY((m_height - item->getHeight()) >> 1);
    m_items.push_back(item);
}

WindowElement* NotchedSlider::getSelectedItem() const {
    return m_selected;
}

void NotchedSlider::setNotchWidth(int width) {
    m_notchWidth = width;
    m_halfNotchWidth = (float)(width >> 1);
}

void NotchedSlider::setRenderExtra(int extra) {
    m_renderExtra = extra;
}

void NotchedSlider::setTargetNotch(int notch) {
    m_targetNotch = notch;
}

int NotchedSlider::getSelectedNotch() const {
    return m_selectedNotch;
}

bool NotchedSlider::isDragging() const {
    return m_dragging;
}

void NotchedSlider::setLooped(bool looped) {
    m_looped = looped;
}

bool NotchedSlider::isIdle() const {
    const int bits = reinterpret_cast<const int&>(m_velocity);
    return bits == 0 && !m_dragging;
}

int NotchedSlider::getItemCount() const {
    return (int)m_items.size();
}

}

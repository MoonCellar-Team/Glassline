#pragma once

#include "game/UI/WindowElement.h"

namespace UI {

class Window;

class ScrollBar : public WindowElement {
public:
    explicit ScrollBar(Window* window);
    virtual ~ScrollBar();

    virtual void update(int dt);

    virtual int getOffset() const = 0;
    virtual void setOffset(int offset) const = 0;

    void setScrolling(bool scrolling);

    Window* m_window;
    bool m_scrolling;
    int m_lastOffset;
    float m_velocity;
};

}

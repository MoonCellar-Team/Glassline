#pragma once

#include "game/UI/ScrollBar.h"

namespace UI {

class Window;

class VerticalScrollBar : public ScrollBar {
public:
    explicit VerticalScrollBar(Window* window);
    virtual ~VerticalScrollBar();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual int getOffset() const;
    virtual void setOffset(int offset) const;
};

}

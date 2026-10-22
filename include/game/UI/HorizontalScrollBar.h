#pragma once

#include "game/UI/ScrollBar.h"

namespace UI {

class Window;

class HorizontalScrollBar : public ScrollBar {
public:
    explicit HorizontalScrollBar(Window* window);
    virtual ~HorizontalScrollBar();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual int getOffset() const;
    virtual void setOffset(int offset) const;
};

}

#pragma once

#include "game/UI/Window.h"

namespace UI {

class WindowWithBackground : public Window {
public:
    WindowWithBackground(int x, int y, int width, int height);
    virtual ~WindowWithBackground();

    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
};

}

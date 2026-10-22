#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"

namespace UI {

class PopupBox : public Window {
public:
    explicit PopupBox(int text);
    virtual ~PopupBox();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerReleased(int x, int y, int id);

    BorderedElement m_frame;
    WrappedString m_text;
    int m_elapsed;
};

}
